// ===================================================================================
//  Senaryo Oluşturucu Uygulaması
// ===================================================================================
//
//  YAPILAN GÜNCELLEMELER:
//  ---------------------
//  1. GÜNCEL VERİ YAPISI KULLANIMI: Fonksiyon artık verileri doğrudan
//     SearchableDataset nesnesinin kendi içindeki vektörlere yazıyor.
//
// ===================================================================================

#include "CreateSimpleScenario.hpp"
#include "coordinatedTurn.hpp"
#include <vector>
#include <fstream>
#include <stdexcept>
#include <iomanip>

SearchableDataset createAndSaveSimpleScenario(const std::string& output_csv_filename) {

    StateVector initial_state;
    initial_state.x = 1000.0;
    initial_state.vx = -50.0;
    initial_state.y = 0.0;
    initial_state.vy = -60.0;
    initial_state.z = 300.0;
    initial_state.vz = 70.0;
    const double R = 170.0;
    initial_state.yaw_rate = -rad2deg(std::sqrt(initial_state.vx * initial_state.vx + initial_state.vy * initial_state.vy) / R);

    const double dt = 0.01;
    const double t_final = 25.0;

    SearchableDataset scenario_dataset;
    scenario_dataset.name = "Gercek Yorunge";
    scenario_dataset.is_line_series = true;

    std::vector<StateVector> states;
    states.push_back(initial_state);

    for (double t = 0; t <= t_final; t += dt) {
        const auto& current_state = states.back();
        scenario_dataset.points.push_back({ current_state.x, current_state.y, current_state.z });

        // Verileri doğrudan nesnenin kendi vektörlerine ekle
        scenario_dataset.time_data.push_back(t);
        scenario_dataset.x_data.push_back(current_state.x);
        scenario_dataset.y_data.push_back(current_state.y);
        scenario_dataset.z_data.push_back(current_state.z);

        states.push_back(coordinatedTurn(current_state, dt));
    }

    // Aranabilir alanları tanımla
    // Lambda artık nesnenin kendisini yakalayabilir (ancak burada gereksiz, çünkü veriler zaten içinde)
    scenario_dataset.fields.push_back({ "Zaman (s)", [&ds = scenario_dataset](size_t i) { return ds.time_data[i]; } });
    scenario_dataset.fields.push_back({ "X (m)",     [&ds = scenario_dataset](size_t i) { return ds.x_data[i]; } });
    scenario_dataset.fields.push_back({ "Y (m)",     [&ds = scenario_dataset](size_t i) { return ds.y_data[i]; } });
    scenario_dataset.fields.push_back({ "Z (m)",     [&ds = scenario_dataset](size_t i) { return ds.z_data[i]; } });

    // CSV Dosyasını Kaydet
    std::ofstream outfile(output_csv_filename);
    if (!outfile.is_open()) {
        throw std::runtime_error("CSV dosyasi acilamadi: " + output_csv_filename);
    }
    outfile << std::fixed << std::setprecision(8);
    outfile << "time,posX,velX,accelX,posY,velY,accelY,posZ,velZ,accelZ,someValue\n";
    for (size_t i = 0; i < states.size() - 1; ++i) {
        outfile << scenario_dataset.time_data[i] << ","
            << states[i].x << "," << states[i].vx << ",0,"
            << states[i].y << "," << states[i].vy << ",0,"
            << states[i].z << "," << states[i].vz << ",0,0\n";
    }
    outfile.close();

    return scenario_dataset;
}


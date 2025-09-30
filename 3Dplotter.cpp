#include "3Dplotter.hpp"
#include <cmath>
#include <numbers>
#include <vector>
#include <algorithm>
#include <stdexcept>

// --- Koordinat Dönüşüm ve Yardımcı Fonksiyonlar ---
namespace Coords {
    constexpr double A = 6378137.0;
    constexpr double E_SQ = 0.00669437999014;

    auto deg2rad(double d) { return d * std::numbers::pi / 180.0; };
    auto rad2deg(double r) { return r * 180.0 / std::numbers::pi; };

    DataPoint lla_to_ecef(double lat_deg, double lon_deg, double alt_m) {
        double lat_rad = deg2rad(lat_deg);
        double lon_rad = deg2rad(lon_deg);
        double N = A / std::sqrt(1.0 - E_SQ * std::sin(lat_rad) * std::sin(lat_rad));

        return {
            (N + alt_m) * std::cos(lat_rad) * std::cos(lon_rad),
            (N + alt_m) * std::cos(lat_rad) * std::sin(lon_rad),
            (N * (1.0 - E_SQ) + alt_m) * std::sin(lat_rad)
        };
    }
}

// --- Veri İşleme Fonksiyonları ---

SearchableDataset process_truth_data(const SearchableDataset& raw_data, const DataPoint& origin_lla, const DataPoint& radar_pos_ecef) {
    SearchableDataset processed = raw_data;
    processed.has_rbe_capability = true;
    processed.fields.clear();
    processed.fields.push_back({ "Zaman (s)",  [&ds = processed](size_t i) { return ds.time_data[i]; } });
    processed.points.clear();
    processed.range_data.clear();
    processed.bearing_data.clear();
    processed.elevation_data.clear();

    double lat_rad = Coords::deg2rad(origin_lla.x);
    double lon_rad = Coords::deg2rad(origin_lla.y);
    const DataPoint origin_ecef = Coords::lla_to_ecef(origin_lla.x, origin_lla.y, origin_lla.z);

    for (size_t i = 0; i < raw_data.x_data.size(); ++i) {
        double n = raw_data.x_data[i];
        double e = raw_data.y_data[i];
        double d = -raw_data.z_data[i];

        double clat = cos(lat_rad), slat = sin(lat_rad);
        double clon = cos(lon_rad), slon = sin(lon_rad);

        DataPoint ecef_offset = {
            -slat * clon * n - slon * e - clat * clon * d,
            -slat * slon * n + clon * e - clat * slon * d,
             clat * n - slat * d
        };

        DataPoint target_absolute = { origin_ecef.x + ecef_offset.x, origin_ecef.y + ecef_offset.y, origin_ecef.z + ecef_offset.z };

        processed.x_data[i] = target_absolute.x;
        processed.y_data[i] = target_absolute.y;
        processed.z_data[i] = target_absolute.z;

        processed.points.push_back({
            target_absolute.x - origin_ecef.x,
            target_absolute.y - origin_ecef.y,
            target_absolute.z - origin_ecef.z
            });

        double dx = target_absolute.x - radar_pos_ecef.x;
        double dy = target_absolute.y - radar_pos_ecef.y;
        double dz = target_absolute.z - radar_pos_ecef.z;
        double range = std::sqrt(dx * dx + dy * dy + dz * dz);
        processed.range_data.push_back(range);
        processed.bearing_data.push_back(Coords::rad2deg(std::atan2(dy, dx)));
        processed.elevation_data.push_back(Coords::rad2deg(std::asin(dz / range)));
    }
    return processed;
}

SearchableDataset process_radar_data(const MuaseretScenarioOutput& muaseret_input, const DataPoint& origin_lla, const DataPoint& radar_pos_ecef) {
    SearchableDataset processed;
    processed.name = "Radar Olcumleri";
    processed.is_line_series = false;
    processed.has_rbe_capability = true;
    processed.fields.push_back({ "Zaman (s)",  [&ds = processed](size_t i) { return ds.time_data[i]; } });

    const DataPoint origin_ecef = Coords::lla_to_ecef(origin_lla.x, origin_lla.y, origin_lla.z);

    for (size_t i = 0; i < muaseret_input.fusionAlgoMainFile.measurements.size(); ++i) {
        const auto& radar_meas = muaseret_input.fusionAlgoMainFile.measurements[i].trackRadarMeas;
        if (std::isnan(radar_meas.range)) continue;

        processed.time_data.push_back(muaseret_input.fusionAlgoMainFile.systemTime[i]);
        processed.range_data.push_back(radar_meas.range);
        processed.bearing_data.push_back(radar_meas.bearing);
        processed.elevation_data.push_back(radar_meas.elevation);

        double r = radar_meas.range;
        double b_rad = Coords::deg2rad(radar_meas.bearing);
        double e_rad = Coords::deg2rad(radar_meas.elevation);

        double e_local = r * cos(e_rad) * sin(b_rad);
        double n_local = r * cos(e_rad) * cos(b_rad);
        double u_local = r * sin(e_rad);
        double d_local = -u_local;

        double lat_rad = Coords::deg2rad(origin_lla.x);
        double lon_rad = Coords::deg2rad(origin_lla.y);
        double clat = cos(lat_rad), slat = sin(lat_rad);
        double clon = cos(lon_rad), slon = sin(lon_rad);

        DataPoint ecef_offset = {
            -slat * clon * n_local - slon * e_local - clat * clon * d_local,
            -slat * slon * n_local + clon * e_local - clat * slon * d_local,
             clat * n_local - slat * d_local
        };

        DataPoint target_absolute = { radar_pos_ecef.x + ecef_offset.x, radar_pos_ecef.y + ecef_offset.y, radar_pos_ecef.z + ecef_offset.z };

        processed.x_data.push_back(target_absolute.x);
        processed.y_data.push_back(target_absolute.y);
        processed.z_data.push_back(target_absolute.z);

        processed.points.push_back({
            target_absolute.x - origin_ecef.x,
            target_absolute.y - origin_ecef.y,
            target_absolute.z - origin_ecef.z
            });
    }
    return processed;
}

// --- Ana Fonksiyon ---
void plot3D(const SearchableDataset& gercek_yorunge_data, const MuaseretScenarioOutput& muaseret_input) {
    if (gercek_yorunge_data.x_data.empty()) {
        launchInteractivePlotter({}, { 0,0,0 });
        return;
    }

    std::vector<SearchableDataset> datasets_to_plot;

    const DataPoint radar_pos_lla = { 39.90, 32.80, 1000 };
    const DataPoint origin_lla = radar_pos_lla;
    const DataPoint radar_pos_ecef = Coords::lla_to_ecef(radar_pos_lla.x, radar_pos_lla.y, radar_pos_lla.z);
    const DataPoint origin_ecef = Coords::lla_to_ecef(origin_lla.x, origin_lla.y, origin_lla.z);

    // Radar'ın çizim için orijine göreli pozisyonunu hesapla
    const DataPoint radar_pos_relative = {
        radar_pos_ecef.x - origin_ecef.x,
        radar_pos_ecef.y - origin_ecef.y,
        radar_pos_ecef.z - origin_ecef.z
    };


    SearchableDataset truth_processed = process_truth_data(gercek_yorunge_data, origin_lla, radar_pos_ecef);
    datasets_to_plot.push_back(truth_processed);

    SearchableDataset radar_processed = process_radar_data(muaseret_input, origin_lla, radar_pos_ecef);
    if (!radar_processed.points.empty()) {
        datasets_to_plot.push_back(radar_processed);
    }

    launchInteractivePlotter(datasets_to_plot, radar_pos_relative);
}


#pragma once

#include <string>
#include <vector>
#include <functional>

// 3D uzaydaki bir noktayı temsil eder (hem LLA hem ECEF için kullanılabilir).
struct DataPoint {
    double x, y, z;
};

// Veri setleri içinde aranabilir bir alanı tanımlar.
struct DataField {
    std::string name;
    std::function<double(size_t)> getter;
};

// Grafikte çizdirilecek ve içinde arama yapılabilecek bir veri setini temsil eder.
struct SearchableDataset {
    std::string name;

    // Orijine göreli çizim noktaları (OpenGL koordinat sistemine uygun)
    std::vector<DataPoint> points;
    bool is_line_series = false;

    // Tooltip ve arama için ham veriler
    bool has_rbe_capability = false;
    std::vector<DataField> fields; // Geri eklendi
    std::vector<double> time_data;

    // Mutlak ECEF koordinatları
    std::vector<double> x_data;
    std::vector<double> y_data;
    std::vector<double> z_data;

    // Menzil, Yanca, Yükseliş verileri (varsa)
    std::vector<double> range_data;
    std::vector<double> bearing_data;
    std::vector<double> elevation_data;
};


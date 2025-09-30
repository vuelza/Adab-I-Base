// ===================================================================================
//  Adab-I-Sim: Muaseret Senaryo Veri Yapıları
// ===================================================================================
//
//  Bu dosya, MATLAB'dan C++'a aktarılan ve füzyon algoritması girdilerini
//  temsil eden tüm iç içe geçmiş veri yapılarını içerir.
//
//  YAPILAN DÜZELTMELER:
//  ---------------------
//  1. Tekrarlanan yapı tanımları kaldırıldı.
//  2. Yapılar, derleyicinin doğru şekilde okuyabilmesi için mantıksal
//     bir sıraya (bağımlılıklarına göre) dizildi.
//  3. C-tarzı NAN makrosu yerine modern C++ standardı olan
//     std::numeric_limits<double>::quiet_NaN() kullanıldı.
//  4. Gerekli başlık dosyaları (<cmath>, <limits>) eklendi.
//
// ===================================================================================

#ifndef MUASERET_DATA_STRUCTURES_HPP
#define MUASERET_DATA_STRUCTURES_HPP

#pragma once // Alternatif header guard
#include "TypesCommon.hpp"
#include <vector>
#include <array>
#include <cmath>   // std::isnan gibi fonksiyonlar için
#include <limits>  // quiet_NaN için

// Kolaylık sağlaması açısından bir sabit tanımlayalım
constexpr double NaN = std::numeric_limits<double>::quiet_NaN();

// --- Bireysel Sensör ve Durum Veri Yapıları ---

struct TrackRadarMeas {
    double bearing{ NaN };
    double elevation{ NaN };
    double range{ NaN };
    double Doppler{ NaN };
    double measurementTime{ NaN };
    TypesCommon::Status sensorStatus = TypesCommon::Status::ENUM_STATUS_NA;
    double radarCrossSection{ NaN };
    double SNR{ NaN };
};

struct SearchRadarMeas {
    std::array<double, 6> radarMeasECEF{ NaN, NaN, NaN, NaN, NaN, NaN };
    double measurementTime{ NaN };
    TypesCommon::Status sensorStatus = TypesCommon::Status::ENUM_STATUS_NA;
    double SNR{ NaN };
};

struct ThermalCameraMeas {
    double bearing{ NaN };
    double elevation{ NaN };
    double measurementTime{ NaN };
    TypesCommon::Status sensorStatus = TypesCommon::Status::ENUM_STATUS_NA;
};

struct TVCameraMeas {
    double bearing{ NaN };
    double elevation{ NaN };
    double measurementTime{ NaN };
    TypesCommon::Status sensorStatus = TypesCommon::Status::ENUM_STATUS_NA;
    std::array<double, 2> trackSize{ NaN, NaN };
};

struct LaserMeas {
    std::array<double, 10> rangeArray{ NaN, NaN, NaN, NaN, NaN, NaN, NaN, NaN, NaN, NaN };
    double measurementTime{ NaN };
    TypesCommon::Status sensorStatus = TypesCommon::Status::ENUM_STATUS_NA;
};

struct SensorPlatformServoMeas {
    double sensorPlatformBearing{ NaN };
    double sensorPlatformElevation{ NaN };
    double measurementTime{ NaN };
};

struct TurretServoMeas {
    double turretBearing{ NaN };
    double measurementTime{ NaN };
};

struct VehiclePosition {
    double latitude{ NaN };
    double longtitude{ NaN };
    double altitude{ NaN };
    double measurementTime{ NaN };
};

struct VehicleOrientation {
    double yaw{ NaN };
    double pitch{ NaN };
    double roll{ NaN };
    double measurementTime{ NaN };
};

struct VehicleVelocity {
    std::array<double, 3> velocity{ NaN, NaN, NaN };
    double measurementTime{ NaN };
};

// --- Birleşik Veri Yapıları ---

// Tek bir zaman adımındaki tüm ölçümleri bir araya getiren yapı
struct Measurement {
    TrackRadarMeas trackRadarMeas;
    SearchRadarMeas searchRadarMeas;
    ThermalCameraMeas thermalCameraMeas;
    TVCameraMeas tvCameraMeas;
    LaserMeas laserMeas;
    SensorPlatformServoMeas sensorPlatformServoMeas;
    TurretServoMeas turretServoMeas;
    VehiclePosition vehiclePosition;
    VehicleOrientation vehicleOrientation;
    VehicleVelocity vehicleVelocity;
};

// Global parametreleri tutan yapı
struct SetConfigParams {
    // Bu yapı MATLAB kodundaki gibi çok detaylı, şimdilik boş bırakılabilir
    // veya ihtiyaç duyulan alanlar eklenebilir.
};

// Füzyon algoritmasının ana girdi yapısı
struct FusionAlgoMainFile {
    std::vector<Measurement> measurements;
    std::vector<double> systemTime;
    std::vector<int> resetAlgorithm;
    std::vector<int> forcedCoast;
    std::vector<int> enableServoDrive;
    std::vector<int> enableVideoTrack;
};

// Tüm senaryo çıktısını kapsayan en üst seviye yapı
struct MuaseretScenarioOutput {
    SetConfigParams setConfigParams;
    FusionAlgoMainFile fusionAlgoMainFile;
};

#endif // MUASERET_DATA_STRUCTURES_HPP


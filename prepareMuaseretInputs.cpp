#include "prepareMuaseretInputs.hpp"
#include "ScenarioDataPoint.hpp" // Bu dosya CSV satırını temsil eder
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <cmath>
#include <numbers>

// --- Bu dosya için özel yardımcı fonksiyonlar ve yapılar ---
namespace { // Anonim namespace

    struct ScenarioConfigParams {
        double sensorRangeErrSigma = 2.0;
        double sensorBearingErrSigma = 0.02;
        double sensorElevationErrSigma = 0.02;
        std::vector<int> sensorMeasPeriod = { 1, 20, 100 };
        double sensorDelay = 0.0;
        bool sensorHataKapat = false; // Normalde hatalar açık olsun
    };

    std::vector<ScenarioDataPoint> loadScenarioFromCSV(const std::string& filename) {
        std::vector<ScenarioDataPoint> data;
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("CSV dosyasi okunamadi: " + filename);
        }
        std::string line;
        std::getline(file, line); // Başlık satırını atla
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string cell;
            ScenarioDataPoint point;
            std::getline(ss, cell, ','); point.time = std::stod(cell);
            std::getline(ss, cell, ','); point.posX = std::stod(cell);
            std::getline(ss, cell, ','); point.velX = std::stod(cell);
            std::getline(ss, cell, ','); point.accX = std::stod(cell);
            std::getline(ss, cell, ','); point.posY = std::stod(cell);
            std::getline(ss, cell, ','); point.velY = std::stod(cell);
            std::getline(ss, cell, ','); point.accY = std::stod(cell);
            std::getline(ss, cell, ','); point.posZ = std::stod(cell);
            std::getline(ss, cell, ','); point.velZ = std::stod(cell);
            std::getline(ss, cell, ','); point.accZ = std::stod(cell);
            data.push_back(point);
        }
        return data;
    }

    double rad2deg_local(double radians) {
        return radians * 180.0 / std::numbers::pi;
    }

    std::vector<std::vector<double>> senaryodanSensorVerisiUret(const std::vector<ScenarioDataPoint>& scenario, ScenarioConfigParams config, int period) {
        std::vector<std::vector<double>> sensorVerisi;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<> d(0, 1);

        if (config.sensorHataKapat) {
            config.sensorRangeErrSigma = 0;
            config.sensorBearingErrSigma = 0;
            config.sensorElevationErrSigma = 0;
        }

        for (size_t i = 0; i < scenario.size(); ++i) {
            if (i % period == 0) {
                const auto& row = scenario[i];
                double range = std::sqrt(row.posX * row.posX + row.posY * row.posY + row.posZ * row.posZ) + d(gen) * config.sensorRangeErrSigma;
                double bearing = rad2deg_local(std::atan2(row.posY, row.posX));
                double elevation = rad2deg_local(std::atan2(row.posZ, std::sqrt(row.posX * row.posX + row.posY * row.posY)));

                double time = row.time + config.sensorDelay;
                time = round(time * 1000000) / 1000000;

                sensorVerisi.push_back({ time, range, bearing + d(gen) * config.sensorBearingErrSigma, elevation + d(gen) * config.sensorElevationErrSigma });
            }
            else {
                sensorVerisi.push_back({ NAN, NAN, NAN, NAN });
            }
        }
        return sensorVerisi;
    }

    void createServoData(const std::vector<ScenarioDataPoint>& scenario, std::vector<std::vector<double>>& sensorServo, std::vector<std::vector<double>>& turretServo) {
        sensorServo.clear();
        turretServo.clear();
        for (const auto& row : scenario) {
            double pitch = rad2deg_local(std::atan2(row.posZ, std::sqrt(row.posX * row.posX + row.posY * row.posY)));
            double yaw = rad2deg_local(std::atan2(row.posY, row.posX));
            double time = round(row.time * 1000000) / 1000000;
            sensorServo.push_back({ time, yaw, pitch });
            turretServo.push_back({ time, 0.0 });
        }
    }
}

MuaseretScenarioOutput prepareMuaseretInputs(const std::string& recordName) {
    MuaseretScenarioOutput output;
    ScenarioConfigParams config;

    auto scenario = loadScenarioFromCSV(recordName);
    if (scenario.empty()) {
        throw std::runtime_error("Senaryo dosyasi bos veya okunamadi.");
    }

    auto sensorDataTrackRadar = senaryodanSensorVerisiUret(scenario, config, config.sensorMeasPeriod[0]);
    auto sensorDataCamera = senaryodanSensorVerisiUret(scenario, config, config.sensorMeasPeriod[1]);
    auto sensorDataLaser = senaryodanSensorVerisiUret(scenario, config, config.sensorMeasPeriod[2]);

    std::vector<std::vector<double>> sensorPlatformServoData, turretPlatformServoData;
    createServoData(scenario, sensorPlatformServoData, turretPlatformServoData);

    for (size_t i = 0; i < scenario.size(); ++i) {
        Measurement meas;
        meas.trackRadarMeas = { sensorDataTrackRadar[i][2], sensorDataTrackRadar[i][3], sensorDataTrackRadar[i][1], NAN, sensorDataTrackRadar[i][0], TypesCommon::ENUM_STATUS_TRACK };
        meas.thermalCameraMeas = { sensorDataCamera[i][2], sensorDataCamera[i][3], sensorDataCamera[i][0], TypesCommon::ENUM_STATUS_TRACK };
        meas.laserMeas.rangeArray[0] = sensorDataLaser[i][1];
        meas.laserMeas.measurementTime = sensorDataLaser[i][0];
        meas.laserMeas.sensorStatus = TypesCommon::ENUM_STATUS_TRACK;
        meas.sensorPlatformServoMeas = { sensorPlatformServoData[i][1], sensorPlatformServoData[i][2], sensorPlatformServoData[i][0] };
        meas.turretServoMeas = { turretPlatformServoData[i][1], turretPlatformServoData[i][0] };
        meas.vehiclePosition = { 37.61, 33.43, 1042.47, scenario[i].time };
        meas.vehicleOrientation = { 0.01, 0.005, 0, scenario[i].time };

        output.fusionAlgoMainFile.measurements.push_back(meas);
        output.fusionAlgoMainFile.systemTime.push_back(scenario[i].time);
    }

    std::cout << "Muaseret girdileri basariyla hazirlandi." << std::endl;
    return output;
}

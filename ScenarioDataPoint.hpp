#pragma once

// CSV dosyasındaki bir satırı temsil eden yapı.
struct ScenarioDataPoint {
    double time = 0.0;
    double posX = 0.0, velX = 0.0, accX = 0.0;
    double posY = 0.0, velY = 0.0, accY = 0.0;
    double posZ = 0.0, velZ = 0.0, accZ = 0.0;
};


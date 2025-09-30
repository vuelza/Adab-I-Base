#include "coordinatedTurn.hpp"
#include <cmath>
#include <numbers>

double deg2rad(double degrees) {
    return degrees * std::numbers::pi / 180.0;
}

double rad2deg(double radians) {
    return radians * 180.0 / std::numbers::pi;
}

namespace { // Anonim namespace
    inline double sind(double degrees) {
        return std::sin(deg2rad(degrees));
    }
    inline double cosd(double degrees) {
        return std::cos(deg2rad(degrees));
    }
}

StateVector coordinatedTurn(const StateVector& x, double dt) {
    StateVector x_o = x;
    if (std::abs(x.yaw_rate) < 1e-12) {
        x_o.x = x.x + x.vx * dt;
        x_o.y = x.y + x.vy * dt;
        x_o.z = x.z + x.vz * dt;
    }
    else {
        const double horizontal_speed = std::sqrt(x.vx * x.vx + x.vy * x.vy);
        const double yaw_rate_rad = deg2rad(x.yaw_rate);
        const double R = horizontal_speed / yaw_rate_rad;
        const double angleVi = rad2deg(std::atan2(x.vy, x.vx));
        const double total_angle_change_deg = x.yaw_rate * dt;
        x_o.x = R * sind(total_angle_change_deg + angleVi) + x.x - R * sind(angleVi);
        x_o.y = -R * cosd(total_angle_change_deg + angleVi) + x.y + R * cosd(angleVi);
        x_o.z = x.z + x.vz * dt;
        const double cos_angle = cosd(total_angle_change_deg);
        const double sin_angle = sind(total_angle_change_deg);
        x_o.vx = cos_angle * x.vx - sin_angle * x.vy;
        x_o.vy = sin_angle * x.vx + cos_angle * x.vy;
    }
    return x_o;
}

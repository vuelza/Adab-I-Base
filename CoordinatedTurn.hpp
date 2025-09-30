#pragma once

// Durum vektörünü temsil eden yapı
struct StateVector {
    double x{ 0.0 }, y{ 0.0 }, z{ 0.0 };      // Pozisyon (m)
    double vx{ 0.0 }, vy{ 0.0 }, vz{ 0.0 };   // Hız (m/s)
    double yaw_rate{ 0.0 };                  // Dönüş Oranı (derece/s)
};

// Dereceyi radyana çevirir
double deg2rad(double degrees);
// Radyanı dereceye çevirir
double rad2deg(double radians);

// Bir durum vektörünü, koordine dönüş modelini kullanarak günceller.
StateVector coordinatedTurn(const StateVector& x, double dt);

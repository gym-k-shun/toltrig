#pragma once

#include "constants.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace toltrig::detail {

struct reduced_angle { double value; double sign; };
struct quadrant_reduced_angle { double value; int quadrant; };

enum class reduction_backend {
  nearbyint_fast,
  remainder_safe,
  cody_waite_medium,
  payne_hanek_large
};

constexpr double pi_hi = 3.14159262180328369140625;
constexpr double pi_mid = 3.178650954705639668e-08;
constexpr double pi_lo = 1.224646799147353207e-16;
constexpr double half_pi_hi = 1.570796310901641845703125;
constexpr double half_pi_mid = 1.589325477352819834e-08;
constexpr double half_pi_lo = 6.123233995736766036e-17;

inline int quadrant_mod4(std::int64_t q) {
  int k = static_cast<int>(q % 4);
  return k < 0 ? k + 4 : k;
}

inline reduced_angle invalid_angle() {
  return {std::numeric_limits<double>::quiet_NaN(), 1.0};
}
inline quadrant_reduced_angle invalid_quadrant_angle() {
  return {std::numeric_limits<double>::quiet_NaN(), 0};
}
inline reduced_angle reduce_pi_switch_floor(double x) {
  if (!std::isfinite(x)) return invalid_angle();
  const double q = std::floor(x / pi);
  return {x - q * pi, std::fmod(std::fabs(q), 2.0) >= 1.0 ? -1.0 : 1.0};
}
inline reduced_angle reduce_pi_switch_integer(double x) {
  if (!std::isfinite(x)) return invalid_angle();
  const double q = std::floor(x / pi);
  if (q < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
      q > static_cast<double>(std::numeric_limits<std::int64_t>::max()))
    return reduce_pi_switch_floor(x);
  return {x - q * pi, static_cast<std::int64_t>(q) % 2 != 0 ? -1.0 : 1.0};
}
inline reduced_angle reduce_half_pi_remainder(double x) {
  if (!std::isfinite(x)) return invalid_angle();
  double v = std::remainder(x, two_pi);
  double sign = 1.0;
  if (v > half_pi) { v = pi - v; sign = -1.0; }
  else if (v < -half_pi) { v = -pi - v; sign = -1.0; }
  return {v, sign};
}
// Fast bounded-input candidate. This is not a production-grade huge argument
// reducer. Callers must select a documented application-specific input bound.
inline reduced_angle reduce_nearbyint_pi(double x) {
  if (!std::isfinite(x)) return invalid_angle();
  const double q = std::nearbyint(x / pi);
  if (q < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
      q > static_cast<double>(std::numeric_limits<std::int64_t>::max()))
    return invalid_angle();
  return {x - q * pi, static_cast<std::int64_t>(q) % 2 != 0 ? -1.0 : 1.0};
}

inline reduced_angle reduce_remainder_pi(double x) {
  if (!std::isfinite(x)) return invalid_angle();
  const double r = std::remainder(x, pi);
  const double q = std::nearbyint((x - r) / pi);
  if (q < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
      q > static_cast<double>(std::numeric_limits<std::int64_t>::max()))
    return invalid_angle();
  return {r, static_cast<std::int64_t>(q) % 2 != 0 ? -1.0 : 1.0};
}

inline reduced_angle reduce_cody_waite_pi(double x) {
  if (!std::isfinite(x)) return invalid_angle();
  const double q = std::nearbyint(x / pi);
  if (q < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
      q > static_cast<double>(std::numeric_limits<std::int64_t>::max()))
    return invalid_angle();
  const double r = ((x - q * pi_hi) - q * pi_mid) - q * pi_lo;
  return {r, static_cast<std::int64_t>(q) % 2 != 0 ? -1.0 : 1.0};
}

inline quadrant_reduced_angle reduce_nearbyint_half_pi(double x) {
  if (!std::isfinite(x)) return invalid_quadrant_angle();
  const double q = std::nearbyint(x / half_pi);
  if (q < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
      q > static_cast<double>(std::numeric_limits<std::int64_t>::max()))
    return invalid_quadrant_angle();
  const auto qi = static_cast<std::int64_t>(q);
  return {x - q * half_pi, quadrant_mod4(qi)};
}

inline quadrant_reduced_angle reduce_cody_waite_half_pi(double x) {
  if (!std::isfinite(x)) return invalid_quadrant_angle();
  const double q = std::nearbyint(x / half_pi);
  if (q < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
      q > static_cast<double>(std::numeric_limits<std::int64_t>::max()))
    return invalid_quadrant_angle();
  const auto qi = static_cast<std::int64_t>(q);
  const double r = ((x - q * half_pi_hi) - q * half_pi_mid) - q * half_pi_lo;
  return {r, quadrant_mod4(qi)};
}

inline quadrant_reduced_angle reduce_remainder_half_pi(double x) {
  if (!std::isfinite(x)) return invalid_quadrant_angle();
  const double r = std::remainder(x, half_pi);
  const double q = std::nearbyint((x - r) / half_pi);
  if (q < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
      q > static_cast<double>(std::numeric_limits<std::int64_t>::max()))
    return invalid_quadrant_angle();
  return {r, quadrant_mod4(static_cast<std::int64_t>(q))};
}

}  // namespace toltrig::detail

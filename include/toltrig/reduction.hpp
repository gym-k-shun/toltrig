#pragma once

#include "constants.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace toltrig::detail {

struct reduced_angle { double value; double sign; };

inline reduced_angle invalid_angle() {
  return {std::numeric_limits<double>::quiet_NaN(), 1.0};
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

}  // namespace toltrig::detail

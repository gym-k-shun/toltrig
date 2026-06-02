#pragma once

#include "constants.hpp"
#include "reduction.hpp"

#include <cmath>
#include <limits>

namespace toltrig::experimental {

// Continuous low-cost candidate. The coefficients were searched under the
// constraint P(pi / 4) == 1. This makes both value and first derivative
// continuous when the reciprocal branch is selected at pi / 4.
constexpr double tan_pade_b = -0.06860571525033407;
constexpr double tan_pade_d = -0.40178219629842254;

inline double reduce_tan_nearbyint_pi(double x) {
  if (!std::isfinite(x)) return std::numeric_limits<double>::quiet_NaN();
  return x - std::nearbyint(x / pi) * pi;
}

inline double tan_reduced_pade(double x) {
  const double x2 = x * x;
  return x * (1.0 + tan_pade_b * x2) / (1.0 + tan_pade_d * x2);
}

// Bounded-input experiment only. This is not a production-grade huge
// argument reducer and assumes the normal floating-point rounding mode.
inline double tan_bounded_pade(double x) {
  if (x == 0.0) return x;
  const double r = reduce_tan_nearbyint_pi(x);
  const double ar = std::fabs(r);
  if (ar <= quarter_pi) return tan_reduced_pade(r);
  const double u = half_pi - ar;
  if (u == 0.0) {
    return std::copysign(std::numeric_limits<double>::infinity(), r);
  }
  return std::copysign(1.0, r) / tan_reduced_pade(u);
}

namespace detail {

inline double tan_reduced_pade_current(double x) {
  const double x2 = x * x;
  return x * (15.0 - x2) / (15.0 - 6.0 * x2);
}
inline double tan_bounded_pade_current(double x) {
  const double r = reduce_tan_nearbyint_pi(x);
  const double ar = std::fabs(r);
  if (ar <= quarter_pi) return tan_reduced_pade_current(r);
  return std::copysign(1.0, r) /
         tan_reduced_pade_current(half_pi - ar);
}

// Higher-order comparison candidate. It is not the default until additional
// cross-platform measurements justify its extra operations.
constexpr double tan_pade4_b = 0.788791664734832;
constexpr double tan_pade4_c = -0.06121759926807135;
constexpr double tan_pade4_d = 0.45541796800896994;
constexpr double tan_pade4_e = -0.34604850453981323;
inline double tan_reduced_pade4(double x) {
  const double x2 = x * x, x4 = x2 * x2;
  return x * (1.0 + tan_pade4_b * x2 + tan_pade4_c * x4) /
         (1.0 + tan_pade4_d * x2 + tan_pade4_e * x4);
}
inline double tan_bounded_pade4(double x) {
  const double r = reduce_tan_nearbyint_pi(x);
  const double ar = std::fabs(r);
  if (ar <= quarter_pi) return tan_reduced_pade4(r);
  return std::copysign(1.0, r) / tan_reduced_pade4(half_pi - ar);
}

}  // namespace detail
}  // namespace toltrig::experimental

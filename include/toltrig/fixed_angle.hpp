#pragma once

#include "constants.hpp"
#include "cos.hpp"
#include "sin.hpp"
#include "tan.hpp"

#include <cstdint>
#include <limits>

namespace toltrig::experimental {
namespace fixed {

// Experimental binary-angle API.
//
// Mapping:
//   angle == 0        -> 0 rad
//   angle == 2^32     -> 2*pi rad, represented by uint32_t wraparound to 0
//   angle >> 30       -> quadrant [0, 3]
//   angle & (2^30-1)  -> position inside that quadrant
//
// Only the quadrant-local coordinate is converted back to double. That keeps
// the reducer integer-only and lets the existing reduced polynomial kernels do
// the approximation work.
namespace fixed_detail {

constexpr std::uint32_t quadrant_mask = (UINT32_C(1) << 30) - UINT32_C(1);
constexpr double quadrant_scale = half_pi / 1073741824.0;  // half_pi / 2^30

inline double quadrant_offset(std::uint32_t angle) {
  return static_cast<double>(angle & quadrant_mask) * quadrant_scale;
}

inline double tan_first_quadrant(double x) {
  if (x <= quarter_pi) return tan_reduced_pade(x);
  return 1.0 / tan_reduced_pade(half_pi - x);
}

}  // namespace fixed_detail

inline double sin_u32(std::uint32_t angle) {
  const std::uint32_t quadrant = angle >> 30;
  const double x = fixed_detail::quadrant_offset(angle);
  switch (quadrant) {
    case 0: return sin_reduced_n8(x);
    case 1: return cos_reduced_n8(x);
    case 2: return -sin_reduced_n8(x);
    default: return -cos_reduced_n8(x);
  }
}

inline double cos_u32(std::uint32_t angle) {
  const std::uint32_t quadrant = angle >> 30;
  const double x = fixed_detail::quadrant_offset(angle);
  switch (quadrant) {
    case 0: return cos_reduced_n8(x);
    case 1: return -sin_reduced_n8(x);
    case 2: return -cos_reduced_n8(x);
    default: return sin_reduced_n8(x);
  }
}

inline double tan_u32(std::uint32_t angle) {
  const std::uint32_t quadrant = angle >> 30;
  const double x = fixed_detail::quadrant_offset(angle);
  if (quadrant == 0 || quadrant == 2) return fixed_detail::tan_first_quadrant(x);
  if (x == 0.0) return -std::numeric_limits<double>::infinity();
  if (x <= quarter_pi) return -1.0 / tan_reduced_pade(x);
  return -tan_reduced_pade(half_pi - x);
}

}  // namespace fixed
}  // namespace toltrig::experimental

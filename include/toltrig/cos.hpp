#pragma once

#include "polynomial.hpp"
#include "reduction.hpp"

namespace toltrig {

enum class precision_policy { fast, balanced, accurate };
enum class reduction_policy { reduced_input, bounded_nearbyint, general_remainder };

inline double cos_reduced_n7(double x) { return detail::cos_taylor_n7_horner_reduced(x); }
inline double cos_reduced_n8(double x) { return detail::cos_taylor_n8_horner_reduced(x); }
inline double cos_bounded_n7(double x) {
  const auto r = detail::reduce_nearbyint_pi(x);
  return r.sign * cos_reduced_n7(r.value);
}
inline double cos_bounded_n8(double x) {
  const auto r = detail::reduce_nearbyint_pi(x);
  return r.sign * cos_reduced_n8(r.value);
}
inline double cos_remainder_n7(double x) {
  const auto r = detail::reduce_half_pi_remainder(x);
  return r.sign * cos_reduced_n7(r.value);
}
inline double cos_remainder_n8(double x) {
  const auto r = detail::reduce_half_pi_remainder(x);
  return r.sign * cos_reduced_n8(r.value);
}
inline double cos(double x, precision_policy precision = precision_policy::balanced,
                  reduction_policy reduction = reduction_policy::bounded_nearbyint) {
  const bool n8 = precision == precision_policy::accurate;
  switch (reduction) {
    case reduction_policy::reduced_input: return n8 ? cos_reduced_n8(x) : cos_reduced_n7(x);
    case reduction_policy::bounded_nearbyint: return n8 ? cos_bounded_n8(x) : cos_bounded_n7(x);
    case reduction_policy::general_remainder: return n8 ? cos_remainder_n8(x) : cos_remainder_n7(x);
  }
  return cos_bounded_n7(x);
}

}  // namespace toltrig

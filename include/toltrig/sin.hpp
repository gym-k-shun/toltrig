#pragma once

#include "cos.hpp"
#include "reduction.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace toltrig::experimental {
namespace detail {

constexpr std::array<double, 8> sin_taylor_n7 = {
    1.0, -1.0 / 6.0, 1.0 / 120.0, -1.0 / 5040.0,
    1.0 / 362880.0, -1.0 / 39916800.0, 1.0 / 6227020800.0,
    -1.0 / 1307674368000.0};
constexpr std::array<double, 9> sin_taylor_n8 = {
    1.0, -1.0 / 6.0, 1.0 / 120.0, -1.0 / 5040.0,
    1.0 / 362880.0, -1.0 / 39916800.0, 1.0 / 6227020800.0,
    -1.0 / 1307674368000.0, 1.0 / 355687428096000.0};

template <std::size_t N>
inline double sin_horner(double x, const std::array<double, N>& c) {
  return x * toltrig::detail::evaluate_horner(x * x, c);
}
inline double sin_reduced_n7_estrin(double x) {
  const double y=x*x,y2=y*y,y4=y2*y2;
  return x*((sin_taylor_n7[0]+sin_taylor_n7[1]*y+
             (sin_taylor_n7[2]+sin_taylor_n7[3]*y)*y2)+
            (sin_taylor_n7[4]+sin_taylor_n7[5]*y+
             (sin_taylor_n7[6]+sin_taylor_n7[7]*y)*y2)*y4);
}
inline double sin_reduced_n8_estrin(double x) {
  const double y=x*x,y2=y*y,y4=y2*y2,y8=y4*y4;
  return sin_reduced_n7_estrin(x)+x*sin_taylor_n8[8]*y8;
}

}  // namespace detail

inline double sin_from_cos_n7(double x) { return cos_bounded_n7(half_pi-x); }
inline double sin_from_cos_n8(double x) { return cos_bounded_n8(half_pi-x); }
inline double sin_reduced_n7(double x) { return detail::sin_horner(x,detail::sin_taylor_n7); }
inline double sin_reduced_n8(double x) { return detail::sin_horner(x,detail::sin_taylor_n8); }
inline double sin_bounded_n7(double x) {
  if(x==0.0)return x;
  const auto r=toltrig::detail::reduce_nearbyint_pi(x);
  return r.sign*sin_reduced_n7(r.value);
}
inline double sin_bounded_n8(double x) {
  if(x==0.0)return x;
  const auto r=toltrig::detail::reduce_nearbyint_pi(x);
  return r.sign*sin_reduced_n8(r.value);
}

}  // namespace toltrig::experimental

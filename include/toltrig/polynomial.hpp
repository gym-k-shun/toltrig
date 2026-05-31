#pragma once

#include <array>
#include <cstddef>

namespace toltrig::detail {

constexpr std::array<double, 8> taylor_n7 = {
    1.0, -1.0 / 2.0, 1.0 / 24.0, -1.0 / 720.0, 1.0 / 40320.0,
    -1.0 / 3628800.0, 1.0 / 479001600.0, -1.0 / 87178291200.0};
constexpr std::array<double, 9> taylor_n8 = {
    1.0, -1.0 / 2.0, 1.0 / 24.0, -1.0 / 720.0, 1.0 / 40320.0,
    -1.0 / 3628800.0, 1.0 / 479001600.0, -1.0 / 87178291200.0,
    1.0 / 20922789888000.0};

template <std::size_t N>
inline double evaluate_horner(double y, const std::array<double, N>& c) {
  double result = c[N - 1];
  for (std::size_t i = N - 1; i-- > 0;) result = result * y + c[i];
  return result;
}

inline double cos_taylor_n7_horner_reduced(double x) {
  return evaluate_horner(x * x, taylor_n7);
}
inline double cos_taylor_n8_horner_reduced(double x) {
  return evaluate_horner(x * x, taylor_n8);
}
inline double cos_taylor_n7_estrin_reduced(double x) {
  const double y = x * x, y2 = y * y, y4 = y2 * y2;
  return (taylor_n7[0] + taylor_n7[1] * y +
          (taylor_n7[2] + taylor_n7[3] * y) * y2) +
         (taylor_n7[4] + taylor_n7[5] * y +
          (taylor_n7[6] + taylor_n7[7] * y) * y2) * y4;
}
inline double cos_taylor_n8_estrin_reduced(double x) {
  const double y = x * x, y2 = y * y, y4 = y2 * y2, y8 = y4 * y4;
  return cos_taylor_n7_estrin_reduced(x) + taylor_n8[8] * y8;
}

}  // namespace toltrig::detail

#include <toltrig/toltrig.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
int failures = 0;

void check(bool ok, const char* message) {
  if (!ok) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

double radians_from_u32(std::uint32_t angle) {
  return static_cast<double>(angle) * (toltrig::two_pi / 4294967296.0);
}

template <class F, class Ref>
double grid_error(F f, Ref ref) {
  double worst = 0.0;
  for (std::uint32_t i = 0; i < 20000; ++i) {
    const std::uint32_t angle = i * UINT32_C(214748);
    worst = std::max(worst, std::fabs(f(angle) - ref(radians_from_u32(angle))));
  }
  return worst;
}

double tan_grid_error() {
  double worst = 0.0;
  for (std::uint32_t i = 0; i < 20000; ++i) {
    const std::uint32_t angle = i * UINT32_C(214748);
    const double x = radians_from_u32(angle);
    if (std::fabs(std::cos(x)) < 1e-6) continue;
    worst = std::max(
        worst,
        std::fabs(toltrig::experimental::fixed::tan_u32(angle) - std::tan(x)));
  }
  return worst;
}
}  // namespace

int main() {
  using namespace toltrig::experimental::fixed;

  check(sin_u32(0) == 0.0, "sin zero");
  check(cos_u32(0) == 1.0, "cos zero");
  check(tan_u32(0) == 0.0, "tan zero");

  check(std::fabs(sin_u32(UINT32_C(1) << 30) - 1.0) < 1e-15, "sin pi/2");
  check(std::fabs(cos_u32(UINT32_C(1) << 30)) < 1e-15, "cos pi/2");
  check(std::isinf(tan_u32(UINT32_C(1) << 30)), "tan pi/2 pole");

  check(std::fabs(sin_u32(UINT32_C(2) << 30)) < 1e-15, "sin pi");
  check(std::fabs(cos_u32(UINT32_C(2) << 30) + 1.0) < 1e-15, "cos pi");
  check(std::fabs(tan_u32(UINT32_C(2) << 30)) < 1e-15, "tan pi");

  check(std::fabs(grid_error(sin_u32, static_cast<double (*)(double)>(std::sin))) <
            2e-12,
        "sin grid");
  check(std::fabs(grid_error(cos_u32, static_cast<double (*)(double)>(std::cos))) <
            2e-12,
        "cos grid");
  check(tan_grid_error() < 1e-4, "tan grid away from poles");

  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}

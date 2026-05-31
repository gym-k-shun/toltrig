#include <toltrig/toltrig.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

int failures = 0;
void check(bool ok, const char* msg) { if (!ok) { std::cerr << "FAIL: " << msg << '\n'; ++failures; } }
template <class F> double grid_error(double lo, double hi, int n, F fn) {
  double worst = 0;
  for (int i = 0; i <= n; ++i) { const double x = lo + (hi - lo) * i / n; worst = std::max(worst, std::fabs(fn(x) - std::cos(x))); }
  return worst;
}
int main() {
  using namespace toltrig;
  for (double x : {0.0, pi/6, pi/4, pi/3, half_pi, pi, two_pi, -half_pi, -0.0})
    check(std::fabs(cos_bounded_n8(x) - std::cos(x)) < 1e-11, "special value");
  for (double x : {half_pi, -half_pi}) {
    check(std::fabs(cos_remainder_n8(std::nextafter(x, -INFINITY)) - std::cos(std::nextafter(x, -INFINITY))) < 1e-11, "fold left");
    check(std::fabs(cos_remainder_n8(std::nextafter(x, INFINITY)) - std::cos(std::nextafter(x, INFINITY))) < 1e-11, "fold right");
  }
  check(grid_error(-half_pi, half_pi, 10000, cos_reduced_n7) < 1e-10, "reduced n7 grid");
  check(grid_error(-half_pi, half_pi, 10000, cos_reduced_n8) < 1e-12, "reduced n8 grid");
  check(grid_error(-pi, pi, 10000, cos_bounded_n8) < 1e-12, "bounded pi grid");
  check(grid_error(-100, 100, 10000, cos_bounded_n8) < 1e-11, "bounded 100 grid");
  check(grid_error(-1e6, 1e6, 200000, cos_bounded_n8) < 2e-10, "bounded 1e6 grid");
  check(grid_error(-100, 100, 10000, cos_remainder_n8) < 1e-11, "remainder 100 grid");
  check(std::isnan(cos_bounded_n8(std::numeric_limits<double>::quiet_NaN())), "nan");
  check(std::isnan(cos_bounded_n8(std::numeric_limits<double>::infinity())), "infinity");
  check(cos_bounded_n8(-0.0) == 1.0, "negative zero");
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#include <toltrig/toltrig.hpp>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace {
double radians_from_u32(std::uint32_t angle) {
  return static_cast<double>(angle) * (toltrig::two_pi / 4294967296.0);
}

void emit(std::ofstream& csv, const char* category, std::uint32_t angle) {
  using namespace toltrig::experimental::fixed;
  const double x = radians_from_u32(angle);
  csv << category << ',' << angle << ',' << x << ',' << std::sin(x) << ','
      << sin_u32(angle) << ',' << std::fabs(sin_u32(angle) - std::sin(x))
      << ',' << std::cos(x) << ',' << cos_u32(angle) << ','
      << std::fabs(cos_u32(angle) - std::cos(x)) << ',' << std::tan(x) << ','
      << tan_u32(angle) << ',' << std::fabs(tan_u32(angle) - std::tan(x))
      << '\n';
}
}  // namespace

int main() {
  std::ofstream csv("fixed-angle-diagnostic.csv");
  csv << std::setprecision(17)
      << "category,angle_u32,radians,std_sin,fixed_sin,sin_abs_error,std_cos,"
         "fixed_cos,cos_abs_error,std_tan,fixed_tan,tan_abs_error\n";

  constexpr std::uint32_t q = UINT32_C(1) << 30;
  for (auto angle : {UINT32_C(0), q / 6, q / 4, q / 2, q, q + q / 4,
                     UINT32_C(2) * q, UINT32_C(3) * q,
                     UINT32_MAX}) {
    emit(csv, "special", angle);
  }

  for (auto delta : {UINT32_C(1), UINT32_C(2), UINT32_C(16), UINT32_C(256),
                     UINT32_C(65536)}) {
    emit(csv, "near_pi_over_2_left", q - delta);
    emit(csv, "near_pi_over_2_right", q + delta);
    emit(csv, "near_3pi_over_2_left", UINT32_C(3) * q - delta);
    emit(csv, "near_3pi_over_2_right", UINT32_C(3) * q + delta);
  }

  std::cout << "CSV written to fixed-angle-diagnostic.csv\n";
}

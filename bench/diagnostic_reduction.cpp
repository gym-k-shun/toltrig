#include "bench_common.hpp"

#include <functional>
#include <iostream>
#include <iterator>

namespace {

constexpr long double ld_pi =
    3.141592653589793238462643383279502884L;
constexpr long double ld_half_pi = ld_pi / 2.0L;
constexpr long double ld_two_pi = 2.0L * ld_pi;

struct sample_result {
  double value = 0.0;
  int quadrant = -1;
};

struct reduction_stats {
  double max_error = 0.0;
  long double mean_error = 0.0;
  long double rmse = 0.0;
  std::uint64_t quadrant_failures = 0;
  std::uint64_t failures = 0;
  double tan_pole_risk = 0.0;
};

using reducer = sample_result (*)(double);

sample_result nearbyint_pi(double x) {
  auto r = toltrig::detail::reduce_nearbyint_pi(x);
  return {r.value, r.sign < 0.0 ? 1 : 0};
}
sample_result nearbyint_half_pi(double x) {
  auto r = toltrig::detail::reduce_nearbyint_half_pi(x);
  return {r.value, r.quadrant};
}
sample_result std_remainder_two_pi(double x) {
  return {std::remainder(x, toltrig::two_pi), -1};
}
sample_result std_remainder_pi(double x) {
  return {std::remainder(x, toltrig::pi), -1};
}
sample_result floor_pi(double x) {
  const double q = std::floor(x / toltrig::pi);
  return {x - q * toltrig::pi, static_cast<long long>(q) & 1 ? 1 : 0};
}
sample_result current_half_pi_fold(double x) {
  auto r = toltrig::detail::reduce_half_pi_remainder(x);
  return {r.sign * r.value, -1};
}
sample_result cody_waite_pi(double x) {
  auto r = toltrig::detail::reduce_cody_waite_pi(x);
  return {r.value, r.sign < 0.0 ? 1 : 0};
}
sample_result cody_waite_half_pi(double x) {
  auto r = toltrig::detail::reduce_cody_waite_half_pi(x);
  return {r.value, r.quadrant};
}
sample_result payne_hanek_placeholder(double) {
  return {std::numeric_limits<double>::quiet_NaN(), -1};
}

long double reference_value(const char* model, double x) {
  const long double xl = static_cast<long double>(x);
  if (std::string(model) == "std_remainder_two_pi") return std::remainderl(xl, ld_two_pi);
  if (std::string(model) == "floor_pi") {
    const long double q = std::floor(xl / ld_pi);
    return xl - q * ld_pi;
  }
  if (std::string(model) == "nearbyint_half_pi" ||
      std::string(model) == "cody_waite_half_pi") {
    const long double q = std::nearbyintl(xl / ld_half_pi);
    return xl - q * ld_half_pi;
  }
  if (std::string(model) == "current_half_pi_fold") {
    long double v = std::remainderl(xl, ld_two_pi);
    long double sign = 1.0L;
    if (v > ld_half_pi) { v = ld_pi - v; sign = -1.0L; }
    else if (v < -ld_half_pi) { v = -ld_pi - v; sign = -1.0L; }
    return sign * v;
  }
  return std::remainderl(xl, ld_pi);
}

int reference_quadrant(const char* model, double x) {
  const long double xl = static_cast<long double>(x);
  long double q = 0.0L;
  if (std::string(model) == "nearbyint_half_pi" ||
      std::string(model) == "cody_waite_half_pi") {
    q = std::nearbyintl(xl / ld_half_pi);
    long long qi = static_cast<long long>(std::fmod(q, 4.0L));
    return static_cast<int>(qi < 0 ? qi + 4 : qi);
  }
  if (std::string(model) == "nearbyint_pi" ||
      std::string(model) == "cody_waite_pi") {
    q = std::nearbyintl(xl / ld_pi);
    return static_cast<long long>(q) & 1 ? 1 : 0;
  }
  if (std::string(model) == "floor_pi") {
    q = std::floor(xl / ld_pi);
    return static_cast<long long>(q) & 1 ? 1 : 0;
  }
  return -1;
}

long double reference_pole_distance(double x) {
  return std::fabsl(std::remainderl(static_cast<long double>(x) - ld_half_pi, ld_pi));
}

long double model_pole_distance(const char* model, sample_result r) {
  const long double ar = std::fabsl(static_cast<long double>(r.value));
  if (std::string(model) == "nearbyint_half_pi" ||
      std::string(model) == "cody_waite_half_pi") {
    return (r.quadrant & 1) ? ar : std::fabsl(ld_half_pi - ar);
  }
  return std::fabsl(ld_half_pi - ar);
}

reduction_stats analyze(const std::vector<double>& v, const char* name, reducer f) {
  reduction_stats s;
  long double ss = 0.0L;
  for (double x : v) {
    const auto got = f(x);
    if (!std::isfinite(got.value)) {
      ++s.failures;
      continue;
    }
    const long double ref = reference_value(name, x);
    const double e = static_cast<double>(std::fabsl(static_cast<long double>(got.value) - ref));
    s.max_error = std::max(s.max_error, e);
    s.mean_error += e;
    ss += static_cast<long double>(e) * e;
    const int qref = reference_quadrant(name, x);
    if (qref >= 0 && got.quadrant != qref) ++s.quadrant_failures;
    const long double dref = reference_pole_distance(x);
    const long double dgot = model_pole_distance(name, got);
    const double risk = static_cast<double>(
        std::fabsl(dgot - dref) / std::max(dref, 1.0e-18L));
    s.tan_pole_risk = std::max(s.tan_pole_risk, risk);
  }
  if (!v.empty()) {
    s.mean_error /= v.size();
    s.rmse = std::sqrt(ss / v.size());
  }
  return s;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    auto o = bench::parse(argc, argv);
    std::ofstream c(o.csv);
    c << std::setprecision(17)
      << "platform,compiler,flags,benchmark_type,range,model,samples,warmup,"
         "trials,median_ms,ns_per_sample,max_reduction_error,"
         "mean_reduction_error,rmse,quadrant_failures,failures,"
         "tan_pole_risk_score,reference_note\n";
    const bench::range ranges[] = {
        {"[-pi,pi]", -toltrig::pi, toltrig::pi},
        {"[-100,100]", -100, 100},
        {"[-1e6,1e6]", -1e6, 1e6},
        {"[-1e9,1e9]", -1e9, 1e9},
        {"[-1e12,1e12]", -1e12, 1e12},
        {"[-1e16,1e16]", -1e16, 1e16},
    };
    const std::pair<const char*, reducer> reducers[] = {
        {"nearbyint_pi", nearbyint_pi},
        {"nearbyint_half_pi", nearbyint_half_pi},
        {"std_remainder_two_pi", std_remainder_two_pi},
        {"std_remainder_pi", std_remainder_pi},
        {"floor_pi", floor_pi},
        {"current_half_pi_fold", current_half_pi_fold},
        {"cody_waite_pi", cody_waite_pi},
        {"cody_waite_half_pi", cody_waite_half_pi},
        {"payne_hanek_placeholder", payne_hanek_placeholder},
    };
    for (std::size_t i = 0; i < std::size(ranges); ++i) {
      auto v = bench::inputs(ranges[i], o.samples, o.seed + i);
      for (const auto& m : reducers) {
        const double ms = bench::median_ms(v, [f = m.second](double x) {
          const auto r = f(x);
          return r.value + static_cast<double>(r.quadrant) * 0.0;
        }, o);
        const auto s = analyze(v, m.first, m.second);
        c << '"' << bench::platform() << "\",\"" << o.compiler << "\",\""
          << o.flags << "\",\"reduction_only\",\"" << ranges[i].name
          << "\",\"" << m.first << "\"," << o.samples << ',' << o.warmup
          << ',' << o.trials << ',' << ms << ',' << ms * 1e6 / o.samples
          << ',' << s.max_error << ',' << s.mean_error << ',' << s.rmse
          << ',' << s.quadrant_failures << ',' << s.failures << ','
          << s.tan_pole_risk
          << ",\"long double remainder/quotient reference; not exact for all huge inputs\"\n";
      }
    }
    std::cout << "CSV written to " << o.csv << '\n';
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}

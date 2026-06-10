#include <toltrig/toltrig.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

volatile double sink = 0.0;

struct options {
  std::size_t samples = 500000;
  int warmup = 2;
  int trials = 7;
  std::uint64_t seed = 20260610;
  std::string csv = "fixed-angle-results.csv";
  std::string compiler = "record-manually";
  std::string flags = "record-manually";
};

struct metrics {
  double max_abs = 0.0;
  long double mean_abs = 0.0;
  long double rmse = 0.0;
  std::uint64_t max_ulp = 0;
};

const char* platform() {
#if defined(_WIN32)
  return "Windows";
#elif defined(__APPLE__)
  return "macOS";
#elif defined(__linux__)
  return "Linux";
#else
  return "Unknown";
#endif
}

options parse(int argc, char** argv) {
  options o;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto value = [&] {
      if (++i >= argc) throw std::runtime_error("missing value");
      return std::string(argv[i]);
    };
    if (a == "--quick") {
      o.samples = 20000;
      o.warmup = 1;
      o.trials = 3;
    } else if (a == "--samples") {
      o.samples = std::stoull(value());
    } else if (a == "--warmup") {
      o.warmup = std::stoi(value());
    } else if (a == "--trials") {
      o.trials = std::stoi(value());
    } else if (a == "--csv") {
      o.csv = value();
    } else if (a == "--compiler-label") {
      o.compiler = value();
    } else if (a == "--flags-label") {
      o.flags = value();
    } else {
      throw std::runtime_error("unknown option: " + a);
    }
  }
  return o;
}

double radians_from_u32(std::uint32_t angle) {
  return static_cast<double>(angle) * (toltrig::two_pi / 4294967296.0);
}

std::vector<std::uint32_t> make_angles(std::size_t n, std::uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<std::uint32_t> dist;
  std::vector<std::uint32_t> v(n);
  for (auto& x : v) x = dist(rng);
  return v;
}

std::vector<double> make_radians(const std::vector<std::uint32_t>& angles) {
  std::vector<double> v(angles.size());
  for (std::size_t i = 0; i < angles.size(); ++i) {
    v[i] = radians_from_u32(angles[i]);
  }
  return v;
}

template <class T, class F>
double median_ms(const std::vector<T>& v, F f, const options& o) {
  auto run = [&] {
    double sum = 0.0;
    for (T x : v) sum += f(x);
    sink = sum;
  };
  for (int i = 0; i < o.warmup; ++i) run();
  std::vector<double> durations;
  for (int i = 0; i < o.trials; ++i) {
    const auto begin = std::chrono::steady_clock::now();
    run();
    const auto end = std::chrono::steady_clock::now();
    durations.push_back(std::chrono::duration<double, std::milli>(end - begin).count());
  }
  std::sort(durations.begin(), durations.end());
  return durations[durations.size() / 2];
}

std::uint64_t bits(double x) {
  std::uint64_t u;
  std::memcpy(&u, &x, sizeof u);
  constexpr auto sign = UINT64_C(1) << 63;
  return (u & sign) ? ~u : u | sign;
}

std::uint64_t ulp(double a, double b) {
  if (a == b) return 0;
  if (!std::isfinite(a) || !std::isfinite(b)) {
    return std::numeric_limits<std::uint64_t>::max();
  }
  const auto x = bits(a);
  const auto y = bits(b);
  return x > y ? x - y : y - x;
}

template <class F, class Ref>
metrics error(const std::vector<std::uint32_t>& angles, F f, Ref ref) {
  metrics m;
  long double sum_sq = 0.0;
  for (auto angle : angles) {
    const double expected = ref(radians_from_u32(angle));
    const double actual = f(angle);
    const double e = std::fabs(actual - expected);
    m.max_abs = std::max(m.max_abs, e);
    m.mean_abs += e;
    sum_sq += e * e;
    m.max_ulp = std::max(m.max_ulp, ulp(actual, expected));
  }
  m.mean_abs /= angles.size();
  m.rmse = std::sqrt(sum_sq / angles.size());
  return m;
}

void header(std::ofstream& csv) {
  csv << "platform,compiler,flags,function,model,samples,warmup,trials,median_ms,"
         "ns_per_sample,speed_ratio_vs_std,max_abs_error,mean_abs_error,rmse,"
         "max_ulp_error\n";
  csv << std::setprecision(17);
}

void row(std::ofstream& csv, const options& o, const char* function,
         const char* model, double ms, double base, metrics m = {}) {
  csv << '"' << platform() << "\",\"" << o.compiler << "\",\"" << o.flags
      << "\",\"" << function << "\",\"" << model << "\"," << o.samples << ','
      << o.warmup << ',' << o.trials << ',' << ms << ','
      << ms * 1e6 / o.samples << ',' << (base ? base / ms : 0.0) << ','
      << m.max_abs << ',' << m.mean_abs << ',' << m.rmse << ','
      << m.max_ulp << '\n';
}

template <class Fixed, class Bounded, class Std>
void emit_function(std::ofstream& csv, const options& o, const char* name,
                   const std::vector<std::uint32_t>& angles,
                   const std::vector<double>& radians, Fixed fixed,
                   Bounded bounded, Std std_fn) {
  const double std_ms = median_ms(radians, std_fn, o);
  row(csv, o, name, "std", std_ms, std_ms);

  const double bounded_ms = median_ms(radians, bounded, o);
  row(csv, o, name, "bounded_double_n8", bounded_ms, std_ms,
      error(angles, [&](std::uint32_t a) { return bounded(radians_from_u32(a)); },
            std_fn));

  const double fixed_ms = median_ms(angles, fixed, o);
  row(csv, o, name, "fixed_u32", fixed_ms, std_ms, error(angles, fixed, std_fn));
}

}  // namespace

int main(int argc, char** argv) {
  try {
    const options o = parse(argc, argv);
    const auto angles = make_angles(o.samples, o.seed);
    const auto radians = make_radians(angles);

    std::ofstream csv(o.csv);
    header(csv);

    using namespace toltrig::experimental;
    using namespace toltrig::experimental::fixed;
    emit_function(csv, o, "sin", angles, radians, sin_u32,
                  [](double x) { return sin_bounded_n8(x); },
                  static_cast<double (*)(double)>(std::sin));
    emit_function(csv, o, "cos", angles, radians, cos_u32,
                  [](double x) { return toltrig::cos_bounded_n8(x); },
                  static_cast<double (*)(double)>(std::cos));
    emit_function(csv, o, "tan", angles, radians, tan_u32,
                  [](double x) { return tan_bounded_pade(x); },
                  static_cast<double (*)(double)>(std::tan));

    std::cout << "CSV written to " << o.csv << '\n';
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}

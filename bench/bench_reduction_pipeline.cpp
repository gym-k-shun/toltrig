#include "bench_common.hpp"

#include <functional>
#include <iostream>
#include <iterator>

namespace {

struct metrics {
  double max_abs = 0.0;
  double max_rel = 0.0;
  long double mean_abs = 0.0;
  long double rmse = 0.0;
  std::uint64_t max_ulp = 0;
  std::uint64_t failures = 0;
};

using fn = double (*)(double);

template <class F, class Ref>
metrics error(const std::vector<double>& v, F f, Ref ref_fn) {
  metrics m;
  long double ss = 0.0L;
  for (double x : v) {
    const double got = f(x);
    const double ref = ref_fn(x);
    if (!std::isfinite(got) || !std::isfinite(ref)) {
      if (!(std::isinf(got) && std::isinf(ref) &&
            std::signbit(got) == std::signbit(ref))) {
        ++m.failures;
      }
      continue;
    }
    const double e = std::fabs(got - ref);
    m.max_abs = std::max(m.max_abs, e);
    if (ref != 0.0) m.max_rel = std::max(m.max_rel, e / std::fabs(ref));
    m.mean_abs += e;
    ss += static_cast<long double>(e) * e;
    m.max_ulp = std::max(m.max_ulp, bench::ulp(got, ref));
  }
  if (!v.empty()) {
    m.mean_abs /= v.size();
    m.rmse = std::sqrt(ss / v.size());
  }
  return m;
}

template <int N>
double cos_pi_reduce(double x, toltrig::detail::reduced_angle (*reduce)(double)) {
  const auto r = reduce(x);
  return r.sign * (N == 8 ? toltrig::cos_reduced_n8(r.value)
                          : toltrig::cos_reduced_n7(r.value));
}

template <int N>
double sin_pi_reduce(double x, toltrig::detail::reduced_angle (*reduce)(double)) {
  if (x == 0.0) return x;
  const auto r = reduce(x);
  return r.sign * (N == 8 ? toltrig::experimental::sin_reduced_n8(r.value)
                          : toltrig::experimental::sin_reduced_n7(r.value));
}

double tan_pi_reduce(double x, toltrig::detail::reduced_angle (*reduce)(double)) {
  if (x == 0.0) return x;
  const auto r = reduce(x);
  const double ar = std::fabs(r.value);
  if (ar <= toltrig::quarter_pi) return toltrig::experimental::tan_reduced_pade(r.value);
  const double u = toltrig::half_pi - ar;
  if (u == 0.0) {
    return std::copysign(std::numeric_limits<double>::infinity(), r.value);
  }
  return std::copysign(1.0, r.value) / toltrig::experimental::tan_reduced_pade(u);
}

template <int N>
double cos_quadrant_reduce(
    double x, toltrig::detail::quadrant_reduced_angle (*reduce)(double)) {
  const auto r = reduce(x);
  const double c = N == 8 ? toltrig::cos_reduced_n8(r.value)
                          : toltrig::cos_reduced_n7(r.value);
  const double s = N == 8 ? toltrig::experimental::sin_reduced_n8(r.value)
                          : toltrig::experimental::sin_reduced_n7(r.value);
  switch (r.quadrant) {
    case 0: return c;
    case 1: return -s;
    case 2: return -c;
    case 3: return s;
  }
  return std::numeric_limits<double>::quiet_NaN();
}

template <int N>
double sin_quadrant_reduce(
    double x, toltrig::detail::quadrant_reduced_angle (*reduce)(double)) {
  if (x == 0.0) return x;
  const auto r = reduce(x);
  const double s = N == 8 ? toltrig::experimental::sin_reduced_n8(r.value)
                          : toltrig::experimental::sin_reduced_n7(r.value);
  const double c = N == 8 ? toltrig::cos_reduced_n8(r.value)
                          : toltrig::cos_reduced_n7(r.value);
  switch (r.quadrant) {
    case 0: return s;
    case 1: return c;
    case 2: return -s;
    case 3: return -c;
  }
  return std::numeric_limits<double>::quiet_NaN();
}

double tan_quadrant_reduce(
    double x, toltrig::detail::quadrant_reduced_angle (*reduce)(double)) {
  if (x == 0.0) return x;
  const auto r = reduce(x);
  const double t = toltrig::experimental::tan_reduced_pade(r.value);
  if ((r.quadrant & 1) == 0) return t;
  if (t == 0.0) {
    return std::copysign(std::numeric_limits<double>::infinity(), -r.value);
  }
  return -1.0 / t;
}

std::vector<double> pole_inputs(bench::range r, std::size_t n) {
  std::vector<double> v;
  v.reserve(n);
  const long long k0 = static_cast<long long>(std::ceil((r.lo - toltrig::half_pi) / toltrig::pi));
  const long long k1 = static_cast<long long>(std::floor((r.hi - toltrig::half_pi) / toltrig::pi));
  const double deltas[] = {1e-3, 1e-6, 1e-9, 1e-12};
  for (long long k = k0; k <= k1 && v.size() < n; ++k) {
    const double pole = toltrig::half_pi + static_cast<double>(k) * toltrig::pi;
    for (double d : deltas) {
      if (pole - d >= r.lo && pole - d <= r.hi && v.size() < n) v.push_back(pole - d);
      if (pole + d >= r.lo && pole + d <= r.hi && v.size() < n) v.push_back(pole + d);
    }
  }
  if (v.empty()) v.push_back(std::nextafter(toltrig::half_pi, 0.0));
  return v;
}

void row(std::ofstream& c, const bench::options& o, const char* type,
         bench::range r, const char* model, double ms, double base, metrics m) {
  c << '"' << bench::platform() << "\",\"" << o.compiler << "\",\""
    << o.flags << "\",\"" << type << "\",\"" << r.name << "\",\""
    << model << "\"," << o.samples << ',' << o.warmup << ',' << o.trials
    << ',' << ms << ',' << ms * 1e6 / o.samples << ','
    << (ms ? base / ms : 0.0) << ',' << m.max_abs << ',' << m.mean_abs
    << ',' << m.rmse << ',' << m.max_rel << ',' << m.max_ulp << ','
    << m.failures << '\n';
}

template <class F, class Ref>
void emit(std::ofstream& c, const bench::options& o, const char* type,
          bench::range r, const std::vector<double>& v, const char* name,
          double base, F f, Ref ref_fn) {
  const double ms = bench::median_ms(v, f, o);
  row(c, o, type, r, name, ms, base, error(v, f, ref_fn));
}

}  // namespace

int main(int argc, char** argv) {
  try {
    auto o = bench::parse(argc, argv);
    std::ofstream c(o.csv);
    c << std::setprecision(17)
      << "platform,compiler,flags,benchmark_type,range,model,samples,warmup,"
         "trials,median_ms,ns_per_sample,speed_ratio_vs_std,max_abs_error,"
         "mean_abs_error,rmse,max_relative_error,max_ulp_error,failure_count\n";
    const bench::range ranges[] = {
        {"[-pi,pi]", -toltrig::pi, toltrig::pi},
        {"[-100,100]", -100, 100},
        {"[-1e6,1e6]", -1e6, 1e6},
        {"[-1e9,1e9]", -1e9, 1e9},
        {"[-1e12,1e12]", -1e12, 1e12},
    };
    for (std::size_t i = 0; i < std::size(ranges); ++i) {
      auto v = bench::inputs(ranges[i], o.samples, o.seed + i);
      const double bcos = bench::median_ms(v, [](double x) { return std::cos(x); }, o);
      const double bsin = bench::median_ms(v, [](double x) { return std::sin(x); }, o);
      const double btan = bench::median_ms(v, [](double x) { return std::tan(x); }, o);
      emit(c, o, "cos", ranges[i], v, "cos_n7_nearbyint_pi", bcos,
           [](double x) { return cos_pi_reduce<7>(x, toltrig::detail::reduce_nearbyint_pi); },
           [](double x) { return std::cos(x); });
      emit(c, o, "cos", ranges[i], v, "cos_n8_nearbyint_pi", bcos,
           [](double x) { return cos_pi_reduce<8>(x, toltrig::detail::reduce_nearbyint_pi); },
           [](double x) { return std::cos(x); });
      emit(c, o, "cos", ranges[i], v, "cos_n8_remainder_pi", bcos,
           [](double x) { return cos_pi_reduce<8>(x, toltrig::detail::reduce_remainder_pi); },
           [](double x) { return std::cos(x); });
      emit(c, o, "cos", ranges[i], v, "cos_n8_cody_waite_pi", bcos,
           [](double x) { return cos_pi_reduce<8>(x, toltrig::detail::reduce_cody_waite_pi); },
           [](double x) { return std::cos(x); });
      emit(c, o, "cos", ranges[i], v, "cos_n8_cody_waite_quadrant", bcos,
           [](double x) { return cos_quadrant_reduce<8>(x, toltrig::detail::reduce_cody_waite_half_pi); },
           [](double x) { return std::cos(x); });

      emit(c, o, "sin", ranges[i], v, "sin_n7_nearbyint_pi", bsin,
           [](double x) { return sin_pi_reduce<7>(x, toltrig::detail::reduce_nearbyint_pi); },
           [](double x) { return std::sin(x); });
      emit(c, o, "sin", ranges[i], v, "sin_n8_nearbyint_pi", bsin,
           [](double x) { return sin_pi_reduce<8>(x, toltrig::detail::reduce_nearbyint_pi); },
           [](double x) { return std::sin(x); });
      emit(c, o, "sin", ranges[i], v, "sin_n8_remainder_pi", bsin,
           [](double x) { return sin_pi_reduce<8>(x, toltrig::detail::reduce_remainder_pi); },
           [](double x) { return std::sin(x); });
      emit(c, o, "sin", ranges[i], v, "sin_n8_cody_waite_pi", bsin,
           [](double x) { return sin_pi_reduce<8>(x, toltrig::detail::reduce_cody_waite_pi); },
           [](double x) { return std::sin(x); });
      emit(c, o, "sin", ranges[i], v, "sin_n8_cody_waite_quadrant", bsin,
           [](double x) { return sin_quadrant_reduce<8>(x, toltrig::detail::reduce_cody_waite_half_pi); },
           [](double x) { return std::sin(x); });

      emit(c, o, "tan", ranges[i], v, "tan_pade_nearbyint_pi", btan,
           [](double x) { return tan_pi_reduce(x, toltrig::detail::reduce_nearbyint_pi); },
           [](double x) { return std::tan(x); });
      emit(c, o, "tan", ranges[i], v, "tan_pade_remainder_pi", btan,
           [](double x) { return tan_pi_reduce(x, toltrig::detail::reduce_remainder_pi); },
           [](double x) { return std::tan(x); });
      emit(c, o, "tan", ranges[i], v, "tan_pade_cody_waite_pi", btan,
           [](double x) { return tan_pi_reduce(x, toltrig::detail::reduce_cody_waite_pi); },
           [](double x) { return std::tan(x); });
      emit(c, o, "tan", ranges[i], v, "tan_pade_cody_waite_quadrant", btan,
           [](double x) { return tan_quadrant_reduce(x, toltrig::detail::reduce_cody_waite_half_pi); },
           [](double x) { return std::tan(x); });

      auto pv = pole_inputs(ranges[i], std::min<std::size_t>(o.samples, 4096));
      const auto saved_samples = o.samples;
      o.samples = pv.size();
      const double pbase = bench::median_ms(pv, [](double x) { return std::tan(x); }, o);
      emit(c, o, "tan_pole_neighborhood", ranges[i], pv, "tan_pade_nearbyint_pi", pbase,
           [](double x) { return tan_pi_reduce(x, toltrig::detail::reduce_nearbyint_pi); },
           [](double x) { return std::tan(x); });
      emit(c, o, "tan_pole_neighborhood", ranges[i], pv, "tan_pade_cody_waite_pi", pbase,
           [](double x) { return tan_pi_reduce(x, toltrig::detail::reduce_cody_waite_pi); },
           [](double x) { return std::tan(x); });
      o.samples = saved_samples;
    }
    std::cout << "CSV written to " << o.csv << '\n';
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}

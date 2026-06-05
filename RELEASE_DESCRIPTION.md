# toltrig 0.2.0-alpha.1

toltrig is an experimental C++17 header-only library for bounded-input,
tolerance-aware trigonometric approximation.

This alpha release focuses on making the tradeoff explicit: if your application
already knows its angle range and can tolerate measured absolute error, toltrig
offers small kernels that may be faster than general-purpose `std::cos`,
`std::sin`, or `std::tan` paths on some platforms.

## Highlights

- Stable bounded `double` cosine kernels remain the main focus.
- Experimental sine candidates are available under `toltrig::experimental`.
- Experimental tangent support adds a continuous low-cost Pade candidate.
- Benchmarks report timing, speed ratio, absolute error, RMSE, and ULP distance.
- Claim policy remains conservative: no universal speedup claims, no hidden
  negative results, and no production-grade huge-argument reduction claims.

## Observed results

Measured results are platform-specific.

- On one M3 Pro Apple clang run, bounded cosine N=7 reached `3.78x` with `-O3`
  and `4.91x` with `-O3 -ffast-math` versus `std::cos` on `[-1e6,1e6]`.
- On Windows/MSVC, experimental sine and tangent show useful wins only in
  narrower ranges; broad-input gains are much smaller or sometimes negative.

Please benchmark on your own compiler, flags, input range, and tolerance before
adopting.

## Good fit

- simulations
- animation phases
- particle systems
- signal experiments
- visualizers
- other hot loops where bounded input and absolute error are acceptable

## Not a good fit

- correctly-rounded math
- arbitrary huge arguments
- financial or safety-critical numerical code
- drop-in replacement for `libm`

## Build

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

MIT licensed.

# Fixed-angle reduction experiment

This document records an experimental prototype for binary integer angle
reduction. It is not a production implementation and does not change the
existing `double` radian API.

## API

The prototype lives under the separate fixed-angle namespace
`toltrig::experimental::fixed`:

```cpp
double toltrig::experimental::fixed::sin_u32(std::uint32_t angle);
double toltrig::experimental::fixed::cos_u32(std::uint32_t angle);
double toltrig::experimental::fixed::tan_u32(std::uint32_t angle);
```

The existing `double` radian APIs remain in their current namespaces and are
not redirected through this path.

## Angle mapping

The `uint32_t` input represents a full turn:

- `0` maps to `0` radians.
- `2^32` maps to `2*pi` radians and is represented by unsigned wraparound
  back to `0`.
- The top two bits select the quadrant: `quadrant = angle >> 30`.
- The lower 30 bits select the position inside that quadrant.

After quadrant extraction, only the quadrant-local offset is converted to
`double`:

```cpp
double x = double(angle & ((1u << 30) - 1u)) * (half_pi / double(1u << 30));
```

This avoids the floating-point division and `nearbyint`/`remainder` style
argument-reduction step used by the bounded `double` APIs. The reduced
polynomial kernels are still reused:

- `sin_u32` uses `sin_reduced_n8` and `cos_reduced_n8`.
- `cos_u32` uses `cos_reduced_n8` and `sin_reduced_n8`.
- `tan_u32` uses the existing experimental `tan_reduced_pade` with reciprocal
  folding inside each quadrant.

## Quadrant rules

For `x` in `[0, pi/2)`:

| quadrant | angle interval | sin | cos | tan |
| --- | --- | --- | --- | --- |
| 0 | `[0, pi/2)` | `sin(x)` | `cos(x)` | `tan(x)` |
| 1 | `[pi/2, pi)` | `cos(x)` | `-sin(x)` | `-cot(x)` |
| 2 | `[pi, 3*pi/2)` | `-sin(x)` | `-cos(x)` | `tan(x)` |
| 3 | `[3*pi/2, 2*pi)` | `-cos(x)` | `sin(x)` | `-cot(x)` |

`tan_u32` returns negative infinity for exact odd-quarter turns. That is a
prototype convention for the exact binary pole; callers should not treat it as
a final API decision.

## Benchmarking

Build the project and run:

```sh
./build/toltrig_fixed_angle_bench --quick --csv fixed-angle-results.csv
./build/toltrig_fixed_angle_diagnostic
```

The benchmark compares, for the same generated angles:

- `std::sin`, `std::cos`, `std::tan` on the corresponding `double` radians.
- Existing bounded `double` implementations.
- Fixed-angle `uint32_t` implementations.

The CSV records timing and error versus the standard-library function.

## Current result status

The prototype and measurement harness are in place. One local Windows run was
measured with MSVC 19.44, `/O2`, 5,000,000 samples, 3 warm-ups, and 9 trials.

| function | model | ns/sample | speed vs `std` | max absolute error |
| --- | --- | ---: | ---: | ---: |
| `sin` | `std` | 7.226 | 1.000x | 0 |
| `sin` | bounded `double` N8 | 8.571 | 0.843x | `4.38e-14` |
| `sin` | fixed `uint32_t` | 5.245 | 1.378x | `5.27e-13` |
| `cos` | `std` | 7.244 | 1.000x | 0 |
| `cos` | bounded `double` N8 | 8.515 | 0.851x | `5.27e-13` |
| `cos` | fixed `uint32_t` | 5.324 | 1.361x | `5.27e-13` |
| `tan` | `std` | 8.487 | 1.000x | 0 |
| `tan` | bounded `double` Pade | 8.481 | 1.001x | `2.75e-4` |
| `tan` | fixed `uint32_t` | 7.097 | 1.196x | `5.55e-4` |

For this run, fixed-angle `sin_u32` and `cos_u32` were faster than both
`std::*` and the bounded `double` reducers. `tan_u32` was also faster, but its
accuracy remains governed by the current experimental tangent approximation and
pole behavior. These are platform-specific measurements, not a universal speed
claim.

A coefficient-level numerical mirror of the prototype over 200,000 random
`uint32_t` angles gave these rough accuracy checks before compiling the C++
benchmark:

| function | sampled max absolute error |
| --- | --- |
| `sin_u32` | about `5.3e-13` |
| `cos_u32` | about `5.3e-13` |
| `tan_u32`, excluding `abs(cos(x)) <= 1e-8` | about `3.2e-5` |

These were diagnostic numbers only; the C++ benchmark CSV remains the source of
truth for platform-specific timing and final error reporting.

Expected tradeoff:

- `sin_u32` and `cos_u32` should benefit when the caller already has angles in
  turn-scaled integer form, because quadrant selection is integer-only and no
  `double` argument reduction is needed.
- If the caller starts with radians, converting into `uint32_t` may erase the
  benefit.
- `tan_u32` is less certain because it still needs reciprocal branches near
  poles and uses the current experimental tangent approximation.

If the measured fixed-angle path is not faster on a platform, keep that result:
this experiment is intended to test the reduction strategy, not to guarantee a
speedup.

# Range reduction accuracy

The main accuracy question is not the kernel approximation alone. It is whether
the reducer returns the correct small residual and the correct quadrant/sign.
A tiny residual error can be amplified near tangent poles.

## Reference policy

`bench/diagnostic_reduction.cpp` compares reducers against `long double`
quotient or `std::remainderl` references. This is a practical diagnostic, not
a proof of correct reduction for every finite `double`. The CSV records this
limitation in `reference_note`.

On MSVC, `long double` has the same precision as `double`. In that environment
the reducer-only diagnostic is still useful for timing, failure counts, and
relative comparisons against the local library, but it is not a high-precision
accuracy oracle. Prefer x86 extended precision, MPFR, or a future dedicated
reference generator for release-quality reducer accuracy tables.

For full pipelines, `bench/bench_reduction_pipeline.cpp` compares against
`std::cos`, `std::sin`, and `std::tan` and reports:

- maximum absolute error;
- mean absolute error;
- RMSE;
- maximum relative error;
- maximum ULP error;
- failure count.

Relative and ULP errors must be interpreted carefully near zeros. For tangent,
pole-neighborhood rows are reported separately.

## Expected failure modes

### Nearbyint pi

`q = nearbyint(x / pi)` is fast and usually good for small bounded inputs. It
can fail when the quotient is so large that the low bits needed for parity or
residual reconstruction are no longer represented. The subtraction
`x - q*pi` also suffers cancellation as `q` grows.

It is rounding-mode dependent because `nearbyint` honors the current
floating-point rounding mode.

### Remainder

`std::remainder` delegates harder reduction to the standard library. It is the
safe comparison point in this project, but it is not free: benchmark rows are
expected to show a higher ns/sample cost than nearbyint on small and medium
ranges.

The project should not assume all standard libraries have identical
implementation quality or speed.

### Cody-Waite

Cody-Waite split subtraction reduces cancellation in the residual:

```text
r = ((x - q*pi_hi) - q*pi_mid) - q*pi_lo
```

It does not solve quotient selection for huge inputs. If `q` is wrong, the
split subtraction only computes a more carefully wrong residual. Therefore the
diagnostic must track both residual error and quadrant/sign correctness.

### Tangent poles

The tangent pipeline folds values near `pi/2` into a reciprocal branch. If the
reducer gets the distance to the pole wrong by `delta`, the final tangent error
can be far larger than `delta`. The reduction diagnostic records
`tan_pole_risk_score` as a relative pole-distance error:

```text
abs(model_pole_distance - reference_pole_distance)
/ max(reference_pole_distance, 1e-18)
```

This score is intentionally pessimistic. It is meant to identify dangerous
reducers, not to produce a user-facing accuracy promise.

## Current answer to the research questions

1. `nearbyint(x/pi)` is safe only under an explicit bounded-input contract and
   the normal rounding mode. Existing tests cover up to `1e6` for cosine, but
   that is not a proof for all values in the interval.
2. `std::remainder` becomes unattractive when its measured ns/sample cost is
   larger than the entire polynomial kernel. The new benchmark records that
   per range and platform.
3. Cody-Waite is a good fit to investigate for toltrig because it targets the
   gap between tiny bounded inputs and full Payne-Hanek. It should remain
   experimental until the new CSV results show a clear range where it improves
   error without losing too much speed.
4. Payne-Hanek is needed when quotient low bits matter and floating-point
   division cannot recover them. In practice this means huge inputs such as
   the `1e15` to `1e16` diagnostics already called out in the sine and tangent
   documents.
5. cos/sin/tan can share a reducer if the backend returns `pi/2` quadrant
   information, not just a residual and sign.
6. The likely speed/accuracy compromise is Cody-Waite for medium bounded
   ranges and standard-library or future Payne-Hanek reduction for broad
   finite inputs. Measurements must decide the exact cut points.

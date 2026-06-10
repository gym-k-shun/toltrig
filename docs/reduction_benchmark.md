# Range reduction benchmark

Two benchmark programs were added:

```text
toltrig_reduction_diagnostic
toltrig_reduction_pipeline_bench
```

The edit environment did not have `cmake` on `PATH`, so the programs were built
directly with MSVC 19.44 through the Visual Studio developer environment.
Quick benchmark CSV files were generated:

```text
reduction-diagnostic-quick.csv
reduction-pipeline-quick.csv
```

Treat these as smoke measurements, not release numbers. On MSVC,
`long double` has double precision, so reducer-only error columns are not a
high-precision oracle.

## Reducer-only benchmark

Source: `bench/diagnostic_reduction.cpp`

Suggested command:

```text
./build/toltrig_reduction_diagnostic --csv reduction-diagnostic.csv
```

Quick smoke command:

```text
./build/toltrig_reduction_diagnostic --quick --csv reduction-diagnostic-quick.csv
```

Reducers:

- `nearbyint_pi`
- `nearbyint_half_pi`
- `std_remainder_two_pi`
- `std_remainder_pi`
- `floor_pi`
- `current_half_pi_fold`
- `cody_waite_pi`
- `cody_waite_half_pi`
- `payne_hanek_placeholder`

Ranges:

- `[-pi,pi]`
- `[-100,100]`
- `[-1e6,1e6]`
- `[-1e9,1e9]`
- `[-1e12,1e12]`
- `[-1e16,1e16]`

Columns:

- `ns_per_sample`
- `max_reduction_error`
- `mean_reduction_error`
- `rmse`
- `quadrant_failures`
- `failures`
- `tan_pole_risk_score`
- `reference_note`

The reference is `long double` quotient or `std::remainderl`, which is better
than plain double diagnostics but not a formal high-precision oracle.

## Full pipeline benchmark

Source: `bench/bench_reduction_pipeline.cpp`

Suggested command:

```text
./build/toltrig_reduction_pipeline_bench --csv reduction-pipeline.csv
```

Quick smoke command:

```text
./build/toltrig_reduction_pipeline_bench --quick --csv reduction-pipeline-quick.csv
```

Pipelines:

- cosine N=7 and N=8 with current nearbyint;
- cosine N=8 with remainder pi, Cody-Waite pi, and Cody-Waite quadrant;
- sine N=7 and N=8 with current nearbyint;
- sine N=8 with remainder pi, Cody-Waite pi, and Cody-Waite quadrant;
- tangent continuous Pade with nearbyint pi, remainder pi, Cody-Waite pi, and
  Cody-Waite quadrant;
- tangent pole-neighborhood rows for current nearbyint and Cody-Waite pi.

Ranges:

- `[-pi,pi]`
- `[-100,100]`
- `[-1e6,1e6]`
- `[-1e9,1e9]`
- `[-1e12,1e12]`

Columns:

- `speed_ratio_vs_std`
- `max_abs_error`
- `mean_abs_error`
- `rmse`
- `max_relative_error`
- `max_ulp_error`
- `failure_count`

## Decision table template

Fill this after running release-machine benchmarks.

| Question | Evidence to use | Current status |
| --- | --- | --- |
| Where is `nearbyint(x/pi)` safe? | residual error, parity failures, full-pipeline errors by range | bounded only; exact cutoff unmeasured here |
| Where does `std::remainder` become too slow? | ns/sample versus nearbyint and kernel-only rows | platform-specific; unmeasured here |
| Does Cody-Waite fit toltrig? | Cody-Waite residual error versus speed on `1e6`, `1e9`, `1e12` | experimental candidate |
| When is Payne-Hanek required? | quadrant failures and huge-input full-pipeline failure rows | required for robust huge inputs |
| Can reducers be shared? | Cody-Waite quadrant cos/sin/tan rows | yes in design, cost must be measured |
| Best speed/error compromise | medium-range Cody-Waite rows versus remainder rows | not decided |

## Quick MSVC smoke observations

These observations used `--quick` with 20,000 random samples.

Reducer-only timing:

| Range | Reducer | ns/sample | Notable result |
| --- | --- | ---: | --- |
| `[-1e6,1e6]` | `nearbyint_pi` | `9.07` | fast, max reducer error about `5.82e-11` against the local reference |
| `[-1e6,1e6]` | `std_remainder_pi` | `14.16` | slower, local-reference residual error recorded as zero |
| `[-1e6,1e6]` | `cody_waite_pi` | `8.47` | speed close to nearbyint; error about `7.80e-11` in this smoke run |
| `[-1e12,1e12]` | `nearbyint_pi` | `9.06` | max reducer error around `pi`, showing quotient/residual breakdown |
| `[-1e12,1e12]` | `cody_waite_half_pi` | `6.09` | max residual error about `1.98e-4`; still not huge-input safe |

Full pipeline:

| Range | Pipeline | ns/sample | Max abs error | Observation |
| --- | --- | ---: | ---: | --- |
| `[-1e6,1e6]` | `cos_n8_nearbyint_pi` | `8.60` | `9.52e-11` | fast but less accurate than remainder/Cody-Waite |
| `[-1e6,1e6]` | `cos_n8_cody_waite_pi` | `8.66` | `3.94e-11` | same speed class, better smoke accuracy |
| `[-1e6,1e6]` | `sin_n8_cody_waite_pi` | `9.72` | `3.89e-11` | better than nearbyint, slower than nearbyint |
| `[-1e6,1e6]` | `tan_pade_cody_waite_quadrant` | `13.44` | `2.51e-2` | fastest tangent candidate in this smoke row |
| `[-1e12,1e12]` | `cos_n8_remainder_pi` | `20.04` | `3.89e-5` | slower but better than nearbyint/Cody-Waite pi |
| `[-1e12,1e12]` | `tan_pade_cody_waite_pi` | `14.56` | `7.63e4` | bad absolute error remains; tangent needs pole-aware large reduction |

Pole-neighborhood rows are intentionally harsh. For `[-1e6,1e6]`, tangent
near-pole max absolute errors were enormous for both nearbyint and Cody-Waite,
because reciprocal amplification dominates once pole distance is wrong.

## Reporting policy

Keep the `payne_hanek_placeholder` failure rows. They make it explicit that
the design slot exists but the implementation does not.

If Cody-Waite is slower than `std::remainder` or fails at `1e12`, keep that
result. The purpose is bounded trigonometry kernels plus honest range
reduction trade-offs, not a universal replacement for the standard library.

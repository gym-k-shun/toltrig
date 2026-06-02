# Experimental sine accuracy

Absolute error is the primary metric. Relative error and ULP distance become
unstable near sine zeros.

The repository measures direct Taylor N=7/N=8 candidates and cosine-reuse
candidates across bounded and broad ranges. Run:

```sh
./build/toltrig_sin_bench --csv sin-results.csv
./build/toltrig_sin_diagnostic
```

The diagnostic CSV records zero-neighborhood and huge-input behavior.
Measured results are platform-specific and must be retained with compiler,
flags, sample count, warm-up count, and trial count.

## Windows/MSVC observations

MSVC `/O2`, 500,000 samples, median of seven trials:

| Range | Model | Max absolute error | Mean absolute error | RMSE |
| --- | --- | ---: | ---: | ---: |
| `[-pi/2,pi/2]` | direct N=7 | `6.02e-12` | `3.34e-13` | `1.02e-12` |
| `[-pi/2,pi/2]` | direct N=8 | `4.36e-14` | `2.19e-15` | `6.97e-15` |
| `[-pi/2,pi/2]` | cosine reuse N=8 | `5.26e-13` | `2.78e-14` | `8.68e-14` |
| `[-100,100]` | direct N=8 | `4.39e-14` | `3.90e-15` | `7.56e-15` |
| `[-1e6,1e6]` | direct N=8 | `9.68e-11` | `1.53e-11` | `2.37e-11` |

Near zero, direct N=8 preserved the tested values to machine precision. The
cosine-reuse candidate retained an absolute error around `5.26e-13`, including
at zero, because the shifted cosine polynomial is evaluated near `pi / 2`.

At `x = 1e16`, direct N=8 returned signed zero while `std::sin(x)` was about
`0.780`. The bounded reducer is not suitable for huge arguments.

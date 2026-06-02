# Tangent coefficient and switch search report

## Fixed kernel switch search

For the original kernel:

```text
P(x) = x (15 - x^2) / (15 - 6 x^2)
```

no switchpoint in `(0, pi / 2)` satisfies:

```text
P(a) P(pi / 2 - a) = 1
```

The sampled maximum product was `0.9995754069` at `a = pi / 4`. Moving the
switch cannot remove the discontinuity. The original fixed-switch proposal is
rejected.

Error-only search for the original kernel selected a switch close to `pi / 4`
for `epsilon` from `1e-3` through `1e-6`, but retained a value jump of about
`4.25e-4`. Such candidates are excluded by the continuity requirement.

## Continuous low-cost candidate

The normalized form

```text
P(x) = x (1 + b x^2) / (1 + d x^2)
```

was searched under `P(pi / 4) = 1`. The retained coefficients are:

```text
b = -0.06860571525033407
d = -0.40178219629842254
```

The condition makes both the value jump and first-derivative jump zero at the
reciprocal switch. Dense reduced-grid maximum absolute error was `5.42e-6`,
compared with about `2.12e-4` for the rejected original kernel.

## Higher-order comparison candidate

A normalized quartic-over-quartic form was also explored:

```text
P(x) = x (1 + b x^2 + c x^4) / (1 + d x^2 + e x^4)
b =  0.788791664734832
c = -0.06121759926807135
d =  0.45541796800896994
e = -0.34604850453981323
```

It reduced dense-grid kernel maximum absolute error to about `5.09e-7`, but
adds operations and did not improve the measured Windows full-pipeline speed.
It remains an internal comparison candidate.

The coefficient search was numerical and heuristic, not a proof of global
optimality. A stable API would require cross-platform measurements, a formal
input contract, and a production-grade range reducer.

## Windows/MSVC performance

MSVC `/O2`, 500,000 samples, median of seven trials:

| Range | `std::tan` ns/sample | Continuous candidate | Ratio |
| --- | ---: | ---: | ---: |
| `[-pi/4,pi/4]` | `4.32` | `2.17` | `1.99x` |
| `[-pi/2,pi/2]` | `6.96` | `8.12` | `0.86x` |
| `[-pi,pi]` | `7.89` | `8.19` | `0.96x` |
| `[-100,100]` | `8.52` | `8.07` | `1.06x` |
| `[-1e6,1e6]` | `8.54` | `8.17` | `1.04x` |

These are platform-specific observations, not universal speed claims.

# Experimental sine design

Sine support is experimental and does not change the stable cosine API.

## Cosine reuse

The reuse candidates evaluate:

```text
sin(x) = cos(pi / 2 - x)
```

through the existing bounded cosine implementation. This is mathematically
valid but adds a phase shift and may lose accuracy near sine zeros.

## Direct Taylor evaluation

The direct candidates evaluate:

```text
S_N(x) = x sum(k=0..N) (-1)^k x^(2k) / (2k + 1)!
```

after reducing the input to approximately `[-pi / 2, pi / 2]`.

## Bounded reduction

The bounded direct candidates use:

```text
q = nearbyint(x / pi)
r = x - q pi
sin(x) = (-1)^q sin(r)
```

This is a bounded-input candidate only. It is rounding-mode dependent and is
not a production-grade huge-argument reducer.

## Candidate decision

The direct N=7 and N=8 candidates are retained under `toltrig::experimental`.
N=7 is the lower-cost candidate; N=8 is the higher-accuracy candidate.

The cosine-reuse candidates remain available for comparison but are rejected
as preferred implementations. On the measured Windows/MSVC build they were
slower than `std::sin` across all tested ranges and less suitable near sine
zeros because the phase shift leaves cosine-polynomial error near `pi / 2`.

## Windows/MSVC performance

MSVC `/O2`, 500,000 samples, median of seven trials:

| Range | `std::sin` ns/sample | Direct N=7 | Direct N=8 | Cosine reuse N=8 |
| --- | ---: | ---: | ---: | ---: |
| `[-pi/2,pi/2]` | `5.12` | `4.20` | `4.22` | `8.79` |
| `[-pi,pi]` | `6.53` | `8.84` | `8.65` | `8.79` |
| `[-100,100]` | `7.50` | `8.64` | `8.74` | `8.82` |
| `[-1e6,1e6]` | `7.43` | `8.71` | `8.74` | `8.80` |

Polynomial-only N=7 Horner took about `1.53 ns/sample`; N=7 Estrin took about
`1.39 ns/sample`; N=8 Estrin took about `1.75 ns/sample`. The Horner/Estrin
ranking requires cross-platform confirmation. Range reduction dominates the
broad-input pipeline.

On `[-1e6,1e6]`, `nearbyint(x / pi)` quotient calculation took about
`1.58 ns/sample`, while the direct sine fold including sign handling took about
`9.10 ns/sample`. The `std::remainder` folding path took about
`17.79 ns/sample`.

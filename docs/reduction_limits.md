# Range reduction limits

This document maps where the current toltrig range reducers remain useful and
where they start to fail. It does not add a public API, a Payne-Hanek
implementation, or new trigonometric functions.

Generated artifacts:

- `reduction-limits.csv`
- `reduction-limits-summary.csv`
- `docs/assets/reduction-limits-summary.svg`

Command used:

```text
python bench/reduction_limits.py --samples 2000 --csv reduction-limits.csv --summary-csv reduction-limits-summary.csv --svg docs/assets/reduction-limits-summary.svg
```

Reducer-only references use Python `decimal` with 90 digits. Full-pipeline
references use the platform `math` library. Therefore the reducer rows are the
primary evidence for reduction limits; full-pipeline rows are practical
characterization data.

## Classification

The summary labels are intentionally coarse:

- `Good`: max reducer error <= `1e-11` or full-pipeline max abs error <= `1e-11`.
- `OK`: error is above `Good` but <= `1e-8`.
- `Warning`: error is above `OK` but <= `1e-3`.
- `Fail`: error exceeds `1e-3`, or structural sign/quadrant failures exceed
  the diagnostic tolerance.

Sign and quadrant failure counts remain in the detailed CSV. A few sign flips
at exact fold boundaries are retained but not treated as a domain-wide failure.

## Limit map

| Input range | nearbyint pi | Cody-Waite pi | remainder pi | nearbyint half-pi | Cody-Waite half-pi | remainder half-pi | cos pipeline | sin pipeline | tan pipeline |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `1e0` | Good | Good | Good | Good | Good | Good | Good | Good | Fail |
| `1e1` | Good | Good | Good | Good | Good | Good | Good | Good | Fail |
| `1e2` | Good | Good | Good | Good | Good | Good | Good | Good | Fail |
| `1e3` | Good | Good | Good | Good | Good | Good | Good | Good | Fail |
| `1e4` | Good | Good | Good | Good | Good | Good | Good | Good | Fail |
| `1e5` | OK | Good | Good | OK | Good | Good | OK | OK | Fail |
| `1e6` | OK | OK | OK | OK | OK | OK | OK | OK | Fail |
| `1e7` | OK | OK | OK | OK | OK | OK | OK | OK | Fail |
| `1e8` | Warning | OK | OK | Warning | OK | OK | Warning | Warning | Fail |
| `1e9` | Warning | Warning | Warning | Warning | Warning | Warning | Warning | Warning | Fail |
| `1e10` | Warning | Warning | Warning | Warning | Warning | Warning | Warning | Warning | Fail |
| `1e11` | Warning | Warning | Warning | Warning | Warning | Warning | Warning | Warning | Fail |
| `1e12` | Warning | Warning | Warning | Warning | Warning | Warning | Warning | Warning | Fail |
| `1e13` | Fail | Fail | Warning | Fail | Fail | Warning | Fail | Fail | Fail |
| `1e14` | Fail | Fail | Fail | Fail | Fail | Fail | Fail | Fail | Fail |
| `1e15` | Fail | Fail | Fail | Fail | Fail | Fail | Fail | Fail | Fail |
| `1e16` | Fail | Fail | Fail | Fail | Fail | Fail | Fail | Fail | Fail |

## Boundary observations

At `1e6`, all measured reducers are still `OK`. The largest reducer errors are
around `1e-10`, and cosine/sine full pipelines stay around `1e-10` maximum
absolute error.

At `1e8`, `nearbyint` begins to show `Warning` behavior. Cody-Waite and
`std::remainder` still classify as `OK` in this run, which supports the idea
that Cody-Waite is useful as a medium-range experiment.

At `1e9` through `1e12`, all current reducers are only `Warning`. This is not
catastrophic for every cos/sin sample, but it is no longer a comfortable
bounded-kernel range.

At `1e13`, pi and half-pi nearbyint/Cody-Waite reducers fail the diagnostic.
The `std::remainder` variants remain `Warning` in this run, but full cos/sin
pipelines with the current nearbyint path already fail.

At `1e14` and above, every measured reducer is `Fail`. At `1e16`, sign and
quadrant failures are common, not just a few boundary artifacts.

## Tangent result

The tangent pipeline is `Fail` at every range in this diagnostic because the
sample set intentionally includes pole-neighborhood and exact fold-boundary
points. This is the useful result: current tangent is not recommendable without
an explicit minimum distance from poles.

For ordinary random samples away from poles, the Pade kernel can still be a
bounded experiment. But the reducer limit map says the API contract must include
pole distance. A range bound alone is insufficient.

## Recommendations

Safe domain recommendation:

- For current public cosine and experimental direct sine, recommend
  `|x| <= 1e6` when using the current nearbyint path.
- Treat `|x| <= 1e7` as still plausible but needing project-specific error
  tolerance review.
- Do not recommend current tangent without a pole-distance contract.

Experimental domain recommendation:

- Cody-Waite is worth keeping for `1e5` through `1e8`.
- Cody-Waite is not clearly better beyond `1e9`; it becomes `Warning` together
  with nearbyint and remainder.
- Half-pi quadrant reducers are a good design shape for shared cos/sin/tan
  reduction, but they are not a huge-input solution.

Failure boundary:

- `nearbyint` starts showing warning-level residual error around `1e8`.
- Cody-Waite starts showing warning-level residual error around `1e9`.
- Current nearbyint and Cody-Waite reducers fail around `1e13`.
- All measured reducers fail by `1e14`.

Payne-Hanek necessity:

- The measurements do not require Payne-Hanek for `|x| <= 1e6`.
- Cody-Waite appears sufficient to investigate for the medium range up to about
  `1e8`.
- Payne-Hanek becomes necessary if toltrig wants a credible story beyond
  `1e12`, and especially at `1e13+`, where quotient/sign/quadrant reliability
  is no longer stable.

## Roadmap

v0.2:

- Keep public APIs unchanged.
- Keep current nearbyint reduction as bounded only.
- Keep Cody-Waite and quadrant reducers experimental/detail only.
- Publish `reduction-limits.csv` style measurements with any release claims.
- State that tangent requires both range and pole-distance constraints.

v0.3:

- Add an internal Payne-Hanek `pi/2` quadrant reducer prototype.
- Re-run this same limit map with Payne-Hanek included.
- Decide whether an experimental backend selector is justified.
- Only consider public API changes after the backend limit map is stable across
  MSVC, clang, and gcc.

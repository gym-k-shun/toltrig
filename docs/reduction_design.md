# Range reduction design

toltrig should treat range reduction as an independent backend. The polynomial
or Pade kernel is cheap enough that the argument reducer can dominate both
speed and accuracy for broad input ranges.

This work does not add a new stable trigonometric API. It adds experimental
and `detail` reducers so the project can measure honest trade-offs before
choosing any public contract.

## Current reducers

| Reducer | Current use | Range contract | Main speed property | Main risk |
| --- | --- | --- | --- | --- |
| `nearbyint(x / pi)` | stable cosine bounded path, experimental sine and tangent | bounded inputs only | very low overhead | inaccurate for huge inputs, cancellation in `x - q*pi`, rounding-mode dependent |
| `std::remainder` plus half-pi fold | cosine safe comparison path | broad finite inputs, implementation dependent quality | slower than `nearbyint` on measured small/medium ranges | still not a complete correctly-rounded trig reducer; folding hides quadrant details |
| pi-switch floor/integer | diagnostic cosine only | bounded/diagnostic | simple quotient path | floor quotient gives `[0,pi]` residual and is not the right nearest reduction for kernels centered at zero |
| half-pi folding | cosine remainder path | broad finite inputs | uses library remainder, then folds to `[-pi/2,pi/2]` | gives only value/sign, not enough as a shared sin/cos/tan quadrant API |
| sine reduction | experimental direct sine | bounded inputs only | reuses `nearbyint(x / pi)` | same huge-input failure as cosine; better near sine zero than cosine-shift reuse |
| tangent reduction | experimental tangent | bounded inputs only and away from poles | reuses `nearbyint(x / pi)` | pole distance errors are amplified by reciprocal branch |

## New backend candidates

### A. Nearbyint fast

`reduce_nearbyint_pi` remains the fastest bounded candidate. It is appropriate
for small domains where the caller can guarantee a finite input bound and the
normal rounding mode. It should not be promoted as a huge-argument reducer.

### B. Remainder safe

`reduce_remainder_pi` was added as a comparison backend that returns both the
residual and the sign implied by the quotient parity. The existing
`reduce_half_pi_remainder` remains useful for cosine, but it does not expose a
quadrant.

### C. Cody-Waite medium

`reduce_cody_waite_pi` and `reduce_cody_waite_half_pi` use split constants:

```text
r = ((x - q*pi_hi) - q*pi_mid) - q*pi_lo
```

The intent is medium-range input such as `|x| <= 1e6`, `1e9`, and possibly
`1e12`, where the current single multiply/subtract begins to lose residual
bits but a full multiprecision Payne-Hanek reducer may be too expensive.

The implementation still computes `q` with a floating-point divide, so it is
not a huge-input solution. Its value must be proven by `diagnostic_reduction`
and full-pipeline measurements.

### D. Payne-Hanek large

Payne-Hanek is not implemented in v0.2. `payne_hanek_placeholder` exists only
as a benchmark row that records failure. A real implementation needs:

- a fixed-point or multiprecision representation of `2/pi`;
- enough limbs to cover all finite `double` exponents;
- quotient extraction for `pi/2` multiples;
- correctly signed residual reconstruction;
- special handling for NaN and infinities.

Payne-Hanek becomes necessary when quotient computation with `x / pi` or
`x / (pi/2)` cannot reliably determine the low quotient bits. That is the
region where quadrant/sign mistakes appear, not merely where the residual is a
few ulps worse.

### E. Quadrant reducer

`quadrant_reduced_angle` was added:

```cpp
struct quadrant_reduced_angle {
  double value;
  int quadrant;
};
```

It reduces by nearest multiples of `pi/2` and returns `quadrant mod 4`.
This is the right shape for a shared sin/cos/tan backend:

- cosine: select `cos(r)`, `-sin(r)`, `-cos(r)`, or `sin(r)`;
- sine: select `sin(r)`, `cos(r)`, `-sin(r)`, or `-cos(r)`;
- tangent: use `tan(r)` for even quadrants and `-1/tan(r)` for odd quadrants.

The cost is that cosine and sine pipelines need both sine and cosine reduced
kernels available. The benefit is a single quadrant contract and better pole
distance accounting for tangent.

## API position

The backend enum is currently in `toltrig::detail`:

```cpp
enum class reduction_backend {
  nearbyint_fast,
  remainder_safe,
  cody_waite_medium,
  payne_hanek_large
};
```

It should stay out of the stable API until measurements show a durable default
for each function family and input range.

## Initial classification

| Classification | Reducers |
| --- | --- |
| usable now under bounded contracts | `nearbyint_fast`, current half-pi remainder fold |
| worth keeping experimental | Cody-Waite pi and half-pi reducers, quadrant reducer |
| reject as default | floor/pi-switch for centered kernels |
| cos-oriented | nearbyint pi, remainder half-pi fold, Cody-Waite quadrant |
| sin-oriented | nearbyint pi, Cody-Waite quadrant |
| tan-oriented | Cody-Waite pi or quadrant, but only with explicit pole-distance contract |
| not huge-input safe | nearbyint, floor, current Cody-Waite |
| next implementation target | real Payne-Hanek `pi/2` reducer feeding `quadrant_reduced_angle` |

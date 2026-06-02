# Experimental tangent design

The original fixed `pi / 4` switch used `x(15 - x^2) / (15 - 6 x^2)`.
Moving the switch cannot remove its jump: the required product never reaches
one. The experimental replacement keeps the same low operation count but uses

```text
P(x) = x (1 + b x^2) / (1 + d x^2)
b = -0.06860571525033407
d = -0.40178219629842254
```

The coefficients were searched under `P(pi / 4) = 1`. At a `pi / 4` reciprocal
switch, this makes value and first derivative continuous.

`tan_bounded_pade` uses `nearbyint(x / pi)` reduction. It is experimental,
bounded-input only, rounding-mode dependent, and not suitable for huge
arguments. A stable general tangent API requires a production reducer and an
explicit pole contract.

For exact reduced poles, the implementation returns signed infinity
explicitly. It does not rely on an IEEE-754 division-by-zero side effect.

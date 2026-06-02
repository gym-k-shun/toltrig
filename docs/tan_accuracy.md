# Experimental tangent accuracy

The former fixed-switch kernel agreed with the Taylor expansion of tangent
through `x^5`, but introduced a value jump of about `4.25e-4` at `pi / 4`.

The continuous low-cost candidate enforces `P(pi / 4) = 1`. On a dense reduced
grid its observed maximum absolute kernel error was about `5.42e-6`.

Do not treat this as a huge-argument or correctly-rounded tangent function.
Near poles, small reduction errors are amplified. Validate an application
specific input range and distance from poles before use.

Windows/MSVC `/O2` measurements for the continuous low-cost candidate:

| Range | Max absolute error | Mean absolute error | RMSE |
| --- | ---: | ---: | ---: |
| `[-pi/4,pi/4]` | `5.41e-6` | `2.70e-6` | `3.33e-6` |
| `[-pi/2,pi/2]` | `7.47e-4` | `9.58e-6` | `1.41e-5` |
| `[-pi,pi]` | `3.13e-5` | `9.59e-6` | `1.41e-5` |
| `[-100,100]` | `1.37e-1` | `9.90e-6` | `1.94e-4` |
| `[-1e6,1e6]` | `2.23` | `1.83e-5` | `3.42e-3` |

The broad-range maximum absolute outliers are caused by reduction-error
amplification near poles. Maximum relative error remained about `1.18e-5` in
the sampled sets. ULP distance remains large and this is not a high-accuracy
replacement for `std::tan`.

The `[-pi/2,pi/2]` maximum also depends on how close sampled values land to a
pole. Applications need both an input bound and a minimum pole-distance
contract.

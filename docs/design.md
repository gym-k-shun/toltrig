# Design
toltrig is a bounded-input, tolerance-aware experiment, not a universal `std::cos` replacement.
The public API separates `reduced_input`, `bounded_nearbyint`, and `general_remainder`.
The reduction layer produces a value and sign; the polynomial layer evaluates Taylor N=7 or N=8 with Horner or Estrin. The public API currently uses Horner. A future backend may select SIMD implementations without changing input contracts.

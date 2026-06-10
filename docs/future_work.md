# Future work
- Linux GCC/Clang and Windows clang-cl/MinGW measurements
- Apple clang assembly analysis
- Cody-Waite and Payne-Hanek reduction
- SIMD backend and explicit FMA
- Minimax coefficients
- `float`, fixed-point, stable `sin`, `sincos`, and stable `tan`
- Production-grade reduction and cross-platform measurements for the
  experimental sine and tangent candidates
- Cross-platform measurements for the experimental `uint32_t` fixed-angle
  reducer, including cases where callers already store angles in binary turns

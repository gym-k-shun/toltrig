# Changelog
## Unreleased

## [0.2.0-alpha.1] - 2026-06-05
- Stable bounded cosine API remains the supported core.
- Added experimental sine support under `toltrig::experimental`.
- Added experimental bounded-input tangent support under `toltrig::experimental`.
- Added / updated benchmark and diagnostic targets for sin/cos/tan.
- Added README benchmark summary hero image.
- Added/updated docs for claim policy, benchmark methodology, sine/tangent
  design and accuracy.
- CI covers Ubuntu GCC, Ubuntu Clang, macOS Apple clang, and Windows MSVC.
- Known limitation: sine/tangent are experimental and not stable API.
- Known limitation: nearbyint-based bounded reducers are not huge-argument
  reducers.

## 0.1.0
- Initial experimental C++17 header-only cosine approximation.
- Taylor N=7/N=8 with Horner and Estrin evaluation.
- Bounded `nearbyint_pi` candidate and comparison reducers.
- Diagnostic benchmark and reproducibility report.

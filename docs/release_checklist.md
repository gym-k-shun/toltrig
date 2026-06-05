# Release checklist

- Update CMake project version.
- Update CHANGELOG with the release version and date.
- Update README so release assets and GitHub main describe the same API state.
- Regenerate benchmark summary image if benchmark claims change.
- Run CI.
- Run quick benchmarks for cosine, sine, and tangent.
- Build release zip from the current commit.
- Verify zip README matches GitHub README.
- Attach report PDF and assets when needed.
- Recommended GitHub About: `Fast bounded trigonometry kernels for C++17.`
- Recommended GitHub Topics: `cpp`, `cpp17`, `header-only`, `trigonometry`,
  `numerical-computing`, `approximation`, `benchmarking`, `cmake`,
  `performance`.

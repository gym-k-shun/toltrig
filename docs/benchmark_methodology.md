# Benchmark methodology
The benchmark uses deterministic uniform random samples with seed `20260111`, warm-up runs, repeated trials, and the median duration. A volatile sink prevents dead-code elimination.
CSV rows record platform, compiler label, flags label, benchmark type, range, samples, warm-up, trials, timing, speed ratio, and errors. Speed claims must remain per-platform observations.

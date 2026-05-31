# Accuracy
For `P_N(x)`, the Taylor remainder is bounded by `|x|^(2N+2)/(2N+2)!`.
This is why the reduced interval matters. N=7 is a balanced candidate; N=8 is the stricter candidate.
Benchmarks report maximum absolute error, mean absolute error, RMSE, and maximum ULP distance.
Relative error and ULP distance can become large near cosine zeros, so interpret them with absolute error. Validate the intended bounded input range before adoption.

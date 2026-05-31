#pragma once
#include <toltrig/toltrig.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace bench {
using fn = double(*)(double);
struct options { std::size_t samples=500000; int warmup=2,trials=7; std::uint64_t seed=20260111; std::string csv="results.csv",compiler="record-manually",flags="record-manually"; };
struct range { const char* name; double lo,hi; };
struct metrics { double max_abs=0; long double mean_abs=0,rmse=0; std::uint64_t max_ulp=0; };
inline volatile double sink=0;
inline std::string platform() {
#if defined(_WIN32)
return "Windows";
#elif defined(__APPLE__)
return "macOS";
#elif defined(__linux__)
return "Linux";
#else
return "Unknown";
#endif
}
inline options parse(int argc,char** argv) { options o; for(int i=1;i<argc;++i){std::string a=argv[i]; auto val=[&]{if(++i>=argc)throw std::runtime_error("missing value");return std::string(argv[i]);}; if(a=="--quick"){o.samples=20000;o.warmup=1;o.trials=3;}else if(a=="--samples")o.samples=std::stoull(val());else if(a=="--warmup")o.warmup=std::stoi(val());else if(a=="--trials")o.trials=std::stoi(val());else if(a=="--csv")o.csv=val();else if(a=="--compiler-label")o.compiler=val();else if(a=="--flags-label")o.flags=val();else throw std::runtime_error("unknown option: "+a);} return o; }
inline std::vector<double> inputs(range r,std::size_t n,std::uint64_t seed){std::mt19937_64 g(seed);std::uniform_real_distribution<double>d(r.lo,r.hi);std::vector<double>v(n);for(auto&x:v)x=d(g);return v;}
template<class F> inline double run(const std::vector<double>&v,F f){double s=0;for(double x:v)s+=f(x);sink=s;return s;}
template<class F> inline double median_ms(const std::vector<double>&v,F f,const options&o){for(int i=0;i<o.warmup;++i)run(v,f);std::vector<double>d;for(int i=0;i<o.trials;++i){auto a=std::chrono::steady_clock::now();run(v,f);auto b=std::chrono::steady_clock::now();d.push_back(std::chrono::duration<double,std::milli>(b-a).count());}std::sort(d.begin(),d.end());return d[d.size()/2];}
inline std::uint64_t bits(double x){std::uint64_t u;std::memcpy(&u,&x,sizeof u);constexpr auto s=UINT64_C(1)<<63;return(u&s)?~u:u|s;}
inline std::uint64_t ulp(double a,double b){if(a==b)return 0;if(!std::isfinite(a)||!std::isfinite(b))return std::numeric_limits<std::uint64_t>::max();auto x=bits(a),y=bits(b);return x>y?x-y:y-x;}
template<class F> inline metrics error(const std::vector<double>&v,F f){metrics m;long double ss=0;for(double x:v){double e=std::fabs(f(x)-std::cos(x));m.max_abs=std::max(m.max_abs,e);m.mean_abs+=e;ss+=e*e;m.max_ulp=std::max(m.max_ulp,ulp(f(x),std::cos(x)));}m.mean_abs/=v.size();m.rmse=std::sqrt(ss/v.size());return m;}
inline void header(std::ofstream&c){c<<"platform,compiler,flags,benchmark_type,range,model,samples,warmup,trials,median_ms,ns_per_sample,speed_ratio_vs_std_cos,max_abs_error,mean_abs_error,rmse,max_ulp_error\n";c<<std::setprecision(17);}
inline void row(std::ofstream&c,const options&o,const char*type,range r,const char*model,double ms,double base,metrics m={}){c<<'"'<<platform()<<"\",\""<<o.compiler<<"\",\""<<o.flags<<"\",\""<<type<<"\",\""<<r.name<<"\",\""<<model<<"\","<<o.samples<<','<<o.warmup<<','<<o.trials<<','<<ms<<','<<ms*1e6/o.samples<<','<<(base?base/ms:0)<<','<<m.max_abs<<','<<m.mean_abs<<','<<m.rmse<<','<<m.max_ulp<<'\n';}
}

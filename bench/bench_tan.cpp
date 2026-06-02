#include <toltrig/toltrig.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
volatile double sink=0; struct R{const char*n;double l,h;};struct O{std::size_t n=500000;int w=2,t=7;std::string csv="tan-results.csv",cc="record-manually",fl="record-manually";};struct M{double ma=0,mr=0;long double mean=0,rmse=0;std::uint64_t ulp=0;};
std::uint64_t bits(double x){std::uint64_t u;std::memcpy(&u,&x,8);constexpr auto s=UINT64_C(1)<<63;return(u&s)?~u:u|s;}std::uint64_t ud(double a,double b){if(a==b)return 0;if(!std::isfinite(a)||!std::isfinite(b))return UINT64_MAX;auto x=bits(a),y=bits(b);return x>y?x-y:y-x;}
O parse(int ac,char**av){O o;for(int i=1;i<ac;++i){std::string a=av[i];auto v=[&]{if(++i>=ac)throw std::runtime_error("missing value");return std::string(av[i]);};if(a=="--quick"){o.n=20000;o.w=1;o.t=3;}else if(a=="--csv")o.csv=v();else if(a=="--compiler-label")o.cc=v();else if(a=="--flags-label")o.fl=v();else throw std::runtime_error("unknown option");}return o;}
std::vector<double> input(R r,std::size_t n,int seed){std::mt19937_64 g(seed);std::uniform_real_distribution<double>d(r.l,r.h);std::vector<double>v(n);for(auto&x:v)x=d(g);return v;}
template<class F>double med(const std::vector<double>&v,F f,const O&o){auto run=[&]{double s=0;for(double x:v)s+=f(x);sink=s;};for(int i=0;i<o.w;++i)run();std::vector<double>d;for(int i=0;i<o.t;++i){auto a=std::chrono::steady_clock::now();run();auto b=std::chrono::steady_clock::now();d.push_back(std::chrono::duration<double,std::milli>(b-a).count());}std::sort(d.begin(),d.end());return d[d.size()/2];}
template<class F>M err(const std::vector<double>&v,F f){M m;long double ss=0;for(double x:v){double ref=std::tan(x),got=f(x),e=std::fabs(got-ref);m.ma=std::max(m.ma,e);if(ref)m.mr=std::max(m.mr,e/std::fabs(ref));m.ulp=std::max(m.ulp,ud(got,ref));m.mean+=e;ss+=e*e;}m.mean/=v.size();m.rmse=std::sqrt(ss/v.size());return m;}
const char* platform(){
#if defined(_WIN32)
return "Windows";
#elif defined(__APPLE__)
return "macOS";
#else
return "Linux";
#endif
}
void row(std::ofstream&c,const O&o,const char*type,R r,const char*model,double ms,double base,M m={},double j=0,double dj=0){c<<platform()<<','<<o.cc<<','<<o.fl<<','<<type<<",\""<<r.n<<"\","<<model<<','<<o.n<<','<<o.w<<','<<o.t<<','<<ms<<','<<ms*1e6/o.n<<','<<base/ms<<','<<m.ma<<','<<m.mean<<','<<m.rmse<<','<<m.mr<<','<<m.ulp<<','<<j<<','<<dj<<'\n';}
template<class F>void emit(std::ofstream&c,const O&o,const char*type,R r,const char*name,const std::vector<double>&v,double base,F f,double j=0,double dj=0){row(c,o,type,r,name,med(v,f,o),base,err(v,f),j,dj);}
template<class F>double jump(F p){double a=toltrig::pi/4;return std::fabs(p(a)-1/p(a));}template<class F>double djump(F p){double a=toltrig::pi/4,h=1e-7,l=(p(a+h)-p(a-h))/(2*h),r=l/(p(a)*p(a));return std::fabs(l-r);}
double nearby(double x){double q=std::nearbyint(x/toltrig::pi);return x-q*toltrig::pi;}double floor_near(double x){double q=std::floor(x/toltrig::pi+.5);return x-q*toltrig::pi;}double rem(double x){return std::remainder(x,toltrig::pi);}constexpr double pi_hi=3.141592653589793116,pi_lo=1.224646799147353207e-16;double cw(double x){double q=std::nearbyint(x/toltrig::pi);return(x-q*pi_hi)-q*pi_lo;}
}
int main(int ac,char**av){try{O o=parse(ac,av);std::ofstream c(o.csv);c<<"platform,compiler,flags,benchmark_type,range,model,samples,warmup,trials,median_ms,ns_per_sample,speed_ratio_vs_std_tan,max_abs_error,mean_abs_error,rmse,max_relative_error,max_ulp_error,switch_value_jump,switch_derivative_jump\n"<<std::setprecision(17);const R rr[]={{"[-pi/4,pi/4]",-toltrig::quarter_pi,toltrig::quarter_pi},{"[-pi/2,pi/2]",-toltrig::half_pi,toltrig::half_pi},{"[-pi,pi]",-toltrig::pi,toltrig::pi},{"[-100,100]",-100,100},{"[-1e6,1e6]",-1e6,1e6}};using namespace toltrig::experimental;auto cur=[](double x){return detail::tan_bounded_pade_current(x);};auto opt=[](double x){return tan_bounded_pade(x);};auto high=[](double x){return detail::tan_bounded_pade4(x);};auto pc=[](double x){return detail::tan_reduced_pade_current(x);};auto po=[](double x){return tan_reduced_pade(x);};auto ph=[](double x){return detail::tan_reduced_pade4(x);};for(int i=0;i<5;++i){auto v=input(rr[i],o.n,20260602+i);double b=med(v,[](double x){return std::tan(x);},o);row(c,o,"full",rr[i],"std_tan",b,b);emit(c,o,"full",rr[i],"current_fixed_pi4",v,b,cur,jump(pc),djump(pc));emit(c,o,"full",rr[i],"optimized_continuous",v,b,opt,jump(po),djump(po));emit(c,o,"full",rr[i],"optimized_higher_order",v,b,high,jump(ph),djump(ph));if(i==0){emit(c,o,"polynomial_only",rr[i],"current_kernel",v,b,pc);emit(c,o,"polynomial_only",rr[i],"optimized_kernel",v,b,po);emit(c,o,"polynomial_only",rr[i],"higher_order_kernel",v,b,ph);}if(i==4){for(auto m:{std::pair<const char*,double(*)(double)>{"nearbyint",nearby},{"floor_nearest",floor_near},{"remainder",rem},{"cody_waite_preliminary",cw}})row(c,o,"reduction_only",rr[i],m.first,med(v,m.second,o),b);}}std::cout<<"CSV written to "<<o.csv<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

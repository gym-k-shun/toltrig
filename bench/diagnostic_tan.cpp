#include <toltrig/toltrig.hpp>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
namespace {
constexpr double pi_hi=3.141592653589793116,pi_lo=1.224646799147353207e-16;
double nearby(double x){double q=std::nearbyint(x/toltrig::pi);return x-q*toltrig::pi;}
double floor_nearest(double x){double q=std::floor(x/toltrig::pi+0.5);return x-q*toltrig::pi;}
double remainder_pi(double x){return std::remainder(x,toltrig::pi);}
double cody_waite_preliminary(double x){double q=std::nearbyint(x/toltrig::pi);return(x-q*pi_hi)-q*pi_lo;}
double from_reduced(double r){double ar=std::fabs(r);if(ar<=toltrig::pi/4)return toltrig::experimental::tan_reduced_pade(r);return std::copysign(1.0,r)/toltrig::experimental::tan_reduced_pade(toltrig::half_pi-ar);}
}
int main(){std::ofstream c("tan-diagnostic.csv");c<<std::setprecision(17)<<"category,x,model,reduced,std_tan,approx,abs_error\n";using namespace toltrig::experimental;for(double x:{1e6,1e9,1e12,1e15,1e16})for(auto m:{std::pair<const char*,double(*)(double)>{"nearbyint",nearby},{"floor_nearest",floor_nearest},{"remainder",remainder_pi},{"cody_waite_preliminary",cody_waite_preliminary}}){double r=m.second(x),got=from_reduced(r);c<<"huge_input,"<<x<<','<<m.first<<','<<r<<','<<std::tan(x)<<','<<got<<','<<std::fabs(got-std::tan(x))<<'\n';}for(double d:{1e-1,1e-2,1e-3,1e-4,1e-5,1e-6}){double x=toltrig::half_pi-d,got=tan_bounded_pade(x);c<<"pole_neighborhood,"<<x<<",optimized_continuous,,"<<std::tan(x)<<','<<got<<','<<std::fabs(got-std::tan(x))<<'\n';}std::cout<<"CSV written to tan-diagnostic.csv\n";}

#include <toltrig/toltrig.hpp>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
int main(){std::ofstream c("sin-diagnostic.csv");c<<std::setprecision(17)<<"category,x,std_sin,from_cos_n8,direct_n8,from_cos_abs_error,direct_abs_error\n";using namespace toltrig::experimental;for(double x:{-1e-1,-1e-2,-1e-3,-1e-4,-1e-5,-1e-6,0.0,1e-6,1e-5,1e-4,1e-3,1e-2,1e-1})c<<"zero_neighborhood,"<<x<<','<<std::sin(x)<<','<<sin_from_cos_n8(x)<<','<<sin_bounded_n8(x)<<','<<std::fabs(sin_from_cos_n8(x)-std::sin(x))<<','<<std::fabs(sin_bounded_n8(x)-std::sin(x))<<'\n';for(double x:{1e6,1e9,1e12,1e15,1e16})c<<"huge_input,"<<x<<','<<std::sin(x)<<','<<sin_from_cos_n8(x)<<','<<sin_bounded_n8(x)<<','<<std::fabs(sin_from_cos_n8(x)-std::sin(x))<<','<<std::fabs(sin_bounded_n8(x)-std::sin(x))<<'\n';std::cout<<"CSV written to sin-diagnostic.csv\n";}

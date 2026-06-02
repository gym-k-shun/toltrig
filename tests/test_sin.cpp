#include <toltrig/toltrig.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
namespace {int failures=0;void check(bool ok,const char*m){if(!ok){std::cerr<<"FAIL: "<<m<<'\n';++failures;}}template<class F>double grid(double lo,double hi,int n,F f){double w=0;for(int i=0;i<=n;++i){double x=lo+(hi-lo)*i/n;w=std::max(w,std::fabs(f(x)-std::sin(x)));}return w;}}
int main(){using namespace toltrig;using namespace experimental;
  for(double x:{0.0,pi/6,pi/4,pi/3,half_pi,pi,two_pi,-pi/6,-pi/4,-pi/3,-half_pi,-pi,-two_pi,-0.0})
    check(std::fabs(sin_bounded_n8(x)-std::sin(x))<2e-11,"special value");
  for(double x:{-half_pi,half_pi}){check(std::fabs(sin_bounded_n8(std::nextafter(x,-INFINITY))-std::sin(std::nextafter(x,-INFINITY)))<2e-12,"fold left");check(std::fabs(sin_bounded_n8(std::nextafter(x,INFINITY))-std::sin(std::nextafter(x,INFINITY)))<2e-12,"fold right");}
  check(grid(-half_pi,half_pi,10000,sin_reduced_n7)<7e-11,"reduced n7 grid");
  check(grid(-half_pi,half_pi,10000,sin_reduced_n8)<1e-12,"reduced n8 grid");
  check(grid(-pi,pi,10000,sin_bounded_n8)<1e-12,"bounded pi grid");
  check(grid(-100,100,10000,sin_bounded_n8)<2e-11,"bounded 100 grid");
  check(std::isnan(sin_bounded_n8(NAN)),"nan");check(std::isnan(sin_bounded_n8(INFINITY)),"inf");check(std::isnan(sin_bounded_n8(-INFINITY)),"-inf");check(std::signbit(sin_bounded_n8(-0.0)),"negative zero");
  return failures?EXIT_FAILURE:EXIT_SUCCESS;}

#include <toltrig/toltrig.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
int failures = 0;
void check(bool ok, const char* msg) { if (!ok) { std::cerr << "FAIL: " << msg << '\n'; ++failures; } }
template<class F> double grid_error(double lo,double hi,int n,F f){double m=0;for(int i=0;i<=n;++i){double x=lo+(hi-lo)*i/n;m=std::max(m,std::fabs(f(x)-std::tan(x)));}return m;}
}
int main(){
  using namespace toltrig;
  using experimental::tan_bounded_pade;
  using experimental::tan_reduced_pade;
  for(double x:{0.0,pi/6,quarter_pi,-pi/6,-quarter_pi,-0.0})
    check(std::fabs(tan_bounded_pade(x)-std::tan(x))<6e-6,"special value");
  for(double x:{0.0,pi/6,quarter_pi,-pi/6,-quarter_pi,-0.0})
    check(std::fabs(tan_reduced_pade(x)-std::tan(x))<6e-6,"reduced special value");
  const double a=quarter_pi;
  const double left=tan_bounded_pade(std::nextafter(a,0.0));
  const double right=tan_bounded_pade(std::nextafter(a,half_pi));
  check(std::fabs(left-right)<1e-12,"continuous switch value");
  check(grid_error(-quarter_pi,quarter_pi,100000,tan_reduced_pade)<6e-6,"reduced grid");
  check(grid_error(-half_pi+1e-6,half_pi-1e-6,100000,tan_bounded_pade)<7e-5,"bounded pole grid");
  for(double delta:{1e-1,1e-2,1e-3,1e-4,1e-5,1e-6}) {
    const double x=half_pi-delta, expected=std::tan(x);
    check(std::fabs(tan_bounded_pade(x)-expected)/expected<2e-6,"pole neighborhood");
  }
  check(std::isnan(tan_bounded_pade(NAN)),"nan");
  check(std::isnan(tan_bounded_pade(INFINITY)),"positive infinity");
  check(std::isnan(tan_bounded_pade(-INFINITY)),"negative infinity");
  check(std::signbit(tan_bounded_pade(-0.0)),"negative zero");
  check(std::isinf(tan_bounded_pade(half_pi))&&!std::signbit(tan_bounded_pade(half_pi)),"positive pole");
  check(std::isinf(tan_bounded_pade(-half_pi))&&std::signbit(tan_bounded_pade(-half_pi)),"negative pole");
  return failures?EXIT_FAILURE:EXIT_SUCCESS;
}

//
// File: xdotc.cpp
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 20:35:40
//

// Include Files
#include "xdotc.h"
#include "rt_nonfinite.h"

// Function Definitions
//
// Arguments    : const double x[9]
//                const double y[9]
//                int iy0
// Return Type  : double
//
namespace coder {
namespace internal {
namespace blas {
double xdotc(const double x[9], const double y[9], int iy0)
{
  return x[1] * y[iy0 - 1] + x[2] * y[iy0];
}

} // namespace blas
} // namespace internal
} // namespace coder

//
// File trailer for xdotc.cpp
//
// [EOF]
//

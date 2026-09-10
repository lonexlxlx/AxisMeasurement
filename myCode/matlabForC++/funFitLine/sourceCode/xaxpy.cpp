//
// File: xaxpy.cpp
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 20:35:40
//

// Include Files
#include "xaxpy.h"
#include "rt_nonfinite.h"
#include "coder_array.h"

// Function Definitions
//
// Arguments    : int n
//                double a
//                const ::coder::array<double, 2U> &x
//                int ix0
//                ::coder::array<double, 1U> &y
// Return Type  : void
//
namespace coder {
namespace internal {
namespace blas {
void xaxpy(int n, double a, const ::coder::array<double, 2U> &x, int ix0,
           ::coder::array<double, 1U> &y)
{
  if ((n >= 1) && (!(a == 0.0))) {
    int i;
    i = n - 1;
    for (int k{0}; k <= i; k++) {
      int i1;
      i1 = k + 1;
      y[i1] = y[i1] + a * x[(ix0 + k) - 1];
    }
  }
}

//
// Arguments    : int n
//                double a
//                const ::coder::array<double, 1U> &x
//                ::coder::array<double, 2U> &y
//                int iy0
// Return Type  : void
//
void xaxpy(int n, double a, const ::coder::array<double, 1U> &x,
           ::coder::array<double, 2U> &y, int iy0)
{
  if ((n >= 1) && (!(a == 0.0))) {
    int i;
    i = n - 1;
    for (int k{0}; k <= i; k++) {
      int i1;
      i1 = (iy0 + k) - 1;
      y[i1] = y[i1] + a * x[k + 1];
    }
  }
}

//
// Arguments    : int n
//                double a
//                int ix0
//                ::coder::array<double, 2U> &y
//                int iy0
// Return Type  : void
//
void xaxpy(int n, double a, int ix0, ::coder::array<double, 2U> &y, int iy0)
{
  if ((n >= 1) && (!(a == 0.0))) {
    int i;
    i = n - 1;
    for (int k{0}; k <= i; k++) {
      int i1;
      i1 = (iy0 + k) - 1;
      y[i1] = y[i1] + a * y[(ix0 + k) - 1];
    }
  }
}

//
// Arguments    : double a
//                double y[9]
//                int iy0
// Return Type  : void
//
void xaxpy(double a, double y[9], int iy0)
{
  if (!(a == 0.0)) {
    y[iy0 - 1] += a * y[1];
    y[iy0] += a * y[2];
  }
}

} // namespace blas
} // namespace internal
} // namespace coder

//
// File trailer for xaxpy.cpp
//
// [EOF]
//

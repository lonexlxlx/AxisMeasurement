//
// File: xaxpy.h
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 20:35:40
//

#ifndef XAXPY_H
#define XAXPY_H

// Include Files
#include "rtwtypes.h"
#include "coder_array.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
namespace coder {
namespace internal {
namespace blas {
void xaxpy(int n, double a, const ::coder::array<double, 2U> &x, int ix0,
           ::coder::array<double, 1U> &y);

void xaxpy(int n, double a, const ::coder::array<double, 1U> &x,
           ::coder::array<double, 2U> &y, int iy0);

void xaxpy(int n, double a, int ix0, ::coder::array<double, 2U> &y, int iy0);

void xaxpy(double a, double y[9], int iy0);

} // namespace blas
} // namespace internal
} // namespace coder

#endif
//
// File trailer for xaxpy.h
//
// [EOF]
//

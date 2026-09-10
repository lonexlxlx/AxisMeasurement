//
// File: xgemv.h
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

#ifndef XGEMV_H
#define XGEMV_H

// Include Files
#include "rtwtypes.h"
#include "coder_array.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
namespace coder {
namespace internal {
namespace blas {
void xgemv(int m, const ::coder::array<double, 2U> &A, int lda,
           const ::coder::array<double, 2U> &x, double y[3]);

}
} // namespace internal
} // namespace coder

#endif
//
// File trailer for xgemv.h
//
// [EOF]
//

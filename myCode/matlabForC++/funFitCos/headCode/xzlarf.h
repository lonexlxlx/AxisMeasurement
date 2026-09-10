//
// File: xzlarf.h
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

#ifndef XZLARF_H
#define XZLARF_H

// Include Files
#include "rtwtypes.h"
#include "coder_array.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
namespace coder {
namespace internal {
namespace reflapack {
void xzlarf(int m, int n, int iv0, double tau, ::coder::array<double, 2U> &C,
            int ic0, int ldc, double work[3]);

}
} // namespace internal
} // namespace coder

#endif
//
// File trailer for xzlarf.h
//
// [EOF]
//

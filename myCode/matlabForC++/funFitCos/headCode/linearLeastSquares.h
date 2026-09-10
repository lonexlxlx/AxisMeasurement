//
// File: linearLeastSquares.h
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

#ifndef LINEARLEASTSQUARES_H
#define LINEARLEASTSQUARES_H

// Include Files
#include "rtwtypes.h"
#include "coder_array.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace levenbergMarquardt {
void linearLeastSquares(::coder::array<double, 2U> &lhs,
                        ::coder::array<double, 1U> &rhs, double dx[3], int m);

}
} // namespace coder
} // namespace optim
} // namespace coder

#endif
//
// File trailer for linearLeastSquares.h
//
// [EOF]
//

//
// File: jacobianFiniteDifference.h
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

#ifndef JACOBIANFINITEDIFFERENCE_H
#define JACOBIANFINITEDIFFERENCE_H

// Include Files
#include "rtwtypes.h"
#include "coder_array.h"
#include <cstddef>
#include <cstdlib>

// Type Declarations
struct c_struct_T;

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace levenbergMarquardt {
boolean_T jacobianFiniteDifference(::coder::array<double, 2U> &augJacobian,
                                   const ::coder::array<double, 2U> &fCurrent,
                                   int &funcCount, const double x[3],
                                   const c_struct_T &FiniteDifferences);

}
} // namespace coder
} // namespace optim
} // namespace coder

#endif
//
// File trailer for jacobianFiniteDifference.h
//
// [EOF]
//

//
// File: driver.h
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

#ifndef DRIVER_H
#define DRIVER_H

// Include Files
#include "rtwtypes.h"
#include "coder_array.h"
#include <cstddef>
#include <cstdlib>

// Type Declarations
namespace coder {
class anonymous_function;

}
struct c_struct_T;

// Function Declarations
void binary_expand_op(c_struct_T &in1, const double in2[3],
                      const coder::array<double, 2U> &in3);

namespace coder {
namespace optim {
namespace coder {
namespace levenbergMarquardt {
double driver(const anonymous_function &fun, double x[3],
              ::coder::array<double, 2U> &fCurrent, char output_algorithm[19],
              double lambda_lower[3], double lambda_upper[3],
              ::coder::array<double, 2U> &jacobian, double &exitflag,
              double &output_iterations, double &output_funcCount,
              double &output_stepsize, double &output_firstorderopt);

}
} // namespace coder
} // namespace optim
} // namespace coder

#endif
//
// File trailer for driver.h
//
// [EOF]
//

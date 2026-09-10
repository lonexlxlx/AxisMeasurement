//
// File: lsqcurvefit.cpp
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

// Include Files
#include "lsqcurvefit.h"
#include "anonymous_function1.h"
#include "funFitCos_internal_types11.h"
#include "rt_nonfinite.h"
#include "coder_array.h"

// Function Definitions
//
// Arguments    : coder::array<double, 2U> &in1
//                const double in2[3]
//                const coder::array<double, 2U> &in3
//                const coder::anonymous_function &in4
// Return Type  : void
//
void binary_expand_op(coder::array<double, 2U> &in1, const double in2[3],
                      const coder::array<double, 2U> &in3,
                      const coder::anonymous_function &in4)
{
  double b_in2;
  double c_in2;
  int i;
  int loop_ub;
  int stride_0_1;
  int stride_1_1;
  b_in2 = in2[0];
  c_in2 = in2[2];
  if (in4.workspace.ydata.size(1) == 1) {
    i = in3.size(1);
  } else {
    i = in4.workspace.ydata.size(1);
  }
  in1.set_size(1, i);
  stride_0_1 = (in3.size(1) != 1);
  stride_1_1 = (in4.workspace.ydata.size(1) != 1);
  if (in4.workspace.ydata.size(1) == 1) {
    loop_ub = in3.size(1);
  } else {
    loop_ub = in4.workspace.ydata.size(1);
  }
  for (i = 0; i < loop_ub; i++) {
    in1[i] = (b_in2 * in3[i * stride_0_1] + c_in2) -
             in4.workspace.ydata[i * stride_1_1];
  }
}

//
// File trailer for lsqcurvefit.cpp
//
// [EOF]
//

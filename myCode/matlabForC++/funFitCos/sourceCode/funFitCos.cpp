//
// File: funFitCos.cpp
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

// Include Files
#include "funFitCos.h"
#include "anonymous_function1.h"
#include "driver.h"
#include "funFitCos_internal_types11.h"
#include "rt_nonfinite.h"
#include "coder_array.h"

// Function Definitions
//
// A是偏心距；B是相位；C是实际旋转1中心偏移量
//
// Arguments    : const coder::array<double, 2U> &center
//                double *A
//                double *B
//                double *C
// Return Type  : void
//
void funFitCos(const coder::array<double, 2U> &center, double *A, double *B,
               double *C)
{
  coder::anonymous_function b_this;
  coder::anonymous_function c_this;
  coder::array<double, 2U> jacobian;
  double b_expl_temp[3];
  double c_expl_temp[3];
  double coeff[3];
  double d_expl_temp;
  double delta1;
  double e_expl_temp;
  double f_expl_temp;
  double g_expl_temp;
  int i;
  int k;
  char expl_temp[19];
  b_this.workspace.xdata.set_size(1, center.size(1));
  if (center.size(1) >= 1) {
    b_this.workspace.xdata[center.size(1) - 1] = -6.2831853071795862;
    if (b_this.workspace.xdata.size(1) >= 2) {
      b_this.workspace.xdata[0] = 0.0;
      if (b_this.workspace.xdata.size(1) >= 3) {
        delta1 = -6.2831853071795862 /
                 (static_cast<double>(b_this.workspace.xdata.size(1)) - 1.0);
        i = b_this.workspace.xdata.size(1);
        for (k = 0; k <= i - 3; k++) {
          b_this.workspace.xdata[k + 1] =
              (static_cast<double>(k) + 1.0) * delta1;
        }
      }
    }
  }
  //  最小二乘法拟合目标函数
  //  初始系数猜测值
  b_this.workspace.ydata.set_size(1, center.size(1));
  k = center.size(1);
  for (i = 0; i < k; i++) {
    b_this.workspace.ydata[i] = center[i];
  }
  c_this = b_this;
  coder::optim::coder::levenbergMarquardt::driver(
      c_this, coeff, b_this.workspace.xdata, expl_temp, b_expl_temp,
      c_expl_temp, jacobian, delta1, d_expl_temp, e_expl_temp, f_expl_temp,
      g_expl_temp);
  if (coeff[0] > 0.0) {
    *A = coeff[0];
    *B = coeff[1];
  } else {
    *A = -coeff[0];
    *B = coeff[1] + 3.1415926535897931;
  }
  *C = coeff[2];
}

//
// File trailer for funFitCos.cpp
//
// [EOF]
//

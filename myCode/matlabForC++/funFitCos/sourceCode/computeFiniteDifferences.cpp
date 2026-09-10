//
// File: computeFiniteDifferences.cpp
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

// Include Files
#include "computeFiniteDifferences.h"
#include "anonymous_function.h"
#include "anonymous_function1.h"
#include "driver.h"
#include "funFitCos_data.h"
#include "funFitCos_internal_types.h"
#include "funFitCos_internal_types1.h"
#include "funFitCos_internal_types11.h"
#include "rt_nonfinite.h"
#include "coder_array.h"
#include <cmath>

// Function Definitions
//
// Arguments    : c_struct_T &obj
//                const ::coder::array<double, 2U> &cEqCurrent
//                double xk[3]
//                ::coder::array<double, 2U> &JacCeqTrans
// Return Type  : boolean_T
//
namespace coder {
namespace optim {
namespace coder {
namespace utils {
namespace FiniteDifferences {
boolean_T computeFiniteDifferences(c_struct_T &obj,
                                   const ::coder::array<double, 2U> &cEqCurrent,
                                   double xk[3],
                                   ::coder::array<double, 2U> &JacCeqTrans)
{
  array<double, 2U> x;
  boolean_T evalOK;
  if (obj.isEmptyNonlcon) {
    evalOK = true;
  } else {
    int idx;
    boolean_T exitg1;
    evalOK = true;
    obj.numEvals = 0;
    idx = 0;
    exitg1 = false;
    while ((!exitg1) && (idx < 3)) {
      double b_deltaX;
      double deltaX;
      double lbDiff;
      double temp;
      double ubDiff;
      int k;
      int nx;
      boolean_T guard1{false};
      boolean_T modifiedStep;
      deltaX = 1.4901161193847656E-8 *
               (1.0 - 2.0 * static_cast<double>(xk[idx] < 0.0)) *
               std::fmax(std::abs(xk[idx]), 1.0);
      modifiedStep = false;
      if ((xk[idx] >= dv[idx]) && (xk[idx] <= dv1[idx])) {
        lbDiff = xk[idx] + deltaX;
        if ((lbDiff > dv1[idx]) || (lbDiff < dv[idx])) {
          deltaX = -deltaX;
          modifiedStep = true;
          lbDiff = xk[idx] + deltaX;
          if ((lbDiff > dv1[idx]) || (lbDiff < dv[idx])) {
            lbDiff = xk[idx] - dv[idx];
            ubDiff = dv1[idx] - xk[idx];
            if (lbDiff <= ubDiff) {
              deltaX = -lbDiff;
            } else {
              deltaX = ubDiff;
            }
          }
        }
      }
      b_deltaX = deltaX;
      evalOK = true;
      temp = xk[idx];
      xk[idx] += deltaX;
      x.set_size(1, obj.nonlin.workspace.fun.workspace.xdata.size(1));
      lbDiff = xk[1];
      nx = obj.nonlin.workspace.fun.workspace.xdata.size(1);
      for (k = 0; k < nx; k++) {
        x[k] = obj.nonlin.workspace.fun.workspace.xdata[k] + lbDiff;
      }
      nx = x.size(1);
      for (k = 0; k < nx; k++) {
        x[k] = std::cos(x[k]);
      }
      if (x.size(1) == obj.nonlin.workspace.fun.workspace.ydata.size(1)) {
        lbDiff = xk[0];
        ubDiff = xk[2];
        if (x.size(1) == 1) {
          nx = obj.nonlin.workspace.fun.workspace.ydata.size(1);
        } else {
          nx = x.size(1);
        }
        if (x.size(1) == 1) {
          k = obj.nonlin.workspace.fun.workspace.ydata.size(1);
        } else {
          k = x.size(1);
        }
        obj.cEq_1.set_size(k);
        for (k = 0; k < nx; k++) {
          obj.cEq_1[k] = (lbDiff * x[k] + ubDiff) -
                         obj.nonlin.workspace.fun.workspace.ydata[k];
        }
      } else {
        binary_expand_op(obj, xk, x);
      }
      nx = 0;
      while (evalOK && (nx + 1 <= obj.mEq)) {
        evalOK = ((!std::isinf(obj.cEq_1[nx])) && (!std::isnan(obj.cEq_1[nx])));
        nx++;
      }
      xk[idx] = temp;
      obj.f_1 = 0.0;
      obj.numEvals++;
      guard1 = false;
      if (!evalOK) {
        if (!modifiedStep) {
          b_deltaX = -deltaX;
          lbDiff = xk[idx] - deltaX;
          if ((lbDiff >= dv[idx]) && (lbDiff <= dv1[idx])) {
            modifiedStep = true;
          } else {
            modifiedStep = false;
          }
          if ((!obj.hasBounds) || modifiedStep) {
            evalOK = true;
            temp = xk[idx];
            xk[idx] = lbDiff;
            x.set_size(1, obj.nonlin.workspace.fun.workspace.xdata.size(1));
            lbDiff = xk[1];
            nx = obj.nonlin.workspace.fun.workspace.xdata.size(1);
            for (k = 0; k < nx; k++) {
              x[k] = obj.nonlin.workspace.fun.workspace.xdata[k] + lbDiff;
            }
            nx = x.size(1);
            for (k = 0; k < nx; k++) {
              x[k] = std::cos(x[k]);
            }
            if (x.size(1) == obj.nonlin.workspace.fun.workspace.ydata.size(1)) {
              lbDiff = xk[0];
              ubDiff = xk[2];
              if (x.size(1) == 1) {
                nx = obj.nonlin.workspace.fun.workspace.ydata.size(1);
              } else {
                nx = x.size(1);
              }
              if (x.size(1) == 1) {
                k = obj.nonlin.workspace.fun.workspace.ydata.size(1);
              } else {
                k = x.size(1);
              }
              obj.cEq_1.set_size(k);
              for (k = 0; k < nx; k++) {
                obj.cEq_1[k] = (lbDiff * x[k] + ubDiff) -
                               obj.nonlin.workspace.fun.workspace.ydata[k];
              }
            } else {
              binary_expand_op(obj, xk, x);
            }
            nx = 0;
            while (evalOK && (nx + 1 <= obj.mEq)) {
              evalOK = ((!std::isinf(obj.cEq_1[nx])) &&
                        (!std::isnan(obj.cEq_1[nx])));
              nx++;
            }
            xk[idx] = temp;
            obj.f_1 = 0.0;
            obj.numEvals++;
          }
        }
        if (!evalOK) {
          exitg1 = true;
        } else {
          guard1 = true;
        }
      } else {
        guard1 = true;
      }
      if (guard1) {
        k = obj.mEq;
        for (nx = 0; nx < k; nx++) {
          JacCeqTrans[idx + 3 * nx] =
              (obj.cEq_1[nx] - cEqCurrent[nx]) / b_deltaX;
        }
        idx++;
      }
    }
  }
  return evalOK;
}

} // namespace FiniteDifferences
} // namespace utils
} // namespace coder
} // namespace optim
} // namespace coder

//
// File trailer for computeFiniteDifferences.cpp
//
// [EOF]
//

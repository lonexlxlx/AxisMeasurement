//
// File: jacobianFiniteDifference.cpp
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

// Include Files
#include "jacobianFiniteDifference.h"
#include "computeFiniteDifferences.h"
#include "funFitCos_internal_types.h"
#include "rt_nonfinite.h"
#include "coder_array.h"

// Function Definitions
//
// Arguments    : ::coder::array<double, 2U> &augJacobian
//                const ::coder::array<double, 2U> &fCurrent
//                int &funcCount
//                const double x[3]
//                const c_struct_T &FiniteDifferences
// Return Type  : boolean_T
//
namespace coder {
namespace optim {
namespace coder {
namespace levenbergMarquardt {
boolean_T jacobianFiniteDifference(::coder::array<double, 2U> &augJacobian,
                                   const ::coder::array<double, 2U> &fCurrent,
                                   int &funcCount, const double x[3],
                                   const c_struct_T &FiniteDifferences)
{
  array<double, 2U> JacCeqTrans;
  c_struct_T b_FiniteDifferences;
  double a__3[3];
  int loop_ub;
  boolean_T evalOK;
  JacCeqTrans.set_size(3, fCurrent.size(1));
  a__3[0] = x[0];
  a__3[1] = x[1];
  a__3[2] = x[2];
  b_FiniteDifferences = FiniteDifferences;
  evalOK = utils::FiniteDifferences::computeFiniteDifferences(
      b_FiniteDifferences, fCurrent, a__3, JacCeqTrans);
  funcCount += b_FiniteDifferences.numEvals;
  loop_ub = JacCeqTrans.size(1);
  for (int i{0}; i < 3; i++) {
    for (int i1{0}; i1 < loop_ub; i1++) {
      augJacobian[i1 + augJacobian.size(0) * i] = JacCeqTrans[i + 3 * i1];
    }
  }
  return evalOK;
}

} // namespace levenbergMarquardt
} // namespace coder
} // namespace optim
} // namespace coder

//
// File trailer for jacobianFiniteDifference.cpp
//
// [EOF]
//

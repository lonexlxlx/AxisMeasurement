//
// File: factoryConstruct.cpp
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

// Include Files
#include "factoryConstruct.h"
#include "anonymous_function.h"
#include "funFitCos_internal_types.h"
#include "rt_nonfinite.h"
#include "coder_array.h"

// Function Definitions
//
// Arguments    : const b_anonymous_function &nonlin
//                int mCeq
//                c_struct_T &obj
// Return Type  : void
//
namespace coder {
namespace optim {
namespace coder {
namespace utils {
namespace FiniteDifferences {
void factoryConstruct(const b_anonymous_function &nonlin, int mCeq,
                      c_struct_T &obj)
{
  obj.nonlin = nonlin;
  obj.f_1 = 0.0;
  obj.cEq_1.set_size(mCeq);
  obj.f_2 = 0.0;
  obj.cEq_2.set_size(mCeq);
  obj.nVar = 3;
  obj.mIneq = 0;
  obj.mEq = mCeq;
  obj.numEvals = 0;
  obj.SpecifyObjectiveGradient = false;
  obj.SpecifyConstraintGradient = false;
  obj.isEmptyNonlcon = (mCeq == 0);
  obj.FiniteDifferenceType = 0;
  obj.hasLB[0] = true;
  obj.hasUB[0] = true;
  for (int idx{1}; idx < 3; idx++) {
    obj.hasLB[idx] = true;
    obj.hasUB[idx] = true;
  }
  obj.hasBounds = true;
}

} // namespace FiniteDifferences
} // namespace utils
} // namespace coder
} // namespace optim
} // namespace coder

//
// File trailer for factoryConstruct.cpp
//
// [EOF]
//

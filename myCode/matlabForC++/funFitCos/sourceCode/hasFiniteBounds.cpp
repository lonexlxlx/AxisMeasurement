//
// File: hasFiniteBounds.cpp
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

// Include Files
#include "hasFiniteBounds.h"
#include "rt_nonfinite.h"

// Function Definitions
//
// Arguments    : boolean_T hasLB[3]
//                boolean_T hasUB[3]
// Return Type  : boolean_T
//
namespace coder {
namespace optim {
namespace coder {
namespace utils {
boolean_T hasFiniteBounds(boolean_T hasLB[3], boolean_T hasUB[3])
{
  boolean_T hasBounds;
  hasLB[0] = true;
  hasUB[0] = true;
  hasBounds = true;
  for (int idx{1}; idx < 3; idx++) {
    hasLB[idx] = true;
    hasUB[idx] = true;
  }
  return hasBounds;
}

} // namespace utils
} // namespace coder
} // namespace optim
} // namespace coder

//
// File trailer for hasFiniteBounds.cpp
//
// [EOF]
//

//
// File: funFitCos_internal_types.h
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

#ifndef FUNFITCOS_INTERNAL_TYPES_H
#define FUNFITCOS_INTERNAL_TYPES_H

// Include Files
#include "anonymous_function.h"
#include "funFitCos_types.h"
#include "rtwtypes.h"
#include "coder_array.h"

// Type Definitions
struct c_struct_T {
  coder::b_anonymous_function nonlin;
  double f_1;
  coder::array<double, 1U> cEq_1;
  double f_2;
  coder::array<double, 1U> cEq_2;
  int nVar;
  int mIneq;
  int mEq;
  int numEvals;
  boolean_T SpecifyObjectiveGradient;
  boolean_T SpecifyConstraintGradient;
  boolean_T isEmptyNonlcon;
  boolean_T hasLB[3];
  boolean_T hasUB[3];
  boolean_T hasBounds;
  int FiniteDifferenceType;
};

#endif
//
// File trailer for funFitCos_internal_types.h
//
// [EOF]
//

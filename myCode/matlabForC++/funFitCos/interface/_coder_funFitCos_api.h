//
// File: _coder_funFitCos_api.h
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

#ifndef _CODER_FUNFITCOS_API_H
#define _CODER_FUNFITCOS_API_H

// Include Files
#include "coder_array_mex.h"
#include "emlrt.h"
#include "tmwtypes.h"
#include <algorithm>
#include <cstring>

// Variable Declarations
extern emlrtCTX emlrtRootTLSGlobal;
extern emlrtContext emlrtContextGlobal;

// Function Declarations
void funFitCos(coder::array<real_T, 2U> *center, real_T *A, real_T *B,
               real_T *C);

void funFitCos_api(const mxArray *prhs, int32_T nlhs, const mxArray *plhs[3]);

void funFitCos_atexit();

void funFitCos_initialize();

void funFitCos_terminate();

void funFitCos_xil_shutdown();

void funFitCos_xil_terminate();

#endif
//
// File trailer for _coder_funFitCos_api.h
//
// [EOF]
//

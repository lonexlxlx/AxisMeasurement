//
// File: _coder_funFitLine_api.h
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 20:35:40
//

#ifndef _CODER_FUNFITLINE_API_H
#define _CODER_FUNFITLINE_API_H

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
void funFitLine(coder::array<real_T, 2U> *xdata,
                coder::array<real_T, 2U> *ydata,
                coder::array<real_T, 2U> *zdata, real_T *x0, real_T *b_y0,
                real_T *z0, real_T *directionX, real_T *directionY,
                real_T *directionZ);

void funFitLine_api(const mxArray *const prhs[3], int32_T nlhs,
                    const mxArray *plhs[6]);

void funFitLine_atexit();

void funFitLine_initialize();

void funFitLine_terminate();

void funFitLine_xil_shutdown();

void funFitLine_xil_terminate();

#endif
//
// File trailer for _coder_funFitLine_api.h
//
// [EOF]
//

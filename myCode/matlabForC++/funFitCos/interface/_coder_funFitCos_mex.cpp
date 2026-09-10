//
// File: _coder_funFitCos_mex.cpp
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

// Include Files
#include "_coder_funFitCos_mex.h"
#include "_coder_funFitCos_api.h"

// Function Definitions
//
// Arguments    : int32_T nlhs
//                mxArray *plhs[]
//                int32_T nrhs
//                const mxArray *prhs[]
// Return Type  : void
//
void mexFunction(int32_T nlhs, mxArray *plhs[], int32_T nrhs,
                 const mxArray *prhs[])
{
  mexAtExit(&funFitCos_atexit);
  // Module initialization.
  funFitCos_initialize();
  // Dispatch the entry-point.
  unsafe_funFitCos_mexFunction(nlhs, plhs, nrhs, prhs);
  // Module termination.
  funFitCos_terminate();
}

//
// Arguments    : void
// Return Type  : emlrtCTX
//
emlrtCTX mexFunctionCreateRootTLS()
{
  emlrtCreateRootTLSR2022a(&emlrtRootTLSGlobal, &emlrtContextGlobal, nullptr, 1,
                           nullptr, "GBK", true);
  return emlrtRootTLSGlobal;
}

//
// Arguments    : int32_T nlhs
//                mxArray *plhs[3]
//                int32_T nrhs
//                const mxArray *prhs[1]
// Return Type  : void
//
void unsafe_funFitCos_mexFunction(int32_T nlhs, mxArray *plhs[3], int32_T nrhs,
                                  const mxArray *prhs[1])
{
  emlrtStack st{
      nullptr, // site
      nullptr, // tls
      nullptr  // prev
  };
  const mxArray *outputs[3];
  int32_T i;
  st.tls = emlrtRootTLSGlobal;
  // Check for proper number of arguments.
  if (nrhs != 1) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:WrongNumberOfInputs", 5, 12, 1, 4,
                        9, "funFitCos");
  }
  if (nlhs > 3) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:TooManyOutputArguments", 3, 4, 9,
                        "funFitCos");
  }
  // Call the function.
  funFitCos_api(prhs[0], nlhs, outputs);
  // Copy over outputs to the caller.
  if (nlhs < 1) {
    i = 1;
  } else {
    i = nlhs;
  }
  emlrtReturnArrays(i, &plhs[0], &outputs[0]);
}

//
// File trailer for _coder_funFitCos_mex.cpp
//
// [EOF]
//

//
// File: funFitLine.h
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 20:35:40
//

#ifndef FUNFITLINE_H
#define FUNFITLINE_H

// Include Files
#include "rtwtypes.h"
#include "coder_array.h"
#include <cstddef>
#include <cstdlib>

// Function Declarations
extern void funFitLine(const coder::array<double, 2U> &xdata,
                       const coder::array<double, 2U> &ydata,
                       const coder::array<double, 2U> &zdata, double *x0,
                       double *b_y0, double *z0, double *directionX,
                       double *directionY, double *directionZ);

#endif
//
// File trailer for funFitLine.h
//
// [EOF]
//

//
// File: funFitLine.cpp
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 20:35:40
//

// Include Files
#include "funFitLine.h"
#include "mean.h"
#include "rt_nonfinite.h"
#include "xzsvdc.h"
#include "coder_array.h"
#include <cmath>

// Function Definitions
//
// x0,y0,z0为直线上一点的坐标，direction为方向向量
// 求过直线一点坐标
//
// Arguments    : const coder::array<double, 2U> &xdata
//                const coder::array<double, 2U> &ydata
//                const coder::array<double, 2U> &zdata
//                double *x0
//                double *b_y0
//                double *z0
//                double *directionX
//                double *directionY
//                double *directionZ
// Return Type  : void
//
void funFitLine(const coder::array<double, 2U> &xdata,
                const coder::array<double, 2U> &ydata,
                const coder::array<double, 2U> &zdata, double *x0, double *b_y0,
                double *z0, double *directionX, double *directionY,
                double *directionZ)
{
  static const signed char b_V[9]{1, 0, 0, 0, 1, 0, 0, 0, 1};
  coder::array<double, 2U> U;
  coder::array<double, 2U> centeredLine;
  coder::array<double, 2U> result;
  double V[9];
  double s_data[3];
  double xyz0[3];
  int acoef;
  int i;
  boolean_T p;
  xyz0[0] = coder::mean(xdata);
  xyz0[1] = coder::mean(ydata);
  xyz0[2] = coder::mean(zdata);
  // 通过奇异值分解来计算方向向量
  result.set_size(xdata.size(1), 3);
  acoef = xdata.size(1);
  for (i = 0; i < acoef; i++) {
    result[i] = xdata[i];
  }
  acoef = ydata.size(1);
  for (i = 0; i < acoef; i++) {
    result[i + result.size(0)] = ydata[i];
  }
  acoef = zdata.size(1);
  for (i = 0; i < acoef; i++) {
    result[i + result.size(0) * 2] = zdata[i];
  }
  centeredLine.set_size(result.size(0), 3);
  if (result.size(0) != 0) {
    acoef = (result.size(0) != 1);
    for (int k{0}; k < 3; k++) {
      i = centeredLine.size(0) - 1;
      for (int b_k{0}; b_k <= i; b_k++) {
        centeredLine[b_k + centeredLine.size(0) * k] =
            result[acoef * b_k + result.size(0) * k] - xyz0[k];
      }
    }
  }
  acoef = centeredLine.size(0) * 3;
  p = true;
  for (int k{0}; k < acoef; k++) {
    if ((!p) || (std::isinf(centeredLine[k]) || std::isnan(centeredLine[k]))) {
      p = false;
    }
  }
  if (p) {
    if (centeredLine.size(0) == 0) {
      for (i = 0; i < 9; i++) {
        V[i] = b_V[i];
      }
    } else {
      coder::internal::reflapack::xzsvdc(centeredLine, U, s_data, V);
    }
  } else {
    for (i = 0; i < 9; i++) {
      V[i] = rtNaN;
    }
  }
  // 返回计算结果
  *x0 = xyz0[0];
  *b_y0 = xyz0[1];
  *z0 = xyz0[2];
  *directionX = V[0];
  *directionY = V[1];
  *directionZ = V[2];
  // {
  // function[a,b,c,x_intersection,y_intersection,z_intersection]=funFitLine(xdata,ydata,zdata)
  // x,y,z是空间点位置列表
  // A=[xdata',ydata',ones(size(xdata'))]
  // coefficients = A \ zdata';
  // a=coefficients(1);
  // b=coefficients(2);
  // c=coefficients(3);
  // t_intersection = (50 - c) / a; % 根据 z = 50 求解 t
  // x_intersection = a * t_intersection + c;
  // y_intersection = b * t_intersection + c;
  // z_intersection = 50;
  // end
  // }
}

//
// File trailer for funFitLine.cpp
//
// [EOF]
//

//
// File: driver.cpp
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 19:50:37
//

// Include Files
#include "driver.h"
#include "anonymous_function.h"
#include "anonymous_function1.h"
#include "checkStoppingCriteria.h"
#include "computeFiniteDifferences.h"
#include "factoryConstruct.h"
#include "funFitCos_data.h"
#include "funFitCos_internal_types.h"
#include "funFitCos_internal_types1.h"
#include "funFitCos_internal_types11.h"
#include "hasFiniteBounds.h"
#include "jacobianFiniteDifference.h"
#include "linearLeastSquares.h"
#include "lsqcurvefit.h"
#include "norm.h"
#include "rt_nonfinite.h"
#include "xgemv.h"
#include "coder_array.h"
#include <cmath>

// Function Declarations
namespace coder {
namespace optim {
namespace coder {
namespace levenbergMarquardt {
static double computeFirstOrderOpt(const double gradf[3],
                                   boolean_T hasFiniteBounds,
                                   const double &projSteepestDescentInfNorm);

}
} // namespace coder
} // namespace optim
} // namespace coder

// Function Definitions
//
// Arguments    : const double gradf[3]
//                boolean_T hasFiniteBounds
//                const double &projSteepestDescentInfNorm
// Return Type  : double
//
namespace coder {
namespace optim {
namespace coder {
namespace levenbergMarquardt {
static double computeFirstOrderOpt(const double gradf[3],
                                   boolean_T hasFiniteBounds,
                                   const double &projSteepestDescentInfNorm)
{
  double firstOrderOpt;
  if (hasFiniteBounds) {
    double absx;
    double b;
    b = 0.0;
    absx = std::abs(gradf[0]);
    if (std::isnan(absx) || (absx > 0.0)) {
      b = absx;
    }
    absx = std::abs(gradf[1]);
    if (std::isnan(absx) || (absx > b)) {
      b = absx;
    }
    absx = std::abs(gradf[2]);
    if (std::isnan(absx) || (absx > b)) {
      b = absx;
    }
    if ((std::abs(projSteepestDescentInfNorm - b) <
         2.2204460492503131E-16 * std::fmax(projSteepestDescentInfNorm, b)) ||
        (b == 0.0)) {
      firstOrderOpt = projSteepestDescentInfNorm;
    } else {
      firstOrderOpt =
          projSteepestDescentInfNorm * projSteepestDescentInfNorm / b;
    }
  } else {
    double absx;
    firstOrderOpt = 0.0;
    absx = std::abs(gradf[0]);
    if (std::isnan(absx) || (absx > 0.0)) {
      firstOrderOpt = absx;
    }
    absx = std::abs(gradf[1]);
    if (std::isnan(absx) || (absx > firstOrderOpt)) {
      firstOrderOpt = absx;
    }
    absx = std::abs(gradf[2]);
    if (std::isnan(absx) || (absx > firstOrderOpt)) {
      firstOrderOpt = absx;
    }
  }
  return firstOrderOpt;
}

//
// Arguments    : c_struct_T &in1
//                const double in2[3]
//                const coder::array<double, 2U> &in3
// Return Type  : void
//
} // namespace levenbergMarquardt
} // namespace coder
} // namespace optim
} // namespace coder
void binary_expand_op(c_struct_T &in1, const double in2[3],
                      const coder::array<double, 2U> &in3)
{
  double b_in2;
  double c_in2;
  int in3_idx_0;
  int stride_0_0;
  int stride_1_0;
  b_in2 = in2[0];
  c_in2 = in2[2];
  if (in3.size(1) == 1) {
    in3_idx_0 = in1.nonlin.workspace.fun.workspace.ydata.size(1);
  } else {
    in3_idx_0 = in3.size(1);
  }
  in1.cEq_1.set_size(in3_idx_0);
  stride_0_0 = (in3.size(1) != 1);
  stride_1_0 = (in1.nonlin.workspace.fun.workspace.ydata.size(1) != 1);
  for (int i{0}; i < in3_idx_0; i++) {
    in1.cEq_1[i] = (b_in2 * in3[i * stride_0_0] + c_in2) -
                   in1.nonlin.workspace.fun.workspace.ydata[i * stride_1_0];
  }
}

//
// Arguments    : const anonymous_function &fun
//                double x[3]
//                ::coder::array<double, 2U> &fCurrent
//                char output_algorithm[19]
//                double lambda_lower[3]
//                double lambda_upper[3]
//                ::coder::array<double, 2U> &jacobian
//                double &exitflag
//                double &output_iterations
//                double &output_funcCount
//                double &output_stepsize
//                double &output_firstorderopt
// Return Type  : double
//
namespace coder {
namespace optim {
namespace coder {
namespace levenbergMarquardt {
double driver(const anonymous_function &fun, double x[3],
              ::coder::array<double, 2U> &fCurrent, char output_algorithm[19],
              double lambda_lower[3], double lambda_upper[3],
              ::coder::array<double, 2U> &jacobian, double &exitflag,
              double &output_iterations, double &output_funcCount,
              double &output_stepsize, double &output_firstorderopt)
{
  static const char cv[19]{'l', 'e', 'v', 'e', 'n', 'b', 'e', 'r', 'g', '-',
                           'm', 'a', 'r', 'q', 'u', 'a', 'r', 'd', 't'};
  b_anonymous_function b_this;
  array<double, 2U> JacCeqTrans;
  array<double, 2U> augJacobian;
  array<double, 2U> b_varargout_f1;
  array<double, 2U> b_x;
  array<double, 2U> fNew;
  array<double, 2U> varargout_f1;
  array<double, 1U> rhs;
  c_struct_T FiniteDifferences;
  double dx[3];
  double gradf[3];
  double xp[3];
  double b_gamma;
  double funDiff;
  double projSteepestDescentInfNorm;
  double relFactor;
  double resnorm;
  double sqrtGamma;
  double tolActive;
  int bIdx;
  int f_temp_size_idx_1;
  int funcCount;
  int i;
  int iter;
  int m;
  int m_temp;
  int nx;
  boolean_T indActive[3];
  boolean_T exitg1;
  boolean_T hasFiniteBounds;
  boolean_T stepSuccessful;
  for (i = 0; i < 19; i++) {
    output_algorithm[i] = cv[i];
  }
  boolean_T hasLB[3];
  boolean_T hasUB[3];
  indActive[0] = false;
  dx[0] = rtInf;
  indActive[1] = false;
  dx[1] = rtInf;
  indActive[2] = false;
  dx[2] = rtInf;
  funDiff = rtInf;
  iter = 0;
  hasFiniteBounds = utils::hasFiniteBounds(hasLB, hasUB);
  x[0] = 0.004;
  x[1] = 3.1415926535897931;
  x[2] = -0.03;
  b_x.set_size(1, fun.workspace.xdata.size(1));
  nx = fun.workspace.xdata.size(1);
  for (i = 0; i < nx; i++) {
    b_x[i] = fun.workspace.xdata[i] + 3.1415926535897931;
  }
  nx = b_x.size(1);
  for (int k{0}; k < nx; k++) {
    b_x[k] = std::cos(b_x[k]);
  }
  if (b_x.size(1) == fun.workspace.ydata.size(1)) {
    varargout_f1.set_size(1, b_x.size(1));
    nx = b_x.size(1);
    for (i = 0; i < nx; i++) {
      varargout_f1[i] = (0.004 * b_x[i] - 0.03) - fun.workspace.ydata[i];
    }
  } else {
    binary_expand_op(varargout_f1, x, b_x, fun);
  }
  if (b_x.size(1) == 1) {
    f_temp_size_idx_1 = fun.workspace.ydata.size(1);
  } else {
    f_temp_size_idx_1 = b_x.size(1);
  }
  m_temp = varargout_f1.size(1);
  jacobian.set_size(f_temp_size_idx_1, 3);
  m = f_temp_size_idx_1 - 1;
  fCurrent.set_size(1, varargout_f1.size(1));
  fNew.set_size(1, varargout_f1.size(1));
  for (int b_i{0}; b_i <= m; b_i++) {
    fCurrent[b_i] = varargout_f1[b_i];
  }
  augJacobian.set_size(varargout_f1.size(1) + 3, 3);
  rhs.set_size(varargout_f1.size(1) + 3);
  i = varargout_f1.size(1) - 1;
  for (int k{0}; k < 3; k++) {
    nx = k * m_temp;
    bIdx = k * (f_temp_size_idx_1 + 3);
    for (int b_i{0}; b_i <= i; b_i++) {
      augJacobian[bIdx + b_i] = jacobian[nx + b_i];
    }
  }
  resnorm = 0.0;
  if (varargout_f1.size(1) >= 1) {
    for (int k{0}; k < f_temp_size_idx_1; k++) {
      sqrtGamma = fCurrent[k];
      resnorm += sqrtGamma * sqrtGamma;
    }
  }
  b_this.workspace.fun = fun;
  JacCeqTrans.set_size(3, fCurrent.size(1));
  utils::FiniteDifferences::factoryConstruct(b_this, f_temp_size_idx_1,
                                             FiniteDifferences);
  xp[0] = x[0];
  xp[1] = x[1];
  xp[2] = x[2];
  utils::FiniteDifferences::computeFiniteDifferences(FiniteDifferences,
                                                     fCurrent, xp, JacCeqTrans);
  nx = JacCeqTrans.size(1);
  for (int k{0}; k < 3; k++) {
    for (bIdx = 0; bIdx < nx; bIdx++) {
      augJacobian[bIdx + augJacobian.size(0) * k] = JacCeqTrans[k + 3 * bIdx];
    }
  }
  funcCount = FiniteDifferences.numEvals + 1;
  b_gamma = 0.01;
  for (int b_i{0}; b_i < 3; b_i++) {
    nx = (m_temp + 3) * (b_i + 1);
    augJacobian[nx - 3] = 0.0;
    augJacobian[nx - 2] = 0.0;
    augJacobian[nx - 1] = 0.0;
    augJacobian[(m_temp + b_i) + augJacobian.size(0) * b_i] = 0.1;
  }
  for (int k{0}; k < 3; k++) {
    nx = k * (f_temp_size_idx_1 + 3);
    bIdx = k * m_temp;
    for (int b_i{0}; b_i <= i; b_i++) {
      jacobian[bIdx + b_i] = augJacobian[nx + b_i];
    }
  }
  internal::blas::xgemv(varargout_f1.size(1), jacobian, varargout_f1.size(1),
                        fCurrent, gradf);
  projSteepestDescentInfNorm = std::fmax(
      std::fmax(
          std::fmax(0.0, std::abs(std::fmin(
                             0.5 - x[0], std::fmax(-0.5 - x[0], -gradf[0])))),
          std::abs(
              std::fmin(6.2831853071795862 - x[1],
                        std::fmax(-6.2831853071795862 - x[1], -gradf[1])))),
      std::abs(std::fmin(1.0 - x[2], std::fmax(-1.0 - x[2], -gradf[2]))));
  sqrtGamma =
      computeFirstOrderOpt(gradf, hasFiniteBounds, projSteepestDescentInfNorm);
  relFactor = std::fmax(sqrtGamma, 1.0);
  stepSuccessful = true;
  bIdx = checkStoppingCriteria(gradf, relFactor, FiniteDifferences.numEvals + 1,
                               projSteepestDescentInfNorm, hasFiniteBounds);
  exitg1 = false;
  while ((!exitg1) && (bIdx == -5)) {
    boolean_T b_guard1{false};
    boolean_T evalOK;
    b_x.set_size(1, fCurrent.size(1));
    nx = fCurrent.size(1);
    for (int k{0}; k < nx; k++) {
      b_x[k] = -fCurrent[k];
    }
    for (int k{0}; k <= m; k++) {
      rhs[k] = b_x[k];
    }
    rhs[f_temp_size_idx_1] = 0.0;
    rhs[f_temp_size_idx_1 + 1] = 0.0;
    rhs[f_temp_size_idx_1 + 2] = 0.0;
    if (hasFiniteBounds) {
      projSteepestDescentInfNorm = std::fmax(
          std::fmax(
              std::fmax(0.0, std::abs(std::fmin(
                                 0.5 - x[0],
                                 std::fmax(-0.5 - x[0],
                                           -gradf[0] / (b_gamma + 1.0))))),
              std::abs(std::fmin(6.2831853071795862 - x[1],
                                 std::fmax(-6.2831853071795862 - x[1],
                                           -gradf[1] / (b_gamma + 1.0))))),
          std::abs(
              std::fmin(1.0 - x[2],
                        std::fmax(-1.0 - x[2], -gradf[2] / (b_gamma + 1.0)))));
      tolActive = std::fmin(projSteepestDescentInfNorm, 0.5);
      for (int b_i{0}; b_i < 3; b_i++) {
        boolean_T guard1{false};
        sqrtGamma = x[b_i];
        guard1 = false;
        if ((sqrtGamma - dv[b_i] <= tolActive) && (gradf[b_i] > 0.0)) {
          guard1 = true;
        } else {
          indActive[b_i] = false;
          if ((dv1[b_i] - sqrtGamma <= tolActive) && (gradf[b_i] < 0.0)) {
            guard1 = true;
          } else {
            indActive[b_i] = false;
          }
        }
        if (guard1) {
          indActive[b_i] = true;
          nx = (m_temp + 3) * b_i;
          for (int k{0}; k <= m; k++) {
            augJacobian[nx + k] = 0.0;
          }
        }
      }
    }
    linearLeastSquares(augJacobian, rhs, dx, m_temp + 3);
    if (hasFiniteBounds) {
      if (indActive[0]) {
        dx[0] = -gradf[0] / (b_gamma + 1.0);
      }
      xp[0] = std::fmin(0.5, std::fmax(-0.5, x[0] + dx[0]));
      if (indActive[1]) {
        dx[1] = -gradf[1] / (b_gamma + 1.0);
      }
      xp[1] = std::fmin(6.2831853071795862,
                        std::fmax(-6.2831853071795862, x[1] + dx[1]));
      if (indActive[2]) {
        dx[2] = -gradf[2] / (b_gamma + 1.0);
      }
      xp[2] = std::fmin(1.0, std::fmax(-1.0, x[2] + dx[2]));
    } else {
      xp[0] = x[0] + dx[0];
      xp[1] = x[1] + dx[1];
      xp[2] = x[2] + dx[2];
    }
    b_x.set_size(1, fun.workspace.xdata.size(1));
    nx = fun.workspace.xdata.size(1);
    for (int k{0}; k < nx; k++) {
      b_x[k] = fun.workspace.xdata[k] + xp[1];
    }
    nx = b_x.size(1);
    for (int k{0}; k < nx; k++) {
      b_x[k] = std::cos(b_x[k]);
    }
    if (b_x.size(1) == fun.workspace.ydata.size(1)) {
      b_varargout_f1.set_size(1, b_x.size(1));
      nx = b_x.size(1);
      for (int k{0}; k < nx; k++) {
        b_varargout_f1[k] = (xp[0] * b_x[k] + xp[2]) - fun.workspace.ydata[k];
      }
    } else {
      binary_expand_op(b_varargout_f1, xp, b_x, fun);
    }
    for (int b_i{0}; b_i <= m; b_i++) {
      fNew[b_i] = b_varargout_f1[b_i];
    }
    tolActive = 0.0;
    if (m_temp >= 1) {
      for (int k{0}; k <= m; k++) {
        sqrtGamma = fNew[k];
        tolActive += sqrtGamma * sqrtGamma;
      }
    }
    evalOK = true;
    for (int b_i{0}; b_i < m_temp; b_i++) {
      if (evalOK) {
        sqrtGamma = fNew[b_i];
        if (std::isinf(sqrtGamma) || std::isnan(sqrtGamma)) {
          evalOK = false;
        }
      } else {
        evalOK = false;
      }
    }
    funcCount++;
    b_guard1 = false;
    if ((tolActive < resnorm) && evalOK) {
      iter++;
      funDiff = std::abs(tolActive - resnorm) / resnorm;
      resnorm = tolActive;
      fCurrent.set_size(1, fNew.size(1));
      nx = fNew.size(1);
      for (int k{0}; k < nx; k++) {
        fCurrent[k] = fNew[k];
      }
      evalOK = jacobianFiniteDifference(augJacobian, fCurrent, funcCount, xp,
                                        FiniteDifferences);
      for (int k{0}; k < 3; k++) {
        nx = k * (f_temp_size_idx_1 + 3);
        bIdx = k * m_temp;
        for (int b_i{0}; b_i <= i; b_i++) {
          jacobian[bIdx + b_i] = augJacobian[nx + b_i];
        }
      }
      if (evalOK) {
        x[0] = xp[0];
        x[1] = xp[1];
        x[2] = xp[2];
        if (stepSuccessful) {
          b_gamma *= 0.1;
        }
        stepSuccessful = true;
        b_guard1 = true;
      } else {
        bIdx = 2;
        nx = m_temp * 3;
        for (int k{0}; k < nx; k++) {
          jacobian[k] = rtNaN;
        }
        exitg1 = true;
      }
    } else {
      b_gamma *= 10.0;
      stepSuccessful = false;
      for (int k{0}; k < 3; k++) {
        nx = k * m_temp;
        bIdx = k * (f_temp_size_idx_1 + 3);
        for (int b_i{0}; b_i <= i; b_i++) {
          augJacobian[bIdx + b_i] = jacobian[nx + b_i];
        }
      }
      b_guard1 = true;
    }
    if (b_guard1) {
      sqrtGamma = std::sqrt(b_gamma);
      for (int b_i{0}; b_i < 3; b_i++) {
        nx = (m_temp + 3) * (b_i + 1);
        augJacobian[nx - 3] = 0.0;
        augJacobian[nx - 2] = 0.0;
        augJacobian[nx - 1] = 0.0;
        augJacobian[(m_temp + b_i) + augJacobian.size(0) * b_i] = sqrtGamma;
      }
      internal::blas::xgemv(m_temp, jacobian, m_temp, fCurrent, gradf);
      projSteepestDescentInfNorm = std::fmax(
          std::fmax(std::fmax(0.0, std::abs(std::fmin(
                                       0.5 - x[0],
                                       std::fmax(-0.5 - x[0], -gradf[0])))),
                    std::abs(std::fmin(
                        6.2831853071795862 - x[1],
                        std::fmax(-6.2831853071795862 - x[1], -gradf[1])))),
          std::abs(std::fmin(1.0 - x[2], std::fmax(-1.0 - x[2], -gradf[2]))));
      bIdx = b_checkStoppingCriteria(
          gradf, relFactor, funDiff, x, dx, funcCount, stepSuccessful, iter,
          projSteepestDescentInfNorm, hasFiniteBounds);
      if (bIdx != -5) {
        exitg1 = true;
      }
    }
  }
  output_firstorderopt =
      computeFirstOrderOpt(gradf, hasFiniteBounds, projSteepestDescentInfNorm);
  lambda_lower[0] = 0.0;
  lambda_upper[0] = 0.0;
  lambda_lower[1] = 0.0;
  lambda_upper[1] = 0.0;
  lambda_lower[2] = 0.0;
  lambda_upper[2] = 0.0;
  if (hasFiniteBounds) {
    tolActive = std::fmin(
        std::fmax(
            std::fmax(
                std::fmax(0.0, std::abs(std::fmin(
                                   0.5 - x[0],
                                   std::fmax(-0.5 - x[0],
                                             -gradf[0] / (b_gamma + 1.0))))),
                std::abs(std::fmin(6.2831853071795862 - x[1],
                                   std::fmax(-6.2831853071795862 - x[1],
                                             -gradf[1] / (b_gamma + 1.0))))),
            std::abs(
                std::fmin(1.0 - x[2], std::fmax(-1.0 - x[2],
                                                -gradf[2] / (b_gamma + 1.0))))),
        0.5);
    if ((x[0] - -0.5 <= tolActive) && (gradf[0] > 0.0)) {
      lambda_lower[0] = 2.0 * gradf[0];
    }
    if ((0.5 - x[0] <= tolActive) && (gradf[0] < 0.0)) {
      lambda_upper[0] = -2.0 * gradf[0];
    }
    if ((x[1] - -6.2831853071795862 <= tolActive) && (gradf[1] > 0.0)) {
      lambda_lower[1] = 2.0 * gradf[1];
    }
    if ((6.2831853071795862 - x[1] <= tolActive) && (gradf[1] < 0.0)) {
      lambda_upper[1] = -2.0 * gradf[1];
    }
    if ((x[2] - -1.0 <= tolActive) && (gradf[2] > 0.0)) {
      lambda_lower[2] = 2.0 * gradf[2];
    }
    if ((1.0 - x[2] <= tolActive) && (gradf[2] < 0.0)) {
      lambda_upper[2] = -2.0 * gradf[2];
    }
  }
  exitflag = bIdx;
  output_iterations = iter;
  output_funcCount = funcCount;
  output_stepsize = b_norm(dx);
  return resnorm;
}

} // namespace levenbergMarquardt
} // namespace coder
} // namespace optim
} // namespace coder

//
// File trailer for driver.cpp
//
// [EOF]
//

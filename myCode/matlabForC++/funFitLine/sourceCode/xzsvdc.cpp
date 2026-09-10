//
// File: xzsvdc.cpp
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 20:35:40
//

// Include Files
#include "xzsvdc.h"
#include "rt_nonfinite.h"
#include "xaxpy.h"
#include "xdotc.h"
#include "xnrm2.h"
#include "xrot.h"
#include "xrotg.h"
#include "xswap.h"
#include "coder_array.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Function Definitions
//
// Arguments    : ::coder::array<double, 2U> &A
//                ::coder::array<double, 2U> &U
//                double S_data[]
//                double V[9]
// Return Type  : int
//
namespace coder {
namespace internal {
namespace reflapack {
int xzsvdc(::coder::array<double, 2U> &A, ::coder::array<double, 2U> &U,
           double S_data[], double V[9])
{
  array<double, 1U> work;
  double e[3];
  double s_data[3];
  double b;
  double f;
  double rt;
  double scale;
  int S_size;
  int i;
  int n;
  int ns;
  n = A.size(0);
  ns = A.size(0) + 1;
  if (ns > 3) {
    ns = 3;
  }
  S_size = A.size(0);
  if (S_size > 3) {
    S_size = 3;
  }
  if (ns - 1 >= 0) {
    std::memset(&s_data[0], 0, static_cast<unsigned int>(ns) * sizeof(double));
  }
  e[0] = 0.0;
  e[1] = 0.0;
  e[2] = 0.0;
  work.set_size(A.size(0));
  ns = A.size(0);
  for (i = 0; i < ns; i++) {
    work[i] = 0.0;
  }
  U.set_size(A.size(0), A.size(0));
  ns = A.size(0) * A.size(0);
  for (i = 0; i < ns; i++) {
    U[i] = 0.0;
  }
  std::memset(&V[0], 0, 9U * sizeof(double));
  if (A.size(0) == 0) {
    V[0] = 1.0;
    V[4] = 1.0;
    V[8] = 1.0;
  } else {
    double nrm;
    double snorm;
    int ii;
    int iter;
    int m;
    int mm;
    int nct;
    int nctp1;
    int qjj;
    int qp1;
    int qq;
    nct = A.size(0) - 1;
    if (nct > 3) {
      nct = 3;
    }
    nctp1 = nct + 1;
    if (nct >= 1) {
      i = nct;
    } else {
      i = 1;
    }
    for (mm = 0; mm < i; mm++) {
      boolean_T apply_transform;
      qp1 = mm + 2;
      qq = (mm + n * mm) + 1;
      ns = (n - mm) - 1;
      apply_transform = false;
      if (mm + 1 <= nct) {
        nrm = blas::xnrm2(ns + 1, A, qq);
        if (nrm > 0.0) {
          apply_transform = true;
          if (A[qq - 1] < 0.0) {
            nrm = -nrm;
          }
          s_data[mm] = nrm;
          if (std::abs(nrm) >= 1.0020841800044864E-292) {
            nrm = 1.0 / nrm;
            qjj = qq + ns;
            for (int k{qq}; k <= qjj; k++) {
              A[k - 1] = nrm * A[k - 1];
            }
          } else {
            qjj = qq + ns;
            for (int k{qq}; k <= qjj; k++) {
              A[k - 1] = A[k - 1] / s_data[mm];
            }
          }
          A[qq - 1] = A[qq - 1] + 1.0;
          s_data[mm] = -s_data[mm];
        } else {
          s_data[mm] = 0.0;
        }
      }
      for (iter = qp1; iter < 4; iter++) {
        qjj = mm + n * (iter - 1);
        if (apply_transform) {
          nrm = 0.0;
          if (ns + 1 >= 1) {
            for (int k{0}; k <= ns; k++) {
              nrm += A[(qq + k) - 1] * A[qjj + k];
            }
          }
          nrm = -(nrm / A[mm + A.size(0) * mm]);
          blas::xaxpy(ns + 1, nrm, qq, A, qjj + 1);
        }
        e[iter - 1] = A[qjj];
      }
      if (mm + 1 <= nct) {
        for (ii = mm + 1; ii <= n; ii++) {
          U[(ii + U.size(0) * mm) - 1] = A[(ii + A.size(0) * mm) - 1];
        }
      }
      if (mm + 1 <= 1) {
        nrm = blas::xnrm2(e);
        if (nrm == 0.0) {
          e[0] = 0.0;
        } else {
          if (e[1] < 0.0) {
            e[0] = -nrm;
          } else {
            e[0] = nrm;
          }
          nrm = e[0];
          if (std::abs(e[0]) >= 1.0020841800044864E-292) {
            nrm = 1.0 / e[0];
            for (int k{qp1}; k < 4; k++) {
              e[k - 1] *= nrm;
            }
          } else {
            for (int k{qp1}; k < 4; k++) {
              e[k - 1] /= nrm;
            }
          }
          e[1]++;
          e[0] = -e[0];
          if (n >= 2) {
            for (ii = qp1; ii <= n; ii++) {
              work[ii - 1] = 0.0;
            }
            for (iter = qp1; iter < 4; iter++) {
              blas::xaxpy(ns, e[iter - 1], A, n * (iter - 1) + 2, work);
            }
            for (iter = qp1; iter < 4; iter++) {
              blas::xaxpy(ns, -e[iter - 1] / e[1], work, A, n * (iter - 1) + 2);
            }
          }
        }
        for (ii = qp1; ii < 4; ii++) {
          V[ii - 1] = e[ii - 1];
        }
      }
    }
    if (A.size(0) + 1 >= 3) {
      m = 3;
    } else {
      m = A.size(0) + 1;
    }
    if (nct < 3) {
      s_data[nct] = A[nct + A.size(0) * nct];
    }
    if (A.size(0) < m) {
      s_data[m - 1] = 0.0;
    }
    if (m > 2) {
      e[1] = A[A.size(0) * (m - 1) + 1];
    }
    e[m - 1] = 0.0;
    if (nct + 1 <= A.size(0)) {
      for (iter = nctp1; iter <= n; iter++) {
        for (ii = 0; ii < n; ii++) {
          U[ii + U.size(0) * (iter - 1)] = 0.0;
        }
        U[(iter + U.size(0) * (iter - 1)) - 1] = 1.0;
      }
    }
    for (mm = nct; mm >= 1; mm--) {
      qp1 = mm + 1;
      ns = n - mm;
      qq = (mm + n * (mm - 1)) - 1;
      if (s_data[mm - 1] != 0.0) {
        for (iter = qp1; iter <= n; iter++) {
          qjj = mm + n * (iter - 1);
          nrm = 0.0;
          if (ns + 1 >= 1) {
            for (int k{0}; k <= ns; k++) {
              nrm += U[qq + k] * U[(qjj + k) - 1];
            }
          }
          nrm = -(nrm / U[qq]);
          blas::xaxpy(ns + 1, nrm, qq + 1, U, qjj);
        }
        for (ii = mm; ii <= n; ii++) {
          U[(ii + U.size(0) * (mm - 1)) - 1] =
              -U[(ii + U.size(0) * (mm - 1)) - 1];
        }
        U[qq] = U[qq] + 1.0;
        for (ii = 0; ii <= mm - 2; ii++) {
          U[ii + U.size(0) * (mm - 1)] = 0.0;
        }
      } else {
        for (ii = 0; ii < n; ii++) {
          U[ii + U.size(0) * (mm - 1)] = 0.0;
        }
        U[qq] = 1.0;
      }
    }
    for (mm = 2; mm >= 0; mm--) {
      if ((mm + 1 <= 1) && (e[0] != 0.0)) {
        blas::xaxpy(-(blas::xdotc(V, V, 5) / V[1]), V, 5);
        blas::xaxpy(-(blas::xdotc(V, V, 8) / V[1]), V, 8);
      }
      V[3 * mm] = 0.0;
      V[3 * mm + 1] = 0.0;
      V[3 * mm + 2] = 0.0;
      V[mm + 3 * mm] = 1.0;
    }
    i = static_cast<unsigned char>(m);
    for (mm = 0; mm < i; mm++) {
      nrm = s_data[mm];
      if (nrm != 0.0) {
        rt = std::abs(nrm);
        nrm /= rt;
        s_data[mm] = rt;
        if (mm + 1 < m) {
          e[mm] /= nrm;
        }
        if (mm + 1 <= n) {
          ns = n * mm;
          qjj = ns + n;
          for (int k{ns + 1}; k <= qjj; k++) {
            U[k - 1] = nrm * U[k - 1];
          }
        }
      }
      if (mm + 1 < m) {
        nrm = e[mm];
        if (nrm != 0.0) {
          rt = std::abs(nrm);
          nrm = rt / nrm;
          e[mm] = rt;
          s_data[mm + 1] *= nrm;
          ns = 3 * (mm + 1);
          qjj = ns + 3;
          for (int k{ns + 1}; k <= qjj; k++) {
            V[k - 1] *= nrm;
          }
        }
      }
    }
    mm = m;
    iter = 0;
    snorm = 0.0;
    for (ii = 0; ii < i; ii++) {
      snorm =
          std::fmax(snorm, std::fmax(std::abs(s_data[ii]), std::abs(e[ii])));
    }
    while ((m > 0) && (iter < 75)) {
      boolean_T exitg1;
      nctp1 = m - 1;
      ii = m - 1;
      exitg1 = false;
      while (!(exitg1 || (ii == 0))) {
        nrm = std::abs(e[ii - 1]);
        if ((nrm <= 2.2204460492503131E-16 *
                        (std::abs(s_data[ii - 1]) + std::abs(s_data[ii]))) ||
            (nrm <= 1.0020841800044864E-292) ||
            ((iter > 20) && (nrm <= 2.2204460492503131E-16 * snorm))) {
          e[ii - 1] = 0.0;
          exitg1 = true;
        } else {
          ii--;
        }
      }
      if (ii == m - 1) {
        ns = 4;
      } else {
        qjj = m;
        ns = m;
        exitg1 = false;
        while ((!exitg1) && (ns >= ii)) {
          qjj = ns;
          if (ns == ii) {
            exitg1 = true;
          } else {
            nrm = 0.0;
            if (ns < m) {
              nrm = std::abs(e[ns - 1]);
            }
            if (ns > ii + 1) {
              nrm += std::abs(e[ns - 2]);
            }
            rt = std::abs(s_data[ns - 1]);
            if ((rt <= 2.2204460492503131E-16 * nrm) ||
                (rt <= 1.0020841800044864E-292)) {
              s_data[ns - 1] = 0.0;
              exitg1 = true;
            } else {
              ns--;
            }
          }
        }
        if (qjj == ii) {
          ns = 3;
        } else if (qjj == m) {
          ns = 1;
        } else {
          ns = 2;
          ii = qjj;
        }
      }
      switch (ns) {
      case 1: {
        f = e[m - 2];
        e[m - 2] = 0.0;
        for (int k{nctp1}; k >= ii + 1; k--) {
          double sqds;
          sqds = blas::xrotg(s_data[k - 1], f, scale);
          if (k > ii + 1) {
            f = -scale * e[0];
            e[0] *= sqds;
          }
          blas::xrot(V, 3 * (k - 1) + 1, 3 * (m - 1) + 1, sqds, scale);
        }
      } break;
      case 2: {
        f = e[ii - 1];
        e[ii - 1] = 0.0;
        for (int k{ii + 1}; k <= m; k++) {
          double sqds;
          sqds = blas::xrotg(s_data[k - 1], f, scale);
          b = e[k - 1];
          f = -scale * b;
          e[k - 1] = b * sqds;
          if (n >= 1) {
            nct = n * (k - 1);
            qq = n * (ii - 1);
            for (qp1 = 0; qp1 < n; qp1++) {
              ns = qq + qp1;
              nrm = U[ns];
              qjj = nct + qp1;
              rt = U[qjj];
              U[ns] = sqds * nrm - scale * rt;
              U[qjj] = sqds * rt + scale * nrm;
            }
          }
        }
      } break;
      case 3: {
        double sqds;
        nrm = s_data[m - 1];
        rt = s_data[m - 2];
        b = e[m - 2];
        scale = std::fmax(
            std::fmax(
                std::fmax(std::fmax(std::abs(nrm), std::abs(rt)), std::abs(b)),
                std::abs(s_data[ii])),
            std::abs(e[ii]));
        f = nrm / scale;
        nrm = rt / scale;
        rt = b / scale;
        sqds = s_data[ii] / scale;
        b = ((nrm + f) * (nrm - f) + rt * rt) / 2.0;
        nrm = f * rt;
        nrm *= nrm;
        if ((b != 0.0) || (nrm != 0.0)) {
          rt = std::sqrt(b * b + nrm);
          if (b < 0.0) {
            rt = -rt;
          }
          rt = nrm / (b + rt);
        } else {
          rt = 0.0;
        }
        f = (sqds + f) * (sqds - f) + rt;
        b = sqds * (e[ii] / scale);
        for (int k{ii + 1}; k <= nctp1; k++) {
          sqds = blas::xrotg(f, b, scale);
          if (k > ii + 1) {
            e[0] = f;
          }
          nrm = e[k - 1];
          b = s_data[k - 1];
          e[k - 1] = sqds * nrm - scale * b;
          rt = scale * s_data[k];
          s_data[k] *= sqds;
          blas::xrot(V, 3 * (k - 1) + 1, 3 * k + 1, sqds, scale);
          s_data[k - 1] = sqds * b + scale * nrm;
          sqds = blas::xrotg(s_data[k - 1], rt, scale);
          b = e[k - 1];
          f = sqds * b + scale * s_data[k];
          s_data[k] = -scale * b + sqds * s_data[k];
          b = scale * e[k];
          e[k] *= sqds;
          if (k < n) {
            nct = n * (k - 1);
            qq = n * k;
            for (qp1 = 0; qp1 < n; qp1++) {
              ns = qq + qp1;
              nrm = U[ns];
              qjj = nct + qp1;
              rt = U[qjj];
              U[ns] = sqds * nrm - scale * rt;
              U[qjj] = sqds * rt + scale * nrm;
            }
          }
        }
        e[m - 2] = f;
        iter++;
      } break;
      default:
        if (s_data[ii] < 0.0) {
          s_data[ii] = -s_data[ii];
          ns = 3 * ii;
          i = ns + 3;
          for (int k{ns + 1}; k <= i; k++) {
            V[k - 1] = -V[k - 1];
          }
        }
        qp1 = ii + 1;
        while ((ii + 1 < mm) && (s_data[ii] < s_data[qp1])) {
          rt = s_data[ii];
          s_data[ii] = s_data[qp1];
          s_data[qp1] = rt;
          blas::xswap(V, 3 * ii + 1, 3 * (ii + 1) + 1);
          if (ii + 1 < n) {
            nct = n * ii;
            qq = n * (ii + 1);
            for (int k{0}; k < n; k++) {
              ns = nct + k;
              nrm = U[ns];
              i = qq + k;
              U[ns] = U[i];
              U[i] = nrm;
            }
          }
          ii = qp1;
          qp1++;
        }
        iter = 0;
        m--;
        break;
      }
    }
  }
  if (S_size - 1 >= 0) {
    std::copy(&s_data[0], &s_data[S_size], &S_data[0]);
  }
  return S_size;
}

} // namespace reflapack
} // namespace internal
} // namespace coder

//
// File trailer for xzsvdc.cpp
//
// [EOF]
//

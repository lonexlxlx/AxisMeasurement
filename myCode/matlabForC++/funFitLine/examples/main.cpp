//
// File: main.cpp
//
// MATLAB Coder version            : 5.6
// C/C++ source code generated on  : 24-Jul-2023 20:35:40
//

/*************************************************************************/
/* This automatically generated example C++ main file shows how to call  */
/* entry-point functions that MATLAB Coder generated. You must customize */
/* this file for your application. Do not modify this file directly.     */
/* Instead, make a copy of this file, modify it, and integrate it into   */
/* your development environment.                                         */
/*                                                                       */
/* This file initializes entry-point function arguments to a default     */
/* size and value before calling the entry-point functions. It does      */
/* not store or use any values returned from the entry-point functions.  */
/* If necessary, it does pre-allocate memory for returned values.        */
/* You can use this file as a starting point for a main function that    */
/* you can deploy in your application.                                   */
/*                                                                       */
/* After you copy the file, and before you deploy it, you must make the  */
/* following changes:                                                    */
/* * For variable-size function arguments, change the example sizes to   */
/* the sizes that your application requires.                             */
/* * Change the example values of function arguments to the values that  */
/* your application requires.                                            */
/* * If the entry-point functions return values, store these values or   */
/* otherwise use them as required by your application.                   */
/*                                                                       */
/*************************************************************************/

// Include Files
#include "main.h"
#include "funFitLine.h"
#include "funFitLine_terminate.h"
#include "rt_nonfinite.h"
#include "coder_array.h"

// Function Declarations
static coder::array<double, 2U> argInit_1xUnbounded_real_T();

static double argInit_real_T();

// Function Definitions
//
// Arguments    : void
// Return Type  : coder::array<double, 2U>
//
static coder::array<double, 2U> argInit_1xUnbounded_real_T()
{
  coder::array<double, 2U> result;
  // Set the size of the array.
  // Change this size to the value that the application requires.
  result.set_size(1, 2);
  // Loop over the array to initialize each element.
  for (int idx0{0}; idx0 < 1; idx0++) {
    for (int idx1{0}; idx1 < result.size(1); idx1++) {
      // Set the value of the array element.
      // Change this value to the value that the application requires.
      result[idx1] = argInit_real_T();
    }
  }
  return result;
}

//
// Arguments    : void
// Return Type  : double
//
static double argInit_real_T()
{
  return 0.0;
}

//
// Arguments    : int argc
//                char **argv
// Return Type  : int
//
void listtest(int b[]){
  b[0]++;
  b[1] = 9;
};

int main(int, char **)
{
  // The initialize function is being called automatically from your entry-point
  // function. So, a call to initialize is not included here. Invoke the
  // entry-point functions.
  // You can call entry-point functions multiple times.
  main_funFitLine();
  int a[3] = {1, 2, 3};
  listtest(a);
  for (int i = 0; i < 3; i++) {
    cout << a[i] << endl;
  };
  
  // Terminate the application.
  // You do not need to do this more than one time.
  funFitLine_terminate();
  return 0;
}

//
// Arguments    : void
// Return Type  : void
//
void main_funFitLine()
{
  coder::array<double, 2U> xdata_tmp1;
  coder::array<double, 2U> xdata_tmp2;
  coder::array<double, 2U> xdata_tmp3;

  xdata_tmp1.set_size(1, 6);
  xdata_tmp1[0] = 2222.015;
  xdata_tmp1[1] = 1488.935 ;
  xdata_tmp1[2] = 1479.015; 
  xdata_tmp1[3] = 1608.675; 
  xdata_tmp1[4] = 1747.542;
  xdata_tmp1[5] = 1754.288;

  xdata_tmp2.set_size(1, 6);
  xdata_tmp2[0] = 477.055;
  xdata_tmp2[1] = 470.907;
  xdata_tmp2[2] = 470.803;
  xdata_tmp2[3] = 471.967;
  xdata_tmp2[4] = 473.029;
  xdata_tmp2[5] = 473.099;

  xdata_tmp3.set_size(1, 6);
  xdata_tmp3[0] = -423.390;
  xdata_tmp3[1] = -500.077;
  xdata_tmp3[2] = -501.172;
  xdata_tmp3[3] = -487.638;
  xdata_tmp3[4] = -473.021;
  xdata_tmp3[5] = -472.336;

  double b_y0;
  double directionX;
  double directionY;
  double directionZ;
  double x0;
  double z0;
  // Initialize function 'funFitLine' input arguments.
  // Initialize function input argument 'xdata'.
  //xdata_tmp = argInit_1xUnbounded_real_T();
  // Initialize function input argument 'ydata'.
  // Initialize function input argument 'zdata'.
  // Call the entry-point 'funFitLine'.
  funFitLine(xdata_tmp1, xdata_tmp2, xdata_tmp3, &x0, &b_y0, &z0, &directionX,
             &directionY, &directionZ);
  cout << "direction" << directionX << " " << directionY  << "  "
       << directionZ  << endl;
  cout << "0point" << x0 << " " << b_y0 << "  " << z0
       << endl;

}

//
// File trailer for main.cpp
//
// [EOF]
//

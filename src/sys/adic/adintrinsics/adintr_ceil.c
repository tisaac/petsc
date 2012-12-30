/*
  macro expansion:
  function_driver -> adintr_ceil
  exception number -> ADINTR_CEIL
  exceptional code ->
*fx = ADIntr_Partials[ADINTR_CEIL][ADINTR_FX];

  */

#include <stdarg.h>
#include <adintrinsics.h>
#include <knr-compat.h>
#if defined(__cplusplus)
extern "C" {
#endif

/* #include "report-once.h" */
void reportonce_accumulate Proto((int,int,int));


void
adintr_ceil (int deriv_order, int file_number, int line_number,
		 double*fx,...)
{
     /* Hack to make assignments to (*fxx) et alia OK, regardless */
     double scratch;
     double *fxx = &scratch;

     const int exception = ADINTR_CEIL;

     va_list argptr;
     va_start(argptr,fx);

     if (deriv_order == 2)
     {
	  fxx = va_arg(argptr, double *);
     }

     /* Here is where exceptional partials should be set. */
     *fx = ADIntr_Partials[ADINTR_CEIL][ADINTR_FX];
     *fxx = ADIntr_Partials[ADINTR_CEIL][ADINTR_FXX];


     /* Here is where we perform the action appropriate to the current mode. */
     if (ADIntr_Mode == ADINTR_REPORTONCE)
     {
	  reportonce_accumulate(file_number, line_number, exception);
     }

     va_end(argptr);
}
#if defined(__cplusplus)
}
#endif


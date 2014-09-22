/* mathALib.s - C-callable floating-point math entry points */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history 
--------------------
01d,05sep01,hdn  added FUNC/FUNC_LABEL GTEXT/GDATA macros
		 replaced .align with .balign
		 removed cosh, sinh, tanh for one in libc/math
01c,16jun93,hdn  updated to 5.1.
01b,14oct92,hdn  aligned all functions.
01a,16sep92,hdn  written by modifying Tron's mathALib.s
*/

/*
DESCRIPTION
This library provides a C interface to the high-level math functions,
provided either by an i80387 floating-point coprocessor or
a software floating point emulation library.  The appropriate function
is called, based on whether mathHardInit() or mathSoftInit() has been
called.

All angle-related parameters and return values are expressed in radians.  
Functions capable of errors will set errno upon an error. All functions
included in this library whos names correspond to the ANSI C specification
are, indeed, ANSI-compatable. In the spirit of ANSI, HUGE_VAL is now
supported.

SEE ALSO:
fppLib (1), floatLib (1), The C Programming Language - Second Edition

INCLUDE FILE: math.h

*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "errno.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)
	

	/* externals */

	.globl FUNC(logMsg)


	/* internals */

        .globl  GTEXT(acos)		/* double-precision functions */
        .globl  GTEXT(asin)
        .globl  GTEXT(atan)
	.globl  GTEXT(atan2)
	.globl  GTEXT(cbrt)
	.globl  GTEXT(ceil)
        .globl  GTEXT(cos)
        .globl  GTEXT(exp)
        .globl  GTEXT(fabs)
	.globl  GTEXT(floor)
	.globl  GTEXT(fmod)
	.globl	GTEXT(hypot)
	.globl  GTEXT(infinity)
	.globl  GTEXT(irint)
	.globl  GTEXT(iround)
        .globl  GTEXT(log)
        .globl  GTEXT(log2)
        .globl  GTEXT(log10)
        .globl  GTEXT(pow)
	.globl  GTEXT(round)
        .globl  GTEXT(sin)
	.globl  GTEXT(sincos)
        .globl  GTEXT(sqrt)
        .globl  GTEXT(tan)
	.globl  GTEXT(trunc)

        .globl  GTEXT(facos)		/* single-precision functions */
        .globl  GTEXT(fasin)
        .globl  GTEXT(fatan)
	.globl  GTEXT(fatan2)
	.globl  GTEXT(fcbrt)
	.globl  GTEXT(fceil)
        .globl  GTEXT(fcos)
        .globl  GTEXT(fcosh)
        .globl  GTEXT(fexp)
        .globl  GTEXT(ffabs)
	.globl  GTEXT(ffloor)
	.globl  GTEXT(ffmod)
	.globl	GTEXT(fhypot)
	.globl  GTEXT(finfinity)
	.globl  GTEXT(firint)
	.globl  GTEXT(firound)
        .globl  GTEXT(flog)
        .globl  GTEXT(flog2)
        .globl  GTEXT(flog10)
        .globl  GTEXT(fpow)
	.globl  GTEXT(fround)
        .globl  GTEXT(fsin)
	.globl  GTEXT(fsincos)
        .globl  GTEXT(fsinh)
        .globl  GTEXT(fsqrt)
        .globl  GTEXT(ftan)
        .globl  GTEXT(ftanh)
	.globl  GTEXT(ftrunc)

	.globl	GTEXT(mathErrNoInit)	/* default routine (log error msg) */

	.globl  GDATA(mathAcosFunc)	/* double-precision function pointers */
	.globl  GDATA(mathAsinFunc)
	.globl  GDATA(mathAtanFunc)
	.globl  GDATA(mathAtan2Func)
	.globl  GDATA(mathCbrtFunc)
	.globl  GDATA(mathCeilFunc)
	.globl  GDATA(mathCosFunc)
	.globl  GDATA(mathCoshFunc)
	.globl  GDATA(mathExpFunc)
	.globl  GDATA(mathFabsFunc)
	.globl  GDATA(mathFloorFunc)
	.globl  GDATA(mathFmodFunc)
	.globl  GDATA(mathHypotFunc)
	.globl  GDATA(mathInfinityFunc)
	.globl  GDATA(mathIrintFunc)
	.globl  GDATA(mathIroundFunc)
	.globl  GDATA(mathLogFunc)
	.globl  GDATA(mathLog2Func)
	.globl  GDATA(mathLog10Func)
	.globl  GDATA(mathPowFunc)
	.globl  GDATA(mathRoundFunc)
	.globl  GDATA(mathSinFunc)
	.globl  GDATA(mathSincosFunc)
	.globl  GDATA(mathSinhFunc)
	.globl  GDATA(mathSqrtFunc)
	.globl  GDATA(mathTanFunc)
	.globl  GDATA(mathTanhFunc)
	.globl  GDATA(mathTruncFunc)

	.globl  GDATA(mathFacosFunc)	/* single-precision function pointers */
	.globl  GDATA(mathFasinFunc)
	.globl  GDATA(mathFatanFunc)
	.globl  GDATA(mathFatan2Func)
	.globl  GDATA(mathFcbrtFunc)
	.globl  GDATA(mathFceilFunc)
	.globl  GDATA(mathFcosFunc)
	.globl  GDATA(mathFcoshFunc)
	.globl  GDATA(mathFexpFunc)
	.globl  GDATA(mathFfabsFunc)
	.globl  GDATA(mathFfloorFunc)
	.globl  GDATA(mathFfmodFunc)
	.globl  GDATA(mathFhypotFunc)
	.globl  GDATA(mathFinfinityFunc)
	.globl  GDATA(mathFirintFunc)
	.globl  GDATA(mathFiroundFunc)
	.globl  GDATA(mathFlogFunc)
	.globl  GDATA(mathFlog2Func)
	.globl  GDATA(mathFlog10Func)
	.globl  GDATA(mathFpowFunc)
	.globl  GDATA(mathFroundFunc)
	.globl  GDATA(mathFsinFunc)
	.globl  GDATA(mathFsincosFunc)
	.globl  GDATA(mathFsinhFunc)
	.globl  GDATA(mathFsqrtFunc)
	.globl  GDATA(mathFtanFunc)
	.globl  GDATA(mathFtanhFunc)
	.globl  GDATA(mathFtruncFunc)


	.data
	.balign 16

FUNC_LABEL(mathAcosFunc)		/* double-precision function pointers */
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathAsinFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathAtanFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathAtan2Func)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathCbrtFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathCeilFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathCosFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathCoshFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathExpFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFabsFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFloorFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFmodFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathHypotFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathInfinityFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathIrintFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathIroundFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathLogFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathLog2Func)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathLog10Func)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathPowFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathRoundFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathSinFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathSincosFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathSinhFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathSqrtFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathTanFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathTanhFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathTruncFunc)
	.long	FUNC(mathErrNoInit)

FUNC_LABEL(mathFacosFunc)		/* single-precision function pointers */
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFasinFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFatanFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFatan2Func)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFcbrtFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFceilFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFcosFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFcoshFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFexpFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFfabsFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFfloorFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFfmodFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFhypotFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFinfinityFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFirintFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFiroundFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFlogFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFlog2Func)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFlog10Func)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFpowFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFroundFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFsinFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFsincosFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFsinhFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFsqrtFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFtanFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFtanhFunc)
	.long	FUNC(mathErrNoInit)
FUNC_LABEL(mathFtruncFunc)
	.long	FUNC(mathErrNoInit)

FUNC_LABEL(mathErrNoInitString)
	.asciz	"ERROR - floating point math not initialized!\n"


        .text
	.balign 16

/*******************************************************************************
*
* acos - ANSI-compatable floating-point arc-cosine
*
* RETURNS: The arc-cosine in the range -pi/2 to pi/2 radians.

* double acos (dblParam)
*     double dblParam;	/* angle in radians *

*/

FUNC_LABEL(acos)
	movl	FUNC(mathAcosFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* asin - ANSI-compatable floating-point arc-sine
*
* RETURNS: The arc-sine in the range 0.0 to pi radians.
* 
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* double asin (dblParam)
*     double dblParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(asin)
	movl	FUNC(mathAsinFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* atan - ANSI-compatable floating-point arc-tangent
*
* RETURNS: The arc-tangent of dblParam in the range -pi/2 to pi/2.
*
* SEE ALSO: floatLib (1), acos (2), asin (2)

* double atan (dblParam)
*     double dblParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(atan)
	movl	FUNC(mathAtanFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* atan2 - function returns the arc tangent of (dblY/dblX) 
*
* RETURNS:
*    The arc-tangent of (dblY/dblX) in the range -pi to pi.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* double atan2 (dblY, dblX)
*     double dblY;		/* Y *
*     double dblX;		/* X *

*/

	.balign 16,0x90
FUNC_LABEL(atan2)
	movl	FUNC(mathAtan2Func),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* cbrt - floating-point cube root
*
* This routine takes a double-precision floating point parameter
* and returns the double-precision cube root.
*
* RETURNS: double-precision cube root

* double cbrt (dblParam)
*     double dblParam;  /* argument *

*/

	.balign 16,0x90
FUNC_LABEL(cbrt)
	movl	FUNC(mathCbrtFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* ceil - ANSI-compatable floating-point ceiling
*
* Performs a 'round-to-positive-infinity'
*
* RETURNS: 
* The least integral value greater than or equal to dblParam,
* result is returned in double precision.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* double ceil (dblParam)
*     double dblParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(ceil)
	movl	FUNC(mathCeilFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* cos - ANSI-compatable floating-point cosine
*
* RETURNS: the cosine of the radian argument dblParam
*
* SEE ALSO: 
* floatLib (1), sin (2), tan(2),
* "The C Programming Language - Second Edition"

* double cos (dblParam)
*     double dblParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(cos)
	movl	FUNC(mathCosFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* exp - exponential function
*
* RETURNS:
*    Floating-point inverse natural logarithm (e ** (dblExponent)).
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* double exp (dblExponent)
*     double dblExponent;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(exp)
	movl	FUNC(mathExpFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fabs - ANSI-compatable floating-point absolute value
*
* RETURNS: The floating-point absolute value of dblParam.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* double fabs (dblParam)
*     double dblParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(fabs)
	movl	FUNC(mathFabsFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* floor - ANSI-compatable floating-point floor
*
* Performs a 'round-to-negative-infinity'.
*
* RETURNS: 
* The largest integral value less than or equal to dblParam,
* result is returned in double precision.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"
*

* double floor (dblParam)
*     double dblParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(floor)
	movl	FUNC(mathFloorFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fmod - ANSI-compatable floating-point modulus 
*
* RETURNS: 
* Floating-point modulus of (dblParam / dblDivisor) with the sign of dblParam.  
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* double fmod (dblParam, dblDivisor)
*     double dblParam;		/* argument *
*     double dblDivisor;	/* divisor *

*/

	.balign 16,0x90
FUNC_LABEL(fmod)
	movl	FUNC(mathFmodFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* hypot - double-precision Euclidean distance (hypotenuse)
*
* This routine takes two input double-precision floating point
* parameters and returns length of the corresponding Euclidean distance
* (hypotenuse).
*
* The distance is calculated as
*	sqrt ((dblX * dblX) + (dblY * dblY))
*
* RETURNS: double-precision hypotenuse.
*

* double hypot (dblX, dblY)
*     double dblX;		/* argument *
*     double dblY;		/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(hypot)
	movl	FUNC(mathHypotFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/********************************************************************************
* infinity - return a very large double
*
* SEE ALSO: floatLib(1)
 
* double infinity ()

*/
 
	.balign 16,0x90
FUNC_LABEL(infinity)
	movl	FUNC(mathInfinityFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */


/********************************************************************************
* irint - convert double to integer
*
* Convert dblParam to an integer using the selected IEEE
* rounding direction.
*
* CAVEAT:
* The rounding direction is not pre-selectable
* and is fixed for 'round-to-the-nearest'.
*
* SEE ALSO: floatLib (1)
 
* int irint (dblParam)
*     double dblParam;	/* argument *

*/
 
	.balign 16,0x90
FUNC_LABEL(irint)
	movl	FUNC(mathIrintFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* iround - INTEGER floating-point rounding
*
* Performs a 'round-to-the-nearest' function.
*
* NOTE:
* If dblParam is spaced evenly between two integers,
* then the  even integer will be returned.

* int iround (dblParam)
*     double dblParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(iround)
	movl	FUNC(mathIroundFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* log - ANSI-compatable floating-point natural logarithm 
*
* RETURNS: The natural logarithm of dblParam.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* double log (dblParam)
*     double dblParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(log)
	movl	FUNC(mathLogFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* log10 - ANSI-compatable floating-point logarithm base 10 
*
* RETURNS: The logarithm (base 10) of dblParam.
*
* SEE ALSO: floatLib (1), log2 (2)

* double log10 (dblParam)
*     double dblParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(log10)
	movl	FUNC(mathLog10Func),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* log2 - ANSI-compatable floating-point logarithm base 2 
*
* RETURNS: The logarithm (base 2) of dblParam.
*
* SEE ALSO: floatLib (1), log10 (2)

* double log2 (dblParam)
*     double dblParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(log2)
	movl	FUNC(mathLog2Func),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* pow - ANSI-compatable floating-point dblX to the power of floating-point dblY 
*
* RETURNS: The floating-point value of dblX to the power of dblY.
*
* SEE ALSO: floatLib (1), sqrt (2)

* double pow (dblX, dblY)
*     double dblX;	/* X *
*     double dblY;	/* X *

*/

	.balign 16,0x90
FUNC_LABEL(pow) 					/* pow (x,y) = exp (y * log(x)) */
	movl	FUNC(mathPowFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* round - floating-point rounding
*
* Performs a 'round-to-nearest'.
*
* SEE ALSO: floatLib (1)

* double round (dblParam)
*     double dblParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(round)
	movl	FUNC(mathRoundFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* sin - ANSI-compatable floating-point sine
*
* RETURNS: The floating-point sine of dblParam.
*
* SEE ALSO: 
*   floatLib (1), cos (2), tan (2), 
*   "The C Programming Language - Second Edition"

* double sin (dblParam)
*     double dblParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(sin)
	movl	FUNC(mathSinFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* sincos - simultaneous floating-point sine and cosine
*
* RETURNS:
* The simultaeous floating point results of sine and cosine of the 
* radian argument  The dblParam must be in range of -1.0 to +1.0.
*
*
* SEE ALSO: floatLib (1), "i80387 Floating-Point User's Manual"

* void sincos (dblParam, sinResult, cosResult)
*     double dblParam;		/* angle in radians *
*     double *sinResult;	/* sine result buffer *
*     double *cosResult;	/* cosine result buffer *

*/

	.balign 16,0x90
FUNC_LABEL(sincos)
	movl	FUNC(mathSincosFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* sqrt - ANSI-compatable floating-point square root
*
* RETURNS: The floating-point square root of dblParam.
*
* SEE ALSO: floatLib(1), pow (2)

* double sqrt (dblParam)
*     double dblParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(sqrt)
	movl	FUNC(mathSqrtFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* tan - ANSI-compatable floating-point tangent
*
* RETURNS: Floating-point tangent of dblParam.
*
* SEE ALSO: floatLib (1), cos (2), sin (2),
* "The C Programming Language - Second Edition"

* double tan (dblParam)
*     double dblParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(tan)
	movl	FUNC(mathTanFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* trunc - floating-point truncation
*
* Performs FINTRZ.
*
* RETURNS:
* The integer portion of a double-precision number,
* result is in double-precision.
*
* SEE ALSO: floatLib (1)

* double trunc (dblParam)
*     double dblParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(trunc)
	movl	FUNC(mathTruncFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* facos - single-precision floating-point arc-cosine
*
* RETURNS: The arc-cosine in the range -pi/2 to pi/2 radians.

* float facos (fltParam)
*     float fltParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(facos)
	movl	FUNC(mathFacosFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fasin - single-precision floating-point arc-sine
*
* RETURNS: The arc-sine in the range 0.0 to pi radians.
* 
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* float fasin (fltParam)
*     float fltParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(fasin)
	movl	FUNC(mathFasinFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fatan - single-precision floating-point arc-tangent
*
* RETURNS: The arc-tangent of fltParam in the range -pi/2 to pi/2.
*
* SEE ALSO: floatLib (1), acos (2), asin (2)

* float fatan (fltParam)
*     float fltParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(fatan)
	movl	FUNC(mathFatanFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fatan2 - function returns the arc tangent of (fltY/fltX) 
*
* RETURNS:
*    The arc-tangent of (fltY/fltX) in the range -pi to pi.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* float fatan2 (fltY, fltX)
*     float fltY;		/* Y *
*     float fltX;		/* X *

*/

	.balign 16,0x90
FUNC_LABEL(fatan2)
	movl	FUNC(mathFatan2Func),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fcbrt - single-precision floating-point cube root
*
* This routine takes a single-precision floating point parameter
* and returns the single-precision cube root.
*
* RETURNS: single-precision cube root

* float fcbrt (fltParam)
*     float fltParam;  /* argument *

*/

	.balign 16,0x90
FUNC_LABEL(fcbrt)
	movl	FUNC(mathFcbrtFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fceil - single-precision floating-point ceiling
*
* Performs a 'round-to-positive-infinity'
*
* RETURNS: 
* The least integral value greater than or equal to fltParam,
* result is returned in single precision.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* float fceil (fltParam)
*     float fltParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(fceil)
	movl	FUNC(mathFceilFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fcos - single-precision floating-point cosine
*
* RETURNS: the cosine of the radian argument fltParam
*
* SEE ALSO: 
* floatLib (1), sin (2), cos (2), tan(2),
* "The C Programming Language - Second Edition"

* float fcos (fltParam)
*     float fltParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(fcos)
	movl	FUNC(mathFcosFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fcosh - single-precision floating-point hyperbolic cosine
*
* RETURNS:
*    The hyperbolic cosine of fltParam if (fltParam > 1.0), or
*    NaN if (fltParam < 1.0) 
*
* SEE ALSO: "The C Programming Language - Second Edition"

* float fcosh (fltParam)
*     float fltParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(fcosh)
	movl	FUNC(mathFcoshFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fexp - single-precision exponential function
*
* RETURNS:
*    Floating-point inverse natural logarithm (e ** (fltExponent)).
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* float fexp (fltExponent)
*     float fltExponent;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(fexp)
	movl	FUNC(mathFexpFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* ffabs - single-precision floating-point absolute value
*
* RETURNS: The floating-point absolute value of fltParam.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* float ffabs (fltParam)
*     float fltParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(ffabs)
	movl	FUNC(mathFfabsFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* ffloor - single-precision floating-point floor
*
* Performs a 'round-to-negative-infinity'.
*
* RETURNS: 
* The largest integral value less than or equal to fltParam,
* result is returned in single precision.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* float ffloor (fltParam)
*     float fltParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(ffloor)
	movl	FUNC(mathFfloorFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* ffmod - single-precision floating-point modulus 
*
* RETURNS: 
* Floating-point modulus of (fltParam / fltDivisor) with the sign of fltParam.  
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* float ffmod (fltParam, fltDivisor)
*     float fltParam;		/* argument *
*     float fltDivisor;		/* divisor *

*/

	.balign 16,0x90
FUNC_LABEL(ffmod)
	movl	FUNC(mathFfmodFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fhypot - single-precision Euclidean distance (hypotenuse)
*
* This routine takes two input single-precision floating point
* parameters and returns length of the corresponding Euclidean distance
* (hypotenuse).
*
* The distance is calculated as
*	sqrt ((fltX * fltX) + (fltY * fltY))
*
* RETURNS: single-precision hypotenuse.
*

* float fhypot (fltX, fltY)
*     float fltX;		/* argument *
*     float fltY;		/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(fhypot)
	movl	FUNC(mathFhypotFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/********************************************************************************
* finfinity - return a very large float
*
* SEE ALSO: floatLib(1)

* float finfinity ()

*/
 
	.balign 16,0x90
FUNC_LABEL(finfinity)
	movl	FUNC(mathFinfinityFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/********************************************************************************
* firint - convert float to integer
*
* Convert fltParam to an integer using the selected IEEE
* rounding direction.
*
* CAVEAT:
* The rounding direction is not pre-selectable
* and is fixed for 'round-to-the-nearest'.
*
* SEE ALSO: floatLib (1)

* int firint (fltParam)
*     float fltParam;	/* argument *

*/
 
	.balign 16,0x90
FUNC_LABEL(firint)
	movl	FUNC(mathFirintFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* firound - INTEGER floating-point rounding
*
* Performs a 'round-to-the-nearest' function.
*
* NOTE:
* If fltParam is spaced evenly between two integers,
* then the  even integer will be returned.

* int firound (fltParam)
*     float fltParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(firound)
	movl	FUNC(mathFiroundFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* flog - single-precision floating-point natural logarithm 
*
* RETURNS: The natural logarithm of fltParam.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* float flog (fltParam)
*     float fltParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(flog)
	movl	FUNC(mathFlogFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* flog10 - single-precision floating-point logarithm base 10 
*
* RETURNS: The logarithm (base 10) of fltParam.
*
* SEE ALSO: floatLib (1), log2 (2)

* float flog10 (fltParam)
*     float fltParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(flog10)
	movl	FUNC(mathFlog10Func),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* flog2 - single-precision floating-point logarithm base 2 
*
* RETURNS: The logarithm (base 2) of fltParam.
*
* SEE ALSO: floatLib (1), log10 (2)

* float flog2 (fltParam)
*     float fltParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(flog2)
	movl	FUNC(mathFlog2Func),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fpow - single-precision floating point X to the power of floating-point Y 
*
* RETURNS: The floating-point value of fltX to the power of fltY.
*
* SEE ALSO: floatLib (1), sqrt (2)

* float fpow (fltX, fltY)
*     float fltX;	/* X *
*     float fltY;	/* X *

*/

	.balign 16,0x90
FUNC_LABEL(fpow) 					/* pow (x,y) = exp (y * log(x)) */
	movl	FUNC(mathFpowFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fround - floating-point rounding
*
* Performs a 'round-to-nearest'.
*
* SEE ALSO: floatLib (1)

* float fround (fltParam)
*     float fltParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(fround)
	movl	FUNC(mathFroundFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fsin - single-precision floating-point sine
*
* RETURNS: The floating-point sine of fltParam.
*
* SEE ALSO: 
*   floatLib (1), cos (2), tan (2), 
*   "The C Programming Language - Second Edition"

* float fsin (fltParam)
*     float fltParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(fsin)
	movl	FUNC(mathFsinFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fsincos - simultaneous single-precision floating-point sine and cosine
*
* RETURNS:
* The simultaeous floating point results of sine and cosine of the 
* radian argument  The fltParam must be in range of -1.0 to +1.0.
*
*
* SEE ALSO: floatLib (1), "i80387 Floating-Point User's Manual"

* void fsincos (fltParam, sinResult, cosResult)
*     float fltParam;		/* angle in radians *
*     float *sinResult;	/* sine result buffer *
*     float *cosResult;	/* cosine result buffer *

*/

	.balign 16,0x90
FUNC_LABEL(fsincos)
	movl	FUNC(mathFsincosFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fsinh - single-precision floating-point hyperbolic sine
*
* RETURNS: The floating-point hyperbolic sine of fltParam.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"

* float fsinh (fltParam)
*     float fltParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(fsinh)
	movl	FUNC(mathFsinhFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* fsqrt - single-precision floating-point square root
*
* RETURNS: The floating-point square root of fltParam.
*
* SEE ALSO: floatLib(1), pow (2)

* float fsqrt (fltParam)
*     float fltParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(fsqrt)
	movl	FUNC(mathFsqrtFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* ftan - single-precision floating-point tangent
*
* RETURNS: Floating-point tangent of fltParam.
*
* SEE ALSO: floatLib (1), cos (2), sin (2),
* "The C Programming Language - Second Edition"

* float ftan (fltParam)
*     float fltParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(ftan)
	movl	FUNC(mathFtanFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* ftanh - single-precision floating-point hyperbolic tangent
*
* RETURNS: Floating-point hyperbolic tangent of fltParam.
*
* SEE ALSO: 
* floatLib (1), fcosh (), fsinh ()
* "The C Programming Language - Second Edition"

* float ftanh (fltParam)
*     float fltParam;	/* angle in radians *

*/

	.balign 16,0x90
FUNC_LABEL(ftanh)
	movl	FUNC(mathFtanhFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* ftrunc - single-precision floating-point truncation
*
* Performs FINTRZ.
*
* RETURNS:
* The integer portion of a single-precision number,
* result is in single-precision.
*
* SEE ALSO: floatLib (1)

* float ftrunc (fltParam)
*     float fltParam;	/* argument *

*/

	.balign 16,0x90
FUNC_LABEL(ftrunc)
	movl	FUNC(mathFtruncFunc),%eax
	jmp	*%eax			/* jump, let that routine rts */

/*******************************************************************************
*
* mathErrNoInit - default routine for uninitialized floating-point functions
*
* This routine is called if floating point math operations are attempted
* before either a mathSoftInit() or mathHardInit() call is performed.  The
* address of this routine is the initial value of the various mathXXXFunc
* pointers, which are re-initialized with actual function addresses during
* either of the floating point initialization calls.
*
* SEE ALSO: mathHardInit(), mathSoftInit()

* void mathErrNoInit ()

*/

	.balign 16,0x90
FUNC_LABEL(mathErrNoInit)
	pushl	$FUNC(mathErrNoInitString)
	call	FUNC(logMsg)
	addl	$4,%esp
	ret


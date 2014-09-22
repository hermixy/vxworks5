/* mathHardALib.s - C-callable math routines for the i80387 */

/* Copyright 1984-2001 Wind River Systems, Inc. */

/*
modification history 
--------------------
01g,28aug01,hdn  changed "fldl %st" to "fld %st" to shut off warning
		 added mathHardFmodIeee() for IEEE compliance
		 replaced .align with .balign
		 moved round-mode bit macros to fppI86Lib.h
		 added FUNC/FUNC_LABEL and GTEXT/GDATA macros
01f,09may00,pai  fixed _mathHardFmod to test for partial remainder (SPR
                 #30548).
01e,29sep97,hdn  fixed a bug in _mathHardPow.
01d,17jun96,hdn  fixed a bug in _mathHardSincos.
01c,16jun93,hdn  updated to 5.1.
01b,14oct92,hdn  aligned all functions.
01a,16sep92,hdn  written by modifying Tron's mathHardALib.s.
*/

/*
DESCRIPTION
This library provides a C interface to the high-level math functions
on the i80387 floating-point coprocessor.  All angle-related
parameters and return values are expressed in radians.  Functions
capable errors, will set errno upon an error. All functions
included in this library whos names correspond to the ANSI C specification
are, indeed, ANSI-compatable. In the spirit of ANSI, HUGE_VAL is now
supported.

WARNING
This library works only if an i80387 coprocessor is in the system! 
Attempts to use these routines with no coprocessor present 
will result in illegal instruction traps.

SEE ALSO:
fppLib (1), floatLib (1), The C Programming Language - Second Edition

INCLUDE FILE: math.h

INTERNAL
Each routine has the following format:
    o calculate floating-point function using double parameter 
    o store result to %st0 register
*/

#define _ASMLANGUAGE
#include "vxWorks.h"
#include "asm.h"
#include "fppLib.h"
#include "errno.h"


	.data
	.globl	FUNC(copyright_wind_river)
	.long	FUNC(copyright_wind_river)


	/* externals */


	/* internals */

        .globl  GTEXT(mathHardLog2)
        .globl  GTEXT(mathHardLog10)
        .globl  GTEXT(mathHardLog)
        .globl  GTEXT(mathHardExp)
	.globl  GTEXT(mathHardAsin)
	.globl  GTEXT(mathHardAcos)
	.globl  GTEXT(mathHardAtan)
	.globl  GTEXT(mathHardAtan2)
        .globl  GTEXT(mathHardTan)
        .globl  GTEXT(mathHardCos)
        .globl  GTEXT(mathHardSin)
        .globl  GTEXT(mathHardPow)
        .globl  GTEXT(mathHardSqrt)
        .globl  GTEXT(mathHardFabs)
	.globl  GTEXT(mathHardFmod)
	.globl  GTEXT(mathHardFmodIeee)

	.globl  GTEXT(mathHardSincos)
	.globl  GTEXT(mathHardFloor)
	.globl  GTEXT(mathHardCeil)
	.globl  GTEXT(mathHardTrunc)
	.globl  GTEXT(mathHardRound)
	.globl  GTEXT(mathHardIround)
	.globl  GTEXT(mathHardIrint)
	.globl  GTEXT(mathHardInfinity)


        .text
	.balign 16


/*******************************************************************************
*
* mathHardAcos - ANSI-compatable hardware floating-point arc-cosine
*
* RETURNS: The arc-cosine in the range -pi/2 to pi/2 radians.
*
* double mathHardAcos (dblParam)
*     double dblParam;	/* angle in radians *
*
*/

FUNC_LABEL(mathHardAcos)	/* acos(x) = pi/2 - atan(x/sqrt(1-x**2)) */
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG1(%ebp)
	fldl	DARG1(%ebp)
	fmul	%st,%st(1)
	fxch	%st(1)
	fld1
	fsubp	%st,%st(1)
	fsqrt
	fpatan
	fld1
	fld1
	faddp	%st,%st(1)
	fldpi
	fdivp	%st,%st(1)
	fsubp	%st,%st(1)

	leave
	ret

/*******************************************************************************
*
* mathHardAsin - ANSI-compatable hardware floating-point arc-sine
*
* RETURNS: The arc-sine in the range 0.0 to pi radians.
* 
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"
*
* double mathHardAsin (dblParam)
*     double dblParam;	/* angle in radians *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardAsin)	/* asin(x) = atan(x/sqrt(1-x**2)) */
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG1(%ebp)
	fldl	DARG1(%ebp)
	fmul	%st,%st(1)
	fxch	%st(1)
	fld1
	fsubp	%st,%st(1)
	fsqrt
	fpatan

	leave
	ret

/*******************************************************************************
*
* mathHardAtan - ANSI-compatable hardware floating-point arc-tangent
*
* RETURNS: The arc-tangent of dblParam in the range -pi/2 to pi/2.
*
* SEE ALSO: floatLib (1), acos (2), asin (2)
*
* double mathHardAtan (dblParam)
*     double dblParam;	/* angle in radians *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardAtan)
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG1(%ebp)
	fld1
	fpatan

	leave
	ret

/*******************************************************************************
*
* mathHardAtan2 - hardware floating point function for arctangent of (dblY/dblX)
*
* RETURNS:
*    The arc-tangent of (dblY/dblX) in the range -pi to pi.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"
*
* double mathHardAtan2 (dblY, dblX)
*     double dblY;		/* Y *
*     double dblX;		/* X *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardAtan2)
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG1(%ebp)
	fldl	DARG2(%ebp)
	fpatan

	leave
	ret
    
/*******************************************************************************
*
* mathHardCos - ANSI-compatable hardware floating-point cosine
*
* RETURNS: the cosine of the radian argument dblParam
*
* SEE ALSO: 
* floatLib (1), sin (2), cos (2), tan(2),
* "The C Programming Language - Second Edition"
*
* double mathHardCos (dblParam)
*     double dblParam;	/* angle in radians *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardCos)
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG1(%ebp)
	fcos

	fstl	DARG1(%ebp)
	fwait
	movl	DARG1+4(%ebp),%eax
	movl	DARG1(%ebp),%edx

	leave
	ret

/*******************************************************************************
*
* mathHardExp - hardware floating-point exponential function
*
* RETURNS:
*    Floating-point inverse natural logarithm (e ** (dblExponent)).
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"
*
* double mathHardExp (dblExponent)
*     double dblExponent;	/* argument *
*
*/

	.balign 16,0x90
powerOftwo:
	/* 2**z. 
	 * z1 is integer of z. z2 is fractal of z.
	 * if z2 is greater than 0.5, z2 -= 0.5, then
	 *     2**z = 2**(z1 + z2 + 0.5) = 2**(z1) * 2**(z2) * 2**(0.5)
	 * if z2 is less than 0.5, then
	 *     2**z = 2**(z1 + z2) = 2**(z1) * 2**(z2)
	 */
	pushl	%ebp
	movl	%esp,%ebp

	/* change the Round Control bits */
	subl	$8,%esp
	fstcw	-4(%ebp)
	fwait
	movw	-4(%ebp),%ax
	andw	$ FPCR_RC_MASK,%ax
	orw	$ FPCR_RC_DOWN,%ax
	movw	%ax,-8(%ebp)
	fldcw	-8(%ebp)

	/* get z1(integer part of z) and z2(fractal part of z) */
	fld	%st
	frndint
	fldcw	-4(%ebp)
	fsub	%st,%st(1)
	fxch	%st(1)
	fchs

	/* get a value 0.5 */
	fld1
	fchs
	fld1
	fscale
	fxch	%st(1)
	fstp	%st

	/* get z2 % 0.5 */
	fxch	%st(1)
	fprem
	fstsw	%ax
	fstp	%st(1)

	/* get A = 2**(z2) */
	f2xm1
	fld1
	faddp	%st,%st(1)
	test	$0x0200,%ax
	jz	powerOftwo0

	/* get A = 2**(z2) * 2**(0.5) */
	fld1
	fadd	%st,%st(0)
	fsqrt
	fmulp	%st,%st(1)

powerOftwo0:
	/* get 2**z = A * 2**(z1) */
	fscale
	fstp	%st(1)
	addl	$8,%esp

	leave
	ret

	.balign 16,0x90
FUNC_LABEL(mathHardExp)		/* e**y = 2**(y * log2(e)) */
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG1(%ebp)
	fldl2e
	fmulp	%st,%st(1)
	call	powerOftwo

	leave
	ret

/*******************************************************************************
*
* mathHardFabs - ANSI-compatable hardware floating-point absolute value
*
* RETURNS: The floating-point absolute value of dblParam.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"
*
* double mathHardFabs (dblParam)
*     double dblParam;	/* argument *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardFabs) 
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG1(%ebp)
	fabs

	leave
	ret

/*******************************************************************************
*
* mathHardLog - ANSI-compatable hardware floating-point natural logarithm 
*
* RETURNS: The natural logarithm of dblParam.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"
*
* double mathHardLog (dblParam)
*     double dblParam;	/* argument *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardLog)		/* loge(x) = loge(2) * log2(x) */
	pushl	%ebp
	movl	%esp,%ebp

	fldln2			/* st0 = loge(2) */
	fldl	DARG1(%ebp)	/* st0 = x, st1 = loge(2) */
	fyl2x			/* st0 = loge(2) * log2(x) */

	leave
	ret

/*******************************************************************************
*
* mathHardLog10 - ANSI-compatable hardware floating-point base 10 logarithm
*
* RETURNS: The logarithm (base 10) of dblParam.
*
* SEE ALSO: floatLib (1), log2 (2)
*
* double mathHardLog10 (dblParam)
*     double dblParam;	/* argument *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardLog10)	/* log10(x) = log10(2) * log2(x) */
	pushl	%ebp
	movl	%esp,%ebp

	fldlg2			/* st0 = log10(2) */
	fldl	DARG1(%ebp)	/* st0 = x, st1 = log10(2) */
	fyl2x			/* st0 = log10(2) * log2(x) */

	leave
	ret

/*******************************************************************************
*
* mathHardLog2 - ANSI-compatable hardware floating-point logarithm base 2 
*
* RETURNS: The logarithm (base 2) of dblParam.
*
* SEE ALSO: floatLib (1), log10 (2)
*
* double mathHardLog2 (dblParam)
*     double dblParam;	/* argument *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardLog2)
	pushl	%ebp
	movl	%esp,%ebp

	fld1
	fldl	DARG1(%ebp)
	fyl2x

	leave
	ret

/*******************************************************************************
*
* mathHardPow - ANSI-compatable hardware floating-point power function
*
* RETURNS: The floating-point value of dblX to the power of dblY.
*
* SEE ALSO: floatLib (1), sqrt (2)
*
* double mathHardPow (dblX, dblY)
*     double dblX;	/* X *
*     double dblY;	/* Y *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardPow) 		/* x**y = 2**(y * log2(x)) */
	pushl	%ebp
	movl	%esp,%ebp

	fldz
	fldl	DARG1(%ebp)
	fcompp
	fstsw	%ax
	sahf
	je	powZeroX		/* if (x == 0) */
	jb	powNegX			/* if (x < 0) */

	fldl	DARG2(%ebp)		/* x > 0 */
	fldl	DARG1(%ebp)		/* x**y = 2**(y * log2(x)) */
	fyl2x
	call	powerOftwo
	jmp	powExit

powZeroX:				/* x == 0 */
	fldz
	fldl	DARG2(%ebp)
	fcompp
	fstsw	%ax
	sahf
	ja	powNan			/* if (y > 0),  (0**y = 0) */
	je	powOne			/* if (y == 0), (0**0 = 1) */

	fldl	mathHardInfinity0	/* if (y < 0),  (0**y = HUGE_VALUE */
	fldl2t
	fmulp	%st, %st(1)
	call	powerOftwo
	jmp	powErrExit

powOne:
	fld1
	jmp	powErrExit

powNan:
	fldz
	jmp	powErrExit

powNegX:				/* x < 0 */
	fldl	DARG2(%ebp)
	frndint
	fcompl	DARG2(%ebp)
	fstsw	%ax
	sahf
	jne	powNan			/* if (y != int(y)), (x**y = 0) */

	fldl	DARG2(%ebp)		/* x**y = 2**(y * log2(|x|)) */
	fldl	DARG1(%ebp)
	fabs
	fyl2x
	call	powerOftwo

	fld1				/* if ((y % 2) != 0)              */
	fld1				/*   x**y = -(%st)                */
	faddp				/* else                           */
	fldl	DARG2(%ebp)             /*   x**y = %st	                  */
	fprem
	fldz
	fcompp
	fstsw	%ax
	sahf
	je	powExit0

	fcomp	%st			/* pop, fincstp + ffree might work */
	fchs				/* x**y = -(%st) */
	jmp	powExit

powExit0:
	fcomp	%st			/* pop, fincstp + ffree might work */
	jmp	powExit

powErrExit:
powExit:
	leave
	ret

/*******************************************************************************
*
* mathHardSin - ANSI-compatable hardware floating-point sine
*
* RETURNS: The floating-point sine of dblParam.
*
* SEE ALSO: 
*   floatLib (1), cos (2), tan (2), 
*   "The C Programming Language - Second Edition"
*
* double mathHardSin (dblParam)
*     double dblParam;	/* angle in radians *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardSin)
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG1(%ebp)
	fsin

	leave
	ret

/*******************************************************************************
*
* mathHardSqrt - ANSI-compatable hardware floating-point square root
*
* RETURNS: The floating-point square root of dblParam.
*
* SEE ALSO: floatLib(1), pow (2)
*
* double mathHardSqrt (dblParam)
*     double dblParam;	/* argument *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardSqrt)
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG1(%ebp)
	fsqrt

	leave
	ret

/*******************************************************************************
*
* mathHardTan - ANSI-compatable hardware floating-point tangent
*
* RETURNS: Floating-point tangent of dblParam.
*
* SEE ALSO: floatLib (1), cos (2), sin (2),
* "The C Programming Language - Second Edition"
*
* double mathHardTan (dblParam)
*     double dblParam;	/* angle in radians *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardTan)
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG1(%ebp)
	fptan
	fstp	%st

	leave
	ret

/*******************************************************************************
*
* mathHardSincos - simultaneous hardware floating-point sine and cosine
*
* RETURNS:
* The simultaeous floating point results of sine and cosine of the 
* radian argument  The dblParam must be in range of -1.0 to +1.0.
*
* CAVEAT:
* Supported for the HD648132 only.
*
* SEE ALSO: floatLib (1), "HD648132 Floating-Point User's Manual"
*
* void mathHardSincos (dblParam, sinResult, cosResult)
*     double dblParam;		/* angle in radians *
*     double *sinResult;	/* sine result buffer *
*     double *cosResult;	/* cosine result buffer *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardSincos)
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG1(%ebp)
	fsincos
	movl	DARG2L(%ebp),%eax
	fstpl	(%eax)
	movl	DARG2(%ebp),%edx
	fstpl	(%edx)

	leave
	ret

/*******************************************************************************
*
* mathHardFmod - ANSI-compatable hardware floating-point modulus (fprem)
*
* RETURNS: 
* Floating-point modulus of (dblParam / dblDivisor) with the sign of dblParam.  
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"
*
* double mathHardFmod (dblParam, dblDivisor)
*     double dblParam;		/* argument *
*     double dblDivisor;	/* divisor *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardFmod)
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG2(%ebp)
	fldl	DARG1(%ebp)

reducePartial:

	fprem			/* fprem for non IEEE */
	fstsw	%ax		/* put the FPU status word in AX */
	test	$0x0400,%ax	/* is there a partial remainder at ST(0) ? */
	jnz	reducePartial

	fstp	%st(1)

	leave
	ret

/*******************************************************************************
*
* mathHardFmodIeee - ANSI-compatable hardware floating-point modulus (fprem1)
*
* RETURNS: 
* Floating-point modulus of (dblParam / dblDivisor) with the sign of dblParam.  
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"
*
* double mathHardFmodIeee (dblParam, dblDivisor)
*     double dblParam;		/* argument *
*     double dblDivisor;	/* divisor *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardFmodIeee)
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG2(%ebp)
	fldl	DARG1(%ebp)

reducePartialIeee:

	fprem1			/* fprem1 for IEEE */
	fstsw	%ax		/* put the FPU status word in AX */
	test	$0x0400,%ax	/* is there a partial remainder at ST(0) ? */
	jnz	reducePartialIeee

	fstp	%st(1)

	leave
	ret

/*******************************************************************************
*
* mathHardFloor - ANSI-compatable hardware floating-point floor
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
*
* double mathHardFloor (dblParam)
*     double dblParam;	/* argument *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardFloor)
	pushl	%ebp
	movl	%esp,%ebp

	subl	$8,%esp
	fstcw	-4(%ebp)
	fwait
	movw	-4(%ebp),%ax
	andw	$ FPCR_RC_MASK,%ax
	orw	$ FPCR_RC_DOWN,%ax
	movw	%ax,-8(%ebp)
	fldcw	-8(%ebp)
	fldl	DARG1(%ebp)
	frndint
	fldcw	-4(%ebp)
	addl	$8,%esp

	leave
	ret

/*******************************************************************************
*
* mathHardCeil - ANSI-compatable hardware floating-point ceiling
*
* Performs a 'round-to-positive-infinity'
*
* RETURNS: 
* The least integral value greater than or equal to dblParam,
* result is returned in double precision.
*
* SEE ALSO: 
* floatLib (1), "The C Programming Language - Second Edition"
*
* double mathHardCeil (dblParam)
*     double dblParam;	/* argument *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardCeil)
	pushl	%ebp
	movl	%esp,%ebp

	subl	$8,%esp
	fstcw	-4(%ebp)
	fwait
	movw	-4(%ebp),%ax
	andw	$ FPCR_RC_MASK,%ax
	orw	$ FPCR_RC_UP,%ax
	movw	%ax,-8(%ebp)
	fldcw	-8(%ebp)
	fldl	DARG1(%ebp)
	frndint
	fldcw	-4(%ebp)
	addl	$8,%esp

	leave
	ret

/*******************************************************************************
*
* mathHardTrunc - hardware floating-point truncation
*
* Performs FINTRZ.
*
* RETURNS:
* The integer portion of a double-precision number,
* result is in double-precision.
*
* SEE ALSO: floatLib (1)
*
* double mathHardTrunc (dblParam)
*     double dblParam;	/* argument *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardTrunc)
	pushl	%ebp
	movl	%esp,%ebp

	subl	$8,%esp
	fstcw	-4(%ebp)
	fwait
	movw	-4(%ebp),%ax
	andw	$ FPCR_RC_MASK,%ax
	orw	$ FPCR_RC_ZERO,%ax
	movw	%ax,-8(%ebp)
	fldcw	-8(%ebp)
	fldl	DARG1(%ebp)
	frndint
	fldcw	-4(%ebp)
	addl	$8,%esp

	leave
	ret

/*******************************************************************************
*
* mathHardRound - hardware floating-point rounding
*
* Performs a 'round-to-nearest'.
*
* SEE ALSO: floatLib (1)
*
* double mathHardRound (dblParam)
*     double dblParam;	/* argument *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardRound)
	pushl	%ebp
	movl	%esp,%ebp

	subl	$8,%esp
	fstcw	-4(%ebp)
	fwait
	movw	-4(%ebp),%ax
	andw	$ FPCR_RC_MASK,%ax
	orw	$ FPCR_RC_NEAREST,%ax
	movw	%ax,-8(%ebp)
	fldcw	-8(%ebp)
	fldl	DARG1(%ebp)
	frndint
	fldcw	-4(%ebp)
	addl	$8,%esp

	leave
	ret

/*******************************************************************************
*
* mathHardIround - hardware floating-point rounding to nearest integer
*
* Performs a 'round-to-nearest' function.
*
* NOTE:
* If dblParam is spaced evenly between two integers,
* then the  even integer will be returned.
*
* int mathHardIround (dblParam)
*     double dblParam;	/* argument *
*
*/

	.balign 16,0x90
FUNC_LABEL(mathHardIround)
	pushl	%ebp
	movl	%esp,%ebp

	subl	$8,%esp
	fstcw	-4(%ebp)
	fwait
	movw	-4(%ebp),%ax
	andw	$0xf3ff,%ax
	orw	$0x0,%ax
	movw	%ax,-8(%ebp)
	fldcw	-8(%ebp)
	fldl	DARG1(%ebp)
	frndint
	fistpl	DARG1(%ebp)
	fwait
	movl	DARG1(%ebp),%eax
	fldcw	-4(%ebp)
	addl	$8,%esp

	leave
	ret

/********************************************************************************
* mathHardIrint - hardware floating-point double to integer conversion
*
* Convert dblParam to an integer using the selected IEEE
* rounding direction.
*
* CAVEAT:
* The rounding direction is not pre-selectable
* and is fixed for 'round-to-the-nearest'.
*
* SEE ALSO: floatLib (1)
* 
* int mathHardIrint (dblParam)
*     double dblParam;	/* argument *
*
*/
 
	.balign 16,0x90
FUNC_LABEL(mathHardIrint)
	pushl	%ebp
	movl	%esp,%ebp

	fldl	DARG1(%ebp)
	frndint
	fistpl	DARG1(%ebp)
	fwait
	movl	DARG1(%ebp),%eax

	leave
	ret

/********************************************************************************
* mathHardInfinity - hardware floating-point return of a very large double
*
* SEE ALSO: floatLib(1)
* 
* double mathHardInfinity ()
*
*/
 
	.balign 16
mathHardInfinity0:
	.double 0d4.09600000000000000000e+03
	.balign 16

FUNC_LABEL(mathHardInfinity)		/* 10**4096 = 2**(4096 * log2(10)) */
	pushl	%ebp
	movl	%esp,%ebp

	fldl	mathHardInfinity0
	fldl2t
	fmulp	%st,%st(1)
	call	powerOftwo

	leave
	ret


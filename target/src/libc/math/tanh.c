/* tanh.c - math routines */

/* Copyright 1992-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,09dec94,rhp  fix man pages for hyperbolic fns.
01e,05feb93,jdi  doc changes based on kdl review.
01d,02dec92,jdi  doc tweaks.
01c,28oct92,jdi  documentation cleanup.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  documentation.
*/

/*
DESCRIPTION
* Copyright (c) 1985 Regents of the University of California.
* All rights reserved.
*
* Redistribution and use in source and binary forms are permitted
* provided that the above copyright notice and this paragraph are
* duplicated in all such forms and that any documentation,
* advertising materials, and other materials related to such
* distribution and use acknowledge that the software was developed
* by the University of California, Berkeley.  The name of the
* University may not be used to endorse or promote products derived
* from this software without specific prior written permission.
* THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
* WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*
* All recipients should regard themselves as participants in an ongoing
* research project and hence should feel obligated to report their
* experiences (good or bad) with these elementary function codes, using
* the sendbug(8) program, to the authors.
*
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"

/*******************************************************************************
*
* tanh - compute a hyperbolic tangent (ANSI)
*
* This routine returns the hyperbolic tangent of <x> in
* double precision (IEEE double, 53 bits).
*
* INTERNAL:
* Method:
*
* (1) Reduce <x> to non-negative by:
*
*         tanh(-x) = - tanh(x)
*
* (2)
*         0      <  x <=  1.e-10 :  tanh(x) := x
*
*                                               -expm1(-2x)
*         1.e-10 <  x <=  1      :  tanh(x) := --------------
*                                              expm1(-2x) + 2
*
*                                                       2
*         1      <= x <=  22.0   :  tanh(x) := 1 -  ---------------
*                                                   expm1(2x) + 2
*         22.0   <  x <= INF     :  tanh(x) := 1.
*
*     Note: 22 was chosen so that fl(1.0+2/(expm1(2*22)+2)) == 1.
*
* INCLUDE FILES: math.h
*
* RETURNS:
* The double-precision hyperbolic tangent of <x>.
*
* Special cases:
*     If <x> is NaN, tanh() returns NaN.
*
* SEE ALSO: mathALib
*
* INTERNAL:
* Coded in C by K.C. Ng, 1/8/85;
* Revised by K.C. Ng on 2/8/85, 2/11/85, 3/7/85, 3/24/85.
*/

double tanh
    (
    double x	/* number whose hyperbolic tangent is required */
    )

    {
	static double one=1.0, two=2.0, small = 1.0e-10, big = 1.0e10;
	double expm1(), t, copysign(), sign;
	int finite();

#if !defined(vax)&&!defined(tahoe)
	if(x!=x) return(x);	/* x is NaN */
#endif	/* !defined(vax)&&!defined(tahoe) */

	sign=copysign(one,x);
	x=copysign(x,one);
	if(x < 22.0)
	    if( x > one )
		return(copysign(one-two/(expm1(x+x)+two),sign));
	    else if ( x > small )
		{t= -expm1(-(x+x)); return(copysign(t/(two-t),sign));}
	    else		/* raise the INEXACT flag for non-zero x */
		{big+x; return(copysign(x,sign));}
	else if(finite(x))
	    return (sign+1.0E-37); /* raise the INEXACT flag */
	else
	    return(sign);	/* x is +- INF */
    }

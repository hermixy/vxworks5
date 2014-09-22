/* sinh.c - math routine */

/* Copyright 1992-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,09dec94,rhp  fix descriptions of hyperbolic fns.
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

#if defined(vax)||defined(tahoe)
#ifdef vax
#define _0x(A,B)	0x/**/A/**/B
#else	/* vax */
#define _0x(A,B)	0x/**/B/**/A
#endif	/* vax */
/* static double */
/* mln2hi =  8.8029691931113054792E1     , Hex  2^  7   *  .B00F33C7E22BDB */
/* mln2lo = -4.9650192275318476525E-16   , Hex  2^-50   * -.8F1B60279E582A */
/* lnovfl =  8.8029691931113053016E1     ; Hex  2^  7   *  .B00F33C7E22BDA */
static long    mln2hix[] = { _0x(0f33,43b0), _0x(2bdb,c7e2)};
static long    mln2lox[] = { _0x(1b60,a70f), _0x(582a,279e)};
static long    lnovflx[] = { _0x(0f33,43b0), _0x(2bda,c7e2)};
#define   mln2hi    (*(double*)mln2hix)
#define   mln2lo    (*(double*)mln2lox)
#define   lnovfl    (*(double*)lnovflx)
#else	/* defined(vax)||defined(tahoe) */
static double
mln2hi =  7.0978271289338397310E2     , /*Hex  2^ 10   *  1.62E42FEFA39EF */
mln2lo =  2.3747039373786107478E-14   , /*Hex  2^-45   *  1.ABC9E3B39803F */
lnovfl =  7.0978271289338397310E2     ; /*Hex  2^  9   *  1.62E42FEFA39EF */
#endif	/* defined(vax)||defined(tahoe) */

#if defined(vax)||defined(tahoe)
static max = 126                      ;
#else	/* defined(vax)||defined(tahoe) */
static max = 1023                     ;
#endif	/* defined(vax)||defined(tahoe) */

/*******************************************************************************
*
* sinh - compute a hyperbolic sine (ANSI)
*
* This routine returns the hyperbolic sine of <x> in
* double precision (IEEE double, 53 bits).
*
* A range error occurs if <x> is too large.
*
* INTERNAL:
* Method:
*
* (1) Reduce <x> to non-negative by sinh(-x) = - sinh(x).
*
* (2)
*    	                                     expm1(x) + expm1(x)/(expm1(x)+1)
*    	   0 <= x <= lnovfl     : sinh(x) := --------------------------------
*    			       		                      2
*         lnovfl <= x <= lnovfl+ln2 : sinh(x) := expm1(x)/2 (avoid overflow)
*     lnovfl+ln2 <  x <  INF        :  overflow to INF
*    
* INCLUDE FILES: math.h
*
* RETURNS:
* The double-precision hyperbolic sine of <x>.
*
* Special cases:
*     If <x> is +INF, -INF, or NaN, sinh() returns <x>.
*
* SEE ALSO: mathALib
*/

double sinh
    (
    double x	/* number whose hyperbolic sine is required */
    )

    {
	static double  one=1.0, half=1.0/2.0 ;
	double expm1(), t, scalb(), copysign(), sign;
#if !defined(vax)&&!defined(tahoe)
	if(x!=x) return(x);	/* x is NaN */
#endif	/* !defined(vax)&&!defined(tahoe) */
	sign=copysign(one,x);
	x=copysign(x,one);
	if(x<lnovfl)
	    {t=expm1(x); return(copysign((t+t/(one+t))*half,sign));}

	else if(x <= lnovfl+0.7)
		/* subtract x by ln(2^(max+1)) and return 2^max*exp(x)
	    		to avoid unnecessary overflow */
	    return(copysign(scalb(one+expm1((x-mln2hi)-mln2lo),max),sign));

	else  /* sinh(+-INF) = +-INF, sinh(+-big no.) overflow to +-INF */
	    return( expm1(x)*sign );
    }

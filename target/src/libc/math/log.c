/* log.c - math routines */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,05feb93,jdi  doc changes based on kdl review.
01e,02dec92,jdi  doc tweaks.
01d,28oct92,jdi  documentation cleanup.
01c,13oct92,jdi  mangen fixes.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  documentation
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

#if defined(vax)||defined(tahoe)	/* VAX D format */
#include <errno.h>
#ifdef vax
#define _0x(A,B)	0x/**/A/**/B
#else	/* vax */
#define _0x(A,B)	0x/**/B/**/A
#endif	/* vax */
/* static double */
/* ln2hi  =  6.9314718055829871446E-1    , Hex  2^  0   *  .B17217F7D00000 */
/* ln2lo  =  1.6465949582897081279E-12   , Hex  2^-39   *  .E7BCD5E4F1D9CC */
/* sqrt2  =  1.4142135623730950622E0     ; Hex  2^  1   *  .B504F333F9DE65 */
static long     ln2hix[] = { _0x(7217,4031), _0x(0000,f7d0)};
static long     ln2lox[] = { _0x(bcd5,2ce7), _0x(d9cc,e4f1)};
static long     sqrt2x[] = { _0x(04f3,40b5), _0x(de65,33f9)};
#define    ln2hi    (*(double*)ln2hix)
#define    ln2lo    (*(double*)ln2lox)
#define    sqrt2    (*(double*)sqrt2x)
#else	/* defined(vax)||defined(tahoe) */
static double
ln2hi  =  6.9314718036912381649E-1    , /*Hex  2^ -1   *  1.62E42FEE00000 */
ln2lo  =  1.9082149292705877000E-10   , /*Hex  2^-33   *  1.A39EF35793C76 */
sqrt2  =  1.4142135623730951455E0     ; /*Hex  2^  0   *  1.6A09E667F3BCD */
#endif	/* defined(vax)||defined(tahoe) */

/*******************************************************************************
*
* log - compute a natural logarithm (ANSI)
*
* This routine returns the natural logarithm of <x>
* in double precision (IEEE double, 53 bits).
*
* A domain error occurs if the argument is negative.  A range error may occur
* if the argument is zero.
*
* INTERNAL:
* Method:
* (1) Argument Reduction: find <k> and <f> such that:
*
*         x = 2^k * (1+f)
*
*     where:
*
*         sqrt(2)/2 < 1+f < sqrt(2)
*
* (2) Let s = f/(2+f); based on:
*
*         log(1+f) = log(1+s) - log(1-s) = 2s + 2/3 s**3 + 2/5 s**5 + .....
*
*     log(1+f) is computed by:
*
*         log(1+f) = 2s + s*log__L(s*s)
*
*     where:
*
*        log__L(z) = z*(L1 + z*(L2 + z*(... (L6 + z*L7)...)))
*
* (3) Finally:
*
*         log(x) = k*ln2 + log(1+f)
*
*     (Here n*ln2 will be stored
*     in two floating-point numbers: n*ln2hi + n*ln2lo; n*ln2hi is exact
*     since the last 20 bits of ln2hi is 0.)
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision natural logarithm of <x>.
*
* Special cases:
*     If <x> < 0 (including -INF), it returns NaN with signal.
*     If <x> is +INF, it returns <x> with no signal.
*     If <x> is 0, it returns -INF with signal.
*     If <x> is NaN it returns <x> with no signal.
*
* SEE ALSO: mathALib
*
* INTERNAL
* Coded in C by K.C. Ng, 1/19/85;
* Revised by K.C. Ng on 2/7/85, 3/7/85, 3/24/85, 4/16/85.
*/

double log
    (
    double x	/* value to compute the natural logarithm of */
    )

    {
	static double zero=0.0, negone= -1.0, half=1.0/2.0;
	double logb(),scalb(),copysign(),log__L(),s,z,t;
	int k,n,finite();

#if !defined(vax)&&!defined(tahoe)
	if(x!=x) return(x);	/* x is NaN */
#endif	/* !defined(vax)&&!defined(tahoe) */
	if(finite(x)) {
	   if( x > zero ) {

	   /* argument reduction */
	      k=logb(x);   x=scalb(x,-k);
	      if(k == -1022) /* subnormal no. */
		   {n=logb(x); x=scalb(x,-n); k+=n;}
	      if(x >= sqrt2 ) {k += 1; x *= half;}
	      x += negone ;

	   /* compute log(1+x)  */
              s=x/(2+x); t=x*x*half;
	      z=k*ln2lo+s*(t+log__L(s*s));
	      x += (z - t) ;

	      return(k*ln2hi+x);
	   }
	/* end of if (x > zero) */

	   else {
#if defined(vax)||defined(tahoe)
		extern double infnan();
		if ( x == zero )
		    return (infnan(-ERANGE));	/* -INF */
		else
		    return (infnan(EDOM));	/* NaN */
#else	/* defined(vax)||defined(tahoe) */
		/* zero argument, return -INF with signal */
		if ( x == zero )
		    return( negone/zero );

		/* negative argument, return NaN with signal */
		else
		    return ( zero / zero );
#endif	/* defined(vax)||defined(tahoe) */
	    }
	}
    /* end of if (finite(x)) */
    /* NOTREACHED if defined(vax)||defined(tahoe) */

    /* log(-INF) is NaN with signal */
	else if (x<0)
	    return(zero/zero);

    /* log(+INF) is +INF */
	else return(x);

    }

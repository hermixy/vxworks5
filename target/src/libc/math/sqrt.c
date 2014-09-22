/* sqrt.c - software version of sqare-root routine */

/* Copyright 1992-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,18nov99,dra  added SPARCV9 support for h/w sqrt.
01g,05feb99,dgp  document errno values
01f,02sep93,jwt  moved sparcHardSqrt to src/arch/sparc/sparcLib.c.
01e,05feb93,jdi  doc changes based on kdl review.
01d,02dec92,jdi  doc tweaks.
01c,28oct92,jdi  documentation cleanup.
01b,13oct92,jdi  mangen fixes.
01a,23jun92,kdl  extracted from v.01d of support.c.
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
* Some IEEE standard 754 recommended functions and remainder and sqrt for
* supporting the C elementary functions.
* -------------------------------------------------------------------------
* WARNING:
*      These codes are developed (in double) to support the C elementary
* functions temporarily. They are not universal, and some of them are very
* slow (in particular, drem and sqrt is extremely inefficient). Each
* computer system should have its implementation of these functions using
* its own assembler.
* -------------------------------------------------------------------------
*
* IEEE 754 required operations:
*     drem(x,p)
*              returns  x REM y  =  x - [x/y]*y , where [x/y] is the integer
*              nearest x/y; in half way case, choose the even one.
*     sqrt(x)
*              returns the square root of x correctly rounded according to
*		the rounding mod.
*
* IEEE 754 recommended functions:
* (a) copysign(x,y)
*              returns x with the sign of y.
* (b) scalb(x,N)
*              returns  x * (2**N), for integer values N.
* (c) logb(x)
*              returns the unbiased exponent of x, a signed integer in
*              double precision, except that logb(0) is -INF, logb(INF)
*              is +INF, and logb(NAN) is that NAN.
* (d) finite(x)
*              returns the value TRUE if -INF < x < +INF and returns
*              FALSE otherwise.
*
*
* CODED IN C BY K.C. NG, 11/25/84;
* REVISED BY K.C. NG on 1/22/85, 2/13/85, 3/24/85.
*
* SEE ALSO: American National Standard X3.159-1989
* NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "private/mathP.h"
#include "errno.h"

extern double	scalb();
extern double	logb();
extern int	finite();

/*******************************************************************************
*
* sqrt - compute a non-negative square root (ANSI)
*
* This routine computes the non-negative square root of <x> in double
* precision.  A domain error occurs if the argument is negative.
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision square root of <x>.
*
* ERROR: EDOM
*
* SEE ALSO: mathALib
*/

double sqrt
    (
    double x	/* value to compute the square root of */
    )

    {
        double q,s,b,r;
        double t,zero=0.0;
        int m,n,i;

#if defined(vax)||defined(tahoe)
        int k=54;
#else	/* defined(vax)||defined(tahoe) */
        int k=51;
#endif	/* defined(vax)||defined(tahoe) */

	/* Select hardware/software square root */
#if	(CPU_FAMILY == SPARC) || (CPU_FAMILY == SPARCV9)
        extern BOOL sparcHardSqrt;

        if (sparcHardSqrt == TRUE)
	    {
	    double  sqrtHw();

	    return (sqrtHw (x));
	    }
#endif	/* (CPU_FAMILY == SPARC) */

    /* sqrt(NaN) is NaN, sqrt(+-0) = +-0 */
        if(x!=x||x==zero) return(x);

    /* sqrt(negative) is invalid */
        if(x<zero) {
#if defined(vax)||defined(tahoe)
		extern double infnan();
		return (infnan(EDOM));	/* NaN */
#else	/* defined(vax)||defined(tahoe) */
		errno = EDOM; 
		return(zero/zero);
#endif	/* defined(vax)||defined(tahoe) */
	}

    /* sqrt(INF) is INF */
        if(!finite(x)) return(x);

    /* scale x to [1,4) */
        n=logb(x);
        x=scalb(x,-n);
        if((m=logb(x))!=0) x=scalb(x,-m);       /* subnormal number */
        m += n;
        n = m/2;
        if((n+n)!=m) {x *= 2; m -=1; n=m/2;}

    /* generate sqrt(x) bit by bit (accumulating in q) */
            q=1.0; s=4.0; x -= 1.0; r=1;
            for(i=1;i<=k;i++) {
                t=s+1; x *= 4; r /= 2;
                if(t<=x) {
                    s=t+t+2, x -= t; q += r;}
                else
                    s *= 2;
                }

    /* generate the last bit and determine the final rounding */
            r/=2; x *= 4;
            if(x==zero) goto end; 100+r; /* trigger inexact flag */
            if(s<x) {
                q+=r; x -=s; s += 2; s *= 2; x *= 4;
                t = (x-s)-5;
                b=1.0+3*r/4; if(b==1.0) goto end; /* b==1 : Round-to-zero */
                b=1.0+r/4;   if(b>1.0) t=1;	/* b>1 : Round-to-(+INF) */
                if(t>=0) q+=r; }	      /* else: Round-to-nearest */
            else {
                s *= 2; x *= 4;
                t = (x-s)-1;
                b=1.0+3*r/4; if(b==1.0) goto end;
                b=1.0+r/4;   if(b>1.0) t=1;
                if(t>=0) q+=r; }

end:        return(scalb(q,n));
    }

/* support.c - math routines */

/* Copyright 1991-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,03oct01,to   use IEEE little endian for ARM
01i,20apr01,h_k  fixed P_INDEX for ARM big_endian.
01h,11jan99,dra  removed non-ANSI aliasing code.
01g,12feb97,jpd  added support for ARM with early Cygnus release.
01f,23sep92,kdl  removed define of sparcHardSqrt (now in sqrt.c).
01e,23sep92,kdl  removed sqrt(), placed in sqrt.c.
01d,20sep92,smb  documentation additions.
01c,08jul92,smb  merged with ANSI and documentation.
                 set EDOM in sqrt().
01b,02jul92,jwt  changed sysHardSqrt to sparcHardSqrt; removed warnings.
01a,25jun91,jwt  created sqrt() function select - hard versus soft.
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
******************************************************************************
* WARNING:
*      These codes are developed (in double) to support the C elementary
* functions temporarily. They are not universal, and some of them are very
* slow (in particular, drem and sqrt is extremely inefficient). Each
* computer system should have its implementation of these functions using
* its own assembler.
******************************************************************************
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
* 25jun91,jwt  created sqrt() function select - hard versus soft
SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "private/mathP.h"
#include "errno.h"

#if (_BYTE_ORDER == _LITTLE_ENDIAN)
#define P_INDEX	3
#else	/* little_endian */
#define P_INDEX	0
#endif	/* little_endian */

typedef union
    {
    unsigned short	s[4];
    double		d;
    } tdouble;

#if defined(vax)||defined(tahoe)      /* VAX D format */
#include <errno.h>
    static unsigned short msign=0x7fff , mexp =0x7f80 ;
    static short  prep1=57, gap=7, bias=129           ;
    static double novf=1.7E38, nunf=3.0E-39, zero=0.0 ;
#else	/* defined(vax)||defined(tahoe) */
    static unsigned short msign=0x7fff, mexp =0x7ff0  ;
    static short prep1=54, gap=4, bias=1023           ;
    static double novf=1.7E308, nunf=3.0E-308,zero=0.0;
#endif	/* defined(vax)||defined(tahoe) */

/*****************************************************************************
* scalb	-
*
* RETURN:
* NOMANUAL
*/

double scalb(x,N)
double x; int N;
{
        int k;
        double scalb();
	tdouble y;

	y.d = x;
        if( y.d == zero )
	    return(y.d);

#if defined(vax)||defined(tahoe)
        if( (k= y.s[P_INDEX] & mexp ) != ~msign )
	    {
            if (N < -260)
		return(nunf*nunf);
	    else if (N > 260) {
		extern double infnan(),copysign();
		return(copysign(infnan(ERANGE),y.d));
	    }
#else	/* defined(vax)||defined(tahoe) */
        if( (k= y.s[P_INDEX] & mexp ) != mexp )
	    {
            if( N<-2100)
		return(nunf*nunf);
	    else if(N>2100)
		return(novf+novf);
            if( k == 0 )
		{
		y.d *= scalb(1.0,(int)prep1);
		N -= prep1;
		return(scalb(y.d,N));
		}
#endif	/* defined(vax)||defined(tahoe) */

            if((k = (k>>gap)+ N) > 0 )
                if( k < (mexp>>gap) )
		    y.s[P_INDEX] = (y.s[P_INDEX]&~mexp) | (k<<gap);
                else
		    x=novf+novf;               /* overflow */
            else
                if( k > -prep1 )
                                        /* gradual underflow */
                    {
		    y.s[P_INDEX]=(y.s[P_INDEX]&~mexp)|(short)(1<<gap);
		    y.d *= scalb(1.0,k-1);
		    }
                else
                return(nunf*nunf);
            }
        return(y.d);
}


/*****************************************************************************
* copysign - determine the sign
*
* RETURN:
* NOMANUAL
*/

double copysign(x,y)
double x,y;
{
	tdouble a, b;

	a.d = x;
	b.d = y;

#if defined(vax)||defined(tahoe)
        if ( (a.s[P_INDEX] & mexp) == 0 ) return(a.d);
#endif	/* defined(vax)||defined(tahoe) */

        a.s[P_INDEX] = ( a.s[P_INDEX] & msign ) | ( b.s[P_INDEX] & ~msign );
        return(a.d);
}

/*****************************************************************************
* logb -
*
* RETURN:
* NOMANUAL
*/

double logb(x)
double x;
{
	tdouble a;
	short k;

	a.d = x;
#if defined(vax)||defined(tahoe)
        return (int)(((a.s[P_INDEX]&mexp)>>gap)-bias);
#else	/* defined(vax)||defined(tahoe) */
        if( (k= a.s[P_INDEX] & mexp ) != mexp )
            if ( k != 0 )
                return ( (k>>gap) - bias );
            else if( a.d != zero)
                return ( -1022.0 );
            else
                return(-(1.0/zero));
        else if(a.d != a.d)
            return(a.d);
        else
            {a.s[P_INDEX] &= msign; return(a.d);}
#endif	/* defined(vax)||defined(tahoe) */
}

/*****************************************************************************
* finite - the finite value for this machine 
*
* RETURN:
* NOMANUAL
*/

int finite(x)
double x;
{
	tdouble a;
	a.d = x;
#if defined(vax)||defined(tahoe)
        return(1);
#else	/* defined(vax)||defined(tahoe) */
        return( ( a.s[P_INDEX] & mexp ) != mexp );
#endif	/* defined(vax)||defined(tahoe) */
}

/*****************************************************************************
* drem - remainder
*
* RETURN:
* NOMANUAL
*/

double drem(x,p)
double x,p;
{
        short sign;
        double hp, drem(),scalb();
	tdouble tmp, xd, pd, dp;
        unsigned short  k;

	xd.d = x;
	pd.d = p;

        pd.s[P_INDEX] &= msign;

#if defined(vax)||defined(tahoe)
        if( ( xd.s[P_INDEX] & mexp ) == ~msign )	/* is x a reserved operand? */
#else	/* defined(vax)||defined(tahoe) */
        if( ( xd.s[P_INDEX] & mexp ) == mexp )
#endif	/* defined(vax)||defined(tahoe) */
		return  (xd.d-pd.d)-(xd.d-pd.d);	/* create nan if x is inf */
	if (pd.d == zero) {
#if defined(vax)||defined(tahoe)
		extern double infnan();
		return(infnan(EDOM));
#else	/* defined(vax)||defined(tahoe) */
		return zero/zero;
#endif	/* defined(vax)||defined(tahoe) */
	}

#if defined(vax)||defined(tahoe)
        if( ( pd.s[P_INDEX] & mexp ) == ~msign )	/* is p a reserved operand? */
#else	/* defined(vax)||defined(tahoe) */
        if( ( pd.s[P_INDEX] & mexp ) == mexp )
#endif	/* defined(vax)||defined(tahoe) */
		{ if (pd.d != pd.d) return pd.d; else return xd.d;}

        else  if ( ((pd.s[P_INDEX] & mexp)>>gap) <= 1 )
                /* subnormal p, or almost subnormal p */
            { double b; b=scalb(1.0,(int)prep1);
              pd.d *= b; xd.d = drem(xd.d,pd.d); xd.d *= b; return(drem(xd.d,pd.d)/b);}
        else  if ( pd.d >= novf/2)
            { pd.d /= 2 ; xd.d /= 2; return(drem(xd.d,pd.d)*2);}
        else
            {
                dp.d=pd.d+pd.d;
		hp=pd.d/2;
                sign= xd.s[P_INDEX] & ~msign ;
                xd.s[P_INDEX] &= msign       ;
                while ( xd.d > dp.d )
                    {
                        k=(xd.s[P_INDEX] & mexp) - (dp.s[P_INDEX] & mexp) ;
                        tmp.d = dp.d ;
                        tmp.s[P_INDEX] += k ;

#if defined(vax)||defined(tahoe)
                        if( xd.d < tmp.d ) tmp.s[P_INDEX] -= 128 ;
#else	/* defined(vax)||defined(tahoe) */
                        if( xd.d < tmp.d ) tmp.s[P_INDEX] -= 16 ;
#endif	/* defined(vax)||defined(tahoe) */

                        xd.d -= tmp.d ;
                    }
                if ( xd.d > hp )
                    { xd.d -= pd.d ;  if ( xd.d >= hp ) xd.d -= pd.d ; }

#if defined(vax)||defined(tahoe)
		if (xd.d)
#endif	/* defined(vax)||defined(tahoe) */
			xd.s[P_INDEX] ^= sign;
                return( xd.d);

            }
}


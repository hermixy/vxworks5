/* rint.c - round-to-integer routine */

/* Copyright 1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,30jul92,smb  documentation.
*/

/*
DESCRIPTION
 * algorithm for rint(x) in pseudo-pascal form ...
 *
 * real rint(x): real x;
 *      ... delivers integer nearest x in direction of prevailing rounding
 *      ... mode
 * const        L = (last consecutive integer)/2
 *        = 2**55; for VAX D
 *        = 2**52; for IEEE 754 Double
 * real s,t;
 * begin
 *      if x != x then return x;                ... NaN
 *      if |x| >= L then return x;              ... already an integer
 *      s := copysign(L,x);
 *      t := x + s;                             ... = (x+s) rounded to integer
 *      return t - s
 * end;
 *
 * Note: Inexact will be signaled if x is not an integer, as is
 *      customary for IEEE 754.  No other signal can be emitted.

SEE ALSO: American National Standard X3.159-1989
*/

#include "vxWorks.h"
#include "math.h"
#include "stddef.h"
#include "errno.h"
#include "private/mathP.h"

#if defined(vax)||defined(tahoe)
#ifdef vax
#define _0x(A,B)        0x/**/A/**/B
#else   /* vax */
#define _0x(A,B)        0x/**/B/**/A
#endif  /* vax */
static long Lx[] = {_0x(0000,5c00),_0x(0000,0000)};     /* 2**55 */
#define L *(double *) Lx
#else   /* defined(vax)||defined(tahoe) */
static double L = 4503599627370496.0E0;         /* 2**52 */
#endif  /* defined(vax)||defined(tahoe) */

/********************************************************************************     
* rint -  needed for sparc math support 
* 
* RETURNS: 
*/

double
rint(x)
double x;
{
        double s,t,one = 1.0,copysign();
#if !defined(vax)&&!defined(tahoe)
        if (x != x)                             /* NaN */
                return (x);
#endif  /* !defined(vax)&&!defined(tahoe) */
        if (copysign(x,one) >= L)               /* already an integer */
            return (x);
        s = copysign(L,x);
        t = x + s;                              /* x+s rounded to integer */
        return (t - s);
}

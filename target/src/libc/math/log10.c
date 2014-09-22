/* log10.c - math routine */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
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

#if defined(vax)||defined(tahoe)	/* VAX D format (56 bits) */
#ifdef vax
#define _0x(A,B)	0x/**/A/**/B
#else	/* vax */
#define _0x(A,B)	0x/**/B/**/A
#endif	/* vax */
/* static double */
/* ln10hi =  2.3025850929940456790E0     ; Hex   2^  2   *  .935D8DDDAAA8AC */
static long    ln10hix[] = { _0x(5d8d,4113), _0x(a8ac,ddaa)};
#define   ln10hi    (*(double*)ln10hix)
#else	/* defined(vax)||defined(tahoe) */
static double
ivln10 =  4.3429448190325181667E-1    ; /*Hex   2^ -2   *  1.BCB7B1526E50E */
#endif	/* defined(vax)||defined(tahoe) */

/*******************************************************************************
*
* log10 - compute a base-10 logarithm (ANSI)
*
* This routine returns the base 10 logarithm of <x> in
* double precision (IEEE double, 53 bits).
*
* A domain error occurs if the argument is negative.  A range error may
* if the argument is zero.
*
* INTERNAL:
* Method:
*                  log(x)
*     log10(x) = ---------  or  [1/log(10)]*log(x)
*                 log(10)
*
*     [log(10)]   rounded to 56 bits has error  .0895  ulps,
*     [1/log(10)] rounded to 53 bits has error  .198   ulps;
*     therefore, for better accuracy, in VAX D format, we divide
*     log(x) by log(10), but in IEEE Double format, we multiply
*     log(x) by [1/log(10)].
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision base-10 logarithm of <x>.
*
* Special cases:
*     If <x> < 0, log10() returns NaN with signal.
*     if <x> is +INF, it returns <x> with no signal.
*     if <x> is 0, it returns -INF with signal.
*     if <x> is NaN it returns <x> with no signal.
*
* SEE ALSO: mathALib
*
* INTERNAL:
* Coded in C by K.C. Ng, 1/20/85;
* Revised by K.C. Ng on 1/23/85, 3/7/85, 4/16/85.
*/

double log10
    (
    double x	/* value to compute the base-10 logarithm of */
    )

    {
	double log();

#if defined(vax)||defined(tahoe)
	return(log(x)/ln10hi);
#else	/* defined(vax)||defined(tahoe) */
	return(ivln10*log(x));
#endif	/* defined(vax)||defined(tahoe) */
    }

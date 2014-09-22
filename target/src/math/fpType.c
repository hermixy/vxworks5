/* fpType.c - floating point type functions */

/* Copyright 1992-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,03oct01,to   use IEEE little endian for ARM
01g,16may00,pai  Implemented fixes for incorrect mask usage in fpTypeGet
                 (SPR #7515), (SPR #8515), (SPR #20508).
01f,23jan97,cdp  reverse words in double for ARM.
01e,19mar95,dvs  removed tron reference - no longer supported.
01d,01sep93,hdn  added byte order condition for I80X86.
01c,17aug92,kdl  added dummy pointer declaration to work around tron gcc bug.
01b,30jul92,kdl  removed unused static variables.
01a,30jul92,kdl  created.
*/

/*
DESCRIPTION

This file contains a routine, fpTypeGet(), which will return the
type of a floating point number, where the type is one of NULL,
NAN, INTEGER, INF, or REAL.

NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "private/mathP.h"

#if (_BYTE_ORDER == _BIG_ENDIAN)

#define LOW	0		/* array indices for mantissa */
#define HIGH	1

#else

#define LOW	1		/* array indices for mantissa */
#define HIGH	0

#endif /* _BYTE_ORDER == _BIG_ENDIAN */


static unsigned map[32] =
    {
    0xffffffff, 0xfffffffe, 0xfffffffc, 0xfffffff8,
    0xfffffff0, 0xffffffe0, 0xffffffc0, 0xffffff80,
    0xffffff00, 0xfffffe00, 0xfffffc00, 0xfffff800,
    0xfffff000, 0xffffe000, 0xffffc000, 0xffff8000,
    0xffff0000, 0xfffe0000, 0xfffc0000, 0xfff80000,
    0xfff00000, 0xffe00000, 0xffc00000, 0xff800000,
    0xff000000, 0xfe000000, 0xfc000000, 0xf8000000,
    0xf0000000, 0xe0000000, 0xc0000000, 0x80000000,
    };

/*******************************************************************************
*
* fpTypeGet - determine type of fp number
*
* This routine examines a double floating point value and returns
* a code indicating the type (i.e. ZERO, INF, NAN, REAL, INT).
*
* RETURNS: encoded type (value).
* 
* NOMANUAL
*/

int fpTypeGet
    (
    double 	v, 		/* value */
    double *	r		/* pointer to integral */
    )
    {
    __cv_type	cv;
    int  	exp;
    BOOL 	any;


    cv.p_double = v;

    if (((cv.p_mant[LOW] & 0x7fffffff) == 0) && (cv.p_mant[HIGH] == 0))
        {
        if (r != NULL)
            *r = v;
        return (ZERO);

        }

    exp = ((cv.p_mant[LOW] & 0x7ff00000) >> 20);

    if (exp == 2047)
        {
        if (r != NULL)
            *r = v;
        return (((cv.p_mant[LOW] & 0x000fffff) == 0) && 
		(cv.p_mant[HIGH] == 0)) ? INF : NAN;
        }

    if (r == NULL)
        return (REAL);

    exp -= 1023;
    any = FALSE;

    if (exp < 0)
        {
        any = TRUE;
        cv.p_mant[LOW] &= 0x80000000;
        cv.p_mant[HIGH] = 0;
        }
    else if (exp < 20)
        {
        if ((cv.p_mant[HIGH] != 0) || ((cv.p_mant[LOW] & ~(map[20 - exp])) != 0))
            {
            any = TRUE;
            cv.p_mant[HIGH] = 0;
            cv.p_mant[LOW] &= map[20 - exp];
            }
        }
    else if (exp == 20)
        {
        if ((cv.p_mant[HIGH] & ~(0x00000000)) != 0)
            {
            any = TRUE;
            cv.p_mant[HIGH] &= 0x00000000;
            }
        }
    else if (exp < 52)
        {
        if ((cv.p_mant[HIGH] & ~(map [52 - exp])) != 0)
            {
            any = TRUE;
            cv.p_mant[HIGH] &= map [52 - exp];
            }
        }

    *r = cv.p_double;

    return (any ? REAL : INTEGER);
    }

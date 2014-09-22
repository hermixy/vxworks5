/* frexp.c - frexp math routine */

/* Copyright 1992-1996 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,23may01,to   doc fix
01i,05feb99,dgp  document errno values
01h,05aug96,dbt  frexp now returned a value >= 0.5 and strictly less than 1.0.
		 (SPR #4280).
		 Updated copyright.
01g,05feb93,jdi  doc changes based on kdl review.
01f,02dec92,jdi  doc tweaks.
01e,28oct92,jdi  documentation cleanup.
01d,21sep92,smb  replaced orignal versions.
01c,20sep92,smb  documentation additions
01b,30jul92,kdl  replaced routine contents with frexp() from floatLib.c.
01a,08jul92,smb  written, documentation.
*/

/*
DESCRIPTION

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "math.h"
#include "stddef.h"
#include "errno.h"
#include "private/mathP.h"

/*******************************************************************************
*
* frexp - break a floating-point number into a normalized fraction and power of 2 (ANSI)
*
* This routine breaks a double-precision number <value> into a normalized
* fraction and integral power of 2.  It stores the integer exponent in <pexp>.
*
* INCLUDE FILES: math.h
*
* RETURNS: 
* The double-precision value <x>, such that the magnitude of <x> is
* in the interval [1/2,1) or zero, and <value> equals <x> times
* 2 to the power of <pexp>. If <value> is zero, both parts of the result
* are zero.
* 
* ERRNO: EDOM
*
*/
double frexp
    (
    double value,	/* number to be normalized */
    int *pexp   	/* pointer to the exponent */
    )

    {
    double r;

    *pexp = 0;

    /* determine number type */

    switch (fpTypeGet(value, &r))	
      	{
        case NAN:		/* NOT A NUMBER */
    	case INF:		/* INFINITE */
            errno = EDOM;
	    return (value);

    	case ZERO:		/* ZERO */
	    return (0.0);

	default:
	    /*
	     * return value must be strictly less than 1.0 and >=0.5 .
	     */
	    if ((r = fabs(value)) >= 1.0)
	        for(; (r >= 1.0); (*pexp)++, r /= 2.0);
	    else
	        for(; (r < 0.5); (*pexp)--, r *= 2.0);

	    return (value < 0 ? -r : r);
        }
    }

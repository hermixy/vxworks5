/* ldexp.c - ldexp math routine */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,05feb99,dgp  document errno values
01g,05feb93,jdi  doc changes based on kdl review.
01f,02dec92,jdi  doc tweaks.
01e,28oct92,jdi  documentation cleanup.
01d,21sep92,smb  replaced original version.
01c,20sep92,smb  documentation additions
01b,30jul92,kdl  replaced routine contents with ldexp() from floatLib.c.
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
* ldexp - multiply a number by an integral power of 2 (ANSI)
*
* This routine multiplies a floating-point number by an integral power of 2.
* A range error may occur.
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision value of <v> times 2 to the power of <xexp>.
*
* ERRNO: EDOM, ERANGE
*
*/

double ldexp
    (
    double v,		/* a floating point number */
    int xexp		/* exponent                */
    )

    {
    double zero = 0.0;

    switch (fpTypeGet (v, NULL))	
       	{
       	case NAN:		/* NOT A NUMBER */
            errno = EDOM;
	    break;

    	case INF:		/* INFINITE */
	    errno = ERANGE;
	    break;

    	case ZERO:		/* ZERO */
	    return (zero);

	default:
	    if (xexp >= 0)
		for(; (xexp > 0); xexp--, v *= 2.0);
	    else
		for(; (xexp < 0); xexp++, v /= 2.0);

	    return (v);
    	}

    return (zero);
    }

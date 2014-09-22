/* modf.c - separate floating-point number into integer and fraction parts */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,19mar95,dvs  removed TRON ifdef - no longer supported.
01i,05feb93,jdi  doc changes based on kdl review.
01h,02dec92,jdi  doc tweaks.
01g,28oct92,jdi  documentation cleanup.
01f,20sep92,smb  documentation additions
01e,10sep92,wmd  changed dummy function for i960KB from printf to bcopy.
01d,04sep92,wmd  restored to rev 1b, instead modified mathP.h using  _BYTE_ORDER 
		 conditionals to flip the contents of struct DOUBLE.  Added a dummy
		 printf() in order to force compiler to compile code correctly for i960kb.
01c,03sep92,wmd  modified modf() for the i960, word order for DOUBLE is reversed.
01b,25jul92,kdl	 replaced modf() routine contents with version from floatLib.c;
		 deleted isnan(), iszero(), isinf(), _d_type().
01a,08jul92,smb  documentation.
*/

/*
DESCRIPTION

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "math.h"
#include "stddef.h"
#include "errno.h"
#include "private/mathP.h"



/*******************************************************************************
*
* modf - separate a floating-point number into integer and fraction parts (ANSI)
*
* This routine stores the integer portion of <value>
* in <pIntPart> and returns the fractional portion.
* Both parts are double precision and will have the same sign as <value>.
*
* INCLUDE FILES: math.h
*
* RETURNS: The double-precision fractional portion of <value>.
*
* SEE ALSO: frexp(), ldexp()
*/

double modf
    (
    double value,               /* value to split                  */
    double *pIntPart            /* where integer portion is stored */
    )
    {
    DOUBLE 	dat;
    FAST int 	exp;
    FAST double fracPart;
    FAST double intPart;
    int 	negflag = (value < 0);

    if (negflag)
        value = -value;		/* make it positive */

    dat.ddat = value;

    /* Separate the exponent, and change it from biased to 2's comp. */
    exp = ((dat.ldat.l1 & 0x7ff00000) >> 20) - 1023;

#if CPU==I960KB
    bcopy ((char *)&negflag, (char *)&negflag, 0);
#endif  /* CPU==I960KB - to force gcc960 to compile correct code for the i960kb */
    
    if (exp <= -1)
        {
	/* If exponent is negative, fracPart == |value|, and intPart == 0. */

        fracPart = value;
        intPart = 0.;
        }

    /* clear the fractional part in dat */

    else if (exp <= 20)
        {
        dat.ldat.l1 &= (-1 << (20 - exp));
        dat.ldat.l2 = 0;
        intPart = dat.ddat;
        fracPart = value - intPart;
        }

    else if (exp <= 52)
        {
        dat.ldat.l2 &= (-1 << (52 - exp));
        intPart = dat.ddat;
        fracPart = value - intPart;
	}

    else
        {
        fracPart = 0.;
        intPart = value;
        }

    *pIntPart = (negflag ? -intPart : intPart);
    return (negflag ? -fracPart : fracPart);
    }


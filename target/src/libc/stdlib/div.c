/* div.c - div file for stdlib  */

/* Copyright 1992-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,25sep01,gls  fixed div_r to check for negative numerator (SPR #30636)
01f,10feb95,jdi  doc format tweak.
01e,08feb93,jdi  documentation cleanup for 5.1.
01d,20sep92,smb  documentation additions.
01c,24jul92,smb  corrected parameter ordering.
01b,24jul92,smb  added reentrant version for div()
01a,19jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/*******************************************************************************
*
* div - compute a quotient and remainder (ANSI)
*
* This routine computes the quotient and remainder of <numer>/<denom>.
* If the division is inexact, the resulting quotient is the integer of lesser
* magnitude that is the nearest to the algebraic quotient.  If the result cannot
* be represented, the behavior is undefined; otherwise, `quot' * <denom> + `rem'
* equals <numer>.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS:
* A structure of type `div_t', containing both the quotient and the 
* remainder. 
*
* INTERNAL
* The structure shall contain the following members, in either order:
*		int quot; * quotient *
*		int rem;  * remainder *
*/

div_t div 
    (
    int numer, 		/* numerator */
    int denom		/* denominator */
    ) 
    {
    static div_t divStruct;	/* div_t structure */

    div_r (numer, denom, &divStruct);
    return (divStruct);
    }

/*******************************************************************************
*
* div_r - compute a quotient and remainder (reentrant)
*
* This routine computes the quotient and remainder of <numer>/<denom>.
* The quotient and remainder are stored in the `div_t' structure
* pointed to by <divStructPtr>.
*
* This routine is the reentrant version of div().
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: N/A
*/

void div_r
    (
    int     numer,          /* numerator */
    int     denom,          /* denominator */
    div_t * divStructPtr    /* div_t structure */
    )
    {
    /* calculate quotient */
    divStructPtr->quot = numer / denom;

    /* calculate remainder */
    divStructPtr->rem = numer - (denom * divStructPtr->quot);

    /* check for negative numerator */
    if ((numer < 0) && (divStructPtr->rem > 0))
        {
        divStructPtr->quot++;
        divStructPtr->rem -= denom;
        }
    }

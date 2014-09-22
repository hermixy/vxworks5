/* ldiv.c - ldiv file for stdlib  */

/* Copyright 1992-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,25sep01,gls  fixed ldiv_r to check for negative numerator (SPR #30636)
01e,08feb93,jdi  documentation cleanup for 5.1.
01d,20sep92,smb  documentation additions.
01c,24jul92,smb  corrected parameter ordering.
01b,24jul92,smb  added reentrant version of ldiv()
01a,19jul92,smb  written.
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
* ldiv - compute the quotient and remainder of the division (ANSI)
*
* This routine computes the quotient and remainder of <numer>/<denom>.
* This routine is similar to div(), except that the arguments and the
* elements of the returned structure are all of type `long'.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS:
* A structure of type `ldiv_t', containing both the quotient and the 
* remainder. 
*/

ldiv_t ldiv 
    (
    long numer, 	/* numerator */
    long denom		/* denominator */
    ) 
    {
    static ldiv_t divStruct;		

    ldiv_r (numer, denom, &divStruct); 
    return (divStruct);
    }
 
/*******************************************************************************
*
* ldiv_r - compute a quotient and remainder (reentrant)
*
* This routine computes the quotient and remainder of <numer>/<denom>.
* The quotient and remainder are stored in the `ldiv_t' structure
* `divStructPtr'.
*
* This routine is the reentrant version of ldiv().
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: N/A
*/

void ldiv_r
    (
    long     numer,          /* numerator */
    long     denom,          /* denominator */
    ldiv_t * divStructPtr    /* ldiv_t structure */
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

/* fixunssfsi() -- routine to convert a float to an unsigned 			*/

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,13sep94,ism	 created.
*/

/*
DESCRIPTION
This library contains the fixunssfsi() routine required by the GNU
compiler.  This addition fixes SPR #2649.

AUTHOR

NOMANUAL
*/


#include "vxWorks.h"

/*******************************************************************************
*
* __fixunssfsi - truncate a float to an unsigned
*
* WARNING: This function is pass a float but we declare it a long so
* we can fiddle with the bits.  If the value comes in on a floating
* point register (such as with the sparc compiler), this aint gona work.
*
* The magic numbers 23 and 0x7f appear frequently. Read the ieee spec for
* more info.
*/
unsigned __fixunssfsi
    (
    long	a			/* this is really a float (yuk) */
    )
    {
    int exp;				/* the exponent inside the float */
 
    /*
     * This handles numbers less than 1.0 including ALL negitive numbers.
     */
    if (a < (0x7f<<23))
        return (0);

    /*
     * This handles all numbers to big.
     */
    if (a >= ((0x7f+32)<<23))
        return (0xffffffff);		/* return MAX_UINT */
 
    /*
     * Get the exponent, then clear it out to leave just the mantissa.
     */
    exp = a>>23;
    a &= 0x007fffff;		/* ((1<<23) - 1) */
    a |= 0x00800000;		/* (1<<23) */
 
    /*
     * Shift the mantissa to the correct place, if exp == (0x7f+23) then
     * it is already at the right place.
     */
    if (exp > (0x7f+23))
        a <<= (exp - (0x7f+23));
    else if (exp < (0x7f+23))
        a >>= ((0x7f+23) - exp);

    return (a);
    }

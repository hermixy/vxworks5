/* rand.c - rand, srand functions for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
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

ulong_t _Randseed = 1;

/*******************************************************************************
*
* rand - generate a pseudo-random integer between 0 and RAND_MAX  (ANSI)
*
* This routine generates a pseudo-random integer between 0 and RAND_MAX.
* The seed value for rand() can be reset with srand().
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: A pseudo-random integer.
*
* SEE ALSO: srand()
*/

int rand (void) 
    {
    _Randseed = _Randseed * 1103515245 + 12345;
    return (uint_t) (_Randseed/65536) % (RAND_MAX + 1);
    }

/*******************************************************************************
*
* srand - reset the value of the seed used to generate random numbers (ANSI)
*
* This routine resets the seed value used by rand().  If srand() is then
* called with the same seed value, the sequence of pseudo-random numbers is
* repeated.  If rand() is called before any calls to srand() have been made,
* the same sequence shall be generated as when srand() is first called with
* the seed value of 1.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: N/A
*
* SEE ALSO: rand()
*/

void * srand 
    (
    uint_t seed		/* random number seed */
    ) 
    {
    _Randseed = seed;
    return (void *)0;
    }

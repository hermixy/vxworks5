/* strcspn.c - search string for character, string */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,08jul92,smb  written and documented.
*/

/*
DESCRIPTION

INCLUDE FILES: string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"


/*******************************************************************************
*
* strcspn - return the string length up to the first character from a given set (ANSI)
*
* This routine computes the length of the maximum initial segment of string
* <s1> that consists entirely of characters not included in string <s2>.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* The length of the string segment.
*
* SEE ALSO: strpbrk(), strspn()
*/
 
size_t strcspn   
    (
    const char * s1,	/* string to search */
    const char * s2	/* set of characters to look for in <s1> */
    )
    {
    const char *save;
    const char *p;
    char 	c1;
    char 	c2;

    for (save = s1 + 1; (c1 = *s1++) != EOS; )	/* search for EOS */
	for (p = s2; (c2 = *p++) != EOS; )	/* search for first occurance */
	    {
	    if (c1 == c2)
		return (s1 - save);	      	/* return index of substring */
            }

    return (s1 - save);
    }

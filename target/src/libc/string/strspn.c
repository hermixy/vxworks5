/* strspn.c - string search, string */

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


/******************************************************************************
*
* strspn - return the string length up to the first character not in a given set (ANSI)
*
* This routine computes the length of the maximum initial segment of
* string <s> that consists entirely of characters from the string <sep>.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* The length of the string segment.
*
* SEE ALSO: strcspn()
*/
 
size_t strspn
    (
    const char * s,             /* string to search */
    const char * sep            /* set of characters to look for in <s> */
    )
    {
    const char *save;
    const char *p;
    char c1;
    char c2;

    for (save = s + 1; (c1 = *s++) != EOS; )
	for (p = sep; (c2 = *p++) != c1; )
	    {
	    if (c2 == EOS)
		return (s - save);
            }

    return (s - save);
    }

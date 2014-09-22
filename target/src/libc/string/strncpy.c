/* strncpy.c - string copy, string */

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
* strncpy - copy characters from one string to another (ANSI)
*
* This routine copies <n> characters from string <s2> to string <s1>.
* If <n> is greater than the length of <s2>, nulls are added to <s1>.
* If <n> is less than or equal to the length of <s2>, the target
* string will not be null-terminated.
*
* INCLUDE FILES: string.h
*
* RETURNS: A pointer to <s1>.
*/

char *strncpy
    (
    char *      s1,   	/* string to copy to */
    const char *s2,   	/* string to copy from */
    size_t      n      	/* max no. of characters to copy */
    )
    {
    FAST char *d = s1;

    if (n != 0)
	{
	while ((*d++ = *s2++) != 0)	/* copy <s2>, checking size <n> */
	    {
	    if (--n == 0)
		return (s1);
            }

	while (--n > 0)
	    *d++ = EOS;			/* NULL terminate string */
	}

    return (s1);
    }

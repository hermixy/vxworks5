/* strncmp.c - string compare, string */

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
* strncmp - compare the first <n> characters of two strings (ANSI)
*
* This routine compares up to <n> characters of string <s1> to string <s2>
* lexicographically.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* An integer greater than, equal to, or less than 0, according to whether
* <s1> is lexicographically greater than, equal to, or less than <s2>,
* respectively.
*/

int strncmp
    (
    const char * s1,           	/* string to compare */
    const char * s2,           	/* string to compare <s1> to */
    size_t       n             	/* max no. of characters to compare */
    )
    {
    if (n == 0)
	return (0);

    while (*s1++ == *s2++)
	{
	if ((s1 [-1] == EOS) || (--n == 0))
	    return (0);
        }

    return ((s1 [-1]) - (s2 [-1]));
    }

/* strstr.c - file for string */

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
* strstr - find the first occurrence of a substring in a string (ANSI)
*
* This routine locates the first occurrence in string <s>
* of the sequence of characters (excluding the terminating null character)
* in the string <find>.
*
* INCLUDE FILES: string.h
*
* RETURNS:
* A pointer to the located substring, or <s> if <find> points to a
* zero-length string, or NULL if the string is not found.
*/

char * strstr
    (
    const char * s,        /* string to search */
    const char * find      /* substring to look for */
    )
    {
    char *t1;
    char *t2;
    char c;
    char c2;

    if ((c = *find++) == EOS)		/* <find> an empty string */
	return (CHAR_FROM_CONST(s));

    FOREVER
	{
	while (((c2 = *s++) != EOS) && (c2 != c))
	    ;

	if (c2 == EOS)
	    return (NULL);

	t1 = CHAR_FROM_CONST(s);
	t2 = CHAR_FROM_CONST(find); 

	while (((c2 = *t2++) != 0) && (*t1++ == c2))
	    ;

	if (c2 == EOS)
	    return (CHAR_FROM_CONST(s - 1));
	}
    }

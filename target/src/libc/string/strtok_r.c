/* strtok_r.c - file for string */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,23oct95,jdi  doc: added comment that input string will be
		    changed (SPR 4874).
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


/*****************************************************************************
*
* strtok_r - break down a string into tokens (reentrant) (POSIX)
*
* This routine considers the null-terminated string <string> as a sequence
* of zero or more text tokens separated by spans of one or more characters
* from the separator string <separators>.  The argument <ppLast> points to a
* user-provided pointer which in turn points to the position within <string>
* at which scanning should begin.
*
* In the first call to this routine, <string> points to a null-terminated
* string; <separators> points to a null-terminated string of separator
* characters; and <ppLast> points to a NULL pointer.  The function returns a
* pointer to the first character of the first token, writes a null character
* into <string> immediately following the returned token, and updates the
* pointer to which <ppLast> points so that it points to the first character
* following the null written into <string>.  (Note that because the
* separator character is overwritten by a null character, the input string
* is modified as a result of this call.)
*
* In subsequent calls <string> must be a NULL pointer and <ppLast> must
* be unchanged so that subsequent calls will move through the string <string>,
* returning successive tokens until no tokens remain.  The separator
* string <separators> may be different from call to call.  When no token
* remains in <string>, a NULL pointer is returned.
*
* INCLUDE FILES: string.h
* 
* RETURNS
* A pointer to the first character of a token,
* or a NULL pointer if there is no token.
*
* SEE ALSO: strtok()
*/

char * strtok_r
    (
    char *       string,	/* string to break into tokens */
    const char * separators,	/* the separators */
    char **      ppLast		/* pointer to serve as string index */
    )
    {
    if ((string == NULL) && ((string = *ppLast) == NULL))
	return (NULL);

    if (*(string += strspn (string, separators)) == EOS)
	return (*ppLast = NULL);

    if ((*ppLast = strpbrk (string, separators)) != NULL)
	*(*ppLast)++ = EOS;

    return (string);
    }

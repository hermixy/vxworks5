/* strtok.c - file for string */

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


/******************************************************************************
*
* strtok - break down a string into tokens (ANSI)
*
* A sequence of calls to this routine breaks the string <string> into a
* sequence of tokens, each of which is delimited by a character from the
* string <separator>.  The first call in the sequence has <string> as its
* first argument, and is followed by calls with a null pointer as their
* first argument.  The separator string may be different from call to call.
* 
* The first call in the sequence searches <string> for the first character
* that is not contained in the current separator string.  If the character
* is not found, there are no tokens in <string> and strtok() returns a
* null pointer.  If the character is found, it is the start of the first
* token.
* 
* strtok() then searches from there for a character that is contained in the
* current separator string.  If the character is not found, the current
* token expands to the end of the string pointed to by <string>, and
* subsequent searches for a token will return a null pointer.  If the
* character is found, it is overwritten by a null character, which
* terminates the current token.  strtok() saves a pointer to the following
* character, from which the next search for a token will start.
* (Note that because the separator character is overwritten by a null
* character, the input string is modified as a result of this call.)
* 
* Each subsequent call, with a null pointer as the value of the first
* argument, starts searching from the saved pointer and behaves as
* described above.
* 
* The implementation behaves as if strtok() is called by no library functions.
*
* REENTRANCY
* This routine is not reentrant; the reentrant form is strtok_r().
*
* INCLUDE FILES: string.h
* 
* RETURNS
* A pointer to the first character of a token, or a NULL pointer if there is
* no token.
*
* SEE ALSO: strtok_r()
*/ 

char * strtok
    (
    char *       string,	/* string */
    const char * separator	/* separator indicator */
    )
    {
    static char *last = NULL;

    return (strtok_r (string, separator, &last));
    }

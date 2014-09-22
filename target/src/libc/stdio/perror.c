/* perror.c - print error value. stdio.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/* 
modification history 
-------------------- 
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,smb  written.
*/ 

/*
DESCRIPTION

INCLUDE FILE: stdio.h, error.h, string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"


/******************************************************************************
*
* perror - map an error number in `errno' to an error message (ANSI)
*
* This routine maps the error number in the integer expression `errno' to an
* error message.  It writes a sequence of characters to the standard error
* stream as follows:  first (if <__s> is not a null pointer and the character
* pointed to by <__s> is not the null character), the string pointed to by
* <__s> followed by a colon (:) and a space; then an appropriate error
* message string followed by a new-line character.  The contents of the
* error message strings are the same as those returned by strerror() with
* the argument `errno'.
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS: N/A
*
* SEE ALSO: strerror()
*/

void perror 
    (
    const char *  __s	/* error string */
    )
    {
    if ((__s) && (*__s != EOS))
    	fprintf (stderr, "%s: ", __s);

    fprintf (stderr, "%s\n", strerror (errno));
    }

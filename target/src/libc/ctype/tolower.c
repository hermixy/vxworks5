/* tolower.c - character classification and conversion macros */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,03mar93,jdi  more documentation cleanup for 5.1.
01e,07feb93,jdi  documentation cleanup for 5.1.
01d,13oct92,jdi  mangen fixes.
01c,20sep92,smb  documentation additions
01b,04jul92,smb  added vxWorks.h and extra documentation
01a,24mar92,smb  written
*/

/*
DESCRIPTION

INCLUDE FILE: ctype.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ctype.h"


#undef tolower
/*******************************************************************************
*
* tolower - convert an upper-case letter to its lower-case equivalent (ANSI)
*
* This routine converts an upper-case letter to the corresponding lower-case
* letter.
*
* INCLUDE FILES: ctype.h
*
* RETURNS:
* If <c> is an upper-case letter, it returns the lower-case equivalent;
* otherwise, it returns the argument unchanged.
*/

int tolower 
    (
    int c       /* character to convert */
    )
    {
    return __tolower(c);
    }


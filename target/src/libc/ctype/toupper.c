/* toupper.c - character classification and conversion macros */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,03mar93,jdi  more documentation cleanup for 5.1.
01d,07feb93,jdi  documentation cleanup for 5.1.
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


#undef toupper
/*******************************************************************************
*
* toupper - convert a lower-case letter to its upper-case equivalent (ANSI)
*
* This routine converts a lower-case letter to the corresponding upper-case
* letter.
*
* INCLUDE FILES: ctype.h
*
* RETURNS:
* If <c> is a lower-case letter, it returns the upper-case equivalent;
* otherwise, it returns the argument unchanged.
*/

int toupper 
    (
    int c       /* character to convert */
    )
    {
    return __toupper(c);
    }

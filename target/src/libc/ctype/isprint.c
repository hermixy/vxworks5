/* isprint.c - character classification and conversion macros */

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

#undef isprint
/*******************************************************************************
*
* isprint - test whether a character is printable, including the space character (ANSI)
*
* This routine returns true if <c> is a printing character or the space
* character.
*
* INCLUDE FILES: ctype.h
*
* RETURNS: Non-zero if and only if <c> is printable, including the space
* character.
*/

int isprint 
    (
    int c       /* character to test */
    )
    {
    return __isprint(c);
    }

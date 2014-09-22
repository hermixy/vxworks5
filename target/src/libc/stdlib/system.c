/* system.c - system file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,25mar92,smb  written.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/*****************************************************************************
*
* system - pass a string to a command processor (Unimplemented) (ANSI)
*
* This function is not applicable to VxWorks.
*
* INCLUDE FILES: stdlib.h 
*
* RETURNS: OK, always.
*/
int system 
    (
    const char * string		/* pointer to string */
    ) 
    {
    return (OK);
    }

/* assert.c - ANSI standard assert function */

/* Copyright 1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,24oct92,smb  removed some redundant documentation.
01b,20sep92,smb  documentation additions.
01a,20jul92,smb  written.
*/

/*
DESCRIPTION

INCLUDE FILES: stdio.h, stdlib.h, assert.h

SEE ALSO: American National Standard X3.159-1989
NOMANUAL
*/

#include "vxWorks.h"
#include "assert.h"
#include "stdio.h"
#include "stdlib.h"

/******************************************************************************
*
* __assert - function called by the assert macro. 
*
* INCLUDE: stdio.h assert.h
*
* RETURNS: never returns
* NOMANUAL
*/
void __assert
    (
    const char *msg	/* message string */
    )
    {
    fdprintf(2, "%s\n", CHAR_FROM_CONST (msg));	/* print msg to error stream */
    abort(); 		
    }

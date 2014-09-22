/* atexit.c - atexit file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,08feb93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions.
01a,19jul92,smb  written.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h, signal.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"

/*******************************************************************************
*
* atexit - call a function at program termination (Unimplemented) (ANSI)
*
* This routine is unimplemented.  VxWorks task exit hooks
* provide this functionality.
*
* INCLUDE FILES: stdlib.h
*
* RETURNS: ERROR, always.
*
* SEE ALSO: taskHookLib
*/

int atexit 
    (
    void (*__func)(void)	/* pointer to a function */
    ) 
    {
    return (ERROR); 
    }


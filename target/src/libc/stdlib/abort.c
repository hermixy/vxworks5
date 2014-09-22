/* abort.c - abort file for stdlib  */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,08feb93,jdi  documentation cleanup for 5.1.
01c,20sep92,smb  documentation additions.
01b,27jul92,smb  abort now raises an abort signal.
01a,19jul92,smb  Phase 1 of ANSI merge.
*/

/*
DESCRIPTION

INCLUDE FILES: stdlib.h, signal.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdlib.h"
#include "signal.h"

/******************************************************************************
*
* abort - cause abnormal program termination (ANSI)
*
* This routine causes abnormal program termination, unless the signal
* SIGABRT is being caught and the signal handler does not return.  VxWorks
* does not flush output streams, close open streams, or remove temporary
* files.  abort() returns unsuccessful status termination to the host
* environment by calling:
* .CS
*     raise (SIGABRT);
* .CE
*
* INCLUDE FILES: stdlib.h
*
* RETURNS: This routine cannot return to the caller.
*/

void abort (void)
	{
	raise (SIGABRT);
	exit (EXIT_FAILURE);
	}

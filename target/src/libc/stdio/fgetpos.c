/* fgetpos.c - get position in file. stdio.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */
 
/*
modification history
--------------------
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,jcf  Added OBJ_VERIFY
	    smb  written.
*/
  
/*
DESCRIPTION

INCLUDE FILE: stdio.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"
#include "objLib.h"


/******************************************************************************
*
* fgetpos - store the current value of the file position indicator for a stream (ANSI)
*
* This routine stores the current value of the file position indicator for a
* specified stream <fp> in the object pointed to by <pos>.  The value stored
* contains unspecified information usable by fsetpos() for repositioning
* the stream to its position at the time fgetpos() was called.
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS:
* Zero, or non-zero if unsuccessful, with `errno' set to indicate the error.
*
* SEE ALSO: fsetpos()
*/

int fgetpos
    (
    FILE *	fp,	/* stream */
    fpos_t *	pos	/* where to store position */
    )
    {
    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (ERROR);

    return (!((*pos = ftell (fp)) != (fpos_t) -1));
    }

/* putw.c - put a word. stdio.h */ 
 
/* Copyright 1992-1993 Wind River Systems, Inc. */
 
/* 
modification history 
-------------------- 
01d,05mar93,jdi  documentation cleanup for 5.1.
01c,21sep92,smb  tweaks for mg
01b,20sep92,smb  documentation additions
01a,29jul92,smb  written.
*/ 
 
/* 
DESCRIPTION 
 
INCLUDE FILE: stdio.h, string.h 
 
SEE ALSO: American National Standard X3.159-1989 

NOMANUAL
*/ 
 
#include "vxWorks.h" 
#include "stdio.h" 
 
/******************************************************************************
*
* putw - write a word (32-bit integer) to a stream
*
* This routine appends the 32-bit quantity <w> to a specified stream.
*
* This routine is provided for compatibility with earlier VxWorks releases.
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS: The value written.
*/

int putw
    (
    int     w, 	/* word (32-bit integer) */
    FILE *  fp	/* output stream */
    )
    {
    return (fwrite ((void *) &w, sizeof (int), 1, fp) == 1 ? w : EOF);
    }

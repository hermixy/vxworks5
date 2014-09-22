/* tmpfile.c - creat a temporary file. stdio.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/* 
modification history 
-------------------- 
01d,05mar93,jdi  documentation cleanup for 5.1.
01c,21sep92,smb  tweaks for mg.
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

/*******************************************************************************
*
* tmpfile - create a temporary binary file (Unimplemented) (ANSI)
*
* This routine is not be implemented
* because VxWorks does not close all open files at task exit.
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS: NULL 
*/

FILE * tmpfile (void)
    {
    return (NULL);
    }

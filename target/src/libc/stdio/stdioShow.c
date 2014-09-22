/* stdioShow.c - standard I/O show library */

/* Copyright 1984-1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,17mar99,jdi  doc: updated w/ info about proj facility (SPR 25727).
01e,16sep93,jmm  cleaned up warnings about printf args
01d,15sep93,kdl  fixed man page for stdioShowInit() (SPR #2244).
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,jcf  created.
*/

/*
DESCRIPTION
This library provides a show routine for file pointers.

NOMANUAL
*/


#include "vxWorks.h"
#include "stdio.h"
#include "sys/types.h"
#include "stdlib.h"
#include "taskLib.h"
#include "fioLib.h"
#include "classLib.h"
#include "errnoLib.h"
#include "private/objLibP.h"
#include "private/stdioP.h"


/******************************************************************************
*
* stdioShowInit - initialize the standard I/O show facility
*
* This routine links the file pointer show routine into the VxWorks system.
* It is called automatically when this show facility is
* configured into VxWorks using either of the following methods:
* .iP
* If you use the configuration header files, define
* INCLUDE_SHOW_ROUTINES in config.h.
* .iP
* If you use the Tornado project facility, select INCLUDE_STDIO_SHOW.
*
* RETURNS: OK, or ERROR if an error occurs installing the file pointer show
* routine.
*/

STATUS stdioShowInit (void)
    {
    classShowConnect (fpClassId, (FUNCPTR)stdioShow);

    return (OK);
    }
/*******************************************************************************
*
* stdioShow - display file pointer internals
*
* This routine displays information about a specified stream.
*
* RETURNS: OK, or ERROR if the file pointer is invalid.
*/

STATUS stdioShow
    (
    FAST FILE * fp,	/* stream */
    int		level	/* level */
    )
    {
    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (ERROR);				/* invalid file pointer */

    printf ("%-20s: %#-10x\n", "Owning Task", fp->taskId);
    printf ("%-20s: %-10hd\n", "File Descriptor", fp->_file);
    printf ("%-20s: %#-10x\n", "Current Position", (unsigned int) fp->_p);
    printf ("%-20s: %#-10x\n", "Read Space Left", fp->_r);
    printf ("%-20s: %#-10x\n", "Write Space Left", fp->_w);
    printf ("%-20s: %#-10x\n", "Buffer Base", (unsigned int) fp->_bf._base);
    printf ("%-20s: %#-10x\n", "Buffer Size", fp->_bf._size);	
    printf ("%-20s: %#-10x\n", "Ungetc Buffer Base",
	    (unsigned int) fp->_ub._base);
    printf ("%-20s: %#-10x\n", "Ungetc Buffer Size", fp->_ub._size);	
    printf ("%-20s: %#-10x\n", "Line Buffer Base", (unsigned int) fp->_lb._base);
    printf ("%-20s: %#-10x\n", "Line Buffer Size", fp->_lb._size);	
    printf ("%-20s: %#-10x\n", "stat.st_blksize", fp->_blksize);	
    printf ("%-20s: %#-10x\n", "lseek Offset", fp->_offset);	
    printf ("%-20s: %#-10hx\n", "Flags", fp->_flags);	

    /* someday display the buffer contents with level >= 1 */

    return (OK);
    }

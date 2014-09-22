/* fdopen.c - open file. stdio.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,05feb99,dgp  document errno values
01d,07jul93,jmm  fixed fdopen to validate the fd (spr 2197)
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,smb  taken from UCB stdio
*/

/*
DESCRIPTION
 *
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.

INCLUDE FILE: sys/types.h, fcntl.h, unistd.h, stdio.h, errno.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"
#include "sys/types.h"
#include "fcntl.h"
#include "unistd.h"
#include "errno.h"
#include "objLib.h"
#include "iosLib.h"
#include "private/stdioP.h"


/*******************************************************************************
*
* fdopen - open a file specified by a file descriptor (POSIX)
*
* This routine opens the file specified by the file descriptor <fd> and
* associates a stream with it.
* The <mode> argument is used just as in the fopen() function.
*
* INCLUDE FILES: stdio.h 
*
* RETURNS: A pointer to a stream, or a null pointer if an error occurs,
* with `errno' set to indicate the error.
*
* ERRNO: EINVAL
*
* SEE ALSO: fopen(), freopen(),
* .br
* .I "Information Technology - POSIX - Part 1:"
* .I "System API [C Language], IEEE Std 1003.1"
*/

FILE * fdopen
    (
    int		 fd,	/* file descriptor */
    const char * mode	/* mode to open with */
    )
    {
    FAST FILE *	fp;
    int		flags;
    int 	oflags;

    /* validate the file descriptor */

    if (iosFdValue (fd) == ERROR)
        return (NULL);
    
    if ((flags = __sflags(mode, &oflags)) == 0)
	    return (NULL);

    /* Make sure the mode the user wants is a subset of the actual mode. */

#if FALSE  /*  XXX */
    if ((fdflags = fcntl(fd, F_GETFL, 0)) < 0)
	return (NULL);

    tmp = fdflags & O_ACCMODE;

    if (tmp != O_RDWR && (tmp != (oflags & O_ACCMODE))) 
	{
	errno = EINVAL;
	return (NULL);
	}
#endif

    if ((fp = stdioFpCreate()) == NULL)
	return (NULL);

    fp->_flags = flags;

    /*
     * If opened for appending, but underlying descriptor does not have
     * O_APPEND bit set, assert __SAPP so that __swrite() will lseek to
     * end before each write.
     */

    if ((oflags & O_APPEND) /* XXX && !(fdflags & O_APPEND) */ )
	fp->_flags |= __SAPP;

    fp->_file = fd;

    return (fp);
    }

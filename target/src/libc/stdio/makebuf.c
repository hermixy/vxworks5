/* makebuf.c - make a file buffer. stdio.h */

/* Copyright 1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,20sep92,smb  documentation additions
01a,29jul92,smb  taken from UCB stdio, removed fstat().
*/

/*
DESCRIPTION
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

INCLUDE FILE: stdio.h, sys/types.h, sys/stat.h, unistd.h, stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "ioLib.h"
#include "stdio.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "unistd.h"
#include "stdlib.h"
#include "private/stdioP.h"


/******************************************************************************
*
* __smakebuf - Allocate a file buffer, or switch to unbuffered I/O.
*
* As a side effect, we set __SOPT or __SNPT (en/dis-able fseek
* optimisation) right after the fstat() that finds the buffer size.
* 
* INCLUDE: stdio.h 
*
* RETURNS: 
* NOMANUAL
*/

void __smakebuf
    (
    FAST FILE *fp
    )
    {
    FAST size_t	size;
    FAST BOOL 	couldBeTty;
    FAST void *	p;
    struct stat st;

    if (fp->_flags & __SNBF) 		/* is buffering disabled? */
	{
	fp->_bf._base	= fp->_nbuf;	/* base of single character buf */
	fp->_p		= fp->_nbuf;	/* point to character buf */
	fp->_bf._size	= 1;		/* length of buf is one */
	return;				/* RETURN */
	}

    if (fp->_file < 0)			/* has fd been associated with file? */
	{
	couldBeTty  = FALSE;		/* can't be a tty */
	size	    = BUFSIZ;		/* standard buffer size */
	fp->_flags |= __SNPT;		/* don't optimize fseek() */
	} 
    					/* get blksize hint from driver */
    else if ((ioctl (fp->_file, FIOFSTATGET, (int)&st)) < 0)
	{
	couldBeTty  = TRUE;		/* tyLib does not support fstat! */
	size	    = BUFSIZ;		/* standard buffer size */
	fp->_flags |= __SNPT;		/* do not try to optimise fseek() */
	}
    else
	{
	couldBeTty = ((st.st_mode & S_IFMT) == S_IFCHR);

	size = (st.st_blksize <= 0) ? (BUFSIZ) : (st.st_blksize);

	/*
	 * Optimise fseek() only if it is a regular file.
	 * (The test for __sseek is mainly paranoia.)
	 */

	if ((st.st_mode & S_IFMT) == S_IFREG) 
	    {
	    fp->_flags	|= __SOPT;
	    fp->_blksize = st.st_blksize;
	    } 
	else
	    fp->_flags	|= __SNPT;
	}

    if ((p = malloc (size)) == NULL) 	/* if malloc fails, disable buffering */
	{
	fp->_flags	|= __SNBF;
	fp->_bf._base	 = fp->_p = fp->_nbuf;
	fp->_bf._size	 = 1;
	}
    else 
	{
	fp->_flags	|= __SMBF;	/* mark buffer as malloced to reclaim */
	fp->_bf._base	 = fp->_p = p;
	fp->_bf._size	 = size;

	if (couldBeTty && isatty (fp->_file))	
	    fp->_flags	|= __SLBF;	/* turn on line buffering if tty */
	}
    }

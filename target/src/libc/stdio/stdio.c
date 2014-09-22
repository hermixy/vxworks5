/* stdio.c - internal misc. routines for stdio.h */

/* Copyright 1992 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,20sep92,smb  documentation additions
01a,29jul92,smb  taken from UCB stdio
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
 *
 *DESCRIPTION OF STDIO STATE VARIABLES
 *
 * stdio state variables.
 *
 * The following always hold:
 *
 *      if (_flags&(__SLBF|__SWR)) == (__SLBF|__SWR),
 *              _lbfsize is -_bf._size, else _lbfsize is 0
 *      if _flags&__SRD, _w is 0
 *      if _flags&__SWR, _r is 0
 *
 * This ensures that the getc and putc macros (or inline functions) never
 * try to write or read from a file that is in `read' or `write' mode.
 * (Moreover, they can, and do, automatically switch from read mode to
 * write mode, and back, on "r+" and "w+" files.)
 *
 * _lbfsize is used only to make the inline line-buffered output stream
 * code as compact as possible.
 *
 * _ub, _up, and _ur are used when ungetc() pushes back more characters
 * than fit in the current _bf, or when ungetc() pushes back a character
 * that does not match the previous one in _bf.  When this happens,
 * _ub._base becomes non-nil (i.e., a stream has ungetc() data iff
 * _ub._base!=NULL) and _up and _ur save the current values of _p and _r.


INCLUDE FILE: stdio.h, fcntl.h, unistd.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"
#include "fcntl.h"
#include "unistd.h"
#include "private/stdioP.h"

/******************************************************************************
*
* __sread - internal routine
* 
* INCLUDE: stdio.h 
*
* RETURNS: 
* NOMANUAL
*/

int __sread
    (
    FAST FILE *	fp,
    char *	buf,
    int		n
    )
    {
    FAST int ret;
    
    ret = read (fp->_file, buf, n);

    if (ret >= 0)		/* if read succeeded, update current offset */
	fp->_offset += ret;
    else
	fp->_flags &= ~__SOFF;	/* paranoia */

    return (ret);
    }


/******************************************************************************
*
* __swrite - internal routine
* 
* INCLUDE: stdio.h 
*
* RETURNS: 
* NOMANUAL
*/

int __swrite
    (
    register FILE * fp,
    char const *    buf,
    int             n
    )
    {
    if (fp->_flags & __SAPP)
	(void) lseek (fp->_file, (off_t)0, SEEK_END);

    fp->_flags &= ~__SOFF;	/* in case FAPPEND mode is set */

    return (write (fp->_file, CHAR_FROM_CONST(buf), n));
    }

/******************************************************************************
*
* __sseek - internal routine
* 
* INCLUDE: stdio.h 
*
* RETURNS: 
* NOMANUAL
*/

fpos_t __sseek
    (
    FAST FILE *	fp,
    fpos_t	offset,
    int		whence
    )
    {
    FAST off_t ret;
    
    ret = lseek (fp->_file, (off_t)offset, whence);

    if (ret == -1L)
	fp->_flags &= ~__SOFF;
    else 
	{
	fp->_flags |= __SOFF;
	fp->_offset = ret;
	}

    return (ret);
    }

/******************************************************************************
*
* __sclose - internal routine
* 
* INCLUDE: stdio.h 
*
* RETURNS: 
* NOMANUAL
*/

int __sclose
    (
    FILE * fp
    )
    {
    int result	= 0;
    int fd	= fp->_file;

    if ((fd >= 0) && (fd < 3))		/* careful of closing standard fds! */
	result = 0;
    else if (close (fd) < 0)
	result = EOF;

    return (result);
    }

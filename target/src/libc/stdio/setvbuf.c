/* setvbuf.c - set buffered mode. stdio.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,jcf  Added OBJ_VERIFY
	   +smb  taken from UCB stdio
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

INCLUDE FILE: stdio.h, stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"
#include "objLib.h"
#include "private/stdioP.h"


/******************************************************************************
*
* setvbuf - specify buffering for a stream (ANSI)
*
* This routine sets the buffer size and buffering mode for a specified
* stream.  It should be called only after the stream has been associated
* with an open file and before any other operation is performed on the
* stream.  The argument <mode> determines how the stream will be buffered,
* as follows:
* .iP _IOFBF 12
* input/output is to be fully buffered.
* .iP _IOLBF
* input/output is to be line buffered.
* .iP _IONBF
* input/output is to be unbuffered. 
* .LP
*
* If <buf> is not a null pointer, the array it points to may be used instead
* of a buffer allocated by setvbuf().  The argument <size> specifies
* the size of the array.  The contents of the array at any time are 
* indeterminate.
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS:
* Zero, or non-zero if <mode> is invalid or the request cannot be honored.
*/

int setvbuf
    (
    FAST FILE *	 fp,	/* stream to set buffering for */
    char *	 buf,	/* buffer to use (optional) */
    FAST int	 mode,	/* _IOFBF = fully buffered */
			/* _IOLBF = line buffered */
			/* _IONBF = unbuffered */
    FAST size_t  size	/* buffer size */
    )
    {
    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (ERROR);

    /*
     * Verify arguments.  The `int' limit on `size' is due to this
     * particular implementation.
     */

    if (((mode != _IOFBF) && (mode != _IOLBF) && (mode != _IONBF)) ||
	((int)size < 0))
	return (EOF);

    /*
     * Write current buffer, if any; drop read count, if any.
     * Make sure putc() will not think fp is line buffered.
     * Free old buffer if it was from malloc().  Clear line and
     * non buffer flags, and clear malloc flag.
     */

    (void) __sflush (fp);

    fp->_r	 = 0;
    fp->_lbfsize = 0;

    if (fp->_flags & __SMBF)
	free ((void *)fp->_bf._base);

    fp->_flags &= ~(__SLBF | __SNBF | __SMBF);

    /*
     * Now put back whichever flag is needed, and fix _lbfsize
     * if line buffered.  Ensure output flush on exit if the
     * stream will be buffered at all.
     */

    switch (mode) 
	{
	case _IONBF:
		fp->_flags	|= __SNBF;
		fp->_bf._base	 = fp->_p = fp->_nbuf;
		fp->_bf._size	 = 1;
		break;

	case _IOLBF:
		fp->_flags 	|= __SLBF;
		fp->_lbfsize	 = -size;
		/* FALLTHROUGH */

	case _IOFBF:
		/* no flag */
		fp->_bf._base = fp->_p = (unsigned char *)buf;
		fp->_bf._size = size;
		break;
	default:
		break;
	}

    /*
     * Patch up write count if necessary.
     */

    if (fp->_flags & __SWR)
	fp->_w = (fp->_flags & (__SLBF | __SNBF)) ? (0) : (size);

    return (0);
    }

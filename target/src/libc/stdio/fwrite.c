/* fwrite.c - write to a file. stdio.h */

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

INCLUDE FILE: stdio.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "objLib.h"
#include "private/stdioP.h"
#include "private/fvwriteP.h"


/******************************************************************************
*
* fwrite - write from a specified array (ANSI)
*
* This routine writes, from the array <buf>, up to <count> elements whose
* size is <size>, to a specified stream.  The file position indicator for
* the stream (if defined) is advanced by the number of characters
* successfully written.  If an error occurs, the resulting value of the file
* position indicator for the stream is indeterminate.
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS:
* The number of elements successfully written, which will be less than
* <count> only if a write error is encountered.
*/

int fwrite
    (
    const void *  buf,		/* where to copy from */
    size_t	  size,		/* element size */
    size_t	  count,	/* no. of elements */
    FILE *	  fp		/* stream to write to */
    )
    {
    size_t		n;
    struct __suio	uio;
    struct __siov	iov;

    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (0);

    iov.iov_base	= (void *)buf;
    uio.uio_resid	= iov.iov_len = n = (count * size);
    uio.uio_iov		= &iov;
    uio.uio_iovcnt	= 1;

    /* The usual case is success (__sfvwrite returns 0);
     * skip the divide if this happens, since divides are
     * generally slow and since this occurs whenever size==0.
     */

    if (__sfvwrite(fp, &uio) == 0)
	return (count);

    return ((n - uio.uio_resid) / size);
    }

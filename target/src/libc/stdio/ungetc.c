/* ungetc.c - return a character to a stream. stdio.h */

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

INCLUDE FILE: stdio.h, stdlib.h, string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "objLib.h"
#include "private/stdioP.h"


/******************************************************************************
*
* __submore - internal routine
*
* Expand the ungetc buffer `in place'.  That is, adjust fp->_p when
* the buffer moves, so that it points the same distance from the end,
* and move the bytes in the buffer around as necessary so that they
* are all at the end (stack-style).
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS: 
* NOMANUAL
*/

static int __submore
    (
    FAST FILE *fp
    )
    {
    FAST int       i;
    FAST uchar_t * p;

    if (fp->_ub._base == fp->_ubuf) 
	{
	/* Get a new buffer (rather than expanding the old one) */

	if ((p = malloc ((size_t)BUFSIZ)) == NULL)
	    return (EOF);

	fp->_ub._base = p;
	fp->_ub._size = BUFSIZ;

	p += BUFSIZ - sizeof(fp->_ubuf);

	for (i = sizeof(fp->_ubuf); --i >= 0;)
	    p[i] = fp->_ubuf[i];

	fp->_p = p;

	return (0);
	}

    i = fp->_ub._size;

    p = realloc (fp->_ub._base, i << 1);

    if (p == NULL)
	return (EOF);

    (void) bcopy ((void *)p, (void *)(p + i), (size_t)i);

    fp->_p	  = p + i;
    fp->_ub._base = p;
    fp->_ub._size = i << 1;

    return (0);
    }

/******************************************************************************
*
* ungetc - push a character back into an input stream (ANSI)
*
* This routine pushes a character <c> (converted to an `unsigned char') back
* into the specified input stream.  The pushed-back characters will be
* returned by subsequent reads on that stream in the reverse order of their
* pushing.  A successful intervening call on the stream to a file
* positioning function (fseek(), fsetpos(), or rewind()) discards any
* pushed-back characters for the stream.  The external storage corresponding
* to the stream is unchanged.
*
* One character of push-back is guaranteed.  If ungetc() is called too many
* times on the same stream without an intervening read or file positioning
* operation, the operation may fail.
*
* If the value of <c> equals EOF, the operation fails and the 
* input stream is unchanged.
*
* A successful call to ungetc() clears the end-of-file indicator for the
* stream.  The value of the file position indicator for the stream after
* reading or discarding all pushed-back characters is the same as it was
* before the character were pushed back.  For a text stream, the value of
* its file position indicator after a successful call to ungetc() is
* unspecified until all pushed-back characters are read or discarded.
* For a binary stream, the file position indicator is decremented by each
* successful call to ungetc(); if its value was zero before a call, it is
* indeterminate after the call.
* 
* INCLUDE: stdio.h 
*
* RETURNS:
* The pushed-back character after conversion, or EOF if the operation fails.
*
* SEE ALSO: getc(), fgetc()
*/

int ungetc
    (
    int		 c,	/* character to push */
    FAST FILE *	 fp	/* input stream */
    )
    {
    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (EOF);

    if (c == EOF)
	return (EOF);

    if ((fp->_flags & __SRD) == 0) 
	{
	/*
	 * Not already reading: no good unless reading-and-writing.
	 * Otherwise, flush any current write stuff.
	 */

	if ((fp->_flags & __SRW) == 0)
	    return (EOF);

	if (fp->_flags & __SWR) 
	    {
	    if (__sflush(fp))
		return (EOF);

	    fp->_flags  &= ~__SWR;
	    fp->_w	 = 0;
	    fp->_lbfsize = 0;
	    }
	fp->_flags |= __SRD;
	}

    c = (uchar_t) c;

    /*
     * If we are in the middle of ungetc'ing, just continue.
     * This may require expanding the current ungetc buffer.
     */

    if (HASUB(fp)) 
	{
	if (fp->_r >= fp->_ub._size && __submore(fp))
	    return (EOF);

	*--fp->_p = c;
	fp->_r++;

	return (c);
	}

    /*
     * If we can handle this by simply backing up, do so,
     * but never replace the original character.
     * (This makes sscanf() work when scanning `const' data.)
     */

    if ((fp->_bf._base != NULL) && 
	(fp->_p > fp->_bf._base) &&
	(fp->_p[-1] == c)) 
	{
	fp->_p--;
	fp->_r++;

	return (c);
	}

    /*
     * Create an ungetc buffer.
     * Initially, we will use the `reserve' buffer.
     */

    fp->_ur				= fp->_r;
    fp->_up				= fp->_p;
    fp->_ub._base			= fp->_ubuf;
    fp->_ub._size			= sizeof(fp->_ubuf);
    fp->_ubuf[sizeof(fp->_ubuf) - 1]	= c;
    fp->_p				= &fp->_ubuf[sizeof(fp->_ubuf) - 1];
    fp->_r				= 1;

    return (c);
    }

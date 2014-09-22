/* fvwrite.c - internal routine for puts function. stdio.h */

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

INCLUDE FILE: stdio.h, string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"
#include "string.h"
#include "private/stdioP.h"
#include "private/fvwriteP.h"

#define	MIN(a, b) ((a) < (b) ? (a) : (b))
#define	COPY(n)	  (void) bcopy ((void *)p, (void *)fp->_p, (size_t)(n));

#define GETIOV(extra_work)	\
    while (len == 0) 		\
	{			\
	extra_work;		\
	p = iov->iov_base;	\
	len = iov->iov_len;	\
	iov++;			\
	}


/******************************************************************************
*
* __sfvwrite - internal function used by the write ANSI functions.
* 
* INCLUDE: stdio.h 
*
* RETURNS: zero on success, EOF on error
* NOMANUAL
*/

int __sfvwrite
    (
    FAST FILE *		 fp,
    FAST struct __suio * uio
    )
    {
    FAST size_t		 len;
    FAST char *		 p;
    FAST struct __siov * iov;
    FAST int		 w;
    FAST int		 s;
    char *		 nl;
    int			 nlknown;
    int			 nldist;

    if ((len = uio->uio_resid) == 0)
	return (0);

    if (cantwrite (fp))				/* make sure we can write */
	return (EOF);

    iov	= uio->uio_iov;
    p	= iov->iov_base;
    len = iov->iov_len;
    iov++;

    if (fp->_flags & __SNBF) 
	{
	/* Unbuffered: write up to BUFSIZ bytes at a time */
	do 
	    {
	    GETIOV(;);

	    w = __swrite (fp, p, MIN(len, BUFSIZ));

	    if (w <= 0)
		goto err;

	    p	+= w;
	    len -= w;

	    } while ((uio->uio_resid -= w) != 0);
	} 
    else if ((fp->_flags & __SLBF) == 0) 
	{
	/*
	 * Fully buffered: fill partially full buffer, if any,
	 * and then flush.  If there is no partial buffer, write
	 * one _bf._size byte chunk directly (without copying).
	 *
	 * String output is a special case: write as many bytes
	 * as fit, but pretend we wrote everything.  This makes
	 * snprintf() return the number of bytes needed, rather
	 * than the number used, and avoids its write function
	 * (so that the write function can be invalid).
	 */
	do 
	    {
	    GETIOV(;);
	    w = fp->_w;

	    if (fp->_flags & __SSTR) 
		{
		if (len < w)
		    w = len;

		COPY(w);			/* copy MIN(fp->_w,len), */
		fp->_w -= w;
		fp->_p += w;
		w	= len;			/* but pretend copied all */
		}
	    else if (fp->_p > fp->_bf._base && len > w) 
		{
		COPY(w);			/* fill */
		fp->_p += w;			/* fp->_w -= w; unneeded */
		if (fflush(fp))			/* flush */
		    goto err;
		}
	    else if (len >= (w = fp->_bf._size)) 
		{
		w = __swrite (fp, p, w);	/* write directly */
		if (w <= 0)
		    goto err;
		}
	    else 
		{
		w = len;
		COPY(w);			/* fill and done */
		fp->_w -= w;
		fp->_p += w;
		}
	    p	+= w;
	    len -= w;
	    } while ((uio->uio_resid -= w) != 0);

	}
    else 
	{
	/*
	 * Line buffered: like fully buffered, but we
	 * must check for newlines.  Compute the distance
	 * to the first newline (including the newline),
	 * or `infinity' if there is none, then pretend
	 * that the amount to write is MIN(len,nldist).
	 */

	nlknown = 0;
	nldist	= 0;

	do 
	    {
	    GETIOV(nlknown = 0);
	    if (!nlknown) 
		{
		nl = memchr ((void *)p, '\n', len);
		nldist = (nl) ? (nl + 1 - p) : (len + 1);
		nlknown = 1;
		}

	    s = MIN(len, nldist);
	    w = fp->_w + fp->_bf._size;

	    if (fp->_p > fp->_bf._base && s > w) 
		{
		COPY(w);
		fp->_p += w;		/* fp->_w -= w; unneeded */
		if (fflush (fp))
		    goto err;
		}
	    else if (s >= (w = fp->_bf._size)) 
		{
		w = __swrite (fp, p, w);
		if (w <= 0)
		    goto err;
		}
	    else 
		{
		w = s;
		COPY(w);
		fp->_w -= w;
		fp->_p += w;
		}

	    if ((nldist -= w) == 0) 
		{
		if (fflush (fp))	/* copied newline: flush and forget */
		    goto err;
		nlknown = 0;
		}

	    p += w;
	    len -= w;

	    } while ((uio->uio_resid -= w) != 0);
	}

    return (0);

err:
    fp->_flags |= __SERR;

    return (EOF);
    }

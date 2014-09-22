/* fseek.c - seek a position in a file. stdio.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,05feb99,dgp  document errno values
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,jcf  Added OBJ_VERIFY
	   +smb  taken from UCB stdio, removed fstat().
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

INCLUDE FILE: stdio.h, sys/types.h, sys/stat.h, fcntl.h, stdlib.h, errno.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"
#include "ioLib.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "errno.h"
#include "objLib.h"
#include "private/stdioP.h"

#define	POS_ERR	(-(fpos_t)1)


/******************************************************************************
*
* fseek - set the file position indicator for a stream (ANSI)
*
* This routine sets the file position indicator for a specified stream.
* For a binary stream, the new position, measured in characters from the 
* beginning of the file, is obtained by adding <offset> to the position 
* specified by <whence>, whose possible values are:
* .iP SEEK_SET 16
* the beginning of the file.
* .iP SEEK_CUR
* the current value of the file position indicator.
* .iP SEEK_END
* the end of the file.
* .LP
* A binary stream does not meaningfully
* support fseek() calls with a <whence> value of SEEK_END.
*
* For a text stream, either <offset> is zero, or <offset> is a value
* returned by an earlier call to ftell() on the stream, in which case
* <whence> should be SEEK_SET.
*
* A successful call to fseek() clears the end-of-file indicator for the
* stream and undoes any effects of ungetc() on the same stream.  After an
* fseek() call, the next operation on an update stream can be either input
* or output.
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS: Non-zero only for a request that cannot be satisfied.
*
* ERRNO: EINVAL
*
* SEE ALSO: ftell()
*/

int fseek
    (
    FAST FILE *	fp,		/* stream */
    long	offset,		/* offset from <whence> */
    int		whence		/* position to offset from: */
				/* SEEK_SET = beginning */
				/* SEEK_CUR = current position */
				/* SEEK_END = end-of-file */
    )
    {
    fpos_t	target;
    fpos_t	curoff;
    size_t	n;
    struct stat st;
    int		havepos;
    BOOL	doStat;

    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (ERROR);

    /*
     * Change any SEEK_CUR to SEEK_SET, and check `whence' argument.
     * After this, whence is either SEEK_SET or SEEK_END.
     */

    switch (whence) 
	{
	case SEEK_CUR:
		/*
		 * In order to seek relative to the current stream offset,
		 * we have to first find the current stream offset a la
		 * ftell (see ftell for details).
		 */
		if (fp->_flags & __SOFF)
		    curoff = fp->_offset;
		else 
		    {
		    curoff = __sseek (fp, (fpos_t)0, SEEK_CUR);
		    if (curoff == -1L)
			return (EOF);
		    }

		if (fp->_flags & __SRD) 
		    {
		    curoff -= fp->_r;
		    if (HASUB(fp))
			curoff -= fp->_ur;
		    }
		else if (fp->_flags & __SWR && fp->_p != NULL)
		    curoff += fp->_p - fp->_bf._base;

		offset += curoff;
		whence	= SEEK_SET;
		havepos = 1;
		break;

	case SEEK_SET:
	case SEEK_END:
		curoff = 0;		/* XXX just to keep gcc quiet */
		havepos = 0;
		break;

	default:
		errno = EINVAL;
		return (EOF);
	}

    /*
     * Can only optimise if:
     *	reading (and not reading-and-writing);
     *	not unbuffered; and
     *	this is a `regular' Unix file (and hence seekfn==__sseek).
     * We must check __NBF first, because it is possible to have __NBF
     * and __SOPT both set.
     */

    if (fp->_bf._base == NULL)
	__smakebuf (fp);

    if (fp->_flags & (__SWR | __SRW | __SNBF | __SNPT))
	goto dumb;


    doStat = ioctl (fp->_file, FIOFSTATGET, (int)&st);

    if ((fp->_flags & __SOPT) == 0) 
	{
	if ((fp->_file < 0 || (doStat) ||
	    (st.st_mode & S_IFMT) != S_IFREG)) 
	    {
	    fp->_flags |= __SNPT;
	    goto dumb;
	    }

	fp->_blksize 	 = st.st_blksize;
	fp->_flags	|= __SOPT;
	}

    /*
     * We are reading; we can try to optimise.
     * Figure out where we are going and where we are now.
     */

    if (whence == SEEK_SET)
	target = offset;
    else 
	{
	if (doStat)
	    goto dumb;

	target = st.st_size + offset;
	}

    if (!havepos) 
	{
	if (fp->_flags & __SOFF)
	    curoff = fp->_offset;
	else 
	    {
	    curoff = __sseek (fp, 0L, SEEK_CUR);

	    if (curoff == POS_ERR)
		goto dumb;
	    }

	curoff -= fp->_r;

	if (HASUB(fp))
	    curoff -= fp->_ur;
	}

    /*
     * Compute the number of bytes in the input buffer (pretending
     * that any ungetc() input has been discarded).  Adjust current
     * offset backwards by this count so that it represents the
     * file offset for the first byte in the current input buffer.
     */

    if (HASUB(fp)) 
	{
	n	= fp->_up - fp->_bf._base;
	curoff -= n;
	n      += fp->_ur;
	} 
    else 
	{
	n	= fp->_p - fp->_bf._base;
	curoff -= n;
	n      += fp->_r;
	}

    /*
     * If the target offset is within the current buffer,
     * simply adjust the pointers, clear EOF, undo ungetc(),
     * and return.  (If the buffer was modified, we have to
     * skip this; see fgetline.c.)
     */

    if (((fp->_flags & __SMOD) == 0) && 
	(target >= curoff) && 
	(target < (curoff + n))) 
	{
	FAST int o = target - curoff;

	fp->_p = fp->_bf._base + o;
	fp->_r = n - o;

	if (HASUB(fp))
	    FREEUB(fp);

	fp->_flags &= ~__SEOF;

	return (0);
	}

    /*
     * The place we want to get to is not within the current buffer,
     * but we can still be kind to the kernel copyout mechanism.
     * By aligning the file offset to a block boundary, we can let
     * the kernel use the VM hardware to map pages instead of
     * copying bytes laboriously.  Using a block boundary also
     * ensures that we only read one block, rather than two.
     */

    curoff = target & ~(fp->_blksize - 1);

    if (__sseek (fp, curoff, SEEK_SET) == POS_ERR)
	goto dumb;

    fp->_r = 0;

    if (HASUB(fp))
	FREEUB(fp);

    fp->_flags &= ~__SEOF;
    n = target - curoff;

    if (n) 
	{
	if (__srefill (fp) || fp->_r < n)
	    goto dumb;
	fp->_p += n;
	fp->_r -= n;
	}

    return (0);

    /*
     * We get here if we cannot optimise the seek ... just
     * do it.  Allow the seek function to change fp->_bf._base.
     */
dumb:
    if ((__sflush (fp)) || (__sseek (fp, offset, whence) == POS_ERR)) 
	return (EOF);

    /* success: clear EOF indicator and discard ungetc() data */

    if (HASUB(fp))
	FREEUB(fp);

    fp->_p = fp->_bf._base;
    fp->_r = 0;

    /* fp->_w = 0; */	/* unnecessary (I think...) */

    fp->_flags &= ~__SEOF;

    return (0);
    }

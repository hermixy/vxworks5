/* ftell.c - remember a position in a file. stdio.h */

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

INCLUDE FILE: stdio.h, error.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"
#include "errno.h"
#include "objLib.h"
#include "private/stdioP.h"


/******************************************************************************
*
* ftell - return the current value of the file position indicator for a stream (ANSI)
*
* This routine returns the current value of the file position indicator for
* a specified stream.  For a binary stream, the value is the number of
* characters from the beginning of the file.  For a text stream, the file
* position indicator contains unspecified information, usable by fseek() for
* returning the file position indicator to its position at the time of the
* ftell() call; the difference between two such return values is not
* necessary a meaningful measure of the number of characters written or read.
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS:
* The current value of the file position indicator,
* or -1L if unsuccessful, with `errno' set to indicate the error.
*
* SEE ALSO: fseek()
*/

long ftell
    (
    FAST FILE *  fp	/* stream */
    )
    {
    FAST fpos_t pos;

    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (ERROR);

    /*
     * Find offset of underlying I/O object, then
     * adjust for buffered bytes.
     */

    if (fp->_flags & __SOFF)
	pos = fp->_offset;
    else 
	{
	pos = __sseek (fp, (fpos_t)0, SEEK_CUR);

	if (pos == -1L)
	    return (pos);
	}

    if (fp->_flags & __SRD) 
	{
	/*
	 * Reading.  Any unread characters (including
	 * those from ungetc) cause the position to be
	 * smaller than that in the underlying object.
	 */

	pos -= fp->_r;

	if (HASUB(fp))
	    pos -= fp->_ur;
	} 
    else if (fp->_flags & __SWR && fp->_p != NULL) 
	{
	/*
	 * Writing.  Any buffered characters cause the
	 * position to be greater than that in the
	 * underlying object.
	 */

	pos += fp->_p - fp->_bf._base;
	}

    return (pos);
    }

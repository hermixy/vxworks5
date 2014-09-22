/* fgets.c - Read lines of text from a stream. stdio.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */
 
/*
modification history
--------------------
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,jcf  Added OBJ_VERIFY
	    smb  taken from UCB stdio
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

INCLUDE FILE: stdio.h, string.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"
#include "string.h"
#include "objLib.h"
#include "private/stdioP.h"


/******************************************************************************
*
* fgets - read a specified number of characters from a stream (ANSI)
*
* This routine stores in the array <buf> up to <n>-1 characters from a
* specified stream.  No additional characters are read after a new-line or
* end-of-line.  A null character is written immediately after the last
* character read into the array.
*
* If end-of-file is encountered and no characters have been read, the
* contents of the array remain unchanged.  If a read error occurs, the array
* contents are indeterminate.
*
* INCLUDE FILES: stdio.h 
*
* RETURNS:
* A pointer to <buf>, or a null pointer if an error occurs or end-of-file is
* encountered and no characters have been read.
*
* SEE ALSO: fread(), fgetc()
*/

char * fgets
    (
    char *	buf,	/* where to store characters */
    FAST size_t n,	/* no. of bytes to read + 1 */
    FAST FILE *	fp	/* stream to read from */
    )
    {
    FAST size_t		len;
    FAST char *		s;
    FAST uchar_t *      p;
    FAST uchar_t *      t;

    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (NULL);

    if (n < 2)				/* sanity check */
	return (NULL);

    s = buf;
    n--;				/* leave space for NULL */

    do 
	{
	if ((len = fp->_r) <= 0)	/* If the buffer is empty, refill it */
	    {
	    if (__srefill(fp))		/* EOF/error: partial or no line */
		{
		if (s == buf)
		    return (NULL);
		break;
		}
	    len = fp->_r;
	    }

	p = fp->_p;

	/*
	 * Scan through at most n bytes of the current buffer,
	 * looking for '\n'.  If found, copy up to and including
	 * newline, and stop.  Otherwise, copy entire chunk
	 * and loop.
	 */

	if (len > n)
	    len = n;

	t = memchr ((void *)p, '\n', len);

	if (t != NULL) 
	    {
	    len = ++t - p;
	    fp->_r -= len;
	    fp->_p = t;
	    (void) bcopy ((void *)p, (void *)s, len);
	    s[len] = 0;

	    return (buf);
	    }

	fp->_r -= len;
	fp->_p += len;

	(void) bcopy ((void *)p, (void *)s, len);

	s += len;

	} while ((n -= len) != 0);

    *s = 0;

    return (buf);
    }

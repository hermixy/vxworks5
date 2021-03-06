/* setbuffer.c- file for stdio.h */

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


/******************************************************************************
*
* setbuffer - specify buffering for a stream
*
* This routine specifies a buffer <buf> to be used for a stream in place of the
* automatically allocated buffer.  If <buf> is NULL, the stream is unbuffered.
* This routine should be called only after the stream has been associated with
* an open file and before any other operation is performed on the stream.
*
* This routine is provided for compatibility with earlier VxWorks releases.
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS: N/A
*
* SEE ALSO: setvbuf()
*/

void setbuffer
    (
    FAST FILE *	 fp,	/* stream to set buffering for */
    char *	 buf,	/* buffer to use */
    int		 size	/* buffer size */
    )
    {
    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return;

    (void) setvbuf (fp, buf, ((buf) ? (_IOFBF) : (_IONBF)), size);
    }

/******************************************************************************
*
* setlinebuf - set line buffering for standard output or standard error
*
* This routine changes `stdout' or `stderr' streams from block-buffered or
* unbuffered to line-buffered.  Unlike setbuf(), setbuffer(), or setvbuf(), it
* can be used at any time the stream is active.
* 
* A stream can be changed from unbuffered or line-buffered to fully buffered
* using freopen().  A stream can be changed from fully buffered or
* line-buffered to unbuffered using freopen() followed by setbuf() with a
* buffer argument of NULL.
* 
* This routine is provided for compatibility with earlier VxWorks releases.
* 
* INCLUDE: stdio.h 
*
* RETURNS: OK, or ERROR if <fp> is not a valid stream.
*/

int setlinebuf
    (
    FILE *  fp	/* stream - stdout or stderr */
    )
    {
    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (ERROR);

    (void) setvbuf (fp, (char *)NULL, _IOLBF, (size_t)0);
    return (OK);						/* ??? */
    }

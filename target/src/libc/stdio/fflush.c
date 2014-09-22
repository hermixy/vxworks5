/* fflush.c - flush file stdio.h */

/* Copyright 1992-1995 Wind River Systems, Inc. */
 
/*
modification history
--------------------
01g,05feb99,dgp  document errno values
01f,10mar95,ism  removed SPR#2548 changes.
01e,20jan95,ism  changed FIOFLUSH to FIOSYNC
01d,03oct94,ism  added FIOFLUSH ioctl to __sflush() as per SPR#2548
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,jcf Added OBJ_VERIFY
	    smb taken from UCB stdio
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

INCLUDE FILE: stdio.h, error.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"
#include "errno.h"
#include "objLib.h"
#include "private/stdioP.h"
#include "sys/ioctl.h"
#include "ioLib.h"

/******************************************************************************
*
* fflush - flush a stream (ANSI)
*
* This routine writes to the file any unwritten data for a specified output
* or update stream for which the most recent operation was not input; for an
* input stream the behavior is undefined.
*
* CAVEAT
* ANSI specifies that if <fp> is a null pointer, fflush() performs the
* flushing action on all streams for which the behavior is defined; however,
* this is not implemented in VxWorks.
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS: Zero, or EOF if a write error occurs.
*
* ERRNO: EBADF
*
* SEE ALSO: fclose()
*/

int fflush
    (
    FAST FILE * fp	/* stream to flush */
    )
    {
    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (EOF);

    /* ANSI specifies that fflush (NULL) will flush all task's file pointers.
     * The problem is that we don't keep such a list today so we return EOF
     * for now.
     */

    if ((fp == NULL) ||
        ((fp->_flags & __SWR) == 0))
	{
	errno = EBADF;
	return (EOF);
	}

    return (__sflush(fp));
    }

/******************************************************************************
*
* __sflush - flush stream pointed to by fp.
* 
* INCLUDE: stdio.h 
*
* RETURNS: ZERO or EOF
* NOMANUAL
*/

int __sflush
    (
    FAST FILE *fp
    )
    {
    FAST unsigned char *p;
    FAST int n;
    FAST int t;

    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (EOF);

    t = fp->_flags;

    if ((t & __SWR) == 0)
	return (0);

    if ((p = fp->_bf._base) == NULL)
	return (0);

    n = fp->_p - p;		/* write this much */

    /* Set these immediately to avoid problems with longjmp and to allow
     * exchange buffering (via setvbuf) in user write function.
     */

    fp->_p = p;
    fp->_w = (t & (__SLBF|__SNBF)) ? (0) : (fp->_bf._size);

    for (; n > 0; n -= t, p += t) 
	{
	t = __swrite (fp, (char *)p, n);
	if (t <= 0) 
	    {
	    fp->_flags |= __SERR;
	    return (EOF);
	    }
	}

    return (0);
    }

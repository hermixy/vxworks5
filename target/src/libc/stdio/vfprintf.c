/* vfprintf.c - print to a file. stdio.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,jcf  Added OBJ_VERIFY, use fioFormatV()
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

INCLUDE FILE: stdio.h, stdarg.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdarg.h"
#include "fioLib.h"
#include "objLib.h"
#include "private/stdioP.h"

/* forward declarations */

LOCAL STATUS putbuf (char *buffer, int nbytes, FILE *fp);


/*******************************************************************************
*
* vfprintf - write a formatted string to a stream (ANSI)
*
* This routine is equivalent to fprintf(), except that it takes the variable
* arguments to be formatted from a list <vaList> of type `va_list' rather
* than from in-line arguments.
*
* INCLUDE FILES: stdio.h 
*
* RETURNS:
* The number of characters written, or a negative value if an
* output error occurs.
*
* SEE ALSO: fprintf()
*/

int vfprintf
    (
    FILE *	  fp,		/* stream to write to */
    const char *  fmt,		/* format string */
    va_list       vaList	/* arguments to format string */
    )
    {
    uchar_t localbuf[BUFSIZE];
    int	    ret;
    FILE    fake;

    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (EOF);

    if (cantwrite (fp))
	return (EOF);

    if ((fp->_flags & (__SNBF|__SWR|__SRW)) == (__SNBF|__SWR) && fp->_file >= 0)
	{
	/* 
	 * This optimizes an fprintf to unbuffered unix file by creating a
	 * temporary buffer.  This avoids worries about ungetc buffers and
	 * so forth.  It is particularly useful for stderr.
	 */

	fake._flags 	= fp->_flags & ~__SNBF;		/* turn on buffering */
	fake._file  	= fp->_file;			/* store fd */
	fake._bf._base	= fake._p = localbuf;		/* set up the buffer */
	fake._bf._size	= fake._w = sizeof(localbuf);	/* initialize size */
	fake._lbfsize	= 0;				/* init to be safe */
	fake._r		= 0;				/* init to be safe */
	fake._ub._base	= NULL;				/* init to be safe */
	fake._ub._size	= 0;				/* init to be safe */
	fake._lb._base	= NULL;				/* init to be safe */
	fake._lb._size	= 0;				/* init to be safe */
	objCoreInit (&fake.objCore, fpClassId);		/* validate fake fp */

        ret = fioFormatV (fmt, vaList, putbuf, (int) &fake);

	if ((ret >= 0) && fflush (&fake))
	    ret = EOF;

	if (fake._flags & __SERR)
	    fp->_flags |= __SERR;

	objCoreTerminate (&fake.objCore);		/* invalidate fp */
	}
    else
	{
	ret = fioFormatV (fmt, vaList, putbuf, (int) fp);
    	ret = (ferror (fp)) ? EOF : ret;
        }

    return (ret);
    }

/*******************************************************************************
*
* putbuf - put a buffer on an output stream
* NOMANUAL
*/

LOCAL STATUS putbuf
    (
    char * buffer,       /* source */
    int    nbytes,       /* number of bytes in source */
    FILE * fp            /* stream */
    )
    {
    FAST int ix;

    for (ix = 0; ix < nbytes; ix++)
	putc (buffer [ix], fp);

    return (OK);
    }

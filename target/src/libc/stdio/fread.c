/* fread.c - read a file. stdio.h */

/* Copyright 1992-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,21jan02,jkf  SPR#72774, fread is checking for NULL on wrong arg
01e,12dec01,jkf  fixing SPR#72128, fread should check for NULL before bcopy
01d,10nov01,jkf  SPR#70967, fread returns wrong value when 3rd arg == 0
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,jcf  Added OBJ_VERIFY
	    smb  taken from UCB stdio
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
#include "objLib.h"
#include "private/stdioP.h"


/******************************************************************************
*
* fread - read data into an array (ANSI)
*
* This routine reads, into the array <buf>, up to <count> elements of size
* <size>, from a specified stream <fp>.  The file position indicator for the
* stream (if defined) is advanced by the number of characters successfully
* read.  If an error occurs, the resulting value of the file position
* indicator for the stream is indeterminate.  If a partial element is read,
* its value is indeterminate.
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS:
* The number of elements successfully read, which may be less than <count>
* if a read error or end-of-file is encountered; or zero if <size> or
* <count> is zero, with the contents of the array and the state of the
* stream remaining unchanged.
*/

int fread
    (
    void *	buf,	/* where to copy data */
    size_t	size, 	/* element size */
    size_t	count,	/* no. of elements */
    FAST FILE *	fp	/* stream to read from */
    )
    {
    FAST size_t resid;
    FAST char *	p;
    FAST int	r;
    size_t	total;

    /* SPR#72128, check for NULL args before bcopy */

    if ((NULL == fp) || (NULL == buf))
	{
        return (0);
	}

    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (0);

    /* 
     * SPR#70967: fread returns wrong value when 3rd arg == 0 
     *
     * ANSI specification declares fread() returns zero if size 
     * _or_ count is zero.  But the vxWorks implementation returned 
     * non-zero given the following input case:
     *
     * fread ( buf , 0 ,1 ,fp );
     *
     * Old (broken) code:
     *
     * if ((resid = (count * size)) == 0)
     *     return (count);     
     *
     * New (fixed) code:
     *
     * if (0 == (resid = (count * size)))
     *     return (0);     
     */

    if (0 == (resid = (count * size)))
	return (0);

    if (fp->_r < 0)
	fp->_r = 0;

    total = resid;
    p	  = buf;

    while (resid > (r = fp->_r)) 
	{
	(void) bcopy ((void *)fp->_p, (void *)p, (size_t)r);

	fp->_p	+= r;
	p	+= r;
	resid	-= r;

	if (__srefill (fp))				/* fp->_r = 0 is done */
		return ((total - resid) / size);	/* partial result */
	}

    (void) bcopy ((void *)fp->_p, (void *)p, resid);

    fp->_r	-= resid;
    fp->_p	+= resid;

    return (count);
    }

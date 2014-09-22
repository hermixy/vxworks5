/* freopen.c - reopen a file. stdio.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,jcf  added memory reclaimation.
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

INCLUDE FILE: stdio.h, sys/types.h, sys/stat.h, fcntl.h, error.h, unistd.h, 
	      stdlib.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"
#include "ioLib.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "errno.h"
#include "unistd.h"
#include "stdlib.h"
#include "objLib.h"
#include "private/stdioP.h"


/******************************************************************************
*
* freopen - open a file specified by name (ANSI)
*
* This routine opens a file whose name is the string pointed to by <file>
* and associates it with a specified stream <fp>.
* The <mode> argument is used just as in the fopen() function.
*
* This routine first attempts to close any file that is associated
* with the specified stream.  Failure to close the file successfully is
* ignored. The error and end-of-file indicators for the stream are cleared.
*
* Typically, freopen() is used to attach the already-open streams
* `stdin', `stdout', and `stderr' to other files.
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS:
* The value of <fp>, or a null pointer if the open operation fails.
*
* SEE ALSO: fopen()
*/

FILE * freopen
    (
    const char *  file,		/* name of file */
    const char *  mode,		/* mode */
    FAST FILE *	  fp		/* stream */
    )
    {
    FAST int	f;
    int		flags;
    int		oflags;
    int		sverrno;
    BOOL	isopen = TRUE;			/* its already open */

    if (OBJ_VERIFY (fp, fpClassId) != OK)
	return (NULL);

    if ((flags = __sflags (mode, &oflags)) == 0) 
	{
	(void) fclose (fp);
	return (NULL);
	}

    /*
     * There are actually programs that depend on being able to "freopen"
     * descriptors that weren't originally open.  Keep this from breaking.
     * Remember whether the stream was open to begin with, and which file
     * descriptor (if any) was associated with it.  If it was attached to
     * a descriptor, defer closing it; freopen("/dev/stdin", "r", stdin)
     * should work.  This is unnecessary if it was not a Unix file.
     */

    if (fp->_flags & __SWR)			/* flush the stream though */
	(void) __sflush(fp); 			/* ANSI doesn't require this */

    if (fp->_file < 0)				/* need to close the fp? */
	{
	(void) __sclose (fp);
	isopen = FALSE;
	}

    /* Get a new descriptor to refer to the new file. */

    f = open (file, oflags, DEFFILEMODE);

    if ((f < 0) && isopen)
	{
	/* If out of fd's close the old one and try again. */

	if ((errno == ENFILE) || (errno == EMFILE))
	    {
	    (void) __sclose (fp);
	    isopen = FALSE;
	    f = open (file, oflags, DEFFILEMODE);
	    }
	}

    sverrno = errno;

    /*
     * Finish closing fp.  Even if the open succeeded above, we cannot
     * keep fp->_base: it may be the wrong size.  This loses the effect
     * of any setbuffer calls, but stdio has always done this before.
     */

    if (isopen)
	(void) __sclose (fp);

    if (fp->_flags & __SMBF)
	free ((char *)fp->_bf._base);

    fp->_w		= 0;
    fp->_r		= 0;
    fp->_p		= NULL;
    fp->_bf._base	= NULL;
    fp->_bf._size	= 0;
    fp->_lbfsize	= 0;

    if (HASUB(fp))
	FREEUB(fp);

    fp->_ub._size	= 0;

    if (HASLB(fp))
	FREELB(fp);

    fp->_lb._size	= 0;

    if ((f < 0) && (fp->_flags != 0))
	{				/* did not get it after all */
	fp->_flags = 0;			/* set it free */
	stdioFpDestroy (fp);		/* destroy file pointer */
	errno = sverrno;		/* restore in case _close clobbered */
	return (NULL);
	}

    fp->_flags	= flags;
    fp->_file	= f;

    return (fp);
    }

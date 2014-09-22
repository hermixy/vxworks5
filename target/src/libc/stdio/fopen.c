/* fopen.c - open a file. stdio.h */

/* Copyright 1992-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,10feb95,jdi  doc format tweak.
01c,05mar93,jdi  documentation cleanup for 5.1.
01b,20sep92,smb  documentation additions
01a,29jul92,jcf  modified to use stdioFpCreate and close memory leaks.
	    smb  taken from UCB stdio.
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
#include "sys/types.h"
#include "sys/stat.h"
#include "ioLib.h"
#include "fcntl.h"
#include "errno.h"
#include "private/stdioP.h"


/******************************************************************************
*
* fopen - open a file specified by name (ANSI)
*
* This routine opens a file whose name is the string pointed to by <file>
* and associates a stream with it.
* The argument <mode> points to a string beginning with one of the following 
* sequences:
* .iP r "" 3
* open text file for reading
* .iP w
* truncate to zero length or create text file for writing
* .iP a
* append; open or create text file for writing at end-of-file
* .iP rb
* open binary file for reading
* .iP wb
* truncate to zero length or create binary file for writing
* .iP ab
* append; open or create binary file for writing at end-of-file
* .iP r+
* open text file for update (reading and writing)
* .iP w+
* truncate to zero length or create text file for update.
* .iP a+
* append; open or create text file for update, writing at end-of-file
* .iP "r+b / rb+"
* open binary file for update (reading and writing)
* .iP "w+b / wb+"
* truncate to zero length or create binary file for update
* .iP "a+b / ab+"
* append; open or create binary file for update, writing at end-of-file
* .LP
*
* Opening a file with read mode (`r' as the first character in the <mode>
* argument) fails if the file does not exist or cannot be read.
*
* Opening a file with append mode (`a' as the first character in the <mode>
* argument) causes all subsequent writes to the file to be forced to the
* then current end-of-file, regardless of intervening calls to fseek().  In
* some implementations, opening a binary file with append mode (`b' as the
* second or third character in the <mode> argument) may initially position
* the file position indicator for the stream beyond the last data written,
* because of null character padding.  In VxWorks, whether append mode is
* supported is device-specific.
*
* When a file is opened with update mode (`+' as the second or third
* character in the <mode> argument), both input and output may be performed
* on the associated stream.  However, output may not be directly followed by
* input without an intervening call to fflush() or to a file positioning
* function (fseek(), fsetpos(), or rewind()), and input may not be directly
* followed by output without an intervening call to a file positioning
* function, unless the input operation encounters end-of-file.  Opening (or
* creating) a text file with update mode may instead open (or create) a
* binary stream in some implementations.
*
* When opened, a stream is fully buffered if and only if it can be determined
* not to refer to an interactive device.  The error and end-of-file
* indicators for the stream are cleared.
* 
* INCLUDE FILES: stdio.h 
*
* RETURNS:
* A pointer to the object controlling the stream, or a null pointer if the
* operation fails.
*
* SEE ALSO: fdopen(), freopen()
*/

FILE * fopen
    (
    const char *	file,	/* name of file */
    const char *	mode	/* mode */
    )
    {
    FAST FILE *	fp;
    FAST int	f;
    int		flags;
    int		oflags;

    if ((flags = __sflags (mode, &oflags)) == 0)
	return (NULL);

    if ((fp = stdioFpCreate ()) == NULL)
	return (NULL);

    if ((f = open (file, oflags, DEFFILEMODE )) < 0) 
	{ 
	fp->_flags = 0x0;			/* release */
        stdioFpDestroy (fp);			/* destroy file pointer */
	return (NULL);
	}

    fp->_file	= f;
    fp->_flags	= flags;

    /*
     * When opening in append mode, even though we use O_APPEND,
     * we need to seek to the end so that ftell() gets the right
     * answer.  If the user then alters the seek pointer, or
     * the file extends, this will fail, but there is not much
     * we can do about this.  (We could set __SAPP and check in
     * fseek and ftell.)
     */

    if (oflags & O_APPEND)
	(void) __sseek ((void *)fp, (fpos_t)0, SEEK_END);

    return (fp);
    }

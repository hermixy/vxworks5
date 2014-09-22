/* puts.c - put a string of characters. stdio.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,05mar93,jdi  documentation cleanup for 5.1.
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
#include "private/fvwriteP.h"


/******************************************************************************
*
* puts - write a string to the standard output stream (ANSI)
*
* This routine writes to the standard output stream a specified string <s>,
* minus the terminating null character, and appends a new-line character to
* the output.
*
* INCLUDE FILES: stdio.h 
*
* RETURNS:
* A non-negative value, or EOF if a write error occurs.
*
* SEE ALSO: fputs()
*/

int puts
    (
    char const *  s	/* string to write */
    )
    {
    size_t c = strlen (s);
    struct __suio uio;
    struct __siov iov [2];

    iov[0].iov_base	= (void *)s;
    iov[0].iov_len	= c;
    iov[1].iov_base	= "\n";
    iov[1].iov_len	= 1;

    uio.uio_resid	= c + 1;
    uio.uio_iov		= &iov [0];
    uio.uio_iovcnt	= 2;

    return ((__sfvwrite (stdout, &uio)) ? (EOF) : ('\n'));
    }

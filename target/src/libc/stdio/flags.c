/* flags.c - internal routines for flags manipulation. stdio.h */

/* Copyright 1992-1993 Wind River Systems, Inc. */
 
/*
modification history
--------------------
01c,27feb93,smb  added error checking for incorrect modes
01b,20sep92,smb  documentation additions
01a,29jul92,smb  taken from UCB stdio
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

INCLUDE FILE: stdio.h, sys/types.h, errno.h, fcntl.h

SEE ALSO: American National Standard X3.159-1989

NOMANUAL
*/

#include "vxWorks.h"
#include "stdio.h"
#include "sys/types.h"
#include "errno.h"
#include "fcntl.h"


/******************************************************************************
*
* __sflags - return flags for a given mode.
* 
* Return the (stdio) flags for a given mode.  Store the flags
* to be passed to an open() syscall through *optr.
* Return 0 on error.
*
* INCLUDE: stdio.h 
*
* RETURNS: flags or ZERO
* NOMANUAL
*/

int __sflags
    (
    FAST char *	mode,
    int *	optr
    )
    {
    FAST int ret;
    FAST int m;
    FAST int o;

    switch (*mode++) 
	{
	case 'r':				/* open for reading */
		ret = __SRD;
		m = O_RDONLY;
		o = 0;
		break;

	case 'w':				/* open for writing */
		ret = __SWR;
		m = O_WRONLY;
		o = O_CREAT | O_TRUNC;
		break;

	case 'a':				/* open for appending */
		ret = __SWR;
		m = O_WRONLY;
		o = O_CREAT | O_APPEND;
		break;

	default:				/* illegal mode */
		errno = EINVAL;
		return (0);
	}

    /* [rwa]\+ or [rwa]b\+ means read and write */

    if ((*mode == '+') || (*mode == 'b' && mode[1] == '+')) 
	{
	ret = __SRW;
	m = O_RDWR;
	}

    *optr = m | o;

    /* check for garbage in second character */
    if ((*mode != '+') && (*mode != 'b') && (*mode != '\0'))
	return (0);

    /* check for garbage in third character */
    if (*mode++ == '\0') 
	return (ret);			/* no third char */

    if ((*mode != '+') && (*mode != 'b') && (*mode != '\0'))
	return (0);

    /* check for garbage in fourth character */
    if (*mode++ == '\0') 
	return (ret);			/* no fourth char */

    if (*mode != '\0')
	return (0);
    else
	return (ret);
    }

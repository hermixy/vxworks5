/* ftruncate.c - POSIX file truncation */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,28jan94,dvs  doc changes
01b,05jan94,kdl  general cleanup.
01a,02nov93,dvs  written
*/

/*
DESCRIPTION: This module contains the POSIX compliant ftruncate() routine 
for truncating a file.

INCLUDE FILES: unistd.h

SEE ALSO:
NOMANUAL
*/

/* INCLUDES */
#include "vxWorks.h"
#include "unistd.h"
#include "errno.h"
#include "ioLib.h"
#include "dosFsLib.h"

/******************************************************************************
*
* ftruncate - truncate a file (POSIX)
*
* This routine truncates a file to a specified size. 
*
* RETURNS: 0 (OK) or -1 (ERROR) if unable to truncate file.
*
* ERRNO:
*  EROFS 
*   - File resides on a read-only file system.
*  EBADF
*   - File is open for reading only.
*  EINVAL
*   - File descriptor refers to a file on which this operation is impossible.
*
*/

int ftruncate 
    (
    int 	fildes,			/* fd of file to truncate */
    off_t	length			/* length to truncate file */
    )
    {
    int 	status;			/* status value from ioctl */

    if ((status = ioctl (fildes, FIOTRUNC, length)) == ERROR)
	{
	/* map errno to that specified by POSIX */
	switch (errno)
            {
	    case S_ioLib_WRITE_PROTECTED:
		errno = EROFS;
		break;

	    case S_dosFsLib_READ_ONLY:
		errno = EBADF;
		break;

	    case S_ioLib_UNKNOWN_REQUEST:
	    default:
		errno = EINVAL;
		break;
	    }
	return (ERROR);
	}
    else 
	return (OK);
    }

/* devSplit.c - extra library for the I/O system */

/* Copyright 1984-1997 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,21jun00,rsh  upgrade to dosFs 2.0
01c,31jul99,jkf  T2 merge, tidiness & spelling.
01b,14oct98,lrn  added MS-DOS style device names support
01a,10dec97,ms   taken from usrExtra.c
*/

/*
DESCRIPTION
Provide a routine to split the device name from a full path name.
*/

#include "vxWorks.h"
#include "string.h"

/******************************************************************************
*
* devSplit - split the device name from a full path name
*
* This routine returns the device name from a valid UNIX-style path name
* by copying until two slashes ("/") are detected.  The device name is
* copied into <devName>.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void devSplit
    (
    FAST char *fullFileName,    /* full file name being parsed */
    FAST char *devName          /* result device name */
    )
    {
    FAST int nChars = 0;

    if (fullFileName != NULL)
        {
        char *p0 = fullFileName;
        char *p1 = devName;

        while ((nChars < 2) && (*p0 != EOS))
            {
            if (*p0 == ':')
		break;

            if (*p0 == '/')
                nChars++;

            *p1++ = *p0++;
            }
        *p1 = EOS;
        }
    else
        {
        (void) strcpy (devName, "");
        }
    }




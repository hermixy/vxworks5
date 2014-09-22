/* mmanPxLib.c - memory management library (POSIX) */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,19jan95,jdi  doc tweaks.
01c,01feb94,dvs  documentation changes.
01b,05jan94,kdl  general cleanup.
01a,05nov93,dvs  written
*/

/*
DESCRIPTION
This library contains POSIX interfaces designed to lock and unlock memory pages,
i.e., to control whether those pages may be swapped to secondary storage.
Since VxWorks does not use swapping (all pages are always kept in memory), 
these routines have no real effect and simply return 0 (OK).



INCLUDE FILES: sys/mman.h

SEE ALSO: POSIX 1003.1b document
*/

/* INCLUDES */
#include "vxWorks.h"
#include "sys/mman.h"

/******************************************************************************
*
* mlockall - lock all pages used by a process into memory (POSIX)
*
* This routine guarantees that all pages used by a process are memory resident.
* In VxWorks, the <flags> argument is ignored, since all pages are memory
* resident.
*
* RETURNS: 0 (OK) always.
*
* ERRNO: N/A
*
*/

int mlockall 
    (
    int flags
    )
    {
    return (OK);
    }

/******************************************************************************
*
* munlockall - unlock all pages used by a process (POSIX)
*
* This routine unlocks all pages used by a process from being memory resident.
*
* RETURNS: 0 (OK) always.
*
* ERRNO: N/A
*
*/

int munlockall (void)
    {
    return (OK);
    }

/******************************************************************************
*
* mlock - lock specified pages into memory (POSIX)
*
* This routine guarantees that the specified pages are memory resident.
* In VxWorks, the <addr> and <len> arguments are ignored, since all pages
* are memory resident.
*
* RETURNS: 0 (OK) always.
*
* ERRNO: N/A
*
*/

int mlock 
    (
    const void * 	addr,
    size_t 		len
    )
    {
    return (OK);
    }

/******************************************************************************
*
* munlock - unlock specified pages (POSIX)
*
* This routine unlocks specified pages from being memory resident.
*
* RETURNS: 0 (OK) always.
*
* ERRNO: N/A
*
*/

int munlock 
    (
    const void * 	addr,
    size_t		len
    )
    {
    return (OK);
    }

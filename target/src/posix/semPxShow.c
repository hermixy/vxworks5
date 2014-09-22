/* semPxShow.c - POSIX semaphore show library */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01i,17mar99,jdi  doc: updated w/ info about proj facility (SPR 25727).
01h,05oct98,jmp  doc: replaced INCLUDE_SHOW_RTNS by INCLUDE_SHOW_ROUTINES.
01g,19jan95,jdi  doc tweaks.
01f,03feb94,kdl  changed show format.
01e,01feb94,dvs  documentation tweaks.
01d,21jan94,kdl	 minor changes to display labels.
01c,05jan94,kdl	 changed sem_t "close" field to "refCnt"; general cleanup.
01b,17dec93,dvs  changed semClassId to semPxClassId
01a,16jun93,smb  created.
*/

/*
DESCRIPTION
This library provides a show routine for POSIX semaphore objects.

*/


#include "vxWorks.h"
#include "semPxShow.h"
#include "semaphore.h"
#include "stdio.h"
#include "sys/types.h"
#include "taskLib.h"
#include "classLib.h"
#include "private/objLibP.h"
#include "private/stdioP.h"
#include "private/semPxLibP.h"

/******************************************************************************
*
* semPxShowInit - initialize the POSIX semaphore show facility
*
* This routine links the POSIX semaphore show routine into the VxWorks system.
* It is called automatically when the this show facility is
* configured into VxWorks using either of the following methods:
* .iP
* If you use the configuration header files, define
* INCLUDE_SHOW_ROUTINES in config.h.
* .iP
* If you use the Tornado project facility, select INCLUDE_POSIX_SEM_SHOW.
*
* RETURNS: OK, or ERROR if an error occurs installing the file pointer 
* show routine.
*/

STATUS semPxShowInit (void)
    {
    classShowConnect (semPxClassId, (FUNCPTR)semPxShow);

    return (OK);
    }

/*******************************************************************************
*
* semPxShow - display semaphore internals
*
* NOMANUAL
*/

STATUS semPxShow
    (
    sem_t *	semDesc,
    int		level
    )
    {
    int 	sval;

    if (OBJ_VERIFY (semDesc, semPxClassId) != OK)
	{
        return (ERROR);                         /* invalid file pointer */
	}

    if (semDesc->sem_name != NULL)		/* if a named semaphore */
	{
    	printf ("%-32s: %-10s\n", "Semaphore name", semDesc->sem_name);
    	printf ("%-32s: %-10d\n", "sem_open() count", semDesc->refCnt);
	}

    sem_getvalue (semDesc, &sval);

    if (sval >= 0)
	{
        printf ("%-32s: %-10d\n", "Semaphore value", sval);
	}
    else
	{
	/* tell the user the value is zero */
        printf ("%-32s: %-10d\n", "Semaphore value", 0);
        printf ("%-32s: %-10d\n", "No. of blocked tasks", -(sval));
	}
    printf ("\n");

    /* someday display the buffer contents with level >= 1 */

    return (OK);

    }

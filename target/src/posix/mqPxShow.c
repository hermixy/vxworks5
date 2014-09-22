/* mqPxShow.c - POSIX message queue show */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,17mar99,jdi  doc: updated w/ info about proj facility (SPR 25727).
01d,19aug96,dbt  modified test of name in mqPxShow (fixed SPR #4228).
01c,19jan95,jdi  doc cleanup.
01c,03feb94,dvs  changed show format; added include of private/mqPxLibP.h.
01b,01feb94,dvs  documentation tweaks.
01a,16jun93,smb  created.
*/

/*
DESCRIPTION
This library provides a show routine for POSIX objects.

*/


#include "vxWorks.h"
#include "stdio.h"
#include "sys/types.h"
#include "taskLib.h"
#include "classLib.h"
#include "private/objLibP.h"
#include "private/stdioP.h"
#include "private/mqPxLibP.h"
#include "mqueue.h"

extern CLASS_ID mqClassId;

/*******************************************************************************
*
* mqPxShow - display message queue internals
*
* NOMANUAL
*/

STATUS mqPxShow
    (
    mqd_t	mqDesc,
    int		level
    )
    {
    struct mq_attr attr;

    if (mq_getattr(mqDesc, &attr) != 0)
	return (ERROR);

#if 0
    printf ("%-32s: %-10s\n", "Message queue name", 
	    mqDesc->f_data->mq_name);
    printf ("%-32s: %-10d\n", "open message queue descriptors", 
	    mqDesc->f_data->close);
    printf ("%-32s: %#-10hx\n", "Flags", mqDesc->f_flag);
#endif

    printf ("%-32s: %s\n", "Message queue name", 
	    (mqDesc->f_data->msgq_sym.name == NULL) ? "<NONE>" : 
						 mqDesc->f_data->msgq_sym.name);

    printf ("%-32s: %-10d\n", "No. of messages in queue", attr.mq_curmsgs);
    printf ("%-32s: %-10d\n", "Maximum no. of messages", attr.mq_maxmsg);
    printf ("%-32s: %-10d\n", "Maximum message size", attr.mq_msgsize);
    printf ("\n");

    /* someday display the buffer contents with level >= 1 */

    return (OK);
    }

/******************************************************************************
*
* mqPxShowInit - initialize the POSIX message queue show facility
*
* This routine links the POSIX message queue show routine into the VxWorks 
* system. It is called automatically when this show facility is configured
* into VxWorks using either of the following methods:
* .iP
* If you use the configuration header files, define
* INCLUDE_SHOW_ROUTINES in config.h.
* .iP
* If you use the Tornado project facility, select INCLUDE_POSIX_MQ_SHOW.
*
* RETURNS: OK, or ERROR if an error occurs installing the file pointer show 
* routine.
*/

STATUS mqPxShowInit (void)
    {
    classShowConnect (mqClassId, (FUNCPTR)mqPxShow);

    return (OK);
    }

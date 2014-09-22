/* wdShow.c - watchdog show routines */

/* Copyright 1990-1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01j,18dec00,pes  Correct compiler warnings
01i,17mar99,jdi  doc: updated w/ info about proj facility (SPR 25727).
01h,24jun96,sbs  made windview instrumentation conditionally compiled
01f,10oct95,jdi  doc: added .tG Shell to wdShow().
01g,16jan94,c_s  wdShowInit () now initializes instrumented class.
01f,10dec93,smb  included private/classLibP.h for windview
01e,03feb93,jdi  changed INCLUDE_SHOW_RTNS to ...ROUTINES.
01d,01feb93,jdi  documentation cleanup for 5.1.
01c,28jul92,jcf  changed wdShowInit to call wdLibInit.
01b,07jul92,ajm  changed wdStateMsg to be static
01a,15jun92,jcf  extracted from v1l of semLib.c.
*/

/*
DESCRIPTION
This library provides routines to show watchdog statistics, such as
watchdog activity, a watchdog routine, etc.

The routine wdShowInit() links the watchdog show facility into
the VxWorks system.  It is called automatically when this show
facility is configured into VxWorks using either of the
following methods:
.iP
If you use the configuration header files, define
INCLUDE_SHOW_ROUTINES in config.h.
.iP
If you use the Tornado project facility, select INCLUDE_WATCHDOGS_SHOW.

INCLUDE FILES: wdLib.h

SEE ALSO: wdLib,
.pG "Basic OS, Target Shell,"
windsh,
.tG "Shell"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "errno.h"
#include "intLib.h"
#include "qLib.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"
#include "private/wdLibP.h"
#include "private/kernelLibP.h"
#include "private/objLibP.h"
#include "private/classLibP.h"

/* locals */

LOCAL char * wdStateMsg [0x3] = 
    {
    "OUT_OF_Q", "IN_Q", "DEAD"
    };

/******************************************************************************
*
* wdShowInit - initialize the watchdog show facility
*
* This routine links the watchdog show facility into the VxWorks system.
* It is called automatically when the watchdog show facility is
* configured into VxWorks using either of the following methods:
* .iP
* If you use the configuration header files, define
* INCLUDE_SHOW_ROUTINES in config.h.
* .iP
* If you use the Tornado project facility, select INCLUDE_WATCHDOGS_SHOW.
*
* RETURNS: N/A
*/
void wdShowInit (void)
    {
    if (wdLibInit () == OK)
	{
	classShowConnect (wdClassId, (FUNCPTR)wdShow);

#ifdef WV_INSTRUMENTATION
	classShowConnect (wdInstClassId, (FUNCPTR)wdShow);
#endif
	}
    }

/*******************************************************************************
*
* wdShow - show information about a watchdog
*
* This routine displays the state of a watchdog.
*
* EXAMPLE:
* A summary of the state of a watchdog is displayed as follows:
* .CS
*     -> wdShow myWdId
*     Watchdog Id         : 0x3dd46c
*     State               : OUT_OF_Q
*     Ticks Remaining     : 0
*     Routine             : 0
*     Parameter           : 0
* .CE
*
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

STATUS wdShow
    (
    WDOG_ID	wdId		/* watchdog to display */
    )
    {
    int		ticks;
    int		state;
    int		arg;
    FUNCPTR	rtn;
    FAST int	lock;

    lock = intLock ();					/* LOCK INTERRUPTS */

    if (OBJ_VERIFY (wdId, wdClassId) != OK)
	{
	intUnlock (lock);				/* UNLOCK INTERRUPTS */
	return (ERROR);
	}

    state = (int)wdId->status;				/* record state */

    if (state == WDOG_IN_Q)
	{
	ticks	= Q_KEY (&tickQHead, &wdId->tickNode, 1);
	rtn	= wdId->wdRoutine;			/* record routine */
	arg	= wdId->wdParameter;			/* record parameter */
	}
    else
	{
	ticks	= 0;
	rtn	= (FUNCPTR) NULL;
	arg	= 0;
	}

    intUnlock (lock);					/* UNLOCK INTERRUPTS */

    /* show summary information */

    printf ("\n");
    printf ("%-20s: 0x%-10x\n", "Watchdog Id", (int)wdId);
    printf ("%-20s: %-10s\n", "State", wdStateMsg[state & 0x3]);
    printf ("%-20s: %-10d\n", "Ticks Remaining", ticks);
    printf ("%-20s: 0x%-10x\n", "Routine", (int)rtn);
    printf ("%-20s: 0x%-10x\n", "Parameter", arg);
    printf ("\n");

    return (OK);
    }

/* semShow.c - semaphore show routines */

/* Copyright 1990-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01q,31oct01,aeg  added display of VxWorks events information.
01p,26sep01,jws  move vxMP show & info rtn ptrs to funcBind.c (SPR36055)
01o,18dec00,pes  Correct compiler warnings
01n,17mar99,jdi  doc: updated w/ info about proj facility (SPR 25727).
01m,24jun96,sbs  made windview instrumentation conditionally compiled
01l,10oct95,jdi  doc: added .tG Shell to SEE ALSO for semShow().
01l,16jan94,c_s  semShowInit () now initializes instrumented class.
01k,03feb93,jdi  changed INCLUDE_SHOW_RTNS to ...ROUTINES.
01j,02feb93,jdi  documentation tweaks.
01i,23nov92,jdi  documentation cleanup.
01h,13nov92,dnw  added include of smObjLib.h
01g,30jul92,smb  changed format for printf to avoid zero padding.
01f,29jul92,pme  added NULL function pointer check for smObj routines.
01e,28jul92,jcf  changed semShowInit to call semLibInit.
01d,19jul92,pme  added shared memory semaphores support.
01c,12jul92,jcf  changed level compare to >=
01b,07jul92,ajm  changed semTypeMsg to be static
01a,15jun92,jcf  extracted from v1l of semLib.c.
*/

/*
DESCRIPTION
This library provides routines to show semaphore statistics, such as
semaphore type, semaphore queuing method, tasks pended, etc.

The routine semShowInit() links the semaphore show facility into the VxWorks
system.  It is called automatically when the semaphore show facility is
configured into VxWorks using either of the following methods:
.iP
If you use the configuration header files, define
INCLUDE_SHOW_ROUTINES in config.h.
.iP
If you use the Tornado project facility, select INCLUDE_SEM_SHOW.
.LP


INCLUDE FILES: semLib.h

SEE ALSO: semLib,
.pG "Basic OS"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "intLib.h"
#include "qLib.h"
#include "errno.h"
#include "stdlib.h"
#include "stdarg.h"
#include "string.h"
#include "stdio.h"
#include "smObjLib.h"
#include "private/eventLibP.h"
#include "private/semLibP.h"
#include "private/kernelLibP.h"
#include "private/taskLibP.h"
#include "private/semSmLibP.h"

/* globals */

/* locals */

LOCAL char * 	semTypeMsg [MAX_SEM_TYPE] = 
    {
    "BINARY", "MUTEX", "COUNTING", "OLD", "\0", "\0", "\0", "\0"
    };

/******************************************************************************
*
* semShowInit - initialize the semaphore show facility
*
* This routine links the semaphore show facility into the VxWorks system.
* It is called automatically when the semaphore show facility is
* configured into VxWorks using either of the following methods:
* .iP
* If you use the configuration header files, define
* INCLUDE_SHOW_ROUTINES in config.h.
* .iP
* If you use the Tornado project facility, select INCLUDE_SEM_SHOW.
*
* RETURNS: N/A
*/

void semShowInit (void)
    {
    if (semLibInit () == OK)
	{

	classShowConnect (semClassId, (FUNCPTR)semShow);

#ifdef WV_INSTRUMENTATION	
	classShowConnect (semInstClassId, (FUNCPTR)semShow);
#endif

	}
    }

/*******************************************************************************
*
* semInfo - get a list of task IDs that are blocked on a semaphore
*
* This routine reports the tasks blocked on a specified semaphore.
* Up to <maxTasks> task IDs are copied to the array specified by <idList>.
* The array is unordered.
*
* WARNING:
* There is no guarantee that all listed tasks are still valid or that new
* tasks have not been blocked by the time semInfo() returns.
*
* RETURNS: The number of blocked tasks placed in <idList>.
*/

int semInfo
    (
    SEM_ID semId,               /* semaphore ID to summarize */
    int idList[],               /* array of task IDs to be filled in */
    int maxTasks                /* max tasks idList can accommodate */
    )
    {
    int       numBlk;
    int       lock;

    if (INT_RESTRICT () != OK)			/* restrict ISR use */
	return (ERROR);

    if (ID_IS_SHARED (semId))           	/* semaphore is shared */
        {
        if (semSmInfoRtn == NULL)
            {
            errno = S_smObjLib_NOT_INITIALIZED;
            return (ERROR);
            }

        return ((* semSmInfoRtn) (SM_OBJ_ID_TO_ADRS (semId), idList, maxTasks));
	}

    lock = intLock ();				/* LOCK INTERRUPTS */

    if (OBJ_VERIFY (semId, semClassId) != OK)
	{
	intUnlock (lock);			/* UNLOCK INTERRUPTS */
	return (ERROR);
	}

    numBlk = Q_INFO (&semId->qHead, idList, maxTasks);

    intUnlock (lock);				/* UNLOCK INTERRUPTS */

    return (numBlk);				/* return blocked task count */
    }

/*******************************************************************************
*
* semShow - show information about a semaphore
*
* This routine displays the state and optionally the pended tasks of a 
* semaphore.
*
* A summary of the state of the semaphore is displayed as follows:
* .CS
*     Semaphore Id        : 0x585f2
*     Semaphore Type      : BINARY
*     Task Queuing        : PRIORITY
*     Pended Tasks        : 1
*     State               : EMPTY {Count if COUNTING, Owner if MUTEX}
*     Options             : 0x1       SEM_Q_PRIORITY
*
*     VxWorks Events
*     --------------
*     Registered Task     : 0x594f0 (t1)
*     Event(s) to Send    : 0x1
*     Options             : 0x7       EVENTS_SEND_ONCE
*                                     EVENTS_ALLOW_OVERWRITE
*                                     EVENTS_SEND_IF_FREE
* .CE
*
* If <level> is 1, then more detailed information will be displayed.
* If tasks are blocked on the queue, they are displayed in the order
* in which they will unblock, as follows:
* .CS
*     Pended Tasks
*     ------------
*        NAME      TID    PRI DELAY
*     ---------- -------- --- -----
*     tExcTask    3fd678   0    21
*     tLogTask    3f8ac0   0   611
* .CE
*
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

STATUS semShow
    (
    SEM_ID	semId,		/* semaphore to display */
    int		level		/* 0 = summary, 1 = details */
    )
    {
    int		taskIdList [20];
    int		taskDList [20];
    int	 	numTasks;
    WIND_TCB *	pTcb;
    char *	qMsg;
    int	        ix;
    int  	lock;
    char	optionsString [128];
    EVENTS_RSRC	semEvResource;

    if (ID_IS_SHARED (semId))           	/* semaphore is shared */
        {
        if (semSmShowRtn == NULL)
            {
            errno = S_smObjLib_NOT_INITIALIZED;
            return (ERROR);
            }

        return ((*semSmShowRtn) ((SM_SEM_ID) SM_OBJ_ID_TO_ADRS (semId), level));
	}

    lock = intLock ();					/* LOCK INTERRUPTS */

    if ((numTasks = semInfo (semId, &taskIdList[0], 20)) == ERROR)
	{
	intUnlock (lock);				/* UNLOCK INTERRUPTS */
	printf ("Invalid semaphore id: %#x\n", (int)semId);
	return (ERROR);
	}

    if (numTasks > 0)					/* record delays */
	{
	for (ix = 0; ix < min (numTasks, NELEMENTS (taskIdList)); ix++)
	    {
	    pTcb = (WIND_TCB *)(taskIdList [ix]);
	    if (pTcb->status & WIND_DELAY)
		taskDList[ix] = Q_KEY (&tickQHead, &pTcb->tickNode, 1);
	    else
		taskDList[ix] = 0;
	    }
	}

    pTcb          = semId->semOwner;			/* record owner/count */
    semEvResource = semId->events;			/* record event info */

    intUnlock (lock);					/* UNLOCK INTERRUPTS */

    qMsg = ((semId->options & SEM_Q_MASK) == SEM_Q_FIFO) ? "FIFO" : "PRIORITY";

    /* show summary information */

    printf ("\n");
    printf ("%-20s: 0x%-10x\n", "Semaphore Id", (int)semId);
    printf ("%-20s: %-10s\n", "Semaphore Type", semTypeMsg[semId->semType]);
    printf ("%-20s: %-10s\n", "Task Queuing", qMsg);
    printf ("%-20s: %-10d\n", "Pended Tasks", numTasks);

    /* start generating semaphore options string */

    if ((semId->options & SEM_Q_MASK) == SEM_Q_FIFO)
	strcpy (optionsString, "SEM_Q_FIFO\n");
    else
	strcpy (optionsString, "SEM_Q_PRIORITY\n");

    switch (semId->semType)
	{
	case SEM_TYPE_BINARY:
	    if (pTcb != NULL)
		printf ("%-20s: %-10s\n", "State", "EMPTY");
	    else
		printf ("%-20s: %-10s\n", "State", "FULL");
	    break;

	case SEM_TYPE_COUNTING:
	    printf ("%-20s: %-10d\n", "Count", (int)pTcb);
	    break;

	case SEM_TYPE_MUTEX:
	    if (pTcb != NULL)
		{
		printf ("%-20s: 0x%-10x", "Owner", (int)pTcb);

		if (taskIdVerify ((int)pTcb) != OK)
		    printf (" Deleted!\n");
		else if (pTcb->name != NULL)
		    printf (" (%s)\n", pTcb->name);
		else
		    printf ("\n");
		}
	    else
		printf ("%-20s: %-10s\n", "Owner", "NONE");

	    /* continue generating semaphore options string */

	    if ((semId->options & SEM_DELETE_SAFE) != 0)
		strcat (optionsString, "\t\t\t\tSEM_DELETE_SAFE\n");

	    if ((semId->options & SEM_INVERSION_SAFE) != 0)
		strcat (optionsString, "\t\t\t\tSEM_INVERSION_SAFE\n");

	    break;

	default :
	    printf ("%-20s: 0x%-10x\n", "State", (int)pTcb);
	    break;
	}

    /* complete generating semaphore options string */

    if ((semId->options & SEM_EVENTSEND_ERR_NOTIFY) != 0)
	strcat (optionsString, "\t\t\t\tSEM_EVENTSEND_ERR_NOTIFY\n");

    printf ("%-20s: 0x%x\t%s", "Options", semId->options, optionsString);

    /* display VxWorks events information */

    eventRsrcShow (&semEvResource);

    if (level >= 1)		 			/* show blocked tasks */
	{
	if (numTasks > 0)	
	    {
	    printf ("\nPended Tasks\n------------\n"
		    "   NAME      TID    PRI TIMEOUT\n"
		    "---------- -------- --- -------\n");

	    for (ix = 0; ix < min (numTasks, NELEMENTS (taskIdList)); ix++)
		printf ("%-11s%8x %3d %7u\n", 
			taskName (taskIdList [ix]),
			taskIdList [ix],
			((WIND_TCB *)taskIdList [ix])->priority,
			taskDList[ix]);
	    }
	}

    printf ("\n");

    return (OK);
    }

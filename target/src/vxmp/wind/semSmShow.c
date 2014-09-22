/* semSmShow.c - shared memory semaphore show utility (VxMP Option) */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,06may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
01i,24oct01,mas  doc update (SPR 71149)
01h,03feb93,jdi  changed INCLUDE_SHOW_RTNS to ...ROUTINES.
01g,29jan93,pme  added little endian support.
01f,13nov92,dnw  added include of private/smObjLibP.h
01e,02oct92,pme  added SPARC support. documentation cleanup.
01d,29sep92,pme  fixed WARNING in printf calls.
01c,29jul92,pme  changed module name to semSmShow in first line.
01b,28jul92,jcf  changed semSmShowInit to call semLibInit.
01a,19jul92,pme  written.
*/

/*
DESCRIPTION
This library provides routines to show shared semaphore statistics, such as
semaphore type, semaphore queueing method, tasks pended, and so forth.

These routines are included automatically when the component INCLUDE_SM_OBJ
is included.

There are no user callable routines.

AVAILABILITY
This module is distributed as a component of the unbundled shared memory
support option, VxMP.

INCLUDE FILE: semSmLib.h

SEE ALSO: semLib, semSmLib, smObjLib,
\tb VxWorks Programmer's Guide: Shared Memory Objects,
\tb VxWorks Programmer's Guide: Basic OS

NOROUTINES
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "errno.h"
#include "qFifoGLib.h"
#include "smObjLib.h"
#include "smMemLib.h"
#include "semLib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "taskLib.h"
#include "cacheLib.h"
#include "intLib.h"
#include "netinet/in.h"
#include "private/semSmLibP.h"
#include "private/smObjLibP.h"
#include "private/windLibP.h"

/*****************************************************************************
*
* semSmShowInit - initialize shared semaphores show routine
*
* This routine links the shared memory semaphores show routine into the
* VxWorks system.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void semSmShowInit (void)
    {
    if (semLibInit () == OK)
	{
	/* 
	 * Initialize info and show routine pointer to allow semShow and 
	 * semInfo to handle shared semaphores ids.
	 */

	semSmShowRtn = (FUNCPTR) semSmShow;
	semSmInfoRtn = (FUNCPTR) semSmInfo;
	}
    }

/*****************************************************************************
*
* semSmInfo - get list of shared TCB that are blocked on shared semaphore
*
* This routine reports the shared task control block of tasks that are
* blocked on a specified shared semaphore.
* Up to <maxTasks> shared TCBs are copied to the array specified by <idList>.
* The array is unordered.
*
* WARNING:
* There is no guarantee that all the tasks are still valid or that no new
* tasks have blocked by the time semSmInfo() returns.
* This routines locks interrupts while getting the list of pended
* tasks thus increasing local CPU interupt latency.
* This routine must be used for debug purpose only.
*
* RETURNS: The number of blocked tasks placed in <idList>.
*
* ERRNO: S_smObjLib_LOCK_TIMEOUT
* 
* SEE ALSO: semSmShow
*
* NOMANUAL
*/

int semSmInfo
    (
    SM_SEM_ID smSemId,          /* shared semaphore to summarize */
    int idList[],               /* array of shared tcb to be filled in */
    int maxTasks                /* max tasks idList can accommodate */
    )
    {
    Q_FIFO_G_HEAD      pendQ;   /* temporary queue to get info */
    int                numBlk;	/* current number of blocked tasks */
    int                level;	
    SM_SEM_ID volatile smSemIdv = (SM_SEM_ID volatile) smSemId;
    int                tmp;

    kernelState = TRUE;                         /* KERNEL ENTER */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = smSemIdv->verify;                     /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (smSemIdv) != OK)		/* check semaphore */
        {
        windExit ();                            /* KERNEL EXIT */
        return (ERROR);
        }

    /* ENTER LOCKED SECTION */

    if (SM_OBJ_LOCK_TAKE (&smSemId->lock, &level) != OK)
        {
        smObjTimeoutLogMsg ("semInfo", (char *) &smSemId->lock);
        return (ERROR);                         /* can't take lock */
        }

    /* initialize pseudo multi-way Queue */

    pendQ.pLock   = NULL;                       /* we already have the lock */
    pendQ.pFifoQ  = &smSemId->smPendQ;          /* address of actual queue */
    pendQ.pQClass = qFifoGClassId;              /* global fifo multi way Q */

    numBlk = Q_INFO (&pendQ, idList, maxTasks);

    /* EXIT LOCKED SECTION */

    SM_OBJ_LOCK_GIVE (&smSemId->lock, level);

    windExit ();                                /* KERNEL EXIT */

    return (numBlk);                            /* return blocked task count */
    }

/*****************************************************************************
*
* semSmShow - displays list of task blocked on a shared semaphore
*
* This routine displays the task IDs and CPU number of tasks
* blocked on a specified shared binary or counting semaphore.
*
* WARNING:
* There is no guarantee that all the tasks are still valid or that no new
* tasks have blocked by the time semSmShow() returns.
* Interrupts are locked while getting the list of tasks pending
* on the shared semaphore.
*
* Informations will be displayed as follows:
* 
* \cs
*
* Semaphore Id        : 0x585f2
* Semaphore Type      : SHARED BINARY
* Task Queuing        : PRIORITY
* Pended Tasks        : 1
* State               : EMPTY {Count if COUNTING}
*
* \ce
*
* If <level> is 1, then more detailed information will be displayed.
* If tasks are blocked on the queue, they will be displayed in the order
* they will unblock as follows:
*
* \cs
*
*    TID     CPU number shared TCB
* ---------- ---------- ----------
* 0xc7854        0      0xaa580
* 0x97028        0      0xaa59c
* 0x920a8        0      0xaa5b8
* 0x8d128        0      0xaa5d4
*
* value = 4 = 0x4
*
* \ce
*
* RETURNS: The number of blocked tasks, or ERROR if semaphore is not shared.
*
* SEE ALSO: semSmInfoGet
* 
* NOMANUAL
*/

int semSmShow 
    (
    SM_SEM_ID smSemId,	/* shared semaphore to display info on */
    int       level	/* 0 = summary, 1 = details */
    )
    {
    SM_OBJ_TCB *       pSmObjTcb;      /* shared tcb */
    int *              smObjTcbList;   /* shared tcb list */
    int *              pList;          /* shared tcb list pointer */
    int                numTasks;       /* current # of blocked tasks */
    int                ix;
    int                maxTasks = 100; /* absolute max# of tasks displayed */
    SM_SEM_ID volatile smSemIdv = (SM_SEM_ID volatile) smSemId;
    int                tmp;
    char *             semTypeMsg [MAX_SEM_TYPE] =
        {
        "BINARY", "MUTEX", "COUNTING", "OLD",
	"SHARED BINARY", "SHARED COUNTING", "\0", "\0"
        };

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = smSemIdv->verify;                     /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (smSemIdv) != OK)          /* check semaphore */
        {
        return (ERROR);
        }

    /* allocate shared tcb list */

    smObjTcbList = (int *) malloc (maxTasks * sizeof (int));

    if (smObjTcbList == NULL)
        {
	return (ERROR);
        }

    /* get list of shared TCBs blocked on smSemId */

    numTasks = semSmInfo (smSemId, smObjTcbList, maxTasks);

    if (numTasks == ERROR)
	{
	free (smObjTcbList);
	return (ERROR);
	}

    /* show summary information */

    printf ("\n");
    printf ("%-20s: 0x%-10x\n", "Semaphore Id", SM_OBJ_ADRS_TO_ID (smSemId));
    printf ("%-20s: %-10s\n", "Semaphore Type", 
	    semTypeMsg[ntohl (smSemId->objType)]);
    printf ("%-20s: %-10s\n", "Task Queuing", "FIFO");
    printf ("%-20s: %-10d\n", "Pended Tasks", numTasks);

    switch (ntohl (smSemId->objType))
        {
        case SEM_TYPE_SM_BINARY:
            if (smSemId->state.flag == htonl (SEM_EMPTY))
                {
                printf ("%-20s: %-10s\n", "State", "EMPTY");
                }
            else
                {
                printf ("%-20s: %-10s\n", "State", "FULL");
                }
            break;

        case SEM_TYPE_SM_COUNTING:
            printf ("%-20s: %-10d\n", "Count", ntohl (smSemId->state.count));
            break;

	default :
	    break;
	}

    if (level == 0)		/* no more info required */
  	{
	free (smObjTcbList);
	return (numTasks);
	}

    /* print list of TCB and CPU number if required */

    if (numTasks == 0)
	{
	free (smObjTcbList);
	printf ("\nNo Pended Task.\n\n");
	return (numTasks);
	}

    printf ("\n");
    printf ("   TID     CPU Number Shared TCB\n");
    printf ("---------- ---------- ----------\n");

    pList = smObjTcbList; 			/* go to first shared TCB */

    for (ix = 0; ix < numTasks; ix++)
	{
        pSmObjTcb = (SM_OBJ_TCB *) *pList;	/* get shared TCB */
	printf ("%#-10x    %2d      %#-10x\n", ntohl ((int)pSmObjTcb->localTcb),
		ntohl (pSmObjTcb->ownerCpu), (int) pSmObjTcb);
	pList++;				/* go to next shared TCB */
	}
    
    printf ("\n");
    free (smObjTcbList);

    return (numTasks);
    }


/* qFifoGLib.c - global FIFO queue library (VxMP Option) */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,06may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
01f,24oct01,mas  fixed diab warnings (SPR 71120); doc update (SPR 71149)
01e,29jan93,pme  added little endian support
01d,02oct92,pme  added SPARC support.
01c,29sep92 pme  added comments to qFifoGRemove.
01b,30jul92,pme	 removed qFifoGLibInit, added qFifoGInit.
                 cleanup.
01a,19jul92,pme	 fixed shared semaphore race, made qFifoGRemove return STATUS.
               	 written.
*/

/*
DESCRIPTION
This library contains routines to manage a first-in-first-out global queue
stored in shared memory.

There are no user callable routines.

This queue complies with the multi-way queue data structures and thus may be
utilized by any multi-way queue.  The FIFO multi-way queue class is accessed
by the global ID qFifoGClassId.

The pointer to a global fifo queue points to a structure which
contains the effective queue and an integer used as a spin-lock for the queue.
This mechanism allows use of standard kernel functions such as windPendPut()
and windPendQGet() to operate on multi-processor shared queues.

AVAILABILITY
This module is distributed as a component of the unbundled shared memory
objects support option, VxMP.

INTERNAL
The routine qFifoGPut() takes a key in addition to a node to queue.  This key
determines whether the node is placed at the head or tail of the queue.

INCLUDE FILE: qFifoGLib.h

SEE ALSO: qLib, smObjLib,
\tb VxWorks Programmer's Guide: Shared Memory Objects

NOROUTINES
*/

#include "vxWorks.h"
#include "intLib.h"
#include "smLib.h"
#include "qClass.h"
#include "qFifoGLib.h"
#include "stdlib.h"
#include "taskLib.h"
#include "cacheLib.h"
#include "netinet/in.h"
#include "private/smObjLibP.h"

/* forward static functions */

static STATUS qFifoGNullRtn (void);

/* locals */

LOCAL Q_CLASS qFifoGClass =
    {
    (FUNCPTR)qFifoGNullRtn,
    (FUNCPTR)qFifoGInit,
    (FUNCPTR)qFifoGNullRtn,
    (FUNCPTR)qFifoGNullRtn,
    (FUNCPTR)qFifoGPut,
    (FUNCPTR)qFifoGGet,
    (FUNCPTR)qFifoGRemove,
    (FUNCPTR)qFifoGNullRtn,
    (FUNCPTR)qFifoGNullRtn,
    (FUNCPTR)qFifoGNullRtn,
    (FUNCPTR)qFifoGNullRtn,
    (FUNCPTR)qFifoGNullRtn,
    (FUNCPTR)qFifoGInfo,
    (FUNCPTR)qFifoGEach,
    &qFifoGClass
    };

/* globals */

Q_CLASS_ID qFifoGClassId = &qFifoGClass;


/******************************************************************************
*
* qFifoGInit - initialize global FIFO queue 
*
* This routine initializes the specified global FIFO queue.
*
* RETURNS: OK, or ERROR if FIFO queue could not be initialized.
*
* NOMANUAL
*/

STATUS qFifoGInit 
    (
    Q_FIFO_G_HEAD * pQFifoGHead		/* global FIFO queue to initialize */
    )
    {
    Q_FIFO_G_HEAD volatile * pQFifoGHeadv =
                                        (Q_FIFO_G_HEAD volatile *) pQFifoGHead;
    void *                   temp;

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = (void *) pQFifoGHeadv->pFifoQ;       /* PCI bridge bug [SPR 68844]*/

    smDllInit (pQFifoGHeadv->pFifoQ);	/* initialize the real queue */

    return (OK);
    }

/******************************************************************************
*
* qFifoGPut - add a node to a global FIFO queue
*
* This routine adds the specifed node to the global FIFO queue.
* The insertion is either at the tail or head based on the specified <key>
* FIFO_KEY_TAIL or FIFO_KEY_HEAD respectively.
*
* This routine is only called through Q_PUT in windPendQPut() to add a shared
* task control block into a semaphore pend Queue.  Thus, what we always
* have to do when this function is called is to insert the shared TCB
* of the task that calls us in the semaphore queue specified by <pQFifoGHead>.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void qFifoGPut 
    (
    Q_FIFO_G_HEAD * pQFifoGHead,	/* Q to where to put node */	
    SM_DL_NODE *    pQFifoGNode,	/* not used */
    ULONG           key 
    )
    {
    Q_FIFO_G_HEAD volatile * pQFifoGHeadv =
                                        (Q_FIFO_G_HEAD volatile *) pQFifoGHead;
    void *                   temp;

    /* appease the Diab gods (SPR 71120) */

    if (pQFifoGNode == NULL)
        ;

    /* do the shared TCB queueing */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    if (key == FIFO_KEY_HEAD)
        {
        temp = (void *) pQFifoGHeadv->pFifoQ;   /* PCI bridge bug [SPR 68844]*/
	smDllInsert (pQFifoGHeadv->pFifoQ, (SM_DL_NODE *) NULL, 
		     (SM_DL_NODE *) taskIdCurrent->pSmObjTcb);
        }
    else
        {
        temp = (void *) pQFifoGHeadv->pFifoQ;   /* PCI bridge bug [SPR 68844]*/
	smDllAdd (pQFifoGHeadv->pFifoQ, (SM_DL_NODE*)taskIdCurrent->pSmObjTcb);
        }
    }

/******************************************************************************
*
* qFifoGGet - get the first node out of a global FIFO queue
*
* This routine dequeues and returns the first node of a global FIFO queue.
* If the queue is empty, NULL will be returned.
*
* RETURNS: pointer the first node, or NULL if queue is empty.
*
* NOMANUAL
*/

SM_DL_NODE * qFifoGGet 
    (
    Q_FIFO_G_HEAD * pQFifoGHead 	/* Q to get node from */
    )
    {
    SM_DL_NODE volatile *    node;
    Q_FIFO_G_HEAD volatile * pQFifoGHeadv =
                                        (Q_FIFO_G_HEAD volatile *) pQFifoGHead;
    void *                   temp;

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = (void *) pQFifoGHeadv->pFifoQ;       /* PCI bridge bug [SPR 68844]*/

    if (SM_DL_EMPTY (pQFifoGHeadv->pFifoQ))
        {
	return (NULL);
        }

    node = (SM_DL_NODE volatile *) smDllGet (pQFifoGHeadv->pFifoQ);

    return ((SM_DL_NODE *) ntohl ((int) \
                                (((SM_OBJ_TCB volatile *) (node))->localTcb)));
    }

/******************************************************************************
*
* qFifoGRemove - remove the specified node from a global FIFO queue
*
* This routine removes the specified node from a global FIFO queue.
*
* RETURNS: OK, ALREADY_REMOVED if node was removed by a timeout, a task
*          deletion or a siganl, or ERROR if lock cannot be taken.
*
* ERRNO:  S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

STATUS qFifoGRemove 
    (
    Q_FIFO_G_HEAD * pQFifoGHead,	/* Q head to remove from */
    SM_DL_NODE *    pQFifoGNode 	/* pointer to node to remove */
    )
    {
    BOOL                     status;	/* return status */
    int                      level;	/* inlock return value */
    Q_FIFO_G_HEAD volatile * pQFifoGHeadv =
                                        (Q_FIFO_G_HEAD volatile *) pQFifoGHead;
    SM_DL_NODE volatile *    pQFifoGNodev = (SM_DL_NODE volatile *)pQFifoGNode;
    void *                   temp;

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = (void *) pQFifoGHeadv->pLock;        /* PCI bridge bug [SPR 68844]*/

    if (pQFifoGHeadv->pLock != NULL)
	{
        /* 
         * qFifoGRemove can be called by windPendQRemove() to remove a task
         * from a semaphore pend queue when a task is deleted or when a delay
	 * expires.  It can also be called by windRestart() when a signal is
	 * sent to the task.  In all cases this function is called with
	 * pQFifoGHead->pLock not NULL indicating that lock access must
	 * be obtained before manipulating the queue and a pointer to
	 * the task control block is passed as node to remove, but
	 * actually what we need to remove from the pendQ is the shared TCB
	 * not the TCB itself.
         */

	/* ENTER LOCKED SECTION */

	if (SM_OBJ_LOCK_TAKE(pQFifoGHeadv->pLock, &level) == ERROR)
	    {
	    return (ERROR);
	    }

	/* 
	 * What can happen here is that because the notification time
	 * is not null, the shared TCB may have been removed by a semGive()
	 * on another CPU but this removal has not shown up on the take
	 * side.
	 */
	  
        if (! ntohl (((WIND_TCB volatile *) (pQFifoGNodev))\
                                                   ->pSmObjTcb->removedByGive))
	    {
	    /*
	     * This is the normal case, a task deletion, a timeout or
	     * a signal when the shared TCB has not been already removed
	     * by semGive() on another CPU, so we remove it.
	     */

	     /*
	      * If this shared TCB has been previously removed from a shared
	      * semaphore pendQ, removedByGive has been reset to FALSE
	      * by smObjEventProcess().
	      */

	    smDllRemove (pQFifoGHeadv->pFifoQ, (SM_DL_NODE *)\
                          (((WIND_TCB volatile *) (pQFifoGNodev))->pSmObjTcb));

	    status = OK;
	    }
        else
	    {
	    /*
	     * This is the bad case, the removedByGive field of the shared
	     * TCB has been set to TRUE.  In that case we set the local TCB
	     * pointer in the shared TCB to NULL indicating that the task
	     * has been deleted, got a timeout or has received a signal.
	     * This will be use by smObjEventProcess() when the shared TCB
	     * eventually shows up.
	     * We also set the pointer to the shared TCB stored in the local
	     * TCB to NULL to oblige the task to re-allocate a shared TCB if
	     * it is not deleted and goes back to a semTake() on the same or
	     * on another shared semaphore.
	     * Then we return a special status to the upper layers of
	     * the kernel.
	     */

	     /* 
	      * This cannot be done outside the LOCKED SECTION because
	      * we want to be sure that the notification interrupt
	      * does not occur between the test on removedByGive and here.
	      */

	     /*
	      * Note that we don't need to reset removedByGive to FALSE
	      * because this shared TCB will be dropped by smObjEventProcess.
	      */

	    ((WIND_TCB volatile *) (pQFifoGNodev))->pSmObjTcb->localTcb = NULL;
	    ((WIND_TCB volatile *) (pQFifoGNodev))->pSmObjTcb = NULL;

	    status = ALREADY_REMOVED;
	    }

        /* EXIT LOCKED SECTION */

        SM_OBJ_LOCK_GIVE (pQFifoGHeadv->pLock, level);
	}

    else
	{
	/* 
	 * We are called by semGive() with lock already obtained,
	 * simply remove the node from the queue.  Actually,
	 * pQFifoGNode is a pointer to a shared TCB and pQFifoGHead->pFifoQ
	 * is a shared semaphore pendQ.
	 */

    	smDllRemove (pQFifoGHeadv->pFifoQ, pQFifoGNode);

	/* tell everybody that we are no longer on the semaphore pend Q */ 

	((SM_OBJ_TCB volatile *) (pQFifoGNodev))->removedByGive = htonl (TRUE);
				
	status = OK;
	}

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = (void *) pQFifoGHeadv->pLock;        /* BRIDGE FLUSH  [SPR 68334] */

    return (status);
    }

/******************************************************************************
*
* qFifoGInfo - get information of a FIFO queue
*
* This routine returns information of a FIFO queue.  If the parameter
* <nodeArray> is NULL, the number of queued nodes is returned.  Otherwise, the
* specified <nodeArray> is filled with node pointers of the FIFO queue
* starting from the head.  Node pointers are copied until the TAIL of the
* queue is reached or until <maxNodes> has been entered in the <nodeArray>.
*
* RETURNS: Number of nodes entered in nodeArray, or nodes in FIFO queue.
*
* NOMANUAL
*/

int qFifoGInfo 
    (
    Q_FIFO_G_HEAD * pQFifoGHead,/* FIFO queue to gather list for */
    int             nodeArray[],/* array of node pointers to be filled in */
    int             maxNodes    /* max node pointers nodeArray can accomodate*/
    )
    {
    SM_DL_NODE volatile *    pNode;
    int *                    pElement     = nodeArray;
    Q_FIFO_G_HEAD volatile * pQFifoGHeadv = \
                                        (Q_FIFO_G_HEAD volatile *) pQFifoGHead;
    void *                   temp;

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = (void *) pQFifoGHeadv->pFifoQ;       /* PCI bridge bug [SPR 68844]*/

    pNode = (SM_DL_NODE volatile *) SM_DL_FIRST (pQFifoGHeadv->pFifoQ);

    /* NULL node array means return count */

    if (nodeArray == NULL)
        {
	return (smDllCount (pQFifoGHeadv->pFifoQ));
        }

    while ((pNode != LOC_NULL) && (--maxNodes >= 0))
	{
	*(pElement++) = (int) pNode;	                /* fill in table */
	pNode = (SM_DL_NODE volatile *) SM_DL_NEXT (pNode); /* next node */
	}

    return (pElement - nodeArray);	/* return count of active tasks */
    }

/******************************************************************************
*
* qFifoGEach - call a routine for each node in a queue
*
* This routine calls a user-supplied <routine> once for each node in the
* queue.  The routine should be declared as follows:
* \cs
*  BOOL routine (pQNode, arg)
*      SM_DL_NODE volatile * pQNode;	/@ pointer to a queue node          @/
*      int	             arg;	/@ arbitrary user-supplied argument @/
* \ce
* The user-supplied <routine> should return TRUE if qFifoGEach() is to continue
* calling it for each entry, or FALSE if it is done and qFifoGEach() can exit.
*
* RETURNS: NULL if traversed whole queue, or pointer to SM_DL_NODE that
*          qFifoGEach() stopped on.
*
* NOMANUAL
*/

SM_DL_NODE * qFifoGEach 
    (
    Q_FIFO_G_HEAD * pQHead,     /* queue head of queue to call routine for */
    FUNCPTR         routine,    /* the routine to call for each table entry */
    int             routineArg  /* arbitrary user-supplied argument */
    )
    {
    SM_DL_NODE volatile *    pQNode;
    Q_FIFO_G_HEAD volatile * pQHeadv = (Q_FIFO_G_HEAD volatile *) pQHead;
    void *                   temp;

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = (void *) pQHeadv->pFifoQ;            /* PCI bridge bug [SPR 68844]*/

    pQNode = (SM_DL_NODE volatile *) SM_DL_FIRST (pQHeadv->pFifoQ);

    while ((pQNode != NULL) && ((* routine) (pQNode, routineArg)))
        {
	pQNode = (SM_DL_NODE volatile *) SM_DL_NEXT (pQNode);
        }

    return ((SM_DL_NODE *) pQNode);		/* return node we ended with */
    }

/******************************************************************************
*
* qFifoGNullRtn - null routine for global queue class structure
*
* This routine does nothing and returns OK.
*
* RETURNS: OK
*
* NOMANUAL
*/

LOCAL STATUS qFifoGNullRtn (void)
    {
    return (OK);
    }


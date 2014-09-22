/* ioQLib.c - I/O queue library for async I/O */

/* Copyright 1984-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01c,26jan94,kdl  changed name from ioQPxLib.c to ioQLib.c.
01b,05jan94,kdl  general cleanup.
01a,04apr93,elh  written.
*/

/* 
DESCRIPTION

This library is used in conjuction with asynchronous I/O.  It defines
the I/O queues used by AIO drivers, as well as the functions to 
manipulate these queues.  To use this library, the driver mantains a 
definition of an IO_Q in its driver specific structures.  The IO_Q
structure defines the work queue, the wait queue and the lock routines 
to synchronize access to these queues. Each I/O operation is defined by 
the ioNode structure.   

INTERNAL
The I/O queues are currently written as linked lists.  This is probably 
not reasonable for I/O drivers.  These routines should (are expected to) 
change if/when true asynchronous drivers are written for VxWorks.

NOMANUAL 
*/

/* includes */

#include "vxWorks.h"
#include "ioQLib.h"

/*******************************************************************************
* 
* ioQInit - initialize an I/O queue structure.
* 
* This routine initializes the I/O work and wait queue associated with <pQ>.
* <pLock> and <pUnlock> are the routines which provide mutual exclusion 
* of the queue.
*
* RETURNS: N/A
*/

void ioQInit 
    (
    IO_Q *		pQ,			/* queue */
    VOIDFUNCPTR		pLock,			/* lock routine */
    VOIDFUNCPTR		pUnlock,		/* unlock routine */
    int 		lockArg			/* lock/unlock arg */
    )
    {

    /* Initialize linked lists */

    lstInit (&pQ->workQ);
    lstInit (&pQ->waitQ);

    /* Set up queue function pointers */

    pQ->lock    = pLock;
    pQ->unlock  = pUnlock;
    pQ->lockArg = lockArg;
    }

/*******************************************************************************
* 
* ioQAdd - add an I/O node to an I/O queue. 
* 
* This routine adds <pNode> to the queue <pQ>.   Requests are queued in priority
* order according to <prio>.
*
* RETURNS: N/A
*/

void ioQAdd
    (
    LIST *		pQ,			/* I/O queue */
    IO_NODE *		pNode, 			/* I/O node */
    int			prio			/* priority */
    )
    {						
    IO_NODE *		pTemp; 			/* temp var */

    pNode->prio = prio;
    for (pTemp = (IO_NODE *) lstFirst (pQ); (pTemp != NULL); 
	 pTemp = (IO_NODE *) lstNext (&pTemp->node))
	{
	if (prio < pTemp->prio)
	    {
	    lstInsert (pQ, pTemp->node.previous, &pNode->node);
	    return;
	    }
	}
    lstAdd (pQ, &pNode->node);	
    }

/*******************************************************************************
* 
* ioQNodeDone - node completed 
*
* ioQNodeDone gets called on completion of I/O node <pNode>.
*
* RETURNS: N/A
*/

void ioQNodeDone
    (
    IO_NODE *   	pNode			/* I/O Node */
    )
    {
    (* pNode->doneRtn) (pNode);
    }

/*******************************************************************************
* 
* ioQEach - call routine for node on queue. 
*
* This routine calls <pRoutine> with arguments <arg1> and <arg2>.  It does
* this for each node on list <pQ> until <pRoutine> returns FALSE or until
* the end of the list is reached.
*
* RETURNS: the pointer to the node which returned false or 
*	   NULL if all elements in the list were processed.
*/

IO_NODE * ioQEach 
    (
    LIST * 		pQ,			/* I/O list */
    FUNCPTR		pRoutine,		/* routine to call */
    int			arg1,			/* argument 1 */
    int			arg2			/* argument 2 */
    )
    {
    IO_NODE *		pNode;			/* I/O node */
    IO_NODE *		pNext;			/* next I/O node */

    for (pNode = (IO_NODE *) lstFirst (pQ); (pNode != NULL);)
     	{
	pNext = (IO_NODE *) lstNext (&pNode->node);

  	if (((* pRoutine) (pNode, arg1, arg2)) == FALSE)
	    break;

	pNode = pNext;
        }

    return (pNode);
    }

/*******************************************************************************
* 
* ioQLockSem - lock I/O queue with semaphores
*
* This routine provides mutual exclusion to the I/O queues with semaphores.
* It is an example lock routine that can be passed to ioQInit.
*
* RETURNS: N/A
*/

void ioQLockSem
    (
    SEM_ID	semId				/* semaphore */
    )
    {
    semTake (semId, WAIT_FOREVER);
    }

/*******************************************************************************
* 
* ioQUnlockSem - unlock I/O queue with semaphores
*
* This routine provides mutual exclusion to the I/O queues with semaphores.
* It is an example lock routine that can be passed to ioQInit.
*
* RETURNS: N/A
*/

void ioQUnlockSem
    (
    SEM_ID	semId				/* semaphore */
    )
    {
    semGive (semId);
    }

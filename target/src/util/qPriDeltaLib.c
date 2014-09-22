/* qPriDeltaLib.c - priority queue with relative priority sorting library */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01h,19jul92,pme  made qPriDeltaRemove return STATUS.
01g,26may92,rrr  the tree shuffle
01f,19nov91,rrr  shut up some ansi warnings.
01e,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
01d,28sep90,jcf	 documentation.
01c,05jul90,jcf	 added null routine for calibrateRtn field in Q_CLASS.
01b,26jun90,jcf	 fixed queue class definition.
01a,14jun89,jcf	 written.
*/

/*
DESCRIPTION
This library contains routines to manage a priority queue.  The queue is
maintained in priority order with each node key a priority delta to the node
ahead of it.  This queue performs a qPriDeltaPut() operation preportional in
time with number of nodes in the queue.  This queue is used for timer queues.

This queue complies with the multi-way queue data structures and thus may be
utilized by any multi-way queue.  The priority delta multi-way queue
class is accessed by the global id qPriDeltaClassId.

SEE ALSO: qLib.
*/

#include "vxWorks.h"
#include "qClass.h"
#include "qPriDeltaLib.h"
#include "stdlib.h"

IMPORT ULONG vxTicks;		/* current time in ticks */


/* forward static functions */

static STATUS qPriDeltaNullRtn (void);


/* locals */

LOCAL Q_CLASS qPriDeltaClass =
    {
    (FUNCPTR)qPriDeltaCreate,
    (FUNCPTR)qPriDeltaInit,
    (FUNCPTR)qPriDeltaDelete,
    (FUNCPTR)qPriDeltaTerminate,
    (FUNCPTR)qPriDeltaPut,
    (FUNCPTR)qPriDeltaGet,
    (FUNCPTR)qPriDeltaRemove,
    (FUNCPTR)qPriDeltaResort,
    (FUNCPTR)qPriDeltaAdvance,
    (FUNCPTR)qPriDeltaGetExpired,
    (FUNCPTR)qPriDeltaKey,
    (FUNCPTR)qPriDeltaNullRtn,
    (FUNCPTR)qPriDeltaInfo,
    (FUNCPTR)qPriDeltaEach,
    &qPriDeltaClass
    };

/* globals */

Q_CLASS_ID qPriDeltaClassId = &qPriDeltaClass;


/******************************************************************************
*
* qPriDeltaCreate - allocate and initialize a priority delta queue
*
* This routine allocates and initializes a priority delta queue by allocating
* a Q_PRI_HEAD structure from the free memory pool.
*
* RETURNS:
*  Pointer to a Q_PRI_HEAD, or
*  NULL if out of memory.
*/

Q_PRI_HEAD *qPriDeltaCreate (void)
    {
    Q_PRI_HEAD *pQPriHead = (Q_PRI_HEAD *) malloc (sizeof (Q_PRI_HEAD));

    if (pQPriHead == NULL)
	return (NULL);

    qPriDeltaInit (pQPriHead);

    return (pQPriHead);
    }

/******************************************************************************
*
* qPriDeltaInit - initialize a priority delta queue
*
* This routine initializes the specified priority delta queue.
*/

STATUS qPriDeltaInit
    (
    Q_PRI_HEAD *pQPriHead
    )
    {
    dllInit (pQPriHead);	 /* initialize doubly linked list */

    return (OK);
    }

/******************************************************************************
*
* qPriDeltaDelete - delete a priority delta queue
*
* This routine deallocates memory associated with the queue.  All queued
* nodes are lost.
*
* RETURNS:
*  OK, or
*  ERROR if memory cannot be deallocated.
*/

STATUS qPriDeltaDelete
    (
    Q_PRI_HEAD *pQPriHead
    )
    {
    free ((char *) pQPriHead);
    return OK;
    }

/******************************************************************************
*
* qPriDeltaTerminate - terminate a priority delta queue
*
* This routine terminates a priority delta queue.  All queued nodes are lost.
*
* ARGSUSED
*/

STATUS qPriDeltaTerminate
    (
    Q_PRI_HEAD *pQPriHead
    )
    {
    return (OK);
    }

/*******************************************************************************
*
* qPriDeltaPut - insert a node into a priority delta queue
*
* This routine inserts a node into a priority delta queue.  The insertion is
* based on the priority key.  Lower key values are higher in priority.
*/

void qPriDeltaPut
    (
    Q_PRI_HEAD  *pQPriHead,
    Q_PRI_NODE  *pQPriNode,
    ULONG       key
    )
    {
    FAST Q_PRI_NODE *pQNode = (Q_PRI_NODE *) DLL_FIRST (pQPriHead);

    pQPriNode->key = key - vxTicks;	/* relative queue keeps delta time */

    while (pQNode != NULL)		/* empty list? */
        {
	if (pQPriNode->key < pQNode->key)
	    {
	    /* We've reached the place in the delta queue to insert this task */

	    dllInsert (pQPriHead, DLL_PREVIOUS (&pQNode->node),
		       &pQPriNode->node);

	    /* Now decrement the delta key of the guy behind us. */

	    pQNode->key -= pQPriNode->key;
	    return;				/* we're done */
	    }

	pQPriNode->key -= pQNode->key;
	pQNode = (Q_PRI_NODE *) DLL_NEXT (&pQNode->node);
	}

    /* if we fall through, add to end of delta q */

    dllInsert (pQPriHead, (DL_NODE *) DLL_LAST (pQPriHead), &pQPriNode->node);
    }

/*******************************************************************************
*
* qPriDeltaGet - remove and return first node in priority delta queue
*
* This routine removes and returns the first node in a priority delta queue.  If
* the queue is empty, NULL is returned.
*
* RETURNS
*  Pointer to first queue node in queue head, or
*  NULL if queue is empty.
*/

Q_PRI_NODE *qPriDeltaGet
    (
    Q_PRI_HEAD *pQPriHead
    )
    {
    if (DLL_EMPTY (pQPriHead))
	return (NULL);

    return ((Q_PRI_NODE *) dllGet (pQPriHead));
    }

/*******************************************************************************
*
* qPriDeltaRemove - remove a node from a priority delta queue
*
* This routine removes a node from the specified priority delta queue.
*/

STATUS qPriDeltaRemove
    (
    Q_PRI_HEAD *pQPriHead,
    Q_PRI_NODE *pQPriNode
    )
    {
    Q_PRI_NODE *pNextNode = (Q_PRI_NODE *) DLL_NEXT (&pQPriNode->node);

    if (pNextNode != NULL)
	pNextNode->key += pQPriNode->key; /* add key to next node */

    dllRemove (pQPriHead, &pQPriNode->node);

    return (OK);
    }

/*******************************************************************************
*
* qPriDeltaResort - resort a node to a new position based on a new key
*
* This routine resorts a node to a new position based on a new priority key.
*/

void qPriDeltaResort
    (
    Q_PRI_HEAD *pQPriHead,
    Q_PRI_NODE *pQPriNode,
    ULONG       newKey
    )
    {
    qPriDeltaRemove (pQPriHead, pQPriNode);
    qPriDeltaPut (pQPriHead, pQPriNode, newKey);
    }

/*******************************************************************************
*
* qPriDeltaAdvance - advance a queues concept of time
*
* Priority delta queues keep nodes prioritized by time-to-fire.  The library
* utilizes this routine to advance time.  It is usually called from within a
* clock-tick interrupt service routine.
*/

void qPriDeltaAdvance
    (
    Q_PRI_HEAD *pQPriHead
    )
    {
    FAST Q_PRI_NODE *pQPriNode = (Q_PRI_NODE *) DLL_FIRST (pQPriHead);

    if (pQPriNode != NULL)
	pQPriNode->key --;
    }

/*******************************************************************************
*
* qPriDeltaGetExpired - return a time-to-fire expired node
*
* This routine returns a time-to-fire expired node in a priority delta timer
* queue.  Expired nodes result from a qPriDeltaAdvance(2) advancing a node
* beyond its delay.  As many nodes may expire on a single qPriDeltaAdvance(2),
* this routine should be called within a while loop until NULL is returned.
* NULL is returned when there are no expired nodes.
*
* RETURNS
*  Pointer to first queue node in queue head, or
*  NULL if queue is empty.
*/

Q_PRI_NODE *qPriDeltaGetExpired
    (
    Q_PRI_HEAD *pQPriHead
    )
    {
    FAST Q_PRI_NODE *pQPriNode = (Q_PRI_NODE *) DLL_FIRST (pQPriHead);

    if (pQPriNode != NULL)
	{
	if (pQPriNode->key != 0)
	    pQPriNode = NULL;
	else
	    qPriDeltaRemove (pQPriHead, pQPriNode);	/* get off delay list */
	}

    return (pQPriNode);
    }

/*******************************************************************************
*
* qPriDeltaKey - return the key of a node
*
* This routine returns the key of a node currently in a priority delta queue.
*
* RETURNS
*  Node's key.
*
* ARGSUSED
*/

ULONG qPriDeltaKey
    (
    Q_PRI_NODE  *pQPriNode      /* node to get key for */
    )
    {
    FAST ULONG sum = 0;

    while (pQPriNode != NULL)
        {
	sum += pQPriNode->key;
	pQPriNode = (Q_PRI_NODE *) DLL_PREVIOUS (&pQPriNode->node);
	}

    return (sum);	/* return key */
    }

/*******************************************************************************
*
* qPriDeltaInfo - gather information on a priority delta queue
*
* This routine fills up to maxNodes elements of a nodeArray with nodes
* currently in a multi-way queue.  The actual number of nodes copied to the
* array is returned.  If the nodeArray is NULL, then the number of nodes in
* the priority delta queue is returned.
*
* RETURNS
*  Number of node pointers copied into the nodeArray, or
*  Number of nodes in multi-way queue if nodeArray is NULL
*/

int qPriDeltaInfo
    (
    Q_PRI_HEAD *pQPriHead,      /* priority queue to gather list for */
    FAST int nodeArray[],       /* array of node pointers to be filled in */
    FAST int maxNodes           /* max node pointers nodeArray can accomodate */
    )
    {
    FAST Q_PRI_NODE *pQNode = (Q_PRI_NODE *) DLL_FIRST (pQPriHead);
    FAST int *pElement  = nodeArray;

    if (nodeArray == NULL)		/* NULL node array means return count */
	return (dllCount (pQPriHead));

    while ((pQNode != NULL) && (--maxNodes >= 0))
	{
	*(pElement++) = (int)pQNode;			/* fill in table */
	pQNode = (Q_PRI_NODE *) DLL_NEXT (&pQNode->node);	/* next node */
	}

    return (pElement - nodeArray);	/* return count of active tasks */
    }

/*******************************************************************************
*
* qPriDeltaEach - call a routine for each node in a queue
*
* This routine calls a user-supplied routine once for each node in the
* queue.  The routine should be declared as follows:
* .CS
*  BOOL routine (pQNode, arg)
*      Q_PRI_NODE *pQNode;	/@ pointer to a queue node          @/
*      int   	  arg;		/@ arbitrary user-supplied argument @/
* .CE
* The user-supplied routine should return TRUE if qPriDeltaEach (2) is to
* continue calling it for each entry, or FALSE if it is done and
* qPriDeltaEach can exit.
*
* RETURNS: NULL if traversed whole queue, or pointer to Q_PRI_NODE that
*          qPriDeltaEach stopped on.
*/

Q_PRI_NODE *qPriDeltaEach
    (
    Q_PRI_HEAD  *pQHead,        /* queue head of queue to call routine for */
    FUNCPTR     routine,        /* the routine to call for each table entry */
    int         routineArg      /* arbitrary user-supplied argument */
    )
    {
    FAST Q_PRI_NODE *pQNode = (Q_PRI_NODE *) DLL_FIRST (pQHead);

    while ((pQNode != NULL) && ((* routine) (pQNode, routineArg)))
	pQNode = (Q_PRI_NODE *) DLL_NEXT (&pQNode->node);

    return (pQNode);			/* return node we ended with */
    }

/*******************************************************************************
*
* qPriDeltaNullRtn - nop that returns OK
*
* This routine does nothing and returns OK.  It is used to by the queue class
* structure for unsupported functions.
*/

LOCAL STATUS qPriDeltaNullRtn (void)
    {
    return (OK);
    }

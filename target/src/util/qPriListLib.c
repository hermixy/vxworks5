/* qPriListLib.c - priority ordered linked list queue library */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01h,19jul92,pme  made qPriListRemove return STATUS.
01g,26may92,rrr  the tree shuffle
01f,19nov91,rrr  shut up some ansi warnings.
01e,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
01d,28sep90,jcf	 documentation.
01c,05jul90,jcf  added qPriListCalibrate().
01b,26jun90,jcf	 fixed qPriListResort();
01a,14jun89,jcf	 written.
*/

/*
DESCRIPTION
This library contains routines to manage a priority queue.  The queue is
maintained in priority order with simple priority insertion into a linked list.
This queue performs a qPriListPut() operation preportional in time with number
of nodes in the queue.  This general purpose priority queue has many uses
including the basis for priority semaphore queues.

This queue complies with the multi-way queue data structures and thus may be
utilized by any multi-way queue.  The priority list multi-way queue
class is accessed by the global id qPriListClassId.

SEE ALSO: qLib.
*/

#include "vxWorks.h"
#include "qClass.h"
#include "qPriListLib.h"
#include "stdlib.h"

IMPORT ULONG vxTicks;		/* current time in ticks */

/* locals */

LOCAL Q_CLASS qPriListClass =
    {
    (FUNCPTR)qPriListCreate,
    (FUNCPTR)qPriListInit,
    (FUNCPTR)qPriListDelete,
    (FUNCPTR)qPriListTerminate,
    (FUNCPTR)qPriListPut,
    (FUNCPTR)qPriListGet,
    (FUNCPTR)qPriListRemove,
    (FUNCPTR)qPriListResort,
    (FUNCPTR)qPriListAdvance,
    (FUNCPTR)qPriListGetExpired,
    (FUNCPTR)qPriListKey,
    (FUNCPTR)qPriListCalibrate,
    (FUNCPTR)qPriListInfo,
    (FUNCPTR)qPriListEach,
    &qPriListClass
    };

LOCAL Q_CLASS qPriListFromTailClass =
    {
    (FUNCPTR)qPriListCreate,
    (FUNCPTR)qPriListInit,
    (FUNCPTR)qPriListDelete,
    (FUNCPTR)qPriListTerminate,
    (FUNCPTR)qPriListPutFromTail,
    (FUNCPTR)qPriListGet,
    (FUNCPTR)qPriListRemove,
    (FUNCPTR)qPriListResort,
    (FUNCPTR)qPriListAdvance,
    (FUNCPTR)qPriListGetExpired,
    (FUNCPTR)qPriListKey,
    (FUNCPTR)qPriListCalibrate,
    (FUNCPTR)qPriListInfo,
    (FUNCPTR)qPriListEach,
    &qPriListFromTailClass
    };

/* globals */

Q_CLASS_ID qPriListClassId	   = &qPriListClass;
Q_CLASS_ID qPriListFromTailClassId = &qPriListFromTailClass;


/******************************************************************************
*
* qPriListCreate - allocate and initialize a priority list queue
*
* This routine allocates and initializes a priority list queue by allocating a
* Q_PRI_HEAD structure from the free memory pool.
*
* RETURNS:
*  Pointer to a Q_PRI_HEAD, or
*  NULL if out of memory.
*/

Q_PRI_HEAD *qPriListCreate (void)
    {
    Q_PRI_HEAD *pQPriHead = (Q_PRI_HEAD *) malloc (sizeof (Q_PRI_HEAD));

    if (pQPriHead == NULL)
	return (NULL);

    qPriListInit (pQPriHead);

    return (pQPriHead);
    }

/******************************************************************************
*
* qPriListInit - initialize a priority list queue
*
* This routine initializes the specified priority list queue.
*/

STATUS qPriListInit
    (
    Q_PRI_HEAD *pQPriHead
    )
    {
    dllInit (pQPriHead);	 /* initialize doubly linked list */

    return (OK);
    }

/******************************************************************************
*
* qPriListDelete - delete a priority list queue
*
* This routine deallocates memory associated with the queue.  All queued
* nodes are lost.
*
* RETURNS:
*  OK, or
*  ERROR if memory cannot be deallocated.
*/

STATUS qPriListDelete
    (
    Q_PRI_HEAD *pQPriHead
    )
    {
    free ((char *) pQPriHead);
    return OK;
    }

/******************************************************************************
*
* qPriListTerminate - terminate a priority list queue
*
* This routine terminates a priority list queue.  All queued nodes will be lost.
*
* ARGSUSED
*/

STATUS qPriListTerminate
    (
    Q_PRI_HEAD *pQPriHead
    )
    {
    return (OK);
    }

/*******************************************************************************
*
* qPriListPut - insert a node into a priority list queue
*
* This routine inserts a node into a priority list queue.  The insertion is
* based on the specified prioriyt key.  The lower the key the higher the
* priority.
*/

void qPriListPut
    (
    Q_PRI_HEAD  *pQPriHead,
    Q_PRI_NODE  *pQPriNode,
    ULONG        key
    )
    {
    FAST Q_PRI_NODE *pQNode = (Q_PRI_NODE *) DLL_FIRST (pQPriHead);

    pQPriNode->key = key;

    while (pQNode != NULL)
        {
	if (key < pQNode->key)		/* it will be last of same priority */
	    {
	    dllInsert (pQPriHead, DLL_PREVIOUS (&pQNode->node),
		       &pQPriNode->node);
	    return;
	    }
	pQNode = (Q_PRI_NODE *) DLL_NEXT (&pQNode->node);
	}

    dllInsert (pQPriHead, (DL_NODE *) DLL_LAST (pQPriHead), &pQPriNode->node);
    }

/*******************************************************************************
*
* qPriListPutFromTail - insert a node into a priority list queue from tail
*
* This routine inserts a node into a priority list queue.  The insertion is
* based on the specified prioriyt key.  The lower the key the higher the
* priority.  Unlike qPriListPut(2), this routine searches for the correct
* position in the queue from the queue's tail.  This is useful if the
* caller has a priori knowledge that the key is of low priority.
*/

void qPriListPutFromTail
    (
    Q_PRI_HEAD  *pQPriHead,
    Q_PRI_NODE  *pQPriNode,
    ULONG        key
    )
    {
    FAST Q_PRI_NODE *pQNode = (Q_PRI_NODE *) DLL_LAST (pQPriHead);

    pQPriNode->key = key;

    while (pQNode != NULL)
        {
	if (key >= pQNode->key)		/* it will be last of same priority */
	    {
	    dllInsert (pQPriHead, &pQNode->node, &pQPriNode->node);
	    return;
	    }
	pQNode = (Q_PRI_NODE *) DLL_PREVIOUS (&pQNode->node);
	}

    dllInsert (pQPriHead, (DL_NODE *)NULL, &pQPriNode->node);
    }

/*******************************************************************************
*
* qPriListGet - remove and return first node in priority list queue
*
* This routine removes and returns the first node in a priority list queue.  If
* the queue is empty, NULL is returned.
*
* RETURNS
*  Pointer to first queue node in queue head, or
*  NULL if queue is empty.
*/

Q_PRI_NODE *qPriListGet
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
* qPriListRemove - remove a node from a priority list queue
*
* This routine removes a node from the specified priority list queue.
*/

STATUS qPriListRemove
    (
    Q_PRI_HEAD *pQPriHead,
    Q_PRI_NODE *pQPriNode
    )
    {
    dllRemove (pQPriHead, &pQPriNode->node);

    return (OK);
    }

/*******************************************************************************
*
* qPriListResort - resort a node to a new position based on a new key
*
* This routine resorts a node to a new position based on a new key.
*/

void qPriListResort
    (
    FAST Q_PRI_HEAD *pQPriHead,
    FAST Q_PRI_NODE *pQPriNode,
    FAST ULONG       newKey
    )
    {
    FAST Q_PRI_NODE *pPrev = (Q_PRI_NODE *)DLL_PREVIOUS (&pQPriNode->node);
    FAST Q_PRI_NODE *pNext = (Q_PRI_NODE *)DLL_NEXT (&pQPriNode->node);

    if (((pPrev == NULL) || (newKey >= pPrev->key)) &&
	((pNext == NULL) || (newKey <= pNext->key)))
	{
	pQPriNode->key = newKey;
	}
    else
	{
	qPriListRemove (pQPriHead, pQPriNode);
	qPriListPut (pQPriHead, pQPriNode, newKey);
	}
    }

/*******************************************************************************
*
* qPriListAdvance - advance a queues concept of time
*
* Priority list queues need not keep track of time because nodes contain time of
* expiration.  So this routine is a NOP.
*
* NOMANUAL
* ARGSUSED
*/

void qPriListAdvance
    (
    Q_PRI_HEAD *pQPriHead
    )
    {
    /* Absolute queue so advancing key of lead node unnecessary */
    }

/*******************************************************************************
*
* qPriListGetExpired - return a time-to-fire expired node
*
* This routine returns a time-to-fire expired node in a priority list timer
* queue.  Expired nodes result from a comparison with the global variable
* vxTicks.  As many nodes may expire on a single advance of vxTicks, this
* routine should be called inside a while loop until NULL is returned.  NULL
* is returned when there are no expired nodes.
*
* RETURNS
*  Pointer to first queue node in queue head, or
*  NULL if queue is empty.
*/

Q_PRI_NODE *qPriListGetExpired
    (
    Q_PRI_HEAD *pQPriHead
    )
    {
    FAST Q_PRI_NODE *pQPriNode = (Q_PRI_NODE *) DLL_FIRST (pQPriHead);

    if ((pQPriNode != NULL) && (pQPriNode->key <= vxTicks))
	return ((Q_PRI_NODE *) dllGet (pQPriHead));
    else
	return (NULL);
    }

/*******************************************************************************
*
* qPriListCalibrate - offset every node in a queue by some delta
*
* This routine offsets every node in a priority list queue by some delta.  The
* offset may either by positive or negative.
*/

void qPriListCalibrate
    (
    Q_PRI_HEAD *pQHead,         /* queue head of queue to calibrate nodes for */
    ULONG       keyDelta        /* offset to add to each node's key */
    )
    {
    FAST Q_PRI_NODE *pQPriNode;

    for (pQPriNode = (Q_PRI_NODE *) DLL_FIRST (pQHead);
         pQPriNode != NULL;
	 pQPriNode = (Q_PRI_NODE *) DLL_NEXT (&pQPriNode->node))
	{
        pQPriNode->key += keyDelta;			/* offset key */
	}
    }

/*******************************************************************************
*
* qPriListKey - return the key of a node
*
* This routine returns the key of a node currently in a priority list queue.
*
* RETURNS
*  Node's key.
*/

ULONG qPriListKey
    (
    Q_PRI_NODE *pQPriNode,
    int         keyType                 /* 0 = normal; 1 = time queue */
    )
    {
    if (keyType == 0)
	return (pQPriNode->key);
    else
	return (pQPriNode->key - vxTicks);
    }

/*******************************************************************************
*
* qPriListInfo - gather information on a priority list queue
*
* This routine fills up to maxNodes elements of a nodeArray with nodes
* currently in a priority list queue.  The actual number of nodes copied to the
* array is returned.  If the nodeArray is NULL, then the number of nodes in
* the priority list queue is returned.
*
* RETURNS
*  Number of node pointers copied into the nodeArray, or
*  Number of nodes in priority list queue if nodeArray is NULL
*/

int qPriListInfo
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
* qPriListEach - call a routine for each node in a queue
*
* This routine calls a user-supplied routine once for each node in the
* queue.  The routine should be declared as follows:
* .CS
*  BOOL routine (pQNode, arg)
*      Q_PRI_NODE *pQNode;	/@ pointer to a queue node          @/
*      int	  arg;		/@ arbitrary user-supplied argument @/
* .CE
* The user-supplied routine should return TRUE if qPriListEach (2) is to
* continue calling it for each entry, or FALSE if it is done and
* qPriListEach can exit.
*
* RETURNS: NULL if traversed whole queue, or pointer to Q_PRI_NODE that
*          qPriListEach stopped on.
*/

Q_PRI_NODE *qPriListEach
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

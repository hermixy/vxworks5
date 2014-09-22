/* qLib.c - queue library */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01m,19jul92,pme  made qRemove return STATUS.
01l,26may92,rrr  the tree shuffle
01k,04dec91,rrr  removed VARARG_OK, no longer needed with ansi c.
01j,19nov91,rrr  shut up some ansi warnings.
01i,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
01h,14aug91,del  changed qCreate/qInit interfaces to take fixed number
		 of args for I960 case only.
01g,18may91,gae  fixed typo in 01f.
01f,18may91,gae  fixed varargs for 960 with conditional VARARG_OK,
		 namely: qCreate() & qInit().
01e,28sep90,jcf  documentation.
01d,10aug90,dnw  changed qCalibrate() to not return value since it is void.
01c,05jul90,jcf  added qCalibrate().
01b,26jun90,jcf  added qResort().
01a,14jun89,jcf	 written.
*/

/*
DESCRIPTION
This queue management library implements the multi-way queue data structures.
Queues which utilize the data structures Q_HEAD, and Q_NODE are shareable
queues.  The routines of this library perform generic operation on the queue
by dereferencing the queue class pointer to invode the correct method for
the operation selected.  All kernel queues are implemented as multi-way queues,
and thus may be configured to utilize any queue that conforms to this library.

SEE ALSO: qFifoLib, qPriBMapLib, qPriDeltaLib, qPriHeapLib, qPriListLib.
*/

#include "vxWorks.h"
#include "errno.h"
#include "stdarg.h"
#include "qClass.h"
#include "qLib.h"
#include "stdlib.h"

#define	MAX_ARGS	10	/* number of variable arguments */

/*******************************************************************************
*
* qCreate - allocate and initialize a multi-way queue
*
* This routine allocates a Q_HEAD structure from the free memory pool.  The
* queue head is initialized to the specified multi-way queue class.  Additional
* arguments are passed to the queue classes underlying initialization routine.
*
* RETURNS: Pointer to a queue head, or NULL if queue could not be created.
*
* ERRNO: S_qLib_Q_CLASS_ID_ERROR
*
*/

Q_HEAD *qCreate
    (
    Q_CLASS *pQClass,   /* pointer to queue class */
    ...              /* optional arguments to create routine */
    )
    {
    va_list pArg;	/* traverses argument list */
    Q_HEAD *pQHead;	/* pointer to queue head */
    int ix;		/* handy index */
    int arg[MAX_ARGS];	/* indigenous variables */

    if (Q_CLASS_VERIFY (pQClass) != OK)
	{
	errno = S_qLib_Q_CLASS_ID_ERROR;
	return (NULL);
	}

    va_start (pArg, pQClass);
    for (ix = 0; ix < MAX_ARGS; ++ix)
	arg[ix] = va_arg (pArg, int);	/* put args in local vars */
    va_end (pArg);

    pQHead = (Q_HEAD *) malloc (sizeof (Q_HEAD));

    if ((pQHead != NULL) && qInit (pQHead, pQClass, arg[0], arg[1], arg[2],
				   arg[3], arg[4], arg[5], arg[6], arg[7],
				   arg[8], arg[9]) != OK)
	{
	/* XXX MAX_ARGS */
	free ((char *) pQHead);
	return (NULL);
	}

    return (pQHead);
    }

/*******************************************************************************
*
* qInit - initialize multi-way queue head
*
* This routine initializes the multi-way queue head to the specified queue
* class.  Additional arguments are passed to the underlying queue class
* initialization routine.
*
* RETURNS OK, or ERROR if initialiazation fails.
*
* ERRNO: S_qLib_Q_CLASS_ID_ERROR
*
*/

STATUS qInit
    (
    Q_HEAD *pQHead,     /* pointer to queue head to initialize */
    Q_CLASS *pQClass,   /* pointer to queue class */
    ...              /* optional arguments to create routine */
    )
    {
    va_list pArg;	/* traverses argument list */
    int ix;		/* handy index */
    int arg[MAX_ARGS];	/* indigenous variables */

    if (Q_CLASS_VERIFY (pQClass) != OK)
	{
	errno = S_qLib_Q_CLASS_ID_ERROR;
	return (ERROR);
	}

    va_start (pArg, pQClass);
    for (ix = 0; ix < MAX_ARGS; ++ix)
	arg[ix] = va_arg (pArg, int);	/* put args in local vars */
    va_end (pArg);

    pQHead->pQClass = pQClass;			/* store pointer to q class */

    return ((* (pQClass->initRtn)) (pQHead, arg[0], arg[1], arg[2], arg[3],
				    arg[4], arg[5], arg[6], arg[7], arg[8],
				    arg[9]));	/* XXX MAX_ARGS */
    }

/*******************************************************************************
*
* qDelete - deallocate a multi-way queue head
*
* This routine deallocates the multi-way queue head.  All queued nodes are
* lost.
*
* RETURNS OK, or ERROR if queue deallocation fails.
*/

STATUS qDelete
    (
    Q_HEAD *pQHead
    )
    {
    if (qTerminate (pQHead) != OK)
	return (ERROR);

    pQHead->pQClass = NULL;			/* invalidate q class */

    free ((char *) pQHead);
    return OK;
    }

/*******************************************************************************
*
* qTerminate - terminate a multi-way queue head
*
* This routine terminates a multi-way queue head.  All queued nodes will be
* lost.
*
* RETURNS OK, or ERROR if termination fails.
*/

STATUS qTerminate
    (
    Q_HEAD *pQHead
    )
    {
    if (((* (pQHead->pQClass->terminateRtn)) (pQHead)) != OK)
	return (ERROR);

    pQHead->pQClass = NULL;			/* invalidate q class */

    return (OK);
    }

/*******************************************************************************
*
* qFirst - return first node in multi-way queue
*
* This routine returns a pointer to the first node in the specified multi-way
* queue head.  If the queue is empty, NULL is returned.
*
* RETURNS Pointer to first queue node in queue head, or NULL if queue is empty.
*/

Q_NODE *qFirst
    (
    Q_HEAD *pQHead
    )
    {
    return (Q_FIRST (pQHead));
    }

/*******************************************************************************
*
* qPut - insert a node into a multi-way queue
*
* This routine inserts a node into a multi-way queue.  The insertion is based
* on the key and the underlying queue class.
*/

void qPut
    (
    Q_HEAD *pQHead,
    Q_NODE *pQNode,
    ULONG   key
    )
    {
    Q_PUT (pQHead, pQNode, key);
    }

/*******************************************************************************
*
* qGet - remove and return first node in multi-way queue
*
* This routine removes and returns the first node in a multi-way queue.  If
* the queue is empty, NULL is returned.
*
* RETURNS Pointer to first queue node in queue head, or NULL if queue is empty.
*/

Q_NODE *qGet
    (
    Q_HEAD *pQHead
    )
    {
    return (Q_GET (pQHead));
    }

/*******************************************************************************
*
* qRemove - remove a node from a multi-way queue
*
* This routine removes a node from the specified multi-way queue.
*/

STATUS qRemove
    (
    Q_HEAD *pQHead,
    Q_NODE *pQNode
    )
    {
    return (Q_REMOVE (pQHead, pQNode));
    }

/*******************************************************************************
*
* qResort - resort a node to a new position based on a new key
*
* This routine resorts a node to a new position based on a new key.  It can
* be used to change the priority of a queued element, for instance.
*/

void qResort
    (
    Q_HEAD *pQHead,
    Q_NODE *pQNode,
    ULONG   newKey
    )
    {
    Q_RESORT (pQHead, pQNode, newKey);
    }

/*******************************************************************************
*
* qAdvance - advance a queues concept of time (timer queues only)
*
* Multi-way queues that keep nodes prioritized by time-to-fire utilize this
* routine to advance time.  It is usually called from within a clock-tick
* interrupt service routine.
*/

void qAdvance
    (
    Q_HEAD *pQHead
    )
    {
    Q_ADVANCE (pQHead);
    }

/*******************************************************************************
*
* qGetExpired - return a time-to-fire expired node
*
* This routine returns a time-to-fire expired node in a multi-way timer queue.
* Expired nodes result from a qAdvance(2) advancing a node beyond its delay.
* As many nodes may expire on a single qAdvance(2), this routine should be
* called inside a while loop until NULL is returned.  NULL is returned when
* there are no expired nodes.
*
* RETURNS: Pointer to first queue node in queue head, or NULL if queue is empty.
*/

Q_NODE *qGetExpired
    (
    Q_HEAD *pQHead
    )
    {
    return (Q_GET_EXPIRED (pQHead));
    }

/*******************************************************************************
*
* qKey - return the key of a node
*
* This routine returns the key of a node currently in a multi-way queue.  The
* keyType determines key style on certain queue classes.
*
* RETURNS: Node's key.
*/

ULONG qKey
    (
    Q_HEAD *pQHead,
    Q_NODE *pQNode,
    int     keyType
    )
    {
    return (Q_KEY (pQHead, pQNode, keyType));
    }

/*******************************************************************************
*
* qCalibrate - offset every node in a queue by some delta
*
* This routine offsets every node in a multi-way queue by some delta.  The
* offset may either by positive or negative.
*/

void qCalibrate
    (
    Q_HEAD *pQHead,
    ULONG   keyDelta
    )
    {
    Q_CALIBRATE (pQHead, keyDelta);
    }

/*******************************************************************************
*
* qInfo - gather information on a multi-way queue
*
* This routine fills up to maxNodes elements of a nodeArray with nodes
* currently in a multi-way queue.  The actual number of nodes copied to the
* array is returned.  If the nodeArray is NULL, then the number of nodes in
* the multi-way queue is returned.
*
* RETURNS: Number of node pointers copied to the nodeArray, or nodes in
*	   multi-way queue if nodeArray is NULL.
*/

int qInfo
    (
    Q_HEAD *pQHead,
    Q_NODE *nodeArray[],
    int    maxNodes
    )
    {
    return (Q_INFO (pQHead, nodeArray, maxNodes));
    }

/*******************************************************************************
*
* qEach - call a routine for each node in a queue
*
* This routine calls a user-supplied routine once for each node in the
* queue.  The routine should be declared as follows:
* .CS
*  BOOL routine (pQNode, arg)
*      Q_NODE	*pQNode;	/@ pointer to a queue node          @/
*      int	arg;		/@ arbitrary user-supplied argument @/
* .CE
* The user-supplied routine should return TRUE if qEach() is to continue
* calling it for each entry, or FALSE if it is done and qEach can exit.
*
* RETURNS: NULL if traversed whole queue, or pointer to Q_NODE that
*          qEach ended with.
*/

Q_NODE *qEach
    (
    Q_HEAD      *pQHead,        /* queue head of queue to call routine for */
    FUNCPTR     routine,        /* the routine to call for each queue node */
    int         routineArg      /* arbitrary user-supplied argument */
    )
    {
    return (Q_EACH (pQHead, routine, routineArg));
    }

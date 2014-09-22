/* qPriHeapLib.c - heap priority queue management library */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01p,11nov01,dee  Add COLDFIRE support
01o,04sep98,cdp  make ARM CPUs with ARM_THUMB==TRUE use portable routines.
01n,22apr97,jpd  Added ARM to non-portable list.
01m,19mar95,dvs  removed tron references.
01l,19jul92,pme  made qPriHeapRemove return STATUS.
01k,18jul92,smb  Changed errno.h to errnoLib.h.
01j,26may92,rrr  the tree shuffle
01i,19nov91,rrr  shut up some ansi warnings.
01h,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01g,23jul91,hdn  added conditional macro for optimized TRON codes.
01f,24may91,wmd  added predeclarations to avoid compiler errors.
01e,28sep90,jcf	 documentation.
01d,05jul90,jcf	 added qPriHeapCalibrate().
01c,26jun90,jcf	 add qPriHeapResort().
01b,10may90,jcf	 fixed PORTABLE definition.
01a,14jun89,jcf	 written.
*/

/*
DESCRIPTION
This library contains routines to manage a priority queue.  The queue is
maintained in priority order in a binary tree.  This queue performs a
qPriHeapPut() operation preportional in time with log base 2 of the number of
nodes in the queue.  This queue is used for timer queues.  The only restriction
is that the heap can only handle a static number of nodes as specified at
creation time.

This queue complies with the multi-way queue data structures and thus may be
utilized by any multi-way queue.  The priority heap multi-way queue
class is accessed by the global id qPriHeapClassId.

SEE ALSO: qLib()
*/

#include "vxWorks.h"
#include "qClass.h"
#include "qPriHeapLib.h"
#include "stdlib.h"
#include "string.h"
#include "errnoLib.h"
#include "stdio.h"

/* XXX should break out for each architecture */

/* optimized version available for 680X0 and ARM */

#if (defined(PORTABLE) || \
     ((CPU_FAMILY != MC680X0) && (CPU_FAMILY != ARM) && (CPU_FAMILY != COLDFIRE)) || \
     ((CPU_FAMILY == ARM) && ARM_THUMB))
#define qPriHeapLib_PORTABLE
#endif	/* (defined(PORTABLE) || (CPU_FAMILY != MC680X0)) */

/* imports */

IMPORT ULONG vxTicks;		/* current time in ticks */

/* globals */

LOCAL Q_CLASS qPriHeapClass =
    {
    (FUNCPTR)qPriHeapCreate,
    (FUNCPTR)qPriHeapInit,
    (FUNCPTR)qPriHeapDelete,
    (FUNCPTR)qPriHeapTerminate,
    (FUNCPTR)qPriHeapPut,
    (FUNCPTR)qPriHeapGet,
    (FUNCPTR)qPriHeapRemove,
    (FUNCPTR)qPriHeapResort,
    (FUNCPTR)qPriHeapAdvance,
    (FUNCPTR)qPriHeapGetExpired,
    (FUNCPTR)qPriHeapKey,
    (FUNCPTR)qPriHeapCalibrate,
    (FUNCPTR)qPriHeapInfo,
    (FUNCPTR)qPriHeapEach,
    &qPriHeapClass
    };

Q_CLASS_ID qPriHeapClassId = &qPriHeapClass;


/* forward static functions */

#ifdef qPriHeapLib_PORTABLE
static void qPriHeapUp (Q_PRI_HEAP_HEAD *pQPriHeapHead, int index);
static void qPriHeapDown (Q_PRI_HEAP_HEAD *pQPriHeapHead, int index);
#else
extern void qPriHeapUp (Q_PRI_HEAP_HEAD *pQPriHeapHead, int index);
extern void qPriHeapDown (Q_PRI_HEAP_HEAD *pQPriHeapHead, int index);
#endif


/******************************************************************************
*
* qPriHeapArrayCreate - create and initialized a heap priority queue
*
* Create a heap priority queue.  Initialize the specified queue header.
*
* RETURNS: OK or ERROR if not enough memory to create queue.
*
* SEE ALSO: qPriHeapInit (2)
*/

HEAP_ARRAY *qPriHeapArrayCreate
    (
    int heapSize
    )
    {
    return ((HEAP_ARRAY *) malloc ((unsigned) 4 * heapSize));
    }

/******************************************************************************
*
* qPriHeapArrayDelete - deallocate a heap array
*
* This routine returns an allocated HEAP_ARRAY structure to the free memory
* pool.
*
* RETURNS:
*  OK, or
*  ERROR if could not deallocate heap array.
*/

STATUS qPriHeapArrayDelete
    (
    HEAP_ARRAY *pHeapArray
    )
    {
    free ((char *) pHeapArray);
    return OK;
    }

/******************************************************************************
*
* qPriHeapCreate - create and initialized a heap priority queue
*
* Create a heap priority queue.  Initialize the specified queue header.
*
* RETURNS: OK or ERROR if not enough memory to create queue.
*
* SEE ALSO: qPriHeapInit (2)
*/

Q_PRI_HEAP_HEAD *qPriHeapCreate
    (
    HEAP_ARRAY *pHeapArray
    )
    {
    Q_PRI_HEAP_HEAD *pQPriHeapHead = (Q_PRI_HEAP_HEAD *)
				     malloc (sizeof (Q_PRI_HEAP_HEAD));

    if (pQPriHeapHead == NULL)
	return (NULL);

    if (qPriHeapInit (pQPriHeapHead, pHeapArray) != OK)
	{
	free ((char *)pQPriHeapHead);
	return (NULL);
	}

    return (pQPriHeapHead);
    }

/******************************************************************************
*
* qPriHeapInit - initialize a heap priority queue
*
* Initialize the heap priority queue pointed to by the specified queue
* header.
*
* RETURNS:  OK, or ERROR if heap priority queue could not be initialized.
*
* ERRNO: S_qPriHeapLib_NULL_HEAP_ARRAY
*
*/

STATUS qPriHeapInit
    (
    Q_PRI_HEAP_HEAD *pQPriHeapHead,
    HEAP_ARRAY      *pHeapArray
    )
    {
    if (pHeapArray == NULL)
	{
	errnoSet (S_qPriHeapLib_NULL_HEAP_ARRAY);
	return (ERROR);
	}

    pQPriHeapHead->pHeapArray	= pHeapArray;	/* store bmap list pointer */
    pQPriHeapHead->pHighNode	= NULL;		/* zero the highest node */
    pQPriHeapHead->heapIndex	= 0;		/* initialize the heap index */

    return (OK);
    }

/******************************************************************************
*
* qPriHeapDelete - delete a priority heap queue
*
* This routine deallocates memory associated with the queue.  All queued
* nodes are lost.
*
* RETURNS:
*  OK, or
*  ERROR if memory cannot be deallocated.
*/

STATUS qPriHeapDelete
    (
    Q_PRI_HEAP_HEAD *pQPriHeapHead
    )
    {
    free ((char *) pQPriHeapHead);
    return OK;
    }

/******************************************************************************
*
* qPriHeapTerminate - terminate a heap priority queue
*
* This routine terminates a heap priority queue.  All queued nodes will be lost.
*
* ARGSUSED
*/

STATUS qPriHeapTerminate
    (
    Q_PRI_HEAP_HEAD *pQPriHeapHead
    )
    {
    return (OK);
    }

/*******************************************************************************
*
* qPriHeapPut - insert a node into a heap priority queue
*
* This routine inserts a node into a heap priority queue.  The insertion is
* based on the priority key.  The lower the key the higher the priority.
*/

void qPriHeapPut
    (
    Q_PRI_HEAP_HEAD     *pQPriHeapHead,
    Q_PRI_HEAP_NODE     *pQPriHeapNode,
    ULONG                key
    )
    {
    int index = pQPriHeapHead->heapIndex++;

    pQPriHeapNode->key = key;
    (*pQPriHeapHead->pHeapArray)[index] = pQPriHeapNode;

    qPriHeapUp (pQPriHeapHead, index);
    }

/*******************************************************************************
*
* qPriHeapGet - remove and return first node in heap priority queue
*
* This routine removes and returns the first node in a heap priority queue.  If
* the queue is empty, NULL is returned.
*
* RETURNS
*  Pointer to first queue node in queue head, or
*  NULL if queue is empty.
*/

Q_PRI_HEAP_NODE *qPriHeapGet
    (
    Q_PRI_HEAP_HEAD *pQPriHeapHead
    )
    {
    FAST Q_PRI_HEAP_NODE **heapArray = *pQPriHeapHead->pHeapArray;
    Q_PRI_HEAP_NODE *pQPriHeapNode = pQPriHeapHead->pHighNode;

    if (pQPriHeapHead->heapIndex == 0)
	return (NULL);
    else if (pQPriHeapHead->heapIndex-- == 1)
	pQPriHeapHead->pHighNode = NULL;
    else
	{
	heapArray [0] = heapArray [pQPriHeapHead->heapIndex];
	qPriHeapDown (pQPriHeapHead, 0);
	}

    return (pQPriHeapNode);
    }

/*******************************************************************************
*
* qPriHeapRemove - remove a node from a heap priority queue
*
* This routine removes a node from the specified heap priority queue.
*/

STATUS qPriHeapRemove
    (
    Q_PRI_HEAP_HEAD *pQPriHeapHead,
    Q_PRI_HEAP_NODE *pQPriHeapNode
    )
    {
    FAST int index = pQPriHeapNode->index;
    FAST Q_PRI_HEAP_NODE **heapArray = *pQPriHeapHead->pHeapArray;
    int heapIndex = pQPriHeapHead->heapIndex;

    if (--pQPriHeapHead->heapIndex == 0)
	pQPriHeapHead->pHighNode = NULL;
    else
	{
	heapArray [index] = heapArray [heapIndex - 1];

	if ((index > 0) &&
	    (heapArray [(index - 1) / 2]->key > heapArray [index]->key))
	    qPriHeapUp (pQPriHeapHead, index);
	else
	    qPriHeapDown (pQPriHeapHead, index);
	}

    return (OK);
    }

/*******************************************************************************
*
* qPriHeapResort - resort a node to a new position based on a new key
*
* This routine resorts a node to a new position based on a new key.
*/

void qPriHeapResort
    (
    Q_PRI_HEAP_HEAD *pQPriHeapHead,
    Q_PRI_HEAP_NODE *pQPriHeapNode,
    ULONG            newKey
    )
    {
    FAST int index = pQPriHeapNode->index;
    FAST Q_PRI_HEAP_NODE **heapArray = *pQPriHeapHead->pHeapArray;

    pQPriHeapNode->key = newKey;

    if ((index > 0) &&
	(heapArray [(index - 1) / 2]->key > heapArray [index]->key))
	qPriHeapUp (pQPriHeapHead, index);
    else
	qPriHeapDown (pQPriHeapHead, index);
    }

/*******************************************************************************
*
* qPriHeapAdvance - advance a queues concept of time
*
* Heap queues need not keep track of time because nodes contain time of
* expiration.  So this routine is a NOP.
*
* NOMANUAL
* ARGSUSED
*/

void qPriHeapAdvance
    (
    Q_PRI_HEAP_HEAD *pQPriHeapHead
    )
    {
    /* Absolute queue so advancing key of lead node unnecessary */
    }

/*******************************************************************************
*
* qPriHeapGetExpired - return a time-to-fire expired node
*
* This routine returns a time-to-fire expired node in a heap priority timer
* queue.  Expired nodes result from a comparison with the global variable
* vxTicks.  As many nodes may expire on a single advance of vxTicks, this
* routine should be called inside a while loop until NULL is returned.  NULL
* is returned when there are no expired nodes.
*
* RETURNS
*  Pointer to first queue node in queue head, or
*  NULL if queue is empty.
*/

Q_PRI_HEAP_NODE *qPriHeapGetExpired
    (
    Q_PRI_HEAP_HEAD *pQPriHeapHead
    )
    {
    Q_PRI_HEAP_NODE *pQPriHeapNode = pQPriHeapHead->pHighNode;

    if ((pQPriHeapNode != NULL) && (pQPriHeapNode->key <= vxTicks))
	return (qPriHeapGet (pQPriHeapHead));
    else
	return ((Q_PRI_HEAP_NODE *) NULL);

    }

/*******************************************************************************
*
* qPriHeapKey - return the key of a node
*
* This routine returns the key of a node currently in a heap priority queue.
* The keyType determines key style.  A normal key style returns the nodes
* internal key.  A timer queue key type style returns the key as the
* time-to-fire.
*
* RETURNS
*  Node's key, or
*  node's time-to-fire.
*/

ULONG qPriHeapKey
    (
    Q_PRI_HEAP_NODE *pQPriHeapNode,
    int              keyType            /* 0 = normal; 1 = time queue */
    )
    {
    if (keyType == 0)
	return (pQPriHeapNode->key);
    else
	return (pQPriHeapNode->key - vxTicks);
    }

/*******************************************************************************
*
* qPriHeapCalibrate - offset every node in a queue by some delta
*
* This routine offsets every node in a heap priority queue by some delta.  The
* offset may either by positive or negative.
*/

void qPriHeapCalibrate
    (
    Q_PRI_HEAP_HEAD *pQPriHeapHead,     /* queue to calibrate nodes for */
    ULONG            keyDelta           /* offset to add to each node's key */
    )
    {
    FAST int ix;
    FAST Q_PRI_HEAP_NODE **heapArray = *pQPriHeapHead->pHeapArray;

    for (ix = 0; ix < pQPriHeapHead->heapIndex; ix ++)
	 heapArray[ix]->key += keyDelta;
    }

/*******************************************************************************
*
* qPriHeapInfo - gather information on a priority heap queue
*
* This routine fills up to maxNodes elements of a nodeArray with nodes
* currently in a priority heap queue.  The actual number of nodes copied to the
* array is returned.  If the nodeArray is NULL, then the number of nodes in
* the priority heap queue is returned.
*
* RETURNS
*  Number of node pointers copied into the nodeArray, or
*  Number of nodes in multi-way queue if nodeArray is NULL
*/

int qPriHeapInfo
    (
    Q_PRI_HEAP_HEAD *pQPriHeapHead,     /* heap queue to gather list for */
    FAST int nodeArray[],               /* array of node pointers for filling */
    FAST int maxNodes                   /* max node pointers for nodeArray */
    )
    {
    int numNodes = min (maxNodes, pQPriHeapHead->heapIndex);

    if (nodeArray == NULL)		/* NULL node array means return count */
	return (pQPriHeapHead->heapIndex);

    bcopy ((char *)*pQPriHeapHead->pHeapArray, (char *)nodeArray, numNodes * 4);

    return (numNodes);
    }

/*******************************************************************************
*
* qPriHeapEach - call a routine for each node in a queue
*
* This routine calls a user-supplied routine once for each node in the
* queue.  The routine should be declared as follows:
* .CS
*  BOOL routine (pQNode, arg)
*      Q_PRI_HEAP_NODE	*pQNode;	/@ pointer to a queue node          @/
*      int		arg;		/@ arbitrary user-supplied argument @/
* .CE
* The user-supplied routine should return TRUE if qPriHeapEach (2) is to
* continue calling it for each entry, or FALSE if it is done and
* qPriHeapEach can exit.
*
* RETURNS: NULL if traversed whole queue, or pointer to Q_PRI_HEAP_NODE that
*          qPriHeapEach stopped on.
*/

Q_PRI_HEAP_NODE *qPriHeapEach
    (
    Q_PRI_HEAP_HEAD *pQHead,    /* queue head of queue to call routine for */
    FUNCPTR          routine,   /* the routine to call for each table entry */
    int              routineArg /* arbitrary user-supplied argument */
    )
    {
    FAST int ix;

    for (ix = 0;
	 (ix < pQHead->heapIndex) &&
	 ((* routine) ((*pQHead->pHeapArray)[ix], routineArg));
	 ix ++)
	;

    if (ix < pQHead->heapIndex)
	return ((*pQHead->pHeapArray)[ix]);	/* return node we ended with */
    else
	return ((Q_PRI_HEAP_NODE *) NULL);	/* did all nodes */
    }


#ifdef qPriHeapLib_PORTABLE

/*******************************************************************************
*
* qPriHeapUp - elevate a node to its proper place in the heap tree
*
* This routine elevates a node to its proper place in the heap tree.
*/

LOCAL void qPriHeapUp
    (
    Q_PRI_HEAP_HEAD *pQPriHeapHead,
    int index
    )
    {
    int workIx   = index;
    int parentIx = (workIx - 1) / 2;
    Q_PRI_HEAP_NODE **heapArray = *pQPriHeapHead->pHeapArray;
    Q_PRI_HEAP_NODE *workNode = heapArray [workIx];

    while ((workIx > 0) && (heapArray [parentIx]->key > workNode->key))
	{
	heapArray [workIx] = heapArray [parentIx];
	workIx = parentIx;
	parentIx = (workIx - 1) / 2;
	}

    heapArray [workIx] = workNode;
    pQPriHeapHead->pHighNode = heapArray [0];
    }

/*******************************************************************************
*
* qPriHeapDown - move a node down to its proper place in the heap tree
*
* This routine moves a node down to its proper place in the heap tree.
*/

LOCAL void qPriHeapDown
    (
    Q_PRI_HEAP_HEAD *pQPriHeapHead,
    int index
    )
    {
    int workIx   = index;
    int lesserChildIx;
    int leftChildIx = 2 * workIx + 1;
    int rightChildIx = leftChildIx + 1;
    Q_PRI_HEAP_NODE **heapArray = *pQPriHeapHead->pHeapArray;
    Q_PRI_HEAP_NODE *workNode = heapArray [workIx];

    while (leftChildIx < pQPriHeapHead->heapIndex)
	{
	if ((rightChildIx >= pQPriHeapHead->heapIndex) ||
	    (heapArray [leftChildIx]->key < heapArray [rightChildIx]->key))
	    lesserChildIx = leftChildIx;
	else
	    lesserChildIx = rightChildIx;

	if (heapArray [lesserChildIx]->key < workNode->key)
	    {
	    heapArray [workIx] = heapArray [lesserChildIx];
	    workIx = lesserChildIx;
	    }
	else
	    break;

	leftChildIx  = 2 * workIx + 1;
	rightChildIx = 2 * workIx + 2;
	}

    heapArray [workIx] = workNode;
    pQPriHeapHead->pHighNode = heapArray [0];
    }

#endif	/* qPriHeapLib_PORTABLE */

/*******************************************************************************
*
* qPriHeapShow - dump the heap in human readable form by key or node
*
* This routine prints a humun readable representation of the heap to standard
* out.  The two output formats are selected as: 0 node format, 1 key format.
*
* CAVEATS
* The output is only printed for the first 16 nodes, because beyond this the
* output is unintelligible.
*/

void qPriHeapShow
    (
    Q_PRI_HEAP_HEAD *pHeap,     /* pointer to heap head to dump */
    int format                  /* 0 - node format; 1 - key format */
    )
    {
    int ix;
    char gap[100];
    char halfgap[100];
    char *space = "                                               ";
    int nodesPerLine = 1;
    int endOfLine = 0;
    int fmtlen = 3;
    int limit = 15;

    if (pHeap->heapIndex > 0)
	printf ("First: %x\n", pHeap->pHighNode);
    else
	printf ("First: NULL\n");

    if (format > 0)
	{
	limit = 7;
	fmtlen = 8;
	printf ("%24s"," ");
	}
    else
	printf ("%29s"," ");

    for (ix = 0; ix < min (pHeap->heapIndex, limit); ix ++)
	{
	if (ix == endOfLine)
	    {
	    strncpy (gap, space, 32 / nodesPerLine);
	    gap [(32 / nodesPerLine) - fmtlen] = EOS;
	    strncpy (halfgap, space, 16 / nodesPerLine);
	    halfgap [(16 / nodesPerLine) - fmtlen] = EOS;
	    nodesPerLine = nodesPerLine << 1;
	    endOfLine += nodesPerLine;
	    if (format > 0)
		printf ("%8x\n%s", (*pHeap->pHeapArray)[ix], halfgap);
	    else
		printf ("%3d\n%s", (*pHeap->pHeapArray)[ix]->key, halfgap);
	    }
	else
	    if (format > 0)
		printf ("%8x%s", (*pHeap->pHeapArray)[ix], gap);
	    else
		printf ("%3d%s", (*pHeap->pHeapArray)[ix]->key, gap);
	}

    if (ix == limit)
	printf ("\nTerminated at %d nodes because output gets ugly.\n", limit);
    else
	printf ("\n");
    }

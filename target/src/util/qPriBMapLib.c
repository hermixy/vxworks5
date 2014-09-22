/* qPriBMapLib.c - bit mapped priority queue management library */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02o,16nov98,cdp  make ARM CPUs with ARM_THUMB==TRUE use portable routines.
02m,06jan98,cym  added simnt support
02n,22apr97,jpd  removed ARM from PORTABLE list.
02m,29jan97,elp  added ARM support
02l,12jul95,ism  added simsolaris support
02l,04nov94,yao  added PPC support.
02k,11aug93,gae  vxsim hppa from rrr.
02j,12jun93,rrr  vxsim.
02i,27jul92,jcf  all architectures now utilize this library.
                 added nPriority selection for memory conservation.
02h,19jul92,pme  made qPriBMapRemove return STATUS.
02g,18jul92,smb  Changed errno.h to errnoLib.h
02f,26may92,rrr  the tree shuffle
02e,22apr92,jwt  converted CPU==SPARC to CPU_FAMILY==SPARC; copyright.
02d,19nov91,rrr  shut up some ansi warnings.
02c,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -fixed #else and #endif
		  -changed TINY and UTINY to INT8 and UINT8
		  -changed VOID to void
		  -changed copyright notice
02b,25mar91,del  added I960 to portable checklist.
02a,10jan91,jcf  changed BMAP_LIST for portability to other architectures.
01e,20dec90,gae  added declaration of qPriBMapSet and qPriBMapClear.
01d,28sep90,jcf	 documentation.
01c,05jul90,jcf	 added null routine for calibrateRtn field in Q_CLASS.
01b,10may90,jcf	 fixed PORTABLE definition.
		 changed ffs () to ffsMsb ().
01a,14jun89,jcf	 written.
*/

/*
DESCRIPTION
This library contains routines to manage a priority queue.  The queue is
maintained in priority order with no sorting time, so its performance is
constant.  Its restrictions are that it requires a 2K byte bit map, and it
can only prioritize nodes with keys in the range 0 to 255.

This queue complies with the multi-way queue data structures and thus may be
utilized by any multi-way queue.  The priority bit mapped multi-way queue
class is accessed by the global id qPriBMapClassId.

SEE ALSO: qLib()
*/

#include "vxWorks.h"
#include "qClass.h"
#include "qPriBMapLib.h"
#include "memLib.h"
#include "errnoLib.h"
#include "string.h"
#include "stdlib.h"


#if (defined(PORTABLE) || \
     (CPU_FAMILY == SIMNT) || (CPU_FAMILY == SPARC) || (CPU_FAMILY == I960) || \
     (CPU_FAMILY == SIMSPARCSUNOS) || (CPU_FAMILY == SIMHPPA) || \
     (CPU_FAMILY == SIMSPARCSOLARIS) || ((CPU_FAMILY == ARM) && ARM_THUMB))
#define qPriBMapLib_PORTABLE
#endif

#if     (CPU_FAMILY == PPC)
#define qPriBMapLib_PORTABLE
#endif  /* (CPU_FAMILY == PPC) */

#if	defined(PORTABLE)
#define qPriBMapLib_PORTABLE
#endif	/* PORTABLE */


/* forward static functions */

static STATUS qPriBMapNullRtn (void);

#ifdef qPriBMapLib_PORTABLE

static void qPriBMapSet (BMAP_LIST *pBMapList, int priority);
static void qPriBMapClear (BMAP_LIST *pBMapList, int priority);
static int qPriBMapHigh (BMAP_LIST *pBMapList);

#endif	/* qPriBMapLib_PORTABLE */


/* locals */

LOCAL Q_CLASS qPriBMapClass =
    {
    (FUNCPTR)qPriBMapCreate,
    (FUNCPTR)qPriBMapInit,
    (FUNCPTR)qPriBMapDelete,
    (FUNCPTR)qPriBMapNullRtn,
    (FUNCPTR)qPriBMapPut,
    (FUNCPTR)qPriBMapGet,
    (FUNCPTR)qPriBMapRemove,
    (FUNCPTR)qPriBMapResort,
    (FUNCPTR)qPriBMapNullRtn,
    (FUNCPTR)qPriBMapNullRtn,
    (FUNCPTR)qPriBMapKey,
    (FUNCPTR)qPriBMapNullRtn,
    (FUNCPTR)qPriBMapInfo,
    (FUNCPTR)qPriBMapEach,
    &qPriBMapClass
    };

/* globals */

Q_CLASS_ID qPriBMapClassId = &qPriBMapClass;


/******************************************************************************
*
* qPriBMapListCreate - create and initialized a bit mapped priority queue
*
* Create a bit mapped priority queue.  Initialize the specified queue header.
*
* RETURNS: OK or ERROR if not enough memory to create queue.
*
* SEE ALSO: qPriBMapInit().
*/

BMAP_LIST *qPriBMapListCreate 
    (
    UINT nPriority	/* 1 priority to 256 priorities */
    )
    {
    UINT size;

    if ((nPriority < 1) || (nPriority > 256))
	return (NULL);
    
    size = sizeof (BMAP_LIST) - (sizeof (DL_LIST) * (256 - nPriority));

    return ((BMAP_LIST *) malloc (size));
    }

/******************************************************************************
*
* qPriBMapListDelete - deallocate a bit mapped list
*
* This routine returns an allocated BMAP_LIST to the free memory pool.
*
* RETURNS: OK, or ERROR if bit mapped list could not be deallocated.
*/

STATUS qPriBMapListDelete
    (
    BMAP_LIST *pBMapList
    )
    {
    free ((char *)pBMapList);

    return (OK);
    }

/******************************************************************************
*
* qPriBMapCreate - create and initialized a bit mapped priority queue
*
* Create a bit mapped priority queue.  Initialize the specified queue header.
*
* RETURNS: OK, or ERROR if not enough memory to create queue.
*
* SEE ALSO: qPriBMapInit()
*/

Q_PRI_BMAP_HEAD *qPriBMapCreate
    (
    BMAP_LIST *	pBMapList,
    UINT	nPriority	/* 1 priority to 256 priorities */
    )
    {
    Q_PRI_BMAP_HEAD *pQPriBMapHead;

    if ((nPriority < 1) || (nPriority > 256))
	return (NULL);

    pQPriBMapHead = (Q_PRI_BMAP_HEAD *) malloc (sizeof (Q_PRI_BMAP_HEAD));

    if (pQPriBMapHead == NULL)
	return (NULL);

    if (qPriBMapInit (pQPriBMapHead, pBMapList, nPriority) != OK)
	{
	free ((char *)pQPriBMapHead);
	return (NULL);
	}

    return (pQPriBMapHead);
    }

/******************************************************************************
*
* qPriBMapInit - initialize a bit mapped priority queue
*
* Initialize the bit mapped priority queue pointed to by the specified queue
* header.
*
* RETURNS: OK or ERROR
*
* ERRNO: S_qPriBMapLib_NULL_BMAP_LIST
*
*/

STATUS qPriBMapInit
    (
    Q_PRI_BMAP_HEAD *	pQPriBMapHead,
    BMAP_LIST *		pBMapList,
    UINT		nPriority	/* 1 priority to 256 priorities */
    )
    {
    FAST int ix;

    if ((nPriority < 1) || (nPriority > 256))
	return (ERROR);

    if (pBMapList == NULL)
	{
	errnoSet (S_qPriBMapLib_NULL_BMAP_LIST);
	return (ERROR);
	}

    pQPriBMapHead->pBMapList = pBMapList;	/* store bmap list pointer */

    /* initialize the q */

    for (ix = 0; ix < nPriority; ++ix)
	dllInit (&pBMapList->listArray[ix]);

    pQPriBMapHead->highNode	= NULL;		/* zero the highest node */
    pQPriBMapHead->nPriority	= nPriority;	/* higest legal priority */

    /* zero the bit maps */

    pBMapList->metaBMap = 0;
    bzero ((char *) pBMapList->bMap, sizeof (pBMapList->bMap));

    return (OK);
    }

/******************************************************************************
*
* qPriBMapDelete - deallocate a bit mapped queue head
*
* This routine deallocates a bit mapped queue head.  All queued nodes will
* be lost.
*
* RETURNS: OK, or ERROR in bit mapped queue head could not be deallocated.
*/

STATUS qPriBMapDelete
    (
    Q_PRI_BMAP_HEAD *pQPriBMapHead
    )
    {
    free ((char *)pQPriBMapHead);

    return (OK);
    }

#ifdef qPriBMapLib_PORTABLE

/*******************************************************************************
*
* qPriBMapPut - insert a node into a priority bit mapped queue
*
* This routine inserts a node into a priority bit mapped queue.  The insertion
* is based on the specified priority key which is constrained to the range
* 0 to 255.  The highest priority is zero.
*/

void qPriBMapPut
    (
    Q_PRI_BMAP_HEAD     *pQPriBMapHead,
    Q_PRI_NODE          *pQPriNode,
    ULONG                key
    )
    {
    pQPriNode->key = key;

    if ((pQPriBMapHead->highNode == NULL) ||
        (key < pQPriBMapHead->highNode->key))
	{
	pQPriBMapHead->highNode = pQPriNode;
	}

    qPriBMapSet (pQPriBMapHead->pBMapList, key);

    dllAdd (&pQPriBMapHead->pBMapList->listArray[key], &pQPriNode->node);
    }

/*******************************************************************************
*
* qPriBMapGet - remove and return first node in priority bit-mapped queue
*
* This routine removes and returns the first node in a priority bit-mapped
* queue.  If the queue is empty, NULL is returned.
*
* RETURNS Pointer to first queue node in queue head, or NULL if queue is empty.
*/

Q_PRI_NODE *qPriBMapGet
    (
    Q_PRI_BMAP_HEAD *pQPriBMapHead
    )
    {
    Q_PRI_NODE *pQPriNode = pQPriBMapHead->highNode;

    if (pQPriNode != NULL)
	qPriBMapRemove (pQPriBMapHead, pQPriNode);

    return (pQPriNode);
    }

/*******************************************************************************
*
* qPriBMapRemove - remove a node from a priority bit mapped queue
*
* This routine removes a node from the specified bit mapped queue.
*/

STATUS qPriBMapRemove
    (
    Q_PRI_BMAP_HEAD *pQPriBMapHead,
    Q_PRI_NODE *pQPriNode
    )
    {
    dllRemove (&pQPriBMapHead->pBMapList->listArray[pQPriNode->key],
	       &pQPriNode->node);

    if (DLL_EMPTY (&pQPriBMapHead->pBMapList->listArray[pQPriNode->key]))
        {
	qPriBMapClear (pQPriBMapHead->pBMapList, pQPriNode->key);

	if (pQPriNode == pQPriBMapHead->highNode)
	    pQPriBMapHead->highNode =
	      (Q_PRI_NODE *) DLL_FIRST(&pQPriBMapHead->pBMapList->
	      listArray[qPriBMapHigh(pQPriBMapHead->pBMapList)]);
	}
    else if (pQPriNode == pQPriBMapHead->highNode)
	pQPriBMapHead->highNode =
	  (Q_PRI_NODE *) DLL_FIRST (&pQPriBMapHead->pBMapList->
	  listArray[pQPriBMapHead->highNode->key]);

    return (OK);
    }

#endif	/* qPriBMapLib_PORTABLE */

/*******************************************************************************
*
* qPriBMapResort - resort a node to a new position based on a new key
*
* This routine resorts a node to a new position based on a new priority key.
*/

void qPriBMapResort
    (
    Q_PRI_BMAP_HEAD *pQPriBMapHead,
    Q_PRI_NODE      *pQPriNode,
    ULONG            newKey
    )
    {
    if (pQPriNode->key != newKey)
	{
	qPriBMapRemove (pQPriBMapHead, pQPriNode);
	qPriBMapPut (pQPriBMapHead, pQPriNode, newKey);
	}
    }

/*******************************************************************************
*
* qPriBMapKey - return the key of a node
*
* This routine returns the key of a node currently in a multi-way queue.  The
* keyType is ignored.
*
* RETURNS: Node's key.
*
* ARGSUSED
*/

ULONG qPriBMapKey
    (
    Q_PRI_NODE  *pQPriNode      /* node to get key for */
    )
    {
    return (pQPriNode->key);	/* return key */
    }

/*******************************************************************************
*
* qPriBMapInfo - gather information on a bit mapped queue
*
* This routine fills up to maxNodes elements of a nodeArray with nodes
* currently in a multi-way queue.  The actual number of nodes copied to the
* array is returned.  If the nodeArray is NULL, then the number of nodes in
* the multi-way queue is returned.
*
* RETURNS: Number of node pointers copied into the nodeArray, or number of
*	   nodes in bit mapped queue if nodeArray is NULL
*/

int qPriBMapInfo
    (
    Q_PRI_BMAP_HEAD *pQPriBMapHead,     /* bmap q to gather list for */
    FAST int nodeArray[],               /* array of node pointers for filling */
    FAST int maxNodes                   /* max node pointers for nodeArray */
    )
    {
    FAST Q_PRI_NODE *pNode;
    FAST int *pElement = nodeArray;
    FAST int ix;
    int count = 0;

    if (nodeArray == NULL)
	{
	for (ix = 0; ix < pQPriBMapHead->nPriority; ++ix)
	    count += dllCount (&pQPriBMapHead->pBMapList->listArray[ix]);
	return (count);
	}

    for (ix = 0; ix < pQPriBMapHead->nPriority; ++ix) /* search the array */
	{
	pNode=(Q_PRI_NODE *)DLL_FIRST(&pQPriBMapHead->pBMapList->listArray[ix]);

	while ((pNode != NULL) && (--maxNodes >= 0))	/* anybody left? */
	    {
	    *(pElement++) = (int)pNode;			/* fill in table */

	    pNode = (Q_PRI_NODE *) DLL_NEXT (&pNode->node);  /* next node */
	    }

	if (maxNodes < 0)		/* out of room? */
	    break;
	}

    return (pElement - nodeArray);	/* return count of active tasks */
    }

/*******************************************************************************
*
* qPriBMapEach - call a routine for each node in a queue
*
* This routine calls a user-supplied routine once for each node in the
* queue.  The routine should be declared as follows:
* .CS
*  BOOL routine (pQNode, arg)
*      Q_PRI_NODE *pQNode;	/@ pointer to a queue node          @/
*      int	   arg;		/@ arbitrary user-supplied argument @/
* .CE
* The user-supplied routine should return TRUE if qPriBMapEach() is to
* continue calling it for each entry, or FALSE if it is done and
* qPriBMapEach() can exit.
*
* RETURNS: NULL if traversed whole queue, or pointer to Q_PRI_NODE that
*          qPriBMapEach stopped on.
*/

Q_PRI_NODE *qPriBMapEach
    (
    Q_PRI_BMAP_HEAD *pQHead,     /* queue head of queue to call routine for */
    FUNCPTR          routine,    /* the routine to call for each table entry */
    int              routineArg  /* arbitrary user-supplied argument */
    )
    {
    FAST int	     ix;
    FAST Q_PRI_NODE *pNode = NULL;

    for (ix = 0; ix < pQHead->nPriority; ++ix)	/* search array */
	{
	pNode = (Q_PRI_NODE *)
		DLL_FIRST (&pQHead->pBMapList->listArray[ix]);

	while (pNode != NULL)
	    {
	    if (!((* routine) (pNode, routineArg)))
		goto done;				/* bail out */

	    pNode = (Q_PRI_NODE *) DLL_NEXT (&pNode->node);
	    }
	}

done:

    return (pNode);			/* return node we ended with */
    }

/*******************************************************************************
*
* qPriBMapNullRtn - null routine returns OK
*
* This routine does nothing and returns OK.  It is used by the queue class
* structure for operations not supported by this queue type.
*/

LOCAL STATUS qPriBMapNullRtn (void)
    {
    return (OK);
    }

#ifdef qPriBMapLib_PORTABLE

/*******************************************************************************
*
* qPriBMapSet - set the bits in the bit map for the specified priority
*
* This routine sets the bits in the bit map to reflect the addition of a node
* of the specified priority.
*/

LOCAL void qPriBMapSet
    (
    BMAP_LIST *pBMapList,
    int priority
    )
    {
    priority = 255 - priority;

    pBMapList->metaBMap			|= (1 << (priority >> 3));
    pBMapList->bMap [priority >> 3]	|= (1 << (priority & 0x7));
    }

/*******************************************************************************
*
* qPriBMapClear - clear the bits in the bit map for the specified priority
*
* This routine clears the bits in the bit map to reflect the removal of a node
* of the specified priority.
*/

LOCAL void qPriBMapClear
    (
    BMAP_LIST *pBMapList,
    int priority
    )
    {
    priority = 255 - priority;

    pBMapList->bMap [priority >> 3] &= ~(1 << (priority & 0x7));

    if (pBMapList->bMap [priority >> 3] == 0)
	pBMapList->metaBMap &= ~(1 << (priority >> 3));
    }

/*******************************************************************************
*
* qPriBMapHigh - return highest priority in ready queue
*
* This routine utilizes the bit map structure to determine the highest active
* priority group.
*
* RETURNS: Priority of highest active priority group.
*/

LOCAL int qPriBMapHigh
    (
    BMAP_LIST *pBMapList
    )
	{
	UINT8 highBits = (UINT8) ffsMsb ((int)pBMapList->metaBMap) - 1;
	UINT8 lowBits  = (UINT8) ffsMsb ((int)pBMapList->bMap[highBits]) - 1;

	return (255 - (((highBits << 3) | lowBits) & 0xff));
	}

#endif	/* qPriBMapLib_PORTABLE */

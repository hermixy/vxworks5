/* qFifoLib.c - wind active queue library */

/* Copyright 1984-1992 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01i,19jul92,pme  made qFifoRemove return STATUS.
01h,26may92,rrr  the tree shuffle
01g,19nov91,rrr  shut up some ansi warnings.
01f,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
01e,20sep90,jcf  documentation.
01d,10aug90,dnw  changed qFifoPut() to be void.
01c,05jul90,jcf	 added null routine for calibrateRtn field in Q_CLASS.
01b,26jun90,jcf	 fixed qFifoClass definition.
01a,14jun89,jcf	 written.
*/

/*
DESCRIPTION
This library contains routines to manage a first-in-first-out queue.  The
routine qFifoPut takes a key in addition to a node to queue.  This key
determines whether the node is placed at the head or tail of the queue.

This queue complies with the multi-way queue data structures and thus may be
utilized by any multi-way queue.  The FIFO multi-way queue class is accessed
by the global id qFifoClassId.

SEE ALSO: qLib.
*/

#include "vxWorks.h"
#include "qClass.h"
#include "qFifoLib.h"
#include "stdlib.h"


/* forward static functions */

static STATUS qFifoNullRtn (void);


/* locals */

LOCAL Q_CLASS qFifoClass =
    {
    (FUNCPTR)qFifoCreate,
    (FUNCPTR)qFifoInit,
    (FUNCPTR)qFifoDelete,
    (FUNCPTR)qFifoNullRtn,
    (FUNCPTR)qFifoPut,
    (FUNCPTR)qFifoGet,
    (FUNCPTR)qFifoRemove,
    (FUNCPTR)qFifoNullRtn,
    (FUNCPTR)qFifoNullRtn,
    (FUNCPTR)qFifoNullRtn,
    (FUNCPTR)qFifoNullRtn,
    (FUNCPTR)qFifoNullRtn,
    (FUNCPTR)qFifoInfo,
    (FUNCPTR)qFifoEach,
    &qFifoClass
    };

/* globals */

Q_CLASS_ID qFifoClassId = &qFifoClass;


/******************************************************************************
*
* qFifoCreate - allocate and initialize a FIFO queue
*
* This routine allocates and initializes a FIFO queue by allocating a
* Q_FIFO_HEAD structure from the free memory pool.
*
* RETURNS: Pointer to a Q_FIFO_HEAD, or NULL if out of memory.
*/

Q_FIFO_HEAD *qFifoCreate (void)
    {
    Q_FIFO_HEAD *pQFifoHead = (Q_FIFO_HEAD *) malloc (sizeof (Q_FIFO_HEAD));

    if (pQFifoHead == NULL)
	return (NULL);

    qFifoInit (pQFifoHead);

    return (pQFifoHead);
    }

/******************************************************************************
*
* qFifoInit - initialize a FIFO queue
*
* This routine initializes the specified FIFO queue.
*
* RETURNS: OK, or ERROR if FIFO queue could not be initialized.
*/

STATUS qFifoInit
    (
    Q_FIFO_HEAD *pQFifoHead
    )
    {
    dllInit (pQFifoHead);

    return (OK);
    }

/*******************************************************************************
*
* qFifoDelete - delete a FIFO queue
*
* This routine deallocates memory associated with the queue.  All queued
* nodes are lost.
*
* RETURNS: OK, or ERROR if memory cannot be deallocated.
*/

STATUS qFifoDelete
    (
    Q_FIFO_HEAD *pQFifoHead
    )
    {
    free ((char *) pQFifoHead);
    return OK;
    }

/*******************************************************************************
*
* qFifoPut - add a node to a FIFO queue
*
* This routine adds the specifed node to the FIFO queue.  The insertion is
* either at the tail or head based on the specified key FIFO_KEY_TAIL or
* FIFO_KEY_HEAD respectively.
*
* RETURNS: OK, or ERROR if node could not be added.
*/

void qFifoPut
    (
    Q_FIFO_HEAD *pQFifoHead,
    Q_FIFO_NODE *pQFifoNode,
    ULONG        key
    )
    {
    if (key == FIFO_KEY_HEAD)
	dllInsert (pQFifoHead, (DL_NODE *)NULL, pQFifoNode);
    else
	dllAdd (pQFifoHead, pQFifoNode);
    }

/*******************************************************************************
*
* qFifoGet - get the first node out of a FIFO queue
*
* This routines dequeues and returns the first node of a FIFO queue.  If the
* queue is empty, NULL will be returned.
*
* RETURNS: pointer the first node, or NULL if queue is empty.
*/

Q_FIFO_NODE *qFifoGet
    (
    Q_FIFO_HEAD *pQFifoHead
    )
    {
    if (DLL_EMPTY (pQFifoHead))
	return (NULL);

    return ((Q_FIFO_NODE *) dllGet (pQFifoHead));
    }

/*******************************************************************************
*
* qFifoRemove - remove the specified node from a FIFO queue
*
* This routine removes the specified node from a FIFO queue.
*/

STATUS qFifoRemove
    (
    Q_FIFO_HEAD *pQFifoHead,
    Q_FIFO_NODE *pQFifoNode
    )
    {
    dllRemove (pQFifoHead, pQFifoNode);

    return (OK);
    }

/*******************************************************************************
*
* qFifoInfo - get information of a FIFO queue
*
* This routine returns information of a FIFO queue.  If the parameter nodeArray
* is NULL, the number of queued nodes is returned.  Otherwise, the specified
* nodeArray is filled with a node pointers of the FIFO queue starting from the
* head.  Node pointers are copied until the TAIL of the queue is reached or
* until maxNodes has been entered in the nodeArray.
*
* RETURNS: Number of nodes entered in nodeArray, or nodes in FIFO queue.
*
* ARGSUSED
*/

int qFifoInfo
    (
    Q_FIFO_HEAD *pQFifoHead,    /* FIFO queue to gather list for */
    FAST int nodeArray[],       /* array of node pointers to be filled in */
    FAST int maxNodes           /* max node pointers nodeArray can accomodate */
    )
    {
    FAST DL_NODE *pNode = DLL_FIRST (pQFifoHead);
    FAST int *pElement  = nodeArray;

    if (nodeArray == NULL)		/* NULL node array means return count */
	return (dllCount (pQFifoHead));

    while ((pNode != NULL) && (--maxNodes >= 0))
	{
	*(pElement++) = (int)pNode;	/* fill in table */
	pNode = DLL_NEXT (pNode);	/* next node */
	}

    return (pElement - nodeArray);	/* return count of active tasks */
    }

/*******************************************************************************
*
* qFifoEach - call a routine for each node in a queue
*
* This routine calls a user-supplied routine once for each node in the
* queue.  The routine should be declared as follows:
* .CS
*  BOOL routine (pQNode, arg)
*      Q_FIFO_NODE *pQNode;	/@ pointer to a queue node          @/
*      int	   arg;		/@ arbitrary user-supplied argument @/
* .CE
* The user-supplied routine should return TRUE if qFifoEach() is to continue
* calling it for each entry, or FALSE if it is done and qFifoEach() can exit.
*
* RETURNS: NULL if traversed whole queue, or pointer to Q_FIFO_NODE that
*          qFifoEach stopped on.
*/

Q_FIFO_NODE *qFifoEach
    (
    Q_FIFO_HEAD *pQHead,        /* queue head of queue to call routine for */
    FUNCPTR     routine,        /* the routine to call for each table entry */
    int         routineArg      /* arbitrary user-supplied argument */
    )
    {
    FAST DL_NODE *pQNode = DLL_FIRST (pQHead);

    while ((pQNode != NULL) && ((* routine) (pQNode, routineArg)))
	pQNode = DLL_NEXT (pQNode);

    return (pQNode);			/* return node we ended with */
    }

/*******************************************************************************
*
* qFifoNullRtn - null routine for queue class structure
*
* This routine does nothing and returns OK.
*
* RETURNS: OK
*
* NOMANUAL
*/

LOCAL STATUS qFifoNullRtn (void)
    {
    return (OK);
    }

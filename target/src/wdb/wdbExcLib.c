/* wdbExcLib.c - handles exceptions */

/* Copyright 1995-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,14sep01,jhw  Fixed warnings from compiling with gnu -pedantic flag
01b,22may95,ms	Only suspend the system if the agent is in system mode.
01a,14feb95,ms  written.
*/

/*
DESCPRIPTION

This library handles the reporting of exceptions to the host.
If multiple tasks have exceptions before the host uploads the info,
then the exception events are queued.
A maximum of five exception events can be queued at a time.
*/

#include "wdb/wdb.h"
#include "wdb/wdbLibP.h"
#include "wdb/dll.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbRtIfLib.h"
#include "wdb/wdbEvtLib.h"
#include "wdb/wdbArchIfLib.h"

/* definitions */

#define MAX_EXC_EVENTS	5	/* max of 5 exception events at once */

/* structures */

typedef struct
    {
    dll_t	node;
    BOOL	valid;
    WDB_CTX	context;
    u_int	excVector;
    char *	pESF;
    } wdbExcInfoNode_t;

/* local variables */

static WDB_EVT_NODE	wdbExcEvtNode;
static dll_t		wdbExcEvtList;
static wdbExcInfoNode_t	excInfoNode [MAX_EXC_EVENTS];

/* forward declarations */

static void wdbExcHook (WDB_CTX context, u_int vec, char *pESF, WDB_IU_REGS *);
static void wdbExcGetEvent (void *arg, WDB_EVT_DATA *pEvtMsg);
static void wdbExcDeqEvent (void *arg);

/******************************************************************************
*
* wdbExcLibInit -
*/

void wdbExcLibInit (void)
    {
    int ix;

    /* initialize our event node */

    wdbEventNodeInit (&wdbExcEvtNode, wdbExcGetEvent,
			wdbExcDeqEvent, NULL);

    /* initialize the linked list of exceptions */

    dll_init (&wdbExcEvtList);
    for (ix = 0; ix < MAX_EXC_EVENTS; ix++)
	dll_insert (&excInfoNode[ix].node, &wdbExcEvtList);

    /* add our exception hook to the run-time exception handler */

    (*pWdbRtIf->excHookAdd)(wdbExcHook);
    }

/******************************************************************************
*
* wdbExcHook -
*/

static void wdbExcHook
    (
    WDB_CTX	context,
    u_int	vec,
    char *	pESF,
    WDB_IU_REGS	* pRegs
    )
    {
    wdbExcInfoNode_t *	pExcInfoNode;
    int			lockKey;

    /* get a node off the queue */

    lockKey = intLock();
    pExcInfoNode = (wdbExcInfoNode_t *)dll_tail (&wdbExcEvtList);

    /* if event list if full and this is a task exception, just return */

    if ((pExcInfoNode->valid) && (context.contextType == WDB_CTX_TASK))
	{
	intUnlock (lockKey);
	return;
	}

    pExcInfoNode->valid     = TRUE;
    pExcInfoNode->context   = context;
    pExcInfoNode->excVector = vec;
    pExcInfoNode->pESF	    = pESF;

    dll_remove (&pExcInfoNode->node);
    dll_insert (&pExcInfoNode->node, &wdbExcEvtList);
    intUnlock (lockKey);

    /* exception in task context */

    if (wdbIsNowTasking())
	{
	wdbEventPost (&wdbExcEvtNode);
	return;
	}

    /* exception in system context - suspend system before posting event */

    wdbSuspendSystem (pRegs, wdbEventPost, (int)&wdbExcEvtNode);

    /* NOTREACHED */
    }

/******************************************************************************
*
* wdbExcGetEvent - upload the exception event to the host.
*/

void wdbExcGetEvent
    (
    void *	arg,
    WDB_EVT_DATA *	pEvtData
    )
    {
    wdbExcInfoNode_t *	pNode;
    wdbExcInfoNode_t *	pTmpNode;
    WDB_EXC_INFO *	pExcInfo;

    int lockKey;

    /* get a node from the exception queue */

    lockKey = intLock();
    pNode   = (wdbExcInfoNode_t *) dll_head (&wdbExcEvtList);
    dll_remove (&pNode->node);
    intUnlock (lockKey);

    /* give the node info to the host */

    pExcInfo = (WDB_EXC_INFO *)&pEvtData->eventInfo;
    pEvtData->evtType		= WDB_EVT_EXC;
    pExcInfo->numInts		= 4;
    pExcInfo->context		= pNode->context;
    pExcInfo->vec		= pNode->excVector;
    pExcInfo->pEsf		= (TGT_ADDR_T) pNode->pESF;

    /* mark the node invalid and put back in queue (after valid nodes) */

    pNode->valid = FALSE;
    lockKey = intLock();
    for (pTmpNode =  (wdbExcInfoNode_t *) dll_head (&wdbExcEvtList);
	 pTmpNode != (wdbExcInfoNode_t *) dll_end  (&wdbExcEvtList);
	 pTmpNode =  (wdbExcInfoNode_t *) dll_next (&pTmpNode->node))
	{
	if (pTmpNode->valid == FALSE)
	    break;
	}
    pTmpNode = (wdbExcInfoNode_t *) dll_prev (&pTmpNode->node);
    dll_insert (&pNode->node, &pTmpNode->node);
    intUnlock (lockKey);
    }

/******************************************************************************
*
* wdbExcDeqEvent - dequeue the event node.
*/

void wdbExcDeqEvent
    (
    void *arg
    )
    {
    int			lockKey;
    wdbExcInfoNode_t *	pNode;

    lockKey = intLock();

    pNode = (wdbExcInfoNode_t *) dll_head (&wdbExcEvtList);

    if (pNode->valid)
	{
	intUnlock (lockKey);
	wdbEventPost (&wdbExcEvtNode);
	return;
	}

    intUnlock (lockKey);
    }


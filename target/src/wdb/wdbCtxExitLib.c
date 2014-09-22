/* wdbCtxExitLib.c - notify host of context exit */

/* Copyright 1984-1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,26feb98,dbt  fixed a problem if a context is exited while in system mode.
		 return WDB_ERR_INVALID_EVENTPOINT when we can't delete a
		 context exit eventpoint (task no longer exists) to enable
		 a host tool to remove it in target server eventpoint list.
01d,26jan98,dbt  replaced wdbEventClassConnect() with wdbEvtptClassConnect().
		 replaced WDB_EVT_CLASS with WDB_EVTPT_CLASS
01c,23jan96,tpr  added cast to compile with Diab Data tools.
01b,22sep95,ms   allow removal of context exit eventpoints (SPR 4853)
01a,09mar95,ms   written.
*/

/*
DESCPRIPTION

*/

#include "wdb/wdb.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbEvtLib.h"
#include "wdb/wdbEvtptLib.h"
#include "wdb/wdbRtIfLib.h"

/* data types */

typedef struct
    {
    WDB_EVT_NODE	eventNode;
    WDB_CTX		context;
    UINT32		returnVal;
    UINT32		errnoVal;
    } wdbCtxExitNode_t;

/* local variables */

LOCAL WDB_EVTPT_CLASS	ctxExitClass;
LOCAL wdbCtxExitNode_t	externExitNode;

/* forward static declarations */

static UINT32 ctxExitEvtptAdd    (WDB_EVTPT_ADD_DESC *pEvtPt, UINT32 *pId);
static UINT32 ctxExitEvtptDelete (TGT_ADDR_T *pId);

/******************************************************************************
*
* wdbCtxExitLibInit - initialize the library.
*/

void wdbCtxExitLibInit (void)
    {
    ctxExitClass.evtptType	= WDB_EVT_CTX_EXIT;
    ctxExitClass.evtptAdd	= ctxExitEvtptAdd;
    ctxExitClass.evtptDel	= ctxExitEvtptDelete;
    wdbEvtptClassConnect (&ctxExitClass);
    }

/******************************************************************************
*
* ctxExitEventGet - fill in the WDB_EVT_DATA for the host.
*/ 

static void ctxExitEventGet
    (
    void *		pNode,
    WDB_EVT_DATA *	pEvtData
    )
    {
    wdbCtxExitNode_t *	pExitNode = pNode;
    WDB_CTX_EXIT_INFO *	pCtxExitInfo;
    pCtxExitInfo = (WDB_CTX_EXIT_INFO *)&pEvtData->eventInfo;

    pEvtData->evtType			= WDB_EVT_CTX_EXIT;
    pCtxExitInfo->numInts		= 4;
    pCtxExitInfo->context		= pExitNode->context;
    pCtxExitInfo->returnVal		= pExitNode->returnVal;
    pCtxExitInfo->errnoVal		= pExitNode->errnoVal;
    }

/******************************************************************************
*
* ctxExitEventDeq - dequeue the event node.
*/ 

static void ctxExitEventDeq
    (
    void *	pExitNode
    )
    {
    if (wdbIsNowTasking ())
	(*pWdbRtIf->free) (pExitNode);
    }

/******************************************************************************
*
* wdbCtxExitNotifyHook - task-specific exit hook.
*/ 

void wdbCtxExitNotifyHook
    (
    WDB_CTX	context,		/* exiting context */
    UINT32	exitCode,		/* exit code/return value */
    UINT32	errnoVal		/* errno value */
    )
    {
    wdbCtxExitNode_t *	pExitNode;

    if (wdbIsNowExternal ())
	pExitNode = &externExitNode;
    else
	{
	pExitNode = (wdbCtxExitNode_t *)(*pWdbRtIf->malloc)
				(sizeof (wdbCtxExitNode_t));
	if (pExitNode == NULL)
	    return;
	}

    pExitNode->context	 = context;
    pExitNode->returnVal = exitCode;
    pExitNode->errnoVal	 = errnoVal;

    wdbEventNodeInit (&pExitNode->eventNode, ctxExitEventGet, ctxExitEventDeq,
			pExitNode);

    wdbEventPost (&pExitNode->eventNode);
    }

/******************************************************************************
*
* ctxExitEvtptAdd - notify the host when some task exits.
*/ 

static UINT32 ctxExitEvtptAdd
    (
    WDB_EVTPT_ADD_DESC *pEvtPt,
    UINT32 *	pId
    )
    {
    if ((*pWdbRtIf->taskDeleteHookAdd) (pEvtPt->context.contextId,
					wdbCtxExitNotifyHook) == ERROR)
	return (WDB_ERR_INVALID_CONTEXT);

    *pId = pEvtPt->context.contextId;
    return (OK);
    }

/******************************************************************************
*
* ctxExitEvtptDelete -
*/ 

static UINT32 ctxExitEvtptDelete
    (
    TGT_ADDR_T *pId
    )
    {
    if ((int ) *pId == -1)	/* XXX - should delete all exit event points */
	return (OK);

    if ((*pWdbRtIf->taskDeleteHookAdd) ((UINT32 ) *pId, NULL) == ERROR)
        return (WDB_ERR_INVALID_EVENTPOINT);

    return (OK);
    }

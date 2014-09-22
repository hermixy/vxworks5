/* wdbCtxStart.c - notify host of context creation */

/* Copyright 1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,25feb98,dbt	 written.
*/

/*
DESCPRIPTION
This library provides routines to notify host on context creation.

NOMANUAL
*/

#include "vxWorks.h"
#include "taskHookLib.h"
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
    WDB_CTX		createdCtx;
    WDB_CTX		creationCtx;
    } wdbCtxStartNode_t;

/* local variables */

LOCAL WDB_EVTPT_CLASS		wdbCtxStartClass;
LOCAL wdbCtxStartNode_t		externStartNode;
LOCAL UINT32			wdbCtxStartEvtptCount;

/* forward static declarations */

static UINT32 wdbCtxStartEvtptAdd (WDB_EVTPT_ADD_DESC *pEvtPt, UINT32 *pId);
static UINT32 wdbCtxStartEvtptDelete (TGT_ADDR_T *pId);

/******************************************************************************
*
* wdbCtxStartInit - initialize the library
*
* RETURNS : N/A
*
* NOMANUAL
*/

void wdbCtxStartLibInit (void)
    {
    wdbCtxStartClass.evtptType	= WDB_EVT_CTX_START;
    wdbCtxStartClass.evtptAdd	= wdbCtxStartEvtptAdd;
    wdbCtxStartClass.evtptDel	= wdbCtxStartEvtptDelete;

    /* initialize context start eventpoint count */

    wdbCtxStartEvtptCount = 0;

    wdbEvtptClassConnect (&wdbCtxStartClass);
    }

/******************************************************************************
*
* wdbCtxStartEventGet - fill in the WDB_EVT_DATA for the host.
* 
* This routine fills in the WDB_EVT_DATA with event informations. 
*
* RETURNS: N/A
*/ 

static void wdbCtxStartEventGet
    (
    void *		pNode,
    WDB_EVT_DATA *	pEvtData
    )
    {
    wdbCtxStartNode_t *		pStartNode = pNode;
    WDB_CTX_START_INFO *	pCtxStartInfo;

    pCtxStartInfo = (WDB_CTX_START_INFO *)&pEvtData->eventInfo;

    pEvtData->evtType			= WDB_EVT_CTX_START;
    pCtxStartInfo->numInts		= 4;
    pCtxStartInfo->createdCtx		= pStartNode->createdCtx;
    pCtxStartInfo->creationCtx		= pStartNode->creationCtx;
    }

/******************************************************************************
*
* wdbCtxStartEventDeq - dequeue the event node.
*
* This routine dequeues the event node.
*
* RETURNS: N/A
*/ 

static void wdbCtxStartEventDeq
    (
    void *	pStartNode
    )
    {
    if (wdbIsNowTasking ())
	(*pWdbRtIf->free) (pStartNode);
    }

/******************************************************************************
*
* wdbCtxStartNotifyHook - task-specific create hook.
*
* This routine is called at every task creation in order to notify the
* host of this creation.
*
* RETURNS: N/A
*/ 

static void wdbCtxStartNotifyHook
    (
    WDB_CTX *	createdCtx,		/* created context */
    WDB_CTX *	creationCtx		/* context where it was created from */
    )
    {
    wdbCtxStartNode_t *	pStartNode;

    if (wdbIsNowExternal ())
	pStartNode = &externStartNode;
    else
	{
	pStartNode = (wdbCtxStartNode_t *)(*pWdbRtIf->malloc)
				    (sizeof (wdbCtxStartNode_t));

	if (pStartNode == NULL)
	    return;
	}

    pStartNode->createdCtx	 = *createdCtx;
    pStartNode->creationCtx	 = *creationCtx;

    wdbEventNodeInit (&pStartNode->eventNode, wdbCtxStartEventGet, 
				wdbCtxStartEventDeq, pStartNode);

    wdbEventPost (&pStartNode->eventNode);
    }

/******************************************************************************
*
* wdbCtxStartEvtptAdd - notify the host when some task exits.
*
* Add a context start eventpoint
*
* RETURN : WDB_OK always.
*/ 

static UINT32 wdbCtxStartEvtptAdd
    (
    WDB_EVTPT_ADD_DESC *	pEvtPt,
    UINT32 *			pId
    )
    {
    (*pWdbRtIf->taskCreateHookAdd) (wdbCtxStartNotifyHook);

    wdbCtxStartEvtptCount ++;

    /* XXX DBT hack to get a unique eventpoint ID */

    *pId = (UINT32)&wdbCtxStartEvtptCount;

    return (OK);
    }

/******************************************************************************
*
* wdbCtxStartEvtptDelete - delete context start eventpoint
*
* This routine deletes a context start eventpoint.
*
* RETURNS : WDB_OK always.
*/ 

static UINT32 wdbCtxStartEvtptDelete
    (
    TGT_ADDR_T *pId
    )
    {
    if ((int ) *pId == -1)	/* remove all context start eventpoints */
	{
	(*pWdbRtIf->taskCreateHookAdd) (NULL);
	wdbCtxStartEvtptCount = 0;
	return (WDB_OK);
	}

    /* test if there is an eventpoint to delete and if the ID is good */

    if (wdbCtxStartEvtptCount == 0 || 
	((int ) *pId != (int) &wdbCtxStartEvtptCount))
	return (WDB_ERR_INVALID_EVENTPOINT);

    /* 
     * Remove the context start notification hook only if there are no
     * more context start eventpoints.
     */

    if (wdbCtxStartEvtptCount == 1)
	(*pWdbRtIf->taskCreateHookAdd) (NULL);
	
    wdbCtxStartEvtptCount --;

    return (WDB_OK);
    }

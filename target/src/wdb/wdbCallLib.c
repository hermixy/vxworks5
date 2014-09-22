/* wdbCallLib.c - Call a function on the target */

/* Copyright 1984-1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,04sep98,cdp enable Thumb support for all ARM CPUs with ARM_THUMB==TRUE.
01e,11jul97,cdp added ARM7TDMI_T support (func call in Thumb state).
01d,02oct96,elp added casts due to TGT_ADDR_T type change.
01c,31aug95,ms  WDB_FUNC_CALL now allows file redirection (SPR #4497)
01b,19jun95,ms	removed dependancy on wdbCtxLib
01a,02nov94,ms  written.
*/

/*
DESCPRIPTION

This library contains the RPC to call a function on the target.
The return value of the function is passed to the host asynchronously.
*/

#include "wdb/wdb.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbRtIfLib.h"
#include "wdb/wdbEvtLib.h"
#include "errno.h"
#include "string.h"

/* data types */

typedef struct
    {
    WDB_EVT_NODE	eventNode;
    WDB_CALL_RET_INFO	callRetInfo;
    int			(*entry)();
    int			arg[10];
    WDB_CALL_RET_TYPE	returnType;
    } WDB_CALL_RET_NODE;

/* forward declarations */

static UINT32 wdbFuncCall (WDB_CTX_CREATE_DESC *pCtxCreate, UINT32 *pTid);

/******************************************************************************
*
* wdbFuncCallLibInit -
*/

void wdbFuncCallLibInit (void)
    {
    wdbSvcAdd (WDB_FUNC_CALL, wdbFuncCall, xdr_WDB_CTX_CREATE_DESC, xdr_UINT32);
    }

/******************************************************************************
*
* callRetEvtDeq - dequeue a function call event node.
*/ 

static void callRetEvtDeq
    (
    void *      pExitNode
    )
    {
    (*pWdbRtIf->free) (pExitNode);
    }

/******************************************************************************
*
* callRetEvtGet - get a call return event.
*/ 

static void callRetEvtGet
    (
    void *              pNode,
    WDB_EVT_DATA *      pEvtData
    )
    {
    WDB_CALL_RET_NODE *  pCallRetNode = pNode;

    pEvtData->evtType                   = WDB_EVT_CALL_RET;
    pEvtData->eventInfo.callRetInfo	= pCallRetNode->callRetInfo;
    }


/******************************************************************************
*
* funcCallWrapper - wrapper for a function call.
*/ 

static void funcCallWrapper
    (
    WDB_CALL_RET_NODE * pReturnNode
    )
    {
    if (pReturnNode->callRetInfo.returnType == WDB_CALL_RET_DBL)
	{
	pReturnNode->callRetInfo.returnVal.returnValDbl =
				(*(double (*)())pReturnNode->entry)
				(pReturnNode->arg[0],
				 pReturnNode->arg[1], pReturnNode->arg[2],
				 pReturnNode->arg[3], pReturnNode->arg[4],
				 pReturnNode->arg[5], pReturnNode->arg[6],
				 pReturnNode->arg[7], pReturnNode->arg[8],
				 pReturnNode->arg[9]);
	}
    else
	{
	pReturnNode->callRetInfo.returnVal.returnValInt =
				(*pReturnNode->entry) (pReturnNode->arg[0],
				 pReturnNode->arg[1], pReturnNode->arg[2],
				 pReturnNode->arg[3], pReturnNode->arg[4],
				 pReturnNode->arg[5], pReturnNode->arg[6],
				 pReturnNode->arg[7], pReturnNode->arg[8],
				 pReturnNode->arg[9]);
	}

    pReturnNode->callRetInfo.errnoVal = errno;

    wdbEventNodeInit (&pReturnNode->eventNode, callRetEvtGet,
                        callRetEvtDeq, pReturnNode);

    wdbEventPost (&pReturnNode->eventNode);
    }

/******************************************************************************
*
* wdbFuncCall - spawn a task execute a routine, then send back the reply.
*/ 

static UINT32 wdbFuncCall
    (
    WDB_CTX_CREATE_DESC *	pCtxCreate,
    UINT32 *		pTid
    )
    {
    WDB_CTX	context;
    WDB_CALL_RET_NODE * pReturnNode;

    if (!wdbIsNowTasking())
	return (WDB_ERR_AGENT_MODE);

    if ((pWdbRtIf->taskCreate == NULL) ||
	(pWdbRtIf->malloc == NULL) ||
	(pWdbRtIf->free == NULL))
	return (WDB_ERR_NO_RT_PROC);

    pReturnNode = (WDB_CALL_RET_NODE *)(*pWdbRtIf->malloc)
					(sizeof(WDB_CALL_RET_NODE));

    if (pReturnNode == NULL)
	return (WDB_ERR_RT_ERROR);

#if ((CPU_FAMILY == ARM) && ARM_THUMB)
    pReturnNode->entry = (int (*)())(pCtxCreate->entry | 1);
#else /* CPU_FAMILY == ARM */
    pReturnNode->entry = (int (*)())pCtxCreate->entry;
#endif /* CPU_FAMILY == ARM */
    bcopy ((char *)pCtxCreate->args, (char *)pReturnNode->arg,
		10 * sizeof (int));
    pReturnNode->callRetInfo.returnType =
		(pCtxCreate->options & WDB_FP_RETURN ? WDB_CALL_RET_DBL :
							   WDB_CALL_RET_INT);
    pCtxCreate->args[0] = (int)pReturnNode;

    *pTid = (*pWdbRtIf->taskCreate)
                (pCtxCreate->name, pCtxCreate->priority,
                pCtxCreate->options, (char *)pCtxCreate->stackBase,
                pCtxCreate->stackSize, (char *)funcCallWrapper,
		pCtxCreate->args, pCtxCreate->redirIn,
                pCtxCreate->redirOut, pCtxCreate->redirErr);

    if (*pTid == ERROR)
	return (WDB_ERR_RT_ERROR);

    pReturnNode->callRetInfo.callId = *pTid;

    context.contextId = *pTid;
    context.contextType = WDB_CTX_TASK;

    return (((*pWdbRtIf->taskResume) (&context) == OK ?
		OK : WDB_ERR_RT_ERROR));
    }

/* wdbCtxLib.c - WDB context control routines */

/* Copyright 1994-1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,24mar98,dbt  added WDB_CTX_STATUS_GET service.
01d,02oct96,elp  added casts due to TGT_ADDR_T type change in wdb.h.
01c,22sep95,ms   don't allow agent to WDB_SUSPEND itself (id=0). SPR #4979.
01b,19jun95,ms   the untested "system context" creation is no longer supported.
01a,30sep94,ms   written.
*/

/*
DESCPRIPTION

Control other contexts from the agent.
*/

#include "wdb/wdb.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbRtIfLib.h"
#include "wdb/wdbArchIfLib.h"

/* hacks */

extern BOOL      wdbOneShot;

/* forward declarations */

static UINT32 wdbCtxKill	(WDB_CTX * pCtx);
static UINT32 wdbCtxSuspend	(WDB_CTX * pCtx);
static UINT32 wdbCtxStatusGet	(WDB_CTX * pCtx, UINT32 * pCtxStatus);

/******************************************************************************
*
* wdbCtxLibInit -
*/

void wdbCtxLibInit (void)
    {
    wdbSvcAdd (WDB_CONTEXT_CREATE,	wdbCtxCreate, xdr_WDB_CTX_CREATE_DESC,
								xdr_UINT32);
    wdbSvcAdd (WDB_CONTEXT_KILL,	wdbCtxKill, xdr_WDB_CTX, xdr_void);
    wdbSvcAdd (WDB_CONTEXT_SUSPEND,	wdbCtxSuspend, xdr_WDB_CTX, xdr_void);
    wdbSvcAdd (WDB_CONTEXT_RESUME,	wdbCtxResume, xdr_WDB_CTX, xdr_void);
    wdbSvcAdd (WDB_CONTEXT_STATUS_GET,	wdbCtxStatusGet, xdr_WDB_CTX,
								xdr_UINT32);
    }

/******************************************************************************
*
* wdbCtxCreate - create a context.
*/

UINT32 wdbCtxCreate
    (
    WDB_CTX_CREATE_DESC *	pCtxCreate,
    UINT32 *		pTid
    )
    {
    /* task mode context creation */

    if (wdbIsNowTasking())
	{
	if (pWdbRtIf->taskCreate == NULL)
	    return (WDB_ERR_NO_RT_PROC);

        *pTid = (*pWdbRtIf->taskCreate)
		(pCtxCreate->name, pCtxCreate->priority,
		pCtxCreate->options, (char *)pCtxCreate->stackBase,
		pCtxCreate->stackSize, (char *)pCtxCreate->entry,
		pCtxCreate->args, pCtxCreate->redirIn,
		pCtxCreate->redirOut, pCtxCreate->redirErr);

	if (*pTid == ERROR)
	    return (WDB_ERR_RT_ERROR);

	return (OK);
	}

    /* external mode context creation */

    return (WDB_ERR_NO_AGENT_PROC);
    }

/******************************************************************************
*
* wdbCtxKill - kill a context.
*
* killing the "WDB_CTX_SYSTEM" context causes a reboot.
* Only the tasking agent can kill a task context.
*/

static UINT32 wdbCtxKill
    (
    WDB_CTX *	 pCtx		/* context to kill */
    )
    {
    /* killing the WDB_CTX_SYSTEM means reboot */

    if (pCtx->contextType == WDB_CTX_SYSTEM)
	(*pWdbRtIf->reboot)();

    /* killing any other context is only valid in tasking mode */

    if (wdbIsNowExternal())
	return (WDB_ERR_AGENT_MODE);

    /* else we are a tasking agent and must kill another task */

    if (pWdbRtIf->taskDelete == NULL)
        return (WDB_ERR_NO_RT_PROC);

    return (((*pWdbRtIf->taskDelete) (pCtx) == OK ?
				 OK : WDB_ERR_INVALID_CONTEXT));
    }

/******************************************************************************
*
* wdbCtxSuspend - suspend a context.
*
* Task agent:   makes an OS callout to suspend a context (WDB_CTX).
* Extern agent: ignores WDB_CTX. Suspends the system.
*/

static UINT32 wdbCtxSuspend
    (
    WDB_CTX *	 pCtx		/* context to suspend */
    )
    {
    /* task mode agent's context suspend routine */

    if (wdbIsNowTasking ())
	{
	if (pWdbRtIf->taskSuspend == NULL)
	    return (WDB_ERR_NO_RT_PROC);

	if (pCtx->contextId == 0)
	    return (WDB_ERR_INVALID_CONTEXT);

	return (((*pWdbRtIf->taskSuspend) (pCtx) == OK ?
				OK : WDB_ERR_INVALID_CONTEXT));
	}

    /* external mode agent's context suspend routine */

    if (pCtx->contextType != WDB_CTX_SYSTEM)
	return (WDB_ERR_AGENT_MODE);

    wdbOneShot = FALSE;
    return (OK);
    }

/******************************************************************************
*
* wdbCtxResume - resume a context.
*
* Task agent:   makes an OS callout to resume a context (WDB_CTX).
* Extern agent: ignores WDB_CTX. Resumes the system.
*/

UINT32 wdbCtxResume
    (
    WDB_CTX *	 pCtx		/* context to resume */
    )
    {
    /* task mode agent's context resume routine */

    if (wdbIsNowTasking ())
        {
        if (pWdbRtIf->taskResume == NULL)
	    return (WDB_ERR_NO_RT_PROC);

	return (((*pWdbRtIf->taskResume) (pCtx) == OK ?
				OK : WDB_ERR_INVALID_CONTEXT));
        }

    /* external mode agent's context resume routine */

    if (pCtx->contextType != WDB_CTX_SYSTEM)
	return (WDB_ERR_AGENT_MODE);

    wdbOneShot = TRUE;
    return (OK);
    }

/******************************************************************************
*
* wdbCtxStatusGet - get the status of a context.
*
* This routine returns the status of a context.
*
*/

static UINT32 wdbCtxStatusGet
    (
    WDB_CTX *	pCtx,		/* context to get status of */
    UINT32 *	pCtxStatus	/* were to put the context status */
    )
    {
    /* 
     * At that moment we can only get the status of the system context.
     * Support for task should be implemented latter if we need it.
     */

    if (wdbIsNowTasking ())
	return (WDB_ERR_AGENT_MODE);

    /* external mode agent's context status get routine */

    if (pCtx->contextType != WDB_CTX_SYSTEM)
	return (WDB_ERR_AGENT_MODE);

    if (wdbOneShot)	/* the system context is running */
	*pCtxStatus = WDB_CTX_RUNNING;
    else		/* the system context is suspended */
	*pCtxStatus = WDB_CTX_SUSPENDED;

    return (WDB_OK);
    }

/* wdbCntctLib.c - handles WDB connections */

/* Copyright 1984-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,23jul98,dbt  if the agent is already connected to a target server when we
                 receive a new connect request, call wdbTargetDisconnect() to
                 remove all old eventpoints.
01e,12feb98,dbt  replaced wdbEvtLib.h include with wdbEvtptLib.h. Remove
                 dynamically loaded services when we connect to a new target
                 server.
01d,05jul96,p_m  added WDB_TARGET_MODE_GET (SPR# 6200).
01c,07jun95,ms   remove all eventpoints during WDB_TARGET_DISCONNECT.
01b,02jun95,ms   Added wdbTargetIsConnected().
01a,21sep94,ms   written.
*/

/*
DESCPRIPTION

This library contains the session management services:
	WDB_TARGET_CONNECT	connect to agent
	WDB_TARGET_DISCONNECT	disconnect from agent
	WDB_TARGET_MODE_SET	change agent mode (system vs tasking)
	WDB_TARGET_MODE_GET	return agent mode 
	WDB_TARGET_PING		test the connection.
*/

#include "vxWorks.h"
#include "wdb/wdb.h"
#include "wdb/wdbLib.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbRtIfLib.h"
#include "wdb/wdbEvtptLib.h"

/* local variables */

static BOOL    _wdbTargetIsConnected = FALSE;

/* forward declarations */

static UINT32  wdbTargetConnect    (void * noParams, WDB_TGT_INFO * pTgtInfo);
static UINT32  wdbTargetModeSet    (u_int * pModeVal);
static UINT32  wdbTargetModeGet    (void * noParams, u_int * pModeVal);
static UINT32  wdbTargetDisconnect (void);
static UINT32  wdbTargetPing	   (void);

/******************************************************************************
*
* wdbConnectLibInit -
*/

void wdbConnectLibInit (void)
    {
    wdbSvcAdd (WDB_TARGET_PING, wdbTargetPing, xdr_void, xdr_void);
    wdbSvcAdd (WDB_TARGET_CONNECT, wdbTargetConnect, xdr_void,
		xdr_WDB_TGT_INFO);
    wdbSvcAdd (WDB_TARGET_DISCONNECT, wdbTargetDisconnect, xdr_void, xdr_void);
    wdbSvcAdd (WDB_TARGET_MODE_SET, wdbTargetModeSet, xdr_u_int, xdr_void);
    wdbSvcAdd (WDB_TARGET_MODE_GET, wdbTargetModeGet, xdr_void, xdr_u_int);
    }

/******************************************************************************
*
* wdbTargetIsConnected - test if the target is conected.
*/ 

BOOL wdbTargetIsConnected (void)
    {
    return (_wdbTargetIsConnected);
    }

/******************************************************************************
*
* wdbTargetConnect - connect to the target server.
*/

static UINT32 wdbTargetConnect
    (
    void *		noParams,
    WDB_TGT_INFO *	pTgtInfo
    )
    {
    /*
     * If the agent is already connected to a target server, disconnect from
     * this target server.
     */

    if (wdbTargetIsConnected ())
	wdbTargetDisconnect ();

    /* get the run time info */

    (*pWdbRtIf->rtInfoGet)(&pTgtInfo->rtInfo);

    /* get the agent info */

    wdbInfoGet (&pTgtInfo->agentInfo);

    /* mark the target as connnected */

    _wdbTargetIsConnected = TRUE;

    return (OK);
    }

/******************************************************************************
*
* wdbTargetPing - ping the target.
*/ 

static UINT32  wdbTargetPing       (void)
    {
    return (OK);
    }

/******************************************************************************
*
* wdbTargetDisconnect - disconnect from the target
*/

static UINT32 wdbTargetDisconnect (void)
    {
    _wdbTargetIsConnected = FALSE;

    /* remove all dynamically loaded services */

    wdbSvcDsaSvcRemove ();

    /* remove all eventpoints */

    wdbEvtptDeleteAll();

    return (OK);
    }

/******************************************************************************
*
* wdbTargetModeSet - change agent modes.
*/

static UINT32 wdbTargetModeSet
    (
    UINT32 *	pMode
    )
    {
    if (wdbModeSet (*pMode) != OK)
	return (WDB_ERR_AGENT_MODE);

    return (OK);
    }

/******************************************************************************
*
* wdbTargetModeGet - return current agent mode.
*/

static UINT32 wdbTargetModeGet
    (
    void *	noParams,
    UINT32 *	pMode
    )
    {
    if (wdbIsNowExternal())
	*pMode = WDB_MODE_EXTERN;
    else
	*pMode = WDB_MODE_TASK;

    return (OK);
    }

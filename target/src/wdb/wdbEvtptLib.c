/* wdbEvtptLib.c - Eventpoint library for the WDB agent */

/* Copyright 1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,23jan98,dbt	 written based on wdbEvtLib.c.
*/

/*
DESCRIPTION

This library provides a framework for eventpoints.

Many debugging facilities require the host to specify which
eventpoints to look for. For example, the breakpoint library requires the
host to set breakpoints at specific addresses, and the task-exit facility
requires the host to tell it which task-exits to report.
The host sets "eventpoints" by calling wdbEvtptAdd, specifying
an EVENT and a CONTEXT. This library routes the request to the appropriate
library based on the EVENT_TYPE field in the EVENT structure.
The routine wdbEvtptClassConnect() allows libraries to register their 
eventpoint services.
*/

#include "vxWorks.h"
#include "wdb/dll.h"
#include "wdb/wdb.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbArchIfLib.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbEvtptLib.h"

/* definitions */

#ifndef offsetof
#define offsetof(type, mbr)	((size_t) &((type *)0)->mbr)
#endif
#define STRUCT_BASE(s,m,p)	((s *)(void *)((char *)(p) - offsetof(s,m)))
#define EVTPT_BASE(p)		STRUCT_BASE(WDB_EVTPT_CLASS, evtptList, (p))

/* local variables */

static dll_t		wdbEvtptClassList;	/* attached libraries */

/* forward declarations */

static void	_wdbEvtptDeleteAll (void);
static UINT32	wdbEvtptAdd    (WDB_EVTPT_ADD_DESC * pEvt, UINT32 *pId);
static UINT32	wdbEvtptDelete (WDB_EVTPT_DEL_DESC * pEvt);

/******************************************************************************
*
* wdbEventpointLibInit - initialize the WDB eventpoint library.
*/

void wdbEvtptLibInit (void)
    {
    static BOOL	wdbEvtptLibInitialized = FALSE;

    if (wdbEvtptLibInitialized)
	return;

    dll_init (&wdbEvtptClassList);
    __wdbEvtptDeleteAll   = _wdbEvtptDeleteAll;

    wdbSvcAdd (WDB_EVENTPOINT_ADD, wdbEvtptAdd, xdr_WDB_EVTPT_ADD_DESC, 
							xdr_UINT32);
    wdbSvcAdd (WDB_EVENTPOINT_DELETE, wdbEvtptDelete, xdr_WDB_EVTPT_DEL_DESC,
							xdr_void);
    wdbEvtptLibInitialized = TRUE;
    }

/*******************************************************************************
*
* wdbEvtptClassConnect - connect an eventpoint class to wdbEvtptAdd
*
* Connect an event classes eventpointAdd routine to wdbEvtptAdd.
* The class whose routine is called is based on the requested WDB_EVT_TYPE.
*/

void wdbEvtptClassConnect
    (
    WDB_EVTPT_CLASS *	pEvt
    )
    {
    dll_insert(&pEvt->evtptList, &wdbEvtptClassList);
    }

/*******************************************************************************
*
* wdbEvtptAdd - add an eventpoint to some detection mechanism.
*
* This routine checks all the services registered (in the wdbEvtptClassList),
* and calls the service which handles the requested WDB_EVT_TYPE.
*/

static UINT32 wdbEvtptAdd
    (
    WDB_EVTPT_ADD_DESC *	pEvtpt,	/* eventpoint to add */
    UINT32 *			pId	/* event-point ID returned */
    )
    {
    dll_t *	pDll;

    for (pDll = dll_head(&wdbEvtptClassList);
	 pDll != dll_end(&wdbEvtptClassList);
	 pDll = dll_next(pDll))
	{
	if (pEvtpt->evtType == EVTPT_BASE(pDll)->evtptType)
	    return ((*EVTPT_BASE(pDll)->evtptAdd)(pEvtpt, pId));
	}

    return (WDB_ERR_INVALID_EVENT);
    }

/******************************************************************************
*
* _wdbEvtptDeleteAll - delete all eventpoints in the system.
*/ 

static void _wdbEvtptDeleteAll (void)
    {
    dll_t *	pDll;
    TGT_ADDR_T	evtAllId = (TGT_ADDR_T)-1;	/* ID for "all eventpoints" */

    for (pDll = dll_head(&wdbEvtptClassList);
	 pDll != dll_end(&wdbEvtptClassList);
	 pDll = dll_next(pDll))
	{
	(*EVTPT_BASE(pDll)->evtptDel) (&evtAllId);
	}
    }

/*******************************************************************************
*
* wdbEvtptDelete - Delete an event point from the agent.
*
* pEvtptDel->evtptType is used to determine which event class's deletion
* routine to call.
*/

static UINT32 wdbEvtptDelete
    (
    WDB_EVTPT_DEL_DESC *	pEvtptDel	/* eventpoint to delete */
    )
    {
    dll_t *	pDll;

    for (pDll = dll_head(&wdbEvtptClassList);
	 pDll != dll_end(&wdbEvtptClassList);
	 pDll = dll_next(pDll))
	{
	if (pEvtptDel->evtType == EVTPT_BASE(pDll)->evtptType)
	    return ((*EVTPT_BASE(pDll)->evtptDel) (&pEvtptDel->evtptId));
	}

    return (WDB_ERR_INVALID_EVENTPOINT);
    }

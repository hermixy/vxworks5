/* wdbEvtLib.c - Event library for the WDB agent */

/* Copyright 1984-1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,27jan99,dbt  reworked dequeue mechanism (SPR #24654).
01d,26jan98,dbt  moved eventpoint handling to wdbEvtptLib.c
		 call event dequeue routine only if it is not NULL.
01c,31aug95,ms   events are now dequeued in FIFO order (SPR #4631)
01b,07jun95,ms   added _wdbEvtptDeleteAll
01a,01nov94,ms   written.
*/

/*
DESCRIPTION

This library provides a framework for asynchrous debugging events.

It provides a means for any library to send event data up to
the host. Many debugging facilities require that the host
be notified of some event occuring on the target at an unknow time in
the future. Examples would be breakpoints, exceptions, the return
of a function call, task exit, etc. The routine wdbEventPost() provides
a means for all of these libraries to notify the host of async events.
To post an event, a library must pass wdbEventPost() a static WDB_EVT_NODE
structure. This structure is initialized via wdbEventNodeInit(), and
should otherwise be treated as an abstract data type by other libraries.
*/

#include "vxWorks.h"
#include "wdb/dll.h"
#include "wdb/wdb.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbArchIfLib.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbEvtLib.h"

/* local variables */

static dll_t		wdbEventList;		/* events for the host */

/* forward declarations */

static BOOL	_wdbEventListIsEmpty (void);
static UINT32	wdbEventGet    (void *dummy,  WDB_EVT_DATA *pEvtData);

/******************************************************************************
*
* wdbEventLibInit - initialize the WDB event library.
*/

void wdbEventLibInit (void)
    {
    dll_init (&wdbEventList);
    __wdbEventListIsEmpty = _wdbEventListIsEmpty;

    wdbSvcAdd (WDB_EVENT_GET, wdbEventGet, xdr_void, xdr_WDB_EVT_DATA);
    }

/******************************************************************************
*
* wdbEventNodeInit - Initialize an event node.
*
* Each eventpoint library (breakpoints, async function calls, exceptions,
* etc.) uses WDB_EVT_NODE's to post asynchronous events to the host.
*
* A libary initializes an WDB_EVT_NODE, and then passes it to wdbEventPost()
* in order to post an event to the host.
* When the host is ready to recieve the event, the nodes getEvent() routine
* is called to upload the data.
* After the data has been uploaded, the nodes deq() routine is called
* to return the node to the poster. If the poster has more event data
* available, it can call wdbEventPost() again from within it's deq() routine.
*/

void wdbEventNodeInit
    (
    WDB_EVT_NODE *pEvtNode,		/* abstract data type */
    void        (*getEvent)		/* routine to upload event data */
		    (
		    void *arg,
		    WDB_EVT_DATA *pEvtMsg
		    ),
    void        (*deq)			/* called after data is uploaded */
		    (
		    void *arg
		    ),
    void *	arg			/* arg to pass to routines above */
    )
    {
    pEvtNode->getEvent	= getEvent;
    pEvtNode->deq	= deq;
    pEvtNode->arg	= arg;
    pEvtNode->onQueue	= FALSE;
    }

/******************************************************************************
*
* wdbEventPost - post an event.
*
* Add an event node to the event list, and notfiy the host.
* To save bandwidth, the host is only notified when the eventList
* first becomes non-empty.
*/

void wdbEventPost
    (
    WDB_EVT_NODE *	pEvtNode
    )
    {
    BOOL	notify;
    int		lockKey;

    lockKey = intLock();

    /* already on the queue? then return */

    if (pEvtNode->onQueue == TRUE)
        {
        intUnlock (lockKey);
        return;
        }

    notify		= wdbEventListIsEmpty();
    dll_insert (&pEvtNode->node, &wdbEventList);
    pEvtNode->onQueue	= TRUE;

    intUnlock (lockKey);

    if (notify)
	{
        wdbNotifyHost();
	}
    }

/******************************************************************************
*
* wdbEventDeq - dequeue an event from the event list.
*
* This routine can only be called from an agent RPC handler.
*
* RETURNS: OK, or ERROR if the node was not on the queue.
*/

STATUS wdbEventDeq
    (
    WDB_EVT_NODE *	pEvtNode
    )
    {
    int			lockKey;

    lockKey		= intLock();

    /* already off the queue? then return ERROR */

    if (pEvtNode->onQueue == FALSE)
	{
	intUnlock (lockKey);
	return (ERROR);
	}

    /* else dll_remove it */

    dll_remove (&pEvtNode->node);
    pEvtNode->onQueue	= FALSE;
    intUnlock (lockKey);

    /* call the nodes cleanup routine */

    if (*pEvtNode->deq != NULL)
	(*pEvtNode->deq) (pEvtNode->arg);

    return (OK);
    }

/******************************************************************************
*
* wdbEventGet - get an event from the agents event list.
*/

static UINT32 wdbEventGet
    (
    void  *		dummy,		/* ignored */
    WDB_EVT_DATA *	pEvtData	/* fill in this with the event data */
    )
    {
    uint_t	 	lockKey;
    WDB_EVT_NODE * 	pEventNode = NULL;

    lockKey = intLock();

    if (!dll_empty (&wdbEventList))
	{
	/* remove the event from the queue */

	pEventNode = (WDB_EVT_NODE *)dll_tail (&wdbEventList);
	dll_remove (&pEventNode->node);
	pEventNode->onQueue = FALSE;
	}

    intUnlock (lockKey);

    /* if the event list is empty return a WDB_EVT_NONE */

    if (pEventNode == NULL)
        {
        pEvtData->evtType = WDB_EVT_NONE;
        return (OK);
        }

    /* get event data from the event node */

    (*pEventNode->getEvent) (pEventNode->arg, pEvtData);

    /* call the nodes cleanup routine */

    if (*pEventNode->deq != NULL)
	(*pEventNode->deq) (pEventNode->arg);

    return (OK);
    }

/******************************************************************************
*
* _wdbEventListIsEmpty -
*/

static BOOL _wdbEventListIsEmpty (void)
    {
    return (dll_empty (&wdbEventList));
    }

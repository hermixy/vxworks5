/* wdbUserEvtLib.c - WDB user event library */

/* Copyright 1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,09feb99,fle  doc : put the code examples between .CS and .CE markups
01d,11jan99,dbt  use wdbSvcHookAdd() to free memory (fixed SPR #24323).
01c,09nov98,dbt  removed direct free() call.
01b,05oct98,jmp  doc: fixed DESCRIPTION section.
01a,25jan98,dbt  written.
*/

/*
DESCRIPTION
This library contains routines for sending WDB User Events.  The event
is sent through the WDB agent, the WDB communication link and the target
server to the host tools that have registered for it. The event received
by host tools will be a WTX user event string.

INCLUDE FILES: wdb/wdbLib.h

SEE ALSO:
.I "API Guide: WTX Protocol"
*/

#include "vxWorks.h"
#include "string.h"
#include "wdb/wdb.h"
#include "wdb/wdbLib.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbEvtLib.h"
#include "wdb/wdbRtIfLib.h"

/* data types */

typedef struct
    {
    WDB_EVT_NODE	eventNode;	/* event node */
    char *		pEvtData;	/* event to send through WDB */
    UINT32		numBytes;	/* size of the event */	
    } wdbUserEvtNode_t;

/* local variables */

LOCAL wdbUserEvtNode_t	externUserEvtNode;
LOCAL char		externUserData [WDB_MAX_USER_EVT_SIZE];

/* forward declarations */

static void wdbUserEvtGet (void * pNode, WDB_EVT_DATA * pWdbEvtData);
static void wdbUserEvtMemFree (wdbUserEvtNode_t * pUserEvtNode);

/******************************************************************************
*
* wdbUserEvtLibInit - include the WDB user event library 
*
* This null routine is provided so that wdbUserEvtLib can be linked into
* the system. If INCLUDE_WDB_USER_EVENT is defined in configAll.h,
* wdbUserEvtLibInit is called by the WDB config routine, wdbConfig(),
* in usrWdb.c.  
* 
* RETURNS: N/A 
*/

void wdbUserEvtLibInit (void)
    {
    }

/******************************************************************************
*
* wdbUserEvtPost - post a user event string to host tools.
* 
* This routine posts the string <event> to host tools that have registered 
* for it. Host tools will receive a USER WTX event string. The 
* maximum size of the event is WDB_MAX_USER_EVT_SIZE (defined in 
* $WIND_BASE/target/h/wdb/wdbLib.h).
*
* EXAMPLE
*
* The code below sends a WDB user event to host tools :
*
* .CS
*   char * message = "Alarm: reactor overheating !!!";
*
*   if (wdbUserEvtPost (message) != OK)
*       printf ("Can't send alarm message to host tools");
* .CE
*
* This event will be received by host tools that have registered for it.
* For example a WTX TCL based tool would do :
*
* .CS
*   wtxtcl> wtxToolAttach EP960CX
*   EP960CX_ps@sevre
*   wtxtcl> wtxRegisterForEvent "USER.*"
*   0
*   wtxtcl> wtxEventGet
*   USER Alarm: reactor overheating !!!
* .CE
*
* Host tools can register for more specific user events :
*
* .CS
*   wtxtcl> wtxToolAttach EP960CX
*   EP960CX_ps@sevre
*   wtxtcl> wtxRegisterForEvent "USER Alarm.*"
*   0
*   wtxtcl> wtxEventGet
*   USER Alarm: reactor overheating !!!
* .CE
*
* In this piece of code, only the USER events beginning with "Alarm"
* will be received.
*
* RETURNS:
* OK upon successful completion, a WDB error code if unable to send the
* event to the host or ERROR if the size of the event is greater
* than WDB_MAX_USER_EVT_SIZE.
*/

STATUS wdbUserEvtPost
    (
    char *	event		/* event string to send */
    )
    {
    wdbUserEvtNode_t *	pWdbUserEvtNode;	/* event node */
    char * 		pWdbUsrEvtData;		/* event data */
    UINT32		nBytes;			/* event size */

    nBytes = strlen (event) + 1; 

    if (nBytes > WDB_MAX_USER_EVT_SIZE)		/* check the event size */
	return (ERROR);

    if (wdbIsNowTasking())	/* tasking mode */
	{
	if ((pWdbRtIf->malloc == NULL) ||
	    (pWdbRtIf->free == NULL))
	    return (WDB_ERR_NO_RT_PROC);

	pWdbUserEvtNode = (wdbUserEvtNode_t *)(*pWdbRtIf->malloc)
					(sizeof (wdbUserEvtNode_t));
    
	if (pWdbUserEvtNode == NULL)
	    return (WDB_ERR_RT_ERROR);

	/* allocate room to copy user's data */

	if ((pWdbUsrEvtData = (*pWdbRtIf->malloc) (nBytes)) == NULL)
	    {
	    (*pWdbRtIf->free) (pWdbUserEvtNode);
	    return (WDB_ERR_RT_ERROR);
	    }
	}
    else			/* external mode */
	{
	pWdbUserEvtNode = &externUserEvtNode;
	pWdbUsrEvtData = externUserData;
	}

    /* fill pWdbUsrEvtData structure */

    strcpy (pWdbUsrEvtData, event);

    pWdbUserEvtNode->pEvtData = pWdbUsrEvtData;
    pWdbUserEvtNode->numBytes = nBytes;

    wdbEventNodeInit (&pWdbUserEvtNode->eventNode, wdbUserEvtGet, 
				    NULL, pWdbUserEvtNode);

    /* post the user event */

    wdbEventPost (&pWdbUserEvtNode->eventNode);

    return (OK);
    }

/******************************************************************************
*
* wdbUserEvtGet - fill in the WDB_EVT_DATA for the host.
*
* This routine fills the WDB_EVT_DATA structure for the host.
*
* RETURNS : NA
*
* NOMANUAL
*/ 

static void wdbUserEvtGet
    (
    void *		pNode,
    WDB_EVT_DATA *	pWdbEvtData	/* Event to send through WDB */
    )
    {
    wdbUserEvtNode_t *	pWdbUserEvtNode = pNode;
    WDB_MEM_XFER *	pUserEvtInfo;

    pUserEvtInfo = (WDB_MEM_XFER *)&pWdbEvtData->eventInfo.vioWriteInfo;

    pWdbEvtData->evtType	= WDB_EVT_USER;
    pUserEvtInfo->source	= pWdbUserEvtNode->pEvtData;
    pUserEvtInfo->destination	= 0;
    pUserEvtInfo->numBytes	= pWdbUserEvtNode->numBytes;

    wdbSvcHookAdd ((FUNCPTR) wdbUserEvtMemFree, (u_int) pWdbUserEvtNode);
    }

/******************************************************************************
*
* wdbUserEvtMemFree - free the memory used for the user event node.
*
* RETURNS : NA
*
* NOMANUAL
*/ 

static void wdbUserEvtMemFree
    (
    wdbUserEvtNode_t *	pUserEvtNode
    )
    {
    if (wdbIsNowTasking ())
	{
	(*pWdbRtIf->free) (pUserEvtNode->pEvtData);
	(*pWdbRtIf->free) (pUserEvtNode);
	}
    }

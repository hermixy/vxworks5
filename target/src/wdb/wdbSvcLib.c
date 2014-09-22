/* wdbSvcLib.c - maintain the set of services provided by the WDB agent */

/* Copyright 1984-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,11jan99,dbt  added a hook to call after the WDB request is handled (fixed
                 SPR #24323).
01f,12feb98,dbt  added a routine to unload all dynamically loaded services.
                 Fixed a problem in wdbSvcDispatch when the service is
                 unavailabe and the target has rebooted
01e,31aug95,ms   better handling of sequence number overflows (SPR #4498)
01d,20jun95,ms  handled sequence number overflow
01c,03jun95,ms  call wdbRpcNotifyConnect() on connection.
01b,07feb95,ms	added XPORT handle to dispacth routine.
01a,21sep94,ms  written.
*/

/*
DESCPRIPTION

This library is used to hold the current set of services supported
by the WDB agent. It provides scalability and extensibility.
*/

#include "wdb/wdb.h"
#include "wdb/wdbLib.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbEvtLib.h"
#include "string.h"

/* definitions */

#define MAX_RPC_PARAM_SIZE	200
#define NOT_CONNECTED		-1
#define SEQ_NUM_DELTA		100

/* local variables */

static WDB_SVC *	pWdbSvcArray;
static u_int		wdbSvcArraySize;
static u_int		wdbNumSvcs;
static FUNCPTR		wdbSvcHookRtn = NULL;
static u_int		wdbSvcHookRtnArg;

static u_int		wdbSeqNum = NOT_CONNECTED;

/******************************************************************************
*
* wdbSvcLibInit - initialize the library.
*/ 

void wdbSvcLibInit
    (
    WDB_SVC *	pArray,
    u_int	size
    )
    {
    pWdbSvcArray	= pArray;
    wdbSvcArraySize	= size;
    wdbSvcHookRtn	= NULL;
    wdbSeqNum		= NOT_CONNECTED;
    }

/******************************************************************************
*
* wdbSvcHookAdd - add a hook to be called when the service is handled
*
* This routine adds a hook to be called when the RPC service is handled
* (when the answer is sent). This can be used to free memory that was 
* allocated to send the answer.
*
* RETURNS: N/A
*
* NOMANUAL
*/ 

void wdbSvcHookAdd
    (
    FUNCPTR	hookRtn, /* routine to be called when a service is handled */
    u_int	arg	 /* argument of the routine */
    )
    {
    wdbSvcHookRtn	= hookRtn;
    wdbSvcHookRtnArg	= arg;
    }
/******************************************************************************
*
* wdbSvcGetSvc - get information about an RPC procedure number.
*
* RETURNS: Pointer to a WDB_SERVICE structure which contains
*      1) The address of the associated service function.
*      2) The address of the associated XDR input filter
*      3) The address of the associated XDR output filter.
* Or NULL if the procedure number is not supported.
*/

static WDB_SVC * wdbSvcGetSvc
    (
    u_int	procNum
    )
    {
    uint_t	i;

    for (i = 0; i < wdbSvcArraySize; i++)
	if (pWdbSvcArray[i].serviceNum >= procNum)
	    break;

    return (pWdbSvcArray[i].serviceNum == procNum ? &pWdbSvcArray[i] : NULL);
    }

/******************************************************************************
*
* wdbSvcAdd - Add an RPC service.
*
* RETURNS: OK or ERROR if the service table is full.
*/

STATUS wdbSvcAdd
    (
    uint_t	procNum,		/* procedure number */
    UINT32	(*serviceRtn)(),	/* function to call */
    BOOL	(*inProc)(),		/* XDR filter for parameters */
    BOOL	(*outProc)()		/* XDR filter for return value */
    )
    {
    int i;

    /* any more room left in service table ? */

    if (wdbNumSvcs >= wdbSvcArraySize)
        return (ERROR);

    /* We add the service to the array in sorted order */

    for (i = wdbNumSvcs; i > 0; i--)
	{
	if (pWdbSvcArray[i-1].serviceNum < procNum)
	    break;
	pWdbSvcArray[i].serviceNum = pWdbSvcArray[i-1].serviceNum;
	pWdbSvcArray[i].serviceRtn = pWdbSvcArray[i-1].serviceRtn;
	pWdbSvcArray[i].inProc     = pWdbSvcArray[i-1].inProc;
	pWdbSvcArray[i].outProc    = pWdbSvcArray[i-1].outProc;
	pWdbSvcArray[i].dynamic	   = pWdbSvcArray[i-1].dynamic;
	}

    pWdbSvcArray[i].serviceNum = procNum;
    pWdbSvcArray[i].serviceRtn = serviceRtn;
    pWdbSvcArray[i].inProc     = inProc;
    pWdbSvcArray[i].outProc    = outProc;

    /* 
     * We consider that all service added after the target server is connected
     * are dynamic services (loaded by the target server dynamically). Those
     * services must be removed when we disconnect from target server.
     */

    if (wdbTargetIsConnected ())
	pWdbSvcArray[i].dynamic	= TRUE;
    else
	pWdbSvcArray[i].dynamic	= FALSE;

    wdbNumSvcs ++;

    return (OK);
    }

/******************************************************************************
*
* wdbSvcDispatch - invoke the service associated with an RPC procedure number.
*/

void wdbSvcDispatch
    (
    WDB_XPORT * pXport,		/* RPC transport handle */
    uint_t	procNum		/* RPC procedure number */
    )
    {
    WDB_SVC *	pWdbService;		/* node for procedure lookup */
    UINT32	(*rout)();		/* service proc address */
    BOOL	(*inProc)();         	/* XDR input filter */
    BOOL	(*outProc)();        	/* XDR output filter */
    UINT32	args [MAX_RPC_PARAM_SIZE/4];	/* procedure args */
    UINT32	reply [MAX_RPC_PARAM_SIZE/4];	/* procedure reply */
    WDB_PARAM_WRAPPER paramWrapper;
    WDB_REPLY_WRAPPER replyWrapper;

    /* get the routine and XDR filters associated with the service number */

    pWdbService = wdbSvcGetSvc (procNum);

    if (pWdbService == NULL)
        {
	/* if we are not connected, reply SYSTEM_ERR */

	if (wdbSeqNum == NOT_CONNECTED)
	    wdbRpcReplyErr (pXport, SYSTEM_ERR);
	else
	    wdbRpcReplyErr (pXport, PROC_UNAVAIL);

        return;
        }

    rout	= pWdbService->serviceRtn;
    inProc	= pWdbService->inProc;
    outProc	= pWdbService->outProc;

    /* initialize the parameter and reply wrapper data */

    paramWrapper.pParams = args;
    paramWrapper.xdr     = inProc;

    replyWrapper.pReply  = reply;
    replyWrapper.xdr     = outProc;

    /* use the input wrapper to decode the sequence number and parameters */

    bzero ((caddr_t)args, MAX_RPC_PARAM_SIZE);
    if (!wdbRpcGetArgs (pXport, xdr_WDB_PARAM_WRAPPER, (char *)&paramWrapper))
	{
	wdbRpcReplyErr (pXport, GARBAGE_ARGS);
	return;
	}

    /* if this is a connection request, mark the agent as connected */

    if (procNum == WDB_TARGET_CONNECT)
        {
        wdbSeqNum	= paramWrapper.seqNum;
	wdbRpcNotifyConnect (pXport);
        }

    /* if we are not connected, reply SYSTEM_ERR */

    if (wdbSeqNum == NOT_CONNECTED)
        {
        wdbRpcReplyErr (pXport, SYSTEM_ERR);
        return;
        }

    /* if we are connected to another host, reply PROG_UNAVAIL */

    if ((paramWrapper.seqNum ^ wdbSeqNum) & WDB_HOST_ID_MASK)
        {
        wdbRpcReplyErr (pXport, PROG_UNAVAIL);
        return;
        }

    /* check if this is a duplicate of an old request */

    if (procNum != WDB_TARGET_CONNECT)
        {
        /*
         * If the last reply got lost, the host will resend
         * the request. In this case, we need to resend the reply without
         * reexecuting the routine.
         */

        if (paramWrapper.seqNum == wdbSeqNum)
            {
	    wdbRpcResendReply (pXport);
            return;
            }


        /*
         * Packets can arrive out of order with UDP. If an old request
         * arrives after a more recent request, it means that the host
         * has already forgotten about the old request. We should just
         * ignore the old request.
	 */

	/* ignore a smaller sequence number unless it is a wrap-around */

        if (paramWrapper.seqNum < wdbSeqNum)
            {
	    if (((wdbSeqNum & ~WDB_HOST_ID_MASK) < 0xffff - 100) ||
		((paramWrapper.seqNum & ~WDB_HOST_ID_MASK) > 100))
		return;
            }

	/* ignore a larger sequence number if it jumps by too much */

	if (paramWrapper.seqNum > wdbSeqNum + SEQ_NUM_DELTA)
	    {
	    return;
	    }
        }

    /* save away the sequence number */

    wdbSeqNum = paramWrapper.seqNum;

    /* invoke the service */

    replyWrapper.errCode = (*rout) (args, &reply);

    /*
     * The first word of the reply is always the errCode field.
     * One bit is reserved to let the host know that events are pending.
     */

    if (!wdbEventListIsEmpty())
	replyWrapper.errCode |= WDB_EVENT_NOTIFY;

    /* send the reply */

    wdbRpcReply (pXport, xdr_WDB_REPLY_WRAPPER, (char *)&replyWrapper);

    /* if this was a WDB_TARGET_DISCONNECT reset the sequence number */

    if (procNum == WDB_TARGET_DISCONNECT)
        {
        wdbSeqNum       = NOT_CONNECTED;
        }

    /* call the hook routine if it was initialized */

    if (wdbSvcHookRtn != NULL)
    	{
	wdbSvcHookRtn (wdbSvcHookRtnArg);
	wdbSvcHookRtn = NULL;		/* reset the hook */
	}
    }

/******************************************************************************
*
* wdbSvcDsaSvcRemove - Removed all dynamically loaded services.
*
* RETURNS: NA 
*/

void wdbSvcDsaSvcRemove (void)
    {
    int	i;
    int	count = 0;	/* number of services removed */

    for (i = 0; i < wdbNumSvcs; i++)
	{ 
	if (pWdbSvcArray[i].dynamic)
	    {
	    pWdbSvcArray[i].serviceNum	= 0;	/* unvalid entry */
	    count ++;
	    }
	else
	    /* fill table with valid entry */

	    pWdbSvcArray[i - count]	= pWdbSvcArray[i];
	}

    wdbNumSvcs = wdbNumSvcs - count;
    }

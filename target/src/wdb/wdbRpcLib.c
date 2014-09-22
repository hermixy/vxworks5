/* wdbRpcLib.c - RPC routines used by the WDB agent */

/* Copyright 1984-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,26may97,elp suppressed erroneous addition in numBytesOut reply computation
		(fix SPR 8356).
01c,15jun95,ms	reworked.
01b,03jun95,ms  added wdbRpcNotifyConnect to set event notify host.
		use new commIf structure (sendto/rcvfrom replaces read/write).
01a,06oct94,ms  written.
*/

/*
DESCRIPTION

This library contains routines for sending/receiving RPC messages.
It has been tested with the UDP backend to RPC.
It is possible that the format of an RPC send/reply message may
be different for a different protocol, in which case one would need
only to replace this module.
Also, if RPC authentication is added, this is the only module that
would need to be changed.
*/

#include "wdb/wdb.h"
#include "wdb/wdbRpcLib.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbCommIfLib.h"
#include "string.h"

/* Offsets within an RPC message */

	/* Offsets that apply to both incomming and outgoing messages */
#define RPC_XID		0
#define RPC_DIR		1

	/* Offsets for outgoing messages */
#define RPC_AUTH	2
#define RPC_STATUS	5
#define RPC_XDRS_OUT	6
#define RPC_REPLY_HDR_SIZE (RPC_XDRS_OUT * sizeof (int))

	/* Offsets for incoming messages */
#define RPC_VERS	2
#define RPC_PROG_NUM	3
#define RPC_VERS_NUM	4
#define RPC_PROC_NUM	5
#define RPC_XDRS_IN	10
#define RPC_RECV_HDR_SIZE (RPC_XDRS_IN * sizeof (int))

/******************************************************************************
*
* wdbRpcXportInit - initialize an RPC transport handle.
*/

void wdbRpcXportInit
    (
    WDB_XPORT *  pXport,	/* RPC transport handle */
    WDB_COMM_IF * pCommIf,	/* underlying communication interface */
    caddr_t 	  pInBuf,	/* input buffer */
    caddr_t 	  pOutBuf,	/* reply buffer */
    u_int 	  bufSize	/* buffer size */
    )
    {
    pXport->pCommIf	= pCommIf;
    xdrmem_create (&pXport->xdrs, 0, 0, 0);
    pXport->pInBuf	= (UINT32 *)pInBuf;
    pXport->numBytesIn	= 0;
    pXport->pOutBuf	= (UINT32 *)pOutBuf;
    pXport->numBytesOut	= 0;
    pXport->bufSize	= bufSize;
    pXport->xid		= 0;
    }

/******************************************************************************
*
* wdbRpcRcv - wait for an RPC with a timeout.
*
* RETURNS: TRUE if a message was received. FALSE on timeout.
*/

BOOL wdbRpcRcv
    (
    void *	     xportHandle,
    struct timeval * tv
    )
    {
    WDB_XPORT *pXport = (WDB_XPORT *)xportHandle;
    UINT32 *	pRpcMsg = pXport->pInBuf;

    pXport->numBytesIn = (*pXport->pCommIf->rcvfrom)(pXport->pCommIf->commId,
                           (caddr_t)pXport->pInBuf, wdbCommMtu,
			   &pXport->replyAddr, tv);

    /* check the message is for us */

    if ((pXport->numBytesIn < RPC_RECV_HDR_SIZE) ||
	(ntohl(pRpcMsg[RPC_PROG_NUM]) != WDBPROG) ||
	(ntohl(pRpcMsg[RPC_VERS_NUM]) != WDBVERS))
	{
	return (FALSE);
	}

    /* save away the xid */

    pXport->xid	= ntohl(pRpcMsg[RPC_XID]);

    /* call the RPC dispatch routine */

    wdbSvcDispatch (pXport, ntohl(pRpcMsg[RPC_PROC_NUM]));

    return (TRUE);
    }


/******************************************************************************
*
* wdbRpcReply - send an RPC reply.
*
* If the "pReply" parameter passed to us is NULL, it means that
* the XDR output stream in the output buffer has already been filled in
* by the service routine. We need just fill in the static
* header and call the drivers output routine.
*/

void wdbRpcReply
    (
    void *	xportHandle,	/* RPC transport handle */
    BOOL	(*xdrProc)(),	/* XDR output filter */
    caddr_t	reply		/* reply buffer */
    )
    {
    WDB_XPORT *pXport = (WDB_XPORT *)xportHandle;

    /* fill in the static part of the reply buffer */

    pXport->pOutBuf[RPC_XID]	= htonl(pXport->xid);
    pXport->pOutBuf[RPC_DIR]	= htonl(REPLY);
    pXport->pOutBuf[RPC_AUTH]	= htonl(MSG_ACCEPTED);
    pXport->pOutBuf[RPC_STATUS]	= htonl(SUCCESS);

    /*  XDR encode the reply */

    xdrmem_create (&pXport->xdrs, (caddr_t)pXport->pOutBuf,
		pXport->bufSize, XDR_ENCODE);

    XDR_SETPOS (&pXport->xdrs, RPC_REPLY_HDR_SIZE);

    if (!(*xdrProc)(&pXport->xdrs, reply))
	return;

    /* calculate the number of bytes sent */

    pXport->numBytesOut = XDR_GETPOS (&pXport->xdrs);

    /* send the reply */

    (*pXport->pCommIf->sendto)(pXport->pCommIf->commId,
		(caddr_t)pXport->pOutBuf, pXport->numBytesOut,
		&pXport->replyAddr);
    }

/******************************************************************************
*
* wdbRpcReplyErr - send an ERROR reply.
*/

void wdbRpcReplyErr
    (
    void *	xportHandle,	/* RPC transport handle */
    u_int	errStatus
    )
    {
    WDB_XPORT *pXport = (WDB_XPORT *)xportHandle;

    /* fill in the static part of the reply buffer */

    pXport->pOutBuf[RPC_XID]	= htonl(pXport->xid);
    pXport->pOutBuf[RPC_DIR]	= htonl(REPLY);
    pXport->pOutBuf[RPC_AUTH]	= htonl(MSG_ACCEPTED);
    pXport->pOutBuf[RPC_STATUS]	= htonl(errStatus);

    /* record some info so we can resend the reply if needed */

    pXport->numBytesOut	= RPC_REPLY_HDR_SIZE;

    /* send the reply */

    (*pXport->pCommIf->sendto)(pXport->pCommIf->commId,
		(caddr_t)pXport->pOutBuf, pXport->numBytesOut,
		&pXport->replyAddr);
    }

/******************************************************************************
*
* wdbRpcResendReply - resend the last reply.
*
* Resend the last reply using the current xid (from the last request).
*/

void wdbRpcResendReply
    (
    void *	xportHandle
    )
    {
    WDB_XPORT *pXport = (WDB_XPORT *)xportHandle;

    /* fill in the static part of the reply buffer */

    pXport->pOutBuf[RPC_XID]    = htonl(pXport->xid);
    pXport->pOutBuf[RPC_DIR]    = htonl(REPLY);
    pXport->pOutBuf[RPC_AUTH]   = htonl(MSG_ACCEPTED);
    pXport->pOutBuf[RPC_STATUS] = htonl(OK);

    /* send the reply */

    (*pXport->pCommIf->sendto)(pXport->pCommIf->commId,
                (caddr_t)pXport->pOutBuf, pXport->numBytesOut,
		&pXport->replyAddr);
    }

/******************************************************************************
*
* wdbRpcNotifyConnect - set address for future wdbRpcNotifyHost's
*
* wdbRpcReply() sends a reply to whomever sent the request.
* wdbRpcNotifyHost, on the other hand, sends data to the
* connected host. This routine allows us to set the address of the
* conected host.
*/ 

void wdbRpcNotifyConnect
    (
    void *      xportHandle	/* RPC transport handle */
    )
    {
    WDB_XPORT *pXport = (WDB_XPORT *)xportHandle;

    pXport->notifyAddr = pXport->replyAddr;
    }
 
/******************************************************************************
*
* wdbRpcNotifyHost - notify the host over the RPC channel.
*
* This routine sends an RPC packet that is not an RPC reply.
*/

void wdbRpcNotifyHost
    (
    void *      xportHandle	/* RPC transport handle */
    )
    {
    WDB_XPORT *pXport = (WDB_XPORT *)xportHandle;
    UINT32	rpcNotifyMsg [6];

    if (pXport->pCommIf->notifyHost != NULL)
	{
	(*pXport->pCommIf->notifyHost)(pXport->pCommIf->commId);
	return;
	}

    rpcNotifyMsg[RPC_XID]	= 0;
    rpcNotifyMsg[RPC_DIR]	= htonl(CALL);
    rpcNotifyMsg[RPC_AUTH]	= htonl(MSG_DENIED);
    rpcNotifyMsg[RPC_STATUS]	= htonl(SYSTEM_ERR);

    (*pXport->pCommIf->sendto)(pXport->pCommIf->commId,
		(caddr_t)rpcNotifyMsg, sizeof (rpcNotifyMsg),
		 &pXport->notifyAddr);
    }

/******************************************************************************
*
* wdbRpcGetArgs - get arguments from an RPC request.
*/

BOOL wdbRpcGetArgs
    (
    void *      xportHandle,	/* RPC transport handle */
    BOOL	(*xdrProc)(),	/* XDR input filter to apply */
    char *	args		/* where to place the arguments */
    )
    {
    WDB_XPORT *pXport = (WDB_XPORT *)xportHandle;

    xdrmem_create (&pXport->xdrs, (caddr_t)pXport->pInBuf,
		pXport->bufSize, XDR_DECODE);

    XDR_SETPOS (&pXport->xdrs, RPC_RECV_HDR_SIZE);

    return ((*xdrProc)(&pXport->xdrs, args));
    }


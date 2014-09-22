/* wtxexch.c - wtx message exchange implementation */

/* Copyright 1984-1996 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,28sep01,fle  SPR#28684 : added usePMap parameter to ExchangeCreate ()
                 routines
01d,11may99,fle  modified wtxExchangeFree since wtxRpcExchangeFree has been
		 modified  SPR 67367
01c,30sep96,elp  put in share, adapted to be compiled on target side SPR# 6775.
01b,10jul96,pad  changed order of include files.
01a,15may95,s_w  written. 
*/

/* includes */

#ifdef HOST
#include <stdlib.h>
#else
#include "stdlib.h"
#endif /* HOST */

#include "wtxexch.h"
#include "private/wtxexchp.h"

/* defines */

#define CHECK_HANDLE(handle, retVal) \
	if (handle == NULL || handle->self != handle) \
	    return retVal;

/* exported functions */

/*******************************************************************************
*
* wtxExchangeInitialize - initialize an exchange id
*
* This routine initailizes a handle pointed to buy <pXid> for use in
* further exchange requests.  When the handle is finished with is should be
* passed to wtxExchangeTerminate() to free any internal resources allocated.
* If this routine fails then the exchange id will not be set.
*
* RETURNS: WTX_OK, or WTX_ERROR
*
* NOMANUAL
*/
STATUS wtxExchangeInitialize 
    (
    WTX_XID * pXid		/* Pointer to exchange id to initialize */
    )
    {
    WTX_XID newXid;

    if (pXid == NULL)
	return WTX_ERROR;

    /* Allocate the new exchange struct */
    newXid = calloc (1, sizeof (_WTX_EXCHANGE));
    if (newXid == NULL)
	return WTX_ERROR;

    /* Set the self field for validation */
    newXid->self = newXid;
    newXid->timeout = 30000; /* Default 30 second timeout */

    *pXid = newXid;
    
    return WTX_OK;
    }

/*******************************************************************************
*
* wtxExchangeInstall - install exchange marshalling functions for handle
*
* This routine sets the exchange marshalling functions that define how
* the exchange mechanism behaves for that handle. The function pointer
* parameters <xCreate>, <xDelete>, <xExchange>, <xFree> and <xControl> 
* should have the same function prototype as wtxExchangeCreate(), 
* wtxExchangeDelete(), wtxExchange(), wtxExchangeFree() and
* wtxExchangeControl() respectively. 
*
* RETURNS: WTX_OK, or WTX_ERROR
*
* NOMANUAL
*/
STATUS wtxExchangeInstall 
    (
    WTX_XID xid,					/* exchange id */
    STATUS (*xCreate) (WTX_XID, const char *, BOOL),	/* create routine */
    STATUS (*xDelete) (WTX_XID),			/* delete routine */
    STATUS (*xExchange) (WTX_XID, WTX_REQUEST,		/* exchange routine */
			void *, void *), 	
    WTX_ERROR_T (*xFree) (WTX_REQUEST, void *),		/* free routine */
    STATUS (*xControl) (WTX_XID, UINT32, void *)	/* control routine */
    )
    {
    CHECK_HANDLE (xid, WTX_ERROR);

    xid->xCreate = (FUNCPTR) xCreate;
    xid->xDelete = (FUNCPTR) xDelete;
    xid->xExchange = (FUNCPTR) xExchange;
    xid->xFree = (FUNCPTR) xFree;
    xid->xControl = (FUNCPTR) xControl;

    return WTX_OK;
    }

/*******************************************************************************
*
* wtxExchangeErrGet - get the last error code reported for the exchange
*
* This routine returns the WTX error code corresponding to the last 
* exchange call that returned a WTX_ERROR status.  The error code is not
* valid if no routine has returned WTX_ERROR unless it has been cleared
* using wtxExchangeErrClear().
*
* RETURNS: WTX error code for handle.
*
* NOMANUAL
*/
WTX_ERROR_T wtxExchangeErrGet
    (
    WTX_XID xid		/* exchange id */
    )
    {
    CHECK_HANDLE (xid, WTX_ERR_EXCHANGE_INVALID_HANDLE);

    return xid->error;
    }

/*******************************************************************************
*
* wtxExchangeErrClear - clear the error code associated with the exchange
*
* This routine clears the error code for the exchange id <xid> so that
* wtxExchangeErrGet() will return WTX_ERR_NONE until an error occurs.
*
* RETURNS: WTX_OK, or WTX_ERROR
*
* NOMANUAL
*/
STATUS wtxExchangeErrClear
    (
    WTX_XID xid		/* exchange id */
    )
    {
    CHECK_HANDLE (xid, WTX_ERROR);

    xid->error = WTX_ERR_NONE;

    return WTX_OK;
    }

/*******************************************************************************
*
* wtxExchangeTerminate - terminate use of exchange id
*
* This routine performs all necessary cleanups on the exchange id <xid>
* and makes is invalid for further use.  This routine should always be 
* called when a handle is no longer needed.  Further use of the handle 
* after termination will probably result in WTX_ERR_EXCHANGE_INVALID_HANDLE
* errors or unpredictable system behaviour.
*
* RETURNS: WTX_OK, or WTX_ERROR
*
* NOMANUAL
*/
STATUS wtxExchangeTerminate
    (
    WTX_XID xid		/* exchange id */
    )
    {
    CHECK_HANDLE (xid, WTX_ERROR);

    xid->self = 0;

    free (xid);

    return WTX_OK;
    }
    
/*******************************************************************************
*
* wtxExchange - perform an exchange request
*
* This routine performs the exchange request <request> on the server that
* <xid> has been connected to using wtxExchangeCreate(). <pIn> and <pOut>
* point to message in and out args (ie. the request paramters and result)
* that are used in the call which must contain the wtxCore structure.
* 
* During the exchange request memory may be internally allocated to the
* result pointed to by <pOut> and this must be freed by the caller by
* calling wtxExchangeFree() except if the result of the call is an error.
* This is because the exchange mechanism detects transport and request
* errors and does any necessary deallocation implicitly.
*
* RETURNS: WTX_OK, or WTX_ERROR
*
* NOMANUAL
*/
STATUS wtxExchange 
    (
    WTX_XID	xid,		/* exchange id */
    WTX_REQUEST	request,	/* exchange request number */
    void *	pIn,		/* pointer to input arg message */
    void *	pOut		/* pointer to output arg (result) message */
    )
    {
    CHECK_HANDLE (xid, WTX_ERROR);
    
    if (xid->xExchange == NULL)
	WTX_EXCHANGE_RETURN (xid, WTX_ERR_EXCHANGE_MARSHALPTR, WTX_ERROR);

    return (*xid->xExchange) (xid, request, pIn, pOut);
    }

/*******************************************************************************
*
* wtxExchangeFree - free result of exchange request
*
* This routine frees an memory *internally* allocated to a result during
* an exchange request. It will not deallocate the message memory itself
* which should be done by the user.  The parameter <request> must 
* specify the exchange request number that the result was created by.
*
* RETURNS: WTX_OK, or WTX_ERROR
*
* NOMANUAL
*/

STATUS wtxExchangeFree 
    (
    WTX_XID	xid,			/* exchange id               */
    WTX_REQUEST	request,		/* WTX request number        */
    void *	pData			/* pointer to result message */
    )
    {
    WTX_ERROR_T	freeErr = WTX_ERR_NONE;	/* free function error code  */

    CHECK_HANDLE (xid, WTX_ERROR);

    /* check for the free function pointer */

    if (xid->xFree == NULL)
	WTX_EXCHANGE_RETURN (xid, WTX_ERR_EXCHANGE_MARSHALPTR, WTX_ERROR);

    freeErr = (*xid->xFree) (request, pData);
    if (freeErr != WTX_ERR_NONE)
	{
	WTX_EXCHANGE_RETURN (xid, freeErr, WTX_ERROR);
	}
    else
	{
	return (WTX_OK);
	}
    }

/*******************************************************************************
*
* wtxExchangeControl - perform miscellaneous control requests on exchange
*
* This routine performs control requests on an exchange id eg. setting
* the exchange transport timeout.  Not all requests will be supported by 
* all exchange implementations, eg. some transports may not support setting
* of a timeout, in which case the error WTX_ERR_EXCHANGE_REQUEST_UNSUPPORTED
* is reported.
*
* RETURNS: WTX_OK, or WTX_ERROR
*
* NOMANUAL
*/
STATUS wtxExchangeControl 
    (
    WTX_XID	xid,		/* exchange id */
    UINT32	ctlRequest,	/* control request number */
    void *	pArg		/* pointer to control request arg data */
    )
    {
    CHECK_HANDLE (xid, WTX_ERROR);

    if (xid->xControl == NULL)
	WTX_EXCHANGE_RETURN (xid, WTX_ERR_EXCHANGE_MARSHALPTR, WTX_ERROR);

    return (*xid->xControl) (xid, ctlRequest, pArg);
    }

/*******************************************************************************
*
* wtxExchangeCreate - connect the exchange id to a WTX server
*
* This routine makes a connection between an unconnected exchange
* handle and a WTX request server. The server is specified by the
* service identification key <key> which specifies the transport,
* server name and transport address for contacting it.  Server keys
* are obtained from the WTX registry which is a server itself. To
* avoid the user having to bootstrap the key mechanism the special
* NULL key value can be supplied which means "the default key for the
* registry service using the current transport mechanism". The current
* transport mechanism is defined by the marshalling routines installed
* by wtxExchangeInstall.
*
* RETURNS: WTX_OK, or WTX_ERROR
*
* NOMANUAL
*/

STATUS wtxExchangeCreate 
    (
    WTX_XID		xid,	/* exchange id */
    const char *	key,	/* server key string */
    BOOL		usePMap	/* shall we use portmapper ? */
    )
    {
    CHECK_HANDLE (xid, WTX_ERROR);

    if (xid->xCreate == NULL)
	WTX_EXCHANGE_RETURN (xid, WTX_ERR_EXCHANGE_MARSHALPTR, WTX_ERROR);

    return (*xid->xCreate) (xid, key, usePMap);
    }

/*******************************************************************************
*
* wtxExchangeDelete - disconnect the exchange id from a WTX server.
*
* This routine disconnects an exchange id from the currently
* attached WTX server so no more requests can be made.
*
* RETURNS: WTX_OK, or WTX_ERROR
*
* NOMANUAL
*/
STATUS wtxExchangeDelete 
    (
    WTX_XID xid		/* exchange id */
    )
    {
    CHECK_HANDLE (xid, WTX_ERROR);

    if (xid->xDelete == NULL)
	WTX_EXCHANGE_RETURN (xid, WTX_ERR_EXCHANGE_MARSHALPTR, WTX_ERROR);

    return (*xid->xDelete) (xid);
    }

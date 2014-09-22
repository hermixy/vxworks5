/* wdbVioLib.c - virtual I/O library for WDB */

/* Copyright 1984-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,25may95,ms	 included intLib.h, fixed invalid VIO channel detection.
01b,24may95,ms   added wdbVioChannelUnregister().
01a,08nov94,ms   written.
*/

/*
DESCRIPTION

*/

#include "wdb/wdb.h"
#include "wdb/dll.h"
#include "intLib.h"
#include "wdb/wdbEvtLib.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbVioLib.h"

/* local variables */

static dll_t	wdbVioDevList;

/* forward declarations */

static UINT32 wdbVioWrite (WDB_MEM_XFER *pMemXfer, UINT32 *pBytesWriten);

/******************************************************************************
*
* wdbVioLibInit - initialize the VIO library
*/

void wdbVioLibInit (void)
    {
    dll_init (&wdbVioDevList);

    wdbSvcAdd (WDB_VIO_WRITE, wdbVioWrite, xdr_WDB_MEM_XFER, xdr_UINT32);
    }

/******************************************************************************
*
* wdbVioChannelUnregister - unregister a VIO channel.
*/ 

void wdbVioChannelUnregister
    (
    WDB_VIO_NODE *	pVioNode
    )
    {
    int lockKey;

    lockKey = intLock();
    dll_remove (&pVioNode->node);
    intUnlock (lockKey);
    }

/******************************************************************************
*
* wdbVioChannelRegister - register a VIO channel.
*
* RETURNS: OK, or ERROR if the VIO channel is already registered.
*/

STATUS wdbVioChannelRegister
    (
    WDB_VIO_NODE *	pVioNode
    )
    {
    WDB_VIO_NODE *	pThisNode;
    int lockKey;

    for (pThisNode =  (WDB_VIO_NODE *)dll_head (&wdbVioDevList);
	 pThisNode != (WDB_VIO_NODE *)dll_end  (&wdbVioDevList);
	 pThisNode =  (WDB_VIO_NODE *)dll_next (&pThisNode->node)
	)
	{
	if (pThisNode->channel == pVioNode->channel)
	    return (ERROR);
	}

    lockKey = intLock();
    dll_insert (&pVioNode->node, &wdbVioDevList);
    intUnlock (lockKey);

    return (OK);
    }

/******************************************************************************
*
* wdbVioWrite - RPC to put data in a VIO device (from the host to the target).
*/

static UINT32 wdbVioWrite
    (
    WDB_MEM_XFER *	pMemXfer,
    UINT32 *		pBytesWritten
    )
    {
    WDB_VIO_NODE *	pThisNode;

    for (pThisNode =  (WDB_VIO_NODE *)dll_head (&wdbVioDevList);
	 pThisNode != (WDB_VIO_NODE *)dll_end  (&wdbVioDevList);
	 pThisNode =  (WDB_VIO_NODE *)dll_next (&pThisNode->node)
	)
	{
	if (pThisNode->channel == (UINT32)pMemXfer->destination)
	    {
	    *pBytesWritten = (*pThisNode->inputRtn)(pThisNode,
				    pMemXfer->source,
				    pMemXfer->numBytes);
	    return (OK);
	    }
	}

    return (WDB_ERR_INVALID_VIO_CHANNEL);
    }


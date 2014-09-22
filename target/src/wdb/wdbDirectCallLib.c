/* wdbDirectCallLib.c - Call a function on the target */

/* Copyright 1984-1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,04sep98,cdp enable Thumb support for all ARM CPUs with ARM_THUMB==TRUE.
01b,11jul97,cdp added Thumb (ARM7TDMI_T) support.
01a,18jan95,ms  written.
*/

/*
DESCPRIPTION

This library contains the RPC to call a function on the target.
The return value of the function is passed to the host.
The function is called in the agents context (unlike wdbCallLib,
which calls functions in an external context), so care must be taken
when using this procedure.
*/

#include "wdb/wdb.h"
#include "wdb/wdbSvcLib.h"

/* forward declarations */

static UINT32 wdbDirectCall (WDB_CTX_CREATE_DESC *pCtxCreate, UINT32 *pVal);

/******************************************************************************
*
* wdbDirectCallLibInit -
*/

void wdbDirectCallLibInit (void)
    {
    wdbSvcAdd (WDB_DIRECT_CALL, wdbDirectCall, xdr_WDB_CTX_CREATE_DESC, xdr_UINT32);
    }

/******************************************************************************
*
* wdbDirectCall - RPC routine to directly call a function in the agents context
*/

static UINT32 wdbDirectCall
    (
    WDB_CTX_CREATE_DESC *	pCtxCreate,
    UINT32 *		pVal
    )
    {
    UINT32	(*proc)();	/* function to call */

#if ((CPU_FAMILY == ARM) && ARM_THUMB)
    proc	= (UINT32 (*)())(pCtxCreate->entry | 1);
#else /* CPU_FAMILY == ARM */
    proc	= (UINT32 (*)())pCtxCreate->entry;
#endif /* CPU_FAMILY == ARM */

    *pVal		= (*proc)(pCtxCreate->args[0], pCtxCreate->args[1],
			pCtxCreate->args[2], pCtxCreate->args[3],
			pCtxCreate->args[4], pCtxCreate->args[5],
			pCtxCreate->args[6], pCtxCreate->args[7],
			pCtxCreate->args[8], pCtxCreate->args[9]);

    return (OK);
    }


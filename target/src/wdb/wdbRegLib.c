/* wdbRegLib.c - register manipulation library for the wdb agent */

/* Copyright 1984-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,25may95,ms	minor tweek.
01b,31jan95,ms  fixed wdbRegsGet routine for system registers.
01a,29oct94,ms  written.
*/

/*
DESCPRIPTION

This library contains the RPCs to get and set a contexts registers.
*/

#include "wdb/wdb.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbRtIfLib.h"
#include "string.h"

/* forward declarations */

static UINT32 wdbRegGet (WDB_REG_READ_DESC *pRegRead, WDB_MEM_XFER *pMemXfer);
static UINT32 wdbRegSet (WDB_REG_WRITE_DESC *pRegWrite);

/******************************************************************************
*
* wdbRegsLibInit -
*/

void wdbRegsLibInit (void)
    {
    wdbSvcAdd (WDB_REGS_GET, wdbRegGet, xdr_WDB_REG_READ_DESC, xdr_WDB_MEM_XFER);
    wdbSvcAdd (WDB_REGS_SET, wdbRegSet, xdr_WDB_REG_WRITE_DESC, xdr_void);
    }

/******************************************************************************
*
* wdbRegGet - get a contexts registers.
*/

static UINT32 wdbRegGet
    (
    WDB_REG_READ_DESC *	pRegRead,
    WDB_MEM_XFER *	pMemXfer
    )
    {
    int		status;
    char *	pRegs;

    /* use wdb call to get pointer to system registers */

    if (pRegRead->context.contextType == WDB_CTX_SYSTEM)
        {
	status = wdbExternRegsGet (pRegRead->regSetType, &pRegs);
        }

    /* use run-time callout to get pointer to task registers */

    else
	{
	if (pWdbRtIf->taskRegsGet == NULL)
	    return (WDB_ERR_NO_RT_PROC);
	
	status = (*pWdbRtIf->taskRegsGet) (&pRegRead->context,
					pRegRead->regSetType,
					&pRegs);
	}

    if (status == ERROR)
	return (WDB_ERR_INVALID_PARAMS);

    pMemXfer->numBytes		= pRegRead->memRegion.numBytes;
    pMemXfer->source		= &pRegs[(int)pRegRead->memRegion.baseAddr];
    return (OK);
    }

/******************************************************************************
*
* wdbRegSet - set a contexts registers.
*/

static UINT32 wdbRegSet
    (
    WDB_REG_WRITE_DESC *	pRegWrite
    )
    {
    int         status;
    char *	pRegs;

    /* get old regs */

    if (pRegWrite->context.contextType == WDB_CTX_SYSTEM)
        {
	status = wdbExternRegsGet (pRegWrite->regSetType, &pRegs);
	}
    else
	{
	if ((pWdbRtIf->taskRegsSet == NULL) || (pWdbRtIf->taskRegsGet == NULL))
            return (WDB_ERR_NO_RT_PROC);

	status = (*pWdbRtIf->taskRegsGet) (&pRegWrite->context,
                                        pRegWrite->regSetType,
                                        &pRegs);
	}

    if (status == ERROR)
        return (WDB_ERR_INVALID_PARAMS);

    /* overwrite some of the regs */

    bcopy ((caddr_t)pRegWrite->memXfer.source,
	   (caddr_t)&pRegs[(int)pRegWrite->memXfer.destination],
	   pRegWrite->memXfer.numBytes);

    /* set the regs */

    if (pRegWrite->context.contextType == WDB_CTX_SYSTEM)
	{
	status = wdbExternRegsSet (pRegWrite->regSetType, pRegs);
	}
    else
	{
	status = (*pWdbRtIf->taskRegsSet) (&pRegWrite->context,
                                        pRegWrite->regSetType,
                                        pRegs);
	}

    if (status == ERROR)
        return (WDB_ERR_INVALID_PARAMS);

    return (OK);
    }


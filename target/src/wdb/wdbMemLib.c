/* wdbMemLib.c - WDB memory services */

/* Copyright 1984-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,05feb98,dbt  moved wdbMemRead and wdbMemProtect to wdbMemCoreLib.c (needed
                 by dynamic scalable agent).
01g,02oct96,elp  added casts due to TGT_ADDR_T type change in wdb.h.
01f,29feb96,ms  fixed WDB_MEM_READ on a register (SPR 6092).
01e,19jun95,tpr divided in two files wdbMemLib.c  and wdbMemCoreLib.c.
01d,07jun95,ms	wdbMemScan sets WDB_ERR_NOT_FOUND instead of returning -1
01c,14mar95,p_m	fixed checksum calculation according to RFC 1071.
01b,01mar95,ms	changed chksum, added memMove and cacheUpdate.
01a,21sep94,ms  written.
*/

/*
DESCPRIPTION

This library contains the RPC routines to manipulate the targets memory.
*/

#include "wdb/wdb.h"
#include "wdb/wdbArchIfLib.h"
#include "wdb/wdbRtIfLib.h"
#include "wdb/wdbLib.h"
#include "wdb/wdbSvcLib.h"
#include "string.h"

extern STATUS wdbMemTest (char * addr, u_int nBytes, u_int accessType);

/* forward declarations */

static UINT32 wdbMemMove	(WDB_MEM_REGION *pMemRegion);
static UINT32 wdbMemScan	(WDB_MEM_SCAN_DESC *pMemScan, TGT_ADDR_T *val);

/******************************************************************************
*
* wdbMemLibInit - install the agent memory services.
*/

void wdbMemLibInit (void)
    {
    wdbSvcAdd (WDB_MEM_MOVE,	wdbMemMove,    xdr_WDB_MEM_REGION, xdr_void);
    wdbSvcAdd (WDB_MEM_SCAN,	wdbMemScan,	xdr_WDB_MEM_SCAN_DESC,
								xdr_TGT_ADDR_T);
    }

/******************************************************************************
*
* wdbMemMove - move a block of target memory.
*
* pMemRegion->param contains the destination address.
*/

static UINT32 wdbMemMove
    (
    WDB_MEM_REGION * 	pMemRegion		/* region of memory to move */
    )
    {
    char bb;		/* for memProbe() */

    if (wdbMemTest ((char *)pMemRegion->baseAddr, pMemRegion->numBytes,
			VX_READ) != OK)
	goto error;

    if (pMemRegion->numBytes == 0)
	return (OK);

    /* 
     * Normally source and destination blocks don't overlap, so we
     * can just probe the destination for VX_WRITE to see if it is safe.
     * If they overlap, the we have to make sure that the probe character
     * written is the same as what is already there (else we'd mess up the
     * source block).
     */

    if ((pMemRegion->param > (UINT32)pMemRegion->baseAddr) &&
	(pMemRegion->param < 
			(UINT32)pMemRegion->baseAddr + pMemRegion->numBytes))
	bb = *(char *)(pMemRegion->param);

    if ((*pWdbRtIf->memProbe) ((char *)pMemRegion->param, VX_WRITE,
		1, &bb) != OK)
	goto error;

    if ((pMemRegion->param + pMemRegion->numBytes >
			(UINT32)pMemRegion->baseAddr) &&
	(pMemRegion->param + pMemRegion->numBytes <
			(UINT32)pMemRegion->baseAddr + pMemRegion->numBytes))
	bb = *(char *)(pMemRegion->param + pMemRegion->numBytes - 1);

    if ((*pWdbRtIf->memProbe) ((char *)pMemRegion->param + pMemRegion->numBytes
		- 1, VX_WRITE, 1, &bb) != OK)
	goto error;

    memmove ((char *)pMemRegion->param, (char *)pMemRegion->baseAddr,
			pMemRegion->numBytes);
    return (OK);

error:
    return (WDB_ERR_MEM_ACCES);
    }

/******************************************************************************
*
* wdbMemScan - scan memory for a pattern
*
* if (pMemScan->memXfer.numBytes > 0) search forwards. Otherwise
* a backwards search is performed.
*
* if (pMemScan->memRegion.param == 0), we search for the pattern
* pMemScan->memXfer.source. Otherwise we search until we don't match
* the pattern.
*
* On return, val is set to the address on which the match was found.
*
* RETURNS: OK, WDB_ERR_MEM_ACCES, or WDB_ERR_NOT_FOUND
*/ 

static UINT32 wdbMemScan
    (
    WDB_MEM_SCAN_DESC *	pMemScan,
    TGT_ADDR_T	*	val
    )
    {
    char *	pattern	= pMemScan->memXfer.source;
    int   	size	= pMemScan->memXfer.numBytes;
    char *	start;			/* first address to check */
    char *	end;			/* last address to check */
    char *	addr;			/* current address to check */
    int		step;			/* address increment */

    if (wdbMemTest ((char *)pMemScan->memRegion.baseAddr,
		    pMemScan->memRegion.numBytes, VX_READ) != OK)
        return (WDB_ERR_MEM_ACCES);

    if (pMemScan->memRegion.numBytes > 0)
	{
	start	= (char *)pMemScan->memRegion.baseAddr;
	end	= start + pMemScan->memRegion.numBytes - size + 1;
	step	= 1;
	if (start > end)
	    return (WDB_ERR_NOT_FOUND);
	}
    else
	{
	start	= (char *)pMemScan->memRegion.baseAddr - size;
	end	= start + pMemScan->memRegion.numBytes - 1;
	step	= -1;
	if (start < end)
	    return (WDB_ERR_NOT_FOUND);
	}

    /* scan for a match */

    if (pMemScan->memRegion.param == 0)
	{
	for (addr = start; addr != end; addr += step)
	    {
	    if (memcmp (addr, pattern, size) == 0)
		{
		*val = (TGT_ADDR_T)addr;
		return (OK);
		}
	    }
	}

    /* scan for a mismatch */

    else
	{
	for (addr = start; addr != end; addr += step)
	    {
	    if (memcmp (addr, pattern, size) != 0)
		{
		*val = (TGT_ADDR_T)addr;
		return (OK);
		}
	    }
	}

    return (WDB_ERR_NOT_FOUND);
    }


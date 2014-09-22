/* wdbMemCoreLib.c - WDB memory services */

/* Copyright 1984-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,05feb98,dbt  added wdbMemRead() and wdbMemProtect().
01d,02oct96,elp  added casts due to TGT_ADDR_T type change in wdb.h.
01c,29feb96,ms  fixed WDB_MEM_WRITE on a register (SPR 6092).
01b,22aug95,ms	wdbMemTxtUpdate now returns OK if no run-time callout present
01a,19jun95,tpr written, inspired from wdbMemLib.c version 01d.
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

/* forward declarations */

static UINT32 wdbMemWrite	(WDB_MEM_XFER *  pMemXfer);
static UINT32 wdbMemFill	(WDB_MEM_REGION *pMemRegion);
static UINT32 wdbMemChecksum	(WDB_MEM_REGION *pMemRegion, UINT32 *cksum);
static UINT32 wdbMemTxtUpdate	(WDB_MEM_REGION *pMemRegion);
static UINT32 wdbMemRead	(WDB_MEM_REGION *pMemRegion,
						WDB_MEM_XFER *pMemXfer);
static UINT32 wdbMemProtect	(WDB_MEM_REGION *pMemRegion);

/******************************************************************************
*
* wdbMemCoreLibInit - install the agent memory services.
*/

void wdbMemCoreLibInit (void)
    {
    wdbSvcAdd (WDB_MEM_WRITE,	wdbMemWrite,   xdr_WDB_MEM_XFER, xdr_void);
    wdbSvcAdd (WDB_MEM_FILL,	wdbMemFill,    xdr_WDB_MEM_REGION, xdr_void);
    wdbSvcAdd (WDB_MEM_CHECKSUM, wdbMemChecksum, xdr_WDB_MEM_REGION,
                						xdr_UINT32);
    wdbSvcAdd (WDB_MEM_CACHE_TEXT_UPDATE,
				wdbMemTxtUpdate, xdr_WDB_MEM_REGION, xdr_void);

    wdbSvcAdd (WDB_MEM_READ,	wdbMemRead,    xdr_WDB_MEM_REGION,
							xdr_WDB_MEM_XFER);

    wdbSvcAdd (WDB_MEM_PROTECT,	wdbMemProtect, xdr_WDB_MEM_REGION, xdr_void);
    }

/******************************************************************************
*
* wdbMemTest - test memory for valid access.
*
* This test a range of addresses.
*/

STATUS wdbMemTest
    (
    char *	addr,
    u_int	nBytes,
    u_int	accessType		/* VX_WRITE or VX_READ */
    )
    {
    char        bb;	/* bit bucket for memProbe() */

    if (nBytes == 0)
	return (OK);

    if (((*pWdbRtIf->memProbe) (addr, accessType, 1, &bb) != OK) ||
        ((*pWdbRtIf->memProbe) (addr + nBytes - 1, accessType, 1, &bb)
                        != OK))
	return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* wdbCksum - returns the ones compliment checksum of some memory.
*
* This routine is also used by the WDB UDP/IP module (which needs to do
* a ~htons(wdbCksum())) to get it right).
*
* NOMANUAL
*/

UINT32 wdbCksum
    (
    uint16_t *	pAddr,			/* start of buffer */
    int 	len			/* size of buffer in bytes */
    )
    {
    UINT32 	sum = 0;
    BOOL	swap = FALSE;

    /* take care of unaligned buffer address */

    if ((UINT32)pAddr & 0x1)
	{
	sum += *((unsigned char *) pAddr) ++;
	len--;
	swap = TRUE;
	}

    /* evaluate checksum */

    while (len > 1)
	{
	sum += *(uint16_t *) pAddr ++;
	len -= 2;
	}

    /* take care of last byte */

    if (len > 0)
	sum = sum + ((*(unsigned char *) pAddr) << 8);

    /* fold to 16 bits */

    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);

    /* swap if we started on unaligned address */

    if (swap)
	sum = ((sum & 0x00ff) << 8) | ((sum & 0xff00) >> 8);

    return (sum);
    }

/******************************************************************************
*
* wdbMemWrite - write target memory.
*
* pMemRegion->param is unused.
*/

static UINT32 wdbMemWrite
    (
    WDB_MEM_XFER *	pMemXfer	/* memory to transfer */
    )
    {
    char *dest    = (char *)pMemXfer->destination;
    int  numBytes = pMemXfer->numBytes;

    if (((numBytes == 1) || (numBytes == 2) || (numBytes == 4)) &&
        (((int)dest % numBytes) == 0))
        {
        if (pWdbRtIf->memProbe (dest, VX_WRITE, numBytes, pMemXfer->source) != OK)
            return (WDB_ERR_MEM_ACCES);

        return (OK);
        }

    if (wdbMemTest ((char *)pMemXfer->destination, pMemXfer->numBytes,
			VX_WRITE) != OK)
	return (WDB_ERR_MEM_ACCES);

    bcopy (pMemXfer->source, (char *)pMemXfer->destination, pMemXfer->numBytes);
    return (OK);
    }

/******************************************************************************
*
* wdbMemFill - fill target memory with some pattern.
*
* pMemRegion->param contains the fill pattern
*/

static UINT32 wdbMemFill
    (
    WDB_MEM_REGION * 	pMemRegion	/* region to fill + pattern */
    )
    {
    int 	ix;
    char *	startAddr = (char *)pMemRegion->baseAddr;
    u_int	numBytes  = pMemRegion->numBytes;
    UINT32	pattern   = ntohl (pMemRegion->param);
    u_int	nMisaligned;

    if (wdbMemTest ((char *)pMemRegion->baseAddr, pMemRegion->numBytes,
			VX_WRITE) != OK)
	return (WDB_ERR_MEM_ACCES);

    /* copy any misaligned bytes at the begining */

    nMisaligned = (4 - ((u_int)startAddr & 0x3)) & 0x3;
    if (nMisaligned > numBytes)
	nMisaligned = numBytes;
    bcopy ((char *)&pattern, startAddr, nMisaligned);
    startAddr += nMisaligned;
    numBytes  -= nMisaligned;
    pattern   =  ((pattern << (8 * nMisaligned)) |
		  (pattern >> (8 * (4 - nMisaligned))));

    /* copy any misaligned bytes at the end */

    nMisaligned = (numBytes & 0x3);
    bcopy ((char *)&pattern, startAddr + numBytes - nMisaligned, nMisaligned);
    numBytes    -= nMisaligned;

    /* copy the bulk of the pattern 4 bytes at a time */

    for (ix = 0; ix < numBytes; ix += 4)
	{
	*(UINT32 *)(startAddr + ix) = pattern;
	}

    return (OK);
    }

/******************************************************************************
*
* wdbMemChecksum - checksum a block of memory.
*
* pMemRegion->param is unused.
*/

static UINT32 wdbMemChecksum
    (
    WDB_MEM_REGION *	pMemRegion,		/* region to checksum */
    UINT32 *		val
    )
    {
    if (wdbMemTest ((char *)pMemRegion->baseAddr, pMemRegion->numBytes,
			VX_READ) != OK)
	return (WDB_ERR_MEM_ACCES);

    *val = ~wdbCksum ((uint16_t *)pMemRegion->baseAddr, pMemRegion->numBytes);

    return (OK);
    }

/******************************************************************************
*
* wdbMemTxtUpdate - make data and instruction cache coherent after loading
*		    text to (data) memory.
*
* pMemRegion->param is unused.
*/

static UINT32 wdbMemTxtUpdate
    (
    WDB_MEM_REGION * 	pMemRegion	/* region to do */
    )
    {
    if (pWdbRtIf->cacheTextUpdate == NULL)
	return (OK);

    if (wdbMemTest ((char *)pMemRegion->baseAddr, pMemRegion->numBytes,
		    VX_READ) != OK)
	return (WDB_ERR_MEM_ACCES);

    (*pWdbRtIf->cacheTextUpdate) ((char *)pMemRegion->baseAddr,
			pMemRegion->numBytes);

    return (OK);
    }

/******************************************************************************
*
* wdbMemRead - read target memory.
*
* pMemRegion->param is unused.
*/

static UINT32 wdbMemRead
    (
    WDB_MEM_REGION *	pMemRegion,	/* region to read */
    WDB_MEM_XFER *	pMemXfer	/* transfer data up to the host */
    )
    {
    char *baseAddr = (char *)pMemRegion->baseAddr;
    int  numBytes  = pMemRegion->numBytes;
    static int buf;

    if (((numBytes == 1) || (numBytes == 2) || (numBytes == 4)) &&
	(((int)baseAddr % numBytes) == 0))
	{
	if (pWdbRtIf->memProbe (baseAddr, VX_READ, numBytes, (char *)&buf)
						!= OK)
	    return (WDB_ERR_MEM_ACCES);

	pMemXfer->source	= (char *)&buf;
	pMemXfer->numBytes	= numBytes;
	return (OK);
	}

    if (wdbMemTest (baseAddr, numBytes, VX_READ) != OK)
	return (WDB_ERR_MEM_ACCES);

    pMemXfer->source		= baseAddr;
    pMemXfer->numBytes		= numBytes;
    return (OK);
    }

/******************************************************************************
*
* wdbMemProtect - write (un)protect a region of memory.
*
* This routine modifies the write protection state of pMemRegion->numBytes
* bytes of memory starting at pMemRegion->baseAddr.
* If pMemRegion->param != 0, then the memory is write protected,
* if pMemRegion->param == 0, then the memory is un-write-protected.
*
* RETURNS: OK on success,
*	   WDB_ERR_NO_RT_PROC if no runtime routine is installed, or
*	   WDB_ERR_MEM_ACCES if the runtime routine fails.
*/

static UINT32 wdbMemProtect
    (
    WDB_MEM_REGION * 	pMemRegion		/* region to (un)protect */
    )
    {
    if (pWdbRtIf->memProtect == NULL)
	return (WDB_ERR_NO_RT_PROC);

    if ((*pWdbRtIf->memProtect)((char *)pMemRegion->baseAddr,
				 pMemRegion->numBytes,
				 pMemRegion->param) != OK)
	return (WDB_ERR_MEM_ACCES);

    return (OK);
    }

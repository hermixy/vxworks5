/* cacheArchLib.c - I80X86 cache management library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01r,15apr02,pai  Removed sysCpuProbe() call, as the CPU probe is separated
                 from the cache library configuration (SPR 74951).
01q,07mar02,hdn  added CLFLUSH macros.  cleaned up (spr 73360)
01p,04dec01,hdn  made SNOOP_ENABLE dataMode independent of CPU type
		 added cacheArchDmaMallocSnoop() and cacheArchDmaFreeSnoop()
01o,20nov01,hdn  doc clean up for 5.5
01n,02nov01,hdn  enabled SSE and SSE2.
01m,23aug01,hdn  added PENTIUM4 support
01l,15feb99,hdn  added support for PentiumPro's bus snoop.
01k,09apr98,hdn  added support for PentiumPro.
01j,17jun96,hdn  stopped to use a return value of sysCpuProbe().
01i,21sep95,hdn  added support for NS486.
01h,01nov94,hdn  added a check of sysProcessor for Pentium
01g,27oct94,hdn  cleaned up.
01f,04oct94,hdn  added a checking and setting the cache mode.
01e,29may94,hdn  changed a macro I80486 to sysProcessor.
01d,05nov93,hdn  added cacheArchFlush().
01c,30jul93,hdn  added cacheArchDmaMalloc(), cacheArchDmaFree().
01b,27jul93,hdn  added cacheArchClearEntry().
01a,08jun93,hdn  written.
*/

/*
DESCRIPTION
This library contains architecture-specific cache library functions for
the Intel 80X86 family caches.
The 386 family does not have a cache, though it has the prefetch queue.  
The 486 family includes a unified L1 cache for both instructions and data.
The P5(Pentium) family includes separate L1 instruction and data caches.  
The data cache supports a writeback or writethrough update policy.
The P6(PentiumPro, II, III) family includes a separate L1 instruction and 
data caches, and unified internal L2 cache.  The MESI cache protocol 
maintains consistency in the L1 and L2 caches in both uni-processor and
multi-processor system.  The P7(Pentium4) family includes the trace cache 
that caches decoded instructions, L1 data cache and L2 unified cache.  
The CLFLUSH instruction allows selected cache line to be flushed from 
memory.

INCLUDE FILES: cacheLib.h

SEE ALSO: cacheLib, vmLib, Intel Architecture Software Developer's Manual

*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "errnoLib.h"
#include "cacheLib.h"
#include "stdlib.h"
#include "private/memPartLibP.h"
#include "private/vmLibP.h"
#include "private/funcBindP.h"
#include "regs.h"


/* externals */

IMPORT UINT sysProcessor;
IMPORT CPUID sysCpuId;


/* globals */

int cacheFlushBytes = CLFLUSH_DEF_BYTES; /* def bytes flushed by CLFLUSH */


/* forward declarations */

LOCAL void * cacheArchDmaMallocSnoop	(size_t bytes);
LOCAL STATUS cacheArchDmaFreeSnoop	(void * pBuf);


/*******************************************************************************
*
* cacheArchLibInit - initialize the cache library
* 
* This routine initializes the cache library for Intel 80X86
* processors.  The caching mode CACHE_WRITETHROUGH is available for the x86
* processor family.  It initializes the function pointers.
*
* RETURNS: OK.
*/

STATUS cacheArchLibInit
    (
    CACHE_MODE	instMode,	/* instruction cache mode */
    CACHE_MODE	dataMode	/* data cache mode */
    )
    {
    static BOOL cacheArchLibInitDone = FALSE;

    if (!cacheArchLibInitDone)			/* do it only once */
        {
	/* get bytes flushed by CLFLUSH instruction if supported */

	if (sysCpuId.featuresEdx & CPUID_CLFLUSH)
	    cacheFlushBytes = (sysCpuId.featuresEbx & CPUID_CHUNKS) >> 5;

	cacheArchLibInitDone = TRUE;		/* mark the done-flag TRUE */
        }

    /* 386 family does not have cache and WBINVD instruction */

    if (sysProcessor != X86CPU_386)
	{
        cacheLib.enableRtn	= cacheArchEnable;	/* cacheEnable() */
        cacheLib.disableRtn	= cacheArchDisable;	/* cacheDisable() */
        cacheLib.lockRtn	= cacheArchLock;	/* cacheLock() */
        cacheLib.unlockRtn	= cacheArchUnlock;	/* cacheUnlock() */
        cacheLib.dmaMallocRtn	= (FUNCPTR)cacheArchDmaMalloc;
        cacheLib.dmaFreeRtn	= (FUNCPTR)cacheArchDmaFree;
        cacheLib.dmaVirtToPhysRtn = NULL;
        cacheLib.dmaPhysToVirtRtn = NULL;
        cacheLib.textUpdateRtn	= NULL;

	/* note: return type of cachePen4Flush() and cacheI86Flush() are void */

        if (sysCpuId.featuresEdx & CPUID_CLFLUSH)	/* cacheFlush() */
	    {
            cacheLib.flushRtn	   = (FUNCPTR)cachePen4Flush;	/* w CLFLUSH */
	    cacheLib.clearRtn	   = (FUNCPTR)cachePen4Clear;	/* w CLFLUSH */
            cacheLib.invalidateRtn = (FUNCPTR)cachePen4Clear;	/* w CLFLUSH */
	    }
	else
	    {
            cacheLib.flushRtn	   = (FUNCPTR)cacheI86Flush;	/* w WBINVD */
	    cacheLib.clearRtn	   = (FUNCPTR)cacheI86Clear;	/* w WBINVD */
            cacheLib.invalidateRtn = (FUNCPTR)cacheI86Clear;	/* w WBINVD */
	    }

        cacheLib.pipeFlushRtn	= NULL;
	}

    /* fully coherent cache needs nothing */

    if (dataMode & CACHE_SNOOP_ENABLE)
	{
        cacheLib.lockRtn	= NULL;			/* cacheLock() */
        cacheLib.unlockRtn	= NULL;			/* cacheUnlock() */
        cacheLib.clearRtn	= NULL;			/* cacheClear() */
        cacheLib.dmaMallocRtn   = (FUNCPTR)cacheArchDmaMallocSnoop;
        cacheLib.dmaFreeRtn	= (FUNCPTR)cacheArchDmaFreeSnoop;
        cacheLib.flushRtn	= NULL;			/* cacheFlush() */
        cacheLib.invalidateRtn  = NULL;			/* cacheClear() */
	}

    /* check for parameter errors */

    if ((instMode & CACHE_WRITEALLOCATE)	|| 
	(dataMode & CACHE_WRITEALLOCATE)	||
        (instMode & CACHE_NO_WRITEALLOCATE)	|| 
	(dataMode & CACHE_NO_WRITEALLOCATE)	||
        (instMode & CACHE_BURST_ENABLE)		|| 
	(dataMode & CACHE_BURST_ENABLE)		||
        (instMode & CACHE_BURST_DISABLE)	|| 
	(dataMode & CACHE_BURST_DISABLE))
	return (ERROR);

    /* reset to the known state(disabled), since the current mode is unknown */

    if (sysProcessor != X86CPU_386)
        cacheI86Reset ();			/* reset and disable a cache */

    cacheDataMode	= dataMode;		/* save dataMode for enable */
    cacheDataEnabled	= FALSE;		/* d-cache is currently off */
    cacheMmuAvailable	= FALSE;		/* no mmu yet */

    return (OK);
    }

/*******************************************************************************
*
* cacheArchEnable - enable a cache
*
* This routine enables the cache.
*
* RETURNS: OK.
*
* NOMANUAL
*/

STATUS cacheArchEnable
    (
    CACHE_TYPE	cache		/* cache to enable */
    )
    {

    cacheI86Enable ();

    if (cache == DATA_CACHE)
	{
	cacheDataEnabled = TRUE;
	cacheFuncsSet ();
	}

    return (OK);
    }

/*******************************************************************************
*
* cacheArchDisable - disable a cache
*
* This routine disables the cache.
*
* RETURNS: OK.
*
* NOMANUAL
*/

STATUS cacheArchDisable
    (
    CACHE_TYPE	cache		/* cache to disable */
    )
    {

    cacheI86Disable ();

    if (cache == DATA_CACHE)
	{
	cacheDataEnabled = FALSE;		/* data cache is off */
	cacheFuncsSet ();			/* update data function ptrs */
	}

    return (OK);
    }

/*******************************************************************************
*
* cacheArchLock - lock entries in a cache
*
* This routine locks all entries in the cache.
*
* RETURNS: OK.
*
* NOMANUAL
*/

STATUS cacheArchLock
    (
    CACHE_TYPE	cache, 		/* cache to lock */
    void *	address,	/* address to lock */
    size_t	bytes		/* bytes to lock (ENTIRE_CACHE) */
    )
    {

    cacheI86Lock ();
    return (OK);
    }

/*******************************************************************************
*
* cacheArchUnlock - unlock a cache
*
* This routine unlocks all entries in the cache.
*
* RETURNS: OK.
*
* NOMANUAL
*/

STATUS cacheArchUnlock
    (
    CACHE_TYPE	cache, 		/* cache to unlock */
    void *	address,	/* address to unlock */
    size_t	bytes		/* bytes to unlock (ENTIRE_CACHE) */
    )
    {

    cacheI86Unlock ();
    return (OK);
    }

/*******************************************************************************
*
* cacheArchClear - clear all entries from a cache
*
* This routine clears all entries from the cache.  
*
* RETURNS: OK.
*
* NOMANUAL
*/

STATUS cacheArchClear
    (
    CACHE_TYPE	cache, 		/* cache to clear */
    void *	address,	/* address to clear */
    size_t	bytes		/* bytes to clear */
    )
    {

    if (sysCpuId.featuresEdx & CPUID_CLFLUSH)
	cachePen4Clear (cache, address, bytes);
    else
	WRS_ASM ("wbinvd");

    return (OK);
    }

/*******************************************************************************
*
* cacheArchFlush - flush all entries from a cache
*
* This routine flushs all entries from the cache.  
*
* RETURNS: OK.
*
* NOMANUAL
*/

STATUS cacheArchFlush
    (
    CACHE_TYPE	cache, 		/* cache to clear */
    void *	address,	/* address to clear */
    size_t	bytes		/* bytes to clear */
    )
    {

    if (sysCpuId.featuresEdx & CPUID_CLFLUSH)
	cachePen4Flush (cache, address, bytes);
    else
	WRS_ASM ("wbinvd");

    return (OK);
    }

/*******************************************************************************
*
* cacheArchClearEntry - clear an entry from a cache
*
* This routine clears a specified entry from the cache.
* The 386 family processors do not have a cache, thus it does nothing.  The 486, 
* P5(Pentium), and P6(PentiumPro, II, III) family processors do have a cache but 
* does not support a line by line cache control, thus it performs WBINVD 
* instruction.  The P7(Pentium4) family processors support the line by line 
* cache control with CLFLUSH instruction, thus flushes the specified cache line.
*
* RETURNS: OK
*/

STATUS cacheArchClearEntry
    (
    CACHE_TYPE	cache,		/* cache to clear entry for */
    void *	address		/* entry to clear */
    )
    {

    if (sysProcessor == X86CPU_386)
        return (OK);

    if (sysCpuId.featuresEdx & CPUID_CLFLUSH)
	cachePen4Clear (cache, address, cacheFlushBytes);
    else
	WRS_ASM ("wbinvd");

    return (OK);
    }

/*******************************************************************************
*
* cacheArchDmaMalloc - allocate a cache-safe buffer
*
* This routine attempts to return a pointer to a section of memory
* that will not experience cache coherency problems.  This routine
* is only called when MMU support is available 
* for cache control.
*
* INTERNAL
* We check if the cache is actually on before allocating the memory.  It
* is possible that the user wants Memory Management Unit (MMU)
* support but does not need caching.
*
* RETURNS: A pointer to a cache-safe buffer, or NULL.
*
* SEE ALSO: cacheArchDmaFree(), cacheDmaMalloc()
*
* NOMANUAL
*/

void *cacheArchDmaMalloc 
    (
    size_t      bytes			/* size of cache-safe buffer */
    )
    {
    void *pBuf;
    int	  pageSize;

    if ((pageSize = VM_PAGE_SIZE_GET ()) == ERROR)
	return (NULL);

    /* make sure bytes is a multiple of pageSize */

    bytes = ROUND_UP (bytes, pageSize);

    if ((_func_valloc == NULL) || 
	((pBuf = (void *)(* _func_valloc) (bytes)) == NULL))
	return (NULL);

    VM_STATE_SET (NULL, pBuf, bytes,
		  VM_STATE_MASK_CACHEABLE, VM_STATE_CACHEABLE_NOT);

    return (pBuf);
    }
	
/*******************************************************************************
*
* cacheArchDmaFree - free the buffer acquired by cacheArchDmaMalloc()
*
* This routine returns to the free memory pool a block of memory previously
* allocated with cacheArchDmaMalloc().  The buffer is marked cacheable.
*
* RETURNS: OK, or ERROR if cacheArchDmaMalloc() cannot be undone.
*
* SEE ALSO: cacheArchDmaMalloc(), cacheDmaFree()
*
* NOMANUAL
*/

STATUS cacheArchDmaFree
    (
    void *pBuf		/* ptr returned by cacheArchDmaMalloc() */
    )
    {
    BLOCK_HDR *	pHdr;			/* pointer to block header */
    STATUS	status = OK;		/* return value */

    if (vmLibInfo.vmLibInstalled)
	{
	pHdr = BLOCK_TO_HDR (pBuf);

	status = VM_STATE_SET (NULL,pBuf,(pHdr->nWords * 2) - sizeof(BLOCK_HDR),
			       VM_STATE_MASK_CACHEABLE, VM_STATE_CACHEABLE);
	}

    free (pBuf);			/* free buffer after modified */

    return (status);
    }

/*******************************************************************************
*
* cacheArchDmaMallocSnoop - allocate a cache line aligned buffer
*
* This routine attempts to return a pointer to a section of memory in the
* SNOOP_ENABLED cache mode.  This routine is called regardless of the current
* MMU or CACHE status.  And does not change cache attribute of the buffer.
*
* RETURNS: A pointer to a cache line aligned buffer, or NULL.
*
* SEE ALSO: cacheArchDmaFreeSnoop(), cacheDmaMalloc()
*
* NOMANUAL
*/

LOCAL void * cacheArchDmaMallocSnoop 
    (
    size_t      bytes			/* size of cache-safe buffer */
    )
    {
    void * pBuf;

    /* make sure bytes is a multiple of cache line size */

    bytes = ROUND_UP (bytes, _CACHE_ALIGN_SIZE);

    /* allocate memory at cache line boundary */

    if ((pBuf = memalign (_CACHE_ALIGN_SIZE, bytes)) == NULL)
	return (NULL);

    return (pBuf);
    }
	
/*******************************************************************************
*
* cacheArchDmaFreeSnoop - free the buffer acquired by cacheArchDmaMallocSnoop()
*
* This routine returns to the free memory pool a block of memory previously
* allocated with cacheArchDmaMallocSnoop().  The cache attribute of the buffer 
* does not change.
*
* RETURNS: OK always.
*
* SEE ALSO: cacheArchDmaMallocSnoop(), cacheDmaFree()
*
* NOMANUAL
*/

LOCAL STATUS cacheArchDmaFreeSnoop
    (
    void * pBuf		/* ptr returned by cacheArchDmaMallocSnoop() */
    )
    {

    free (pBuf);		/* free buffer */

    return (OK);
    }


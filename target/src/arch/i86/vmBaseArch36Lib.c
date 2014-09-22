/* vmBaseArch36Lib.c - VM (bundled) library for PentiumPro/2/3/4 36 bit mode */

/* Copyright 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,12jun02,hdn  written
*/

/*
DESCRIPTION:

vmBaseArch36Lib.c provides the virtual memory mapping and virtual address
translation that works with the bundled VM library.
It provides 2 routines that are linked in when INCLUDE_MMU_BASIC and 
INCLUDE_MMU_P6_36BIT are both defined in the BSP.

*/


/* includes */

#include "vxWorks.h"
#include "errno.h"
#include "mmuLib.h"
#include "private/vmLibP.h"
#include "arch/i86/mmuPro36Lib.h"


/* imports */

IMPORT VM_CONTEXT *	currentContext;		/* vmBaseLib.c */


/* defines */

#define NOT_PAGE_ALIGNED(addr)  (((UINT)(addr)) & ((UINT)pageSize - 1))
#define MMU_PAGE_MAP		(mmuPro36PageMap)
#define MMU_TRANSLATE		(mmuPro36Translate)


/****************************************************************************
*
* vmBaseArch36LibInit - initialize the arch specific bundled VM library
*
* This routine links the arch specific bundled VM library into the VxWorks 
* system.  It is called automatically when \%INCLUDE_MMU_BASIC and 
* \%INCLUDE_MMU_P6_36BIT are both defined in the BSP.
*
* RETURNS: N/A
*/

void vmBaseArch36LibInit (void)
    {
    } 

/****************************************************************************
*
* vmBaseArch36Map - map 36bit physical to the 32bit virtual memory
*
* vmBaseArch36Map maps 36bit physical pages to 32bit virtual space then
* sets the specified state.
*
* RETURNS: OK, or ERROR if virtAddr or physical page addresses are not on
* page boundaries, len is not a multiple of page size, or mapping failed.
*/

STATUS vmBaseArch36Map 
    (
    void * virtAddr, 		/* 32bit virtual address */
    LL_INT physAddr, 		/* 36bit physical address */
    UINT32 stateMask,		/* state mask */
    UINT32 state,		/* state */
    UINT32 len			/* length */
    )
    {
    INT32 pageSize      = vmBasePageSizeGet ();
    INT8 * thisVirtPage = (INT8 *) virtAddr;
    LL_INT thisPhysPage = physAddr;
    FAST UINT32 numBytesProcessed = 0;
    STATUS retVal       = OK;

    if (!vmLibInfo.vmBaseLibInstalled)
	return (ERROR);

    if ((NOT_PAGE_ALIGNED (thisVirtPage)) ||
        (NOT_PAGE_ALIGNED (thisPhysPage)) ||
        (NOT_PAGE_ALIGNED (len)))
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    semTake (&currentContext->sem, WAIT_FOREVER);

    while (numBytesProcessed < len)
	{
	if (MMU_PAGE_MAP (currentContext->mmuTransTbl,
			  thisVirtPage, thisPhysPage) != OK)
	    {
	    retVal = ERROR;
	    break;
	    }

	if (vmBaseStateSet (currentContext, thisVirtPage,
			    pageSize, stateMask, state) != OK)
	    {
	    retVal = ERROR;
	    break;
	    }

	thisVirtPage += pageSize;
	thisPhysPage += pageSize;
	numBytesProcessed += pageSize;
	}

    semGive (&currentContext->sem);

    return (retVal);
    }

/****************************************************************************
*
* vmBaseArch36Translate - translate a 32bit virtual address to a 36bit physical address
*
* vmBaseArch36Translate may be used to retrieve the mapping information 
* for a virtual address from the page translation tables.  If the given 
* virtual address has never been mapped, either the returned status will
* be ERROR, or, the returned status will be OK, but the returned physical
* address will be -1.
* If context is specified as NULL, the current context is used.
* This routine is callable from interrupt level.
*
* RETURNS: OK, or validation failed, or translation failed.
*/

STATUS vmBaseArch36Translate 
    (
    void *  virtAddr, 		/* 32bit virtual address */
    LL_INT * physAddr		/* place to put 36bit result */
    )
    {
    STATUS retVal;

    if (!vmLibInfo.vmBaseLibInstalled)
	return (ERROR);

    retVal = MMU_TRANSLATE (currentContext->mmuTransTbl, virtAddr, physAddr);

    return (retVal);
    }


/* vmShow.c - virtual memory show routines (VxVMI Option) */

/* Copyright 1984-1998 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01q,17mar99,jdi  doc: updated w/ info about proj facility (SPR 25727).
01p,16apr98,hdn  fixed typo by renaming XXX_WRITEBACK to XXX_WBACK.
01o,09apr98,hdn  added support for Pentium, PentiumPro.
01k,18aug94,tpr  changed VM_STATE_CACHEABLE to VM_STATE_MASK_CACHEABLE in
		 vmContextShowNewBlock().
01k,13sep93,caf  made null for MIPS.
01l,18nov93,edm  changed BOOL mmuPhysAddrShifted to int mmuPhysAddrShift.
01k,23mar93,edm  added support for physical addresses->page number option.
01j,23feb93,jdi  doc: added Availability section.
01i,04feb93,rdc  fixed bug preventing display of blocks at end of vm space.
		 removed display of "V+", minor doc tweak.
01h,03feb93,jdi  documentation - marked as optional product.
01g,02feb93,jdi  documentation tweaks.
01f,21oct92,jdi  doc change as per pme.
01e,19oct92,jcf  guarded against zero divide.
01d,02oct92,jdi  documentation cleanup.
01c,22sep92,rdc  changed globalPageBlockArray to type UINT8;
01b,30jul92,rdc  added show routine to vmContextClass.
01a,27jul92,rdc  written.

DESCRIPTION
This library contains virtual memory information display routines.

The routine vmShowInit() links this facility into the VxWorks system.
It is called automatically when this facility is configured into
VxWorks using either of the following methods:
.iP
If you use the configuration header files, define both INCLUDE_MMU_FULL
and INCLUDE_SHOW_ROUTINES in config.h.
.iP
If you use the Tornado project facility, select INCLUDE_MMU_FULL_SHOW.
.LP

AVAILABILITY
This module and vmLib are distributed as the unbundled virtual memory 
support option, VxVMI.

INCLUDE FILES: vmLib.h

SEE ALSO: vmLib,
.pG "Virtual Memory"
*/

#include "vxWorks.h"

#if	(CPU_FAMILY != MIPS)

#include "private/vmLibP.h"
#include "semLib.h"
#include "errno.h"
#include "stdio.h"

/* macros */

#define ADDR_IN_GLOBAL_SPACE(vAddr) (globalPageBlockArray[(unsigned) vAddr / mmuPageBlockSize])

/* imports */

IMPORT UINT8 *globalPageBlockArray;
IMPORT int    mmuPageBlockSize;
IMPORT int    mmuPhysAddrShift;

/* forward declarations */

LOCAL void vmContextShowNewBlock (UINT blockBegin, UINT blockEnd, UINT physAddr,
				  UINT blockState);

/****************************************************************************
*
* vmShowInit - include virtual memory show facility (VxVMI Option)
*
* This routine acts as a hook to include vmContextShow().
* It is called automatically when the virtual memory show facility is
* configured into VxWorks using either of the following methods:
* .iP
* If you use the configuration header files, define both INCLUDE_MMU_FULL
* and INCLUDE_SHOW_ROUTINES in config.h.
* .iP
* If you use the Tornado project facility, select INCLUDE_MMU_FULL_SHOW.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: N/A
*/

void vmShowInit (void)
    {
    classShowConnect ((CLASS_ID) vmContextClassId, (FUNCPTR)vmContextShow);
    }

/****************************************************************************
*
* vmContextShow - display the translation table for a context (VxVMI Option)
*
* This routine displays the translation table for a specified context.  
* If <context> is specified as NULL, the current context is displayed.  
* Output is formatted to show blocks of virtual memory with consecutive
* physical addresses and the same state.  State information shows the
* writable and cacheable states.  If the block is in global virtual
* memory, the word "global" is appended to the line.  Only virtual memory 
* that has its valid state bit set is displayed.
*
* This routine should be used for debugging purposes only.
*
* Note that this routine cannot report non-standard architecture-dependent 
* states.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: OK, or ERROR if the virtual memory context is invalid.
*/

STATUS vmContextShow
    (
    VM_CONTEXT_ID context	/* context - NULL == currentContext */
    )
    {
    UINT	thisVirtPage; 
    UINT	thisPhysPage; 
    UINT	blockBegin;
    UINT	physPageAddr;
    UINT	thisState;
    UINT	blockPhysAddr;
    UINT	blockState;
    int		vmPageSize;
    int		physPageInc;
    int		numPages;
    int	 	pageCounter;
    BOOL 	blockEnd;
    BOOL 	printIt;
    BOOL 	previousPageWasInvalid;

    vmPageSize		= vmPageSizeGet ();

    if (vmPageSize == 0)
	return (ERROR);				/* vmLib is not installed! */

    numPages		= (unsigned) 0x80000000 / vmPageSize * 2;
    pageCounter 	= 0;
    blockEnd 		= FALSE;
    blockPhysAddr	= NULL;
    printIt 		= TRUE;
    previousPageWasInvalid = TRUE;

    if (context == NULL)
	context = vmCurrentGet ();

    if (OBJ_VERIFY (context, vmContextClassId) != OK)
	return (ERROR);

    if (mmuPhysAddrShift)
	{
	printf ("VIRTUAL ADDR  BLOCK LENGTH  PHYSICAL PAGE   STATE\n");
	physPageInc = 1;
	}
    else
	{
	printf ("VIRTUAL ADDR  BLOCK LENGTH  PHYSICAL ADDR   STATE\n");
	physPageInc = vmPageSize;
	}

    semTake (&context->sem, WAIT_FOREVER);

    thisVirtPage = thisPhysPage = 0;

    vmStateGet (context, (void *) 0, &blockState); 

    for (blockBegin = 0;
	 pageCounter < numPages;
	 pageCounter++,
	 thisVirtPage += vmPageSize,
	 thisPhysPage += physPageInc)
	{
	if (vmTranslate (context, (void *) thisVirtPage, 
			 (void **) &physPageAddr) == ERROR)  
	    {
	    blockEnd = TRUE; 

	    if (previousPageWasInvalid)
		printIt = FALSE;

	    previousPageWasInvalid = TRUE;
	    thisPhysPage = 0xffffffff;
	    }
	else
	    {

	    if (!previousPageWasInvalid && (physPageAddr != thisPhysPage))
		blockEnd = TRUE;


	    if (vmStateGet (context, (void *) thisVirtPage, &thisState)== ERROR)
		{
		printf ("vmContextShow: error getting state for addr %x\n", 
			thisVirtPage);
		return (ERROR);
		}

	    if (!previousPageWasInvalid && (thisState != blockState))
		blockEnd = TRUE;

	    if (previousPageWasInvalid)
		{
		blockBegin = thisVirtPage;
		blockPhysAddr = thisPhysPage = physPageAddr;
		blockState = thisState;
		}

	    previousPageWasInvalid = FALSE;
	    }

	if (blockEnd)
	    { 
	    if (printIt)
		vmContextShowNewBlock (blockBegin, thisVirtPage, 
				       blockPhysAddr, blockState);

	    blockBegin = thisVirtPage;
	    blockState = thisState;
	    blockPhysAddr = thisPhysPage = physPageAddr;
	    blockEnd = FALSE;
	    printIt = TRUE;
	    }
	}

    /* see if there was a block that went up to the last virtual page */

    if (!previousPageWasInvalid)
	vmContextShowNewBlock (blockBegin, thisVirtPage,
			       blockPhysAddr, blockState);

    semGive (&context->sem);

    return (OK);
    }


/****************************************************************************
*
* vmContextShowNewBlock - display info for a block of virtual mem.
*
*/

LOCAL void vmContextShowNewBlock
    (
    UINT blockBegin,
    UINT blockEnd,
    UINT physAddr,
    UINT blockState 
    )
    {
    
    printf ("0x%08x    0x%08x    0x%08x      ", blockBegin,
	    blockEnd - blockBegin, physAddr);

    if (blockState & VM_STATE_WRITABLE)
	printf ("W+ ");	
    else
	printf ("W- ");	

    if (blockState & VM_STATE_MASK_CACHEABLE)
	printf ("C+ ");	
    else
	printf ("C- ");	

#if	(CPU_FAMILY == I80X86)
    if ((blockState & VM_STATE_MASK_WBACK) == VM_STATE_WBACK)
	printf ("B+ ");	
    else
	printf ("B- ");	

    if ((blockState & VM_STATE_MASK_GLOBAL) == VM_STATE_GLOBAL)
	printf ("G+ ");	
    else
	printf ("G- ");	
#endif	/* (CPU_FAMILY == I80X86) */

    if (ADDR_IN_GLOBAL_SPACE (blockBegin))
	printf (" (global)");

    printf ("\n");
    }

#endif	/* (CPU_FAMILY != MIPS) */

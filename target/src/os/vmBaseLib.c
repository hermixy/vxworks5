/* vmBaseLib.c - base virtual memory support library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01w,16nov01,to   declare global variables in vmData.c to avoid getting multiple
                 vm*Lib.o pulled
01v,26jul01,scm  add extended small page table support for XScale...
01u,13nov98,jpd  added support for Protection Unit style MMUs (MPUs).
01v,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01u,21feb99,jdi  doc: listed errnos.
01t,31jul98,ms   make vmContextClassId LOCAL.
01s,01jan96,tpr  added VM_STATE_MASK_MEM_COHERENCY and VM_STATE_MASK_GUARDED in
		 vmBaseGlobalMapInit().
01r,18nov93,edm  changed BOOL mmuPhysAddrShifted to int mmuPhysAddrShift.
01q,13sep93,caf  made null for MIPS.
01p,05apr93,edm  added support for physical addresses->page number option.
01o,23feb93,jdi  doc tweaks.
01n,12feb93,jdi  doc tweaks.
01m,12feb93,rdc  mods to aid testing and reorg of data struture visability.
01l,10feb93,rdc  made vmBaseStateSet global - allow write protection.
01k,04feb93,jdi  documentation cleanup for 5.1.
01j,19oct92,jcf  cleanup.  
01i,08oct92,rdc  name changes, doc, cleanup.
01h,01oct92,jcf  added cacheFuncsSet() to attach mmu to cache support. warnings.
01g,22sep92,rdc  changed NUM_PAGE_STATES to 256.  
01f,30jul92,rdc  added pointer to vmTranslate routine to vmLibInfo. 
01e,27jul92,rdc  moved  "vmLib.c" to "vmBaseLib.c"
01d,21jul92,rdc  took out mutex in vmStateSet so callable from int level.
01c,13jul92,rdc  changed reference to vmLib.h to vmLibP.h.
01b,09jul92,rdc  changed indirect routine pointer configuration.
01a,08jul92,rdc  written.
*/

/* 
This library provides the minimal MMU (Memory Management Unit) support 
needed in a system.  Its primary purpose is to create cache-safe buffers for
cacheLib.  Buffers are provided to optimize I/O throughput.

A call to vmBaseLibInit() initializes this library, thus permitting
vmBaseGlobalMapInit() to initialize the MMU and set up MMU translation tables.
Additionally, vmBaseStateSet() can be called to change the 
translation tables dynamically. 

This library is a release-bundled complement to vmLib and vmShow, 
modules that offer full-featured MMU support and virtual memory information
display routines.  The vmLib and vmShow libraries are distributed as the
unbundled virtual memory support option, VxVMI.

CONFIGURATION
Bundled MMU support is included in VxWorks when the configuration macro
INCLUDE_MMU_BASIC is defined.  If the configuration macro INCLUDE_MMU_FULL
is also defined, the default is full MMU support (unbundled).

INCLUDE FILES: sysLib.h, vmLib.h

SEE ALSO: vmLib, vmShow,
.pG "Virtual Memory"
*/

#include "vxWorks.h"

#if	(CPU_FAMILY != MIPS)

#include "private/vmLibP.h"
#include "stdlib.h"
#include "mmuLib.h"
#include "private/classLibP.h"
#include "private/objLibP.h"
#include "errno.h"
#include "cacheLib.h"
#include "sysLib.h"

/* macros to reference mmuLib routines indirectly through mmuLibFunc table */

#define MMU_LIB_INIT        	(*(mmuLibFuncs.mmuLibInit))
#define MMU_TRANS_TBL_CREATE 	(*(mmuLibFuncs.mmuTransTblCreate))
#define MMU_TRANS_TBL_DELETE 	(*(mmuLibFuncs.mmuTransTblDelete))
#define MMU_ENABLE 	  	(*(mmuLibFuncs.mmuEnable))
#define MMU_STATE_SET 	  	(*(mmuLibFuncs.mmuStateSet))
#define MMU_STATE_GET 	  	(*(mmuLibFuncs.mmuStateGet))
#define MMU_PAGE_MAP 	  	(*(mmuLibFuncs.mmuPageMap))
#define MMU_GLOBAL_PAGE_MAP 	(*(mmuLibFuncs.mmuGlobalPageMap))
#define MMU_TRANSLATE 	  	(*(mmuLibFuncs.mmuTranslate))
#define MMU_CURRENT_SET 	(*(mmuLibFuncs.mmuCurrentSet))

IMPORT int mmuPageBlockSize;                    /* initialized by mmuLib.c */
IMPORT STATE_TRANS_TUPLE *mmuStateTransArray;   /* initialized by mmuLib.c */
IMPORT int mmuStateTransArraySize;              /* initialized by mmuLib.c */
IMPORT MMU_LIB_FUNCS mmuLibFuncs;               /* initialized by mmuLib.c */


IMPORT int			mmuPhysAddrShift;       /* see vmData.c */


/* locals */

/* arch indep. state info uses 8 bits - 2^8=256 possible states */
   
#define NUM_PAGE_STATES 256
   
LOCAL UINT 			vmStateTransTbl[NUM_PAGE_STATES];
LOCAL UINT 			vmMaskTransTbl[NUM_PAGE_STATES];
LOCAL int 			vmPageSize;
LOCAL VM_CONTEXT 		sysVmContext;      	/* initial vm context */
LOCAL VM_CONTEXT_CLASS 		vmContextClass;
LOCAL VM_CONTEXT_CLASS_ID 	vmContextClassId = &vmContextClass;

/* globals */

VM_CONTEXT *		currentContext = NULL;
int mutexOptionsVmBaseLib = 	SEM_Q_PRIORITY | 
				SEM_DELETE_SAFE |
				SEM_INVERSION_SAFE;

/* forward declarations */

LOCAL STATUS	vmBaseContextInit (VM_CONTEXT *context);
LOCAL STATUS	vmBaseEnable (BOOL enable);
LOCAL STATUS	vmBaseGlobalMap (void *virtualAddr, void *physicalAddr,
				 UINT len);
LOCAL STATUS	vmBaseTranslate (VM_CONTEXT_ID context, void *virtualAddr, 
			         void **physicalAddr);


/******************************************************************************
*
* vmBaseLibInit - initialize base virtual memory support
*
* This routine initializes the virtual memory context class and 
* module-specific data structures.  It is called only once during 
* system initialization, and should be followed with a call to 
* vmBaseGlobalMapInit(), which initializes and enables the MMU.
*
* RETURNS: OK.
*
* SEE ALSO: vmBaseGlobalMapInit()
*/

STATUS vmBaseLibInit 
    (
    int pageSize  /* size of page */
    )
    {
    int		i;
    int 	j;
    UINT 	newState;
    UINT 	newMask;

    if (vmLibInfo.vmBaseLibInstalled)	/* only install vmBaseLib once */
	return (OK);

    if (pageSize == 0)
	return (ERROR);			/* check for reasonable parameter */

    vmPageSize = pageSize;

    /*
     * Initialize the page state translation table.  This table is used to
     * translate between architecture independent state values and architecture
     * dependent state values.
     */

    for (i = 0; i < NUM_PAGE_STATES; i++)
	{
	newState = 0;

	for (j = 0; j < mmuStateTransArraySize; j++)
	    {
	    STATE_TRANS_TUPLE *thisTuple = &mmuStateTransArray[j];
	    UINT archIndepState = thisTuple->archIndepState;
	    UINT archDepState = thisTuple->archDepState;
	    UINT archIndepMask = thisTuple->archIndepMask;

	    if ((i & archIndepMask) == archIndepState)
		newState |= archDepState;
	    }

	vmStateTransTbl [i] = newState;
	}

    /*
     * Initialize the page state mask translation table.  This table is used to
     * translate between architecture independent state masks and architecture
     * dependent state masks.
     */

    for (i = 0; i < NUM_PAGE_STATES; i++)
	{
	newMask = 0;
	for (j = 0; j < mmuStateTransArraySize; j++)
	    {
	    STATE_TRANS_TUPLE *thisTuple = &mmuStateTransArray[j];
	    UINT archIndepMask = thisTuple->archIndepMask;
	    UINT archDepMask = thisTuple->archDepMask;

	    if ((i & archIndepMask) == archIndepMask)
		newMask |= archDepMask;
	    }

	vmMaskTransTbl [i] = newMask;
	}

    classInit ((OBJ_CLASS *) &vmContextClass.coreClass, sizeof (VM_CONTEXT), 
	       OFFSET(VM_CONTEXT, objCore), NULL, NULL, NULL);

    vmLibInfo.pVmStateSetRtn     = vmBaseStateSet;
    vmLibInfo.pVmEnableRtn       = vmBaseEnable;
    vmLibInfo.pVmPageSizeGetRtn  = vmBasePageSizeGet;
    vmLibInfo.pVmTranslateRtn    = vmBaseTranslate;
    vmLibInfo.vmBaseLibInstalled = TRUE;
    cacheMmuAvailable		 = TRUE;
    cacheFuncsSet ();			/* update cache funtion pointers */

    return (OK);
    }

/******************************************************************************
*
* vmBaseGlobalMapInit - initialize global mapping
*
* This routine creates and installs a virtual memory context with mappings
* defined for each contiguous memory segment defined in <pMemDescArray>.
* In the standard VxWorks configuration, an instance of PHYS_MEM_DESC (called
* `sysPhysMemDesc') is defined in sysLib.c; the variable is passed to
* vmBaseGlobalMapInit() by the system configuration mechanism.
*
* The physical memory descriptor also contains state information
* used to initialize the state information in the MMU's translation table
* for that memory segment.  The following state bits may be or'ed together:
*
* .TS
* tab(|);
* l0 l0 l .
*  VM_STATE_VALID     | VM_STATE_VALID_NOT     | valid/invalid
*  VM_STATE_WRITABLE  | VM_STATE_WRITABLE_NOT  | writable/write-protected
*  VM_STATE_CACHEABLE | VM_STATE_CACHEABLE_NOT | cacheable/not-cacheable
* .TE
*
* Additionally, mask bits are or'ed together in the `initialStateMask' structure
* element to describe which state bits are being specified in the `initialState'
* structure element:
*
*  VM_STATE_MASK_VALID
*  VM_STATE_MASK_WRITABLE
*  VM_STATE_MASK_CACHEABLE
*
* If <enable> is TRUE, the MMU is enabled upon return.
*
* RETURNS: A pointer to a newly created virtual memory context, or NULL if
* memory cannot be mapped.
*
* SEE ALSO: vmBaseLibInit()
*/

VM_CONTEXT_ID vmBaseGlobalMapInit 
    (
    PHYS_MEM_DESC *pMemDescArray, 	/* pointer to array of mem descs */
    int numDescArrayElements,		/* no. of elements in pMemDescArray */
    BOOL enable				/* enable virtual memory */
    )
    {
    int i;
    PHYS_MEM_DESC *thisDesc;
    FAST char *thisPage; 
#if !defined(BUILD_MPU_LIB)
    FAST UINT numBytesProcessed; 
    int pageSize = vmPageSize;
#endif
    UINT archIndepStateMask; 
    UINT archDepStateMask; 
    UINT archDepState;
    UINT archIndepState;

    /* set up global transparent mapping of physical to virtual memory */

    for (i = 0; i < numDescArrayElements; i++)
        {
        thisDesc = &pMemDescArray[i];

        /* map physical directly to virtual and set initial state */

        if (vmBaseGlobalMap ((void *) thisDesc->virtualAddr, 
			 (void *) thisDesc->physicalAddr,
			 thisDesc->len) == ERROR)
            {
	    return (NULL);
            }
        }

    /* create default virtual memory context */

    if (vmBaseContextInit (&sysVmContext) == ERROR)
	{
	return (NULL);
	}

    /* set the state of all the global memory we just created */ 

    for (i = 0; i < numDescArrayElements; i++)
        {
        thisDesc = &pMemDescArray[i];

	thisPage = thisDesc->virtualAddr;
#if !defined(BUILD_MPU_LIB)
	numBytesProcessed = 0;

	while (numBytesProcessed < thisDesc->len)
	    {
	    /* we let the use specifiy cacheablility, but not writeablility */

	    archIndepState = thisDesc->initialState; 

	    archDepState = vmStateTransTbl [archIndepState];

	    archIndepStateMask = 
		VM_STATE_MASK_CACHEABLE     | 
		VM_STATE_MASK_VALID         |
		VM_STATE_MASK_WRITABLE      |
		VM_STATE_MASK_BUFFERABLE    |
		VM_STATE_MASK_EX_CACHEABLE  |
		VM_STATE_MASK_EX_BUFFERABLE |
		VM_STATE_MASK_MEM_COHERENCY |
		VM_STATE_MASK_GUARDED;

	    archDepStateMask = vmMaskTransTbl [archIndepStateMask];

	    if (MMU_STATE_SET (sysVmContext.mmuTransTbl, thisPage, 
			    archDepStateMask, archDepState) == ERROR)
		return (NULL);

	    thisPage += pageSize;
	    numBytesProcessed += pageSize;
	    }
#else /* !defined(BUILD_MPU_LIB) */
	/*
	 * On an MPU, we need to pass the whole region on to the
	 * architecture-specific code, not break it up into pages.
	 */

	archIndepState = thisDesc->initialState; 

	archDepState = vmStateTransTbl [archIndepState];

	archIndepStateMask = 
	    VM_STATE_MASK_CACHEABLE     | 
	    VM_STATE_MASK_VALID         |
	    VM_STATE_MASK_WRITABLE      |
            VM_STATE_MASK_BUFFERABLE    |
            VM_STATE_MASK_EX_CACHEABLE  |
            VM_STATE_MASK_EX_BUFFERABLE |
	    VM_STATE_MASK_MEM_COHERENCY |
	    VM_STATE_MASK_GUARDED;

	archDepStateMask = vmMaskTransTbl [archIndepStateMask];

	if (MMU_STATE_SET (sysVmContext.mmuTransTbl, thisPage, 
			archDepStateMask, archDepState, thisDesc->len) == ERROR)
	    return NULL;
#endif /* !defined(BUILD_MPU_LIB) */
        }

    currentContext = &sysVmContext;
    MMU_CURRENT_SET (sysVmContext.mmuTransTbl);

    if (enable)
	if (MMU_ENABLE (TRUE) == ERROR)
	    return (NULL);

    return (&sysVmContext);
    }

/****************************************************************************
*
* vmBaseContextInit - initializer for VM_CONTEXT.
*
* This routine may be used to initialize static definitions of VM_CONTEXT
*
* RETURNS: OK, or ERROR if the translation table could not be created.
*/

LOCAL STATUS vmBaseContextInit 
    (
    VM_CONTEXT *context
    )
    {
    objCoreInit (&context->objCore, (CLASS_ID) vmContextClassId);

    semMInit (&context->sem, mutexOptionsVmBaseLib);

    context->mmuTransTbl = MMU_TRANS_TBL_CREATE ();

    if (context->mmuTransTbl == NULL)
	return (ERROR);

    return (OK);
    }

/****************************************************************************
*
* vmBaseGlobalMap - map physical to virtual in shared global virtual memory
*
* vmBaseGlobalMap maps physical pages to virtual space that is shared by all
* virtual memory contexts.  Calls to vmBaseGlobalMap should be made before any
* virtual memory contexts are created to insure that the shared global mappings
* will be included in all virtual memory contexts.  Mappings created with 
* vmBaseGlobalMap after virtual memory contexts are created are not guaranteed 
* to appear in all virtual memory contexts.
*
* RETURNS: OK, or ERROR if virtualAddr or physical page addresses are not on
* page boundaries, len is not a multiple of page size, or mapping failed.
*/

LOCAL STATUS vmBaseGlobalMap 
    (
    void *virtualAddr, 
    void *physicalAddr, 
    UINT len
    )
    {
    int pageSize = vmPageSize;
    char *thisVirtPage = (char *) virtualAddr;
    char *thisPhysPage = (char *) physicalAddr;
#if !defined(BUILD_MPU_LIB)
    FAST UINT numBytesProcessed = 0;
#endif

    if (!vmLibInfo.vmBaseLibInstalled)
	return (ERROR);

    if (((UINT) thisVirtPage % pageSize) != 0)
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    if ((!mmuPhysAddrShift) && (((UINT) thisPhysPage % pageSize) != 0))
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    if ((len % pageSize) != 0)
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

#if !defined(BUILD_MPU_LIB)
    while (numBytesProcessed < len)
	{
	if (MMU_GLOBAL_PAGE_MAP (thisVirtPage, thisPhysPage) == ERROR)
	    return (ERROR);

	thisVirtPage += pageSize;
	thisPhysPage += (mmuPhysAddrShift ? 1 : pageSize);
	numBytesProcessed += pageSize;
	}
#else /* !defined(BUILD_MPU_LIB) */
	/*
	 * On an MPU, we need to pass the whole region on to the
	 * architecture-specific code, not break it up into pages.
	 */

    if (MMU_GLOBAL_PAGE_MAP (thisVirtPage, thisPhysPage, len) == ERROR)
	return ERROR;
#endif /* !defined(BUILD_MPU_LIB) */

    return (OK);
    }

/****************************************************************************
*
* vmBaseStateSet - change the state of a block of virtual memory
*
* This routine changes the state of a block of virtual memory.  Each page
* of virtual memory has at least three elements of state information:
* validity, writability, and cacheability.  Specific architectures may
* define additional state information; see vmLib.h for additional
* architecture-specific states.  Memory accesses to a page marked as
* invalid will result in an exception.  Pages may be invalidated to prevent
* them from being corrupted by invalid references.  Pages may be defined as
* read-only or writable, depending on the state of the writable bits.
* Memory accesses to pages marked as not-cacheable will always result in a
* memory cycle, bypassing the cache.  This is useful for multiprocessing,
* multiple bus masters, and hardware control registers.
*
* The following states are provided and may be or'ed together in the 
* state parameter:  
*
* .TS
* tab(|);
* l0 l0 l .
*  VM_STATE_VALID     | VM_STATE_VALID_NOT     | valid/invalid
*  VM_STATE_WRITABLE  | VM_STATE_WRITABLE_NOT  | writable/write-protected
*  VM_STATE_CACHEABLE | VM_STATE_CACHEABLE_NOT | cacheable/not-cacheable
* .TE
*
* Additionally, the following masks are provided so that only specific
* states may be set.  These may be or'ed together in the `stateMask' parameter. 
*
*  VM_STATE_MASK_VALID
*  VM_STATE_MASK_WRITABLE
*  VM_STATE_MASK_CACHEABLE
*
* If <context> is specified as NULL, the current context is used.
*
* This routine is callable from interrupt level.
*
* RETURNS: OK, or ERROR if the validation fails, <pVirtual> is not on a page
* boundary, <len> is not a multiple of the page size, or the
* architecture-dependent state set fails for the specified virtual address.
*
* ERRNO:
* S_vmLib_NOT_PAGE_ALIGNED,
* S_vmLib_BAD_STATE_PARAM,
* S_vmLib_BAD_MASK_PARAM
*
*/

STATUS vmBaseStateSet 
    (
    VM_CONTEXT_ID context, 	/* context - NULL == currentContext */
    void *pVirtual, 		/* virtual address to modify state of */
    int len, 			/* len of virtual space to modify state of */
    UINT stateMask, 		/* state mask */
    UINT state			/* state */
    )
    {
    FAST int pageSize = vmPageSize;
    FAST char *thisPage = (char *) pVirtual;
#if !defined(BUILD_MPU_LIB)
    FAST UINT numBytesProcessed = 0;
#endif
    UINT archDepState;
    UINT archDepStateMask;
    STATUS retVal = OK;

    if (!vmLibInfo.vmBaseLibInstalled)
	return (ERROR);

    if (context == NULL)
	context = currentContext;

    if (OBJ_VERIFY (context, vmContextClassId) != OK)
	return (ERROR);

    if (((UINT) thisPage % pageSize) != 0)
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    if ((len % pageSize) != 0)
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    if (state > NUM_PAGE_STATES)
        {
        errno = S_vmLib_BAD_STATE_PARAM;
        return (ERROR);
        }

    if (stateMask > NUM_PAGE_STATES)
        {
        errno = S_vmLib_BAD_MASK_PARAM;
        return (ERROR);
        }

    /* get the architecture dependent states and state masks */

    archDepState = vmStateTransTbl [state];
    archDepStateMask = vmMaskTransTbl [stateMask];

    /* call mmuStateSet to do the actual work */

#if !defined(BUILD_MPU_LIB)
    while (numBytesProcessed < (UINT)len)
        {
        if (MMU_STATE_SET (context->mmuTransTbl, thisPage,
                         archDepStateMask, archDepState) == ERROR)
           {
	   retVal = ERROR;
	   break;
	   }

        thisPage += pageSize;
	numBytesProcessed += pageSize;
	}
#else /* !defined(BUILD_MPU_LIB) */
	/*
	 * On an MPU, we need to pass the whole region on to the
	 * architecture-specific code, not break it up into pages.
	 */

    if (MMU_STATE_SET (context->mmuTransTbl, thisPage,
		     archDepStateMask, archDepState, len) == ERROR)
       retVal = ERROR;
#endif /*!defined(BUILD_MPU_LIB) */

    return (retVal);
    }

/****************************************************************************
*
* vmBaseEnable - enable/disable virtual memory
*
* vmBaseEnable turns virtual memory on and off.  
*
* RETURNS: OK, or ERROR if validation failed, or architecture-dependent 
* code failed.
*/

LOCAL STATUS vmBaseEnable
    (
    BOOL enable         /* TRUE == enable MMU, FALSE == disable MMU */
    )
    {
    return (MMU_ENABLE (enable));
    }

/****************************************************************************
*
* vmBasePageSizeGet - return the page size
*
* This routine returns the architecture-dependent page size.
*
* This routine is callable from interrupt level.
*
* RETURNS: The page size of the current architecture. 
*
*/

int vmBasePageSizeGet (void)
    {
    return (vmPageSize);
    }

/****************************************************************************
*
* vmBaseTranslate - translate a virtual address to a physical address
*
* vmBaseTranslate may be used to retrieve the mapping information for a
* virtual address from the page translation tables.  If the given 
* virtual address has never been mapped, either the returned status will
* be ERROR, or, the returned status will be OK, but the returned physical
* address will be -1.
* If context is specified as NULL, the current context is used.
* This routine is callable from interrupt level.
*
* RETURNS: OK, or validation failed, or translation failed.
*/

LOCAL STATUS vmBaseTranslate 
    (
    VM_CONTEXT_ID context, 	/* context - NULL == currentContext */
    void *virtualAddr, 		/* virtual address */
    void **physicalAddr		/* place to put result */
    )
    {
    STATUS retVal;

    if (context == NULL)
	context = currentContext;

    if (OBJ_VERIFY (context, vmContextClassId) != OK)
	return (ERROR);

    retVal =  MMU_TRANSLATE (context->mmuTransTbl, virtualAddr, physicalAddr);

    return (retVal);
    }

#endif	/* (CPU_FAMILY != MIPS) */

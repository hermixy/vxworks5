/* vmLib.c - architecture-independent virtual memory support library (VxVMI Option) */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02a,02apr02,dtr  Adding PPC860.
01z,16nov01,to   declare global variables in vmData.c to avoid getting multiple
                 vm*Lib.o pulled
01y,26jul01,scm  add extended small page table support for XScale...
01x,03mar00,zl   merged SH support into T2
01w,30mar99,jdi  doc: fixed table formatting that refgen can't handle.
01v,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01u,21feb99,jdi  doc: listed errnos.
01t,02jul96,ms   replaced "%" with "NOT_PAGE_ALIGNED" to fix SPR 6833.
01s,04mar96,ms   made null for PPC.
01r,13sep93,caf  made null for MIPS.
01s,18nov93,edm  changed BOOL mmuPhysAddrShifted to int mmuPhysAddrShift.
01r,05apr93,edm  added support for physical addresses->page number option.
01q,19feb93,jdi  doc: add VxVMI option label and Availability section.
01p,12feb93,jdi  doc tweaks.
01o,12feb93,rdc  changed scheme for sharing data structures with mmuLib. Doc.
01n,09feb93,rdc  added vmTextPageProtect.  
01m,21oct92,jdi  doc change as per pme.
01l,19oct92,jcf  cleanup. fixed vmLibInit.
01k,14oct92,jdi  made vmContextInit() NOMANUAL.
01j,02oct92,jdi  documentation cleanup.
01i,01oct92,jcf  added cacheFuncsSet() to attach mmu to cache support.
01h,22sep92,rdc  changed NUM_PAGE_STATES to 256.  
		 changed globalPageBlockArray to be type UINT8 instead of BOOL.
01g,23aug92,jcf  cleanup.
01f,30jul92,rdc  added pointer to vmTranslate routine to vmLibInfo. 
01e,27jul92,rdc  moved  "vmLib.c" to "vmBaseLib.c", and installed the
		 unbundled vmLib.c here.
01d,21jul92,rdc  took out mutex in vmStateSet so callable from int level.
01c,13jul92,rdc  changed reference to vmLib.h to vmLibP.h.
01b,09jul92,rdc  changed indirect routine pointer configuration.
01a,08jul92,rdc  written.
*/

/*
DESCRIPTION
This library provides an architecture-independent interface to the CPU's
memory management unit (MMU).  Although vmLib is implemented with
architecture-specific libraries, application code need never reference
directly the architecture-dependent code in these libraries.

A fundamental goal in the design of vmLib was to permit transparent
backward compatibility with previous versions of VxWorks that did not use
the MMU.  System designers may opt to disable the MMU because of timing
constraints, and some architectures do not support MMUs; therefore VxWorks
functionality must not be dependent on the MMU.  The resulting design
permits a transparent configuration with no change in the programming
environment (but the addition of several protection features, such as text
segment protection) and the ability to disable virtual memory in systems
that require it.

The vmLib library provides a mechanism for creating virtual memory contexts,
vmContextCreate().  These contexts are not automatically created for individual 
tasks, but may be created dynamically by tasks, and swapped in and out 
in an application specific manner. 

All virtual memory contexts share a global transparent mapping of virtual
to physical memory for all of local memory and the local hardware device
space (defined in sysLib.c for each board port in the `sysPhysMemDesc'
data structure).  When the system is initialized, all of local physical
memory is accessible at the same address in virtual memory (this is done
with calls to vmGlobalMap().)  Modifications made to this global mapping
in one virtual memory context appear in all virtual memory contexts.  For
example, if the exception vector table (which resides at address 0 in
physical memory) is made read only by calling vmStateSet() on virtual
address 0, the vector table will be read only in all virtual memory
contexts.

Private virtual memory can also be created.  When physical pages are
mapped to virtual memory that is not in the global transparent region,
this memory becomes accessible only in the context in which it was
mapped.  (The physical pages will also be accessible in the transparent
translation at the physical address, unless the virtual pages in the
global transparent translation region are explicitly invalidated.)  State
changes (writability, validity, etc.) to a section of private virtual
memory in a virtual memory context do not appear in other contexts.  To
facilitate the allocation of regions of virtual space, vmGlobalInfoGet()
returns a pointer to an array of booleans describing which portions of the
virtual address space are devoted to global memory.  Each successive array
element corresponds to contiguous regions of virtual memory the size of
which is architecture-dependent and which may be obtained with a call to
vmPageBlockSizeGet().  If the boolean array element is true, the
corresponding region of virtual memory, a "page block", is reserved for
global virtual memory and should not be used for private virtual memory.
(If vmMap() is called to map virtual memory previously defined as global,
the routine will return an error.)

All the state information for a block of virtual memory can be set in a
single call to vmStateSet().  It performs parameter checking and checks
the validity of the specified virtual memory context.  It may also be used
to set architecture-dependent state information.  See vmLib.h for additional
architecture-dependent state information.

The routine vmContextShow() in vmShow displays the virtual memory context
for a specified context.  For more information, see the manual entry for
this routine.

CONFIGURATION:
Full MMU support (vmLib, and optionally, vmShow) is included in VxWorks
when the configuration macro INCLUDE_MMU_FULL is defined.  If the
configuration macro INCLUDE_MMU_BASIC is also defined, the default is
full MMU support (unbundled).

The sysLib.c library contains a data structure called `sysPhysMemDesc',
which is an array of PHYS_MEM_DESC structures.  Each element of the array
describes a contiguous section of physical memory.  The description of
this memory includes its physical address, the virtual address where it
should be mapped (typically, this is the same as the physical address, but
not necessarily so), an initial state for the memory, and a mask defining
which state bits in the state value are to be set.  Default configurations
are defined for each board support package (BSP), but these mappings may
be changed to suit user-specific system configurations.  For example, the
user may need to map additional VME space where the backplane network
interface data structures appear.

INTERNAL
Mutual exclusion is provided in all operations that modify and examine
translation tables;  routines in mmuLib need not provide mutual exclusion
mechanisms for translation tables.

AVAILABILITY
This library and vmShow are distributed as the unbundled virtual memory
support option, VxVMI.  A scaled down version, vmBaseLib, is
provided with VxWorks for systems that do not permit optional use of the
MMU, or for architectures that require certain features of the MMU to
perform optimally (in particular, architectures that rely heavily on
caching, but do not support bus snooping, and thus require the ability to
mark interprocessor communications buffers as non-cacheable.)  Most
routines in vmBaseLib are referenced internally by VxWorks; they are not
callable by application code.

INCLUDE FILES: vmLib.h 

SEE ALSO: sysLib, vmShow,
.pG "Virtual Memory"
*/

#include "vxWorks.h"

#if     (CPU_FAMILY != MIPS)  && ((CPU_FAMILY!=PPC) || (CPU==PPC860))
#include "private/vmLibP.h"
#include "stdlib.h"
#include "mmuLib.h"
#include "private/classLibP.h"
#include "private/objLibP.h"
#include "errno.h"
#include "cacheLib.h"
#include "sysLib.h"
#include "private/semLibP.h"	

/* macros and defines */

/* detect if addr is in global virtual memory */

#define ADDR_IN_GLOBAL_SPACE(vAddr) (globalPageBlockArray[(unsigned) vAddr / mmuPageBlockSize])

/* compute if page is aligned without using "%" (SPR 6833) */

#define	NOT_PAGE_ALIGNED(addr)	(((UINT)(addr)) & ((UINT)vmPageSize - 1))

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


IMPORT int mmuPageBlockSize;			/* initialized by mmuLib.c */
IMPORT STATE_TRANS_TUPLE *mmuStateTransArray;	/* initialized by mmuLib.c */
IMPORT int mmuStateTransArraySize;		/* initialized by mmuLib.c */
IMPORT MMU_LIB_FUNCS mmuLibFuncs; 		/* initialized by mmuLib.c */


IMPORT int                      mmuPhysAddrShift;       /* see vmData.c */


/* imports */

IMPORT void sysInit ();
IMPORT void etext ();

/* class declaration */

LOCAL VM_CONTEXT_CLASS	vmContextClass;
VM_CONTEXT_CLASS_ID	vmContextClassId = &vmContextClass;

/* state translation tables - these tables are initialized in vmLibInit,
 * and are used to translate between architecture-independent and architecture
 * dependent states. arch indep. state info uses 8 bits - 2^8=256 possible
 * states 
 */

#define NUM_PAGE_STATES 256

/* locals */

LOCAL UINT		vmStateTransTbl[NUM_PAGE_STATES];
LOCAL UINT		vmMaskTransTbl[NUM_PAGE_STATES];
LOCAL LIST		vmContextList;		/* XXX delete ? */
LOCAL int 		vmPageSize;
LOCAL VM_CONTEXT	sysVmContext;      	/* initial vm context */
LOCAL VM_CONTEXT *	currentContext = NULL;
LOCAL SEMAPHORE 	globalMemMutex;


/* globals */

UINT8 *globalPageBlockArray;
int    mutexOptionsVmLib = SEM_Q_PRIORITY |SEM_DELETE_SAFE | SEM_INVERSION_SAFE;

LOCAL STATUS vmTextPageProtect (void *textPageAddr, BOOL protect);

/*******************************************************************************
*
* vmLibInit - initialize the virtual memory support module (VxVMI Option)
*
* This routine initializes the virtual memory context class.
* It is called only once during system initialization.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: OK.
*/

STATUS vmLibInit 
    (
    int pageSize		/* size of page */
    )
    {
    int		i;
    int 	j;
    UINT 	newState;
    UINT 	newMask;
    int 	arraySize;

    if (vmLibInfo.vmLibInstalled)	/* only install vmLib once */
	return (OK);

    if (pageSize == 0)
	return (ERROR);			/* check for reasonable parameter */

    vmPageSize = pageSize;

    classInit ((OBJ_CLASS *) &vmContextClass.coreClass, sizeof (VM_CONTEXT), 
	       OFFSET(VM_CONTEXT, objCore), (FUNCPTR) vmContextCreate, 
	       NULL, NULL);

    /* initialize the mutual exclusion semaphore that protects the global
     * memory mappings.
     */

    semMInit (&globalMemMutex, mutexOptionsVmLib);

    /* initialize the page state translation table.  This table is used to
     * translate betseen architecture-independent state values and architecture
     * dependent state values */

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

    /* initialize the page state mask translation table.  This table is used to
     * translate betseen architecture-independent state masks and architecture
     * dependent state masks */

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


    /* allocate memory for global page block array to keep track of which
     * sections of virtual memory are reserved for global virtual memory.
     * number of page blocks in virtual space = (2**32)/pageBlockSize.
     */

    /* what we really want is: 2^32/mmuPageBlockSize, but we can't 
       represent 2^32, so we assume mmuPageBlockSize is even, and divide
       everything by 2 so we can do 32 bit arithmatic */

    arraySize = (UINT) 0x80000000 / (mmuPageBlockSize / 2);

    globalPageBlockArray = (UINT8 *) calloc (arraySize, sizeof (UINT8));

    if (globalPageBlockArray == NULL)
        return (ERROR);

    lstInit (&vmContextList); 			/* XXX not currently used */

    vmLibInfo.pVmStateSetRtn    = vmStateSet; 
    vmLibInfo.pVmStateGetRtn    = vmStateGet;
    vmLibInfo.pVmEnableRtn      = vmEnable;
    vmLibInfo.pVmPageSizeGetRtn = vmPageSizeGet;
    vmLibInfo.pVmTranslateRtn   = vmTranslate;
    vmLibInfo.pVmTextProtectRtn = vmTextPageProtect;
    vmLibInfo.vmLibInstalled    = TRUE;
    cacheMmuAvailable		= TRUE;

    cacheFuncsSet ();			/* update cache funtion pointers */

    return (OK);
    }

/*******************************************************************************
*
* vmGlobalMapInit - initialize global mapping (VxVMI Option)
*
* This routine is a convenience routine that creates and installs a virtual
* memory context with global mappings defined for each contiguous memory
* segment defined in the physical memory descriptor array passed as an
* argument.  The context ID returned becomes the current virtual memory
* context.  
*
* The physical memory descriptor also contains state information
* used to initialize the state information in the MMU's translation table
* for that memory segment.  The following state bits may be or'ed together:
*
* .TS
* tab(|);
* l2 l2 l .
* VM_STATE_VALID    | VM_STATE_VALID_NOT    | valid/invalid
* VM_STATE_WRITABLE | VM_STATE_WRITABLE_NOT | writable/write-protected
* VM_STATE_CACHEABLE| VM_STATE_CACHEABLE_NOT| cacheable/not-cacheable
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
* If the <enable> parameter is TRUE, the MMU is enabled upon return.  
* The vmGlobalMapInit() routine should be called only after vmLibInit() has been
* called.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: A pointer to a newly created virtual memory context, or
* NULL if the memory cannot be mapped.
*/

VM_CONTEXT_ID vmGlobalMapInit 
    (
    PHYS_MEM_DESC *pMemDescArray, 	/* pointer to array of mem descs    */
    int numDescArrayElements,		/* num of elements in pMemDescArray */
    BOOL enable				/* enable virtual memory            */
    )
    {
    int i;
    PHYS_MEM_DESC *thisDesc;

    /* set up global transparent mapping of physical to virtual memory */

    for (i = 0; i < numDescArrayElements; i++)
        {
        thisDesc = &pMemDescArray[i];

        /* map physical directly to virtual and set initial state */

        if (vmGlobalMap ((void *) thisDesc->virtualAddr, 
			 (void *) thisDesc->physicalAddr,
			 thisDesc->len) == ERROR)
            {
	    return (NULL);
            }
        }

    /* create default virtual memory context */

    if (vmContextInit (&sysVmContext) == ERROR)
	{
	return (NULL);
	}

    /* set the state of all the global memory we just created */ 

    for (i = 0; i < numDescArrayElements; i++)
        {
        thisDesc = &pMemDescArray[i];

	if (vmStateSet (&sysVmContext, thisDesc->virtualAddr, thisDesc->len,
		thisDesc->initialStateMask, thisDesc->initialState) == ERROR)
	    return (NULL);
        }

    currentContext = &sysVmContext;
    MMU_CURRENT_SET (sysVmContext.mmuTransTbl);

    if (enable)
	if (MMU_ENABLE (TRUE) == ERROR)
	    return (NULL);

    return (&sysVmContext);
    }

/*******************************************************************************
*
* vmContextCreate - create a new virtual memory context (VxVMI Option)
*
* This routine creates a new virtual memory context.  The newly created
* context does not become the current context until explicitly installed by
* a call to vmCurrentSet().  Modifications to the context state (mappings,
* state changes, etc.) may be performed on any virtual memory context, even
* if it is not the current context.
*
* This routine should not be called from interrupt level.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: A pointer to a new virtual memory context, or 
* NULL if the allocation or initialization fails.
*/

VM_CONTEXT_ID vmContextCreate (void)
    {
    VM_CONTEXT_ID context;

    /* call the vm class's memory allocator to get memory for the object */

    context = (VM_CONTEXT *) objAlloc ((OBJ_CLASS *) vmContextClassId);

    if (context == NULL)
	return (NULL);

    if (vmContextInit (context) == ERROR)
	{
	objFree ((OBJ_CLASS *) vmContextClassId, (char *) context);
	return (NULL);
	}
    
    return (context);
    }

/*******************************************************************************
*
* vmContextInit - initialize VM_CONTEXT structures
*
* This routine may be used to initialize static definitions of VM_CONTEXT
* structures, instead of dynamically creating the object with
* vmContextCreate().  Note that virtual memory contexts created in this
* manner may not be deleted.
*
* This routine should not be called from interrupt level.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: OK, or ERROR if the translation table cannot be created.
*
* NOMANUAL
*/

STATUS vmContextInit 
    (
    VM_CONTEXT *pContext
    )
    {
    objCoreInit (&pContext->objCore, (CLASS_ID) vmContextClassId);

    semMInit (&pContext->sem, mutexOptionsVmLib);

    pContext->mmuTransTbl = MMU_TRANS_TBL_CREATE ();

    if (pContext->mmuTransTbl == NULL)
	return (ERROR);

    lstAdd (&vmContextList, &pContext->links);
    return (OK);
    }

/*******************************************************************************
*
* vmContextDelete - delete a virtual memory context (VxVMI Option)
*
* This routine deallocates the underlying translation table associated with
* a virtual memory context.  It does not free the physical memory already
* mapped into the virtual memory space.
*
* This routine should not be called from interrupt level.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: OK, or ERROR if <context> is not a valid context descriptor or
* if an error occurs deleting the translation table.
*/

STATUS vmContextDelete 
    (
    VM_CONTEXT_ID context
    )
    {

    if (OBJ_VERIFY (context, vmContextClassId) != OK)
	return (ERROR);

    /* take the context's mutual exclusion semaphore - this is really 
     * inadequate.
     */

    semTake (&context->sem, WAIT_FOREVER);

    /* invalidate the object */

    objCoreTerminate (&context->objCore);

    if (MMU_TRANS_TBL_DELETE (context->mmuTransTbl) == ERROR)
	return (ERROR);

    lstDelete (&vmContextList, &context->links);

    free (context);
    return (OK);
    }

/*******************************************************************************
*
* vmStateSet - change the state of a block of virtual memory (VxVMI Option)
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
* l2 l2 l .
* VM_STATE_VALID     | VM_STATE_VALID_NOT     | valid/invalid
* VM_STATE_WRITABLE  | VM_STATE_WRITABLE_NOT  | writable/write-protected
* VM_STATE_CACHEABLE | VM_STATE_CACHEABLE_NOT | cacheable/not-cacheable
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
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: OK or, ERROR if the validation fails, <pVirtual> is not on a page
* boundary, <len> is not a multiple of page size, or the
* architecture-dependent state set fails for the specified virtual address.
*
* ERRNO: 
* S_vmLib_NOT_PAGE_ALIGNED,
* S_vmLib_BAD_STATE_PARAM,
* S_vmLib_BAD_MASK_PARAM
*/

STATUS vmStateSet 
    (
    VM_CONTEXT_ID context, 	/* context - NULL == currentContext         */
    void *pVirtual, 		/* virtual address to modify state of       */
    int len, 			/* len of virtual space to modify state of  */
    UINT stateMask, 		/* state mask                               */
    UINT state			/* state                                    */
    )
    {
    FAST int	pageSize 	= vmPageSize;
    FAST char *	thisPage 	= (char *) pVirtual;
    FAST UINT 	numBytesProcessed = 0;
    UINT 	archDepState;
    UINT 	archDepStateMask;
    STATUS 	retVal 		= OK;

    if (!vmLibInfo.vmLibInstalled)
	return (ERROR);

    if (context == NULL)
	context = currentContext;

    if (OBJ_VERIFY (context, vmContextClassId) != OK)
	return (ERROR);

    if (NOT_PAGE_ALIGNED (thisPage))
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    if (NOT_PAGE_ALIGNED (len))
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

    archDepState = vmStateTransTbl [state];
    archDepStateMask = vmMaskTransTbl [stateMask];

    while (numBytesProcessed < len)
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

    return (retVal);
    }

/*******************************************************************************
*
* vmStateGet - get the state of a page of virtual memory (VxVMI Option)
*
* This routine extracts state bits with the following masks: 
*
*  VM_STATE_MASK_VALID
*  VM_STATE_MASK_WRITABLE
*  VM_STATE_MASK_CACHEABLE
*
* Individual states may be identified with the following constants:
*
* .TS
* tab(|);
* l2 l2 l .
* VM_STATE_VALID    | VM_STATE_VALID_NOT     | valid/invalid
* VM_STATE_WRITABLE | VM_STATE_WRITABLE_NOT  | writable/write-protected
* VM_STATE_CACHEABLE| VM_STATE_CACHEABLE_NOT | cacheable/not-cacheable
* .TE
*
* For example, to see if a page is writable, the following code would be used:
*
* .CS
*     vmStateGet (vmContext, pageAddr, &state);
*     if ((state & VM_STATE_MASK_WRITABLE) & VM_STATE_WRITABLE)
*        ...
* .CE
*
* If <context> is specified as NULL, the current virtual memory context 
* is used.
*
* This routine is callable from interrupt level.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: OK, or ERROR if <pageAddr> is not on a page boundary, the
* validity check fails, or the architecture-dependent state get fails for
* the specified virtual address.
*
* ERRNO: S_vmLib_NOT_PAGE_ALIGNED
*/

STATUS vmStateGet 
    (
    VM_CONTEXT_ID context, 	/* context - NULL == currentContext */
    void *pPageAddr, 		/* virtual page addr                */
    UINT *pState		/* where to return state            */
    )
    {
    UINT archDepStateGotten;
    int j;

    if (context == NULL)
	context = currentContext;

    if (OBJ_VERIFY (context, vmContextClassId) != OK)
	return (ERROR);

    if (NOT_PAGE_ALIGNED (pPageAddr))
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    *pState = 0;

    if (MMU_STATE_GET (context->mmuTransTbl,
		       pPageAddr, &archDepStateGotten) == ERROR)
	return (ERROR);

    /* translate from arch dependent state to arch independent state */

    for (j = 0; j < mmuStateTransArraySize; j++)
	{
	STATE_TRANS_TUPLE *thisTuple = &mmuStateTransArray[j];
	UINT archDepMask = thisTuple->archDepMask;
	UINT archDepState = thisTuple->archDepState;
	UINT archIndepState = thisTuple->archIndepState;

	if ((archDepStateGotten & archDepMask) == archDepState)
	    *pState |= archIndepState;
	}

    return (OK);
    }

/*******************************************************************************
*
* vmMap - map physical space into virtual space (VxVMI Option)
*
* This routine maps physical pages into a contiguous block of virtual
* memory.  <virtualAddr> and <physicalAddr> must be on page boundaries, and
* <len> must be evenly divisible by the page size.  After the call to
* vmMap(), the state of all pages in the the newly mapped virtual memory is
* valid, writable, and cacheable.
*
* The vmMap() routine can fail if the specified virtual address space
* conflicts with the translation tables of the global virtual memory space.
* The global virtual address space is architecture-dependent and is
* initialized at boot time with calls to vmGlobalMap() by
* vmGlobalMapInit().  If a conflict results, `errno' is set to
* S_vmLib_ADDR_IN_GLOBAL_SPACE.  To avoid this conflict, use
* vmGlobalInfoGet() to ascertain which portions of the virtual address space
* are reserved for the global virtual address space.  If <context> is
* specified as NULL, the current virtual memory context is used.
*
* This routine should not be called from interrupt level.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: OK, or ERROR if <virtualAddr> or <physicalAddr> are not 
* on page boundaries, <len> is not a multiple of the page size, 
* the validation fails, or the mapping fails.
*
* ERRNO:
* S_vmLib_NOT_PAGE_ALIGNED,
* S_vmLib_ADDR_IN_GLOBAL_SPACE
*/

STATUS vmMap 
    (
    VM_CONTEXT_ID context, 	/* context - NULL == currentContext   */
    void *virtualAddr, 		/* virtual address                    */
    void *physicalAddr,		/* physical address                   */
    UINT len			/* len of virtual and physical spaces */
    )
    {
    int pageSize = vmPageSize;
    char *thisVirtPage = (char *) virtualAddr;
    char *thisPhysPage = (char *) physicalAddr;
    FAST UINT numBytesProcessed = 0;
    STATUS retVal = OK;

    if (context == NULL)
	context = currentContext;

    if (OBJ_VERIFY (context, vmContextClassId) != OK)
	return (ERROR);

    if (NOT_PAGE_ALIGNED (thisVirtPage))
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    if ((!mmuPhysAddrShift) && (NOT_PAGE_ALIGNED (thisPhysPage)))
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    if (NOT_PAGE_ALIGNED (len))
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    /* take mutual exclusion semaphore to protect translation table */

    semTake (&context->sem, WAIT_FOREVER);

    while (numBytesProcessed < len)
	{
	/* make sure there isn't a conflict with global virtual memory */

	if (ADDR_IN_GLOBAL_SPACE (thisVirtPage))
	    {
	    errno = S_vmLib_ADDR_IN_GLOBAL_SPACE;
	    retVal = ERROR;
	    break;
	    }

	if (MMU_PAGE_MAP (context->mmuTransTbl, thisVirtPage, 
			thisPhysPage) == ERROR)
	    {
	    retVal = ERROR;
	    break;
	    }

        if (vmStateSet (context, thisVirtPage, pageSize,
			 VM_STATE_MASK_VALID     |
			 VM_STATE_MASK_WRITABLE  |
			 VM_STATE_MASK_CACHEABLE |
			 VM_STATE_MASK_EX_CACHEABLE,
			 VM_STATE_VALID    |
			 VM_STATE_WRITABLE |
			 VM_STATE_CACHEABLE|
			 VM_STATE_EX_CACHEABLE) == ERROR)
	    {
	    retVal = ERROR;
	    break;
	    }

	thisVirtPage += pageSize;
	thisPhysPage += (mmuPhysAddrShift ? 1 : pageSize);
	numBytesProcessed += pageSize;
	}

    semGive (&context->sem);

    return (retVal);
    }

/*******************************************************************************
*
* vmGlobalMap - map physical pages to virtual space in shared global virtual memory (VxVMI Option)
*
* This routine maps physical pages to virtual space that is shared by all
* virtual memory contexts.  Calls to vmGlobalMap() should be made before any
* virtual memory contexts are created to insure that the shared global
* mappings are included in all virtual memory contexts.  Mappings created
* with vmGlobalMap() after virtual memory contexts are created are not
* guaranteed to appear in all virtual memory contexts.  After the call to
* vmGlobalMap(), the state of all pages in the the newly mapped virtual
* memory is unspecified and must be set with a call to vmStateSet(), once
* the initial virtual memory context is created.
*
* This routine should not be called from interrupt level.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: OK, or ERROR if <virtualAddr> or <physicalAddr> are not 
* on page boundaries, <len> is not a multiple of the page size, 
* or the mapping fails.
*
* ERRNO: S_vmLib_NOT_PAGE_ALIGNED
*/

STATUS vmGlobalMap 
    (
    void *virtualAddr, 		/* virtual address                    */
    void *physicalAddr,		/* physical address                   */
    UINT len			/* len of virtual and physical spaces */
    )
    {
    int 	pageSize 		= vmPageSize;
    char *	thisVirtPage 		= (char *) virtualAddr;
    char *	thisPhysPage 		= (char *) physicalAddr;
    FAST UINT 	numBytesProcessed 	= 0;
    STATUS 	retVal 			= OK;

    if (!vmLibInfo.vmLibInstalled)
	return (ERROR);

    if (NOT_PAGE_ALIGNED (thisVirtPage))
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    if ((!mmuPhysAddrShift) && (NOT_PAGE_ALIGNED (thisPhysPage)))
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    if (NOT_PAGE_ALIGNED (len))
	{
	errno = S_vmLib_NOT_PAGE_ALIGNED;
        return (ERROR); 
	}

    semTake (&globalMemMutex, WAIT_FOREVER);

    while (numBytesProcessed < len)
	{
	if (MMU_GLOBAL_PAGE_MAP (thisVirtPage, thisPhysPage) == ERROR)
	    {
	    retVal = ERROR;
	    break;
	    }

	/* mark the block containing the page in the globalPageBlockArray as 
         * being global 
	 */

        globalPageBlockArray[(unsigned) thisVirtPage / mmuPageBlockSize] = TRUE;

	thisVirtPage += pageSize;
	thisPhysPage += (mmuPhysAddrShift ? 1 : pageSize);
	numBytesProcessed += pageSize;
	}

    semGive (&globalMemMutex);

    return (retVal);
    }

/*******************************************************************************
*
* vmGlobalInfoGet - get global virtual memory information (VxVMI Option)
*
* This routine provides a description of those parts of the virtual memory
* space dedicated to global memory.  The routine returns a pointer to an
* array of UINT8.  Each element of the array corresponds to a block of
* virtual memory, the size of which is architecture-dependent and can be
* obtained with a call to vmPageBlockSizeGet().  To determine if a
* particular address is in global virtual memory, use the following code:
*
* .CS
*     UINT8 *globalPageBlockArray = vmGlobalInfoGet ();
*     int pageBlockSize = vmPageBlockSizeGet ();
*    
*     if (globalPageBlockArray[addr/pageBlockSize])
*        ...
* .CE
*
* The array pointed to by the returned pointer is guaranteed to be static as
* long as no calls are made to vmGlobalMap() while the array is being
* examined.  The information in the array can be used to determine what
* portions of the virtual memory space are available for use as private
* virtual memory within a virtual memory context.
*
* This routine is callable from interrupt level.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: A pointer to an array of UINT8.
*
* SEE ALSO: vmPageBlockSizeGet()
*/

UINT8 *vmGlobalInfoGet  (void)
    {
    return (globalPageBlockArray);
    }

/*******************************************************************************
*
* vmPageBlockSizeGet - get the architecture-dependent page block size (VxVMI Option)
*
* This routine returns the size of a page block for the current
* architecture.  Each MMU architecture constructs translation tables such
* that a minimum number of pages are pre-defined when a new section of the
* translation table is built.  This minimal group of pages is referred to as
* a "page block." This routine may be used in conjunction with
* vmGlobalInfoGet() to examine the layout of global virtual memory.
*
* This routine is callable from interrupt level.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: The page block size of the current architecture.
*
* SEE ALSO: vmGlobalInfoGet()
*/

int vmPageBlockSizeGet  (void)
    {
    return (mmuPageBlockSize);
    }

/*******************************************************************************
*
* vmTranslate - translate a virtual address to a physical address (VxVMI Option)
*
* This routine retrieves mapping information for a virtual address from the
* page translation tables.  If the specified virtual address has never been
* mapped, the returned status can be either OK or ERROR; however, if it is
* OK, then the returned physical address will be -1.  If <context> is
* specified as NULL, the current context is used.
*
* This routine is callable from interrupt level.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: OK, or ERROR if the validation or translation fails.
*/

STATUS vmTranslate 
    (
    VM_CONTEXT_ID context, 	/* context - NULL == currentContext */
    void *virtualAddr, 		/* virtual address                  */
    void **physicalAddr		/* place to put result              */
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

/*******************************************************************************
*
* vmPageSizeGet - return the page size (VxVMI Option)
*
* This routine returns the architecture-dependent page size.
*
* This routine is callable from interrupt level.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: The page size of the current architecture. 
*
*/

int vmPageSizeGet (void)
    {
    return (vmPageSize);
    }

/*******************************************************************************
*
* vmCurrentGet - get the current virtual memory context (VxVMI Option)
*
* This routine returns the current virtual memory context.
*
* This routine is callable from interrupt level.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: The current virtual memory context, or
* NULL if no virtual memory context is installed.
*/

VM_CONTEXT_ID vmCurrentGet  (void)
    {
    return (currentContext);
    }

/*******************************************************************************
*
* vmCurrentSet - set the current virtual memory context (VxVMI Option)
*
* This routine installs a specified virtual memory context.
*
* This routine is callable from interrupt level.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: OK, or ERROR if the validation or context switch fails.
*/

STATUS vmCurrentSet 
    (
    VM_CONTEXT_ID context		/* context to install */
    )
    {
    if (OBJ_VERIFY (context, vmContextClassId) != OK)
	return (ERROR);

    /* XXX do we need to flush the cpu's cache on a context switch?
     * yes, if the cache operates on virtual addresses (68k does)
     */

    cacheClear (INSTRUCTION_CACHE, 0, ENTIRE_CACHE);
    cacheClear (DATA_CACHE, 0, ENTIRE_CACHE);

    currentContext = context;
    MMU_CURRENT_SET (context->mmuTransTbl);
    return (OK);
    }

/*******************************************************************************
*
* vmEnable - enable or disable virtual memory (VxVMI Option)
*
* This routine turns virtual memory on and off.  Memory management should not
* be turned off once it is turned on except in the case of system shutdown.
*
* This routine is callable from interrupt level.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: OK, or ERROR if the validation or architecture-dependent code
* fails.
*/

STATUS vmEnable 
    (
    BOOL enable		/* TRUE == enable MMU, FALSE == disable MMU */
    )
    {
    return (MMU_ENABLE (enable));
    }

/*******************************************************************************
*
* vmTextProtect - write-protect a text segment (VxVMI Option)
*
* This routine write-protects the VxWorks text segment and sets a flag so
* that all text segments loaded by the incremental loader will be
* write-protected.  The routine should be called after both vmLibInit() and
* vmGlobalMapInit() have been called.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled virtual memory
* support option, VxVMI.
*
* RETURNS: OK, or ERROR if the text segment cannot be write-protected.
*
* ERRNO: S_vmLib_TEXT_PROTECTION_UNAVAILABLE
*/ 

STATUS vmTextProtect (void)
    {
    UINT begin;
    UINT end;
#if (CPU==SH7750 || CPU==SH7729 || CPU==SH7700)
    UINT memBase = (UINT) etext & 0xe0000000;	/* identify logical space */
#endif

    if (!vmLibInfo.vmLibInstalled)
	{
	errno = S_vmLib_TEXT_PROTECTION_UNAVAILABLE;
	return (ERROR);
	}
#if (CPU==SH7750 || CPU==SH7729 || CPU==SH7700)
    begin = (((UINT) sysInit & 0x1fffffff) | memBase) / vmPageSize * vmPageSize;
#else
    begin = (UINT) sysInit / vmPageSize * vmPageSize;
#endif
    end	  = (UINT) etext   / vmPageSize * vmPageSize + vmPageSize;

    vmLibInfo.protectTextSegs = TRUE;

    return (vmStateSet (0, (void *) begin, end - begin, VM_STATE_MASK_WRITABLE,
			VM_STATE_WRITABLE_NOT));
    }

/*******************************************************************************
*
* vmTextPageProtect - protect or unprotect a page of the text segment
*
* RETURNS: 
*/

LOCAL STATUS vmTextPageProtect
    (
    void *textPageAddr,  /* page to change */
    BOOL protect	 /* TRUE = write protect, FALSE = write enable */
    )
    {
    UINT newState = (protect ? VM_STATE_WRITABLE_NOT : VM_STATE_WRITABLE);
    int retVal = ERROR;
    
    if (vmLibInfo.protectTextSegs)
	retVal = vmStateSet (NULL, textPageAddr, vmPageSize, 
			     VM_STATE_MASK_WRITABLE, newState);

    return (retVal);
    }

#endif	/* (CPU_FAMILY != MIPS) && (CPU_FAMILY != PPC) */

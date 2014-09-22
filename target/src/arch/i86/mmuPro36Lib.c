/* mmuPro36Lib.c - MMU library for PentiumPro/2/3/4 36 bit mode */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01i,23may02,hdn  aligned PT/DT at mmuPageSize boundary for 4K/2MB page size
		 imported updates from mmuPro32Lib.c
01h,16may02,hdn  moved the GDT reloading to sysPhysMemTop() in sysLib.c
01g,27aug01,hdn  made MMU_TLB_FLUSH, updated MMU_LOCK/MMU_UNLOCK macros.
		 initialized oldIntLev to shut off warnings.
01f,09feb99,wsl  add comment to document ERRNO value
01e,17sep98,hdn  renamed mmuEnabled to mmuPro36Enabled.
01d,13apr98,hdn  added support for 4KB/2MB page for PentiumPro w PAE.
01c,07jan95,hdn  re-initialized the GDT in mmuLibInit().
01b,01nov94,hdn  added a support for COPY_BACK cache mode for Pentium.
01a,26jul93,hdn  written based on mc68k's version.
*/

/*
DESCRIPTION:

mmuPro36Lib.c provides the architecture dependent routines that directly control
the memory management unit.  It provides 10 routines that are called by the
higher level architecture independent routines in vmLib.c: 

 mmuPro36LibInit() - initialize module
 mmuTransTblCreate() - create a new translation table
 mmuTransTblDelete() - delete a translation table.
 mmuPro36Enable() - turn MMU on or off
 mmuStateSet() - set state of virtual memory page
 mmuStateGet() - get state of virtual memory page
 mmuPageMap() - map physical memory page to virtual memory page
 mmuGlobalPageMap() - map physical memory page to global virtual memory page
 mmuTranslate() - translate a virtual address to a physical address
 mmuCurrentSet() - change active translation table

Applications using the MMU will never call these routines directly; 
the visible interface is supported in vmLib.c.

mmuLib supports the creation and maintenance of multiple translation tables,
one of which is the active translation table when the MMU is enabled.  
Note that VxWorks does not include a translation table as part of the task
context;  individual tasks do not reside in private virtual memory.  However,
we include the facilities to create multiple translation tables so that
the user may create "private" virtual memory contexts and switch them in an
application specific manner.  New
translation tables are created with a call to mmuTransTblCreate(), and installed
as the active translation table with mmuCurrentSet().  Translation tables
are modified and potentially augmented with calls to mmuPageMap() and 
mmuStateSet().
The state of portions of the translation table can be read with calls to 
mmuStateGet() and mmuTranslate().

The traditional VxWorks architecture and design philosophy requires that all
objects and operating systems resources be visible and accessible to all agents
(tasks, isrs, watchdog timers, etc) in the system.  This has traditionally been
insured by the fact that all objects and data structures reside in physical 
memory; thus, a data structure created by one agent may be accessed by any
other agent using the same pointer (object identifiers in VxWorks are often
pointers to data structures.) This creates a potential 
problem if you have multiple virtual memory contexts.  For example, if a
semaphore is created in one virtual memory context, you must guarantee that
that semaphore will be visible in all virtual memory contexts if the semaphore
is to be accessed at interrupt level, when a virtual memory context other than
the one in which it was created may be active. Another example is that
code loaded using the incremental loader from the shell must be accessible
in all virtual memory contexts, since code is shared by all agents in the
system.

This problem is resolved by maintaining a global "transparent" mapping
of virtual to physical memory for all the contiguous segments of physical 
memory (on board memory, i/o space, sections of vme space, etc) that is shared
by all translation tables;  all available  physical memory appears at the same 
address in virtual memory in all virtual memory contexts. This technique 
provides an environment that allows
resources that rely on a globally accessible physical address to run without
modification in a system with multiple virtual memory contexts.

An additional requirement is that modifications made to the state of global 
virtual memory in one translation table appear in all translation tables.  For
example, memory containing the text segment is made read only (to avoid
accidental corruption) by setting the appropriate writable bits in the 
translation table entries corresponding to the virtual memory containing the 
text segment.  This state information must be shared by all virtual memory 
contexts, so that no matter what translation table is active, the text segment
is protected from corruption.  The mechanism that implements this feature is
architecture dependent, but usually entails building a section of a 
translation table that corresponds to the global memory, that is shared by
all other translation tables.  Thus, when changes to the state of the global
memory are made in one translation table, the changes are reflected in all
other translation tables.

mmuLib provides a separate call for constructing global virtual memory -
mmuGlobalPageMap() - which creates translation table entries that are shared
by all translation tables.  Initialization code in usrConfig makes calls
to vmGlobalMap() (which in turn calls mmuGlobalPageMap()) to set up global 
transparent virtual memory for all
available physical memory.  All calls made to mmuGlobalPageMap() must occur 
before any virtual memory contexts are created;  changes made to global virtual
memory after virtual memory contexts are created are not guaranteed to be 
reflected in all virtual memory contexts.

Most MMU architectures will dedicate some fixed amount of virtual memory to 
a minimal section of the translation table (a "segment", or "block").  This 
creates a problem in that the user may map a small section of virtual memory
into the global translation tables, and then attempt to use the virtual memory
after this section as private virtual memory.  The problem is that the 
translation table entries for this virtual memory are contained in the global 
translation tables, and are thus shared by all translation tables.  This 
condition is detected by vmMap, and an error is returned, thus, the lower
level routines in mmuPro36Lib.c (mmuPageMap(), mmuGlobalPageMap()) need not 
perform any error checking.

A global variable `mmuPageBlockSize' should be defined which is equal to 
the minimum virtual segment size.  

This module supports the PentiumPro/2/3/4 MMU:

.CS
	    PDBR
	     |
	     |
            -------------------------
            |pdp  |pdp  |pdp  |pdp  |
            -------------------------
	     |
             v
            -------------------------------------
 top level  |pde  |pde  |pde  |pde  |pde  |pde  | ... 
            -------------------------------------
	       |     |     |     |     |     |    
	       |     |     |     |     |     |    
      ----------     |     v     v     v     v
      |         ------    NULL  NULL  NULL  NULL
      |         |
      v         v
     ----     ----   
l   |pte |   |pte |
o    ----     ----
w   |pte |   |pte |     
e    ----     ----
r   |pte |   |pte |
l    ----     ----
e   |pte |   |pte |
v    ----     ----
e     .         .
l     .         .
      .         .

.CE

where the top level consists of two tables that are the page directory
pointer table and the page directory table which is an array of pointers 
(Page Directory Entry) held within a single 4k page.  These point to 
arrays of Page Table Entry arrays in the lower level.  Each of these 
lower level arrays is also held within a single 4k page, and describes 
a virtual space of 2 MB (each Page Table Entry is 8 bytes, so we get 
512 of these in each array, and each Page Table Entry maps a 4KB page - 
thus 512 * 4096 = 2MB.)  

To implement global virtual memory, a separate translation table called 
mmuGlobalTransTbl is created when the module is initialized.  Calls to 
mmuGlobalPageMap will augment and modify this translation table.  When new
translation tables are created, memory for the top level array of sftd's is
allocated and initialized by duplicating the pointers in mmuGlobalTransTbl's
top level sftd array.  Thus, the new translation table will use the global
translation table's state information for portions of virtual memory that are
defined as global.  Here's a picture to illustrate:

.CS
	         GLOBAL TRANS TBL		      NEW TRANS TBL

 		       PDBR				   PDBR
		        |				    |
		        |				    |
            -------------------------           -------------------------
            |pdp  |pdp  |pdp  |pdp  |           |pdp  |pdp  |pdp  |pdp  |
            -------------------------           -------------------------
	     |                                   |
             v                                   v
            -------------------------           -------------------------
 top level  |pde  |pde  | NULL| NULL|           |pde  |pde  | NULL| NULL|
            -------------------------           -------------------------
	       |     |     |     |                 |     |     |     |   
	       |     |     |     |                 |     |     |     |  
      ----------     |     v     v        ----------     |     v     v
      |         ------    NULL  NULL      |		 |    NULL  NULL
      |         |			  |		 |
      o------------------------------------		 |
      |		|					 |
      |		o-----------------------------------------
      |		|
      v         v
     ----     ----   
l   |pte |   |pte |
o    ----     ----
w   |pte |   |pte |     
e    ----     ----
r   |pte |   |pte |
l    ----     ----
e   |pte |   |pte |
v    ----     ----
e     .         .
l     .         .
      .         .
.CE

Note that with this scheme, the global memory granularity is 4MB.  Each time
you map a section of global virtual memory, you dedicate at least 4MB of 
the virtual space to global virtual memory that will be shared by all virtual
memory contexts.

The physical memory that holds these data structures is obtained from the
system memory manager via memalign to insure that the memory is page
aligned.  We want to protect this memory from being corrupted,
so we invalidate the descriptors that we set up in the global translation
that correspond to the memory containing the translation table data structures.
This creates a "chicken and the egg" paradox, in that the only way we can
modify these data structures is through virtual memory that is now invalidated,
and we can't validate it because the page descriptors for that memory are
in invalidated memory (confused yet?)
So, you will notice that anywhere that page table descriptors (PTE's)
are modified, we do so by locking out interrupts, momentarily disabling the 
MMU, accessing the memory with its physical address, enabling the MMU, and
then re-enabling interrupts (see mmuStateSet(), for example.)

Support for two new page attribute bits are added for PentiumPro's enhanced
MMU.  They are Global bit (G) and Page-level write-through/back bit (PWT).
Global bit indicates a global page when set.  When a page is marked global and
the page global enable (PGE) bit in register CR4 is set, the page-table or
page-directory entry for the page is not invalidated in the TLB when register
CR3 is loaded or a task switch occurs.  This bit is provided to prevent
frequently used pages (such as pages that contain kernel or other operating
system or executive code) from being flushed from the TLB.
Page-level write-through/back bit (PWT) controls the write-through or write-
back caching policy of individual pages or page tables.  When the PWT bit is
set, write-through caching is enabled for the associated page or page table.
When the bit is clear, write-back caching is enabled for the associated page
and page table.
Following macros are used to describe these attribute bits in the physical
memory descriptor table sysPhysMemDesc[] in sysLib.c.

 VM_STATE_WBACK - use write-back cache policy for the page 
 VM_STATE_WBACK_NOT - use write-through cache policy for the page 
 VM_STATE_GLOBAL - set page global bit
 VM_STATE_GLOBAL_NOT - not set page global bit

Support for two page size (4KB and 2MB) are added also.
The linear address for 4KB pages is divided into four sections:

 Page directory pointer - bits 30 through 31.
 Page directory entry - bits 21 through 29.
 Page table entry - Bits 12 through 20.
 Page offset - Bits  0 through 11.

The linear address for 2MB pages is divided into three sections:

 Page directory pointer - bits 30 through 31.
 Page directory entry - Bits 21 through 29.
 Page offset - Bits  0 through 20.

These two page size is configurable by VM_PAGE_SIZE macro in config.h.

*/


/* includes */

#include "vxWorks.h"
#include "string.h"
#include "intLib.h"
#include "stdlib.h"
#include "memLib.h"
#include "private/vmLibP.h"
#include "arch/i86/mmuPro36Lib.h"
#include "arch/i86/vxI86Lib.h"
#include "mmuLib.h"
#include "errno.h"
#include "cacheLib.h"
#include "regs.h"
#include "sysLib.h"


/* defines */

/*
 * MMU_WP_UNLOCK and MMU_WP_LOCK are used to access page table entries that
 * are in virtual memory that has been invalidated to protect it from being
 * corrupted.  The write protection bit is toggled to stay in the virtual
 * address space.
 * MMU_UNLOCK and MMU_LOCK are used to allocate memory for Directory/Page
 * Table from the initial system memory in the 1st 32 bit physical address
 * space. Thus, for all the Directory/Page Tables', the upper 4 bits of 
 * 36 bit physical address becomes zero.  This makes this library simple.
 * Allocated memory can be freed from the virtual address space
 * assuming the transparent mapping of the initial system memory.
 * These four macros do not invoke function calls. Therefore, we do not 
 * need to worry about stack mismatch in the virtual/physical address 
 * space, or the called out function's disappearance.  It is guaranteed 
 * that this code is in physical memory.
 */

#define MMU_ON() \
    do { int tempReg; WRS_ASM ("movl %%cr0,%0; orl $0x80010000,%0; \
    movl %0,%%cr0; jmp 0f; 0:" : "=r" (tempReg) : : "memory"); } while (0)

#define MMU_OFF() \
    do { int tempReg; WRS_ASM ("movl %%cr0,%0; andl $0x7ffeffff,%0; \
    movl %0,%%cr0; jmp 0f; 0:" : "=r" (tempReg) : : "memory"); } while (0)

#define MMU_UNLOCK(wasEnabled, oldLevel) \
    do { if (((wasEnabled) = mmuPro36Enabled) == TRUE) \
	{INT_LOCK (oldLevel); MMU_OFF (); } \
    } while (0)

#define MMU_LOCK(wasEnabled, oldLevel) \
    do { if ((wasEnabled) == TRUE) \
        {MMU_ON (); INT_UNLOCK (oldLevel);} \
    } while (0)

#define WP_ON() \
    do { int tempReg; WRS_ASM ("movl %%cr0,%0; orl $0x00010000,%0; \
    movl %0,%%cr0; jmp 0f; 0:" : "=r" (tempReg) : : "memory"); } while (0)

#define WP_OFF() \
    do { int tempReg; WRS_ASM ("movl %%cr0,%0; andl $0xfffeffff,%0; \
    movl %0,%%cr0; jmp 0f; 0:" : "=r" (tempReg) : : "memory"); } while (0)

#define MMU_WP_UNLOCK(wasEnabled, oldLevel) \
    do { if (((wasEnabled) = mmuPro36Enabled) == TRUE) \
	{INT_LOCK (oldLevel); WP_OFF (); } \
    } while (0)

#define MMU_WP_LOCK(wasEnabled, oldLevel) \
    do { if ((wasEnabled) == TRUE) \
        {WP_ON (); INT_UNLOCK (oldLevel);} \
    } while (0)

/* inline version of mmuI86TLBFlush() */

#define	MMU_TLB_FLUSH() \
    do { int tempReg; WRS_ASM ("pushfl; cli; movl %%cr3,%0; \
    movl %0,%%cr3; jmp 0f; 0: popfl; " : "=r" (tempReg) : : "memory"); \
    } while (0)


/* imports */

IMPORT STATE_TRANS_TUPLE * mmuStateTransArray;
IMPORT int	mmuStateTransArraySize;
IMPORT MMU_LIB_FUNCS mmuLibFuncs;
IMPORT int	mmuPageBlockSize;
IMPORT int	sysProcessor;
IMPORT CPUID	sysCpuId;


/* globals */

BOOL		mmuPro36Enabled = FALSE;


/* locals */

LOCAL UINT32	mmuPageSize;
LOCAL UINT32	mmuPdpTableNumEnt = 4;
LOCAL BOOL	firstTime = TRUE;
LOCAL UINT32	nDirPages = 0;	/* number of pages for Directory Table */

/* 
 * a translation table to hold the descriptors for the global transparent
 * translation of physical to virtual memory 
 */

LOCAL MMU_TRANS_TBL mmuGlobalTransTbl;

/* initially, the current trans table is a dummy table with mmu disabled */

LOCAL MMU_TRANS_TBL * mmuCurrentTransTbl = &mmuGlobalTransTbl;

/* 
 * array of booleans used to keep track of sections of virtual memory 
 * defined as global.
 */

LOCAL UINT8 * globalPageBlock;

LOCAL STATE_TRANS_TUPLE mmuStateTransArrayLocal [] =
    {
    {VM_STATE_MASK_VALID, MMU_STATE_MASK_VALID, 
     VM_STATE_VALID, MMU_STATE_VALID},

    {VM_STATE_MASK_VALID, MMU_STATE_MASK_VALID, 
     VM_STATE_VALID_NOT, MMU_STATE_VALID_NOT},

    {VM_STATE_MASK_WRITABLE, MMU_STATE_MASK_WRITABLE,
     VM_STATE_WRITABLE, MMU_STATE_WRITABLE},

    {VM_STATE_MASK_WRITABLE, MMU_STATE_MASK_WRITABLE,
     VM_STATE_WRITABLE_NOT, MMU_STATE_WRITABLE_NOT},

    {VM_STATE_MASK_CACHEABLE, MMU_STATE_MASK_CACHEABLE,
     VM_STATE_CACHEABLE, MMU_STATE_CACHEABLE},

    {VM_STATE_MASK_CACHEABLE, MMU_STATE_MASK_CACHEABLE,
     VM_STATE_CACHEABLE_NOT, MMU_STATE_CACHEABLE_NOT},

    {VM_STATE_MASK_WBACK, MMU_STATE_MASK_WBACK,
     VM_STATE_WBACK, MMU_STATE_WBACK},

    {VM_STATE_MASK_WBACK, MMU_STATE_MASK_WBACK,
     VM_STATE_WBACK_NOT, MMU_STATE_WBACK_NOT},

    {VM_STATE_MASK_GLOBAL, MMU_STATE_MASK_GLOBAL,
     VM_STATE_GLOBAL, MMU_STATE_GLOBAL},

    {VM_STATE_MASK_GLOBAL, MMU_STATE_MASK_GLOBAL,
     VM_STATE_GLOBAL_NOT, MMU_STATE_GLOBAL_NOT}
    };


/* forward declarations */
 
LOCAL void	mmuMemPagesWriteDisable (MMU_TRANS_TBL * transTbl);
LOCAL STATUS	mmuPteGet (MMU_TRANS_TBL * pTransTbl, void * virtAddr,
			   PTE ** result);
LOCAL MMU_TRANS_TBL * mmuTransTblCreate ();
LOCAL STATUS	mmuTransTblInit (MMU_TRANS_TBL * newTransTbl);
LOCAL STATUS	mmuTransTblDelete (MMU_TRANS_TBL * transTbl);
LOCAL STATUS	mmuVirtualPageCreate (MMU_TRANS_TBL * thisTbl,
				      void * virtPageAddr);
LOCAL STATUS	mmuStateSet (MMU_TRANS_TBL * transTbl, void * pageAddr,
			     UINT stateMask, UINT state);
LOCAL STATUS	mmuStateGet (MMU_TRANS_TBL * transTbl, void * pageAddr,
			     UINT * state);
LOCAL STATUS	mmuPageMap (MMU_TRANS_TBL * transTbl, void * virtAddr,
			    void * physPage);
LOCAL STATUS	mmuGlobalPageMap (void * virtAddr, void * physPage);
LOCAL STATUS	mmuTranslate (MMU_TRANS_TBL * transTbl, void * virtAddr,
			      void ** physAddr);
LOCAL void	mmuCurrentSet (MMU_TRANS_TBL * transTbl);

LOCAL MMU_LIB_FUNCS mmuLibFuncsLocal =
    {
    mmuPro36LibInit,
    mmuTransTblCreate,
    mmuTransTblDelete,
    mmuPro36Enable,   
    mmuStateSet,
    mmuStateGet,
    mmuPageMap,
    mmuGlobalPageMap,
    mmuTranslate,
    mmuCurrentSet
    };


/******************************************************************************
*
* mmuPro36LibInit - initialize module
*
* Build a dummy translation table that will hold the page table entries
* for the global translation table.  The mmu remains disabled upon
* completion.
*
* RETURNS: OK if no error, ERROR otherwise
*
* ERRNO: S_mmuLib_INVALID_PAGE_SIZE
*/

STATUS mmuPro36LibInit 
    (
    int pageSize	/* system pageSize (must be 4KB or 2MB) */
    )
    {
    PTE * pDirectoryPtrTable;
    PTE * pDirectoryTable;
    INT32 ix;

    /* check if the PSE and PAE are supported */

    if ((sysCpuId.featuresEdx & (CPUID_PAE | CPUID_PSE)) !=
        (CPUID_PAE | CPUID_PSE))
        return (ERROR);

    /* initialize the data objects that are shared with vmLib.c */

    mmuStateTransArray = &mmuStateTransArrayLocal [0];

    mmuStateTransArraySize =
          sizeof (mmuStateTransArrayLocal) / sizeof (STATE_TRANS_TUPLE);

    mmuLibFuncs = mmuLibFuncsLocal;

    mmuPageBlockSize = PAGE_BLOCK_SIZE;

    /* we assume a 4KB or 2MB page size */

    if ((pageSize != PAGE_SIZE_4KB) && (pageSize != PAGE_SIZE_2MB))
	{
	errno = S_mmuLib_INVALID_PAGE_SIZE;
	return (ERROR);
	}

    mmuPro36Enabled = FALSE;

    mmuPageSize = pageSize;

    /* 
     * set number of pages for the page directory table
     * 2048 (4 * 512) PDEs * 8 byte = 16384 byte
     */

    nDirPages = (mmuPageSize == PAGE_SIZE_4KB) ? 4 : 1;

    /* 
     * allocate the global page block array to keep track of which parts
     * of virtual memory are handled by the global translation tables.
     * Allocate on page boundary so we can write protect it.
     */

    globalPageBlock = (UINT8 *) memalign (mmuPageSize, mmuPageSize);

    if (globalPageBlock == NULL)
	return (ERROR);

    bzero ((char *) globalPageBlock, mmuPageSize);

    /*
     * build a dummy translation table which will hold the PTE's for
     * global memory.  All real translation tables will point to this
     * one for controlling the state of the global virtual memory  
     */

    /* allocate pages to hold the directory table */

    pDirectoryTable = (PTE *) memalign (mmuPageSize, mmuPageSize * nDirPages);

    if (pDirectoryTable == NULL)
	{
	free (globalPageBlock);
	return (ERROR);
	}

    /* allocate a page to hold the directory pointer table */

    mmuGlobalTransTbl.pDirectoryTable = pDirectoryPtrTable = 
        (PTE *) memalign (mmuPageSize, mmuPageSize);

    if (pDirectoryPtrTable == NULL)
	{
	free (globalPageBlock);
	free (pDirectoryTable);
	return (ERROR);
	}

    bzero ((char *) pDirectoryPtrTable, mmuPageSize);

    /* validate all the directory pointer table entries */

    for (ix = 0; ix < mmuPdpTableNumEnt; ix++)
	{
        pDirectoryPtrTable[ix].bits[0]		= (UINT)pDirectoryTable + 
						  (PD_SIZE * ix);
        pDirectoryPtrTable[ix].bits[1]		= 0;
        pDirectoryPtrTable[ix].field.present	= 1;
        pDirectoryPtrTable[ix].field.pwt	= 0;
        pDirectoryPtrTable[ix].field.pcd	= 0;
	}

    /* invalidate all the directory table entries */

    for (ix = 0; ix < ((PD_SIZE * 4) / sizeof(PTE)); ix++)
	{
	pDirectoryTable[ix].field.present	= 0;
	pDirectoryTable[ix].field.rw		= 0;
	pDirectoryTable[ix].field.us		= 0;
	pDirectoryTable[ix].field.pwt		= 0;
	pDirectoryTable[ix].field.pcd		= 0;
	pDirectoryTable[ix].field.access	= 0;
	pDirectoryTable[ix].field.dirty		= 0;
	if (pageSize == PAGE_SIZE_4KB)
	    pDirectoryTable[ix].field.pagesize	= 0;
	else
	    pDirectoryTable[ix].field.pagesize	= 1;
	pDirectoryTable[ix].field.global	= 0;
	pDirectoryTable[ix].field.avail		= 0;
	pDirectoryTable[ix].field.page		= -1;
	pDirectoryTable[ix].field.page36	= 0;
	pDirectoryTable[ix].field.reserved	= 0;
	}

    /* set CR4 register: PAE=1 PSE=0/1 PGE=1 */

    if (pageSize == PAGE_SIZE_4KB)
        vxCr4Set ((vxCr4Get() & ~CR4_PSE) | (CR4_PAE | CR4_PGE));
    else
        vxCr4Set (vxCr4Get() | (CR4_PSE | CR4_PAE | CR4_PGE));

    return (OK);
    }

/******************************************************************************
*
* mmuPteGet - get the PTE for a given page
*
* mmuPteGet traverses a translation table and returns the (physical) address of
* the PTE for the given virtual address.
*
* RETURNS: OK or ERROR if there is no virtual space for the given address 
*
*/

LOCAL STATUS mmuPteGet 
    (
    MMU_TRANS_TBL *pTransTbl, 	/* translation table */
    void *virtAddr,		/* virtual address */ 
    PTE **result		/* result is returned here */
    )
    {
    PTE * pDte;			/* directory table entry */
    PTE * pDirectoryTable;	/* directory table */
    PTE * pPageTable;		/* page table */

    pDirectoryTable = (PTE *) (pTransTbl->pDirectoryTable->bits[0] & PDP_TO_ADDR);

    pDte = &pDirectoryTable 
	   [((UINT) virtAddr & (DIR_PTR_BITS | DIR_BITS)) >> DIR_INDEX];

    if (mmuPageSize == PAGE_SIZE_4KB)
	{
        pPageTable = (PTE *) (pDte->bits[0] & PTE_TO_ADDR_4KB); 

        if ((UINT) pPageTable == (0xffffffff & PTE_TO_ADDR_4KB))
	    return (ERROR);

        *result = &pPageTable [((UINT) virtAddr & TBL_BITS) >> TBL_INDEX];
	}
    else
	{
        if ((UINT) pDte == (0xffffffff & PTE_TO_ADDR_2MB))
	    return (ERROR);

        *result = pDte;
	}

    return (OK);
    }

/******************************************************************************
*
* mmuTransTblCreate - create a new translation table.
*
* create a i86 translation table.  Allocates space for the MMU_TRANS_TBL
* data structure and calls mmuTransTblInit on that object.  
*
* RETURNS: address of new object or NULL if allocation failed,
*          or NULL if initialization failed.
*/

LOCAL MMU_TRANS_TBL *mmuTransTblCreate 
    (
    )
    {
    MMU_TRANS_TBL * newTransTbl;
    INT32 oldIntLev = 0;
    BOOL wasEnabled;

    MMU_UNLOCK (wasEnabled, oldIntLev);
    newTransTbl = (MMU_TRANS_TBL *) malloc (sizeof (MMU_TRANS_TBL));
    MMU_LOCK (wasEnabled, oldIntLev);

    if (newTransTbl == NULL)
	return (NULL);

    if (mmuTransTblInit (newTransTbl) == ERROR)
	{
	free ((char *) newTransTbl);
	return (NULL);
	}

    return (newTransTbl);
    }

/******************************************************************************
*
* mmuTransTblInit - initialize a new translation table 
*
* Initialize a new translation table.  The directory table is copied from the
* global translation mmuGlobalTransTbl, so that we will share the global 
* virtual memory with all other translation tables.
* 
* RETURNS: OK or ERROR if unable to allocate memory for directory table.
*/

LOCAL STATUS mmuTransTblInit 
    (
    MMU_TRANS_TBL *newTransTbl		/* translation table to be inited */
    )
    {
    PTE * pDirectoryPtrTable;	/* directory pointer table */
    PTE * pDirectoryTable;	/* directory table */
    INT32 oldIntLev = 0;
    BOOL wasEnabled;
    INT32 ix;

    /* allocate a page to hold the directory pointer table */

    MMU_UNLOCK (wasEnabled, oldIntLev);
    newTransTbl->pDirectoryTable = pDirectoryPtrTable = 
	(PTE *) memalign (mmuPageSize, mmuPageSize);
    MMU_LOCK (wasEnabled, oldIntLev);

    if (pDirectoryPtrTable == NULL)
	return (ERROR);

    /* allocate pages to hold the directory table */

    MMU_UNLOCK (wasEnabled, oldIntLev);
    pDirectoryTable = (PTE *) memalign (mmuPageSize, mmuPageSize * nDirPages);
    MMU_LOCK (wasEnabled, oldIntLev);

    if (pDirectoryTable == NULL)
	{
	free (pDirectoryPtrTable);
	return (ERROR);
	}

    /* setup the directory pointer table */

    for (ix = 0; ix < mmuPdpTableNumEnt; ix++)
	{
        pDirectoryPtrTable[ix].bits[0]		= (UINT)pDirectoryTable +
					 	  (PD_SIZE * ix);
        pDirectoryPtrTable[ix].bits[1]		= 0;
        pDirectoryPtrTable[ix].field.present	= 1;
        pDirectoryPtrTable[ix].field.pwt	= 0;
        pDirectoryPtrTable[ix].field.pcd	= 0;
	}

    /* 
     * copy the directory table from the mmuGlobalTransTbl, 
     * so we get the global virtual memory 
     */

    bcopy ((char *) (mmuGlobalTransTbl.pDirectoryTable->bits[0] & PDP_TO_ADDR), 
	   (char *) pDirectoryTable, PD_SIZE * 4);

    /* 
     * write protect virtual memory pointing to the directory pointer table 
     * and the directory table in the global translation table to insure 
     * that it can't be corrupted 
     */

    for (ix = 0; ix < nDirPages; ix++)
	{
        mmuStateSet (&mmuGlobalTransTbl, 
		     (void *) ((UINT)pDirectoryTable + (mmuPageSize * ix)),
		     MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE_NOT);
	}

    mmuStateSet (&mmuGlobalTransTbl, (void *) pDirectoryPtrTable, 
		 MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE_NOT);

    return (OK);
    }

/******************************************************************************
*
* mmuTransTblDelete - delete a translation table.
* 
* mmuTransTblDelete deallocates all the memory used to store the translation
* table entries.  It does not deallocate physical pages mapped into the
* virtual memory space.
*
* RETURNS: OK
*
*/

LOCAL STATUS mmuTransTblDelete 
    (
    MMU_TRANS_TBL *transTbl		/* translation table to be deleted */
    )
    {
    PTE * pDte;
    PTE * pDirectoryTable;	/* directory table */
    PTE * pPageTable;
    INT32 ix;

    /* write enable the physical page containing the directory ptr table */

    mmuStateSet (&mmuGlobalTransTbl, transTbl->pDirectoryTable,
		 MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE);

    /* write enable the physical page containing the directory table */

    pDirectoryTable = (PTE *)(transTbl->pDirectoryTable->bits[0] & PDP_TO_ADDR);
    for (ix = 0; ix < nDirPages; ix++)
	{
	pDte = (PTE *)((UINT)pDirectoryTable + (mmuPageSize * ix));
        mmuStateSet (&mmuGlobalTransTbl, pDte, 
		      MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE);
	}

    /* deallocate only non-global page blocks */

    pDte = pDirectoryTable;
    if (mmuPageSize == PAGE_SIZE_4KB)
        for (ix = 0; ix < ((PD_SIZE * 4) / sizeof(PTE)); ix++, pDte++)
	    if ((pDte->field.present == 1) && !globalPageBlock[ix]) 
	        {
	        pPageTable = (PTE *) (pDte->bits[0] & PTE_TO_ADDR_4KB);
	        mmuStateSet (&mmuGlobalTransTbl, pPageTable,
		             MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE);
	        free (pPageTable);
	        }

    /* free the pages holding the directory table */

    free ((void *) (transTbl->pDirectoryTable->bits[0] & PDP_TO_ADDR));

    /* free the page holding the directory pointer table */

    free (transTbl->pDirectoryTable);

    /* free the translation table data structure */

    free (transTbl);
    
    return (OK);
    }

/******************************************************************************
*
* mmuVirtualPageCreate - set up translation tables for a virtual page
*
* simply check if there's already a page table that has a
* PTE for the given virtual page.  If there isn't, create one.
* this routine is called only if the page size is 4KB, since a page table
* is not necessary for 2MB or 4MB page sizes.
*
* RETURNS OK or ERROR if couldn't allocate space for a page table.
*/

LOCAL STATUS mmuVirtualPageCreate 
    (
    MMU_TRANS_TBL *thisTbl, 		/* translation table */
    void *virtPageAddr			/* virtual addr to create */
    )
    {
    PTE * pDirectoryTable;	/* directory table */
    PTE * pPageTable;		/* page table */
    PTE * pDte;			/* directory table entry */
    PTE * dummy;
    INT32 oldIntLev = 0;
    BOOL wasEnabled;
    UINT ix;

    if (mmuPteGet (thisTbl, virtPageAddr, &dummy) == OK)
	return (OK);

    MMU_UNLOCK (wasEnabled, oldIntLev);
    pPageTable = (PTE *) memalign (mmuPageSize, mmuPageSize);
    MMU_LOCK (wasEnabled, oldIntLev);

    if (pPageTable == NULL)
	return (ERROR);

    /* invalidate every page in the new page block */

    for (ix = 0; ix < (PT_SIZE / sizeof(PTE)); ix++)
	{
	pPageTable[ix].field.present	= 0;
	pPageTable[ix].field.rw		= 0;
	pPageTable[ix].field.us		= 0;
	pPageTable[ix].field.pwt	= 0;
	pPageTable[ix].field.pcd	= 0;
	pPageTable[ix].field.access	= 0;
	pPageTable[ix].field.dirty	= 0;
	pPageTable[ix].field.pagesize	= 0;
	pPageTable[ix].field.global	= 0;
	pPageTable[ix].field.avail	= 0;
	pPageTable[ix].field.page	= -1;
	pPageTable[ix].field.page36	= 0;
	pPageTable[ix].field.reserved	= 0;
	}

    /* 
     * write protect the new physical page containing the PTE's
     * for this new page block 
     */

    mmuStateSet (&mmuGlobalTransTbl, pPageTable, 
	         MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE_NOT); 

    /* 
     * unlock the physical page containing the directory pointer table
     * and the directory table, so we can modify it 
     */

    pDirectoryTable = (PTE *) (thisTbl->pDirectoryTable->bits[0] & PDP_TO_ADDR);

    for (ix = 0; ix < nDirPages; ix++)
	{
        mmuStateSet (&mmuGlobalTransTbl, 
		     (void *) ((UINT32)pDirectoryTable + (mmuPageSize * ix)),
		     MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE);
	}

    /* modify the directory table entry to point to the new page table */

    pDte = &pDirectoryTable 
	   [((UINT) virtPageAddr & (DIR_PTR_BITS | DIR_BITS)) >> DIR_INDEX];    

    pDte->field.page = (UINT) pPageTable >> ADDR_TO_PAGE;
    pDte->field.present = 1;
    pDte->field.rw = 1;

    /* write protect the directory pointer table and the directory table */

    for (ix = 0; ix < nDirPages; ix++)
	{
        mmuStateSet (&mmuGlobalTransTbl, 
		     (void *) ((UINT32)pDirectoryTable + (mmuPageSize * ix)),
		     MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE_NOT);
	}

    /* defer the TLB flush to the mmuCurrentSet() at init time */

    if (!firstTime)
        {
        MMU_TLB_FLUSH ();
        }

    return (OK);
    }

/******************************************************************************
*
* mmuStateSet - set state of virtual memory page
*
* mmuStateSet is used to modify the state bits of the PTE for the given
* virtual page.  The following states are provided:
*
* MMU_STATE_VALID 	MMU_STATE_VALID_NOT	 valid/invalid
* MMU_STATE_WRITABLE 	MMU_STATE_WRITABLE_NOT	 writable/write_protected
* MMU_STATE_CACHEABLE 	MMU_STATE_CACHEABLE_NOT	 notcachable/cachable
* MMU_STATE_WBACK 	MMU_STATE_WBACK_NOT      write_back/write_through
* MMU_STATE_GLOBAL    	MMU_STATE_GLOBAL_NOT     global/not_global
*
* these may be or'ed together in the state parameter.  Additionally, masks
* are provided so that only specific states may be set:
*
* MMU_STATE_MASK_VALID 
* MMU_STATE_MASK_WRITABLE
* MMU_STATE_MASK_CACHEABLE
* MMU_STATE_MASK_WBACK
* MMU_STATE_MASK_GLOBAL
*
* These may be or'ed together in the stateMask parameter.  
*
* Accesses to a virtual page marked as invalid will result in a page fault.
*
* RETURNS: OK or ERROR if virtual page does not exist.
*/

LOCAL STATUS mmuStateSet 
    (
    MMU_TRANS_TBL *transTbl, 	/* translation table */
    void *pageAddr,		/* page whose state to modify */ 
    UINT stateMask,		/* mask of which state bits to modify */
    UINT state			/* new state bit values */
    )
    {
    PTE * pPte;
    INT32 oldIntLev = 0;
    BOOL wasEnabled;

    if (mmuPteGet (transTbl, pageAddr, &pPte) != OK)
	return (ERROR);

    /* modify the PTE with mmu turned off and interrupts locked out */

    MMU_WP_UNLOCK (wasEnabled, oldIntLev);
    pPte->bits[0] = (pPte->bits[0] & ~stateMask) | (state & stateMask);
    MMU_WP_LOCK (wasEnabled, oldIntLev);

    /* defer the TLB and Cache flush to the mmuCurrentSet() */

    if (!firstTime)
	{
        MMU_TLB_FLUSH ();
        cacheArchClearEntry (DATA_CACHE, pPte);
	}

    return (OK);
    }

/******************************************************************************
*
* mmuStateGet - get state of virtual memory page
*
* mmuStateGet is used to retrieve the state bits of the PTE for the given
* virtual page.  The following states are provided:
*
* MMU_STATE_VALID 	MMU_STATE_VALID_NOT	 valid/invalid
* MMU_STATE_WRITABLE 	MMU_STATE_WRITABLE_NOT	 writable/write_protected
* MMU_STATE_CACHEABLE 	MMU_STATE_CACHEABLE_NOT	 notcachable/cachable
* MMU_STATE_WBACK 	MMU_STATE_WBACK_NOT      write_back/write_through
* MMU_STATE_GLOBAL    	MMU_STATE_GLOBAL_NOT     global/not_global
*
* these are or'ed together in the returned state.  Additionally, masks
* are provided so that specific states may be extracted from the returned state:
*
* MMU_STATE_MASK_VALID 
* MMU_STATE_MASK_WRITABLE
* MMU_STATE_MASK_CACHEABLE
* MMU_STATE_MASK_WBACK
* MMU_STATE_MASK_GLOBAL
*
* RETURNS: OK or ERROR if virtual page does not exist.
*/

LOCAL STATUS mmuStateGet 
    (
    MMU_TRANS_TBL *transTbl, 	/* tranlation table */
    void *pageAddr, 		/* page whose state we're querying */
    UINT *state			/* place to return state value */
    )
    {
    PTE *pPte;

    if (mmuPteGet (transTbl, pageAddr, &pPte) != OK)
	return (ERROR);

    *state = pPte->bits[0]; 

    return (OK);
    }

/******************************************************************************
*
* mmuPageMap - map physical memory page to virtual memory page
*
* The physical page address is entered into the PTE corresponding to the
* given virtual page.  The state of a newly mapped page is undefined. 
*
* RETURNS: OK or ERROR if translation table creation failed. 
*/

LOCAL STATUS mmuPageMap 
    (
    MMU_TRANS_TBL *transTbl, 	/* translation table */
    void *virtualAddress, 	/* virtual address */
    void *physPage		/* physical address */
    )
    {
    PTE * pPte;
    INT32 oldIntLev = 0;
    BOOL wasEnabled;
    BOOL status	= mmuPteGet (transTbl, virtualAddress, &pPte);

    if ((mmuPageSize == PAGE_SIZE_4KB) && (status != OK))
	{
	/* build the translation table for the virtual address */

	if (mmuVirtualPageCreate (transTbl, virtualAddress) != OK)
	    return (ERROR);

	if (mmuPteGet (transTbl, virtualAddress, &pPte) != OK)
	    return (ERROR);
	}

    if (mmuPageSize != PAGE_SIZE_4KB)
        (UINT)physPage &= ADDR_TO_PAGEBASE; 

    MMU_WP_UNLOCK (wasEnabled, oldIntLev);
    pPte->field.page   = (UINT)physPage >> ADDR_TO_PAGE;
    MMU_WP_LOCK (wasEnabled, oldIntLev);

    MMU_TLB_FLUSH ();
    cacheArchClearEntry (DATA_CACHE, pPte);

    return (OK);
    }

/******************************************************************************
*
* mmuGlobalPageMap - map physical memory page to global virtual memory page
*
* mmuGlobalPageMap is used to map physical pages into global virtual memory
* that is shared by all virtual memory contexts.  The translation tables
* for this section of the virtual space are shared by all virtual memory
* contexts.
*
* RETURNS: OK or ERROR if no PTE for given virtual page.
*/

LOCAL STATUS mmuGlobalPageMap 
    (
    void *virtualAddress, 	/* virtual address */
    void *physPage		/* physical address */
    )
    {
    PTE * pPte;
    INT32 oldIntLev = 0;
    BOOL wasEnabled;
    BOOL status = mmuPteGet (&mmuGlobalTransTbl, virtualAddress, &pPte);
    int ix;

    if ((mmuPageSize == PAGE_SIZE_4KB) && (status != OK))
	{
	/* build the translation table for the virtual address */

	if (mmuVirtualPageCreate (&mmuGlobalTransTbl, virtualAddress) != OK)
	    return (ERROR);

	if (mmuPteGet (&mmuGlobalTransTbl, virtualAddress, &pPte) != OK)
	    return (ERROR);

        /* the globalPageBlock array is write protected */

        mmuStateSet (&mmuGlobalTransTbl, globalPageBlock,
		     MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE);

        ix = ((UINT) virtualAddress & (DIR_PTR_BITS | DIR_BITS)) >> DIR_INDEX;
        globalPageBlock [ix] = (UINT8) TRUE;	

        mmuStateSet (&mmuGlobalTransTbl, globalPageBlock,
    	             MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE_NOT);
	}

    if (mmuPageSize != PAGE_SIZE_4KB)
	(UINT)physPage &= ADDR_TO_PAGEBASE;

    MMU_WP_UNLOCK (wasEnabled, oldIntLev);
    pPte->field.page = (UINT)physPage >> ADDR_TO_PAGE;
    MMU_WP_LOCK (wasEnabled, oldIntLev);

    /* defer the TLB and Cache flush to the mmuCurrentSet() */

    if (!firstTime)
	{
        MMU_TLB_FLUSH ();
        cacheArchClearEntry (DATA_CACHE, pPte);
	}

    return (OK);
    }

/******************************************************************************
*
* mmuTranslate - translate a virtual address to a physical address
*
* Traverse the translation table and extract the physical address for the
* given virtual address from the PTE corresponding to the virtual address.
*
* RETURNS: OK or ERROR if no PTE for given virtual address.
*/

LOCAL STATUS mmuTranslate 
    (
    MMU_TRANS_TBL *transTbl, 		/* translation table */
    void *virtAddress, 			/* virtual address */
    void **physAddress			/* place to return result */
    )
    {
    PTE *pPte;

    if (mmuPteGet (transTbl, virtAddress, &pPte) != OK)
	{
	errno = S_mmuLib_NO_DESCRIPTOR; 
	return (ERROR);
	}

    if (pPte->field.present == 0)
	{
	errno = S_mmuLib_INVALID_DESCRIPTOR; 
	return (ERROR);
	}

    /* add offset into page */

    if (mmuPageSize == PAGE_SIZE_4KB)
        *physAddress = (void *)((UINT)(pPte->bits[0] & PTE_TO_ADDR_4KB) + 
			        ((UINT)virtAddress & OFFSET_BITS_4KB));
    else
        *physAddress = (void *)((UINT)(pPte->bits[0] & PTE_TO_ADDR_2MB) + 
			        ((UINT)virtAddress & OFFSET_BITS_2MB));

    return (OK);
    }

/******************************************************************************
*
* mmuCurrentSet - change active translation table
*
* mmuCurrent set is used to change the virtual memory context.
* Load the CRP (root pointer) register with the given translation table.
*
*/

LOCAL void mmuCurrentSet 
    (
    MMU_TRANS_TBL *transTbl	/* new active tranlation table */
    ) 
    {
    INT32 oldLev;

    if (firstTime)
	{
	mmuMemPagesWriteDisable (&mmuGlobalTransTbl);
	mmuMemPagesWriteDisable (transTbl);
	/* perform the deferred TLB and Cache flush */

        /* MMU_TLB_FLUSH (); */	/* done by following mmuPro32PdbrSet () */
        if (sysProcessor != X86CPU_386)
            WRS_ASM ("wbinvd");	/* flush the entire cache */

	firstTime = FALSE;
	}

    oldLev = intLock ();		/* LOCK INTERRUPTS */
    mmuCurrentTransTbl = transTbl;
    mmuPro36PdbrSet (transTbl);
    intUnlock (oldLev);			/* UNLOCK INTERRUPTS */
    }

/******************************************************************************
*
* mmuMemPagesWriteDisable - write disable memory holding a table's descriptors
*
* Memory containing translation table descriptors is marked as read only
* to protect the descriptors from being corrupted.  This routine write protects
* all the memory used to contain a given translation table's descriptors.
*
*/

LOCAL void mmuMemPagesWriteDisable
    (
    MMU_TRANS_TBL *transTbl
    )
    {
    PTE * pDirectoryTable;
    void * thisPage;
    INT32 ix;

    pDirectoryTable = (PTE *)(transTbl->pDirectoryTable->bits[0] & PDP_TO_ADDR);

    if (mmuPageSize == PAGE_SIZE_4KB)
        for (ix = 0; ix < ((PD_SIZE * 4) / sizeof(PTE)); ix++)
	    {
	    thisPage = (void *)(pDirectoryTable[ix].bits[0] & PTE_TO_ADDR_4KB);
	    if ((int)thisPage != (0xffffffff & PTE_TO_ADDR_4KB))
	        mmuStateSet (transTbl, thisPage, MMU_STATE_MASK_WRITABLE,
			     MMU_STATE_WRITABLE_NOT);
	    }

    for (ix = 0; ix < nDirPages; ix++)
	{
	thisPage = (void *)((UINT)pDirectoryTable + (mmuPageSize * ix));
        mmuStateSet (transTbl, thisPage, MMU_STATE_MASK_WRITABLE, 
		     MMU_STATE_WRITABLE_NOT);
	}

    mmuStateSet (transTbl, transTbl->pDirectoryTable, MMU_STATE_MASK_WRITABLE, 
		 MMU_STATE_WRITABLE_NOT);
    }

/******************************************************************************
*
* mmuPro36PageMap - map 36bit physical memory page to virtual memory page
*
* The 36bit physical page address is entered into the PTE corresponding to 
* the given virtual page.  The state of a newly mapped page is undefined. 
*
* RETURNS: OK or ERROR if translation table creation failed. 
*/

STATUS mmuPro36PageMap 
    (
    MMU_TRANS_TBL * transTbl, 	/* translation table */
    void * virtualAddress, 	/* 32bit virtual address */
    LL_INT physPage		/* 36bit physical address */
    )
    {
    PTE *    pPte;
    BOOL     wasEnabled;
    INT32    oldIntLev = 0;
    UINT32 * pPhysPage = (UINT32 *)&physPage;
    BOOL     status    = mmuPteGet (transTbl, virtualAddress, &pPte);

    if ((mmuPageSize == PAGE_SIZE_4KB) && (status != OK))
	{
	/* build the translation table for the virtual address */

	if (mmuVirtualPageCreate (transTbl, virtualAddress) != OK)
	    return (ERROR);

	if (mmuPteGet (transTbl, virtualAddress, &pPte) != OK)
	    return (ERROR);
	}

    if (mmuPageSize != PAGE_SIZE_4KB)
        pPhysPage[0] &= ADDR_TO_PAGEBASE; 

    MMU_WP_UNLOCK (wasEnabled, oldIntLev);
    pPte->field.page   = pPhysPage[0] >> ADDR_TO_PAGE;
    pPte->field.page36 = pPhysPage[1] & 0x0000000f;
    MMU_WP_LOCK (wasEnabled, oldIntLev);

    MMU_TLB_FLUSH ();
    cacheArchClearEntry (DATA_CACHE, pPte);

    return (OK);
    }

/******************************************************************************
*
* mmuPro36Translate - translate a virtual address to a 36bit physical address
*
* Traverse the translation table and extract the 36bit physical address for 
* the given virtual address from the PTE corresponding to the virtual address.
*
* RETURNS: OK or ERROR if no PTE for given virtual address.
*/

STATUS mmuPro36Translate 
    (
    MMU_TRANS_TBL * transTbl, 		/* translation table */
    void * virtAddress,			/* 32bit virtual address */
    LL_INT * physAddress		/* place to return 36bit result */
    )
    {
    PTE *    pPte;
    UINT32 * pPhysAddr = (UINT32 *) physAddress;

    if (mmuPteGet (transTbl, virtAddress, &pPte) != OK)
	{
	errno = S_mmuLib_NO_DESCRIPTOR; 
	return (ERROR);
	}

    if (pPte->field.present == 0)
	{
	errno = S_mmuLib_INVALID_DESCRIPTOR; 
	return (ERROR);
	}

    /* add offset into page */

    if (mmuPageSize == PAGE_SIZE_4KB)
        pPhysAddr[0] = (UINT32)(pPte->bits[0] & PTE_TO_ADDR_4KB) + 
		       ((UINT32)virtAddress & OFFSET_BITS_4KB);
    else
        pPhysAddr[0] = (UINT32)(pPte->bits[0] & PTE_TO_ADDR_2MB) + 
		       ((UINT32)virtAddress & OFFSET_BITS_2MB);
    pPhysAddr[1] = pPte->bits[1];

    return (OK);
    }


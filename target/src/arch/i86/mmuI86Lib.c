/* mmuI86Lib.c - MMU library for i86 */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01h,16may02,hdn  moved the GDT reloading to sysPhysMemTop() in sysLib.c
01g,09nov01,hdn  deferred TLB flush in mmuVirtualPageCreate() at init time
01f,27aug01,hdn  made MMU_TLB_FLUSH, updated MMU_LOCK/MMU_UNLOCK macros.
		 deferred TLB & Cache flush at initialization time.
		 renamed mmuLibInit to mmuI86LibInit.
		 doc: cleanup.
01e,09feb99,wsl  add comment to document ERRNO value
01d,13apr98,hdn  added support for PentiumPro.
01c,07jan95,hdn  re-initialized the GDT in mmuLibInit().
01b,01nov94,hdn  added a support for COPY_BACK cache mode for Pentium.
01a,26jul93,hdn  written based on mc68k's version.
*/

/*
DESCRIPTION:

mmuI86Lib.c provides the architecture dependent routines that directly control
the memory management unit.  It provides 10 routines that are called by the
higher level architecture independent routines in vmLib.c: 

 mmuI86LibInit() - initialize module
 mmuTransTblCreate() - create a new translation table
 mmuTransTblDelete() - delete a translation table.
 mmuI86Enable() - turn MMU on or off
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
level routines in mmuI86Lib.c (mmuPageMap(), mmuGlobalPageMap()) need not 
perform any error checking.

A global variable `mmuPageBlockSize' should be defined which is equal to 
the minimum virtual segment size.  mmuLib must provide a routine 
mmuGlobalInfoGet(), which returns a pointer to the globalPageBlock[] array.
This provides the user with enough information to be able to allocate virtual 
memory space that does not conflict with the global memory space.

This module supports the 80386/80486 MMU:

.CS
			    PDBR
			     |
			     |
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

where the top level consists of an array of pointers (Page Directory Entry)
held within a single 4k page.  These point to arrays of Page Table Entry 
arrays in the lower level.  Each of these lower level arrays is also held 
within a single 4k page, and describes a virtual space of 4 MB (each Page 
Table Entry is 4 bytes, so we get 1000 of these in each array, and each Page
Table Entry maps a 4KB page - thus 1000 * 4096 = 4MB.)  

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
So, you will notice that anywhere that page table descriptors (pte's)
are modified, we do so by locking out interrupts, momentarily disabling the 
MMU, accessing the memory with its physical address, enabling the MMU, and
then re-enabling interrupts (see mmuStateSet(), for example.)

The 80386 MMU does not have a write protection mechanism, since there is no
WP(Write Protect) bit in CR0 unlike 80486 MMU. The 80386 has no internal
cache, thus precluding control caching on a page-by-page basis. Also there 
is neither a PWT(Page Write Through) bit nor PCD(Page Cache Disable) bit 
in the Page Table Entry.
The 80486 MMU set a PWT(Page Write Through) bit all the time and uses PCD
(Page Cache Disable) bit to control caching on a page-by-page basis.
The Pentium MMU set a PWT(Page Write Through) bit if the cacheDataMode is
WRITE_THROUGH. If it is COPY_BACK, it doesn't set the bit.

*/




#include "vxWorks.h"
#include "string.h"
#include "intLib.h"
#include "stdlib.h"
#include "memLib.h"
#include "private/vmLibP.h"
#include "arch/i86/mmuI86Lib.h"
#include "mmuLib.h"
#include "errno.h"
#include "cacheLib.h"
#include "regs.h"


/* forward declarations */
 
LOCAL void mmuMemPagesWriteDisable (MMU_TRANS_TBL *transTbl);
LOCAL STATUS mmuPteGet (MMU_TRANS_TBL *pTransTbl, void *virtAddr, PTE **result);
LOCAL MMU_TRANS_TBL *mmuTransTblCreate ();
LOCAL STATUS mmuTransTblInit (MMU_TRANS_TBL *newTransTbl);
LOCAL STATUS mmuTransTblDelete (MMU_TRANS_TBL *transTbl);
LOCAL STATUS mmuVirtualPageCreate (MMU_TRANS_TBL *thisTbl, void *virtPageAddr);
LOCAL STATUS mmuStateSet (MMU_TRANS_TBL *transTbl, void *pageAddr, UINT stateMask, UINT state);
LOCAL STATUS mmuStateGet (MMU_TRANS_TBL *transTbl, void *pageAddr, UINT *state);
LOCAL STATUS mmuPageMap (MMU_TRANS_TBL *transTbl, void *virtualAddress, void *physPage);
LOCAL STATUS mmuGlobalPageMap (void *virtualAddress, void *physPage);
LOCAL STATUS mmuTranslate (MMU_TRANS_TBL *transTbl, void *virtAddress, void **physAddress);
LOCAL void mmuCurrentSet (MMU_TRANS_TBL *transTbl);

/* kludgey static data structure for parameters in __asm__ directives. */

LOCAL int mmuPageSize;
BOOL mmuI86Enabled = FALSE;

/* a translation table to hold the descriptors for the global transparent
 * translation of physical to virtual memory 
 */

LOCAL MMU_TRANS_TBL mmuGlobalTransTbl;

/* initially, the current trans table is a dummy table with MMU disabled */

LOCAL MMU_TRANS_TBL *mmuCurrentTransTbl = &mmuGlobalTransTbl;

/* array of booleans used to keep track of sections of virtual memory defined
 * as global.
 */

LOCAL BOOL *globalPageBlock;

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
     VM_STATE_CACHEABLE_NOT, MMU_STATE_CACHEABLE_NOT}

    };

LOCAL MMU_LIB_FUNCS mmuLibFuncsLocal =
    {
    mmuI86LibInit,
    mmuTransTblCreate,
    mmuTransTblDelete,
    mmuI86Enable,   
    mmuStateSet,
    mmuStateGet,
    mmuPageMap,
    mmuGlobalPageMap,
    mmuTranslate,
    mmuCurrentSet
    };

IMPORT STATE_TRANS_TUPLE *mmuStateTransArray;
IMPORT int mmuStateTransArraySize;
IMPORT MMU_LIB_FUNCS mmuLibFuncs;
IMPORT int mmuPageBlockSize;
IMPORT int sysProcessor;

LOCAL BOOL firstTime = TRUE;

/* MMU_UNLOCK and MMU_LOCK are used to access page table entries that are in
 * virtual memory that has been invalidated to protect it from being corrupted
 * MMU_UNLOCK and MMU_LOCK makes no function call, so no worry about stack
 * mismatch in virtual/physical address space, or the called out function's 
 * disapearance.  It is guaranteed that this code is in physical memory.
 */

#define MMU_ON() \
    do { int tempReg; WRS_ASM ("movl %%cr0,%0; orl $0x80010000,%0; \
    movl %0,%%cr0; jmp 0f; 0:" : "=r" (tempReg) : : "memory"); } while (0)

#define MMU_OFF() \
    do { int tempReg; WRS_ASM ("movl %%cr0,%0; andl $0x7ffeffff,%0; \
    movl %0,%%cr0; jmp 0f; 0:" : "=r" (tempReg) : : "memory"); } while (0)

#define MMU_UNLOCK(wasEnabled, oldLevel) \
    do { if (((wasEnabled) = mmuI86Enabled) == TRUE) \
	{INT_LOCK (oldLevel); MMU_OFF (); } \
    } while (0)

#define MMU_LOCK(wasEnabled, oldLevel) \
    do { if ((wasEnabled) == TRUE) \
        {MMU_ON (); INT_UNLOCK (oldLevel);} \
    } while (0)

/* inline version of mmuI86TLBFlush() */

#define	MMU_TLB_FLUSH() \
    do { int tempReg; WRS_ASM ("pushfl; cli; movl %%cr3,%0; \
    movl %0,%%cr3; jmp 0f; 0: popfl; " : "=r" (tempReg) : : "memory"); \
    } while (0)


/******************************************************************************
*
* mmuI86LibInit - initialize module
*
* Build a dummy translation table that will hold the page table entries
* for the global translation table.  The MMU remains disabled upon
* completion.
*
* RETURNS: OK if no error, ERROR otherwise
*
* ERRNO: S_mmuLib_INVALID_PAGE_SIZE
*/

STATUS mmuI86LibInit 
    (
    int pageSize	/* system pageSize (must be 4096 for i86) */
    )
    {

    PTE *pDirectoryTable;
    int i;

    /* initialize the data objects that are shared with vmLib.c */

    mmuStateTransArray = &mmuStateTransArrayLocal [0];

    mmuStateTransArraySize =
          sizeof (mmuStateTransArrayLocal) / sizeof (STATE_TRANS_TUPLE);

    mmuLibFuncs = mmuLibFuncsLocal;

    mmuPageBlockSize = PAGE_BLOCK_SIZE;

    /* we assume a 4096 byte page size */

    if (pageSize != PAGE_SIZE)
	{
	errno = S_mmuLib_INVALID_PAGE_SIZE;
	return (ERROR);
	}

    mmuI86Enabled = FALSE;

    mmuPageSize = pageSize;

    /* allocate the global page block array to keep track of which parts
     * of virtual memory are handled by the global translation tables.
     * Allocate on page boundry so we can write protect it.
     */

    globalPageBlock = (BOOL *) memalign (pageSize, pageSize);
    bzero ((char *) globalPageBlock, pageSize);

    /* build a dummy translation table which will hold the pte's for
     * global memory.  All real translation tables will point to this
     * one for controling the state of the global virtual memory  
     */

    /* allocate a page to hold the directory table */

    mmuGlobalTransTbl.pDirectoryTable = pDirectoryTable = 
	(PTE *) memalign (pageSize, pageSize);

    if (pDirectoryTable == NULL)
	return (ERROR);

    /* invalidate all the directory table entries */

    for (i = 0; i < (PAGE_SIZE / sizeof(PTE)) ;i++)
	{
	pDirectoryTable[i].field.present	= 0;
	pDirectoryTable[i].field.rw		= 0;
	pDirectoryTable[i].field.us		= 0;
	pDirectoryTable[i].field.pwt		= 0;
	pDirectoryTable[i].field.pcd		= 0;
	pDirectoryTable[i].field.access		= 0;
	pDirectoryTable[i].field.dirty		= 0;
	pDirectoryTable[i].field.zero		= 0;
	pDirectoryTable[i].field.avail		= 0;
	pDirectoryTable[i].field.page		= -1;
	}

    return (OK);
    }

/******************************************************************************
*
* mmuPteGet - get the pte for a given page
*
* mmuPteGet traverses a translation table and returns the (physical) address of
* the pte for the given virtual address.
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

    PTE *pDte;
    PTE *pPageTable;

    pDte = &pTransTbl->pDirectoryTable [(UINT) virtAddr >> DIRECTORY_INDEX];
    pPageTable = (PTE *) (pDte->bits & PTE_TO_ADDR); 

    if ((UINT) pPageTable == 0xfffff000)
	return (ERROR);

    *result = &pPageTable[((UINT) virtAddr & TABLE_BITS) >> TABLE_INDEX];

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

    MMU_TRANS_TBL *newTransTbl;

    newTransTbl = (MMU_TRANS_TBL *) malloc (sizeof (MMU_TRANS_TBL));

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
* Initialize a new translation table.  The directory table is copyed from the
* global translation mmuGlobalTransTbl, so that we
* will share the global virtual memory with all
* other translation tables.
* 
* RETURNS: OK or ERROR if unable to allocate memory for directory table.
*/

LOCAL STATUS mmuTransTblInit 
    (
    MMU_TRANS_TBL *newTransTbl		/* translation table to be inited */
    )
    {

    FAST PTE *pDirectoryTable;

    /* allocate a page to hold the directory table */

    newTransTbl->pDirectoryTable = pDirectoryTable = 
	(PTE *) memalign (mmuPageSize, mmuPageSize);

    if (pDirectoryTable == NULL)
	return (ERROR);

    /* copy the directory table from mmuGlobalTransTbl,
     * so we get the global virtual memory 
     */

    bcopy ((char *) mmuGlobalTransTbl.pDirectoryTable, 
	   (char *) pDirectoryTable, mmuPageSize);

    /* write protect virtual memory pointing to the the directory table in 
     * the global translation table to insure that it can't be corrupted 
     */

    mmuStateSet (&mmuGlobalTransTbl, (void *) pDirectoryTable, 
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
    FAST int i;
    FAST PTE *pDte = transTbl->pDirectoryTable;
    FAST PTE *pPageTable;

    /* write enable the physical page containing the directory table */

    mmuStateSet (&mmuGlobalTransTbl, transTbl->pDirectoryTable, 
		 MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE);

    /* deallocate only non-global page blocks */

    for (i = 0; i < (PAGE_SIZE / sizeof(PTE)); i++, pDte++)
	if ((pDte->field.present == 1) && !globalPageBlock[i]) 
	    {
	    pPageTable = (PTE *) (pDte->bits & PTE_TO_ADDR);
	    mmuStateSet (&mmuGlobalTransTbl, pPageTable,
			 MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE);
	    free (pPageTable);
	    }

    /* free the page holding the directory table */

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
* pte for the given virtual page.  If there isn't, create one.
*
* RETURNS OK or ERROR if couldn't allocate space for a page table.
*/

LOCAL STATUS mmuVirtualPageCreate 
    (
    MMU_TRANS_TBL *thisTbl, 		/* translation table */
    void *virtPageAddr			/* virtual addr to create */
    )
    {
    PTE *pDte;
    FAST PTE *pPageTable;
    FAST UINT i;
    PTE *dummy;

    if (mmuPteGet (thisTbl, virtPageAddr, &dummy) == OK)
	return (OK);

    pPageTable = (PTE *) memalign (mmuPageSize, mmuPageSize);

    if (pPageTable == NULL)
	return (ERROR);

    /* invalidate every page in the new page block */

    for (i = 0; i < (PAGE_SIZE / sizeof(PTE)); i++)
	{
	pPageTable[i].field.present	= 0;
	pPageTable[i].field.rw		= 0;
	pPageTable[i].field.us		= 0;
	pPageTable[i].field.pwt		= 0;
	pPageTable[i].field.pcd		= 0;
	pPageTable[i].field.access	= 0;
	pPageTable[i].field.dirty	= 0;
	pPageTable[i].field.zero	= 0;
	pPageTable[i].field.avail	= 0;
	pPageTable[i].field.page	= -1;
	}

    /* write protect the new physical page containing the pte's
       for this new page block */

    mmuStateSet (&mmuGlobalTransTbl, pPageTable, 
   		 MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE_NOT); 

    /* unlock the physical page containing the directory table,
       so we can modify it */

    mmuStateSet (&mmuGlobalTransTbl, thisTbl->pDirectoryTable, 
		 MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE);

    pDte = &thisTbl->pDirectoryTable [(UINT) virtPageAddr >> DIRECTORY_INDEX];    

    /* modify the directory table entry to point to the new page table */

    pDte->field.page = (UINT) pPageTable >> ADDR_TO_PAGE;
    pDte->field.present = 1;
    pDte->field.rw = 1;

    /* write protect the directory table */

    mmuStateSet (&mmuGlobalTransTbl, thisTbl->pDirectoryTable, 
		     MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE_NOT);

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
* mmuStateSet is used to modify the state bits of the pte for the given
* virtual page.  The following states are provided:
*
* MMU_STATE_VALID 	MMU_STATE_VALID_NOT	 vailid/invalid
* MMU_STATE_WRITABLE 	MMU_STATE_WRITABLE_NOT	 writable/writeprotected
* MMU_STATE_CACHEABLE 	MMU_STATE_CACHEABLE_NOT	 notcachable/cachable
*
* these may be or'ed together in the state parameter.  Additionally, masks
* are provided so that only specific states may be set:
*
* MMU_STATE_MASK_VALID 
* MMU_STATE_MASK_WRITABLE
* MMU_STATE_MASK_CACHEABLE
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
    PTE *pPte;
    int oldIntLev = 0;
    BOOL wasEnabled;

    if (mmuPteGet (transTbl, pageAddr, &pPte) != OK)
	return (ERROR);

    /* modify the pte with MMU turned off and interrupts locked out */

    MMU_UNLOCK (wasEnabled, oldIntLev);
    pPte->bits = (pPte->bits & ~stateMask) | (state & stateMask);
    if ((sysProcessor != X86CPU_386) && (cacheDataMode & CACHE_COPYBACK))
	if ((state & MMU_STATE_MASK_CACHEABLE) == MMU_STATE_CACHEABLE)
            pPte->bits &= ~MMU_STATE_CACHEABLE_WT;
    MMU_LOCK (wasEnabled, oldIntLev);

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
* mmuStateGet is used to retrieve the state bits of the pte for the given
* virtual page.  The following states are provided:
*
* MMU_STATE_VALID 	MMU_STATE_VALID_NOT	 vailid/invalid
* MMU_STATE_WRITABLE 	MMU_STATE_WRITABLE_NOT	 writable/writeprotected
* MMU_STATE_CACHEABLE 	MMU_STATE_CACHEABLE_NOT	 notcachable/cachable
*
* these are or'ed together in the returned state.  Additionally, masks
* are provided so that specific states may be extracted from the returned state:
*
* MMU_STATE_MASK_VALID 
* MMU_STATE_MASK_WRITABLE
* MMU_STATE_MASK_CACHEABLE
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

    *state = pPte->bits; 
    if ((sysProcessor != X86CPU_386) && (cacheDataMode & CACHE_COPYBACK))
	if ((*state & MMU_STATE_MASK_CACHEABLE) != MMU_STATE_CACHEABLE_NOT)
            *state |= MMU_STATE_CACHEABLE_WT;

    return (OK);
    }

/******************************************************************************
*
* mmuPageMap - map physical memory page to virtual memory page
*
* The physical page address is entered into the pte corresponding to the
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
    PTE *pPte;
    int oldIntLev = 0;
    FAST UINT page = (UINT)physPage >> ADDR_TO_PAGE; 
    BOOL wasEnabled;

    if (mmuPteGet (transTbl, virtualAddress, &pPte) != OK)
	{
	/* build the translation table for the virtual address */

	if (mmuVirtualPageCreate (transTbl, virtualAddress) != OK)
	    return (ERROR);

	if (mmuPteGet (transTbl, virtualAddress, &pPte) != OK)
	    return (ERROR);
	}

    MMU_UNLOCK (wasEnabled, oldIntLev);
    pPte->field.page = page; 
    MMU_LOCK (wasEnabled, oldIntLev);

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
* RETURNS: OK or ERROR if no pte for given virtual page.
*/

LOCAL STATUS mmuGlobalPageMap 
    (
    void *virtualAddress, 	/* virtual address */
    void *physPage		/* physical address */
    )
    {
    PTE *pPte;
    int oldIntLev = 0;
    FAST UINT page = (UINT)physPage >> ADDR_TO_PAGE; 
    BOOL wasEnabled;

    if (mmuPteGet (&mmuGlobalTransTbl, virtualAddress, &pPte) != OK)
	{
	/* build the translation table for the virtual address */

	if (mmuVirtualPageCreate (&mmuGlobalTransTbl, virtualAddress) != OK)
	    return (ERROR);

	if (mmuPteGet (&mmuGlobalTransTbl, virtualAddress, &pPte) != OK)
	    return (ERROR);


	/* the globalPageBlock array is write protected */

	mmuStateSet (&mmuGlobalTransTbl, globalPageBlock, 
		     MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE);
	globalPageBlock [(unsigned) virtualAddress >> DIRECTORY_INDEX] = TRUE;	
	mmuStateSet (&mmuGlobalTransTbl, globalPageBlock, 
		     MMU_STATE_MASK_WRITABLE, MMU_STATE_WRITABLE_NOT);
	}

    MMU_UNLOCK (wasEnabled, oldIntLev);
    pPte->field.page = page; 
    MMU_LOCK (wasEnabled, oldIntLev);

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
* given virtual address from the pte corresponding to the virtual address.
*
* RETURNS: OK or ERROR if no pte for given virtual address.
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

    *physAddress = (void *)(pPte->bits & PTE_TO_ADDR); 

    /* add offset into page */

    *physAddress = (void *)((UINT)*physAddress + 
			   ((UINT)virtAddress & OFFSET_BITS));

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
    FAST int oldLev;

    if (firstTime)
	{
	mmuMemPagesWriteDisable (&mmuGlobalTransTbl);
	mmuMemPagesWriteDisable (transTbl);
	
	/* perform the deferred TLB and Cache flush */

        /* MMU_TLB_FLUSH (); */	/* done by following mmuI86PdbrSet () */
        if (sysProcessor != X86CPU_386)
            WRS_ASM ("wbinvd");	/* flush the entire cache */

	firstTime = FALSE;
	}

    oldLev = intLock ();
    mmuCurrentTransTbl = transTbl;
    mmuI86PdbrSet (transTbl);
    intUnlock (oldLev);
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
    void *thisPage;
    int i;

    for (i = 0; i < (PAGE_SIZE / sizeof(PTE)) ;i++)
	{
	thisPage = (void *) (transTbl->pDirectoryTable[i].bits & PTE_TO_ADDR);
	if ((int)thisPage != 0xfffff000)
	    mmuStateSet (transTbl, thisPage, MMU_STATE_MASK_WRITABLE,
			 MMU_STATE_WRITABLE_NOT);
	}

    mmuStateSet (transTbl, transTbl->pDirectoryTable, MMU_STATE_MASK_WRITABLE, 
		 MMU_STATE_WRITABLE_NOT);
    }


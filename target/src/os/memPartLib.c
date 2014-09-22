/* memPartLib.c - core memory partition manager */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02c,22may02,zl   use the ALIGNED() macro for alignment test (SPR#74247).
02b,23apr02,gls  added check for overflow in memPartAlignedAlloc (SPR #27741)
02a,04oct01,tam  fixed doc. of arch alignment
01z,03mar00,zl   merged SH support into T2
01y,15mar99,c_c  Doc: Fixed alignment for MIPS (SPR #8883).
01x,21feb99,jdi  doc: listed errnos.
01w,17feb98,dvs  added instrumentation points (pr)
01v,17dec97,yp   corrected documentation for memPartIsValid (SPR #8537)
01u,07oct96,dgp  doc: change description of memPartXxx() (no partition
		 deletion available)
01t,19mar95,dvs  removed tron references.
01s,19sep94,rhp  added stdlib.h to INCLUDE FILES list in doc (SPR#2285)
01r,13sep93,jmm  fixed memPartAddToPool to check correct sizes of memory
01q,24jun93,jmm  fixed memPartAddToPool to reject pool sizes that are too small
01p,24jun93,jmm  fixed memPartAddToPool to round poolSize down to alignment
                 boundry (spr 2185)
01o,10feb93,jdi  deleted mention of "error" handling in doc for free();
		 fixed spelling of stdlib.h.
01n,05feb93,jdi  tweaked wording for 01m.
01m,05feb93,smb  corrected return documentation for free and malloc
01l,23jan93,jdi  documentation cleanup for 5.1.
01k,13nov92,dnw  added include of smObjLib.h
01j,20oct92,pme  added reference to shared memory manager documentation.
01i,02oct92,jdi  documentation cleanup.
01h,08sep92,jmm  changed memPartFree() to mark coalesced blocks as not free
01g,23aug92,jcf  changed bzero to bfill.  removed taskOptionsGet.
01f,29jul92,pme  added NULL function pointer check for smObj routines.
01e,28jul92,jcf  changed default memory partition options.
		 moved MEM_BLOCK_ERROR_SUSPEND_FLAG handling here.
01d,19jul92,pme  added shared memory partition support.
01c,19jul92,smb  added some ANSI documentation.
01b,18jul92,smb  Changed errno.h to errnoLib.h.
01a,01jul92,jcf  extracted from v4n memLib.c
*/

/*
DESCRIPTION
This library provides core facilities for managing the allocation of
blocks of memory from ranges of memory called memory partitions.  The
library was designed to provide a compact implementation; full-featured
functionality is available with memLib, which provides enhanced memory
management features built as an extension of memPartLib.  (For more
information about enhanced memory partition management options, see the
manual entry for memLib.)  This library consists of two sets of routines.
The first set, memPart...(), comprises a general facility for the creation
and management of memory partitions, and for the allocation and deallocation
of blocks from those partitions.  The second set provides a traditional
ANSI-compatible malloc()/free() interface to the system memory partition.

The system memory partition is created when the kernel is initialized by
kernelInit(), which is called by the root task, usrRoot(), in
usrConfig.c.  The ID of the system memory partition is stored in the
global variable `memSysPartId'; its declaration is included in memLib.h.

The allocation of memory, using malloc() in the typical case and
memPartAlloc() for a specific memory partition, is done with a first-fit
algorithm.  Adjacent blocks of memory are coalesced when they are freed
with memPartFree() and free().  There is also a routine provided for allocating
memory aligned to a specified boundary from a specific memory partition,
memPartAlignedAlloc().

CAVEATS
Architectures have various alignment constraints.  To provide optimal
performance, malloc() returns a pointer to a buffer having the appropriate
alignment for the architecture in use.  The portion of the allocated
buffer reserved for system bookkeeping, known as the overhead, may vary
depending on the architecture.

.ne 12
.TS
center,tab(|);
lf3 cf3 cf3
a n n .
Architecture | Boundary | Overhead
_
 ARM       |   4  |   8
 COLDFIRE  |   4  |   8
 I86       |   4  |   8
 M68K      |   4  |   8
 MCORE     |   8  |   8
 MIPS      |  16  |  16
 PPC *     | 8/16 | 8/16
 SH        |   4  |   8
 SIMNT     |   8  |   8
 SIMSOLARIS|   8  |   8
 SPARC     |   8  |   8
.TE

* On PowerPC, the boundary and overhead values are 16 bytes for system based
on the PPC604 CPU type (including ALTIVEC). For all other PowerPC CPU types
(PPC403, PPC405, PPC440, PPC860, PPC603, etc...), the boundary and overhead 
are 8 bytes.

INCLUDE FILES: memLib.h, stdlib.h

SEE ALSO: memLib, smMemLib

INTERNAL
This package is initialized by kernelInit() which calls memInit() with a
pointer to a block of memory from which memory will be allocated.

Blocks allocated by malloc() are actually larger than the size requested
by the user.  Each block is prefaced by a header which contains the size
and status of that block and a pointer to the previous block.  The pointer
returned to the user points just past this header.  Likewise when a block
is freed, the header is found just in front of the block pointer passed by
the user.

The header data is arranged so that the pointer to the previous block
comes first and is therefore adjacent to that previous block.  Thus each
block has 4 bytes of redundant information on either side of it (the 4
bytes of size and status before it in its own header, and the 4-byte
pointer after it in the next header).  This redundant information is
optionally used in free() and realloc() to make a consistency check on
blocks specified by the user.  This mechanism helps to detect two common
errors: (1) bad block pointers passed to free() or realloc() are usually
detected, and (2) trashing up to 4 bytes on either side of the block will
only affect that block and will also be detected by free() or realloc().

There is a minimum block size which malloc() allocates; this is to insure
that there will be enough room for the free list links that must be used
when the block is freed if it cannot be coalesced with an adjacent block.
The malloc() and realloc() routines always allocate memory aligned to the 
boundary defined in the global variable memDefaultAlignment, which is 
initialized to the alignment required for the specific architecture.

The memory partition semaphore is a structure in the partition descriptor
rather than a pointer to a dynamically created semaphore structure.
This is because of the chicken-and-the-egg problem of memLib using semaphores
and semCreate calling malloc.  Instead the structure is simply declared
directly in the partition structure and we call semInit() instead of
semCreate().
*/

#include "vxWorks.h"
#include "semLib.h"
#include "dllLib.h"
#include "logLib.h"
#include "taskLib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "errnoLib.h"
#include "smObjLib.h"
#include "private/memPartLibP.h"
#include "private/eventP.h"


/* forward static functions */

LOCAL void	 memPartSemInit (PART_ID partId);
LOCAL STATUS	 memPartDestroy (PART_ID partId);
LOCAL BLOCK_HDR *memAlignedBlockSplit (PART_ID partId, BLOCK_HDR *pHdr, 
				       unsigned nWords, unsigned minWords,
				       unsigned alignment);
/* local variables */

LOCAL PARTITION memSysPartition;	/* system partition used by malloc */
LOCAL OBJ_CLASS memPartClass;		/* memory partition object class */
LOCAL BOOL	memPartLibInstalled;	/* TRUE if library has been installed */

#ifdef WV_INSTRUMENTATION
LOCAL OBJ_CLASS memPartInstClass;	/* mem part instrumented object class */
#endif

/* global variables */

FUNCPTR  smMemPartAddToPoolRtn	= NULL;
FUNCPTR  smMemPartFreeRtn	= NULL;
FUNCPTR  smMemPartAllocRtn	= NULL;
CLASS_ID memPartClassId 	= &memPartClass;	/* partition class id */

#ifdef WV_INSTRUMENTATION
CLASS_ID memPartInstClassId	= &memPartInstClass;    /* part inst class id */
#endif

PART_ID  memSysPartId		= &memSysPartition;	/* sys partition id */
UINT	 memDefaultAlignment	= _ALLOC_ALIGN_SIZE;	/* default alignment */
FUNCPTR  memPartBlockErrorRtn	= NULL;			/* block error method */
FUNCPTR  memPartAllocErrorRtn	= NULL;			/* alloc error method */
FUNCPTR  memPartSemInitRtn	= (FUNCPTR) memPartSemInit;
unsigned memPartOptionsDefault	= MEM_BLOCK_ERROR_SUSPEND_FLAG |
				  MEM_BLOCK_CHECK;


/*******************************************************************************
*
* memPartLibInit - initialize the system memory partition
*
* This routine initializes the system partition free list with the
* specified memory block.  It must be called exactly once before invoking any
* other routine in memLib.  It is called by kernelInit() in usrRoot()
* in usrConfig.c.
*
* RETURNS: OK or ERROR.
* NOMANUAL
*/

STATUS memPartLibInit 
    (
    char *pPool,        /* pointer to memory block */
    unsigned poolSize   /* block size in bytes */
    )
    {
    if ((!memPartLibInstalled) &&
	(classInit (memPartClassId, sizeof (PARTITION),
		    OFFSET (PARTITION, objCore), (FUNCPTR) memPartCreate,
		    (FUNCPTR) memPartInit, (FUNCPTR) memPartDestroy) == OK))
	{
#ifdef WV_INSTRUMENTATION
	/* Instrumented class for windview */
	memPartClassId->initRtn = (FUNCPTR) memPartInstClassId;
	classInstrument (memPartClassId, memPartInstClassId); 
#endif
	memPartInit (&memSysPartition, pPool, poolSize);
	memPartLibInstalled = TRUE;
	}

    return ((memPartLibInstalled) ? OK : ERROR);
    }
/*******************************************************************************
*
* memPartCreate - create a memory partition
*
* This routine creates a new memory partition containing a specified
* memory pool.  It returns a partition ID, which can then be passed to
* other routines to manage the partition (i.e., to allocate and free
* memory blocks in the partition).  Partitions can be created to manage
* any number of separate memory pools.
*
* NOTE
* The descriptor for the new partition is allocated out of the system memory
* partition (i.e., with malloc()).
*
* RETURNS:
* The partition ID, or NULL if there is insufficient memory in the system
* memory partition for a new partition descriptor.
*
* SEE ALSO: smMemLib
*/

PART_ID memPartCreate 
    (
    char *pPool,        /* pointer to memory area */
    unsigned poolSize   /* size in bytes */
    )
    {
    FAST PART_ID pPart;

    /* allocate a partition structure from the system memory partition */

    pPart = (PART_ID) objAlloc (memPartClassId);

    if (pPart != NULL)
	memPartInit (pPart, pPool, poolSize);

#ifdef WV_INSTRUMENTATION
    EVT_OBJ_2 (OBJ, pPart, memPartClassId, EVENT_MEMPARTCREATE, pPart, poolSize);
#endif

    return (pPart);
    }
/*******************************************************************************
*
* memPartInit - initialize a memory partition
*
* This routine initializes a partition free list, seeding it with the
* memory block passed as an argument.  It must be called exactly once
* for each memory partition created.
*
* SEE ALSO: memPartCreate()
*
* NOMANUAL
*/

void memPartInit 
    (
    FAST PART_ID partId,        /* partition to initialize */
    char *pPool,                /* pointer to memory block */
    unsigned poolSize           /* block size in bytes */
    )
    {
    /* initialize partition descriptor */

    bfill ((char *) partId, sizeof (*partId), 0);

    partId->options	  = memPartOptionsDefault;
    partId->minBlockWords = sizeof (FREE_BLOCK) >> 1;

    /* initialize partition semaphore with a virtual function so semaphore
     * type is selectable.  By default memPartLibInit() will utilize binary
     * semaphores while memInit() will utilize mutual exclusion semaphores
     * with the options stored in _mutexOptionsMemLib.
     */

    (* memPartSemInitRtn) (partId);

    dllInit (&partId->freeList);			/* init. free list */

#ifdef WV_INSTRUMENTATION
    if (wvObjIsEnabled)
    {
    /* windview - connect object class event logging routine */
    objCoreInit (&partId->objCore, memPartInstClassId); 
    }
    else
#endif
    objCoreInit (&partId->objCore, memPartClassId);	/* initialize core */

    (void) memPartAddToPool (partId, pPool, poolSize);
    }
/*******************************************************************************
*
* memPartDestroy - destroy a partition and optionally free associated memory
*
* This routine is not currently supported.  Partitions may not be destroyed.
*
* ARGSUSED
*/

LOCAL STATUS memPartDestroy 
    (
    PART_ID partId      /* partition to initialize */
    )
    {
    errno = S_memLib_NO_PARTITION_DESTROY;

    return (ERROR);
    }
/*******************************************************************************
*
* memPartAddToPool - add memory to a memory partition
*
* This routine adds memory to a specified memory partition already created
* with memPartCreate().  The memory added need not be contiguous with
* memory previously assigned to the partition.
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_smObjLib_NOT_INITIALIZED, S_memLib_INVALID_NBYTES
*
* SEE ALSO: smMemLib, memPartCreate()
*/

STATUS memPartAddToPool 
    (
    FAST PART_ID partId,                /* partition to initialize */
    FAST char *pPool,                   /* pointer to memory block */
    FAST unsigned poolSize              /* block size in bytes */
    )
    {
    FAST BLOCK_HDR *pHdrStart;
    FAST BLOCK_HDR *pHdrMid;
    FAST BLOCK_HDR *pHdrEnd;
    char *          tmp;
    int             reducePool;		/* reduce size of pool by this amount*/

    if (ID_IS_SHARED (partId))  /* partition is shared? */
        {
	if (smMemPartAddToPoolRtn == NULL)
	    {
	    errno = S_smObjLib_NOT_INITIALIZED;
	    return (ERROR);
	    }

        return ((*smMemPartAddToPoolRtn) (SM_OBJ_ID_TO_ADRS (partId), 
					  pPool, poolSize));
	}

    /* partition is local */

    if (OBJ_VERIFY (partId, memPartClassId) != OK)
	return (ERROR);

    /* insure that the pool starts on an even byte boundary */

    tmp       = (char *) MEM_ROUND_UP (pPool);	/* get actual start */
    reducePool = tmp - pPool;
    if (poolSize >= reducePool)		 	/* adjust length */
        poolSize -= reducePool;
    else
        poolSize = 0;
    pPool     = tmp;

    /*
     * Make sure poolSize is an even multiple of the memory alignment size
     * and is large enough to include at least one valid free block and
     * three header blocks.
     */

    poolSize = MEM_ROUND_DOWN (poolSize);

    if (poolSize < ((sizeof (BLOCK_HDR) * 3) + (partId->minBlockWords * 2)))
	{
	errno = S_memLib_INVALID_NBYTES;
        return (ERROR);
	}

    /* initialize three blocks -
     * one at each end of the pool for end cases and real initial free block */

    pHdrStart		= (BLOCK_HDR *) pPool;
    pHdrStart->pPrevHdr = NULL;
    pHdrStart->free     = FALSE;
    pHdrStart->nWords   = sizeof (BLOCK_HDR) >> 1;

    pHdrMid		= NEXT_HDR (pHdrStart);
    pHdrMid->pPrevHdr   = pHdrStart;
    pHdrMid->free       = TRUE;
    pHdrMid->nWords     = (poolSize - 2 * sizeof (BLOCK_HDR)) >> 1;

    pHdrEnd		= NEXT_HDR (pHdrMid);
    pHdrEnd->pPrevHdr   = pHdrMid;
    pHdrEnd->free       = FALSE;
    pHdrEnd->nWords     = sizeof (BLOCK_HDR) >> 1;

    semTake (&partId->sem, WAIT_FOREVER);

#ifdef WV_INSTRUMENTATION
    EVT_OBJ_2 (OBJ, partId, memPartClassId, EVENT_MEMADDTOPOOL, partId, poolSize);
#endif

    dllInsert (&partId->freeList, (DL_NODE *) NULL, HDR_TO_NODE (pHdrMid));
    partId->totalWords += (poolSize >> 1);

    semGive (&partId->sem);

    return (OK);
    }

/*******************************************************************************
*
* memPartAlignedAlloc - allocate aligned memory from a partition
*
* This routine allocates a buffer of size <nBytes> from a specified
* partition.  Additionally, it insures that the allocated buffer begins on a
* memory address evenly divisible by <alignment>.  The <alignment> parameter
* must be a power of 2.
*
* RETURNS:
* A pointer to the newly allocated block, or NULL if the buffer could not be
* allocated.
*/

void *memPartAlignedAlloc 
    (
    FAST PART_ID	partId,		/* memory partition to allocate from */
    unsigned		nBytes,		/* number of bytes to allocate */
    unsigned		alignment	/* boundary to align to */
    )
    {
    FAST unsigned	nWords;
    FAST unsigned	nWordsExtra;
    FAST DL_NODE *	pNode;
    FAST BLOCK_HDR *	pHdr;
    BLOCK_HDR *		pNewHdr;
    BLOCK_HDR 		origpHdr;

    if (OBJ_VERIFY (partId, memPartClassId) != OK)
	return (NULL);

    /* get actual size to allocate; add overhead, check for minimum */

    nWords = (MEM_ROUND_UP (nBytes) + sizeof (BLOCK_HDR)) >> 1;

    /* check if we have an overflow. if so, set errno and return NULL */

    if ((nWords << 1) < nBytes)
	{
	if (memPartAllocErrorRtn != NULL)
	    (* memPartAllocErrorRtn) (partId, nBytes);

	errnoSet (S_memLib_NOT_ENOUGH_MEMORY);

	if (partId->options & MEM_ALLOC_ERROR_SUSPEND_FLAG)
	    {
	    if ((taskIdCurrent->options & VX_UNBREAKABLE) == 0)
		taskSuspend (0);			/* suspend ourselves */
	    }

	return (NULL);
	}

    if (nWords < partId->minBlockWords)
	nWords = partId->minBlockWords;

    /* get exclusive access to the partition */

    semTake (&partId->sem, WAIT_FOREVER);

    /* first fit */

    pNode = DLL_FIRST (&partId->freeList);

    /* We need a free block with extra room to do the alignment.  
     * Worst case we'll need alignment extra bytes.
     */

    nWordsExtra = nWords + alignment / 2;

    FOREVER
	{
	while (pNode != NULL)
	    {
	    /* fits if:
	     *	- blocksize > requested size + extra room for alignment or,
	     *	- block is already aligned and exactly the right size
	     */
	    if ((NODE_TO_HDR (pNode)->nWords > nWordsExtra) ||
		((NODE_TO_HDR (pNode)->nWords == nWords) &&
		 (ALIGNED (HDR_TO_BLOCK(NODE_TO_HDR(pNode)), alignment))))
		  break;
	    pNode = DLL_NEXT (pNode);
	    }
	
	if (pNode == NULL)
	    {
	    semGive (&partId->sem);

	    if (memPartAllocErrorRtn != NULL)
		(* memPartAllocErrorRtn) (partId, nBytes);

	    errnoSet (S_memLib_NOT_ENOUGH_MEMORY);

	    if (partId->options & MEM_ALLOC_ERROR_SUSPEND_FLAG)
		{
		if ((taskIdCurrent->options & VX_UNBREAKABLE) == 0)
		    taskSuspend (0);			/* suspend ourselves */
		}

	    return (NULL);
	    }

	pHdr = NODE_TO_HDR (pNode);
	origpHdr = *pHdr;


	/* now we split off from this block, the amount required by the user;
	 * note that the piece we are giving the user is at the end of the
	 * block so that the remaining piece is still the piece that is
	 * linked in the free list;  if memAlignedBlockSplit returned NULL,
	 * it couldn't split the block because the split would leave the
	 * first block too small to be hung on the free list, so we continue
	 * trying blocks.
	 */

	pNewHdr = 
	    memAlignedBlockSplit (partId, pHdr, nWords, partId->minBlockWords,
				  alignment);

	if (pNewHdr != NULL)
	    {
	    pHdr = pNewHdr;			/* give split off block */
	    break;
	    }
	
	pNode = DLL_NEXT (pNode);
	}

    /* mark the block we're giving as not free */

    pHdr->free = FALSE;

#ifdef WV_INSTRUMENTATION
    EVT_OBJ_4 (OBJ, partId, memPartClassId, EVENT_MEMALLOC, partId, HDR_TO_BLOCK (pHdr), 2 * (pHdr->nWords), nBytes);
#endif

    /* update allocation statistics */

    partId->curBlocksAllocated++;
    partId->cumBlocksAllocated++;
    partId->curWordsAllocated += pHdr->nWords;
    partId->cumWordsAllocated += pHdr->nWords;

    semGive (&partId->sem);

    return ((void *) HDR_TO_BLOCK (pHdr));
    }

/*******************************************************************************
*
* memPartAlloc - allocate a block of memory from a partition
*
* This routine allocates a block of memory from a specified partition. 
* The size of the block will be equal to or greater than <nBytes>.  
* The partition must already be created with memPartCreate().
*
* RETURNS:
* A pointer to a block, or NULL if the call fails.
*
* ERRNO: S_smObjLib_NOT_INITIALIZED
*
* SEE ALSO: smMemLib, memPartCreate()
*/

void *memPartAlloc 
    (
    FAST PART_ID partId,        /* memory partition to allocate from */
    unsigned nBytes             /* number of bytes to allocate */
    )
    {
    if (ID_IS_SHARED (partId))  /* partition is shared? */
        {
	if (smMemPartAllocRtn == NULL)
	    {
	    errno = S_smObjLib_NOT_INITIALIZED;
	    return (NULL);
	    }

        return ((void *) (*smMemPartAllocRtn) (SM_OBJ_ID_TO_ADRS (partId), 
					       nBytes));
	}

    /* partition is local */

    return (memPartAlignedAlloc (partId, nBytes, memDefaultAlignment));
    }

/*******************************************************************************
*
* memPartFree - free a block of memory in a partition
*
* This routine returns to a partition's free memory list a block of 
* memory previously allocated with memPartAlloc().
*
* RETURNS: OK, or ERROR if the block is invalid.
*
* ERRNO: S_smObjLib_NOT_INITIALIZED
*
* SEE ALSO: smMemLib, memPartAlloc()
*/

STATUS memPartFree 
    (
    PART_ID partId,     /* memory partition to add block to */
    char *pBlock        /* pointer to block of memory to free */
    )
    {
    FAST BLOCK_HDR *pHdr;
    FAST unsigned   nWords;
    FAST BLOCK_HDR *pNextHdr;


    if (ID_IS_SHARED (partId))  /* partition is shared? */
        {
	if (smMemPartFreeRtn == NULL)
	    {
	    errno = S_smObjLib_NOT_INITIALIZED;
	    return (ERROR);
	    }

        return ((*smMemPartFreeRtn) (SM_OBJ_ID_TO_ADRS (partId), pBlock));
	}

    /* partition is local */

    if (OBJ_VERIFY (partId, memPartClassId) != OK)
	return (ERROR);

    if (pBlock == NULL)
	return (OK);				/* ANSI C compatibility */

    pHdr   = BLOCK_TO_HDR (pBlock);

    /* get exclusive access to the partition */

    semTake (&partId->sem, WAIT_FOREVER);

    /* optional check for validity of block */

    if ((partId->options & MEM_BLOCK_CHECK) &&
        !memPartBlockIsValid (partId, pHdr, FALSE))
	{
	semGive (&partId->sem);			/* release mutual exclusion */

	if (memPartBlockErrorRtn != NULL)
	    (* memPartBlockErrorRtn) (partId, pBlock, "memPartFree");

	if (partId->options & MEM_BLOCK_ERROR_SUSPEND_FLAG)
	    {
	    if ((taskIdCurrent->options & VX_UNBREAKABLE) == 0)
		taskSuspend (0);
	    }

	errnoSet (S_memLib_BLOCK_ERROR);
	return (ERROR);
	}
#ifdef WV_INSTRUMENTATION
    EVT_OBJ_3 (OBJ, partId, memPartClassId, EVENT_MEMFREE, partId, pBlock, 2 * (pHdr->nWords));
#endif

    nWords = pHdr->nWords; 

    /* check if we can coalesce with previous block;
     * if so, then we just extend the previous block,
     * otherwise we have to add this as a new free block */

    if (PREV_HDR (pHdr)->free)
	{
	pHdr->free = FALSE;	                /* this isn't a free block */
	pHdr = PREV_HDR (pHdr);			/* coalesce with prev block */
	pHdr->nWords += nWords;
	}
    else
	{
	pHdr->free = TRUE;			/* add new free block */
	dllInsert (&partId->freeList, (DL_NODE *) NULL, HDR_TO_NODE (pHdr));
	}

    /* check if we can coalesce with next block;
     * if so, then we can extend our block delete next block from free list */

    pNextHdr = NEXT_HDR (pHdr);
    if (pNextHdr->free)
	{
	pHdr->nWords += pNextHdr->nWords;	/* coalesce with next */
	dllRemove (&partId->freeList, HDR_TO_NODE (pNextHdr));
	}

    /* fix up prev info of whatever block is now next */

    NEXT_HDR (pHdr)->pPrevHdr = pHdr;

    /* adjust allocation stats */

    partId->curBlocksAllocated--;
    partId->curWordsAllocated -= nWords;

    semGive (&partId->sem);

    return (OK);
    }

/*******************************************************************************
*
* memPartBlockIsValid - check validity of block
*
* This routine checks the validity of a block. If the pointer to the block
* header is bogus we could run into bus errors and the calling task could take
* the partId semaphore with it. To prvent this from happening we make the task
* preemption proof and release the semaphore before doing the block validity 
* check. 
*
* NOMANUAL
*/

BOOL memPartBlockIsValid 
    (
    PART_ID partId,
    FAST BLOCK_HDR *pHdr,
    BOOL isFree                 /* expected status */
    )
    {
    BOOL valid;

    TASK_LOCK ();					/* LOCK PREEMPTION */
    semGive (&partId->sem);				/* release mutex */

    valid = MEM_ALIGNED (pHdr)				/* aligned */
            && MEM_ALIGNED (2 * pHdr->nWords)		/* size is round */
            && (pHdr->nWords <= partId->totalWords)	/* size <= total */
	    && (pHdr->free == isFree)			/* right alloc-ness */
#if (CPU_FAMILY == SH)
	    && MEM_ALIGNED(NEXT_HDR (pHdr))		/* aligned(08aug95,sa)*/
	    && MEM_ALIGNED(PREV_HDR (pHdr))		/* aligned(08aug95,sa)*/
#endif
	    && (pHdr == PREV_HDR (NEXT_HDR (pHdr)))	/* matches next block */
	    && (pHdr == NEXT_HDR (PREV_HDR (pHdr)));	/* matches prev block */

    semTake (&partId->sem, WAIT_FOREVER);		/* reacquire mutex */
    TASK_UNLOCK ();					/* UNLOCK PREEMPTION */

    return (valid);
    }

/*****************************************************************************
*
* memAlignedBlockSplit - split a block on the free list into two blocks
*
* This routine is like memBlockSplit, but also aligns the data block to the
* given alignment.  The block looks like this after it is split:
*
*        |----------------------------------------------------------------|
*        ^                      ^                       ^                 ^
*        |                      |                       | space left over |
*        |                      |<------- nWords ------>| after alignment |
*        |                      |                       |                 |
*   block begin       new aligned buffer begin         end           block end
*
*
* After the split, if the first block has less than minWords in it, the
* block is rejected, and null is returned, since we can't place this first
* block on the free list.
* If the space succeeding the newly allocated aligned buffer is less than the
* minimum block size, then the bytes are added to the newly allocated buffer.
* If the space is greater than the minimum block size,  then a new memory
* fragment is created and added to the free list.
* Care must be taken to insure that the orignal block passed in
* to be split has at least (nWords + alignment/2) words in it.  
* If the block has exactly nWords and is already aligned to the given alignment,
* it will be deleted from the free list and returned unsplit.
* Otherwise, the second block will be returned.
*
* RETURNS: A pointer to a BLOCK_HDR 
*/

LOCAL BLOCK_HDR *memAlignedBlockSplit 
    (
    PART_ID partId,
    FAST BLOCK_HDR *pHdr,
    FAST unsigned nWords,            /* number of words in second block */
    unsigned minWords,               /* min num of words allowed in a block */
    unsigned alignment		     /* boundary to align to */
    )
    {
    FAST BLOCK_HDR *pNewHdr;
    FAST BLOCK_HDR *pNextHdr;
    FAST char *endOfBlock;
    FAST char *pNewBlock;
    int blockSize;

    /* calculate end of pHdr block */

    endOfBlock = (char *) pHdr + (pHdr->nWords * 2); 

    /* caluclate unaligned beginning of new block */ 

    pNewBlock = (char *) ((unsigned) endOfBlock - 
		((nWords - sizeof (BLOCK_HDR) / 2) * 2));

    /* align the beginning of the block */

    pNewBlock = (char *)((unsigned) pNewBlock & ~(alignment - 1));

    pNewHdr = BLOCK_TO_HDR (pNewBlock);

    /* adjust original block's word count */

    blockSize = ((char *) pNewHdr - (char *) pHdr) / 2;


    if (blockSize < minWords)
	{
	/* check to see if the new block is the same as the original block -
	 * if so, delete if from the free list.  If not, reject the newly
	 * split block because it's too small to hang on the free list.
	 */

	if (pNewHdr == pHdr)
	    dllRemove (&partId->freeList, HDR_TO_NODE (pHdr));
	else
	    return (NULL);
	}
    else
	{
	pNewHdr->pPrevHdr = pHdr;
	pHdr->nWords = blockSize;
	}

    /* check to see if space left over after we aligned the new buffer
     * is big enough to be a fragment on the free list.
     */

    if (((UINT) endOfBlock - (UINT) pNewHdr - (nWords * 2)) < (minWords * 2))
	{
	/* nope - give all the memory to the newly allocated block */

	pNewHdr->nWords = (endOfBlock - pNewBlock + sizeof (BLOCK_HDR)) / 2;
	pNewHdr->free     = TRUE; 

	/* fix next block to point to newly allocated block */

	NEXT_HDR (pNewHdr)->pPrevHdr = pNewHdr;
	}
    else
	{
	/* the extra bytes are big enough to be a fragment on the free list -
	 * first, fix up the newly allocated block.
	 */

	pNewHdr->nWords = nWords;
	pNewHdr->free     = TRUE; 

	/* split off the memory after pNewHdr and add it to the free list */

	pNextHdr = NEXT_HDR (pNewHdr);
	pNextHdr->nWords = ((UINT) endOfBlock - (UINT) pNextHdr) / 2;
	pNextHdr->pPrevHdr = pNewHdr;
	pNextHdr->free = TRUE;

	dllAdd (&partId->freeList, HDR_TO_NODE (pNextHdr));	

	/* fix next block to point to the new fragment on the free list */

	NEXT_HDR (pNextHdr)->pPrevHdr = pNextHdr;
	}

    return (pNewHdr);
    }

/******************************************************************************
*
* memAddToPool - add memory to the system memory partition
*
* This routine adds memory to the system memory partition, after the initial
* allocation of memory to the system memory partition.
*
* RETURNS: N/A
*
* SEE ALSO: memPartAddToPool()
*/

void memAddToPool 
    (
    FAST char *pPool,           /* pointer to memory block */
    FAST unsigned poolSize      /* block size in bytes */
    )
    {
    (void) memPartAddToPool (&memSysPartition, pPool, poolSize);
    }
/*******************************************************************************
*
* malloc - allocate a block of memory from the system memory partition (ANSI)
*
* This routine allocates a block of memory from the free list.  The size of
* the block will be equal to or greater than <nBytes>.
*
* RETURNS: A pointer to the allocated block of memory, or a null pointer if
* there is an error.
*
* SEE ALSO: 
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: General Utilities (stdlib.h)"
*/

void *malloc 
    (
    size_t nBytes             /* number of bytes to allocate */
    )
    {
    return (memPartAlloc (&memSysPartition, (unsigned) nBytes));
    }
/*******************************************************************************
*
* free - free a block of memory (ANSI)
*
* This routine returns to the free memory pool a block of memory previously
* allocated with malloc() or calloc().
*
* RETURNS: N/A
*
* SEE ALSO: 
* malloc(), calloc(),
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: General Utilities (stdlib.h)"
*/

void free 
    (
    void *ptr       /* pointer to block of memory to free */
    )
    {
    (void) memPartFree (&memSysPartition, (char *) ptr);
    }

/*******************************************************************************
*
* memPartSemInit - initialize the partition semaphore as type binary
*
* This routine initializes the partition semaphore as a binary semaphore.
* This function is called indirectly from memPartInit().  This coupling
* allows alternative semaphore types to serve as the basis for memory
* partition interlocking.
*/

LOCAL void memPartSemInit
    (
    PART_ID partId		/* partition to initialize semaphore for */
    )
    {
    semBInit (&partId->sem, SEM_Q_PRIORITY, SEM_FULL);
    }

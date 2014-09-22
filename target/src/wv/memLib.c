/* memLib.c - full-featured memory partition manager */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
05x,21feb99,jdi  doc: listed errnos.
05w,01may98,cjtc moved instrumentation point in memPartRealloc to fix problem
                 with reported size of bytesPlusHeaderAlign
05v,17feb98,dvs  added instrumentation points (pr)
05u,11feb97,dbt  Fixed case of memPartRealloc() call with pBlock = NULL (SPR
                 #7012)
05t,18jan96,tam  checked for nbBytes=0 in memPartRealloc() (SPR 5858).
		 reworked the fix for memPartRealloc() (SPR 1407 & 3564).
05s,23jan95,ism  Fixed possible problem with memPartRealloc() from earlier fix
        	 (SPR#3563).  Fixed in-line docs.
05r,23jan95,rhp  doc: explain memPartRealloc() with NULL pointer.
05q,12jan95,rhp  doc: in memLib and memPartOptionsSet man pages, mention
                 VX_UNBREAKABLE override of MEM_*_ERROR_SUSPEND_FLAG (SPR#2499)
05p,29aug94,ism  memBlockSplit() bypasses the problem with the number of free
        	 blocks statistic by incrementing before the memBlockSplit() 
		 call (SPR #3564).
05q,25aug94,ism  realloc() no longer checks for NULL block; instead, this is
		 done at the next level down, in memPartRealloc() (SPR #3563).
05p,05sep93,jcf  calls to logMsg are now indirect for scalability.
05o,23feb93,jdi  documentation: cfree() not ANSI.
05n,10feb93,jdi  fixed spelling of stdlib.
05m,23jan93,jdi  documentation cleanup for 5.1.
05l,13nov92,dnw  added include of smObjLib.h
05k,20oct92,pme  added reference to shared memory manager documentation.
05j,15oct92,pme  fixed valloc documentation.
05i,02oct92,jdi  documentation cleanup.
05h,23aug92,jcf  added _func_{memalign,valloc}
05g,19aug92,smb  fixed SPR #1400, realloc(NULL) now malloc's memory
05f,29jul92,pme  added NULL function pointer check for smObj routines.
05e,29jul92,jcf  moved SUSPEND option to memPartLib.  beefed up package init.
05d,19jul92,pme  added shared memory object support.
05c,19jul92,smb  added some ANSI documentation.
05b,13jul92,rdc  added valloc.
05a,04jul92,jcf  split into memShow/memPartLib/memLib for modularity.
04n,22jun92,rdc  added memalign and memPartAlignedAlloc.
04m,26may92,rrr  the tree shuffle
04l,07apr92,yao  removed macros MEM_ROUND_UP/DOWN, MEM_IS_ROUND.  
04n,25mar92,jmm  Changed way we handle memory pool options for error handling
04m,23mar92,jmm  changed memBlockAllocError to handle MEM_ALLOC_ERROR_RETURN
                 changed memPartAllocError to handle MEM_ALLOC_ERROR_RETURN
		 SPR #1377
04l,16dec91,gae  added includes for ANSI.
04k,25nov91,llk  ansi stuff.  Modified malloc(), calloc(), realloc(), free().
04j,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
04i,07aug91,ajm  made MIPS MEM_ROUNDUP use 8 byte boundaries for
                   varargs problem.
04h,23may91,jwt  forced 8-byte alignment for SPARC - stack, optimization.
04g,13may91,wmd  added define for TRON rounding.
04f,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
04e,24mar91,del  added I960 rounding and padding to structures.
04d,13feb91,jaa	 documentation cleanup.
04c,05oct90,dnw  made memPartInit() be NOMANUAL.
04b,18jul90,jcf  changed free/memPartFree to return OK if pBlock is NULL.
04a,18jul90,jcf  changed partitions into objects w/ object class.
		 change memPartBlockIsValid () to check validity w/o race.
		 lint & cleanup.
03i,26jun90,jcf  changed partition semaphore to mutex w/ inversion safety.
03h,21nov89,jcf  added memSysPartId which is global variable for system part.
03g,07nov89,shl  added error checking for memPartShow().
		 fixed documentation on memory overhead.
		 fixed memPart{Block,Alloc}Error() to not suspend
		 unbreakable tasks.
		 cosmetics.
03f,15mar88,llk  fixed bug in realloc.  memBlockSplit() wasn't checking to
		   ensure that both resulting blocks were of minimum length.
03e,07nov88,gae  fixed memPartFindMax() return value in bytes not words.
03d,18aug88,gae  documentation.
03c,07jul88,jcf  lint.
03b,22jun88,dnw  fixed bug in realloc.
		 added cfree(), memPartFindMax(), and memFindMax().
		 changed hdr to have ptr to prev block (instead of prev block
		   size and free flag), as per gae suggestion.
03a,20may88,dnw  rewritten - now has following features:
		   - faster allocation
		   - much faster deallocation (no searching)
		   - checks validity of blocks being freed
		   - options to specify how to deal with errors
		   - always allocates on even 4 byte address
		   - VxWorks v4 names
02q,28mar88,gae  added calloc().
02p,22jan88,jcf  made kernel independent.
02o,19feb88,rdc  malloc of 0 bytes in no longer an error.
02n,18nov87,dnw  changed to set S_memLib_INVALID_NBYTES appropriately.
02m,17nov87,ecs  lint.
02l,13nov87,gae  moved FRAGMENT definition here from memLib.h.
		 fixed memFindMax() from returning too large an amount.
02k,05nov87,jlf  documentation
02j,23oct87,rdc  generalized to support private memory partitions.
02i,07apr87,jlf  delinted.
02h,24mar87,jlf  documentation.
02g,30jan87,rdc  fixed bug in memAddToPool - was screwing up allocation stats.
02f,21dec86,dnw  changed to not get include files from default directories.
02e,04sep86,jlf  documentation.
02d,02jul86,rdc  fixed bug in memStat.
02c,19jun86,rdc  spruced up memStat.
02b,18jun86,rdc  de-linted.
02a,14apr86,rdc  completely rewritten to support dynamic allocation.
01i,03mar86,jlf  added malloc and free.  free is a null routine, for now.
01h,20jul85,jlf  Documentation.
01g,08sep84,jlf  Changed to lock out while allocating memory.  added copyright
		 and some comments.
01f,13aug84,ecs  changed S_memLib_NO_MORE_MEMORY to S_memLib_NOT_ENOUGH_MEMORY.
		 added testing for negative nbytes to memAllocate.
		 added setting of status to S_memLib_INVALID_NBYTES.
01e,07aug84,ecs  added setStatus call to memAllocate.
01d,21may84,dnw  changed memInit to specify both start and end of pools.
		 removed memEnd.
		 added LINTLIBRARY and various coercions for lint.
01c,03sep83,dnw  added memInit.
01b,13jul83,dnw  fixed memAlloc to return NULL if insufficient memory available.
01a,24may83,dnw  written
*/

/*
DESCRIPTION
This library provides full-featured facilities for managing the allocation
of blocks of memory from ranges of memory called memory partitions.  The
library is an extension of memPartLib and provides enhanced memory management
features, including error handling, aligned allocation, and ANSI allocation
routines.  For more information about the core memory partition management 
facility, see the manual entry for memPartLib.

The system memory partition is created when the kernel is initialized
by kernelInit(), which is called by the root task, usrRoot(), in usrConfig.c.
The ID of the system memory partition is stored in the global variable
`memSysPartId'; its declaration is included in memLib.h.

The memalign() routine is provided for allocating memory aligned to a specified
boundary.

This library includes three ANSI-compatible routines:
calloc() allocates a block of memory for an array;
realloc() changes the size of a specified block of memory; and
cfree() returns to the free memory pool a block of memory that
was previously allocated with calloc().

ERROR OPTIONS
Various debug options can be selected for each partition using
memPartOptionsSet() and memOptionsSet().  Two kinds of errors are 
detected: attempts to allocate more memory
than is available, and bad blocks found when memory is freed.  In both
cases, the error status is returned.  There are four error-handling
options that can be individually selected:

.iP "MEM_ALLOC_ERROR_LOG_FLAG" 8
Log a message when there is an error in allocating memory.
.iP "MEM_ALLOC_ERROR_SUSPEND_FLAG"
Suspend the task when there is an error in allocating memory (unless
the task was spawned with the VX_UNBREAKABLE option, in which case it
cannot be suspended).
.iP "MEM_BLOCK_ERROR_LOG_FLAG"
Log a message when there is an error in freeing memory.
.iP "MEM_BLOCK_ERROR_SUSPEND_FLAG"
Suspend the task when there is an error in freeing memory (unless
the task was spawned with the VX_UNBREAKABLE option, in which case it
cannot be suspended).
.LP

When the following option is specified to check every block freed to the
partition, memPartFree() and free() in memPartLib run consistency checks
of various pointers and values in the header of the block being freed.  If
this flag is not specified, no check will be performed when memory is
freed.

.iP "MEM_BLOCK_CHECK" 8
Check each block freed.
.LP

Setting either of the MEM_BLOCK_ERROR options automatically 
sets MEM_BLOCK_CHECK.

The default options when a partition is created are:

    MEM_ALLOC_ERROR_LOG_FLAG
    MEM_BLOCK_CHECK
    MEM_BLOCK_ERROR_LOG_FLAG
    MEM_BLOCK_ERROR_SUSPEND_FLAG

When setting options for a partition with memPartOptionsSet() or
memOptionsSet(), use the logical OR operator between each specified
option to construct the <options> parameter.  For example:
.CS
    memPartOptionsSet (myPartId, MEM_ALLOC_ERROR_LOG_FLAG |
                                 MEM_BLOCK_CHECK |
                                 MEM_BLOCK_ERROR_LOG_FLAG);
.CE

INCLUDE FILES: memLib.h

SEE ALSO: memPartLib, smMemLib

INTERNAL
This package is initialized by kernelInit() which calls memInit() with a
pointer to a block of memory from which memory will be allocated.

Blocks allocated by malloc() are actually larger than the size requested by
the user.  Each block is prefaced by a header which contains the size and
status of that block and a pointer to the previous block.
The pointer returned to the user points just past this header.
Likewise when a block is freed, the header is found just in front of
the block pointer passed by the user.

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
#include "errno.h"
#include "smObjLib.h"
#include "private/memPartLibP.h"
#include "private/vmLibP.h"
#include "private/smMemLibP.h"
#include "private/funcBindP.h"
#include "private/eventP.h"


/* global variables */

int mutexOptionsMemLib = SEM_Q_PRIORITY | 	/* default mutual exclusion */
			 SEM_DELETE_SAFE |	/* options for memory */
			 SEM_INVERSION_SAFE;	/* partition semaphore */

/* shared memory manager function pointers */

FUNCPTR  smMemPartOptionsSetRtn;
FUNCPTR  smMemPartFindMaxRtn;
FUNCPTR  smMemPartReallocRtn;

/* local variables */

LOCAL BOOL  memLibInstalled;		/* TRUE if library has been installed */
LOCAL char *memMsgBlockTooBig =
	"memPartAlloc: block too big - %d in partition %#x.\n";

LOCAL char *memMsgBlockError =
	"%s: invalid block %#x in partition %#x.\n";

/* forward static functions */

static void memSemInit (PART_ID pPart);
static void memPartAllocError (PART_ID pPart, unsigned nBytes);
static void memPartBlockError (PART_ID pPart, char *pBlock, char *label);
static BLOCK_HDR *memBlockSplit (BLOCK_HDR *pHdr, unsigned nWords,
				 unsigned minWords);


/*******************************************************************************
*
* memLibInit - initialize the higher level memory management functions
*
* This routine initializes the higher level memory management functions
* suchas mutex semaphore selection, and error notification.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS memLibInit (void)
    {
    if (!memLibInstalled)
	{
	_func_valloc	     	= (FUNCPTR) valloc;
	_func_memalign	     	= (FUNCPTR) memalign;
	memPartBlockErrorRtn  	= (FUNCPTR) memPartBlockError;
	memPartAllocErrorRtn  	= (FUNCPTR) memPartAllocError;
	memPartSemInitRtn     	= (FUNCPTR) memSemInit;
    	memPartOptionsDefault	= MEM_ALLOC_ERROR_LOG_FLAG |
			          MEM_BLOCK_ERROR_LOG_FLAG |
			          MEM_BLOCK_ERROR_SUSPEND_FLAG |
			          MEM_BLOCK_CHECK;

	memLibInstalled = TRUE;
	}

    return ((memLibInstalled) ? OK : ERROR);
    }

/*******************************************************************************
*
* memInit - initialize the system memory partition
*
* This routine initializes the system partition free list with the
* specified memory block.  It must be called exactly once before invoking any
* other routine in memLib.  It is called by kernelInit() in usrRoot()
* in usrConfig.c.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS memInit 
    (
    char *pPool,        /* pointer to memory block */
    unsigned poolSize   /* block size in bytes */
    )
    {
    memLibInit ();		/* attach memLib functions to memPartLib */

    return (memPartLibInit (pPool, poolSize));
    }

/*******************************************************************************
*
* memPartOptionsSet - set the debug options for a memory partition
*
* This routine sets the debug options for a specified memory partition.
* Two kinds of errors are detected: attempts to allocate more memory
* than is available, and bad blocks found when memory is freed.  In both
* cases, the error status is returned.  There are four error-handling
* options that can be individually selected:
* 
* .iP "MEM_ALLOC_ERROR_LOG_FLAG" 8
* Log a message when there is an error in allocating memory.
* .iP "MEM_ALLOC_ERROR_SUSPEND_FLAG"
* Suspend the task when there is an error in allocating memory (unless
* the task was spawned with the VX_UNBREAKABLE option, in which case it
* cannot be suspended).
* .iP "MEM_BLOCK_ERROR_LOG_FLAG"
* Log a message when there is an error in freeing memory.
* .iP "MEM_BLOCK_ERROR_SUSPEND_FLAG"
* Suspend the task when there is an error in freeing memory (unless
* the task was spawned with the VX_UNBREAKABLE option, in which case it
* cannot be suspended).
* .LP
* 
* These options are discussed in detail in the library manual entry for
* memLib.
* 
* RETURNS: OK or ERROR.
*
* ERRNO: S_smObjLib_NOT_INITIALIZED
*
* SEE ALSO: smMemLib
*/

STATUS memPartOptionsSet 
    (
    PART_ID  	partId,    /* partition to set option for */
    unsigned  	options    /* memory management options */
    )
    {
    if ((!memLibInstalled) && (memLibInit () != OK))	/* initialize package */
	return (ERROR);

    if (ID_IS_SHARED (partId))  /* partition is shared? */
	{
	if (smMemPartOptionsSetRtn == NULL)
	    {
	    errno = S_smObjLib_NOT_INITIALIZED;
	    return (ERROR);
	    }

        return ((*smMemPartOptionsSetRtn)(SM_OBJ_ID_TO_ADRS (partId), options));
	}

    /* partition is local */

    if (OBJ_VERIFY (partId, memPartClassId) != OK)
	return (ERROR);

    semTake (&partId->sem, WAIT_FOREVER);

    /*
     * Need to convert old-style options to new-style options
     */

    if (options & (MEM_ALLOC_ERROR_MASK | MEM_BLOCK_ERROR_MASK))
	{
	if (options & MEM_ALLOC_ERROR_LOG_MSG)
	   options |= MEM_ALLOC_ERROR_LOG_FLAG;

	if (options & MEM_ALLOC_ERROR_LOG_AND_SUSPEND)
	    options |= (MEM_ALLOC_ERROR_LOG_FLAG |
			MEM_ALLOC_ERROR_SUSPEND_FLAG);

	if (options & MEM_BLOCK_ERROR_LOG_MSG)
	   options |= MEM_BLOCK_ERROR_LOG_FLAG;

	if (options & MEM_BLOCK_ERROR_LOG_AND_SUSPEND)
	    options |= (MEM_BLOCK_ERROR_LOG_FLAG |
			MEM_BLOCK_ERROR_SUSPEND_FLAG);
	}

    /*
     * If either of the MEM_BLOCK options have been set, also set
     * MEM_BLOCK_CHECK.
     */

    if (options & (MEM_BLOCK_ERROR_LOG_FLAG | MEM_BLOCK_ERROR_SUSPEND_FLAG))
        options |= MEM_BLOCK_CHECK;
    
    partId->options = options;

    semGive (&partId->sem);

    return (OK);
    }

/*******************************************************************************
*
* memalign - allocate aligned memory 
*
* This routine allocates a buffer of size <size> from the system memory 
* partition.  Additionally, it insures that the allocated buffer begins 
* on a memory address evenly divisible by the specified alignment parameter.
* The alignment parameter must be a power of 2.
*
* RETURNS:
* A pointer to the newly allocated block, or NULL if the buffer could not be
* allocated.
*/

void *memalign 
    (
    unsigned alignment,		/* boundary to align to (power of 2) */
    unsigned size               /* number of bytes to allocate */
    )
    {
    return (memPartAlignedAlloc (memSysPartId, size, alignment));
    }

/*******************************************************************************
*
* valloc - allocate memory on a page boundary 
*
* This routine allocates a buffer of <size> bytes from the system memory
* partition.  Additionally, it insures that the allocated buffer
* begins on a page boundary.  Page sizes are architecture-dependent.
*
* RETURNS:
* A pointer to the newly allocated block, or NULL if the buffer could not be
* allocated or the memory management unit (MMU) support library has not been
* initialized.
*
* ERRNO: S_memLib_PAGE_SIZE_UNAVAILABLE
*/

void * valloc 
    (
    unsigned size	/* number of bytes to allocate */
    )
    {
    int pageSize = VM_PAGE_SIZE_GET();

    if (pageSize == ERROR)
	{
	errno = S_memLib_PAGE_SIZE_UNAVAILABLE;
	return (NULL);
	}

    return (memalign (pageSize, size));
    }

/*******************************************************************************
*
* memPartRealloc - reallocate a block of memory in a specified partition
*
* This routine changes the size of a specified block of memory and returns a
* pointer to the new block.  The contents that fit inside the new size (or
* old size if smaller) remain unchanged.  The memory alignment of the new
* block is not guaranteed to be the same as the original block.
*
* If <pBlock> is NULL, this call is equivalent to memPartAlloc().
*
* RETURNS:
* A pointer to the new block of memory, or NULL if the call fails.
*
* ERRNO: S_smObjLib_NOT_INITIALIZED
*
* SEE ALSO: smMemLib
*/

void *memPartRealloc 
    (
    PART_ID partId,             /* partition ID */
    char *pBlock,               /* block to be reallocated */
    unsigned nBytes             /* new block size in bytes */
    )
    {
    FAST BLOCK_HDR *pHdr = BLOCK_TO_HDR (pBlock);
    FAST BLOCK_HDR *pNextHdr;
    FAST unsigned nWords;
    void *pNewBlock;

    if (ID_IS_SHARED (partId))  /* partition is shared? */
	{
	if (smMemPartReallocRtn == NULL)
	    {
	    errno = S_smObjLib_NOT_INITIALIZED;
	    return (NULL);
	    }

        return ((void *) (*smMemPartReallocRtn) (SM_OBJ_ID_TO_ADRS (partId), 
						 pBlock, nBytes));
	}

    /* partition is local */

    if (OBJ_VERIFY (partId, memPartClassId) != OK)
	return (NULL);

    /* Call memPartAlloc() if user passed us a NULL pointer */

    if (pBlock == NULL)
        return (memPartAlloc (partId, (unsigned) nBytes));
   
    /* Call memPartFree() if nBytes=0 (SPR 5858) */

    if (nBytes == 0)
	{
	(void) memPartFree (memSysPartId, pBlock);
	return (NULL);
	}

    /* get exclusive access to the partition */

    semTake (&partId->sem, WAIT_FOREVER);

    /* get actual new size; round-up, add overhead, check for minimum */

    nWords = (MEM_ROUND_UP (nBytes) + sizeof (BLOCK_HDR)) >> 1;
    if (nWords < partId->minBlockWords)
	nWords = partId->minBlockWords;

    /* optional check for validity of block */

    if ((partId->options & MEM_BLOCK_CHECK) &&
        !memPartBlockIsValid (partId, pHdr, FALSE))
	{
	semGive (&partId->sem);			/* release mutual exclusion */
	memPartBlockError (partId, pBlock, "memPartRealloc");
	return (NULL);
	}

    /* test if we are trying to increase size of block */

    if (nWords > pHdr->nWords)
	{
	/* increase size of block -
	 * check if next block is free and is big enough to satisfy request*/

	pNextHdr = NEXT_HDR (pHdr);

	if (!pNextHdr->free || ((pHdr->nWords + pNextHdr->nWords) < nWords))
	    {
	    /* can't use adjacent free block -
	     * allocate an entirely new block and copy data */

	    semGive (&partId->sem);

	    if ((pNewBlock = memPartAlloc (partId, nBytes)) == NULL)
		return (NULL);

	    bcopy (pBlock, (char *) pNewBlock,
		   (int) (2 * pHdr->nWords - sizeof (BLOCK_HDR)));

	    (void) memPartFree (partId, pBlock);

	    return (pNewBlock);		/* RETURN, don't fall through */
	    }
	else
	    {
	    /* append next block to this one -
	     *  - delete next block from freelist,
	     *  - add its size to this block,
	     *  - update allocation statistics,
	     *  - fix prev info in new "next" block header */
	    dllRemove (&partId->freeList, HDR_TO_NODE (pNextHdr));

	    pHdr->nWords += pNextHdr->nWords;			/* fix size */

#ifdef WV_INSTRUMENTATION
	    /*
	     * The following call to instrumentation will log an a greater
	     * than needed nBytesPlusHeaderAlign. This is because, at this
	     * point, nBytesPlusHeaderAlign includes the total size of the
	     * newly freed memory block. The unused portion will be returned
	     * to the free list (see memBlockSplit() just below, at which
	     * time the excess will be logged as an EVENT_MEMFREE
	     */
	    EVT_OBJ_4 (OBJ, partId, memPartClassId, EVENT_MEMREALLOC,
	    		    partId, pBlock, 2* (pHdr->nWords), nBytes);
#endif

	    partId->curWordsAllocated += pNextHdr->nWords;	/* fix stats */
	    partId->cumWordsAllocated += pNextHdr->nWords;

	    NEXT_HDR (pHdr)->pPrevHdr = pHdr;			/* fix next */

	    /* if this combined block is too big, it will get fixed below */
	    }
	}


    /* split off any extra and give it back;
     * note that this covers both the case of a realloc for smaller space
     * and the case of a realloc for bigger space that caused a coalesce
     * with the next block that resulted in larger than required block */

    pNextHdr = memBlockSplit (pHdr, pHdr->nWords - nWords,
			      partId->minBlockWords);

    semGive (&partId->sem);

    if (pNextHdr != NULL)
	{
	(void) memPartFree (partId, HDR_TO_BLOCK (pNextHdr));
	partId->curBlocksAllocated++;

	/* memPartFree() is going to subtract one from the curBlocksAllocated
	 * statistic. Since memPartRealloc() should not change the value of
	 * curBlocksAllocated, it has to be corrected  (SPR 1407 & 3564)
	 */

	}

    return ((void *) pBlock);
    }
/*******************************************************************************
*
* memPartFindMax - find the size of the largest available free block
*
* This routine searches for the largest block in the memory partition free
* list and returns its size.
*
* RETURNS: The size, in bytes, of the largest available block.
*
* ERRNO: S_smObjLib_NOT_INITIALIZED
*
* SEE ALSO: smMemLib
*/

int memPartFindMax 
    (
    FAST PART_ID partId         /* partition ID */
    )
    {
    FAST BLOCK_HDR *	pHdr;
    FAST DL_NODE *	pNode;
    FAST unsigned	biggestWords = 0;

    if (ID_IS_SHARED (partId))  /* partition is shared? */
	{
	if (smMemPartFindMaxRtn == NULL)
	    {
	    errno = S_smObjLib_NOT_INITIALIZED;
	    return (ERROR);
	    }

        return ((*smMemPartFindMaxRtn) (SM_OBJ_ID_TO_ADRS (partId)));
	}

    /* partition is local */

    if (OBJ_VERIFY (partId, memPartClassId) != OK)
	return (ERROR);

    semTake (&partId->sem, WAIT_FOREVER);

    /* go through free list and find largest free */

    for (pNode = DLL_FIRST (&partId->freeList);
	 pNode != NULL;
	 pNode = DLL_NEXT (pNode))
	{
	pHdr = NODE_TO_HDR (pNode);
	if (pHdr->nWords > biggestWords)
	    biggestWords = pHdr->nWords;
	}

    semGive (&partId->sem);

    return (2 * biggestWords - sizeof (BLOCK_HDR));
    }
/*******************************************************************************
*
* memPartAllocError - handle allocation error
*/

LOCAL void memPartAllocError 
    (
    PART_ID pPart,
    unsigned nBytes 
    )
    {
    if ((_func_logMsg != NULL) &&
	(pPart->options & MEM_ALLOC_ERROR_LOG_FLAG))
	(* _func_logMsg) (memMsgBlockTooBig, nBytes, (int) pPart, 0, 0, 0, 0);
    }

/*******************************************************************************
*
* memPartBlockError - handle invalid block error
*/

LOCAL void memPartBlockError 
    (
    PART_ID pPart,
    char *pBlock,
    char *label 
    )
    {
    if ((_func_logMsg != NULL) &&
        (pPart->options & MEM_BLOCK_ERROR_LOG_FLAG))
        (* _func_logMsg) (memMsgBlockError, (int)label, (int) pBlock, 
			  (int) pPart, 0, 0, 0);
    }

/*****************************************************************************
*
* memBlockSplit - split a block into two blocks
*
* This routine splits the block pointed to into two blocks.  The second
* block will have nWords words in it.  A pointer is returned to this block.
* If either resultant block would end up having less than minWords in it,
* NULL is returned.
*
* RETURNS: A pointer to the second block, or NULL.
*/

LOCAL BLOCK_HDR *memBlockSplit 
    (
    FAST BLOCK_HDR *pHdr,
    unsigned nWords,            /* number of words in second block */
    unsigned minWords           /* min num of words allowed in a block */
    )
    {
    FAST unsigned wordsLeft;
    FAST BLOCK_HDR *pNewHdr;

    /* check if block can be split */

    if ((nWords < minWords) ||
        ((wordsLeft = (pHdr->nWords - nWords)) < minWords))
        return (NULL);                  /* not enough space left */

    /* adjust original block size and create new block */

    pHdr->nWords = wordsLeft;

    pNewHdr = NEXT_HDR (pHdr);

    pNewHdr->pPrevHdr = pHdr;
    pNewHdr->nWords   = nWords;
    pNewHdr->free     = pHdr->free;

    /* fix next block */

    NEXT_HDR (pNewHdr)->pPrevHdr = pNewHdr;

    return (pNewHdr);
    }


/*******************************************************************************
*
* memOptionsSet - set the debug options for the system memory partition
*
* This routine sets the debug options for the system memory partition.
* Two kinds of errors are detected:  attempts to allocate more memory than
* is available, and bad blocks found when memory is freed.  In both cases,
* the following options can be selected for actions to be taken when the error
* is detected:  (1) return the error status, (2) log an error message and
* return the error status, or (3) log an error message and suspend the
* calling task.
*
* These options are discussed in detail in the library manual
* entry for memLib.
*
* RETURNS: N/A
*
* SEE ALSO: memPartOptionsSet()
*/

void memOptionsSet 
    (
    unsigned options    /* options for system partition */
    )
    {
    memPartOptionsSet (memSysPartId, options);
    }
/*******************************************************************************
*
* calloc - allocate space for an array (ANSI)
*
* This routine allocates a block of memory for an array that contains
* <elemNum> elements of size <elemSize>.  This space is initialized to
* zeros.
*
* RETURNS:
* A pointer to the block, or NULL if the call fails.
*
* SEE ALSO:
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: General Utilities (stdlib.h)"
*/

void *calloc 
    (
    size_t elemNum,	/* number of elements */
    size_t elemSize	/* size of elements */
    )
    {
    FAST void *pMem;
    FAST size_t nBytes = elemNum * elemSize;
    
    if ((pMem = memPartAlloc (memSysPartId, (unsigned) nBytes)) != NULL)
	bzero ((char *) pMem, (int) nBytes);

    return (pMem);
    }
/*******************************************************************************
*
* realloc - reallocate a block of memory (ANSI)
*
* This routine changes the size of a specified block of memory and returns a
* pointer to the new block of memory.  The contents that fit inside the new
* size (or old size if smaller) remain unchanged.  The memory alignment of
* the new block is not guaranteed to be the same as the original block.
*
* RETURNS:
* A pointer to the new block of memory, or NULL if the call fails.
*
* SEE ALSO:
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: General Utilities (stdlib.h)"
*/

void *realloc 
    (
    void *pBlock,	/* block to reallocate */
    size_t newSize	/* new block size */
    )
    {
        return (memPartRealloc (memSysPartId, (char *) pBlock, 
				(unsigned) newSize));
    }
/*******************************************************************************
*
* cfree - free a block of memory
*
* This routine returns to the free memory pool a block of memory 
* previously allocated with calloc().
*
* It is an error to free a memory block that was not previously allocated.
*
* RETURNS: OK, or ERROR if the the block is invalid.
*/

STATUS cfree 
    (
    char *pBlock        /* pointer to block of memory to free */
    )
    {
    return (memPartFree (memSysPartId, pBlock));
    }
/*******************************************************************************
*
* memFindMax - find the largest free block in the system memory partition
*
* This routine searches for the largest block in the system memory partition
* free list and returns its size.
*
* RETURNS: The size, in bytes, of the largest available block.
*
* SEE ALSO: memPartFindMax()
*/

int memFindMax (void)
    {
    return (memPartFindMax (memSysPartId));
    }

/*******************************************************************************
*
* memSemInit - initialize the partition semaphore as type mutex
*
* This routine initializes the partition semaphore as a mutex semaphore with
* the mutex options stored in mutexOptionsMemLib.  This function is called
* indirectly from memPartInit ().  This coupling allows alternative semaphore
* types to serve as the basis for memory partition interlocking.
*/

LOCAL void memSemInit
    (
    PART_ID partId		/* partition to initialize semaphore for */
    )
    {
    semMInit (&partId->sem, mutexOptionsMemLib);
    }


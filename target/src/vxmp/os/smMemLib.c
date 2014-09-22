/* smMemLib.c - shared memory management library (VxMP Option) */

/* Copyright 1995-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01p,03may02,mas  default now is always MEM_BLOCK_CHECK (SPR 3756); cache flush
		 and volatile fix (SPR 68334); bridge flush fix (SPR 68844)
01o,24oct01,mas  doc update (SPR 71149)
01n,09oct01,mas  smMemPartFindMax() can now return zero; code cleanup(SPR70280)
01m,14mar99,dat  SPR 20308, fixed smMemBlockSplit.
01l,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01k,21jun96,dbt  Update copyright.
		 Removed warning concerning SM_ALIGN_BOUNDARY (SPR #4903).
01j,08feb93,jdi  documentation cleanup.
            pme  removed BLOCK_CHECK default option.
01i,29jan93,pme  added little endian support.
		 added partOptions where needed to get rid of i960 cc warnings.
		 made smMemAddToPool() return STATUS. Made smMemFindMax() 
		 return ERROR if access to the partition fails.
		 documentation.
01h,21nov92,jdi  documentation cleanup.
01g,02oct92,pme  added SPARC support. documentation cleanup.
01f,29sep92,pme  changed user callable routine names for coherency
                 added ERRNO section in comments. 
		 rounded down poolSize in smMemPartAddToPool
		 changed smMemPartFree() to mark coalesced blocks as not free
01e,29jul92,pme  removed partition access release from smMemPartBlockIsValid.
01d,28jul92,pme  made smMemPartCreate return PART_ID instead of SM_PART_ID.
01c,28jul92,pme  changed SM_OBJ_ALIGN_BOUNDARY to SM_ALIGN_BOUNDARY.
01b,28jul92,pme  changed memory alignement to SM_OBJ_ALIGN_BOUNDARY bytes.
		 replaced MEM_ROUND_UP by SM_MEM_ROUND_UP and
		 MEM_IS_ROUND by SM_MEM_IS_ROUND.
		 added smMemPartAlignedAlloc() and smMemAlignedBlockSplit().
01a,19jul92,pme  simplified smMemPartBlockError and smMemPartAllocError.
                 moved smMemShow() and smMemPartShow() to smMemShow.c
                 written from 04l memLib.c
*/

/*
DESCRIPTION
This library provides facilities for managing the allocation of blocks of
shared memory from ranges of memory called shared memory partitions.  The
routine memPartSmCreate() is used to create shared memory partitions in
the shared memory pool.  The created partition can be manipulated using
the generic memory partition calls, memPartAlloc(), memPartFree(), etc.
(for a complete list of these routines, see the manual entry for
memPartLib).  The maximum number of partitions that can be created is
determined by the configuration parameter SM_OBJ_MAX_MEM_PART .

The smMem...() routines provide an easy-to-use interface to the shared
memory system partition.  The shared memory system partition is created 
when the shared memory object facility is initialized.

Shared memory management information and statistics display routines are
provided by smMemShow.

The allocation of memory, using memPartAlloc() in the general case and 
smMemMalloc() for the shared memory system partition, is done with a 
first-fit algorithm.  Adjacent blocks of memory are coalesced when freed
using memPartFree() and smMemFree().

There is a 28-byte overhead per allocated block (architecture dependent),
and allocated blocks are aligned on a 16-byte boundary.

All memory used by the shared memory facility must be in the same 
address space, that is, it must be reachable from all the CPUs with the 
same offset as the one used for the shared memory anchor.

CONFIGURATION
Before routines in this library can be called, the shared memory objects
facility must be initialized by a call to usrSmObjInit(), which is 
found in \f3target/config/comps/src/usrSmObj.c\f1.  This is done automatically
by VxWorks when the INCLUDE_SM_OBJ component is included.

ERROR OPTIONS
Various debug options can be selected for each partition using
memPartOptionsSet() and smMemOptionsSet().  Two kinds of errors are
detected:  attempts to allocate more memory than is available, and bad
blocks found when memory is freed.  In both cases, options can be selected
for system actions to take place when the error is detected: (1) return
the error status, (2) log an error message and return the error status, or
(3) log an error message and suspend the calling task.

One of the following options can be specified to determine
the action to be taken when there is an attempt to allocate more
memory than is available in the partition:

\is
\i `MEM_ALLOC_ERROR_RETURN'
just return the error status to the calling task.
\i `MEM_ALLOC_ERROR_LOG_MSG'
log an error message and return the status to the calling task.
\i `MEM_ALLOC_ERROR_LOG_AND_SUSPEND'
log an error message and suspend the calling task.
\ie

The following option is specified by default to check every
block freed to the partition.  If this option is specified, memPartFree()
and smMemFree() will make a consistency check of various pointers and values
in the header of the block being freed.

\is
\i `MEM_BLOCK_CHECK'
check each block freed.
\ie

One of the following options can be specified to determine the action to
be taken when a bad block is detected when freed.  These options apply
only if the MEM_BLOCK_CHECK option is selected.

\is
\i `MEM_BLOCK_ERROR_RETURN'
just return the status to the calling task.
\i `MEM_BLOCK_ERROR_LOG_MSG'
log an error message and return the status to the calling task.
\i `MEM_BLOCK_ERROR_LOG_AND_SUSPEND'
log an error message and suspend the calling task.
\ie
The default options when a shared partition is created are
MEM_ALLOC_ERROR_LOG_MSG, MEM_BLOCK_CHECK, MEM_BLOCK_ERROR_RETURN.

When setting options for a partition with memPartOptionsSet() or
smMemOptionsSet(), use the logical OR operator between each specified
option to construct the <options> parameter.  For example:

\cs
    memPartOptionsSet (myPartId, MEM_ALLOC_ERROR_LOG_MSG |
                                 MEM_BLOCK_CHECK | 
				 MEM_BLOCK_ERROR_LOG_MSG);
\ce
AVAILABILITY
This module is distributed as a component of the unbundled shared memory
objects support option, VxMP.

INCLUDE FILES: smMemLib.h

SEE ALSO: smMemShow, memLib, memPartLib, smObjLib, usrSmObjInit(),
\tb VxWorks Programmer's Guide: Shared Memory Objects

INTERNAL
Blocks allocated by smMemMalloc() are actually larger than the size
requested by the user.  Each block is prefaced by a header which contains
the size and status of that block and a pointer to the previous block.
The pointer returned to the user points just past this header.  Likewise
when a block is freed, the header is found just in front of the block
pointer passed by the user.

The header data is arranged so that the pointer to the previous block
comes first and is therefore adjacent to that previous block.  Thus each
block has 4 bytes of redundant information on either side of it (the 4
bytes of size and status before it in its own header, and the 4-byte
pointer after it in the next header).  This redundant information is
optionally used in smMemFree() and smMemRealloc() to make a consistency
check on blocks specified by the user.  This mechanism helps to detect two
common errors: (1) bad block pointers passed to smMemFree() or
smMemRealloc() are usually detected, and (2) trashing up to 4 bytes on
either side of the block will only affect that block and will also be
detected by smMemFree() or smMemRealloc().

There is a minimum block size which malloc() allocates; this is to insure
that there will be enough room for the free list links that must be used
when the block is freed if it cannot be coalesced with an adjacent block.
The malloc() and realloc() routines always force the requested block size to
be multiple of SM_ALIGN_BOUNDARY; since smMemPartInit() forces the 
initial pool to be on a SM_ALIGN_BOUNDARY-byte boundary, this causes 
all blocks to lie on a SM_ALIGN_BOUNDARY-byte boundaries, 
thus insuring that the block passed back to the user will always 
lie on a SM_ALIGN_BOUNDARY byte boundary.

The memory partition shared semaphore is a structure in the partition
descriptor rather than a pointer to a dynamically created shared semaphore
structure.  This is because of the chicken-and-the-egg problem of smMemLib
using semaphores and semBSmCreate calling smMemPartAlloc.  Instead the
structure is simply declared directly in the partition structure and we call
semSmBInit() instead of semBSmCreate().

All blocks allocated from a shared memory partition are aligned on
SM_ALIGN_BOUNDARY which defines the minimum common alignement value
needed by all the supported architecture. 
*/

/* includes */

#include "vxWorks.h"
#include "memLib.h"
#include "semLib.h"
#include "logLib.h"
#include "intLib.h"
#include "taskLib.h"
#include "cacheLib.h"
#include "string.h"
#include "errnoLib.h"
#include "netinet/in.h"
#include "smLib.h"
#include "smMemLib.h"
#include "private/smObjLibP.h"
#include "private/memPartLibP.h"


/* defines */

/* 
 * Macros for rounding: all allocated blocks are aligned 
 * on the maximum required alignement value across all available 
 * architectures.  This value named SM_ALIGN_BOUNDARY 
 * is set to 16 bytes since i960 version is available.
 */

#define SM_MEM_ROUND_UP(x)	(ROUND_UP(x, SM_ALIGN_BOUNDARY))
#define SM_MEM_ROUND_DOWN(x)    (ROUND_DOWN(x, SM_ALIGN_BOUNDARY))
#define SM_MEM_IS_ROUND(x)	(ALIGNED(x, SM_ALIGN_BOUNDARY))

/* locals */

LOCAL char * smMemMsgBlockTooBig =
      "smMemPartAlloc: block too big - %d in shared partition %#x.\n";

LOCAL char * smMemMsgBlockError =
      "%s: invalid block %#x in shared partition %#x.\n";

/* forward declarations */

LOCAL void smMemPartAllocError (SM_PART_ID volatile pPart, unsigned nBytes);
LOCAL void smMemPartBlockError (SM_PART_ID volatile pPart, char * pBlock,
				 char * label);
LOCAL SM_BLOCK_HDR volatile * smMemBlockSplit (SM_BLOCK_HDR volatile * pHdr,
                                               unsigned nWords,
                                               unsigned minWords);
LOCAL SM_BLOCK_HDR volatile * smMemAlignedBlockSplit (\
                                        SM_PART_ID volatile partId,
					SM_BLOCK_HDR volatile * pHdr,
				        unsigned nWords, unsigned minWords,
				      	unsigned alignment);


/******************************************************************************
*
* smMemPartLibInit - initialize the shared memory manager facility
*
* This routine initializes the shared memory manager function pointers to
* allow generic memory management calls to manipulate shared memory
* partitions.  It is called by smObjInit().
*
* RETURNS: N/A.
*
* NOMANUAL
*/

void smMemPartLibInit (void)
    {
    /* Initialize shared memory manager function pointers */

    smMemPartAddToPoolRtn  = (FUNCPTR) smMemPartAddToPool;
    smMemPartFreeRtn       = (FUNCPTR) smMemPartFree;
    smMemPartAllocRtn      = (FUNCPTR) smMemPartAlloc;
    smMemPartOptionsSetRtn = (FUNCPTR) smMemPartOptionsSet;
    smMemPartFindMaxRtn    = (FUNCPTR) smMemPartFindMax;
    smMemPartReallocRtn    = (FUNCPTR) smMemPartRealloc;
    }

/******************************************************************************
*
* memPartSmCreate - create a shared memory partition (VxMP Option)
*
* This routine creates a shared memory partition that can be used by tasks
* on all CPUs in the system.  It returns a partition ID which can then be
* passed to generic memPartLib routines to manage the partition (i.e., to
* allocate and free memory blocks in the partition).
*
* <pPool> is the global address of shared memory dedicated to the
* partition.  The memory area pointed to by <pPool> must be in the same
* address space as the shared memory anchor and shared memory pool.
*
* <poolSize> is the size in bytes of shared memory dedicated to the partition.
*
* Before this routine can be called, the shared memory objects facility must
* be initialized (see smMemLib).
*
* NOTE
* The descriptor for the new partition is allocated out of an internal
* dedicated shared memory partition.  The maximum number of partitions that can
* be created is SM_OBJ_MAX_MEM_PART .
*
* Memory pool size is rounded down to a 16-byte boundary.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS:
* The partition ID, or NULL if there is insufficient memory in the dedicated
* partition for a new partition descriptor.
*
* ERRNO:
*  S_memLib_NOT_ENOUGH_MEMORY
*  S_smObjLib_LOCK_TIMEOUT 
*
* SEE ALSO: memLib
*/

PART_ID memPartSmCreate
    (
    char *	pPool,		/* global address of shared memory area */
    unsigned	poolSize	/* size in bytes */
    )
    {
    SM_PART_ID pPart;           /* created partition ID */
    int        tmp;             /* temp storage */

    /* 
     * Allocate a shared partition structure from the internal 
     * shared partition memory pool.
     */

    pPart = (SM_PART_ID) smMemPartAlloc ((SM_PART_ID) smPartPartId,
                                         sizeof (SM_PARTITION));

    if (pPart != LOC_NULL)
	{
        smMemPartInit ((SM_PART_ID) pPart, pPool, poolSize);

        /* update smObj statistics */

        CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
        tmp = pSmObjHdr->curNumPart;    /* PCI bridge bug [SPR 68844] */

    	pSmObjHdr->curNumPart = htonl (ntohl (pSmObjHdr->curNumPart) + 1);

        CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
        tmp = pSmObjHdr->curNumPart;    /* BRIDGE FLUSH  [SPR 68334] */
	}

    /* now return the global address of the partition */

    return ((PART_ID) SM_OBJ_ADRS_TO_ID (pPart));
    }

/******************************************************************************
*
* smMemPartInit - initialize a shared memory partition
*
* This routine initializes a shared partition free list, seeding it with the
* memory block passed as an argument.  It must be called exactly once
* for each memory partition created. 
* 
* <partId> is the local address of the partition structure 
* previously allocated from the shared partitions structures partition 
* (smPartPartId).  
*
* <pPool> is the global address of shared memory dedicated to the partition.
*
* <poolSize> is the size in bytes of shared memory dedicated to the partition.
*
* SEE ALSO: memPartSmCreate()
*
* NOMANUAL
*/

void smMemPartInit 
    (
    SM_PART_ID volatile partId,      /* partition to initialize */
    char *              pPool,       /* pointer to memory pool */
    unsigned            poolSize     /* pool size in bytes */
    )
    {
    int                 tmp;         /* temp storage */

    /* initialize partition descriptor */

    bzero ((char *) partId, sizeof (*partId));

    partId->options = htonl (MEM_ALLOC_ERROR_LOG_FLAG | MEM_BLOCK_CHECK |
                             MEM_BLOCK_ERROR_RETURN);

    partId->minBlockWords = htonl (sizeof (SM_FREE_BLOCK) >> 1);

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->options;          /* BRIDGE FLUSH  [SPR 68334] */

    /* initialize partition semaphore and free list and add memory to pool */

    semSmBInit (&partId->sem,  SEM_Q_FIFO, SEM_FULL);
    smDllInit (&partId->freeList);

    partId->objType = htonl (MEM_PART_TYPE_SM_STD);

    /* verify field must contain partition header global address */

    partId->verify = (UINT32) htonl (LOC_TO_GLOB_ADRS (partId));

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->options;          /* BRIDGE FLUSH  [SPR 68334] */

    (void) smMemPartAddToPool (partId, pPool, poolSize);
    }

/******************************************************************************
*
* smMemPartAddToPool - add memory to a shared memory partition
*
* This routine adds memory to a shared memory partition after the initial 
* call to memPartSmCreate().  The memory added need not be contiguous with 
* memory previously assigned to the partition but it must be in the same
* address space.
*
* This routine is not user callable, it is called by the generic memory
* partition routine memPartAddToPool().
*
* <partId> is the shared partition header local address of partition to use.
*
* <pPool> is the global address of shared memory dedicated to the partition.
* The memory area pointed to by <pPool> must be in the same address space
* than the shared memory anchor and shared memory pool since shared
* memory cannot be referenced using different offsets.
*
* <poolSize> is the size in bytes of shared memory added to the partition.
* 
* NOTE: Internally memory pool size is rounded down to a 16-byte boundary.
*
* RETURNS: OK or ERROR.
*
* ERRNO:
*  S_objLib_OBJ_ID_ERROR
*  S_smObjLib_LOCK_TIMEOUT 
*
* SEE ALSO: memPartSmCreate()
*
* NOMANUAL
*/

STATUS smMemPartAddToPool 
    (
    SM_PART_ID volatile partId,     /* partition to modify */
    char *              pPool,      /* pointer to memory pool */
    unsigned            poolSize    /* pool size in bytes */
    )
    {
    SM_BLOCK_HDR volatile * pHdrStart;
    SM_BLOCK_HDR volatile * pHdrMid;
    SM_BLOCK_HDR volatile * pHdrEnd;
    char *                  pTmp;
    int                     tmp;         /* temp storage */

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->verify;           /* PCI bridge bug [SPR 68844] */

    if (SM_OBJ_VERIFY (partId) != OK)
        {
	return (ERROR);
        }
 
    /* insure that the pool starts on an even byte boundry */

    pTmp      = (char *) SM_MEM_ROUND_UP (pPool);	 /* get actual start */
    poolSize  = SM_MEM_ROUND_DOWN (poolSize - (pTmp - pPool)); /* adj length */
    pPool     = (char *) GLOB_TO_LOC_ADRS (pTmp);        /* get local adrs */

    /* 
     * initialize three blocks one at each end of the pool 
     * for end cases and real initial free block 
     */

    pHdrStart		= (SM_BLOCK_HDR volatile *) pPool;
    pHdrStart->pPrevHdr = NULL;
    pHdrStart->free     = htonl (FALSE);
    pHdrStart->nWords   = htonl (sizeof (SM_BLOCK_HDR) >> 1);

    pHdrMid		= (SM_BLOCK_HDR volatile *) SM_NEXT_HDR (pHdrStart);
    pHdrMid->pPrevHdr   = (SM_BLOCK_HDR *)htonl (LOC_TO_GLOB_ADRS (pHdrStart));
    pHdrMid->free       = htonl (TRUE);
    pHdrMid->nWords     = htonl ((poolSize - 2 * sizeof (SM_BLOCK_HDR)) >> 1);

    pHdrEnd		= (SM_BLOCK_HDR volatile *) SM_NEXT_HDR (pHdrMid);
    pHdrEnd->pPrevHdr   = (SM_BLOCK_HDR *) htonl (LOC_TO_GLOB_ADRS (pHdrMid));
    pHdrEnd->free       = htonl (FALSE);
    pHdrEnd->nWords     = htonl (sizeof (SM_BLOCK_HDR) >> 1);

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->options;          /* BRIDGE FLUSH  [SPR 68334] */

    if (smMemPartAccessGet (partId) != OK)
        {
        return (ERROR);
        }

    smDllInsert(&partId->freeList, (SM_DL_NODE *)NULL, SM_HDR_TO_NODE(pHdrMid));

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->totalWords;       /* PCI bug       [SPR 68844] */

    partId->totalWords = htonl (ntohl (partId->totalWords) + (poolSize >> 1));

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->options;          /* BRIDGE FLUSH  [SPR 68334] */

    if (smMemPartAccessRelease (partId) != OK)
        {
        return (ERROR);
        }

    return (OK);
    }

/******************************************************************************
*
* smMemPartOptionsSet - set the debug options for a shared memory partition
*
* This routine sets the debug options for a specified shared memory partition.
* Two kinds of errors are detected:  attempts to allocate more memory than
* is available, and bad blocks found when memory is freed.  In both cases,
* the following options can be selected for actions to be taken when the error
* is detected:  (1) return the error status, (2) log an error message and
* return the error status, or (3) log an error message and suspend the
* calling task.  These options are discussed in detail in the library manual
* entry for smMemLib.
*
* This routine is not user callable, it is called by the generic memory
* partition routine memPartOptionsSet().
*
* RETURNS: OK or ERROR.
*
* ERRNO:
*  S_objLib_OBJ_ID_ERROR
*  S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

STATUS smMemPartOptionsSet
    (
    SM_PART_ID volatile partId,    /* partition for which to set option */
    unsigned            options    /* memory management options */
    )
    {
    int                 tmp;       /* temp storage */

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->verify;           /* PCI bridge bug [SPR 68844] */

    if (SM_OBJ_VERIFY (partId) != OK)
        {
        return (ERROR);
        }

    if (smMemPartAccessGet(partId) != OK)
        {
        return (ERROR);
        }

    partId->options = htonl (options);

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->options;          /* BRIDGE FLUSH  [SPR 68334] */

    if (smMemPartAccessRelease (partId) != OK)
        {
        return (ERROR);
        }

    return (OK);
    }

/******************************************************************************
*
* smMemPartAlignedAlloc - allocate aligned memory from a partition
*
* This routine allocates a buffer of size nBytes from the given 
* shared partition.  Additionally, it will insure the allocated buffer 
* begins on a memory address that is evenly divisable by the given 
* alignment parameter.  The alignment parameter must be a power of 2.
*
* RETURNS:
* A pointer to the newly allocated block, or NULL if the buffer could not be
* allocated.
*
* NOMANUAL
*/

void * smMemPartAlignedAlloc 
    (
    SM_PART_ID volatile	partId,		/* memory partition to allocate from */
    unsigned		nBytes,		/* number of bytes to allocate */
    unsigned		alignment	/* boundry to align to */
    )
    {
    unsigned			nWords;
    unsigned			nWordsExtra;
    SM_DL_NODE volatile *	pNode;
    SM_BLOCK_HDR volatile *	pHdr;
    SM_BLOCK_HDR volatile *	pNewHdr;
    SM_BLOCK_HDR 		origpHdr;
    unsigned                    tmp;       /* temp storage */

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->verify;           /* PCI bridge bug [SPR 68844] */

    if (SM_OBJ_VERIFY (partId) != OK)
        {
	return (NULL);
        }

    /* get actual size to allocate; add overhead, check for minimum */

    nWords = (SM_MEM_ROUND_UP (nBytes) + sizeof (SM_BLOCK_HDR)) >> 1;

    tmp = partId->minBlockWords;    /* PCI bug       [SPR 68844] */
    if (nWords < ntohl (partId->minBlockWords))
        {
	nWords = ntohl (partId->minBlockWords);
        }

    /* get exclusive access to the partition */

    if (smMemPartAccessGet (partId) != OK)
        {
        return (NULL);
        }

    /* first fit */

    pNode = (SM_DL_NODE volatile *) SM_DL_FIRST (&partId->freeList);

    /*
     * We need a free block with extra room to do the alignment.  
     * Worst case we'll need alignment extra bytes.
     */

    nWordsExtra = nWords + alignment / 2;

    FOREVER
	{
	while (pNode != LOC_NULL)
	    {
	    /*
	     * fits if:
	     *	- blocksize > requested size + extra room for alignment or,
	     *	- block is already aligned and exactly the right size
	     */

	    if ((ntohl (SM_NODE_TO_HDR (pNode)->nWords) > nWordsExtra) ||
		((((UINT) SM_HDR_TO_BLOCK (SM_NODE_TO_HDR (pNode)) % alignment)
		  == 0) &&
		 (ntohl (SM_NODE_TO_HDR (pNode)->nWords) == nWords)))
                {
		break;
                }
	    pNode = (SM_DL_NODE volatile *) SM_DL_NEXT (pNode);
	    }

	if (pNode == LOC_NULL)
	    {
    	    if (smMemPartAccessRelease (partId) != OK)
    	        {
    	        return (NULL);
    	        }

	    smMemPartAllocError (partId, nBytes);

	    return (NULL);
	    }

	pHdr = (SM_BLOCK_HDR volatile *) SM_NODE_TO_HDR (pNode);
	origpHdr = *pHdr;

	/* 
	 * Now we split off from this block, the amount required by the user;
	 * note that the piece we are giving the user is at the end of the
	 * block so that the remaining piece is still the piece that is
	 * linked in the free list;  if smMemAlignedBlockSplit returned NULL,
	 * it couldn't split the block because the split would leave the
	 * first block too small to be hung on the free list, so we continue
	 * trying blocks.
	 */

	pNewHdr = smMemAlignedBlockSplit (partId, pHdr, nWords,
                                          ntohl (partId->minBlockWords),
                                          alignment);

	if (pNewHdr != NULL)
	    {
	    pHdr = pNewHdr;			/* give split off block */
	    break;
	    }
	
	pNode = (SM_DL_NODE volatile *) SM_DL_NEXT (pNode);
	}

    /* mark the block we're giving as not free  */

    pHdr->free = htonl (FALSE);

    /* update allocation statistics */

    partId->curBlocksAllocated = htonl (ntohl (partId->curBlocksAllocated) +1);
    partId->cumBlocksAllocated = htonl (ntohl (partId->cumBlocksAllocated) +1);
    partId->curWordsAllocated  = htonl (ntohl (partId->curWordsAllocated) + 
					ntohl (pHdr->nWords));
    partId->cumWordsAllocated  = htonl (ntohl (partId->cumWordsAllocated) + 
					ntohl (pHdr->nWords));

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->options;          /* BRIDGE FLUSH  [SPR 68334] */

    if (smMemPartAccessRelease (partId) != OK)
        {
        return (NULL);
        }

    return ((void *) SM_HDR_TO_BLOCK (pHdr));
    }

/******************************************************************************
*
* smMemPartAlloc - allocate a block of memory from a specified shared partition
*
* From a specified shared partition, this routine allocates a block of memory
* whose size is equal to or greater than <nBytes>.  The shared partition 
* must have been previously created with memPartSmCreate().
*
* This routine is not user callable, it is called by the generic memory
* partition routine memPartAlloc.
*
* RETURNS:
* A pointer to a block, or
* NULL if the call fails.
*
* ERRNO:
*  S_objLib_OBJ_ID_ERROR
*  S_memLib_NOT_ENOUGH_MEMORY
*  S_smObjLib_LOCK_TIMEOUT
*
* SEE ALSO: smMemPartInit()
*
* NOMANUAL
*/

void * smMemPartAlloc 
    (
    SM_PART_ID partId,     /* partition to allocate from */
    unsigned   nBytes      /* number of bytes to allocate */
    )
    {
    return (smMemPartAlignedAlloc (partId, nBytes, SM_ALIGN_BOUNDARY));
    }

/******************************************************************************
*
* smMemPartRealloc - reallocate a block of memory in specified shared partition
*
* This routine changes the size of a specified block and returns a pointer to
* the new block of memory.  The contents that fit inside the new size (or old
* size if smaller) remain unchanged.
*
* This routine is not user callable, it is called by the generic memory
* partition routine memPartRealloc.
*
* RETURNS:
* A pointer to the new block of memory, or NULL if the call fails.
*
* ERRNO:
*  S_objLib_OBJ_ID_ERROR
*  S_memLib_NOT_ENOUGH_MEMORY
*  S_memLib_BLOCK_ERROR
*  S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

void * smMemPartRealloc 
    (
    SM_PART_ID volatile partId,	/* partition to reallocate from */
    char *              pBlock,	/* block to be reallocated */
    unsigned            nBytes	/* new block size in bytes */
    )
    {
    SM_BLOCK_HDR volatile * pHdr = SM_BLOCK_TO_HDR (pBlock);
    SM_BLOCK_HDR volatile * pNextHdr;
    unsigned                nWords;
    void *                  pNewBlock;
    int 	            partOptions;	/* partition options */
    unsigned                tmp;                /* temp storage */

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->verify;           /* PCI bridge bug [SPR 68844] */

    if (SM_OBJ_VERIFY (partId) != OK)
        {
        return (NULL);
        }

    /* get exclusive access to the partition */

    if (smMemPartAccessGet (partId) != OK)
        {
        return (NULL);
        }

    /* get actual new size; round-up, add overhead, check for minimum */

    nWords = (SM_MEM_ROUND_UP (nBytes) + sizeof (SM_BLOCK_HDR)) >> 1;
    tmp = partId->minBlockWords;    /* PCI bug       [SPR 68844] */
    if (nWords < ntohl (partId->minBlockWords))
        {
	nWords = ntohl (partId->minBlockWords);
        }

    /* optional check for validity of block */

    partOptions = ntohl (partId->options);

    if ((partOptions & MEM_BLOCK_CHECK) &&
        !smMemPartBlockIsValid (partId, pHdr, FALSE))
        {
        smMemPartAccessRelease (partId); 	/* release access */
        smMemPartBlockError (partId, pBlock, "smMemPartRealloc");

        return (NULL);
        }

    /* test if we are trying to increase size of block */

    tmp = pHdr->nWords;             /* PCI bug       [SPR 68844] */
    if (nWords > ntohl (pHdr->nWords))
	{
	/*
	 * increase size of block -
	 * check if next block is free and is big enough to satisfy request
	 */

	pNextHdr = (SM_BLOCK_HDR volatile *) SM_NEXT_HDR (pHdr);

	if (!(ntohl (pNextHdr->free)) || 
	    ((ntohl (pHdr->nWords) + ntohl (pNextHdr->nWords)) < nWords))
	    {
	    /*
	     * can't use adjacent free block -
	     * allocate an entirely new block and copy data
	     */

            if (smMemPartAccessRelease (partId) != OK)
                {
                return (NULL);
                }

	    if ((pNewBlock = smMemPartAlloc (partId, nBytes)) == NULL)
	        {
		return (NULL);
	        }

            CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
            tmp = pHdr->nWords;             /* PCI bug       [SPR 68844] */

	    bcopy (pBlock, (char *) pNewBlock,
		   (int) (2 * ntohl (pHdr->nWords) - sizeof (SM_BLOCK_HDR)));

            CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
            tmp = pHdr->nWords;             /* BRIDGE FLUSH  [SPR 68334] */

	    (void) smMemPartFree (partId, pBlock);

	    return (pNewBlock);		/* RETURN, don't fall through */
	    }
	else
	    {
	    /* 
	     * append next block to this one -
	     *  - delete next block from freelist,
	     *  - add its size to this block,
	     *  - update allocation statistics,
	     *  - fix prev info in new "next" block header 
	     */

	    smDllRemove (&partId->freeList, SM_HDR_TO_NODE (pNextHdr));

            /* fix size */

            CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
            tmp = pHdr->nWords;             /* PCI bug       [SPR 68844] */

	    pHdr->nWords = htonl (ntohl (pHdr->nWords) + 
				  ntohl (pNextHdr->nWords));

            /* fix stats */

	    partId->curWordsAllocated = htonl (ntohl (partId->curWordsAllocated)
					       + ntohl (pNextHdr->nWords));
	    partId->cumWordsAllocated = htonl (ntohl (partId->cumWordsAllocated)
					       + ntohl (pNextHdr->nWords));

								/* fix next */
	    SM_NEXT_HDR (pHdr)->pPrevHdr = (SM_BLOCK_HDR *) 
					    htonl (LOC_TO_GLOB_ADRS (pHdr));

            CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
            tmp = pHdr->nWords;             /* BRIDGE FLUSH  [SPR 68334] */

	    /* if this combined block is too big, it will get fixed below */
	    }
	}

    /* 
     * split off any extra and give it back;
     * note that this covers both the case of a realloc for smaller space
     * and the case of a realloc for bigger space that caused a coalesce
     * with the next block that resulted in larger than required block 
     */

    pNextHdr = smMemBlockSplit (pHdr, ntohl (pHdr->nWords) - nWords,
                                ntohl (partId->minBlockWords));

    if (smMemPartAccessRelease (partId) != OK)
        {
        return (NULL);
        }

    if (pNextHdr != NULL)
	{
	(void) smMemPartFree (partId, SM_HDR_TO_BLOCK (pNextHdr));

        /* adjust statistics */

        partId->curBlocksAllocated = htonl(ntohl(partId->curBlocksAllocated)+1);

        CACHE_PIPE_FLUSH ();                /* CACHE FLUSH   [SPR 68334] */
        tmp = pHdr->nWords;                 /* BRIDGE FLUSH  [SPR 68334] */
	}

    return ((void *) pBlock);
    }

/******************************************************************************
*
* smMemPartFree - free a block of memory in a specified shared partition
*
* This routine takes a block of memory previously allocated with
* smMemPartAlloc() and returns it to the shared partition's free memory list.
*
* This routine is not user callable, it is called by the generic memory
* partition routine memPartFree().
*
* RETURNS: OK or ERROR if there is an invalid block.
*
* ERRNO:
*  S_objLib_OBJ_ID_ERROR
*  S_memLib_BLOCK_ERROR
*  S_smObjLib_LOCK_TIMEOUT:
*
* NOMANUAL
*/

STATUS smMemPartFree 
    (
    SM_PART_ID volatile partId,	/* partition to use */
    char *              pBlock	/* pointer to block of memory to be freed */
    )
    {
    SM_BLOCK_HDR volatile * pHdr;
    SM_BLOCK_HDR volatile * pNextHdr;
    unsigned                nWords;
    int                     partOptions;	/* partition options */
    unsigned                tmp;                /* temp storage */

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->verify;           /* PCI bridge bug [SPR 68844] */

    if (SM_OBJ_VERIFY (partId) != OK)
        {
        return (ERROR);
        }

    if (pBlock == NULL)
        {
	return (OK);				/* ANSI C compatibility */
        }

    pHdr   = (SM_BLOCK_HDR volatile *) SM_BLOCK_TO_HDR (pBlock);
    nWords = ntohl (pHdr->nWords);

    /* get exclusive access to the partition */

    if (smMemPartAccessGet (partId) != OK)
        {
        return (ERROR);
        }

    /* optional check for validity of block */

    tmp = partId->options;          /* PCI bug       [SPR 68844] */
    partOptions = ntohl (partId->options);

    if ((partOptions & MEM_BLOCK_CHECK) &&
        !smMemPartBlockIsValid (partId, pHdr, FALSE))
        {
        smMemPartAccessRelease (partId); 	/* release access */
        smMemPartBlockError (partId, pBlock, "smMemPartFree");

        return (ERROR);
        }
    /* 
     * Check if we can coalesce with previous block;
     * if so, then we just extend the previous block,
     * otherwise we have to add this as a new free block.
     */

    if (ntohl (SM_PREV_HDR (pHdr)->free))
	{
	pHdr->free = htonl (FALSE); 		/* this isn't a free block */
	pHdr = SM_PREV_HDR (pHdr);		/* coalesce with prev block */
        CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
        tmp = pHdr->nWords;             /* PCI bug       [SPR 68844] */
	pHdr->nWords = htonl (ntohl (pHdr->nWords) + nWords);
	}
    else
	{
	pHdr->free = htonl (TRUE);		/* add new free block */
	smDllInsert (&partId->freeList, (SM_DL_NODE *) NULL, 
		     SM_HDR_TO_NODE (pHdr));
	}

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = pHdr->nWords;             /* BRIDGE FLUSH  [SPR 68334] */

    /* 
     * Check if we can coalesce with next block;
     * if so, then we can extend our block delete next block from free list 
     */

    pNextHdr = (SM_BLOCK_HDR volatile *) SM_NEXT_HDR (pHdr);
    if (ntohl (pNextHdr->free))
	{
        /* coalesce with next */

	pHdr->nWords = htonl (ntohl (pHdr->nWords) + ntohl (pNextHdr->nWords));
	smDllRemove (&partId->freeList, SM_HDR_TO_NODE (pNextHdr));
	}

    /* fix up prev info of whatever block is now next */

    SM_NEXT_HDR (pHdr)->pPrevHdr = (SM_BLOCK_HDR *) 
					htonl (LOC_TO_GLOB_ADRS (pHdr));

    /* adjust allocation stats */

    partId->curBlocksAllocated = htonl (ntohl (partId->curBlocksAllocated) - 1);
    partId->curWordsAllocated  = htonl (ntohl (partId->curWordsAllocated) - 
					nWords);

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = pHdr->nWords;             /* BRIDGE FLUSH  [SPR 68334] */

    if (smMemPartAccessRelease (partId) != OK)
        {
        return (ERROR);
        }

    return (OK);
    }

/******************************************************************************
*
* smMemPartFindMax - find the size of the largest available free block
*
* This routine searches for the largest block in a shared memory partition free
* list, and returns its size.
*
* This routine is not user callable, it is called by the generic memory
* partition routine memPartFindMax().
*
* RETURNS: The size (in bytes) of the largest available block or ERROR if
* <partId> is not a valid partition or if attempt to get access to the 
* partition fails.
*
* ERRNO:
*  S_objLib_OBJ_ID_ERROR
*  S_smObjLib_LOCK_TIMEOUT
*
* NOMANUAL
*/

int smMemPartFindMax 
    (
    SM_PART_ID volatile partId     /* partition to use */
    )
    {
    SM_BLOCK_HDR volatile *    pHdr;
    SM_DL_NODE volatile *      pNode;
    unsigned                   biggestWords = 0;
    unsigned                   tmp;                /* temp storage */

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->verify;           /* PCI bridge bug [SPR 68844] */

    if (SM_OBJ_VERIFY (partId) != OK)
        {
        return (ERROR);
        }

    if (smMemPartAccessGet (partId) != OK)
        {
        return (ERROR);
        }

    /* go through free list and find largest free */

    for (pNode = (SM_DL_NODE volatile *) SM_DL_FIRST (&partId->freeList);
	 pNode != LOC_NULL;
	 pNode = (SM_DL_NODE volatile *) SM_DL_NEXT (pNode))
	{
	pHdr = (SM_BLOCK_HDR volatile *) SM_NODE_TO_HDR (pNode);
	if (ntohl (pHdr->nWords) > biggestWords)
	    {
	    biggestWords = ntohl (pHdr->nWords);
	    }
	}

    if (smMemPartAccessRelease (partId) != OK)
        {
        return (ERROR);
        }

    /* convert words to bytes only if non-zero value [SPR 70280] */

    if (biggestWords > 0)
        {
        biggestWords *= 2;
        biggestWords -= sizeof (SM_BLOCK_HDR);
        }

    return (biggestWords);
    }

/******************************************************************************
*
* smMemAlignedBlockSplit - split a block on the free list into two blocks
*
* This routine is like smMemBlockSplit(), but also aligns the data block to
* the given alignment.  The block looks like this after it is split:
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
* block on the free list.  If the space succeeding the newly allocated
* aligned buffer is less than the minimum block size, then the bytes are
* added to the newly allocated buffer.  If the space is greater than the
* minimum block size,  then a new memory fragment is created and added to
* the free list.  Care must be taken to insure that the orignal block passed
* in to be split has at least (nWords + alignment/2) words in it.  If the
* block has exactly nWords and is already aligned to the given alignment, it
* will be deleted from the free list and returned unsplit.  Otherwise, the
* second block will be returned.
*
* RETURNS: A pointer to a BLOCK_HDR 
*
* NOMANUAL
*/

LOCAL SM_BLOCK_HDR volatile * smMemAlignedBlockSplit 
    (
    SM_PART_ID volatile     partId,
    SM_BLOCK_HDR volatile * pHdr,
    unsigned                nWords,   /* number of words in second block */
    unsigned                minWords, /* min num of words allowed in a block */
    unsigned                alignment /* boundry to align to */
    )
    {
    SM_BLOCK_HDR volatile * pNewHdr;
    SM_BLOCK_HDR volatile * pNextHdr;
    char *		    endOfBlock;
    char *		    pNewBlock;
    int 		    blockSize;
    unsigned                   tmp;                /* temp storage */

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = pHdr->nWords;             /* PCI bridge bug [SPR 68844] */

    /* calculate end of pHdr block */

    endOfBlock = (char *) pHdr + (ntohl (pHdr->nWords) * 2); 

    /* calculate unaligned beginning of new block */ 

    pNewBlock = (char *) ((unsigned) endOfBlock - 
		((nWords - sizeof (SM_BLOCK_HDR) / 2) * 2));

    /* align the beginning of the block */

    pNewBlock = (char *)((unsigned) pNewBlock & ~(alignment - 1));

    pNewHdr = (SM_BLOCK_HDR volatile *) SM_BLOCK_TO_HDR (pNewBlock);

    /* adjust original block's word count */

    blockSize = ((char *) pNewHdr - (char *) pHdr) / 2;

    if (blockSize < minWords)
	{
	/* 
	 * Check to see if the new block is the same as the original block -
	 * if so, delete if from the free list.  If not, reject the newly
	 * split block because it's too small to hang on the free list.
	 */

	if (pNewHdr == pHdr)
	    {
	    smDllRemove (&partId->freeList, SM_HDR_TO_NODE (pHdr));
	    }
	else
	    {
	    return (NULL);
	    }
	}
    else
	{
	pNewHdr->pPrevHdr = (SM_BLOCK_HDR *) htonl (LOC_TO_GLOB_ADRS (pHdr));
	pHdr->nWords = htonl (blockSize);
	}

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = pNewHdr->nWords;          /* BRIDGE FLUSH  [SPR 68844] */

    /* 
     * Check to see if space left over after we aligned the new buffer
     * is big enough to be a fragment on the free list.
     */

    if (((UINT) endOfBlock - (UINT) pNewHdr - (nWords * 2)) < (minWords * 2))
	{
	/* nope - give all the memory to the newly allocated block */

	pNewHdr->nWords = htonl ((endOfBlock - pNewBlock + 
				 sizeof (SM_BLOCK_HDR)) / 2);
	pNewHdr->free   = htonl (TRUE); 

	/* fix next block to point to newly allocated block */

	SM_NEXT_HDR (pNewHdr)->pPrevHdr = (SM_BLOCK_HDR *) 
					   htonl (LOC_TO_GLOB_ADRS (pNewHdr));
	}
    else
	{
	/* 
	 * The extra bytes are big enough to be a fragment on the free list -
	 * first, fix up the newly allocated block.
	 */

	pNewHdr->nWords = htonl (nWords);
	pNewHdr->free   = htonl (TRUE); 

	/* split off the memory after pNewHdr and add it to the free list */

	pNextHdr = SM_NEXT_HDR (pHdr);

	pNextHdr->nWords   = htonl (((UINT) endOfBlock - (UINT) pNextHdr) / 2);
	pNextHdr->pPrevHdr = (SM_BLOCK_HDR *) htonl (LOC_TO_GLOB_ADRS (pHdr)); 
	pNextHdr->free     = htonl (TRUE);

	smDllAdd (&partId->freeList, SM_HDR_TO_NODE (pNextHdr));	

	/* fix next block to point to the new fragment on the free list */

	SM_NEXT_HDR (pNextHdr)->pPrevHdr = (SM_BLOCK_HDR *) 
					    htonl (LOC_TO_GLOB_ADRS (pNewHdr)); 
	}

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = pNewHdr->nWords;          /* BRIDGE FLUSH  [SPR 68334] */

    return (pNewHdr);
    }

/******************************************************************************
*
* smMemBlockSplit - split a block into two blocks
*
* This routine splits the block pointed to into two blocks.  The second 
* block will have nWords words in it.  A pointer is returned to this block.
* If either resultant block would end up having less than minWords in it,
* NULL is returned.
*
* RETURNS: A pointer to the second block, or NULL.
* 
* NOMANUAL
*/

LOCAL SM_BLOCK_HDR volatile * smMemBlockSplit 
    (
    SM_BLOCK_HDR volatile * pHdr,
    unsigned                nWords,  /* number of words in second block */
    unsigned                minWords /* min num of words allowed in a block */
    )
    {
    unsigned                wordsLeft;
    SM_BLOCK_HDR volatile * pNewHdr;
    unsigned                tmp;                /* temp storage */

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = pHdr->nWords;             /* PCI bug       [SPR 68844] */

    /* check if block can be split */

    if ((nWords < minWords) ||
        ((wordsLeft = (ntohl (pHdr->nWords) - nWords)) < minWords))
        {
        return (NULL);                  /* not enough space left */
        }

    /* adjust original block size and create new block */

    pHdr->nWords = htonl (wordsLeft);

    pNewHdr = (SM_BLOCK_HDR volatile *) SM_NEXT_HDR (pHdr);

    pNewHdr->pPrevHdr = (SM_BLOCK_HDR *) htonl (LOC_TO_GLOB_ADRS (pHdr));
    pNewHdr->nWords   = htonl (nWords);
    pNewHdr->free     = pHdr->free;

    /* fix next block */

    SM_NEXT_HDR (pNewHdr)->pPrevHdr = (SM_BLOCK_HDR *) 
					htonl (LOC_TO_GLOB_ADRS (pNewHdr));

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = pNewHdr->nWords;          /* BRIDGE FLUSH  [SPR 68334] */

    return (pNewHdr);
    }

/******************************************************************************
*
* smMemAddToPool - add memory to shared memory system partition (VxMP Option)
*
* This routine adds memory to the shared memory system partition after the
* initial allocation of memory.  The memory added need not be contiguous
* with memory previously assigned, but it must be in the same address
* space.
*
* <pPool> is the global address of shared memory added to the partition.
* The memory area pointed to by <pPool> must be in the same address space
* as the shared memory anchor and shared memory pool.
*
* <poolSize> is the size in bytes of shared memory added to the partition.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: OK, or ERROR if access to the shared memory system partition fails.
*
* ERRNO:
*  S_smObjLib_LOCK_TIMEOUT
*/

STATUS smMemAddToPool 
    (
    char *	pPool,		/* pointer to memory pool */
    unsigned	poolSize	/* block size in bytes */
    )
    {
    return (smMemPartAddToPool ((SM_PART_ID) smSystemPartId, pPool, poolSize));
    }

/******************************************************************************
*
* smMemOptionsSet - set debug options for shared memory system partition (VxMP Option)
*
* This routine sets the debug options for the shared system memory partition.
* Two kinds of errors are detected:  attempts to allocate more memory than
* is available, and bad blocks found when memory is freed or reallocated.  
* In both cases, the following options can be selected for actions to be 
* taken when an error is detected:  (1) return the error status, (2) log 
* an error message and return the error status, or (3) log an error message 
* and suspend the calling task.  These options are discussed in detail in 
* the library manual entry for smMemLib.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: OK or ERROR.
*
* ERRNO
*  S_smObjLib_LOCK_TIMEOUT
*/

STATUS smMemOptionsSet
    (
    unsigned options    /* options for system partition */
    )
    {
    return (smMemPartOptionsSet ((SM_PART_ID) smSystemPartId, options));
    }

/******************************************************************************
*
* smMemMalloc - allocate block of memory from shared memory system partition (VxMP Option)
*
* This routine allocates a block of memory from the shared memory 
* system partition whose size is equal to or greater than <nBytes>.
* The return value is the local address of the allocated shared memory block.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS:
* A pointer to the block, or NULL if the memory cannot be allocated.
*
* ERRNO:
*  S_memLib_NOT_ENOUGH_MEMORY
*  S_smObjLib_LOCK_TIMEOUT
*/

void * smMemMalloc 
    (
    unsigned nBytes             /* number of bytes to allocate */
    )
    {
    return (smMemPartAlloc ((SM_PART_ID) smSystemPartId, (unsigned) nBytes));
    }

/******************************************************************************
*
* smMemCalloc - allocate memory for array from shared memory system partition (VxMP Option)
*
* This routine allocates a block of memory for an array that contains
* <elemNum> elements of size <elemSize> from the shared memory system 
* partition.
* The return value is the local address of the allocated shared memory block.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS:
* A pointer to the block, or NULL if the memory cannot be allocated.
*
* ERRNO:
*  S_memLib_NOT_ENOUGH_MEMORY
*  S_smObjLib_LOCK_TIMEOUT
*/

void * smMemCalloc 
    (
    int elemNum,	/* number of elements */
    int elemSize	/* size of elements */
    )
    {
    void * pMem;
    size_t nBytes = elemNum * elemSize;
    
    if ((pMem = smMemPartAlloc ((SM_PART_ID) smSystemPartId,
                                (unsigned) nBytes))
             != NULL)
        {
	bzero ((char *) pMem, (int) nBytes);
        }

    return (pMem);
    }

/******************************************************************************
*
* smMemRealloc - reallocate block of memory from shared memory system partition (VxMP Option)
*
* This routine changes the size of a specified block and returns a pointer to
* the new block of shared memory.  The contents that fit inside the new 
* size (or old size, if smaller) remain unchanged.
* The return value is the local address of the reallocated shared memory block.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS:
* A pointer to the new block of memory, or NULL if the reallocation cannot
* be completed.
*
* ERRNO:
*  S_memLib_NOT_ENOUGH_MEMORY
*  S_memLib_BLOCK_ERROR
*  S_smObjLib_LOCK_TIMEOUT
*/

void * smMemRealloc 
    (
    void *	pBlock,		/* block to be reallocated */
    unsigned	newSize		/* new block size */
    )
    {
    return (smMemPartRealloc ((SM_PART_ID) smSystemPartId, (char *) pBlock,
	    (unsigned) newSize));
    }

/******************************************************************************
*
* smMemFree - free a shared memory system partition block of memory (VxMP Option)
*
* This routine takes a block of memory previously allocated with smMemMalloc() 
* or smMemCalloc() and returns it to the free shared memory system pool.
*
* It is an error to free a block of memory that was not previously allocated.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: OK, or ERROR if the block is invalid.
*
* ERRNO:
*  S_memLib_BLOCK_ERROR
*  S_smObjLib_LOCK_TIMEOUT
*
* SEE ALSO: smMemMalloc(), smMemCalloc()
*/

STATUS smMemFree 
    (
    void * ptr       /* pointer to block of memory to be freed */
    )
    {
    return (smMemPartFree ((SM_PART_ID) smSystemPartId, (char *) ptr));
    }

/******************************************************************************
*
* smMemFindMax - find largest free block in shared memory system partition (VxMP Option)
*
* This routine searches for the largest block in the shared memory 
* system partition free list and returns its size.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: The size (in bytes) of the largest available block, or ERROR if
* the attempt to access the partition fails.
*
* ERRNO:
*  S_smObjLib_LOCK_TIMEOUT
*/

int smMemFindMax (void)
    {
    return (smMemPartFindMax ((SM_PART_ID) smSystemPartId));
    }

/******************************************************************************
*
* smMemPartAllocError - handle allocation error
*
* This routine handles an allocation error according to the options set for
* the specified partition.  <pPart> is the local address of partition to use.
*
* RETURNS: N/A
*/

LOCAL void smMemPartAllocError
    (
    SM_PART_ID volatile pPart,
    unsigned            nBytes
    )
    {
    int		taskOptions;
    int		partOptions;	/* partition options */
    unsigned    tmp;            /* temp storage */

    errno = S_memLib_NOT_ENOUGH_MEMORY;

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = pPart->options;           /* PCI bridge bug [SPR 68844] */

    partOptions = ntohl (pPart->options);

    if (partOptions & MEM_ALLOC_ERROR_LOG_FLAG)
        {
        logMsg (smMemMsgBlockTooBig, nBytes, (int) SM_OBJ_ADRS_TO_ID (pPart), 
                0, 0, 0, 0);
        }

    if (partOptions & MEM_ALLOC_ERROR_SUSPEND_FLAG)
        {
        taskOptionsGet (0, &taskOptions);

        if ((taskOptions & VX_UNBREAKABLE) == 0)
            {
            taskSuspend (0);
            }
        }
    }

/******************************************************************************
*
* smMemPartBlockError - handle invalid block error
*
* This routine handles an invalid block error according to the options set for
* the specified partition.  <pPart> is the local address of partition to use.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void smMemPartBlockError
    (
    SM_PART_ID volatile pPart,
    char *              pBlock,
    char *              label
    )
    {
    int		taskOptions;
    int		partOptions;	/* partition options */
    unsigned    tmp;            /* temp storage */

    errno = S_memLib_BLOCK_ERROR;

    /*
     * if ...LOG_FLAG is set, send a log message
     */

    CACHE_PIPE_FLUSH ();            /* CACHE FLUSH   [SPR 68334] */
    tmp = pPart->options;           /* PCI bridge bug [SPR 68844] */

    partOptions = ntohl (pPart->options);

    if (partOptions & MEM_BLOCK_ERROR_LOG_FLAG)
        {
        logMsg (smMemMsgBlockError, (int) label, (int) pBlock, 
                (int) SM_OBJ_ADRS_TO_ID (pPart), 0, 0, 0);
        }

    /*
     * If ...SUSPEND_FLAG is set, suspend the task
     */

    if (partOptions & MEM_BLOCK_ERROR_SUSPEND_FLAG)  
        {
        taskOptionsGet (0, &taskOptions);

        if ((taskOptions & VX_UNBREAKABLE) == 0)
            {
            taskSuspend (0);
            }
        }
    }

/******************************************************************************
*
* smMemPartBlockIsValid - check validity of block
*
* NOTE: This routine should NOT be called with the partition semaphore taken
* if possible, because if the block IS screwed up, then a bus error is
* not unlikely and we would hate to have the task die with the semaphore
* taken, thus hanging the partition.
*
* <pPart> is the local address of partition to use.
*
* NOMANUAL
*/

BOOL smMemPartBlockIsValid
    (
    SM_PART_ID volatile     partId,
    SM_BLOCK_HDR volatile * pHdr,
    BOOL                    isFree       /* expected status */
    )
    {
    BOOL valid;

    valid = SM_MEM_IS_ROUND (pHdr)			/* aligned */
							/* size is round */
            && SM_MEM_IS_ROUND (2 * ntohl (pHdr->nWords))
							/* size <= total */
            && (ntohl (pHdr->nWords) <= ntohl (partId->totalWords))	
            && (ntohl (pHdr->free) == isFree)		/* right alloc-ness */
            && (pHdr == SM_PREV_HDR(SM_NEXT_HDR (pHdr)))/* matches next block*/
            && (pHdr ==SM_NEXT_HDR(SM_PREV_HDR (pHdr)));/* matches prev block*/

    return (valid);
    }

/******************************************************************************
*
* smMemPartAccessGet - obtain exclusive access to a partition
*
* <pPart> is the local address of partition to use. <pOldLvl> is a pointer 
* to the processor status register value.
*
* RETURNS: OK, or ERROR if access cannot be obtained
*
* NOMANUAL
*/

STATUS smMemPartAccessGet 
    (
    SM_PART_ID volatile partId  	/* partition to access */
    )
    {
    TASK_SAFE ();			/* prevent task deletion */

    if (semSmTake (&partId->sem, WAIT_FOREVER) != OK)
	{                               /* get exclusive access */
	TASK_UNSAFE ();

	return (ERROR);
	}

    return (OK);
    }

/******************************************************************************
*
* smMemPartAccessRelease - release exclusive access to a partition
*
* <pPart> is the local address of partition to use. 
*
* RETURNS: OK, or ERROR if access cannot be released
*
* NOMANUAL
*/

STATUS smMemPartAccessRelease 
    (
    SM_PART_ID volatile partId      		/* partition to release */
    )
    {
    if (semSmGive (&partId->sem) != OK)		/* give back partition sem */
        {              
	TASK_UNSAFE ();				/* allow task deletion */

	return (ERROR);
	}

    TASK_UNSAFE ();				/* allow task deletion */

    return (OK);
    }


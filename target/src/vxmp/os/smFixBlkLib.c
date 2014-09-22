/* smFixBlkLib.c - VxWorks fixed block shared memory manager (VxMP Option) */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,03may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
01f,24oct01,mas  doc update (SPR 71149)
01e,20sep01,jws  correct partition spin-lock tries calc (SPR68418)
01d,29jan93,pme  added little endian support.
                 added #include "private/smObjLibP.h".
		 changed MEM_ROUND_UP() to ROUND_UP() in smFixBlkPartInit().
01c,02oct92,pme  added SPARC support.
01b,27jul92,pme  added lock timeout failure notification.
01a,19jul92,pme  written.
*/

/*
DESCRIPTION
This library provides a simple facility for managing the 
allocation of fixed size blocks of shared memory from ranges 
of memory called fixed block size shared memory partitions.

There are no user callable routines.

A shared memory fixed block partition is created when the shared memory 
object facility (VxMP) is installed during system startup by VxWorks
initialization code calling smObjSetup().

This memory allocator is only used to allocate shared TCBs in shared memory
and is provided to give a lower interrupt lattency than the 
standard variable block size shared memory manager.

The allocation of a block is done via smFixBlkPartAlloc() and freeing a block
is done via smFixBlkPartFree().

AVAILABILITY
This module is distributed as a component of the unbundled shared memory
support option, VxMP.

INTERNAL
The fixed block memory partition header is stored in the shared memory header
reachable by all CPUs in the system.

There is no overhead per allocated block since blocks have no headers. The
first 8 bytes of each block are used as doubly linked list nodes when 
blocks are in the free list but they are part of the block when it is
allocated to user.

No validity checking is done on blocks passed to smFixBlkPartFree().

INCLUDE FILE: smFixBlkLib.h

SEE ALSO: smObjLib

NOROUTINES
*/

/* includes */

#include "vxWorks.h"
#include "memLib.h"
#include "smObjLib.h"
#include "smLib.h"
#include "intLib.h"
#include "smMemLib.h"
#include "string.h"
#include "taskLib.h"
#include "cacheLib.h"
#include "errno.h"
#include "netinet/in.h"
#include "private/smObjLibP.h"
#include "private/smFixBlkLibP.h"

/* defines */
				
/* locals */

/* number of tries to get access to a shared fixed block memory partition */

LOCAL int smFixBlkSpinTries;

/******************************************************************************
*
* smFixBlkPartInit - initialize a fixed block shared memory partition
*
* smFixBlkPartInit initializes a shared fixed block memory partition.
* This function is used in smObjInit to initialize the shared TCB
* partition.
* 
* <gPartId> is the global (system wide) identifier of the partition to 
* initialize. Its value is the global address of the partition structure. 
*
* <pPool> is the global address of shared memory dedicated to 
* the fixed block partition.
*
* <poolSize> is the size in bytes of shared memory dedicated to the partition.
*
* <blockSize> is the size in bytes of each block in the partition.
*
* NOMANUAL
*/

void smFixBlkPartInit 
    (
    SM_FIX_BLK_PART_ID gPartId,   /* global id of partition to initialize */
    char *             pPool,     /* global adress of memory pool */
    unsigned           poolSize,  /* pool size in bytes */
    unsigned           blockSize  /* block size in bytes */
    )
    {
    SM_FIX_BLK_PART_ID volatile partId; /* local addr of partition to init */
    int                         memLeft;
    char *                      tmp;
    int                         numBlock = 0;
    int                         temp;   /* temp storage */

    partId = (SM_FIX_BLK_PART_ID volatile) GLOB_TO_LOC_ADRS (gPartId);

    /* initialize partition descriptor */

    bzero ((char *) partId, sizeof (SM_FIX_BLK_PART));

    partId->blockSize = htonl (blockSize);

    /* insure that the pool starts on an even byte boundary */

    tmp       = (char *) ROUND_UP (pPool, 4);	/* get actual start */
    poolSize -= tmp - pPool;			/* adjust length */
    pPool     = (char *) GLOB_TO_LOC_ADRS (tmp);/* convert to local address */

    memLeft = poolSize;

    /* initialize free list */

    smDllInit (&partId->freeList);

    /* Insert blocks in the free list */

    while (memLeft >= blockSize)
	{
        smDllAdd (&partId->freeList, (SM_DL_NODE *) pPool);
	numBlock ++;
	pPool += blockSize;
	memLeft -= blockSize;
	}

    /*
     * initialize number of tries to get partition spin-lock
     * smObjSpinTries has been set to config param SM_OBJ_MAX_TRIES
     * before the call to this routine
     */

    smFixBlkSpinTries = smObjSpinTries;

    partId->totalBlocks = htonl (numBlock);

    partId->objType = htonl (MEM_PART_TYPE_SM_FIX);

    partId->verify = (UINT32) htonl (LOC_TO_GLOB_ADRS (partId));

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = partId->verify;                      /* BRIDGE FLUSH  [SPR 68334] */
    }
 
/******************************************************************************
*
* smFixBlkPartAlloc - allocate a block of memory from fixed block partition
*
* From a specified fixed block shared partition, this routine 
* allocates one block of memory whose size is equal to the partition blockSize.
* The fixed block partition must have been previously initialized 
* with smFixBlkPartInit().
*
* <gPartId> is the global (system wide) identifier of the partition to 
* use. Its value is the global address of the partition structure. 
*
* RETURNS:
* A local adress of a block, or
* NULL if the call fails.
*
* ERRNO:
*
*   S_objLib_OBJ_ID_ERROR
*
*   S_memLib_NOT_ENOUGH_MEMORY
*
*   S_smObjLib_LOCK_TIMEOUT
*
* SEE ALSO: smFixBlkPartInit()
*
* NOMANUAL
*/

void * smFixBlkPartAlloc 
    (
    SM_FIX_BLK_PART_ID gPartId /* global id of partition to allocate from */
    )
    {
    SM_FIX_BLK_PART_ID volatile partId; /* local address of partition */
    void *			pBlock; /* local addr of allocated block */
    int                         level;
    int                         temp;   /* temp storage */

    partId = (SM_FIX_BLK_PART_ID volatile) GLOB_TO_LOC_ADRS (gPartId);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = partId->verify;                      /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (partId) != OK)
        return (NULL);

    /* ENTER LOCKED SECTION */	

    if (smLockTake ((int *) &partId->lock, smObjTasRoutine, 
		   smFixBlkSpinTries, &level) != OK)
	{
	smObjTimeoutLogMsg ("smFixBlkPartAlloc", (char *) &partId->lock);
	return (NULL);				/* can't take lock */
	}

    pBlock = (void *) smDllGet (&partId->freeList); /* get a free block */

    if (pBlock == LOC_NULL)			/* no more free blocks */
	{
        /* EXIT LOCKED SECTION */

        SM_OBJ_LOCK_GIVE ((SM_SEM_ID) &partId->lock, level);
	errno = S_memLib_NOT_ENOUGH_MEMORY;
	return (NULL);
	}

    /* update allocation statistics */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = partId->curBlocksAllocated;          /* PCI bridge bug [SPR 68844]*/

    partId->curBlocksAllocated = htonl (ntohl (partId->curBlocksAllocated) +1);
    partId->cumBlocksAllocated = htonl (ntohl (partId->cumBlocksAllocated) +1);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = partId->curBlocksAllocated;          /* BRIDGE FLUSH  [SPR 68334] */

    /* EXIT LOCKED SECTION */

    SM_OBJ_LOCK_GIVE ((SM_SEM_ID) &partId->lock, level);

    return ((void *) pBlock);
    }

/******************************************************************************
*
* smFixBlkPartFree - give back a block to a fixed block shared partition
*
* This routine takes a block of memory previously allocated with
* smFixBlkPartAlloc() and returns it to the shared partition's free memory list.
*
* <gPartId> is the global (system wide) identifier of the partition to 
* use. Its value is the global address of the partition structure. 
*
* <pBlock> is a local address of a shared memory block previously allocated
* by smFixBlkPartAlloc.
*
* WARNING: No validy check is done on <pBlock>.
*
* RETURNS: OK or ERROR if partition id is invalid.
*
* ERRNO:
*
*   S_objLib_OBJ_ID_ERROR:
*
*   S_smObjLib_LOCK_TIMEOUT:
*
* NOMANUAL
*/

STATUS smFixBlkPartFree 
    (
    SM_FIX_BLK_PART_ID gPartId, /* global id of partition to use */
    char *             pBlock   /* pointer to block of memory to be freed */
    )
    {
    SM_FIX_BLK_PART_ID volatile partId; /* local address of partition to use */
    int                         level;
    int                         temp;   /* temp storage */

    partId = (SM_FIX_BLK_PART_ID volatile) GLOB_TO_LOC_ADRS (gPartId);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = partId->verify;                      /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (partId) != OK)
        return (ERROR);

    if (pBlock == NULL)
	return (OK);				/* ANSI C compatibility */

    /* 
     * If attempt to take lock fails here we only return ERROR,
     * we do not notify the failure since this function is only called
     * in taskDelete which will send a WARNING.
     */
    						/* ENTER LOCKED SECTION */	
    if (smLockTake ((int *) &partId->lock, smObjTasRoutine,
                    smFixBlkSpinTries, &level) != OK)
	return (ERROR);				/* can't take lock */

    /* add the block to the free list */

    smDllInsert ((SM_DL_LIST *) &partId->freeList, (SM_DL_NODE *) NULL,
		 (SM_DL_NODE *) (pBlock));

    /* adjust allocation statistics */

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = partId->curBlocksAllocated;          /* PCI bridge bug [SPR 68844]*/

    partId->curBlocksAllocated = htonl (ntohl (partId->curBlocksAllocated) - 1);

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = partId->curBlocksAllocated;          /* BRIDGE FLUSH  [SPR 68334] */

    /* EXIT LOCKED SECTION */

    SM_OBJ_LOCK_GIVE ((SM_SEM_ID) &partId->lock, level);

    return (OK);
    }


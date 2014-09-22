/* memShow.c - memory show routines */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01t,06oct01,tam  fixed sign and formatting
01s,05oct01,gls  moved printf in memPartInfoGet() to after semGive (SPR #20102)
01r,26sep01,jws  move vxMP smMemPartShowRtn ptr to funcBind.c (SPR36055)
01q,17mar99,jdi  doc: updated w/ info about proj facility (SPR 25727).
01p,21feb99,jdi  doc: listed errnos.
01o,17dec97,yp   (SPR #20054) memPartInfoGet now releases partId semaphore if 
		 memPartBlockIsValid fails
01n,10oct95,jdi  doc: added .tG Shell to SEE ALSOs;
		 fixed typo in memPartInfoGet().
01m,28mar95,ms	 memPartAvailable is now static, and doesn't call semTake().
01l,18jan95,jdi  doc cleanup for memPartInfoGet().
01k,01dec93,jag  added function memPartInfoGet.
01j,20jan94,jmm  memPartAvailable() now gives the semaphore on error (spr 2908)
01i,13sep93,jmm  fixed calll to memPartBlockIsValid() from memPartAvailable ()
01h,30aug93,jmm  added memPartAvailable, changed memPartShow to use it
01g,02feb93,jdi  doctweak (INCLUDE_SHOW_RTNS to ...ROUTINES).
01f,22jan93,jdi  documentation cleanup for 5.1.
01e,13nov92,dnw  added include of smObjLib.h
01d,02oct92,jdi  documentation cleanup.
01c,29jul92,pme  added NULL function pointer check for smObj routines.
                 moved OBJ_VERIFY before first printf in memPartShow.
01b,19jul92,pme  added shared memory management support.
01a,01jul92,jcf  extracted from memLib.c.
*/

/*
DESCRIPTION
This library contains memory partition information display routines.
To use this facility, it must first be installed using memShowInit(),
which is called automatically when the memory partition show facility
is configured into VxWorks using either of the following methods:
.iP
If you use the configuration header files, define
INCLUDE_SHOW_ROUTINES in config.h.
.iP
If you use the Tornado project facility, select INCLUDE_MEM_SHOW.
.LP


SEE ALSO: memLib, memPartLib,
.pG "Target Shell,"
windsh,
.tG "Shell"
*/

#include "vxWorks.h"
#include "semLib.h"
#include "dllLib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "smObjLib.h"
#include "private/memPartLibP.h"
#include "private/smMemLibP.h"

/* globals */

/* forward declarations */

static size_t memPartAvailable (PART_ID partId, size_t * largestBlock,
				BOOL printEach);
 
/******************************************************************************
*
* memShowInit - initialize the memory partition show facility
*
* This routine links the memory partition show facility into the VxWorks system.
* These routines are included automatically when this show facility is
* configured into VxWorks using either of the following methods:
* .iP
* If you use the configuration header files, define
* INCLUDE_SHOW_ROUTINES in config.h.
* .iP
* If you use the Tornado project facility, select INCLUDE_MEM_SHOW.
*
* RETURNS: N/A
*/

void memShowInit (void)
    {
    classShowConnect (memPartClassId, (FUNCPTR)memPartShow);
    }

/*******************************************************************************
*
* memShow - show system memory partition blocks and statistics
*
* This routine displays statistics about the available and allocated memory
* in the system memory partition.  It shows the number of bytes, the number
* of blocks, and the average block size in both free and allocated memory,
* and also the maximum block size of free memory.  It also shows the number
* of blocks currently allocated and the average allocated block size.
*
* In addition, if <type> is 1, the routine displays a list of all the blocks in
* the free list of the system partition.
*
* EXAMPLE
* .CS
*     -> memShow 1
*
*     FREE LIST:
*       num     addr      size
*       --- ---------- ----------
*         1   0x3fee18         16
*         2   0x3b1434         20
*         3    0x4d188    2909400
* 
*     SUMMARY:
*      status   bytes    blocks   avg block  max block
*      ------ --------- -------- ---------- ----------
*     current
*        free   2909436        3     969812   2909400
*       alloc    969060    16102         60        -
*     cumulative
*       alloc   1143340    16365         69        -
* .CE
*
* RETURNS: N/A
*
* SEE ALSO: memPartShow(),
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

void memShow 
    (
    int type 	/* 1 = list all blocks in the free list */
    )
    {
    memPartShow (memSysPartId, type);
    }

/*******************************************************************************
*
* memPartShow - show partition blocks and statistics
*
* This routine displays statistics about the available and allocated memory
* in a specified memory partition.  It shows the number of bytes, the number
* of blocks, and the average block size in both free and allocated memory,
* and also the maximum block size of free memory.  It also shows the number
* of blocks currently allocated and the average allocated block size.
*
* In addition, if <type> is 1, the routine displays a list of all the blocks
* in the free list of the specified partition.
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_smObjLib_NOT_INITIALIZED
*
* SEE ALSO: memShow(),
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

STATUS memPartShow 
    (
    PART_ID partId,	/* partition ID                          */
    int		 type		/* 0 = statistics, 1 = statistics & list */
    )
    {
    int		numBlocks;
    unsigned	totalBytes = 0;
    unsigned	biggestWords = 0;

    if (partId == NULL)
	{
	printf ("No partId specified.\n");
	return (ERROR);
	}

    if (ID_IS_SHARED (partId))  /* partition is shared? */
	{
	if (smMemPartShowRtn == NULL)
	    {
	    errno = S_smObjLib_NOT_INITIALIZED;
	    return (ERROR);
	    }

        return ((STATUS) (*smMemPartShowRtn)(SM_OBJ_ID_TO_ADRS (partId), type));
	}

    /* partition is local */

    if (OBJ_VERIFY (partId, memPartClassId) != OK)
	return (ERROR);

    /* print out list header if we are going to print list */

    if (type == 1)
	{
	printf ("\nFREE LIST:\n");
	printf ("   num    addr       size\n");
	printf ("  ---- ---------- ----------\n");
	}

    semTake (&partId->sem, WAIT_FOREVER);

    if ((totalBytes = memPartAvailable (partId, &biggestWords, type)) == ERROR)
	{
	semGive (&partId->sem);
        return (ERROR);
	}
    else
        biggestWords /= 2;	/* memPartAvailable returns bytes, not words */
    
    if (type == 1)
	printf ("\n\n");

    numBlocks = dllCount (&partId->freeList);

    if (type == 1)
	printf ("SUMMARY:\n");
    printf (" status    bytes     blocks   avg block  max block\n");
    printf (" ------ ---------- --------- ---------- ----------\n");
    printf ("current\n");

    if (numBlocks != 0)
	printf ("   free %10u %9u %10u %10u\n", totalBytes, numBlocks,
		totalBytes / numBlocks, 2 * biggestWords);
    else
	printf ("   no free blocks\n");

    if (partId->curBlocksAllocated != 0)
	printf ("  alloc %10u %9u %10u          -\n",
		2 * partId->curWordsAllocated, partId->curBlocksAllocated,
	        (2 * partId->curWordsAllocated) / partId->curBlocksAllocated);
    else
	printf ("   no allocated blocks\n");

    printf ("cumulative\n");

    if (partId->cumBlocksAllocated != 0)
	printf ("  alloc %10u %9u %10u          -\n",
		2 * partId->cumWordsAllocated, partId->cumBlocksAllocated,
		(2 * partId->cumWordsAllocated) / partId->cumBlocksAllocated);
    else
	printf ("   no allocated blocks\n");

    semGive (&partId->sem);

    return (OK);
    }

/*******************************************************************************
*
* memPartAvailable - return the amount of available memory in the partition
*
* This routine returns the amount of available memory in a specified partition.
* Additionally, if largestBlock is set to non-NULL, the value it points to
* is set to the size in bytes of the largest available block.
* 
* If printEach is TRUE, each block's address and size is printed.
* 
* RETURNS: Number of bytes of remaining memory, or ERROR.
*
* NOMANUAL
*/

static size_t memPartAvailable 
    (
    PART_ID 	 partId, 	/* partition ID                              */
    size_t *     largestBlock,	/* returns largest block of memory in bytes  */
    BOOL	 printEach	/* TRUE if each block to be printed          */
    )
    {
    BLOCK_HDR *	 pHdr;
    DL_NODE *	 pNode;
    size_t	 totalBytes   = 0;
    size_t	 biggestWords = 0;
    int		 ix           = 1;

    if (ID_IS_SHARED (partId))  /* partition is shared? */
	{
	if (smMemPartShowRtn == NULL)
	    {
	    errno = S_smObjLib_NOT_INITIALIZED;
	    return (ERROR);
	    }

	/* shared partitions not supported yet */
	
        return (ERROR);
	}

    /* partition is local */

    if (OBJ_VERIFY (partId, memPartClassId) != OK)
	return (ERROR);

    for (pNode = DLL_FIRST (&partId->freeList);
	 pNode != NULL;
	 pNode = DLL_NEXT (pNode))
	{
	pHdr = NODE_TO_HDR (pNode);

	/* check consistency and delete if not */

	if (!memPartBlockIsValid (partId, pHdr, pHdr->free))
	    {
	    printf ("  invalid block at %#x deleted\n", (UINT) pHdr);
	    dllRemove (&partId->freeList, HDR_TO_NODE (pHdr));
	    return (ERROR);
	    }
	else
	    {
	    totalBytes += 2 * pHdr->nWords;

	    if (pHdr->nWords > biggestWords)
		biggestWords = pHdr->nWords;

	    if (printEach)
		printf ("  %4d 0x%08x %10u\n", ix++, (UINT) pHdr,
			(UINT) 2 * pHdr->nWords);
	    }
	}
    if (largestBlock != NULL)
        *largestBlock = biggestWords * 2;

    return (totalBytes);
    }

/*******************************************************************************
*
* memPartInfoGet - get partition information
*
* This routine takes a partition ID and a pointer to a MEM_PART_STATS structure.
* All the parameters of the structure are filled in with the current partition
* information.
*
* RETURNS: OK if the structure has valid data, otherwise ERROR.
*
* SEE ALSO: memShow()
*/

STATUS memPartInfoGet 
    (
    PART_ID 		partId,		/* partition ID    	     */
    MEM_PART_STATS    * ppartStats      /* partition stats structure */
    )
    {
    BLOCK_HDR    * pHdr;
    DL_NODE      * pNode;

    if (partId == NULL || ppartStats == NULL)
	{
	return (ERROR);
	}

    if (ID_IS_SHARED (partId))  /* partition is shared? */
	{
	return (ERROR);         /* No support for Shared Partitions */
	}

    /* partition is local */

    if (OBJ_VERIFY (partId, memPartClassId) != OK)
	return (ERROR);

    ppartStats->numBytesFree     = 0;
    ppartStats->numBlocksFree    = 0;
    ppartStats->maxBlockSizeFree = 0;
    ppartStats->numBytesAlloc    = 0;
    ppartStats->numBlocksAlloc   = 0;

    semTake (&partId->sem, WAIT_FOREVER);

    /* Get free memory information */

    for (pNode = DLL_FIRST (&partId->freeList);
	 pNode != NULL;
	 pNode = DLL_NEXT (pNode))
	{
	pHdr = NODE_TO_HDR (pNode);

	/* check consistency and delete if not */

	if (!memPartBlockIsValid (partId, pHdr, pHdr->free))
	    {
	    dllRemove (&partId->freeList, HDR_TO_NODE (pHdr));
    	    semGive (&partId->sem);
	    printf ("  invalid block at %#x deleted\n", (UINT) pHdr);
	    return (ERROR);
	    }
	else
	    {
	    /* All byte counts are in words, the conversion is done later */

	    ppartStats->numBytesFree +=  pHdr->nWords;
	    ppartStats->numBlocksFree++;

	    if (ppartStats->maxBlockSizeFree < pHdr->nWords)
		ppartStats->maxBlockSizeFree = pHdr->nWords;
	    }
	}

    /* Get allocated memory information */

    if (partId->curBlocksAllocated != 0)
	{
	ppartStats->numBytesAlloc  = partId->curWordsAllocated; 
	ppartStats->numBlocksAlloc = partId->curBlocksAllocated;
	}

    semGive (&partId->sem);

    /* Convert from words to bytes */

    ppartStats->numBytesFree *= 2;
    ppartStats->maxBlockSizeFree *= 2;
    ppartStats->numBytesAlloc  *= 2;

    return (OK);
    }

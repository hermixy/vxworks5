/* smMemShow.c - shared memory management show routines (VxMP Option) */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01k,06may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
01j,24oct01,mas  fixed gnu warnings (SPR 71113); doc update (SPR 71149)
01i,14jul97,dgp  doc: add windsh x-ref to smMemShow()
01h,14feb93,jdi  documentation cleanup for 5.1.
01g,29jan93,pme  added little endian support.
01f,21nov92,jdi  documentation cleanup.
01e,21nov92,jdi  documentation cleanup.
01d,02oct92,pme  documentation cleanup.
                 added SPARC support.
01c,29sep92,pme  fixed WARNING in printf call.
01b,29jul92,pme  Added partition type check.
01a,19jul92,pme  written.
*/

/*
DESCRIPTION
This library provides routines to show the statistics on a shared memory
system partition.

General shared memory management routines are provided by smMemLib.

CONFIGURATION
The routines in this library are included by default if the component
INCLUDE_SM_OBJ is included.

AVAILABILITY
This module is distributed as a component of the unbundled shared memory
objects support option, VxMP.

INCLUDE FILES: smLib.h, smObjLib.h, smMemLib.h

SEE ALSO: smMemLib,
\tb VxWorks Programmer's Guide: Shared Memory Objects
*/

/* includes */

#include "vxWorks.h"
#include "errno.h"
#include "semLib.h"
#include "smObjLib.h"
#include "smLib.h"
#include "logLib.h"
#include "intLib.h"
#include "smMemLib.h"
#include "taskLib.h"
#include "cacheLib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "netinet/in.h"


/******************************************************************************
*
* smMemShowInit - initialize shared memory manager show routine
*
* This routine links the shared memory objects show routine into the VxWorks
* system.  These routines are included automatically by defining
* \%INCLUDE_SHOW_ROUTINES in configAll.h.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void smMemShowInit (void)
    {
    smMemPartShowRtn = (FUNCPTR) smMemPartShow;
    }

/******************************************************************************
*
* smMemPartShow - show shared partition blocks and statistics
*
* For a specified shared partition, this routine displays the total 
* amount of free space in the partition, the number of blocks, the average 
* block size, and the maximum block size.  It also shows the number of 
* blocks currently allocated, and the average allocated block size.
*
* In addition, if <type> is 1, this routine displays a list of all the blocks
* in the free list of the specified shared partition.
*
* WARNING
* This routine locks access to the partition while displaying the information.
* This can compromise the access time to the partition from other CPU in 
* the system.  Generally this routine is used for debugging purposes only.
*
* RETURNS: OK or ERROR.
*
* ERRNO:
*  S_objLib_OBJ_ID_ERROR
*  S_smObjLib_LOCK_TIMEOUT
*
* SEE ALSO: smMemShow()
* 
* NOMANUAL
*/

STATUS smMemPartShow 
    (
    SM_PART_ID partId,     /* global partition id to use */
    int        type        /* 0 = statistics, 1 = statistics & list */
    )
    {
    SM_BLOCK_HDR volatile * pHdr;
    SM_DL_NODE volatile *   pNode;
    unsigned                totalBytes   = 0;
    unsigned                biggestWords = 0;
    int                     numBlocks;
    int                     ix = 1;
    SM_PART_ID volatile     partIdv = (SM_PART_ID volatile) partId;
    int                     temp;            /* temp storage */

    if (partId == NULL)
	{
	printf ("No partId specified.\n");
	return (ERROR);
	}

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    temp = partIdv->verify;                     /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (partIdv) != OK)			/* shared object? */
        return (ERROR);

    if (ntohl (partIdv->objType) != MEM_PART_TYPE_SM_STD)/* shared partition?*/
	{
	errno = S_objLib_OBJ_ID_ERROR;
	return (ERROR);
	}

    /* print out list header if we are going to print list */

    if (type > 0)
	{
	printf ("\nFREE LIST:\n");
	printf ("  num     addr      size\n");
	printf ("  --- ---------- ----------\n");
	}

    if (smMemPartAccessGet(partIdv) != OK)
        {
        return (ERROR);
        }

    /*
     * go through free list and find total free and largest free,
     * and print each block if specified
     */

    for (pNode = (SM_DL_NODE volatile *) SM_DL_FIRST (&partId->freeList);
	 pNode != LOC_NULL;
	 pNode = (SM_DL_NODE volatile *) SM_DL_NEXT (pNode))
	{
	pHdr = (SM_BLOCK_HDR volatile *) SM_NODE_TO_HDR (pNode);

        /* check consistency and delete if not */

        if (!smMemPartBlockIsValid (partIdv, pHdr, TRUE))
            {
            printf ("  invalid block at %p deleted\n", pHdr);
            smDllRemove (&partId->freeList, SM_HDR_TO_NODE (pHdr));
            }
        else
	    {
	    totalBytes += 2 * (ntohl (pHdr->nWords));

	    if (ntohl (pHdr->nWords) > biggestWords)
		biggestWords = ntohl (pHdr->nWords);

	    if (type >= 1)
		printf ("  %3d %#10x %10d\n", ix++, LOC_TO_GLOB_ADRS (pHdr), 
			2 * ntohl (pHdr->nWords));
	    }
	}

    if (type > 0)
        {
	printf ("\n\n");
        }

    numBlocks = smDllCount (&partId->freeList);

    if (type > 0)
        {
	printf ("SUMMARY:\n");
        }
    printf (" status   bytes    blocks   ave block  max block\n");
    printf (" ------ --------- -------- ---------- ----------\n");
    printf ("current\n");

    if (numBlocks != 0)
        {
	printf ("   free  %8d  %7d  %9d %9d\n", totalBytes, numBlocks,
		totalBytes / numBlocks, 2 * biggestWords);
        }
    else
        {
	printf ("   no free blocks\n");
        }

    if (ntohl (partIdv->curBlocksAllocated) != 0)
        {
	printf ("  alloc  %8d  %7d  %9d        -\n",
		2 * ntohl (partId->curWordsAllocated), 
		ntohl (partId->curBlocksAllocated),
	        (2 * ntohl (partId->curWordsAllocated)) / 
		ntohl (partId->curBlocksAllocated));
        }
    else
        {
	printf ("   no allocated blocks\n");
        }

    printf ("cumulative\n");

    if (ntohl (partIdv->cumBlocksAllocated) != 0)
        {
	printf ("  alloc  %8d  %7d  %9d        -\n",
		2 * ntohl (partId->cumWordsAllocated), 
		ntohl (partId->cumBlocksAllocated),
		(2 * ntohl (partId->cumWordsAllocated)) / 
		ntohl (partId->cumBlocksAllocated));
        }
    else
        {
	printf ("   no allocated blocks\n");
        }

    if (smMemPartAccessRelease (partIdv) != OK)
        {
        return (ERROR);
        }

    return (OK);
    }

/******************************************************************************
*
* smMemShow - show the shared memory system partition blocks and statistics (VxMP Option)
*
* This routine displays the total amount of free space in the shared memory
* system partition, including the number of blocks, the average block size,
* and the maximum block size.  It also shows the number of blocks currently
* allocated, and the average allocated block size.
*
* If <type> is 1, it displays a list of all the blocks in the free list of
* the shared memory system partition.
*
* WARNING
* This routine locks access to the shared memory system partition while
* displaying the information.  This can compromise the access time to
* the partition from other CPUs in the system.  Generally, this routine is
* used for debugging purposes only.
*
* EXAMPLE
* \cs
*    -> smMemShow 1
* 
*    FREE LIST:
*      num    addr       size
*      --- ---------- ----------
*        1   0x4ffef0        264
*        2   0x4fef18       1700
*
*    SUMMARY:
*        status        bytes    blocks   ave block  max block
*    --------------- --------- -------- ---------- ----------
*            current
*               free      1964        2        982       1700
*              alloc      2356        1       2356         -
*         cumulative
*              alloc      2620        2       1310         -
*    value = 0 = 0x0
* \ce
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled shared memory
* objects support option, VxMP.
* 
* RETURNS: N/A
*
* SEE ALSO
* windsh, chapter on target shell in 
* \tb VxWorks Programmer's Guide, 
* \tb Tornado User's Guide: Shell
*/

void smMemShow 
    (
    int type 		/* 0 = statistics, 1 = statistics & list */
    )
    {
    smMemPartShow ((SM_PART_ID) smSystemPartId, type);
    }


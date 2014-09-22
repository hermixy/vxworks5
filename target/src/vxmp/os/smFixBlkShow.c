/* smFixBlkShow.c - fixed block shared memory manager show utility (VxMP Option) */

/* Copyright 1984-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,06may02,mas  cache flush and volatile fix (SPR 68334); bridge flush fix
		 (SPR 68844)
01d,24oct01,mas  doc update (SPR 71149)
01c,29jan93,pme  added little endian support
01b,02oct92,pme  added SPARC support.
01a,19jul92,pme  written.
*/

/*
DESCRIPTION
This library provides routines to show current status and statistics
for the fixed block shared memory manager.

There are no user callable routines.

AVAILABILITY
This module is distributed as a component of the unbundled shared memory
support option, VxMP.

SEE ALSO: smFixBlkLib, smObjLib

NOROUTINES
*/

/* includes */

#include "vxWorks.h"
#include "cacheLib.h"
#include "smObjLib.h"
#include "smLib.h"
#include "smMemLib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "netinet/in.h"
#include "private/smFixBlkLibP.h"

/******************************************************************************
*
* smFixBlkPartShow - show fixed block shared partition statistics
*
* For a specified fixed block shared partition, this routine displays the total 
* number of blocks in the partition and the block size. It also shows 
* the number of blocks currently allocated, and the cumulative number of
* allocated blocks.
*
* <gPartId> is the global (system wide) identifier of the partition to 
* use. Its value is the global address of the partition structure. 
*
* WARNING: This routine does not get exclusive access to the partition 
*          to display its status.
*
* RETURNS: OK or ERROR.
*
* ERRNO:
*
*   S_smMemLib_INVALID_PART_ID
*
* NOMANUAL
*/

STATUS smFixBlkPartShow 
    (
    SM_FIX_BLK_PART_ID gPartId  /* global partition id to use */
    )
    {
    SM_FIX_BLK_PART_ID volatile partId;  /* local address of partition */
    int                         tmp;

    partId = (SM_FIX_BLK_PART_ID volatile) GLOB_TO_LOC_ADRS (gPartId);

    if (partId == LOC_NULL)
	{
	printf ("No partId specified.\n");
	return (ERROR);
	}

    CACHE_PIPE_FLUSH ();                        /* CACHE FLUSH   [SPR 68334] */
    tmp = partId->verify;                       /* PCI bridge bug [SPR 68844]*/

    if (SM_OBJ_VERIFY (partId) != OK)
        return (ERROR);

    /* now print the statistics without locking access to partition */

    printf ("\nBlock size                  : %d bytes  \n", 
	    ntohl (partId->blockSize));
    printf ("Allocated blocks            : max : %d current : %d free : %d\n", 
	     ntohl (partId->totalBlocks),
	     ntohl (partId->curBlocksAllocated),
	     (ntohl(partId->totalBlocks) - ntohl(partId->curBlocksAllocated)));
    printf ("Cumulative allocated blocks : %d\n", 
	    ntohl (partId->cumBlocksAllocated));

    return (OK);
    }


/* rBuffShow.c - dynamic ring buffer (rBuff) show routines */

/* Copyright 1984-1997 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,04mar99,dgp  replace INCLUDE_SHOW_ROUTINES with INCLUDE_RBUFF_SHOW
01e,28aug98,dgp  FCS man page edit
01d,06may98,dgp  clean up man pages for WV 2.0 beta release
01c,18dec97,cth  added include of wvBufferP.h
01b,16nov97,cth  changed rBuffId to WV2.0 generic-buffer id in rBuffShow
01a,23Sep97,nps  extracted from rBuffLib.c (based upon memShow.c).
*/

/*
DESCRIPTION
This library contains display routines for the dynamic ring buffer.
These routines are included automatically when INCLUDE_RBUFF_SHOW and
INCLUDE_WINDVIEW are defined.


SEE ALSO: rBuffLib, windsh,
.pG "Target Shell,"
.tG "Shell"
*/

#include "vxWorks.h"
#include "dllLib.h"
#include "rBuffLib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "errno.h"

#include "private/semLibP.h"
#include "private/wvBufferP.h"

/* globals */

FUNCPTR rBuffShowRtn;	/* rBuff show routine */

/* forward declarations */

/******************************************************************************
*
* rBuffShowInit - initialize the rBuff show facility
*
* This routine links the rBuff show facility into the VxWorks system.
* These routines are included automatically when INCLUDE_RBUFF_SHOW is
* defined.
*
* RETURNS: OK or ERROR.
*/

void rBuffShowInit (void)
    {
    classShowConnect (rBuffClassId, (FUNCPTR)rBuffShow);
    }

/*******************************************************************************
*
* rBuffShow - show rBuff details and statistics
*
* If <type> is 1, the routine displays a list of all the buffers in
* the rBuff.
*
* Protection is provided to ensure the snapshot shown is consistent.
*
* EXAMPLE
* .CS
*     -> rBuffShow buffId,1
* .CE
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: windsh, memPartShow(),
* .pG "Target Shell,"
* .tG "Shell"
*/

STATUS rBuffShow
    (
    BUFFER_ID buffId,		/* generic buffer identifier */
    UINT32    type      /* 0 = statistics, 1 = statistics & list */
    )
    {
    RBUFF_ID rBuff;		/* access to private members of this rBuff */

    /* Get access to this buffer's private information. */

    rBuff = (RBUFF_ID) buffId;

    /* Check validity of arguments. */

    if (rBuff == NULL)
	{
	printf ("No rBuffId specified.\n");
	return (ERROR);
	}

    if (OBJ_VERIFY (rBuff, rBuffClassId) != OK)
	return (ERROR);

    RBUFF_LOCK (rBuff);

    printf ("\nSummary of rBuff id: %p :\n",rBuff);
    printf ("---------------------------------\n");

    if (rBuff->info.srcPart == memSysPartId)
        {
        printf ("source partition   : System Partition\n");
        }
    else
        {
        printf ("source partition   : %p\n", rBuff->info.srcPart);
        }

    printf ("options            : %08x\n", rBuff->info.options);
    printf ("buffer size        : %#x\n",  rBuff->info.buffSize);
    printf ("curr no. of buffs  : %d",     rBuff->info.currBuffs);

    if (rBuff->info.currBuffs == rBuff->info.minBuffs)
        {
        printf (" (min)");
        }
    else if (rBuff->info.currBuffs == rBuff->info.maxBuffs)
        {
        printf (" (max)");
        }

    printf ("\nmin/actualMax/max  : %d / %d / %d\n",
        rBuff->info.minBuffs,
        rBuff->info.maxBuffsActual,
        rBuff->info.maxBuffs);

    printf ("empty buffs        : %d\n",   rBuff->info.emptyBuffs);
    printf ("times extended     : %d\n",   rBuff->info.timesExtended);

#if 0

    printf ("curr buff to read  : %p\n",   rBuff->info.buffRead);
    printf ("curr buff to write : %p\n",   rBuff->info.buffWrite);

#endif

    printf ("threshold (X'd)    : %#x (%#x)", rBuff->info.threshold,
                                              rBuff->info.timesXThreshold);

    if (rBuff->info.dataContent > rBuff->info.threshold)
        {
        printf (" *");
        }

    printf ("\ndata content       : %#x (%d%%)\n",
        rBuff->info.dataContent,
        (rBuff->info.dataContent * 100) /
            (rBuff->info.buffSize * rBuff->info.currBuffs));

    printf ("access count w / r : %d / %d\n",
        rBuff->info.writesSinceReset,
        rBuff->info.readsSinceReset);

    printf ("bytes total  w / r : %d / %d\n",
        rBuff->info.bytesWritten,
        rBuff->info.bytesRead);

    printf ("average      w / r : %d / %d\n",
        (rBuff->info.writesSinceReset == 0 ? 0 :
            rBuff->info.bytesWritten / rBuff->info.writesSinceReset),
        (rBuff->info.readsSinceReset == 0 ? 0 :
            rBuff->info.bytesRead / rBuff->info.readsSinceReset));


    if (type == 1)
        {
        /* now traverse the ring displaying the individual buffers */

        RBUFF_PTR backHome, currBuff;
        UINT32 buffCount   = 1;


        backHome = currBuff = rBuff->buffRead;

        printf ("\n- traversing ring of buffers -\n");

        do
            {
            printf ("%03d: addr: %p buff: %p dataLen: 0x%04x spaceAvail: 0x%04x",
                buffCount++, currBuff, currBuff->dataStart,
                currBuff->dataLen, currBuff->spaceAvail);

            if (currBuff == rBuff->buffWrite)
                {
                printf (" W");
                }

            if (currBuff == rBuff->buffRead)
                {
                printf (" R");
                }

            printf ("\n");

            currBuff = currBuff->next;
            }
        while(currBuff != backHome);

        printf ("... and back to %p\n",backHome);
        }

    RBUFF_UNLOCK(rBuff);

    return (OK);
    }


/*******************************************************************************
*
* rBuffInfoGet - get rBuff information
*
* This routine takes a rBuff ID and a pointer to a 'rBuff_STATS' structure.
* All the parameters of the structure are filled in with the current partition
* information.
*
* RETURNS: OK if the structure has valid data, otherwise ERROR.
*
* SEE ALSO: rBuffShow()
*/

STATUS rBuffInfoGet 
    (
    BUFFER_ID buffId,		/* generic buffer identifier */
    RBUFF_INFO_TYPE *pRBuffInfo /* rBuff info structure */
    )
    {
    RBUFF_ID    rBuff;          /* access to private members of this rBuff */

    /* Get access to this buffer's private information. */

    rBuff = (RBUFF_ID) buffId;

    /* Check validity of the arguments. */

    if (rBuff == NULL)
	{
	printf ("No rBuffId specified.\n");
	return (ERROR);
	}

    if (OBJ_VERIFY (rBuff, rBuffClassId) != OK)
	return (ERROR);

    RBUFF_LOCK (rBuff);

    pRBuffInfo->srcPart          = rBuff->info.srcPart;
    pRBuffInfo->options          = rBuff->info.options;
    pRBuffInfo->buffSize         = rBuff->info.buffSize;
    pRBuffInfo->currBuffs        = rBuff->info.currBuffs;
    pRBuffInfo->minBuffs         = rBuff->info.minBuffs;
    pRBuffInfo->maxBuffs         = rBuff->info.maxBuffs;
    pRBuffInfo->maxBuffsActual   = rBuff->info.maxBuffsActual;
    pRBuffInfo->emptyBuffs       = rBuff->info.emptyBuffs;
    pRBuffInfo->timesExtended    = rBuff->info.timesExtended;
    pRBuffInfo->threshold        = rBuff->info.threshold;
    pRBuffInfo->timesXThreshold  = rBuff->info.timesXThreshold;
    pRBuffInfo->dataContent      = rBuff->info.dataContent;
    pRBuffInfo->writesSinceReset = rBuff->info.writesSinceReset;
    pRBuffInfo->writesSinceReset = rBuff->info.readsSinceReset;
    pRBuffInfo->bytesWritten     = rBuff->info.bytesWritten;
    pRBuffInfo->bytesRead        = rBuff->info.bytesRead;


    RBUFF_UNLOCK(rBuff);

    return (OK);
    }

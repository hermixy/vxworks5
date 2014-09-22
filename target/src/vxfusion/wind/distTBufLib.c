/* distTBufLib.c - distributed objects telegram buffer library (VxFusion option) */

/* Copyright 1999 - 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,26nov01,jws  fix void ptr arith not seen by diab (SPR 71154)
01f,30oct01,jws  fix man pages (SPR 71239)
01e,24may99,drm  added vxfusion prefix to VxFusion related includes
01d,23feb99,wlf  doc edits
01c,18feb99,wlf  doc cleanup
01b,29oct98,drm  documentation updates
01a,03sep97,ur   written.
*/

/*
DESCRIPTION
This library provides routines for allocating and freeing telegram buffers.  
Telegrams are the largest packets that can be sent between nodes by the 
distributed objects product; their size is limited by the MTU size of the
underlying communications.  If a distributed objects message exceeds the space 
allocated in a telegram for message data, that message is divided 
into multiple telegrams that are sent out in sequence. 

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.

INCLUDE FILES: distTBufLib.h
*/

#include "vxWorks.h"
#ifdef DIST_DIAGNOSTIC
#include "stdio.h"
#endif
#include "stdlib.h"
#include "string.h"
#include "semLib.h"
#include "private/semLibP.h"
#include "vxfusion/distIfLib.h"
#include "vxfusion/distStatLib.h"
#include "vxfusion/private/distNetLibP.h"
#include "vxfusion/private/distTBufLibP.h"

/* defines */

#define MIN(a,b)    (((a) < (b)) ? (a) : (b))

/* local variables */

LOCAL BOOL                distTBufLibInstalled = FALSE;
LOCAL DIST_TBUF_POOL      distTBufPool;
LOCAL int                 tbHSz, ifHSz, ifDSz, tBufSize;
LOCAL SEMAPHORE           distTBufPoolLock;

/***************************************************************************
*
* distTBufLibInit - pre-allocate a pool of telegram buffers (VxFusion option)
*
* This routine initializes a pool of telegram buffers.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, when successful.  ERROR, when initialization fails.
* 
* NOMANUAL
*/

STATUS distTBufLibInit
    (
    int      numTBufs    /* the number of buffers to allocate */
    )
    {
    int    i;
    char * pPool;
    char * p;

    if (distTBufLibInstalled == TRUE)
        return (OK);

    tbHSz = MEM_ROUND_UP (sizeof (DIST_TBUF));
    ifHSz = MEM_ROUND_UP (DIST_IF_HDR_SZ);
    ifDSz = MEM_ROUND_UP (DIST_IF_MTU - DIST_IF_HDR_SZ);

    tBufSize = tbHSz + ifHSz + ifDSz;
    if (tBufSize < MEM_ROUND_UP (sizeof (DIST_TBUF_HDR)))
        tBufSize = MEM_ROUND_UP (sizeof (DIST_TBUF_HDR));

    if ((pPool = (char *) malloc (numTBufs * tBufSize)) == NULL)
        {
#ifdef DIST_DIAGNOSTIC
        printf ("distTBufLibInit: memory allocation failed\n");
#endif
        return (ERROR);
        }

    distTBufPool.pTBufFree = (DIST_TBUF *) pPool;

    for (p = pPool, i = 1; i < numTBufs; p += tBufSize, i++)
        {
        ((DIST_TBUF_GEN *) p)->pTBufGenNext = (DIST_TBUF_GEN *) (p + tBufSize);
        }
    ((DIST_TBUF_GEN *) p)->pTBufGenNext = NULL;

    semBInit (&distTBufPoolLock, SEM_Q_PRIORITY, SEM_FULL);

    distTBufLibInstalled = TRUE;
    return (OK);
    }

/***************************************************************************
*
* distTBufAlloc - allocate a telegram buffer from the pool of buffers (VxFusion option)
*
* This routine allocates a telegram buffer from a pre-allocated pool
* of telegram buffers.
*
* It is the responsibility of the caller to use the distTBufFree() routine to
* free the buffer when the caller is finished with it.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: A pointer to a DIST_TBUF, or NULL if the allocation fails.
*
* SEE ALSO: distTBufFree
*/

DIST_TBUF * distTBufAlloc (void)
    {
    DIST_TBUF * pTBuf;

    distTBufPoolLock();

    if ((pTBuf = distTBufPool.pTBufFree) != NULL)
        {
        distTBufPool.pTBufFree =
                (DIST_TBUF *) ((DIST_TBUF_GEN *) pTBuf)->pTBufGenNext;

        distTBufPoolUnlock();

        bzero ((char *) pTBuf, tBufSize);
        
        pTBuf->pTBufData = ((char *) pTBuf) + tbHSz + ifHSz;
        ((DIST_TBUF_GEN *) pTBuf)->pTBufGenNext = NULL;
        ((DIST_TBUF_GEN *) pTBuf)->pTBufGenLast = (DIST_TBUF_GEN *) pTBuf;

#ifdef DIST_DIAGNOSTIC
        pTBuf->tBufHeader = FALSE;
#endif
        return (pTBuf);
        }

    distTBufPoolUnlock();

    distStat.tBufShortage++;
    return (NULL);
    }

/***************************************************************************
*
* distTBufAllocHdr - allocate a DIST_TBUF_HDR object (VxFusion option)
*
* This routine allocates and returns a pointer to a DIST_TBUF_HDR.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: A pointer to the DIST_TBUF_HDR, or NULL.
* 
* NOMANUAL
*/

DIST_TBUF_HDR * distTBufAllocHdr (void)
    {
    DIST_TBUF_HDR * pTBufHdr;

    distTBufPoolLock();

    if ((pTBufHdr = (DIST_TBUF_HDR *) distTBufPool.pTBufFree) != NULL)
        {
        distTBufPool.pTBufFree =
                (DIST_TBUF *) ((DIST_TBUF_GEN *) pTBufHdr)->pTBufGenNext;

        distTBufPoolUnlock();

        bzero ((char *) pTBufHdr, tBufSize);

        ((DIST_TBUF_GEN *) pTBufHdr)->pTBufGenNext = NULL;
        ((DIST_TBUF_GEN *) pTBufHdr)->pTBufGenLast =
                                             (DIST_TBUF_GEN *) pTBufHdr;

        pTBufHdr->pTBufHdrNext = pTBufHdr->pTBufHdrPrev = NULL;
        pTBufHdr->pTBufHdrLast = pTBufHdr;
        pTBufHdr->pTBufHdrTm = NULL;

#ifdef DIST_DIAGNOSTIC
        pTBufHdr->tBufHdrHeader = TRUE;
#endif
        return (pTBufHdr);
        }

    distTBufPoolUnlock();

    distStat.tBufShortage++;
    return (NULL);
    }

/***************************************************************************
*
* distTBufFree - return a telegram buffer to the pool of buffers (VxFusion option)
*
* This routine returns a buffer previously allocated to a caller back to
* the pool of free telegram buffers.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* SEE ALSO: distTBufAlloc
*/

void distTBufFree
    (
    DIST_TBUF * pTBuf  /* ptr to buffer to be returned to pool */
    )
    {
    distTBufPoolLock();

    ((DIST_TBUF_GEN *) pTBuf)->pTBufGenNext =
            (DIST_TBUF_GEN *) distTBufPool.pTBufFree;
    distTBufPool.pTBufFree = pTBuf;

    distTBufPoolUnlock();
    }


/***************************************************************************
*
* distTBufCopy - copies from tBuf list to buffer (VxFusion option)
*
* This routine copies bytes from a DIST_TBUF to a buffer.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: The number of bytes copied.
* 
* NOMANUAL
*/

int distTBufCopy
    (
    DIST_TBUF * pTBuf,   /* source DIST_TBUF */
    int         offset,  /* byte offset into pTBuf */
    char *      dest,    /* destination */
    int         nBytes   /* maximum bytes to transfer */
    )
    {
    int    copied = 0;
    int    len;

    do
        {
        if (offset == 0 || (offset - pTBuf->tBufNBytes) < 0)
            {
            len = MIN (nBytes, (pTBuf->tBufNBytes - offset));
            bcopy (((char *)pTBuf->pTBufData + offset), dest, len);
            nBytes -= len;
            dest += len;
            copied += len;
            offset = 0;
            }
        else
            offset -= pTBuf->tBufNBytes;
        }
    while (nBytes > 0 && (pTBuf = DIST_TBUF_GET_NEXT (pTBuf)));

    return (copied);
    }


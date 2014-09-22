/* distTBufLibP.h - telegram buffer library private header (VxFusion option) */

/* Copyright 1999 -2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,23oct01,jws  fix compiler warnings
01f,24may99,drm  Added vxfusion prefix to VxFusion related includes
01e,29oct98,drm  Moved distTBufLibInit() here from public header file
01d,12aug98,drm  Moved all non-private declarations to distTBufLib.h
01c,08aug98,drm  Changed fields in DIST_TBUF to be uint16_t 
01b,26jan98,ur   bugfix in makro DIST_TBUF_HDR_INSERT_AFTER().
01a,17jul97,ur   written.
*/

#ifndef __INCdistTBufLibPh
#define __INCdistTBufLibPh

#ifdef __cplusplus
extern "C" {
#endif

#include "vxWorks.h"
#include "vxfusion/distTBufLib.h"
#include "private/semLibP.h"

/* typedefs */

typedef int DIST_TBUF_HDR_STATUS;             /* DIST_TBUF_HDR_STATUS */

typedef struct                                /* DIST_TRANSMISSION */
    {
    SEMAPHORE                tmWait4;       /* place to wait for an event */
    DIST_TBUF_HDR_STATUS     tmStatus;      /* status of packet */
    int                      tmRetmTimo;    /* timeout for retransmission */
    int                      tmNumTm;       /* count of transmissions */
    int                      tmAckCnt;
    } DIST_TRANSMISSION;

typedef struct _DIST_TBUF_HDR               /* DIST_TBUF_HDR */
    {
    DIST_TBUF_GEN           tBufGen;        /* generic part of a tbuf */
    struct _DIST_TBUF_HDR * pTBufHdrPrev;   /* ptr to prev pkt in queue */
    struct _DIST_TBUF_HDR * pTBufHdrNext;   /* ptr to next pkt in queue */
    struct _DIST_TBUF_HDR * pTBufHdrLast;   /* ptr to last pkt in queue */

    DIST_NODE_ID            tBufHdrNodeId;
    short                   tBufHdrId;
    short                   tBufHdrOverall;
    int                     tBufHdrPrio;    /* priority */

                                            /* Sending */
    int                     tBufHdrTimo;    /* user supplied timeout */
    DIST_TRANSMISSION *     pTBufHdrTm;

                                            /* Receiving */
    short                   tBufHdrNLeaks;  /* for reassembly */

    } DIST_TBUF_HDR;

#define tBufHdrSrc        tBufHdrNodeId
#define tBufHdrDest       tBufHdrNodeId

typedef struct                              /* DIST_TBUF_POOL */
    {
    DIST_TBUF *      pTBufFree;
    } DIST_TBUF_POOL;

/* defines */

#define distTBufPoolLock()        semTake (&distTBufPoolLock, WAIT_FOREVER)
#define distTBufPoolUnlock()      semGive (&distTBufPoolLock);

/*
 *      horizontal chaining ---------->
 *
 *
 *   tBufHdr0 <-> tBufHdr1 <-> tBufHdr2 --||
 *       |            |            |               vertical
 *       v            v            v               chaining
 *    tBuf0.0      tBuf1.0      tBuf2.0               |
 *       |            |            |                  |
 *       v            _            v                  |
 *    tBuf0.1         -         tBuf2.1               |
 *       |                         |                  |
 *       v                         _                  v
 *    tBuf0.2                      -
 *       |
 *       _
 *       -
 */

#define DIST_TBUF_ALLOC()       distTBufAlloc ()     /* allocate a TBuf*/

#define DIST_TBUF_FREE(pTBuf)   distTBufFree (pTBuf) /* free a TBuf */
    
#define DIST_TBUF_HDR_ALLOC()   distTBufAllocHdr ()

#define DIST_TBUF_FREEM(pTBuf)                             \
    {                                                      \
    DIST_TBUF_GEN *pOld, *p = (DIST_TBUF_GEN *) (pTBuf);   \
    while ((pOld = p) != 0)                                \
        {                                                  \
        p = p->pTBufGenNext;                               \
        distTBufFree ((DIST_TBUF *) pOld);                 \
        }                                                  \
    }

/*
 * vertical chaining
 */

#define DIST_TBUF_GET_LAST(pTBuf)                               \
    ((DIST_TBUF *) (((DIST_TBUF_GEN *) (pTBuf))->pTBufGenLast))

#define DIST_TBUF_GET_NEXT(pTBuf)                               \
    ((DIST_TBUF *) (((DIST_TBUF_GEN *) (pTBuf))->pTBufGenNext))

#define DIST_TBUF_SET_LAST(pTBuf0, pTBuf1)                      \
    ((((DIST_TBUF_GEN *) (pTBuf0))->pTBufGenLast) =             \
            (DIST_TBUF_GEN *) pTBuf1)

#define DIST_TBUF_SET_NEXT(pTBuf0, pTBuf1)                      \
    ((((DIST_TBUF_GEN *) (pTBuf0))->pTBufGenNext) =             \
            (DIST_TBUF_GEN *) pTBuf1)

#define DIST_TBUF_ENQUEUE(pTBuf0, pTBuf1)                       \
    {                                                           \
    ((DIST_TBUF_GEN *) (pTBuf0))->pTBufGenLast->pTBufGenNext =  \
            (DIST_TBUF_GEN *) pTBuf1;                           \
    ((DIST_TBUF_GEN *) (pTBuf0))->pTBufGenLast =                \
            (DIST_TBUF_GEN *) pTBuf1;                           \
    ((DIST_TBUF_GEN *) (pTBuf1))->pTBufGenNext =                \
            ((DIST_TBUF_GEN *) (pTBuf1))->pTBufGenLast = NULL;  \
    }

#define DIST_TBUF_INSERT_AFTER(pTBuf0, pTBuf1, pTBuf2)                   \
    {                                                                    \
    if (! (pTBuf1))                                                      \
    {                                                                    \
    ((DIST_TBUF_GEN *) pTBuf2)->pTBufGenNext = (DIST_TBUF_GEN *) pTBuf0; \
    ((DIST_TBUF_GEN *) pTBuf2)->pTBufGenLast =                           \
                            ((DIST_TBUF_GEN *) pTBuf0)->pTBufGenLast;    \
    pTBuf0 = (void *) pTBuf2;                                            \
    }                                                                    \
    else                                                                 \
    {                                                                    \
    if (! (((DIST_TBUF_GEN *) pTBuf2)->pTBufGenNext =                    \
                            ((DIST_TBUF_GEN *) pTBuf1)->pTBufGenNext))   \
        ((DIST_TBUF_GEN *) pTBuf0)->pTBufGenLast = (DIST_TBUF_GEN *) pTBuf2; \
    ((DIST_TBUF_GEN *) pTBuf1)->pTBufGenNext = (DIST_TBUF_GEN *) pTBuf2;     \
    }                                                                        \
    }

/*
 * horizontal chaining
 */

#define DIST_TBUF_HDR_GET_NEXT(pTBufHdr)                                    \
    ((pTBufHdr)->pTBufHdrNext)

#define DIST_TBUF_HDR_SET_NEXT(pTBufHdr0, pTBufHdr1)                        \
    (((pTBufHdr0)->pTBufHdrNext) = (pTBufHdr1))

#define DIST_TBUF_HDR_GET_PREV(pTBufHdr)                                    \
    ((pTBufHdr)->pTBufHdrPrev)

#define DIST_TBUF_HDR_SET_PREV(pTBufHdr0, pTBufHdr1)                        \
    (((pTBufHdr0)->pTBufHdrPrev) = (pTBufHdr1))

#define DIST_TBUF_HDR_SET_LAST(pTBufHdr0, pTBufHdr1)                        \
    (((pTBufHdr0)->pTBufHdrLast) = (pTBufHdr1))

/*
 *                            queue      enqueue me
 *                              |           |
 *                              v           v
 */
#define DIST_TBUF_HDR_ENQUEUE(pTBufHdr0, pTBufHdr1)                         \
    {                                                                       \
    if ((pTBufHdr0))                                                        \
        {                                                                   \
        (pTBufHdr0)->pTBufHdrLast->pTBufHdrNext = (pTBufHdr1);              \
        (pTBufHdr1)->pTBufHdrPrev = (pTBufHdr0)->pTBufHdrLast;              \
        (pTBufHdr0)->pTBufHdrLast = (pTBufHdr1);                            \
        (pTBufHdr1)->pTBufHdrNext = (pTBufHdr1)->pTBufHdrLast = NULL;       \
        }                                                                   \
    else                                                                    \
        {                                                                   \
        (pTBufHdr0) = (pTBufHdr1);                                          \
        (pTBufHdr1)->pTBufHdrLast = (pTBufHdr1);                            \
        (pTBufHdr1)->pTBufHdrNext = (pTBufHdr1)->pTBufHdrPrev = NULL;       \
        }                                                                   \
    }

/*
 *                            queue      has the dequeued
 *                              |        one afterwards
 *                              |           |
 *                              v           v
 */
#define DIST_TBUF_HDR_DEQUEUE(pTBufHdr0, pTBufHdr1)                       \
    {                                                                     \
    (pTBufHdr1) = (pTBufHdr0);                                            \
    (pTBufHdr0) = (pTBufHdr0)->pTBufHdrNext;                              \
    if ((pTBufHdr0))                                                      \
        (pTBufHdr0)->pTBufHdrLast = (pTBufHdr1)->pTBufHdrLast;            \
    }

/*
 *                           queue   unlink me
 *                             |       |
 *                             v       v
 */
#define DIST_TBUF_HDR_UNLINK(pTBuf0, pTBuf1)                              \
    {                                                                     \
    DIST_TBUF_HDR *prev = (pTBuf1)->pTBufHdrPrev;                         \
    if (! prev)                                                           \
        {                                                                 \
        if (((pTBuf0) = (pTBuf1)->pTBufHdrNext) != 0)                     \
            {                                                             \
            (pTBuf0)->pTBufHdrPrev = NULL;                                \
            (pTBuf0)->pTBufHdrLast = (pTBuf1)->pTBufHdrLast;              \
            }                                                             \
        }                                                                 \
    else                                                                  \
        {                                                                 \
        if ((prev->pTBufHdrNext = (pTBuf1)->pTBufHdrNext) != 0)           \
            prev->pTBufHdrNext->pTBufHdrPrev = prev;                      \
        else                                                              \
            (pTBuf0)->pTBufHdrLast = prev;                                \
        }                                                                 \
    }

/*
 *                                 queue   prev    insert me
 *                                   |       |        |
 *                                   v       v        v
 */
#define DIST_TBUF_HDR_INSERT_AFTER(pTBuf0, pTBuf1, pTBuf2)                \
    {                                                                     \
    if (! (pTBuf1))                                                       \
        {                                                                 \
        (pTBuf2)->pTBufHdrPrev = NULL;                                    \
        if (! ((pTBuf2)->pTBufHdrNext = (pTBuf0)))                        \
            (pTBuf2)->pTBufHdrLast = NULL;                                \
        else                                                              \
            {                                                             \
            (pTBuf2)->pTBufHdrLast = (pTBuf0)->pTBufHdrLast;              \
            (pTBuf0)->pTBufHdrPrev = (pTBuf2);                            \
            }                                                             \
        (pTBuf0) = (pTBuf2);                                              \
        }                                                                 \
    else                                                                  \
        {                                                                 \
        if (! ((pTBuf2)->pTBufHdrNext = (pTBuf1)->pTBufHdrNext))          \
            (pTBuf0)->pTBufHdrLast = (pTBuf2);                            \
        else                                                              \
            (pTBuf1)->pTBufHdrNext->pTBufHdrPrev = (pTBuf2);              \
        (pTBuf1)->pTBufHdrNext = (pTBuf2);                                \
        (pTBuf2)->pTBufHdrPrev = (pTBuf1);                                \
            }                                                             \
    }

#define DIST_TM_STATUS_OK         0
#define DIST_TM_STATUS_TIMEOUT    1
#define DIST_TM_STATUS_UNREACH    2

/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

STATUS distTBufLibInit (int numTBufs);
DIST_TBUF_HDR *distTBufAllocHdr (void);
int distTBufCopy (DIST_TBUF *pTBuf, int offset, char *dest, int nBytes);

#else    /* __STDC__ */

STATUS distTBufLibInit ();
DIST_TBUF_HDR *distTBufAllocHdr ();
void distTBufCopy ();

#endif    /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif  /* __INCdistTBufLibPh */


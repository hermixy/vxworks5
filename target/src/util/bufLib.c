/* bufLib.c - allocate/free a collection of fixed size memory buffers */

/* Copyright 1984-1995 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,04may95,ms  written.
*/

/*
DESCPRIPTION

This library contains routines to allocate and free a collection
of fixed size memory buffers.

It provides an alternative memory allocation scheme to the one provided
in memPartLib.
bufLib has several advantages over memPartLib:
   1) It is an order of magnitude faster than memPartLib.
   2) It can be used at interrupt time.
   3) There is no per-buffer bookkeeping overhead.
   4) There is no memory fragmentation.
bufLib has several disadvantages compared with memPartLib:
   1) It only handles fixed size blocks of memory.
   2) The blocks it handles must be at least eight bytes (2*sizeof(void *)).

A memory buffer object is initialized with
    BUF_POOL bufPool;
    bufPoolInit (&bufPool, pBufs, bufSize, numBufs);

Memory is then allocated/freed with
    pBuf = bufAlloc (pBufPool);
    bufFree  (pBufPool, pData);
*/

#define FREE_MAGIC	0x753a012b
#define ALLOC_MAGIC	0x2e63d498

typedef struct
    {
    char *	next;		/* next free buffer */
    int		type;		/* FREE_MAGIC or ALLOC_MAGIC */
    } BUFFER;

/* includes */

#include "vxWorks.h"
#include "intLib.h"
#include "bufLib.h"


#if DEBUG

extern logMsg();

/******************************************************************************
*
* bufPoolShow - only used for debugging. XXX - remove.
*/ 
void bufPoolShow (BUF_POOL * pBufPool)
    {
    char * pBuf;
    int ix;

    logMsg ("pBufs = 0x%x, numBufs = %d, bufSize = %d\n",
		pBufPool->pBufs, pBufPool->numBufs, pBufPool->bufSize);

    for (ix = 0; ix < pBufPool->numBufs; ix++)
	{
	pBuf = pBufPool->pBufs + ix * pBufPool->bufSize;
	logMsg ("0x%x	%s\n", (int)pBuf,
		((BUFFER *)pBuf)->type == FREE_MAGIC ? "Free" : "Allocated");
	}
    logMsg ("\n");
    }

#endif

/******************************************************************************
*
* bufPoolInit - initialize a buffer pool
*
* bufLib manages I/O buffers for the WDB agent. Since the agent
* can't use malloc(), the buffers are installed at library init time.
* Currently, the agent uses mbufs.
*/ 

void bufPoolInit
    (
    BUF_POOL *	pBufPool,		/* buffer pool to initialize */
    char *	pBufs,			/* array of buffers to use */
    int		numBufs,		/* number of bufs in the array */
    int		bufSize			/* size of each buffer */
    )
    {
    int ix;

    pBufPool->pBufs	= pBufs;
    pBufPool->numBufs	= numBufs;
    pBufPool->bufSize	= bufSize;
    pBufPool->pFreeBufs	= pBufs;

    for (ix = 0; ix < numBufs - 1; ix++)
	{
        ((BUFFER *)(pBufs + bufSize*ix))->next = pBufs + bufSize * (ix + 1);
	((BUFFER *)(pBufs + bufSize*ix))->type = FREE_MAGIC;
	}

    ((BUFFER *)(pBufs + bufSize*ix))->next = NULL;
    ((BUFFER *)(pBufs + bufSize*ix))->type = FREE_MAGIC;
    }

/******************************************************************************
*
* bufAlloc - allocate a buffer.
*
* RETURNS: a char *, or NULL if none are available.
*/ 

char * bufAlloc
    (
    BUF_POOL * pBufPool			/* buffer pool from which to alloc */
    )
    {
    int 	lockKey;
    char *	pThisBuf;

    lockKey = intLock ();

    if (pBufPool->pFreeBufs == NULL)
	{
	intUnlock (lockKey);
	return (NULL);
	}

    pThisBuf = pBufPool->pFreeBufs;
    pBufPool->pFreeBufs = ((BUFFER *)pBufPool->pFreeBufs)->next;
    ((BUFFER *)pThisBuf)->type = ALLOC_MAGIC;
    ((BUFFER *)pThisBuf)->next = NULL;
    intUnlock (lockKey);

    return (pThisBuf);
    }

/******************************************************************************
*
* bufFree - return a buffer to the buffer pool.
*/ 

void bufFree
    (
    BUF_POOL *	pBufPool,	/* buffer pool to which we free */
    char *	pBuf		/* buffer to free */
    )
    {
    int lockKey;

    lockKey = intLock();

    /* valid buffer? */

    if ((((BUFFER *)pBuf)->type == FREE_MAGIC) ||
	((unsigned int)pBuf < (unsigned int)pBufPool->pBufs) ||
	((unsigned int)pBuf >= (unsigned int)pBufPool->pBufs +
				pBufPool->bufSize * pBufPool->numBufs) ||
	(((unsigned int)pBuf - (unsigned int)pBufPool->pBufs)
			% pBufPool->bufSize != 0))
	{
	intUnlock (lockKey);
	return;
	}


    ((BUFFER *)pBuf)->next	= pBufPool->pFreeBufs;
    pBufPool->pFreeBufs		= pBuf;
    ((BUFFER *)pBuf)->type	= FREE_MAGIC;

    intUnlock (lockKey);
    }

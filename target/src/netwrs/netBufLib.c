/* netBufLib.c - network buffer library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01r,07may02,kbw  man page edits
01q,15oct01,rae  merge from truestack ver 01w, base 01n (SPR #65195 etc.)
01p,08feb01,kbw  fixing a man page format problem
01o,07feb01,spm  removed unused garbage collection code; updated documentation
01n,26aug98,fle  doc : fixed a proc header trouble with netPoolInit
01m,25aug98,n_s  M_WAIT will only call _pNetBufCollect 1 time. spr #22104
01l,12dec97,kbw  making minor man page fixes
01k,11dec97,vin  added netMblkOffsetToBufCopy part of SPR 9563.
01j,08dec97,vin  added netMblkChainDup() SPR 9966.
01i,05dec97,vin  changed netMblkClFree to netMblkFree in netTupleGet()
01h,03dec97,vin  added netTupleGet() SPR 9955, added some unsupported routines 
01g,13nov97,vin  code clean up.
01f,25oct97,kbw  making minor man page fixes
01e,08oct97,vin  corrected clBlkFree()
01d,06oct97,vin  fixed a man page.
01c,30sep97,vin  changed MSIZE to M_BLK_SZ.
01b,19sep97,vin  added cluster Blks, fixed bugs, removed reference count
		 pointers.
01a,08aug97,vin  written.
*/

/*
DESCRIPTION

This library contains routines that you can use to organize and maintain
a memory pool that consists of pools of `mBlk' structures, pools of `clBlk'
structures, and pools of clusters.  The `mBlk' and `clBlk' structures
are used to manage the clusters.  The clusters are containers for the data
described by the `mBlk' and `clBlk' structures.

These structures and the various routines of this library constitute a
buffering API that has been designed to meet the needs both of network 
protocols and network device drivers.
 
The `mBlk' structure is the primary vehicle for passing data between
a network driver and a protocol.  However, the `mBlk' structure must
first be properly joined with a `clBlk' structure that was previously
joined with a cluster.  Thus, the actual vehicle for passing data is
not merely an `mBlk' structure but an `mBlk'-`clBlk'-cluster
construct.
 
To use this feature, include the following component:
INCLUDE_NETWRS_NETBUFLIB


INCLUDE FILES: netBufLib.h

INTERNAL
	 ------------
        |mBlk       |
        |manages    |
	|clusterBlk |
        |           |
	|	    |
        ------------|\    -------------
	              \   |clBlk      |
		       \  |manages the|
			\ |cluster.   |
		         \|clRefCnt=1 |
			  ------------|
			           \
				    \
				     \  ------------|
				      \|Cluster     |
				       |	    |
				       |	    |
				       |	    |
				       |	    |
				       |-------------


Two mBlks sharing the same cluster:
-----------------------------------
                                                 
         -----------|                      |-----------|
        | mBlk      |                      | mBlk      |
        |           |                      |           |
        |           |                      |           |
        |           |                      |           |
        |           |                      |           |
        |-----------\                      /-----------|
                     \                    /    
                      \                  /
                       \                /
                        \|------------|/
                         | clBlk      |               
                         |            |                
                         |clRefCnt = 2|                
                         |            |                
                         |            |                
                         |------------\                
                                       \               
                                       |---------------|
                                       | cluster       |
                                       |               |
                                       |               |
                                       |---------------|
*/

/* includes */

#include "vxWorks.h"
#include "stdlib.h"
#include "intLib.h"
#include "string.h"
#include "semaphore.h"
#include "memLib.h"
#include "errnoLib.h"
#include "netBufLib.h"
#include "private/semLibP.h"
#include "memPartLib.h"

/* Virtual Stack Support */

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif

/* defines */
#undef NETBUF_DEBUG

#ifdef NETBUF_DEBUG
#include "logLib.h"
#endif /* NETBUF_DEBUG */

#define FROM_KHEAP      1   /* let _poolInit() use KHEAP_ALLOC */
#define FROM_HOMEPDHEAP 0   /* let _poolInit() use calloc() */ 

/* global */

VOIDFUNCPTR 		_pNetBufCollect = NULL; /* protocol specific routine */

/* extern */
IMPORT int ffsMsb ();

/* forward declarations */
LOCAL STATUS		_poolInit (NET_POOL_ID pNetPool, M_CL_CONFIG *
                                   pMclBlkConfig, CL_DESC * pClDescTbl,
                                   int clDescTblNumEnt, BOOL fromKheap);
LOCAL M_BLK_ID 		_mBlkCarve (NET_POOL_ID pNetPool, int num,
                                    char * pool);
LOCAL CL_BLK_ID 	_clBlkCarve (int num, char * pool);
LOCAL CL_BUF_ID 	_clPoolCarve (CL_POOL_ID pClPool, int num, int clSize,
                                      char * pool);
LOCAL STATUS 		_memPoolInit (int num, int unitSize, int headerSize,
                                      char * memArea);
LOCAL void		_mBlkFree (NET_POOL_ID pNetPool, M_BLK_ID pMblk);
LOCAL void 		_clBlkFree (CL_BLK_ID pClBlk);
LOCAL void 		_clFree (NET_POOL_ID pNetPool, char * pClBuf);
LOCAL M_BLK_ID 		_mBlkClFree (NET_POOL_ID pNetPool, M_BLK_ID pMblk);
LOCAL M_BLK_ID 		_mBlkGet (NET_POOL_ID pNetPool, int canWait,
                                  UCHAR type);
LOCAL CL_BLK_ID		_clBlkGet (NET_POOL_ID pNetPool, int canWait);
LOCAL char *	 	_clusterGet (NET_POOL_ID pNetPool, CL_POOL_ID pClPool);
LOCAL STATUS 		_mClGet (NET_POOL_ID pNetPool, M_BLK_ID pMblk,
                                 int bufSize, int canWait, BOOL bestFit);
LOCAL CL_POOL_ID 	_clPoolIdGet (NET_POOL_ID pNetPool, int	bufSize,
                                      BOOL bestFit);

LOCAL POOL_FUNC dfltFuncTbl =		/* default pool function table */
    {
    _poolInit,
    _mBlkFree,
    _clBlkFree,
    _clFree,
    _mBlkClFree,
    _mBlkGet,
    _clBlkGet,
    _clusterGet,
    _mClGet,
    _clPoolIdGet,
    };

/*
 * this is a global pointer to a function table provided as a back door
 * for users who want to use a different allocation scheme for defaults
 * By initializing this _pNetPoolFuncTbl at runtime before initialization of
 * the network stack, one can change the default function table.  The system
 * pools use the default function table.
 */
   
POOL_FUNC * 	_pNetPoolFuncTbl = &dfltFuncTbl;

/*******************************************************************************
*  
* _poolInit - initialize the net pool
*
* This function initializes a net pool
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

LOCAL STATUS _poolInit
    (
    NET_POOL_ID		pNetPool,	/* pointer to a net pool */
    M_CL_CONFIG *	pMclBlkConfig,	/* pointer to mBlk,clBlk config */
    CL_DESC *		pClDescTbl,	/* pointer to cluster desc table */
    int			clDescTblNumEnt, /* number of cluster desc entries */
    BOOL                fromKheap       /* 1:KHEAP_ALLOC  0:malloc/calloc */
    )
    {
    int 		ix;		/* index variable */
    int 		iy; 		/* index variable */
    int 		log2Size;	/* cluster size to the base 2 */
    CL_DESC * 		pClDesc;	/* pointer to the cluster descriptor */
    CL_POOL * 		pClPool; 	/* pointer to the cluster pool */
    char * 		memArea;

    bzero ((char *) pNetPool->clTbl, sizeof (pNetPool->clTbl));

    if (fromKheap)
	{
	if ((pNetPool->pPoolStat = (M_STAT *) KHEAP_ALLOC(sizeof (M_STAT))) == NULL)
	    return (ERROR);
	bzero ((char *)pNetPool->pPoolStat, sizeof (M_STAT));
	}
    else {
	if ((pNetPool->pPoolStat = (M_STAT *) calloc (1, sizeof (M_STAT))) == NULL)
	    return (ERROR);
        }
    pNetPool->pmBlkHead = NULL;

    if (pMclBlkConfig != NULL)	/* if mbuf config is specified */
        {

        if (pMclBlkConfig->memSize <
            ((pMclBlkConfig->mBlkNum *  (M_BLK_SZ + sizeof(long))) +
             (pMclBlkConfig->clBlkNum * CL_BLK_SZ)))
            {
            errno = S_netBufLib_MEMSIZE_INVALID;
            goto poolInitError;
            }

        /* initialize the memory pool */

        if (_memPoolInit (pMclBlkConfig->mBlkNum, M_BLK_SZ, sizeof(void *),
                          pMclBlkConfig->memArea
                          ) != OK)
            {
            goto poolInitError;
            }

        /* carve up the mBlk pool */

        pNetPool->pmBlkHead = _mBlkCarve (
        				 pNetPool,
                                         pMclBlkConfig->mBlkNum,
                                         pMclBlkConfig->memArea
                                         );

        /* increment free mBlks  and number of mBlks */
        pNetPool->mBlkCnt		      = pMclBlkConfig->mBlkNum;
        pNetPool->mBlkFree		      = pMclBlkConfig->mBlkNum;
        pNetPool->pPoolStat->mTypes [MT_FREE] = pMclBlkConfig->mBlkNum;
        pNetPool->pPoolStat->mNum 	      = pMclBlkConfig->mBlkNum;

        memArea = (char * )((int)pMclBlkConfig->memArea +
                            (pMclBlkConfig->mBlkNum * (M_BLK_SZ +
                                                       sizeof(long))));

        if (pMclBlkConfig->clBlkNum > 0)
            {
            /* initialize the memory pool */

            if (_memPoolInit (pMclBlkConfig->clBlkNum, CL_BLK_SZ, 0, memArea)
                != OK)
                goto poolInitError;


            pNetPool->pClBlkHead = _clBlkCarve (
                                               pMclBlkConfig->clBlkNum,
                                               memArea
                                               );

            if (pNetPool->pClBlkHead == NULL)
                goto poolInitError;
            }
        }

    /* initialize clusters */
    pNetPool->clMask   = 0;
    pNetPool->clLg2Max = 0;
    pNetPool->clLg2Min = 0;

    for (pClDesc = pClDescTbl, ix = 0 ; (pClDesc != NULL) &&
             (pClDesc->clNum > 0) && (ix < clDescTblNumEnt); ix++, pClDesc++)
	{
	/* range check cluster type */

	if ((pClDesc->clSize < CL_SIZE_MIN) || (pClDesc->clSize > CL_SIZE_MAX))
            {
#ifdef NETBUF_DEBUG
	    logMsg ("poolInit -- Invalid cluster type\n", 0, 0, 0, 0, 0, 0);
#endif /* NETBUF_DEBUG */
            errno = S_netBufLib_CLSIZE_INVALID;
            goto poolInitError;
            }

	log2Size 	 = SIZE_TO_LOG2(pClDesc->clSize);
	pNetPool->clMask |= CL_LOG2_TO_CL_SIZE(log2Size); /* set the mask */

	if ((pNetPool->clLg2Max == 0) && (pNetPool->clLg2Min == 0))
	    {
	    pNetPool->clLg2Min 	= log2Size;
	    pNetPool->clLg2Max 	= log2Size;
	    }

	pNetPool->clLg2Max  = max(log2Size, pNetPool->clLg2Max);
	pNetPool->clLg2Min  = min(log2Size, pNetPool->clLg2Min);
        pNetPool->clSizeMax = CL_LOG2_TO_CL_SIZE(pNetPool->clLg2Max);
        pNetPool->clSizeMin = CL_LOG2_TO_CL_SIZE(pNetPool->clLg2Min);

	if (fromKheap)
	    {
	    if ((pClPool = (CL_POOL *) KHEAP_ALLOC(sizeof(CL_POOL))) == NULL)
		{
#ifdef NETBUF_DEBUG
		logMsg ("poolInit -- cluster pool allocation\n", 0, 0, 0, 0, 0, 0);
#endif /* NETBUF_DEBUG */
		errno = S_netBufLib_NO_SYSTEM_MEMORY;
		goto poolInitError;
		}
	    bzero ((char *)pClPool, sizeof (CL_POOL));
	    }
	else {
	    if ((pClPool = (CL_POOL *) calloc (1, sizeof(CL_POOL))) == NULL)
		{
#ifdef NETBUF_DEBUG
		logMsg ("poolInit -- cluster pool allocation\n", 0, 0, 0, 0, 0, 0);
#endif /* NETBUF_DEBUG */
		errno = S_netBufLib_NO_SYSTEM_MEMORY;
		goto poolInitError;
		}
	    }

        pNetPool->clTbl [CL_LOG2_TO_CL_INDEX(log2Size)] = pClPool;

	for (iy = (log2Size - 1);
	    ((!(pNetPool->clMask & CL_LOG2_TO_CL_SIZE(iy))) &&
	     (CL_LOG2_TO_CL_INDEX(iy) >= CL_INDX_MIN))  ; iy--)
	    {
	    pNetPool->clTbl [CL_LOG2_TO_CL_INDEX(iy)] = pClPool;
	    }

	pClPool->clSize	  = pClDesc->clSize;
        pClPool->clLg2	  = log2Size;
        pClPool->pNetPool = pNetPool; /* initialize the back pointer */

        if (pClDesc->memSize < (pClDesc->clNum * (pClDesc->clSize +
                                                  sizeof(int))))
            {
            errno = S_netBufLib_MEMSIZE_INVALID;
            goto poolInitError;
            }

        if (_memPoolInit (pClDesc->clNum, pClDesc->clSize,
                          sizeof (void *) , pClDesc->memArea) != OK)
            goto poolInitError;

        pClPool->pClHead = _clPoolCarve (pClPool, pClDesc->clNum,
                                         pClDesc->clSize, pClDesc->memArea);

        if (pClPool->pClHead == NULL)
            {
            goto poolInitError;
            }

        pClPool->clNum	   = pClDesc->clNum;
        pClPool->clNumFree = pClDesc->clNum;
	}

    return (OK);

    poolInitError:

    netPoolDelete (pNetPool);
    return (ERROR);
    }

/*******************************************************************************
*
* _mBlkCarve - carve up the mBlk pool.
*
* This function carves the the mBlks from a pre allocated pool.
*
* RETURNS: M_BLK_ID or NULL.
*
* NOMANUAL
*/

LOCAL M_BLK_ID _mBlkCarve
    (
    NET_POOL_ID		pNetPool,	/* pointer to net pool */
    int 		num,		/* number of units to allocate */
    char *		pool		/* pre allocated memory area */
    )
    {
    M_BLK_ID		pMblk = NULL;
    int 		ix;
    int			size;		/* size of each unit */
    M_BLK_ID *		ppMblk;

    size = M_BLK_SZ + sizeof (void *);	/* accomodate for netPoolId */

    ppMblk = &pMblk;

    for (ix = 0; ix < num; ++ix)
	{
	*ppMblk 		 = (M_BLK_ID) (pool + sizeof (void *));
        *((NET_POOL_ID *)(pool)) = pNetPool;
	(*ppMblk)->mBlkHdr.mType = MT_FREE;
        ppMblk 			 = &((*ppMblk)->mBlkHdr.mNext);
	pool 			 += size;
	}

    return (pMblk);
    }

/*******************************************************************************
*
* _clBlkCarve - carve up the clBlk pool.
*
* This function carves the the clBlks from a pre allocated pool.
*
* RETURNS: CL_BLK_ID or NULL.
*
* NOMANUAL
*/

LOCAL CL_BLK_ID _clBlkCarve
    (
    int 		num,		/* number of units to allocate */
    char *		pool		/* pre allocated memory area */
    )
    {
    CL_BLK_ID		pClBlk = NULL;
    int 		ix;
    CL_BLK_ID *		ppClBlk;

    ppClBlk = &pClBlk;

    for (ix = 0; ix < num; ++ix)
	{
	*ppClBlk = (CL_BLK_ID) (pool);
        ppClBlk  = &((*ppClBlk)->clNode.pClBlkNext);
	pool 	 += CL_BLK_SZ;
	}

    return (pClBlk);
    }

/*******************************************************************************
*
* _clPoolCarve - carve up the cluster pool
*
* This function carves the clusters from a pre allocated pool.
* Each cluster maintains an 4 byte header info.  This header info contains
* a place to hold the pointer to the cluster pool.
* This header info is prepended to the cluster buffer
* and is hidden from the user.  The header info is used at the time of
* freeing the cluster.  This function returns the pointer the cluster buffer
* chain.
*
* RETURNS: CL_BUF_ID or NULL.
*
* NOMANUAL
*/

LOCAL CL_BUF_ID _clPoolCarve
    (
    CL_POOL_ID		pClPool,	/* pointer to the cluster pool */
    int 		num,		/* number of units to allocate */
    int			clSize,		/* cluster size */
    char *		pool		/* pre allocated memory area */
    )
    {
    CL_BUF_ID		pClBuf = NULL; 	/* pointer to cluster buffer */
    FAST int 		ix;		/* counter */
    CL_BUF_ID *		ppClBuf;	/* ptr to ptr to cluster buffer */

    clSize += sizeof (void *);		/* make space for pool pointer */

    ppClBuf = &pClBuf;

    for (ix = 0; ix < num; ++ix)
	{
	*ppClBuf		 = (CL_BUF_ID) (pool + sizeof (void *));
        *((CL_POOL_ID *) (pool)) = pClPool;
        ppClBuf 		 = &((*ppClBuf)->pClNext);
	pool 			 += clSize;
	}

    return (pClBuf);
    }

/*******************************************************************************
*
* _memPoolInit - initialize the memory
*
* This function initialized the memory passed to it.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

LOCAL STATUS _memPoolInit
    (
    int 		num,		/* number of units to allocate */
    int			unitSize,	/* size of each unit */
    int			headerSize,	/* hidden header size */
    char *		memArea		/* pre allocated memory area */
    )
    {
    if (((int) unitSize & ~(sizeof (void *) - 1)) != unitSize )
        {
        errno = S_netBufLib_MEMSIZE_UNALIGNED;
        return (ERROR);				/* unaligned size */
        }

    unitSize += headerSize;		/* adjust for NET_POOL_ID */

    if (memArea == NULL)
        {
        errno = S_netBufLib_MEMAREA_INVALID;
        return (ERROR);
        }

    if (((int) memArea & ~(sizeof (void *) - 1)) != (int)memArea )
        {
        errno = S_netBufLib_MEM_UNALIGNED;
        return (ERROR);				/* unaligned memory */
        }

    bzero ((char *)memArea, (num * unitSize));

    return (OK);
    }

/*******************************************************************************
*
* _mBlkFree - free the given mBlk
*
* This function frees the given mBlk and returns it to the mBlk pool.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void _mBlkFree
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    M_BLK_ID	 	pMblk		/* mBlk to free */
    )
    {
    FAST int		ms;

    pMblk->mBlkHdr.mNextPkt 	= NULL;

    ms = intLock ();
    pNetPool->pPoolStat->mTypes [pMblk->mBlkHdr.mType]--;
    pNetPool->pPoolStat->mTypes [MT_FREE]++;
    pMblk->mBlkHdr.mType 	= MT_FREE;
    pMblk->mBlkHdr.mNext 	= pNetPool->pmBlkHead;
    pNetPool->pmBlkHead 	= pMblk; /* add the mbuf to the free list */

    intUnlock (ms);
    }

/*******************************************************************************
*
* _clBlkFree - free the given cluster Blk
*
* This function frees the given clBlk and returns it to the cluster Blk pool.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void _clBlkFree
    (
    CL_BLK_ID	 	pClBlk		/* clBlk to free */
    )
    {
    FAST int		level;
    CL_POOL_ID		pClPool;
    CL_BUF_ID		pClBuf;
    NET_POOL_ID		pNetPool;	/* pointer to the net pool */
    
    pNetPool = pClBlk->pNetPool;
    pClBuf   = (CL_BUF_ID)pClBlk->clNode.pClBuf;

    level = intLock ();			/* lock interrupts briefly */
    if (pClBuf == NULL)			/* no cluster attached */
        goto returnClBlk;

    else if (--(pClBlk->clRefCnt) == 0)
        {
        if (pClBlk->pClFreeRtn != NULL)
            {	/* call the free routine if associated with one */
            intUnlock (level);
	    (*pClBlk->pClFreeRtn) (pClBlk->clFreeArg1, pClBlk->clFreeArg2,
                                   pClBlk->clFreeArg3);
            level = intLock ();
            }
        else
            {
            /* return the cluster to its pool */
            pClPool 			= CL_BUF_TO_CL_POOL (pClBuf);
            pClBuf->pClNext 		= pClPool->pClHead;
            pClPool->pClHead		= pClBuf;
            /* update mask */
            pClPool->pNetPool->clMask 	|= CL_LOG2_TO_CL_SIZE(pClPool->clLg2);
            pClPool->clNumFree++;
	    }
        goto returnClBlk;
	}
    else
        goto clBlkFreeEnd;

    returnClBlk:
    /* free the cluster blk and add it to the free list */
    pClBlk->clNode.pClBlkNext 	= pNetPool->pClBlkHead;
    pNetPool->pClBlkHead 	= pClBlk;

    clBlkFreeEnd:
    intUnlock (level);
    }

/*******************************************************************************
*
* _clFree - free a  cluster of a given size.
*
* This function frees a cluster of given size in a net pool
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void _clFree
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    char * 		pClBuf		/* pointer to the cluster buffer */
    )
    {
    CL_POOL_ID		pClPool;
    FAST int		level;

    pClPool = CL_BUF_TO_CL_POOL (pClBuf);

    level = intLock ();
    ((CL_BUF_ID)pClBuf)->pClNext = pClPool->pClHead;
    pClPool->pClHead		 = (CL_BUF_ID)pClBuf;

    /* update mask */
    pClPool->pNetPool->clMask 	|= CL_LOG2_TO_CL_SIZE(pClPool->clLg2);
    pClPool->clNumFree++;

    intUnlock (level);
    }

/*******************************************************************************
*
* _mBlkClFree - free an mBlk/cluster pair.
*
* This function frees a mBlk/cluster pair.  This function returns a pointer
* to the next mBlk which is connected to the current one.  This routine will
* free the cluster only if it is attached to the mBlk.
*
* RETURNS: M_BLK_ID or NULL.
*
* NOMANUAL
*/

LOCAL M_BLK_ID _mBlkClFree
    (
    NET_POOL_ID		pNetPool,	/* pointer to net pool */
    M_BLK_ID 		pMblk		/* pointer to the mBlk */
    )
    {
    M_BLK_ID		pMblkNext;	/* pointer to the next mBlk */

    if (pMblk->mBlkHdr.mType == MT_FREE)
        {
#ifdef NETBUF_DEBUG
        logMsg ("mBlkClFree -- Invalid mBlk\n", 0, 0, 0, 0, 0, 0);
#endif /* NETBUF_DEBUG */
        errno = S_netBufLib_MBLK_INVALID;
        return (NULL);
        }

    pMblkNext = pMblk->mBlkHdr.mNext;

    /* free the cluster first if it is attached to the mBlk */

    if (M_HASCL(pMblk))
        netClBlkFree (pMblk->pClBlk->pNetPool, pMblk->pClBlk);

    _mBlkFree (pNetPool, pMblk); 		/* free the mBlk */

    return (pMblkNext);

    }

/*******************************************************************************
*
* _mBlkGet - get a free mBlk
*
* This routine returns a free mBlk if one is available. If the <canWait>
* parameter is set to M_WAIT and an mBlk is not immediately available, the
* routine repeats the allocation attempt after calling any installed garbage
* collection routine.
*
* RETURNS: M_BLK_ID or NULL if none available
*
* NOMANUAL
*/

LOCAL M_BLK_ID _mBlkGet
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    int			canWait,	/* M_WAIT/M_DONTWAIT */
    UCHAR		type		/* mBlk type */
    )
    {
    M_BLK_ID	 	pMblk = NULL;	/* pointer to mbuf */
    int 		level;		/* level of interrupt */

    reTry:
    level = intLock();			/* lock interrupts very briefly */
    if ((pMblk = pNetPool->pmBlkHead) != NULL)
        {
        pNetPool->pmBlkHead = pMblk->mBlkHdr.mNext;
        pNetPool->pPoolStat->mTypes [MT_FREE]--;
        pNetPool->pPoolStat->mTypes [type]++;
        intUnlock (level); 			/* unlock interrupts */

        if (pMblk->mBlkHdr.mType != MT_FREE)
            {
#ifdef NETBUF_DEBUG
            logMsg("mBlkGet free error:\n", 0, 0, 0, 0, 0, 0);
#endif /* NETBUF_DEBUG */
            errno = S_netBufLib_MBLK_INVALID;
            return (NULL);
            }

        pMblk->mBlkHdr.mType	= type;
        pMblk->mBlkHdr.mNext 	= NULL;
        pMblk->mBlkHdr.mNextPkt = NULL;
        pMblk->mBlkHdr.mFlags 	= 0;
        }
    else /* if (canWait != M_ISR) */
        {
        if (canWait == M_WAIT)
            {
            if (_pNetBufCollect)
                {
                intUnlock (level);			/* unlock interrupts */
                (*_pNetBufCollect) (pNetPool->pPoolStat);
		canWait = M_DONTWAIT;
                goto reTry;
                }
            }
        pNetPool->pPoolStat->mDrops++;
        intUnlock (level);
	errnoSet (S_netBufLib_NO_POOL_MEMORY);
        }

    return (pMblk);
    }

/*******************************************************************************
*
* _clBlkGet - get a free clBlk
*
* This routine returns a free clBlk if one is available. If the <canWait>
* parameter is set to M_WAIT and a clBlk is not immediately available, the
* routine repeats the allocation attempt after calling any installed garbage
* collection routine.
*
* RETURNS: CL_BLK_ID or NULL.
*
* NOMANUAL
*/

LOCAL CL_BLK_ID _clBlkGet
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    int			canWait		/* M_WAIT/M_DONTWAIT */
    )
    {
    CL_BLK_ID	 	pClBlk = NULL;	/* pointer to mbuf */
    int 		level;		/* level of interrupt */

    reTry:
    level = intLock();			/* lock interrupts very briefly */
    if ((pClBlk = pNetPool->pClBlkHead) != NULL)
        {
        pNetPool->pClBlkHead = pClBlk->clNode.pClBlkNext;
        intUnlock (level); 			/* unlock interrupts */

        pClBlk->clNode.pClBuf = NULL;
        pClBlk->pClFreeRtn    = NULL;
        pClBlk->clRefCnt      = 0;
        pClBlk->pNetPool      = pNetPool; 	/* netPool originator */
        }
    else /* if (canWait != M_ISR) */
        {
        if (canWait == M_WAIT)
            {
            if (_pNetBufCollect)
                {
                intUnlock (level);			/* unlock interrupts */
                (*_pNetBufCollect) (pNetPool->pPoolStat);
		canWait = M_DONTWAIT;
                goto reTry;
                }
            }
        intUnlock (level);
       	errnoSet (S_netBufLib_NO_POOL_MEMORY);
	}

    return (pClBlk);
    }

/*******************************************************************************
*
* _clusterGet - get a new cluster of a given cluster pool.
*
* This function returns a cluster given a pool pointer.  The reference count
* for the cluster is incremented.
*
* RETURNS: pointer to a cluster or NULL
*
* NOMANUAL
*/

LOCAL char * _clusterGet
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    CL_POOL_ID 		pClPool 	/* ptr to the cluster pool */
    )
    {
    int		level;				/* level of interrupt */
    CL_BUF_ID	pClBuf;				/* ptr to the cluster buffer */

    level = intLock (); 			/* lock interrupts briefly */

    if (pClPool->pClHead == NULL)		/* return if no buffers */
        {
        intUnlock (level);
        return (NULL);
        }

    pClBuf = pClPool->pClHead;			/* update the head */

    if ((pClPool->pClHead = pClBuf->pClNext) == NULL)
        {	/* update the pool Mask */
        pNetPool->clMask &= ~(CL_LOG2_TO_CL_SIZE(pClPool->clLg2));
        }

    pClPool->clNumFree--; 			/* decrement the free count */
    pClPool->clUsage++;				/* increment the usage count */
    intUnlock (level);

    return ((char *)pClBuf);
    }

/*******************************************************************************
*
* mClGet - get a new mBlk/cluster pair.
*
* This function gets a free cluster from the NET_POOL and joins it with
* the mBlk passed to it.  An mBlk must be pre allocated and passed to this
* function.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

LOCAL STATUS _mClGet
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    M_BLK_ID	 	pMblk, 		/* mBlk to embed the cluster in */
    int			bufSize,	/* size of the buffer to get */
    int			canWait,	/* wait or dontwait */
    BOOL		bestFit		/* TRUE/FALSE */
    )
    {
    int			log2Size;	/* size of cluster to the base 2 */
    CL_POOL_ID 		pClPool; 	/* pointer to the cluster pool */
    CL_BUF_ID		pClBuf = NULL; 	/* pointer to the cluster buffer */
    CL_BLK_ID		pClBlk = NULL; 	/* pointer to the cluster blk */
    FAST int 		ms; 		/* integer for level */

    /* check pMblk */
    if ((pMblk == NULL) || ((pClBlk = _clBlkGet (pNetPool, canWait)) == NULL))
	goto mClGetError;

    /* check the boundary conditions */
    if (bufSize > pNetPool->clSizeMax)
	{
	if (bestFit)
            {
            errno = S_netBufLib_CLSIZE_INVALID;
	    goto mClGetError;
            }
	else
	    log2Size = pNetPool->clLg2Max;
	}
    else if (bufSize < pNetPool->clSizeMin)
	log2Size = pNetPool->clLg2Min;
    else
	{
	log2Size = SIZE_TO_LOG2(bufSize);
	if (bufSize > CL_LOG2_TO_CL_SIZE(log2Size))
	    log2Size++;
	}

    /* get the appropriate pool pointer */
    if ((pClPool = pNetPool->clTbl [CL_LOG2_TO_CL_INDEX (log2Size)]) == NULL)
	{
#ifdef NETBUF_DEBUG
	logMsg ("mClGet: Invalid cluster type\n", 0, 0, 0, 0, 0, 0);
#endif /* NETBUF_DEBUG */
        errno = S_netBufLib_CLSIZE_INVALID;
	goto mClGetError;
	}

    reTry:
    ms = intLock(); 			/* lock interrupts briefly */

    if (pClPool->pClHead == NULL)
        {
        /* pool has max clusters, find best fit or close fit */
        if (pNetPool->clMask >= CL_LOG2_TO_CL_SIZE(pClPool->clLg2))
            {	/* first fetch a cluster with a closest bigger size */
            bufSize  = CL_LOG2_TO_CL_SIZE(pClPool->clLg2);
            log2Size = pClPool->clLg2;
            while (bufSize <= CL_SIZE_MAX)
                {
                if (pNetPool->clMask & bufSize)
                    {
                    pClPool = pNetPool->clTbl [CL_LOG2_TO_CL_INDEX (log2Size)];
                    break;
                    }
                bufSize <<= 1;
                log2Size ++;
                }
            }

        /* if close fit then find closest lower size */
        else if (!bestFit && pNetPool->clMask)
            {
            pClPool = pNetPool->clTbl [CL_SIZE_TO_CL_INDEX(pNetPool->clMask)];
            }

        else if (canWait == M_WAIT)	/* want for buffers */
            {
            if (_pNetBufCollect)
                {
                intUnlock (ms);
                (*_pNetBufCollect) (pNetPool->pPoolStat);
		canWait = M_DONTWAIT;
                goto reTry;
                }
            }
        }

    if ((pClBuf = pClPool->pClHead))
	{
	/* if this is the last cluster available set the mask */
	if ((pClPool->pClHead = pClBuf->pClNext) == NULL)
            {
	    pNetPool->clMask &= ~(CL_LOG2_TO_CL_SIZE(pClPool->clLg2));
            }
	pClPool->clNumFree--;
	pClPool->clUsage++;
        intUnlock (ms);
	}
    else
	{
	pNetPool->pPoolStat->mDrops++;	/* no. times failed to find space */
        intUnlock (ms);
	errnoSet (S_netBufLib_NO_POOL_MEMORY);
	goto mClGetError;
	}

    pMblk->mBlkHdr.mData  = (caddr_t) pClBuf;
    pMblk->mBlkHdr.mFlags |= M_EXT;
    pClBlk->clNode.pClBuf = (caddr_t) pClBuf;
    pClBlk->clSize	  = pClPool->clSize;
    pClBlk->pClFreeRtn	  = NULL;
    pClBlk->clRefCnt	  = 1;
    pMblk->pClBlk 	  = pClBlk;

    return (OK); 		/* return OK */

    mClGetError:
    if (pClBlk != NULL)
        _clBlkFree (pClBlk);

    return (ERROR); 		/* return ERROR */
    }

/*******************************************************************************
*
* clPoolIdGet - return a pool Id for a given cluster Size
*
* This function returns a poolID for a given cluster Size.
* If this returns NULL then the corresponding pool has not been initialized.
*
* RETURNS: CL_POOL_ID or NULL.
*
* NOMANUAL
*/

LOCAL CL_POOL_ID _clPoolIdGet
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    int 		bufSize,	/* size of the buffer */
    BOOL		bestFit		/* TRUE/FALSE */
    )
    {
    int			log2Size;

    if (bufSize > pNetPool->clSizeMax)
	{
	log2Size = pNetPool->clLg2Max;
	if (bestFit)
	    return (NULL);
	}
    else if (bufSize < pNetPool->clSizeMin)
        {
	log2Size = pNetPool->clLg2Min;
        }
    else
	{
	log2Size = SIZE_TO_LOG2(bufSize);

	if (bufSize > CL_LOG2_TO_CL_SIZE(log2Size))
	    log2Size++;
	}
    return (pNetPool->clTbl [CL_LOG2_TO_CL_INDEX(log2Size)]);
    }

/*******************************************************************************
*
* netBufLibInit - initialize netBufLib
*
* This routine executes during system startup if INCLUDE_NETWORK is defined
* when the image is built. It links the network buffer library into the image.
*
* RETURNS: OK or ERROR.
*
*/

STATUS netBufLibInit (void)
    {
    return (OK);
    }

/*******************************************************************************
*
* netPoolInit - initialize a netBufLib-managed memory pool
*
* Call this routine to set up a netBufLib-managed memory
* pool.  Within this pool, netPoolInit() organizes several sub-pools: one
* for `mBlk' structures, one for `clBlk' structures, and as many cluster
* sub-pools are there are cluster sizes.  As input, this routine expects
* the following parameters:
* .IP <pNetPool> 15
* Expects a NET_POOL_ID that points to a previously allocated NET_POOL
* structure.  You need not initialize any values in this structure.  That
* is handled by netPoolInit().
* .IP <pMclBlkConfig>
* Expects a pointer to a previously allocated and initialized M_CL_CONFIG
* structure.  Within this structure, you must provide four
* values: 'mBlkNum', a count of `mBlk' structures; 'clBlkNum', a count
* of `clBlk' structures; 'memArea', a pointer to an area of memory
* that can contain all the `mBlk' and `clBlk' structures; and 'memSize', 
* the size of that memory area.  For example, you can set up an M_CL_CONFIG 
* structure as follows:
* .CS
*     M_CL_CONFIG mClBlkConfig = /@ mBlk, clBlk configuration table @/
*         {
*         mBlkNum     clBlkNum        memArea         memSize
*         ----------  ----            -------         -------
*         400,        245,            0xfe000000,     21260
*         };
* .CE
* You can 
* calculate the 'memArea' and 'memSize' values.  Such code could first 
* define a table as shown above, but set both 'memArea' and 'memSize' 
* as follows:
* .CS
* mClBlkConfig.memSize = (mClBlkConfig.mBlkNum * (M_BLK_SZ + sizeof(long))) +
*                        (mClBlkConfig.clBlkNum * CL_BLK_SZ);
* .CE
* You can set the memArea value to a pointer to private memory, or you can
* reserve the memory with a call to malloc().  For example:
* .CS
*     mClBlkConfig.memArea = malloc(mClBlkConfig.memSize);
* .CE
*
* The netBufLib.h file defines M_BLK_SZ as:
* .CS
*     sizeof(struct mBlk)
* .CE
* Currently, this evaluates to 32 bytes.  Likewise, this file 
* defines CL_BLK_SZ as:
* .CS 
*     sizeof(struct clBlk)
* .CE
* Currently, this evaluates to 32 bytes. 
*
* When choosing values for 'mBlkNum' and 'clBlkNum', remember that you need 
* as many `clBlk' structures as you have clusters (data buffers).  You also 
* need at least as many `mBlk' structures as you have `clBlk' structures, but 
* you will most likely need more. 
* That is because netBufLib shares buffers by letting multiple `mBlk' 
* structures join to the same `clBlk' and thus to its underlying cluster.  
* The `clBlk' keeps a count of the number of `mBlk' structures that 
* reference it.  
*
* .IP <pClDescTbl>
* Expects a pointer to a table of previously allocated and initialized CL_DESC
* structures.  Each structure in this table describes a single cluster pool.
* You need a dedicated cluster pool for each cluster size you want to support.
* Within each CL_DESC structure, you must provide four values: 'clusterSize',
* the size of a cluster in this cluster pool; 'num', the number of clusters 
* in this cluster pool; 'memArea', a pointer to an area of memory that can 
* contain all the clusters; and 'memSize', the size of that memory area.
* 
* Thus, if you need to support six different cluster sizes, this parameter
* must point to a table containing six CL_DESC structures.  For example,
* consider the following:
* .CS
*     CL_DESC clDescTbl [] =   /@ cluster descriptor table @/
*         {
*         /@
*         clusterSize        num     memArea         memSize
*         ----------         ----    -------         -------
*         @/
*         {64,               100,    0x10000,        6800},
*         {128,              50,     0x20000,        6600},
*         {256,              50,     0x30000,        13000},
*         {512,              25,     0x40000,        12900},
*         {1024,             10,     0x50000,        10280},
*         {2048,             10,     0x60000,        20520}
*         };
* .CE
* As with the 'memArea' and 'memSize' members in the M_CL_CONFIG structure,
* you can set these members of the CL_DESC structures by calculation after
* you create the table.  The formula would be as follows:
* .CS
*     clDescTbl[n].memSize = 
*         (clDescTbl[n].num * (clDescTbl[n].clusterSize + sizeof(long)));
* .CE
* The 'memArea' member can point to a private memory area that you know to be 
* available for storing clusters, or you can use malloc().
* .CS
*     clDescTbl[n].memArea =  malloc( clDescTbl[n].memSize ); 
* .CE
*
* Valid cluster sizes range from 64 bytes to 65536 bytes.  If there are 
* multiple cluster pools, valid sizes are further restricted to powers of
* two (for example, 64, 128, 256, and so on).  If there is only one cluster 
* pool (as is often the case for the memory pool specific to a single 
* device driver), there is no power of two restriction.  Thus, the cluster 
* can be of any size between 64 bytes and 65536 bytes on 4-byte alignment.  A 
* typical buffer size for Ethernet devices is 1514 bytes.  However, because
* a cluster size requires a 4-byte alignment, the cluster size for this 
* Ethernet buffer would have to be increased to at least 1516 bytes.
* 
* .IP <clDescTblNumEnt>
* Expects a count of the elements in the CL_DESC table referenced by 
* the <pClDescTbl> parameter.  This is a count of the number of cluster
* pools.  You can get this value using the NELEMENTS macro defined  
* in vxWorks.h.  For example:
* .CS
*     int clDescTblNumEnt = (NELEMENTS(clDescTbl));
* .CE
* .IP <pFuncTbl>
* Expects a NULL or a pointer to a function table.  This table contains 
* pointers to the functions used to manage the buffers in this memory pool.  
* Using a NULL for this parameter tells netBufLib to use its default function 
* table.  If you opt for the default function table, every `mBlk' and 
* every cluster is prepended by a 4-byte header (which is why the size 
* calculations above for clusters and `mBlk' structures contained an 
* extra 'sizeof(long)').  However, users need not concern themselves 
* with this header when accessing these buffers.  The returned pointers 
* from functions such as netClusterGet() return pointers to the start of
* data, which is just after the header.  
* .LP
* Assuming you have set up the configuration tables as shown above, a 
* typical call to netPoolInit() would be as follows:
* .CS
*     int clDescTblNumEnt = (NELEMENTS(clDescTbl));
*     NET_POOL 	netPool;
*     NET_POOL_ID 	pNetPool = &netPool;
*
*     if (netPoolInit (pNetPool, &mClBlkConfig, &clDescTbl [0], clDescTblNumEnt,
*         NULL) != OK)
*         return (ERROR);
*.CE
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, access to the contents of a memory pool is limited to 
* the protection domain within which you made the netPoolInit() call that 
* created the pool.  In addition, all parameters to a netPoolInit() call 
* must be valid within the protection domain from which you make the call. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK or ERROR.
*
* ERRNO:
*  S_netBufLib_MEMSIZE_INVALID
*  S_netBufLib_CLSIZE_INVALID
*  S_netBufLib_NO_SYSTEM_MEMORY
*  S_netBufLib_MEM_UNALIGNED
*  S_netBufLib_MEMSIZE_UNALIGNED
*  S_netBufLib_MEMAREA_INVALID
*
* SEE ALSO: netPoolDelete()
*/

STATUS netPoolInit
    (
    NET_POOL_ID		pNetPool,	 /* pointer to a net pool */
    M_CL_CONFIG *	pMclBlkConfig,	 /* pointer to a mBlk configuration */
    CL_DESC *		pClDescTbl,	 /* pointer to cluster desc table */
    int			clDescTblNumEnt, /* number of cluster desc entries */
    POOL_FUNC *		pFuncTbl	 /* pointer to pool function table */
    )
    {
    if (pNetPool == NULL)
        return (ERROR);

    if (pFuncTbl != NULL)
        pNetPool->pFuncTbl = pFuncTbl;
    else
        pNetPool->pFuncTbl = _pNetPoolFuncTbl;	/* default func table */

    return (poolInit (pNetPool, pMclBlkConfig, pClDescTbl,
        clDescTblNumEnt, FROM_HOMEPDHEAP)); /* allocate from the homePD's heap */
    }

/*******************************************************************************
*
* netPoolKheapInit - kernel heap version of netPoolInit()
* 
* This initializes a netBufLib-managed memory pool from Kernel heap.
* See netPoolInit() for more detail.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK or ERROR.
*
* ERRNO:
*  N/A
*
* SEE ALSO: netPoolInit(), netPoolDelete()
*/

STATUS netPoolKheapInit
    (
    NET_POOL_ID		pNetPool,	 /* pointer to a net pool */
    M_CL_CONFIG *	pMclBlkConfig,	 /* pointer to a mBlk configuration */
    CL_DESC *		pClDescTbl,	 /* pointer to cluster desc table */
    int			clDescTblNumEnt, /* number of cluster desc entries */
    POOL_FUNC *		pFuncTbl	 /* pointer to pool function table */
    )
    {
    if (pNetPool == NULL)
        return (ERROR);

    if (pFuncTbl != NULL)
        pNetPool->pFuncTbl = pFuncTbl;
    else
        pNetPool->pFuncTbl = _pNetPoolFuncTbl;	/* default func table */

    return (poolInit (pNetPool, pMclBlkConfig, pClDescTbl,
        clDescTblNumEnt, FROM_KHEAP)); /* allocate from kernel heap */
    }

/*******************************************************************************
*
* netPoolDelete - delete a memory pool
*
* This routine deletes the specified netBufLib-managed memory pool.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK or ERROR.
*
* ERRNO:
*  S_netBufLib_NETPOOL_INVALID
*/

STATUS netPoolDelete
    (
    NET_POOL_ID		pNetPool	/* pointer to a net pool */
    )
    {
    int			ix;		/* index variable */
    int			iy;		/* index variable */
    CL_POOL_ID		pClPool;	/* pointer to the pool */

    if (pNetPool->pPoolStat == NULL)
        {
        errno = S_netBufLib_NETPOOL_INVALID;
        return (ERROR);			/* pool already deleted */
        }

    free (pNetPool->pPoolStat);

    for (ix = 0; ix < CL_TBL_SIZE; ix++)
        {
        pClPool = pNetPool->clTbl [ix];

        if (pClPool != NULL)
            {
            for (iy = ix + 1; iy < CL_TBL_SIZE; iy++)
                {
                if (pClPool == pNetPool->clTbl [iy])
                    pNetPool->clTbl [iy] = NULL;
                }
            free (pClPool);
            }
        }

    bzero ((char *)pNetPool, sizeof(NET_POOL));	/* zero the structure */

    return (OK);
    }

/*******************************************************************************
*
* netMblkFree - free an `mBlk' back to its memory pool
*
* This routine frees the specified `mBlk' back to the specified memory pool.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: N/A
*/

void netMblkFree
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    M_BLK_ID	 	pMblk		/* mBlk to free */
    )
    {
    if (pNetPool == NULL || pNetPool->pFuncTbl == NULL ||
        pNetPool->pFuncTbl->pMblkFreeRtn == NULL)
        return;

    (*pNetPool->pFuncTbl->pMblkFreeRtn) (pNetPool, pMblk);
    }

/*******************************************************************************
*
* netClBlkFree - free a `clBlk'-cluster construct back to the memory pool
*
* This routine decrements the reference counter in the specified `clBlk'. 
* If the reference count falls to zero, this routine frees both the `clBlk'
* and its associated cluster back to the specified memory pool.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: N/A
*/

void netClBlkFree
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    CL_BLK_ID	 	pClBlk		/* pointer to the clBlk to free */
    )
    {
    pNetPool 		= pClBlk->pNetPool;
    
    if (pNetPool == NULL || pNetPool->pFuncTbl == NULL ||
        pNetPool->pFuncTbl->pClBlkFreeRtn == NULL)
        return;

    (*pNetPool->pFuncTbl->pClBlkFreeRtn) (pClBlk);
    }

/*******************************************************************************
*
* netClFree - free a cluster back to the memory pool
*
* This routine returns the specified cluster buffer back to the specified
* memory pool.  
* 
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: N/A
*/

void netClFree
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    UCHAR * 		pClBuf		/* pointer to the cluster buffer */
    )
    {
    if (pNetPool == NULL || pNetPool->pFuncTbl == NULL ||
        pNetPool->pFuncTbl->pClFreeRtn == NULL)
        return;

    (*pNetPool->pFuncTbl->pClFreeRtn) (pNetPool, pClBuf);
    }

/*******************************************************************************
*
* netMblkClFree - free an `mBlk'-`clBlk'-cluster construct
*
* For the specified `mBlk'-`clBlk'-cluster construct, this routine
* frees the `mBlk' back to the specified memory pool.  It also decrements 
* the reference count in the `clBlk' structure.  If the reference count 
* falls to zero, no other `mBlk' structure reference this `clBlk'.  In that 
* case, this routine also frees the `clBlk' structure and its associated 
* cluster back to the specified memory pool.  
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned ID is valid in the kernel protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS
* If the specified `mBlk' was part of an `mBlk' chain, this routine returns 
* a pointer to the next `mBlk'.  Otherwise, it returns a NULL.
*
* ERRNO:
*  S_netBufLib_MBLK_INVALID
*/

M_BLK_ID netMblkClFree
    (
    M_BLK_ID 		pMblk		/* pointer to the mBlk */
    )
    {
    NET_POOL_ID 	pNetPool;

    if (pMblk == NULL)
        return (NULL);

    pNetPool = MBLK_TO_NET_POOL(pMblk);

    return ((*pNetPool->pFuncTbl->pMblkClFreeRtn) (pNetPool, pMblk));
    }

/*******************************************************************************
*
* netMblkClChainFree - free a chain of `mBlk'-`clBlk'-cluster constructs
*
* For the specified chain of `mBlk'-`clBlk'-cluster constructs, this 
* routine frees all the `mBlk' structures back to the specified memory pool.  
* It also decrements the reference count in all the `clBlk' structures.  If 
* the reference count in a `clBlk' falls to zero, this routine also frees 
* that `clBlk' and its associated cluster back to the specified memory pool. 
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: N/A
*
* ERRNO:
*  S_netBufLib_MBLK_INVALID
*/

void netMblkClChainFree
    (
    M_BLK_ID 		pMblk		/* pointer to the mBlk */
    )
    {
    NET_POOL_ID 	pNetPool;

    while (pMblk != NULL)
        {
        pNetPool = MBLK_TO_NET_POOL(pMblk);
        pMblk 	 = (*pNetPool->pFuncTbl->pMblkClFreeRtn) (pNetPool, pMblk);
        }
    }

/*******************************************************************************
*
* netMblkGet - get an `mBlk' from a memory pool
*
* This routine allocates an `mBlk' from the specified memory pool, if
* available.
*
* .IP <pNetPool> 9
* Expects a pointer to the pool from which you want an `mBlk'.
* .IP <canWait>
* Expects either M_WAIT or M_DONTWAIT.  If no 'mBlk' is immediately available,
* the M_WAIT value allows this routine to repeat the allocation attempt after
* performing garbage collection. It omits these steps when the M_DONTWAIT
* value is used.
* .IP <type>
* Expects the type value that you want to associate with the returned `mBlk'.
* .LP
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned ID is valid in the kernel protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS
* M_BLK_ID or NULL if no `mBlk' is available.
*
* ERRNO:
*  S_netBufLib_MBLK_INVALID
*/

M_BLK_ID netMblkGet
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    int			canWait,	/* M_WAIT/M_DONTWAIT */
    UCHAR		type		/* mBlk type */
    )
    {
    if (pNetPool == NULL || pNetPool->pFuncTbl == NULL ||
        pNetPool->pFuncTbl->pMblkGetRtn == NULL)
        return (NULL);

    return ((*pNetPool->pFuncTbl->pMblkGetRtn) (pNetPool, canWait, type));
    }

/*******************************************************************************
*
* netClBlkGet - get a `clBlk'
*
* This routine gets a `clBlk' from the specified memory pool.
* .IP <pNetPool> 9
* Expects a pointer to the pool from which you want a `clBlk'.
* .IP <canWait>
* Expects either M_WAIT or M_DONTWAIT.  If no 'clBlk' is immediately available,
* the M_WAIT value allows this routine to repeat the allocation attempt after
* performing garbage collection. It omits these steps when the M_DONTWAIT
* value is used.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned ID is valid in the kernel protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: CL_BLK_ID or a NULL if no `clBlk' was available.
*/

CL_BLK_ID netClBlkGet
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    int			canWait		/* M_WAIT/M_DONTWAIT */
    )
    {
    if (pNetPool == NULL || pNetPool->pFuncTbl == NULL ||
        pNetPool->pFuncTbl->pClBlkGetRtn == NULL)
        return (NULL);

    return ((*pNetPool->pFuncTbl->pClBlkGetRtn) (pNetPool, canWait));
    }

/*******************************************************************************
*
* netClusterGet - get a cluster from the specified cluster pool
*
* This routine gets a cluster from the specified cluster pool <pClPool> 
* within the specified memory pool <pNetPool>. 
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned ID is valid in the kernel protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS
* This routine returns a character pointer to a cluster buffer or NULL
* if none was available.
*
*/

char * netClusterGet
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    CL_POOL_ID 		pClPool 	/* ptr to the cluster pool */
    )
    {
    if (pNetPool == NULL || pNetPool->pFuncTbl == NULL ||
        pNetPool->pFuncTbl->pClGetRtn == NULL)
        return (NULL);

    return ((*pNetPool->pFuncTbl->pClGetRtn) (pNetPool, pClPool));
    }

/*******************************************************************************
*
* netMblkClGet - get a `clBlk'-cluster and join it to the specified `mBlk'
*
* This routine gets a `clBlk'-cluster pair from the specified memory pool
* and joins it to the specified `mBlk' structure.  The `mBlk'-`clBlk'-cluster
* triplet it produces is the basic structure for handling data at all layers
* of the network stack.
*
* .IP <pNetPool> 9
* Expects a pointer to the memory pool from which you want to get a 
* free `clBlk'-cluster pair.
* .IP <pMbkl>
* Expects a pointer to the `mBlk' structure (previously allocated) to which 
* you want to join the retrieved `clBlk'-cluster pair.  
* .IP <bufSize>
* Expects the size, in bytes, of the cluster in the `clBlk'-cluster pair.  
* .IP <canWait>
* Expects either M_WAIT or M_DONTWAIT.  If either item is not immediately
* available, the M_WAIT value allows this routine to repeat the allocation
* attempt after performing garbage collection. It omits those steps when the
* M_DONTWAIT value is used.
* .IP <bestFit>
* Expects either TRUE or FALSE.  If <bestFit> is TRUE and a cluster of the 
* exact size is unavailable, this routine gets a larger cluster (if
* available).  If <bestFit> is FALSE and an exact size cluster is unavailable, 
* this routine gets either a smaller or a larger cluster (depending 
* on what is available).  Otherwise, it returns immediately with an ERROR value.
* For memory pools containing only one cluster size, <bestFit> should always
* be set to FALSE.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* .SH "RETURNS"
* OK or ERROR.
*
* ERRNO:
*  S_netBufLib_CLSIZE_INVALID
*/

STATUS netMblkClGet
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    M_BLK_ID	 	pMblk, 		/* mBlk to embed the cluster in */
    int			bufSize,	/* size of the buffer to get */
    int			canWait,	/* wait or dontwait */
    BOOL		bestFit		/* TRUE/FALSE */
    )
    {
    if (pNetPool == NULL || pNetPool->pFuncTbl == NULL ||
        pNetPool->pFuncTbl->pMblkClGetRtn == NULL)
        return (ERROR);

    return ((*pNetPool->pFuncTbl->pMblkClGetRtn) (pNetPool, pMblk, bufSize,
                                                  canWait, bestFit));
    }

/*******************************************************************************
*
* netTupleGet - get an `mBlk'-`clBlk'-cluster
*
* This routine gets an `mBlk'-`clBlk'-cluster triplet from the specified
* memory pool.  The resulting structure is the basic method for accessing
* data at all layers of the network stack.
*
* .IP <pNetPool> 9
* Expects a pointer to the memory pool with which you want to build a
* `mBlk'-`clBlk'-cluster triplet.
* .IP <bufSize>
* Expects the size, in bytes, of the cluster in the `clBlk'-cluster
* pair.
* .IP <canWait>
* Expects either M_WAIT or M_DONTWAIT.  If any item in the triplet is not
* immediately available, the M_WAIT value allows this routine to repeat the
* allocation attempt after performing garbage collection. The M_DONTWAIT value
* prevents those extra steps.
* .IP <type>
* Expects the type of data, for example MT_DATA, MT_HEADER. The various
* values for this type are defined in netBufLib.h.
* .IP <bestFit>
* Expects either TRUE or FALSE.  If <bestFit> is TRUE and a cluster of the 
* exact size is unavailable, this routine gets a larger cluster (if
* available).  If <bestFit> is FALSE and an exact size cluster is unavailable, 
* this routine gets either a smaller or a larger cluster (depending 
* on what is available).  Otherwise, it returns immediately with an ERROR value.
* For memory pools containing only one cluster size, <bestFit> should always
* be set to FALSE.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned ID is valid in the kernel protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS
* M_BLK_ID or NULL.
*
* ERRNO:
*  S_netBufLib_MBLK_INVALID
*  S_netBufLib_CLSIZE_INVALID
*  S_netBufLib_NETPOOL_INVALID
*/

M_BLK_ID netTupleGet
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    int			bufSize,	/* size of the buffer to get */
    int			canWait,	/* wait or dontwait */
    UCHAR		type,		/* type of data */
    BOOL		bestFit		/* TRUE/FALSE */
    )
    {
    M_BLK_ID		pMblk = NULL; 		/* pointer to mBlk */

    if (pNetPool != NULL && pNetPool->pFuncTbl != NULL)
        {
        pMblk = mBlkGet (pNetPool, canWait, type); 	/* get an mBlk */

        /* allocate a cluster and point the mBlk to it */
        if (pMblk && (mClGet (pNetPool, pMblk, bufSize, canWait, bestFit)
                      != OK))
            {
            netMblkFree (pNetPool, pMblk); 
            pMblk = NULL; 
            }
        }
    else
        errno = S_netBufLib_NETPOOL_INVALID; 

    return (pMblk); 
    }

/*******************************************************************************
*
* netClBlkJoin - join a cluster to a `clBlk' structure 
*
* This routine joins the previously reserved cluster specified by <pClBuf> 
* to the previously reserved `clBlk' structure specified by <pClBlk>. 
* The <size> parameter passes in the size of the cluster referenced 
* in <pClBuf>.  The arguments <pFreeRtn>, <arg1>, <arg2>, <arg3> set the  
* values of the 'pCLFreeRtn', 'clFreeArg1', 'clFreeArg2', and 'clFreeArg1', 
* members of the specified `clBlk' structure.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned ID is valid in the kernel protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: CL_BLK_ID or NULL.
*/

CL_BLK_ID netClBlkJoin
    (
    CL_BLK_ID		pClBlk,		/* pointer to a cluster Blk */
    char *		pClBuf,		/* pointer to a cluster buffer */
    int			size,		/* size of the cluster buffer */
    FUNCPTR		pFreeRtn,	/* pointer to the free routine */
    int			arg1,		/* argument 1 of the free routine */
    int			arg2,		/* argument 2 of the free routine */
    int			arg3		/* argument 3 of the free routine */
    )
    {
    if (pClBlk == NULL || pClBuf == NULL)
        return (NULL);

    pClBlk->clNode.pClBuf 	= pClBuf;
    pClBlk->clSize 		= size;
    pClBlk->pClFreeRtn 		= pFreeRtn;
    pClBlk->clFreeArg1 		= arg1;
    pClBlk->clFreeArg2 		= arg2;
    pClBlk->clFreeArg3 		= arg3;
    pClBlk->clRefCnt		= 1;
    return (pClBlk);
    }

/*******************************************************************************
*
* netMblkClJoin - join an `mBlk' to a `clBlk'-cluster construct
*
* This routine joins the previously reserved `mBlk' referenced in <pMblk> to
* the `clBlk'-cluster construct referenced in <pClBlk>. 
* Internally, this routine sets the M_EXT flag in 'mBlk.mBlkHdr.mFlags'.  It 
* also and sets the 'mBlk.mBlkHdr.mData' to point to the start of the data 
* in the cluster.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned ID is valid in the kernel protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* .SH "RETURNS"
* M_BLK_ID or NULL. 
*
*/

M_BLK_ID netMblkClJoin
    (
    M_BLK_ID 		pMblk,		/* pointer to an mBlk */
    CL_BLK_ID		pClBlk		/* pointer to a cluster Blk */
    )
    {
    if (pMblk == NULL || pClBlk == NULL)
        return (NULL);

    pMblk->mBlkHdr.mData 	= pClBlk->clNode.pClBuf;
    pMblk->mBlkHdr.mFlags 	|= M_EXT;
    pMblk->pClBlk		= pClBlk;

    return (pMblk);
    }

/*******************************************************************************
*
* netClPoolIdGet - return a CL_POOL_ID for a specified buffer size 
* This routine returns a CL_POOL_ID for a cluster pool containing clusters 
* that match the specified <bufSize>.  If bestFit is TRUE, this routine returns 
* a CL_POOL_ID for a pool that contains clusters greater than or equal 
* to <bufSize>.  If <bestFit> is FALSE, this routine returns a CL_POOL_ID for a 
* cluster from whatever cluster pool is available.  If the memory pool 
* specified by <pNetPool> contains only one cluster pool, <bestFit> should 
* always be FALSE.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned ID is valid in the kernel protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: CL_POOL_ID or NULL.
*/

CL_POOL_ID netClPoolIdGet
    (
    NET_POOL_ID		pNetPool,	/* pointer to the net pool */
    int 		bufSize,	/* size of the buffer */
    BOOL		bestFit		/* TRUE/FALSE */
    )
    {
    if (pNetPool == NULL || pNetPool->pFuncTbl == NULL ||
        pNetPool->pFuncTbl->pClPoolIdGetRtn == NULL)
        return (NULL);

    return ((*pNetPool->pFuncTbl->pClPoolIdGetRtn) (pNetPool, bufSize,
                                                    bestFit));
    }

/*******************************************************************************
*
* netMblkToBufCopy - copy data from an `mBlk' to a buffer
*
* This routine copies data from the `mBlk' chain referenced in <pMblk> to 
* the buffer referenced in <pBuf>.  It is assumed that <pBuf> points to 
* enough memory to contain all the data in the entire `mBlk' chain.
* The argument <pCopyRtn> expects either a NULL or a function pointer to a 
* copy routine.  The arguments passed to the copy routine are source 
* pointer, destination pointer and the length of data to copy.  If <pCopyRtn> 
* is NULL, netMblkToBufCopy() uses a default routine to extract the data from 
* the chain.
* 
* .SH "RETURNS"
* The length of data copied or zero.
*/

int netMblkToBufCopy
    (
    M_BLK_ID 		pMblk,		/* pointer to an mBlk */
    char *		pBuf,		/* pointer to the buffer to copy */
    FUNCPTR		pCopyRtn	/* function pointer for copy routine */
    )
    {
    char *		pChar;

    if (pCopyRtn == NULL)
        pCopyRtn = (FUNCPTR) bcopy;	/* default copy routine */

    pChar = pBuf;

    while (pMblk != NULL)
        {
        (*pCopyRtn) ((char *)pMblk->mBlkHdr.mData, pBuf, pMblk->mBlkHdr.mLen);
        pBuf 	+= pMblk->mBlkHdr.mLen;
        pMblk 	= pMblk->mBlkHdr.mNext;
        }

    return ((int) pBuf - (int) pChar);		/* return length copied */
    }

/*******************************************************************************
*
* netMblkDup - duplicate an `mBlk'
*
* This routine copies the references from a source `mBlk' in 
* an `mBlk'-`clBlk'-cluster construct to a stand-alone `mBlk'.
* This lets the two `mBlk' structures share the same `clBlk'-cluster
* construct.  This routine also increments the reference count in the 
* shared `clBlk'.  The <pSrcMblk> expects a pointer to the source `mBlk'.  
* The <pDescMblk> parameter expects a pointer to the destination `mBlk'. 
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned ID is valid in the kernel protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* .SH "RETURNS"
* A pointer to the destination `mBlk' or NULL if the source `mBlk' 
* referenced in <pSrcMblk> is not part of a 
* valid `mBlk'-`clBlk'-cluster construct.
*
*/

M_BLK_ID netMblkDup
    (
    M_BLK_ID	pSrcMblk,		/* pointer to source mBlk */
    M_BLK_ID	pDestMblk		/* pointer to the destination mBlk */
    )
    {
    int		level;			/* level of interrupt */

    if (pSrcMblk == NULL || pDestMblk == NULL)
        return (NULL);

    if (M_HASCL (pSrcMblk))		/* only if associated with a cluster */
        {
        if (pSrcMblk->mBlkHdr.mFlags & M_PKTHDR)
            pDestMblk->mBlkPktHdr = pSrcMblk->mBlkPktHdr;

        pDestMblk->mBlkHdr.mData 	= pSrcMblk->mBlkHdr.mData;
        pDestMblk->mBlkHdr.mLen 	= pSrcMblk->mBlkHdr.mLen;
        pDestMblk->mBlkHdr.mType 	= pSrcMblk->mBlkHdr.mType;
        pDestMblk->mBlkHdr.mFlags 	= pSrcMblk->mBlkHdr.mFlags;
        pDestMblk->pClBlk 		= pSrcMblk->pClBlk;

        level = intLock ();
        ++(pDestMblk->pClBlk->clRefCnt);	/* increment the ref count */
        intUnlock (level);
        }
    else
        pDestMblk = NULL;

    return (pDestMblk);
    }

/*******************************************************************************
*
* netMblkChainDup - duplicate an `mBlk' chain
*
* This routine makes a copy of an `mBlk' chain starting at <offset> bytes from
* the beginning of the chain and continuing for <len> bytes.  If <len> 
* is M_COPYALL, then this routine will copy the entire `mBlk' chain from 
* the <offset>.
* 
* This routine copies the references from a source <pMblk> chain to
* a newly allocated `mBlk' chain. 
* This lets the two `mBlk' chains share the same `clBlk'-cluster
* constructs.  This routine also increments the reference count in the 
* shared `clBlk'.  The <pMblk> expects a pointer to the source `mBlk'
* chain.  
* The <pNetPool> parameter expects a pointer to the netPool from which
* the new `mBlk' chain is allocated.
*
* The <canWait> parameter determines the behavior if any required 'mBlk'
* is not immediately available. A value of M_WAIT allows this routine to
* repeat the allocation attempt after performing garbage collection. The
* M_DONTWAIT value prevents those extra steps.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* Likewise, the returned ID is valid in the kernel protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* SEE ALSO: netMblkDup()
*
* .SH "RETURNS"
* A pointer to the newly allocated `mBlk' chain or NULL.
*
* ERRNO:
*  S_netBufLib_INVALID_ARGUMENT
*  S_netBufLib_NO_POOL_MEMORY
*/

M_BLK_ID netMblkChainDup
    (
    NET_POOL_ID	pNetPool,		/* pointer to the pool */
    M_BLK_ID	pMblk,			/* pointer to source mBlk chain*/
    int 	offset,			/* offset to duplicate from */	
    int 	len,			/* length to copy */
    int		canWait			/* M_DONTWAIT/M_WAIT */
    )
    {
    M_BLK_ID 	pNewMblk = NULL;
    M_BLK_ID *	ppMblk   = NULL;
    M_BLK_ID 	top      = NULL;
    BOOL	copyhdr  = FALSE;
    int 	off      = offset;
    int		level;

    if (off < 0 || len < 0)
        {
        errno = S_netBufLib_INVALID_ARGUMENT;
        goto netMblkChainDupError;
        }

    if (off == 0 && pMblk->mBlkHdr.mFlags & M_PKTHDR)
	copyhdr = TRUE;
    while (off > 0)
	{
	if (pMblk == 0)
            {
            errno = S_netBufLib_INVALID_ARGUMENT;
            goto netMblkChainDupError;
            }
	if (off < pMblk->mBlkHdr.mLen)
	    break;
	off   -= pMblk->mBlkHdr.mLen;
	pMblk = pMblk->mBlkHdr.mNext;
	}
    ppMblk = &top;

    while (len > 0)
	{
	if (pMblk == 0)
	    {
	    if (len != M_COPYALL)
                {
                errno = S_netBufLib_INVALID_ARGUMENT;
                goto netMblkChainDupError;
                }
	    break;
	    }
	pNewMblk = mBlkGet(pNetPool, canWait, pMblk->mBlkHdr.mType);
	if ((*ppMblk = pNewMblk) == NULL)
            {
            errno = S_netBufLib_NO_POOL_MEMORY;
            goto netMblkChainDupError;
            }
	if (copyhdr)
	    {
            pNewMblk->mBlkPktHdr = pMblk->mBlkPktHdr;
	    if (len == M_COPYALL)
		pNewMblk->mBlkPktHdr.len -= offset;
	    else
		pNewMblk->mBlkPktHdr.len = len;
	    copyhdr = FALSE;
	    }
	pNewMblk->mBlkHdr.mFlags = pMblk->mBlkHdr.mFlags; 
	pNewMblk->mBlkHdr.mLen = min(len, pMblk->mBlkHdr.mLen - off);
	pNewMblk->mBlkHdr.mData = pMblk->mBlkHdr.mData + off;
	pNewMblk->pClBlk = pMblk->pClBlk;

        level = intLock ();
        ++(pNewMblk->pClBlk->clRefCnt);	/* increment the ref count */
        intUnlock (level);

	if (len != M_COPYALL)
	    len -= pNewMblk->mBlkHdr.mLen;
	off = 0;
	pMblk = pMblk->mBlkHdr.mNext;
	ppMblk = &pNewMblk->mBlkHdr.mNext;
	}

    return (top);

    netMblkChainDupError:
    	{
        if (top != NULL)
            netMblkClChainFree (top);

        return (NULL);
        }
    }

/*******************************************************************************
*
* netMblkOffsetToBufCopy - copy data from an `mBlk' from an offset to a buffer
*
* This routine copies data from the `mBlk' chain referenced in <pMblk> to 
* the buffer referenced in <pBuf>.  It is assumed that <pBuf> points to 
* enough memory to contain all the data in the entire `mBlk' chain.
* The argument <pCopyRtn> expects either a NULL or a function pointer to a 
* copy routine.  The arguments passed to the copy routine are source 
* pointer, destination pointer and the length of data to copy.  If <pCopyRtn> 
* is NULL, netMblkToBufCopy() uses a default routine to extract the data from 
* the chain.  The argument <offset> is a an offset from the first byte of data
* in the mBlk chain.  The argument <len> is the total of length of data to be
* copied from the mBlk Chain.  If value of <len> is M_COPYALL then the entire
* mBlkChain is copied from the offset.  M_COPYALL is defined to be 1000000000
*
* NOMANUAL
*
* .SH "RETURNS"
* The length of data copied or zero.
*
* ERRNO:
*  S_netBufLib_INVALID_ARGUMENT
*/

int netMblkOffsetToBufCopy
    (
    M_BLK_ID 	pMblk,		/* pointer to an mBlk */
    int		offset,		/* offset to copy from */
    char *	pBuf,		/* pointer to the buffer to copy */
    int 	len,		/* length to copy */
    FUNCPTR	pCopyRtn	/* function pointer for copy routine */
    )
    {
    ULONG 	count;
    char *	pChar;

    pChar = pBuf;

    if (pCopyRtn == NULL)
        pCopyRtn = (FUNCPTR) bcopy;	/* default copy routine */

    if (offset < 0 || len < 0 || pMblk == NULL) 
        goto netMblkOffsetToBufCopyError;
    
    while (offset > 0 && pMblk != NULL)
	{
	if (offset < pMblk->mBlkHdr.mLen)
	    break;
	offset -= pMblk->mBlkHdr.mLen;
	pMblk  = pMblk->mBlkHdr.mNext;
	}

    while (len > 0 && pMblk != NULL)
	{
        if (len != M_COPYALL)
            {
            count = min(pMblk->mBlkHdr.mLen - offset, len);
            len   -= count;
            }
        else
            count = pMblk->mBlkHdr.mLen - offset;
            
        (*pCopyRtn) ((char *)pMblk->mBlkHdr.mData + offset, pBuf, count);
	pBuf   += count;
	offset = 0;
	pMblk  = pMblk->mBlkHdr.mNext;
	}

    return ((int) pBuf - (int) pChar);		/* return length copied */
    
    netMblkOffsetToBufCopyError:
    	{
        errno = S_netBufLib_INVALID_ARGUMENT;
        return (0);
        }
    }

#if 0 /* XXX support may be required later */
/*******************************************************************************
*
* netMblkFromBufCopy - copy data to an mBlk chain
*
* This routine copies data from a given buffer to an mBlk chain.
* The argument <pCopyRtn> is a function pointer to a copy routine which
* if not specified a default copy routine would be used.  The arguments passed
* to the copy routine are source pointer, destination pointer and the length
* of data to copy.  The data is copied from an offset indicated the argument
* <offset>.
*
* NOMANUAL
*
* RETURNS unsigned char pointer to the buffer or NULL.
*/

UCHAR * netMblkFromBufCopy
    (
    M_BLK_ID 	pMblk,		/* pointer to an mBlk chain */
    char *	pBuffer,	/* pointer to the data buffer */
    int 	offset,		/* offset from the beginning of the data */
    int 	len,		/* total length to copy */
    FUNCPTR	pCopyRtn	/* function pointer for copy routine */
    )
    {
    return (NULL);
    }
#endif /* XXX future support */

/*******************************************************************************
*
* netTupleGet2 - makes a mblk chain
*
* This routine allocates a mblk chain just like how a network card driver
* would do it. It also changes mLen to the <len> parameter.
*
* RETURNS: a Mblk if successful, else NULL
*
* NOMANUAL
*/

M_BLK_ID netTupleGet2 
    (
    NET_POOL_ID poolId, 
    int len,
    int bestFit
    )
    {
    M_BLK_ID            pMblk           = NULL;
    CL_BLK_ID           pClBlk          = NULL;
    char *              pData;
    CL_POOL_ID		clPoolId;

    clPoolId = clPoolIdGet (poolId, len, bestFit);

    if ((pMblk = netMblkGet (poolId,
                            M_DONTWAIT, MT_DATA)) == NULL)
        {
        goto Error;
        }

    if ((pClBlk = netClBlkGet (poolId, M_DONTWAIT)) == NULL)
        {
        goto Error;
        }

    if ((pData = (char *) netClusterGet (poolId, clPoolId)) == NULL)
        {
        goto Error;
        }

    /* is the RFD to be given back?? */

    if (netClBlkJoin (pClBlk, pData, len, NULL, 0, 0, 0) == NULL)
        {
        goto Error;
        }

    if (netMblkClJoin (pMblk, pClBlk) == NULL)
        {
        goto Error;
        }

    pMblk->mBlkHdr.mLen = len;
    pMblk->mBlkPktHdr.len = len;

    return (pMblk);
Error:
    return ((M_BLK_ID)0);
    }



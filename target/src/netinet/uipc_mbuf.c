/* uipc_mbuf.c - mbuf routines */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1982, 1986, 1988, 1991, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)uipc_mbuf.c	8.4 (Berkeley) 2/14/95
 */

/*
modification history
--------------------
01p,12oct01,rae  merge from truestack ver 01t, base 01n
01o,07feb01,spm  removed unused garbage collection code
01n,25aug98,n_s  made _bufCollect only call pr_drain 1 time. spr #22104
01m,11dec97,vin  made m_free and m_freem as macros calling netMblkClFree
		 and netMblkClChainFree functions.
01l,08dec97,vin  removed m_copym merged with netMblkChainDup SPR 9966.
		 moved old m_copym code to #if 0 section.
01k,01dec97,vin  added system pool configuration logic.
		 Ifdefed m_copyback and m_copydata in uipc_mbuf.c these
                 need some rework so that they can be tied up efficiently
                 with the new buffering scheme.
01j,08oct97,vin  fixed clBlkFree.
01i,05oct97,vin  bug fix in m_copym
01h,19sep97,vin  added clBlk specific code, changed pointers to refcounts
		 of clusters to refcounts since refcount has been moved to
                 clBlk
01g,11aug97,vin  interfaced with netBufLib.c, moved all windRiver specific
		 buffering code to netBufLib.c
01f,02jul97,vin  SPR 8878.
01e,15may97,vin  reworked m_devget(), added support for width of copying.
01d,31jan97,vin  removed local variable clLog2Def, cleanup
01c,03dec96,vin  changed interface to m_getclr() added additional parameters.
01b,13nov96,vin  removed BSD4.4 mbufs, modified m_devget, deleted m_get,
		 m_gethdr, m_split, wrote new cluster and mBlk functions.
		 moved m_copyback from rtsock.c.
01a,03mar96,vin  created from BSD4.4 stuff,integrated with 02v of uipc_mbuf.c
*/

#include "vxWorks.h"
#include "intLib.h"
#include "net/mbuf.h"
#include "net/systm.h"
#include "net/domain.h"
#include "net/protosw.h"
#include "memPartLib.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif /* INCLUDE_WVNET */
#endif

/* Virtual Stack support. */

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif

/* externs */

#ifndef VIRTUAL_STACK
IMPORT struct domain *domains;
#endif

IMPORT VOIDFUNCPTR _pNetBufCollect;
IMPORT STATUS netPoolKheapInit   (NET_POOL_ID pNetPool,
                                  M_CL_CONFIG *pMclBlkConfig,
				  CL_DESC *pClDescTbl, int clDescTblNumEnt,
				  POOL_FUNC *pFuncTbl);
/* globals */

/* locals */

LOCAL int 	MPFail;	 	/* XXX temp variable pullup fails */

#ifndef VIRTUAL_STACK
LOCAL NET_POOL	_netDpool; 	/* system network data pool */
LOCAL NET_POOL	_netSysPool;	/* system network structure pool */

NET_POOL_ID	_pNetDpool   = &_netDpool; 
NET_POOL_ID	_pNetSysPool = &_netSysPool; 

#endif

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_UIMBUF_MODULE; /* Value for uipc_mbuf.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

/* forward declarations */
LOCAL void	_bufCollect (M_STAT * pPoolStat);
LOCAL STATUS 	_netStackPoolInit (NET_POOL_ID pNetPool,
                                   M_CL_CONFIG * pMclConfig,
                                   CL_DESC * pClDescTbl,
                                   int clTblNumEnt);

/*******************************************************************************
*
* _netStackPoolInit - initialize the net pool
*
* This routines initializes the netpool.
*
* NOMANUAL
*
* RETURNS: OK/ERROR
*/

LOCAL STATUS _netStackPoolInit
    (
    NET_POOL_ID		pNetPool,	/* pointer to a net pool */
    M_CL_CONFIG * 	pMclConfig,	/* ptr to mblk/clBlk config tbl */
    CL_DESC * 		pClDescTbl,	/* ptr to first entry in clDesc tbl */
    int 		clTblNumEnt	/* number of entries */
    )
    {
    int 		ix;		/* index variable */
    CL_DESC * 		pClDesc;	/* pointer to the cluster descriptor */

    if (pMclConfig->memArea == NULL)	/* do a default allocation */
        {
        /* memory size adjusted to hold the netPool pointer at the head */

        pMclConfig->memSize = (pMclConfig->mBlkNum *
                                (MSIZE + sizeof (long))) +
            		       (pMclConfig->clBlkNum * CL_BLK_SZ); 

	/* the config table MUST be allocated in kernel heap */
        if ((pMclConfig->memArea = (char *) KHEAP_ALLOC(pMclConfig->memSize))
            == NULL)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_EMERGENCY, 33, 1,
                             WV_NETEVENT_POOLINIT_PANIC,
                             pNetPool, pMclConfig->memSize)
#endif  /* INCLUDE_WVNET */
#endif

            panic ("no memory: mBlks/clBlks");
            return (ERROR); 
            }
        }

    for (pClDesc = pClDescTbl, ix = 0; ix < clTblNumEnt; ix++, pClDesc++)
        {
        /* memory size adjusted for refcount, pool ptr and 4 byte alignmnt */
        
        if (pClDesc->memArea == NULL) /* do a default allocation */
            {
            pClDesc->memSize = (pClDesc->clNum *
                                (pClDesc->clSize + sizeof (long)));

	    /* the cluster desc MUST be allocated in kernel heap */
            if ((pClDesc->memArea = (char *) KHEAP_ALLOC(pClDesc->memSize)) == NULL)
                {
                panic ("no memory: clusters");
                return (ERROR);
                }
            }
        }
    return (netPoolKheapInit (pNetPool, pMclConfig, pClDescTbl, clTblNumEnt, NULL));
    }

/*******************************************************************************
*
* mbinit - initialize mbuf package
*
* Initialize a number of mbufs and clusters to start the ball rolling.
* This is accomplished by calling routines _mBlkAlloc() and _clAlloc()
*
* NOMANUAL
*/

void mbinit (void)
    {
    netBufLibInit ();			/* initialize the buffering library */

    _pNetBufCollect = _bufCollect; 	/* intialize buffer collect routine */

#ifdef VIRTUAL_STACK

    /*
     * Set the pointers appropriately.  This is because in C we can't
     * do this in the global structure itself.
     */
    
    _pNetDpool   = &_netDpool; 
    _pNetSysPool = &_netSysPool; 
#endif /* VIRTUAL_STACK */

    if (_netStackPoolInit (_pNetDpool, &mClBlkConfig, &clDescTbl [0],
                           clDescTblNumEnt) != OK)
        goto mbInitError;

    if (_netStackPoolInit (_pNetSysPool, &sysMclBlkConfig, &sysClDescTbl [0],
                           sysClDescTblNumEnt) != OK)
        goto mbInitError;

    return;
    
    mbInitError:
    	{
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
        WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_EMERGENCY, 34, 2,
                         WV_NETEVENT_MEMINIT_PANIC)
#endif  /* INCLUDE_WVNET */
#endif

        panic ("mbinit");
        return;
        }
    }

/*******************************************************************************
*
* _bufCollect - drain protocols and collect required buffers
*
* Must be called at splimp.
* 
* NOMANUAL
*
*/

LOCAL void _bufCollect
    (
    M_STAT *	pPoolStat		/* pointer to the pools stats struct */
    )
    {
    FAST struct domain	*dp;
    FAST struct protosw	*pr;
    int	 		level; 		/* level of interrupt */

    /* ask protocols to free space */

    for (dp = domains; dp; dp = dp->dom_next)
	for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
	    if (pr->pr_drain)
		(*pr->pr_drain)();

    level = intLock (); 
    pPoolStat->mDrain++;		/* no. of times protocols drained */
    intUnlock (level); 
    }

/*******************************************************************************
*
* mBufClGet - get a new mBlk with a cluster 
*
* This routine allocates an mBlk and points it to a cluster allocated
* Different clusters are allocated depending upon the cluster size requested.
* 
* INTERNAL
* 
* NOMANUAL
* 
* RETURNS: NULL/mBlk
*/
struct mbuf * mBufClGet
    (
    int			canWait,	/* M_DONTWAIT/M_WAIT */
    UCHAR		type,		/* type of data */
    int			bufSize,	/* size of the buffer to get */
    BOOL		bestFit		/* TRUE/FALSE */
    )
    {
    FAST struct mbuf *	pMblk; 		/* pointer to mbuf */

    pMblk = mBlkGet (_pNetDpool, canWait, type); 	/* get an mBlk */

    /* allocate a cluster and point the mbuf to it */
    if (pMblk && (mClGet (_pNetDpool, pMblk, bufSize, canWait, bestFit) != OK))
	{
	(void)m_free (pMblk); 
	pMblk = NULL; 
	}

    return (pMblk);
    }

/*******************************************************************************
*
* mHdrClGet - get a new mBlkHdr with a cluster 
*
* This routine allocates an mBlkHdr and points it to a cluster allocated
* Different clusters are allocated depending upon the cluster size requested.
*
* INTERNAL
*  
* NOMANUAL
* 
* RETURNS: NULL/mBlkHdr
*/

struct mbuf * mHdrClGet
    (
    int			canWait,		/* M_DONTWAIT/M_WAIT */
    UCHAR		type,			/* type of data */
    int			bufSize,		/* size of the buffer to get */
    BOOL		bestFit			/* TRUE/FALSE */
    )
    {
    FAST struct mbuf *	pMblk = NULL; 			/* pointer to mbuf */

    /* get an mBlk/cluster pair */
    
    pMblk = mBlkGet (_pNetDpool, canWait, type); 	/* get an mBlk */

    if (pMblk == NULL)
        return (NULL);
    
    pMblk->m_flags |= M_PKTHDR; 

    /* allocate a cluster and point the mbuf to it */
    if (mClGet (_pNetDpool, pMblk, bufSize, canWait, bestFit) != OK)
	{
	(void)m_free (pMblk); 
	pMblk = NULL; 
	}
    return (pMblk);
    }

/*******************************************************************************
*
* m_getclr - get a clean mBlk, cluster pair
*
* This routine allocates an mBlk, points it to a cluster,
* and zero's the cluster allocated.
*
* INTERNAL
*  
* NOMANUAL
* 
* RETURNS: NULL/mBlk
*/

struct mbuf * m_getclr
    (
    int 		canwait,		/* M_WAIT/M_DONTWAIT */
    UCHAR		type,			/* type of mbuf to allocate */
    int			bufSize,		/* size of the buffer to get */
    BOOL		bestFit			/* TRUE/FALSE */
    )
    {
    FAST struct mbuf *	pMblk = NULL; 		/* pointer to mbuf */

    /* allocate mBlk/cluster pair */
    
    if ((pMblk = mBufClGet (canwait, type, bufSize, bestFit)) != NULL)
	bzero(mtod(pMblk, caddr_t), pMblk->m_extSize); 

    return (pMblk);
    }

#if 0 /* XXX */
#endif /* XXX */

/*
 * Mbuffer utility routines.
 */

/*******************************************************************************
*
* m_prepend - prepend an mbuf to a chain.
*
* Lesser-used path for M_PREPEND:
* allocate new mbuf to prepend to chain,
* copy junk along.
*/

struct mbuf * m_prepend(m, len, how)
    register struct mbuf *m;
    int len;
    int how;
    {
    struct mbuf *mn;

    if ((mn = mBufClGet (how, m->m_type, len, TRUE)) == NULL)
	{
	m_freem(m);
	return ((struct mbuf *)NULL);
	}
    if (m->m_flags & M_PKTHDR)
	{
	mn->m_pkthdr = m->m_pkthdr;
	mn->m_flags = m->m_flags;
	m->m_flags &= ~M_PKTHDR;
	}
    mn->m_next = m;
    m = mn;
    if (len < m->m_extSize)
	MH_ALIGN(m, len);
    m->m_len = len;
    return (m);
    }

/*******************************************************************************
*
* m_cat - concatenate mbuf chain n to m.
*
* Concatenate mbuf chain n to m.
* Both chains must be of the same type (e.g. MT_DATA).
* Any m_pkthdr is not updated.
*/

void m_cat(m, n)
    register struct mbuf * m;
    register struct mbuf * n;
    {
    while (m->m_next)
	m = m->m_next;

    if (n != NULL)		/* clusters are used by default */
	m->m_next = n; 
    }

/*******************************************************************************
*
* m_adj - adjust the data in the mbuf
*
*/

void m_adj(mp, req_len)
    struct mbuf * mp;
    int req_len;
    {
    register int len = req_len;
    register struct mbuf *m;
    register int count;

    if ((m = mp) == NULL)
	return;
    if (len >= 0)
	{
	/*
	 * Trim from head.
	 */
	while (m != NULL && len > 0)
	    {
	    if (m->m_len <= len)
		{
		len -= m->m_len;
		m->m_len = 0;
		m = m->m_next;
		}
	    else
		{
		m->m_len -= len;
		m->m_data += len;
		len = 0;
		}
	    }
	m = mp;
	if (mp->m_flags & M_PKTHDR)
	    m->m_pkthdr.len -= (req_len - len);
	} 
    else
	{
	/*
	 * Trim from tail.  Scan the mbuf chain,
	 * calculating its length and finding the last mbuf.
	 * If the adjustment only affects this mbuf, then just
	 * adjust and return.  Otherwise, rescan and truncate
	 * after the remaining size.
	 */
	len = -len;
	count = 0;
	for (;;)
	    {
	    count += m->m_len;
	    if (m->m_next == (struct mbuf *)0)
		break;
	    m = m->m_next;
	    }
	if (m->m_len >= len)
	    {
	    m->m_len -= len;
	    if (mp->m_flags & M_PKTHDR)
		mp->m_pkthdr.len -= len;
	    return;
	    }
	count -= len;
	if (count < 0)
	    count = 0;
	/*
	 * Correct length for chain is "count".
	 * Find the mbuf with last data, adjust its length,
	 * and toss data from remaining mbufs on chain.
	 */
	m = mp;
	if (m->m_flags & M_PKTHDR)
	    m->m_pkthdr.len = count;
	for (; m; m = m->m_next) 
	    {
	    if (m->m_len >= count) 
		{
		m->m_len = count;
		break;
		}
	    count -= m->m_len;
	    }
	while ((m = m->m_next))
	    m->m_len = 0;
	}
    }

/*******************************************************************************
*
* m_pullup - ensure contiguous data area at the beginnig of an mbuf chain
*
* Rearange an mbuf chain so that len bytes are contiguous
* and in the data area of an mbuf (so that mtod and dtom
* will work for a structure of size len).  Returns the resulting
* mbuf chain on success, frees it and returns null on failure.
*/

struct mbuf *
m_pullup(n, len)
    register struct mbuf *n;
    int len;
    {
    register struct mbuf *m;
    register int count;
    int space;

    if (((n->m_data + len) < (n->m_extBuf + n->m_extSize)) && n->m_next)
	{
	if (n->m_len >= len)
	    return (n); 
	m = n;
	n = n->m_next;
	len -= m->m_len;
	}
    else 
	{
	m = mBufClGet(M_DONTWAIT, n->m_type, len, TRUE);
	if (m == 0)
	    goto bad;
	m->m_len = 0;
	if (n->m_flags & M_PKTHDR)
	    {
	    m->m_pkthdr = n->m_pkthdr;
	    m->m_flags = n->m_flags; 
	    n->m_flags &= ~M_PKTHDR;
	    }
	}
    space = (m->m_extBuf + m->m_extSize) - (m->m_data + m->m_len);
    do 
	{
	count = min(min(max(len, max_protohdr), space), n->m_len);
	bcopy(mtod(n, caddr_t), mtod(m, caddr_t) + m->m_len,
	      (unsigned)count);
	len -= count;
	m->m_len += count;
	n->m_len -= count;
	space -= count;
	if (n->m_len)
	    n->m_data += count;
	else
	    n = m_free(n);
	} while (len > 0 && n);
    if (len > 0)
	{
	(void) m_free(m);
	goto bad;
	}
    m->m_next = n;
    return (m);
    bad:
    m_freem(n);
    MPFail++;
    return (0);
    }

/*******************************************************************************
*
* m_devget - Routine to copy from device local memory into mbufs.
*
* Routine to copy from device local memory into mbufs.
* This routine copies the device local memory into mbufs according to the
* specified width. The interface pointer is initialized as a part of the
* packet header in the mbuf chain. The first mbuf in the chain is always
* a packet header and contains the interface pointer. This field is used
* when it is necessary to retrieve the interface pointer from the mbuf chain.
* If a copy function pointer is given, this function uses that function
* pointer. This is given to enable the hardware specific copy routines to be
* used instead of system routines.
*
*/

struct mbuf * m_devget(buf, totlen, width, ifp, copy)
    char * buf;
    int totlen;
    int width;
    struct ifnet *ifp;
    void (*copy)();
    {
    register struct mbuf *m;
    struct mbuf *top = NULL, **mp = &top;
    register int len;
    register char *cp;

    cp = buf;
    while (totlen > 0)
	{
        m = mBufClGet (M_DONTWAIT, MT_DATA, totlen, FALSE);

        if (m == NULL)
            {
            if (top != NULL)
                m_freem (top);
            return (NULL);
            }
        
	if (top == NULL)
	    {
            m->m_flags 		|= M_PKTHDR; 
	    m->m_pkthdr.rcvif 	= ifp;
	    m->m_pkthdr.len 	= totlen;
	    len = min(totlen, m->m_extSize); 	/* length to copy */
	    if (m->m_extSize > max_hdr) 
		{
		/* avoid costly m_prepends done in the context of tNetTask,
		 * especially effective while forwarding packets.
		 */
		len = min (len, (m->m_extSize - max_linkhdr)); 
		m->m_data += max_linkhdr;
		}
	    }
	else
            {
	    len = min(totlen, m->m_extSize); 	/* length to copy */
            }
	m->m_len = len; 		/* initialize the length of mbuf */
        
	if (copy)
	    (*copy)(cp, mtod(m, caddr_t), (unsigned)len);
	else
            {
            switch (width)
                {
                case NONE:
                    bcopy(cp, mtod(m, caddr_t), (unsigned)len);
                    break;

                case 1:
                    bcopyBytes (cp, mtod(m, caddr_t), len);
                    break;

                case 2:
                    bcopyWords (cp, mtod(m, caddr_t), (len + 1) >> 1);
                    break;

                case 4:
                    bcopyLongs (cp, mtod(m, caddr_t), (len + 3) >> 2);
                    break;
                        
                default:
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
                    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_EMERGENCY, 35, 3,
                             WV_NETEVENT_DEVMEM_PANIC, width)
#endif  /* INCLUDE_WVNET */
#endif

                    panic ("m_devget -- invalid mem copy width");
                    break;
                }
            }

	cp += len;		/* advance the pointer */
	*mp = m;
	mp = &m->m_next;	/* advance the mbuf */
	totlen -= len;		/* decrment the total length */
	}

    return (top);		/* return the filled mbuf chain */
    }

#if 0 /* XXX */
/* both m_copyback and m_copydata have to be reworked, these functions
 * are only used in rt_sock.c. They have been moved to rt_sock.c.
 * They have to be reworked and integrated into netBufLib.c
 */

/*
 * Copy data from a buffer back into the indicated mbuf chain,
 * starting "off" bytes from the beginning, extending the mbuf
 * chain if necessary.
 */
void
m_copyback(m0, off, len, cp)
	struct	mbuf *m0;
	register int off;
	register int len;
	caddr_t cp;
{
	register int mlen;
	register struct mbuf *m = m0, *n;
	int totlen = 0;

	if (m0 == 0)
		return;
	while (off > (mlen = m->m_len)) {
		off -= mlen;
		totlen += mlen;
		if (m->m_next == 0) {
			n = m_getclr(M_DONTWAIT, m->m_type, CL_SIZE_128, TRUE);
			if (n == 0)
				goto out;
			n->m_len = min(n->m_extSize, len + off);
			m->m_next = n;
		}
		m = m->m_next;
	}
	while (len > 0) {
		mlen = min (m->m_len - off, len);
		bcopy(cp, off + mtod(m, caddr_t), (unsigned)mlen);
		cp += mlen;
		len -= mlen;
		mlen += off;
		off = 0;
		totlen += mlen;
		if (len == 0)
			break;
		if (m->m_next == 0) {
			n = mBufClGet(M_DONTWAIT, m->m_type, CL_SIZE_128, TRUE);
			if (n == 0)
				break;
			n->m_len = min(n->m_extSize, len);
			m->m_next = n;
		}
		m = m->m_next;
	}
out:	if (((m = m0)->m_flags & M_PKTHDR) && (m->m_pkthdr.len < totlen))
		m->m_pkthdr.len = totlen;
}

/*******************************************************************************
*
* m_copydata - copy data from an mbuf chain into a buff
* Copy data from an mbuf chain starting "off" bytes from the beginning,
* continuing for "len" bytes, into the indicated buffer.
*/
void m_copydata(m, off, len, cp)
    register struct mbuf *m;
    register int off;
    register int len;
    caddr_t cp;
    {
    register unsigned count;

    if (off < 0 || len < 0)
	panic("m_copydata");
    while (off > 0)
	{
	if (m == 0)
	    panic("m_copydata");
	if (off < m->m_len)
	    break;
	off -= m->m_len;
	m = m->m_next;
	}
    while (len > 0)
	{
	if (m == 0)
	    panic("m_copydata");
	count = min(m->m_len - off, len);
	bcopy(mtod(m, caddr_t) + off, cp, count);
	len -= count;
	cp += count;
	off = 0;
	m = m->m_next;
	}
    }

/* XXX merged with netMblkChainDup in netBufLib.c */
LOCAL int 	MCFail;	 	/* XXX temp variable pullup fails */
/*******************************************************************************
*
* m_copym - make a copy of an mbuf chain.
*
* Make a copy of an mbuf chain starting "off0" bytes from the beginning,
* continuing for "len" bytes.  If len is M_COPYALL, copy to end of mbuf.
* The wait parameter is a choice of M_WAIT/M_DONTWAIT from caller.
*/

struct mbuf * m_copym(m, off0, len, wait)
    register struct mbuf *m;
    int off0, wait;
    register int len;
    {
    register struct mbuf *n, **np;
    register int off = off0;
    struct mbuf *top;
    int copyhdr = 0;

    if (off < 0 || len < 0)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
        WV_NET_MARKER_3 (NET_AUX_EVENT, WV_NET_EMERGENCY, 36, 4,
                         WV_NETEVENT_MEMCOPY_PANIC, off, len, m)
#endif  /* INCLUDE_WVNET */
#endif

        panic("m_copym");
        }

    if (off == 0 && m->m_flags & M_PKTHDR)
	copyhdr = 1;
    while (off > 0)
	{
	if (m == 0)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_MARKER_3 (NET_AUX_EVENT, WV_NET_EMERGENCY, 36, 4,
                             WV_NETEVENT_MEMCOPY_PANIC, off, len, m)
#endif  /* INCLUDE_WVNET */
#endif

	    panic("m_copym");
            }

	if (off < m->m_len)
	    break;
	off -= m->m_len;
	m = m->m_next;
	}
    np = &top;
    top = 0;

    while (len > 0)
	{
	if (m == 0)
	    {
	    if (len != M_COPYALL)
                {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
                WV_NET_MARKER_3 (NET_AUX_EVENT, WV_NET_EMERGENCY, 36, 4,
                                 WV_NETEVENT_MEMCOPY_PANIC, off, len, m)
#endif  /* INCLUDE_WVNET */
#endif

                panic("m_copym");
                }
	    break;
	    }
	n = mBlkGet(_pNetDpool, wait, m->m_type);
	if ((*np = n) == NULL)
	    goto nospace;
	if (copyhdr)
	    {
            n->m_pkthdr = m->m_pkthdr;
	    if (len == M_COPYALL)
		n->m_pkthdr.len -= off0;
	    else
		n->m_pkthdr.len = len;
	    copyhdr = 0;
	    }
	n->m_flags = m->m_flags; 
	n->m_len = min(len, m->m_len - off);
	n->m_data = m->m_data + off;
	n->m_ext = m->m_ext;
	++(n->m_extRefCnt);

	if (len != M_COPYALL)
	    len -= n->m_len;
	off = 0;
	m = m->m_next;
	np = &n->m_next;
	}

    if (top == 0)
	MCFail++;
    return (top);

    nospace:
    m_freem(top);
    MCFail++;
    return (0);
    }

/*
 * XXX merged with netMblkClFree & netMblkClChainFree in netBufLib.c
 * turned m_free and m_freem as macros in mbuf.h
 */
/*******************************************************************************
*
* m_free - free the mBlk and cluster pair
*
* This function frees the mBlk and also the cluster if the mBlk points to it.
* This function returns a pointer to the next mBlk connected to the one that
* is freed.
*
* NOMANUAL
* 
* RETURNS mBLK/NULL
*/

struct mbuf * m_free
    (
    FAST struct mbuf * 	pMblk		/* pointer to the mBlk to free */
    )
    {
    FAST struct mbuf *	pMblkNext;	/* pointer to the next mBlk */
    NET_POOL_ID		pNetPool;	/* pointer to net pool */
    
    if (pMblk->m_type == MT_FREE)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_EMERGENCY, 37, 5,
                         WV_NETEVENT_MEMFREE_PANIC, pMblk)
#endif  /* INCLUDE_WVNET */
#endif

        panic("m_free");
        }

    pNetPool 	= MBLK_TO_NET_POOL(pMblk); 
    pMblkNext 	= pMblk->m_next;

    /* free the cluster first if it is attached to the mBlk */
    
    if (M_HASCL(pMblk))			
        clBlkFree (pMblk->pClBlk->pNetPool, pMblk->pClBlk);

    mBlkFree (pNetPool, pMblk); 		/* free the mBlk */

    return (pMblkNext);
    }

/*******************************************************************************
*
* m_freem - free a list of mbufs linked via m_next
*
* N.B.: If MFREE macro changes in mbuf.h m_freem should also change
* 	accordingly.
*/

void m_freem(m)
    FAST struct mbuf *m;
    {
    FAST struct mbuf *n;

    if (m == NULL)
	return;
    do
	{
	n = m_free (m); 
	}
    while ((m = n));
    }
#endif /* XXX have to be reworked */

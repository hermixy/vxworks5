/* uipc_sock2.c - uipc primitive socket routines */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993
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
 *	@(#)uipc_socket2.c	8.2 (Berkeley) 2/14/95
 */

/*
modification history
--------------------
01g,15oct01,rae  merge from truestack ver 01m, base 01e (SPR #34005 etc.)
01f,12nov98,n_s  fixed sbcompress to work with netBufLib buffers. spr # 22966
01e,19sep97,vin  changed m_ext.ext_size to m_extSize
01d,11aug97,vin  adjusted sb_max to 5 *cc and low water mark to CL_SIZE_64
01c,03dec96,vin	 changed calloc(..) to MALLOC(..) and free(..) to FREE(..)
		 to use network buffers.
01b,22nov96,vin	 added cluster support, replace m_get(..) with mBufClGet(..). 
01a,03mar96,vin  created from BSD4.4lite2. Integrated with 02t of uipc_sock2.c
*/

/*
DESCRIPTION
*/


#ifdef BSDUNIX44 

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#else

#include "vxWorks.h"
#include "semLib.h"
#include "memLib.h"

#include "errno.h"
#include "net/mbuf.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "net/socketvar.h"
#include "net/systm.h"
#include "net/unixLib.h"
#include "stdio.h"

#endif /* BSDUNIX44 */

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif    /* VIRTUAL_STACK */

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_UISOCK2_MODULE; /* Value for uipc_sock2.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

/* globals */

VOIDFUNCPTR sowakeupHook = NULL;	/* user callback in sowakeup() */

#ifndef VIRTUAL_STACK
u_long	sb_max = SB_MAX;		/* patchable max socket buffer size*/
#endif    /* VIRTUAL_STACK */

/*
 * Primitive routines for operating on sockets and socket buffers
 */

/*
 * Procedures to manipulate state flags of socket
 * and do appropriate wakeups.  Normal sequence from the
 * active (originating) side is that soisconnecting() is
 * called during processing of connect() call,
 * resulting in an eventual call to soisconnected() if/when the
 * connection is established.  When the connection is torn down
 * soisdisconnecting() is called during processing of disconnect() call,
 * and soisdisconnected() is called when the connection to the peer
 * is totally severed.  The semantics of these routines are such that
 * connectionless protocols can call soisconnected() and soisdisconnected()
 * only, bypassing the in-progress calls when setting up a ``connection''
 * takes no time.
 *
 * From the passive side, a socket is created with
 * two queues of sockets: so_q0 for connections in progress
 * and so_q for connections already made and awaiting user acceptance.
 * As a protocol is preparing incoming connections, it creates a socket
 * structure queued on so_q0 by calling sonewconn().  When the connection
 * is established, soisconnected() is called, and transfers the
 * socket structure to so_q, making it available to accept().
 * 
 * If a socket is closed with sockets on either
 * so_q0 or so_q, these sockets are dropped.
 *
 * If higher level protocols are implemented in
 * the kernel, the wakeups done here will sometimes
 * cause software-interrupt process scheduling.
 */

void
soisconnecting(so)
	register struct socket *so;
{

	so->so_state &= ~(SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= SS_ISCONNECTING;
}

void
soisconnected(so)
	register struct socket *so;
{
	register struct socket *head = so->so_head;

	so->so_state &= ~(SS_ISCONNECTING|SS_ISDISCONNECTING|SS_ISCONFIRMING);
	so->so_state |= SS_ISCONNECTED;
	if (head && soqremque(so, 0)) {
		soqinsque(head, so, 1);
		sorwakeup(head);
		wakeup(head->so_timeoSem);
	} else {
	        wakeup(so->so_timeoSem);
		sorwakeup(so);
		sowwakeup(so);
	}
}

void
soisdisconnecting(so)
	register struct socket *so;
{

	so->so_state &= ~SS_ISCONNECTING;
	so->so_state |= (SS_ISDISCONNECTING|SS_CANTRCVMORE|SS_CANTSENDMORE);
	wakeup(so->so_timeoSem);
	sowwakeup(so);
	sorwakeup(so);
}

void
soisdisconnected(so)
	register struct socket *so;
{

	so->so_state &= ~(SS_ISCONNECTING|SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= (SS_CANTRCVMORE|SS_CANTSENDMORE);
        if (so->so_timeoSem)
	    wakeup(so->so_timeoSem);
	sowwakeup(so);
	sorwakeup(so);
}

/*
 * When an attempt at a new connection is noted on a socket
 * which accepts connections, sonewconn is called.  If the
 * connection is possible (subject to space constraints, etc.)
 * then we allocate a new structure, propoerly linked into the
 * data structure of the original socket, and return this.
 * Connstatus may be 0, or SO_ISCONFIRMING, or SO_ISCONNECTED.
 *
 * Currently, sonewconn() is defined as sonewconn1() in socketvar.h
 * to catch calls that are missing the (new) second parameter.
 */
struct socket *
sonewconn1(head, connstatus)
	register struct socket *head;
	int connstatus;
{
	register struct socket *so;
	int soqueue = connstatus ? 1 : 0;

	if (head->so_qlen + head->so_q0len > 3 * head->so_qlimit / 2)
		return ((struct socket *)0);

        MALLOC(so, struct socket *, sizeof(*so), MT_SOCKET, M_DONTWAIT);
        if (so == NULL)
                return ((struct socket *)0);
        bzero((caddr_t)so, sizeof(*so));
	so->so_type = head->so_type;
	so->so_options = head->so_options &~ SO_ACCEPTCONN;
	so->so_linger = head->so_linger;
	so->so_state = head->so_state | SS_NOFDREF;
	so->so_proto = head->so_proto;
	so->so_timeo = head->so_timeo;
	so->so_pgrp = head->so_pgrp;

	/* initialize the socket's semaphores */

	so->so_timeoSem = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
        if (so->so_timeoSem == NULL)
            {
            FREE(so, MT_SOCKET);
            return ((struct socket *)0);
            }
	so->so_rcv.sb_Sem = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
        if (so->so_rcv.sb_Sem == NULL)
            {
            semDelete (so->so_timeoSem);
            FREE(so, MT_SOCKET);
            return ((struct socket *)0);
            }
	so->so_snd.sb_Sem = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
        if (so->so_snd.sb_Sem == NULL)
            {
            semDelete (so->so_timeoSem);
            semDelete (so->so_rcv.sb_Sem);
            FREE(so, MT_SOCKET);
            return ((struct socket *)0);
            }

        /* initialize the select stuff */

	selWakeupListInit (&so->so_selWakeupList);
 
        so->selectFlag = TRUE;

	(void) soreserve(so, head->so_snd.sb_hiwat, head->so_rcv.sb_hiwat);
	soqinsque(head, so, soqueue);
	if ((*so->so_proto->pr_usrreq)(so, PRU_ATTACH,
	    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0)) {
		(void) soqremque(so, soqueue);
		semDelete (so->so_timeoSem);
		semDelete (so->so_rcv.sb_Sem);
		semDelete (so->so_snd.sb_Sem);
		selWakeupListTerm (&so->so_selWakeupList);
		FREE(so, MT_SOCKET);
		return ((struct socket *)0);
	}
	if (connstatus) {
		sorwakeup(head);
		wakeup(head->so_timeoSem);
		so->so_state |= connstatus;
	}
	return (so);
}

void
soqinsque(head, so, q)
	register struct socket *head, *so;
	int q;
{

	register struct socket **prev;
	so->so_head = head;
	if (q == 0) {
		head->so_q0len++;
		so->so_q0 = 0;
		for (prev = &(head->so_q0); *prev; )
			prev = &((*prev)->so_q0);
	} else {
		head->so_qlen++;
		so->so_q = 0;
		for (prev = &(head->so_q); *prev; )
			prev = &((*prev)->so_q);
	}
	*prev = so;
}

int
soqremque(so, q)
	register struct socket *so;
	int q;
{
	register struct socket *head, *prev, *next;

	head = so->so_head;
	prev = head;
	for (;;) {
		next = q ? prev->so_q : prev->so_q0;
		if (next == so)
			break;
		if (next == 0)
			return (0);
		prev = next;
	}
	if (q == 0) {
		prev->so_q0 = next->so_q0;
		head->so_q0len--;
	} else {
		prev->so_q = next->so_q;
		head->so_qlen--;
	}
	next->so_q0 = next->so_q = 0;
	next->so_head = 0;
	return (1);
}

/*
 * Socantsendmore indicates that no more data will be sent on the
 * socket; it would normally be applied to a socket when the user
 * informs the system that no more data is to be sent, by the protocol
 * code (in case PRU_SHUTDOWN).  Socantrcvmore indicates that no more data
 * will be received, and will normally be applied to the socket by a
 * protocol when it detects that the peer will send no more data.
 * Data queued for reading in the socket may yet be read.
 */

void
socantsendmore(so)
	struct socket *so;
{

	so->so_state |= SS_CANTSENDMORE;
	sowwakeup(so);
}

void
socantrcvmore(so)
	struct socket *so;
{

	so->so_state |= SS_CANTRCVMORE;
	sorwakeup(so);
}

/*
 * Queue a process for a select on a socket buffer.
 */

void
sbselqueue(so, sb, wakeupNode)
	struct socket *so;
        struct sockbuf *sb;
	SEL_WAKEUP_NODE *wakeupNode;
{
	if (selNodeAdd (&so->so_selWakeupList, wakeupNode) == ERROR)
	    return;

	/* this is race free ONLY if we assume we're at splnet - rdc */
	sb->sb_sel = (struct proc *) 1;
}

void
sbseldequeue(so, sb, wakeupNode)
    struct socket *so;
    struct sockbuf *sb;
    SEL_WAKEUP_NODE *wakeupNode;
    {
    selNodeDelete (&so->so_selWakeupList, wakeupNode);

    /* this is race free ONLY if we assume we're at splnet - rdc */
    if (selWakeupListLen (&so->so_selWakeupList) == 0)
	sb->sb_sel = (struct proc *) 0;
    }


/*
 * Wait for data to arrive at/drain from a socket buffer.
 */
void sbwait
    (
    struct sockbuf *sb
    )
    {
    sb->sb_want++;
    sb->sb_flags |= SB_WAIT;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_0 (NET_CORE_EVENT, WV_NET_INFO, 53, 5,
                     WV_NETEVENT_SBWAIT_SLEEP)
#endif  /* INCLUDE_WVNET */
#endif

    ksleep (sb->sb_Sem);
    }


/* 
 * Lock a sockbuf already known to be locked;
 * return any error returned from sleep (EINTR).
 */
int
sb_lock(sb)
	register struct sockbuf *sb;
{

	while (sb->sb_flags & SB_LOCK) {
		sb->sb_flags |= SB_WANT;
		ksleep(sb->sb_Sem);
	}
	sb->sb_flags |= SB_LOCK;
	return (0);
}

/*
 * Wakeup processes waiting on a socket buffer.
 */

void sbwakeup
    (
    struct socket *so,
    register struct sockbuf *sb,
    SELECT_TYPE wakeupType
    )
    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_2 (NET_CORE_EVENT, WV_NET_INFO, 54, 6, 
                     WV_NETEVENT_SBWAKEUP_START, so->so_fd, wakeupType)
#endif  /* INCLUDE_WVNET */
#endif

    if (sb->sb_want > 0)
        {
        sb->sb_want--;
        if (sb->sb_Sem)
            wakeup (sb->sb_Sem);
        }
    else
        sb->sb_want = 0;

    if (sb->sb_sel)
        {
        /* 
         * Wake up everybody on selectTask list.
         * (Assume this is running in netTask context).
         */

        if (so->selectFlag)
            selWakeupAll (&so->so_selWakeupList, wakeupType);
        }
    }


/*
 * Wakeup socket readers and writers.
 */
/* ARGSUSED */
void sowakeup(so, sb, wakeupType)
	struct socket *so;
	struct sockbuf *sb;
	SELECT_TYPE wakeupType;
{
        sbwakeup(so, sb, wakeupType);

	/* user callback, instead of full signals implementation */

	if (sowakeupHook != NULL)
	     (*sowakeupHook) (so, wakeupType);

#ifdef	BERKELEY
	/* XXX one day asynchronous IO will work... -gae */

        register struct proc *p;

        if (so->so_state & SS_ASYNC) {
                if (so->so_pgrp < 0)
                        gsignal(-so->so_pgrp, SIGIO);
                else if (so->so_pgrp > 0 && (p = pfind(so->so_pgrp)) != 0)
                        psignal(p, SIGIO);
        }
#endif	/* BERKELEY */
}

/*
 * Socket buffer (struct sockbuf) utility routines.
 *
 * Each socket contains two socket buffers: one for sending data and
 * one for receiving data.  Each buffer contains a queue of mbufs,
 * information about the number of mbufs and amount of data in the
 * queue, and other fields allowing select() statements and notification
 * on data availability to be implemented.
 *
 * Data stored in a socket buffer is maintained as a list of records.
 * Each record is a list of mbufs chained together with the m_next
 * field.  Records are chained together with the m_nextpkt field. The upper
 * level routine soreceive() expects the following conventions to be
 * observed when placing information in the receive buffer:
 *
 * 1. If the protocol requires each message be preceded by the sender's
 *    name, then a record containing that name must be present before
 *    any associated data (mbuf's must be of type MT_SONAME).
 * 2. If the protocol supports the exchange of ``access rights'' (really
 *    just additional data associated with the message), and there are
 *    ``rights'' to be received, then a record containing this data
 *    should be present (mbuf's must be of type MT_RIGHTS).
 * 3. If a name or rights record exists, then it must be followed by
 *    a data record, perhaps of zero length.
 *
 * Before using a new socket structure it is first necessary to reserve
 * buffer space to the socket, by calling sbreserve().  This should commit
 * some of the available buffer space in the system buffer pool for the
 * socket (currently, it does nothing but enforce limits).  The space
 * should be released by calling sbrelease() when the socket is destroyed.
 */

int
soreserve(so, sndcc, rcvcc)
	register struct socket *so;
	u_long sndcc, rcvcc;
{

	if (sbreserve(&so->so_snd, sndcc) == 0)
		goto bad;
	if (sbreserve(&so->so_rcv, rcvcc) == 0)
		goto bad2;
	if (so->so_rcv.sb_lowat == 0)
		so->so_rcv.sb_lowat = 1;
	if (so->so_snd.sb_lowat == 0)
		so->so_snd.sb_lowat = CL_SIZE_64;
	if (so->so_snd.sb_lowat > so->so_snd.sb_hiwat)
		so->so_snd.sb_lowat = so->so_snd.sb_hiwat;
	return (0);
bad2:
	sbrelease(&so->so_snd);
bad:
	return (ENOBUFS);
}

/*
 * Allot mbufs to a sockbuf.
 * Attempt to scale mbmax so that mbcnt doesn't become limiting
 * if buffering efficiency is near the normal case.
 */
int
sbreserve(sb, cc)
	struct sockbuf *sb;
	u_long cc;
{

	if (cc > sb_max * MCLBYTES / (MSIZE + MCLBYTES))
		return (0);
	sb->sb_hiwat = cc;
	sb->sb_mbmax = min(cc * 5, sb_max);
	if (sb->sb_lowat > sb->sb_hiwat)
		sb->sb_lowat = sb->sb_hiwat;
	return (1);
}

/*
 * Free mbufs held by a socket, and reserved mbuf space.
 */
void
sbrelease(sb)
	struct sockbuf *sb;
{

	sbflush(sb);
	sb->sb_hiwat = sb->sb_mbmax = 0;
}

/*
 * Routines to add and remove
 * data from an mbuf queue.
 *
 * The routines sbappend() or sbappendrecord() are normally called to
 * append new mbufs to a socket buffer, after checking that adequate
 * space is available, comparing the function sbspace() with the amount
 * of data to be added.  sbappendrecord() differs from sbappend() in
 * that data supplied is treated as the beginning of a new record.
 * To place a sender's address, optional access rights, and data in a
 * socket receive buffer, sbappendaddr() should be used.  To place
 * access rights and data in a socket receive buffer, sbappendrights()
 * should be used.  In either case, the new data begins a new record.
 * Note that unlike sbappend() and sbappendrecord(), these routines check
 * for the caller that there will be enough space to store the data.
 * Each fails if there is not enough space, or if it cannot find mbufs
 * to store additional information in.
 *
 * Reliable protocols may use the socket send buffer to hold data
 * awaiting acknowledgement.  Data is normally copied from a socket
 * send buffer in a protocol with m_copy for output to a peer,
 * and then removing the data from the socket buffer with sbdrop()
 * or sbdroprecord() when the data is acknowledged by the peer.
 */

/*
 * Append mbuf chain m to the last record in the
 * socket buffer sb.  The additional space associated
 * the mbuf chain is recorded in sb.  Empty mbufs are
 * discarded and mbufs are compacted where possible.
 */
void
sbappend(sb, m)
	struct sockbuf *sb;
	struct mbuf *m;
{
	register struct mbuf *n;

	if (m == 0)
		return;
	if ((n = sb->sb_mb)) {
		while (n->m_nextpkt)
			n = n->m_nextpkt;
		do {
			if (n->m_flags & M_EOR) {
				sbappendrecord(sb, m); /* XXXXXX!!!! */
				return;
			}
		} while (n->m_next && (n = n->m_next));
	}
	sbcompress(sb, m, n);
}

#ifdef SOCKBUF_DEBUG
void
sbcheck(sb)
	register struct sockbuf *sb;
{
	register struct mbuf *m;
	register int len = 0, mbcnt = 0;

	for (m = sb->sb_mb; m; m = m->m_next) {
		len += m->m_len;
		mbcnt += MSIZE;
		if (m->m_flags & M_EXT)
			mbcnt += m->m_extSize;
		if (m->m_nextpkt)
			panic("sbcheck nextpkt");
	}
	if (len != sb->sb_cc || mbcnt != sb->sb_mbcnt) {
		printf("cc %d != %d || mbcnt %d != %d\n", len, sb->sb_cc,
		    mbcnt, sb->sb_mbcnt);
		panic("sbcheck");
	}
}
#endif

/*
 * As above, except the mbuf chain
 * begins a new record.
 */
void
sbappendrecord(sb, m0)
	register struct sockbuf *sb;
	register struct mbuf *m0;
{
	register struct mbuf *m;

	if (m0 == 0)
		return;
	if ((m = sb->sb_mb) != NULL)
		while (m->m_nextpkt)
			m = m->m_nextpkt;
	/*
	 * Put the first mbuf on the queue.
	 * Note this permits zero length records.
	 */
	sballoc(sb, m0);
	if (m)
		m->m_nextpkt = m0;
	else
		sb->sb_mb = m0;
	m = m0->m_next;
	m0->m_next = 0;
	if (m && (m0->m_flags & M_EOR)) {
		m0->m_flags &= ~M_EOR;
		m->m_flags |= M_EOR;
	}
	sbcompress(sb, m, m0);
}

/*
 * As above except that OOB data
 * is inserted at the beginning of the sockbuf,
 * but after any other OOB data.
 */
void
sbinsertoob(sb, m0)
	register struct sockbuf *sb;
	register struct mbuf *m0;
{
	register struct mbuf *m;
	register struct mbuf **mp;

	if (m0 == 0)
		return;
	for (mp = &sb->sb_mb; (m = *mp) != NULL; mp = &((*mp)->m_nextpkt)) {
	    again:
		switch (m->m_type) {

		case MT_OOBDATA:
			continue;		/* WANT next train */

		case MT_CONTROL:
			if ((m = m->m_next) != NULL)
				goto again;	/* inspect THIS train further */
		}
		break;
	}
	/*
	 * Put the first mbuf on the queue.
	 * Note this permits zero length records.
	 */
	sballoc(sb, m0);
	m0->m_nextpkt = *mp;
	*mp = m0;
	m = m0->m_next;
	m0->m_next = 0;
	if (m && (m0->m_flags & M_EOR)) {
		m0->m_flags &= ~M_EOR;
		m->m_flags |= M_EOR;
	}
	sbcompress(sb, m, m0);
}

/*
 * Append address and data, and optionally, control (ancillary) data
 * to the receive queue of a socket.  If present,
 * m0 must include a packet header with total length.
 * Returns 0 if no space in sockbuf or insufficient mbufs.
 */
int
sbappendaddr(sb, asa, m0, control)
	register struct sockbuf *sb;
	struct sockaddr *asa;
	struct mbuf *m0, *control;
{
	register struct mbuf *m, *n;
	int space = asa->sa_len;

    if (m0 && (m0->m_flags & M_PKTHDR) == 0)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
        WV_NET_EVENT_0 (NET_CORE_EVENT, WV_NET_EMERGENCY, 41, 1, 
                        WV_NETEVENT_SBAPPENDADDR_PANIC, WV_NET_RECV)
#endif  /* INCLUDE_WVNET */
#endif

        panic("sbappendaddr");
        }

	if (m0)
		space += m0->m_pkthdr.len;
	for (n = control; n; n = n->m_next) {
		space += n->m_len;
		if (n->m_next == 0)	/* keep pointer to last control buf */
			break;
	}
	if (space > sbspace(sb))
		return (0);
	if (asa->sa_len > CL_SIZE_128)
		return (0);
	m = mBufClGet(M_DONTWAIT, MT_SONAME, CL_SIZE_128, TRUE);
	if (m == 0)
		return (0);
	m->m_len = asa->sa_len;
	bcopy((caddr_t)asa, mtod(m, caddr_t), asa->sa_len);
	if (n)
		n->m_next = m0;		/* concatenate data to control */
	else
		control = m0;
	m->m_next = control;
	for (n = m; n; n = n->m_next)
		sballoc(sb, n);
	if ((n = sb->sb_mb) != NULL) {
		while (n->m_nextpkt)
			n = n->m_nextpkt;
		n->m_nextpkt = m;
	} else
		sb->sb_mb = m;
	return (1);
}

int
sbappendcontrol(sb, m0, control)
	struct sockbuf *sb;
	struct mbuf *m0, *control;
{
	register struct mbuf *m, *n;
	int space = 0;

    if (control == 0)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
        WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_EMERGENCY, 42, 2, 
                         WV_NETEVENT_SBAPPENDCTRL_PANIC)
#endif  /* INCLUDE_WVNET */
#endif

        panic ("sbappendcontrol");
        }

	for (m = control; ; m = m->m_next) {
		space += m->m_len;
		if (m->m_next == 0)
			break;
	}
	n = m;			/* save pointer to last control buffer */
	for (m = m0; m; m = m->m_next)
		space += m->m_len;
	if (space > sbspace(sb))
		return (0);
	n->m_next = m0;			/* concatenate data to control */
	for (m = control; m; m = m->m_next)
		sballoc(sb, m);
	if ((n = sb->sb_mb) != NULL) {
		while (n->m_nextpkt)
			n = n->m_nextpkt;
		n->m_nextpkt = control;
	} else
		sb->sb_mb = control;
	return (1);
}

/*
 * Compress mbuf chain m into the socket
 * buffer sb following mbuf n.  If n
 * is null, the buffer is presumed empty.
 */
void
sbcompress(sb, m, n)
	register struct sockbuf *sb;
	register struct mbuf *m, *n;
{
	register int eor = 0;
	register struct mbuf *o;

	while (m) {
		eor |= m->m_flags & M_EOR;
		if (m->m_len == 0 &&
		    (eor == 0 ||
		     (((o = m->m_next) || (o = n)) &&
		      o->m_type == m->m_type))) {
			m = m_free(m);
			continue;
		}

		/*
		 * This test determines whether or not the data in <m> will
		 * be copied to the cluster referenced by <n>.  The conditions
		 * which are required for compression are:
		 *     1) <n> is non-NULL
		 *     2) <n> is not the end of a record.  M_EOR is set for
		 *        datagram protocols (UDP, raw IP) but not stream
		 *        protocols (TCP)
		 *     3) length of data in <m> is less than CL_SIZE_MIN.  This
		 *        condition optimizes buffer usage for small clusters
		 *        and optimizes speed for large clusters, by not 
		 *        copying data in those cases.
		 *     4) The data in <m> must fit in the remaining space in 
		 *        the cluster referenced by <n>
		 *     5) <m> and <n> must contain the same type of data
		 */

		if (n && 
		    (n->m_flags & M_EOR) == 0 && 
		    m->m_len < CL_SIZE_MIN &&
		    (n->m_data + n->m_len + m->m_len) < 
		    (n->m_extBuf + n->m_extSize) &&
		    n->m_type == m->m_type) 
		    {
		    bcopy(mtod(m, caddr_t), mtod(n, caddr_t) + n->m_len,
			  (unsigned)m->m_len);
		    n->m_len += m->m_len;
		    sb->sb_cc += m->m_len;
		    m = m_free(m);
		    continue;
		    }
		if (n)
			n->m_next = m;
		else
			sb->sb_mb = m;
		sballoc(sb, m);
		n = m;
		m->m_flags &= ~M_EOR;
		m = m->m_next;
		n->m_next = 0;
	}
	if (eor) {
		if (n)
			n->m_flags |= eor;
		else
			printf("semi-panic: sbcompress\n");
	}
}

/*
 * Free all mbufs in a sockbuf.
 * Check that all resources are reclaimed.
 */

void sbflush
    (
    register struct sockbuf *sb
    )
    {
    if (sb->sb_flags & SB_LOCK)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_EMERGENCY, 43, 3, 
                         WV_NETEVENT_SBFLUSH_PANIC, 1)
#endif  /* INCLUDE_WVNET */
#endif

        panic("sbflush");
        }

    while (sb->sb_mbcnt)
        sbdrop (sb, (int)sb->sb_cc);
    if (sb->sb_cc || sb->sb_mb)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_EMERGENCY, 43, 3, 
                         WV_NETEVENT_SBFLUSH_PANIC, 0)
#endif  /* INCLUDE_WVNET */
#endif

        panic ("sbflush 2");
        }
    }

/*
 * Drop data from (the front of) a sockbuf.
 */
void sbdrop
    (
    register struct sockbuf *sb,
    register int len
    )
    {
    register struct mbuf *m, *mn;
    struct mbuf *next;

    next = (m = sb->sb_mb) ? m->m_nextpkt : 0;
    while (len > 0) 
        {
        if (m == 0) 
            {
            if (next == 0)
                {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
                WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_EMERGENCY, 44, 4, 
                                 WV_NETEVENT_SBDROP_PANIC)
#endif  /* INCLUDE_WVNET */
#endif

                panic ("sbdrop");
                }
            m = next;
            next = m->m_nextpkt;
            continue;
            }
        if (m->m_len > len)
            {
            m->m_len -= len;
            m->m_data += len;
            sb->sb_cc -= len;
            break;
            }
        len -= m->m_len;
        sbfree (sb, m);
        mn = m_free (m);
        m = mn;
        }
    while (m && m->m_len == 0)
        {
        sbfree (sb, m);
        mn = m_free (m);
        m = mn;
        }
    if (m)
        {
        sb->sb_mb = m;
        m->m_nextpkt = next;
        }
    else
        sb->sb_mb = next;
    }

/*
 * Drop a record off the front of a sockbuf
 * and move the next record to the front.
 */
void
sbdroprecord(sb)
	register struct sockbuf *sb;
{
	register struct mbuf *m, *mn;

	m = sb->sb_mb;
	if (m) {
		sb->sb_mb = m->m_nextpkt;
		do {
			sbfree(sb, m);
			mn = m_free(m);
		} while ((m = mn));
	}
}

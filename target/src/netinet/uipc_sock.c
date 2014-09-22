/* uipc_sock.c - uipc socket routines */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
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
 *	@(#)uipc_socket.c	8.6 (Berkeley) 5/2/95
 */

/*
modification history
--------------------
01l,05jun02,vvv  fixed Nagle for large writes (SPR #72213)
01k,12oct01,rae  merge from truestack ver 01r, base 01h (SPR #64845 etc)
01j,07feb01,spm  updated socket handling to detect memory allocation failures
01i,24sep98,ham  considered if(m->m_extSize<max_hdr) in sosend() SPR#22209.
01h,26aug98,n_s  optimized processing of uio buffer in sosend. spr #22246.
01g,26aug98,n_s  added return val check for MALLOC in socreate and for 
                 mBufClGet in soreceive. spr #22238.
01f,11aug97,vin  fixed problems in sosend, adjusted space in sosend
01e,23jan97,vin  added fix for SPR7502.
01d,03dec96,vin  replaced calloc(..) with MALLOC(..), free(..) with FREE(..)
01c,22nov96,vin  added cluster support, replaced m_get with mBufClGet and
		 m_gethdr with mHdrClGet.
01b,29aug96,vin  added zerocopy interface. 
01a,03mar96,vin  created from BSD4.4lite2. Integrated with 02u of uipc_sock.c
*/

#include "vxWorks.h"
#include "semLib.h"
#include "memLib.h"

#include "errno.h"
#include "net/mbuf.h"
#include "net/domain.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "sys/times.h"
#include "net/socketvar.h"
#include "sys/ioctl.h"
#include "net/uio.h"
#include "net/route.h"
#include "netinet/in.h"
#include "net/if.h"
#include "net/systm.h"
#include "selectLib.h"
#include "sockLib.h"
#include "netLib.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_UISOCK_MODULE;  /* Value for uipc_sock.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

extern int uiomove();

/*
 * Socket operation routines.
 * These routines are called by the routines in
 * sys_socket.c or from a system process, and
 * implement the semantics of socket operations by
 * switching out to the protocol specific routines.
 */
/*ARGSUSED*/
int
socreate(dom, aso, type, proto)
	int dom;
	struct socket **aso;
	register int type;
	int proto;
{
	register struct protosw *prp;
	register struct socket *so;
	register int error;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_VERBOSE, 45, 11,
                     WV_NETEVENT_SOCREATE_START)
#endif  /* INCLUDE_WVNET */
#endif

	if (proto)
		prp = pffindproto(dom, proto, type);
	else
		prp = pffindtype(dom, type);
	if (prp == 0 || prp->pr_usrreq == 0)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
            WV_NET_MARKER_3 (NET_AUX_EVENT, WV_NET_ERROR, 43, 6,
                             WV_NETEVENT_SOCREATE_SEARCHFAIL,
                             dom, proto, type)
#endif  /* INCLUDE_WVNET */
#endif

            return (EPROTONOSUPPORT);
            }
	if (prp->pr_type != type)
            {
/* XXX - This event does not occur because the search routines will always
 *       return a protocol switch entry which matches the given type value.

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_ERROR event @/
            WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_ERROR, 44, 7,
                             WV_NETEVENT_SOCREATE_BADTYPE, prp->pr_type, type)
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */
            return (EPROTOTYPE);
            }

        MALLOC(so, struct socket *, sizeof(*so), MT_SOCKET, M_WAIT);
	if (so == (struct socket *) NULL)
	    {
	    return (ENOBUFS);
	    }

        bzero((caddr_t)so, sizeof(*so));

	so->so_options = 0; 
	so->so_type = type;

	/* all sockets will be priveledged */
	so->so_state = SS_PRIV;
	so->so_proto = prp;

	/* initialize socket semaphores */

	so->so_timeoSem = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
        if (so->so_timeoSem == NULL)
            {
            so->so_state |= SS_NOFDREF;
            sofree(so);
            return (ENOMEM);
            }
	so->so_rcv.sb_Sem = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
        if (so->so_rcv.sb_Sem == NULL)
            {
            so->so_state |= SS_NOFDREF;
            sofree(so);
            return (ENOMEM);
            }
	so->so_snd.sb_Sem = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
        if (so->so_snd.sb_Sem == NULL)
            {
            so->so_state |= SS_NOFDREF;
            sofree(so);
            return (ENOMEM);
            }

	so->so_rcv.sb_want = 0;
	so->so_snd.sb_want = 0;

	/* initialize the select stuff */

	selWakeupListInit (&so->so_selWakeupList);

        so->selectFlag = TRUE;

	error =
	    (*prp->pr_usrreq)(so, PRU_ATTACH, (struct mbuf *)0,
		(struct mbuf *)(long)proto, (struct mbuf *)0);
	if (error) {
		so->so_state |= SS_NOFDREF;
		sofree(so);
		return (error);
	}
	*aso = so;
	return (0);
}

int
sobind(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 46, 12,
                     WV_NETEVENT_SOBIND_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_BIND,
		(struct mbuf *)0, nam, (struct mbuf *)0);
	splx(s);
	return (error);
}

int
solisten(so, backlog)
	register struct socket *so;
	int backlog;
{
	int s = splnet(), error;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 47, 13,
                     WV_NETEVENT_SOLISTEN_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_LISTEN,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
	if (error) {
		splx(s);
		return (error);
	}
	if (so->so_q == 0)
		so->so_options |= SO_ACCEPTCONN;
	if (backlog < 0)
		backlog = 0;
	so->so_qlimit = min(backlog, SOMAXCONN);
	splx(s);
	return (0);
}

void sofree
    (
    register struct socket *so
    )
    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 48, 14,
                     WV_NETEVENT_SOFREE_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

    if (so->so_state & SS_NOFDREF)
        {
        /*
         * Always remove semaphores when closing socket. Existing TCP
         * connections exit after the next test, preventing correct
         * reclamation of these objects.
         */

        if (so->so_timeoSem)
            {
            semDelete (so->so_timeoSem);
            so->so_timeoSem = NULL;
            }
        if (so->so_snd.sb_Sem)
            {
            semDelete (so->so_snd.sb_Sem);
            so->so_snd.sb_Sem = NULL;
            }
        if (so->so_rcv.sb_Sem)
            {
            semDelete (so->so_rcv.sb_Sem);
            so->so_rcv.sb_Sem = NULL;
            }

        if (so->selectFlag)
            {
            selWakeupListTerm (&so->so_selWakeupList);
            so->selectFlag = FALSE;
            }
        }
 
    if (so->so_pcb || (so->so_state & SS_NOFDREF) == 0)
        return;

    if (so->so_head)
        {
        if (!soqremque (so, 0) && !soqremque (so, 1))
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_EMERGENCY, 38, 1,
                             WV_NETEVENT_SOFREE_PANIC, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

            panic ("sofree dq");
            }
        so->so_head = 0;
        }
    sbrelease (&so->so_snd);
    sorflush (so);

    FREE (so, MT_SOCKET); 
    }


/*
 * Close a socket on last file table reference removal.
 * Initiate disconnect if connected.
 * Free socket when disconnect complete.
 */

int soclose
    (
    register struct socket *so
    )
    {
    int s = splnet();		/* conservative */
    int error = 0;
    SEL_WAKEUP_NODE *pWakeupNode;
    SEL_WAKEUP_LIST *pWakeupList;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 49, 15,
                     WV_NETEVENT_SOCLOSE_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

    if (so->so_options & SO_ACCEPTCONN)
        {
        while (so->so_q0)
               (void) soabort(so->so_q0);
        while (so->so_q)
               (void) soabort(so->so_q);
        }
    if (so->so_pcb == 0)
        goto discard;
    if (so->so_state & SS_ISCONNECTED)
        {
        if ((so->so_state & SS_ISDISCONNECTING) == 0)
            {
            error = sodisconnect(so);
            if (error)
                goto drop;
            }
        if (so->so_options & SO_LINGER)
            {
            if ( (so->so_state & SS_ISDISCONNECTING) &&
                (so->so_state & SS_NBIO))
                goto drop;
            while (so->so_state & SS_ISCONNECTED)
                {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
                WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_INFO, 52, 10,
                                 WV_NETEVENT_SOCLOSE_WAIT, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

                ksleep(so->so_timeoSem);
                }
            }
        }
drop:
    if (so->so_pcb)
        {
        int error2 = (*so->so_proto->pr_usrreq) (so, PRU_DETACH,
                                                 (struct mbuf *)0, 
                                                 (struct mbuf *)0, 
                                                 (struct mbuf *)0);
        if (error == 0)
            error = error2;
        }
discard:

    /* clean up the select wakeup list for this socket */

    pWakeupList = &so->so_selWakeupList;

    if (lstCount (&pWakeupList->wakeupList) != 0)
        {
        semTake (&pWakeupList->listMutex, WAIT_FOREVER);

        while ((pWakeupNode =
  	       (SEL_WAKEUP_NODE *) lstFirst (&pWakeupList->wakeupList)))
	    soo_unselect (so, pWakeupNode);

	semGive (&pWakeupList->listMutex);
	}

    if (so->so_state & SS_NOFDREF)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_EMERGENCY, 39, 2,
                         WV_NETEVENT_SOCLOSE_PANIC, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif
    
        panic ("soclose: NOFDREF");
        }
    so->so_state |= SS_NOFDREF;
    sofree (so);
    splx (s);
    return (error);
    }

/*
 * Must be called at splnet...
 */
int soabort
    (
    struct socket *so
    )
    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 50, 16,
                     WV_NETEVENT_SOABORT_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

    return ( (*so->so_proto->pr_usrreq) (so, PRU_ABORT, (struct mbuf *)0,
                                         (struct mbuf *)0, (struct mbuf *)0));
    }

int
soaccept(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 51, 17,
                     WV_NETEVENT_SOACCEPT_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

    if ( (so->so_state & SS_NOFDREF) == 0)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_EMERGENCY, 40, 3,
                         WV_NETEVENT_SOACCEPT_PANIC, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

        panic("soaccept: !NOFDREF");
        }
    so->so_state &= ~SS_NOFDREF;
    error = (*so->so_proto->pr_usrreq)(so, PRU_ACCEPT, (struct mbuf *)0, 
                                       nam, (struct mbuf *)0);
    splx (s);
    return (error);
    }

int
soconnect(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int s;
	int error;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 52, 18,
                     WV_NETEVENT_SOCONNECT_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

	if (so->so_options & SO_ACCEPTCONN)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
            WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_ERROR, 45, 8,
                             WV_NETEVENT_SOCONNECT_BADSOCK, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

            return (EOPNOTSUPP);
            }

	s = splnet();
	/*
	 * If protocol is connection-based, can only connect once.
	 * Otherwise, if connected, try to disconnect first.
	 * This allows user to disconnect by connecting to, e.g.,
	 * a null address.
	 */
	if (so->so_state & (SS_ISCONNECTED|SS_ISCONNECTING) &&
	    ((so->so_proto->pr_flags & PR_CONNREQUIRED) ||
	    (error = sodisconnect(so))))
		error = EISCONN;
	else
		error = (*so->so_proto->pr_usrreq)(so, PRU_CONNECT,
		    (struct mbuf *)0, nam, (struct mbuf *)0);
	splx(s);
	return (error);
}

int
soconnect2(so1, so2)
	register struct socket *so1;
	struct socket *so2;
{
	int s = splnet();
	int error;

/*
 * XXX - This event cannot currently occur: the socket operation which uses
 *       the soconnect2() routine is not supported by VxWorks

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_VERBOSE event @/
    WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_VERBOSE, 53, 19,
                     WV_NETEVENT_SOCONNECT2_START, so1->so_fd, so2->so_fd)
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */

	error = (*so1->so_proto->pr_usrreq)(so1, PRU_CONNECT2,
	    (struct mbuf *)0, (struct mbuf *)so2, (struct mbuf *)0);
	splx(s);
	return (error);
}

int
sodisconnect(so)
	register struct socket *so;
{
	int s = splnet();
	int error;

	if ((so->so_state & SS_ISCONNECTED) == 0) {
		error = ENOTCONN;
		goto bad;
	}
	if (so->so_state & SS_ISDISCONNECTING) {
		error = EALREADY;
		goto bad;
	}
	error = (*so->so_proto->pr_usrreq)(so, PRU_DISCONNECT,
	    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
bad:
	splx(s);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
    WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_ERROR, 46, 9,
                     WV_NETEVENT_SODISCONN_STATUS, so->so_fd, error)
#endif  /* INCLUDE_WVNET */
#endif

	return (error);
}

#define	SBLOCKWAIT(f)	(((f) & MSG_DONTWAIT) ? M_DONTWAIT : M_WAIT)
/*
 * Send on a socket.
 * If send must go all at once and message is larger than
 * send buffering, then hard error.
 * Lock against other senders.
 * If must go all at once and not enough room now, then
 * inform user that this would block and do nothing.
 * Otherwise, if nonblocking, send as much as possible.
 * The data to be sent is described by "uio" if nonzero,
 * otherwise by the mbuf chain "top" (which must be null
 * if uio is not).  Data provided in mbuf chain must be small
 * enough to send all at once.
 *
 * Returns nonzero on error, timeout or signal; callers
 * must check for short counts if EINTR/ERESTART are returned.
 * Data and control buffers are freed on return.
 *
 * WRS mods removed sblock and sbunlock and replaced with splnet and splx
 * which should serve the purpose.
 * Removed null check of uio so that blocking can be implemented for zbufs
 * also. 
 * CAVEAT: the zbuf length cannot be more than the socket high water mark.
 * The user should implement his flow control. 
 * So this call would block only if zbuf length is bigger the space available
 * in the socket buffer and less than the socket higt water mark. 
 * Added a flag canWait which is set to M_DONTWAIT if the socket option SS_NBIO
 * is set. This prevents blocking if the user chose SS_NBIO and for some reason
 * if the system runs out of mbufs. canWait defaults to M_WAIT -(vinai).
 */
int
sosend(so, addr, uio, top, control, flags)
	register struct socket *so;
	struct mbuf *addr;
	struct uio *uio;
	struct mbuf *top;
	struct mbuf *control;
	int flags;
{
	struct mbuf **mp;
	register struct mbuf *m;
	register long space, len, resid;
	int clen = 0, error, s, dontroute;
	int atomic = sosendallatonce(so) || top;
	register int canWait;
	int outcount = 0;

	if (uio)
		resid = uio->uio_resid;
	else
		resid = top->m_pkthdr.len;
	/*
	 * In theory resid should be unsigned.
	 * However, space must be signed, as it might be less than 0
	 * if we over-committed, and we must use a signed comparison
	 * of space and resid.  On the other hand, a negative resid
	 * causes us to loop sending 0-length segments to the protocol.
	 */
	if (resid < 0)
		return (EINVAL);
	dontroute =
	    (flags & MSG_DONTROUTE) && (so->so_options & SO_DONTROUTE) == 0 &&
	    (so->so_proto->pr_flags & PR_ATOMIC);

	if (control)
		clen = control->m_len;

	canWait = (so->so_state & SS_NBIO) ? M_DONTWAIT : M_WAIT;

#define	snderr(errno)	{ error = errno; splx(s); goto out; }

	s = splnet();
restart:
	do {
		if (so->so_state & SS_CANTSENDMORE)
			snderr(EPIPE);
		if (so->so_error)
			snderr(so->so_error);
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			if (so->so_proto->pr_flags & PR_CONNREQUIRED) {
				if ((so->so_state & SS_ISCONFIRMING) == 0 &&
				    !(resid == 0 && clen != 0))
					snderr(ENOTCONN);
			} else if (addr == 0)
				snderr(EDESTADDRREQ);
		}
		space = sbspace(&so->so_snd);
		if (flags & MSG_OOB)
			space += 1024;
		if (atomic && ((resid > so->so_snd.sb_hiwat) ||
		    (clen > so->so_snd.sb_hiwat)))
			snderr(EMSGSIZE);
		if (space < resid + clen && 
		    (atomic || space < so->so_snd.sb_lowat || space < clen)) {
			if (so->so_state & SS_NBIO)
			    {
			    if (flags & MSG_MBUF)
				top = NULL; /* don't free the zero copy mbuf */
			    snderr(EWOULDBLOCK);
			    }
			sbwait(&so->so_snd);
			goto restart;
		}
		mp = &top;
		space -= clen;
		do {
		    if (uio == NULL) {
			/*
			 * Data is prepackaged in "top".
			 */
			resid = 0;
			if (flags & MSG_EOR)
				top->m_flags |= M_EOR;
		    } else do {
		        len = min(resid, space);
			
			if (top == 0) {
			        m = mBufClGet(canWait, MT_DATA, len + max_hdr,
					      FALSE);
				if (m == NULL)
				    snderr(ENOBUFS);
				len = min(len, m->m_extSize);

				m->m_flags |= M_PKTHDR;
				m->m_pkthdr.len = 0;
				m->m_pkthdr.rcvif = (struct ifnet *)0;
                                /*
                                 * the assumption here is that max_hdr is
                                 * always less than the minimum cluster size
                                 * available.  Or don't set len, namely use
                                 * len which set by min() above.
                                 */
                                if (atomic && m->m_extSize > max_hdr) {
                                	len = min((m->m_extSize - max_hdr),
                                                  len);
                                        m->m_data += max_hdr;
                                }
			}
			else
			    {
			    m = mBufClGet(canWait, MT_DATA, len, FALSE);
			    if (m == NULL)
				snderr(ENOBUFS);
			    len = min(len, m->m_extSize);
			    }
                   
			space -= (m->m_extSize + MSIZE);
			error = uiomove(mtod(m, caddr_t), (int)len, uio);
			resid = uio->uio_resid;
			m->m_len = len;
			*mp = m;
			top->m_pkthdr.len += len;
			if (error)
				goto release;
			mp = &m->m_next;
			if (resid <= 0) {
				if (flags & MSG_EOR)
					top->m_flags |= M_EOR;
				break;
			}
		    } while (space > 0 && atomic);
		    if (dontroute)
			    so->so_options |= SO_DONTROUTE;

                    if (!sosendallatonce (so))
			{
		        top->m_flags &= ~M_EOB;
		        if (((resid == 0) || (space <= 0)) && (outcount > 0))
			    top->m_flags |= M_EOB;
                        outcount++;
                        if (resid)
			    top->m_flags &= ~M_EOB;
                        }

		    error = (*so->so_proto->pr_usrreq)(so,
			(flags & MSG_OOB) ? PRU_SENDOOB : PRU_SEND,
			top, addr, control);
		    if (dontroute)
			    so->so_options &= ~SO_DONTROUTE;
		    clen = 0;
		    control = 0;
		    top = 0;
		    mp = &top;
		    if (error)
			goto release;
		} while (resid && space > 0);
	} while (resid);

release:
	splx(s);	
out:
	if (top)
		m_freem(top);
	if (control)
		m_freem(control);

	if (error != 0)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_EVENT_2 (NET_CORE_EVENT, WV_NET_CRITICAL, 34, 4,
                            WV_NETEVENT_SOSEND_FAIL, WV_NET_SEND,
                            so->so_fd, error)
#endif  /* INCLUDE_WVNET */
#endif

	    netErrnoSet (error);
            }
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
        else
            {
            WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_VERBOSE, 54, 20,
                            WV_NETEVENT_SOSEND_FINISH, WV_NET_SEND, so->so_fd)
            }
#endif  /* INCLUDE_WVNET */
#endif

	return (error);
}

/*
 * Implement receive operations on a socket.
 * We depend on the way that records are added to the sockbuf
 * by sbappend*.  In particular, each record (mbufs linked through m_next)
 * must begin with an address if the protocol so specifies,
 * followed by an optional mbuf or mbufs containing ancillary data,
 * and then zero or more mbufs of data.
 * In order to avoid blocking network interrupts for the entire time here,
 * we splx() while doing the actual copy to user space.
 * Although the sockbuf is locked, new data may still be appended,
 * and thus we must maintain consistency of the sockbuf during that time.
 *
 * The caller may receive the data as a single mbuf chain by supplying
 * an mbuf **mp0 for use in returning the chain.  The uio is then used
 * only for the count in uio_resid.
 *
 * WRS mods: implement zero copy if out of band data is requested.
 */
int
soreceive(so, paddr, uio, mp0, controlp, flagsp)
	register struct socket *so;
	struct mbuf **paddr;
	struct uio *uio;
	struct mbuf **mp0;
	struct mbuf **controlp;
	int *flagsp;
{
	register struct mbuf *m, **mp;
	register int flags, len, error = 0, s, offset;
	struct protosw *pr = so->so_proto;
	struct mbuf *nextrecord;
	int moff, type = 0;
	int orig_resid = uio->uio_resid;

	mp = mp0;
	if (paddr)
		*paddr = 0;
	if (controlp)
		*controlp = 0;
	if (flagsp)
		flags = *flagsp &~ MSG_EOR;
	else
		flags = 0;

	/* this is the zero copy ptr To ptr to mbuf passed */
	if (mp)		
		*mp = (struct mbuf *)0;

	if (flags & MSG_OOB) {
		m = mBufClGet(M_WAIT, MT_DATA, CL_SIZE_128, TRUE);
		if (m == (struct mbuf *) NULL)
		    {
		    return (ENOBUFS);
		    }

		error = (*pr->pr_usrreq)(so, PRU_RCVOOB, m,
		    (struct mbuf *)(long)(flags & MSG_PEEK), (struct mbuf *)0);
		if (error)
			goto bad;

		if (mp) do { 			/* if zero copy interface */
		    uio->uio_resid -= m->m_len; 
		    *mp = m; 
		    mp = &m->m_next; 
		    m = m->m_next; 
		} while (*mp);  
		else do {			/* if not zero copy iface */
			error = uiomove(mtod(m, caddr_t),
			    (int) min(uio->uio_resid, m->m_len), uio);
			m = m_free(m);
		} while (uio->uio_resid && error == 0 && m);
bad:
		if (m)
			m_freem(m);
		netErrnoSet (error);
		return (error);
	}

	if (so->so_state & SS_ISCONFIRMING && uio->uio_resid)
		(*pr->pr_usrreq)(so, PRU_RCVD, (struct mbuf *)0,
		    (struct mbuf *)0, (struct mbuf *)0);

	s = splnet();

restart:
	m = so->so_rcv.sb_mb;

	/*
	 * If we have less data than requested, block awaiting more
	 * (subject to any timeout) if:
	 *   1. the current count is less than the low water mark, or
	 *   2. MSG_WAITALL is set, and it is possible to do the entire
	 *	receive operation at once if we block (resid <= hiwat), or
	 *   3. MSG_DONTWAIT is not set.
	 * If MSG_WAITALL is set but resid is larger than the receive buffer,
	 * we have to do the receive in sections, and thus risk returning
	 * a short count if a timeout or signal occurs after we start.
	 */
	if ( m == 0 ||
             ( ((flags & MSG_DONTWAIT) == 0) &&
	       (so->so_rcv.sb_cc < uio->uio_resid) &&
	       (so->so_rcv.sb_cc < so->so_rcv.sb_lowat)
             ) ||
	     ( (flags & MSG_WAITALL) &&
               (uio->uio_resid <= so->so_rcv.sb_hiwat) &&
	       (m->m_nextpkt == 0) &&
               ((pr->pr_flags & PR_ATOMIC) == 0)
             ) 
           ) {
#ifdef DIAGNOSTIC
		if (m == 0 && so->so_rcv.sb_cc)
			panic("receive 1");
#endif
		if (so->so_error) {
			if (m)
				goto dontblock;
			error = so->so_error;
			if ((flags & MSG_PEEK) == 0)
				so->so_error = 0;
			goto release;
		}
		if (so->so_state & SS_CANTRCVMORE) {
			if (m)
				goto dontblock;
			else
				goto release;
		}
		for (; m; m = m->m_next)
			if (m->m_type == MT_OOBDATA  || (m->m_flags & M_EOR)) {
				m = so->so_rcv.sb_mb;
				goto dontblock;
			}
		if ((so->so_state & (SS_ISCONNECTED|SS_ISCONNECTING)) == 0 &&
		    (so->so_proto->pr_flags & PR_CONNREQUIRED)) {
			error = ENOTCONN;
			goto release;
		}
		if (uio->uio_resid == 0)
			goto release;
		if ((so->so_state & SS_NBIO) || (flags & MSG_DONTWAIT)) {
			error = EWOULDBLOCK;
			goto release;
		}
		sbwait(&so->so_rcv);
		goto restart;
	}
dontblock:
	nextrecord = m->m_nextpkt;
	if (pr->pr_flags & PR_ADDR) {
#ifdef DIAGNOSTIC
		if (m->m_type != MT_SONAME)
			panic("receive 1a");
#endif
		orig_resid = 0;
		if (flags & MSG_PEEK) {
			if (paddr)
				*paddr = m_copy(m, 0, m->m_len);
			m = m->m_next;
		} else {
			sbfree(&so->so_rcv, m);
			if (paddr) {
				*paddr = m;
				so->so_rcv.sb_mb = m->m_next;
				m->m_next = 0;
				m = so->so_rcv.sb_mb;
			} else {
				so->so_rcv.sb_mb = m_free(m);
				m = so->so_rcv.sb_mb;
			}
		}
	}
	while (m && m->m_type == MT_CONTROL && error == 0) {
		if (flags & MSG_PEEK) {
			if (controlp)
				*controlp = m_copy(m, 0, m->m_len);
			m = m->m_next;
		} else {
			sbfree(&so->so_rcv, m);
			if (controlp) {
				if (pr->pr_domain->dom_externalize &&
				    mtod(m, struct cmsghdr *)->cmsg_type ==
				    SCM_RIGHTS)
				   error = (*pr->pr_domain->dom_externalize)(m);
				*controlp = m;
				so->so_rcv.sb_mb = m->m_next;
				m->m_next = 0;
				m = so->so_rcv.sb_mb;
			} else {
				so->so_rcv.sb_mb = m_free(m);
				m = so->so_rcv.sb_mb;
			}
		}
		if (controlp) {
			orig_resid = 0;
			controlp = &(*controlp)->m_next;
		}
	}
	if (m) {
		if ((flags & MSG_PEEK) == 0)
			m->m_nextpkt = nextrecord;
		type = m->m_type;
		if (type == MT_OOBDATA)
			flags |= MSG_OOB;
	}
	moff = 0;
	offset = 0;
	while (m && uio->uio_resid > 0 && error == 0) {
		if (m->m_type == MT_OOBDATA) {
			if (type != MT_OOBDATA)
				break;
		} else if (type == MT_OOBDATA)
			break;
#ifdef DIAGNOSTIC
		else if (m->m_type != MT_DATA && m->m_type != MT_HEADER)
			panic("receive 3");
#endif
		so->so_state &= ~SS_RCVATMARK;
		len = uio->uio_resid;
		if (so->so_oobmark && len > so->so_oobmark - offset)
			len = so->so_oobmark - offset;
		if (len > m->m_len - moff)
			len = m->m_len - moff;
		/*
		 * If mp is set, just pass back the mbufs.
		 * Otherwise copy them out via the uio, then free.
		 * Sockbuf must be consistent here (points to current mbuf,
		 * it points to next record) when we drop priority;
		 * we must note any additions to the sockbuf when we
		 * block interrupts again.
		 */
		if (mp == 0) {
			error = uiomove(mtod(m, caddr_t) + moff, (int)len, uio);
		} else
			uio->uio_resid -= len;
		if (len == m->m_len - moff) {
			if (m->m_flags & M_EOR)
				flags |= MSG_EOR;
			if (flags & MSG_PEEK) {
				m = m->m_next;
				moff = 0;
			} else {
				nextrecord = m->m_nextpkt;
				sbfree(&so->so_rcv, m);
				if (mp) {
					*mp = m;
					mp = &m->m_next;
					so->so_rcv.sb_mb = m = m->m_next;
					*mp = (struct mbuf *)0;
				} else {
					so->so_rcv.sb_mb = m_free(m);
					m = so->so_rcv.sb_mb;
				}
				if (m)
					m->m_nextpkt = nextrecord;
			}
		} else {
			if (flags & MSG_PEEK)
				moff += len;
			else {
				if (mp)
                                    {
				    *mp = m_copym(m, 0, len, M_WAIT);
                                    if (*mp == NULL)
                                        {
                                        error = ENOBUFS;
                                        goto release;
                                        }
                                    }
				m->m_data += len;
				m->m_len -= len;
				so->so_rcv.sb_cc -= len;
			}
		}
		if (so->so_oobmark) {
			if ((flags & MSG_PEEK) == 0) {
				so->so_oobmark -= len;
				if (so->so_oobmark == 0) {
					so->so_state |= SS_RCVATMARK;
					break;
				}
			} else {
				offset += len;
				if (offset == so->so_oobmark)
					break;
			}
		}
		if (flags & MSG_EOR)
			break;
		/*
		 * If the MSG_WAITALL flag is set (for non-atomic socket),
		 * we must not quit until "uio->uio_resid == 0" or an error
		 * termination.  If a signal/timeout occurs, return
		 * with a short count but without error.
		 * Keep sockbuf locked against other readers.
		 */
		while (flags & MSG_WAITALL && m == 0 && uio->uio_resid > 0 &&
		    !sosendallatonce(so) && !nextrecord) {
			if (so->so_error || so->so_state & SS_CANTRCVMORE)
				break;
			sbwait(&so->so_rcv);

			if ((m = so->so_rcv.sb_mb))
				nextrecord = m->m_nextpkt;
		}
	}

	if (m && pr->pr_flags & PR_ATOMIC) {
		flags |= MSG_TRUNC;
		if ((flags & MSG_PEEK) == 0)
			(void) sbdroprecord(&so->so_rcv);
	}
	if ((flags & MSG_PEEK) == 0) {
		if (m == 0)
			so->so_rcv.sb_mb = nextrecord;
		if (pr->pr_flags & PR_WANTRCVD && so->so_pcb)
			(*pr->pr_usrreq)(so, PRU_RCVD, (struct mbuf *)0,
			    (struct mbuf *)(long)flags, (struct mbuf *)0,
			    (struct mbuf *)0);
	}
	if (orig_resid == uio->uio_resid && orig_resid &&
	    (flags & MSG_EOR) == 0 && (so->so_state & SS_CANTRCVMORE) == 0) {
		goto restart;
	}
		
	if (flagsp)
		*flagsp |= flags;
release:
	splx(s);

	if (error != 0)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_EVENT_2 (NET_CORE_EVENT, WV_NET_CRITICAL, 35, 5,
                            WV_NETEVENT_SORECV_FAIL, WV_NET_RECV,
                            so->so_fd, error)
#endif  /* INCLUDE_WVNET */
#endif

	    netErrnoSet (error);
            }
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
        else
            {
            WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_VERBOSE, 55, 21,
                            WV_NETEVENT_SORECV_FINISH, WV_NET_RECV, so->so_fd)
            }
#endif  /* INCLUDE_WVNET */
#endif

	return (error);
}

int
soshutdown(so, how)
	register struct socket *so;
	register int how;
{
	register struct protosw *pr = so->so_proto;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_VERBOSE, 56, 22,
                     WV_NETEVENT_SOSHUTDOWN_START, so->so_fd, how)
#endif  /* INCLUDE_WVNET */
#endif

	how++;
	if (how & 1)		/* <=> FREAD of BSD44 */
		sorflush(so);	
	if (how & 2)		/* <=> FWRITE of BSD44 */
		return ((*pr->pr_usrreq)(so, PRU_SHUTDOWN,
		    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0));
	return (0);
}

void
sorflush(so)
	register struct socket *so;
{
	register struct sockbuf *sb = &so->so_rcv;
	register struct protosw *pr = so->so_proto;
	register int s;
	struct sockbuf asb;

	sb->sb_flags |= SB_NOINTR;
	s = splimp();
	socantrcvmore(so);
	asb = *sb;
        if (sb->sb_Sem)
            {
	    semDelete (sb->sb_Sem);
            sb->sb_Sem = NULL;
            }
	bzero((caddr_t)sb, sizeof (*sb));
	splx(s);
	if (pr->pr_flags & PR_RIGHTS && pr->pr_domain->dom_dispose)
		(*pr->pr_domain->dom_dispose)(asb.sb_mb);
	sbrelease(&asb);
}

int
sosetopt(so, level, optname, m0)
	register struct socket *so;
	int level, optname;
	struct mbuf *m0;
{
	int error = 0;
	register struct mbuf *m = m0;

	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput)
			return ((*so->so_proto->pr_ctloutput)
				  (PRCO_SETOPT, so, level, optname, &m0));
		error = ENOPROTOOPT;
	} else {
		switch (optname) {

		case SO_LINGER:
			if (m == NULL || m->m_len != sizeof (struct linger)) {
				error = EINVAL;
				goto bad;
			}
			so->so_linger = mtod(m, struct linger *)->l_linger;
			/* fall thru... */

		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
		case SO_BROADCAST:
		case SO_REUSEADDR:
		case SO_REUSEPORT:
		case SO_OOBINLINE:
                case SO_USEPATHMTU:
			if (m == NULL || m->m_len < sizeof (int)) {
				error = EINVAL;
				goto bad;
			}
			if (*mtod(m, int *))
				so->so_options |= optname;
			else
				so->so_options &= ~optname;
			break;

		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
			if (m == NULL || m->m_len < sizeof (int)) {
				error = EINVAL;
				goto bad;
			}
			switch (optname) {

			case SO_SNDBUF:
			case SO_RCVBUF:
				if (sbreserve(optname == SO_SNDBUF ?
				    &so->so_snd : &so->so_rcv,
				    (u_long) *mtod(m, int *)) == 0) {
					error = ENOBUFS;
					goto bad;
				}
				break;

			case SO_SNDLOWAT:
				so->so_snd.sb_lowat = *mtod(m, int *);
				break;
			case SO_RCVLOWAT:
				so->so_rcv.sb_lowat = *mtod(m, int *);
				break;
			}
			break;

		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
		    {
			switch (optname) {

			case SO_SNDTIMEO:
				so->so_snd.sb_timeo = *mtod(m, int *);
				break;
			case SO_RCVTIMEO:
				so->so_rcv.sb_timeo = *mtod(m, int *);
				break;
			}
			break;
		    }

#ifdef VIRTUAL_STACK
                case SO_VSID:
                    so->vsid = *mtod(m, int *);
                    break;
#endif /* VIRTUAL_STACK */

		default:
			error = ENOPROTOOPT;
			break;
		}
		if (error == 0 && so->so_proto && so->so_proto->pr_ctloutput) {
			(void) ((*so->so_proto->pr_ctloutput)
				  (PRCO_SETOPT, so, level, optname, &m0));
			m = NULL;	/* freed by protocol */
		}
	}
bad:
	if (m)
		(void) m_free(m);
	return (error);
}

int
sogetopt(so, level, optname, mp)
	register struct socket *so;
	int level, optname;
	struct mbuf **mp;
{
	register struct mbuf *m;

	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput) {
			return ((*so->so_proto->pr_ctloutput)
				  (PRCO_GETOPT, so, level, optname, mp));
		} else
			return (ENOPROTOOPT);
	} else {
		m = mBufClGet(M_WAIT, MT_SOOPTS, CL_SIZE_128, TRUE);
                if (m == (struct mbuf *) NULL)
                    return (ENOBUFS);

		m->m_len = sizeof (int);

		switch (optname) {

		case SO_LINGER:
			m->m_len = sizeof (struct linger);
			mtod(m, struct linger *)->l_onoff =
				so->so_options & SO_LINGER;
			mtod(m, struct linger *)->l_linger = so->so_linger;
			break;

		case SO_USELOOPBACK:
		case SO_DONTROUTE:
		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_REUSEADDR:
		case SO_REUSEPORT:
		case SO_BROADCAST:
		case SO_OOBINLINE:
			*mtod(m, int *) = so->so_options & optname;
			break;

		case SO_TYPE:
			*mtod(m, int *) = so->so_type;
			break;

		case SO_ERROR:
			*mtod(m, int *) = so->so_error;
			so->so_error = 0;
			break;

		case SO_SNDBUF:
			*mtod(m, int *) = so->so_snd.sb_hiwat;
			break;

		case SO_RCVBUF:
			*mtod(m, int *) = so->so_rcv.sb_hiwat;
			break;

		case SO_SNDLOWAT:
			*mtod(m, int *) = so->so_snd.sb_lowat;
			break;

		case SO_RCVLOWAT:
			*mtod(m, int *) = so->so_rcv.sb_lowat;
			break;

		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
		        *mtod(m, int *) = (optname == SO_SNDTIMEO ?
			     so->so_snd.sb_timeo : so->so_rcv.sb_timeo);
			break;

#ifdef VIRTUAL_STACK
                case SO_VSID:
                        *mtod(m, int *) = so->vsid;
                        break;
#endif /* VIRTUAL_STACK */

		default:
			(void)m_free(m);
			return (ENOPROTOOPT);
		}
		*mp = m;
		return (0);
	}
}

void
sohasoutofband(so)
	register struct socket *so;
{
#ifdef UNIXBSD44	/* support for out of band data later on */
			/* 
			   This can be done by maintaining a taskId in the
			   in the socket structure and posting a SIGURG to
			   the task which would invoke the signal handler
			   for SIGURG if that signal is registered properly
			   The taskId should be created in the socket structure
			   at the time of the creation of the socket.
			   */
	struct proc *p;

	if (so->so_pgid < 0)
		gsignal(-so->so_pgid, SIGURG);
	else if (so->so_pgid > 0 && (p = pfind(so->so_pgid)) != 0)
		psignal(p, SIGURG);
	selwakeup(&so->so_rcv.sb_sel);
#endif /* UNIXBSD44 */
}

int uiomove(cp, n, uio)
    register caddr_t cp;
    register int n;
    register struct uio *uio;
    {
    register struct iovec *iov;
    u_int cnt;
    int error = 0;

#ifdef DIAGNOSTIC
    if (uio->uio_rw != UIO_READ && uio->uio_rw != UIO_WRITE)
	panic("uiomove: mode");
    if (uio->uio_segflg == UIO_USERSPACE && uio->uio_procp != curproc)
	panic("uiomove proc");
#endif
    while (n > 0 && uio->uio_resid) 
	{
	iov = uio->uio_iov;
	cnt = iov->iov_len;

	if (cnt == 0)
	    {
	    uio->uio_iov++;
	    uio->uio_iovcnt--;
	    continue;
	    }

	if (cnt > n)
	    cnt = n;

	if (uio->uio_rw == UIO_READ)
	    bcopy((caddr_t)cp, iov->iov_base, cnt);
	else
	    bcopy(iov->iov_base, (caddr_t)cp, cnt);

	iov->iov_base += cnt;
	iov->iov_len -= cnt;
	uio->uio_resid -= cnt;
	uio->uio_offset += cnt;
	cp += cnt;
	n -= cnt;
	}

    return (error);
    }

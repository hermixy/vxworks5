/* raw_usrreq.c - raw protocol interface */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1980, 1986, 1993
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
 *	@(#)raw_usrreq.c	8.1 (Berkeley) 6/10/93
 */

/*
modification history
--------------------
02d,12oct01,rae  merge from truestack ver 02e, base 02c
02c,03jul97,vin  SPR 8885. fixed warnings. 
02b,31jan97,vin  changed declaration according to prototype decl in protosw.h
01a,03mar96,vin  created from BSD4.4lite2.
*/

/*
DESCRIPTION
*/

#include "vxWorks.h"
#include "netLib.h"
#include "net/mbuf.h"
#include "net/domain.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "net/socketvar.h"
#include "errno.h"

#include "net/if.h"
#include "net/route.h"
#include "net/raw_cb.h"
#include "net/systm.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_RAWREQ_MODULE; /* Value for raw_usrreq.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif    /* VIRTUAL_STACK */

/*
 * Initialize raw connection block q.
 */
void
raw_init()
{
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_VERBOSE, 32, 7,
                     WV_NETEVENT_RAWINIT_START)
#endif  /* INCLUDE_WVNET */
#endif

#ifdef VIRTUAL_STACK
	_rawcb.rcb_next = _rawcb.rcb_prev = &_rawcb;
#else    /* VIRTUAL_STACK */
	rawcb.rcb_next = rawcb.rcb_prev = &rawcb;
#endif   /* VIRTUAL_STACK */

	return;
}


/*
 * Raw protocol input routine.  Find the socket
 * associated with the packet(s) and move them over.  If
 * nothing exists for this packet, drop it.
 */
/*
 * Raw protocol interface.
 */
void
raw_input(m0, proto, src, dst)
	struct mbuf *m0;
	register struct sockproto *proto;
	struct sockaddr *src, *dst;
{
	register struct rawcb *rp;
	register struct mbuf *m = m0;
	register int sockets = 0;
	struct socket *last;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
    WV_NET_EVENT_2 (NET_CORE_EVENT, WV_NET_NOTICE, 16, 4,
                     WV_NETEVENT_RAWIN_START, WV_NET_RECV,
                     proto->sp_family, proto->sp_protocol)
#endif  /* INCLUDE_WVNET */
#endif
	last = 0;
#ifdef VIRTUAL_STACK
	for (rp = _rawcb.rcb_next; rp != &_rawcb; rp = rp->rcb_next) {
#else
	for (rp = rawcb.rcb_next; rp != &rawcb; rp = rp->rcb_next) {
#endif
		if (rp->rcb_proto.sp_family != proto->sp_family)
			continue;
		if (rp->rcb_proto.sp_protocol  &&
		    rp->rcb_proto.sp_protocol != proto->sp_protocol)
			continue;
		/*
		 * We assume the lower level routines have
		 * placed the address in a canonical format
		 * suitable for a structure comparison.
		 *
		 * Note that if the lengths are not the same
		 * the comparison will fail at the first byte.
		 */
#define	equal(a1, a2) \
  (bcmp((caddr_t)(a1), (caddr_t)(a2), a1->sa_len) == 0)
		if (rp->rcb_laddr && !equal(rp->rcb_laddr, dst))
			continue;
		if (rp->rcb_faddr && !equal(rp->rcb_faddr, src))
			continue;
		if (last) {
			struct mbuf *n;
			if ((n = m_copy(m, 0, (int)M_COPYALL))) {
				if (sbappendaddr(&last->so_rcv, src,
				    n, (struct mbuf *)0) == 0)
                                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_WARNING event */
                    WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_WARNING, 10, 3,
                                    WV_NETEVENT_RAWIN_LOST, WV_NET_RECV,
                                    last->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

					/* should notify about lost packet */
					m_freem(n);
                                    }
				else {
					sorwakeup(last);
					sockets++;
				}
			}
		}
		last = rp->rcb_socket;
	}
	if (last) {
		if (sbappendaddr(&last->so_rcv, src,
		    m, (struct mbuf *)0) == 0)
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_WARNING event */
                    WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_WARNING, 10, 3,
                                    WV_NETEVENT_RAWIN_LOST, WV_NET_RECV,
                                    last->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

			m_freem(m);
                     }
		else {
			sorwakeup(last);
			sockets++;
		}
	} else
		m_freem(m);
	return;
}

/*ARGSUSED*/
void
raw_ctlinput(cmd, arg)
	int cmd;
	struct sockaddr *arg;
{
/*
 * XXX - This event is not reported because the routine currently does
 *       nothing when called.

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_INFO event @/
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_INFO, 15, 5,
                     WV_NETEVENT_RAWCTLIN_START, cmd)
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */


	if (cmd < 0 || cmd > PRC_NCMDS)
		return;
	/* INCOMPLETE */
}

/*ARGSUSED*/
int
raw_usrreq(so, req, m, nam, control)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *control;
{
	register struct rawcb *rp = sotorawcb(so);
	register int error = 0;
	int len;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_2 (NET_CORE_EVENT, WV_NET_INFO, 16, 6,
                     WV_NETEVENT_RAWREQ_START, so->so_fd, req)
#endif  /* INCLUDE_WVNET */
#endif

	if (req == PRU_CONTROL)
		return (EOPNOTSUPP);
	if (control && control->m_len) {
		error = EOPNOTSUPP;
		goto release;
	}
	if (rp == 0) {
		error = EINVAL;
		goto release;
	}
	switch (req) {

	/*
	 * Allocate a raw control block and fill in the
	 * necessary info to allow packets to be routed to
	 * the appropriate raw interface routine.
	 */
	case PRU_ATTACH:
		if ((so->so_state & SS_PRIV) == 0) {
			error = EACCES;
			break;
		}
		error = raw_attach(so, (int)nam);
		break;

	/*
	 * Destroy state just before socket deallocation.
	 * Flush data or not depending on the options.
	 */
	case PRU_DETACH:
		if (rp == 0) {
			error = ENOTCONN;
			break;
		}
		raw_detach(rp);
		break;

#ifdef notdef
	/*
	 * If a socket isn't bound to a single address,
	 * the raw input routine will hand it anything
	 * within that protocol family (assuming there's
	 * nothing else around it should go to). 
	 */
	case PRU_CONNECT:
		if (rp->rcb_faddr) {
			error = EISCONN;
			break;
		}
		nam = m_copym(nam, 0, M_COPYALL, M_WAIT);
		rp->rcb_faddr = mtod(nam, struct sockaddr *);
		soisconnected(so);
		break;

	case PRU_BIND:
		if (rp->rcb_laddr) {
			error = EINVAL;			/* XXX */
			break;
		}
		error = raw_bind(so, nam);
		break;
#endif

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		goto release;

	case PRU_DISCONNECT:
		if (rp->rcb_faddr == 0) {
			error = ENOTCONN;
			break;
		}
		raw_disconnect(rp);
		soisdisconnected(so);
		break;

	/*
	 * Mark the connection as being incapable of further input.
	 */
	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	/*
	 * Ship a packet out.  The appropriate raw output
	 * routine handles any massaging necessary.
	 */
	case PRU_SEND:
		if (nam) {
			if (rp->rcb_faddr) {
				error = EISCONN;
				break;
			}
			rp->rcb_faddr = mtod(nam, struct sockaddr *);
		} else if (rp->rcb_faddr == 0) {
			error = ENOTCONN;
			break;
		}
		error = (*so->so_proto->pr_output)(m, so);
		m = NULL;
		if (nam)
			rp->rcb_faddr = 0;
		break;

	case PRU_ABORT:
		soisdisconnected(so);
		raw_disconnect(rp);
		break;

	case PRU_SENSE:
		/*
		 * stat: don't bother with a blocksize.
		 */
		return (0);

	/*
	 * Not supported.
	 */
	case PRU_RCVOOB:
	case PRU_RCVD:
		return(EOPNOTSUPP);

	case PRU_LISTEN:
	case PRU_ACCEPT:
	case PRU_SENDOOB:
		error = EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		if (rp->rcb_laddr == 0) {
			error = EINVAL;
			break;
		}
		len = rp->rcb_laddr->sa_len;
		bcopy((caddr_t)rp->rcb_laddr, mtod(nam, caddr_t), (unsigned)len);
		nam->m_len = len;
		break;

	case PRU_PEERADDR:
		if (rp->rcb_faddr == 0) {
			error = ENOTCONN;
			break;
		}
		len = rp->rcb_faddr->sa_len;
		bcopy((caddr_t)rp->rcb_faddr, mtod(nam, caddr_t), (unsigned)len);
		nam->m_len = len;
		break;

	default:
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_MARKER_2 (NET_CORE_EVENT, WV_NET_EMERGENCY, 20, 1,
                             WV_NETEVENT_RAWREQ_PANIC, so->so_fd, req)
#endif  /* INCLUDE_WVNET */
#endif

		panic("raw_usrreq");
	}
release:
	if (m != NULL)
		m_freem(m);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
    if (error)
        {
        WV_NET_MARKER_3 (NET_CORE_EVENT, WV_NET_ERROR, 36, 2,
                         WV_NETEVENT_RAWREQ_FAIL, so->so_fd, req, error)
        }
#endif  /* INCLUDE_WVNET */
#endif

	return (error);
}

/* raw_cb.c - raw protocol control block handling routines */

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
 *	@(#)raw_cb.c	8.1 (Berkeley) 6/10/93
 */

/*
modification history
--------------------
01d,12oct01,rae  merge from truestack ver 01f, base 01c
01c,02jul97,vin  fixed warnings.
01b,05dec96,vin  replaced free with FREE(),
01a,03mar96,vin  created from BSD4.4 stuff,integrated with 02i of raw_cb.c
*/

/*
DESCRIPTION
*/

#include "vxWorks.h"
#include "net/systm.h"
#include "net/mbuf.h"
#include "sys/socket.h"
#include "net/socketvar.h"
#include "net/domain.h"
#include "net/protosw.h"
#include "errno.h"

#include "net/if.h"
#include "net/route.h"
#include "net/raw_cb.h"
#include "netinet/in.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif

extern void _insque ();
extern void _remque (); 

#ifndef VIRTUAL_STACK
struct rawcb rawcb;			/* head of list */

/*
 * Routines to manage the raw protocol control blocks.
 *
 * TODO:
 *	hash lookups by protocol family/protocol + address family
 *	take care of unique address problems per AF?
 *	redo address binding to allow wildcards
 */

u_long  raw_sendspace = RAWSNDQ;
u_long  raw_recvspace = RAWRCVQ;
#endif

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_RAWCB_MODULE;   /* Value for raw_cb.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

/*
 * Allocate a control block and a nominal amount
 * of buffer space for the socket.
 */
int
raw_attach(so, proto)
	register struct socket *so;
	int proto;
{
	register struct rawcb *rp = sotorawcb(so);
	int error;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_1 (NET_CORE_EVENT, WV_NET_VERBOSE, 28, 4,
                     WV_NETEVENT_RAWATTACH_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

	/*
	 * It is assumed that raw_attach is called
	 * after space has been allocated for the
	 * rawcb.
	 */
	if (rp == 0)
            {
/* 
 * XXX - This event cannot currently occur: the route_usrreq() routine
 *       already checks for failure to allocate the control block.

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_CRITICAL event @/
        WV_NET_MARKER_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 24, 1,
                         WV_NETEVENT_RAWATTACH_NOPCBMEM, so->so_fd)
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */
		return (ENOBUFS);
            }
	if ((error = soreserve(so, raw_sendspace, raw_recvspace)))
            {

/* XXX - This event does not currently indicate a memory allocation failure.
 *       The soreserve() routine only sets high and low water mark values
 *       and does not attempt to commit any memory, although it should.

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_CRITICAL event @/
        WV_NET_MARKER_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 25, 2,
                         WV_NETEVENT_RAWATTACH_NOSOCKBUFMEM, so->so_fd)
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX - end of unused event
 */

		return (error);
            }
	rp->rcb_socket = so;
	rp->rcb_proto.sp_family = so->so_proto->pr_domain->dom_family;
	rp->rcb_proto.sp_protocol = proto;
#ifdef VIRTUAL_STACK
	insque(rp, &_rawcb);
#else    /* VIRTUAL_STACK */
	insque(rp, &rawcb);
#endif    /* VIRTUAL_STACK */
	return (0);
}

/*
 * Detach the raw connection block and discard
 * socket resources.
 */
void
raw_detach(rp)
	register struct rawcb *rp;
{
	struct socket *so = rp->rcb_socket;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_1 (NET_CORE_EVENT, WV_NET_VERBOSE, 29, 5,
                     WV_NETEVENT_RAWDETACH_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

	so->so_pcb = 0;
	sofree(so);
	remque(rp);
#ifdef notdef
	if (rp->rcb_laddr)
		m_freem(dtom(rp->rcb_laddr));
	rp->rcb_laddr = 0;
#endif
	FREE(rp, MT_PCB);
}

/*
 * Disconnect and possibly release resources.
 */
void
raw_disconnect(rp)
	struct rawcb *rp;
{

#ifdef notdef
	if (rp->rcb_faddr)
		m_freem(dtom(rp->rcb_faddr));
	rp->rcb_faddr = 0;
#endif
	if (rp->rcb_socket->so_state & SS_NOFDREF)
		raw_detach(rp);
}

#ifdef notdef
int
raw_bind(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	struct sockaddr *addr = mtod(nam, struct sockaddr *);
	register struct rawcb *rp;

/* XXX - This event does not occur because this routine is never called
 *       by the existing network stack since the PRU_BIND option within
 *       the raw_usrreq switch statement is unsupported.

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_VERBOSE event @/
    WV_NET_MARKER_1 (NET_CORE_EVENT, WV_NET_VERBOSE, 30, 6,
                     WV_NETEVENT_RAWBIND_START, so->so_fd)
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX -end of unused event
 */

	if (ifnet == 0)
            {
/* XXX - This event does not occur because this routine is never called
 *       by the existing network stack since the PRU_BIND option within
 *       the raw_usrreq switch statement is unsupported.

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /@ WV_NET_ERROR event @/
        WV_NET_MARKER_1 (NET_CORE_EVENT, WV_NET_ERROR, 31, 3,
                         WV_NETEVENT_RAWBIND_NOIF, so->so_fd)
#endif  /@ INCLUDE_WVNET @/
#endif

 * XXX -end of unused event
 */
		return (EADDRNOTAVAIL);
            }
	rp = sotorawcb(so);
	nam = m_copym(nam, 0, M_COPYALL, M_WAIT);
	rp->rcb_laddr = mtod(nam, struct sockaddr *);
	return (0);
}
#endif

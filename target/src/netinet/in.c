/* in.c - internet routines */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1982, 1986, 1991, 1993
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
 *	@(#)in.c	8.4 (Berkeley) 1/9/95
 */

/*
modification history
--------------------
01m,12oct01,rae  merge from truestack ver 01n, base 01j (SPR #67536 etc)
01l,30jan01,spm  ignored existing routes for alias IP addresses (SPR #62333)
01k,20apr00,rae  fixed memory leak in in_ifinit, SPR#28903
01j,22feb99,ham  called in_socktrim() in SIOCSIFNETMASK, SPR#24251.
01h,06oct98,ham  cancelled "01g" moidification, new fix for SPR#22267.
01g,08sep98,ham  fixed SIOCAIFADDR inproper work in in_control SPR#22267.
01f,08dec97,vin  fixed a bug in_addmulti incremented the ref count SPR 9971
01e,03dec97,vin  changed _pNetDpool to _pNetSysPool
01d,05oct97,vin  added fast multicasting.
01c,07jan96,vin  added _igmpJoinGrpHook and _igmpLeaveGrpHook for scaling igmp.
01b,05dec96,vin  replaced malloc with MALLOC and free with FREE(),
01a,03mar96,vin  created from BSD4.4 stuff,integrated with 02n of in.c.
*/

/*
DESCRIPTION
*/

#include "vxWorks.h"
#include "logLib.h"
#include "sys/ioctl.h"
#include "net/mbuf.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "net/socketvar.h"
#include "net/uio.h"
#include "errno.h"
#include "netinet/in_systm.h"
#include "net/if.h"
#include "net/route.h"
#include "netinet/in.h"
#include "netinet/in_var.h"
#include "net/systm.h"
#include "netinet/igmp_var.h"
#include "net/unixLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsIgmp.h"
#endif /* VIRTUAL_STACK */

#ifdef INET

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif

/* defines */

/* externals */

extern void arp_rtrequest (); 
extern VOIDFUNCPTR _igmpJoinGrpHook; 
extern VOIDFUNCPTR _igmpLeaveGrpHook; 

/* globals */
#ifndef SUBNETSARELOCAL
#define	SUBNETSARELOCAL	1
#endif

int subnetsarelocal = SUBNETSARELOCAL;
int	in_interfaces;		/* number of external internet interfaces */

#ifndef VIRTUAL_STACK
extern	struct ifnet loif[];	/* loop back interface */
struct mcastHashInfo	mCastHashInfo;
#endif

int	mCastHashTblSize = 64;	/* default size of hash table */


#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_IN_MODULE;   /* Value for in.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

BOOL inet_netmatch(sin1, sin2)
	struct sockaddr_in *sin1;
	struct sockaddr_in *sin2;
{

	return (in_netof(sin1->sin_addr) == in_netof(sin2->sin_addr));
}

/*
 * The following routines, in_netof, and in_lnaof,
 * are similar to the corresponding routines in inetLib, except that
 * these routines take subnet masks into account.
 */

/*
 * Return the network number from an internet address.
 */
u_long
in_netof(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	register u_long net;
	register struct in_ifaddr *ia;

	if (IN_CLASSA(i))
		net = i & IN_CLASSA_NET; 
	else if (IN_CLASSB(i))
		net = i & IN_CLASSB_NET;
	else if (IN_CLASSC(i))
		net = i & IN_CLASSC_NET;
	else if (IN_CLASSD(i))
		net = i & IN_CLASSD_NET;
	else
		return (0);

	/*
	 * Check whether network is a subnet;
	 * if so, return subnet number.
	 */
#ifdef VIRTUAL_STACK
	for (ia = _in_ifaddr; ia; ia = ia->ia_next)
#else
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
#endif /* VIRTUAL_STACK */
		if (net == ia->ia_net)
			return (i & ia->ia_subnetmask);
	return (net);
}

/*
 * Return 1 if an internet address is for a ``local'' host
 * (one to which we have a connection).  If subnetsarelocal
 * is true, this includes other subnets of the local net.
 * Otherwise, it includes only the directly-connected (sub)nets.
 */
int
in_localaddr(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	register struct in_ifaddr *ia;

	if (subnetsarelocal) {
#ifdef VIRTUAL_STACK
		for (ia = _in_ifaddr; ia; ia = ia->ia_next)
#else
		for (ia = in_ifaddr; ia; ia = ia->ia_next)
#endif /* VIRTUAL_STACK */
			if ((i & ia->ia_netmask) == ia->ia_net)
				return (1);
	} else {
#ifdef VIRTUAL_STACK
		for (ia = _in_ifaddr; ia; ia = ia->ia_next)
#else
		for (ia = in_ifaddr; ia; ia = ia->ia_next)
#endif /* VIRTUAL_STACK */
			if ((i & ia->ia_subnetmask) == ia->ia_subnet)
				return (1);
	}
	return (0);
}

/*
 * Determine whether an IP address is in a reserved set of addresses
 * that may not be forwarded, or whether datagrams to that destination
 * may be forwarded.
 */
int
in_canforward(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);
	register u_long net;

	if (IN_EXPERIMENTAL(i) || IN_MULTICAST(i))
		return (0);
	if (IN_CLASSA(i)) {
		net = i & IN_CLASSA_NET;
		if (net == 0 || net == (IN_LOOPBACKNET << IN_CLASSA_NSHIFT))
			return (0);
	}
	return (1);
}

/*
 * Trim a mask in a sockaddr
 */
void
in_socktrim(ap)
struct sockaddr_in *ap;
{
    register char *cplim = (char *) &ap->sin_addr;
    register char *cp = (char *) (&ap->sin_addr + 1);

    ap->sin_len = 0;
    while (--cp >= cplim)
        if (*cp) {
	    (ap)->sin_len = cp - (char *) (ap) + 1;
	    break;
	}
}

/*
 * Return the host portion of an internet address.
 */
u_long
in_lnaof(in)
        struct in_addr in;
{
        register u_long i = ntohl(in.s_addr);
        register u_long net, host;
        register struct in_ifaddr *ia;

        if (IN_CLASSA(i)) {
                net = i & IN_CLASSA_NET;
                host = i & IN_CLASSA_HOST;
        } else if (IN_CLASSB(i)) {
                net = i & IN_CLASSB_NET;
                host = i & IN_CLASSB_HOST;
        } else if (IN_CLASSC(i)) {
                net = i & IN_CLASSC_NET;
                host = i & IN_CLASSC_HOST;
        } else if (IN_CLASSD(i)) {
                net = i & IN_CLASSD_NET;
                host = i & IN_CLASSD_HOST;
        } else
                return (i);

        /*
         * Check whether network is a subnet;
         * if so, use the modified interpretation of `host'.
         */
#ifdef VIRTUAL_STACK
        for (ia = _in_ifaddr; ia; ia = ia->ia_next)
#else
        for (ia = in_ifaddr; ia; ia = ia->ia_next)
#endif /* VIRTUAL_STACK */
                if (net == ia->ia_net)
                        return (host &~ ia->ia_subnetmask);
        return (host);
}

/*
 * Generic internet control operations (ioctl's).
 * Ifp is 0 if not an interface-specific ioctl.
 */
/* ARGSUSED */
int
in_control(so, cmd, data, ifp)
	struct socket *so;
	u_long cmd;
	caddr_t data;
	register struct ifnet *ifp;
{
	register struct ifreq *ifr = (struct ifreq *)data;
	register struct in_ifaddr *ia = 0;
	register struct ifaddr *ifa;
	struct in_ifaddr *oia;
	struct in_aliasreq *ifra = (struct in_aliasreq *)data;
	struct sockaddr_in oldaddr;
	int error, hostIsNew, maskIsNew;
	u_long i;
        BOOL newRouteFlag = FALSE; 	/* Connection to new IP subnet? */

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 18, 7,
                         WV_NETEVENT_INCTRL_START, cmd)
#endif  /* INCLUDE_WVNET */
#endif

            /*
             * Find address for this interface, if it exists.
             */
            if (ifp)
#ifdef VIRTUAL_STACK
                for (ia = _in_ifaddr; ia; ia = ia->ia_next)
#else
                    for (ia = in_ifaddr; ia; ia = ia->ia_next)
#endif /* VIRTUAL_STACK */
			if (ia->ia_ifp == ifp)
                            break;
        
	switch (cmd) {
        
	case SIOCAIFADDR:
            
            if (ia == 0)
                {
                /* First IP address: no matching route should exist. */
                
                newRouteFlag = TRUE;
                }
            
            /* fall-through */
            
	case SIOCDIFADDR:
            if (ifra->ifra_addr.sin_family == AF_INET)
                for (oia = ia; ia; ia = ia->ia_next) {
                if (ia->ia_ifp == ifp  &&
			    ia->ia_addr.sin_addr.s_addr ==
                    ifra->ifra_addr.sin_addr.s_addr)
                    break;
		}
            if (cmd == SIOCDIFADDR && ia == 0)
                {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_ERROR, 8, 4, 
                                 WV_NETEVENT_INCTRL_SEARCHFAIL, cmd, ifp)
#endif  /* INCLUDE_WVNET */
#endif
                    return (EADDRNOTAVAIL);
                }
            
            /* FALLTHROUGH */
	case SIOCSIFADDR:
	case SIOCSIFNETMASK:
	case SIOCSIFDSTADDR:
            if ((so->so_state & SS_PRIV) == 0)
                {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_WARNING event */
                WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_WARNING, 6, 6, 
                                 WV_NETEVENT_INCTRL_BADSOCK, so->so_fd, cmd)
#endif  /* INCLUDE_WVNET */
#endif
                    return (EPERM);
                }
            if (ifp == 0)
                {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
                WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_EMERGENCY, 10, 1, 
                                 WV_NETEVENT_INCTRL_PANIC, cmd)
#endif  /* INCLUDE_WVNET */
#endif

                    panic("in_control");
                }
            if (ia == (struct in_ifaddr *)0) {
            MALLOC (oia, struct in_ifaddr *, sizeof(*oia),
				MT_IFADDR, M_WAIT); 
			if (oia == (struct in_ifaddr *)NULL)
                            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
                            WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_EMERGENCY, 11, 2, 
                                             WV_NETEVENT_INCTRL_NOBUFS, cmd)
#endif  /* INCLUDE_WVNET */
#endif
                            return (ENOBUFS);
                            }
			bzero((caddr_t)oia, sizeof *oia);
#ifdef VIRTUAL_STACK
			if ((ia = _in_ifaddr)) {
#else
			if ((ia = in_ifaddr)) {
#endif /* VIRTUAL_STACK */
				for ( ; ia->ia_next; ia = ia->ia_next)
					continue;
				ia->ia_next = oia;
			} else
#ifdef VIRTUAL_STACK
				_in_ifaddr = oia;
#else
				in_ifaddr = oia;
#endif /* VIRTUAL_STACK */
			ia = oia;
			if ((ifa = ifp->if_addrlist)) {
				for ( ; ifa->ifa_next; ifa = ifa->ifa_next)
					continue;
				ifa->ifa_next = (struct ifaddr *) ia;
			} else
				ifp->if_addrlist = (struct ifaddr *) ia;
			ia->ia_ifa.ifa_addr = (struct sockaddr *)&ia->ia_addr;
			ia->ia_ifa.ifa_dstaddr
					= (struct sockaddr *)&ia->ia_dstaddr;
			ia->ia_ifa.ifa_netmask
					= (struct sockaddr *)&ia->ia_sockmask;
			ia->ia_sockmask.sin_len = 8;
			if (ifp->if_flags & IFF_BROADCAST) {
				ia->ia_broadaddr.sin_len = sizeof(ia->ia_addr);
				ia->ia_broadaddr.sin_family = AF_INET;
			}
			ia->ia_ifp = ifp;
			if (ifp != loif)
				in_interfaces++;
		}
		break;

	case SIOCSIFBRDADDR:
		if ((so->so_state & SS_PRIV) == 0)
			return (EPERM);
		/* FALLTHROUGH */

	case SIOCGIFADDR:
	case SIOCGIFNETMASK:
	case SIOCGIFDSTADDR:
	case SIOCGIFBRDADDR:
		if (ia == (struct in_ifaddr *)0)
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                    WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_ERROR, 8, 4, 
                                     WV_NETEVENT_INCTRL_SEARCHFAIL, cmd, ifp)
#endif  /* INCLUDE_WVNET */
#endif
                    return (EADDRNOTAVAIL);
                    }
		break;
	}
	switch (cmd) {

	case SIOCGIFADDR:
		*((struct sockaddr_in *)&ifr->ifr_addr) = ia->ia_addr;
		break;

	case SIOCGIFBRDADDR:
		if ((ifp->if_flags & IFF_BROADCAST) == 0)
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                    WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_ERROR, 9, 5, 
                                     WV_NETEVENT_INCTRL_BADFLAGS, cmd, ifp)
#endif  /* INCLUDE_WVNET */
#endif
                    return (EINVAL);
                    }
		*((struct sockaddr_in *)&ifr->ifr_dstaddr) = ia->ia_broadaddr;
		break;

	case SIOCGIFDSTADDR:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                    WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_ERROR, 9, 5, 
                                     WV_NETEVENT_INCTRL_BADFLAGS, cmd, ifp)
#endif  /* INCLUDE_WVNET */
#endif
                    return (EINVAL);
                    }
		*((struct sockaddr_in *)&ifr->ifr_dstaddr) = ia->ia_dstaddr;
		break;

	case SIOCGIFNETMASK:
		*((struct sockaddr_in *)&ifr->ifr_addr) = ia->ia_sockmask;
		break;

	case SIOCSIFDSTADDR:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                    WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_ERROR, 9, 5, 
                                     WV_NETEVENT_INCTRL_BADFLAGS, cmd, ifp)
#endif  /* INCLUDE_WVNET */
#endif
                    return (EINVAL);
                    }
		oldaddr = ia->ia_dstaddr;
		ia->ia_dstaddr = *(struct sockaddr_in *)&ifr->ifr_dstaddr;
		if (ifp->if_ioctl && (error = (*ifp->if_ioctl)
					(ifp, SIOCSIFDSTADDR, (caddr_t)ia))) {
			ia->ia_dstaddr = oldaddr;
			return (error);
		}
		if (ia->ia_flags & IFA_ROUTE) {
			ia->ia_ifa.ifa_dstaddr = (struct sockaddr *)&oldaddr;
			rtinit(&(ia->ia_ifa), (int)RTM_DELETE, RTF_HOST);
			ia->ia_ifa.ifa_dstaddr =
					(struct sockaddr *)&ia->ia_dstaddr;
			rtinit(&(ia->ia_ifa), (int)RTM_ADD, RTF_HOST|RTF_UP);
		}
		break;

	case SIOCSIFBRDADDR:
		if ((ifp->if_flags & IFF_BROADCAST) == 0)
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
                    WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_ERROR, 9, 5, 
                                     WV_NETEVENT_INCTRL_BADFLAGS, cmd, ifp)
#endif  /* INCLUDE_WVNET */
#endif
                    return (EINVAL);
                    }
		ia->ia_broadaddr = *(struct sockaddr_in *)&ifr->ifr_broadaddr;
		break;

	case SIOCSIFADDR:
		return (in_ifinit(ifp, ia,
		    (struct sockaddr_in *) &ifr->ifr_addr, 1));

	case SIOCSIFNETMASK:
		i = ifra->ifra_addr.sin_addr.s_addr;
		ia->ia_subnetmask = ntohl(ia->ia_sockmask.sin_addr.s_addr = i);
		in_socktrim (&ia->ia_sockmask);
		break;

	case SIOCAIFADDR:
		maskIsNew = 0;
		hostIsNew = 1;
		error = 0;
		if (ia->ia_addr.sin_family == AF_INET) {
			if (ifra->ifra_addr.sin_len == 0) {
				ifra->ifra_addr = ia->ia_addr;
				hostIsNew = 0;
			} else if (ifra->ifra_addr.sin_addr.s_addr ==
					       ia->ia_addr.sin_addr.s_addr)
				hostIsNew = 0;
		}

		if (ifra->ifra_mask.sin_len) {
			in_ifscrub(ifp, ia);
			ia->ia_sockmask = ifra->ifra_mask;
			ia->ia_subnetmask =
			     ntohl(ia->ia_sockmask.sin_addr.s_addr);
			maskIsNew = 1;
		}

		if ((ifp->if_flags & IFF_POINTOPOINT) &&
		    (ifra->ifra_dstaddr.sin_family == AF_INET)) {
			in_ifscrub(ifp, ia);
			ia->ia_dstaddr = ifra->ifra_dstaddr;
			maskIsNew  = 1; /* We lie; but the effect's the same */
		}
		if (ifra->ifra_addr.sin_family == AF_INET &&
		    (hostIsNew || maskIsNew))
			error = in_ifinit(ifp, ia, &ifra->ifra_addr, 0);
                /*
                 * Ignore the report of an existing route entry if
                 * this new (alias) IP address might use the same
                 * logical subnet as another address/netmask pair.
                 * The SIOCAIFADDR operation continues to provide
                 * the duplicate route error when assigning the
                 * initial address for an interface.
                 */

                if (error == EEXIST)
                    {
                    /*
                     * The information necessary to determine whether this
                     * address/netmask assignment absolutely requires an
                     * associated route entry is not readily available.
                     * This operation assumes existing routes to a logical
                     * subnet use the same interface as the new address entry.
                     * Under certain circumstances, the ability to ignore
                     * existing routes when assigning additional IP addresses
                     * might allow users to successfully configure interfaces
                     * which are unable to transmit data.
                     */

                    if (newRouteFlag == FALSE)
                        error = 0;
                    }

                /*
                 * Replace the mask-based broadcast address if the
                 * user provides a specific value.
                 */

                if ((ifp->if_flags & IFF_BROADCAST) &&
                    (ifra->ifra_broadaddr.sin_family == AF_INET) &&
                    (ifra->ifra_broadaddr.sin_addr.s_addr))
                        ia->ia_broadaddr = ifra->ifra_broadaddr;

		return (error);

	case SIOCDIFADDR:
		in_ifscrub(ifp, ia);
		if ((ifa = ifp->if_addrlist) == (struct ifaddr *)ia)
			ifp->if_addrlist = ifa->ifa_next;
		else {
			while (ifa->ifa_next &&
			       (ifa->ifa_next != (struct ifaddr *)ia))
				    ifa = ifa->ifa_next;
			if (ifa->ifa_next)
				ifa->ifa_next = ((struct ifaddr *)ia)->ifa_next;
			else
				logMsg("Couldn't unlink inifaddr from ifp\n",
				       0,0,0,0,0,0);
		}
		oia = ia;
#ifdef VIRTUAL_STACK
		if (oia == (ia = _in_ifaddr))
			_in_ifaddr = ia->ia_next;
#else
		if (oia == (ia = in_ifaddr))
			in_ifaddr = ia->ia_next;
#endif /* VIRTUAL_STACK */
		else {
			while (ia->ia_next && (ia->ia_next != oia))
				ia = ia->ia_next;
			if (ia->ia_next)
				ia->ia_next = oia->ia_next;
			else
				logMsg("Didn't unlink inifadr from list\n",
				       0,0,0,0,0,0);
		}
		IFAFREE((&oia->ia_ifa));
		break;

	default:
		if (ifp == 0 || ifp->if_ioctl == 0)
			return (EOPNOTSUPP);
		return ((*ifp->if_ioctl)(ifp, cmd, data));
	}
	return (0);
}

/*
 * Delete any existing route for an interface.
 */
void
in_ifscrub(ifp, ia)
	register struct ifnet *ifp;
	register struct in_ifaddr *ia;
{
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 19, 8, 
                         WV_NETEVENT_INROUTEDEL_START, ifp)
#endif  /* INCLUDE_WVNET */
#endif

	if ((ia->ia_flags & IFA_ROUTE) == 0)
		return;
	if (ifp->if_flags & (IFF_LOOPBACK|IFF_POINTOPOINT))
		rtinit(&(ia->ia_ifa), (int)RTM_DELETE, RTF_HOST);
	else
		rtinit(&(ia->ia_ifa), (int)RTM_DELETE, 0);
	ia->ia_flags &= ~IFA_ROUTE;
}

/*
 * Initialize an interface's internet address
 * and routing table entry.
 */
int
in_ifinit(ifp, ia, sin, scrub)
	register struct ifnet *ifp;
	register struct in_ifaddr *ia;
	struct sockaddr_in *sin;
	int scrub;
{
	register u_long i = ntohl(sin->sin_addr.s_addr);
	struct sockaddr_in oldaddr;
	int s = splimp(), flags = RTF_UP, error, ether_output();

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_VERBOSE, 20, 9, 
                     WV_NETEVENT_INIFINIT_START, ifp, sin->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

	oldaddr = ia->ia_addr;
	ia->ia_addr = *sin;
	/*
	 * Give the interface a chance to initialize
	 * if this is its first address,
	 * and to validate the address if necessary.
	 */
	if (ifp->if_ioctl &&
	    (error = (*ifp->if_ioctl)(ifp, SIOCSIFADDR, (caddr_t)ia))) {
		splx(s);
		ia->ia_addr = oldaddr;
		return (error);
	}
	if (ifp->if_output == ether_output) { /* XXX: Another Kludge */
		ia->ia_ifa.ifa_rtrequest = arp_rtrequest;
		ia->ia_ifa.ifa_flags |= RTF_CLONING;
	}
	splx(s);
	if (scrub) {
		ia->ia_ifa.ifa_addr = (struct sockaddr *)&oldaddr;
		in_ifscrub(ifp, ia);
		ia->ia_ifa.ifa_addr = (struct sockaddr *)&ia->ia_addr;
	}
	if (IN_CLASSA(i))
		ia->ia_netmask = IN_CLASSA_NET;
	else if (IN_CLASSB(i))
		ia->ia_netmask = IN_CLASSB_NET;
	else
		ia->ia_netmask = IN_CLASSC_NET;
	/*
	 * The subnet mask usually includes at least the standard network part,
	 * but may may be smaller in the case of supernetting.
	 * If it is set, we believe it.
	 */
	if (ia->ia_subnetmask == 0) {
		ia->ia_subnetmask = ia->ia_netmask;
		ia->ia_sockmask.sin_addr.s_addr = htonl(ia->ia_subnetmask);
	} else
		ia->ia_netmask &= ia->ia_subnetmask;
	ia->ia_net = i & ia->ia_netmask;
	ia->ia_subnet = i & ia->ia_subnetmask;
	in_socktrim(&ia->ia_sockmask);
	/*
	 * Add route for the network.
	 */
	ia->ia_ifa.ifa_metric = ifp->if_metric;
	if (ifp->if_flags & IFF_BROADCAST) {
		ia->ia_broadaddr.sin_addr.s_addr =
			htonl(ia->ia_subnet | ~ia->ia_subnetmask);
		ia->ia_netbroadcast.s_addr =
			htonl(ia->ia_net | ~ ia->ia_netmask);
	} else if (ifp->if_flags & IFF_LOOPBACK) {
		ia->ia_ifa.ifa_dstaddr = ia->ia_ifa.ifa_addr;
		flags |= RTF_HOST;
	} else if (ifp->if_flags & IFF_POINTOPOINT) {
		if (ia->ia_dstaddr.sin_family != AF_INET)
			return (0);
		flags |= RTF_HOST;
	}
	if ((error = rtinit(&(ia->ia_ifa), (int)RTM_ADD, flags)) == 0)
		ia->ia_flags |= IFA_ROUTE;
	/*
	 * If the interface supports multicast, join the "all hosts"
	 * multicast group on that interface.
	 */
	if (ifp->if_flags & IFF_MULTICAST) {
		struct in_addr addr;
                struct in_multi * inm;
		addr.s_addr = htonl(INADDR_ALLHOSTS_GROUP);
                IN_LOOKUP_MULTI(addr, ifp, inm);
                if (inm == NULL)             /* don't add if already there ... SPR#28903 */
                    ifp->pInmMblk = in_addmulti (&addr, ifp, NULL); 
	}
	return (error);
}

/*******************************************************************************
*
* in_ifaddr_remove - remove an interface from the in_ifaddr list
*
* This function removes the internet interface address given as an input
* parameter, from the global linked-list of internet interface addresses
* pointed by in_ifaddr. This function is called before dettaching an interface
* This function also removes all the internet multicast addresses hooked to 
* the in_ifaddr. It calls in_delmulti() which in turn calls the ioctl for the
* driver to remove the ethernet multicast addresses.
* 
* NOMANUAL
* 
* RETURNS: N/A
*/
 
void in_ifaddr_remove 
    (
    struct ifaddr * pIfAddr		/* pointer to interface address */
    ) 
    {
    struct in_ifaddr ** pPtrInIfAddr; 	/* pointer to pointer to in_ifaddr */
    struct in_ifaddr *  pInIfAddr; 	/* pointer to in_ifaddr */

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_VERBOSE, 21, 10, 
                     WV_NETEVENT_INIFADDRDEL_START)
#endif  /* INCLUDE_WVNET */
#endif
 
 
#ifdef VIRTUAL_STACK
    pPtrInIfAddr = &_in_ifaddr; 	/* global variable */
#else
    pPtrInIfAddr = &in_ifaddr; 		/* global variable */
#endif /* VIRTUAL_STACK */
    while (*pPtrInIfAddr != NULL)
	{
	if (*pPtrInIfAddr == (struct in_ifaddr *)pIfAddr)
	    {
	    pInIfAddr = *pPtrInIfAddr; 
	    *pPtrInIfAddr = (*pPtrInIfAddr)->ia_next; 	/* delete from list */
	    IFAFREE (&pInIfAddr->ia_ifa);		/* free address */
	    break; 
	    }
	pPtrInIfAddr = &(*pPtrInIfAddr)->ia_next; 
	}
    } 

/*
 * Return 1 if the address might be a local broadcast address.
 */
int
in_broadcast(in, ifp)
	struct in_addr in;
        struct ifnet *ifp;
{
	register struct ifaddr *ifa;
	u_long t;

	if (in.s_addr == INADDR_BROADCAST ||
	    in.s_addr == INADDR_ANY)
		return 1;
	if ((ifp->if_flags & IFF_BROADCAST) == 0)
		return 0;
	t = ntohl(in.s_addr);
	/*
	 * Look through the list of addresses for a match
	 * with a broadcast address.
	 */
#define ia ((struct in_ifaddr *)ifa)
	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
		if (ifa->ifa_addr->sa_family == AF_INET &&
		    (in.s_addr == ia->ia_broadaddr.sin_addr.s_addr ||
		     in.s_addr == ia->ia_netbroadcast.s_addr ||
		     /*
		      * Check for old-style (host 0) broadcast.
		      */
		     t == ia->ia_subnet || t == ia->ia_net))
			    return 1;
	return (0);
#undef ia
}

/*
 * Add an address to the list of IP multicast addresses for a given interface.
 */
struct mBlk *
in_addmulti(ap, ifp, pInPcb)
	register struct in_addr *ap;
	register struct ifnet *ifp;
	register struct inpcb * pInPcb;
{
	register struct in_multi *inm;
	struct ifreq ifr;
        struct mBlk * pInPcbMblk = NULL;
        struct mBlk * pInmMblk = NULL;
	struct mBlk * pInmFirstMblk = NULL;
	int s = splnet();

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_VERBOSE, 58, 11, 
                     WV_NETEVENT_INADDMULT_START, ifp, ap->s_addr)
#endif  /* INCLUDE_WVNET */
#endif

	/*
	 * See if address already in list.
	 */
	IN_LOOKUP_MULTI(*ap, ifp, inm);
	if (inm != NULL) {
		/*
		 * Found it; duplicate the mBlk, increment reference count.
		 */
        	if ((pInmMblk = mBlkGet (_pNetSysPool, M_DONTWAIT, MT_IPMADDR))
                    == NULL)
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_EMERGENCY, 12, 3, 
                             WV_NETEVENT_INADDMULT_NOBUFS, ifp, ap->s_addr)
#endif  /* INCLUDE_WVNET */
#endif

                    goto inAddMultiError;
                    }

                /*
		 * XXX - The in_multi structures are stored in Mblk chains
		 * instead of arrays like in BSD. Therefore when a second 
		 * socket joins a multicast group, the Mblk for the in_multi
		 * structure is duplicated for this new member. The Mblk
		 * address for the in_multi is obtained by using a pointer
		 * to the start of the Mblk. This pointer is located just
		 * before the structure and is retrieved using DATA_TO_MBLK.
		 * This poses a problem when there is more than one Mblk
		 * accessing the in_multi structure (like in this case) since
		 * Mblk information for the second Mblk is lost. To fix this
		 * (SPR #67536), we use the mNextPkt field of the first Mblk
		 * to store the Mblk address for the second Mblk. When the 
		 * first Mblk is deleted (leaving group), the address of
		 * second Mblk can be still be retrieved through the pointer.
		 * Though this is not the best way to fix this problem, we
		 * leave it this way since the entire design needs to be 
		 * rethought (using the Mblk back-ptr is not a good idea).
		 */

                pInmFirstMblk = DATA_TO_MBLK (inm);
                netMblkDup (pInmFirstMblk, pInmMblk);
		pInmFirstMblk->mBlkHdr.mNextPkt = pInmMblk;
		pInmMblk->mBlkHdr.mNextPkt = NULL;

                ++inm->inm_refcount;
	}
	else {
		/*
		 * New address; allocate a new multicast record
		 * and link it into the interface's multicast list.
		 */
	        MALLOC(inm, struct in_multi *, sizeof(*inm), MT_IPMADDR,
                       M_DONTWAIT);
                if (inm == NULL)
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_EMERGENCY, 12, 3, 
                             WV_NETEVENT_INADDMULT_NOBUFS, ifp, ap->s_addr)
#endif  /* INCLUDE_WVNET */
#endif
                    goto inAddMultiError;
                    }
                pInmMblk = DATA_TO_MBLK (inm);
		pInmMblk->mBlkHdr.mNextPkt = NULL;

                bzero ((char *)inm, sizeof (*inm));  /* zero the structure */
		inm->inm_addr = *ap;
		inm->inm_ifp = ifp;
		inm->inm_refcount = 1;

                /* insert the multicast address into the hash table */
                if (mcastHashTblInsert (inm) != OK)
                    goto inAddMultiError;

		/*
		 * Ask the network driver to update its multicast reception
		 * filter appropriately for the new address.
		 */
		((struct sockaddr_in *)&ifr.ifr_addr)->sin_family = AF_INET;
		((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr = *ap;
		if ((ifp->if_ioctl == NULL) ||
		    (*ifp->if_ioctl)(ifp, SIOCADDMULTI,(caddr_t)&ifr) != 0) {
                	mcastHashTblDelete (inm);
                        goto inAddMultiError;
		}
		/*
		 * Let IGMP know that we have joined a new IP multicast group.
		 */
		if (_igmpJoinGrpHook != NULL)
		    (*_igmpJoinGrpHook) (inm); 
	}
        /*
         * insert the protocol control block into inm pcb list
         * this is done to get to the list of pcbs which have joined a
         * multicast group. When the hash table is looked up the inm
         * returned should have a list of pcbs which the datagram
         * should be delivered to. refer udp_usrreq.c where the pcb
         * list is used.
         */
        if (pInPcb != NULL)
            {
            if ((pInPcbMblk = mBlkGet (_pNetSysPool, M_DONTWAIT, MT_PCB))
                == NULL)
                {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
            WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_EMERGENCY, 12, 3, 
                             WV_NETEVENT_INADDMULT_NOBUFS, ifp, ap->s_addr)
#endif  /* INCLUDE_WVNET */
#endif
                goto inAddMultiError;
                }

            netMblkDup (DATA_TO_MBLK (pInPcb), pInPcbMblk);
            pInPcbMblk->mBlkHdr.mNext = inm->pInPcbMblk;
            inm->pInPcbMblk = pInPcbMblk;
            }
        
	splx(s);
	return (pInmMblk);	/* return the mBlk pointing to the inm */

inAddMultiError:
        splx (s);
        if (pInmMblk)
            m_free (pInmMblk);
        return (NULL);
}

/*
 * Delete a multicast address record.
 * deletes all pcbs if pInPcb is NULL. 
 */
int
in_delmulti(pInmMblk, pInPcb)
    	register struct mBlk * pInmMblk;
	register struct inpcb * pInPcb;
{
	register struct in_multi *inm;
        M_BLK_ID *		 ppMblk;
        M_BLK_ID		 pInPcbMblk; 
	struct mBlk **           ppInmMblk;
	struct ifreq ifr;
	int s = splnet();

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_VERBOSE, 22, 12, 
                     WV_NETEVENT_INDELMULT_START)
#endif  /* INCLUDE_WVNET */
#endif

        inm = mtod (pInmMblk, struct in_multi *);

        /* delete the pcb from the list */
        ppMblk = &inm->pInPcbMblk;

        if (pInPcb != NULL)
            {
            while ((pInPcbMblk = *ppMblk))
                {
                if (pInPcb == mtod (pInPcbMblk, struct inpcb *))
                    {
                    *ppMblk = (*ppMblk)->mBlkHdr.mNext;
                    m_free (pInPcbMblk);
                    break;		/* jump out of the while loop */
                    }
                ppMblk = &(*ppMblk)->mBlkHdr.mNext;
                }
            }
        else if (inm->pInPcbMblk != NULL)
            m_freem (inm->pInPcbMblk);	/* free up all pcbs */

	if (--inm->inm_refcount == 0) {
		/*
		 * No remaining claims to this record; let IGMP know that
		 * we are leaving the multicast group.
		 */
		if (_igmpLeaveGrpHook != NULL)
		    (*_igmpLeaveGrpHook) (inm); 

		/*
		 * Notify the network driver to update its multicast reception
		 * filter.
		 */
		((struct sockaddr_in *)&(ifr.ifr_addr))->sin_family = AF_INET;
		((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr =
								inm->inm_addr;
		(*inm->inm_ifp->if_ioctl)(inm->inm_ifp, SIOCDELMULTI,
							     (caddr_t)&ifr);
                /* time to delete entry from hash table */
                mcastHashTblDelete (inm);
	}

        /*
	 * If there is another socket listening to this group, replace
	 * inm's back-ptr to its Mblk (currently that of socket leaving
	 * the group) with the pointer to the Mblk of the other socket
	 */

        if ((inm != NULL) && (pInmMblk->mBlkHdr.mNextPkt != NULL))
	    {
	    ppInmMblk = (struct mbuf **)((char *)inm - sizeof (struct mbuf **));
	    *ppInmMblk = pInmMblk->mBlkHdr.mNextPkt;
	    (*ppInmMblk)->mBlkHdr.mNextPkt = NULL;
	    }

        /* free the mblk & inm, actual free only clRefCnt is 1 */

        m_free (pInmMblk); 	
	splx(s);

	return (OK);
}

/*
 * Return address info for specified internet network.
 */
struct in_ifaddr *
in_iaonnetof(net)
	u_long net;
{
	register struct in_ifaddr *ia;

#ifdef VIRTUAL_STACK
	for (ia = _in_ifaddr; ia; ia = ia->ia_next)
#else
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
#endif /* VIRTUAL_STACK */
		if (ia->ia_subnet == net)
			return (ia);
	return ((struct in_ifaddr *)0);
}

/* multicast hashing functions */

void mcastHashInit ()
    {

    /* initialize the mcast hash table */
    mCastHashInfo.hashBase =  hashinit (mCastHashTblSize,
                                        MT_IPMADDR,
                                        &mCastHashInfo.hashMask);
    }

STATUS mcastHashTblInsert
    (
    struct in_multi * 	pInMulti
    )
    {
    IN_MULTI_HEAD * 	inMultiHead;
    int 		s; 

    s = splnet();
    inMultiHead = &mCastHashInfo.hashBase[MCAST_HASH(pInMulti->inm_addr.s_addr,
                                                     pInMulti->inm_ifp,
                                                     mCastHashInfo.hashMask)];

    /* insert a multicast address structure into the hash table */
    LIST_INSERT_HEAD(inMultiHead, pInMulti, inm_hash);
    
    splx(s);
    return (OK); 
    }

STATUS mcastHashTblDelete
    (
    struct in_multi * pInMulti
    )
    {
    int 		s; 
    s = splnet();

    LIST_REMOVE(pInMulti, inm_hash);

    splx(s);
    return (OK); 
    }

struct in_multi * mcastHashTblLookup
    (
    int			mcastAddr,	/* multicast Address always in NBO */
    struct ifnet * 	pIf		/* interface pointer */
    )
    {
    IN_MULTI_HEAD * 	inMultiHead;
    IN_MULTI	* 	pInMulti;
    int 		s;

    s = splnet();

    /* search a hash table for the appropriate multicast address tied to
       a particular network interface
     */
    inMultiHead = &mCastHashInfo.hashBase[MCAST_HASH(mcastAddr, pIf,
                                                     mCastHashInfo.hashMask)];
    for (pInMulti = inMultiHead->lh_first; pInMulti != NULL;
         pInMulti = pInMulti->inm_hash.le_next)
        {
        if (pInMulti->inm_addr.s_addr == mcastAddr &&
            pInMulti->inm_ifp == pIf)
            goto found;
        }

    splx(s);
    return (NULL);

    found:
    /*
     * Move IN_MULTI to head of this hash chain so that it can be
     * found more quickly in the future.
     * XXX - this is a pessimization on machines with few
     * concurrent connections.
     */

    if (pInMulti != inMultiHead->lh_first)
        {
        LIST_REMOVE(pInMulti, inm_hash);
        LIST_INSERT_HEAD(inMultiHead, pInMulti, inm_hash);
        }

    splx(s);
    return (pInMulti);
    }

#endif

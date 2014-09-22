/* ip_mroute.c - internet multicast routing routines */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * IP multicast forwarding procedures
 *
 * Written by David Waitzman, BBN Labs, August 1988.
 * Modified by Steve Deering, Stanford, February 1989.
 * Modified by Mark J. Steiglitz, Stanford, May, 1991
 * Modified by Van Jacobson, LBL, January 1993
 * Modified by Ajit Thyagarajan, PARC, August 1993
 * Modified by Bill Fenner, PARC, April 1995
 *
 * MROUTING Revision: 3.5
 * $Id: ip_mroute.c,v 1.53 1999/01/18 02:06:57 fenner Exp $
 */

/*
modification history
--------------------
01i,16jul02,gls  removed include of igmpRouterLib.h
01h,25oct01,rae  _mCastRouteFwdHook must be IMPORT
01g,12oct01,rae  merge from truestack ver 01x, base 01e (SPRs 70325,
                 69594, 69643, upgrade to MROUTE 3.5, etc).
01f,24sep01,gls  removed definition and use of __P() macro (SPR #28330)
01e,21aug98,n_s  fixed add_lgrp () call to MALLOC(). spr20595.
01d,08apr97,vin  added mCastRouteFwdHook deleted ip_mrouter.
01c,05dec96,vin  changed malloc(..) to MALLOC(..) to use only network buffers,
		 changed free(..) to FREE(..).
01b,22nov96,vin  added cluster support replaced m_gethdr(..) with mHdrClGet(..)
01a,03mar96,vin  created from BSD4.4 stuff.
*/

#include "vxWorks.h"/* mandatory vxWorks header */
#include "errno.h"
#include "timers.h"

#include "net/mbuf.h"
#include "sys/socket.h"
#include "sys/times.h"
#include "net/socketvar.h"
#include "net/protosw.h"
#include "sys/times.h"
#include "net/route.h"
#include "net/mbuf.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "netinet/in_var.h"
#include "netinet/igmp.h"
#include "netinet/igmp_var.h"
#include "memPartLib.h"
#include "tickLib.h"
#include "wdLib.h"
#include "sockLib.h"
#include "timers.h"
#include "net/mbuf.h"
#include "wdLib.h"
#include "netinet/ip_mroute.h"
#include "netinet/udp.h"
#include "netLib.h"
#include "net/systm.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsIgmp.h"
#include "netinet/vsMcast.h"
#endif /* VIRTUAL_STACK */



#ifndef NTOHL
#if BYTE_ORDER != BIG_ENDIAN
#define NTOHL(d) ((d) = ntohl((d)))
#define NTOHS(d) ((d) = ntohs((u_short)(d)))
#define HTONL(d) ((d) = htonl((d)))
#define HTONS(d) ((d) = htons((u_short)(d)))
#else
#define NTOHL(d)
#define NTOHS(d)
#define HTONL(d)
#define HTONS(d)
#endif
#endif


/*
 * Macros for handling bitmaps with one bit per virtual interface.
 */
#define ALL_VIFS (vifi_t)-1
#define	VIFM_SET(n, m)		((m) |= (1 << (n)))
#define	VIFM_CLR(n, m)		((m) &= ~(1 << (n)))
#define	VIFM_ISSET(n, m)	((m) & (1 << (n)))
#define	VIFM_CLRALL(m)		((m) = 0x00000000)
#define	VIFM_COPY(mfrom, mto)	((mto) = (mfrom))
#define	VIFM_SAME(m1, m2)	((m1) == (m2))


/*
 * Globals.  All but ip_mrouter and ip_mrtproto could be static,
 * except for netstat or debugging purposes.
 */

#ifndef VIRTUAL_STACK
extern USHORT ip_id;

struct socket * ip_mrouter = 0;

IMPORT FUNCPTR _mCastRouteFwdHook;
FUNCPTR _pimCacheMissSendHook;


static struct mrtstat	mrtstat;
/* table of mfc entries */
static struct mfc	*mfctable[MFCTBLSIZ];
static u_char		nexpire[MFCTBLSIZ];
/* table of virtual interfaces */
static struct vif	viftable[MAXVIFS];
static u_int	mrtdebug = 0;	  /* debug level 	*/
static u_int  	tbfdebug = 0;     /* tbf debug level 	*/



struct mroute_timer_struct*  expire_upcalls_ch;

/*
 * Define the token bucket filter structures
 * tbftable -> each vif has one of these for storing info 
 */
static struct tbf tbftable[MAXVIFS];
/*
 * 'Interfaces' associated with decapsulator (so we can tell
 * packets that went through it from ones that get reflected
 * by a broken gateway).  These interfaces are never linked into
 * the system ifnet list & no routes point to them.  I.e., packets
 * can't be sent this way.  They only exist as a placeholder for
 * multicast source verification.
 */
static struct ifnet multicast_decap_if[MAXVIFS];
/*
 * Private variables.
 */
static vifi_t	   numvifs = 0;
static int have_encap_tunnel = 0;

/*
 * one-back cache used by ipip_input to locate a tunnel's vif
 * given a datagram's src ip address.
 */
static u_long last_encap_src;
static struct vif *last_encap_vif;
int pim_assert;



#endif /* VIRTUAL_STACK */

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_IPMROUTE_MODULE; /* Value for ip_mroute.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

/* prototype IP hdr for encapsulated packets */
static struct ip multicast_encap_iphdr	= {
#if BYTE_ORDER == LITTLE_ENDIAN
	sizeof(struct ip) >> 2, IPVERSION,
#else
	IPVERSION, sizeof(struct ip) >> 2,
#endif
	0,				/* tos */
	sizeof(struct ip),		/* total length */
	0,				/* id */
	0,				/* frag offset */
	ENCAP_TTL, ENCAP_PROTO,	
	0,				/* checksum */
};


/* 
 * The timer is a wrapper for watchdog timers... functionality from the net/3 release of the bsd code.  
 */


enum sopt_dir { SOPT_GET, SOPT_SET };
struct sockopt {
	enum	sopt_dir sopt_dir; /* is this a get or a set? */
	int	sopt_level;	/* second arg of [gs]etsockopt */
	int	sopt_name;	/* third arg of [gs]etsockopt */
	void   *sopt_val;	/* fourth arg of [gs]etsockopt */
	size_t	sopt_valsize;	/* (almost) fifth arg of [gs]etsockopt */
	struct	proc *sopt_p;	/* calling process or null if kernel */
};

/* timer struct here enables */
typedef struct mroute_timer_struct
{
	WDOG_ID watchdog_id;
	VOIDFUNCPTR expire_routine;
	void* argument;
} MROUTE_TIMER_STRUCT;







static u_long	X_ip_mcast_src __P((int vifi));
static int	X_ip_mforward __P(( struct mbuf *m, struct ifnet *ifp, struct ip *ip, struct ip_moptions *imo) );
static int	X_ip_mrouter_done __P((void));
static int	X_ip_mrouter_get __P((struct socket *so, struct sockopt *m));
static int	X_ip_mrouter_set __P((struct socket *so, struct sockopt *m));
static int	X_legal_vif_num __P((int vif));
static int	X_mrt_ioctl __P((int cmd, caddr_t data));
static int get_sg_cnt(struct sioc_sg_req *);
static int get_vif_cnt(struct sioc_vif_req *);
static int ip_mrouter_init(struct socket *, int);
static int add_vif(struct vifctl *);
static int del_vif(vifi_t);
static int add_mfc(struct mfcctl *);
static int del_mfc(struct mfcctl *);
static int set_assert(FUNCPTR);
static int setDebug(int);
static void expire_upcalls(void *);
static int ip_mdq(struct mbuf *, struct ifnet *, struct mfc *,
		  vifi_t);
static void phyint_send(struct ip *, struct vif *, struct mbuf *);
static void encap_send(struct ip *, struct vif *, struct mbuf *);
static void tbf_control(struct vif *, struct mbuf *, struct ip *, u_long);
static void tbf_queue(struct vif *, struct mbuf *);
static void tbf_process_q(struct vif *);
static void tbf_reprocess_q(void *);
static int tbf_dq_sel(struct vif *, struct ip *);
static void tbf_send_packet(struct vif *, struct mbuf *);
static void tbf_update_tokens(struct vif *);
static int priority(struct vif *, struct ip *);
void multiencap_decap(struct mbuf *);

static void mRouteDumpMfctable();
static BOOL oifListIsEmpty(struct mfc* rt);
static int mRouteOifsAdd(struct mfc* rt, int oif, int ttl);
static int mRouteOifsRemove(struct mfc* rt, int oif);
static int mRouteGroupLoopRemove(struct mfc* rt, struct mfc* root);

int (*ip_mrouter_done)(void) = X_ip_mrouter_done;
u_long (*ip_mcast_src)(int) = X_ip_mcast_src;



/* From other BSD code that has yet to be ported */
static int
sooptcopyin(struct sockopt *, void	*, size_t, size_t);
static int
sooptcopyout(struct sockopt *, void	*, size_t);
static int
if_allmulti(struct ifnet *, int);


/* wrappers for timers in mroute */
static void mRouteNetJobAdd(MROUTE_TIMER_STRUCT* p_timer_struct_object);
static MROUTE_TIMER_STRUCT* mRouteTimeout(VOIDFUNCPTR expire_routine, void* argument, int number_of_ticks);
static void mRouteUntimeOut(MROUTE_TIMER_STRUCT*  p_timer_struct);
#define timeout mRouteTimeout
#define untimeout mRouteUntimeOut


/* vxWorks wrapper for tickGet */
#define GET_TIME(t)	t = tickGet(); 

#ifdef  _WRS_VXWORKS_5_X
#define mRouteNameToPort(ignore, both)  1
#else
#define mRouteNameToPort igmpNameToPort
#endif

#ifndef MRT_ALLOC
#define MRT_ALLOC KHEAP_ALLOC
#endif

#ifndef MRT_FREE 
#define MRT_FREE  KHEAP_FREE
#endif

/* wrapper for header length */
#define	MLEN		(MSIZE - sizeof(M_BLK_HDR))	/* normal data len */
#define	MHLEN		(MLEN - sizeof(M_PKT_HDR))	/* data len w/pkthdr */


#define MRTDEBUG(printThis, string, param1, param2, param3, param4, param5, param6) \
   {\
   if ( (_func_logMsg != NULL) && (printThis) )\
      (* _func_logMsg) (string, param1, param2, param3, param4, param5, param6);\
   }


/* bsd function wrapper */
#define	MGETHDR(m, how, type) {\
		m = mHdrClGet(M_DONTWAIT, MT_DATA, sizeof(*m), TRUE);\
		}
/*
 * Hash function for a source, group entry
 */
#define MFCHASH(a, g) MFCHASHMOD(((a) >> 20) ^ ((a) >> 10) ^ (a) ^ \
			((g) >> 20) ^ ((g) >> 10) ^ (g))
/*
 * Find a route for a given origin IP address and Multicast group address
 * Type of service parameter to be added in the future!!!
 */
#define MFCFIND(o, g, rt) { \
	struct mfc *_rt = mfctable[MFCHASH(o,g)]; \
	rt = NULL; \
	++mrtstat.mrts_mfc_lookups; \
	while (_rt) { \
		if ((_rt->mfc_origin.s_addr == o) && \
		    (_rt->mfc_mcastgrp.s_addr == g) && \
		    (_rt->mfc_stall == NULL)) { \
			rt = _rt; \
			break; \
		} \
		_rt = _rt->mfc_next; \
	} \
	if (rt == NULL) { \
		++mrtstat.mrts_mfc_misses; \
	} \
}

/*
 * Macros to compute elapsed time efficiently
 * Borrowed from Van Jacobson's scheduling code
 */
#define TV_DELTA(a, b, delta)  delta = (a) - (b);
#define TV_LT(a, b) (((a).tv_usec < (b).tv_usec && \
	      (a).tv_sec <= (b).tv_sec) || (a).tv_sec < (b).tv_sec)


/* The net/3 code uses the X_ip_mrouter_set command to handle socket options in
 * a slightly different format.  X_ip_mrouter_cmd reformats the parameters */
int 
X_ip_mrouter_cmd(cmd, so, m)
int cmd;
struct socket* so;
struct mbuf* m;
{ 
	
	struct sockopt socket_option; 
	int result;

	socket_option.sopt_name = cmd;
	if (m == NULL) 
	{
		return (ERROR);
	}
	else 
	{	
		socket_option.sopt_val = m->m_data; 
		socket_option.sopt_valsize = m->mBlkHdr.mLen; 
	}
	result = X_ip_mrouter_set(so, &socket_option); 
	return (0);
}

int ip_mrouter_cmd (int cmd, struct socket* so, struct mbuf* m) 
{
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_INFO, 10, 12,
                     WV_NETEVENT_IPMRT_START, so->so_fd, cmd)
#endif  /* INCLUDE_WVNET */
#endif

	return X_ip_mrouter_cmd(cmd, so, m);
}


/*
 * Handle MRT setsockopt commands to modify the multicast routing tables.
 */
static int
X_ip_mrouter_set(so, sopt)
	struct socket *so;
	struct sockopt *sopt;
{
	int	error, optval;
	vifi_t	vifi;
	struct	vifctl vifc;
	struct	mfcctl mfc;

	error = 0;
	optval = 0;

	switch (sopt->sopt_name) {
	case MRT_INIT:
		error = sooptcopyin(sopt, &optval, sizeof (optval), 
				    sizeof (optval) );
		optval = (int)(sopt->sopt_val);
		if (error)
			break;
            MRTDEBUG
            (
                (mrtdebug &DEBUG_MFC),
                "MROUTE: IP_MROUTER received ip_mrouter init so=%p, optval=%i.\n", 
                (int)so, 
                optval,3,4,5,6
            );
		error = ip_mrouter_init(so, optval);
		break;
	case MRT_DONE:
            MRTDEBUG
            (
                (mrtdebug &DEBUG_MFC),
			    "MROUTE: IP_MROUTER received MRT_DONE.\n",
                1,2,3,4,5,6
            );
		error = ip_mrouter_done();
		break;

	case MRT_ADD_VIF:
			MRTDEBUG
            (
                (mrtdebug &DEBUG_MFC),
			    "MROUTE: IP_MROUTER received MRT_ADD_VIF.\n",
                1,2,3,4,5,6
            );
		error = sooptcopyin(sopt, &vifc, sizeof vifc, sizeof vifc);
		if (error)
			break;
		error = add_vif(&vifc);
		break;

	case MRT_DEL_VIF:
			MRTDEBUG
            (
                (mrtdebug &DEBUG_MFC),
    			"MROUTE: IP_MROUTER received MRT_DEL_VIF.\n",
                1,2,3,4,5,6
            );
		error = sooptcopyin(sopt, &vifi, sizeof (vifi), sizeof (vifi));
		if (error)
			break;
		error = del_vif(vifi);
		break;
	case MRT_ADD_MFC:
	case MRT_DEL_MFC:
            MRTDEBUG
            (
                (mrtdebug &DEBUG_MFC),
                "MROUTE: IP_MROUTER received MRT_ADD/DEL_MFC.\n",
                1,2,3,4,5,6
            );
		error = sooptcopyin(sopt, &mfc, sizeof mfc, sizeof mfc);
		if (error){
					MRTDEBUG(mrtdebug,"MROUTE: I am not dqing...  sooptcopyin error.\n",1,2,3,4,5,6);

					break;
                   }
		if (sopt->sopt_name == MRT_ADD_MFC)
			error = add_mfc(&mfc);
		else
			error = del_mfc(&mfc);
		break;

	case MRT_ASSERT:
			MRTDEBUG
            (
                (mrtdebug &DEBUG_MFC),
			    "MROUTE: IP_MROUTER received MRT_ASSERT.\n",
                1,2,3,4,5,6
            );
		error = sooptcopyin(sopt, &optval, sizeof optval, 
				    sizeof optval);
		if (error)
			break;
		set_assert( (FUNCPTR) optval);
		break;
	case MRT_DEBUG:
		MRTDEBUG
        (
            (mrtdebug &DEBUG_MFC),
            "MROUTE: IP_MROUTER received MRT_DEBUG.\n",
            1,2,3,4,5,6
        );
		error = sooptcopyin(sopt, &optval, sizeof optval, 
				    sizeof optval);
		if (error)
			break;
		setDebug(optval);
		break;

	default:
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
            WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_ERROR, 23, 3,
                             WV_NETEVENT_IPMRT_BADCMD, sopt->sopt_name)
#endif  /* INCLUDE_WVNET */
#endif

		error = EOPNOTSUPP;
		break;
	}
	return (error);
}

int (*ip_mrouter_set)(struct socket *, struct sockopt *) = X_ip_mrouter_set;

/*
 * Handle MRT getsockopt commands
 */
static int
X_ip_mrouter_get(so, sopt)
	struct socket *so;
	struct sockopt *sopt;
{
	int error;
	static int version = 0x0305; /* !!! why is this here? XXX */

	switch (sopt->sopt_name) {
	case MRT_VERSION:
		error = sooptcopyout(sopt, &version, sizeof version);
		break;

	case MRT_ASSERT:
		error = sooptcopyout(sopt, &pim_assert, sizeof pim_assert);
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}
	MRTDEBUG
    (
        (mrtdebug),
	    "MROUTE: ip_mrouter_get returns %i error", 
        error,2,3,4,5,6
    );
	return (error);
}

int (*ip_mrouter_get)(struct socket *, struct sockopt *) = X_ip_mrouter_get;

/*
 * Handle ioctl commands to obtain information from the cache
 */
static int
X_mrt_ioctl(cmd, data)
    int cmd;
    caddr_t data;
{
    int error = 0;

    switch (cmd) {
	case (SIOCGETVIFCNT):
	    return (get_vif_cnt((struct sioc_vif_req *)data));
	    break;
	case (SIOCGETSGCNT):
	    return (get_sg_cnt((struct sioc_sg_req *)data));
	    break;
	default:
	    return (EINVAL);
	    break;
    }
    return error;
}

int (*mrt_ioctl)(int, caddr_t) = X_mrt_ioctl;

/*
 * returns the packet, byte, rpf-failure count for the source group provided
 */
static int
get_sg_cnt(req)
    struct sioc_sg_req *req;
{
    struct mfc *rt;
    int s;

    s = splnet();
    MFCFIND(req->src.s_addr, req->grp.s_addr, rt);
    splx(s);
    if (rt != NULL) {
	req->pktcnt = rt->mfc_pkt_cnt;
	req->bytecnt = rt->mfc_byte_cnt;
	req->wrong_if = rt->mfc_wrong_if;
    } else
	req->pktcnt = req->bytecnt = req->wrong_if = 0xffffffff;

    return 0;
}

/*
 * returns the input and output packet and byte counts on the vif provided
 */
static int
get_vif_cnt(req)
    struct sioc_vif_req *req;
{
    vifi_t vifi = req->vifi;

    if (vifi >= numvifs) return EINVAL;

    req->icount = viftable[vifi].v_pkt_in;
    req->ocount = viftable[vifi].v_pkt_out;
    req->ibytes = viftable[vifi].v_bytes_in;
    req->obytes = viftable[vifi].v_bytes_out;

    return 0;
}

/*
 * Enable multicast routing
 */
static int
ip_mrouter_init(so, version)
	struct socket *so;
	int version;
{
    if (so == NULL)
        return (EINVAL);

    MRTDEBUG
        (
            (mrtdebug),
            "MROUTE: ip_mrouter_init: so_type = %d, pr_protocol = %d\n",
			so->so_type, 
            so->so_proto->pr_protocol,
            3,4,5,6
        );

	if ( _mCastRouteFwdHook != NULL)	/* if already installed */
            return (EADDRINUSE);
	else
		_mCastRouteFwdHook = X_ip_mforward; 	/* forwarding func ptr */

    if (so->so_type != SOCK_RAW ||
	so->so_proto->pr_protocol != IPPROTO_IGMP) return EOPNOTSUPP;

    if (ip_mrouter != 0) 
	{
		return EADDRINUSE;
	}
    ip_mrouter = (struct socket*) so->so_fd;

    bzero((caddr_t)mfctable, sizeof(mfctable));
    bzero((caddr_t)nexpire, sizeof(nexpire));

    pim_assert = 0;

    expire_upcalls_ch = timeout(expire_upcalls, (caddr_t)NULL, EXPIRE_TIMEOUT);
    MRTDEBUG
        (
            (mrtdebug),
		    "MROUTE: ip_mrouter_init returning success.\n",
            1,2,3,4,5,6
        );
    return 0;
}
/*
 * Disable multicast routing
 */
static int
X_ip_mrouter_done()
{
    vifi_t vifi;
    int i;
    struct ifnet *ifp;
    struct ifreq ifr;
    struct mfc *rt;
    struct rtdetq *rte;
    int s;

    s = splnet();

    /*
     * For each phyint in use, disable promiscuous reception of all IP
     * multicasts.
     */
    for (vifi = 0; vifi < numvifs; vifi++) {
	if (viftable[vifi].v_lcl_addr.s_addr != 0 &&
	    !(viftable[vifi].v_flags & VIFF_TUNNEL)) {
	    ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_family = AF_INET;
	    ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr.s_addr
								= INADDR_ANY;
	    ifp = viftable[vifi].v_ifp;
	    if_allmulti(ifp, 0);
	}
    }
    bzero((caddr_t)tbftable, sizeof(tbftable));
    bzero((caddr_t)viftable, sizeof(viftable));
    numvifs = 0;
    pim_assert = 0;

    untimeout(expire_upcalls_ch);
    expire_upcalls_ch = 0;

    _mCastRouteFwdHook = NULL;
    _pimCacheMissSendHook = NULL;
    /*
     * Free all multicast forwarding cache entries.
     */
    for (i = 0; i < MFCTBLSIZ; i++) {
	for (rt = mfctable[i]; rt != NULL; ) {
	    struct mfc *nr = rt->mfc_next;

	    for (rte = rt->mfc_stall; rte != NULL; ) {
		struct rtdetq *n = rte->next;
		m_freem(rte->m);
		MRT_FREE((char*) rte);
		rte = n;
	    }
	    MRT_FREE((char*) rt);
	    rt = nr;
	}
    }

    bzero((caddr_t)mfctable, sizeof(mfctable));

    /*
     * Reset de-encapsulation cache
     */
    last_encap_src = 0;
    last_encap_vif = NULL;
    have_encap_tunnel = 0;
 
    ip_mrouter = 0;

    splx(s);

    MRTDEBUG
        (
            (mrtdebug),
		    "MROUTE: ip_mrouter_done\n",
            1,2,3,4,5,6
        );


    return 0;
}


/*
 * Set PIM assert processing global
 */
static int
set_assert(FUNCPTR i)
{
    pim_assert = (int)i;

	_pimCacheMissSendHook = (FUNCPTR)i;
    return 0;
}

/*
 * Sets mroute debug processing global
 */
static int
setDebug( int i)
{
    mrtdebug = i;
    return 0;
}

/*
 * Add a vif to the vif table
 */
static int
add_vif(vifcp)
    struct vifctl *vifcp;
{
    struct vif *vifp = viftable + vifcp->vifc_vifi;
    static struct sockaddr_in sin = {sizeof sin, AF_INET};
    struct ifaddr *ifa;
    struct ifnet *ifp;
    int error, s;
    struct tbf *v_tbf = tbftable + vifcp->vifc_vifi;

    if (vifcp->vifc_vifi >= MAXVIFS)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
        WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_ERROR, 24, 4,
                         WV_NETEVENT_ADDVIF_BADINDEX,
                         vifcp->vifc_vifi, MAXVIFS)
#endif  /* INCLUDE_WVNET */
#endif

        return EINVAL;
        }

    if (vifp->v_lcl_addr.s_addr != 0)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
        WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_ERROR, 25, 5,
                         WV_NETEVENT_ADDVIF_BADENTRY,
                         vifcp->vifc_vifi, vifp->v_lcl_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

        return EADDRINUSE;
        }

    /* Find the interface with an address in AF_INET family */
    sin.sin_addr = vifcp->vifc_lcl_addr;
    ifa = ifa_ifwithaddr((struct sockaddr *)&sin);
    if (ifa == 0)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
        WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_ERROR, 26, 6,
                         WV_NETEVENT_ADDVIF_SEARCHFAIL,
                         vifcp->vifc_vifi, sin.sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

        return EADDRNOTAVAIL;
        }
    ifp = ifa->ifa_ifp;

    if (vifcp->vifc_flags & VIFF_TUNNEL) {
	if ((vifcp->vifc_flags & VIFF_SRCRT) == 0) {
		/*
		 * An encapsulating tunnel is wanted.  Tell ipip_input() to
		 * start paying attention to encapsulated packets.
		 */
		if (have_encap_tunnel == 0) {
			have_encap_tunnel = 1;
			for (s = 0; s < MAXVIFS; ++s) {
				multicast_decap_if[s].if_name = "mdecap";
				multicast_decap_if[s].if_unit = s;
			}
		}
		/*
		 * Set interface to fake encapsulator interface
		 */
		ifp = &multicast_decap_if[vifcp->vifc_vifi];
		/*
		 * Prepare cached route entry
		 */
		bzero((char*)&vifp->v_route, sizeof(vifp->v_route));
	} else {
        MRTDEBUG
        (
            (mrtdebug),
		    "MROUTE: source routed tunnels not supported\n",
            1,2,3,4,5,6
        );
	    return EOPNOTSUPP;
	}
    } else {
	/* Make sure the interface supports multicast */
	if ((ifp->if_flags & IFF_MULTICAST) == 0)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
            WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_ERROR, 27, 7,
                             WV_NETEVENT_ADDVIF_BADFLAGS,
                             vifcp->vifc_vifi, ifp)
#endif  /* INCLUDE_WVNET */
#endif

	    return EOPNOTSUPP;
            }
	/* Enable promiscuous reception of all IP multicasts from the if */
	s = splnet();
	error = if_allmulti(ifp, 1);/* Many wrs drivers don't support this */
	splx(s);
	if (error)
	    return error;
    }

    s = splnet();
    /* define parameters for the tbf structure */
    vifp->v_tbf = v_tbf;

    GET_TIME(vifp->v_tbf->tbf_last_pkt_t);

    vifp->v_tbf->tbf_n_tok = 0;
    vifp->v_tbf->tbf_q_len = 0;
    vifp->v_tbf->tbf_max_q_len = MAXQSIZE;
    vifp->v_tbf->tbf_q = vifp->v_tbf->tbf_t = NULL;

    vifp->v_flags     = vifcp->vifc_flags;
    vifp->v_threshold = vifcp->vifc_threshold;
    vifp->v_lcl_addr  = vifcp->vifc_lcl_addr;
    vifp->v_rmt_addr  = vifcp->vifc_rmt_addr;
    vifp->v_ifp       = ifp;
    /* scaling up here allows division by 1024 in critical code */
    vifp->v_rate_limit= vifcp->vifc_rate_limit * 1024 / 1000;
    /* initialize per vif pkt counters */
    vifp->v_pkt_in    = 0;
    vifp->v_pkt_out   = 0;
    vifp->v_bytes_in  = 0;
    vifp->v_bytes_out = 0;
    splx(s);

    /* Adjust numvifs up if the vifi is higher than numvifs */
    if (numvifs <= vifcp->vifc_vifi) numvifs = vifcp->vifc_vifi + 1;

    MRTDEBUG
    (
        (mrtdebug),
		"MROUTE: add_vif #%d, lcladdr %lx, %s %lx, thresh %x, rate %d\n",
        vifcp->vifc_vifi, 
		(u_long)ntohl(vifcp->vifc_lcl_addr.s_addr),
        (vifcp->vifc_flags & VIFF_TUNNEL) ? (int)"rmtaddr" : (int)"mask",
        (u_long)ntohl(vifcp->vifc_rmt_addr.s_addr),
        (int)vifcp->vifc_threshold, 
        1
    );    
    return 0;
}

/*
 * Delete a vif from the vif table
 */
static int
del_vif(vifi)
	vifi_t vifi;
{
    struct vif *vifp = &viftable[vifi];
    struct mbuf *m;
    struct ifnet *ifp;
    struct ifreq ifr;
    int s;

    if (vifi >= numvifs)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
        WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_ERROR, 28, 8,
                         WV_NETEVENT_DELVIF_BADINDEX, vifi, numvifs)
#endif  /* INCLUDE_WVNET */
#endif

        return EINVAL;
        }
    if (vifp->v_lcl_addr.s_addr == 0)
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
        WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_ERROR, 29, 9,
                         WV_NETEVENT_DELVIF_BADENTRY, vifi)
#endif  /* INCLUDE_WVNET */
#endif

        return EADDRNOTAVAIL;
        }
    s = splnet();

    if (!(vifp->v_flags & VIFF_TUNNEL)) {
	((struct sockaddr_in *)&(ifr.ifr_addr))->sin_family = AF_INET;
	((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr.s_addr = INADDR_ANY;
	ifp = vifp->v_ifp;
	if_allmulti(ifp, 0);
    }

    if (vifp == last_encap_vif) {
	last_encap_vif = 0;
	last_encap_src = 0;
    }

    /*
     * Free packets queued at the interface
     */
    while (vifp->v_tbf->tbf_q) {
	m = vifp->v_tbf->tbf_q;
	vifp->v_tbf->tbf_q = m->m_act;
	m_freem(m);
    }

    bzero((caddr_t)vifp->v_tbf, sizeof(*(vifp->v_tbf)));
    bzero((caddr_t)vifp, sizeof (*vifp));

    MRTDEBUG
    (
        (mrtdebug),
        "MROUTE: del_vif %d, numvifs %d\n", 
        vifi, 
        numvifs,
        3,4,5,6
    );

    /* Adjust numvifs down */
    for (vifi = numvifs; vifi > 0; vifi--)
	if (viftable[vifi-1].v_lcl_addr.s_addr != 0) break;
    numvifs = vifi;

    splx(s);

    return 0;
}


/* 
 * modifies the outgoing viftable for an s,g entry
 */
static int 
mRouteOifsAdd
    (
    struct mfc* rt, 
    int oif, 
    int ttl
    )
{
	struct mfc* probe;
	if(rt->mfc_root_rt== NULL)
		return (ERROR);
	rt->mfc_root_rt->mfc_ttls[oif] = ttl;
	
	probe = rt->mfc_root_rt;

    while (probe->mfc_next_g != probe->mfc_root_rt)
    {
   		probe = probe->mfc_next_g;
		probe->mfc_ttls[oif] = ttl;
    }

	return (OK);
}
	 
/*
 * Add an mfc entry
 */
static int
add_mfc(mfccp)
    struct mfcctl *mfccp;
{
    struct mfc *rt;
    struct mfc *root_rt;
    u_long hash;
    struct rtdetq *rte;
    u_short nstl;
    int s;
    int i;

	rt = NULL;

    MRTDEBUG
    (
        (mrtdebug & DEBUG_MFC),
        "MROUTE: add_mfc received o %p g %p p %p\n",
        ntohl(mfccp->mfcc_origin.s_addr),
        ntohl(mfccp->mfcc_mcastgrp.s_addr),
		(u_long)mfccp->mfcc_parent,
        4,5,6
    );
    MRTDEBUG
    (
        (mrtdebug & DEBUG_MFC),
        "MROUTE: add_mfc ttls%i%i%i%i%i\n.",
        mfccp->mfcc_ttls[0],
        mfccp->mfcc_ttls[1],
        mfccp->mfcc_ttls[2],
        mfccp->mfcc_ttls[3],
        mfccp->mfcc_ttls[4],
        6
    );

    MFCFIND(mfccp->mfcc_origin.s_addr, mfccp->mfcc_mcastgrp.s_addr, rt);

    /* If an entry already exists, just update the fields */
    if (rt) 
	{
        MRTDEBUG
        (
            (mrtdebug & DEBUG_MFC),
			"MROUTE: add_mfc update o %lx g %lx p %x\n",
            (u_long)ntohl(mfccp->mfcc_origin.s_addr),
            (u_long)ntohl(mfccp->mfcc_mcastgrp.s_addr),
			mfccp->mfcc_parent,
            4,5,6
        );
		s = splnet();
		
		if (mfccp->mfcc_origin.s_addr == 0) 
		{/* updating a 0,g entry updates all s,g entries for that interface*/
			rt->mfc_parent = mfccp->mfcc_parent;
            MRTDEBUG
            (
                (mrtdebug & DEBUG_FORWARD),
				"MROUTE: add_mfc updating all rts %p p %x, val=%x\n",
                ntohl(mfccp->mfcc_mcastgrp.s_addr),
                rt->mfc_parent, 
                viftable[rt->mfc_parent].v_threshold,
                4,5,6
            );

			if (mfccp->mfcc_ttls[rt->mfc_parent] != 0)
				mRouteOifsAdd(rt, 
					rt->mfc_parent, 
					viftable[rt->mfc_parent].v_threshold);
			else
				mRouteOifsAdd(rt, rt->mfc_parent, 0);

		}
		else
		{
			for (i=0; i<numvifs; i++)
			{/* an s,g add/update doesn't affect other entries*/
				rt->mfc_parent = mfccp->mfcc_parent;
			    MRTDEBUG
                (
                    (mrtdebug & DEBUG_FORWARD),
					"MROUTE: add_mfc updating rts o=%p g=%p p %x, val=%x\n",
					ntohl(rt->mfc_origin.s_addr),
					ntohl(rt->mfc_mcastgrp.s_addr),
					rt->mfc_parent, 
                    viftable[rt->mfc_parent].v_threshold,
                    5,6
                );
				rt->mfc_ttls[i] = mfccp->mfcc_ttls[i];
			}
			rt->mfc_notify = mfccp->mfcc_notify;
		}
        MRTDEBUG
        (
            (mrtdebug & DEBUG_MFC),
			"MROUTE: add_mfc updated o %p g%p p%p, root_rt=%p.\n",
			ntohl(mfccp->mfcc_origin.s_addr),
			ntohl(mfccp->mfcc_mcastgrp.s_addr),
			(u_long)rt->mfc_parent, 
			(u_long)rt->mfc_root_rt,
            5,6
        );
		splx(s);
		return 0;
    }


    /* 
     * Find the entry for which the upcall was made and update
     */

    s = splnet();
    hash = MFCHASH(mfccp->mfcc_origin.s_addr, mfccp->mfcc_mcastgrp.s_addr);

    for (rt = mfctable[hash], nstl = 0; rt; rt = rt->mfc_next) 
	{

		if ((rt->mfc_origin.s_addr == mfccp->mfcc_origin.s_addr) &&
			(rt->mfc_mcastgrp.s_addr == mfccp->mfcc_mcastgrp.s_addr) &&
			(rt->mfc_stall != NULL)) 
		{/* looking for entry with packet waiting on it */

			if (nstl++)
			{
			    MRTDEBUG
                (
                    (mrtdebug), 
				    "MROUTE: add_mfc multiple kernel entries \
                    o %p g %p p %x dbx %p\n",
					ntohl(mfccp->mfcc_origin.s_addr),
					ntohl(mfccp->mfcc_mcastgrp.s_addr),
					mfccp->mfcc_parent, 
                    (int)rt->mfc_stall,
                    5,6
                );
			}

            MRTDEBUG
            (
                (mrtdebug & DEBUG_MFC), 
				"MROUTE: add_mfc o %p g %p p %x dbg %p\n",
				ntohl(mfccp->mfcc_origin.s_addr),
				ntohl(mfccp->mfcc_mcastgrp.s_addr),
				mfccp->mfcc_parent, 
                (int)rt->mfc_stall,
                5,6
            );
			/* copy the information into rt entry */
			rt->mfc_origin     = mfccp->mfcc_origin;
			rt->mfc_mcastgrp   = mfccp->mfcc_mcastgrp;
			rt->mfc_notify   = mfccp->mfcc_notify;
			if (! (mfccp->mfcc_parent > numvifs) ) /* can't be less, it's a USHORT value */
				rt->mfc_parent = mfccp->mfcc_parent;
			if (mfccp->mfcc_origin.s_addr == 0)
			{
    		    MRTDEBUG
                (
                    (mrtdebug & DEBUG_FORWARD),
					"MROUTE: add_mfc updating rts o=%p g=%p p %x, val=%x\n",
                    ntohl(rt->mfc_origin.s_addr),
					ntohl(rt->mfc_mcastgrp.s_addr),
					rt->mfc_parent, 
                    viftable[rt->mfc_parent].v_threshold,
                    5,6
                );
				mRouteOifsAdd(rt, rt->mfc_parent, viftable[rt->mfc_parent].v_threshold);
			}/* update all routes for *,g entry add */
			else for (i = 0; i < numvifs; i++)
			{
				if ( (mfccp->mfcc_origin.s_addr != 0)
					&&(mfccp->mfcc_ttls[i] != 0)
					)
				{
                    MRTDEBUG
                    (
                        (mrtdebug & DEBUG_FORWARD),
						"MROUTE: add_mfc changing rt %p,%p vif %i to %i\n",
                        ntohl(rt->mfc_origin.s_addr),
						ntohl(rt->mfc_mcastgrp.s_addr),
						i, 
                        viftable[i].v_threshold,
                        5,6
                    );
					rt->mfc_ttls[i] = viftable[i].v_threshold;
				}/* copy the oif list for s,g entries */
			}
			/* initialize pkt counters per src-grp */
			rt->mfc_pkt_cnt    = 0;
			rt->mfc_byte_cnt   = 0;
			rt->mfc_wrong_if   = 0;
			rt->mfc_last_assert = 0;
            rt->mfc_last_assert = 0;

			rt->mfc_expire = 0;	/* Don't clean this guy up */
			nexpire[hash]--;
			/* free packets Qed at the end of this entry */
			for (rte = rt->mfc_stall; rte != NULL; ) 
			{
				struct rtdetq *n = rte->next;
			    MRTDEBUG
                (
                    (mrtdebug & DEBUG_FORWARD),
					"MROUTE: add_mfc add_mfc calls ip_mdq.\n",
                    1,2,3,4,5,6
                );
				ip_mdq(rte->m, rte->ifp, rt, -1);
                MRTDEBUG
                (
                    (mrtdebug & DEBUG_FORWARD),
					"MROUTE: add_mfc add_mfc done calling ip_mdq.\n",
                    1,2,3,4,5,6
                );
				m_freem(rte->m);
				MRT_FREE((char*) rte);
				rte = n;
			}
			rt->mfc_stall = NULL;
		}/* dQing stall entries */
    }/* for mfctable[hash] loop */

    /*
     * It is possible that an entry is being inserted without an upcall
	 * This would be primarily found in s,g joins from an overlying protcol
	 * e.g. PIM
     */
    if (nstl == 0) 
	{
        MRTDEBUG
        (
            (mrtdebug & DEBUG_MFC),
		    "MROUTE: add_mfc no upcall h %lu o %lx g %lx p %x\n",
            hash, 
            (u_long)ntohl(mfccp->mfcc_origin.s_addr),
			(u_long)ntohl(mfccp->mfcc_mcastgrp.s_addr),
			mfccp->mfcc_parent,
            5,6
        );

		for (rt = mfctable[hash]; rt != NULL; rt = rt->mfc_next) 
		{
			
			if ((rt->mfc_origin.s_addr == mfccp->mfcc_origin.s_addr) &&
			(rt->mfc_mcastgrp.s_addr == mfccp->mfcc_mcastgrp.s_addr)) {

			for (i = 0; i < numvifs; i++)
			{/* if the entry exists, copy the oiflist */
				if (mfccp->mfcc_ttls[i] != 0)
					rt->mfc_ttls[i] = viftable[i].v_threshold;
				else 
					rt->mfc_ttls[i] = 0;
			}
			/* initialize pkt counters per src-grp */
			rt->mfc_pkt_cnt    = 0;
			rt->mfc_byte_cnt   = 0;
			rt->mfc_wrong_if   = 0;
			rt->mfc_last_assert = 0;
            rt->mfc_last_assert = 0;
			if (rt->mfc_expire)
				nexpire[hash]--;
			rt->mfc_expire	   = 0;
			}
		}
		if (rt == NULL) 
		{
			/* no upcall, so make a new entry */
			rt = MRT_ALLOC(sizeof(struct mfc));

			if (rt == NULL) {
			splx(s);
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_EMERGENCY event */
        WV_NET_MARKER_0 (NET_AUX_EVENT, WV_NET_EMERGENCY, 17, 1,
                         WV_NETEVENT_ADDMRT_NOBUFS)
#endif  /* INCLUDE_WVNET */
#endif
			return ENOBUFS;
			}
			rt->mfc_notify = 0;

			rt->mfc_origin     = mfccp->mfcc_origin;
			rt->mfc_mcastgrp   = mfccp->mfcc_mcastgrp;
			/* link into hash chain now so the search for the root_rt will work */
			rt->mfc_next = mfctable[hash];
			mfctable[hash] = rt;
			for (i = 0; i < numvifs; i++)
			{
				if (mfccp->mfcc_ttls[i] != 0)
					rt->mfc_ttls[i] = viftable[i].v_threshold;
				else 
					rt->mfc_ttls[i] = 0;
			}

			/* find the *,g entry and link in */
			MFCFIND(0, mfccp->mfcc_mcastgrp.s_addr, root_rt);
			if (root_rt == NULL)
			{
				root_rt = MRT_ALLOC(sizeof(struct mfc));
				if (root_rt == NULL) 
				{
					MRT_FREE((char*) rt);
					splx(s);
					return ENOBUFS;
				}
				root_rt->mfc_notify = 0;

				/* inset NULL entry at head of hash chain */
				root_rt->mfc_origin.s_addr     = 0;
				root_rt->mfc_mcastgrp   = mfccp->mfcc_mcastgrp;
				root_rt->mfc_parent     = 0;
				for (i = 0; i < numvifs; i++)
				{
					root_rt->mfc_ttls[i] = mfccp->mfcc_ttls[i];
				}
				/* initialize pkt counters per src-grp */
				root_rt->mfc_pkt_cnt    = 0;
				root_rt->mfc_byte_cnt   = 0;
				root_rt->mfc_wrong_if   = 0;
				root_rt->mfc_last_assert = 0;
                rt->mfc_last_assert = 0;
				root_rt->mfc_expire     = 0;
				root_rt->mfc_stall      = NULL;
				/* link *,g into table */
				root_rt->mfc_next = 
					mfctable[MFCHASH(root_rt->mfc_origin.s_addr, root_rt->mfc_mcastgrp.s_addr)];
				mfctable[MFCHASH(root_rt->mfc_origin.s_addr, root_rt->mfc_mcastgrp.s_addr)] = root_rt;
				root_rt->mfc_next_g = root_rt;	/* next group in same chain */
				root_rt->mfc_root_rt = root_rt;	/* 0,g entry in hash table, itself in this case*/
			}
			rt->mfc_root_rt = root_rt;	
			/* insert into front of group chain*/
			rt->mfc_next_g = root_rt->mfc_next_g;
			root_rt->mfc_next_g = rt;

    		if (!(mfccp->mfcc_parent > numvifs) )/* can't be less than 0, it's a USHORT */
				rt->mfc_parent = mfccp->mfcc_parent;

	/* take care of the details before you alter the forwarding state.  */
			rt->mfc_pkt_cnt    = 0;
			rt->mfc_byte_cnt   = 0;
			rt->mfc_wrong_if   = 0;
			rt->mfc_last_assert = 0;
            rt->mfc_last_assert = 0;
			rt->mfc_expire     = 0;
			rt->mfc_stall      = NULL;
		}/* rt == null */
    }/* nstl == 0 */
    splx(s);
    return 0;
}


/* 
 * Resets the passed oif to 0 for a chain
 */
static int 
mRouteOifsRemove
    (
    struct mfc* rt, 
    int oif
    )
{
	struct mfc* probe;
	if (rt == NULL)
		return (EINVAL);
	
	rt->mfc_root_rt->mfc_ttls[oif] = 0;
	
	probe = rt->mfc_root_rt;
    
    while (probe->mfc_next_g != probe->mfc_root_rt)
    {
   		probe = probe->mfc_next_g;
		probe->mfc_ttls[oif] = 0;
    }
	return (OK);

}

/* 
 * Removes from the chain of all g entries
 */
static int
mRouteGroupLoopRemove
    (
    struct mfc* rt, 
    struct mfc* root
    )
{   
	struct mfc* probe;
	probe = root;

    while (probe->mfc_next_g != probe->mfc_root_rt)
    {

   		probe = probe->mfc_next_g ;
		if (probe->mfc_next_g == rt)
			break;

    }

	if (probe->mfc_next_g == rt)
	{
		probe->mfc_next_g = rt->mfc_next_g ;
	}
	return (OK);
}

	 
/*
 * Delete an mfc entry
 */

static int
	del_mfc(mfccp)
    struct mfcctl *mfccp;
{
    struct in_addr 	origin;
    struct in_addr 	mcastgrp;
    struct mfc 		*rt;
    struct mfc 		*root;

	struct mfc	 	**nptr;
    u_long 		hash;
    int s;

    origin = mfccp->mfcc_origin;
    mcastgrp = mfccp->mfcc_mcastgrp;
    hash = MFCHASH(origin.s_addr, mcastgrp.s_addr);

    MRTDEBUG
    (
        (mrtdebug & DEBUG_MFC),
		"MROUTE: del_mfc orig %lx mcastgrp %lx\n",
        (u_long)ntohl(origin.s_addr), 
        (u_long)ntohl(mcastgrp.s_addr),
        3,4,5,6
    );

    s = splnet();

    nptr = &mfctable[hash];
    while ((rt = *nptr) != NULL) 
	{
		if (origin.s_addr == rt->mfc_origin.s_addr &&
			mcastgrp.s_addr == rt->mfc_mcastgrp.s_addr &&
			rt->mfc_stall == NULL)
			break;

		nptr = &rt->mfc_next;
    }
    if (rt == NULL) 
	{
		splx(s);
		return EADDRNOTAVAIL;
    }
	/* if *,g remove, remove all oifs from group chain */
	if (rt->mfc_origin.s_addr == 0)
		mRouteOifsRemove(rt, mfccp->mfcc_parent);
	else
	{

		/* remove entry from group chain */	
		root = rt->mfc_root_rt;
		mRouteGroupLoopRemove(rt, root);

		*nptr = rt->mfc_next;/* removes from hash chain */
		MRT_FREE((char*) rt);/* removes the s,g entry */

		if ( (u_long)root->mfc_next_g == (u_long)root->mfc_root_rt == (u_long)root)
		/* this is the only g entry in the table */
		{
			hash = MFCHASH(0, mcastgrp.s_addr);
			nptr = &mfctable[hash];
			while ((rt = *nptr) != NULL) 
			{
				if (origin.s_addr == rt->mfc_origin.s_addr &&
					mcastgrp.s_addr == rt->mfc_mcastgrp.s_addr &&
					rt->mfc_stall == NULL)
					break;

				nptr = &rt->mfc_next;
			}
			if (root == rt)
			{
				*nptr = rt->mfc_next;/* removes from hash chain */
				MRT_FREE((char*) rt);/* frees up the 0,g entry */
			}
		}
	}/* origin not == NULL */

	splx(s);
    return 0;
}


/******************************************************************************
*
* mRouteGroupAdd - add a multicast group to the given interface
*
* Adds the given group to the list of multicast groups on the given port.
* Thus, multicast packets to that address will go out the port.
*
* RETURNS: OK if successful, ERROR otherwise
*
*/
STATUS mRouteGroupAdd
    (
	struct ifnet* ifp, 
    struct in_addr srcAddr,       /* multicast source to add */
    struct in_addr groupAddr      /* multicast group to add */
    )
    {
	struct mfcctl gCtl;
	int sockfd;
	int port;
    struct mfc *rt;

	rt = NULL;	
	MFCFIND(0, groupAddr.s_addr, rt);
	if (ifp == NULL)
    {
        return (ERROR);
    }

	port = mRouteNameToPort(ifp->if_name, ifp->if_unit);
    if ( (port == ERROR) )
        return (ERROR);
    gCtl.mfcc_parent = port;
                    /* ports are vifs for now */

    gCtl.mfcc_mcastgrp = groupAddr;
	gCtl.mfcc_origin = srcAddr;/* 0 for a star,g entry */

	gCtl.mfcc_notify = 0;   /* mfcc_notify set to 0 so pimNotify never called */

	if (rt != NULL)
	{
		memcpy(gCtl.mfcc_ttls, rt->mfc_ttls, numvifs);
	}
	else 
	{
		bzero (gCtl.mfcc_ttls, 32);
	}
	sockfd = (int)ip_mrouter;
    if (sockfd <= 0)
        return (ERROR);

    MRTDEBUG
    (
        (mrtdebug & DEBUG_MFC),
		"MROUTE: IP_MROUTER calling mfc_add.\n",
        1,2,3,4,5,6
    );

	if (setsockopt (sockfd, IPPROTO_IP, MRT_ADD_MFC,
                   (char *) &gCtl, sizeof(gCtl)) == ERROR)
    {
	    MRTDEBUG
        (
            (mrtdebug),
		    "MROUTE: igmpGroupAdd failed with errno: %d",
            errno,2,3,4,5,6
        );       
        return ERROR;
    }

	return (OK);
}

/******************************************************************************
*
* pimNotify - notifies pim daemon about new packet on an interface
*
* RETURNS: OK for success, ERROR for failure
*
*/
static int pimNotify
    (
    struct ip * pIp, 
    struct mbuf* pMbuf, 
    struct mfc* pRt,
    vifi_t vifi, 
    struct ifnet* pIfnet
    )
{
	    struct sockaddr_in k_igmpsrc;
	    struct mbuf *pMbufCopy;
	    struct igmpmsg *pIGMPMessage;
		struct ip * pIpNew;
	    UINT now;
		int hlen;

        if ( 
              (pIp == NULL)
            ||(pMbuf == NULL)
            ||(pRt == NULL)
            ||(pIfnet == NULL)
           )
        {
            return (ERROR);
        }


		GET_TIME(now);

	    MRTDEBUG
        (
            (mrtdebug),
			"MROUTE: pimNotify calling overlying routing protocol.\n",
            1,2,3,4,5,6
        );		

		pMbufCopy = m_copy(pMbuf, 0, M_COPYALL);
		if (pMbufCopy == NULL) 
		{
			return ENOBUFS;
		}
		hlen = pIp->ip_hl << 2;

		if (pMbufCopy && (M_HASCL(pMbufCopy) || pMbufCopy->m_len < hlen))
			pMbufCopy = m_pullup(pMbufCopy, hlen);

		pRt->mfc_last_assert = now;
/* It looks like they're overlaying the igmpmsg structure on top of the 
ip structure.  This will allow us to access the im_src member 
(which in IP corresponds to the ip_src member of struct ip.  Note that 
both members are offset by 12 bytes.  

struct igmpmsg {
    u_long	    unused1;
    u_long	    unused2;
    u_char	    im_msgtype;
#define IGMPMSG_NOCACHE		1
#define IGMPMSG_WRONGVIF	2
    u_char	    im_mbz;	
    u_char	    im_vif;	
    u_char	    unused3;
    struct in_addr  im_src, im_dst;
};

  
struct ip {
#if _BYTE_ORDER == _LITTLE_ENDIAN
	u_int	ip_hl:4,		
		ip_v:4,			
#endif
#if _BYTE_ORDER == _BIG_ENDIAN
	u_int	ip_v:4,		
		ip_hl:4,		
#endif
		ip_tos:8,		
		ip_len:16;		
	u_short	ip_id;		
	short	ip_off;		
#define	IP_DF 0x4000	
#define	IP_MF 0x2000	
#define	IP_OFFMASK 0x1fff	
	u_char	ip_ttl;			
	u_char	ip_p;			
	u_short	ip_sum;			
	struct	in_addr ip_src,ip_dst;	
};
*/
		pIGMPMessage = mtod(pMbufCopy, struct igmpmsg *);
        if (pIGMPMessage == NULL)
        {
            return (ERROR);
        }
		k_igmpsrc.sin_addr = pIGMPMessage->im_src;

		/* The kernel stores these values in host order, pim requires net order*/
		pIpNew = mtod (pMbufCopy, struct ip*);
        if (pIpNew == NULL)
        {
            return (ERROR);
        }

        pIpNew->ip_len = htons(pIpNew->ip_len);
		pIpNew->ip_off = htons(pIpNew->ip_off);
        /* these values are in the wrong order for PIM*/
    	MRTDEBUG
        (
            (mrtdebug),
			"MROUTE: pimNotify calling pim=%p.\n",
            (int)_pimCacheMissSendHook,2,3,4,5,6
        );		
		if ( _pimCacheMissSendHook != 0)	
		{
			_pimCacheMissSendHook (pMbufCopy, pIfnet, &k_igmpsrc);
		}
		return (OK);
}



/*
 * IP multicast forwarding function. This function assumes that the packet
 * pointed to by "ip" has arrived on (or is about to be sent to) the interface
 * pointed to by "ifp", and the packet is to be relayed to other networks
 * that have members of the packet's destination IP multicast group.
 *
 * The packet is returned unscathed to the caller, unless it is
 * erroneous, in which case a non-zero return value tells the caller to
 * discard it.
 */

#define IP_HDR_LEN  20	/* # bytes of fixed IP header (excluding options) */
#define TUNNEL_LEN  12  /* # bytes of IP option for tunnel encapsulation  */

static int
X_ip_mforward(m, ifp, ip, imo )
    struct mbuf *m;
    struct ifnet *ifp;
    struct ip *ip;
    struct ip_moptions *imo;

{    

    struct mfc *rt;
    struct mfc *root_rt;
    u_char *ipoptions;
    static struct sockaddr_in 	k_igmpsrc	= { sizeof k_igmpsrc, AF_INET };
    static int srctun = 0;
    int s;
	BOOL null_list;
    vifi_t vifi = -1;
    MRTDEBUG
    (
        (mrtdebug & DEBUG_FORWARD),
		"ip_mforward: src %lx, dst %lx, if=%p, %s%i\n",
        (u_long)ntohl(ip->ip_src.s_addr), 
        (u_long)ntohl(ip->ip_dst.s_addr),
		(int)ifp, 
        (int)ifp->if_name, 
        ifp->if_unit,
        6
    );
    if ( (mrtdebug & DEBUG_TABLE) != 0)
        mRouteDumpMfctable();

    if (ip->ip_hl < (IP_HDR_LEN + TUNNEL_LEN) >> 2 ||
	(ipoptions = (u_char *)(ip + 1))[1] != IPOPT_LSRR ) {
	/*
	 * Packet arrived via a physical interface or
	 * an encapsulated tunnel.
	 */
    } else {
    MRTDEBUG
    (
        (mrtdebug & DEBUG_FORWARD),
		"MROUTE: Packet arrived through a source-route tunnel.  \
        Source-route tunnels are no longer supported.\n",
        1,2,3,4,5,6
    );

	/*
	 * Packet arrived through a source-route tunnel.
	 * Source-route tunnels are no longer supported.
	 */
	if ((srctun++ % 1000) == 0)
	{
        MRTDEBUG
        (
            (mrtdebug),
		    "MROUTE: ip_mforward: received source-routed packet from %lx\n",
            (u_long)ntohl(ip->ip_src.s_addr),
            2,3,4,5,6
        );
	}

	return 1;
    }

	if ( (imo) && (vifi < numvifs) ) 
	{ 
		if (ip->ip_ttl < 255)
			ip->ip_ttl++;	/* compensate for -1 in *_send routines */
		vifi = -1;
		return (ip_mdq(m, ifp, NULL, vifi));/* this is for a specific forwarding scenario...  
		explicitly chosen vif.  we no longer support this */
    }
    /*
     * Don't forward a packet with time-to-live of zero or one,
     * or a packet destined to a local-only group.
     */
    if (ip->ip_ttl <= 1 ||
	 (ntohl(ip->ip_dst.s_addr) <= INADDR_MAX_LOCAL_GROUP) )
	{	
		return 0;
	}

    /*
     * Determine forwarding vifs from the forwarding cache table
     */
    s = splnet();
    MFCFIND(ip->ip_src.s_addr, ip->ip_dst.s_addr, rt);
    /* Entry exists, so forward if necessary */
    if (rt != NULL) 
	{
		null_list = oifListIsEmpty(rt);
		splx(s);
		if ( 
            (
			    (null_list)
			 && (pim_assert )
			 && (ifp->if_flags & IFF_BROADCAST) 
			)
			|| 
			(
                rt->mfc_notify != 0
            )
		   )
		{
			rt->mfc_notify = 1;
			pimNotify(ip, m, rt, vifi, ifp);
			/* tell pim about new packet for old group.  
			 * Pim will be responsible for forwarding*/
			if (null_list )
			{
				return (OK);
			}
		}
		return (ip_mdq(m, ifp, rt, -1));
    } 
	else 
	{
	/*
	 * If we don't have a route for packet's origin,
	 * Make a copy of the packet &
	 * send message to routing daemon
	 */

	struct mbuf *mb0;
	struct rtdetq *rte;
	u_long hash;
	int hlen = ip->ip_hl << 2;

    mrtstat.mrts_no_route++;
    MRTDEBUG
    (
        (mrtdebug & (DEBUG_FORWARD | DEBUG_MFC)),
        "MROUTE: ip_mforward: origin unknown no rte s %lx g %lx\n",
		(u_long)ntohl(ip->ip_src.s_addr),
		(u_long)ntohl(ip->ip_dst.s_addr),
        3,4,5,6
    );

	/*
	 * Allocate mbufs early so that we don't do extra work if we are
	 * just going to fail anyway.  Make sure to pullup the header so
	 * that other people can't step on it.
	 */
	rte = MRT_ALLOC (sizeof (struct rtdetq ) );

	if (rte == NULL) {
	    splx(s);

	    return ENOBUFS;
	}
	rte->m = 0;

	mb0 = m_copy(m, 0, M_COPYALL);
	if (mb0 && (M_HASCL(mb0) || mb0->m_len < hlen))
	    mb0 = m_pullup(mb0, hlen);
	if (mb0 == NULL) {
	    MRT_FREE((char*) rte);
	    splx(s);
	    return ENOBUFS;
	}

	/* is there an upcall waiting for this packet? */
	hash = MFCHASH(ip->ip_src.s_addr, ip->ip_dst.s_addr);
	for (rt = mfctable[hash]; rt; rt = rt->mfc_next) 
	{
	    if ((ip->ip_src.s_addr == rt->mfc_origin.s_addr) &&
			(ip->ip_dst.s_addr == rt->mfc_mcastgrp.s_addr) &&
			(rt->mfc_stall != NULL)
			)
		break;
	}

	if (rt == NULL) 
	{
	    int i;
	    /* no upcall, so make a new entry */
		rt = MRT_ALLOC(sizeof(struct mfc));

		if (rt == NULL) 
		{
			MRT_FREE((char*) rte);
			m_freem(mb0);
			splx(s);
			return ENOBUFS;
		}
	    /* Make a copy of the header to send to the user level process */
		rt->mfc_notify = 0;

	    /* 
	     * Send message to routing daemon to install 
	     * a route into the kernel table
	     */
	    k_igmpsrc.sin_addr = ip->ip_src;
	    
	    mrtstat.mrts_upcalls++;

	    rt->mfc_origin.s_addr     = ip->ip_src.s_addr;
	    rt->mfc_mcastgrp.s_addr   = ip->ip_dst.s_addr;
	    rt->mfc_expire	      = UPCALL_EXPIRE;
	    nexpire[hash]++;
		/* find the root route */
		MFCFIND(0, rt->mfc_mcastgrp.s_addr, root_rt);
		if (root_rt != NULL)
		{/* it exists, do nothing */
		}
		else if (root_rt == NULL)
		{/* create one */
			root_rt = MRT_ALLOC(sizeof(struct mfc));
			if (root_rt == NULL) 
			{
				MRT_FREE((char*) rt);
				splx(s);
				return ENOBUFS;
			}
			root_rt->mfc_notify = 0;
			/* inset NULL entry at head of hash chain */
			root_rt->mfc_origin.s_addr     = 0;
			root_rt->mfc_mcastgrp   = rt->mfc_mcastgrp;
			root_rt->mfc_parent     = 0;
			for (i = 0; i < numvifs; i++)
				root_rt->mfc_ttls[i] = 0;
			/* initialize pkt counters per src-grp */
			root_rt->mfc_pkt_cnt    = 0;
			root_rt->mfc_byte_cnt   = 0;
			root_rt->mfc_wrong_if   = 0;
            rt->mfc_last_assert = 0;
			root_rt->mfc_last_assert = 0;
			root_rt->mfc_expire     = 0;
			root_rt->mfc_stall      = NULL;

			/* link into table */
			root_rt->mfc_next = mfctable[MFCHASH(root_rt->mfc_origin.s_addr, root_rt->mfc_mcastgrp.s_addr) ];
			mfctable[MFCHASH(root_rt->mfc_origin.s_addr, root_rt->mfc_mcastgrp.s_addr) ]= root_rt;
			root_rt->mfc_next_g = root_rt;	/* next group in same chain */
			root_rt->mfc_root_rt = root_rt;	/* 0,g entry in hash table, itself in this case*/
		}
	    rt->mfc_parent = mRouteNameToPort(ifp->if_name, ifp->if_unit);
		rt->mfc_root_rt = root_rt;/* set root_rt*/
		rt->mfc_next_g = root_rt->mfc_next_g;	/* insert into from of group chain*/
		for (i = 0; i < numvifs; i++)
			rt->mfc_ttls[i] = root_rt->mfc_ttls[i];
		root_rt->mfc_next_g = rt;


	/* insert new entry at head of hash chain */
	    /* link into table */
	    rt->mfc_next   = mfctable[hash];
	    mfctable[hash] = rt;
	    rt->mfc_stall = rte;
		if (rte->m == 0)
		{
			rte->m 			= mb0;
			rte->ifp 		= ifp;
			rte->next		= NULL;
		}

		if (pim_assert == 0)
		{/* new route automatically inherits *,g entries */
			if (mRouteGroupAdd( ifp, ip->ip_src, ip->ip_dst ) < 0) 
			{
		        MRTDEBUG    
                (
                    (mrtdebug),
					"MROUTE: ip_mforward: ip_mrouter socket queue full, mRouteGroupAdd failed\n",
                    1,2,3,4,5,6
                );
				++mrtstat.mrts_upq_sockfull;
				MRT_FREE((char*) rte);
				m_freem(mb0);
				MRT_FREE((char*) rt);
				splx(s);
				return ENOBUFS;
			}
		}
		else if (pim_assert 
			&& (ifp->if_flags & IFF_BROADCAST) 
			)
		{
			pimNotify(ip, m, rt, vifi, ifp);/* tell pim about new group */
		}

	} else {
	    /* determine if q has overflowed */
	    int npkts = 0;
	    struct rtdetq **p;

	    for (p = &rt->mfc_stall; *p != NULL; p = &(*p)->next)
		npkts++;

	    if (npkts > MAX_UPQ) {
		mrtstat.mrts_upq_ovflw++;
		MRT_FREE((char*) rte);
		m_freem(mb0);
		splx(s);
		return 0;
	    }

	    /* Add this entry to the end of the queue */
	    *p = rte;
	}
	if (rte->m == 0)
	{
	rte->m 			= mb0;
	rte->ifp 		= ifp;
	rte->next		= NULL;
	}
	splx(s);

	return 0;
    }		
}

int ip_mforward (struct mbuf * m, struct ifnet * ifp, struct ip *ip, struct ip_moptions *imo)
{
	return(X_ip_mforward(m, ifp, ip, imo));
};

/*
 * Clean up the cache entry if upcall is not serviced
 */
static void
expire_upcalls(void *unused)
{
    struct rtdetq *rte;
    struct mfc *mfc, **nptr;
    int i;
    int s;

    s = splnet();
    for (i = 0; i < MFCTBLSIZ; i++) 
	{
		if (nexpire[i] == 0)
			continue;
		nptr = &mfctable[i];
		for (mfc = *nptr; mfc != NULL; mfc = *nptr) 
		{
			/*
			 * Skip real cache entries
			 * Make sure it wasn't marked to not expire (shouldn't happen)
			 * If it expires now
			 */
			if (mfc->mfc_stall != NULL &&
				mfc->mfc_expire != 0 &&
			--mfc->mfc_expire == 0) 
			{
				MRTDEBUG
                (
                    (mrtdebug & DEBUG_EXPIRE),
					"MROUTE: expire_upcalls: expiring (%lx %lx)\n",
                    (u_long)ntohl(mfc->mfc_origin.s_addr),
					(u_long)ntohl(mfc->mfc_mcastgrp.s_addr),
                    3,4,5,6
                );
				/*
				 * drop all the packets
				 * free the mbuf with the pkt, if, timing info
				 */
				for (rte = mfc->mfc_stall; rte; ) 
				{
					struct rtdetq *n = rte->next;
					m_freem(rte->m);
					MRT_FREE((char*) rte);
					rte = n;
				}
				++mrtstat.mrts_cache_cleanups;
				nexpire[i]--;

				*nptr = mfc->mfc_next;
				MRT_FREE((char*) mfc);
			} 
			else 
			{
				nptr = &mfc->mfc_next;
			}
		}
    }
    splx(s);
    expire_upcalls_ch = timeout(expire_upcalls, (caddr_t)NULL, EXPIRE_TIMEOUT);
	
}

/*
 * Packet forwarding routine once entry in the cache is made
 */
static int
ip_mdq(m, ifp, rt, xmt_vif)
    struct mbuf *m;
    struct ifnet *ifp;
    struct mfc *rt;
    vifi_t xmt_vif;
{
    struct ip  *ip = mtod(m, struct ip *);
    vifi_t vifi;
	vifi_t vifi_original;
    struct vif *vifp;
	USHORT old_len;
	int port;

    
    int plen = ip->ip_len;
	int forwarded = 0;

/*
 * Macro to send packet on vif.  Since RSVP packets don't get counted on
 * input, they shouldn't get counted on output, so statistics keeping is
 * seperate.
 */
#define MC_SEND(ip,vifp,m) {                             \
                if ((vifp)->v_flags & VIFF_TUNNEL)  	 \
                    encap_send((ip), (vifp), (m));       \
                else                                     \
                    phyint_send((ip), (vifp), (m));      \
}

    /*
     * If xmt_vif is not -1, send on only the requested vif.
     *
     * (since vifi_t is u_short, -1 becomes MAXUSHORT, which > numvifs.)
     */
	port = mRouteNameToPort(ifp->if_name, ifp->if_unit);
    MRTDEBUG
    (
        (mrtdebug),
		"MROUTE:  MDQ found VIF=%i, parent=%i.\n",
        port,
        rt->mfc_parent,
        3,4,5,6
    ); 
 
    if ( (xmt_vif < numvifs) || (rt == NULL) ) {/* rt== NULL makes up for missing xmit vif in x_ip_mforward */
	MRTDEBUG
    (
        (mrtdebug),
		"MROUTE:  MDQ sending packet on VIF, xmt_vif=%i, numvifs=%i.\n", 
        xmt_vif, 
        numvifs,
        3,4,5,6
    );

	MC_SEND(ip, viftable + xmt_vif, m);
	forwarded ++;
	return 1;
    }

    /*
     * Don't forward if it didn't arrive from the parent vif for its origin.
     */
    vifi = rt->mfc_parent;
    if ((vifi >= numvifs) || (viftable[vifi].v_ifp != ifp)) 
	{
	/* came in the wrong interface */
		MRTDEBUG
        (
            (mrtdebug),
			"MROUTE: wrong if: ifp %p vifi %d vififp %p\n",
            (int)ifp, 
            vifi, 
            (int)viftable[vifi].v_ifp,
            4,5,6
        ); 
		++mrtstat.mrts_wrong_if;
		++rt->mfc_wrong_if;
		/*
		 * If we are doing PIM assert processing, and we are forwarding
		 * packets on this interface, and it is a broadcast medium
		 * interface (and not a tunnel), send a message to the routing daemon.
		 */
		if (pim_assert && rt->mfc_ttls[port] &&
			(ifp->if_flags & IFF_BROADCAST) &&
			!(viftable[vifi].v_flags & VIFF_TUNNEL)) 
		{
			pimNotify(ip, m, rt, vifi, ifp);
		}
		return 0;
    }
	vifi_original = vifi;
    /* If I sourced this packet, it counts as output, else it was input. */
    if (ip->ip_src.s_addr == viftable[vifi].v_lcl_addr.s_addr) 
	{
		viftable[vifi].v_pkt_out++;
		viftable[vifi].v_bytes_out += plen;
    } 
	else 
	{
		viftable[vifi].v_pkt_in++;
		viftable[vifi].v_bytes_in += plen;
    }
    rt->mfc_pkt_cnt++;
    rt->mfc_byte_cnt += plen;

    /*
     * For each vif, decide if a copy of the packet should be forwarded.
     * Forward if:
     *		- the ttl exceeds the vif's threshold
     *		- there are group members downstream on interface
     */
    MRTDEBUG
    (
        (mrtdebug),
		"MROUTE:  MDQ iterating through vifs to find oifs.\n",
        1,2,3,4,5,6
    ); 
    for (vifp = viftable, vifi = 0; vifi < numvifs; vifp++, vifi++)
    {
        MRTDEBUG
        (
            (mrtdebug),
			"MROUTE: mdq vif %i has forward threshold %i.  ip->ttl=%i, parent=%i.\n",
            vifi, 
            rt->mfc_ttls[vifi],
            ip->ip_ttl,
            rt->mfc_parent,
            5,6
        ); 

	    if (
		    (rt->mfc_ttls[vifi] > 0) 
		     &&(ip->ip_ttl > rt->mfc_ttls[vifi])
		     &&(vifi != rt->mfc_parent)
		     )
		    {
			    vifp->v_pkt_out++;
			    vifp->v_bytes_out += plen;
			    MRTDEBUG
                (
                    (mrtdebug),
                    "MROUTE:  sending packet out vif %i name %s , ifp=%p.\n",
                    vifi, 
                    (int)vifp->v_ifp->if_name, 
                    (int)ifp,
                    4,5,6
                ); 
			    old_len = ip->ip_len;
			    MC_SEND(ip, vifp, m);
			    if (old_len != ip->ip_len)
			    {
				    HTONS(ip->ip_len);/* the kernel sometimes changes 
									    the byte ordering, 
									    so we have to change it back */
				    HTONS(ip->ip_off);
			    }
                ip->ip_ttl ++;  /* compensate for -1 in send routine */
			    forwarded ++;
		    }
    }
    MRTDEBUG
    (
        (mrtdebug),
		"MROUTE:  MDQ forwarded %i times.\n", forwarded,
        2,3,4,5,6
    ); 

    return 0;
}

/*
 * check if a vif number is legal/ok. This is used by ip_output, to export
 * numvifs there, 
 */
static int
X_legal_vif_num(vif)
    int vif;
{
    if (vif >= 0 && vif < numvifs)
       return(1);
    else
       return(0);
}

int (*legal_vif_num)(int) = X_legal_vif_num;

/*
 * Return the local address used by this vif
 */
static u_long
X_ip_mcast_src(vifi)
    int vifi;
{
    if (vifi >= 0 && vifi < numvifs)
	return viftable[vifi].v_lcl_addr.s_addr;
    else
	return INADDR_ANY;
}

static void
phyint_send(ip, vifp, m)
    struct ip *ip;
    struct vif *vifp;
    struct mbuf *m;
{
    struct mbuf *mb_copy;
    int hlen = ip->ip_hl << 2;

    /*
     * Make a new reference to the packet; make sure that
     * the IP header is actually copied, not just referenced,
     * so that ip_output() only scribbles on the copy.
     */
    mb_copy = m_copy(m, 0, M_COPYALL);
    if (mb_copy && (M_HASCL(mb_copy) || mb_copy->m_len < hlen))
	mb_copy = m_pullup(mb_copy, hlen);
    if (mb_copy == NULL)
	return;

    if (vifp->v_rate_limit == 0)
	tbf_send_packet(vifp, mb_copy);
    else
	tbf_control(vifp, mb_copy, mtod(mb_copy, struct ip *), ip->ip_len);
}

static void
encap_send(ip, vifp, m)
    struct ip *ip;
    struct vif *vifp;
    struct mbuf *m;
{
    struct mbuf *mb_copy;
    struct ip *ip_copy;
    int i, len = ip->ip_len;

    /*
     * copy the old packet & pullup its IP header into the
     * new mbuf so we can modify it.  Try to fill the new
     * mbuf since if we don't the ethernet driver will.
     */
    MGETHDR(mb_copy, M_DONTWAIT, MT_HEADER);
    if (mb_copy == NULL)
	return;
    mb_copy->m_data += max_linkhdr;
    mb_copy->m_len = sizeof(multicast_encap_iphdr);

    if ((mb_copy->m_next = m_copy(m, 0, M_COPYALL)) == NULL) {
	m_freem(mb_copy);
	return;
    }
    i = MHLEN - M_LEADINGSPACE(mb_copy);
    if (i > len)
	i = len;
    mb_copy = m_pullup(mb_copy, i);
    if (mb_copy == NULL)
	return;
    mb_copy->m_pkthdr.len = len + sizeof(multicast_encap_iphdr);

    /*
     * fill in the encapsulating IP header.
     */
    ip_copy = mtod(mb_copy, struct ip *);
    *ip_copy = multicast_encap_iphdr;
#ifndef VIRTUAL_STACK
    ip_copy->ip_id = htons(ip_id++);
#else
    ip_copy->ip_id = htons(_ip_id++);
#endif
    ip_copy->ip_len += len;
    ip_copy->ip_src = vifp->v_lcl_addr;
    ip_copy->ip_dst = vifp->v_rmt_addr;

    /*
     * turn the encapsulated IP header back into a valid one.
     */
    ip = (struct ip *)((caddr_t)ip_copy + sizeof(multicast_encap_iphdr));
    --ip->ip_ttl;
    HTONS(ip->ip_len);
    HTONS(ip->ip_off);
    ip->ip_sum = 0;
    mb_copy->m_data += sizeof(multicast_encap_iphdr);
    ip->ip_sum = in_cksum(mb_copy, ip->ip_hl << 2);
    mb_copy->m_data -= sizeof(multicast_encap_iphdr);

    if (vifp->v_rate_limit == 0)
	tbf_send_packet(vifp, mb_copy);
    else
	tbf_control(vifp, mb_copy, ip, ip_copy->ip_len);
}

/*
 * De-encapsulate a packet and feed it back through ip input (this
 * routine is called whenever IP gets a packet with proto type
 * ENCAP_PROTO and a local destination address).
 */
void
ipip_input(m, iphlen)
	struct mbuf *m;
	int iphlen;
{
    struct ifnet *ifp = m->m_pkthdr.rcvif;
    struct ip *ip = mtod(m, struct ip *);
    int hlen = ip->ip_hl << 2;
    int s;
    struct ifqueue *ifq;
    struct vif *vifp;

    if (!have_encap_tunnel) 
	{
	    rip_input(m);
	    return;
    }
    /*
     * dump the packet if it's not to a multicast destination or if
     * we don't have an encapsulating tunnel with the source.
     * Note:  This code assumes that the remote site IP address
     * uniquely identifies the tunnel (i.e., that this site has
     * at most one tunnel with the remote site).
     */
    if (! IN_MULTICAST(ntohl(((struct ip *)((char *)ip + hlen))->ip_dst.s_addr))) 
	{
		++mrtstat.mrts_bad_tunnel;
		m_freem(m);
		return;
    }
    if (ip->ip_src.s_addr != last_encap_src) 
	{
		struct vif *vife;
		
		vifp = viftable;
		vife = vifp + numvifs;
		last_encap_src = ip->ip_src.s_addr;
		last_encap_vif = 0;
		for ( ; vifp < vife; ++vifp)
			if (vifp->v_rmt_addr.s_addr == ip->ip_src.s_addr) 
			{
				if ((vifp->v_flags & (VIFF_TUNNEL|VIFF_SRCRT))
					== VIFF_TUNNEL)
				last_encap_vif = vifp;
				break;
			}
    }
    if ((vifp = last_encap_vif) == 0) 
	{
		last_encap_src = 0;
		mrtstat.mrts_cant_tunnel++; /*XXX*/
		m_freem(m);
		MRTDEBUG
        (
            (mrtdebug),
		    "MROUTE: ip_mforward: no tunnel with %lx\n",
            (u_long)ntohl(ip->ip_src.s_addr),
            2,3,4,5,6
        );
		return;
    }
    ifp = vifp->v_ifp;

    if (hlen > IP_HDR_LEN)
      ip_stripoptions(m, (struct mbuf *) 0);
    m->m_data += IP_HDR_LEN;
    m->m_len -= IP_HDR_LEN;
    m->m_pkthdr.len -= IP_HDR_LEN;
    m->m_pkthdr.rcvif = ifp;

    ifq = &ipintrq;
    s = splimp();
    if (IF_QFULL(ifq)) 
	{
		IF_DROP(ifq);
		m_freem(m);
    } else 
	{
		IF_ENQUEUE(ifq, m);
		/*
		 * normally we would need a "schednetisr(NETISR_IP)"
		 * here but we were called by ip_input and it is going
		 * to loop back & try to dequeue the packet we just
		 * queued as soon as we return so we avoid the
		 * unnecessary software interrrupt.
		 */
    }
    splx(s);
}


static BOOL oifListIsEmpty
    (
    struct mfc* rt
    )  
{
	int i;
	int j = 0;
	if (rt == NULL) return (TRUE);

    for (i=0;i<numvifs; i++)
	{
		j+=rt->mfc_ttls[i];
	}

    if (j == 0)
		return (TRUE);
	return(FALSE);
}

static void mRouteDumpMfctable()
{
	int i;
	int j;
	struct mfc* rt;
	vifi_t vifi;
    struct vif *vifp;

	MRTDEBUG
    (
        (1),
		"MROUTE: IP_MROUTE dumping mfctable.\n",
        1,2,3,4,5,6
    );
	for (vifp = viftable, vifi = 0; vifi < numvifs; vifp++, vifi++)
	{
		MRTDEBUG
        (
            (1),
			"MROUTE:  vif %i is named %s%i.\n", 
            vifi, 
            (int)vifp->v_ifp->if_name, vifp->v_ifp->if_index,
            4,5,6
        ); 
	}
	for (i=0;i<MFCTBLSIZ; i++)
	{
		if (mfctable[i] != NULL)
		{
			MRTDEBUG
            (
                (1),
                "MROUTE:  mfctable postion %i.\n", i,
                2,3,4,5,6
            );
			for (rt = mfctable[i]; rt != NULL; rt = rt->mfc_next) 
			{
				MRTDEBUG
                (
                    (1),
                    "MROUTE: \tEntry g=%p, s=%p.\n", 
					ntohl(rt->mfc_mcastgrp.s_addr), 
					ntohl(rt->mfc_origin.s_addr),
                    3,4,5,6
                );
				MRTDEBUG
                (   
                    (1),
                    "MROUTE: \t\t OIFLIST=",1,2,3,4,5,6
                );
				for (j=0;j<=numvifs; j++)
					MRTDEBUG
                    (
                        (1),
                        "vif %i ttl=%i,",j,(int)rt->mfc_ttls[j],3,4,5,6
                    );
				MRTDEBUG
                (
                    (1),
                    "MROUTE: \n.",
                    1,2,3,4,5,6
                );
			}
		}
	}
}

/*
 * Token bucket filter module
 */

static void
tbf_control(vifp, m, ip, p_len)
	struct vif *vifp;
	struct mbuf *m;
	struct ip *ip;
	u_long p_len;
{
    struct tbf *t = vifp->v_tbf;

    if (p_len > MAX_BKT_SIZE) {
	/* drop if packet is too large */
	mrtstat.mrts_pkt2large++;
	m_freem(m);
	return;
    }

    tbf_update_tokens(vifp);

    /* if there are enough tokens, 
     * and the queue is empty,
     * send this packet out
     */

    if (t->tbf_q_len == 0) {
	/* queue empty, send packet if enough tokens */
	if (p_len <= t->tbf_n_tok) {
	    t->tbf_n_tok -= p_len;
	    tbf_send_packet(vifp, m);
	} else {
	    /* queue packet and timeout till later */
	    tbf_queue(vifp, m);
		timeout(tbf_reprocess_q, (caddr_t)vifp, TBF_REPROCESS);
	}
    } else if (t->tbf_q_len < t->tbf_max_q_len) {
	/* finite queue length, so queue pkts and process queue */
	tbf_queue(vifp, m);
	tbf_process_q(vifp);
    } else {
	/* queue length too much, try to dq and queue and process */
	if (!tbf_dq_sel(vifp, ip)) {
	    mrtstat.mrts_q_overflow++;
	    m_freem(m);
	    return;
	} else {
	    tbf_queue(vifp, m);
	    tbf_process_q(vifp);
	}
    }
    return;
}

/* 
 * adds a packet to the queue at the interface
 */
static void
tbf_queue(vifp, m) 
	struct vif *vifp;
	struct mbuf *m;
{
    int s = splnet();
    struct tbf *t = vifp->v_tbf;

    if (t->tbf_t == NULL) {
	/* Queue was empty */
	t->tbf_q = m;
    } else {
	/* Insert at tail */
	t->tbf_t->m_act = m;
    }

    /* Set new tail pointer */
    t->tbf_t = m;

#ifdef DIAGNOSTIC
    /* Make sure we didn't get fed a bogus mbuf */
    if (m->m_act)
	panic("tbf_queue: m_act");
#endif
    m->m_act = NULL;

    t->tbf_q_len++;

    splx(s);
}


/* 
 * processes the queue at the interface
 */
static void
tbf_process_q(vifp)
    struct vif *vifp;
{
    struct mbuf *m;
    int len;
    int s = splnet();
    struct tbf *t = vifp->v_tbf;

    /* loop through the queue at the interface and send as many packets
     * as possible
     */
    while (t->tbf_q_len > 0) {
	m = t->tbf_q;

	len = mtod(m, struct ip *)->ip_len;

	/* determine if the packet can be sent */
	if (len <= t->tbf_n_tok) {
	    /* if so,
	     * reduce no of tokens, dequeue the packet,
	     * send the packet.
	     */
	    t->tbf_n_tok -= len;

	    t->tbf_q = m->m_act;
	    if (--t->tbf_q_len == 0)
		t->tbf_t = NULL;

	    m->m_act = NULL;
	    tbf_send_packet(vifp, m);

	} else break;
    }
    splx(s);
}

static void
tbf_reprocess_q(xvifp)
	void *xvifp;
{
    struct vif *vifp = xvifp;
    if (ip_mrouter == 0) 
	return;

    tbf_update_tokens(vifp);

    tbf_process_q(vifp);

    if (vifp->v_tbf->tbf_q_len)
	timeout(tbf_reprocess_q, (caddr_t)vifp, TBF_REPROCESS);
}

/* function that will selectively discard a member of the queue
 * based on the precedence value and the priority
 */
static int
tbf_dq_sel(vifp, ip)
    struct vif *vifp;
    struct ip *ip;
{
    int s = splnet();
    u_int p;
    struct mbuf *m, *last;
    struct mbuf **np;
    struct tbf *t = vifp->v_tbf;

    p = priority(vifp, ip);

    np = &t->tbf_q;
    last = NULL;
    while ((m = *np) != NULL) {
	if (p > priority(vifp, mtod(m, struct ip *))) {
	    *np = m->m_act;
	    /* If we're removing the last packet, fix the tail pointer */
	    if (m == t->tbf_t)
		t->tbf_t = last;
	    m_freem(m);
	    /* it's impossible for the queue to be empty, but
	     * we check anyway. */
	    if (--t->tbf_q_len == 0)
		t->tbf_t = NULL;
	    splx(s);
	    mrtstat.mrts_drop_sel++;
	    return(1);
	}
	np = &m->m_act;
	last = m;
    }
    splx(s);
    return(0);
}

static void
tbf_send_packet(vifp, m)
    struct vif *vifp;
    struct mbuf *m;
{
    struct ip_moptions imo;
    int error;
    static struct route ro;
    int s = splnet();

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    struct ip *ip = mtod(m, struct ip *);
    u_long dstAddr; 	/* Destination group, unavailable if output fails. */
#endif
#endif

    if (vifp->v_flags & VIFF_TUNNEL) {
	/* If tunnel options */
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET

        /*
         * Save the multicast destination address from the original packet,
         * since it will be unavailable if the ip_output() call fails.
         */

        dstAddr = ip->ip_dst.s_addr;
#endif
#endif

	error = ip_output(m, (struct mbuf *)0, &vifp->v_route,
		  IP_FORWARDING, (struct ip_moptions *)0);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
    WV_NET_ADDROUT_MARKER_4 (NET_CORE_EVENT, WV_NET_NOTICE, 12, 10,
                             vifp->v_lcl_addr.s_addr, vifp->v_rmt_addr.s_addr,
                             WV_NETEVENT_TUNNEL_STATUS,
                             error, vifp->v_lcl_addr.s_addr,
                             vifp->v_rmt_addr.s_addr, dstAddr)
#endif  /* INCLUDE_WVNET */
#endif

    } else {
	imo.imo_multicast_ifp  = vifp->v_ifp;
	imo.imo_multicast_ttl  = mtod(m, struct ip *)->ip_ttl - 1;
	imo.imo_multicast_loop = 1;
	/*imo.imo_multicast_vif  = -1;*/

	/*
	 * Re-entrancy should not be a problem here, because
	 * the packets that we send out and are looped back at us
	 * should get rejected because they appear to come from
	 * the loopback interface, thus preventing looping.
	 */
	error = ip_output(m, (struct mbuf *)0, &ro,
			  IP_FORWARDING, &imo);

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
        WV_NET_EVENT_2 (NET_CORE_EVENT, WV_NET_NOTICE, 13, 11,
                        WV_NETEVENT_PHYINT_STATUS, WV_NET_SEND,
                        error, vifp->v_ifp)
#endif  /* INCLUDE_WVNET */
#endif

	MRTDEBUG
    (
        (mrtdebug & DEBUG_XMIT),
        "MROUTE: phyint_send on vif %p err %p\n", 
        (vifp - viftable), 
        error,3,4,5,6
    );
    }
    splx(s);
}

/* determine the current time and then
 * the elapsed time (between the last time and time now)
 * in milliseconds & update the no. of tokens in the bucket
 */
static void
tbf_update_tokens(vifp)
    struct vif *vifp;
{
    UINT tp;
    u_long tm;
    int s = splnet();
    struct tbf *t = vifp->v_tbf;

    GET_TIME(tp);

    TV_DELTA(tp, t->tbf_last_pkt_t, tm);

    /*
     * This formula is actually
     * "time in seconds" * "bytes/second".
     *
     * (tm / 1000000) * (v_rate_limit * 1000 * (1000/1024) / 8)
     *
     * The (1000/1024) was introduced in add_vif to optimize
     * this divide into a shift.
     */
    t->tbf_n_tok += tm * vifp->v_rate_limit / 1024 / 8;
    t->tbf_last_pkt_t = tp;

    if (t->tbf_n_tok > MAX_BKT_SIZE)
	t->tbf_n_tok = MAX_BKT_SIZE;

    splx(s);
}

static int
priority(vifp, ip)
    struct vif *vifp;
    struct ip *ip;
{
    int prio;

    /* temporary hack; may add general packet classifier some day */

    /*
     * The UDP port space is divided up into four priority ranges:
     * [0, 16384)     : unclassified - lowest priority
     * [16384, 32768) : audio - highest priority
     * [32768, 49152) : whiteboard - medium priority
     * [49152, 65536) : video - low priority
     */
    if (ip->ip_p == IPPROTO_UDP) {
	struct udphdr *udp = (struct udphdr *)(((char *)ip) + (ip->ip_hl << 2));
	switch (ntohs(udp->uh_dport) & 0xc000) {
	    case 0x4000:
		prio = 70;
		break;
	    case 0x8000:
		prio = 60;
		break;
	    case 0xc000:
		prio = 55;
		break;
	    default:
		prio = 50;
		break;
	}
	if (tbfdebug > 1)
		MRTDEBUG
        (
            (1),
            "MROUTE: port %x prio%d\n", ntohs(udp->uh_dport), prio,3,4,5,6
        );
    } else {
	    prio = 50;
    }
    return prio;
}

/*
 * End of token bucket filter modifications 
 */




 
 
 /*
 * sooptcopyout copies out of the sockopt into a buffer 
 * it comes from the /kern/uipc_socket.c in the BSD releases
 */
static 
int
sooptcopyout(sopt, buf, len)
	struct	sockopt *sopt;
	void	*buf;
	size_t	len;
{
	int	error;
	size_t	valsize;

	error = 0;
	MRTDEBUG
    (
        (mrtdebug),
		"MROUTE: soopcopyout sopt->sopt_valsize=%i len=%i.\n", 
        (int)sopt->sopt_valsize, 
        (int)len,
        3,4,5,6
    );

	/*
	 * Documented get behavior is that we always return a value,
	 * possibly truncated to fit in the user's buffer.
	 * Traditional behavior is that we always tell the user
	 * precisely how much we copied, rather than something useful
	 * like the total amount we had available for her.
	 * Note that this interface is not idempotent; the entire answer must
	 * generated ahead of time.
	 */
	valsize = min(len, sopt->sopt_valsize);
	sopt->sopt_valsize = valsize;
	if (sopt->sopt_val != 0) {
		if (sopt->sopt_p != 0)
			error = copyout(buf, sopt->sopt_val, valsize);
		else
			memcpy(sopt->sopt_val, buf, valsize);
	}
	return error;
}

/*
 * sooptcopyin copies from a buffer into a sockopt 
 * it comes from the /kern/uipc_socket.c in the BSD releases
 */

static int
sooptcopyin(sopt, buf, len, minlen)
	struct sockopt *sopt;
	void	*buf;
	size_t	len;
	size_t	minlen;
{
	size_t	valsize;
	

	/*
	 * If the user gives us more than we wanted, we ignore it,
	 * but if we don't get the minimum length the caller
	 * wants, we return EINVAL.  On success, sopt->sopt_valsize
	 * is set to however much we actually retrieved.
	 */

	if ((valsize = sopt->sopt_valsize) < minlen){
		return EINVAL;}
	if (valsize > len)
		sopt->sopt_valsize = valsize = len;
	if (sopt->sopt_p != 0)
	{
		valsize = copyin(sopt->sopt_val, buf, valsize);
		return (valsize );
	}
	memcpy(buf, sopt->sopt_val, valsize);
	return 0;
}


/*
 * if_allmulti copies sets multicast characteristics on
 * an interface.  I have left it in for forward compatibility
 * it comes from the net/if.c in the net/3 BSD releases
 */

static int
if_allmulti(ifp, onswitch)
	struct ifnet *ifp;
	int onswitch;
{
	struct ifreq ifr;
	int error = 0;
	int s;
	bzero ((char*)&ifr, IFNAMSIZ);

	s = splimp();
	/* ifp->amcount was removed from the ifnet structure, 
	   but this code was left for future compatibility issues */
	if (onswitch) {
		if (1   /*ifp->if_amcount++ == 0*/) {
			ifp->if_flags |= IFF_ALLMULTI;
			error = ifp->if_ioctl(ifp, SIOCSIFFLAGS, (char*)&ifr);
		}
	} else {
		if (1/*ifp->if_amcount > 1*/) {
			/*ifp->if_amcount--*/;
		} else {
			/*ifp->if_amcount = 0;*/
			ifp->if_flags &= ~IFF_ALLMULTI;
			error = ifp->if_ioctl(ifp, SIOCSIFFLAGS, (char*)&ifr);
		}
	}
	splx(s);

	return (error);
}



/*
 * Adds a timer task to the netJob ring buffer
 */
static void mRouteNetJobAdd(MROUTE_TIMER_STRUCT* p_timer_struct_object)
{

	if ( netJobAdd (  (FUNCPTR)(*p_timer_struct_object->expire_routine) , (int) (p_timer_struct_object->argument), 0, 0,
                           0, 0) == ERROR)
	{
        MRTDEBUG((mrtdebug), "MROUTE failed to execute timer expire.\n", 1,2,3,4,5,6);
		/* log error */
	}

    wdDelete(p_timer_struct_object->watchdog_id);
    MRT_FREE((char*) p_timer_struct_object);
	return;
}

/*
 * Sets the timeout of an routine, calls mRouteNetJobAdd
 */

static MROUTE_TIMER_STRUCT* mRouteTimeout(VOIDFUNCPTR expire_routine, void* argument, int number_of_ticks)
{
	
	WDOG_ID timer_id;
	MROUTE_TIMER_STRUCT* p_timer_struct_object;

	p_timer_struct_object = MRT_ALLOC(sizeof (MROUTE_TIMER_STRUCT));
	timer_id = wdCreate(); 

	p_timer_struct_object->argument = argument;
	p_timer_struct_object->expire_routine = expire_routine;

	if (timer_id == NULL)
	{
		return (NULL);
	}

	p_timer_struct_object->watchdog_id = timer_id;

	if (wdStart(timer_id, number_of_ticks, (FUNCPTR)(*mRouteNetJobAdd), (UINT)p_timer_struct_object) == ERROR)
	{
		/* log_error */
		wdDelete(timer_id);
		MRT_FREE((char*) p_timer_struct_object);
		return (NULL);
	}
	
	return (p_timer_struct_object);

}

/*
 * Unsets the timeout of an routine, calls mRouteNetJobAdd
 */
static void mRouteUntimeOut(MROUTE_TIMER_STRUCT*  p_timer_struct)

{
    if (p_timer_struct == NULL)
        return;
	
	if (wdCancel(p_timer_struct->watchdog_id) == ERROR)
	{
		MRTDEBUG
        (   
            (mrtdebug),
			"MROUTE:  untimeout failed to cancel timer!\n",
            1,2,3,4,5,6
        );
	}
	if (wdDelete(p_timer_struct->watchdog_id) == ERROR)
	{
		MRTDEBUG
        (
            (mrtdebug),
			"MROUTE:  untimeout failed to delete watchdog id!\n",
            1,2,3,4,5,6
        );
	}

    MRT_FREE((char*) p_timer_struct);
	return;
}



/* ppp_vxworks.c - System-dependent procedures for setting up PPP interfaces */

/* Copyright 1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
modification history
--------------------
01i,30jan01,ijm  Fixed spr 34274, OPT_PROXYARP is not setting ARP entry
		 correctly.  It times out.
01h,17feb99,sgv  Fixed spr 22486, Proxy Arp fix.
01g,05aug96,vin  upgraded to BSD4.4., replaced rtentry with ortentry,
		 reworked get_ether_addr since ifreq struct changed.
		 switched the order of initialization in sifaddr(). 
01f,16jun95,dzb  header file consolidation.
01e,15may95,dzb  changed LOG_ERR for route delete to LOG_WARNING.
01d,06mar95,dzb  proxyArp fix (SPR #4074).
01c,07feb95,dzb  changed to look for OK from pppwrite().
01b,16jan95,dzb  renamed to ppp_vxworks.c. warnings cleanup.
01a,21dec94,dab  VxWorks port - first WRS version.
	   +dzb  added: path for ppp header files, WRS copyright.
*/

#include "vxWorks.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "ioctl.h"
#include "ioLib.h"
#include "sys/ioctl.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "sys/times.h"
#include "net/if.h"
#include "net/if_arp.h"
#include "netinet/if_ether.h"
#include "net/if_dl.h"
#include "errno.h"
#include "net/if.h"
#include "net/route.h"
#include "netinet/in.h"
#include "pppLib.h"
#include "logLib.h"

IMPORT struct ifnet     *ifnet;         /* list of all network interfaces */
IMPORT void arptfree (struct llinfo_arp *);
 
int pppArpCmd (int cmd, struct arpreq * ar);



/*
 * establish_ppp - Turn the serial port into a ppp interface.
 */
void
establish_ppp()
{
    int x;

    /* get ppp fd */
    if (ppptioctl(ppp_unit, PPPIOCGFD, (caddr_t) &(ppp_if[ppp_unit]->fd)) < 0) {
        syslog(LOG_ERR, "ppptioctl(PPPIOCGFD) error");
        die(ppp_unit, 1);
    }

    /*
     * Enable debug in the driver if requested.
     */
    if (ppp_if[ppp_unit]->kdebugflag) {
        if (ppptioctl(ppp_unit, PPPIOCGFLAGS, (caddr_t) &x) < 0) {
            syslog(LOG_WARNING, "ioctl (PPPIOCGFLAGS): error");
        } else {
            x |= (ppp_if[ppp_unit]->kdebugflag & 0xFF) * SC_DEBUG;
            if (ppptioctl(ppp_unit, PPPIOCSFLAGS, (caddr_t) &x) < 0)
                syslog(LOG_WARNING, "ioctl(PPPIOCSFLAGS): error");
        }
    }
}


/*
 * disestablish_ppp - Restore the serial port to normal operation.
 * This shouldn't call die() because it's called from die().
 */
void
disestablish_ppp()
{
}


/*
 * output - Output PPP packet.
 */
void
output(unit, p, len)
    int unit;
    u_char *p;
    int len;
{
    if (ppp_if[unit]->debug)
        log_packet(p, len, "sent ");

    if (pppwrite(unit, (char *) p, len) != OK) {
	syslog(LOG_ERR, "write error");
	die(unit, 1);
    }
}


/*
 * read_packet - get a PPP packet from the serial device.
 */
int
read_packet(buf)
    u_char *buf;
{
    int len;

    len = pppread(ppp_unit, (char *) buf, MTU + DLLHEADERLEN);

    if (len == 0) {
        MAINDEBUG((LOG_DEBUG, "read(fd): EWOULDBLOCK"));
	return -1;
    }
    return len;
}


/*
 * ppp_send_config - configure the transmit characteristics of
 * the ppp interface.
 */
void
ppp_send_config(unit, mtu, asyncmap, pcomp, accomp)
    int unit, mtu;
    u_long asyncmap;
    int pcomp, accomp;
{
    u_int x;

    if (ppptioctl(unit, SIOCSIFMTU, (caddr_t) &mtu) < 0) {
	syslog(LOG_ERR, "ioctl(SIOCSIFMTU) error");
	die(unit, 1);
    }

    if (ppptioctl(unit, PPPIOCSASYNCMAP, (caddr_t) &asyncmap) < 0) {
	syslog(LOG_ERR, "ioctl(PPPIOCSASYNCMAP) error");
	die(unit, 1);
    }

    if (ppptioctl(unit, PPPIOCGFLAGS, (caddr_t) &x) < 0) {
	syslog(LOG_ERR, "ioctl (PPPIOCGFLAGS) error");
	die(unit, 1);
    }
    x = pcomp? x | SC_COMP_PROT: x &~ SC_COMP_PROT;
    x = accomp? x | SC_COMP_AC: x &~ SC_COMP_AC;
    if (ppptioctl(unit, PPPIOCSFLAGS, (caddr_t) &x) < 0) {
	syslog(LOG_ERR, "ioctl(PPPIOCSFLAGS) error");
	die(unit, 1);
    }
}


/*
 * ppp_set_xaccm - set the extended transmit ACCM for the interface.
 */
void
ppp_set_xaccm(unit, accm)
    int unit;
    ext_accm accm;
{
    if (ppptioctl(unit, PPPIOCSXASYNCMAP, (caddr_t) accm) < 0)
        syslog(LOG_WARNING, "ioctl(set extended ACCM): error");
}


/*
 * ppp_recv_config - configure the receive-side characteristics of
 * the ppp interface.
 */
void
ppp_recv_config(unit, mru, asyncmap, pcomp, accomp)
    int unit, mru;
    u_long asyncmap;
    int pcomp, accomp;
{
    int x;

    if (ppptioctl(unit, PPPIOCSMRU, (caddr_t) &mru) < 0) {
        syslog(LOG_ERR, "ioctl(PPPIOCSMRU): error");
	die(unit, 1);
    }
    if (ppptioctl(unit, PPPIOCSRASYNCMAP, (caddr_t) &asyncmap) < 0) {
        syslog(LOG_ERR, "ioctl(PPPIOCSRASYNCMAP): error");
	die(unit, 1);
    }
    if (ppptioctl(unit, PPPIOCGFLAGS, (caddr_t) &x) < 0) {
        syslog(LOG_ERR, "ioctl (PPPIOCGFLAGS): error");
	die(unit, 1);
    }
    x = !accomp? x | SC_REJ_COMP_AC: x &~ SC_REJ_COMP_AC;
    if (ppptioctl(unit, PPPIOCSFLAGS, (caddr_t) &x) < 0) {
        syslog(LOG_ERR, "ioctl(PPPIOCSFLAGS): error");
	die(unit, 1);
    }
}

/*
 * sifvjcomp - config tcp header compression
 */
int
sifvjcomp(u, vjcomp, cidcomp, maxcid)
    int u, vjcomp, cidcomp, maxcid;
{
    u_int x;

    if (ppptioctl(u, PPPIOCGFLAGS, (caddr_t) &x) < 0) {
	syslog(LOG_ERR, "ioctl (PPPIOCGFLAGS) error");
	return 0;
    }
    x = vjcomp ? x | SC_COMP_TCP: x &~ SC_COMP_TCP;
    x = cidcomp? x & ~SC_NO_TCP_CCID: x | SC_NO_TCP_CCID;
    if (ppptioctl(u, PPPIOCSFLAGS, (caddr_t) &x) < 0) {
	syslog(LOG_ERR, "ioctl(PPPIOCSFLAGS) error");
	return 0;
    }
    if (ppptioctl(u, PPPIOCSMAXCID, (caddr_t) &maxcid) < 0) {
        syslog(LOG_ERR, "ioctl(PPPIOCSFLAGS): error");
        return 0;
    }
    return 1;
}

/*
 * sifup - Config the interface up and enable IP packets to pass.
 */
int
sifup(u)
    int u;
{
    struct ifreq ifr;
    u_int x;

    strncpy(ifr.ifr_name, ppp_if[u]->ifname, sizeof (ifr.ifr_name));
    if (ioctl(ppp_if[u]->s, SIOCGIFFLAGS, (int) &ifr) < 0) {
	syslog(LOG_ERR, "ioctl (SIOCGIFFLAGS) error");
	return 0;
    }
    ifr.ifr_flags |= IFF_UP;
    if (ioctl(ppp_if[u]->s, SIOCSIFFLAGS, (int) &ifr) < 0) {
	syslog(LOG_ERR, "ioctl(SIOCSIFFLAGS) error");
	return 0;
    }
    if (ppptioctl(u, PPPIOCGFLAGS, (caddr_t) &x) < 0) {
        syslog(LOG_ERR, "ioctl (PPPIOCGFLAGS): error");
        return 0;
    }
    x |= SC_ENABLE_IP;
    if (ppptioctl(u, PPPIOCSFLAGS, (caddr_t) &x) < 0) {
        syslog(LOG_ERR, "ioctl(PPPIOCSFLAGS): error");
        return 0;
    }
    return 1;
}

/*
 * sifdown - Config the interface down and disable IP.
 */
int
sifdown(u)
    int u;
{
    struct ifreq ifr;
    u_int x;
    int rv;

    rv = 1;
    if (ppptioctl(u, PPPIOCGFLAGS, (caddr_t) &x) < 0) {
        syslog(LOG_ERR, "ioctl (PPPIOCGFLAGS): error");
        rv = 0;
    } else {
        x &= ~SC_ENABLE_IP;
        if (ppptioctl(u, PPPIOCSFLAGS, (caddr_t) &x) < 0) {
            syslog(LOG_ERR, "ioctl(PPPIOCSFLAGS): error");
            rv = 0;
        }
    }
    strncpy(ifr.ifr_name, ppp_if[u]->ifname, sizeof (ifr.ifr_name));
    if (ioctl(ppp_if[u]->s, SIOCGIFFLAGS, (int) &ifr) < 0) {
	syslog(LOG_ERR, "ioctl (SIOCGIFFLAGS) error");
	rv = 0;
    } else {
        ifr.ifr_flags &= ~IFF_UP;
        if (ioctl(ppp_if[u]->s, SIOCSIFFLAGS, (int) &ifr) < 0) {
	    syslog(LOG_ERR, "ioctl(SIOCSIFFLAGS): error");
	    rv = 0;
        }
    }
    return rv;
}

/*
 * SET_SA_FAMILY - set the sa_family field of a struct sockaddr,
 * if it exists.
 */
#define SET_SA_FAMILY(addr, family)		\
    BZERO((char *) &(addr), sizeof(addr));	\
    addr.sa_family = (family);			\
	

/*
 * sifaddr - Config the interface IP addresses and netmask.
 */
int
sifaddr(u, o, h, m)
    int u, o, h, m;
{
    struct ifreq ifr;

    strncpy(ifr.ifr_name, ppp_if[u]->ifname, sizeof(ifr.ifr_name));
    SET_SA_FAMILY(ifr.ifr_addr, AF_INET);
    ifr.ifr_addr.sa_len = sizeof (struct sockaddr_in);
    if (m != 0) {
        ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr = m;
        syslog(LOG_NOTICE, "Setting interface mask to %s", ip_ntoa(m));
        if (ioctl(ppp_if[u]->s, SIOCSIFNETMASK, (int) &ifr) < 0) {
	    syslog(LOG_ERR, "ioctl(SIOCSIFNETMASK): error");
	    return (0); 
	}
    }
    SET_SA_FAMILY(ifr.ifr_dstaddr, AF_INET);
    ifr.ifr_dstaddr.sa_len = sizeof (struct sockaddr_in);
    ((struct sockaddr_in *) &ifr.ifr_dstaddr)->sin_addr.s_addr = h;
    if (ioctl(ppp_if[u]->s, SIOCSIFDSTADDR, (int) &ifr) < 0) {
        syslog(LOG_ERR, "ioctl(SIOCSIFDSTADDR): error");
	return (0); 
    }
    SET_SA_FAMILY(ifr.ifr_addr, AF_INET);
    ifr.ifr_addr.sa_len = sizeof (struct sockaddr_in);
    ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr = o;
    if (ioctl(ppp_if[u]->s, SIOCSIFADDR, (int) &ifr) < 0) {
        syslog(LOG_ERR, "ioctl(SIOCSIFADDR): error");
	return (0); 
    }	
    return 1;
}

/*
 * cifaddr - Clear the interface IP addresses, and delete routes
 * through the interface if possible.
 */
int
cifaddr(u, o, h)
    int u, o, h;
{
    struct ortentry rt;
	    
    SET_SA_FAMILY(rt.rt_dst, AF_INET);
    rt.rt_dst.sa_len = sizeof (struct sockaddr_in);
    ((struct sockaddr_in *) &rt.rt_dst)->sin_addr.s_addr = h;
    SET_SA_FAMILY(rt.rt_gateway, AF_INET);
    rt.rt_gateway.sa_len = sizeof (struct sockaddr_in);
    ((struct sockaddr_in *) &rt.rt_gateway)->sin_addr.s_addr = o;
    rt.rt_flags = RTF_HOST;
    if (ioctl(ppp_if[u]->s, SIOCDELRT, (int) &rt) < 0) {
        syslog(LOG_WARNING, "ioctl(SIOCDELRT) error");
        return 0;
    }
    return 1;
}

/*
 * sifdefaultroute - assign a default route through the address given.
 */
int
sifdefaultroute(u, g)
    int u, g;
{
    struct ortentry rt;

    SET_SA_FAMILY(rt.rt_dst, AF_INET);
    rt.rt_dst.sa_len = sizeof (struct sockaddr_in);
    SET_SA_FAMILY(rt.rt_gateway, AF_INET);
    rt.rt_gateway.sa_len = sizeof (struct sockaddr_in);
    ((struct sockaddr_in *) &rt.rt_gateway)->sin_addr.s_addr = g;
    rt.rt_flags = RTF_GATEWAY;
    if (ioctl(ppp_if[u]->s, SIOCADDRT, (int) &rt) < 0) {
        syslog(LOG_ERR, "default route ioctl(SIOCADDRT): error");
        return 0;
    }
    return 1;
}

/*
 * cifdefaultroute - delete a default route through the address given.
 */
int
cifdefaultroute(u, g)
    int u, g;
{
    struct ortentry rt;

    SET_SA_FAMILY(rt.rt_dst, AF_INET);
    rt.rt_dst.sa_len = sizeof (struct sockaddr_in);
    SET_SA_FAMILY(rt.rt_gateway, AF_INET);
    rt.rt_gateway.sa_len = sizeof (struct sockaddr_in);
    ((struct sockaddr_in *) &rt.rt_gateway)->sin_addr.s_addr = g;
    rt.rt_flags = RTF_GATEWAY;
    if (ioctl(ppp_if[u]->s, SIOCDELRT, (int) &rt) < 0)
        syslog(LOG_WARNING, "default route ioctl(SIOCDELRT): error");

    return 1;
}

/*
 * sifproxyarp - Make a proxy ARP entry for the peer.
 */
int
sifproxyarp(unit, hisaddr)
    int unit;
    u_long hisaddr;
{
    struct arpreq arpreq;

    BZERO((char *)&arpreq, sizeof(arpreq));

    /*
     * Get the hardware address of an interface on the same subnet
     * as our local address.
     */
    if (!get_ether_addr(hisaddr, &arpreq.arp_ha)) {
	syslog(LOG_ERR, "Cannot determine ethernet address for proxy ARP");
	return 0;
    }

    SET_SA_FAMILY(arpreq.arp_pa, AF_INET);
    arpreq.arp_pa.sa_len = sizeof (struct sockaddr_in);
   ((struct sockaddr_in *) &arpreq.arp_pa)->sin_addr.s_addr = hisaddr;
    arpreq.arp_flags = ATF_PERM | ATF_PUBL;
    pppArpCmd (SIOCSARP, &arpreq);

    return 1;
}

int pppArpCmd
   (
   int cmd, 
   struct arpreq * ar
   )
   {
   struct sockaddr_in *       soInAddr;
   struct llinfo_arp *        la = NULL;
   struct rtentry *           rt = NULL;
   struct sockaddr_dl *       sdl= NULL;


   if (ar->arp_pa.sa_family != AF_INET ||
       ar->arp_ha.sa_family != AF_UNSPEC)
       return (EAFNOSUPPORT);

   soInAddr = (struct sockaddr_in *)&ar->arp_pa;

   switch (cmd)
        {
        case SIOCSARP:
            {
	    register struct rtentry *rt;
            static struct sockaddr_inarp sin = {sizeof(sin), AF_INET };
	    int proxy = 1;
	    int create = 1;

#define rt_expire rt_rmx.rmx_expire
#define SDL(s) ((struct sockaddr_dl *)s)

            sin.sin_addr.s_addr = soInAddr->sin_addr.s_addr;
            sin.sin_other = proxy ? SIN_PROXY : 0;
            rt = rtalloc1((struct sockaddr *)&sin, create);
            if (rt == 0)
                return (0);
            rt->rt_refcnt--;
	    rt->rt_expire = 0;
            if ((rt->rt_flags & RTF_GATEWAY) || 
                (rt->rt_flags & RTF_LLINFO) == 0 ||
                 rt->rt_gateway->sa_family != AF_LINK) {
                 if (create)
                        logMsg("arptnew failed on %x\n", 
			(int)ntohl(soInAddr->sin_addr.s_addr),
                        0,0,0,0,0);
                 return (0);
            }
            la = (struct llinfo_arp *)rt->rt_llinfo;

            }

            rt = la->la_rt;
            sdl = SDL(rt->rt_gateway);
            bcopy((caddr_t)ar->arp_ha.sa_data, LLADDR(sdl),
                  sdl->sdl_alen = sizeof(struct ether_addr));

            rt->rt_flags &= ~RTF_REJECT;
            rt->rt_flags |= RTF_STATIC; /* manually added route. */
            la->la_asked = 0;
            break;

        case SIOCDARP:          /* delete entry */
            {
	    register struct rtentry *rt;
            static struct sockaddr_inarp sin = {sizeof(sin), AF_INET };
	    int proxy = 1;
	    int create = 0;
            sin.sin_addr.s_addr = soInAddr->sin_addr.s_addr;
            sin.sin_other = proxy ? SIN_PROXY : 0;
            rt = rtalloc1((struct sockaddr *)&sin, create);
            if (rt == 0)
                return (0);
            rt->rt_refcnt--;
            if ((rt->rt_flags & RTF_GATEWAY) || 
                (rt->rt_flags & RTF_LLINFO) == 0 ||
                 rt->rt_gateway->sa_family != AF_LINK) {
                 if (create)
                        logMsg("arptnew failed on %x\n", 
			(int)ntohl(soInAddr->sin_addr.s_addr),
                        0,0,0,0,0);
                 return (0);
            }
            la = (struct llinfo_arp *)rt->rt_llinfo;
            arptfree(la);
	    }
            break;
        }
	return (0);
   }

/*
 * cifproxyarp - Delete the proxy ARP entry for the peer.
 */
int
cifproxyarp(unit, hisaddr)
    int unit;
    u_long hisaddr;
{
    struct arpreq arpreq;

    BZERO((char *)&arpreq, sizeof(arpreq));
    SET_SA_FAMILY(arpreq.arp_pa, AF_INET);
    arpreq.arp_pa.sa_len = sizeof (struct sockaddr_in);
    ((struct sockaddr_in *) &arpreq.arp_pa)->sin_addr.s_addr = hisaddr;
    pppArpCmd (SIOCDARP, &arpreq);

    return 1;
}

/*
 * get_ether_addr - get the hardware address of an interface on the
 * the same subnet as ipaddr.
 */
#define MAX_IFS		32

int
get_ether_addr(ipaddr, hwaddr)
    u_long ipaddr;
    struct sockaddr *hwaddr;
{
    struct ifnet *ifp;
    struct ifreq *ifr, *ifend;
    u_long ina, mask;
    struct ifreq ifreq;
    struct ifconf ifc;
    struct ifreq ifs[MAX_IFS];

    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;
    if (ioctl(ppp_if[ppp_unit]->s, SIOCGIFCONF, (int) &ifc) < 0) {
	syslog(LOG_ERR, "ioctl(SIOCGIFCONF) error");
	return 0;
    }

    /*
     * Scan through looking for an interface with an Internet
     * address on the same subnet as `ipaddr'.
     */
    ifend = (struct ifreq *) (ifc.ifc_buf + ifc.ifc_len);
    for (ifr = ifc.ifc_req; ifr < ifend; ifr = (struct ifreq *)((char *)ifr +
						sizeof(ifr->ifr_name) + 
						ifr->ifr_addr.sa_len)) 
	{
	if (ifr->ifr_addr.sa_family == AF_INET) {
	    ina = ((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr.s_addr;
	    strncpy(ifreq.ifr_name, ifr->ifr_name, sizeof(ifreq.ifr_name));
	    /*
	     * Check that the interface is up, and not point-to-point
	     * or loopback.
	     */
	    if (ioctl(ppp_if[ppp_unit]->s, SIOCGIFFLAGS, (int) &ifreq) < 0)
		continue;
	    if ((ifreq.ifr_flags &
		 (IFF_UP|IFF_BROADCAST|IFF_POINTOPOINT|IFF_LOOPBACK|IFF_NOARP))
		 != (IFF_UP|IFF_BROADCAST))
		continue;
	    /*
	     * Get its netmask and check that it's on the right subnet.
	     */
	    if (ioctl(ppp_if[ppp_unit]->s, SIOCGIFNETMASK, (int) &ifreq) < 0)
		continue;
            mask = ((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr.s_addr;
	    if ((ipaddr & mask) != (ina & mask))
		continue;

	    break;
	}
    }

    if (ifr >= ifend)
	return 0;

    if ((ifp = ifunit (ifr->ifr_name)) == NULL)
	return 0;

    syslog(LOG_INFO, "found interface %s for proxy arp", ifr->ifr_name);

    /* stuff the ethernet hw address for the arp add call */

    bcopy ((caddr_t) ((struct arpcom *)ifp)->ac_enaddr,
	(caddr_t) hwaddr->sa_data, sizeof(((struct arpcom *)ifp)->ac_enaddr));
    hwaddr->sa_family = AF_UNSPEC;
    return 1;
}

/*
 * ppp_available - check whether the system has any ppp interfaces
 * (in fact we check whether we can do an ioctl on ppp0).
 */
int
ppp_available()
{
#ifdef	notyet
    int s, ok;
    struct ifreq ifr;

    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return 1;               /* can't tell - maybe we're not root */

    strncpy(ifr.ifr_name, "ppp0", sizeof (ifr.ifr_name));
    ok = ioctl(s, SIOCGIFFLAGS, (int) &ifr) >= 0;
    close(s);

    return ok;
#else	/* notyet */
    return 1;
#endif	/* notyet */
}

#ifdef	__STDC__
#include <stdarg.h>

void
syslog(int level, char *fmt, ...)
{
    va_list pvar;
    char buf[256];

    va_start(pvar, fmt);
    vsprintf(buf, fmt, pvar);
    va_end(pvar);

    printf("ppp%d: %s\r\n", ppp_unit, buf);
}

#else	/* __STDC__ */
#include <varargs.h>

void
syslog(level, fmt, va_alist)
int level;
char *fmt;
va_dcl
{
    va_list pvar;
    char buf[256];

    va_start(pvar);
    vprintf(fmt, pvar);
    va_end(pvar);

    printf("ppp%d: %s\r\n", ppp_unit, buf);
}
#endif	/* __STDC__ */

char *stringdup(in)
char *in;
{
    char* dup;

    if ((dup = (char *)malloc(strlen(in) + 1)) == NULL)
	return NULL;
    (void) strcpy(dup, in);
    return (dup);
}

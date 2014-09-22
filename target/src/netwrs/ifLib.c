/* ifLib.c - network interface library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03h,10may02,kbw  making man page edits
03g,09may02,wsl  fix doc formatting error
03f,08jan02,vvv  fixed ifRouteDelete to use correct netmask (SPR #33381)
03e,07dec01,rae  merge from synth ver 03r (SPR #70269)
03d,05nov01,vvv  fixed compilation warning
03c,15oct01,rae  merge from truestack ver 03p, base 03b (ROUTER_STACK)
03b,12mar99,c_c  Doc: updated ifAddrGet, ifFlagSet() and ifDeleRoute 
		(SPRs #21401, 9146 and 21076).
03a,06oct98,ham  changed API for ifAddrAdd() (SPR22267)
02a,19mar98,spm  fixed byte ordering in ifRouteDelete() routine; changed to 
                 use mRouteDelete() to support classless routing (SPR #20548)
01z,16apr97,vin  changed SOCK_DGRAM to SOCK_RAW as udp can be scaled out.
		 fixed ifRouteDelete().
01y,16oct96,dgp  doc: add explanations to ifFlagsSet per SPR 7337
01x,11jul94,dzb  Fixed setting of errno in ifIoctlCall() (SPR #3188).
                 Fixed ntohl transfer in ifMaskGet() (SPR #3334).
01w,19oct92,jmm  added ntohl () to ifMaskGet() to make it symetrical w/ifMaskSet
01v,02sep93,elh  changed ifunit back to berkeley orig to fix spr (1516).
01u,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01t,05feb93,jag  Changed call to inet_ntoa to inet_ntoa_b in ifInetAddrToStr,
		 changed ifRouteDelete accordingly SPR# 1814
01s,20jan93,jdi  documentation cleanup for 5.1.
01r,18jul92,smb  Changed errno.h to errnoLib.h.
01q,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01p,10dec91,gae  added includes for ANSI.
01o,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed copyright notice
01n,01may91,elh	 added htonl to ifMaskSet because network wants mask
                 to be in network byte order.
01m,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
	   +elh	 added ifunit() from if.c (elh); doc review by dnw.
01l,18jan91,jaa	 documentation.
01k,26jun90,hjb  moved ifShow, etc. to netShow.c.
01j,18mar90,hjb  de-linted.  minor syntax cleanup.
01i,14mar90,jdi  documentation cleanup.
01h,07aug89,gae  undid 01e -- fixed if_bp.c.
01g,03aug89,hjb  fixed ifShow to work with "bp" interfaces which
                 use incompatible "ifp->if_name" convention (unit number is
                 for some reason appended to the if_name itself?).
01f,30jul89,gae  removed varargs stuff; internal name changes.
		 documentation tweaks.  lint.
01e,28jul89,hjb  added routines ifAddrGet, ifBroadcastGet, ifDstAddrGet,
                 ifAddrParamGet, ifMaskGet, ifFlagChange, ifFlagGet,
		 ifMetricSet, ifMetricGet, ifIoctl, ifSetIoctl, ifGetIoctl,
		 ifIoctlCall, and modified ifEtherPrint.
01d,27jul89,hjb  added routines ifShow, ifFlagSet, ifDstAddrSet, ifAddrPrint,
		 ifFlagPrint, ifEtherPrint, ifRouteDelete.
01c,18aug88,gae  documentation.
01b,30may88,dnw  changed to v4 names.
01a,28may88,dnw  extracted from netLib.
*/

/*
DESCRIPTION
This library contains routines to configure the network interface parameters.
Generally, each routine corresponds to one of the functions of the UNIX
command \f3ifconfig\fP.

To use this feature, include the following component:
INCLUDE_NETWRS_IFLIB

INCLUDE FILES: ifLib.h

SEE ALSO: hostLib
*/

#include "vxWorks.h"
#include "stdio.h"
#include "netinet/in.h"
#include "net/if.h"
#include "netinet/if_ether.h"
#include "sys/ioctl.h"
#include "ioLib.h"
#include "inetLib.h"
#include "string.h"
#include "netinet/in_var.h"
#include "errnoLib.h"
#include "sockLib.h"
#include "hostLib.h"
#include "routeLib.h"
#include "unistd.h"
#include "ifLib.h"
#include "m2Lib.h"
#include "arpLib.h"
#include "routeEnhLib.h"
#ifdef DEBUG
#include "logLib.h"
#endif

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsRadix.h" 	/* for rt_tables definition */
#include "netinet/vsIp.h" 	/* for _in_ifaddr definition */
#endif /* VIRTUAL_STACK */

/*
 * macro to convert interface address into character string representation.
 * The return value is a_ip_addr.
 */
#define ifInetAddrToStr(ifAddr,a_ip_addr) \
((inet_ntoa_b(((struct sockaddr_in *)(ifAddr))->sin_addr,a_ip_addr)),a_ip_addr)

#define RT_ARP          0x100                   /* arp route entry */

/*
 * Imported global variables
 */
#ifndef VIRTUAL_STACK
IMPORT struct ifnet 		*ifnet;	/* global pointer to all interfaces */
IMPORT struct in_ifaddr 	*in_ifaddr; /* global pointer to addresses */
#endif /* VIRTUAL_STACK */

IMPORT STATUS _arpCmd (int, struct in_addr *, u_char *, int *);

/* forward static functions */

static STATUS ifAddrParamGet (char *interfaceName, int code, char *address);
static STATUS ifIoctl (char *interfaceName, int code, int arg);
static STATUS ifIoctlSet (char *interfaceName, int code, int val);
static STATUS ifIoctlGet (char *interfaceName, int code, int *val);
static STATUS ifIoctlCall (int code, struct ifreq *ifrp);
LOCAL int routeNodeDelete ();
LOCAL int _routeNodeDelete (struct rtentry * pRt, int * pDeleted);


/*******************************************************************************
*
* ifUnnumberedSet - configure an interface to be unnumbered
*
* This API sets an interface unnumbered. It sets the IFF_POINTOPOINT flags
* and creates a routing entry through the interface using a user-specified
* destination IP address. The unnumbered link can then be uniquely referred
* to by the destination IP address, <pDstIp>, when adding routes. The
* interface is assigned a "borrowed" IP address--borrowed from another
* interface on the machine. In RFC 1812 it is also called the router ID.
* This address will be used to generate any needed ICMP messages or the like.
* Note that ARP is not able to run on an unnumbered link.
* 
* The initialization of the unnumbered device is similar to other
* network devices, but it does have a few additional steps and concerns.
* ifUnnumberedSet() must come next after ipAttach(). Please note that
* the interface using the IP address that the unnumbered interface will
* borrow must be brought up first and configured with <ifAddrSet> or 
* equivalent. This is required to ensure normal network operation for
* that IP address/interface.  After ifUnnumberedSet(), one must create
* additional routing entries (using mRouteAdd(), routeNetAdd(), etc)
* in order to reach other networks, including the network to which
* the destination IP address belongs. 
* 
* The <pDstMac> field in ifUnnumberedSet() is used to specify the
* destination's MAC address. It should be left NULL if the destination
* is not an Ethernet device. If the MAC address is not known, then
* supply an artificial address. We recommend using "00:00:00:00:00:01"
* The destination interface can then be set promiscuous to accept
* this artificial address. This is accomplished using the <ifpromisc>
* command.
*
* Example:
* \ss
* ipAttach (1, "fei")
* ifUnnumberedSet ("fei1", "120.12.12.12", "140.34.78.94", "00:a0:d0:d8:c8:14")
* routeNetAdd ("120.12.0.0","120.12.12.12") <One possible network>
* routeNetAdd ("178.45.0.0","120.12.12.12") <Another possible network>
* \se
* RETURNS: OK, or ERROR if the interface cannot be set.
*
*/

#ifdef ROUTER_STACK
STATUS ifUnnumberedSet
    (
    char *pIfName,	/* Name of interface to configure */ 
    char *pDstIp, 	/* Destination address of the point to point link */
    char *pBorrowedIp,	/* The borrowed IP address/router ID */
    char *pDstMac	/* Destination MAC address */
    )
    {
    int s, ret;
    struct ifnet *ifp;
    struct sockaddr_in sock;

    if(!(ifp = ifunit (pIfName)))
	return (ERROR);

    /*
     * Make sure that the interface with the "real"
     * pBorrowedIp is already brought up.
     */

    bzero ((char *)&sock, sizeof (struct sockaddr_in));
    sock.sin_family = AF_INET;
    sock.sin_len = sizeof (struct sockaddr_in);
    sock.sin_addr.s_addr = inet_addr (pBorrowedIp);

    if (!ifa_ifwithaddr ((struct sockaddr *)&sock))
        return (EINVAL); 


    /* We manually configure the interface to be IFF_UNNUMBERED */

    s = splnet ();
    ifp->if_flags &= ~IFF_BROADCAST;
    ifp->if_flags |= IFF_UNNUMBERED;	/* It's defined as IFF_POINTOPOINT */
    splx (s);

    ret = ifAddrAdd (pIfName, pBorrowedIp, pDstIp, 0);
    if (ret != OK)
	return (ret);

    /*
     * For ethernet devices, we need to complete the route entry
     * just created by the above operation. The ATF_INCOMPLETE flag
     * will change the routing entry to contain the MAC address
     * of the destination's ethernet device.
     */

    if (pDstMac != (char *)NULL)
        ret = arpAdd (pDstIp, pDstMac, ATF_PERM | ATF_INCOMPLETE);

    return (ret);
    }
#endif /* ROUTER_STACK */

/*******************************************************************************
*
* ifAddrAdd - add an interface address for a network interface
*
* This routine assigns an Internet address to a specified network interface.
* The Internet address can be a host name or a standard Internet address
* format (e.g., 90.0.0.4).  If a host name is specified, it should already
* have been added to the host table with hostAdd().
*
* You must specify both an <interfaceName> and an <interfaceAddress>. A 
* <broadcastAddress> is optional. If <broadcastAddress> is NULL, in_ifinit() 
* generates a <broadcastAddress> value based on the <interfaceAddress> value
* and the netmask.  A <subnetMask> value is optional.  If <subnetMask> is 0, 
* in_ifinit() uses a <subnetMask> the same as the netmask that is generated 
* by the <interfaceAddress>.  The <broadcastAddress> is also <destAddress> in 
* case of IFF_POINTOPOINT.
*
* RETURNS: OK, or ERROR if the interface cannot be set.
*
* SEE ALSO: ifAddrGet(), ifDstAddrSet(), ifDstAddrGet()
*/

STATUS ifAddrAdd
    (
    char *interfaceName,     /* name of interface to configure */
    char *interfaceAddress,  /* Internet address to assign to interface */
    char *broadcastAddress,  /* broadcast address to assign to interface */
    int   subnetMask         /* subnetMask */
    )
    {
    struct ifaliasreq   ifa;
    struct sockaddr_in *pSin_iaddr = (struct sockaddr_in *)&ifa.ifra_addr;
    struct sockaddr_in *pSin_baddr = (struct sockaddr_in *)&ifa.ifra_broadaddr;
    struct sockaddr_in *pSin_snmsk = (struct sockaddr_in *)&ifa.ifra_mask;
    int                 so;
    int                 status = 0;

    bzero ((caddr_t) &ifa, sizeof (ifa));

    /* verify Internet address is in correct format */
    if ((pSin_iaddr->sin_addr.s_addr =
             inet_addr (interfaceAddress)) == ERROR &&
        (pSin_iaddr->sin_addr.s_addr =
             hostGetByName (interfaceAddress) == ERROR))
        {
        return (ERROR);
        }

    /* verify Boradcast address is in correct format */
    if (broadcastAddress != NULL &&
        (pSin_baddr->sin_addr.s_addr =
             inet_addr (broadcastAddress)) == ERROR &&
        (pSin_baddr->sin_addr.s_addr =
             hostGetByName (broadcastAddress) == ERROR))
        {
        return (ERROR);
        }

    strncpy (ifa.ifra_name, interfaceName, sizeof (ifa.ifra_name));

    /* for interfaceAddress */
    ifa.ifra_addr.sa_len = sizeof (struct sockaddr_in);
    ifa.ifra_addr.sa_family = AF_INET;

    /* for broadcastAddress */
    if (broadcastAddress != NULL)
        {
        ifa.ifra_broadaddr.sa_len = sizeof (struct sockaddr_in);
        ifa.ifra_broadaddr.sa_family = AF_INET;
        }

    /* for subnetmask */
    if (subnetMask != 0)
       {
       ifa.ifra_mask.sa_len = sizeof (struct sockaddr_in);
       ifa.ifra_mask.sa_family = AF_INET;
       pSin_snmsk->sin_addr.s_addr = htonl (subnetMask);
       }

    if ((so = socket (AF_INET, SOCK_RAW, 0)) < 0)
        return (ERROR);

    status = ioctl (so, SIOCAIFADDR, (int)&ifa);
    (void)close (so);

    if (status != 0)
        {
        if (status != ERROR)    /* iosIoctl() can return ERROR */
            (void)errnoSet (status);

        return (ERROR);
        }
    return (OK);
    }

/*******************************************************************************
*
* ifAddrSet - set an interface address for a network interface
*
* This routine assigns an Internet address to a specified network interface.
* The Internet address can be a host name or a standard Internet address
* format (e.g., 90.0.0.4).  If a host name is specified, it should already
* have been added to the host table with hostAdd().
*
* A successful call to ifAddrSet() results in the addition of a new route.
*
* The subnet mask used in determining the network portion of the address will
* be that set by ifMaskSet(), or the default class mask if ifMaskSet() has not
* been called.  It is standard practice to call ifMaskSet() prior to calling
* ifAddrSet().
*
* RETURNS: OK, or ERROR if the interface cannot be set.
*
* SEE ALSO: ifAddrGet(), ifDstAddrSet(), ifDstAddrGet()
*/

STATUS ifAddrSet
    (
    char *interfaceName,        /* name of interface to configure, i.e. ei0 */
    char *interfaceAddress      /* Internet address to assign to interface */
    )
    {
    return (ifIoctl (interfaceName, SIOCSIFADDR, (int)interfaceAddress));
    }


/*******************************************************************************
*
* ifAddrDelete - delete an interface address for a network interface
*
* This routine deletes an Internet address from a specified network interface.
* The Internet address can be a host name or a standard Internet address
* format (e.g., 90.0.0.4).  If a host name is specified, it should already
* have been added to the host table with hostAdd().
*
* RETURNS: OK, or ERROR if the interface cannot be deleted.
*
* SEE ALSO: ifAddrGet(), ifDstAddrSet(), ifDstAddrGet()
*/

STATUS ifAddrDelete
    (
    char *interfaceName,        /* name of interface to delete addr from */
    char *interfaceAddress      /* Internet address to delete from interface */
    )
    {
    return (ifIoctl (interfaceName, SIOCDIFADDR, (int) interfaceAddress));
    }

/*******************************************************************************
*
* ifAddrGet - get the Internet address of a network interface
*
* This routine gets the Internet address of a specified network interface and
* copies it to <interfaceAddress>. This pointer should point to a buffer
* large enough to contain INET_ADDR_LEN bytes.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: ifAddrSet(), ifDstAddrSet(), ifDstAddrGet()
*/

STATUS ifAddrGet
    (
    char *interfaceName,        /* name of interface, i.e. ei0 */
    char *interfaceAddress      /* buffer for Internet address */
    )
    {
    return (ifAddrParamGet (interfaceName, SIOCGIFADDR, interfaceAddress));
    }

/*******************************************************************************
*
* ifBroadcastSet - set the broadcast address for a network interface
*
* This routine assigns a broadcast address for the specified network interface.
* The broadcast address must be a string in standard Internet address format
* (e.g., 90.0.0.0).
*
* An interface's default broadcast address is its Internet address with a
* host part of all ones (e.g., 90.255.255.255).  This conforms to current
* ARPA specifications.  However, some older systems use an Internet address
* with a host part of all zeros as the broadcast address.
*
* NOTE
* VxWorks automatically accepts a host part of all zeros as a broadcast
* address, in addition to the default or specified broadcast address.  But
* if VxWorks is to broadcast to older systems using a host part of all zeros
* as the broadcast address, this routine should be used to change the
* broadcast address of the interface.
*
* RETURNS: OK or ERROR.
*/

STATUS ifBroadcastSet
    (
    char *interfaceName,        /* name of interface to assign, i.e. ei0 */
    char *broadcastAddress      /* broadcast address to assign to interface */
    )
    {
    return (ifIoctl (interfaceName, SIOCSIFBRDADDR, (int)broadcastAddress));
    }

/*******************************************************************************
*
* ifBroadcastGet - get the broadcast address for a network interface
*
* This routine gets the broadcast address for a specified network interface.
* The broadcast address is copied to the buffer <broadcastAddress>.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: ifBroadcastSet()
*/

STATUS ifBroadcastGet
    (
    char *interfaceName,        /* name of interface, i.e. ei0 */
    char *broadcastAddress      /* buffer for broadcast address */
    )
    {
    return (ifAddrParamGet (interfaceName, SIOCGIFBRDADDR, broadcastAddress));
    }

/*******************************************************************************
*
* ifDstAddrSet - define an address for the other end of a point-to-point link
*
* This routine assigns the Internet address of a machine connected to the
* opposite end of a point-to-point network connection, such as a SLIP
* connection.  Inherently, point-to-point connection-oriented protocols such as
* SLIP require that addresses for both ends of a connection be specified.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: ifAddrSet(), ifDstAddrGet()
*/

STATUS ifDstAddrSet
    (
    char *interfaceName,        /* name of interface to configure, i.e. ei0 */
    char *dstAddress            /* Internet address to assign to destination */
    )
    {
    return (ifIoctl (interfaceName, SIOCSIFDSTADDR, (int)dstAddress));
    }

/*******************************************************************************
*
* ifDstAddrGet - get the Internet address of a point-to-point peer
*
* This routine gets the Internet address of a machine connected to the
* opposite end of a point-to-point network connection.  The Internet address is
* copied to the buffer <dstAddress>.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: ifDstAddrSet(), ifAddrGet()
*/

STATUS ifDstAddrGet
    (
    char *interfaceName,        /* name of interface, i.e. ei0 */
    char *dstAddress            /* buffer for destination address */
    )
    {
    return (ifAddrParamGet (interfaceName, SIOCGIFDSTADDR, dstAddress));
    }

/*******************************************************************************
*
* ifAddrParamGet - call ifIoctl to get the Internet address of the interface
*
* ifAddrParamGet may be used to retrieve Internet address of the network
* interface in ASCII character string representation (dot-notation
* like "192.0.0.3").
*
* RETURNS: OK or ERROR
*
* ERRNO: EINVAL
*
* SEE ALSO: ifAddrGet (), ifDstAddrGet (), ifBroadcastGet ()
*/

LOCAL STATUS ifAddrParamGet
    (
    char *interfaceName,        /* name of interface to configure, i.e. ei0 */
    int   code,                 /* SIOCG ioctl code */
    char *address               /* address retrieved here */
    )
    {
    struct in_addr inetAddrBuf;	/* result */
    char netString [INET_ADDR_LEN];

    if (address == NULL)
	{
	(void)errnoSet (EINVAL);
	return (ERROR);
	}

    if (ifIoctl (interfaceName, code, (int)&inetAddrBuf) == OK)
	{
	inet_ntoa_b (inetAddrBuf, netString);
	strncpy (address, netString, INET_ADDR_LEN);
	return (OK);
	}

    return (ERROR);
    }

/*******************************************************************************
*
* ifMaskSet - define a subnet for a network interface
*
* This routine allocates additional bits to the network portion of an
* Internet address.  The network portion is specified with a mask that must
* contain ones in all positions that are to be interpreted as the network
* portion.  This includes all the bits that are normally interpreted as the
* network portion for the given class of address, plus the bits to be added.
* Note that all bits must be contiguous.  The mask is specified in host byte
* order.
*
* In order to correctly interpret the address, a subnet mask should be set
* for an interface prior to setting the Internet address of the interface
* with the routine ifAddrSet().
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: ifAddrSet()
*/

STATUS ifMaskSet
    (
    char *interfaceName,   /* name of interface to set mask for, i.e. ei0 */
    int netMask            /* subnet mask (e.g. 0xff000000) */
    )
    {
    return (ifIoctl (interfaceName, SIOCSIFNETMASK, htonl (netMask)));
    }

/*******************************************************************************
*
* ifMaskGet - get the subnet mask for a network interface
*
* This routine gets the subnet mask for a specified network interface.
* The subnet mask is copied to the buffer <netMask>.  The subnet mask is
* returned in host byte order.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: ifAddrGet(), ifFlagGet()
*/

STATUS ifMaskGet
    (
    char *interfaceName,        /* name of interface, i.e. ei0 */
    int  *netMask               /* buffer for subnet mask */
    )
    {
    int status = ifIoctl (interfaceName, SIOCGIFNETMASK, (u_long) netMask);

    if (status != ERROR)
	*netMask = ntohl (*netMask);

    return (status);
    }

/*******************************************************************************
*
* ifFlagChange - change the network interface flags
*
* This routine changes the flags for the specified network interfaces.  If
* the parameter <on> is TRUE, the specified flags are turned on; otherwise,
* they are turned off.  The routines ifFlagGet() and ifFlagSet() are called
* to do the actual work.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: ifAddrSet(), ifMaskSet(), ifFlagSet(), ifFlagGet()
*/

STATUS ifFlagChange
    (
    char *interfaceName,        /* name of the network interface, i.e. ei0 */
    int   flags,                /* the flag to be changed */
    BOOL  on                    /* TRUE=turn on, FALSE=turn off */
    )
    {
    int oldFlags;

    if (ifFlagGet (interfaceName, &oldFlags) == ERROR)
	{
	return (ERROR);
	}

    if (on)
	oldFlags |= flags;
    else
	oldFlags &= ~flags;

    return (ifFlagSet (interfaceName, oldFlags));
    }

/*******************************************************************************
*
* ifFlagSet - specify the flags for a network interface
*
* This routine changes the flags for a specified network interface.
* Any combination of the following flags can be specified:
*
* .iP "IFF_UP (0x1)" 20
* Brings the network up or down.
* .iP "IFF_DEBUG (0x4)"
* Turns on debugging for the driver interface if supported.
* .iP "IFF_LOOPBACK (0x8)"
* Set for a loopback network.
* .iP "IFF_NOTRAILERS (0x20)"
* Always set (VxWorks does not use the trailer protocol).
* .iP "IFF_PROMISC (0x100)"
* Tells the driver to accept all packets, not just broadcast packets and
* packets addressed to itself. 
* .iP "IFF_ALLMULTI (0x200)"
* Tells the driver to accept all multicast packets.
* .iP "IFF_NOARP (0x80)"
* Disables ARP for the interface.
* .LP
*
* NOTE
* The following flags can only be set at interface initialization time.
* Specifying these flags does not change any settings in the interface
* data structure.
*
* .iP "IFF_POINTOPOINT (0x10)" 20
* Identifies a point-to-point interface such as PPP or SLIP.
* .iP "IFF_RUNNING (0x40)"
* Set when the device turns on.
* .iP "IFF_BROADCAST (0x2)"
* Identifies a broadcast interface.
* .LP
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: ifFlagChange(), ifFlagGet()
*/

STATUS ifFlagSet
    (
    char *interfaceName,        /* name of the network interface, i.e. ei0 */
    int   flags                 /* network flags */
    )
    {
    return (ifIoctl (interfaceName, SIOCSIFFLAGS, flags));
    }

/*******************************************************************************
*
* ifFlagGet - get the network interface flags
*
* This routine gets the flags for a specified network interface.
* The flags are copied to the buffer <flags>.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: ifFlagSet()
*/

STATUS ifFlagGet
    (
    char *interfaceName,        /* name of the network interface, i.e. ei0 */
    int  *flags                 /* network flags returned here */
    )
    {
    return (ifIoctl (interfaceName, SIOCGIFFLAGS, (int)flags));
    }

/*******************************************************************************
*
* ifMetricSet - specify a network interface hop count
*
* This routine configures <metric> for a network interface from the host
* machine to the destination network.  This information is used primarily by
* the IP routing algorithm to compute the relative distance for a collection
* of hosts connected to each interface.  For example, a higher <metric> for
* SLIP interfaces can be specified to discourage routing a packet to slower
* serial line connections.  Note that when <metric> is zero, the IP routing
* algorithm allows for the direct sending of a packet having an IP network
* address that is not necessarily the same as the local network address.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: ifMetricGet()
*/

STATUS ifMetricSet
    (
    char *interfaceName,        /* name of the network interface, i.e. ei0 */
    int   metric                /* metric for this interface */
    )
    {
    return (ifIoctl (interfaceName, SIOCSIFMETRIC, metric));
    }

/*******************************************************************************
*
* ifMetricGet - get the metric for a network interface
*
* This routine retrieves the metric for a specified network interface.
* The metric is copied to the buffer <pMetric>.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: ifMetricSet()
*/

STATUS ifMetricGet
    (
    char *interfaceName,        /* name of the network interface, i.e. ei0 */
    int  *pMetric               /* returned interface's metric */
    )
    {
    return (ifIoctl (interfaceName, SIOCGIFMETRIC, (int)pMetric));
    }

/*******************************************************************************
*
* ifIoctl - network interface ioctl front-end
*
* Used to manipulate the characteristics of network interfaces
* using socket specific ioctl functions SIOCSIFADDR, SIOCSIFBRDADDR, etc.
* ifIoctl() accomplishes this by calling ifIoctlSet() and ifIoctlGet().
*
* RETURNS: OK or ERROR
*
* ERRNO: EOPNOTSUPP
*/

LOCAL STATUS ifIoctl
    (
    char *interfaceName,        /* name of the interface, i.e. ei0 */
    int code,                   /* ioctl function code */
    int arg                     /* some argument */
    )
    {
    int status;
    int hostAddr;

    switch ((u_int) code)
	{
	case SIOCAIFADDR:
	case SIOCDIFADDR:
	case SIOCSIFADDR:
	case SIOCSIFBRDADDR:
	case SIOCSIFDSTADDR:

	    /* verify Internet address is in correct format */
	    if ((hostAddr = (int) inet_addr ((char *)arg)) == ERROR &&
		(hostAddr = hostGetByName ((char *)arg)) == ERROR)
		{
		return (ERROR);
		}

	    status = ifIoctlSet (interfaceName, code, hostAddr);
	    break;

	case SIOCSIFNETMASK:
        case SIOCSIFFLAGS:
        case SIOCSIFMETRIC:
	    status = ifIoctlSet (interfaceName, code, arg);
	    break;

	case SIOCGIFNETMASK:
	case SIOCGIFFLAGS:
	case SIOCGIFADDR:
	case SIOCGIFBRDADDR:
	case SIOCGIFDSTADDR:
	case SIOCGIFMETRIC:
	    status = ifIoctlGet (interfaceName, code, (int *)arg);
	    break;

	default:
	    (void)errnoSet (EOPNOTSUPP); /* not supported operation */
	    status = ERROR;
	    break;
	}

    return (status);
    }

/*******************************************************************************
*
* ifIoctlSet - configure network interface
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS ifIoctlSet
    (
    char *interfaceName,        /* name of the network interface, i.e. ei0 */
    int   code,                 /* network interface ioctl function code */
    int   val                   /* value to be changed */
    )
    {
    struct ifreq ifr;

    strncpy (ifr.ifr_name, interfaceName, sizeof (ifr.ifr_name));

    switch ((u_int) code)
	{
	case SIOCSIFFLAGS:
	    ifr.ifr_flags = (short) val;
	    break;

        case SIOCSIFMETRIC:
	    ifr.ifr_metric = val;
	    break;

        default:
	    bzero ((caddr_t) &ifr.ifr_addr, sizeof (ifr.ifr_addr));
	    ifr.ifr_addr.sa_len = sizeof (struct sockaddr_in);
	    ifr.ifr_addr.sa_family = AF_INET;
	    ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr = val;
	    break;
	}

    return (ifIoctlCall (code, &ifr));
    }

/*******************************************************************************
*
* ifIoctlGet - retrieve information about the network interface
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS ifIoctlGet
    (
    char *interfaceName,        /* name of the network interface, i.e. ei0 */
    int   code,                 /* network interface ioctl function code */
    int  *val                   /* where to return result */
    )
    {
    struct ifreq  ifr;

    strncpy (ifr.ifr_name, interfaceName, sizeof (ifr.ifr_name));

    if (ifIoctlCall (code, &ifr) == ERROR)
	return (ERROR);

    switch ((u_int) code)
	{
	case SIOCGIFFLAGS:
	    *val = ifr.ifr_flags;
	    break;

        case SIOCGIFMETRIC:
	    *val = ifr.ifr_metric;
	    break;

	default:
	    *val = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;
	    break;
	}

    return (OK);
    }

/*******************************************************************************
*
* ifIoctlCall - make ioctl call to socket layer
*
* ifIoctlCall creates a dummy socket to get access to the socket layer
* ioctl routines in order to manipulate the interface specific
* configurations.
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS ifIoctlCall
    (
    int           code,         /* ioctl code */
    struct ifreq *ifrp          /* pointer to the interface ioctl request */
    )
    {
    int so;
    int status;

    if ((so = socket (AF_INET, SOCK_RAW, 0)) < 0)
	return (ERROR);

    status = ioctl (so, code, (int)ifrp);
    (void)close (so);

    if (status != 0)
	{
	if (status != ERROR)	/* iosIoctl() can return ERROR */
	    (void)errnoSet (status);

	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* ifRouteDelete - delete routes associated with a network interface
*
* This routine deletes all routes that have been associated with the
* specified interface. A route is associated with an interface if its 
* destination equals to the assigned address, or network number. This routine
* does not remove routes to arbitrary destinations that through the
* given interface.
*
*
* INTERNAL
* This function only works for address families AF_INET
*
* RETURNS:
* The number of routes deleted, or ERROR if an interface is not specified.
*/

int ifRouteDelete
    (
    char *ifName,               /* name of the interface */
    int   unit                  /* unit number for this interface */
    )
    {
    FAST struct ifnet 		*ifp;
    FAST struct ifaddr 		*ifa;
    FAST struct in_ifaddr 	*ia = 0;
    int 			deleted;
    struct sockaddr_in		inetAddr;

    if (ifName == NULL)
	return (ERROR);

    deleted = 0;

#ifdef VIRTUAL_STACK
    for (ifp = _ifnet; ifp; ifp = ifp->if_next)
#else
    for (ifp = ifnet; ifp; ifp = ifp->if_next)
#endif /* VIRTUAL_STACK */
	{
	if (ifp->if_unit != unit || strcmp(ifName, ifp->if_name))
	    continue;

	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
	    {
            /* skip if address family does not belong to AF_INET */
            if (ifa->ifa_addr->sa_family != AF_INET)
                continue;

            /*
	     * Every element in the if_addrlist (type struct ifaddr) with
	     * family AF_INET is actually the first part of a larger
	     * in_ifaddr structure which includes the subnet mask. Access
	     * that structure to use the mask value.
	     */

	    ia = (struct in_ifaddr *) ifa;

	    /* 
	     * The following is a sanity test.
	     */
#ifdef ROUTER_STACK 	/* UNNUMBERED_SUPPORT */ 
	    /*
	     * This test is to prevent an ipDetach on an unnumbered interface
	     * from removing the routes generated by the real interface if that
	     * interface is still up. See ifUnnumberedSet for the definition
	     * of "real interface."
	     */
	    if (ifa == ifa_ifwithaddr (ifa->ifa_addr))
#endif /* ROUTER_STACK */
		{
                if (mRouteEntryDelete (((struct sockaddr_in*)ifa->ifa_addr)->
				    sin_addr.s_addr, 0, 0xffffffff, 0, RTF_HOST, 
				    M2_ipRouteProto_local) == OK)
                    ++deleted;

                inetAddr.sin_addr.s_addr =
                    htonl (ia->ia_subnetmask) &
                    ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr;

                if (mRouteEntryDelete (((struct sockaddr_in*)ifa->ifa_addr)->
				    sin_addr.s_addr, 0, htonl(ia->ia_subnetmask), 
				    0, 0, M2_ipRouteProto_local) == OK)
                    ++deleted;
		}

	    if (ifp->if_flags & (IFF_POINTOPOINT|IFF_UNNUMBERED))
		{
                if (mRouteEntryDelete (((struct sockaddr_in*)ifa->ifa_dstaddr)->
                                       sin_addr.s_addr, 
                                       ((struct sockaddr_in*)ifa->ifa_addr)->
                                       sin_addr.s_addr, 0x0, 0,
                                       RTF_HOST, M2_ipRouteProto_local) == OK)
		    ++deleted;
#ifdef ROUTER_STACK     /* UNNUMBERED_SUPPORT */

		else
		    {
                    _arpCmd (SIOCDARP, 
                             &(((struct sockaddr_in *)
                                ifa->ifa_dstaddr)->sin_addr), NULL, NULL);        
		    deleted++;
		    }
#endif /* ROUTER_STACK */
		}
	    }
	}

    return (deleted);
    }

/*******************************************************************************
*
* routeNodeDelete - delete the route associated with the network interface
*
* This routine deletes the route that has been associated with the
* specified interface. Only the following route is deleted:
* \ml 
* \m -
* the network route added when the interface address is initialized
* \m -
* the static route added by the administrator that passes through the interface
* \m -
* ARP routes passing through the interface
* \me
* 
* Routes added by routing protocols are not deleted.
* For the Router stack, this routine first scans all duplicate routes
* to determine which ones need to be deleted, and then processes the primary
* route. This special handling is needed because the way the duplicate
* routes are organized, deleting the primary route first might lead
* to memory corruption.
*
* INTERNAL
* This function only works for address families AF_INET
*
* RETURNS:
* 0
*/

LOCAL int routeNodeDelete
    (
    struct radix_node *	pRoute,    /* pointer to the node structure */
    void *		pArg       /* pointer to the interface */
    )
    {

    struct ifnet *		ifp =  (struct ifnet *)((int *)pArg)[0];
    int   			type = ((int *)pArg)[1];
    int	*			pDeleted = (int *)((int *)pArg)[2];
    struct rtentry *		pRt;
    FAST struct sockaddr *	destAddr = NULL;
    FAST struct sockaddr *	gateAddr;
    char 			aStr [INET_ADDR_LEN];
#ifdef ROUTER_STACK
    ROUTE_ENTRY * pHead; 
    ROUTE_ENTRY * pDiffSave; 
    ROUTE_ENTRY * pSameSave; 
    ROUTE_ENTRY * pNext;
#endif /* ROUTER_STACK */

    pRt = (struct rtentry *)pRoute;

    destAddr = rt_key(pRt);		/* get the destination address */
    gateAddr = pRt->rt_gateway;		/* get the gateway address */

#ifdef DEBUG
    inet_ntoa_b (((struct sockaddr_in *)destAddr)->sin_addr, aStr);
    logMsg ("routeNodeDelete: Called for %s, if=%s%d, proto=%d, type = %s\n", 
            (int)aStr, (int)(pRt->rt_ifp->if_name), pRt->rt_ifp->if_unit,
            RT_PROTO_GET (destAddr), 
            (int)(type == RT_ARP ? "ARP" : "NON_ARP"), 0); 
#endif /* DEBUG */

    /* Just return if it is an ARP entry and we are not interested in it */

    if (type != RT_ARP && 
        ((pRt->rt_flags & RTF_HOST) && 
         (pRt->rt_flags & RTF_LLINFO) && 
         (pRt->rt_flags & RTF_GATEWAY) == 0 && 
         gateAddr && (gateAddr->sa_family == AF_LINK)))
        return(0);

    /* Just return if we want an ARP entry, but this is not */

    if (type == RT_ARP && 
        !((pRt->rt_flags & RTF_HOST) && 
          (pRt->rt_flags & RTF_LLINFO) && 
          (pRt->rt_flags & RTF_GATEWAY) == 0 && 
          gateAddr && (gateAddr->sa_family == AF_LINK)))
        return(0);

    /* 
     * If it is an ARP entry and it passes through the interface, 
     * delete the ARP entry 
     */

    if ((pRt->rt_flags & RTF_HOST) &&
        (pRt->rt_flags & RTF_LLINFO) &&
        (pRt->rt_flags & RTF_GATEWAY) == 0 &&
        gateAddr && (gateAddr->sa_family == AF_LINK)) 
        {
        if(pRt->rt_ifp == ifp)
            {
            inet_ntoa_b (((struct sockaddr_in *)destAddr)->sin_addr, aStr);
#ifdef DEBUG
            logMsg ("routeNodeDelete: deleting ARP entry for %s\n", 
                    (int)aStr,0,0,0,0,0); 
#endif /* DEBUG */
            _arpCmd (SIOCDARP, &((struct sockaddr_in *)destAddr)->sin_addr, 
                     NULL, NULL);        

            /* Increment the deletion counter */

            ++(*pDeleted);
            }
        return (0);
        }

#ifdef ROUTER_STACK
    /* 
     * Because of the way the duplicate route list is managed, we need to
     * delete all duplicate routes, before we delete the primary route.
     * So walk through all the duplicate routes if any, and delete them
     * if the interface matches.
     */

    pHead = (ROUTE_ENTRY *)pRoute;
    pNext = pHead->sameNode.pFrwd;
    if (pNext == NULL)
        {
        pHead = pNext = pHead->diffNode.pFrwd;
        }

    while (pHead)
        { 
        pDiffSave = pHead->diffNode.pFrwd;
        while (pNext)
            {
            /* Save the next pointer in case we delete this node */

            pSameSave = pNext->sameNode.pFrwd;

            /* If this route passes through the interface, delete it */

            if (pNext->rtEntry.rt_ifp == ifp) 
                _routeNodeDelete ((struct rtentry *)pNext, pDeleted);

            pNext = pSameSave;
            }
        pHead = pNext = pDiffSave;
        }
#endif /* ROUTER_STACK */

    /* Now delete the primary route if it passes through the interface */

    if (pRt->rt_ifp == ifp) 
        _routeNodeDelete (pRt, pDeleted);

    return OK;
    }
/*******************************************************************************
*
* _routeNodeDelete - delete the route associated with the network interface
*
* This routine deletes the route that has been associated with the
* specified interface. Only the following route is deleted:
*
* \ml
* \m -
* the network route added by us when the interface address is initialized
* \m -
* the static route added by the administrator that passes through the interface
* \me
*
* Routes added by routing protocols are not deleted.
*
* INTERNAL
* This function only works for address families AF_INET.
*
* RETURNS:
* 0
*/

LOCAL int _routeNodeDelete
    (
    struct rtentry *	pRt,		/* pointer to the rtentry structure */
    int *		pDeleted	/* counts # of deleted routes */
    )
    {
    FAST struct sockaddr *	destAddr = NULL;
    FAST struct sockaddr *	gateAddr;
    int proto;
    long mask;
#ifdef DEBUG
    char 			aStr [INET_ADDR_LEN];
    char 			bStr [INET_ADDR_LEN];
    char 			cStr [INET_ADDR_LEN];
#endif /* DEBUG */

    destAddr = rt_key(pRt);		/* get the destination address */
    gateAddr = pRt->rt_gateway;		/* get the gateway address */

#ifdef DEBUG
    inet_ntoa_b (((struct sockaddr_in *)destAddr)->sin_addr, aStr);
    logMsg ("_routeNodeDelete: Called for %s, if=%s%d, proto=%d, type = %s\n", 
            (int)aStr, (int)(pRt->rt_ifp->if_name), pRt->rt_ifp->if_unit,
            RT_PROTO_GET (destAddr), (int)"NON_ARP", 0); 
#endif /* DEBUG */

    /*
     * We delete only statically added routes, interface routes and ICMP
     * routes. All other routes belong to the respective routing protocols
     * that should be deleting them
     */

    if ((proto = RT_PROTO_GET (destAddr)) != M2_ipRouteProto_other &&
        proto != M2_ipRouteProto_local &&
        proto != M2_ipRouteProto_icmp)
        return (0);

    /* If it is an interface route, get the interface address */

    if (gateAddr->sa_family == AF_LINK)
        gateAddr = pRt->rt_ifa->ifa_addr; 

    if (rt_mask (pRt) == NULL)
        {
       	mask = 0;
        }
    else
        {
        mask = ((struct sockaddr_in *) rt_mask (pRt))->sin_addr.s_addr;
        }

#ifdef DEBUG
    inet_ntoa_b (((struct sockaddr_in *)destAddr)->sin_addr, aStr);
    if (mask)
        inet_ntoa_b (((struct sockaddr_in *) rt_mask (pRt))->sin_addr, bStr);
    else
        bStr[0] = 0;
    inet_ntoa_b (((struct sockaddr_in *)gateAddr)->sin_addr, cStr);
    logMsg ("_routeNodeDelete: Deleting following route: dest = %s, nmask = %s,"
            "gate = %s\n", (int)aStr, (int)bStr, (int)cStr, 0, 0, 0);
#endif /* DEBUG */

    /* Now delete the route */

    mRouteEntryDelete (((struct sockaddr_in*)destAddr)->sin_addr.s_addr, 
                       ((struct sockaddr_in*)gateAddr)->sin_addr.s_addr, 
                       mask, TOS_GET (destAddr), pRt->rt_flags, proto);

    /* Increment the deletion counter */

    ++(*pDeleted);

    return 0;
    }

/*******************************************************************************
* ifAllRoutesDelete - delete all routes associated with a network interface
*
* This routine deletes all routes that have been associated with the
* specified interface. The routes deleted are:
* \ml
* \m - 
* the network route added when the interface address is initialized
* \m - 
* the static routes added by the administrator
* \m - 
* ARP routes passing through the interface
* \me
* Routes added by routing protocols are not deleted.
*
* INTERNAL
* This function only works for address families AF_INET.
*
* RETURNS:
* The number of routes deleted, or ERROR if an interface is not specified.
*/

int ifAllRoutesDelete
    (
    char *ifName,               /* name of the interface */
    int   unit                  /* unit number for this interface */
    )
    {
    FAST struct ifnet 		*ifp;
    FAST struct ifaddr 		*ifa;
    FAST struct in_ifaddr 	*ia = 0;
    int 			 	deleted;
    int 				args[3];
    struct radix_node_head *  	rnh;

    if (ifName == NULL)
        return (ERROR);

    deleted = 0;

#ifdef VIRTUAL_STACK
    for (ifp = _ifnet; ifp; ifp = ifp->if_next)
#else
    for (ifp = ifnet; ifp; ifp = ifp->if_next)
#endif /* VIRTUAL_STACK */
        {
            if (ifp->if_unit != unit || strcmp(ifName, ifp->if_name))
                continue;

	/*
	 * Find address for this interface, if it exists.
	 */
#ifdef VIRTUAL_STACK
	for (ia = _in_ifaddr; ia; ia = ia->ia_next)
#else
	for (ia = in_ifaddr; ia; ia = ia->ia_next)
#endif /* VIRTUAL_STACK */
	    if (ia->ia_ifp == ifp)
		break;

	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
            {

            /* skip if address family does not belong to AF_INET */
            
            if (ifa->ifa_addr->sa_family != AF_INET)
                continue;

            /*
             * Walk through the entire table and delete routes associated
             * with the interface.
             */

            /* First delete the ARP entries */

            args[0] = (int)ifp;
            args[1] = RT_ARP;
            args[2] = (int)&deleted;

            rnh = rt_tables[AF_INET];
            if (rnh) 
                rn_walktree(rnh, routeNodeDelete, (void *)args);
            
            /* Next delete the non-ARP routes */

            args[0] = (int)ifp;
            args[1] = ~RT_ARP;
            args[2] = (int)&deleted;

            if (rnh) 
                rn_walktree(rnh, routeNodeDelete, (void *)args);
            
            break;
            }
        }

    return (deleted);
    }


/*******************************************************************************
*
* ifunit - map an interface name to an interface structure pointer
*
* This routine returns a pointer to a network interface structure for <name> or
* NULL if no such interface exists.  For example:
* .CS
*     struct ifnet *pIf;
*     ...
*     pIf = ifunit ("ln0");
* .CE
* `pIf' points to the data structure that describes the first network interface
* device if ln0 is mapped successfully.
*
* RETURNS:
* A pointer to the interface structure, or NULL if an interface is not
* found.
*
*/

struct ifnet *ifunit
    (
    register char *ifname    /* name of the interface */
    )
    {
    register char *cp;
    register struct ifnet *ifp;
    int unit;
    unsigned len;
    char *ep, c;
    char name [IFNAMSIZ];

    if (ifname == NULL)
	return ((struct ifnet *)0);

    strncpy (name, ifname, IFNAMSIZ);

    for (cp = name; cp < name + IFNAMSIZ && *cp; cp++)
	if (*cp >= '0' && *cp <= '9')
	    break;

    if (*cp == '\0' || cp == name + IFNAMSIZ)
	return ((struct ifnet *)0);

    /*
     * Save first char of unit, and pointer to it,
     * so we can put a null there to avoid matching
     * initial substrings of interface names.
     */

    len = cp - name + 1;
    c = *cp;
    ep = cp;
    for (unit = 0; *cp >= '0' && *cp <= '9'; )
        unit = unit * 10 + *cp++ - '0';

    *ep = 0;

#ifdef VIRTUAL_STACK
    for (ifp = _ifnet; ifp; ifp = ifp->if_next)
#else
    for (ifp = ifnet; ifp; ifp = ifp->if_next)
#endif /* VIRTUAL_STACK */
	{
	if (bcmp(ifp->if_name, name, len))
	    continue;
        if (unit == ifp->if_unit)
	    break;
	}

    *ep = c;
    return (ifp);
    }

/****************************************************************************
*
* ifNameToIfIndex - returns the interface index given the interface name
*
* This routine returns the interface index for the interface named by the 
* <ifName> parameter, which proviedes a string describing the full interface 
* name. For example, "fei0".
*
* RETURNS: The interface index, if the interface could be located,  
* 0, otherwise.  0 is not a valid value for interface index.
*/

unsigned short ifNameToIfIndex 
    (
    char *	ifName	/* a string describing the full interface name. */
    			/* e.g., "fei0"	*/
    )
    {
    FAST struct ifnet *		ifp;
    int				s;
    int				ifIndex;

    /* Call ifunit under splnet protection as it is not protected */

    s = splnet ();
    ifp = ifunit (ifName);
    ifIndex = (ifp != NULL) ? ifp->if_index : 0;
    splx (s);

    return (ifIndex);
    }

/****************************************************************************
*
* ifIndexToIfName - returns the interface name given the interface index
*
* This routine returns the interface name for the interface referenced
* by the <ifIndex> parameter. 
*
* \is
* \i <ifIndex>
*    The index for the interface.
* \i <ifName>
*    The location where the interface name is copied
* \ie
*
* RETURNS: OK on success, ERROR otherwise.
*/

STATUS ifIndexToIfName 
    (
    unsigned short	ifIndex,/* Interface index */
    char *		ifName	/* Where the name is to be stored */
    )
    {
    FAST struct ifnet *		ifp;
    int				s;


    if (ifName == NULL)
        return (ERROR);

    s = splnet ();
    if ((ifp = ifIndexToIfp (ifIndex)) == NULL)
        {
        splx (s);
        return (ERROR);
        }
    sprintf (ifName, "%s%d", ifp->if_name, ifp->if_unit);
    splx (s);

    return (OK);
    }

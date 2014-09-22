/* routeLib.c - network route manipulation library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03c,10may02,kbw  making man page edits
03b,07dec01,rae  merge from synth ver 03h, (SPR #69690)
03a,15oct01,rae  merge from truestack ver 03g, base 02z (update)
02z,16mar99,spm  recovered orphaned code from tor1_0_1.sens1_1 (SPR #25770)
02y,29sep98,spm  added mPrivRouteEntryDelete for held kernel routes (SPR #9374)
02x,14dec97,jdi  doc: cleanup.
02w,05dec97,rjc  added RTF_CLONING flag to mRoute routines. 
02v,25oct97,kbw  making minor man page fixes 
02u,22aug97,rjc  modifed return vals for mRoute* calls.
02t,08aug97,vin  fixed SPR9121.
02s,04aug97,kwb  fixed man page problems found in beta review
02r,07jul97,rjc  route proto field moved to dest sockaddr.
02q,30jun97,rjc  restored old rtentry size, added socktrim.
02p,04jun97,rjc  fixed route proto priority glitches.
02o,03jun97,rjc  fixed mask and RTF_HOST combinations.
02n,29may97,rjc  fixed byte orders in mRoute routines
02m,09may97,rjc  fixed mRoute{*} man pages
02l,30apr97,kbw  fiddle man page text
02k,28apr97,gnn  fixed some of the documentation.
02j,20apr97,kbw  fixed minor format errors in man page text
02i,15apr97,kbw  fixed minor format errors in man page text
02h,14apr97,gnn  added documentation for new mRoute calls.
02g,11apr97,rjc  added routing interface support, routing priority stuff.
02f,13feb97,rjc  added masking of destination in mRoute--- stuff.
02e,13feb97,rjc  updated mRouteEntryAdd/delete.
02d,07feb97,rjc  newer  versions of route add/delete etc.
02c,05feb97,rjc  new versions of route add/delete etc.
02b,18jan95,jdi  doc tweaks for routeNetAdd().
02a,08jul94,dzb  Added routeNetAdd() for adding subnet routes (SPR #3395).
01z,14feb94,elh  made rtEntryFill global and NOMANUAL.
01y,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01x,20jan93,jdi  documentation cleanup for 5.1.
01w,28jul92,elh  moved routeShow to netShow.c
01v,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01u,01mar92,elh  added routeCmd.
01t,16dec91,gae  added includes for ANSI.
01s,02dec91,elh  moved error handling to lower level (sys_socket).
01r,26nov91,llk  changed include of errnoLib.h to errno.h.
01q,12nov91,elh	 changed routes to interfaces to be direct routes,
		 made routeChange set errno (rtioctl returns errno instead of -1)
01p,07nov91,elh	 changed inet_ntoa to inet_ntoa_b so memory isn't lost.
01o,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01n,21may91,jdi	 documentation tweaks.
01m,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01l,22feb91,jaa	 documentation cleanup.
01k,10aug90,kdl  added forward declarations for functions returning void.
01j,07may90,hjb  added documentation to routeAdd about adding the default route,
		 explanation of the route specific flags, and made the
		 output more unix compatible.
01i,07mar90,jdi  documentation cleanup.
01h,02sep89,hjb  fixed a big in routeShow() - instead of using RTHASHSIZ
		 we now use the global rthashsize which is always set to
		 the correct value in route.c.  (When GATEWAY is undefined,
		 the route.h include file defines RTHASHSIZ to smaller value.)
01g,07aug89,gae  fixed in_lnaof call in routeEntryFill for SPARC.
01g,20aug88,gae  documentation.
01f,30may88,dnw  changed to v4 names.
01e,28may88,dnw  replace call to netGetInetAddr with calls to remGetHostByName
		   and inet_addr.
01d,18feb88,dnw  changed to handle subnet masks correctly (by calling in_lnaof()
		   instead of inet_lnaof() in routeChange).
01c,16nov87,jlf  documentation
01b,16nov87,llk  documentation.
01a,01nov87,llk  written.
*/

/*
DESCRIPTION
This library contains the routines for inspecting the routing table,
as well as routines for adding and deleting routes from that table.  
If you do not configure VxWorks to include a routing protocol, such 
as RIP or OSPF, you can use these routines to maintain the routing 
tables manually.  

To use this feature, include the following component:
INCLUDE_NETWRS_ROUTELIB

INCLUDE FILES: routeLib.h

SEE ALSO: hostLib
*/

#include "vxWorks.h"
#include "net/mbuf.h"
#include "sys/socket.h"
#include "sockLib.h"
#include "unistd.h"
#include "net/if.h"
#include "net/route.h"
#include "netinet/in.h"
#include "sys/ioctl.h"
#include "inetLib.h"
#include "errno.h"
#include "hostLib.h"
#include "stdio.h"
#include "ioLib.h"
#include "routeLib.h"
#include "m2Lib.h"
#include "routeEnhLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif /* VIRTUAL_STACK */

/* forward static functions */

static STATUS routeChange (char *destination, char *gateway, int ioctlCmd);
void routeEntryFill (struct ortentry *pRoute, int destInetAddr, int
		gateInetAddr, BOOL hostAddr);

/*******************************************************************************
*
* routeAdd - add a route
*
* This routine adds gateways to the network routing tables.
* It is called from a VxWorks machine that needs
* to establish a gateway to a destination network (or machine).
*
* You can specify both <destination> and <gateway> in standard Internet
* address format (for example, 90.0.0.2), or you can specify them using
* their host names, as specified with hostAdd().
*
* This routine can be used to add multiple routes to the same destination
* differing by the gateway.
*
* EXAMPLE
* Consider the following example:
* .CS
*     -> routeAdd "90.0.0.0", "gate"
* .CE
* This call tells VxWorks that the machine with the host name "gate" is 
* the gateway to network 90.0.0.0.  The host "gate" must already have 
* been created by hostAdd().
*
* Consider the following example:
* .CS
*     -> routeAdd "90.0.0.0", "91.0.0.3"
* .CE
* This call tells VxWorks that the machine with the Internet 
* address 91.0.0.3 is the gateway to network 90.0.0.0.
*
* Consider the following example:
* .CS
*     -> routeAdd "destination", "gate"
* .CE
* This call tells VxWorks that the machine with the host name "gate" is 
* the gateway to the machine named "destination".  The host names "gate" and 
* "destination" must already have been created by hostAdd().
*
* Consider the following example:
* .CS
*     -> routeAdd "0", "gate"
* .CE
* This call tells VxWorks that the machine with the host name "gate" is 
* the default gateway.  The host "gate" must already have been created 
* by hostAdd().  A default gateway is where Internet Protocol (IP) 
* datagrams are routed when there is no specific routing table entry 
* available for the destination IP network or host.
*
* RETURNS: OK or ERROR.
*/

STATUS routeAdd
    (
    char *destination,  /* inet addr or name of route destination      */
    char *gateway       /* inet addr or name of gateway to destination */
    )
    {
    return (routeChange (destination, gateway, (int) SIOCADDRT));
    }

/*******************************************************************************
*
* routeNetAdd - add a route to a destination that is a network
*
* This routine is equivalent to routeAdd(), except that the destination
* address is assumed to be a network.  This is useful for adding a route
* to a sub-network that is not on the same overall network as the
* local network.
* 
* This routine can be used to add multiple routes to the same destination
* differing by the gateway.
*
* RETURNS: OK or ERROR.
*/

STATUS routeNetAdd
    (
    char *destination,  /* inet addr or name of network destination */
    char *gateway       /* inet addr or name of gateway to destination */
    )
    {
    int destInetAddr;		/* destination internet adrs */
    int	gateInetAddr;		/* gateway internet adrs */
    struct ortentry route;	/* route entry */
    int	sock;			/* socket */
    int	status;			/* return status */

    if ((((destInetAddr = hostGetByName (destination)) == ERROR) &&
	 ((destInetAddr = (int) inet_addr (destination)) == ERROR)) ||
        (((gateInetAddr = hostGetByName (gateway)) == ERROR) &&
	 ((gateInetAddr = (int) inet_addr (gateway)) == ERROR)))
	{
	return (ERROR);
	}

    routeEntryFill (&route, destInetAddr, gateInetAddr, FALSE);

    /* add the route */

    sock = socket (AF_INET, SOCK_RAW, 0);
    status = ioctl (sock, SIOCADDRT, (int) &route);
    (void) close (sock);
    return (status);
    }

/*******************************************************************************
*
* routeDelete - delete a route
*
* This routine deletes a specified route from the network routing tables.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: routeAdd()
*/

STATUS routeDelete
    (
    char *destination,  /* inet addr or name of route destination      */
    char *gateway       /* inet addr or name of gateway to destination */
    )
    {
    return (routeChange (destination, gateway, (int) SIOCDELRT));
    }

/*******************************************************************************
*
* routeChange - change the routing tables
*
* This routine makes changes to the routing tables by using an ioctl() call
* to add or delete a route.
*
* This is a local routine which should not be used directly.
* Instead, routeAdd() and routeDelete() should be used as interfaces
* to this routine.
*
* The <destination> and <gateway> parameters may be specified either by their
* Internet addresses in standard internet address format (e.g., "90.0.0.2"),
* or by their names (their host names) which have already been added to the
* remote host table (with hostAdd()).
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: routeAdd()
*/

LOCAL STATUS routeChange
    (
    char *destination,  /* inet address or name of route destination      */
    char *gateway,      /* inet address or name of gateway to destination */
    int ioctlCmd        /* addition or deletion of a route                */
    )
    {
    int destInetAddr;		/* destination internet adrs */
    int gateInetAddr;		/* gateway internet adrs */

    if ((((destInetAddr = hostGetByName (destination)) == ERROR) &&
	 ((destInetAddr = (int) inet_addr (destination)) == ERROR)) ||
        (((gateInetAddr = hostGetByName (gateway)) == ERROR) &&
	 ((gateInetAddr = (int) inet_addr (gateway)) == ERROR)))
	{
	return (ERROR);
	}

    return (routeCmd (destInetAddr, gateInetAddr, ioctlCmd));
    }

/*******************************************************************************
*
* routeCmd - change the routing tables
*
* routeCmd is the same as routeChange except it provides a non-string
* interface.
*
* NOMANUAL
*/

STATUS routeCmd
    (
    int		destInetAddr,		/* destination adrs */
    int		gateInetAddr,		/* gateway adrs */
    int		ioctlCmd		/* route command */
    )
    {
    struct ortentry route;	/* route entry		*/
    int 	   sock;	/* socket 		*/
    int 	   status;	/* return status	*/

    routeEntryFill (&route, destInetAddr, gateInetAddr, TRUE);

    /* add or delete the route */

    sock = socket (AF_INET, SOCK_RAW, 0);
    if (sock == ERROR)
        return (ERROR);

    status = ioctl (sock, ioctlCmd, (int) &route);
    (void) close (sock);
    return (status);
    }

/*******************************************************************************
*
* routeEntryFill - fill in a route entry (struct ortentry)
*
* Fills in a route entry (struct ortentry) with destination and
* gateway information.
*
* RETURNS: N/A.
*
* NOMANUAL
*/

void routeEntryFill
    (
    struct ortentry *pRoute,     /* pointer to route entry */
    int destInetAddr,           /* destination internet address */
    int gateInetAddr,           /* gateway internet address */
    BOOL hostAddr		/* check host part of address */
    )
    {
    struct sockaddr_in *sin;
    struct in_addr di;

    /* zero out route entry */

    bzero ((caddr_t) pRoute, sizeof (struct ortentry));

    /* zero out sockaddr_in, fill in destination info */

    /* XXX rt_dst really a struct sockaddr */
    sin = (struct sockaddr_in *) &(pRoute->rt_dst);
    bzero ((caddr_t) sin, sizeof (struct sockaddr_in));
    sin->sin_family = AF_INET;
    sin->sin_len = sizeof (struct sockaddr_in);	
    sin->sin_addr.s_addr = (u_long)destInetAddr;

    /* zero out sockaddr_in, fill in gateway info */

    /* XXX rt_gateway really a struct sockaddr */
    sin = (struct sockaddr_in *) &(pRoute->rt_gateway);
    bzero ((caddr_t) sin, sizeof (struct sockaddr_in));
    sin->sin_family = AF_INET;
    sin->sin_len = sizeof (struct sockaddr_in);	
    sin->sin_addr.s_addr = (u_long)gateInetAddr;

    pRoute->rt_flags = RTF_UP;

    di.s_addr = (u_long)destInetAddr;

    /*
     *  if host part of address filled in, then it's a host route
     */

    if ((hostAddr) && (in_lnaof (di) != INADDR_ANY))
	pRoute->rt_flags |= RTF_HOST;

    /*
     *  if the gateway addr is not a local interface,
     *  then it is an indirect route - send it to a gateway.
     */
    if (ifa_ifwithaddr (&(pRoute->rt_gateway)) == NULL)
        pRoute->rt_flags |= RTF_GATEWAY;
    }


/*******************************************************************************
*
* mRouteAdd - add multiple routes to the same destination  
*
* This routine is similar to routeAdd(), except that you can use multiple
* mRouteAdd() calls to add multiple routes to the same location.  Use  
* <pDest> to specify the destination, <pGate> to specify the gateway to
* that destination, <mask> to specify destination mask, and <tos> to specify
* the type of service.  For <tos>, netinet/ip.h defines the following 
* constants as valid values: 
* 
*     IPTOS_LOWDELAY
*     IPTOS_THROUGHPUT
*     IPTOS_RELIABILITY
*     IPTOS_MINCOST
*
* Use <flags> to specify any flags 
* you want to associate with this entry.  The valid non-zero values 
* are RTF_HOST and RTF_CLONING defined in net/route.h.
*
* EXAMPLE
* To add a route to the 90.0.0.0 network through 91.0.0.3: 
* .CS
*     -> mRouteAdd ("90.0.0.0", "91.0.0.3", 0xffffff00, 0, 0);
* .CE
*
* Using mRouteAdd(), you could create multiple routes to the same destination.
* VxWorks would distinguish among these routes based on factors such as the 
* netmask or the type of service.  Thus, it is perfectly legal to say:
* .CS
*     -> mRouteAdd ("90.0.0.0", "91.0.0.3", 0xffffff00, 0, 0);
* 
*     -> mRouteAdd ("90.0.0.0", "91.0.0.254", 0xffff0000, 0, 0);
* .CE
* This adds two routes to the same network, "90.0.0.0", that go by two
* different gateways.  The differentiating factor is the netmask.
*
* This routine adds a route of type `M2_ipRouteProto_other', which is a
* static route.  This route will not be modified or deleted until a
* call to mRouteDelete() removes it.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: mRouteEntryAdd(), mRouteDelete(), routeAdd()
*/

STATUS mRouteAdd 
    (
    char *           pDest,    /* destination addr in internet dot notation */
    char *           pGate,    /* gateway address in internet dot notation */
    long             mask,     /* mask for destination */
    int              tos,      /* type of service */
    int              flags     /* route flags */ 
    )
    {
    long          destIp;
    long          gateIp;


    if (((destIp = (long) inet_addr (pDest)) == ERROR) ||
	 ((gateIp = (long) inet_addr (pGate)) == ERROR))
        {
        return (ERROR);
        }

    mask = htonl (mask);
    return (mRouteEntryAdd (destIp, gateIp, mask, tos, flags,
                            M2_ipRouteProto_other));
    }


    

/*******************************************************************************
*
* mPrivRouteEntryAdd - add a route to the routing table 
*
* A route to the destination <pDest> with gateway <pGate>, destination mask
* <mask>, and the type of service <tos> is added to the routing table.  <flags>
* specifies any flags associated with this entry.  Valid  values for <flags> 
* are 0, RTF_HOST and RTF_CLONING as  defined in net/route.h.  The <proto> 
* parameter identifies the protocol that generated this route.  Values for 
* <proto> may be found in m2Lib.h and <tos> is one of following values defined 
* in netinet/ip.h:
* 
*     IPTOS_LOWDELAY
*     IPTOS_THROUGHPUT
*     IPTOS_RELIABILITY
*     IPTOS_MINCOST
*
* This routine is the internal version of mRouteEntryAdd() and allows
* you to get a ptr to the routing entry added, which is not available
* with mRouteEntryAdd().
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS mPrivRouteEntryAdd 
    (
    long             destIp,    /* destination address, network order */
    long             gateIp,    /* gateway address, network order */
    long             mask,      /* mask for destination, network order */
    int              tos,       /* type of service */
    int              flags,     /* route flags */ 
    int              proto,      /* routing protocol */
    struct rtentry ** ppRtEntry  /* for returning ptr to new entry added */
    )
    {
    struct sockaddr_in      destSock;  /* destination socket */
    struct sockaddr_in      gateSock;  /* gateway socket */
    struct sockaddr_in      maskSock;  /* destination  mask socket */

    if ((mask != 0) && (flags & RTF_HOST)) 
	{
        return (ERROR);
	}
    bzero ((caddr_t)  & destSock, sizeof (destSock));
    bzero ((caddr_t)  & gateSock, sizeof (gateSock));
    bzero ((caddr_t)  & maskSock, sizeof (maskSock));

    destSock.sin_addr.s_addr = (mask != 0) ?  (destIp & mask) :
			       destIp;

    gateSock.sin_addr.s_addr = gateIp;
    destSock.sin_family = gateSock.sin_family = maskSock.sin_family = AF_INET;
    destSock.sin_len = gateSock.sin_len = maskSock.sin_len = sizeof (destSock);

    /* set gateway flag if not a direct route */

    if (ifa_ifwithaddr ((struct sockaddr *) & gateSock) == NULL)
        flags |= RTF_GATEWAY;

    if (mask == 0xffffffff)
        flags |= RTF_GATEWAY;

    maskSock.sin_addr.s_addr = mask;
    TOS_SET (&maskSock, 0x1f);
    in_socktrim (&maskSock);

    TOS_SET (&destSock, tos);
    RT_PROTO_SET (&destSock, proto);


    /*
     * Add the static route using the default weight value. Report the
     * change using both routing socket messages and direct callbacks.
     */

    return (rtrequestAddEqui ((struct sockaddr*) &destSock,
			      (struct sockaddr*) &maskSock,
			      (struct sockaddr*) &gateSock, flags, proto, 0,
			      TRUE, TRUE, (ROUTE_ENTRY**)ppRtEntry));
    }


/*******************************************************************************
*
* mRouteEntryAdd - add a protocol-specific route to the routing table 
*
* For a single destination <destIp>, this routine can add additional 
* routes <gateIp> to the routing table.  The different routes are 
* distinguished by the gateway, a destination mask <mask>, 
* the type of service <tos> and associated flag values <flags>.  
* Valid values for <flags> are 0, RTF_HOST, RTF_CLONING (defined in net/route.h).
* The <proto> parameter identifies the protocol that generated this route.  
* Values for <proto> may be found in m2Lib.h.  The <tos> parameter takes one of 
* following values (defined in netinet/ip.h): 
* 
*     IPTOS_LOWDELAY
*     IPTOS_THROUGHPUT
*     IPTOS_RELIABILITY
*     IPTOS_MINCOST
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: m2Lib.h, mRouteAdd(), mRouteDelete()
*/

STATUS mRouteEntryAdd 
    (
    long             destIp,    /* destination address, network order */
    long             gateIp,    /* gateway address, network order */
    long             mask,      /* mask for destination, network order */
    int              tos,       /* type of service */
    int              flags,     /* route flags */ 
    int              proto      /* routing protocol */
    )
    {
    return (mPrivRouteEntryAdd (destIp, gateIp, mask, tos, flags, proto,
                                NULL));
    }


/*******************************************************************************
*
* mRouteEntryDelete - delete route from the routing table 
*
* This routine deletes a protocol-specific route from the routing 
* table.  Specify the route using a destination <pDest>, a gateway <pGate>, 
* a destination mask <mask>, the type of service <tos>, a <flags> value, 
* and a <proto> value that identifies the routing protocol that added the 
* route.  The valid values for <flags> are 0 and RTF_HOST  (defined 
* in net/route.h).  Values for <proto> may be found in m2Lib.h and <tos> 
* is one of the following values defined in netinet/ip.h:
*
*     IPTOS_LOWDELA
*     IPTOS_THROUGHPU
*     IPTOS_RELIABILIT
*     IPTOS_MINCOST
*
* An existing route is deleted only if it is owned by the protocol specified 
* by <proto>.
*
* RETURNS: OK or ERROR.
*
*/

STATUS mRouteEntryDelete
    (
    long             destIp,    /* destination address, network order */
    long             gateIp,    /* gateway address, network order */
    long             mask,      /* mask for destination, network order */
    int              tos,       /* type of service */
    int              flags,     /* route flags */ 
    int              proto      /* routing protocol */
    )
    {

    struct sockaddr_in      destSock;  /* destination socket */
    struct sockaddr_in      gateSock;  /* gateway socket */
    struct sockaddr_in      maskSock;  /* destination  mask socket */

    if ((mask != 0) && (flags & RTF_HOST)) 
	{
        return (ERROR);
	}

    bzero ((caddr_t)  & destSock, sizeof (destSock));
    bzero ((caddr_t)  & gateSock, sizeof (gateSock));
    bzero ((caddr_t)  & maskSock, sizeof (maskSock));

    destSock.sin_addr.s_addr = (mask != 0) ?  (destIp & mask) :
			       destIp;
    gateSock.sin_addr.s_addr = gateIp;

    destSock.sin_family = gateSock.sin_family = maskSock.sin_family = AF_INET;
    destSock.sin_len = gateSock.sin_len = maskSock.sin_len = sizeof (destSock);


    /* set gateway flag if not a direct route */
    if (ifa_ifwithaddr ((struct sockaddr *) & gateSock) == NULL)
        flags |= RTF_GATEWAY;

    maskSock.sin_addr.s_addr = mask;
    TOS_SET (&maskSock, 0x1f);
    in_socktrim (&maskSock);

    if (mask == 0xffffffff)
        flags |= RTF_GATEWAY;

    TOS_SET (&destSock, tos);

    /*
     * Remove the matching route. Report the change using
     * both routing socket messages and direct callbacks.
     */

    /* check required to support: mRouteDelete() with routing extensions */
    if(gateIp==0)
        {
        return(rtrequestDelEqui((struct sockaddr*) &destSock,
                                (struct sockaddr*) &maskSock,
                                0, flags, proto, TRUE, TRUE, NULL));
        }
    else
        {
        return(rtrequestDelEqui((struct sockaddr*) &destSock,
                                (struct sockaddr*) &maskSock,
                                (struct sockaddr*) &gateSock, flags, proto,
                                TRUE, TRUE, NULL));
        }

    }

/*******************************************************************************
*
* mPrivRouteEntryDelete - delete a held route from the routing table 
*
* This routine deletes a protocol-specific route from the routing 
* table when mPrivRouteEntryAdd retained a copy of the new route
* with the <pRoute> pointer. The routine calls rtfree to adjust the
* reference count then passes the remaining arguments to the 
* mRouteEntryDelete routine to remove the route. 
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS mPrivRouteEntryDelete
    (
    long             destIp,    /* destination address, network order */
    long             gateIp,    /* gateway address, network order */
    long             mask,      /* mask for destination, network order */
    int              tos,       /* type of service */
    int              flags,     /* route flags */ 
    int              proto,     /* routing protocol */
    struct rtentry * pRoute
    )
    {
    int 	s;
    STATUS 	result;

    s = splnet ();

    if (pRoute)
        rtfree (pRoute);

    result = mRouteEntryDelete (destIp, gateIp, mask, tos, flags, proto);

    splx (s);

    return (result);
    }


/*******************************************************************************
*
* mRouteDelete - delete a route from the routing table 
*
* This routine deletes a routing table entry as specified by the
* destination, <pDest>, the destination mask, <mask>, and type of service,
* <tos>.  The <tos> values are as defined in the reference entry for
* mRouteAdd().
*
* EXAMPLE
* Consider the case of a route added in the following manner:
* .CS
*     -> mRouteAdd ("90.0.0.0", "91.0.0.3", 0xffffff00, 0, 0);
* .CE
* To delete a route that was added in the above manner, call mRouteDelete()
* as follows:
* .CS
*     -> mRouteDelete("90.0.0.0", 0xffffff00, 0);
* .CE
*
* If the netmask and or type of service do not match, the route
* is not deleted.
*
* The value of <flags> should be RTF_HOST for host routes, RTF_CLONING for
* routes which need to be cloned, and 0 in all other cases.
*
* RETURNS: OK or ERROR.  
*
* SEE ALSO: mRouteAdd()
*/

STATUS mRouteDelete
    (
    char *           pDest,    /* destination address */
    long             mask,     /* mask for destination */
    int              tos,      /* type of service */
    int              flags     /* either 0 or RTF_HOST */
    )
    {
    struct sockaddr_in      destSock;  /* destination socket */
    struct sockaddr_in      maskSock;  /* destination  mask socket */


    if ((mask != 0) && (flags & RTF_HOST)) 
	{
        return (ERROR);
	}
    bzero ((caddr_t)  & destSock, sizeof (destSock));
    bzero ((caddr_t)  & maskSock, sizeof (maskSock));

    if ((destSock.sin_addr.s_addr = (long) inet_addr (pDest)) == ERROR)
        {
        return (ERROR);
        }

    mask = htonl (mask);
    if (mask != 0)
        destSock.sin_addr.s_addr &= mask;

    destSock.sin_family  = maskSock.sin_family = AF_INET;
    destSock.sin_len =  maskSock.sin_len = sizeof (destSock);

    maskSock.sin_addr.s_addr = mask;
    TOS_SET (&maskSock, 0x1f);
    in_socktrim (&maskSock);

    TOS_SET (&destSock, tos);


    /*
     * Remove the matching route. Report the change using
     * both routing socket messages and direct callbacks.
     */

    return(rtrequestDelEqui((struct sockaddr*) &destSock,
                            (struct sockaddr*) &maskSock, NULL, flags,
                            M2_ipRouteProto_other, TRUE, TRUE, NULL));

    }

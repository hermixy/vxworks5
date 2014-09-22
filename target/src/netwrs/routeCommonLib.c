/* routeCommonLib.c - route interface shared code library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,09may02,niq  Moved routeAgeSet and routeMetric from routeEntryLib.c
01e,18mar02,vvv  fixed rtrequestAddEqui to set errno and return correct
		 value in case of error (SPR #74250)
01d,06mar02,vvv  set protocol id in route for host stack (SPR #73148)
01c,06aug01,spm  fixed netmask value for internal host-specific routes
01b,01aug01,spm  cleanup: removed unused prototype
01a,31jul01,spm  extracted from version 03s of routeEntryLib.c module
*/

/*
DESCRIPTION
This library contains the shared functions which both the host and router
versions of the network stack use internally to add and delete route entries.
The router stack product creates expanded versions of these routines which
create and send the internal callback messages for registered users. That
product also sends routing socket messages (if appropriate) when adding or
removing routes.

None of the routines in this library are published or available to customers.

INTERNAL
Various network libraries use these add and delete routines to insert and
remove route entries. The interfaces mimic the original rtrequest() entry
point from the BSD code, with some additions for features of the router
stack product.

NOMANUAL
*/

/* includes */

#include "vxWorks.h"

#include "routeEnhLib.h"

/* forward declarations */

#ifdef ROUTER_STACK
IMPORT STATUS _routeEntryAdd (ROUTE_DESC *, ROUTE_ENTRY **);
IMPORT STATUS _routeEntryDel (ROUTE_DESC *, ROUTE_ENTRY **);

IMPORT void routeCallBackSend (int, void *);
IMPORT STATUS routeSocketMessageSend (int, ROUTE_DESC *);
#endif /* ROUTER_STACK */

/*******************************************************************************
*
* rtrequestAddEqui - internal common entry point for route creation
*
* This routine replaces all references to the internal rtrequest() routine
* which the network stack used to generate routes when necessary. That
* routine allowed the stack to create internal routes for the local 
* interfaces (with the rtinit() routine) and to create routes in response
* to ICMP redirect messages. It also handled the add operation of routing
* sockets.
*
* All the previous published interfaces for creating static routes now use
* this routine as well. If available, it uses the router stack features to
* create a visible route or a duplicate entry as appropriate. Otherwise,
* it simply creates the initial route entry, or fails if that entry
* already exists.
*
* The router stack features use the <weight> parameter to determine the
* location of the new entry relative to any existing duplicate routes. If
* that weight is the minimum value, the new route will replace the current
* visible route, if any. The router stack also uses the <socketFlag> and
* <notifyFlag> parameters to determine which types of reports to send.
* The host stack variant does not use any of these parameters.
*
* When used within the routing socket output processing, the <socketFlag> is
* FALSE since that code generates the socket messages independently.
*
* When the routing table changes because an interface address is added or
* removed, both the <socketFlag> and <notifyFlag> parameters are FALSE 
* since the network stack calls the address update hook which generates both
* message types.
*
* In all other cases, both parameters are TRUE since the published routines
* for adding and deleting static routes must generate the routing socket
* messages and the callback notifications.
*
* RETURNS: OK on success, ERROR on failure
*
* NOMANUAL
*/

STATUS rtrequestAddEqui
    (
    struct sockaddr * pDstAddr,	/* destination address for route */
    struct sockaddr* pNetMask,  /* trimmed netmask, or 0 for host route */
    struct sockaddr* pGateway,  /* gateway address for new route */
    short flags, 		/* RTF_HOST and RTF_GATEWAY if needed */
    short protoId, 		/* source of routing entry, from m2Lib.h */
    UCHAR weight, 		/* administrative weight */
    BOOL notifyFlag,        /* TRUE: send callback message */
    BOOL socketFlag,        /* TRUE: send socket message */
    ROUTE_ENTRY** ppRouteEntry 	/* direct access to new route for caller */
    )
    {
#ifdef ROUTER_STACK                          /* original version */
    STATUS result;
    struct sockaddr dstAddr;
    struct sockaddr gateway;

    ROUTE_DESC routeDesc;
    bzero((char*)&routeDesc,sizeof(ROUTE_DESC));

    bcopy ( (char *)pDstAddr, (char *)&dstAddr, sizeof (struct sockaddr));
    bcopy ( (char *)pGateway, (char *)&gateway, sizeof (struct sockaddr));

#ifdef DEBUG
    logMsg("rtrequestAddEqui:starting\n",0,0,0,0,0,0);
#endif

    /*
     * Setup the descriptor so the original arguments are not modified.
     * The add operation changes the descriptor contents to create the
     * update messages.
     */

    routeDesc.pDstAddr = &dstAddr;
    routeDesc.pNetmask = pNetMask;
    routeDesc.pGateway = &gateway;
    routeDesc.flags = flags;
    routeDesc.protoId = protoId;
    if (weight == 0)
        routeDesc.weight = 100;   /* XXX: configure default value */
    else
        routeDesc.weight = weight;

    /*
     * Use the correct netmask for a host specific route. The rtinit()
     * routine supplies the interface's mask instead of this value.
     */

    if (flags & RTF_HOST)
        routeDesc.pNetmask = 0;

    /*
     * Create the route entry. If successful, the descriptor contains
     * the actual destination address of the route (with the netmask applied)
     * and the corresponding interface, as well as a flag setting which
     * indicates whether the new route is a duplicate entry. The gateway
     * address is not changed until after the routine completes.
     */

    result = _routeEntryAdd (&routeDesc, ppRouteEntry);

    if (result == ERROR)
        return (result);

    /* Announce the new route to any registered users, if appropriate. */

    if (notifyFlag)
        routeCallBackSend (ROUTE_ADD, &routeDesc);

    /*
     * Save the routing protocol's identifier in the gateway address
     * for any routing socket message.
     */

    RT_PROTO_SET(&gateway, protoId);

    /*
     * Transmit a routing socket message. This operation might occur
     * within the add routine if it is limited to representative nodes
     * which the routing sockets are currently able to modify.
     */

    if (socketFlag)    /* Transmit routing socket message? */
        {
        routeSocketMessageSend (RTM_ADD, &routeDesc);
        }

    /* XXX: should it return ERROR if a message isn't sent? */

    return (OK);
#else                                   /* Host version */
    struct sockaddr dstAddr;
    int    status;

    bcopy ((char *) pDstAddr, (char *) &dstAddr, sizeof (struct sockaddr));
    RT_PROTO_SET (&dstAddr, protoId);

    if ((status = rtrequest ((int)RTM_ADD, &dstAddr, pGateway, pNetMask, flags,
                             (struct rtentry **) ppRouteEntry)) != OK)
        {
	errno = status;
	return (ERROR);
	}

    return (OK);
#endif ROUTER_STACK
    }

/*******************************************************************************
*
* rtrequestDelEqui - internal common entry point for route deletion
*
* This routine replaces all references to the internal rtrequest() routine
* which the network stack used to remove routes when necessary. That
* routine allowed the stack to delete internal routes for the local 
* interfaces (with the rtinit() routine) when an address changed.
* It also handled the delete operation of routing sockets.
*
* All the previous published interfaces for deleting static routes now
* use this routine as well. If available, it uses the router stack features
* to delete the visible route or a duplicate entry as appropriate. Otherwise,
* it simply removes the initial route entry, or fails if that entry does
* not exist.
*
* The router stack features use the <socketFlag> and <notifyFlag> parameters
* to determine which types of reports to send. The host stack variant does
* not use either of those parameters.
*
* When used within the routing socket output processing, the <socketFlag> is
* FALSE since that code generates the socket messages independently.
*
* When the routing table changes because an interface address is added or
* removed, both the <socketFlag> and <notifyFlag> parameters are FALSE
* since the network stack calls the address update hook which generates both
* message types.
*
* In all other cases, both parameters are TRUE since the published routines
* for adding and deleting static routes must generate the routing socket
* messages and the callback notifications.
*
* RETURNS: OK on success, ERROR on failure
*
* NOMANUAL
*/

STATUS rtrequestDelEqui
    (
    struct sockaddr* pDstAddr, 	/* destination address for route */
    struct sockaddr* pNetMask, 	/* trimmed netmask, or 0 for host route */
    struct sockaddr* pGateway, 	/* gateway address for route, 0 for any */
    short flags, 		/* RTF_HOST or RTF_GATEWAY, if appropriate */
    short protoId, 		/* source of specific route, 0 for any */
    BOOL notifyFlag,        /* TRUE: send callback message */
    BOOL socketFlag,        /* TRUE: send socket message */
    ROUTE_ENTRY** ppRouteEntry 	/* direct access to (detached) route entry */
    )
    {
#ifdef ROUTER_STACK                          /* original version */
    STATUS result;
    struct sockaddr dstAddr;
    struct sockaddr gateway;

    ROUTE_DESC routeDesc;

    /*
     * Setup the descriptor so the original arguments are not modified.
     * The delete operation changes the descriptor contents to create the
     * update messages.
     */

    bzero ( (char*)&routeDesc, sizeof (ROUTE_DESC));

    bcopy ( (char *)pDstAddr, (char *)&dstAddr, sizeof (struct sockaddr));

    if (pGateway)    /* Set gateway to zero if not specified. */
        bcopy ( (char *)pGateway, (char *)&gateway, sizeof (struct sockaddr));
    else
        bzero ( (char*)&gateway, sizeof (struct sockaddr));

    routeDesc.pDstAddr = &dstAddr;
    routeDesc.pNetmask = pNetMask;
    routeDesc.pGateway = &gateway;
    routeDesc.flags = flags;
    routeDesc.protoId = protoId;

    /*
     * Use the correct netmask for a host specific route. The rtinit()
     * routine supplies the interface's mask instead of this value.
     */

    if (flags & RTF_HOST)
        routeDesc.pNetmask = 0;

    /*
     * Delete the matching route. If successful, the descriptor contains
     * the actual destination (generated from the netmask), matching gateway,
     * and all the metric values and additional information from the deleted
     * route entry. It also includes a flag setting which indicates whether
     * the deleted route was a duplicate entry.
     */

    result = _routeEntryDel (&routeDesc, ppRouteEntry);

    if (result == ERROR)
        return (result);

    /* Announce the route deletion to any registered users, if appropriate. */

    if (notifyFlag)
        routeCallBackSend (ROUTE_DEL, &routeDesc);

    /*
     * Save the routing protocol's identifier in the gateway address
     * for any routing socket message.
     */

    RT_PROTO_SET(&gateway, protoId);

    /*
     * Transmit a routing socket message. This operation might occur
     * within the preceding delete routine if it is limited to
     * representative nodes which the routing sockets are currently
     * able to modify.
     */

    if (socketFlag)    /* Transmit routing socket message? */
        {
        routeSocketMessageSend (RTM_DELETE, &routeDesc);
        }

    /* XXX: should it return ERROR if a message isn't sent? */

    return (OK);
#else                                   /* Host version */
    return (rtrequest((int)RTM_DELETE, pDstAddr, pGateway, pNetMask,flags,
                      (struct rtentry **)ppRouteEntry));
#endif ROUTER_STACK
    }

/******************************************************************************
*
* routeAgeSet - Set the age of the route entry
*
* This routine searches the routing table for an entry which matches the
* specified destination address. (For a Router stack, the protocol ID
* is also checked for a match; it is ignored for a Host stack).
* If the netmask is specified, the selected route also matches that value.
*
* Once a route is chosen, this routine sets the route age to the value
* specified by <newTime>.
*
* RETURNS: OK if a route is found and time set, or ERROR otherwise.
*
* NOMANUAL
*
* INTERNAL: This routine is currently called only by RIP
*/

STATUS routeAgeSet
    (
    struct sockaddr *	pDstAddr,	/* Destination address */
    struct sockaddr *	pNetmask,	/* Netmask */
    int			protoId,	/* Protocol ID */
    int			newTime		/* New time to set */
    )
    {
    int s;
#ifndef ROUTER_STACK
    register struct rtentry *	pRoute;
    struct radix_node_head *	rnh;

    /* First validate the parameters */

    if (pDstAddr == NULL)
        return (ERROR);

    s = splnet ();

    if ((rnh = rt_tables[pDstAddr->sa_family]) == 0) 
        {
        splx (s);
        return (ERROR);
        }
    if (pNetmask)
	{
	in_socktrim ((struct sockaddr_in *)pNetmask);
	TOS_SET (pNetmask, 0x1f);
	}
    if ((pRoute = (struct rtentry *) rnh->rnh_lookup(pDstAddr, 
                                                     pNetmask, rnh)))
            pRoute->rt_refcnt++;
#else
    ROUTE_ENTRY * 		pRoute;    /* Matching route, if any. */
    ULONG 			netmask;
    ULONG * 			pMask;

    /* First validate the parameters */

    if (pDstAddr == NULL || protoId == 0)
        return (ERROR);

    if (pNetmask)
        {
        netmask = ((struct sockaddr_in *)(pNetmask))->sin_addr.s_addr;
        pMask = &netmask;
        }
    else
        pMask = NULL;

    s = splnet ();

    semTake (routeTableLock, WAIT_FOREVER);
    pRoute = routeLookup (pDstAddr, pMask, protoId);
    semGive (routeTableLock);
#endif /* ROUTER_STACK */

    splx (s);

    if (pRoute == NULL)
        return (ERROR);

    /* Now set the age of the route */
    
#ifdef ROUTER_STACK
    pRoute->rtEntry.rt_mod = newTime;
#else
    pRoute->rt_mod = newTime;
#endif /* ROUTER_STACK */

    /* Release the reference when finished. */

    s = splnet ();

    rtfree ( (struct rtentry *)pRoute);

    splx (s);

    return (OK);
    }

/******************************************************************************
*
* routeMetricSet - Set the metric for the specified route entry
*
* This routine searches the routing table for an entry which matches the
* specified destination address. (For a Router stack, the protocol ID
* is also checked for a match; it is ignored for a Host stack).
* If the netmask is specified, the selected route also matches that value.
*
* Once a route is chosen, this routine sets the route metric to the value
* specified by <metric>.
*
* RETURNS: OK if a route is found and time set, or ERROR otherwise.
*
* NOMANUAL
*
* INTERNAL: This routine is currently called only by RIP
*/

STATUS routeMetricSet
    (
    struct sockaddr *	pDstAddr,	/* Destination address */
    struct sockaddr *	pNetmask,	/* Netmask */
    int			protoId,	/* Protocol ID */
    int			metric		/* metric to set */
    )
    {
    int s;
#ifndef ROUTER_STACK
    register struct rtentry *	pRoute;
    struct radix_node_head *	rnh;

    /* First validate the parameters */

    if (pDstAddr == NULL)
        return (ERROR);

    s = splnet ();

    if ((rnh = rt_tables[pDstAddr->sa_family]) == 0) 
        {
        splx (s);
        return (ERROR);
        }
    if (pNetmask)
	{
	in_socktrim ((struct sockaddr_in *)pNetmask);
	TOS_SET (pNetmask, 0x1f);
	}

    if ((pRoute = (struct rtentry *) rnh->rnh_lookup(pDstAddr, 
                                                     pNetmask, rnh)))
            pRoute->rt_refcnt++;
#else
    ROUTE_ENTRY * 		pRoute;    /* Matching route, if any. */
    ULONG 			netmask;
    ULONG * 			pMask;

    /* First validate the parameters */

    if (pDstAddr == NULL || protoId == 0)
        return (ERROR);

    if (pNetmask)
        {
        netmask = ((struct sockaddr_in *)(pNetmask))->sin_addr.s_addr;
        pMask = &netmask;
        }
    else
        pMask = NULL;

    s = splnet ();

    semTake (routeTableLock, WAIT_FOREVER);
    pRoute = routeLookup (pDstAddr, pMask, protoId);
    semGive (routeTableLock);
#endif /* ROUTER_STACK */

    splx (s);

    if (pRoute == NULL)
        return (ERROR);

    /* Now set the metric of the route */
    
#ifdef ROUTER_STACK
    pRoute->rtEntry.rt_rmx.rmx_hopcount = metric;
#else
    pRoute->rt_rmx.rmx_hopcount = metric;
#endif /* ROUTER_STACK */

    /* Release the reference when finished. */

    s = splnet ();

    rtfree ( (struct rtentry *)pRoute);

    splx (s);

    return (OK);
    }

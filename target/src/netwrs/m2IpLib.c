/* m2IpLib.c - MIB-II IP-group API for SNMP agents */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01o,06mar02,vvv  fixed m2IpAtransTblEntryGet to retrieve all entries
		 (SPR #72963)
01n,15oct01,rae  merge from truestack ver 02b, base 01k (SPRs 69556, 63629,
                 68525, 67888, etc)
01m,24oct00,niq  Merging RFC2233 changes from tor2_0.open_stack-f1 branch
                 01o,18may00,pul  modified m2IpRouteTblEntrySet() to pass 
                 FALSE to routeEntryFill to support addition of network routes.
                 01n,18may00,ann  merging from post R1 to include RFC2233
01l,07feb00,ann  changing the references to the code in m2IfLib
01k,16mar99,spm  recovered orphaned code from tor1_0_1.sens1_1 (SPR #25770)
01j,29sep98,spm  added support for dynamic routing protocols (SPR #9374)
01i,30dec97,vin  fixed SPR 20090
01h,26aug97,spm  removed compiler warnings (SPR #7866)
01g,17may96,rjc  routeEntryFill called with 1 less parmam. fixed.
01f,03apr96,rjc  set the m2RouteSem semaphore to NULL in in m2IpDelete
01e,25jan95,jdi  doc cleanup.
01d,11nov94,rhp  fixed typo in library man page
01c,11nov94,rhp  edited man pages
01b,03mar94,elh  modifed m2IpRouteTblEntrySet to return ERROR if rtioctl
		 failed.
01a,08dec93,jag  written
*/
/*
DESCRIPTION
This library provides MIB-II services for the IP group.  It provides
routines to initialize the group, access the group scalar variables, read
the table IP address, route and ARP table.  The route and ARP table can
also be modified.  For a broader description of MIB-II services,
see the manual entry for m2Lib.

To use this feature, include the following component:
INCLUDE_MIB2_IP

USING THIS LIBRARY
To use this library, the MIB-II interface group must also be initialized;
see the manual entry for m2IfLib.  This library (m2IpLib) can be
initialized and deleted by calling m2IpInit() and m2IpDelete()
respectively, if only the IP group's services are needed.  If full MIB-II
support is used, this group and all other groups can be initialized and
deleted by calling m2Init() and m2Delete().

The following example demonstrates how to access and change IP scalar
variables:
.CS
    M2_IP   ipVars;
    int     varToSet;

    if (m2IpGroupInfoGet (&ipVars) == OK)
	/@ values in ipVars are valid @/

    /@ if IP is forwarding packets (MIB-II value is 1) turn it off @/

    if (ipVars.ipForwarding == M2_ipForwarding_forwarding)
	{
				    /@ Not forwarding (MIB-II value is 2) @/
	ipVars.ipForwarding = M2_ipForwarding_not_forwarding;      
	varToSet |= M2_IPFORWARDING;
	}

    /@ change the IP default time to live parameter @/

    ipVars.ipDefaultTTL = 55;

    if (m2IpGroupInfoSet (varToSet, &ipVars) == OK)
	/@ values in ipVars are valid @/
.CE

The IP address table is a read-only table.  Entries to this table can be 
retrieved as follows:
.CS
    M2_IPADDRTBL ipAddrEntry;

    /@ Specify the index as zero to get the first entry in the table @/

    ipAddrEntry.ipAdEntAddr = 0;       /@ Local IP address in host byte order @/

    /@ get the first entry in the table @/

    if ((m2IpAddrTblEntryGet (M2_NEXT_VALUE, &ipAddrEntry) == OK)
	/@ values in ipAddrEntry in the first entry are valid  @/

    /@ Process first entry in the table @/

    /@ 
     * For the next call, increment the index returned in the previous call.
     * The increment is to the next possible lexicographic entry; for
     * example, if the returned index was 147.11.46.8 the index passed in the
     * next invocation should be 147.11.46.9.  If an entry in the table
     * matches the specified index, then that entry is returned.
     * Otherwise the closest entry following it, in lexicographic order,
     * is returned.
     @/

    /@ get the second entry in the table @/

    if ((m2IpAddrTblEntryGet (M2_NEXT_VALUE, &ipAddrEntryEntry) == OK)
	/@ values in ipAddrEntry in the second entry are valid  @/
.CE

The IP Address Translation Table (ARP table) includes the functionality of
the AT group plus additional functionality.  The AT group is supported
through this MIB-II table.  Entries in this table can be added and
deleted.  An entry is deleted (with a set operation) by setting the
`ipNetToMediaType' field to the MIB-II "invalid" value (2).  The 
following example shows how to delete an entry:
.CS
M2_IPATRANSTBL        atEntry;

    /@ Specify the index for the connection to be deleted in the table @/

    atEntry.ipNetToMediaIfIndex     = 1       /@ interface index @/

    /@ destination IP address in host byte order @/

    atEntry.ipNetToMediaNetAddress  = 0x930b2e08;

					    /@ mark entry as invalid @/
    atEntry.ipNetToMediaType        = M2_ipNetToMediaType_invalid; 

    /@ set the entry in the table @/

    if ((m2IpAtransTblEntrySet (&atEntry) == OK)
	/@ Entry deleted successfully @/
.CE

The IP route table allows for entries to be read, deleted, and modified.  This
example demonstrates how an existing route is deleted:
.CS
    M2_IPROUTETBL        routeEntry;

    /@ Specify the index for the connection to be deleted in the table @/
    /@ destination IP address in host byte order @/

    routeEntry.ipRouteDest       = 0x930b2e08;
						/@ mark entry as invalid @/
    routeEntry.ipRouteType       = M2_ipRouteType_invalid;

    /@ set the entry in the table @/

    if ((m2IpRouteTblEntrySet (M2_IP_ROUTE_TYPE, &routeEntry) == OK)
	/@ Entry deleted successfully @/
.CE

INCLUDE FILES: m2Lib.h
 
SEE ALSO:
m2Lib, m2SysLib, m2IfLib, m2IcmpLib, m2UdpLib, m2TcpLib
*/

/* includes */

#include "vxWorks.h"
#include "stdlib.h"
#include "m2Lib.h"
#include "private/m2LibP.h"
#include "netLib.h"
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <net/route.h>
#include <net/radix.h>
#include <netinet/in_var.h>
#include <netinet/if_ether.h>
#include <net/if_dl.h>
#include <net/if_arp.h>
#include "ioctl.h"
#include "net/mbuf.h"
#include "tickLib.h"
#include "sysLib.h"
#include "routeLib.h"
#include "semLib.h"
#include "errnoLib.h"
#include "memPartLib.h"
#include "routeEnhLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif /* VIRTUAL_STACK */

#ifdef ROUTER_STACK
#include "wrn/fastPath/fastPathLib.h"
#endif /* ROUTER_STACK */

/* defines */

#define M2_MAX_ROUTE_DEFAULT 	40

/* globals */

#ifndef VIRTUAL_STACK
LOCAL int 		m2RouteTableSaved;
LOCAL int		m2RouteTableSize;
LOCAL int		m2NumRouteEntries;
LOCAL M2_IPROUTETBL   * m2RouteTable;
LOCAL SEM_ID            m2RouteSem;
#endif /* VIRTUAL_STACK */

/*
 * The zero object id is used throught out the MIB-II library to fill OID 
 * requests when an object ID is not provided by a group variable.
 */

LOCAL M2_OBJECTID ipZeroObjectId = { 2, {0,0} };


/* forward declarations */

LOCAL int       m2RouteTableGet();
LOCAL int 	routeCacheInit (struct radix_node *rn, void *	pRtArg);

/* external declarations */

#ifndef VIRTUAL_STACK
extern SEM_ID		m2InterfaceSem;
IMPORT struct radix_node_head *rt_tables[]; /* table of radix nodes */
IMPORT struct	llinfo_arp llinfo_arp; 
IMPORT int  _ipCfgFlags; 
IMPORT int  ipMaxUnits;
#endif /* VIRTUAL_STACK */

IMPORT int 	arpioctl (int, caddr_t);
IMPORT void routeEntryFill (struct ortentry *, int, int, BOOL);

/******************************************************************************
*
* m2IpInit - initialize MIB-II IP-group access
*
* This routine allocates the resources needed to allow access to the MIB-II
* IP variables.  This routine must be called before any IP variables
* can be accessed.  The parameter <maxRouteTableSize> is used to increase the
* default size of the MIB-II route table cache.
*
* RETURNS:
* OK, or ERROR if the route table or the route semaphore cannot be allocated.
*
* ERRNO:
* S_m2Lib_CANT_CREATE_ROUTE_SEM
*
* SEE ALSO:
* m2IpGroupInfoGet(), m2IpGroupInfoSet(), m2IpAddrTblEntryGet(),
* m2IpAtransTblEntrySet(),  m2IpRouteTblEntryGet(), m2IpRouteTblEntrySet(), 
* m2IpDelete()
*/

STATUS m2IpInit
    (
    int		maxRouteTableSize 	/* max size of routing table */ 
    )
    {

    /* initialize the routing table stuff */

    if (m2RouteTable == NULL)
	{
	/* only initialized the first time called */

        m2RouteTableSaved = 0;

	m2RouteTableSize = (maxRouteTableSize == 0) ? 
			   M2_MAX_ROUTE_DEFAULT :  maxRouteTableSize ;

    	m2RouteTable = (M2_IPROUTETBL *) KHEAP_ALLOC(m2RouteTableSize *
				   	             sizeof (M2_IPROUTETBL));

	if (m2RouteTable == NULL)
	    {
	    return (ERROR);
	    }
        bzero ((char *)m2RouteTable, m2RouteTableSize * sizeof (M2_IPROUTETBL));
	}

        if (m2RouteSem == NULL)
            {
	    m2RouteSem = semMCreate (SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
				   SEM_DELETE_SAFE);
	    if (m2RouteSem == NULL)
		{
	        errnoSet (S_m2Lib_CANT_CREATE_ROUTE_SEM);
		return (ERROR);
		}
	    }

    (void) m2RouteTableGet (m2RouteTable, m2RouteTableSize);

    return (OK);
    }


/******************************************************************************
*
* m2IpGroupInfoGet - get the MIB-II IP-group scalar variables
*
* This routine fills in the IP structure at <pIpInfo> with the
* values of MIB-II IP global variables.
*
* RETURNS: OK, or ERROR if <pIpInfo> is not a valid pointer.
*
* ERRNO:
* S_m2Lib_INVALID_PARAMETER
*
* SEE ALSO: m2IpInit(), m2IpGroupInfoSet(), m2IpAddrTblEntryGet(),
* m2IpAtransTblEntrySet(), m2IpRouteTblEntryGet(), m2IpRouteTblEntrySet(),
* m2IpDelete()
*/

STATUS m2IpGroupInfoGet
    (
    M2_IP * pIpInfo	/* pointer to IP MIB-II global group variables */
    )
    {
 
    /* Validate Pointer to IP structure */
 
    if (pIpInfo == NULL)
	{
	errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
	}
 
    if (_ipCfgFlags & IP_DO_FORWARDING)
        pIpInfo->ipForwarding       = M2_ipForwarding_forwarding;
    else
        pIpInfo->ipForwarding       = M2_ipForwarding_not_forwarding;
 
    pIpInfo->ipDefaultTTL       = ipTimeToLive;

#ifdef VIRTUAL_STACK
    pIpInfo->ipInReceives       = _ipstat.ips_total;
 
    pIpInfo->ipInHdrErrors      = _ipstat.ips_badsum + _ipstat.ips_tooshort +
                                  _ipstat.ips_toosmall + _ipstat.ips_badhlen +
                                  _ipstat.ips_badlen + _ipstat.ips_badoptions +
				  _ipstat.ips_badvers;

    pIpInfo->ipInAddrErrors     = _ipstat.ips_cantforward;
    pIpInfo->ipForwDatagrams    = _ipstat.ips_forward;
    pIpInfo->ipReasmReqds       = _ipstat.ips_fragments;
    pIpInfo->ipReasmFails       = _ipstat.ips_fragdropped +
                                  _ipstat.ips_fragtimeout;
    pIpInfo->ipReasmOKs         = _ipstat.ips_reassembled; 
    pIpInfo->ipInDiscards       = _ipstat.ips_toosmall;
    pIpInfo->ipInUnknownProtos  = _ipstat.ips_noproto; 
    pIpInfo->ipInDelivers       = _ipstat.ips_delivered; 

    pIpInfo->ipOutRequests      = _ipstat.ips_localout;
    pIpInfo->ipFragOKs          = _ipstat.ips_fragmented;
    pIpInfo->ipFragFails        = _ipstat.ips_cantfrag;
    pIpInfo->ipFragCreates      = _ipstat.ips_ofragments;
    pIpInfo->ipOutDiscards      = _ipstat.ips_odropped;
    pIpInfo->ipOutNoRoutes      = _ipstat.ips_noroute;
#else
    pIpInfo->ipInReceives       = ipstat.ips_total;
 
    pIpInfo->ipInHdrErrors      = ipstat.ips_badsum + ipstat.ips_tooshort +
                                  ipstat.ips_toosmall + ipstat.ips_badhlen +
                                  ipstat.ips_badlen + ipstat.ips_badoptions +
				  ipstat.ips_badvers;
 
    pIpInfo->ipInAddrErrors     = ipstat.ips_cantforward;
    pIpInfo->ipForwDatagrams    = ipstat.ips_forward;
    pIpInfo->ipReasmReqds       = ipstat.ips_fragments;
    pIpInfo->ipReasmFails       = ipstat.ips_fragdropped +
                                  ipstat.ips_fragtimeout;
    pIpInfo->ipReasmOKs         = ipstat.ips_reassembled; 
    pIpInfo->ipInDiscards       = ipstat.ips_toosmall;
    pIpInfo->ipInUnknownProtos  = ipstat.ips_noproto; 
    pIpInfo->ipInDelivers       = ipstat.ips_delivered; 

    pIpInfo->ipOutRequests      = ipstat.ips_localout;
    pIpInfo->ipFragOKs          = ipstat.ips_fragmented;
    pIpInfo->ipFragFails        = ipstat.ips_cantfrag;
    pIpInfo->ipFragCreates      = ipstat.ips_ofragments;
    pIpInfo->ipOutDiscards      = ipstat.ips_odropped;
    pIpInfo->ipOutNoRoutes      = ipstat.ips_noroute;
#endif /* VIRTUAL_STACK */

    pIpInfo->ipReasmTimeout     = IPFRAGTTL;

 
    /* 
     * The MIB-II defines this variable to be the number of routing entries
     * that were discarded to free up buffer space.  The BSD VxWorks 
     * implementation does not free routes to free buffer space.
     */

    pIpInfo->ipRoutingDiscards  = 0;
 
    return (OK);
    }


/******************************************************************************
*
* m2IpGroupInfoSet - set MIB-II IP-group variables to new values
*
* This routine sets one or more variables in the IP group, as specified in the
* input structure <pIpInfo> and the bit field parameter <varToSet>.
*
* RETURNS: OK, or ERROR if <pIpInfo> is not a valid pointer, or <varToSet> has
* an invalid bit field.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_INVALID_VAR_TO_SET
*
* SEE ALSO: m2IpInit(), m2IpGroupInfoGet(), m2IpAddrTblEntryGet(), 
* m2IpAtransTblEntrySet(), m2IpRouteTblEntryGet(),
* m2IpRouteTblEntrySet(), m2IpDelete()
*/

STATUS m2IpGroupInfoSet
    (
    unsigned int varToSet,   /* bit field used to set variables */
    M2_IP * pIpInfo	     /* ptr to the MIB-II IP group global variables */
    )
    {
 
    /* Validate pointer and variable */
 
    if (pIpInfo == NULL ||
        (varToSet & (M2_IPFORWARDING | M2_IPDEFAULTTTL)) == 0)
	{
	if (pIpInfo == NULL)
	    errnoSet (S_m2Lib_INVALID_PARAMETER);
	else
	    errnoSet (S_m2Lib_INVALID_VAR_TO_SET);

        return (ERROR);
	}
 
    /*
     * This variable is toggle from NOT forwarding to forwarding, and vice versa
     */

    if (varToSet & M2_IPFORWARDING)
        {
        if (pIpInfo->ipForwarding == M2_ipForwarding_not_forwarding)
            _ipCfgFlags &= (~IP_DO_FORWARDING);
        else
            _ipCfgFlags |= IP_DO_FORWARDING; 

#ifdef ROUTER_STACK
        /* Inform Fastpath about the change */
 
        FFL_CALL (ffLibIpConfigFlagsChanged, (_ipCfgFlags));
#endif /* ROUTER_STACK */
        }
 
    /*
     * Set the new time.  The calling routine is expected to have verified that
     * the new time is a valid value.
     */

    if (varToSet & M2_IPDEFAULTTTL)
        ipTimeToLive = pIpInfo->ipDefaultTTL;
 
    return (OK);
    }
 

/******************************************************************************
*
* m2IpAddrTblEntryGet - get an IP MIB-II address entry
*
* This routine traverses the IP address table and does an M2_EXACT_VALUE or
* a M2_NEXT_VALUE search based on the <search> parameter.  The calling 
* routine is responsible for supplying a valid MIB-II entry index in the
* input structure <pIpAddrTblEntry>.  The index is the local IP
* address. The first entry in the table is retrieved by doing a NEXT search
* with the index field set to zero. 
*
* RETURNS: 
* OK, ERROR if the input parameter is not specified, or a match is not found.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*
* SEE ALSO:
* m2Lib, m2IpInit(), m2IpGroupInfoGet(), m2IpGroupInfoSet(), 
* m2IpAtransTblEntrySet(), m2IpRouteTblEntryGet(), m2IpRouteTblEntrySet(),
* m2IpDelete()
*/

STATUS m2IpAddrTblEntryGet
    (
    int            search,		/* M2_EXACT_VALUE or M2_NEXT_VALUE */ 
    M2_IPADDRTBL * pIpAddrTblEntry	/* ptr to requested IP address entry */
    )
    {
    struct in_ifaddr * pIfAddr;	       /* Pointer to IP list of internet addr */
    struct in_ifaddr * pIfAddrSaved;   /* Pointer to IP entry */
    unsigned long      ipAddrSaved;    /* IP address selected */
    unsigned long      ipAddr;         /* Caller IP address Table index req */
    unsigned long      currIpAddr;     /* Current IP address Table index req */
    int                netLock;     /* Used to secure the Network Code Access */
 
    /* Validate the input pointer */

    if (pIpAddrTblEntry == NULL)
	{
	errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
	}
 
    /* Setup variables to be used in the table search */

    pIfAddrSaved = NULL;			 /* Nothing found yeat */
    ipAddrSaved  = -1;				 /* Largest IP address */
    						 /* Requested IP address */
    ipAddr       = pIpAddrTblEntry->ipAdEntAddr; 
 
    netLock = splnet ();        /* Get exclusive access to Network Code */
 
    /* Search the IP list of internet addresses to satisfy the request */
 
#ifdef VIRTUAL_STACK
    for (pIfAddr = _in_ifaddr; pIfAddr != NULL; pIfAddr = pIfAddr->ia_next)
#else
    for (pIfAddr = in_ifaddr; pIfAddr != NULL; pIfAddr = pIfAddr->ia_next)
#endif /* VIRTUAL_STACK */
        {
	currIpAddr = ntohl((IA_SIN(pIfAddr)->sin_addr.s_addr));
 
        if (search == M2_EXACT_VALUE)
            {
            if (ipAddr == currIpAddr)
                {
		/* 
		 * Match found. Save a pointer to the entry and the ip address 
		 */

                pIfAddrSaved = pIfAddr;
                ipAddrSaved = currIpAddr;
                break;          /* Found EXACT Match */
                }
            }
        else
            {
	    /*
	     * A NEXT search is satisfied by an IP address lexicographicaly 
	     * greater than the input IP address.  Because IP addresses are not 
	     * in order in the IP list, the list must be traverse complety 
	     * before a selection is made.
	     */

            if (currIpAddr >= ipAddr && currIpAddr < ipAddrSaved)
                {
		/* Save possible IP address selection */

                pIfAddrSaved = pIfAddr;
                ipAddrSaved = currIpAddr;
                }
            }
        }

    /* Entry not found */

    if (pIfAddrSaved == NULL)
        {
        splx (netLock);         /* Give up exclusive access to Network Code */
	errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        return (ERROR);
        }
 
    /* Fill the request structure with the found entry */
 
    pIpAddrTblEntry->ipAdEntIfIndex      = pIfAddrSaved->ia_ifp->if_index;
    pIpAddrTblEntry->ipAdEntNetMask      = pIfAddrSaved->ia_subnetmask;
 
    if (pIfAddrSaved->ia_ifp->if_flags & IFF_BROADCAST)
        pIpAddrTblEntry->ipAdEntBcastAddr =
                            (ntohl(pIfAddrSaved->ia_netbroadcast.s_addr)) & 1;
    else
        pIpAddrTblEntry->ipAdEntBcastAddr = 1;
 
    splx (netLock);             /* Give up exclusive access to Network Code */
 
    pIpAddrTblEntry->ipAdEntAddr         =  ipAddrSaved;
    pIpAddrTblEntry->ipAdEntReasmMaxSize = IP_MAXPACKET;  /* Fromp ip.h */
 
    return (OK);
    }


/******************************************************************************
*
* m2IpAtransTblEntryGet - get a MIB-II ARP table entry
*
* This routine traverses the ARP table and does an M2_EXACT_VALUE or a
* M2_NEXT_VALUE search based on the <search> parameter.  The calling 
* routine is responsible for supplying a valid MIB-II entry index in the 
* input structure <pReqIpatEntry>.  The index is made up of the network
* interface index and the IP address corresponding to the physical address.
* The first entry in the table is retrieved by doing a NEXT search with the
* index fields set to zero. 
*
* RETURNS: 
* OK, ERROR if the input parameter is not specified, or a match is not found.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*
* SEE ALSO:
* m2Lib, m2IpInit(), m2IpGroupInfoGet(), m2IpGroupInfoSet(), 
* m2IpAtransTblEntrySet(), m2IpRouteTblEntryGet(), m2IpRouteTblEntrySet(),
* m2IpDelete()
*/

STATUS m2IpAtransTblEntryGet
    (
    int                   search,       /* M2_EXACT_VALUE or M2_NEXT_VALUE */ 
    M2_IPATRANSTBL      * pReqIpAtEntry	/* ptr to the requested ARP entry */
    )
    {
    unsigned long  ipAddrSaved;        	/* Used for a NEXT search */
    unsigned long  currIpAddr;          /* Current IP address Table index req */
    int            ix;                  /* All purpose loop Index */
    int            netLock;          /* Use to secure the Network Code Access */
    int            maxIndex;            /* max. possible index value */
    int            currIndex;           /* current interface index required */
    struct llinfo_arp * pLnkInfo; 
    struct rtentry *	pRtEnt;
    struct rtentry *	pRtEntMatch; 
 
    /* Validate Pointer to ARP request structure */
 
    if (pReqIpAtEntry == NULL)
	{
	errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
	}
 
    /* Initialize all local variable before the Network Semaphore is taken */
 
    ix          = 0;
    pRtEntMatch = NULL; 	/* initialized to NULL */
    maxIndex    = ipMaxUnits + 1;   /* largest possible value */
    ipAddrSaved = -1;		/* Largest IP address */
				/* Convert IP address to network byte order */
    netLock = splnet ();        /* Get exclusive access to Network Code */

    /* 
     * Traverse the ARP Table.  The whole table is searched
     */
#ifdef VIRTUAL_STACK
    pLnkInfo = _llinfo_arp.la_next;
    while (pLnkInfo != &_llinfo_arp) 
#else
    pLnkInfo = llinfo_arp.la_next;

    while (pLnkInfo != &llinfo_arp) 
#endif /* VIRTUAL_STACK */
	{
	pRtEnt = pLnkInfo->la_rt;
	pLnkInfo = pLnkInfo->la_next;
        currIpAddr = ntohl(((struct sockaddr_in *)
			    rt_key(pRtEnt))->sin_addr.s_addr);
        currIndex = pRtEnt->rt_ifp->if_index;

        if (search == M2_NEXT_VALUE)
            {
	    /*
	     * A NEXT search is satisfied by an IP address lexicographicaly 
	     * equal to or greater than the input IP address.  Because IP 
	     * addresses are not in order in the ARP table, the list must 
	     * be traverse completely before a selection is made.
	     */

            if (((currIndex > pReqIpAtEntry->ipNetToMediaIfIndex) ||
	         ((currIndex == pReqIpAtEntry->ipNetToMediaIfIndex) &&
		  (currIpAddr >= pReqIpAtEntry->ipNetToMediaNetAddress))) &&
		((currIndex < maxIndex) || ((currIndex == maxIndex) &&
		 (currIpAddr < ipAddrSaved))))
                {
		pRtEntMatch = pRtEnt; 	/* Found a Candidate */
                ipAddrSaved = currIpAddr;
                }
            }
        else
            {
            /* Search for an EXACT match in the ARP Table */
 
            if (pReqIpAtEntry->ipNetToMediaNetAddress == currIpAddr)
                {
		pRtEntMatch = pRtEnt; 	/* Found Requested Entry */
                ipAddrSaved = currIpAddr;
                break;
                }
            }
	}

    /* pRtEntMatch is pointer to the route for the IP address found 
     * The interface pointer is obtained from pRtEntMatch. This pointer is 
     *  used as a key to search the MIB-II interface table, 
     *  once the key is matched the interface number is found.
     */
 
    if (pRtEntMatch != NULL)
        {
	/* Search for Interface number in the MIB-II interface table */


        pReqIpAtEntry->ipNetToMediaIfIndex = pRtEntMatch->rt_ifp->if_index;
 
	/* Fill requested ARP entry */

        pReqIpAtEntry->ipNetToMediaNetAddress = ipAddrSaved;
 
        pReqIpAtEntry->ipNetToMediaType =
	    			   (pRtEntMatch->rt_rmx.rmx_expire == 0) ?
                                        M2_ipNetToMediaType_static :
                                        M2_ipNetToMediaType_dynamic;

        bcopy (LLADDR((struct sockaddr_dl *)pRtEntMatch->rt_gateway),
	       (char *) pReqIpAtEntry->ipNetToMediaPhysAddress.phyAddress,
		ETHERADDRLEN);
 
        splx (netLock); /* Give up exclusive access to Network Code */
 
        pReqIpAtEntry->ipNetToMediaPhysAddress.addrLength = ETHERADDRLEN;

        return (OK);
        }
 
    splx (netLock);             /* Give up exclusive access to Network Code */
    errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
    return (ERROR);
    }

/******************************************************************************
*
* m2IpAtransTblEntrySet - add, modify, or delete a MIB-II ARP entry
*
* This routine traverses the ARP table for the entry specified in the parameter
* <pReqIpAtEntry>.  An ARP entry can be added, modified, or deleted.  A  MIB-II
* entry index is specified by the destination IP address and the physical media
* address.  A new ARP entry can be added by specifying all the fields in the
* parameter <pReqIpAtEntry>. An entry can be modified by specifying the MIB-II
* index and the field that is to be modified.  An entry is deleted by 
* specifying the index and setting the type field in the input parameter 
* <pReqIpAtEntry> to the MIB-II value "invalid" (2).
*
* RETURNS: 
* OK, or ERROR if the input parameter is not specified, the physical address
* is not specified for an add/modify request, or the ioctl() request to the ARP
* module fails.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ARP_PHYSADDR_NOT_SPECIFIED
*
* SEE ALSO:
* m2IpInit(), m2IpGroupInfoGet(), m2IpGroupInfoSet(), 
* m2IpAddrTblEntryGet(), m2IpRouteTblEntryGet(), m2IpRouteTblEntrySet(),
* m2IpDelete()
*/

STATUS m2IpAtransTblEntrySet
    (
    M2_IPATRANSTBL     * pReqIpAtEntry  /* pointer to MIB-II ARP entry */
    )
    {
    int                  ioctlCmd;	/* ARP module IOCTL command value */
    struct arpreq        arpCmd;        /* ARP module IOCTL command structure */
    struct sockaddr_in * pIpAddr;	/* Pointer to an IP address */
    struct sockaddr    * pPhyAddr;	/* Pointer to an ethernet address */
 
    /* Validate Pointer to ARP request structure */
 
    if (pReqIpAtEntry == NULL)
	{
	errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
	}
 
    /* Initialize ARP module IOCTL variables */

    bzero ((char *)&arpCmd, sizeof(struct arpreq));
    pIpAddr = (struct sockaddr_in *) &arpCmd.arp_pa;    /* Protocol address */
 
    pIpAddr->sin_family      = AF_INET;
    pIpAddr->sin_len	     = sizeof(struct sockaddr_in); 	
    pIpAddr->sin_addr.s_addr = htonl(pReqIpAtEntry->ipNetToMediaNetAddress);
 
    /* Copy the Ethernet address in the IOCTL structure */

    if (pReqIpAtEntry->ipNetToMediaPhysAddress.addrLength > 0)
        {
        pPhyAddr = &arpCmd.arp_ha;
        pPhyAddr->sa_family = AF_UNSPEC;
 
        bcopy ((char *) pReqIpAtEntry->ipNetToMediaPhysAddress.phyAddress,
               pPhyAddr->sa_data,
               pReqIpAtEntry->ipNetToMediaPhysAddress.addrLength );
        }
 
    arpCmd.arp_flags = 0;
 
    /* Check if the ARP entry is to be deleted, Added or Modified */
 
    if (pReqIpAtEntry->ipNetToMediaType == M2_ipNetToMediaType_invalid)
        {
 
        /*
         * Request to DELETE the specified ARP entry.  The hardware address
         * is optional.  However, if the IP address is not found in the ARP
         * table the request to delete the ARP entry can fail.
         */
 
        ioctlCmd = SIOCDARP;
        }
    else
        {
 
        /*
         * Request to ADD or MODIFY the specified ARP entry.  The hardware
         * address must be specified.  If this is not the case then fail.
         */
 
        if (pReqIpAtEntry->ipNetToMediaPhysAddress.addrLength <= 0)
	    {
	    errnoSet (S_m2Lib_ARP_PHYSADDR_NOT_SPECIFIED);
	    return (ERROR);
	    }
 
        ioctlCmd = SIOCSARP;
 
        if (pReqIpAtEntry->ipNetToMediaType == M2_ipNetToMediaType_dynamic)
            arpCmd.arp_flags &= ~ATF_PERM;
 
 
        if (pReqIpAtEntry->ipNetToMediaType == M2_ipNetToMediaType_static)
            arpCmd.arp_flags |= ATF_PERM;
        }

	/* Issue IOCTL command to the ARP module */
 
        if (arpioctl (ioctlCmd, (caddr_t)&arpCmd) != 0)
            return (ERROR);
 
        return (OK);
    }

/******************************************************************************
*
* m2IpRouteTblEntryGet - get a MIB-2 routing table entry 
*
* This routine retrieves MIB-II information about an entry in 
* the network routing table and returns it in the caller-supplied structure 
* <pIpRouteTblEntry>.  
*
* The routine compares routing table entries to the address specified by the
* `ipRouteDest' member of the <pIpRouteTblEntry> structure, and retrieves an
* entry chosen by the <search> type (M2_EXACT_VALUE or M2_NEXT_VALUE, as
* described in the manual entry for m2Lib).
* 
* RETURNS: OK if successful, otherwise ERROR.
*
* ERRNO:
*   S_m2Lib_INVALID_PARAMETER	
*   S_m2Lib_ENTRY_NOT_FOUND
*    
* SEE ALSO: m2Lib, m2IpInit(), m2IpGroupInfoGet(), m2IpGroupInfoSet(), 
* m2IpAddrTblEntryGet(), m2IpRouteTblEntryGet(), m2IpRouteTblEntrySet(),
* m2IpDelete()
*/

STATUS m2IpRouteTblEntryGet 
    (
    int             search,		/* M2_EXACT_VALUE or M2_NEXT_VALUE */ 
    M2_IPROUTETBL * pIpRouteTblEntry	/* route table entry */
    )
    {
    int 		ix;
    int 		index;
    unsigned long	nextLarger;
    unsigned long	tableDest;
    unsigned long   	dstIpAddr;	

    /* Validate the arguments */

    if ((pIpRouteTblEntry == NULL) || ((search != M2_EXACT_VALUE) &&
				       (search != M2_NEXT_VALUE)))
	{
   	errnoSet(S_m2Lib_INVALID_PARAMETER);
	return (ERROR);				/* invalid argument */
	}

    dstIpAddr = pIpRouteTblEntry->ipRouteDest;

    semTake (m2RouteSem, WAIT_FOREVER);		/* protect the cache */

    /* Reread the route table if it has since been modified. */

    if (m2RouteTableGet (m2RouteTable, m2RouteTableSize) == ERROR)
	{
	errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        semGive (m2RouteSem);
	return (ERROR);				/* no such entry */
	}

    nextLarger = 0xffffffff;	/* will decrease to the proper route address */

    /* Find Match */

    for (ix = 0, index = m2NumRouteEntries; ix < m2NumRouteEntries; ix++)
	{
	/* XXX */
	tableDest = ntohl (m2RouteTable [ix].ipRouteDest);


	if (search == M2_EXACT_VALUE) 
	    {
	    if (dstIpAddr == tableDest)
		{
		index = ix;
	       	break;				/* found exact match */
	    	} 
	    }
	else 			/* (search == M2_NEXT_VALUE) */
	    {
	    /* Find the next dest value.  The alternative to 
	     * going through the entire array each time is to sort it. 
	     * Which is more expensive is dependent on how often 
	     * new routes get added to the table, how often this is used,
	     * and how long the list is.
	     */

	     if ((tableDest >= dstIpAddr) && (tableDest < nextLarger)) 
		{
		nextLarger = tableDest;
	    	index = ix;
		}
	    }
	}

    if (index >= m2NumRouteEntries)
	{
	errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        semGive (m2RouteSem);
	return (ERROR);				/* no such entry */
	}
  
    /* fill in return structure */    

    bcopy ((char *) &m2RouteTable [index], (char *) pIpRouteTblEntry, 
	   sizeof (M2_IPROUTETBL));

    pIpRouteTblEntry->ipRouteAge = 
      (tickGet () - pIpRouteTblEntry->ipRouteAge) / sysClkRateGet ();

    semGive (m2RouteSem);				/* release cache */

    pIpRouteTblEntry->ipRouteDest    = ntohl (pIpRouteTblEntry->ipRouteDest);
    pIpRouteTblEntry->ipRouteNextHop = ntohl (pIpRouteTblEntry->ipRouteNextHop);
    pIpRouteTblEntry->ipRouteMask    = ntohl (pIpRouteTblEntry->ipRouteMask);

    return (OK);
    }

/******************************************************************************
*
* m2IpRouteTblEntrySet - set a MIB-II routing table entry
*
* This routine adds, changes, or deletes a network routing table entry.  
* The table entry to be modified is specified by the `ipRouteDest' and
* `ipRouteNextHop' members of the <pIpRouteTblEntry> structure.
*
* The <varToSet> parameter is a bit-field mask that specifies which values
* in the route table entry are to be set.
*
* If <varToSet> has the M2_IP_ROUTE_TYPE bit set and `ipRouteType' has the
* value of M2_ROUTE_TYPE_INVALID, then the the routing table entry is
* deleted.
*
* If <varToSet> has the either the M2_IP_ROUTE_DEST, M2_IP_ROUTE_NEXT_HOP
* and the M2_IP_ROUTE_MASK bits set, then a new route entry is added 
* to the table. 
*
* RETURNS: OK if successful, otherwise ERROR.
*
* SEE ALSO:
* m2IpInit(), m2IpGroupInfoGet(), m2IpGroupInfoSet(), 
* m2IpAddrTblEntryGet(), m2IpRouteTblEntryGet(), m2IpRouteTblEntrySet(),
* m2IpDelete()
*/

STATUS m2IpRouteTblEntrySet 
    (
    int			varToSet,		/* variable to set */
    M2_IPROUTETBL * 	pIpRouteTblEntry	/* route table entry */
    )

    {
    struct sockaddr_in  ipRouteMask;
    struct ortentry     route;

    /* Convert from host order to network order */

    pIpRouteTblEntry->ipRouteDest    = htonl(pIpRouteTblEntry->ipRouteDest);
    pIpRouteTblEntry->ipRouteNextHop = htonl(pIpRouteTblEntry->ipRouteNextHop);

    bzero ((char *)&ipRouteMask, sizeof (struct sockaddr_in));
    ipRouteMask.sin_family = AF_INET;
    ipRouteMask.sin_len    = sizeof (struct sockaddr_in);

    /* delete the route */

    if ((varToSet & M2_IP_ROUTE_TYPE) && 
        (pIpRouteTblEntry->ipRouteType == M2_ipRouteType_invalid))

	{
        if (m2IpRouteTblEntryGet (M2_EXACT_VALUE, pIpRouteTblEntry) != OK)
            return (ERROR);        

        ipRouteMask.sin_addr.s_addr = (u_long)pIpRouteTblEntry->ipRouteMask;

        routeEntryFill (&route, pIpRouteTblEntry->ipRouteDest, 
                        pIpRouteTblEntry->ipRouteNextHop, FALSE);
        route.rt_flags |= RTF_MGMT;

        /*
         * Remove the matching route. Report the change using
         * both routing socket messages and direct callbacks.
         */

        return (rtrequestDelEqui (&route.rt_dst, 
                                  (struct sockaddr *)&ipRouteMask,
                                  &route.rt_gateway, route.rt_flags, 3,
                                  TRUE, TRUE, NULL)); 
	}

    /* otherwise change it */ 

    if (varToSet & (M2_IP_ROUTE_DEST | M2_IP_ROUTE_NEXT_HOP | 
                                                M2_IP_ROUTE_MASK))
	{
        /* Fill in the route mask */

        ipRouteMask.sin_addr.s_addr = (u_long)pIpRouteTblEntry->ipRouteMask;

        routeEntryFill (&route, pIpRouteTblEntry->ipRouteDest, 
                        pIpRouteTblEntry->ipRouteNextHop, FALSE);

        route.rt_flags |= RTF_MGMT;

        /*
         * Add the requested route using the default weight value. Report
         * the change using both routing socket messages and direct callbacks.
         */

        if (rtrequestAddEqui (&route.rt_dst, (struct sockaddr *)&ipRouteMask, 
                              &route.rt_gateway, route.rt_flags,
                              M2_ipRouteProto_netmgmt, 0, TRUE, TRUE,
                              NULL) != OK)
	    return (ERROR);
	}

    return (OK);
    }

/* generic structure for traversing the route table XXX move elsewhere */ 

typedef struct 
    {
    ULONG	arg1; 
    ULONG	arg2; 
    ULONG	arg3; 
    } RT_TBL_ARGS; 

/*
 * Function to pass to rn_walktree().
 * Return non-zero error to abort walk.
 */
LOCAL int routeCacheInit
    (
    struct radix_node *	rn,
    void *		pRtArg
    )
    {
    M2_IPROUTETBL *	pRouteCache; 		/* the route table cache */
    M2_IPROUTETBL *     pEntry;			/* pointer to route cache */
    int			routeCacheSize;		/* size of cache */
    int *		pRouteCacheIx; 
    struct sockaddr_in * pSin;			/* address pointer */
    struct rtentry *	pRoute = (struct rtentry *)rn;
    
    pRouteCache =(M2_IPROUTETBL *)((RT_TBL_ARGS *)pRtArg)->arg1; 
    routeCacheSize = (int )((RT_TBL_ARGS *)pRtArg)->arg2; 
    pRouteCacheIx = (int * )&((RT_TBL_ARGS *)pRtArg)->arg3; 

    if (*pRouteCacheIx >= routeCacheSize)	/* terminate the table walk */
	return (ENOMEM); 

    if ((pRoute->rt_flags & RTF_UP) == 0)	/* go to the next entry */
	return (OK);				/* route not up */

    /* Get the next entry to add */
    
    pEntry = &pRouteCache [*pRouteCacheIx];

    pSin = (struct sockaddr_in *) pRoute->rt_gateway;

    /* test whether the gateway is of type AF_LINK and if so 
     * test whether it is a real arp entry because interfaces 
     * initialized with RTF_CLONE flag have a dummy gateway of 
     * type AF_LINK. If it is a real arp entry then skip to next
     * entry, we are only dealing with gateways of type AF_INET
     */

    if (pRoute->rt_gateway->sa_family == AF_LINK)
	{
	if (((struct sockaddr_dl *)pSin)->sdl_alen)
	    pSin = NULL; 
	else 
	    pSin = (struct sockaddr_in *)pRoute->rt_ifa->ifa_addr; 
	}

    if (pSin == NULL)
	return (OK); 

    pEntry->ipRouteNextHop = pSin->sin_addr.s_addr;

    pSin = (struct sockaddr_in *) rt_key(pRoute);
    pEntry->ipRouteDest = pSin->sin_addr.s_addr;

    /* Loop through the interface table looking for a match
     * on the ifp. 
     */

    pEntry->ipRouteIfIndex = pRoute->rt_ifp->if_index;

    /* fill in the route mask */

    if (rt_mask(pRoute) == NULL)	/* implicit mask of 0xffffffff */
	pEntry->ipRouteMask = ~0L;	/* host route */

    else
	pEntry->ipRouteMask = ((struct sockaddr_in *)
			       rt_mask(pRoute))->sin_addr.s_addr; 

    /* XXX this may not be necessary condition taken care in the above else */
    if (pEntry->ipRouteDest == 0L)
	pEntry->ipRouteMask = 0L;		/* default route */

    pEntry->ipRouteAge = pRoute->rt_mod;	/* initialize last modified */

    /* is it a direct or indirect route ? */

    if (pRoute->rt_flags & RTF_GATEWAY) 
	{
	pEntry->ipRouteMetric1 =  1;	/* 1 means non local */
	pEntry->ipRouteType = M2_ipRouteType_indirect;
	}
    else
	pEntry->ipRouteType = M2_ipRouteType_direct;

    if (pRoute->rt_flags & RTF_MGMT)
        pEntry->ipRouteProto = M2_ipRouteProto_netmgmt;
    else if (pRoute->rt_flags & (RTF_DYNAMIC | RTF_MODIFIED))
	pEntry->ipRouteProto = M2_ipRouteProto_icmp;
    else
        {
        /* 
         * Routes created using the routeLib API store the correct protocol 
         * value using the RT_PROTO_SET macro. The RT_PROTO_GET macro 
         * retrieves that information. All routing protocols must alter
         * the kernel routing table using the routeLib API so that the 
         * IP group MIB variables can return the correct settings. Both RIP
         * and OSPF comply with this requirement.
         */

	pEntry->ipRouteProto = RT_PROTO_GET (pSin);
        if (pEntry->ipRouteProto == M2_ipRouteProto_rip)
            {
            /* 
             * The VxWorks RIP implementation stores the advertised metric
             * in the (normally unused) rmx_hopcount field of the kernel's 
             * routing table. Use that value instead of the simple 0/1 
             * boolean for local or remote routes assigned earlier.
             */

            pEntry->ipRouteMetric1 = pRoute->rt_rmx.rmx_hopcount;
            }
        else if (pEntry->ipRouteProto == 0)
            {
            /* 
             * Entries added through ioctl() calls on a routing socket or
             * otherwise created directly without using the API have an 
             * undefined value of 0 in the protocol type field. Replace it 
             * with the correct setting.
             */

            if (pRoute->rt_flags & RTF_LLINFO)
                {
                /* 
                 * Sanity check: handle route entries for link-level 
                 * protocols such as ARP in case they are not ignored 
                 * as expected.
                 */

	        pEntry->ipRouteProto = M2_ipRouteProto_other;
                }
            else
                {
                /* Handle protocol-independent locally generated entries. */

                pEntry->ipRouteProto = M2_ipRouteProto_local;
                }
            }
        }

    (*pRouteCacheIx)++;		/* increment the index */

    return (0);
    }


/******************************************************************************
*
* m2RouteTableGet - get a copy of the network routing table.
*
* This routine gets a copy of the network routing table and puts
* it into a pre-allocated route table cache.  The route table cache is
* specified by <pRouteCache> and the size of the cache is <routeCacheSize>.  
*
* RETURNS: the number of entries in the route table cache. 
*
* ERRNO
*   S_m2Lib_INVALID_PARAMETER
*/

LOCAL STATUS m2RouteTableGet 
    (
    M2_IPROUTETBL *	pRouteCache, 		/* the route table cache */
    int			routeCacheSize		/* size of cache */
    )
    {
    M2_IPROUTETBL *     pEntry;			/* pointer to route cache */
    int			routeCacheIx;		/* route cache index */
    int			spl;			/* for splnet */
    struct radix_node_head *	rnh;		/* pointer to radix node hd*/
    RT_TBL_ARGS			rtTblArgs;
     
    if (rtmodified == m2RouteTableSaved)
	return (OK);				/* cache still valid */

    /* Validate argument */

    if (pRouteCache == NULL)
	{       
	errnoSet (S_m2Lib_INVALID_PARAMETER);
	return (ERROR);    			/* bad parameter passed */
	}

    /* If the network interface table has been changed, re-read it */

    semTake (m2InterfaceSem, WAIT_FOREVER);	/* protect interface */

    /* Clear out the routing table and rebuild it */

    bzero ((char *) pRouteCache, routeCacheSize * (sizeof (M2_IPROUTETBL)));

    for (routeCacheIx = 0; routeCacheIx < routeCacheSize; routeCacheIx++) 
	{
	pEntry = &pRouteCache [routeCacheIx];

	/* fill in defaults */

    	pEntry->ipRouteMetric2 	= -1;
	pEntry->ipRouteMetric3 	= -1;
	pEntry->ipRouteMetric4 	= -1;
    	pEntry->ipRouteMetric5 	= -1;

	bcopy ((char *) &ipZeroObjectId, (char *) &pEntry->ipRouteInfo,
	       sizeof (ipZeroObjectId));
	}

    routeCacheIx = 0;

    rnh = rt_tables[AF_INET];

    if (rnh == NULL)
	return (0);

    rtTblArgs.arg1 = (ULONG) pRouteCache;
    rtTblArgs.arg2 = (ULONG) routeCacheSize; 
    rtTblArgs.arg3 = (ULONG) routeCacheIx; 

    spl = splnet ();		

    rn_walktree(rnh, routeCacheInit, (void *)&rtTblArgs);

    m2RouteTableSaved = rtmodified;
    m2NumRouteEntries = rtTblArgs.arg3;	/* Number of routes in the system */

    splx (spl);
    semGive (m2InterfaceSem);
    return (OK);		/* return the number of entries */
    }

/*******************************************************************************
*
* m2IpDelete - delete all resources used to access the IP group
*
* This routine frees all the resources allocated when the IP group was
* initialized.  The IP group should not be accessed after this routine has been
* called.
*
* RETURNS: OK, always.
*
* SEE ALSO: m2IpInit(), m2IpGroupInfoGet(), m2IpGroupInfoSet(), 
* m2IpAddrTblEntryGet(), m2IpAtransTblEntrySet(), m2IpRouteTblEntryGet(),
* m2IpRouteTblEntrySet()
*/

STATUS m2IpDelete (void)
   {
    /* Free route semaphore */

    if (m2RouteSem != NULL)
        {
        semDelete (m2RouteSem);
        m2RouteSem = NULL;
        }
 
    /* Free route table */

    if (m2RouteTable != NULL)
        {
        KHEAP_FREE((char *)m2RouteTable);
        m2RouteTable = NULL;
        }

   return (OK);
   }

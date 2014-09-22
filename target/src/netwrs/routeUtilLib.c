/* routeUtilLib.c - miscellaneous utilities for routing interface library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,07aug01,spm  fixed lookup routine to retrieve default route entry
01d,03aug01,spm  added flag to prevent incorrect interface reference count
01c,02aug01,spm  added virtual stack support
01b,02aug01,spm  cleanup: replaced duplicate copy routine with routeDescFill;
                 changed routeMetricInfoFromDescCopy to shorter name
01a,31jul01,spm  extracted from version 03s of routeEntryLib.c module
*/

/*
DESCRIPTION
This library contains miscellaneous routines which handle duplicate routes
to the same destination address. The router stack version of the network
stack uses these routines support the add, delete, and change operations.

None of the routines in this library are published or available to customers.

INTERNAL
All the routines within this library execute while the system holds the
internal table lock, if needed.

NOMANUAL
*/

#ifdef ROUTER_STACK              /* only build for router stack */

/* includes */

#include "vxWorks.h"
#include "memPartLib.h"     /* for KHEAP_FREE definition */

#include "routeEnhLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"    /* for rt_tables definition */
#endif

#define SOCKADDR_IN(s) (((struct sockaddr_in*)(s))->sin_addr.s_addr)

/* forward declarations */

IMPORT STATUS routeCallBackSend (int, void *);
IMPORT STATUS routeSocketMessageSend (int, ROUTE_DESC *);

void routeDescFill (ROUTE_ENTRY *, ROUTE_DESC *, BOOL);
void routeMetricsCopy (ROUTE_DESC *, ROUTE_ENTRY *);

ROUTE_ENTRY * routeLookup (struct sockaddr *, ULONG *, int);

/*******************************************************************************
*
* routeLookup - fetch an entry in the routing table
*
* This routine searches the routing table for an entry which matches the
* destination address and the <pNetmask> and <protoId> parameters, if
* provided. It returns a pointer to the route for later changes by
* public routines. A mask value of zero retrieves a host-specific route,
* if any.
*
* NOTE: If the <protoId> value is not zero, this routine searches the
*       duplicate routes for a matching entry. That situation requires
*       mutual exclusion protection with the internal lock. Otherwise,
*       no additional protection is needed.
*
* RETURNS: Pointer to matching route entry, or NULL if none.
*
* NOMANUAL
*/

ROUTE_ENTRY * routeLookup
    (
    struct sockaddr * pDest, 	/* IP address reachable with matching route */
    ULONG * pMask, 		/* netmask value, in network byte order */
    int	protoId 		/* route source from m2Lib.h, or 0 for any. */
    )
    {
    struct radix_node_head * pHead = rt_tables[pDest->sa_family];

    struct radix_node * pNode;        /* radix tree entry for route */
    struct rtentry * pRoute = NULL;   /* candidate for matching route */
    ROUTE_ENTRY * pMatch = NULL;      /* matching route entry */

    struct sockaddr_in netmask;       /* netmask value, internal format */
    char * pNetmask = NULL;           /* target netmask value from tree */

    struct rtentry * pNextRoute;

    STATUS result = ERROR;            /* OK indicates a successful search */
    int  s;

    if (pHead == NULL)
        {
        /* No route table exists for the given address family. */

        return (NULL);
        }

    bzero ( (char *)&netmask, sizeof (struct sockaddr_in));

    s = splnet ();

    /*
     * If a specific netmask match is required, find the target value
     * (a pointer to the stored netmask entry) which will determine
     * whether the match exists. A mask value of zero indicates a
     * host-specific route, which does not contain a netmask entry.
     */

    if (pMask && *pMask)
        {
        /* Setup data structure to search mask's tree for given value. */

        netmask.sin_family = AF_INET;
        netmask.sin_len = sizeof (struct sockaddr_in);
        netmask.sin_addr.s_addr = *pMask;

        TOS_SET (&netmask, 0x1f);

        in_socktrim (&netmask);    /* Adjust length field for tree search. */

        /* Search for netmask from corresponding tree. */

        pNode = rn_addmask (&netmask, 1, pHead->rnh_treetop->rn_off);
        if (pNode == 0)
            {
            /* No route currently uses the specified netmask. */

#ifdef DEBUG
            logMsg ("routeLookup: requested mask does not exist.\n",
                    0, 0, 0, 0, 0, 0);
#endif
            splx(s);
            return (NULL);
            }

        pNetmask = pNode->rn_key;
        }

    pNode = pHead->rnh_matchaddr ((caddr_t)pDest, pHead);
    if (pNode && ((pNode->rn_flags & RNF_ROOT) == 0))
        {
        /* Possible match found. Save for later use. */

        pRoute = (struct rtentry *)pNode;
        }
    else
        {
        /* No route matches the given key. */

        splx (s);
        return (NULL);
        }

#ifdef DEBUG
    logMsg ("routeLookup: candidate for match at %p.\n", pNode,
            0, 0, 0, 0, 0);
#endif

    /*
     * Compare the set of routes available with the initial key
     * against the desired values. Each entry in the chain of
     * routes with duplicate keys uses a different netmask value.
     *
     * NOTE: The pNode value is not necessarily the start of the
     * chain. It is the first entry in the chain with a short
     * enough netmask to produce a match against the destination.
     */

    for ( ; pRoute != NULL; pRoute = pNextRoute)
        {
        /* Select the next route candidate in case a test fails. */

        pNextRoute = (struct rtentry *)
                           ((struct radix_node *)pRoute)->rn_dupedkey;

        if (pMask)
            {
            /*
             * Check mask of route against corresponding entry from
             * the stored netmasks (derived from the given value).
             */

#ifdef DEBUG
            logMsg ("routeLookup: checking against specific mask.\n",
                    0, 0, 0, 0, 0, 0);
#endif

            if (*pMask)
                {
                if ( ((struct radix_node *)pRoute)->rn_mask != pNetmask)
                    continue;   /* Mask values do not match. */
                }
            else if ( ( (struct sockaddr_in *)pDest)->sin_addr.s_addr)
                {
                /* Searching for a host-specific route (no netmask entry). */

                if ( ((struct radix_node *)pRoute)->rn_mask != 0)
                    continue;   /* Entry is not a host route. */
                }
            }

        /*
         * Candidate passed any mask requirements. Search for entries
         * which match the specified route source.
         */

#ifdef DEBUG
        logMsg ("routeLookup: Current mask is OK.\n", 0, 0, 0, 0, 0, 0);
#endif

        if (protoId)
            {
            /* Check source of route against specified value. */

#ifdef DEBUG
            logMsg ("routeLookup: testing protocol ID.\n",
                    0, 0, 0, 0, 0, 0);
#endif

            for (pMatch = (ROUTE_ENTRY *)pRoute; pMatch != NULL;
                   pMatch = pMatch->diffNode.pFrwd)
                {
                if (RT_PROTO_GET(ROUTE_ENTRY_KEY(pMatch)) == protoId)
                    break;
                }

            if (pMatch == NULL)   /* Route protocol values do not match. */
                continue;

#ifdef DEBUG
            logMsg ("routeLookup: Current protocol ID is OK.\n",
                    0, 0, 0, 0, 0, 0);
#endif
            }
        else
            {
            /*
             * No route source is specified. Accept the entry
             * which met any mask criteria.
             */

            pMatch = (ROUTE_ENTRY *)pRoute;
            }
 
        /* The candidate route entry met all criteria. Stop the search. */

        result = OK;
        break;
        }

    if (result == OK)
        {
        /* Increase the reference count before returning the matching entry. */

        pMatch->rtEntry.rt_refcnt++;
        }
    else
        pMatch = NULL;

    splx (s);

    return (pMatch);
    }

/*******************************************************************************
*
* routeEntryFind - find a route entry and the appropriate location
*
* This routine searches the extended lists attached to the <pRoute>
* entry which is visible to the IP forwarding process. It retrieves
* any route entry which matches the supplied protocol identifier and
* gateway. If the protocol is not specified, the routine selects an
* entry in the first protocol group. If the gateway is not specified,
* it retrieves the initial route entry in the selected group.
*
* Both the change and delete operations require a matching route. The add
* operation fails in this situation, since the gateway and protocol 
* identifier must be unique for a particular destination address and netmask.
*
* This routine also provides the adjacent entries for a route based on
* the specified weight value, if pointers to retrieve those entries are
* available. The first entry in a group uses a lower weight than all
* subsequent entries from the same protocol. It is connected to the initial
* entries from other groups and t * next entry with the same protocol type.
* Subsequent entries in a group are only connected to successors and
* predecessors within that group.
*
* The add and change operations use this neighboring route information
* to store a new or altered entry at the correct location. The delete
* operation uses a <weight> value of 0 to skip the search for neighboring
* routes since it does not provide pointers to retrieve those values.
*
* NOTE: This routine requires the mutual exclusion protection for
*       the routing table to prevent moving or replacing the matching
*       route's adjacent entries.
*
* RETURNS: Pointer to existing route, or NULL if none.
*
* NOMANUAL
*/

ROUTE_ENTRY * routeEntryFind
    (
    ROUTE_ENTRY * pRoute, 	/* primary route for destination/netmask */
    short protoId, 		/* protocol identifer for route */
    struct sockaddr * pGateway,	/* next hop gateway for route */
    ROUTE_ENTRY ** ppGroup, 	/* initial entry (head) of existing group */
    ROUTE_ENTRY ** ppLastGroup,	/* predecessor if head of group changes */
    ROUTE_ENTRY ** ppNextGroup, /* successor if head of group changes */
    ROUTE_ENTRY ** ppLast, 	/* previous entry in matching group */
    ROUTE_ENTRY ** ppNext, 	/* next entry in matching group */
    UCHAR weight 		/* weight value for route, 0 for delete */
    )
    {
    ROUTE_ENTRY * pGroup;    /* initial route entry with matching protocol */
    ROUTE_ENTRY * pMatch = NULL;  /* duplicate route entry, if any. */

    ROUTE_ENTRY * pNextGroup = NULL;  /* Group with higher minimum weight. */
    ROUTE_ENTRY * pLastGroup = NULL;  /* Previous group entry in list. */
    ROUTE_ENTRY * pBack = NULL;   /* Preceding route entry in group. */
    ROUTE_ENTRY * pNext = NULL;   /* Next route entry in group. */

    ROUTE_ENTRY * pTemp;          /* Loop index. */

    BOOL gatewayFlag;    /* Gateway value specified? */

    UCHAR nextWeight;    /* Weight for initial group entry after increase. */

    if (pGateway == NULL || SOCKADDR_IN (pGateway) == 0)
        gatewayFlag = FALSE;
    else
        gatewayFlag = TRUE;
 
    /*
     * Find the group which matches the protocol for the targeted route,
     * if any. Use the first group if no value is specified. This search
     * also finds an adjacent group if no match occurs or if the protocol
     * does not match the first group and the provided weight value is less
     * than the initial entry in the matching group.
     */

    for (pGroup = pRoute; pGroup != NULL; pGroup = pGroup->diffNode.pFrwd)
        {
        if (protoId == 0)    /* No protocol specified - use initial group. */
            break;

        if (RT_PROTO_GET(ROUTE_ENTRY_KEY(pGroup)) == protoId)
            break;

        /*
         * If the weight value for the targeted route is less than
         * the current minimum in an earlier group (which therefore
         * uses a lower weight than the matching group), save the
         * new successor for the matching group. Ignore this test
         * for the delete operation, which selects a new adjacent
         * group separately, if needed.
         */

        if (weight && pNextGroup == NULL &&
                weight < pGroup->rtEntry.rt_rmx.weight)
            {
            pNextGroup = pGroup;

#ifdef DEBUG
            logMsg ("routeEntryFind: inserting ahead of group at %x.\n",
                    pNextGroup, 0, 0, 0, 0, 0);
#endif
            }

        /*
         * Save the current group entry, which will eventually equal the
         * tail of the list if no matching group exists. That final entry
         * is required when creating a new group with a larger minimum
         * weight than all existing groups.
         */

        pLastGroup = pGroup;
        }

    /*
     * Assign the preceding group if the weight value inserts a new group
     * into the list or promotes an existing group to an earlier position.
     */

    if (pNextGroup)
        {
        pLastGroup = pNextGroup->diffNode.pBack;
        }

    /*
     * Find the specific route entry which matches the supplied gateway
     * address within the selected group. Use the initial entry if no
     * value is specified. The delete and change operations require a
     * match, but the add operation fails in that case.
     *
     * This search also finds the neighboring routes within the matching
     * group based on any specified weight value.
     */

    for (pTemp = pGroup; pTemp != NULL; pTemp = pTemp->sameNode.pFrwd)
        {
        /*
         * Save the first entry which matches the specified gateway.
         * The second condition is not strictly necessary, since each
         * gateway value is unique, but bypasses needless tests once
         * a match is found.
         */

        if (gatewayFlag && pMatch == NULL)
            {
            if (ROUTE_ENTRY_GATEWAY (pTemp)->sa_family == AF_LINK)
                {
                /*
                 * The gateway field contains an ARP template. The entry
                 * only matches for a search using the interface's assigned
                 * IP address as the gateway.
                 */

                if (SOCKADDR_IN (pTemp->rtEntry.rt_ifa->ifa_addr)
                                        == SOCKADDR_IN (pGateway))
                    {
                    /* Specified route entry exists - save the match. */

                    pMatch = pTemp;
                    }
                }
            else if (SOCKADDR_IN (ROUTE_ENTRY_GATEWAY (pTemp))
                                        == SOCKADDR_IN (pGateway))
                {
                /* Specified route entry exists - save the match. */

                pMatch = pTemp;
                }
            }

        if (gatewayFlag == FALSE && pMatch == NULL) 
            {
            /* Use the initial entry if no gateway is specified. */

            pMatch = pGroup;
            }

        /*
         * The delete operation does not provide a weight value. Save
         * the neighboring route for the add and change operations.
         */

        if (weight && pNext == NULL &&
                weight < pTemp->rtEntry.rt_rmx.weight)
            {
            pNext = pTemp;
            }

        /* Halt the search after finding the matching entry and location. */

        if (pMatch && pNext)
            break;

        /*
         * Save the current route, which eventually stores the last
         * entry in the list. That value is only needed if the weight
         * is larger than all existing entries in the group.
         */

        pBack = pTemp;
        }

    /*
     * Save the correct predecessor if the weight does not place the
     * corresponding route at the end of the list. That value will
     * equal NULL if the new position is the head of the list.
     *
     * Otherwise, the preceding entry already equals the last item in the
     * list (which occurs when appending a new route with the maximum weight
     * to an existing group) or remains NULL (if the group does not exist).
     * No correction is needed in either case.
     */

    if (pNext)
        {
        pBack = pNext->sameNode.pBack;
        }

    /*
     * Update the chosen adjacent group, if needed. The results of the
     * previous search are only valid if the weight value corresponds
     * to the initial entry in a group, the selected route is not part
     * of the first group, and the weight is reduced enough to alter
     * the relative position of the selected group.
     */

    if (pNext == pGroup)
        {
        /*
         * The specified weight corresponds to the initial entry in
         * a group, so either the weight of the group is reduced or it
         * corresponds to the first entry in a new group. Update the
         * neighboring groups selected with the earlier search if needed.
         */

        if (pGroup && pNextGroup == NULL)
            {
            /*
             * The group exists and no successor is assigned, so the
             * earlier search found a matching group before detecting
             * a different group with a higher weight than the specified
             * value. This condition indicates that the reduced weight does
             * not change the relative location of the matching group.
             */

            pNextGroup = pGroup->diffNode.pFrwd;
            pLastGroup = pGroup->diffNode.pBack;
            }
        }

    /*
     * Finish assigning the adjacent group, if needed. The earlier search
     * only selects an adjacent group when the specified weight value is
     * less than the weight of the initial entry in the matching group or
     * is the first entry in a new group.
     */

    if (pNext != pGroup && weight)
        {
        /*
         * Find the correct adjacent group for an unchanged weight or an
         * increase in the weight value of an entry in an existing group.
         */

        if (pMatch == NULL || pMatch != pGroup)
            {
            /*
             * If the selected route is not the initial entry, it is
             * not connected to any adjacent groups.
             */

            pLastGroup = pNextGroup = NULL;
            }
        else if (weight == pMatch->rtEntry.rt_rmx.weight)
            {
            /*
             * The earlier search only finds an adjacent group when the
             * weight of the group is reduced. Set the correct adjacent
             * groups since the location in the group list is unchanged.
             */

            pNextGroup = pGroup->diffNode.pFrwd;
            pLastGroup = pGroup->diffNode.pBack;
            }
        else
            {
            /*
             * The initial entry in the group list uses an increased
             * weight. Set the correct weight value and find the new
             * adjacent group.
             */

            pTemp = pMatch->sameNode.pFrwd;
            nextWeight = weight;

            /*
             * Use the weight value of the next initial group entry if 
             * the new weight will change the location of the current
             * initial entry in the group.  
             */

            if (pTemp && weight > pTemp->rtEntry.rt_rmx.weight)
                nextWeight = pTemp->rtEntry.rt_rmx.weight;

            if (pMatch->rtEntry.rt_rmx.weight == nextWeight)
                {
                /*
                 * The next entry in the list used the same weight
                 * as the initial entry, so the position is unchanged.
                 */

                pNextGroup = pGroup->diffNode.pFrwd;
                pLastGroup = pGroup->diffNode.pBack;
                }
            else
                {
                /*
                 * The minimum weight of the group increased. Find the
                 * new location.
                 */

                for (pTemp = pGroup; pTemp != NULL;
                         pTemp = pTemp->diffNode.pFrwd)
                    {
                    if (nextWeight < pTemp->rtEntry.rt_rmx.weight)
                        {
                        pNextGroup = pTemp;
                        break;
                        }

                    /*
                     * Save the current entry in case the next entry
                     * is the correct successor.
                     */

                    pLastGroup = pTemp;
                    }

                /*
                 * Set the preceding group to the correct value if the
                 * increased weight does not change the relative location
                 * of the group.
                 */

                if (pLastGroup == pGroup)
                    pLastGroup = pGroup->diffNode.pBack;
                }
            }
        }

    /*
     * Correct the adjacent route within the matching group, if needed.
     * That search places a route after any other entries with the same
     * weight. This behavior is not correct when the weight of an existing
     * entry does not change.
     */

    if (pMatch && weight == pMatch->rtEntry.rt_rmx.weight)
        {
        pNext = pMatch->sameNode.pFrwd;
        pBack = pMatch->sameNode.pBack;
        }

    /*
     * Save the initial entry in the group and any adjacent entries in
     * the supplied parameters. The delete operation does not retrieve
     * any neighboring routes or specify the weight needed to assign
     * those values.
     */

    if (ppGroup)
        *ppGroup = pGroup;

    if (ppLastGroup)
        *ppLastGroup = pLastGroup;

    if (ppNextGroup)
        *ppNextGroup = pNextGroup;

    if (ppLast)
        *ppLast = pBack;

    if (ppNext)
        *ppNext = pNext;

    return (pMatch);
    }

/*******************************************************************************
*
* routeDescFill - copies the relevant routing information 
*
* This routine copies routing information from a ROUTE_ENTRY structure
* to the corresponding fields in a ROUTE_DESC structure. The callback
* messages and the results of the public lookup routine use the resulting
* structure. The delete operation provides a <copyAllFlag> parameter of
* FALSE since it already assigns the destination and netmask of the given
* <pRouteDesc> structure. All other operations set that parameter to TRUE
* to get all relevant values from the route entry which the <pRoute>
* parameter provides.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void routeDescFill
    (
    ROUTE_ENTRY * pRouteEntry, 	/* reference to existing route */
    ROUTE_DESC * pRouteDesc, 	/* buffers for route information */
    BOOL copyAllFlag 		/* copy all information? */
    )
    {
    if (copyAllFlag)
        {
        if (ROUTE_ENTRY_KEY (pRouteEntry) != NULL)
            bcopy ( (char*) (ROUTE_ENTRY_KEY (pRouteEntry)),
                   (char*) (pRouteDesc->pDstAddr), sizeof (struct sockaddr));

        if ( (ROUTE_ENTRY_MASK (pRouteEntry) != NULL) &&
            (pRouteDesc->pNetmask != 0))
            bcopy ( (char*)ROUTE_ENTRY_MASK (pRouteEntry),
                   (char*)pRouteDesc->pNetmask, sizeof (struct sockaddr));
        else  /* If it is a host route, set netmask to NULL */
            pRouteDesc->pNetmask = NULL;
        }

    if ( (ROUTE_ENTRY_GATEWAY (pRouteEntry))->sa_family == AF_LINK)
	{
        bcopy ( (char*)pRouteEntry->rtEntry.rt_ifa->ifa_addr,
               (char*)pRouteDesc->pGateway, sizeof (struct sockaddr));
        }
    else
	{
        bcopy ( (char*)ROUTE_ENTRY_GATEWAY (pRouteEntry),
               (char*)pRouteDesc->pGateway, sizeof (struct sockaddr));
        }

    pRouteDesc->flags = ROUTE_ENTRY_FLAGS (pRouteEntry);
    pRouteDesc->protoId = RT_PROTO_GET (ROUTE_ENTRY_KEY (pRouteEntry));
    pRouteDesc->pIf = pRouteEntry->rtEntry.rt_ifp;

    pRouteDesc->value1 = pRouteEntry->rtEntry.rt_rmx.value1;
    pRouteDesc->value2 = pRouteEntry->rtEntry.rt_rmx.value2;
    pRouteDesc->value3 = pRouteEntry->rtEntry.rt_rmx.value3;
    pRouteDesc->value4 = pRouteEntry->rtEntry.rt_rmx.value4;
    pRouteDesc->value5 = pRouteEntry->rtEntry.rt_rmx.value5;
    pRouteDesc->routeTag = pRouteEntry->rtEntry.rt_rmx.routeTag;
    pRouteDesc->weight = pRouteEntry->rtEntry.rt_rmx.weight;

    pRouteDesc->primaryRouteFlag = pRouteEntry->primaryRouteFlag;

    return;
    }

/*******************************************************************************
*
* routeMetricsCopy - copies the extra metric information
*
* This routine copies all the additional metric information from
* a ROUTE_DESC structure to a ROUTE_ENTRY structure. It completes
* the modification of an existing entry and stores the initial data
* when creating a new route entry.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void routeMetricsCopy
    (
    ROUTE_DESC * pRouteDesc, 	/* buffers with new settings for route */
    ROUTE_ENTRY * pRouteEntry 	/* reference to existing route entry */
    )
    {
    pRouteEntry->rtEntry.rt_rmx.value1 = pRouteDesc->value1;
    pRouteEntry->rtEntry.rt_rmx.value2 = pRouteDesc->value2;
    pRouteEntry->rtEntry.rt_rmx.value3 = pRouteDesc->value3;
    pRouteEntry->rtEntry.rt_rmx.value4 = pRouteDesc->value4;
    pRouteEntry->rtEntry.rt_rmx.value5 = pRouteDesc->value5;
    pRouteEntry->rtEntry.rt_rmx.routeTag = pRouteDesc->routeTag;
    pRouteEntry->rtEntry.rt_rmx.weight = pRouteDesc->weight;
   
    return;
    }

/*******************************************************************************
*
* routeEntryFree - free a ROUTE_ENTRY structure
*
* This routine removes a duplicate route entry. It executes when that
* entry is specifically deleted or when a new copy of the entry is
* installed as the visible route because of a weight change.
*
* It also executes within the internal rtfree() routine of the kernel.
* That usage allows the original code to handle the reference counts
* for duplicate and visible route entries. Since the current library
* does not permit multiple references to the duplicate routes, this
* ability is only important if future versions provide that feature.
*
* When executing within the rtfree() routine, the <countFlag> parameter
* is FALSE since that routine decrements the reference count for the
* associated interface address. That value is also FALSE when this
* routine releases a partial route before the reference count increases.
*
* RETURNS: N/A 
*
* NOMANUAL
*/

void routeEntryFree
    (
    ROUTE_ENTRY * pRouteEntry, 	/* existing duplicate route entry */
    BOOL 	countFlag 	/* decrement interface reference count? */
    )
    {
    if (countFlag && pRouteEntry->rtEntry.rt_ifa)
        pRouteEntry->rtEntry.rt_ifa->ifa_refcnt--;

    if(pRouteEntry->rtEntry.rt_genmask)
	KHEAP_FREE( (char *)pRouteEntry->rtEntry.rt_genmask);

    if(pRouteEntry->rtEntry.rt_nodes[0].rn_u.rn_leaf.rn_Mask)
	KHEAP_FREE(pRouteEntry->rtEntry.rt_nodes[0].rn_u.rn_leaf.rn_Mask);

    KHEAP_FREE( (char *)pRouteEntry);
    }
#endif /* ROUTER_STACK */

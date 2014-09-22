/* m2UdpLib.c - MIB-II UDP-group API for SNMP agents */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,15oct01,rae  merge from truestack ver 01i, base 01d (SPR #68597, VS)
01d,08mar97,vin  added changes to accomodate changes in pcb structure.
01c,25jan95,jdi  doc cleanup.
01b,11nov94,rhp  edited man pages
01a,08dec93,jag  written
*/

/*
DESCRIPTION
This library provides MIB-II services for the UDP group.  It provides
routines to initialize the group, access the group scalar variables, and
read the table of UDP listeners.  For a broader description of MIB-II
services, see the manual entry for m2Lib.

To use this feature, include the following component:
INCLUDE_MIB2_UDP

USING THIS LIBRARY
This library can be initialized and deleted by calling
m2UdpInit() and m2UdpDelete() respectively, if only the UDP group's services 
are needed.  If full MIB-II support is used, this group and all other
groups can be initialized and deleted by calling m2Init() and m2Delete().

The group scalar variables are accessed by calling m2UdpGroupInfoGet()
as follows:
.CS
    M2_UDP   udpVars;

    if (m2UdpGroupInfoGet (&udpVars) == OK)
	/@ values in udpVars are valid @/
.CE

The UDP table of listeners can be accessed in lexicographical order.
The first entry in the table can be accessed by setting the table
index to zero in a call to m2UdpTblEntryGet().  Every other entry
thereafter can be accessed by incrementing the index returned from the
previous invocation to the next possible lexicographical index, and
repeatedly calling m2UdpTblEntryGet() with the M2_NEXT_VALUE constant
as the search parameter. For example:
.CS
M2_UDPTBL  udpEntry;

    /@ Specify zero index to get the first entry in the table @/

    udpEntry.udpLocalAddress = 0;    /@ local IP Address in host byte order  @/
    udpEntry.udpLocalPort    = 0;    /@ local port Number                  @/

    /@ get the first entry in the table @/

    if ((m2UdpTblEntryGet (M2_NEXT_VALUE, &udpEntry) == OK)
	/@ values in udpEntry in the first entry are valid  @/

    /@ process first entry in the table @/

    /@ 
     * For the next call, increment the index returned in the previous call.
     * The increment is to the next possible lexicographic entry; for
     * example, if the returned index was 0.0.0.0.3000 the index passed in the
     * next invocation should be 0.0.0.0.3001.  If an entry in the table
     * matches the specified index, then that entry is returned.  
     * Otherwise the closest entry following it, in lexicographic order,
     * is returned.
     @/

    /@ get the second entry in the table @/

    if ((m2UdpTblEntryGet (M2_NEXT_VALUE, &udpEntry) == OK)
	/@ values in udpEntry in the second entry are valid  @/
.CE

INCLUDE FILES: m2Lib.h
 
SEE ALSO:
m2Lib, m2IfLib, m2IpLib, m2IcmpLib, m2TcpLib, m2SysLib
*/

/* includes */
#include "vxWorks.h"
#include "vwModNum.h"
#include "m2Lib.h"
#include "socket.h"
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/in_pcb.h>
#include "errnoLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif    /* VIRTUAL_STACK */

/* external declarations */

#ifndef VIRTUAL_STACK

extern struct udpstat     udpstat;	/* UDP statistics variable */
extern struct inpcbhead   udb;		/* UDP link list of listen requests */

#endif    /* VIRTUAL_STACK */

/*******************************************************************************
*
* m2UdpInit - initialize MIB-II UDP-group access
*
* This routine allocates the resources needed to allow access to the UDP 
* MIB-II variables.  This routine must be called before any UDP variables
* can be accessed.
*
* RETURNS: OK, always.
*
* SEE ALSO:
* m2UdpGroupInfoGet(), m2UdpTblEntryGet(), m2UdpDelete()
*/

STATUS m2UdpInit (void)
    {
    return (OK);
    }

/*******************************************************************************
*
* m2UdpGroupInfoGet - get MIB-II UDP-group scalar variables
*
* This routine fills in the UDP structure at <pUdpInfo> with the MIB-II
* UDP scalar variables.
*
* RETURNS: OK, or ERROR if <pUdpInfo> is not a valid pointer.
*
* ERRNO:
* S_m2Lib_INVALID_PARAMETER
*
* SEE ALSO:
* m2UdpInit(), m2UdpTblEntryGet(), m2UdpDelete()
*/

STATUS m2UdpGroupInfoGet
    (
    M2_UDP * pUdpInfo		/* pointer to the UDP group structure */
    )
    {
 
    /* Validate Pointer to UDP structure */
 
    if (pUdpInfo == NULL)
	{
	errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
	}
 
    pUdpInfo->udpNoPorts      = udpstat.udps_noport + 
				udpstat.udps_noportbcast; 
    pUdpInfo->udpOutDatagrams = udpstat.udps_opackets;

    /*  The number UDP packets deliever to UDP users. */

    pUdpInfo->udpInDatagrams  = udpstat.udps_ipackets    -
                                (udpstat.udps_hdrops     +
                                udpstat.udps_badsum      +
                                udpstat.udps_badlen      +
                                udpstat.udps_noportbcast +
                                udpstat.udps_fullsock);
 
    /*  The number UDP packets not deliever due to errors */

    pUdpInfo->udpInErrors     = udpstat.udps_hdrops +
                                udpstat.udps_badsum +
                                udpstat.udps_badlen;
 
    return (OK);
    }


/*******************************************************************************
*
* m2UdpTblEntryGet - get a UDP MIB-II entry from the UDP list of listeners
*
* This routine traverses the UDP table of listeners and does an
* M2_EXACT_VALUE or a M2_NEXT_VALUE search based on the
* <search> parameter.  The calling routine is responsible for
* supplying a valid MIB-II entry index in the input structure
* <pUdpEntry>.  The index is made up of the IP address and the local
* port number.  The first entry in the table is retrieved by doing a
* M2_NEXT_VALUE search with the index fields set to zero.
*
* RETURNS:
* OK, or ERROR if the input parameter is not specified or a match is not 
* found.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*
* SEE ALSO:
* m2Lib, m2UdpInit(), m2UdpGroupInfoGet(), m2UdpDelete()
*/

STATUS m2UdpTblEntryGet
    (
    int              search,        /* M2_EXACT_VALUE or M2_NEXT_VALUE */
    M2_UDPTBL      * pUdpEntry      /* ptr to the requested entry with index */
    )
    {
    M2_UDPTBL        savedEntry;    /* Possible M2_NEXT_VALUE Entry in UDP */
				    /* table */
    M2_UDPTBL        currEntry;
    int              netLock;       /* Use to secure the Network Code Access */
    struct inpcb   * pPcb;          /* Pointer to UDP Listener structure. */
 
 
    /* Validate Pointer to UDP Table Entry structure */
 
    if (pUdpEntry == NULL)
	{
	errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
	}
 
    /* 
     * Initialize Entry for M2_NEXT_VALUE entry search.  Not used in
     * M2_EXACT_VALUE search.
     */
 
    savedEntry.udpLocalAddress = -1;    /* Largest possible value. */
    savedEntry.udpLocalPort    = -1;    /* Largest possible value. */
 
    netLock = splnet ();             /* Get exclusive access to Network Code */
 
    for (pPcb = udb.lh_first; pPcb != NULL; pPcb = pPcb->inp_list.le_next )
        {
	currEntry.udpLocalAddress = ntohl (pPcb->inp_laddr.s_addr);
	currEntry.udpLocalPort    = ntohs (pPcb->inp_lport);

        if (search == M2_EXACT_VALUE)
            {
	    /* Check that the specified index matches the current entry */

            if ((pUdpEntry->udpLocalAddress == currEntry.udpLocalAddress) &&
                (pUdpEntry->udpLocalPort == currEntry.udpLocalPort))
                {
                splx (netLock);   /* Give up exclusive access to Network Code */
                return (OK);      /* The answer is in the structure pUdpEntry */
                }
            }
        else
            {
            /*
             * A NEXT search is satisfied by an entry that is lexicographicaly
             * equal to or greater than the input UDP connection entry. Because
	     * the UDP connection list is not in order, the list must be 
	     * traverse completely before a selection is made.  The rules for a 
	     * lexicographical comparison are built in the next statement.
             */  

	    if (((currEntry.udpLocalAddress > pUdpEntry->udpLocalAddress) ||
		  ((currEntry.udpLocalAddress == pUdpEntry->udpLocalAddress) &&
		   (currEntry.udpLocalPort >= pUdpEntry->udpLocalPort))) &&
                 ((currEntry.udpLocalAddress < savedEntry.udpLocalAddress) ||
                   ((currEntry.udpLocalAddress == savedEntry.udpLocalAddress) &&
                    (currEntry.udpLocalPort < savedEntry.udpLocalPort))))
                {
		/* 
		 * Save the entry which qualifies as the NEXT greater 
		 * lexicographic entry.  Because the table is not order the
		 * search must proceed to the end of the table.
		 */

                savedEntry.udpLocalAddress =  currEntry.udpLocalAddress;
                savedEntry.udpLocalPort    =  currEntry.udpLocalPort;
                }
            }
        }
 
    splx (netLock);             /* Give up exclusive access to Network Code */

    /* If a match was found fill, the requested structure */

    if (savedEntry.udpLocalPort != -1)
        {
        pUdpEntry->udpLocalAddress = savedEntry.udpLocalAddress;
        pUdpEntry->udpLocalPort    = savedEntry.udpLocalPort;
        return (OK);
        }
 
    errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
    return (ERROR);
    }


/*******************************************************************************
*
* m2UdpDelete - delete all resources used to access the UDP group
*
* This routine frees all the resources allocated at the time the group was
* initialized.  The UDP group should not be accessed after this routine has been
* called.
*
* RETURNS: OK, always.
*
* SEE ALSO:
* m2UdpInit(), m2UdpGroupInfoGet(), m2UdpTblEntryGet()
*/

STATUS m2UdpDelete (void)
    {
    return (OK);
    }


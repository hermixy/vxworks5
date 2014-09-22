/* m2TcpLib.c - MIB-II TCP-group API for SNMP agents */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,15oct01,rae  merge from truestack ver 01j, base 01e (VIRTUAL_STACK)
01e,08mar97,vin  added changes to accomodate changes in pcb structure.
01d,25jan95,jdi  doc cleanup.
01c,11nov94,rhp  edit man pages
01b,10nov94,rhp  fixed typo in m2TcpInit man page
01a,08dec93,jag  written
*/

/*
DESCRIPTION
This library provides MIB-II services for the TCP group.  It provides routines
to initialize the group, access the group global variables, read the table
of TCP connections, and change the state of a TCP connection.  For a broader
description of MIB-II services, see the manual entry for m2Lib.

To use this feature, include the following component:
INCLUDE_MIB2_TCP

USING THIS LIBRARY
This library can be initialized and deleted by calling m2TcpInit() and
m2TcpDelete() respectively, if only the TCP group's services are needed.
If full MIB-II support is used, this group and all other groups can be
initialized and deleted by calling m2Init() and m2Delete().

The group global variables are accessed by calling
m2TcpGroupInfoGet() as follows:
.CS
    M2_TCP   tcpVars;

    if (m2TcpGroupInfoGet (&tcpVars) == OK)
	/@ values in tcpVars are valid @/
.CE

The TCP table of connections can be accessed in lexicographical order.  The
first entry in the table can be accessed by setting the table index to
zero.  Every other entry thereafter can be accessed by passing to
m2TcpConnTblEntryGet() the index retrieved in the previous invocation
incremented to the next lexicographical value by giving
M2_NEXT_VALUE as the search parameter.  For example:
.CS
M2_TCPCONNTBL  tcpEntry;

    /@ Specify a zero index to get the first entry in the table @/

    tcpEntry.tcpConnLocalAddress = 0; /@ Local IP address in host byte order @/
    tcpEntry.tcpConnLocalPort    = 0; /@ Local TCP port                    @/
    tcpEntry.tcpConnRemAddress   = 0; /@ remote IP address                 @/
    tcpEntry.tcpConnRemPort      = 0; /@ remote TCP port in host byte order  @/


    /@ get the first entry in the table @/

    if ((m2TcpConnTblEntryGet (M2_NEXT_VALUE, &tcpEntry) == OK)
	/@ values in tcpEntry in the first entry are valid  @/

    /@ process first entry in the table @/

    /@ 
     * For the next call, increment the index returned in the previous call.
     * The increment is to the next possible lexicographic entry; for
     * example, if the returned index was 147.11.46.8.2000.147.11.46.158.1000
     * the index passed in the next invocation should be 
     * 147.11.46.8.2000.147.11.46.158.1001.  If an entry in the table
     * matches the specified index, then that entry is returned.  
     * Otherwise the closest entry following it, in lexicographic order,
     * is returned.
     @/

    /@ get the second entry in the table @/

    if ((m2TcpConnTblEntryGet (M2_NEXT_VALUE, &tcpEntry) == OK)
	/@ values in tcpEntry in the second entry are valid  @/
.CE

The TCP table of connections allows only for a connection to be deleted as
specified in the MIB-II.  For example: 
.CS
    M2_TCPCONNTBL  tcpEntry;

    /@ Fill in the index for the connection to be deleted in the table @/

    /@ Local IP address in host byte order, and local port number @/

    tcpEntry.tcpConnLocalAddress = 0x930b2e08;
    tcpEntry.tcpConnLocalPort    = 3000;

    /@ Remote IP address in host byte order, and remote port number @/

    tcpEntry.tcpConnRemAddress   = 0x930b2e9e;
    tcpEntry.tcpConnRemPort      = 3000;

    tcpEntry.tcpConnState        = 12;	/@ MIB-II state value for delete @/

    /@ set the entry in the table @/

    if ((m2TcpConnTblEntrySet (&tcpEntry) == OK)
	/@ tcpEntry deleted successfuly @/
.CE

INCLUDE FILES: m2Lib.h
 
SEE ALSO:
m2Lib, m2IfLib, m2IpLib, m2IcmpLib, m2UdpLib, m2SysLib
*/

/* includes */

#include "vxWorks.h"
#include "m2Lib.h"
#include <socket.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_timer.h>
#include <netinet/in_pcb.h>
#include <net/protosw.h>
#include <private/iosLibP.h>

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif

/* defines */

/* MIB-II TCP state definitions */

#define M2TCP_CLOSED        1
#define M2TCP_LISTEN        2
#define M2TCP_SYNSENT       3
#define M2TCP_SYNRECEIVED   4
#define M2TCP_ESTABLISHED   5
#define M2TCP_FINWAIT1      6
#define M2TCP_FINWAIT2      7
#define M2TCP_CLOSEWAIT     8
#define M2TCP_LASTACK       9
#define M2TCP_CLOSING       10
#define M2TCP_TIMEWAIT      11
#define M2TCP_DELETETCB     12

/* globals */

/* 
 * This table maps the BSD TCP states to the MIB-II TCP specified states. The
 * table is indexed using the BSD TCP states.
 */

LOCAL long m2TcpStates [TCP_NSTATES] =
	{
	M2TCP_CLOSED,            /* TCPS_CLOSED       -> M2TCP_CLOSED */
	M2TCP_LISTEN,            /* TCPS_LISTEN       -> M2TCP_LISTEN */
	M2TCP_SYNSENT,           /* TCPS_SYN_SENT     -> M2TCP_SYNSENT */
	M2TCP_SYNRECEIVED,       /* TCPS_SYN_RECEIVED -> M2TCP_SYNRECEIVED */
	M2TCP_ESTABLISHED,       /* TCPS_ESTABLISHED  -> M2TCP_ESTABLISHED */
	M2TCP_CLOSEWAIT,         /* TCPS_CLOSE_WAIT   -> M2TCP_CLOSEWAIT */
	M2TCP_FINWAIT1,          /* TCPS_FIN_WAIT_1   -> M2TCP_FINWAIT1  */
	M2TCP_CLOSING,           /* TCPS_CLOSING      -> M2TCP_CLOSING */
	M2TCP_LASTACK,           /* TCPS_LAST_ACK     -> M2TCP_LASTACK */
	M2TCP_FINWAIT2,          /* TCPS_FIN_WAIT_2   -> M2TCP_FINWAIT2 */
        M2TCP_TIMEWAIT,          /* TCPS_TIME_WAIT    -> M2TCP_TIMEWAIT */
	};

/*******************************************************************************
*
* m2TcpInit - initialize MIB-II TCP-group access
*
* This routine allocates the resources needed to allow access to the TCP 
* MIB-II variables.  This routine must be called before any TCP variables
* can be accessed.
*
* RETURNS: OK, always.
*
* SEE ALSO: 
* m2TcpGroupInfoGet(), m2TcpConnEntryGet(), m2TcpConnEntrySet(), m2TcpDelete()
*/

STATUS m2TcpInit (void)
    {
    return (OK);
    }

/******************************************************************************
*
* m2TcpGroupInfoGet - get MIB-II TCP-group scalar variables
*
* This routine fills in the TCP structure pointed to by <pTcpInfo> with the
* values of MIB-II TCP-group scalar variables.
*
* RETURNS: OK, or ERROR if <pTcpInfo> is not a valid pointer.
*
* ERRNO:
* S_m2Lib_INVALID_PARAMETER
*
* SEE ALSO: 
* m2TcpInit(), m2TcpConnEntryGet(), m2TcpConnEntrySet(), m2TcpDelete()
*/

STATUS m2TcpGroupInfoGet
    (
    M2_TCPINFO * pTcpInfo 	/* pointer to the TCP group structure */
    )
    {
    int            netLock;     /* Use to secure the Network Code Access */
    struct inpcb * pInpCb;	/* Ptr to an internet control block */
    struct tcpcb * pTcpCb;	/* Ptr to a TCP connection control block */
 
    /* Validate Pointer to TCP structure */
 
    if (pTcpInfo == NULL)
	{
	errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
	}
 
    netLock = splnet ();        /* Get exclusive access to Network Code */

    /* Traverse the list of TCP control block and count the # of connections */

#ifdef VIRTUAL_STACK
    /*
     * To avoid introducing a conflict with the "tcpcb" structure tag,
     * virtual stacks do not alias the head of the pcb list.
     */

    for (pInpCb = tcb.lh_first, pTcpInfo->tcpCurrEstab = 0;
#else
    for (pInpCb = tcpcb.lh_first, pTcpInfo->tcpCurrEstab = 0;
#endif /* VIRTUAL_STACK */
         pInpCb != NULL; pInpCb = pInpCb->inp_list.le_next)
        {

        /* Get TCP Connection control structure */
 
        pTcpCb = (struct tcpcb *) pInpCb->inp_ppcb;
 
        if ((pTcpCb->t_state == TCPS_ESTABLISHED) ||
            (pTcpCb->t_state == TCPS_CLOSE_WAIT))
            pTcpInfo->tcpCurrEstab++;
        }
 
    splx (netLock);             /* Give up exclusive access to Network Code */
 
    pTcpInfo->tcpRtoAlgorithm           = M2_tcpRtoAlgorithm_vanj;
    pTcpInfo->tcpRtoMin                 = (TCPTV_MIN * 1000) / PR_SLOWHZ;
    pTcpInfo->tcpRtoMax                 = (TCPTV_REXMTMAX * 1000) / PR_SLOWHZ;
 
    /* The Maximum number of TCP connections is determined Dynamically == -1 */
 
    pTcpInfo->tcpMaxConn                = -1;
 
    pTcpInfo->tcpActiveOpens            = tcpstat.tcps_connattempt;
    pTcpInfo->tcpPassiveOpens           = tcpstat.tcps_accepts;
    pTcpInfo->tcpAttemptFails           = tcpstat.tcps_conndrops;
    pTcpInfo->tcpEstabResets            = tcpstat.tcps_drops;
    pTcpInfo->tcpInSegs                 = tcpstat.tcps_rcvtotal;
 
    pTcpInfo->tcpOutSegs                = tcpstat.tcps_sndtotal -
                                          tcpstat.tcps_sndrexmitpack -
                                          tcpstat.tcps_persisttimeo;
 
    pTcpInfo->tcpRetransSegs            = tcpstat.tcps_sndrexmitpack;
 
    pTcpInfo->tcpInErrs                 = tcpstat.tcps_rcvbadsum +
                                          tcpstat.tcps_rcvbadoff +
                                          tcpstat.tcps_rcvshort;

#ifdef VIRTUAL_STACK
    /*
     * (Former) tcpOutRsts global renamed for virtual stacks to prevent
     * name conflict with existing structure element.
     */

    pTcpInfo->tcpOutRsts                = tcpOutResets;
#else
    pTcpInfo->tcpOutRsts                = tcpOutRsts;
#endif

    return (OK);
    }

/******************************************************************************
*
* tcpConnCmp -  compare two TCP connections in lexicographical order
*
* This routine compares two TCP connection control blocks.  It compares the
* value 1 with value 2.  It returns equal, value 1 greater than 
* value 2, or value 1 less than value 2.  The comparison is done based on the
* lexicographical order specified by MIB-II.
*
* RETURNS: 0 = equal, 1 > greater than, and -1 < less than.
*
* SEE ALSO: N/A
*/

LOCAL int tcpConnCmp
    (
    M2_TCPCONNTBL * pConnVal1,	/* pointer to control block (Compare againts) */
    M2_TCPCONNTBL * pConnVal2	/* pointer to control block (Second entry) */
    )
    {
    int result;

	/* Comparison of two unsigned values */

        if (pConnVal1->tcpConnLocalAddress > pConnVal2->tcpConnLocalAddress)
            {
            return 1;                   /* First Entry is Greater */
            }
 
        if (pConnVal1->tcpConnLocalAddress < pConnVal2->tcpConnLocalAddress)
            {
            return -1;                  /* First Entry is Smaller */
            }

	/* Comparison of two signed values */

        result = pConnVal1->tcpConnLocalPort - pConnVal2->tcpConnLocalPort;
 
        if (result != 0)
            return (result);		/* Value > 0 or < 0 */
 
	/* Comparison of two unsigned values */

        if (pConnVal1->tcpConnRemAddress > pConnVal2->tcpConnRemAddress)
            {
            return 1;                   /* First Entry is Greater */
            }
 
        if (pConnVal1->tcpConnRemAddress < pConnVal2->tcpConnRemAddress)
            {
            return -1;                  /* First Entry is Smaller */
            }
 
        /* Result will determine Equal, Less than or Greater than */
 
        return (pConnVal1->tcpConnRemPort - pConnVal2->tcpConnRemPort);
    }

/******************************************************************************
*
* m2TcpConnEntryGet - get a MIB-II TCP connection table entry
*
* This routine traverses the TCP table of users and does an
* M2_EXACT_VALUE or a M2_NEXT_VALUE search based on the
* <search> parameter (see m2Lib).  The calling routine is responsible
* for supplying a valid MIB-II entry index in the input structure
* <pReqTcpConnEntry>.  The index is made up of the local IP address,
* the local port number, the remote IP address, and the remote port.
* The first entry in the table is retrieved by doing a M2_NEXT_VALUE
* search with the index fields set to zero.
*
* RETURNS: 
* OK, or ERROR if the input parameter is not specified or a
* match is not found.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*
* SEE ALSO:
* m2Lib, m2TcpInit(), m2TcpGroupInfoGet(), m2TcpConnEntrySet(), m2TcpDelete()
*/

STATUS m2TcpConnEntryGet
    (
    int             search,             /* M2_EXACT_VALUE or M2_NEXT_VALUE */
    M2_TCPCONNTBL * pReqTcpConnEntry    /* input = Index, Output = Entry */
    )
    {
    M2_TCPCONNTBL   savedEntry; /* Use to save the table entry to return */
    M2_TCPCONNTBL   tableEntry; /* Use to test an entry in the table */
    int              netLock;   /* Use to secure the Network Code Access */
    struct inpcb   * pPcb;      /* Pointer to an entry in TCP Connection List */
 
    /* Validate Pointer to TCP Table Entry structure */
 
    if (pReqTcpConnEntry == NULL)
	{
	errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
	}

    /* Setup structure used in the search for the requested TCP entry */

    savedEntry.tcpConnState        =  TCP_NSTATES;  /* Invalid TCP State */
    savedEntry.tcpConnLocalAddress = -1;	    /* Largest local IP Addr */
    savedEntry.tcpConnLocalPort    = -1;	    /* Largest local port */ 
    savedEntry.tcpConnRemAddress   = -1;	    /* Largest Remote IP Addr */
    savedEntry.tcpConnRemPort      = -1;	    /* Largest Remote Port */
 
    netLock = splnet ();        /* Get exclusive access to Network Code */

    /* 
     * Traverse the circular list of TCP control blocks.  The first item in the
     * list is a dummy header, skip it and used it as the loop exit condition.
     */

#ifdef VIRTUAL_STACK
    /*
     * To avoid introducing a conflict with the "tcpcb" structure tag,
     * virtual stacks do not alias the head of the pcb list.
     */

    for (pPcb = tcb.lh_first; pPcb != NULL; pPcb = pPcb->inp_list.le_next)
#else
    for (pPcb = tcpcb.lh_first; pPcb != NULL; pPcb = pPcb->inp_list.le_next)
#endif
        {
 
	/* Convert control block format into a more convenient format */

        tableEntry.tcpConnLocalAddress = ntohl(pPcb->inp_laddr.s_addr);
        tableEntry.tcpConnLocalPort    = ntohs(pPcb->inp_lport);
        tableEntry.tcpConnRemAddress   = ntohl(pPcb->inp_faddr.s_addr);
        tableEntry.tcpConnRemPort      = ntohs(pPcb->inp_fport);
 
        if (search == M2_EXACT_VALUE &&
	    (tcpConnCmp(&tableEntry, pReqTcpConnEntry) == 0))
            {
	    /* Match, map the connection state to MIB-II format and exit */

            pReqTcpConnEntry->tcpConnState =
		    m2TcpStates [((struct tcpcb *) pPcb->inp_ppcb)->t_state];
 
            splx (netLock);      /* Give up exclusive access to Network Code */
            return (OK);
            }
        else
            {
            /*
             * A NEXT search is satisfied by an entry that is lexicographicaly
             * greater than the input TCP connection entry.  Because the TCP 
	     * connection list is not in order, the list must be traverse 
	     * completly before a selection is made.  The rules for a 
	     * lexicographicaly comparison are built in the routine tcpConnCmp.
             */  

            if ((tcpConnCmp(&tableEntry, pReqTcpConnEntry) >= 0) &&
                (tcpConnCmp(&tableEntry, &savedEntry) < 0))
                {
                savedEntry.tcpConnLocalAddress = ntohl(pPcb->inp_laddr.s_addr);
                savedEntry.tcpConnLocalPort    = ntohs(pPcb->inp_lport);
                savedEntry.tcpConnRemAddress   = ntohl(pPcb->inp_faddr.s_addr);
                savedEntry.tcpConnRemPort      = ntohs(pPcb->inp_fport);

		/* Save the connection state in BSD format */

                savedEntry.tcpConnState =
                        ((struct tcpcb *) pPcb->inp_ppcb)->t_state;
                }
            }
        }
 
    splx (netLock);             /* Give up exclusive access to Network Code */
 
    if (savedEntry.tcpConnState == TCP_NSTATES)
	{
	errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        return (ERROR);                 /* Requested Entry Not Found */
	}
 
    pReqTcpConnEntry->tcpConnState      = m2TcpStates [savedEntry.tcpConnState];
    pReqTcpConnEntry->tcpConnLocalAddress = savedEntry.tcpConnLocalAddress;
    pReqTcpConnEntry->tcpConnLocalPort    = savedEntry.tcpConnLocalPort;
    pReqTcpConnEntry->tcpConnRemAddress   = savedEntry.tcpConnRemAddress;
    pReqTcpConnEntry->tcpConnRemPort      = savedEntry.tcpConnRemPort;
 
    return (OK);
    }


/******************************************************************************
*
* m2TcpConnEntrySet -  set a TCP connection to the closed state
*
* This routine traverses the TCP connection table and searches for the
* connection specified by the input parameter <pReqTcpConnEntry>. The
* calling routine is responsible for providing a valid index as the
* input parameter <pReqTcpConnEntry>.  The index is made up of the
* local IP address, the local port number, the remote IP address, and
* the remote port.  This call can only succeed if the connection is in
* the MIB-II state "deleteTCB" (12).  If a match is found, the socket
* associated with the TCP connection is closed.
*
* RETURNS: 
* OK, or ERROR if the input parameter is invalid, the state of the
* connection specified at <pReqTcpConnEntry> is not "closed,"
* the specified connection is not found, a socket is not associated
* with the connection, or the close() call fails.
*
* SEE ALSO: 
* m2TcpInit(), m2TcpGroupInfoGet(), m2TcpConnEntryGet(), m2TcpDelete()
*/

STATUS m2TcpConnEntrySet
    (
    M2_TCPCONNTBL * pReqTcpConnEntry  /* pointer to TCP connection  to close */
    )
    {
    int               fdToClose;     /* Selected File Descriptor to close */
    int               netLock;       /* Use to secure the Network Code Access */
    struct inpcb    * pInp;	     /* Pointer to BSD PCB list */
    struct in_addr    loc_addr;	     /* Local IP Address structure */
    struct in_addr    rem_addr;      /* Remote IP Address structure */
    unsigned short    locPort;
    unsigned short    remPort;
 
    /* Validate Pointer to TCP Table Entry structure and operation  */
 
    if (pReqTcpConnEntry == NULL ||
        pReqTcpConnEntry->tcpConnState != M2TCP_DELETETCB)
	{
	errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
	}
 
    /* Look for the Internet Control Block which maps to the TCP Entry */

    locPort         = pReqTcpConnEntry->tcpConnLocalPort;
    remPort         = pReqTcpConnEntry->tcpConnRemPort;
 
    loc_addr.s_addr = htonl(pReqTcpConnEntry->tcpConnLocalAddress);
    locPort         = htons(locPort);
    rem_addr.s_addr = htonl(pReqTcpConnEntry->tcpConnRemAddress);
    remPort         = htons(remPort);
 
    netLock = splnet ();        /* Get exclusive access to Network Code */
 
    if ((pInp = in_pcblookup (&tcbinfo, rem_addr,
		      remPort, loc_addr, locPort, 0)) == NULL)
        {
        splx (netLock);         /* Give up exclusive access to Network Code */
	errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        return (ERROR);		/* Entry not found */
        }
 
    splx (netLock);             /* Give up exclusive access to Network Code */
 
    /* Look for the FD to issue close in the File Descriptor Table */
 
    for (fdToClose = 0; fdToClose < maxFiles ; fdToClose++)
        {

#ifdef _WRS_VXWORKS_5_X
            if (fdTable [fdToClose].value == (int) pInp->inp_socket)
#else
            if (fdTable [fdToClose] &&
                fdTable [fdToClose]->value == (int) pInp->inp_socket)
#endif /* _WRS_VXWORKS_5_X */
            
                break;
        }
 
    if (fdToClose >= maxFiles)
	{
	errnoSet (S_m2Lib_TCPCONN_FD_NOT_FOUND);
        return (ERROR);
	}
 
    /* Issue Close */

    fdToClose = STD_FIX(fdToClose);
 
    return (close (fdToClose));
    }

/*******************************************************************************
*
* m2TcpDelete - delete all resources used to access the TCP group
*
* This routine frees all the resources allocated at the time the group was
* initialized.  The TCP group should not be accessed after this routine has been
* called.
*
* RETURNS: OK, always.
*
* SEE ALSO: 
* m2TcpInit(), m2TcpGroupInfoGet(), m2TcpConnEntryGet(), m2TcpConnEntrySet() 
*/

STATUS m2TcpDelete (void)
    {
    return (OK);
    }


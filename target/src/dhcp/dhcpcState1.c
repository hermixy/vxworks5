/* dhcpcState1.c - DHCP client runtime state machine (lease acquisition) */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01r,25apr02,wap  Only do arp_check() once so DHCPDECLINE messages are sent
                 (SPR #76315)
01q,23apr02,wap  use dhcpTime() instead of time() (SPR #68900)
01p,05nov01,wap  Fix memory leak in selecting() (SPR #68981)
01o,12oct01,rae  merge from truestack (modhist update)
                 note: SPRs 29555 30344 fixed here
01n,24oct00,spm  fixed modification history after tor3_x merge
01m,23oct00,niq  merged from version 01p of tor3_x branch (base version 01l);
                 upgrade to BPF replaces tagged frame support
01l,04dec97,spm  added code review modifications
01k,06oct97,spm  removed reference to deleted endDriver global
01j,02sep97,spm  moved data retrieval to prevent dereferenced NULL (SPR #9243);
                 removed excess IMPORT statement
01i,26aug97,spm  major overhaul: reorganized code and changed user interface
                 to support multiple leases at runtime
01h,06aug97,spm  removed parameters linked list to reduce memory required
01g,10jun97,spm  isolated incoming messages in state machine from input hooks
01f,02jun97,spm  changed DHCP option tags to prevent name conflicts (SPR #8667)
01e,06may97,spm  changed memory access to align IP header on four byte boundary
01d,28apr97,spm  corrected placement of conditional include to prevent failure
01c,18apr97,spm  added conditional include DHCPC_DEBUG for displayed output
01b,07apr97,spm  added code to use Host Requirements defaults, rewrote docs
01a,27jan97,spm  extracted from dhcpc.c to reduce object size
*/

/*
DESCRIPTION
This library contains a portion of the finite state machine for the WIDE 
project DHCP client, modified for vxWorks compatibility.

INTERNAL
This module contains the functions used prior to the BOUND state. It was
created to isolate those functions and reduce the size of the boot ROM image 
so that the DHCP client could be used with targets like the MV147 which have 
limited ROM capacity. When executing at boot time, the DHCP client's state
machine only used the states defined in this module. After the initial port 
was completed, the WIDE project implementation was greatly modified to allow 
the DHCP client library to establish and maintain multiple leases unassociated 
with the network interface used for message transfer. That capability is
completely unnecessary for the boot time client, so it no longer shares any 
code with this module.

INCLUDE_FILES: dhcpcLib.h
*/

/*
 * WIDE Project DHCP Implementation
 * Copyright (c) 1995 Akihiro Tominaga
 * Copyright (c) 1995 WIDE Project
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided only with the following
 * conditions are satisfied:
 *
 * 1. Both the copyright notice and this permission notice appear in
 *    all copies of the software, derivative works or modified versions,
 *    and any portions thereof, and that both notices appear in
 *    supporting documentation.
 * 2. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by WIDE Project and
 *      its contributors.
 * 3. Neither the name of WIDE Project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPER ``AS IS'' AND WIDE
 * PROJECT DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE. ALSO, THERE
 * IS NO WARRANTY IMPLIED OR OTHERWISE, NOR IS SUPPORT PROVIDED.
 *
 * Feedback of the results generated from any improvements or
 * extensions made to this software would be much appreciated.
 * Any such feedback should be sent to:
 * 
 *  Akihiro Tominaga
 *  WIDE Project
 *  Keio University, Endo 5322, Kanagawa, Japan
 *  (E-mail: dhcp-dist@wide.ad.jp)
 *
 * WIDE project has the rights to redistribute these changes.
 */

/* includes */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include "vxWorks.h"
#include "rngLib.h"
#include "wdLib.h"
#include "time.h"
#include "inetLib.h"
#include "logLib.h"
#include "taskLib.h"
#include "sysLib.h"
#include "vxLib.h"
#include "netLib.h"

#include "dhcp/dhcpcStateLib.h"
#include "dhcp/dhcpcInternal.h"
#include "dhcp/dhcpcCommonLib.h"

/* defines */

/* Retransmission delay is timer value plus/minus one second (RFC 1541). */

#define	SLEEP_RANDOM(timer) ( (timer - 1) + (rand () % 2) )
#define REQUEST_RETRANS   4  /* Max number of retransmissions (RFC 1541). */

/* globals */

IMPORT int 	dhcpcMinLease; 	/* Minimum accepted lease length. */
IMPORT SEM_ID 	dhcpcMutexSem;	/* Protects status indicator */
 
unsigned char		dhcpCookie [MAGIC_LEN] = RFC1048_MAGIC;
struct buffer		sbuf;

int (*fsm [MAX_STATES]) ();

int (*handle_param [MAXTAGNUM]) () = 
    {
    NULL,           /* PAD */
    handle_ip,      /* SUBNET_MASK */
    handle_num,     /* TIME_OFFSET */
    handle_ips,     /* ROUTER */
    handle_ips,     /* TIME_SERVER */
    handle_ips,     /* NAME_SERVER */
    handle_ips,     /* DNS_SERVER */
    handle_ips,     /* LOG_SERVER */
    handle_ips,     /* COOKIE_SERVER */
    handle_ips,     /* LPR_SERVER */
    handle_ips,     /* IMPRESS_SERVER */
    handle_ips,     /* RLS_SERVER */
    handle_str,     /* HOSTNAME */
    handle_num,     /* BOOTSIZE */
    handle_str,     /* MERIT_DUMP */
    handle_str,     /* DNS_DOMAIN */
    handle_ip,      /* SWAP_SERVER */
    handle_str,     /* ROOT_PATH */
    handle_str,     /* EXTENSIONS_PATH */
    handle_bool,    /* IP_FORWARD */
    handle_bool,    /* NONLOCAL_SRCROUTE */
    handle_ippairs, /* POLICY_FILTER */
    handle_num,     /* MAX_DGRAM_SIZE */
    handle_num,     /* DEFAULT_IP_TTL */
    handle_num,     /* MTU_AGING_TIMEOUT */
    handle_nums,    /* MTU_PLATEAU_TABLE */
    handle_num,     /* IF_MTU */
    handle_bool,    /* ALL_SUBNET_LOCAL */
    handle_ip,      /* BRDCAST_ADDR */
    handle_bool,    /* MASK_DISCOVER */
    handle_bool,    /* MASK_SUPPLIER */
    handle_bool,    /* ROUTER_DISCOVER */
    handle_ip,      /* ROUTER_SOLICIT */
    handle_ippairs, /* STATIC_ROUTE */
    handle_bool,    /* TRAILER */
    handle_num,     /* ARP_CACHE_TIMEOUT */
    handle_bool,    /* ETHER_ENCAP */
    handle_num,     /* DEFAULT_TCP_TTL */
    handle_num,     /* KEEPALIVE_INTER */
    handle_bool,    /* KEEPALIVE_GARBA */
    handle_str,     /* NIS_DOMAIN */
    handle_ips,     /* NIS_SERVER */
    handle_ips,     /* NTP_SERVER */
    handle_list,    /* VENDOR_SPEC */
    handle_ips,     /* NBN_SERVER */
    handle_ips,     /* NBDD_SERVER */
    handle_num,     /* NB_NODETYPE */
    handle_str,     /* NB_SCOPE */
    handle_ips,     /* XFONT_SERVER */
    handle_ips,     /* XDISPLAY_MANAGER */
    NULL,           /* REQUEST_IPADDR */
    handle_num,     /* LEASE_TIME */
    NULL,           /* OPT_OVERLOAD */
    NULL,           /* DHCP_MSGTYPE */
    handle_ip,      /* SERVER_ID */
    NULL,           /* REQ_LIST */
    handle_str,     /* DHCP_ERRMSG */
    NULL,           /* DHCP_MAXMSGSIZE */
    handle_num,     /* DHCP_T1 */
    handle_num,     /* DHCP_T2  */
    NULL,           /* CLASS_ID */
    NULL,           /* CLIENT_ID */
    NULL,
    NULL,
    handle_str,     /* NISP_DOMAIN */
    handle_ips,     /* NISP_SERVER */
    handle_str,     /* TFTP_SERVERNAME */
    handle_str,     /* BOOTFILE */
    handle_ips,     /* MOBILEIP_HA */
    handle_ips,     /* SMTP_SERVER */
    handle_ips,     /* POP3_SERVER */
    handle_ips,     /* NNTP_SERVER */
    handle_ips,     /* DFLT_WWW_SERVER */
    handle_ips,     /* DFLT_FINGER_SERVER */
    handle_ips,     /* DFLT_IRC_SERVER */
    handle_ips,     /* STREETTALK_SERVER */
    handle_ips      /* STDA_SERVER */
    };

/*******************************************************************************
*
* gen_retransmit - generic retransmission after timeout
*
* This routine retransmits the current DHCP client message (a discover or
* a request message) after the appropriate timeout interval expires.
* It is called from multiple locations in the finite state machine.
*
* RETURNS: 0 if transmission completed, or negative value on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int gen_retransmit
    (
    LEASE_DATA * 	pLeaseData, 	/* lease-specific data structures */
    int 		length 		/* length of DHCP message */
    )
    {
    time_t	curr_epoch = 0;
    struct ifnet * 	pIf; 	/* interface used for retransmission */
    struct sockaddr_in  dest;
    BOOL bcastFlag;

    pIf = pLeaseData->ifData.iface;

    if (dhcpTime (&curr_epoch) == -1)
        return (-1);

    /* Update the appropriate fields in the current DHCP message. */

    if (pLeaseData->currState != REQUESTING)
        dhcpcMsgOut.dhcp->secs = htons (curr_epoch - pLeaseData->initEpoch);

    dhcpcMsgOut.udp->uh_sum = 0;
    dhcpcMsgOut.udp->uh_sum = udp_cksum (&spudph, (char *)dhcpcMsgOut.udp, 
                                         ntohs (spudph.ulen));

    /*
     * Retransmit the message. Set the flag to use a link-level broadcast
     * address if needed (prevents ARP messages if using Ethernet devices).
     */

    bzero ( (char *)&dest, sizeof (struct sockaddr_in));

    dest.sin_len = sizeof (struct sockaddr_in);
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dhcpcMsgOut.ip->ip_dst.s_addr;

    if (dest.sin_addr.s_addr == 0xffffffff)
        bcastFlag = TRUE;
    else
        bcastFlag = FALSE;

    pIf = pLeaseData->ifData.iface;

    if (dhcpSend (pIf, &dest, sbuf.buf, length, bcastFlag) == ERROR)
        return (-2);
    return(0);
    }

/*******************************************************************************
*
* retrans_wait_offer - signal reception interval for initial offer expires
*
* This routine sends a timeout notification to the client monitor task when
* the interval for receiving an initial lease offer expires. It is called at 
* interrupt level by a watchdog timer. The monitor task will eventually execute
* the WAIT_OFFER state to process the timeout event and retransmit the
* DHCP discover message.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void retrans_wait_offer
    (
    LEASE_DATA * 	pLeaseData 	/* lease-specific status information */
    )
    { 
    /* 
     * Ignore the timeout if a state transition occurred during 
     * the scheduled timer interval.
     */

    if (pLeaseData->currState != WAIT_OFFER)
        return;

    /* Construct and send a timeout message to the lease monitor task. */

    dhcpcEventAdd (DHCP_AUTO_EVENT, DHCP_TIMEOUT, pLeaseData, TRUE);

    return;
    }

/*******************************************************************************
*
* alarm_selecting - signal when collection time expires 
*
* This routine sends a timeout notification to the client monitor task so
* that the corresponding lease will stop collecting DHCP offers. It is called 
* at interrupt level by a watchdog timer. The monitor task will eventually 
* advance the lease from the SELECTING to the REQUESTING state.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void alarm_selecting
    (
    LEASE_DATA * 	pLeaseData 	/* lease-specific status information */
    )
    {
    STATUS result;

    /* 
     * Ignore the timeout if a state transition occurred during 
     * the scheduled timer interval.
     */

    if (pLeaseData->currState != SELECTING)
        return;

    /* Construct and send a timeout message to the lease monitor task. */

    result = dhcpcEventAdd (DHCP_AUTO_EVENT, DHCP_TIMEOUT, pLeaseData, TRUE);

#ifdef DHCPC_DEBUG
    if (result == ERROR)
        logMsg ("Warning: couldn't add timeout event for SELECTING state.\n", 
                0, 0, 0, 0, 0, 0);
#endif
    return;
    }

/*******************************************************************************
*
* retrans_requesting - signal when reception interval for initial reply expires
*
* This routine sends a timeout notification to the client monitor task when
* the interval for receiving a server reply to a lease request expires. It
* is called at interrupt level by a watchdog timer. The monitor task will 
* eventually execute the REQUESTING state to process the timeout event and 
* retransmit the DHCP request message.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void retrans_requesting
    (
    LEASE_DATA * 	pLeaseData 	/* lease-specific status information */
    )
    {
    /* 
     * Ignore the timeout if a state transition occurred during 
     * the scheduled timer interval.
     */

    if (pLeaseData->currState != REQUESTING)
        return;

    /* Construct and send a timeout message to the lease monitor task. */

    dhcpcEventAdd (DHCP_AUTO_EVENT, DHCP_TIMEOUT, pLeaseData, TRUE);

#ifdef DHCPC_DEBUG
    logMsg ("retransmit DHCPREQUEST(REQUESTING)\n", 0, 0, 0, 0, 0, 0);
#endif

    return;
    }

/*******************************************************************************
*
* inform - external configuration state of client finite state machine
*
* This routine begins the inform message process, which obtains additional
* parameters for a host configured with an external address. This processing
* is isolated from the normal progression through the state machine. Like the
* DHCP discover message, the inform message is the first transmission. 
* However, the only valid response is an acknowledgement by a server. As a
* result, the process is a hybrid between the implementations of the initial
* state and the requesting state which finalizes the lease establishment.
*
* The routine implements the initial state of the finite state machine for an
* externally assigned IP address. It is invoked by the event handler of the
* client monitor task, and should only be called internally.
*
* RETURNS: OK (processing completes), DHCPC_DONE (remove lease), or ERROR.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int inform
    (
    EVENT_DATA * 	pEvent 	/* pointer to event descriptor */
    )
    {
    LEASE_DATA * 	pLeaseData = NULL;
    struct sockaddr_in 	dest;
    struct ifnet * 	pIf;
    int 		length; 	/* Amount of data in message */

    if (pEvent->source == DHCP_AUTO_EVENT)
        {
        /*
         * Received DHCP messages or timeouts can only reach this
         * routine when repeating the DHCP INFORM message exchange.
         * Ignore these stale notifications.
         */

         return (OK);
         }

    bzero (sbuf.buf, sbuf.size);

#ifdef DHCPC_DEBUG
    logMsg ("dhcpc: Entered INFORM state.\n", 0, 0, 0, 0, 0, 0);
#endif

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pEvent->leaseId;

    wdCancel (pLeaseData->timer);    /* Reset watchdog timer. */

    /*
     * The DHCP_USER_RELEASE event occurs in response to the dhcpcRelease()
     * or dhcpcShutdown() call. Remove all data structures for this lease.
     */

    if (pEvent->type == DHCP_USER_RELEASE)
        {
        dhcpcLeaseCleanup (pLeaseData);
        return (DHCPC_DONE);
        }

    /*
     * Set lease to generate newer RFC 2131 messages. Older servers might
     * ignore messages if their length is less than the minimum length
     * obtained with a fixed options field, but will not recognize an
     * inform message anyway.
     */
 
    pLeaseData->oldFlag = FALSE;

    /* Create DHCP INFORM message and assign new transaction ID. */

    length = make_request (pLeaseData, INFORMING, TRUE);
    if (length < 0)
        {
#ifdef DHCPC_DEBUG
        logMsg ("Error making DHCP inform message. Can't continue.\n",
                0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }

    dhcpcMsgOut.dhcp->secs = 0;
    dhcpcMsgOut.udp->uh_sum = 0;
    dhcpcMsgOut.udp->uh_sum = udp_cksum (&spudph, (char *)dhcpcMsgOut.udp,
                                         ntohs (spudph.ulen));
#ifdef DHCPC_DEBUG
    logMsg ("Sending DHCPINFORM.\n", 0, 0, 0, 0, 0, 0);
#endif

    bzero ( (char *)&dest, sizeof (struct sockaddr_in));

    dest.sin_len = sizeof (struct sockaddr_in);
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dhcpcMsgOut.ip->ip_dst.s_addr;

    pIf = pLeaseData->ifData.iface;
 
    if (dhcpSend (pIf, &dest, sbuf.buf, length, TRUE) == ERROR)
        {
#ifdef DHCPC_DEBUG
        logMsg ("Can't send DHCPINFORM.\n", 0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }

    /* Set lease data to execute next state and start retransmission timer. */

    pLeaseData->prevState = INFORMING;
    pLeaseData->currState = REQUESTING;

    pLeaseData->timeout = FIRSTTIMER;
    pLeaseData->numRetry = 0;

    wdStart (pLeaseData->timer, sysClkRateGet() *
                                SLEEP_RANDOM (pLeaseData->timeout),
             (FUNCPTR)retrans_requesting, (int)pLeaseData);

    return (OK);    /* Next state is REQUESTING */
    }

/*******************************************************************************
*
* init - initial state of client finite state machine
*
* This routine implements the initial state of the finite state machine. 
* After a random backoff delay, it resets the network interface if necessary
* and transmits the DHCP discover message. This state may be repeated after a 
* later state if a lease is not renewed or a recoverable error occurs. It 
* could also be executed following unrecoverable errors. The routine is 
* invoked by the event handler of the client monitor task, and should only be 
* called internally.
*
* RETURNS: OK (processing completes), DHCPC_DONE (remove lease), or ERROR.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int init
    (
    EVENT_DATA * 	pEvent 	/* pointer to event descriptor */
    )
    {
    LEASE_DATA * 	pLeaseData = NULL;
    STATUS 		result;
    struct sockaddr_in 	dest;
    struct ifnet * 	pIf;
    int 		length;

    bzero (sbuf.buf, sbuf.size);

#ifdef DHCPC_DEBUG
    logMsg ("dhcpc: Entered INIT state.\n", 0, 0, 0, 0, 0, 0);
#endif

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pEvent->leaseId;

    wdCancel (pLeaseData->timer);    /* Reset watchdog timer. */

    /*
     * The DHCP_USER_RELEASE event occurs in response to the dhcpcRelease()
     * or dhcpcShutdown() call. Remove all data structures for this lease.
     */

    if (pEvent->type == DHCP_USER_RELEASE)
        {
        dhcpcLeaseCleanup (pLeaseData);
        return (DHCPC_DONE);
        }
 
    /* 
     * Unrecoverable errors are only removed when the dhcpcBind() call sets
     * the state indicators to INIT_REBOOT. Ignore all other events until then.
     */

    if (pLeaseData->prevState == DHCPC_ERROR)
        return (OK);
   
    semTake (dhcpcMutexSem, WAIT_FOREVER);    /* Reset status indicator. */
    pLeaseData->leaseGood = FALSE;
    semGive (dhcpcMutexSem);

    /*
     * Set lease to generate newer RFC 2131 messages initially. The client
     * will revert to the older message format if it does not receive a
     * response to the initial set of discover messages. Older servers might
     * ignore messages less than the minimum length obtained with a fixed
     * options field. The older format pads the field to reach that length.
     */

    pLeaseData->oldFlag = FALSE;

    /* Create DHCP_DISCOVER message and assign new transaction ID. */

    length = make_discover (pLeaseData, TRUE);

    /* 
     * Random delay from one to ten seconds to avoid startup congestion.
     * (Delay length specified in RFC 1541).
     *
     */
    taskDelay (sysClkRateGet () * (1 + (rand () % INIT_WAITING)) );

    /* 
     * If an event notification hook is present, send a notification of
     * the lease expiration when appropriate. (None is needed after a reboot).
     */
     
    if (pLeaseData->eventHookRtn != NULL)
        {
        if (pLeaseData->prevState != REBOOTING && 
                pLeaseData->prevState != INIT_REBOOT)
            result = (* pLeaseData->eventHookRtn) (DHCPC_LEASE_INVALID, 
                                                   pEvent->leaseId);
        }

    /* 
     * Reset the network interface if it used the address
     * information provided by the indicated lease.
     */

    if (pLeaseData->autoConfig || pLeaseData->leaseType == DHCP_AUTOMATIC)
        {
        reset_if (&pLeaseData->ifData);
        }

    dhcpcMsgOut.dhcp->secs = 0;
    dhcpcMsgOut.udp->uh_sum = 0;
    dhcpcMsgOut.udp->uh_sum = udp_cksum (&spudph, (char *)dhcpcMsgOut.udp,
                                         ntohs (spudph.ulen));

    if (dhcpTime (&pLeaseData->initEpoch) == -1) 
        {
#ifdef DHCPC_DEBUG
        logMsg ("time() error setting initEpoch\n", 0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }

#ifdef DHCPC_DEBUG
    logMsg ("Sending DHCPDISCOVER\n", 0, 0, 0, 0, 0, 0);
#endif

    bzero ( (char *)&dest, sizeof (struct sockaddr_in));

    dest.sin_len = sizeof (struct sockaddr_in);
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dhcpcMsgOut.ip->ip_dst.s_addr;

    pIf = pLeaseData->ifData.iface; 

    if (dhcpSend (pIf, &dest, sbuf.buf, length, TRUE) == ERROR)
        {
#ifdef DHCPC_DEBUG
        logMsg ("Can't send DHCPDISCOVER\n", 0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }

    /* Set lease data to execute next state and start retransmission timer. */

    pLeaseData->prevState = INIT;
    pLeaseData->currState = WAIT_OFFER;

    pLeaseData->timeout = FIRSTTIMER;
    pLeaseData->numRetry = 0;

    wdStart (pLeaseData->timer, sysClkRateGet() *
                                SLEEP_RANDOM (pLeaseData->timeout),
             (FUNCPTR)retrans_wait_offer, (int)pLeaseData);

    return (OK);    /* Next state is WAIT_OFFER */
    }

/*******************************************************************************
*
* wait_offer - Initial offering state of client finite state machine
*
* This routine contains the initial part of the second state of the finite 
* state machine. It handles all processing until an acceptable DHCP offer is 
* received. If a timeout occurred, it retransmits the DHCP discover message. 
* Otherwise, it evaluates the parameters contained in the received DHCP offer. 
* If the minimum requirements are met, processing will continue with the 
* selecting() routine. If no offer is received before the retransmission
* limit is reached, the negotiation process fails.
* .IP
* This routine is invoked by the event handler of the client monitor task, 
* and should only be called internally. Any user requests generated by 
* incorrect calls or delayed responses to the dhcpcBind() and dhcpcVerify()
* routines are ignored.
*
* RETURNS: OK (processing completes), DHCPC_DONE (remove lease), or ERROR.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int wait_offer
    (
    EVENT_DATA * 	pEvent 	/* pointer to event descriptor */
    )
    {
    char		errmsg [255];
    char * 		option;
    int		timer = 0;
    int		retry = 0;
    struct dhcp_param *	pParams = NULL;
    LEASE_DATA * 	pLeaseData = NULL;
    char * 		pMsgData;
    int 		length;

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pEvent->leaseId;

    /*
     * The DHCP_USER_RELEASE event occurs in response to the dhcpcRelease()
     * or dhcpcShutdown() call. Remove all data structures for this lease.
     */

    if (pEvent->type == DHCP_USER_RELEASE)
        {
        dhcpcLeaseCleanup (pLeaseData);
        return (DHCPC_DONE);
        }

    /* Ignore bind and verify user events, which are meaningless here. */

    if (pEvent->source == DHCP_USER_EVENT)
        return (OK);

    bzero (errmsg, sizeof (errmsg));

#ifdef DHCPC_DEBUG
    logMsg ("dhcpc: Entered WAIT_OFFER state.\n", 0, 0, 0, 0, 0, 0);
#endif

    if (pLeaseData->prevState == INIT) 
        {
        /* Clear any previous parameter settings. */

        if (pLeaseData->dhcpcParam != NULL)
            {
            clean_param (pLeaseData->dhcpcParam);
            free (pLeaseData->dhcpcParam);
            pLeaseData->dhcpcParam = NULL;
            }
        pLeaseData->prevState = WAIT_OFFER;
        }

    if (pEvent->type == DHCP_TIMEOUT)
        {
#ifdef DHCPC_DEBUG
        logMsg ("dhcp: timed out in WAIT_OFFER state.\n", 0, 0, 0, 0, 0, 0);
#endif

        /* Handle timeout - no DHCP offers received yet. */

        retry = pLeaseData->numRetry;
        timer = pLeaseData->timeout;

        retry++;
        if (retry == DISCOVER_RETRANS)    /* Retransmission limit reached. */
            {
#ifdef DHCPC_DEBUG
            logMsg ("No lease offers received by client.\n", 0, 0, 0, 0, 0, 0);
#endif

            /*
             * No response to new DHCP message format, which can be ignored
             * by older DHCP servers as too short. Try the earlier format,
             * which padded the options field to a minimum length. If
             * successful, all subsequent messages will add that padding.
             */

            if (!pLeaseData->oldFlag)
                {
                pLeaseData->oldFlag = TRUE;
                timer = FIRSTTIMER / 2;     /* Counteract later doubling. */
                retry = 0;
                }
            else
                return (ERROR);
            }

        /* Try to retransmit appropriate DHCP message for current state. */

        /* Recreate DHCP_DISCOVER message using the same transaction ID. */

        length = make_discover (pLeaseData, FALSE);

        gen_retransmit (pLeaseData, length);

        if (timer < MAXTIMER)
            {
            /* Double retransmission delay with each attempt. (RFC 1541). */

            timer *= 2;
            }

        /* Set retransmission timer to randomized exponential backoff. */

        wdStart (pLeaseData->timer, sysClkRateGet() * SLEEP_RANDOM (timer),
                 (FUNCPTR)retrans_wait_offer, (int)pLeaseData);

        pLeaseData->timeout = timer;
        pLeaseData->numRetry = retry;
        }
    else
        {
        /*
         * Process DHCP message stored in receive buffer by monitor task. 
         * The 4-byte alignment of the IP header needed by Sun BSP's is
         * guaranteed by the Berkeley Packet Filter during input.
         */

        pMsgData = pLeaseData->msgBuffer;

        align_msg (&dhcpcMsgIn, pMsgData);

        /* Examine type of message. Accept DHCP offers or BOOTP replies. */

        option = (char *)pickup_opt (dhcpcMsgIn.dhcp, DHCPLEN (dhcpcMsgIn.udp),
                                     _DHCP_MSGTYPE_TAG);

        if (option == NULL)
            {
            /* 
             * Message type not found - check message length. Ignore
             * untyped DHCP messages, but accept (shorter) BOOTP replies.
             * This test might still treat (illegal) untyped DHCP messages
             * like BOOTP messages if they are short enough, but that case
             * can't be avoided.
             */

            if (DHCPLEN (dhcpcMsgIn.udp) > DFLTBOOTPLEN)
                return (OK);
            }
        else
            {
            if (*OPTBODY (option) != DHCPOFFER)
                return (OK);
            }

        /* Allocate memory for configuration parameters. */

        pParams = (struct dhcp_param *)calloc (1, sizeof (struct dhcp_param));
        if (pParams == NULL)
            return (OK);

        /* Fill in host requirements defaults. */

        dhcpcDefaultsSet (pParams);

        /* Check offered parameters. Save in lease structure if acceptable. */

        if (dhcp_msgtoparam (dhcpcMsgIn.dhcp, DHCPLEN (dhcpcMsgIn.udp),
                                 pParams) == OK)
            {
            /* 
             * Accept static BOOTP address or verify that the
             * address offered by the DHCP server is non-zero
             * and check offered lease length against minimum value.
             */

            if ( (pParams->msgtype == DHCP_BOOTP ||
                 pParams->server_id.s_addr != 0) &&
                pParams->lease_duration >= dhcpcMinLease)
                {
                /*
                 * Initial offer accepted. Set lease data
                 * to execute next routine and start timer.
                 */

                pParams->lease_origin = pLeaseData->initEpoch;
                pLeaseData->dhcpcParam = pParams;
                pLeaseData->currState = SELECTING;

                /*
                 * Reset timer from retransmission
                 * interval to limit for collecting replies. 
                 */

                wdCancel (pLeaseData->timer);

                wdStart (pLeaseData->timer, sysClkRateGet() * 
                                            pLeaseData->leaseReqSpec.waitsecs,
                         (FUNCPTR)alarm_selecting, (int)pLeaseData);
                }
            else
                {
                /* 
                 * Offer is insufficient. Remove stored parameters
                 * and set lease data to repeat current routine. 
                 */

                pLeaseData->currState = WAIT_OFFER;
                clean_param (pParams);
                free (pParams);
                }
            }
        else
            {
            /*
             * Conversion unsuccessful - remove stored parameters
             * and set lease data to repeat current routine.
             */

            pLeaseData->currState = WAIT_OFFER;
            clean_param (pParams);
            free (pParams);
            }
        }

    return (OK);
    }

/*******************************************************************************
*
* selecting - Second offering state of client finite state machine
*
* This routine continues the second state of the finite state machine. 
* It compares additional offers received from DHCP servers to the current
* offer, and selects the offer which provides the longest lease. When the 
* time limit specified by the DHCPC_OFFER_TIMEOUT definition passes,
* processing of the selected DHCP offer will continue with the requesting() 
* routine. If no DHCP offers were received, the BOOTP reply selected by the 
* wait_offer() routine will be used by the lease.
*
* .IP
* This routine is invoked by the event handler of the client monitor task,
* and should only be called internally. Any user requests generated by
* incorrect calls or delayed responses to the dhcpcBind() and dhcpcVerify() 
* routines are ignored.
*
* RETURNS: OK (processing complete), DHCPC_DONE (remove lease),
*          DHCPC_MORE (continue), or ERROR.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int selecting
    (
    EVENT_DATA * 	pEvent 	/* pointer to event descriptor */
    )
    {
    struct dhcp_param *pParams = NULL;
    struct sockaddr_in dest;
    struct ifnet * pIf;

    int status;
    LEASE_DATA * 	pLeaseData = NULL;
    char * 		pMsgData;
    int 		length;
    char * 	option;

#ifdef DHCPC_DEBUG
    logMsg ("dhcp: Entered SELECTING state.\n", 0, 0, 0, 0, 0, 0);
#endif

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pEvent->leaseId;

    /*
     * The DHCP_USER_RELEASE event occurs in response to the dhcpcRelease()
     * or dhcpcShutdown() call. Remove all data structures for this lease.
     */

    if (pEvent->type == DHCP_USER_RELEASE)
        {
        dhcpcLeaseCleanup (pLeaseData);
        return (DHCPC_DONE);
        }

    /* Ignore bind and verify user events, which are meaningless here. */

    if (pEvent->source == DHCP_USER_EVENT)
        return (OK);

    if (pEvent->type == DHCP_TIMEOUT)
        {
        /* Collection time ended - parameters structure holds chosen offer. */

        if (pLeaseData->dhcpcParam->msgtype == DHCP_BOOTP)
            {
            /* 
             * No DHCP request is needed if a BOOTP reply is chosen. Set
             * lease data to process future events with the bound() routine.
             */

            pLeaseData->leaseType = DHCP_BOOTP;

            pLeaseData->prevState = SELECTING;
            pLeaseData->currState = BOUND;

            status = use_parameter (pLeaseData->dhcpcParam, pLeaseData);

            semTake (dhcpcMutexSem, WAIT_FOREVER);
            if (status != 0)
                {
#ifdef DHCPC_DEBUG
                logMsg ("Error configuring network. Shutting down.\n",
                        0, 0, 0, 0, 0, 0);
#endif
                pLeaseData->leaseGood = FALSE;
                }
            else
                {
                pLeaseData->leaseGood = TRUE;
                }
            semGive (dhcpcMutexSem);

            return (OK);
            }

        /* 
         * A DHCP offer was selected. Build and send the DHCP request using
         * the original transaction ID from the discover message.
         */

        length = make_request (pLeaseData, REQUESTING, FALSE);
        if (length < 0) 
            {
#ifdef DHCPC_DEBUG
            logMsg ("Error making DHCP request. Entering INIT state.\n", 
                    0, 0, 0, 0, 0, 0);
#endif
            pLeaseData->prevState = SELECTING;
            pLeaseData->currState = INIT;
            return (DHCPC_MORE);
            }

        dhcpcMsgOut.udp->uh_sum = 0;
        dhcpcMsgOut.udp->uh_sum = udp_cksum (&spudph, (char *)dhcpcMsgOut.udp, 
                                             ntohs (spudph.ulen));

        bzero ( (char *)&dest, sizeof (struct sockaddr_in));

        dest.sin_len = sizeof (struct sockaddr_in);
        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = dhcpcMsgOut.ip->ip_dst.s_addr;

        pIf = pLeaseData->ifData.iface;

        if (dhcpSend (pIf, &dest, sbuf.buf, length, TRUE) == ERROR)
            {
#ifdef DHCPC_DEBUG
            logMsg ("Can't send DHCPREQUEST\n", 0, 0, 0, 0, 0, 0);
#endif
            pLeaseData->prevState = SELECTING;
            pLeaseData->currState = INIT;
            return (DHCPC_MORE);
            }

        /*
         * DHCP request sent. Set lease data to execute next state and
         * start the retransmission timer.
         */

        pLeaseData->prevState = SELECTING;
        pLeaseData->currState = REQUESTING;
        pLeaseData->timeout = FIRSTTIMER;
        pLeaseData->numRetry = 0;

        wdStart (pLeaseData->timer, sysClkRateGet() * 
                                    SLEEP_RANDOM (pLeaseData->timeout),
                 (FUNCPTR)retrans_requesting, (int)pLeaseData);
        }
    else
        {
        /*
         * Process DHCP message stored in receive buffer by monitor task. 
         * The 4-byte alignment of the IP header needed by Sun BSP's is
         * guaranteed by the Berkeley Packet Filter during input.
         */

        pMsgData = pLeaseData->msgBuffer;

        align_msg (&dhcpcMsgIn, pMsgData);

        /* Examine type of message. Only accept DHCP offers. */

        option = (char *)pickup_opt (dhcpcMsgIn.dhcp, DHCPLEN (dhcpcMsgIn.udp),
                                     _DHCP_MSGTYPE_TAG);
        if (option == NULL)
            {
            /*
             * Message type not found -  discard untyped DHCP messages, and
             * any BOOTP replies.
             */

            return (OK);
            }
        else
            {
            if (*OPTBODY (option) != DHCPOFFER)
                return (OK);
            }

        /* Allocate memory for configuration parameters. */

        pParams = (struct dhcp_param *)calloc (1, sizeof (struct dhcp_param));
        if (pParams == NULL)
            return (OK);

        /* Fill in host requirements defaults. */

        dhcpcDefaultsSet (pParams);

        /* Switch to offered parameters if they provide a longer DHCP lease.
         *
	 * First, check that we can decode the parameters.
         */

        if ((dhcp_msgtoparam (dhcpcMsgIn.dhcp, DHCPLEN (dhcpcMsgIn.udp),
                              pParams) == OK) &&

            /* Then check that the lease time is equal to or greater than
             * the minimum lease time.
             */

            (pParams->server_id.s_addr != 0 &&
             pParams->lease_duration >= dhcpcMinLease) &&

            /* Finally, take any DHCP message over BOOTP,
             * or take any lease with a longer duration than
             * we requested.
             */

             (pLeaseData->dhcpcParam->msgtype == DHCP_BOOTP ||
              pParams->lease_duration > 
              pLeaseData->dhcpcParam->lease_duration))
            {
            pParams->lease_origin = pLeaseData->initEpoch;
            clean_param (pLeaseData->dhcpcParam);
            free (pLeaseData->dhcpcParam);
            pLeaseData->dhcpcParam = pParams;
            }
        else
            {
            clean_param (pParams);
            free (pParams);
            }
        }
    return (OK);
    }


/*******************************************************************************
*
* requesting - Lease request state of client finite state machine
*
* This routine implements the third state of the client finite state machine.
* It handles all processing until a response to a transmitted DHCP request
* message is received from the selected DHCP server or the retransmission 
* limit is reached. The DHCP request is retransmitted if a timeout occurs. 
* If the request is acknowledged, processing will continue with the bound()
* routine. If it is refused or the retransmission limit is reached, the 
* negotiation process will restart with the init() routine.
* .IP
* This routine is invoked by the event handler of the client monitor task,
* and should only be called internally. Any user requests generated by
* incorrect calls or delayed responses to the dhcpcBind() and dhcpcVerify()
* routines are ignored.
*
* RETURNS: OK (processing complete), DHCPC_DONE (remove lease),
*          DHCPC_MORE (continue), or ERROR.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int requesting
    (
    EVENT_DATA * 	pEvent 	/* pointer to event descriptor */
    )
    {
    int arpans = 0;
    char *option = NULL;
    char errmsg[255];
    int timer = 0;
    int retry = 0;
    struct dhcp_param tmpparam;
    struct dhcp_reqspec tmp_reqspec;
    int status;
    int msgtype;
    LEASE_DATA * 	pLeaseData = NULL;
    char * 		pMsgData;
    int 		length;

#ifdef DHCPC_DEBUG
    char newAddr [INET_ADDR_LEN];

    logMsg ("dhcpc: Entering requesting state.\n", 0, 0, 0, 0, 0, 0);
#endif 

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pEvent->leaseId;

    /*
     * The DHCP_USER_RELEASE event occurs in response to the dhcpcRelease()
     * or dhcpcShutdown() call. Remove all data structures for this lease.
     */

    if (pEvent->type == DHCP_USER_RELEASE)
        {
        dhcpcLeaseCleanup (pLeaseData);
        return (DHCPC_DONE);
        }

    /*
     * Ignore bind, verify and inform user events,
     * which are meaningless for this state.
     */

    if (pEvent->source == DHCP_USER_EVENT)
        return (OK);

    bzero (errmsg, sizeof (errmsg));
    bzero ( (char *)&tmp_reqspec, sizeof (tmp_reqspec));

    if (pEvent->type == DHCP_TIMEOUT)
        {
#ifdef DHCPC_DEBUG
        logMsg ("Retransmission from requesting state.\n", 0, 0, 0, 0, 0, 0);
#endif
        /* Handle timeout - no DHCP reply received yet. */

        retry = pLeaseData->numRetry;
        timer = pLeaseData->timeout;

        retry++;
        if (retry > REQUEST_RETRANS)    /* Retransmission limit reached. */
            {
#ifdef DHCPC_DEBUG
            logMsg ("Client can't get ACK/NAK reply from server\n",
                    0, 0, 0, 0, 0, 0);
#endif

            if (pLeaseData->prevState == INFORMING)
                {
                /* Unable to get additional parameters. */

                return (ERROR);
                }

            pLeaseData->prevState = REQUESTING;
            pLeaseData->currState = INIT;
            return (DHCPC_MORE);
            }
        else
            {
            /*
             * Try to retransmit DHCP request or inform
             * message using the same transaction ID.
             */

            if (pLeaseData->prevState == INFORMING)
                msgtype = INFORMING;
            else
                msgtype = REQUESTING;

            length = make_request (pLeaseData, msgtype, FALSE);
            if (length < 0)
                {
#ifdef DHCPC_DEBUG
                logMsg ("Error making DHCP request. Can't continue.\n",
                        0, 0, 0, 0, 0, 0);
#endif

                if (pLeaseData->prevState == INFORMING)
                    {
                    /* Unable to get additional parameters. */

                    return (ERROR);
                    }

                pLeaseData->prevState = REQUESTING;
                pLeaseData->currState = INIT;
                return (DHCPC_MORE);
                }

            gen_retransmit (pLeaseData, length);
            }

        if (timer < MAXTIMER)
            {
            /* Double retransmission delay with each attempt. (RFC 1541). */

            timer *= 2;
            }

        /* Set retransmission timer to randomized exponential backoff. */

        wdStart (pLeaseData->timer, sysClkRateGet() * SLEEP_RANDOM (timer),
                 (FUNCPTR)retrans_requesting, (int)pLeaseData);

        pLeaseData->timeout = timer;
        pLeaseData->numRetry = retry;
        }
    else
        {
        bzero ( (char *)&tmpparam, sizeof (tmpparam));

        /*
         * Process DHCP message stored in receive buffer by monitor task. 
         * The 4-byte alignment of the IP header needed by Sun BSP's is
         * guaranteed by the Berkeley Packet Filter during input.
         */

        pMsgData = pLeaseData->msgBuffer;

        align_msg (&dhcpcMsgIn, pMsgData);

        /* Examine type of message. Accept DHCPACK or DHCPNAK replies. */

        option = (char *)pickup_opt (dhcpcMsgIn.dhcp, DHCPLEN (dhcpcMsgIn.udp),
                                     _DHCP_MSGTYPE_TAG);
        if (option == NULL)
            {
            /*
             * Message type not found -  discard untyped DHCP messages, and
             * any BOOTP replies.
             */

            return (OK);
            }

        /*
         * Message type available. Ignore unexpected values. If the client
         * does not receive a valid response within the expected interval, it
         * will timeout and retransmit the request - RFC 2131, section 3.1.5.
         */

        msgtype = *OPTBODY (option);

        if (msgtype != DHCPACK && msgtype != DHCPNAK)
            {
            return (OK);
            }

        if (msgtype == DHCPNAK)
            {
#ifdef DHCPC_DEBUG
            logMsg ("Got DHCPNAK from server\n", 0, 0, 0, 0, 0, 0);

            option = (char *)pickup_opt (dhcpcMsgIn.dhcp,
                                         DHCPLEN(dhcpcMsgIn.udp),
                                         _DHCP_ERRMSG_TAG);
            if (option != NULL &&
                nvttostr (OPTBODY (option), errmsg,
                          (int)DHCPOPTLEN (option)) == 0)
                logMsg ("DHCPNAK contains the error message \"%s\"\n",
                         (int)errmsg, 0, 0, 0, 0, 0);
#endif

            /* Ignore invalid responses to DHCP inform message. */

            if (pLeaseData->prevState == INFORMING)
                return (OK);

            clean_param (pLeaseData->dhcpcParam);
            free (pLeaseData->dhcpcParam);
            pLeaseData->dhcpcParam = NULL;

            pLeaseData->prevState = REQUESTING;
            pLeaseData->currState = INIT;
            return (DHCPC_MORE);
            }

        /*
         * Got acknowledgement: fill in host requirements defaults
         * and add any parameters from message options.
         */

        dhcpcDefaultsSet (&tmpparam);

        if (dhcp_msgtoparam (dhcpcMsgIn.dhcp, DHCPLEN (dhcpcMsgIn.udp),
                             &tmpparam) == OK)
            {
            /* Options parsed successfully - test as needed. */

            if (pLeaseData->prevState == INFORMING)
                {
                /*
                 * Bypass ARP test and remaining steps after saving
                 * additional parameters.
                 */

                merge_param (pLeaseData->dhcpcParam, &tmpparam);
                *(pLeaseData->dhcpcParam) = tmpparam;

                /*
                 * If an event notification hook is present, send an
                 * indication about the new set of parameters.
                 */

                if (pLeaseData->eventHookRtn != NULL)
                    (* pLeaseData->eventHookRtn) (DHCPC_LEASE_NEW,
                                                  pEvent->leaseId);

                /*
                 * Signal the successful completion of the message
                 * exchange if the dhcpcInformGet() call is executing
                 * synchronously.
                 */

                if (pLeaseData->waitFlag)
                    semGive (pLeaseData->leaseSem);

                return (OK);
                }

            if ( (arpans = arp_check (&tmpparam.yiaddr, 
                                          &pLeaseData->ifData)) == OK)
                {
                merge_param (pLeaseData->dhcpcParam, &tmpparam);
                *(pLeaseData->dhcpcParam) = tmpparam;
                pLeaseData->dhcpcParam->lease_origin = 
                    pLeaseData->initEpoch;
#ifdef DHCPC_DEBUG
                inet_ntoa_b (pLeaseData->dhcpcParam->yiaddr, newAddr);
                logMsg ("Got DHCPACK (IP = %s, duration = %d secs)\n",
                        (int)newAddr,
                        pLeaseData->dhcpcParam->lease_duration,
                        0, 0, 0, 0);
#endif
                pLeaseData->prevState = REQUESTING;
                pLeaseData->currState = BOUND;

                /* Use retrieved configuration parameters. */ 

                status = use_parameter (pLeaseData->dhcpcParam, 
                                        pLeaseData);
                if (status != 0)
                    {
#ifdef DHCPC_DEBUG
                    logMsg ("Error configuring network. Shutting down.\n",
                             0, 0, 0, 0, 0, 0);
#endif
                    release (pLeaseData, FALSE);

                    semTake (dhcpcMutexSem, WAIT_FOREVER);
                    pLeaseData->leaseGood = FALSE;
                    semGive (dhcpcMutexSem);
                    }
                else
                    {
                    semTake (dhcpcMutexSem, WAIT_FOREVER);
                    pLeaseData->leaseGood = TRUE;
                    semGive (dhcpcMutexSem);
                    }
                if (status == 0)
                    return (DHCPC_MORE);
                else
                    return (ERROR);
                }
            }

        /*
         * Invalid parameters or (for a full lease negotiation) a
         * failed ARP probe (address in use). Ignore invalid parameters
         * for DHCP inform messages.
         */

        if (pLeaseData->prevState == INFORMING)
            return (OK);

        /*
         * For the full lease negotiation, send the DHCPDECLINE which
         * is now required when an ARP probe fails: RFC 2131, section 3.1.5.
         */

        set_declinfo (&tmp_reqspec, pLeaseData, errmsg, arpans);
        dhcp_decline (&tmp_reqspec, &pLeaseData->ifData);
        clean_param (pLeaseData->dhcpcParam);
        free (pLeaseData->dhcpcParam);
        pLeaseData->dhcpcParam = NULL;
#ifdef DHCPC_DEBUG
        logMsg ("Received unacceptable DHCPACK. Entering INIT state.\n",
                0, 0, 0, 0, 0, 0);
#endif
        pLeaseData->prevState = REQUESTING;
        pLeaseData->currState = INIT;
        return (DHCPC_MORE);
        }    /* End of DHCP message processing. */
    return (OK);
    }

/* dhcpcBoot.c - DHCP client finite state machine definition (boot time) */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01m,25apr02,wap  transmit DHCPDECLINEs as broadcasts, not unicasts (SPR
                 #75119)
01l,23apr02,wap  use dhcpTime() instead of time() (SPR #68900) and initialize
                 dhcpcBootStates correctly (SPR #75042)
01k,12oct01,rae  merge from truestack ver 01q, base 01g
                 SPRs 65264, 70116, 27426, 69731
01j,26oct00,spm  fixed byte order of transaction identifier; removed reset code
01i,26oct00,spm  fixed modification history after tor3_x merge
01h,23oct00,niq  merged from version 01j of tor3_x branch (base version 01f)
01g,04dec97,spm  added code review modifications
01f,06oct97,spm  removed reference to deleted endDriver global; replaced with
                 support for dynamic driver type detection
01e,02sep97,spm  removed name conflicts with runtime DHCP client (SPR #9241)
01d,26aug97,spm  major overhaul: included version 01h of dhcpcState1.c and
                 version 01i of dhcpc_subr.c to retain single-lease library
                 for boot time use
01c,06aug97,spm  removed parameters linked list to reduce memory required
01b,07apr97,spm  rewrote documentation
01a,29jan97,spm  created by modifying dhcpc.c module
*/

/*
DESCRIPTION
This library contains the boot-time DHCP client implementation. The library
contains routines to obtain a set of boot parameters using DHCP. Because the 
bootstrap loader handles all the system configuration, the boot-time client 
will not apply any configuration parameters received to the underlying network
interface.

INTERNAL 
This module contains a modified version of the DHCP client finite state
machine which eliminates all states from BOUND onwards. Provided that the 
obtained lease exceeds the download time for the runtime image, these routines
are not needed by the ROM-resident bootstrap loader. Their removal reduces the 
size of the boot ROM image so that the DHCP client can be used with targets 
like the MV147 which have limited ROM capacity. The code size is further 
reduced by eliminating the routines which change the network interface settings
and maintain the correct routing table entries. Although necessary to prevent 
access to an unconfigured interface during runtime, it is overkill at boot 
time, since the system will not start if an error occurs. Finally, the runtime 
DHCP client supports the retrieval and maintenance of multiple leases and 
allows users to renew or release the corresponding parameters. Those 
capabilities are not necessary at boot time.

INCLUDE_FILES: dhcpcBootLib.h
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

#include "vxWorks.h"
#include "vxLib.h"             /* checksum() declaration. */
#include "netLib.h"
#include "inetLib.h"
#include "taskLib.h"
#include "sysLib.h"
#include "ioLib.h"
#include "sockLib.h"
#include "wdLib.h"
#include "rngLib.h"
#include "logLib.h"
#include "muxLib.h"

#include "stdio.h"
#include "stdlib.h"
#include "sys/ioctl.h"
#include "time.h"
#include "netinet/in.h"
#include "netinet/ip.h"
#include "netinet/udp.h"

#include "dhcpcBootLib.h"
#include "dhcp/dhcpcCommonLib.h"

#include "bpfDrv.h"

/* defines */

#define MAX_BOOT_STATES (INFORMING + 1) /* Function pointers for all states. */
#define	SLEEP_RANDOM(timer) ( (timer - 1) + (rand () % 2) )
#define REQUEST_RETRANS   4  /* Number of retries, maximum of 5. (RFC 1541). */

    /* Berkeley Packet Filter instructions for catching ARP messages. */

LOCAL struct bpf_insn arpfilter[] = {
  BPF_STMT(BPF_LD+BPF_TYPE, 0),    /* Save lltype in accumulator */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, ETHERTYPE_ARP, 0, 9),  /* ARP in frame? */

  /*
   * The remaining statements use the (new) BPF_HLEN alias to avoid
   * any link-layer dependencies.
   */

  BPF_STMT(BPF_LD+BPF_H+BPF_ABS+BPF_HLEN, 6),    /* A <- ARP OP field */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, ARPOP_REPLY, 0, 7),    /* ARP reply ? */
  BPF_STMT(BPF_LDX+BPF_HLEN, 0),           /* X <- frame data offset */
  BPF_STMT(BPF_LD+BPF_B+BPF_IND, 4),       /* A <- hardware size */
  BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0),      /* A <- sum of variable offsets */
  BPF_STMT(BPF_MISC+BPF_TAX, 0),           /* X <- sum of variable offsets */
  BPF_STMT(BPF_LD+BPF_W+BPF_IND, 8),       /* A <- sender IP address */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, -1, 0, 1),   /* -1 replaced with real IP */
  BPF_STMT(BPF_RET+BPF_K, -1),                     /* copy entire frame */
  BPF_STMT(BPF_RET+BPF_K, 0)          /* unrecognized message: ignore frame */
  };

LOCAL struct bpf_program arpread = {
    sizeof (arpfilter) / sizeof (struct bpf_insn),
    arpfilter
    };

LOCAL int bpfArpDev; 	/* Gets any replies to ARP probes. */

LOCAL u_short dhcps_port;              /* Server port, in network byte order. */
LOCAL u_short dhcpc_port;              /* Client port, in network byte order. */

IMPORT struct bpf_insn dhcpfilter[];    /* For updating xid test */
IMPORT struct bpf_program dhcpread;

/* globals */

struct dhcp_param * 	dhcpcBootParam = NULL; /* Configuration parameters */

IMPORT LEASE_DATA dhcpcBootLeaseData;
IMPORT int 	dhcpcBindType; 	/* DHCP or BOOTP reply selected? */

/* locals */

LOCAL BOOL 			dhcpcOldFlag; /* Use old (padded) messages? */
LOCAL int			dhcpcCurrState;   /* Current state */
LOCAL int			dhcpcPrevState;   /* Previous state */
LOCAL int 			dhcpcMsgLength;   /* Current message size. */
LOCAL struct if_info		dhcpcIntface;     /* Network interface */
LOCAL struct buffer		sbuf; 	/* Buffer for outgoing messages */
LOCAL struct msg 		dhcpcMsgIn;  /* Incoming message components */
LOCAL struct msg 		dhcpcMsgOut; /* Outgoing message components */
LOCAL unsigned char		dhcpCookie [MAGIC_LEN] = RFC1048_MAGIC;
LOCAL struct ps_udph 		spudph;

LOCAL int (*dhcpcBootStates [MAX_BOOT_STATES]) ();

LOCAL int handle_ip();
LOCAL int handle_num();
LOCAL int handle_ips();
LOCAL int handle_str();
LOCAL int handle_bool();
LOCAL int handle_ippairs();
LOCAL int handle_nums();
LOCAL int handle_list();

LOCAL int (*handle_param [MAXTAGNUM]) () = 
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

/* forward declarations */

int dhcp_boot_client (struct if_info *, int, int, BOOL);
LOCAL void msgAlign (struct msg *, char *);
LOCAL int arp_check (struct in_addr *, struct if_info *);
LOCAL int initialize (int, int);
LOCAL void set_declinfo (struct dhcp_reqspec *, char *);
LOCAL int make_discover (void);
LOCAL int make_request (struct dhcp_param *, int);
LOCAL int dhcp_decline (struct dhcp_reqspec *);
LOCAL int dhcp_msgtoparam (struct dhcp *, int, struct dhcp_param *);
LOCAL int merge_param (struct dhcp_param *, struct dhcp_param *);
LOCAL long generate_xid ();
LOCAL int clean_param (struct dhcp_param *);
LOCAL int dhcpcBootInitState();
LOCAL int dhcpcBootWaitOffer();
LOCAL int dhcpcBootSelecting();
LOCAL int dhcpcBootRequesting();
LOCAL int dhcpcBootInforming();

#ifdef DHCPC_DEBUG
LOCAL int nvttostr (char *, char *, int);
#endif

/*******************************************************************************
*
* dhcp_boot_client - Exercise client finite state machine
*
* This routine uses the finite state machine to obtain a set of configuration
* parameters. When the ROM-resident bootstrap loader issues a dhcpcBootBind()
* call, this routine executes the state machine up until the BOUND state.
* The states from BOUND onward are omitted to reduce the size of the boot ROM
* images. If an address is already present, the routine executes the isolated
* portion of the state machine which sends a DHCP INFORM message and waits for
* an acknowledgement containing any additional parameters. The <bindFlag>
* argument selects between these alternatives. In the first case, the boot
* file downloaded by the bootstrap loader must spawn a task containing the
* full state machine defined in the DHCP client runtime library to monitor
* the established lease. 
*
* RETURNS: OK if message exchange completed, or value of failed state on error.
*
* ERRNO: N/A
*
* SEE ALSO: dhcpcLib
*
* NOMANUAL
*/

int dhcp_boot_client
    (
    struct if_info *ifp,
    int         serverPort,     /* port used by DHCP servers */
    int         clientPort,     /* port used by DHCP clients */
    BOOL 	bindFlag 	/* establish new lease? */
    )
    {
    int next_state = 0;
    int retval = 0;

    dhcpcBootStates [INIT] = dhcpcBootInitState;
    dhcpcBootStates [WAIT_OFFER] = dhcpcBootWaitOffer;
    dhcpcBootStates [SELECTING] = dhcpcBootSelecting;
    dhcpcBootStates [REQUESTING] = dhcpcBootRequesting;
    dhcpcBootStates [INFORMING] = dhcpcBootInforming;

    dhcpcIntface = *ifp;

    if ( (retval = initialize (serverPort, clientPort)) < 0)
        return (retval);

    if (bindFlag)
        {
        /* 
         * Full lease negotiation process. Use the state machine to
         * obtain configuration parameters, including an address.
         */

        dhcpcPrevState = dhcpcCurrState = INIT; 
        }
    else
        {
        /*
         * Partial message exchange for known (externally assigned) address.
         * Send an INFORM message and wait for an acknowledgement containing
         * additional parameters.
         */

        dhcpcPrevState = dhcpcCurrState = INFORMING;
        }

    while (next_state != BOUND)
        {
        if ( (next_state = (*dhcpcBootStates [dhcpcCurrState]) ()) < 0) 
            {
#ifdef DHCPC_DEBUG
            logMsg ("Error in finite state machine.\n", 0, 0, 0, 0, 0, 0);
#endif
            if (dhcpcBootParam != NULL)
                {
                clean_param (dhcpcBootParam);
                free (dhcpcBootParam);
                dhcpcBootParam = NULL;
                }
            close (bpfArpDev);

            bpfDevDelete ("/bpf/dhcpc-arp");

            free (sbuf.buf);
            return (ERROR);
            }

       /* If not a full lease negotiation, keep indication of known address. */

        if (dhcpcPrevState != INFORMING)
            dhcpcPrevState = dhcpcCurrState;

        dhcpcCurrState = next_state;
#ifdef DHCPC_DEBUG
        logMsg ("next state= %d\n", next_state, 0, 0, 0, 0, 0);
#endif
        }
    close (bpfArpDev);

    bpfDevDelete ("/bpf/dhcpc-arp");

    free (sbuf.buf);

    return (OK);   /* Bind successful. */
    } 

/*******************************************************************************
*
* gen_retransmit - generic retransmission after timeout
*
* This routine retransmits the current DHCP client message (a discover or
* a request message) when the appropriate timeout interval expires.
* It is called from multiple locations in the finite state machine.
*
* RETURNS: 0 if transmission completed, or negative value on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int gen_retransmit
    (
    void
    )
    {
    time_t	curr_epoch = 0;
    struct sockaddr_in dest;
    BOOL bcastFlag;

    if (dhcpTime (&curr_epoch) == -1)
        return (-1);

    /* Update the appropriate fields in the current DHCP message. */

    if (dhcpcCurrState != REQUESTING)
        dhcpcMsgOut.dhcp->secs = htons (curr_epoch -
                                        dhcpcBootLeaseData.initEpoch);

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

    if (dhcpSend (dhcpcBootLeaseData.ifData.iface, &dest, 
                  sbuf.buf, dhcpcMsgLength, bcastFlag)
           == ERROR)
        return (-2);
    return(0);
    }


/*******************************************************************************
*
* retrans_wait_offer - retransmission in WAIT_OFFER state
*
* This routine retransmits the DHCP discover message when the timeout interval 
* for receiving an initial lease offer expires. It is called in response
* to a watchdog timer at intervals specified by the wait_offer() routine.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL void retrans_wait_offer
    (
    int unused 		/* unused watchdog argument */
    )
    {
    int status;

    if (dhcpcCurrState != WAIT_OFFER)
        return;

    /* Construct and send a timeout message to the finite state machine. */

    status = dhcpcEventAdd (DHCP_AUTO_EVENT, DHCP_TIMEOUT, NULL, TRUE);

#ifdef DHCPC_DEBUG
    if (status == ERROR)
        logMsg ("Can't notify state machine\n", 0, 0, 0, 0, 0, 0);
#endif

    return;
    }

/*******************************************************************************
*
* alarm_selecting - signal when collection time expires 
*
* This routine sends a timeout notification to the finite state machine 
* when the interval for collecting additional DHCP offers has expired. It 
* is called in response to a watchdog timer schedule by the selecting()
* routine. The timeout causes the state machine to advance to the REQUESTING
* state.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL void alarm_selecting
    (
    int unused 		/* unused watchdog argument */
    )
    {
    int 	status;

    if (dhcpcCurrState == SELECTING) 
        {
        /* Construct and send a timeout message to the finite state machine. */

        status = dhcpcEventAdd (DHCP_AUTO_EVENT, DHCP_TIMEOUT, NULL, TRUE);

#ifdef DHCPC_DEBUG
        if (status == ERROR)
            logMsg ("Can't notify state machine\n", 0, 0, 0, 0, 0, 0);
#endif
        }
    return;
    }

/*******************************************************************************
*
* retrans_requesting - retransmission in requesting state
*
* This routine retransmits the DHCP request message when the timeout interval 
* for receiving a server reply expires. It is called in response to a watchdog 
* timer at intervals specified by the requesting() routine.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL void retrans_requesting
    (
    int unused 		/* unused watchdog argument */
    )
    {
    int		status;

    if (dhcpcCurrState != REQUESTING)
        return;

    /* Construct and send a timeout message to the finite state machine. */

    status = dhcpcEventAdd (DHCP_AUTO_EVENT, DHCP_TIMEOUT, NULL, TRUE);

#ifdef DHCPC_DEBUG
    if (status == ERROR)
        logMsg ("Can't notify state machine\n", 0, 0, 0, 0, 0, 0);
#endif

    return;
    }

/*******************************************************************************
*
* dhcpcBootInforming - external configuration state of boot-time state machine
*
* This routine begins the inform message process, which obtains additional
* parameters for a host configured with an external address. This processing
* is isolated from the normal progression through the state machine. Like the
* DHCP discover message, the inform message is the first transmission.
* However, the only valid response is an acknowledgement by a server. As a
* result, the process is a hybrid between the implementations of the initial 
* state and the requesting state which finalizes the lease establishment.
* This routine implements the initial state of the finite state machine for
* an externally assigned IP address.
*
* RETURNS: Next state of state machine, or -1 if error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int dhcpcBootInforming
    (
    void
    )
    {
    struct sockaddr_in 	dest;

    bzero (sbuf.buf, sbuf.size);

#ifdef DHCPC_DEBUG
    logMsg("dhcpc: Entered INFORM state.\n", 0, 0, 0, 0, 0, 0);
#endif

    /* Create and broadcast DHCP INFORM message. */

    dhcpcMsgLength = make_request (dhcpcBootParam, INFORMING);
    if (dhcpcMsgLength < 0)
        {
#ifdef DHCPC_DEBUG
        logMsg ("Error making DHCP inform message. Can't continue.\n",
                0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    dhcpcMsgOut.dhcp->secs = 0;
    dhcpcMsgOut.udp->uh_sum = 0;
    dhcpcMsgOut.udp->uh_sum = udp_cksum (&spudph, (char *)dhcpcMsgOut.udp, 
                                         ntohs (spudph.ulen));

#ifdef DHCPC_DEBUG
    logMsg ("Sending DHCPINFORM\n", 0, 0, 0, 0, 0, 0);
#endif

    /*
     * Retransmit the message using a link-level broadcast address, which
     * prevents ARP messages if using Ethernet devices and allows transmission
     * from interfaces without assigned IP addresses.
     */

    bzero ( (char *)&dest, sizeof (struct sockaddr_in));

    dest.sin_len = sizeof (struct sockaddr_in);
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dhcpcMsgOut.ip->ip_dst.s_addr;

    if (dhcpSend (dhcpcIntface.iface, &dest, sbuf.buf, dhcpcMsgLength, TRUE)
           == ERROR)
        {
#ifdef DHCPC_DEBUG
        logMsg("Can't send DHCPINFORM\n", 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    /* Set retransmission timer to randomized exponential backoff. */

    dhcpcBootLeaseData.timeout = FIRSTTIMER;
    dhcpcBootLeaseData.numRetry = 0;

    wdStart (dhcpcBootLeaseData.timer, sysClkRateGet() *
                                  SLEEP_RANDOM (dhcpcBootLeaseData.timeout),
             (FUNCPTR)retrans_requesting, 0);

    return (REQUESTING);	/* Next state is REQUESTING */
    }

/*******************************************************************************
*
* dhcpcBootInitState - initial state of boot-time client finite state machine
*
* This routine implements the initial state of the finite state machine. 
* After a random backoff delay, it broadcasts a DHCP discover message. This 
* state may be re-entered after a later state if a recoverable error occurs.
*
* RETURNS: Next state of state machine, or -1 if error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int dhcpcBootInitState
    (
    void
    )
    {
    struct sockaddr_in 	dest;

    bzero (sbuf.buf, sbuf.size);

#ifdef DHCPC_DEBUG
    logMsg("dhcpc: Entered INIT state.\n", 0, 0, 0, 0, 0, 0);
#endif

    /*
     * Set lease to generate newer RFC 2131 messages initially. The client
     * will revert to the older message format if it does not receive a
     * response to the initial set of discover messages. Older servers might
     * ignore messages less than the minimum length obtained with a fixed
     * options field. The older format pads the field to reach that length.
     */

    dhcpcOldFlag = FALSE;

    wdCancel (dhcpcBootLeaseData.timer);	/* Reset watchdog timer. */

    dhcpcMsgLength = make_discover ();    /* Create DHCP_DISCOVER message. */

    /* 
     * Random delay from one to ten seconds to avoid startup congestion.
     * (Delay length specified in RFC 1541).
     *
     */

    taskDelay (sysClkRateGet () * (1 + (rand () % INIT_WAITING)) );

    dhcpcMsgOut.dhcp->secs = 0;
    dhcpcMsgOut.udp->uh_sum = 0;
    dhcpcMsgOut.udp->uh_sum = udp_cksum (&spudph, (char *)dhcpcMsgOut.udp, 
                                         ntohs (spudph.ulen));

#ifdef DHCPC_DEBUG
    logMsg ("Sending DHCPDISCOVER\n", 0, 0, 0, 0, 0, 0);
#endif

    /*
     * Retransmit the message using a link-level broadcast address, which
     * prevents ARP messages if using Ethernet devices and allows transmission
     * from interfaces without assigned IP addresses.
     */

    bzero ( (char *)&dest, sizeof (struct sockaddr_in));

    dest.sin_len = sizeof (struct sockaddr_in);
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dhcpcMsgOut.ip->ip_dst.s_addr;

    if (dhcpSend (dhcpcIntface.iface, &dest, sbuf.buf, dhcpcMsgLength, TRUE)
            == ERROR)
        {
#ifdef DHCPC_DEBUG
        logMsg("Can't send DHCPDISCOVER\n", 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    dhcpcBootLeaseData.timeout = FIRSTTIMER;
    dhcpcBootLeaseData.numRetry = 0;

    if (dhcpTime (&dhcpcBootLeaseData.initEpoch) == -1)
        {
#ifdef DHCPC_DEBUG
        logMsg ("time() error setting initEpoch\n", 0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }

    /* Set timer to randomized exponential backoff before retransmission. */

    wdStart (dhcpcBootLeaseData.timer, sysClkRateGet() *
                                  SLEEP_RANDOM (dhcpcBootLeaseData.timeout),
             (FUNCPTR)retrans_wait_offer, 0);

    return (WAIT_OFFER);	/* Next state is WAIT_OFFER */
    }

/*******************************************************************************
*
* dhcpcBootWaitOffer - Initial offering state of client finite state machine
*
* This routine implements the initial part of the second state of the finite 
* state machine. It waits until the current timeout value to receive a lease 
* offer from  a DHCP server. If a timeout occurs, the DHCP offer message is 
* retransmitted.
*
* RETURNS: Next state of state machine, or -1 if error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int dhcpcBootWaitOffer
    (
    void
    )
    {
    int			arpans = 0;
    char		errmsg [255];
    char * 		pOption;
    int 		timer = 0;
    int 		retry = 0;
    struct dhcp_param *	pParams = NULL;
    char * 		pMsgData;
    int 		status;
    EVENT_DATA 		msgEvent;

    bzero (errmsg, sizeof (errmsg));

#ifdef DHCPC_DEBUG
    logMsg ("dhcpc: Entered WAIT_OFFER state.\n", 0, 0, 0, 0, 0, 0);
#endif

    if (dhcpcPrevState == INIT) 
        {
        /* Clear any previous parameter settings. */

        if (dhcpcBootParam != NULL)
            {
            clean_param (dhcpcBootParam);
            free (dhcpcBootParam);
            dhcpcBootParam = NULL;
            }
        }

    /* Wait for message or retransmission. */

    semTake (dhcpcEventSem, WAIT_FOREVER); 

    status = rngBufGet (dhcpcEventRing, (char *)&msgEvent, sizeof (msgEvent));
    if (status != sizeof (msgEvent))
        {
        /* Expected event missing or incomplete - remain in current state. */

#ifdef DHCPC_DEBUG
        logMsg ("dhcpWaitOffer: Notification error.\n", 0, 0, 0, 0, 0, 0);
#endif
        return (WAIT_OFFER);
        }

    if (msgEvent.type == DHCP_TIMEOUT)
        {
        /* Handle timeout - no DHCP offers received yet. */

        retry = dhcpcBootLeaseData.numRetry;
        timer = dhcpcBootLeaseData.timeout;

        retry++;
        if (retry == DISCOVER_RETRANS)	
            {
#ifdef DHCPC_DEBUG
            logMsg ("No lease offers received by client.\n", 0, 0, 0, 0, 0, 0);
#endif


            /*
             * No response to new DHCP message format, which can be ignored
             * by older DHCP servers as too short. Try the earlier format,
             * which padded the options field to a minimum length. If
             * successful, all subsequent messages will add that padding.
             * The initialization guarantees that the transmit buffer can
             * store the padded message.
             */

            if (!dhcpcOldFlag && dhcpcMsgLength < (DFLTDHCPLEN + UDPHL + IPHL))
                {
                dhcpcOldFlag = TRUE;

                dhcpcMsgLength = make_discover ();
                timer = FIRSTTIMER / 2;     /* Counteract later doubling. */
                retry = 0;
                }
            else
                return (ERROR);
            }

        /* Retransmission required - return after sending message. */

        switch (gen_retransmit ())
            {
            case 0:         /* Transmission successful. */
#ifdef DHCPC_DEBUG
                logMsg ("retransmit DHCPDISCOVER\n", 0, 0, 0, 0, 0, 0);
#endif
                break;
            case -1:        /* Couldn't update timing data */
#ifdef DHCPC_DEBUG
                logMsg ("time() error retransmitting DHCPDISCOVER\n",
                         0, 0, 0, 0, 0, 0);
#endif
                break;
            case -2:        /* Transmission error in output routine */
#ifdef DHCPC_DEBUG
                logMsg ("Can't retransmit DHCPDISCOVER\n",
                            0, 0, 0, 0, 0, 0);
#endif
                break;
            }

        if (timer < MAXTIMER)
            {
            /* Double retransmission delay with each attempt. (RFC 1541). */

            timer *= 2;
            }

        /* Set retransmission timer to randomized exponential backoff. */

        wdStart (dhcpcBootLeaseData.timer, sysClkRateGet() *
                                      SLEEP_RANDOM (timer),
                 (FUNCPTR)retrans_wait_offer, 0);

        dhcpcBootLeaseData.timeout = timer;
        dhcpcBootLeaseData.numRetry = retry;
        }
    else
        {
        /*
         * Process DHCP message stored in receive buffer by monitor task.
         * The 4-byte alignment of the IP header needed by Sun BSP's is
         * guaranteed by the Berkeley Packet Filter during input.
         */

        pMsgData = msgEvent.pMsg;

        msgAlign (&dhcpcMsgIn, pMsgData);

        /* Examine type of message. Accept DHCP offers or BOOTP replies. */

        pOption = (char *)pickup_opt (dhcpcMsgIn.dhcp,
                                      DHCPLEN (dhcpcMsgIn.udp),
                                      _DHCP_MSGTYPE_TAG);

        if (pOption == NULL)
            {
            /* 
             * Message type not found - check message length. Ignore
             * untyped DHCP messages, but accept (shorter) BOOTP replies.
             * This test might still treat (illegal) untyped DHCP messages
             * like BOOTP messages if they are short enough, but that case
             * can't be avoided. Mark the message buffer available after
             * handling the last DHCP message.
             */

            if (DHCPLEN (dhcpcMsgIn.udp) > DFLTBOOTPLEN)
                {
                if (msgEvent.lastFlag)
                    dhcpcMessageList [msgEvent.slot].writeFlag = TRUE;

                return (WAIT_OFFER);
                }
            }
        else
            {
            if (*OPTBODY (pOption) != DHCPOFFER)
                {
                /*
                 * Ignore messages with unexpected types. Mark the message
                 * buffer available after handling the last DHCP message.
                 */

                if (msgEvent.lastFlag)
                    dhcpcMessageList [msgEvent.slot].writeFlag = TRUE;

                return (WAIT_OFFER);
                }
            }

        /* Allocate memory for configuration parameters. */

        pParams = (struct dhcp_param *)calloc (1, sizeof (struct dhcp_param));
        if (pParams == NULL)
            {
            /*
             * Memory for configuration parameters is unavailable. Remain
             * in current state. Mark the message buffer available after
             * handling the last DHCP message.
             */

            if (msgEvent.lastFlag)
                dhcpcMessageList [msgEvent.slot].writeFlag = TRUE;

            return (WAIT_OFFER);
            }

        /* Fill in host requirements defaults. */

        dhcpcDefaultsSet (pParams);

        /* Check offered parameters. Copy to parameter list if acceptable. */

        if (dhcp_msgtoparam (dhcpcMsgIn.dhcp, DHCPLEN (dhcpcMsgIn.udp),
                                 pParams) == OK) 
            {
            /*
             * Accept static BOOTP address or verify that the
             * address offered by the DHCP server is available
             * and check offered lease length against minimum value.
             */

            if ( (pParams->msgtype == DHCP_BOOTP ||
                 pParams->server_id.s_addr != 0) &&
                (arpans = arp_check (&pParams->yiaddr, &dhcpcIntface)) == OK && 
                pParams->lease_duration >= dhcpcMinLease) 
                {
                /*
                 * Initial offer accepted. Set client session
                 * to execute next routine and start timer.
                 */

                pParams->lease_origin = dhcpcBootLeaseData.initEpoch;
                dhcpcBootParam = pParams;

                /*
                 * Reset timer from retransmission interval
                 * to limit for collecting replies.
                 */

                wdCancel (dhcpcBootLeaseData.timer);

                wdStart (dhcpcBootLeaseData.timer, sysClkRateGet() *
                                    dhcpcBootLeaseData.leaseReqSpec.waitsecs,
                         (FUNCPTR)alarm_selecting, 0);

                /*
                 * Mark the message buffer available after handling the
                 * last DHCP message.
                 */

                if (msgEvent.lastFlag)
                    dhcpcMessageList [msgEvent.slot].writeFlag = TRUE;

                return (SELECTING);	/* Next state is SELECTING */
                }
	    else 
                {
                /*
                 * Offer is insufficient. Remove stored parameters
                 * and remain in current state.
                 */

	        clean_param (pParams);
	        free (pParams);
                }
            }
        else
            {
            /* Conversion unsuccessful - remove any stored parameters. */

            clean_param (pParams);
            free (pParams);
            }
        }

    /*
     * No valid offer received yet. Next state is (still) WAIT_OFFER. Mark
     * the message buffer available after handling the last DHCP message.
     */

    if (msgEvent.type == DHCP_MSG_ARRIVED && msgEvent.lastFlag)
        dhcpcMessageList [msgEvent.slot].writeFlag = TRUE;

    return (WAIT_OFFER);
    }

/*******************************************************************************
*
* dhcpcBootSelecting - Second offering state of client finite state machine
*
* This routine continues the second state of the finite state machine. 
* It compares additional offers received from DHCP servers to the current
* offer, and selects the offer which provides the lognest lease. When
* the time limit specified by the DHCPC_OFFER_TIMEOUT definition passes, 
* processing of the selected DHCP offer will continue with the requesting() 
* routine. If no DHCP offers were received, the BOOTP reply selected by the 
* wait_offer() routine will be used by the bootstrap loader.
*
* RETURNS: Next state of state machine, or -1 if error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int dhcpcBootSelecting
    (
    void
    )
    {
    int 			arpans = 0;
    struct dhcp_param * 	pParams = NULL; 
    char * 			pMsgData;
    char * 			pOption;
    EVENT_DATA 			msgEvent;
    int 			status;

    struct sockaddr_in 		dest;

#ifdef DHCPC_DEBUG
    logMsg ("dhcp: Entered SELECTING state.\n", 0, 0, 0, 0, 0, 0);
#endif

    /* Wait for message or timer expiration. */

    semTake (dhcpcEventSem, WAIT_FOREVER); 

    status = rngBufGet (dhcpcEventRing, (char *) &msgEvent, 
                        sizeof (msgEvent));
    if (status != sizeof (msgEvent))
        {
        /* Expected event missing or incomplete - remain in current state. */

        return (SELECTING);
        }

    if (msgEvent.type == DHCP_TIMEOUT)  
        {
        /*
         * Collection time expired -
         *     parameters structure holds chosen offer.
         */

        /* No response is needed if a BOOTP reply is chosen. */

        if (dhcpcBootParam->msgtype == DHCP_BOOTP)
            {
            dhcpcBindType = DHCP_BOOTP;
            return (BOUND);
            }

        dhcpcMsgLength = make_request (dhcpcBootParam, REQUESTING);
        if (dhcpcMsgLength < 0)
            {
#ifdef DHCPC_DEBUG
            logMsg ("Error making DHCP request. Entering INIT state.\n",
                    0, 0, 0, 0, 0, 0);
#endif
            return (INIT);
            }

        dhcpcMsgOut.udp->uh_sum = 0;
        dhcpcMsgOut.udp->uh_sum = udp_cksum (&spudph, (char *) dhcpcMsgOut.udp,
                                             ntohs (spudph.ulen));

        /*
         * Retransmit the message using a link-level broadcast address,
         * which prevents ARP messages if using Ethernet devices and allows
         * transmission from interfaces without assigned IP addresses.
         */

        bzero ( (char *)&dest, sizeof (struct sockaddr_in));

        dest.sin_len = sizeof (struct sockaddr_in);
        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = dhcpcMsgOut.ip->ip_dst.s_addr;

        if (dhcpSend (dhcpcIntface.iface, &dest, sbuf.buf,
                      dhcpcMsgLength, TRUE) == ERROR)
            {
#ifdef DHCPC_DEBUG
            logMsg ("Can't send DHCPREQUEST\n", 0, 0, 0, 0, 0, 0);
#endif
            return (INIT);
            }

        /*
         * DHCP request sent. Set client session to execute next state and
         * start the retransmission timer.
         */

        dhcpcBootLeaseData.timeout = FIRSTTIMER;
        dhcpcBootLeaseData.numRetry = 0;

        wdStart (dhcpcBootLeaseData.timer, sysClkRateGet() *
                                    SLEEP_RANDOM (dhcpcBootLeaseData.timeout),
                 (FUNCPTR)retrans_requesting, 0);

        return (REQUESTING);
        }
    else
        {
        /*
         * Process DHCP message stored in receive buffer by monitor task. 
         * The 4-byte alignment of the IP header needed by Sun BSP's is
         * guaranteed by the Berkeley Packet Filter during input.
         */

        pMsgData = msgEvent.pMsg;

        msgAlign (&dhcpcMsgIn, pMsgData);

        /* Examine type of message. Only accept DHCP offers. */

        pOption = (char *)pickup_opt (dhcpcMsgIn.dhcp,
                                      DHCPLEN (dhcpcMsgIn.udp),
                                     _DHCP_MSGTYPE_TAG);
        if (pOption == NULL)
            {
            /*
             * Message type not found - discard untyped DHCP messages, and
             * any BOOTP replies. Mark the message buffer available after
             * handling the last DHCP message.
             */

            if (msgEvent.lastFlag)
                dhcpcMessageList [msgEvent.slot].writeFlag = TRUE;

            return (SELECTING);
            }
        else
            {
            if (*OPTBODY (pOption) != DHCPOFFER)
                {
                /*
                 * Ignore messages with unexpected types. Mark the message
                 * buffer available after handling the last DHCP message.
                 */

                if (msgEvent.lastFlag)
                    dhcpcMessageList [msgEvent.slot].writeFlag = TRUE;

                return (SELECTING);
                }
            }

        /* Allocate memory for configuration parameters. */

        pParams = (struct dhcp_param *)calloc (1, sizeof (struct dhcp_param));
        if (pParams == NULL) 
            {
            /*
             * Memory for configuration parameters is unavailable. Remain
             * in current state. Mark the message buffer available after
             * handling the last DHCP message.
             */

            if (msgEvent.lastFlag)
                dhcpcMessageList [msgEvent.slot].writeFlag = TRUE;

            return (SELECTING);
            }

        /* Fill in host requirements defaults. */

        dhcpcDefaultsSet (pParams);
        
        /* Switch to offered parameters if they provide a longer DHCP lease. */

        if (dhcp_msgtoparam (dhcpcMsgIn.dhcp, DHCPLEN (dhcpcMsgIn.udp),
                                 pParams) == OK) 
            {
            if (pParams->server_id.s_addr != 0 &&
                (arpans = arp_check (&pParams->yiaddr, &dhcpcIntface)) == OK && 
                pParams->lease_duration >= dhcpcMinLease)
                {
                /* Take any DHCP message over BOOTP, or take a longer lease. */

                if (dhcpcBootParam->msgtype == DHCP_BOOTP ||
                     pParams->lease_duration > dhcpcBootParam->lease_duration)
                    { 
                    pParams->lease_origin = dhcpcBootLeaseData.initEpoch;
                    clean_param (dhcpcBootParam);
                    free (dhcpcBootParam);
                    dhcpcBootParam = pParams;
                    }
                }
            else 
                {
                /*
                 * Offer is insufficient. Remove stored parameters
                 * and remain in current state.
                 */

                clean_param (pParams);
                free (pParams);
	        }
	    }
        else
            {
            /* Conversion unsuccessful - remove any stored parameters. */

            clean_param (pParams);
            free (pParams);
            }
        }

    /*
     * Processed new offer before timer expired. Next state is (still)
     * SELECTING. Mark the message buffer available after handling the
     * last DHCP message.
     */

    if (msgEvent.lastFlag)
        dhcpcMessageList [msgEvent.slot].writeFlag = TRUE;

    return (SELECTING);
    }

/*******************************************************************************
*
* dhcpcBootRequesting - Lease request state of client finite state machine
*
* This routine implements the third state of the client finite state machine.
* It sends a lease request to the DHCP server whose offer was selected,
* and waits up to the current timeout value to receive a reply from that
* server. If a timeout occurs, the DHCP request message is retransmitted.
* If the request is refused or the retransmission limit is reached, the lease
* negotiation process will restart or the inform process will exit. If the
* request is acknowledged, the corresponding parameters will be used by the
* bootstrap loader to configure the boot device and fetch the runtime image.
*
* RETURNS: Next state of state machine, or -1 if error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int dhcpcBootRequesting
    (
    void
    )
    {
    STATUS result = OK;
    char *pOption = NULL;
    char errmsg[255];
    int timer = 0;
    int retry = 0;
    struct dhcp_param tmpparam;
    struct dhcp_reqspec tmp_reqspec;
    EVENT_DATA 		newEvent;
    char * pMsgData;
    int status;
    int msgtype;

#ifdef DHCPC_DEBUG
    char newAddr [INET_ADDR_LEN];

    logMsg ("dhcpc: Entering requesting state.\n", 0, 0, 0, 0, 0, 0);
#endif 

    bzero (errmsg, sizeof (errmsg));
    bzero ( (char *)&tmp_reqspec, sizeof (tmp_reqspec));

    /* Wait for message or retransmission. */

    semTake (dhcpcEventSem, WAIT_FOREVER);

    status = rngBufGet (dhcpcEventRing, (char *)&newEvent, sizeof (newEvent));
    if (status != sizeof (newEvent))
        {
        /* Expected event missing or incomplete - remain in current state. */

        return (REQUESTING);
        }

    if (newEvent.type == DHCP_TIMEOUT)
        {
        /* Handle timeout - no DHCP reply received yet. */

        retry = dhcpcBootLeaseData.numRetry;
        timer = dhcpcBootLeaseData.timeout;

        retry++;
        if (retry > REQUEST_RETRANS) 
            {
#ifdef DHCPC_DEBUG
            logMsg ("Client can't get ACK/NAK reply from server\n", 
                     0, 0, 0, 0, 0, 0);
#endif

          /* Exit if unable to get additional parameters for known address. */

            if (dhcpcPrevState == INFORMING)
                return (BOUND);

            return (INIT);           /* Next state is INIT */
            }

        /* Retransmission required - return after sending message. */

        switch (gen_retransmit ())
            {
            case 0:         /* Transmission successful. */
#ifdef DHCPC_DEBUG
                logMsg ("retransmit DHCPREQUEST\n", 0, 0, 0, 0, 0, 0);
#endif
                break;
            case -1:        /* Couldn't update timing data */
#ifdef DHCPC_DEBUG
                logMsg ("time() error retransmitting DHCPREQUEST\n",
                         0, 0, 0, 0, 0, 0);
#endif
                break;
            case -2:        /* Transmission error in output routine */
#ifdef DHCPC_DEBUG
                logMsg ("Can't retransmit DHCPREQUEST\n",
                        0, 0, 0, 0, 0, 0);
#endif
                break;
            }

        if (timer < MAXTIMER) 
            {
            /* Double retransmission delay with each attempt. (RFC 1541). */

            timer *= 2;
            }

        /* Set retransmission timer to randomized exponential backoff. */

        wdStart (dhcpcBootLeaseData.timer, sysClkRateGet() *
                                      SLEEP_RANDOM (timer),
                 (FUNCPTR)retrans_requesting, 0);

        dhcpcBootLeaseData.timeout = timer;
        dhcpcBootLeaseData.numRetry = retry;
        }
    else
        {
        bzero ( (char *)&tmpparam, sizeof (tmpparam));

        /*
         * Process DHCP message stored in receive buffer by monitor task. 
         * The 4-byte alignment of the IP header needed by Sun BSP's is
         * guaranteed by the Berkeley Packet Filter during input.
         */

        pMsgData = newEvent.pMsg;

        msgAlign (&dhcpcMsgIn, pMsgData);

        pOption = (char *) pickup_opt (dhcpcMsgIn.dhcp,
                                       DHCPLEN (dhcpcMsgIn.udp),
                                       _DHCP_MSGTYPE_TAG);
        if (pOption != NULL) 
            {
            msgtype =*OPTBODY (pOption);

            /*
             * Ignore unknown message types. If the client does not receive
             * a valid response within the expected interval, it will
             * timeout and retransmit the request - RFC 2131, section 3.1.5.
             */

            if (msgtype != DHCPACK && msgtype != DHCPNAK)
                {
                /*
                 * Mark the message buffer available after
                 * handling the last DHCP message.
                 */

                if (newEvent.lastFlag)
                    dhcpcMessageList [newEvent.slot].writeFlag = TRUE;

                return (REQUESTING);
                }

	    if (msgtype == DHCPNAK) 
                {
#ifdef DHCPC_DEBUG
	        logMsg ("Got DHCPNAK from server\n", 0, 0, 0, 0, 0, 0);

                pOption = (char *)pickup_opt (dhcpcMsgIn.dhcp, 
                                              DHCPLEN(dhcpcMsgIn.udp), 
                                              _DHCP_ERRMSG_TAG);
                if (pOption != NULL &&
                    nvttostr (OPTBODY (pOption), errmsg, 
                              (int)DHCPOPTLEN (pOption)) == 0)
	            logMsg ("DHCPNAK contains the error message \"%s\"\n", 
                             (int)errmsg, 0, 0, 0, 0, 0);
#endif

                /*
                 * Mark the message buffer available after handling the
                 * last DHCP message.
                 */

                if (newEvent.lastFlag)
                    dhcpcMessageList [newEvent.slot].writeFlag = TRUE;

                /* Ignore invalid responses to DHCP inform message. */

                if (dhcpcPrevState == INFORMING)
                    return (REQUESTING);

                clean_param (dhcpcBootParam);
                free (dhcpcBootParam);
                dhcpcBootParam = NULL;
                return (INIT);               /* Next state is INIT */
	        }

            /*
             * Got acknowledgement: fill in host requirements defaults and
             * add any parameters from message options.
             */

            dhcpcDefaultsSet (&tmpparam);   

            result = dhcp_msgtoparam (dhcpcMsgIn.dhcp,
                                      DHCPLEN (dhcpcMsgIn.udp), &tmpparam);

            /*
             * Mark the message buffer available after
             * handling the last DHCP message.
             */

            if (newEvent.lastFlag)
                dhcpcMessageList [newEvent.slot].writeFlag = TRUE;

            if (result == OK) 
                {
                /* Options parsed successfully - test as needed. */

                if (dhcpcPrevState == INFORMING)
                    dhcpcBindType = DHCP_MANUAL;     /* Not a lease. */
                else
                    {
                    /* Full lease negotiation - send ARP probe. */

                    result = arp_check (&tmpparam.yiaddr, &dhcpcIntface);
                    if (result == OK)
                        { 
#ifdef DHCPC_DEBUG
                        inet_ntoa_b (dhcpcBootParam->yiaddr, newAddr);
                        logMsg ("Got DHCPACK (IP = %s, duration = %d secs)\n",
                                (int)newAddr, dhcpcBootParam->lease_duration, 
                                0, 0, 0, 0);
#endif
                        dhcpcBindType = DHCP_NATIVE;
                        }
                    }

                /* Save additional parameters if address is available. */

                if (result == OK)
                    {
                    merge_param (dhcpcBootParam, &tmpparam);
                    *dhcpcBootParam = tmpparam;

                    if (dhcpcBindType == DHCP_NATIVE)
                        {
                        /* Save the lease start time. */

                        dhcpcBootParam->lease_origin =
                                                dhcpcBootLeaseData.initEpoch;
                        }
                    return (BOUND);    /* Address is available. */
                    }
                }

            /*
             * Invalid parameters or (for a full lease negotiation) a
             * failed ARP probe (address in use). For the full lease
             * negotiation, send the DHCPDECLINE message which is now
             * required when an ARP probe fails: RFC 2131, section 3.1.5.
             */

            if (dhcpcPrevState != INFORMING)
                {
                set_declinfo (&tmp_reqspec, errmsg);
                dhcp_decline (&tmp_reqspec);
                clean_param (dhcpcBootParam);
                free (dhcpcBootParam);
                dhcpcBootParam = NULL;

#ifdef DHCPC_DEBUG
                logMsg ("Received unacceptable ACK. Entering INIT state.\n",
                        0, 0, 0, 0, 0, 0);
#endif
                return (INIT);
                }

            /* Ignore invalid parameters for DHCP inform messages. */

            return (REQUESTING);
            }

        /*
         * Message type unavailable - remain in current state. Mark
         * the message buffer available after handling the last DHCP
         * message.
         */

        if (newEvent.lastFlag)
            dhcpcMessageList [newEvent.slot].writeFlag = TRUE;
        }

    /* Timeout occurred - remain in current state. */

    return (REQUESTING);
    }

/*******************************************************************************
*
* generate_xid - generate a transaction identifier 
*
* This routine forms a transaction identifier for outgoing messages from the
* DHCP client. It is called from multiple locations after the initialization 
* routines have retrieved the network interface hardware address.
*
* RETURNS: 32-bit transaction identifier in host byte order. 
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL long generate_xid
    (
    void
    )
    {
    time_t current = 0;
    u_short result1 = 0;
    u_short result2 = 0;
    u_short tmp[6]; 

    bcopy (dhcpcIntface.haddr.haddr, (char *)tmp, 6);

    dhcpTime (&current);
    result1 = checksum (tmp, 6);
    result2 = checksum ( (u_short *)&current, 2);

    return ( (result1 << 16) + result2);
    }

/*******************************************************************************
*
* msgAlign - set the buffer pointers to access message components
*
* This routine sets the pointers in the given message descriptor
* structure to access the various components of a received DHCP
* message. It is used internally by the state machine routines.
*
* RETURNS: N/A
*
* ERRNO:   N/A
*
* NOMANUAL
*/

LOCAL void msgAlign
    (
    struct msg *rmsg, 	/* Components of received message */
    char *rbuf 		/* Received Ethernet packet */
    )
    {
    rmsg->ip = (struct ip *) rbuf;

    if ( (ntohs (rmsg->ip->ip_off) & 0x1fff) == 0 &&
        ntohs (rmsg->ip->ip_len) >= (DFLTDHCPLEN - DFLTOPTLEN + 4) + 
				    UDPHL + IPHL)
        {
#if BSD<44
        rmsg->udp = (struct udphdr *)&rbuf [(rmsg->ip->ip_v_hl & 0xf) * WORD];
        rmsg->dhcp = (struct dhcp *)&rbuf [(rmsg->ip->ip_v_hl & 0xf) * WORD +
                                           UDPHL];
#else
        rmsg->udp = (struct udphdr *) &rbuf [rmsg->ip->ip_hl * WORD];
        rmsg->dhcp = (struct dhcp *) &rbuf [rmsg->ip->ip_hl * WORD +
                                            UDPHL];
#endif
        }
    else
        {
        rmsg->udp = NULL;
        rmsg->dhcp = NULL;
        }

    return;
    }

/*******************************************************************************
*
* arp_check - use ARP to check if given address is in use
*
* This routine broadcasts an ARP request and waits for a reply to determine if
* an IP address offered by a DHCP server is already in use.
*
* RETURNS: ERROR if ARP indicates client in use, or OK otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int arp_check
    (
    struct in_addr *target,
    struct if_info *pIfData     /* interface used by lease */
    )
    {
    int i = 0;
    int tmp;
    char inbuf [MAX_ARPLEN];
    char *pField1;
    char *pField2;
    struct arphdr * pArpHdr = NULL;


    struct ifreq ifr;
    struct timeval timeout;
    fd_set 	readFds;

    bzero (inbuf, MAX_ARPLEN);
    pArpHdr = (struct arphdr *) inbuf;

    pArpHdr->ar_hrd = htons (pIfData->haddr.htype);
    pArpHdr->ar_pro = htons (ETHERTYPE_IP);
    pArpHdr->ar_hln = pIfData->haddr.hlen;
    pArpHdr->ar_pln = 4;
    pArpHdr->ar_op = htons (ARPOP_REQUEST);

    /* Set sender H/W address to your address for ARP requests. (RFC 1541). */

    pField1 = &inbuf [ARPHL];                   /* Source hardware address. */
    pField2 = pField1 + pArpHdr->ar_hln + 4;    /* Target hardware address. */
    for (i = 0; i < pIfData->haddr.hlen; i++)
        {
        pField1[i] = pIfData->haddr.haddr[i];
        pField2[i] = 0;
        }

    /* Set sender IP address to 0 for ARP requests as per RFC 1541. */

    pField1 += pArpHdr->ar_hln;     /* Source IP address. */
    pField2 += pArpHdr->ar_hln;     /* Target IP address. */

    tmp = 0;
    bcopy ( (char *)&tmp, pField1, pArpHdr->ar_pln);
    bcopy ( (char *)&target->s_addr, pField2, pArpHdr->ar_pln);

    /* Update BPF filter to check for ARP reply from target IP address. */

    arpfilter [9].k = htonl (target->s_addr);
    if (ioctl (bpfArpDev, BIOCSETF, (int)&arpread) != 0)
        return (OK);     /* Ignore errors (permits use of IP address). */

    tmp = ARPHL + 2 * (pArpHdr->ar_hln + 4);    /* Size of ARP message. */

    dhcpcArpSend (pIfData->iface, inbuf, tmp);

    /* Set BPF to monitor interface for reply. */

    bzero ( (char *)&ifr, sizeof (struct ifreq));
    sprintf (ifr.ifr_name, "%s%d", pIfData->iface->if_name,
                                   pIfData->iface->if_unit);
    if (ioctl (bpfArpDev, BIOCSETIF, (int)&ifr) != 0)
        return (OK);     /* Ignore errors (permits use of IP address). */

    /* Wait one-half second for (unexpected) reply from target IP address. */

    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    FD_ZERO (&readFds);
    FD_SET (bpfArpDev, &readFds);

    tmp = select (bpfArpDev + 1, &readFds, NULL, NULL, &timeout);

    /* Disconnect BPF device from input stream until next ARP probe. */

    ioctl (bpfArpDev, BIOCSTOP, 0); 

    if (tmp)    /* ARP reply received? Duplicate IP address. */
        return (ERROR);
    else        /* Timeout occurred - no ARP reply. Probe succeeded. */
        return (OK);
    }

#ifdef DHCPC_DEBUG
/*******************************************************************************
*
* nvttostr - convert NVT ASCII to strings
*
* This routine implements a limited NVT conversion which removes any embedded
* NULL characters from the input string.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int nvttostr
    (
    char *	nvt, 
    char *	str,
    int 	length
    )
    {
    FAST int i = 0;
    FAST char *tmp = NULL;

    tmp = str;

    for (i = 0; i < length; i++) 
        {
        if (nvt[i] != '\0') 
            {
            *tmp = nvt[i];
            tmp++;
            }
        }

    str [length] = '\0';
    return (0);
    }
#endif

/*******************************************************************************
*
* dhcp_msgtoparam - expand DHCP message into data structure
*
* This routine converts a DHCP message from the transmission format into the
* dhcp_param structure. It copies the core fields directly and multiplexes
* the options to the appropriate handle_* functions.
*
* RETURNS: 0 if conversion successful, or -1 otherwise
*
* ERRNO: N/A
*
* INTERNAL
*
* When calculating the values for the lease timers, floating-point calculations
* can't be used because some boards (notably the SPARC architectures) disable 
* software floating point by default to speed up context switching. These 
* boards abort with an exception when floating point operations are 
* encountered. The error introduced by the integer approximations is not
* significant.
*
* NOMANUAL
*/

LOCAL int dhcp_msgtoparam
    (
    struct dhcp *msg,
    int msglen,
    struct dhcp_param *parameter
    )
    {
    FAST char *optp = NULL;
    char tag = 0;
    BOOL sname_is_opt = FALSE;
    BOOL file_is_opt = FALSE;
    int err = 0;
    char *endofopt;

    endofopt = &msg->options [msglen - DFLTDHCPLEN + DFLTOPTLEN];

    bzero (parameter->got_option, OPTMASKSIZE);

    for (optp = &msg->options [MAGIC_LEN]; optp <= endofopt; optp++) 
        {
        tag = *optp;

        /* skip the PAD option */

        if (tag == _DHCP_PAD_TAG)
            continue;

        /* stop processing when the END option is encountered */

        if (tag == _DHCP_END_TAG) 
            break;

        /* handle the "Option Overload" */
        if (tag == _DHCP_OPT_OVERLOAD_TAG) 
            {
            optp += 2;
            switch (*optp) 
                {
                case FILE_ISOPT:
                    file_is_opt = TRUE;
	            break;
                case SNAME_ISOPT:
                    sname_is_opt = TRUE;
                    break;
                case BOTH_AREOPT:
                    file_is_opt = sname_is_opt = TRUE;
                    break;
                default:
                    break;
                }
            continue;
            }

        if ((tag > 0) && (tag < MAXTAGNUM) && (handle_param [(int)tag] != NULL))
            {
            if ( (err = (*handle_param [ (int)tag]) (optp, parameter)) != 0) 
                return (err);
            else 
                SETBIT (parameter->got_option, tag);
            }

        /* Set the message type tag to distinguish DHCP and BOOTP messages. */
        else if (tag == _DHCP_MSGTYPE_TAG)
            SETBIT (parameter->got_option, tag);

        optp++;
        optp += *optp;
        }

    if (file_is_opt) 
        {
        endofopt = &msg->file [MAX_FILE];
        for (optp = msg->file; optp <= endofopt; optp++) 
            {
            tag = *optp;

            /* skip the PAD option */
            if (tag == _DHCP_PAD_TAG) 
                continue;

            /* stop processing when the END option is reached */

            if (tag == _DHCP_END_TAG)
                break;

            if (handle_param [ (int)tag] != NULL) 
                {
                if ( (err = (*handle_param [ (int)tag]) (optp, parameter)) != 0)
                    return (err);
                else 
                    SETBIT(parameter->got_option, tag);
                }

            /* Set the message type to distinguish DHCP and BOOTP messages. */

            else if (tag == _DHCP_MSGTYPE_TAG)
                SETBIT (parameter->got_option, tag);

            optp++;
            optp += *optp;
            }
        } 
    else 
        {
        if ( (parameter->file = calloc (1, strlen (msg->file) + 1)) == NULL) 
            return (-1);
        strcpy (parameter->file, msg->file);
        }


    if (sname_is_opt) 
        {
        endofopt = &msg->sname [MAX_SNAME];
        for (optp = msg->sname; optp <= endofopt; optp++) 
            {
            tag = *optp;

            /* skip the PAD option */

            if (tag == _DHCP_PAD_TAG)
                continue;

            /* stop processing when the END option is reached */ 

            if (tag == _DHCP_END_TAG)
	        break;

            if (handle_param [ (int)tag] != NULL) 
                {
	        if ( (err = (*handle_param [ (int)tag]) (optp, parameter)) != 0)
	            return(err);
                else
	            SETBIT (parameter->got_option, tag);
                }

            /* Set the message type to distinguish DHCP and BOOTP messages. */

            else if (tag == _DHCP_MSGTYPE_TAG)
                SETBIT (parameter->got_option, tag);

            optp++;
            optp += *optp;
            }
        } 
    else 
        {
        if ( (parameter->sname = calloc (1, strlen (msg->sname) + 1)) == NULL) 
            return (-1);
        strcpy(parameter->sname, msg->sname);
        }

    parameter->ciaddr.s_addr = msg->ciaddr.s_addr;
    parameter->yiaddr.s_addr = msg->yiaddr.s_addr;
    parameter->siaddr.s_addr = msg->siaddr.s_addr;
    parameter->giaddr.s_addr = msg->giaddr.s_addr;

    if (ISSET (parameter->got_option, _DHCP_MSGTYPE_TAG))
        parameter->msgtype = DHCP_NATIVE;
    else
        parameter->msgtype = DHCP_BOOTP;

    /* Set lease duration to infinite for BOOTP replies. */
    if (parameter->msgtype == DHCP_BOOTP)
        {
        parameter->lease_duration = ~0;
        return (0);
        }

    /* Assign any server name provided if 'sname' used for options. */
  
    if (sname_is_opt)
        {
        parameter->sname = parameter->temp_sname;
        parameter->temp_sname = NULL;
        }

    /* Assign any bootfile provided if 'file' used for options. */

    if (file_is_opt)
        {
        if (ISSET (parameter->got_option, _DHCP_BOOTFILE_TAG))
            {
            parameter->file = parameter->temp_file;
            parameter->temp_file = NULL;
            }
        }
  
    if (parameter->dhcp_t1 == 0)
        {
        /* Timer t1 is half the lease duration - but don't divide. */

        parameter->dhcp_t1 = (parameter->lease_duration) >> 1;
        SETBIT (parameter->got_option, _DHCP_T1_TAG);
        }
    if (parameter->dhcp_t2 == 0)
        {
        /* Timer T2 is .875 of the lease - but don't use floating point. */

        int tmp = (parameter->lease_duration * 7) >> 3;
        parameter->dhcp_t2 = (unsigned long) tmp;
        SETBIT(parameter->got_option, _DHCP_T2_TAG);
        }

    return(0);
    }

/*******************************************************************************
*
* clean_param - clean the parameters data structure
*
* This routine frees all the memory allocated for the storage of parameters
* received from a DHCP server. It is called to remove the unselected offers
* or if an error occurs in the state machine after offers have been received.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int clean_param
    (
    struct dhcp_param *param
    )
    {
    if (param == NULL)
        return(0);

    if (param->sname != NULL) 
        free (param->sname);
    if (param->temp_sname != NULL) 
        free (param->temp_sname);
    if (param->file != NULL) 
        free (param->file);
    if (param->temp_file != NULL) 
        free (param->temp_file);
    if (param->hostname != NULL) 
        free (param->hostname);
    if (param->merit_dump != NULL) 
        free (param->merit_dump);
    if (param->dns_domain != NULL) 
        free (param->dns_domain);
    if (param->root_path != NULL) 
        free (param->root_path);
    if (param->extensions_path != NULL) 
        free (param->extensions_path);
    if (param->nis_domain != NULL) 
        free (param->nis_domain);
    if (param->nb_scope != NULL) 
        free (param->nb_scope);
    if (param->errmsg != NULL) 
        free (param->errmsg);
    if (param->nisp_domain != NULL) 
        free (param->nisp_domain);

    if (param->mtu_plateau_table != NULL) 
        {
        if (param->mtu_plateau_table->shortnum != NULL)
            free (param->mtu_plateau_table->shortnum);
        free (param->mtu_plateau_table);
        }
    if (param->subnet_mask != NULL) 
        free (param->subnet_mask);
    if (param->swap_server != NULL) 
        free (param->swap_server);
    if (param->brdcast_addr != NULL) 
        free (param->brdcast_addr);
    if (param->router != NULL) 
        {
        if (param->router->addr != NULL)
            free (param->router->addr);
        free (param->router);
        }
    if (param->time_server != NULL) 
        {
        if (param->time_server->addr != NULL)
            free (param->time_server->addr);
        free (param->time_server);
        }
    if (param->name_server != NULL) 
        {
        if (param->name_server->addr != NULL)
            free (param->name_server->addr);
        free (param->name_server);
        }
    if (param->dns_server != NULL) 
        {
        if (param->dns_server->addr != NULL)
            free (param->dns_server->addr);
        free (param->dns_server);
        }
    if (param->log_server != NULL) 
        {
        if (param->log_server->addr != NULL)
            free (param->log_server->addr);
        free (param->log_server);
        }
    if (param->cookie_server != NULL) 
        {
        if (param->cookie_server->addr != NULL)
            free (param->cookie_server->addr);
        free (param->cookie_server);
        }
    if (param->lpr_server != NULL)
        {
        if (param->lpr_server->addr != NULL)
            free (param->lpr_server->addr);
        free (param->lpr_server);
        }
    if (param->impress_server != NULL) 
        {
        if (param->impress_server->addr != NULL)
            free (param->impress_server->addr);
        free (param->impress_server);
        }
    if (param->rls_server != NULL) 
        {
        if (param->rls_server->addr != NULL)
            free (param->rls_server->addr);
        free (param->rls_server);
        }
    if (param->policy_filter != NULL) 
        {
        if (param->policy_filter->addr != NULL)
            free (param->policy_filter->addr);
        free (param->policy_filter);
        }
    if (param->static_route != NULL) 
        {
        if (param->static_route->addr != NULL)
            free (param->static_route->addr);
        free (param->static_route);
        }
    if (param->nis_server != NULL) 
        {
        if (param->nis_server->addr != NULL)
            free (param->nis_server->addr);
        free (param->nis_server);
        }
    if (param->ntp_server != NULL) 
        {
        if (param->ntp_server->addr != NULL)
            free (param->ntp_server->addr);
        free (param->ntp_server);
        }
    if (param->nbn_server != NULL) 
        {
        if (param->nbn_server->addr != NULL)
            free (param->nbn_server->addr);
        free (param->nbn_server);
        }
    if (param->nbdd_server != NULL) 
        {
        if (param->nbdd_server->addr != NULL)
            free (param->nbdd_server->addr);
        free (param->nbdd_server);
        }
    if (param->xfont_server != NULL) 
        {
        if (param->xfont_server->addr != NULL)
            free (param->xfont_server->addr);
        free (param->xfont_server);
        }
    if (param->xdisplay_manager != NULL) 
        {
        if (param->xdisplay_manager->addr != NULL)
            free (param->xdisplay_manager->addr);
        free (param->xdisplay_manager);
        }
    if (param->nisp_server != NULL) 
        {
        if (param->nisp_server->addr != NULL)
            free (param->nisp_server->addr);
        free (param->nisp_server);
        }
    if (param->mobileip_ha != NULL) 
        {
        if (param->mobileip_ha->addr != NULL)
            free (param->mobileip_ha->addr);
        free (param->mobileip_ha);
        }
    if (param->smtp_server != NULL) 
        {
        if (param->smtp_server->addr != NULL)
            free (param->smtp_server->addr);
        free (param->smtp_server);
        }
    if (param->pop3_server != NULL) 
        {
        if (param->pop3_server->addr != NULL)
            free (param->pop3_server->addr);
        free (param->pop3_server);
        }
    if (param->nntp_server != NULL) 
        {
        if (param->nntp_server->addr != NULL)
            free (param->nntp_server->addr);
        free (param->nntp_server);
        }
    if (param->dflt_www_server != NULL) 
        {
        if (param->dflt_www_server->addr != NULL)
            free (param->dflt_www_server->addr);
        free (param->dflt_www_server);
        }
    if (param->dflt_finger_server != NULL) 
        {
        if (param->dflt_finger_server->addr != NULL)
            free (param->dflt_finger_server->addr);
        free (param->dflt_finger_server);
        }
    if (param->dflt_irc_server != NULL) 
        {
        if (param->dflt_irc_server->addr != NULL)
            free (param->dflt_irc_server->addr);
        free (param->dflt_irc_server);
        }
    if (param->streettalk_server != NULL) 
        {
        if (param->streettalk_server->addr != NULL)
            free (param->streettalk_server->addr);
        free (param->streettalk_server);
        }
    if (param->stda_server != NULL) 
        {
        if (param->stda_server->addr != NULL)
            free (param->stda_server->addr);
        free (param->stda_server);
        }
    if (param->vendlist != NULL)
        free (param->vendlist);

    bzero ( (char *)param, sizeof (struct dhcp_param));
    return (0);
    }


/*******************************************************************************
*
* merge_param - combine the parameters from DHCP server responses
*
* This routine copies any parameters from an initial lease offer which were not
* supplied in the later acknowledgement from the server. If the acknowledgement
* from the server duplicates parameters from the offer, then the memory       
* allocated for those parameters in the offer is released.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

/*
 * if there is no information in newp but oldp, copy it to newp
 * else free the appropriate memory of oldp
 */
LOCAL int merge_param
    (
    struct dhcp_param *oldp,     /* Parameters from lease offer. */
    struct dhcp_param *newp      /* Parameters from lease acknowledgement. */
    )
    {
    if (oldp == NULL || newp == NULL)
        return (0);

    if (newp->sname == NULL && oldp->sname != NULL)
        newp->sname = oldp->sname;
    else if (oldp->sname != NULL)
        free (oldp->sname);

    if (newp->file == NULL && oldp->file != NULL)
        newp->file = oldp->file;
    else if (oldp->file != NULL) 
        free (oldp->file);
 
    if (newp->hostname == NULL && oldp->hostname != NULL)
        newp->hostname = oldp->hostname;
    else if (oldp->hostname != NULL)
        free (oldp->hostname);

    if (newp->merit_dump == NULL && oldp->merit_dump != NULL)
        newp->merit_dump = oldp->merit_dump;
    else if (oldp->merit_dump != NULL)
        free (oldp->merit_dump);

    if (newp->dns_domain == NULL && oldp->dns_domain != NULL)
        newp->dns_domain = oldp->dns_domain;
    else if (oldp->dns_domain != NULL) 
        free (oldp->dns_domain);

    if (newp->root_path == NULL && oldp->root_path != NULL)
        newp->root_path = oldp->root_path;
    else if (oldp->root_path != NULL) 
        free (oldp->root_path);

    if (newp->extensions_path == NULL && oldp->extensions_path != NULL)
        newp->extensions_path = oldp->extensions_path;
    else if (oldp->extensions_path != NULL) 
        free (oldp->extensions_path);

    if (newp->nis_domain == NULL && oldp->nis_domain != NULL)
        newp->nis_domain = oldp->nis_domain;
    else if (oldp->nis_domain != NULL) 
        free (oldp->nis_domain);

    if (newp->nb_scope == NULL && oldp->nb_scope != NULL)
        newp->nb_scope = oldp->nb_scope;
    else if (oldp->nb_scope != NULL) 
        free (oldp->nb_scope);

    if (newp->errmsg == NULL && oldp->errmsg != NULL)
        newp->errmsg = oldp->errmsg;
    else if (oldp->errmsg != NULL) 
        free (oldp->errmsg);

    if (newp->nisp_domain == NULL && oldp->nisp_domain != NULL)
        newp->nisp_domain = oldp->nisp_domain;
    else if (oldp->nisp_domain != NULL) 
        free (oldp->nisp_domain);

    if (newp->mtu_plateau_table == NULL && oldp->mtu_plateau_table != NULL) 
        newp->mtu_plateau_table = oldp->mtu_plateau_table;
    else 
        {
        if (oldp->mtu_plateau_table != NULL)
            {
            if (oldp->mtu_plateau_table->shortnum != NULL)
                free (oldp->mtu_plateau_table->shortnum);
            free (oldp->mtu_plateau_table);
            }
        }

    if (newp->subnet_mask == NULL && oldp->subnet_mask != NULL)
        newp->subnet_mask = oldp->subnet_mask;
    else if (oldp->subnet_mask != NULL) 
        free (oldp->subnet_mask);

    if (newp->swap_server == NULL && oldp->swap_server != NULL)
        newp->swap_server = oldp->swap_server;
    else if (oldp->swap_server != NULL) 
        free (oldp->swap_server);

    if (newp->brdcast_addr == NULL && oldp->brdcast_addr != NULL)
        newp->brdcast_addr = oldp->brdcast_addr;
    else if (oldp->brdcast_addr != NULL) 
        free (oldp->brdcast_addr);

    if (newp->router_solicit.s_addr == 0 && oldp->router_solicit.s_addr != 0)
        bcopy ( (char *)&oldp->router_solicit, 
               (char *)&newp->router_solicit, sizeof (u_long));

    if (newp->router == NULL && oldp->router != NULL) 
      newp->router = oldp->router;
    else 
        {
        if (oldp->router != NULL && oldp->router->addr != NULL) 
            free (oldp->router->addr);
        if (oldp->router != NULL)
            free (oldp->router);
        }

    if (newp->time_server == NULL && oldp->time_server != NULL)
        newp->time_server = oldp->time_server;
    else 
        { 
        if (oldp->time_server != NULL && oldp->time_server->addr != NULL) 
            free (oldp->time_server->addr);
        if (oldp->time_server != NULL)
            free (oldp->time_server);
        }

    if (newp->name_server == NULL && oldp->name_server != NULL) 
        newp->name_server = oldp->name_server;
    else 
        {
        if (oldp->name_server != NULL && oldp->name_server->addr != NULL) 
            free (oldp->name_server->addr);
        if (oldp->name_server != NULL)
            free (oldp->name_server);
        }

    if (newp->dns_server == NULL && oldp->dns_server != NULL) 
        newp->dns_server = oldp->dns_server;
    else 
        {
        if (oldp->dns_server != NULL && oldp->dns_server->addr != NULL) 
            free (oldp->dns_server->addr);
        if (oldp->dns_server != NULL)
            free (oldp->dns_server);
        }

    if (newp->log_server == NULL && oldp->log_server != NULL) 
        newp->log_server = oldp->log_server;
    else 
        {
        if (oldp->log_server != NULL && oldp->log_server->addr != NULL) 
            free (oldp->log_server->addr);
        if (oldp->log_server != NULL)
            free (oldp->log_server);
        }

    if (newp->cookie_server == NULL && oldp->cookie_server != NULL) 
        newp->cookie_server = oldp->cookie_server;
    else 
        {
        if (oldp->cookie_server != NULL && oldp->cookie_server->addr != NULL) 
            free (oldp->cookie_server->addr);
        if (oldp->cookie_server != NULL)
            free (oldp->cookie_server);
        }

    if (newp->lpr_server == NULL && oldp->lpr_server != NULL) 
        newp->lpr_server = oldp->lpr_server;
    else 
        {
        if (oldp->lpr_server != NULL && oldp->lpr_server->addr != NULL) 
            free (oldp->lpr_server->addr);
        if (oldp->lpr_server != NULL)
            free (oldp->lpr_server);
        }

    if (newp->impress_server == NULL && oldp->impress_server != NULL) 
        newp->impress_server = oldp->impress_server;
    else 
        {
        if (oldp->impress_server != NULL && oldp->impress_server->addr != NULL)
            free (oldp->impress_server->addr);
        if (oldp->impress_server != NULL)
            free (oldp->impress_server);
        }

    if (newp->rls_server == NULL && oldp->rls_server != NULL) 
        newp->rls_server = oldp->rls_server;
    else 
        {
        if (oldp->rls_server != NULL && oldp->rls_server->addr != NULL) 
            free (oldp->rls_server->addr);
        if (oldp->rls_server != NULL)
            free (oldp->rls_server);
        }

    if (newp->policy_filter == NULL && oldp->policy_filter != NULL) 
        newp->policy_filter = oldp->policy_filter;
    else 
        {
        if (oldp->policy_filter != NULL && oldp->policy_filter->addr != NULL) 
            free (oldp->policy_filter->addr);
        if (oldp->policy_filter != NULL)
            free (oldp->policy_filter);
        }

    if (newp->static_route == NULL && oldp->static_route != NULL) 
        newp->static_route = oldp->static_route;
    else 
        {
        if (oldp->static_route != NULL && oldp->static_route->addr != NULL) 
            free (oldp->static_route->addr);
        if (oldp->static_route != NULL)
            free (oldp->static_route);
        }

    if (newp->nis_server == NULL && oldp->nis_server != NULL) 
        newp->nis_server = oldp->nis_server;
    else 
        {
        if (oldp->nis_server != NULL && oldp->nis_server->addr != NULL) 
            free (oldp->nis_server->addr);
        if (oldp->nis_server != NULL)
            free (oldp->nis_server);
        }

    if (newp->ntp_server == NULL && oldp->ntp_server != NULL) 
        newp->ntp_server = oldp->ntp_server;
    else 
        {
        if (oldp->ntp_server != NULL && oldp->ntp_server->addr != NULL) 
            free (oldp->ntp_server->addr);
        if (oldp->ntp_server != NULL)
            free (oldp->ntp_server);
        }

    if (newp->nbn_server == NULL && oldp->nbn_server != NULL) 
        newp->nbn_server = oldp->nbn_server;
    else 
        {
        if (oldp->nbn_server != NULL && oldp->nbn_server->addr != NULL) 
            free (oldp->nbn_server->addr);
        if (oldp->nbn_server != NULL)
            free (oldp->nbn_server);
        }

    if (newp->nbdd_server == NULL && oldp->nbdd_server != NULL) 
        newp->nbdd_server = oldp->nbdd_server;
    else 
        {
        if (oldp->nbdd_server != NULL && oldp->nbdd_server->addr != NULL) 
            free (oldp->nbdd_server->addr);
        if (oldp->nbdd_server != NULL)
            free (oldp->nbdd_server);
        }

    if (newp->xfont_server == NULL && oldp->xfont_server != NULL) 
        newp->xfont_server = oldp->xfont_server;
    else 
        {
        if (oldp->xfont_server != NULL && oldp->xfont_server->addr != NULL) 
            free (oldp->xfont_server->addr);
        if (oldp->xfont_server != NULL)
            free (oldp->xfont_server);
        }

    if (newp->xdisplay_manager == NULL && oldp->xdisplay_manager != NULL) 
        newp->xdisplay_manager = oldp->xdisplay_manager;
    else 
        {
        if (oldp->xdisplay_manager != NULL && 
            oldp->xdisplay_manager->addr != NULL) 
            free (oldp->xdisplay_manager->addr);
        if (oldp->xdisplay_manager != NULL)
            free (oldp->xdisplay_manager);
        }

    if (newp->nisp_server == NULL && oldp->nisp_server != NULL) 
        newp->nisp_server = oldp->nisp_server;
    else 
        {
        if (oldp->nisp_server != NULL && oldp->nisp_server->addr != NULL) 
            free (oldp->nisp_server->addr);
        if (oldp->nisp_server != NULL)
            free (oldp->nisp_server);
        }

    if (newp->mobileip_ha == NULL && oldp->mobileip_ha != NULL) 
        newp->mobileip_ha = oldp->mobileip_ha;
    else 
        { 
        if (oldp->mobileip_ha != NULL && oldp->mobileip_ha->addr != NULL) 
            free (oldp->mobileip_ha->addr);
        if (oldp->mobileip_ha != NULL)
            free (oldp->mobileip_ha);
        }

    if (newp->smtp_server == NULL && oldp->smtp_server != NULL) 
        newp->smtp_server = oldp->smtp_server;
    else 
        {
        if (oldp->smtp_server != NULL && oldp->smtp_server->addr != NULL) 
            free (oldp->smtp_server->addr);
        if (oldp->smtp_server != NULL)
            free (oldp->smtp_server);
        }

    if (newp->pop3_server == NULL && oldp->pop3_server != NULL) 
        newp->pop3_server = oldp->pop3_server;
    else 
        {
        if (oldp->pop3_server != NULL && oldp->pop3_server->addr != NULL) 
            free (oldp->pop3_server->addr);
        if (oldp->pop3_server != NULL)
            free (oldp->pop3_server);
        }

    if (newp->nntp_server == NULL && oldp->nntp_server != NULL) 
        newp->nntp_server = oldp->nntp_server;
    else 
        {
        if (oldp->nntp_server != NULL && oldp->nntp_server->addr != NULL) 
            free (oldp->nntp_server->addr);
        if (oldp->nntp_server != NULL)
            free (oldp->nntp_server);
        }

    if (newp->dflt_www_server == NULL && oldp->dflt_www_server != NULL) 
        newp->dflt_www_server = oldp->dflt_www_server;
    else 
        {
        if (oldp->dflt_www_server != NULL && 
            oldp->dflt_www_server->addr != NULL) 
            free (oldp->dflt_www_server->addr);
        if (oldp->dflt_www_server != NULL)
            free (oldp->dflt_www_server);
        }

    if (newp->dflt_finger_server == NULL && oldp->dflt_finger_server != NULL)
        newp->dflt_finger_server = oldp->dflt_finger_server;
    else 
        {
        if (oldp->dflt_finger_server != NULL && 
            oldp->dflt_finger_server->addr != NULL) 
            free (oldp->dflt_finger_server->addr);
        if (oldp->dflt_finger_server != NULL)
            free (oldp->dflt_finger_server);
        }

    if (newp->dflt_irc_server == NULL && oldp->dflt_irc_server != NULL) 
        newp->dflt_irc_server = oldp->dflt_irc_server;
    else 
        {
        if (oldp->dflt_irc_server != NULL && 
            oldp->dflt_irc_server->addr != NULL) 
            free (oldp->dflt_irc_server->addr);
        if (oldp->dflt_irc_server != NULL)
            free (oldp->dflt_irc_server);
        }

    if (newp->streettalk_server == NULL && oldp->streettalk_server != NULL) 
        newp->streettalk_server = oldp->streettalk_server;
    else 
        {
        if (oldp->streettalk_server != NULL && 
            oldp->streettalk_server->addr != NULL) 
            free (oldp->streettalk_server->addr);
        if (oldp->streettalk_server != NULL)
            free (oldp->streettalk_server);
        }

    if (newp->stda_server == NULL && oldp->stda_server != NULL) 
        newp->stda_server = oldp->stda_server;
    else 
        {
        if (oldp->stda_server != NULL && oldp->stda_server->addr != NULL) 
            free (oldp->stda_server->addr);
        if (oldp->stda_server != NULL)
            free (oldp->stda_server);
        }

    /* Remove any vendor-specific information from an earlier response. */

    if (oldp->vendlist != NULL)
        free (oldp->vendlist);
 
    return (0);
    }


/*******************************************************************************
*
* initialize - perform general DHCP client initialization
*
* This routine sets the DHCP ports, resets the network interface, and allocates
* storage for incoming messages. It is called from dhcp_boot_client before the
* finite state machine is executed.
*
* RETURNS: 0 if initialization successful, or -1 otherwise. 
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int initialize
    (
    int 	serverPort, 	/* port used by DHCP servers */
    int 	clientPort 	/* port used by DHCP clients */
    )
    {
    char * 	sbufp;
    int 	arpSize; 	/* Maximum size for ARP reply. */
    int         result;

    /*
     * Create BPF device for performing ARP probes. The buffer size
     * for the device is equal to the maximum link header (determined
     * from the provided DHCP buffer size) plus the largest possible
     * ARP payload for IP addresses.
     */
 
    arpSize = dhcpcBootLeaseData.ifData.bufSize - DFLTDHCPLEN - UDPHL - IPHL;
    arpSize += MAX_ARPLEN + sizeof (struct bpf_hdr);

    if (bpfDevCreate ("/bpf/dhcpc-arp", 1, arpSize) == ERROR)
        {
        return (-1);
        }

    bpfArpDev = open ("/bpf/dhcpc-arp0", 0, 0);
    if (bpfArpDev < 0)
        {
        bpfDevDelete ("/bpf/dhcpc-arp0");
        return (-1);
        }

    /* Enable immediate mode for reading messages */

    result = 1;
    result = ioctl (bpfArpDev, BIOCIMMEDIATE, (int)&result);
    if (result != 0)
	{
	close (bpfArpDev);
	bpfDevDelete ("/bpf/dhcpc-arp0");
        return (-1);
	}

    /*
     * Set filter to accept ARP replies when the arp_check() routine
     * attaches the BPF device to a network interface.
     */

    if (ioctl (bpfArpDev, BIOCSETF, (int)&arpread) != 0)
        {
        close (bpfArpDev);
        bpfDevDelete ("/bpf/dhcpc-arp0");
        return (-1);
        }

    /* Reseed random number generator. */

    srand (generate_xid ());

    /* Always use default ports for client and server. */

    dhcps_port = htons (serverPort);
    dhcpc_port = htons (clientPort);

    /* Allocate transmit buffer equal to max. message size from user. */

    sbuf.size = dhcpcBootLeaseData.ifData.bufSize;

    if ( (sbuf.buf = (char *)memalign (4, sbuf.size)) == NULL) 
        {
#ifdef DHCPC_DEBUG
        logMsg ("allocation error for sbuf in initialize\n", 0, 0, 0, 0, 0, 0);
#endif
        close (bpfArpDev);
        bpfDevDelete ("/bpf/dhcpc-arp0");
        return(-1);
        } 
    bzero (sbuf.buf, sbuf.size);

    sbufp = sbuf.buf;
    dhcpcMsgOut.ip = (struct ip *)sbufp;
    dhcpcMsgOut.udp = (struct udphdr *)&sbufp [IPHL];
    dhcpcMsgOut.dhcp = (struct dhcp *)&sbufp [IPHL + UDPHL];

    return (0);
    }

/*******************************************************************************
*
* make_decline - construct a DHCP decline message
*
* This routine constructs an outgoing UDP/IP message containing the values
* required to decline an offered IP address.
*
* RETURNS: Size of decline message, in bytes, or 0 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int make_decline
    (
    struct dhcp_reqspec *pDhcpcReqSpec
    )
    {
    int offopt = 0; 	/* offset in options field */
    int msgsize; 	/* total size of DHCP message */
    u_long tmpul = 0;
    struct ps_udph pudph;

    bzero ( (char *)&pudph, sizeof (pudph));

    /* construct dhcp part */

    bzero (sbuf.buf, sbuf.size);
    dhcpcMsgOut.dhcp->op = BOOTREQUEST;
    dhcpcMsgOut.dhcp->htype = dhcpcIntface.haddr.htype;
    dhcpcMsgOut.dhcp->hlen = dhcpcIntface.haddr.hlen;
    dhcpcMsgOut.dhcp->xid = htonl (generate_xid ());
    dhcpcMsgOut.dhcp->giaddr = dhcpcMsgIn.dhcp->giaddr;
    bcopy (dhcpcIntface.haddr.haddr, dhcpcMsgOut.dhcp->chaddr,
           dhcpcMsgOut.dhcp->hlen);

    /* insert magic cookie */

    bcopy ( (char *)dhcpCookie, dhcpcMsgOut.dhcp->options, MAGIC_LEN);
    offopt = MAGIC_LEN;

    /* insert message type */

    dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_MSGTYPE_TAG;
    dhcpcMsgOut.dhcp->options [offopt++] = 1;
    dhcpcMsgOut.dhcp->options [offopt++] = DHCPDECLINE;

    /* insert requested IP */

    if (pDhcpcReqSpec->ipaddr.s_addr == 0) 
        return (0);

    dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_REQUEST_IPADDR_TAG;
    dhcpcMsgOut.dhcp->options [offopt++] = 4;
    bcopy ( (char *)&pDhcpcReqSpec->ipaddr,
           &dhcpcMsgOut.dhcp->options [offopt], 4);
    offopt += 4;

    /* insert client identifier */

    if (pDhcpcReqSpec->clid != NULL)
        {
        dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_CLIENT_ID_TAG;
        dhcpcMsgOut.dhcp->options [offopt++] = pDhcpcReqSpec->clid->len;
        bcopy (pDhcpcReqSpec->clid->id, &dhcpcMsgOut.dhcp->options [offopt], 
               pDhcpcReqSpec->clid->len);
        offopt += pDhcpcReqSpec->clid->len;
        }

    /* insert server identifier */

    dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_SERVER_ID_TAG;
    dhcpcMsgOut.dhcp->options [offopt++] = 4;
    bcopy ( (char *)&dhcpcBootParam->server_id,
           &dhcpcMsgOut.dhcp->options [offopt], 4);
   offopt += 4;

    /* Insert error message, if available. */

    if (pDhcpcReqSpec->dhcp_errmsg != NULL)
        {
        dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_ERRMSG_TAG;
        dhcpcMsgOut.dhcp->options [offopt++] = 
                                         strlen (pDhcpcReqSpec->dhcp_errmsg);
        bcopy (pDhcpcReqSpec->dhcp_errmsg, &dhcpcMsgOut.dhcp->options [offopt],
               strlen (pDhcpcReqSpec->dhcp_errmsg));
        offopt += strlen (pDhcpcReqSpec->dhcp_errmsg);
        }
    dhcpcMsgOut.dhcp->options [offopt] = _DHCP_END_TAG;

    /*
     * For backward compatibility with earlier DHCP servers, set the
     * reported message size to be at least as large as a BOOTP message.
     */

    msgsize = (DFLTDHCPLEN - DFLTOPTLEN) + offopt + 1;
    if (msgsize < DFLTBOOTPLEN)
        msgsize = DFLTBOOTPLEN;

    /* construct udp part */

    dhcpcMsgOut.udp->uh_sport = dhcpc_port;
    dhcpcMsgOut.udp->uh_dport = dhcps_port;
    dhcpcMsgOut.udp->uh_ulen = htons (msgsize + UDPHL);
    dhcpcMsgOut.udp->uh_sum = 0;

    /* fill pseudo udp header */

    pudph.srcip.s_addr = 0;
    pudph.dstip.s_addr = dhcpcBootParam->server_id.s_addr;
    pudph.zero = 0;
    pudph.prto = IPPROTO_UDP;
    pudph.ulen = dhcpcMsgOut.udp->uh_ulen;
    dhcpcMsgOut.udp->uh_sum = udp_cksum (&pudph, (char *) dhcpcMsgOut.udp,
                                         ntohs (pudph.ulen));

    /* construct ip part */

#if BSD<44
    dhcpcMsgOut.ip->ip_v_hl = 0;
    dhcpcMsgOut.ip->ip_v_hl = IPVERSION << 4;
    dhcpcMsgOut.ip->ip_v_hl |= IPHL >> 2;
#else
    dhcpcMsgOut.ip->ip_v = IPVERSION;
    dhcpcMsgOut.ip->ip_hl = IPHL >> 2;
#endif
    dhcpcMsgOut.ip->ip_tos = 0;
    dhcpcMsgOut.ip->ip_len = htons (msgsize + UDPHL + IPHL);
    tmpul = generate_xid ();
    tmpul += (tmpul >> 16);
    dhcpcMsgOut.ip->ip_id = (u_short) (~tmpul);
    dhcpcMsgOut.ip->ip_off = htons(IP_DF);                         /* XXX */
    dhcpcMsgOut.ip->ip_ttl = 0x20;                                 /* XXX */
    dhcpcMsgOut.ip->ip_p = IPPROTO_UDP;

    msgsize += UDPHL + IPHL;

    dhcpcMsgOut.ip->ip_src.s_addr = 0;
    dhcpcMsgOut.ip->ip_dst.s_addr = INADDR_BROADCAST;
    dhcpcMsgOut.ip->ip_sum = 0;

#if BSD<44
    dhcpcMsgOut.ip->ip_sum = checksum ( (u_short *)dhcpcMsgOut.ip,
                                        (dhcpcMsgOut.ip->ip_v_hl & 0xf) << 2);
#else
    dhcpcMsgOut.ip->ip_sum = checksum ( (u_short *)dhcpcMsgOut.ip,
                                        dhcpcMsgOut.ip->ip_hl << 2);
#endif

    return (msgsize);
    }

/*******************************************************************************
*
* dhcp_decline - send a DHCP decline message
*
* This routine constructs a message declining an offered IP address and sends
* it directly to the responding server. It is called when an ARP request 
* detects that the offered address is already in use.
*
* RETURNS: 0 if message sent, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int dhcp_decline
    (
    struct dhcp_reqspec *pDhcpcReqSpec
    )
    {
    struct sockaddr_in 	dest; 		/* Server's destination address */
    struct ifnet * 	pIf; 		/* Transmit device */
    int 		length; 	/* Amount of data in message */

#ifdef DHCPC_DEBUG
    char output [INET_ADDR_LEN];
#endif

    if (dhcpcBootParam->server_id.s_addr == 0)
        return (-1);

    length = make_decline (pDhcpcReqSpec);
    if (length == 0)
        return (-1);

    bzero ( (char *)&dest, sizeof (struct sockaddr_in));

    dest.sin_len = sizeof (struct sockaddr_in);
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = INADDR_BROADCAST;

    pIf = dhcpcIntface.iface;

    if (dhcpSend (pIf, &dest, sbuf.buf, length, TRUE) == ERROR)
        {
#ifdef DHCPC_DEBUG
        logMsg ("Can't send DHCPDECLINE.\n", 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

#ifdef DHCPC_DEBUG
    inet_ntoa_b (pDhcpcReqSpec->ipaddr, output);
    logMsg ("send DHCPDECLINE(%s)\n", (int)output, 0, 0, 0, 0, 0);
#endif
    return (0);
    }

/*******************************************************************************
*
* set_declinfo - initialize request specification for decline message
*
* This routine assigns the fields in the request specifier used to construct
* messages to the appropriate values for a DHCP decline message according to
* the parameters of the currently active lease.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL void set_declinfo
    (
    struct dhcp_reqspec *pDhcpcReqSpec,
    char *errmsg
    )
    {
    char output [INET_ADDR_LEN];

    pDhcpcReqSpec->ipaddr = dhcpcBootParam->yiaddr;
    if (dhcpcBootLeaseData.leaseReqSpec.clid != NULL)
        pDhcpcReqSpec->clid = dhcpcBootLeaseData.leaseReqSpec.clid;
    else
        pDhcpcReqSpec->clid = NULL;
    if (errmsg[0] == 0) 
        {
        inet_ntoa_b (dhcpcBootParam->yiaddr, output);
        sprintf (errmsg, "Unable to use offer with IP address %s.", output);
        }
    pDhcpcReqSpec->dhcp_errmsg = errmsg;

    return;
    }

/*******************************************************************************
*
* make_discover - construct a DHCP discover message
*
* This routine constructs an outgoing UDP/IP message containing the values
* required to broadcast a lease request.
*
* RETURNS: Size of discover message, in bytes
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int make_discover (void)
    {
    int offopt = 0; 		/* offset in options field */
    int msgsize; 		/* total size of DHCP message */
    u_long tmpul = 0;
    u_short tmpus = 0;

    /* construct dhcp part */

    bzero (sbuf.buf, sbuf.size);
    dhcpcMsgOut.dhcp->op = BOOTREQUEST;
    dhcpcMsgOut.dhcp->htype = dhcpcIntface.haddr.htype;
    dhcpcMsgOut.dhcp->hlen = dhcpcIntface.haddr.hlen;
    tmpul = generate_xid ();
    dhcpcMsgOut.dhcp->xid = htonl (tmpul);

    /* Update BPF filter to check for new transaction ID. */

    dhcpfilter [20].k = tmpul;
    ioctl (dhcpcIntface.bpfDev, BIOCSETF,  (u_int)&dhcpread);

    bcopy (dhcpcIntface.haddr.haddr, dhcpcMsgOut.dhcp->chaddr, 
           dhcpcMsgOut.dhcp->hlen);

    /* insert magic cookie */

    bcopy ( (char *)dhcpCookie, dhcpcMsgOut.dhcp->options, MAGIC_LEN);
    offopt = MAGIC_LEN;

    /* insert message type */

    dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_MSGTYPE_TAG;
    dhcpcMsgOut.dhcp->options [offopt++] = 1;
    dhcpcMsgOut.dhcp->options [offopt++] = DHCPDISCOVER;

    /* Insert requested IP address, if any. */

    if (dhcpcBootLeaseData.leaseReqSpec.ipaddr.s_addr != 0)
        {
        dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_REQUEST_IPADDR_TAG;
        dhcpcMsgOut.dhcp->options [offopt++] = 4;
        bcopy ( (char *)&dhcpcBootLeaseData.leaseReqSpec.ipaddr.s_addr,
                &dhcpcMsgOut.dhcp->options [offopt], 4);
        offopt += 4;
        }

    /* insert Maximum DHCP message size */

    dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_MAXMSGSIZE_TAG;
    dhcpcMsgOut.dhcp->options [offopt++] = 2;
    tmpus = htons (dhcpcBootLeaseData.leaseReqSpec.maxlen);
    bcopy ( (char *)&tmpus, &dhcpcMsgOut.dhcp->options [offopt], 2);
    offopt += 2;

    /* Insert request list, if any. */

    if (dhcpcBootLeaseData.leaseReqSpec.reqlist.len != 0)
        {
        dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_REQ_LIST_TAG;
        dhcpcMsgOut.dhcp->options [offopt++] =
                                dhcpcBootLeaseData.leaseReqSpec.reqlist.len;
        bcopy (dhcpcBootLeaseData.leaseReqSpec.reqlist.list,
               &dhcpcMsgOut.dhcp->options [offopt],
               dhcpcBootLeaseData.leaseReqSpec.reqlist.len);
        offopt += dhcpcBootLeaseData.leaseReqSpec.reqlist.len;
        }

    /* Insert all other entries from custom options field, if any. */

    if (dhcpcBootLeaseData.leaseReqSpec.pOptions != NULL)
        {
        bcopy (dhcpcBootLeaseData.leaseReqSpec.pOptions,
               &dhcpcMsgOut.dhcp->options [offopt], 
               dhcpcBootLeaseData.leaseReqSpec.optlen);
        offopt += dhcpcBootLeaseData.leaseReqSpec.optlen;
        }

    dhcpcMsgOut.dhcp->options[offopt] = _DHCP_END_TAG;

    msgsize = (DFLTDHCPLEN - DFLTOPTLEN) + offopt + 1;

    if (dhcpcOldFlag)
        {
        /*
         * This flag indicates that the client did not receive a response
         * to the initial set of discover messages. Older servers might
         * ignore messages less than the minimum length obtained with a
         * fixed options field, so pad the message to reach that length.
         * The initialize guarantees that the transmit buffer can store
         * the padded message.
         */

        if (msgsize < DFLTDHCPLEN)
            msgsize = DFLTDHCPLEN;
        }

    /* make udp part */

    /* fill udp header */

    dhcpcMsgOut.udp->uh_sport = dhcpc_port;
    dhcpcMsgOut.udp->uh_dport = dhcps_port;
    dhcpcMsgOut.udp->uh_ulen = htons (msgsize + UDPHL);

    /* fill pseudo udp header */

    spudph.srcip.s_addr = 0;
    spudph.dstip.s_addr = 0xffffffff;
    spudph.zero = 0;
    spudph.prto = IPPROTO_UDP;
    spudph.ulen = dhcpcMsgOut.udp->uh_ulen;

    /* make ip part */

    /* fill ip header */

#if BSD<44
    dhcpcMsgOut.ip->ip_v_hl = 0;
    dhcpcMsgOut.ip->ip_v_hl = IPVERSION << 4;
    dhcpcMsgOut.ip->ip_v_hl |= IPHL >> 2;
#else
    dhcpcMsgOut.ip->ip_v = IPVERSION;
    dhcpcMsgOut.ip->ip_hl = IPHL >> 2;
#endif
    dhcpcMsgOut.ip->ip_tos = 0;
    dhcpcMsgOut.ip->ip_len = htons (msgsize + UDPHL + IPHL);
    tmpul = generate_xid ();
    tmpul += (tmpul >> 16);
    dhcpcMsgOut.ip->ip_id = (u_short) (~tmpul);
    dhcpcMsgOut.ip->ip_off = htons (IP_DF);                        /* XXX */
    dhcpcMsgOut.ip->ip_ttl = 0x20;                                 /* XXX */
    dhcpcMsgOut.ip->ip_p = IPPROTO_UDP;
    dhcpcMsgOut.ip->ip_src.s_addr = 0;
    dhcpcMsgOut.ip->ip_dst.s_addr = 0xffffffff;
    dhcpcMsgOut.ip->ip_sum = 0;

    msgsize += UDPHL + IPHL;

#if BSD<44
  dhcpcMsgOut.ip->ip_sum = checksum ( (u_short *)dhcpcMsgOut.ip, 
                                      (dhcpcMsgOut.ip->ip_v_hl & 0xf) << 2);
#else
    dhcpcMsgOut.ip->ip_sum = checksum ( (u_short *)dhcpcMsgOut.ip, 
                                        dhcpcMsgOut.ip->ip_hl << 2);
#endif

    return (msgsize);
    }

/*******************************************************************************
*
* make_request - construct a DHCP request message
*
* This routine constructs an outgoing UDP/IP message containing the values
* required to request a lease (<type> argument of REQUESTING) or to obtain
* additional configuration parameters for an externally configured address
* (<type> argument of INFORMING).
*
* RETURNS: 0 if constructed succesfully, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int make_request
    (
    struct dhcp_param *paramp,
    int type
    )
    {
    int offopt = 0; 	/* offset in options field */
    int msgsize; 	/* total size of DHCP message */
    u_long tmpul = 0;
    u_short tmpus = 0;

    /* construct dhcp part */

    tmpus = dhcpcMsgOut.dhcp->secs;
    bzero (sbuf.buf, sbuf.size);
    dhcpcMsgOut.dhcp->op = BOOTREQUEST;
    dhcpcMsgOut.dhcp->htype = dhcpcIntface.haddr.htype;
    dhcpcMsgOut.dhcp->hlen = dhcpcIntface.haddr.hlen;
    tmpul = generate_xid ();
    dhcpcMsgOut.dhcp->xid = htonl (tmpul);

    /* Update BPF filter to check for new transaction ID. */

    dhcpfilter [20].k = tmpul;
    ioctl (dhcpcIntface.bpfDev, BIOCSETF,  (u_int)&dhcpread);

    dhcpcMsgOut.dhcp->secs = tmpus;

    if (type == INFORMING)
        {
        /* Set externally assigned IP address. */

        dhcpcMsgOut.dhcp->ciaddr.s_addr = 
                              dhcpcBootLeaseData.leaseReqSpec.ipaddr.s_addr;
        }
    else
        dhcpcMsgOut.dhcp->ciaddr.s_addr = 0;

    bcopy (dhcpcIntface.haddr.haddr, dhcpcMsgOut.dhcp->chaddr, 
           dhcpcMsgOut.dhcp->hlen);

    /* insert magic cookie */

    bcopy ( (char *)dhcpCookie, dhcpcMsgOut.dhcp->options, MAGIC_LEN);
    offopt = MAGIC_LEN;

    /* insert message type */

    dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_MSGTYPE_TAG;
    dhcpcMsgOut.dhcp->options [offopt++] = 1;
    if (type == INFORMING)
        dhcpcMsgOut.dhcp->options [offopt++] = DHCPINFORM;
    else
        dhcpcMsgOut.dhcp->options [offopt++] = DHCPREQUEST;

    /* Insert requesting IP address when required. Omit when forbidden. */

    if (type == REQUESTING)
        {
        if (paramp->yiaddr.s_addr != 0)
            {
            dhcpcMsgOut.dhcp->options[offopt++] = _DHCP_REQUEST_IPADDR_TAG;
            dhcpcMsgOut.dhcp->options[offopt++] = 4;
            bcopy ( (char *)&paramp->yiaddr.s_addr,
                   &dhcpcMsgOut.dhcp->options [offopt], 4);
            offopt += 4;
            }
        else
            return (-1);
        }

    /* Insert server identifier when required. Omit if forbidden. */

    if (type == REQUESTING)
        {
        if (paramp->server_id.s_addr == 0)
            {
            return(-1);
            }
        dhcpcMsgOut.dhcp->options[offopt++] = _DHCP_SERVER_ID_TAG;
        dhcpcMsgOut.dhcp->options[offopt++] = 4;
        bcopy ( (char *)&paramp->server_id.s_addr,
               &dhcpcMsgOut.dhcp->options [offopt], 4);
        offopt += 4;
        }

    /* insert Maximum DHCP message size */

    dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_MAXMSGSIZE_TAG;
    dhcpcMsgOut.dhcp->options [offopt++] = 2;
    tmpus = htons (dhcpcBootLeaseData.leaseReqSpec.maxlen);
    bcopy ( (char *)&tmpus, &dhcpcMsgOut.dhcp->options [offopt], 2);
    offopt += 2;

    /* Insert request list, if any. */

    if (dhcpcBootLeaseData.leaseReqSpec.reqlist.len != 0)
        {
        dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_REQ_LIST_TAG;
        dhcpcMsgOut.dhcp->options [offopt++] =
                                dhcpcBootLeaseData.leaseReqSpec.reqlist.len;
        bcopy (dhcpcBootLeaseData.leaseReqSpec.reqlist.list,
               &dhcpcMsgOut.dhcp->options [offopt],
               dhcpcBootLeaseData.leaseReqSpec.reqlist.len);
        offopt += dhcpcBootLeaseData.leaseReqSpec.reqlist.len;
        }

    /* Insert all other entries from custom options field, if any. */

    if (dhcpcBootLeaseData.leaseReqSpec.pOptions != NULL)
        {
        bcopy (dhcpcBootLeaseData.leaseReqSpec.pOptions,
               &dhcpcMsgOut.dhcp->options [offopt],
               dhcpcBootLeaseData.leaseReqSpec.optlen);
        offopt += dhcpcBootLeaseData.leaseReqSpec.optlen;
        }

    dhcpcMsgOut.dhcp->options[offopt] = _DHCP_END_TAG;

    msgsize = (DFLTDHCPLEN - DFLTOPTLEN) + offopt + 1;

    if (dhcpcOldFlag)
        {
        /*
         * This flag indicates that the client did not receive a response
         * to the initial set of discover messages but did receive one
         * using the older message format. The (older) responding server
         * ignores messages less than the minimum length obtained with a
         * fixed options field, so pad the message to reach that length.
         * The initialize guarantees that the transmit buffer can store
         * the padded message.
         */

        if (msgsize < DFLTDHCPLEN)
            msgsize = DFLTDHCPLEN;
        }

    /* make udp part */

    /* fill udp header */

    dhcpcMsgOut.udp->uh_sport = dhcpc_port;
    dhcpcMsgOut.udp->uh_dport = dhcps_port;
    dhcpcMsgOut.udp->uh_ulen = htons (msgsize + UDPHL);

    /* fill pseudo udp header */

    spudph.zero = 0;
    spudph.prto = IPPROTO_UDP;
    spudph.ulen = dhcpcMsgOut.udp->uh_ulen;

    /* make ip part */

    /* fill ip header */

#if BSD<44
    dhcpcMsgOut.ip->ip_v_hl = 0;
    dhcpcMsgOut.ip->ip_v_hl = IPVERSION << 4;
    dhcpcMsgOut.ip->ip_v_hl |= IPHL >> 2;
#else
    dhcpcMsgOut.ip->ip_v = IPVERSION;
    dhcpcMsgOut.ip->ip_hl = IPHL >> 2;
#endif
    dhcpcMsgOut.ip->ip_tos = 0;
    dhcpcMsgOut.ip->ip_len = htons (msgsize + UDPHL + IPHL);
    tmpul = generate_xid ();
    tmpul += (tmpul >> 16);
    dhcpcMsgOut.ip->ip_id = (u_short) (~tmpul);
    dhcpcMsgOut.ip->ip_off = htons(IP_DF);                         /* XXX */
    dhcpcMsgOut.ip->ip_ttl = 0x20;                                 /* XXX */
    dhcpcMsgOut.ip->ip_p = IPPROTO_UDP;

    msgsize += UDPHL + IPHL;

    if (type == INFORMING)
        dhcpcMsgOut.ip->ip_src.s_addr = spudph.srcip.s_addr =
                               dhcpcBootLeaseData.leaseReqSpec.ipaddr.s_addr;
    else
        dhcpcMsgOut.ip->ip_src.s_addr = spudph.srcip.s_addr = 0;

    dhcpcMsgOut.ip->ip_dst.s_addr = spudph.dstip.s_addr = 0xffffffff;

    dhcpcMsgOut.ip->ip_sum = 0;
#if BSD<44
    dhcpcMsgOut.ip->ip_sum = checksum ( (u_short *)dhcpcMsgOut.ip, 
                                        (dhcpcMsgOut.ip->ip_v_hl & 0xf) << 2);
#else
    dhcpcMsgOut.ip->ip_sum = checksum ( (u_short *)dhcpcMsgOut.ip, 
                                        dhcpcMsgOut.ip->ip_hl << 2);
#endif

    return (msgsize);
    }

/*******************************************************************************
*
* handle_ip - process DHCP options containing a single IP address
*
* This routine extracts the IP address from the given option body and 
* copies it to the appropriate field in the parameters structure. It is
* called by the dhcp_msgtoparam() conversion routine when specified by
* the handle_param[] global array.
*
* RETURNS: 0 if extraction successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int handle_ip
    (
    char *buf,
    struct dhcp_param *param
    )
    {
    struct in_addr *addr = NULL;
    char option;

    option = *buf;

    /* Set the appropriate pointers to access allocated memory. */

    if (option == _DHCP_SERVER_ID_TAG) 
        {
        addr = &param->server_id;
        } 
    else if (option == _DHCP_ROUTER_SOLICIT_TAG)
        addr = &param->router_solicit;
    else 
        {
        addr = (struct in_addr *)calloc (1, sizeof (struct in_addr));
        if (addr == NULL) 
            return (-1);

        switch (option) 
            {
            case _DHCP_SUBNET_MASK_TAG:
                if (param->subnet_mask != NULL) 
                    free (param->subnet_mask);
                param->subnet_mask = addr;
                break;
            case _DHCP_SWAP_SERVER_TAG:
                if (param->swap_server != NULL) 
                    free (param->swap_server);
                param->swap_server = addr;
                break;
            case _DHCP_BRDCAST_ADDR_TAG:
                if (param->brdcast_addr != NULL) 
                    free (param->brdcast_addr);
                param->brdcast_addr = addr;
                break;
            default:
                free (addr);
                return (EINVAL);
            }
        }

    bcopy (OPTBODY (buf), (char *)addr, DHCPOPTLEN (buf));
    return (0);
    }


/*******************************************************************************
*
* handle_num - process DHCP options containing a single numeric value
*
* This routine extracts the numeric value from the given option body and 
* stores it in the appropriate field in the parameters structure. It is
* called by the dhcp_msgtoparam() conversion routine when specified by
* the handle_param[] global array.
*
* RETURNS: 0 if extraction successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int handle_num
    (
    char *buf,
    struct dhcp_param *param
    )
    {
    char   charnum = 0;
    short  shortnum = 0;
    long   longnum = 0;

    switch (DHCPOPTLEN (buf)) 
        {
        case 1:
            charnum = *OPTBODY (buf);
            break;
        case 2:
            shortnum = GETHS (OPTBODY (buf));
            break;
        case 4:
            longnum = GETHL (OPTBODY (buf));
            break;
        default:
            return (-1);
        }

    switch (*buf) 
        {
        case _DHCP_TIME_OFFSET_TAG:
            param->time_offset = longnum;
            break;
        case _DHCP_BOOTSIZE_TAG:
            param->bootsize = (unsigned short)shortnum;
            break;
        case _DHCP_MAX_DGRAM_SIZE_TAG:
            param->max_dgram_size = (unsigned short)shortnum;
            break;
        case _DHCP_DEFAULT_IP_TTL_TAG:
            param->default_ip_ttl = (unsigned char)charnum;
            break;
        case _DHCP_MTU_AGING_TIMEOUT_TAG:
            param->mtu_aging_timeout = (unsigned long)longnum;
            break;
        case _DHCP_IF_MTU_TAG:
            param->intf_mtu = (unsigned short)shortnum;
            break;
        case _DHCP_ARP_CACHE_TIMEOUT_TAG:
            param->arp_cache_timeout = (unsigned long)longnum;
            break;
        case _DHCP_DEFAULT_TCP_TTL_TAG:
            param->default_tcp_ttl = (unsigned char)charnum;
            break;
        case _DHCP_KEEPALIVE_INTERVAL_TAG:
            param->keepalive_inter = (unsigned long)longnum;
            break;
        case _DHCP_NB_NODETYPE_TAG:
            param->nb_nodetype = (unsigned)charnum;
            break;
        case _DHCP_LEASE_TIME_TAG:
            param->lease_duration = (unsigned long)longnum;
            break;
        case _DHCP_T1_TAG:
            param->dhcp_t1 = (unsigned long)longnum;
            break;
        case _DHCP_T2_TAG:
            param->dhcp_t2 = (unsigned long)longnum;
            break;
        default:
            return (EINVAL);
        }

    return(0);
    }

/*******************************************************************************
*
* handle_ips - process DHCP options containing multiple IP addresses
*
* This routine extracts the IP addresses from the given option body and 
* copies them to the appropriate field in the parameters structure. It is
* called by the dhcp_msgtoparam() conversion routine when specified by
* the handle_param[] global array.
*
* RETURNS: 0 if extraction successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int handle_ips
    (
    char *buf,
    struct dhcp_param *param
    )
    {
    struct in_addr  *addr = NULL;
    struct in_addrs *addrs = NULL;
    unsigned char  num = 0;

    num = DHCPOPTLEN (buf) / 4;

    addr = (struct in_addr *)calloc ( (int)num, sizeof (struct in_addr));
    if (addr  == NULL)
        return(-1);

    addrs = (struct in_addrs *)calloc (1, sizeof(struct in_addrs));
    if (addrs  == NULL) 
        {
        free (addr);
        return (-1);
        }

    switch (*buf) 
        {
        case _DHCP_ROUTER_TAG:
            if (param->router != NULL) 
                {
                if (param->router->addr != NULL) 
                    free (param->router->addr);
                free (param->router);
                }
            param->router = addrs;
            param->router->num = num;
            param->router->addr = addr;
            break;
        case _DHCP_TIME_SERVER_TAG:
            if (param->time_server != NULL) 
                {
                if (param->time_server->addr != NULL) 
                    free (param->time_server->addr);
                free (param->time_server);
                }
            param->time_server = addrs;
            param->time_server->num = num;
            param->time_server->addr = addr;
            break;
        case _DHCP_NAME_SERVER_TAG:
            if (param->name_server != NULL) 
                {
                if (param->name_server->addr != NULL) 
                    free (param->name_server->addr);
                free (param->name_server);
                }
            param->name_server = addrs;
            param->name_server->num = num;
            param->name_server->addr = addr;
            break;
        case _DHCP_DNS_SERVER_TAG:
            if (param->dns_server != NULL) 
                {
                if (param->dns_server->addr != NULL) 
                    free (param->dns_server->addr);
                free (param->dns_server);
                }
            param->dns_server = addrs;
            param->dns_server->num = num;
            param->dns_server->addr = addr;
            break;
        case _DHCP_LOG_SERVER_TAG:
            if (param->log_server != NULL) 
                {
                if (param->log_server->addr != NULL) 
                    free (param->log_server->addr);
                free (param->log_server);
                }
            param->log_server = addrs;
            param->log_server->num = num;
            param->log_server->addr = addr;
            break;
        case _DHCP_COOKIE_SERVER_TAG:
            if (param->cookie_server != NULL) 
                {
                if (param->cookie_server->addr != NULL) 
                    free (param->cookie_server->addr);
                free (param->cookie_server);
                }
            param->cookie_server = addrs;
            param->cookie_server->num = num;
            param->cookie_server->addr = addr;
            break;
        case _DHCP_LPR_SERVER_TAG:
            if (param->lpr_server != NULL) 
                {
                if (param->lpr_server->addr != NULL) 
                    free (param->lpr_server->addr);
                free (param->lpr_server);
                }
            param->lpr_server = addrs;
            param->lpr_server->num = num;
            param->lpr_server->addr = addr;
            break;
        case _DHCP_IMPRESS_SERVER_TAG:
            if (param->impress_server != NULL) 
                {
                if (param->impress_server->addr != NULL) 
                    free (param->impress_server->addr);
                free (param->impress_server);
                }
            param->impress_server = addrs;
            param->impress_server->num = num;
            param->impress_server->addr = addr;
            break;
        case _DHCP_RLS_SERVER_TAG:
            if (param->rls_server != NULL) 
                {
                if (param->rls_server->addr != NULL) 
                    free (param->rls_server->addr);
                free (param->rls_server);
                }
            param->rls_server = addrs;
            param->rls_server->num = num;
            param->rls_server->addr = addr;
            break;
        case _DHCP_NIS_SERVER_TAG:
            if (param->nis_server != NULL) 
                {
                if (param->nis_server->addr != NULL) 
                    free (param->nis_server->addr);
                free (param->nis_server);
                }
            param->nis_server = addrs;
            param->nis_server->num = num;
            param->nis_server->addr = addr;
            break;
        case _DHCP_NTP_SERVER_TAG:
            if (param->ntp_server != NULL)
                {
                if (param->ntp_server->addr != NULL) 
                    free (param->ntp_server->addr);
                free (param->ntp_server);
                }
            param->ntp_server = addrs;
            param->ntp_server->num = num;
            param->ntp_server->addr = addr;
            break;
        case _DHCP_NBN_SERVER_TAG:
            if (param->nbn_server != NULL) 
                {
                if (param->nbn_server->addr != NULL) 
                    free (param->nbn_server->addr);
                free (param->nbn_server);
                }
            param->nbn_server = addrs;
            param->nbn_server->num = num;
            param->nbn_server->addr = addr;
            break;
        case _DHCP_NBDD_SERVER_TAG:
            if (param->nbdd_server != NULL) 
                {
                if (param->nbdd_server->addr != NULL) 
                    free (param->nbdd_server->addr);
                free (param->nbdd_server);
                }
            param->nbdd_server = addrs;
            param->nbdd_server->num = num;
            param->nbdd_server->addr = addr;
            break;
        case _DHCP_XFONT_SERVER_TAG:
            if (param->xfont_server != NULL)
                {
                if (param->xfont_server->addr != NULL) 
                    free (param->xfont_server->addr);
                free (param->xfont_server);
                }
            param->xfont_server = addrs;
            param->xfont_server->num = num;
            param->xfont_server->addr = addr;
            break;
        case _DHCP_XDISPLAY_MANAGER_TAG:
            if (param->xdisplay_manager != NULL) 
                {
                if (param->xdisplay_manager->addr != NULL) 
                    free (param->xdisplay_manager->addr);
                free (param->xdisplay_manager);
                }
            param->xdisplay_manager = addrs;
            param->xdisplay_manager->num = num;
            param->xdisplay_manager->addr = addr;
            break;
        case _DHCP_NISP_SERVER_TAG:
            if (param->nisp_server != NULL) 
                {
                if (param->nisp_server->addr != NULL) 
                    free (param->nisp_server->addr);
                free (param->nisp_server);
                }
            param->nisp_server = addrs;
            param->nisp_server->num = num;
            param->nisp_server->addr = addr;
            break;
        case _DHCP_MOBILEIP_HA_TAG:
            if (param->mobileip_ha != NULL) 
                {
                if (param->mobileip_ha->addr != NULL) 
                    free (param->mobileip_ha->addr);
                free (param->mobileip_ha);
                }
            param->mobileip_ha = addrs;
            param->mobileip_ha->num = num;
            param->mobileip_ha->addr = addr;
            break;
        case _DHCP_SMTP_SERVER_TAG:
            if (param->smtp_server != NULL) 
                {
                if (param->smtp_server->addr != NULL) 
                    free (param->smtp_server->addr);
                free (param->smtp_server);
                }
            param->smtp_server = addrs;
            param->smtp_server->num = num;
            param->smtp_server->addr = addr;
            break;
        case _DHCP_POP3_SERVER_TAG:
            if (param->pop3_server != NULL) 
                {
                if (param->pop3_server->addr != NULL) 
                    free (param->pop3_server->addr);
                free (param->pop3_server);
                }
            param->pop3_server = addrs;
            param->pop3_server->num = num;
            param->pop3_server->addr = addr;
            break;
        case _DHCP_NNTP_SERVER_TAG:
            if (param->nntp_server != NULL) 
                {
                if (param->nntp_server->addr != NULL) 
                    free (param->nntp_server->addr);
                free (param->nntp_server);
                }
            param->nntp_server = addrs;
            param->nntp_server->num = num;
            param->nntp_server->addr = addr;
            break;
        case _DHCP_DFLT_WWW_SERVER_TAG:
            if (param->dflt_www_server != NULL) 
                {
                if (param->dflt_www_server->addr != NULL) 
                    free (param->dflt_www_server->addr);
                free (param->dflt_www_server);
                }
            param->dflt_www_server = addrs;
            param->dflt_www_server->num = num;
            param->dflt_www_server->addr = addr;
            break;
        case _DHCP_DFLT_FINGER_SERVER_TAG:
            if (param->dflt_finger_server != NULL) 
                {
                if (param->dflt_finger_server->addr != NULL)
	            free (param->dflt_finger_server->addr);
                free (param->dflt_finger_server);
                }
            param->dflt_finger_server = addrs;
            param->dflt_finger_server->num = num;
            param->dflt_finger_server->addr = addr;
            break;
        case _DHCP_DFLT_IRC_SERVER_TAG:
            if (param->dflt_irc_server != NULL) 
                {
                if (param->dflt_irc_server->addr != NULL) 
                    free (param->dflt_irc_server->addr);
                free (param->dflt_irc_server);
                }
            param->dflt_irc_server = addrs;
            param->dflt_irc_server->num = num;
            param->dflt_irc_server->addr = addr;
            break;
        case _DHCP_STREETTALK_SERVER_TAG:
            if (param->streettalk_server != NULL) 
                {
                if (param->streettalk_server->addr != NULL) 
                    free (param->streettalk_server->addr);
                free (param->streettalk_server);
                }
            param->streettalk_server = addrs;
            param->streettalk_server->num = num;
            param->streettalk_server->addr = addr;
            break;
        case _DHCP_STDA_SERVER_TAG:
            if (param->stda_server != NULL) 
                {
                if (param->stda_server->addr != NULL) 
                    free (param->stda_server->addr);
                free (param->stda_server);
                }
            param->stda_server = addrs;
            param->stda_server->num = num;
            param->stda_server->addr = addr;
            break;
        default:
            free (addr);
            free (addrs);
            return (EINVAL);
        } 
    bcopy (OPTBODY (buf), (char *)addr, DHCPOPTLEN (buf));
    return (0);
    }

/*******************************************************************************
*
* handle_str - process DHCP options containing NVT ASCII strings
*
* This routine extracts the NVT ASCII string from the given option body and 
* copies it to the appropriate field in the parameters structure. It is
* called by the dhcp_msgtoparam() conversion routine when specified by
* the handle_param[] global array.
*
* RETURNS: 0 if extraction successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int handle_str
    (
    char *buf,
    struct dhcp_param *param
    )
    {
    char *str = NULL;

    str = calloc (1, (DHCPOPTLEN (buf) + 1));  /* +1 for null terminator. */
    if (str == NULL)
        return (-1);

    switch (*buf) 
        {
        case _DHCP_HOSTNAME_TAG:
            if (param->hostname != NULL) 
                free (param->hostname);
            param->hostname = str;
            break;
        case _DHCP_MERIT_DUMP_TAG:
            if (param->merit_dump != NULL) 
                free (param->merit_dump);
            param->merit_dump = str;
            break;
        case _DHCP_DNS_DOMAIN_TAG:
            if (param->dns_domain != NULL) 
                free (param->dns_domain);
            param->dns_domain = str;
            break;
        case _DHCP_ROOT_PATH_TAG:
            if (param->root_path != NULL) 
                free (param->root_path);
            param->root_path = str;
            break;
        case _DHCP_EXTENSIONS_PATH_TAG:
            if (param->extensions_path != NULL)
                free (param->extensions_path);
            param->extensions_path = str;
            break;
        case _DHCP_NIS_DOMAIN_TAG:
            if (param->nis_domain != NULL) 
                free (param->nis_domain);
            param->nis_domain = str;
            break;
        case _DHCP_NB_SCOPE_TAG:
            if (param->nb_scope != NULL) 
                free (param->nb_scope);
            param->nb_scope = str;
            break;
        case _DHCP_ERRMSG_TAG:
            if (param->errmsg != NULL) 
                free (param->errmsg);
            param->errmsg = str;
            break;
        case _DHCP_NISP_DOMAIN_TAG:
            if (param->nisp_domain != NULL) 
                free (param->nisp_domain);
            param->nisp_domain = str;
            break;
        case _DHCP_TFTP_SERVERNAME_TAG:
            if (param->temp_sname != NULL) 
                free (param->temp_sname);
            param->temp_sname = str;
            break;
        case _DHCP_BOOTFILE_TAG:
            if (param->temp_file != NULL) 
                free (param->temp_file);
            param->temp_file = str;
            break; 
        default:
            free (str);
            return (EINVAL);
        }
    bcopy (OPTBODY (buf), str, DHCPOPTLEN (buf));
    str [DHCPOPTLEN (buf)] = '\0';
    return (0);
    }

/*******************************************************************************
*
* handle_bool - process DHCP options containing boolean values
*
* This routine extracts the boolean value from the given option body and 
* stores it in the appropriate field in the parameters structure. It is
* called by the dhcp_msgtoparam() conversion routine when specified by
* the handle_param[] global array.
*
* RETURNS: 0 if extraction successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int handle_bool
    (
    char *buf,
    struct dhcp_param *param
    )
    {
    switch (*buf) 
        {
        case _DHCP_IP_FORWARD_TAG:
            param->ip_forward = *OPTBODY (buf);
            break;
        case _DHCP_NONLOCAL_SRCROUTE_TAG:
            param->nonlocal_srcroute = *OPTBODY (buf);
            break;
        case _DHCP_ALL_SUBNET_LOCAL_TAG:
            param->all_subnet_local = *OPTBODY (buf);
            break;
        case _DHCP_MASK_DISCOVER_TAG:
            param->mask_discover = *OPTBODY (buf);
            break;
        case _DHCP_MASK_SUPPLIER_TAG:
            param->mask_supplier = *OPTBODY (buf);
            break;
        case _DHCP_ROUTER_DISCOVER_TAG:
            param->router_discover = *OPTBODY (buf);
            break;
        case _DHCP_TRAILER_TAG:
            param->trailer = *OPTBODY (buf);
            break;
        case _DHCP_ETHER_ENCAP_TAG:
            param->ether_encap = *OPTBODY (buf);
            break;
        case _DHCP_KEEPALIVE_GARBAGE_TAG:
            param->keepalive_garba = *OPTBODY (buf);
            break;
        default:
            return (EINVAL);
        }
    return (0);
    }

/*******************************************************************************
*
* handle_ippairs - process DHCP options containing multiple IP value pairs
*
* This routine extracts the IP pairs from the given option body and 
* stores them in the appropriate field in the parameters structure. It is
* called by the dhcp_msgtoparam() conversion routine when specified by
* the handle_param[] global array.
*
* RETURNS: 0 if extraction successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int handle_ippairs
    (
    char *buf,
    struct dhcp_param *param
    )
    {
    struct in_addr  *addr = NULL;
    struct in_addrs *addrs = NULL;
    unsigned char   num = 0;

    num = DHCPOPTLEN(buf) / 4;
    addr = (struct in_addr *)calloc ( (int) num, sizeof (struct in_addr)); 
    if (addr == NULL)
        return(-1);

    addrs = (struct in_addrs *)calloc (1, sizeof (struct in_addrs));
    if (addrs == NULL)
        { 
        free (addr);
        return (-1);
        }

    switch (*buf) 
        {
        case _DHCP_POLICY_FILTER_TAG: /* IP address followed by subnet mask. */
            if (param->policy_filter != NULL) 
                {
                if (param->policy_filter->addr != NULL) 
                    free (param->policy_filter->addr);
                free (param->policy_filter);
                }
            param->policy_filter = addrs;
            param->policy_filter->num = num / 2;
            param->policy_filter->addr = addr;
            break;
        case _DHCP_STATIC_ROUTE_TAG:   /* Destination IP followed by router. */
            if (param->static_route != NULL) 
                {
                if (param->static_route->addr != NULL) 
                    free (param->static_route->addr);
                free (param->static_route);
                }
            param->static_route = addrs;
            param->static_route->num = num / 2;
            param->static_route->addr = addr;
            break;
        default:
            free (addr);
            free (addrs);
            return (EINVAL);
        }
    bcopy (OPTBODY (buf), (char *)addr, DHCPOPTLEN (buf));
    return (0);
    }

/*******************************************************************************
*
* handle_nums - process DHCP options containing multiple numeric values
*
* This routine extracts the numbers from the given option body and 
* stores them in the appropriate field in the parameters structure. It is
* called by the dhcp_msgtoparam() conversion routine when specified by
* the handle_param[] global array.
*
* RETURNS: 0 if extraction successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int handle_nums
    (
    char *buf,
    struct dhcp_param *param
    )
    {
    int i = 0;
    int max = 0;

    if (*buf != _DHCP_MTU_PLATEAU_TABLE_TAG) 
        return (EINVAL);
    else 
        {
        if (param->mtu_plateau_table != NULL) 
            {
            if (param->mtu_plateau_table->shortnum != NULL)
	        free (param->mtu_plateau_table->shortnum);
            free (param->mtu_plateau_table);
            }
        param->mtu_plateau_table = 
                       (struct u_shorts *)calloc(1, sizeof (struct u_shorts));
        if (param->mtu_plateau_table == NULL) 
            return(-1);

        max = param->mtu_plateau_table->num = DHCPOPTLEN (buf) / 2;
        param->mtu_plateau_table->shortnum =
                            (u_short *)calloc (max, sizeof (unsigned short));
        if (param->mtu_plateau_table->shortnum == NULL) 
            {
            free (param->mtu_plateau_table);
            return(-1);
            }

        for (i = 0; i < max; i++) 
            param->mtu_plateau_table->shortnum[i] = GETHS (&buf [i * 2 + 2]);
        }
    return (0);
    }

/*******************************************************************************
*
* handle_list - process the DHCP option containing vendor specific data 
*
* This routine extracts the list of vendor-specific information from the 
* given option body and stores it in the appropriate field in the parameters 
* structure. It is called by the dhcp_msgtoparam() conversion routine when 
* specified by the handle_param[] global array.
*
* RETURNS: 0 if extraction successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL int handle_list
    (
    char *buf,
    struct dhcp_param *param
    )
    {
    if (*buf != _DHCP_VENDOR_SPEC_TAG)
        return (EINVAL);
   
    if (param->vendlist != NULL)
        free (param->vendlist);

    param->vendlist = calloc (1, sizeof (struct vendor_list));
    if (param->vendlist == NULL)
        return (-1);

    bcopy (OPTBODY (buf), param->vendlist->list, DHCPOPTLEN (buf));
    param->vendlist->len = DHCPOPTLEN (buf);

    return (0);
    }

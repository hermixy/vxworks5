/* dhcpc_subr.c - DHCP client general subroutines */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01u,25apr02,wap  transmit DHCPDECLINEs as broadcasts, not unicasts (SPR
                 #75119)
01t,23apr02,wap  use dhcpTime() instead of time() (SPR #68900)
01s,06dec01,vvv  fixed arp_check again (SPR #69731)
01r,12oct01,rae  merge from truestack ver 01x, base 01n
                 SPRs 65352, 65264, 70116, 69731, 27426
01q,26oct00,spm  fixed byte order of transaction identifier
01p,26oct00,spm  fixed modification history after tor3_x merge
01o,23oct00,niq  merged from version 01q of tor3_x branch (base version 01n);
                 upgrade to BPF replaces tagged frame support
01n,01mar99,spm  eliminated creation of invalid route (SPR #24266)
01m,04dec97,spm  added code review modifications
01l,04dec97,spm  replaced muxDevExists call with test of predefined flag;
                 enabled ARP test of intended IP address
01k,06oct97,spm  removed reference to deleted endDriver global; replaced with
                 support for dynamic driver type detection
01j,26aug97,spm  major overhaul: reorganized code and changed user interface
                 to support multiple leases at runtime
01i,06aug97,spm  renamed class field of dhcp_reqspec structure to prevent C++
                 compilation errors (SPR #9079)
01h,15jul97,spm  set sa_len and reordered ioctl calls in configuration routine,
                 changed to raw sockets to avoid memPartFree error (SPR #8650);
                 replaced floating point to prevent ss5 exception (SPR #8738)
01g,10jun97,spm  corrected load exception (SPR #8724) and memalign() parameter;
                 isolated incoming messages in state machine from input hooks
01f,02jun97,spm  changed DHCP option tags to prevent name conflicts (SPR #8667)
                 and updated man pages
01e,09may97,spm  changed memory access to align IP header on four byte boundary
01d,18apr97,spm  added conditional include DHCPC_DEBUG for displayed output,
                 shut down interface when changing address/netmask
01c,07apr97,spm  corrected bugs extracting options, added safety checks and
                 memory cleanup code, rewrote documentation
01b,27jan97,spm  added little-endian support and modified to coding standards 
01a,03oct96,spm  created by modifying WIDE Project DHCP Implementation
*/

/*
DESCRIPTION
This library contains subroutines which implement several utilities for the 
Wide project DHCP client, modified for vxWorks compatibility.

INTERNAL
The routines in this module are called by both portions of the runtime state 
machine, in the dhcpcState1 and dhcpcState2 modules, which execute before and 
after a lease is established.

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
#include <fcntl.h>

#include <sys/types.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <inetLib.h>

#include <sysLib.h>
#include <logLib.h>
#include <sockLib.h>
#include <ioLib.h>
#include <semLib.h>
#include <vxLib.h>
#include "muxLib.h"

#include "dhcp/dhcp.h"
#include "dhcp/dhcpcStateLib.h"
#include "dhcp/dhcpcCommonLib.h"
#include "dhcp/dhcpcInternal.h"

#include "bpfDrv.h"

/* globals */

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

IMPORT struct bpf_insn dhcpfilter[];    /* For updating xid test */
IMPORT struct bpf_program dhcpread;

IMPORT int dhcpcDataSock; 	/* For sending release and renewal messages */

IMPORT int flushroutes (void);

struct msg dhcpcMsgOut;
struct msg dhcpcMsgIn;
struct ps_udph spudph;

/* locals */

LOCAL int bpfArpDev; 			/* Gets any replies to ARP probes. */
LOCAL u_short dhcps_port;               /* Server port, in network byte order. */
LOCAL u_short dhcpc_port;               /* Client port, in network byte order. */

/* forward declarations */

int config_if();
void set_route();
int make_decline (struct dhcp_reqspec *, struct if_info *);
int make_release (struct dhcp_reqspec *, struct if_info *, BOOL);
long generate_xid (struct if_info *);

/*******************************************************************************
*
* generate_xid - generate a transaction identifier 
*
* This routine forms a transaction identifier which is used by the lease 
* for transmissions over the interface described by <pIfData>. The routine 
* is called from multiple locations after the initialization routines have 
* retrieved the network interface hardware address.
*
* RETURNS: 32-bit transaction identifier in host byte order. 
*
* ERRNO: N/A
*
* NOMANUAL
*/

long generate_xid
    (
    struct if_info * 	pIfData 	/* interface used by lease */
    )
    {
    time_t current = 0;
    u_short result1 = 0;
    u_short result2 = 0;
    u_short result3 = 0;

    u_short tmp[6]; 

    bcopy (pIfData->haddr.haddr, (char *)tmp, 6);

    dhcpTime (&current);
    result1 = checksum (tmp, 6);
    result2 = checksum ( (u_short *)&current, 2);

    /*
     * Although unlikely, it is possible that separate leases which are using
     * the same network interface calculate a checksum at the same time. To
     * guarantee uniqueness, include the value of the lease-specific pointer
     * to the interface descriptor.
     */

    tmp [0] = ((long)pIfData) >> 16;
    tmp [1] = ((long)pIfData) & 0xffff;

    result3 = checksum (tmp, 4);

    return ( (result1 << 16) + result2 + result3);
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
    struct if_info *pIfData 	/* interface used by lease */
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

    pField1 = &inbuf [ARPHL]; 			/* Source hardware address. */
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

    /* Set BPF to monitor interface for reply. */

    bzero ( (char *)&ifr, sizeof (struct ifreq));
    sprintf (ifr.ifr_name, "%s%d", pIfData->iface->if_name,
                                   pIfData->iface->if_unit);
    if (ioctl (bpfArpDev, BIOCSETIF, (int)&ifr) != 0)
        return (OK);     /* Ignore errors (permits use of IP address). */

    /*
     * Moved dhcpcArpSend after SETIF so that a valid reply received before 
     * BPF device activation is not missed.
     */

    dhcpcArpSend (pIfData->iface, inbuf, tmp);

    /* Wait one-half second for (unexpected) reply from target IP address. */

    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    FD_ZERO (&readFds);
    FD_SET (bpfArpDev, &readFds);

    tmp = select (bpfArpDev + 1, &readFds, NULL, NULL, &timeout);

    /* Disconnect BPF device from input stream until next ARP probe. */

    ioctl (bpfArpDev, BIOCSTOP, 0);

    if (tmp) 	/* ARP reply received? Duplicate IP address. */
        return (ERROR);
    else 	/* Timeout occurred - no ARP reply. Probe succeeded. */
        return (OK);
    }

/*******************************************************************************
*
* arp_reply - use ARP to notify other hosts of new address in use
*
* This routine broadcasts an ARP reply when the DHCP client changes addresses
* to a new value retrieved from a DHCP server.
*
* RETURNS: -1 if ARP indicates client in use, or 0 otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int arp_reply
    (
    struct in_addr *ipaddr,
    struct if_info *pIfData 	/* interface used by lease */
    )
    {
    int i = 0;
    char inbuf [MAX_ARPLEN];
    char *pField1;
    char *pField2;
    struct arphdr * pArpHdr = NULL;

    bzero (inbuf, MAX_ARPLEN);
    pArpHdr = (struct arphdr *) inbuf;

    pArpHdr->ar_hrd = htons (pIfData->haddr.htype);
    pArpHdr->ar_pro = htons (ETHERTYPE_IP);
    pArpHdr->ar_hln = pIfData->haddr.hlen;
    pArpHdr->ar_pln = 4;
    pArpHdr->ar_op = htons (ARPOP_REPLY);

    /*
     * Set sender and target H/W addresses to your address
     * for sending an unsolicited reply.
     */

    pField1 = &inbuf [ARPHL];                   /* Source hardware address. */
    pField2 = pField1 + pArpHdr->ar_hln + 4;    /* Target hardware address. */
    for (i = 0; i < pIfData->haddr.hlen; i++)
        {
        pField1[i] = pField2[i] = pIfData->haddr.haddr[i];
        }

    /*
     * Set sender and target IP address to given value
     * to announce new IP address usage.
     */

    pField1 += pArpHdr->ar_hln;     /* Source IP address. */
    pField2 += pArpHdr->ar_hln;     /* Target IP address. */

    bcopy ( (char *)&ipaddr->s_addr, pField1, pArpHdr->ar_pln);
    bcopy ( (char *)&ipaddr->s_addr, pField2, pArpHdr->ar_pln);

    /* Broadcast unsolicited reply to notify all hosts of new IP address. */

    i = ARPHL + 2 * (pArpHdr->ar_hln + 4);    /* Size of ARP message. */

    dhcpcArpSend (pIfData->iface, inbuf, i);

    return (0);
    }

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

int nvttostr
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

/*******************************************************************************
*
* align_msg - set the buffer pointers to access message components
*
* This routine sets the pointers in the given message descriptor
* structure to access the various components of a received DHCP
* message. It is used internally by the state machine for incoming
* data stored from the BPF device. 
*
* RETURNS: N/A
*
* ERRNO:   N/A
*
* NOMANUAL
*/

void align_msg
    (
    struct msg * 	rmsg, 	/* Components of received message */
    char * 		rbuf 	/* Received IP packet */
    )
    {
    rmsg->ip = (struct ip *) rbuf;

    if ( (ntohs (rmsg->ip->ip_off) & 0x1fff) == 0 &&
        ntohs (rmsg->ip->ip_len) >= (DFLTDHCPLEN - DFLTOPTLEN + 4) + UDPHL 
				     + IPHL)
        {
#if BSD<44
        rmsg->udp = (struct udphdr *)&rbuf [ (rmsg->ip->ip_v_hl & 0xf) * WORD];
        rmsg->dhcp = (struct dhcp *)&rbuf [ (rmsg->ip->ip_v_hl & 0xf) * WORD +
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

int dhcp_msgtoparam
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

int clean_param
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
int merge_param
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
* initialize - initialize data structures for network message transfer
*
* This routine sets the DHCP ports, resets the network interface, and allocates
* storage for incoming messages. It is called from the dhcp_client_setup()
* routine during the overall initialization of the DHCP client library.
*
* RETURNS: 0 if initialization successful, or -1 otherwise. 
*
* ERRNO: N/A
*
* NOMANUAL
*/

int initialize
    (
    int 	serverPort, 	/* port monitored by DHCP servers */
    int 	clientPort, 	/* port monitored by DHCP client */
    int 	bufSize 	/* required size for transmit buffer */
    )
    {
    char * 	sbufp;
    int 	arpSize; 	/* Maximum size for ARP reply. */
    int         result;

    /* Create BPF device for performing ARP probes. */

    arpSize = bufSize - DFLTDHCPLEN - UDPHL - IPHL; 	/* Max. link header */
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

    /* Always use default ports for client and server. */

    dhcps_port = htons (serverPort);
    dhcpc_port = htons (clientPort);

    sbuf.size = bufSize;    /* Maximum message size, provided by user. */

    if ( (sbuf.buf = (char *)memalign (4, bufSize)) == NULL)
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
* reset_if - halt the network and reset the network interface
*
* This routine sets the IP address of the network interface to a random
* value of 10.x.x.x, which is an old ARPA debugging address, resets the
* broadcast address and subnet mask, and flushes the routing tables.
* It is called before initiating a lease negotiation if no event notification 
* hook is present. It is also always called for a lease established at boot 
* time, whether or not an event hook is registered for that lease.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void reset_if
    (
    struct if_info * 	pIfData 	/* interface used by lease */
    )
    {
    struct in_addr addr;
    struct in_addr mask; 
    struct in_addr brdaddr;

    addr.s_addr = htonl ( (generate_xid (pIfData) & 0xfff) | 0x0a000000);

    mask.s_addr = htonl (0xff000000);
    brdaddr.s_addr = inet_addr ("255.255.255.255");

    config_if (pIfData, &addr, &mask, &brdaddr);

    flushroutes ();
    }

/*******************************************************************************
*
* down_if - change network interface flags
*
* This routine clears the IFF_UP flag of the network interface. It is called
* when a lease is manually terminated with a dhcpcRelease() or dhcpcShutdown()
* call if no event hook is present for the lease. It is always called when
* removing a lease established at boot time.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void down_if
    (
    struct if_info *ifp
    )
    {
    int sockfd;
    struct ifreq ridreq;

    reset_if (ifp);

    if ( (sockfd = socket (AF_INET, SOCK_RAW, 0)) < 0) 
        return;

    bzero ( (char *)&ridreq, sizeof (struct ifreq));
    sprintf (ridreq.ifr_name, "%s%d", ifp->name, ifp->unit);
    ridreq.ifr_addr.sa_len = sizeof (struct sockaddr_in);

    /* down interface */
    ioctl (sockfd, SIOCGIFFLAGS, (int) (caddr_t) &ridreq);
    ridreq.ifr_flags &= (~IFF_UP);
    ioctl (sockfd, SIOCSIFFLAGS, (int) (caddr_t) &ridreq);

    close (sockfd);

    return;
    }

/*******************************************************************************
*
* config_if - configure network interface
*
* This routine sets one or more of the current address, broadcast address, and 
* subnet mask, depending on whether or not the corresponding parameters are 
* NULL.  It is called when a lease is obtained or the network is reset. When a 
* new lease is obtained, a successful return value is used to cause assignment 
* of new routing tables.
*
* RETURNS: -1 on error, 0 if new settings assigned, 1 otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int config_if
    (
    struct if_info *ifp,
    struct in_addr *addr,
    struct in_addr *mask,
    struct in_addr *brdcst
    )
    {
    int sockfd = 0;
    int status;
    struct ifreq ifr;
    struct in_addr current_addr;
    struct in_addr  current_mask;
    struct in_addr  current_brdcst;

    if ( (sockfd = socket (AF_INET, SOCK_RAW, 0)) < 0) 
        return (-1);

    bzero ( (char *)&current_addr, sizeof (current_addr));
    bzero ( (char *)&current_mask, sizeof (current_mask));
    bzero ( (char *)&current_brdcst, sizeof (current_brdcst));
    bzero ( (char *)&ifr, sizeof (struct ifreq));

    sprintf (ifr.ifr_name, "%s%d", ifp->name, ifp->unit);
    ifr.ifr_addr.sa_len = sizeof (struct sockaddr_in);

    status = ioctl (sockfd, SIOCGIFADDR, (int)&ifr);
    current_addr.s_addr =
                       ( (struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

    status = ioctl (sockfd, SIOCGIFNETMASK, (int)&ifr);
    current_mask.s_addr =
                       ( (struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

    status = ioctl (sockfd, SIOCGIFBRDADDR, (int)&ifr);
    current_brdcst.s_addr =
                  ( (struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr.s_addr;

    if (current_addr.s_addr == addr->s_addr &&
        (mask == NULL || current_mask.s_addr == mask->s_addr) &&
        (brdcst == NULL || current_brdcst.s_addr == brdcst->s_addr)) 
        {
        close (sockfd);
        return (1);
        }

    flushroutes (); 

    /* down interface */

    ioctl (sockfd, SIOCGIFFLAGS, (int)&ifr);
    ifr.ifr_flags &= (~IFF_UP);
    ioctl (sockfd, SIOCSIFFLAGS, (int)&ifr);

    /*
     * Deleting the interface address is required to correctly scrub the
     * routing table based on the current netmask.
     */

    bzero ( (char *)&ifr, sizeof (struct ifreq));
    sprintf (ifr.ifr_name, "%s%d", ifp->name, ifp->unit);
    ifr.ifr_addr.sa_len = sizeof (struct sockaddr_in);
    ( (struct sockaddr_in *) &ifr.ifr_addr)->sin_family = AF_INET;
    ( (struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr = 
                                                           current_addr.s_addr;
    status = ioctl (sockfd, SIOCDIFADDR, (int)&ifr);
    if (status < 0) 
        {
         /* Sometimes no address has been set, so ignore that error. */

        if (errno != EADDRNOTAVAIL)
            {
            close (sockfd);
            return (-1);
            }
        }

    if (mask != NULL) 
        {
        bzero ( (char *)&ifr, sizeof (struct ifreq));
        sprintf (ifr.ifr_name, "%s%d", ifp->name, ifp->unit);
        ifr.ifr_addr.sa_len = sizeof (struct sockaddr_in);
        ( (struct sockaddr_in *) &ifr.ifr_addr)->sin_family = AF_INET;
        ( (struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr = 
                                                                 mask->s_addr;
        status = ioctl (sockfd, SIOCSIFNETMASK, (int)&ifr);
        if (status < 0) 
            {
            close (sockfd);
            return (-1);
            }
        }

    if (brdcst != NULL) 
        {
        bzero ( (char *)&ifr, sizeof (struct ifreq));
        sprintf (ifr.ifr_name, "%s%d", ifp->name, ifp->unit);
        ifr.ifr_addr.sa_len = sizeof (struct sockaddr_in);
        ( (struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_family = AF_INET;
        ( (struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr.s_addr = 
                                                               brdcst->s_addr;
        status = ioctl (sockfd, SIOCSIFBRDADDR, (int)&ifr);
        if (status < 0) 
            {
            close (sockfd);
            return (-1);
            }
        }

    if (addr != NULL) 
        {
        bzero ( (char *)&ifr, sizeof (struct ifreq));
        sprintf (ifr.ifr_name, "%s%d", ifp->name, ifp->unit);
        ifr.ifr_addr.sa_len = sizeof (struct sockaddr_in);
        ( (struct sockaddr_in *) &ifr.ifr_addr)->sin_family = AF_INET;
        ( (struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr = 
                                                                 addr->s_addr;
        status = ioctl (sockfd, SIOCSIFADDR, (int)&ifr);
        if (status < 0) 
            {
            close (sockfd);
            return (-1);
            }
        }

    /* bring interface back up */

    ioctl (sockfd, SIOCGIFFLAGS, (int)&ifr);
    ifr.ifr_flags |= IFF_UP;
    ioctl (sockfd, SIOCSIFFLAGS, (int)&ifr);
    close (sockfd);

    return (0);
    }

/*******************************************************************************
*
* set_route - set network routing table
*
* This routine is called when config_if() assigns new address information.
* It sets the default route for a new lease if the DHCP server provided the
* router IP address.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void set_route
    (
    struct dhcp_param *param
    )
    {
    int sockfd = 0;

#if BSD<44
    struct rtentry rt;
#else
    struct ortentry rt;
#endif

    struct sockaddr dst, gateway;

    if (param == NULL)
        return;

    sockfd = socket(AF_INET, SOCK_RAW, 0);
    if (sockfd < 0) 
        {
#ifdef DHCPC_DEBUG
        logMsg("socket() error in set_route()\n", 0, 0, 0, 0, 0, 0);
#endif
        return;
        }

    /* set default route, if router IP address is available. */

    if (ISSET(param->got_option, _DHCP_ROUTER_TAG) && param->router != NULL &&
        param->router->addr != NULL) 
        {
#if BSD<44
        bzero ( (char *)&rt, sizeof (struct rtentry));
#else
        bzero ( (char *)&rt, sizeof (struct ortentry));
#endif

        bzero ( (char *)&dst, sizeof (struct sockaddr));
        bzero ( (char *)&gateway, sizeof (struct sockaddr));
        rt.rt_flags = RTF_UP | RTF_GATEWAY;

        ( (struct sockaddr_in *)&dst)->sin_family = AF_INET;
        ( (struct sockaddr_in *)&dst)->sin_len = sizeof (struct sockaddr_in);
        ( (struct sockaddr_in *)&dst)->sin_addr.s_addr = INADDR_ANY; 

        ( (struct sockaddr_in *)&gateway)->sin_family = AF_INET;
        ( (struct sockaddr_in *)&gateway)->sin_len = 
                                                   sizeof (struct sockaddr_in);
        ( (struct sockaddr_in *)&gateway)->sin_addr.s_addr =
                                                   param->router->addr->s_addr;
        rt.rt_dst = dst;
        rt.rt_gateway = gateway;

        if (ioctl (sockfd, SIOCADDRT, (int)&rt) < 0) 
            {
#ifdef DHCPC_DEBUG
            logMsg ("SIOCADDRT (default route)\n", 0, 0, 0, 0, 0, 0);
#endif
            close (sockfd);
            }
        }

    close (sockfd);
    return;
    }

/*******************************************************************************
*
* make_decline - construct a DHCP decline message
*
* This routine constructs an outgoing UDP/IP message containing the values
* required to decline an offered IP address.
*
* RETURNS: size of decline message, or 0 if error
*
* ERRNO: N/A
*
* NOMANUAL
*/

int make_decline
    (
    struct dhcp_reqspec *pReqSpec,
    struct if_info * 	pIfData 	/* interface used by lease */
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
    dhcpcMsgOut.dhcp->htype = pIfData->haddr.htype;
    dhcpcMsgOut.dhcp->hlen = pIfData->haddr.hlen;
    dhcpcMsgOut.dhcp->xid = htonl (generate_xid (pIfData));
    dhcpcMsgOut.dhcp->giaddr = dhcpcMsgIn.dhcp->giaddr;
    bcopy (pIfData->haddr.haddr, dhcpcMsgOut.dhcp->chaddr,
           dhcpcMsgOut.dhcp->hlen);

    /* insert magic cookie */

    bcopy ( (char *)dhcpCookie, dhcpcMsgOut.dhcp->options, MAGIC_LEN);
    offopt = MAGIC_LEN;

    /* insert message type */

    dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_MSGTYPE_TAG;
    dhcpcMsgOut.dhcp->options [offopt++] = 1;
    dhcpcMsgOut.dhcp->options [offopt++] = DHCPDECLINE;

    /* insert requested IP */

    if (pReqSpec->ipaddr.s_addr == 0)
        return (0);

    dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_REQUEST_IPADDR_TAG;
    dhcpcMsgOut.dhcp->options [offopt++] = 4;
    bcopy ( (char *)&pReqSpec->ipaddr, &dhcpcMsgOut.dhcp->options [offopt], 4);
    offopt += 4;

    /* insert client identifier */

    if (pReqSpec->clid != NULL)
        {
        dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_CLIENT_ID_TAG;
        dhcpcMsgOut.dhcp->options [offopt++] = pReqSpec->clid->len;
        bcopy (pReqSpec->clid->id, &dhcpcMsgOut.dhcp->options [offopt], 
               pReqSpec->clid->len);
        offopt += pReqSpec->clid->len;
        }

    /* insert server identifier */

   dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_SERVER_ID_TAG;
   dhcpcMsgOut.dhcp->options [offopt++] = 4;
   bcopy ( (char *)&pReqSpec->srvaddr, &dhcpcMsgOut.dhcp->options [offopt], 4);
   offopt += 4;

    /* Insert error message, if available. */

    if (pReqSpec->dhcp_errmsg != NULL)
        {
        dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_ERRMSG_TAG;
        dhcpcMsgOut.dhcp->options [offopt++] = strlen (pReqSpec->dhcp_errmsg);
        bcopy (pReqSpec->dhcp_errmsg, &dhcpcMsgOut.dhcp->options [offopt],
	       strlen (pReqSpec->dhcp_errmsg));
        offopt += strlen (pReqSpec->dhcp_errmsg);
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
    pudph.dstip.s_addr = pReqSpec->srvaddr.s_addr;
    pudph.zero = 0;
    pudph.prto = IPPROTO_UDP;
    pudph.ulen = dhcpcMsgOut.udp->uh_ulen;
    dhcpcMsgOut.udp->uh_sum = udp_cksum (&pudph, (char *)dhcpcMsgOut.udp,
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
    tmpul = generate_xid (pIfData);
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
* make_release - construct a DHCP release message
*
* This routine constructs an outgoing UDP/IP message containing the values
* required to relinquish the active lease.
*
* RETURNS: Size of DHCP message, in bytes
*
* ERRNO: N/A
*
* NOMANUAL
*/

int make_release
    (
    struct dhcp_reqspec *pReqSpec,
    struct if_info * 	pIfData, 	/* interface used by lease */
    BOOL 	oldFlag 	/* Use older (padded) DHCP message format? */
    )
    {
    int offopt = 0; 	/* offset in options field */
    int msgsize;

    /* construct dhcp part */

    bzero (sbuf.buf, sbuf.size);
    dhcpcMsgOut.dhcp->op = BOOTREQUEST;
    dhcpcMsgOut.dhcp->htype = pIfData->haddr.htype;
    dhcpcMsgOut.dhcp->hlen = pIfData->haddr.hlen;
    dhcpcMsgOut.dhcp->xid = htonl (generate_xid (pIfData));
    dhcpcMsgOut.dhcp->ciaddr = pReqSpec->ipaddr;
    bcopy (pIfData->haddr.haddr, dhcpcMsgOut.dhcp->chaddr,
           dhcpcMsgOut.dhcp->hlen);

    /* insert magic cookie */

    bcopy ( (char *)dhcpCookie, dhcpcMsgOut.dhcp->options, MAGIC_LEN);
    offopt = MAGIC_LEN;

    /* insert message type */

    dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_MSGTYPE_TAG;
    dhcpcMsgOut.dhcp->options [offopt++] = 1;
    dhcpcMsgOut.dhcp->options [offopt++] = DHCPRELEASE;

    /* insert client identifier */

    if (pReqSpec->clid != NULL)
        {
        dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_CLIENT_ID_TAG;
        dhcpcMsgOut.dhcp->options [offopt++] = pReqSpec->clid->len;
        bcopy (pReqSpec->clid->id, &dhcpcMsgOut.dhcp->options [offopt],
               pReqSpec->clid->len);
        offopt += pReqSpec->clid->len;
        }

    /* insert server identifier */

    dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_SERVER_ID_TAG;
    dhcpcMsgOut.dhcp->options [offopt++] = 4;
    bcopy ( (char *)&pReqSpec->srvaddr, &dhcpcMsgOut.dhcp->options [offopt], 4);
    offopt += 4;

    /* Insert error message, if available. */

    if (pReqSpec->dhcp_errmsg != NULL)
        {
        dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_ERRMSG_TAG;
        dhcpcMsgOut.dhcp->options [offopt++] = strlen (pReqSpec->dhcp_errmsg);
        bcopy (pReqSpec->dhcp_errmsg, &dhcpcMsgOut.dhcp->options [offopt],
               strlen (pReqSpec->dhcp_errmsg));
        offopt += strlen (pReqSpec->dhcp_errmsg);
        }
    dhcpcMsgOut.dhcp->options [offopt] = _DHCP_END_TAG;

    msgsize = (DFLTDHCPLEN - DFLTOPTLEN) + offopt + 1;

    if (oldFlag)
        {
        /*
         * This flag indicates that the client did not receive a response
         * to the initial set of discover messages but did receive one
         * using the older message format. The (older) responding server
         * ignores messages less than the minimum length obtained with a
         * fixed options field, so pad the message to reach that length.
         */

        if (msgsize < DFLTDHCPLEN)
            msgsize = DFLTDHCPLEN;
        }

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

int dhcp_decline
    (
    struct dhcp_reqspec * 	pDhcpcReqSpec,
    struct if_info * 		pIfData 	/* interface used by lease */
    )
    {
    struct sockaddr_in 	dest; 		/* Server's destination address */
    struct ifnet * 	pIf; 		/* Transmit device */
    int 		length; 	/* Amount of data in message */

#ifdef DHCPC_DEBUG
    char output [INET_ADDR_LEN];
#endif

    if (pDhcpcReqSpec->srvaddr.s_addr == 0)
        return(-1);

    length = make_decline (pDhcpcReqSpec, pIfData);
    if (length == 0)
        return (-1);

    bzero ( (char *)&dest, sizeof (struct sockaddr_in));

    dest.sin_len = sizeof (struct sockaddr_in);
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = INADDR_BROADCAST;

    pIf = pIfData->iface;

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
* dhcp_release - send a DHCP release message
*
* This routine constructs a message declining an offered IP address and sends
* it directly to the responding server. It is called when an error prevents
* the use of an acquired lease, or when the lease is relinquished manually
* by a dhcpcRelease() or dhcpcShutdown() call. The message is sent directly to 
* the responding DHCP server.
*
* RETURNS: 0 if message sent, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int dhcp_release
    (
    struct dhcp_reqspec *pDhcpcReqSpec,
    struct if_info * 		pIfData, 	/* interface used by lease */
    BOOL 	oldFlag 	/* Use older (padded) DHCP message format? */
    )
    {
#ifdef DHCPC_DEBUG
    char output [INET_ADDR_LEN];
#endif
    int length;

    if (pDhcpcReqSpec->srvaddr.s_addr == 0)
        return (-1);

    /* send DHCP message */

    length = make_release (pDhcpcReqSpec, pIfData, oldFlag);

    if (send_unicast (&pDhcpcReqSpec->srvaddr, dhcpcMsgOut.dhcp, length) < 0) 
        return (-1);

#ifdef DHCPC_DEBUG
    inet_ntoa_b (pDhcpcReqSpec->ipaddr, output);
    logMsg("send DHCPRELEASE(%s)\n", (int)output, 0, 0, 0, 0, 0);
#endif
    return(0);
    }

/*******************************************************************************
*
* dhcpcPrivateCleanup - remove data structures from client library
*
* The dhcpcCleanup routine uses this routine to release the locally
* allocated data structures which the initialize() call creates. It is
* part of the shutdown process for the DHCP client library. The routine
* executes after all leases are inactive and their data is released.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void dhcpcPrivateCleanup (void)
    {
    /* Close open file and remove BPF device for ARP probe. */

    close (bpfArpDev);

    bpfDevDelete ("/bpf/dhcpc-arp");

    /* Release transmission buffer. */

    free (sbuf.buf);

    return;
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

void set_declinfo
    (
    struct dhcp_reqspec *pDhcpcReqSpec,
    LEASE_DATA * 	pLeaseData,
    char *errmsg,
    int arpans
    )
    {
    char output [INET_ADDR_LEN];
    struct dhcp_param * 	paramp;

    paramp = pLeaseData->dhcpcParam;

    pDhcpcReqSpec->ipaddr = paramp->yiaddr;
    pDhcpcReqSpec->srvaddr = paramp->server_id;
    if (pLeaseData->leaseReqSpec.clid != NULL)
        pDhcpcReqSpec->clid = pLeaseData->leaseReqSpec.clid;
    else
        pDhcpcReqSpec->clid = NULL;
    if (errmsg[0] == 0) 
        {
        inet_ntoa_b (paramp->yiaddr, output);
        if (arpans != OK)
            sprintf (errmsg, "IP address (%s) is already in use.", output);
        else 
            sprintf (errmsg, "IP address (%s) doesn't match requested value.",
	                      output);
        }
    pDhcpcReqSpec->dhcp_errmsg = errmsg;

    return;
    }

/*******************************************************************************
*
* set_relinfo - initialize request specification for release message
*
* This routine assigns the fields in the request specifier used to construct
* messages to the appropriate values for a DHCP release message according to
* the parameters of the currently active lease.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void set_relinfo
    (
    struct dhcp_reqspec *pDhcpcReqSpec,
    LEASE_DATA * 	pLeaseData,
    char *errmsg
    )
    {
    char output [INET_ADDR_LEN];
    struct dhcp_param *paramp;

    paramp = pLeaseData->dhcpcParam;

    pDhcpcReqSpec->ipaddr.s_addr = paramp->yiaddr.s_addr;
    pDhcpcReqSpec->srvaddr.s_addr = paramp->server_id.s_addr;
    if (pLeaseData->leaseReqSpec.clid != NULL)
        pDhcpcReqSpec->clid = pLeaseData->leaseReqSpec.clid;
    else
        pDhcpcReqSpec->clid = NULL;
    if (pLeaseData->leaseReqSpec.dhcp_errmsg != NULL)
        pDhcpcReqSpec->dhcp_errmsg = pLeaseData->leaseReqSpec.dhcp_errmsg;
    else
        {
        inet_ntoa_b (paramp->yiaddr, output);
        sprintf (errmsg, "Releasing the current IP address (%s).", output);
        pDhcpcReqSpec->dhcp_errmsg = errmsg;
        }
    return;
    }

/*******************************************************************************
*
* make_discover - construct a DHCP discover message
*
* This routine constructs an outgoing UDP/IP message containing the values
* required to broadcast a lease request. The <xidFlag> indicates whether
* a transaction ID should be generated. Because multiple leases are supported,
* the contents of the transmit buffer are not guaranteed to remain unchanged.
* Therefore, each message must be rebuilt before it is sent. However, the
* transaction ID must not be changed after the initial transmission.
*
* RETURNS: Size of discover message, in bytes
*
* ERRNO: N/A
*
* NOMANUAL
*/

int make_discover 
    (
    LEASE_DATA * 	pLeaseData, 	/* lease-specific data structures */
    BOOL 		xidFlag 	/* generate a new transaction ID? */
    )
    {
    int offopt = 0; 	/* offset in options field */
    int msgsize; 	/* total size of DHCP message */
    u_long tmpul = 0;
    u_short tmpus = 0;
    struct dhcp_reqspec * 	pReqSpec;

    pReqSpec = &pLeaseData->leaseReqSpec;

    /* construct dhcp part */

    bzero (sbuf.buf, sbuf.size);
    dhcpcMsgOut.dhcp->op = BOOTREQUEST;
    dhcpcMsgOut.dhcp->htype = pLeaseData->ifData.haddr.htype;
    dhcpcMsgOut.dhcp->hlen = pLeaseData->ifData.haddr.hlen;

    if (xidFlag)
        {
        pLeaseData->xid = generate_xid (&pLeaseData->ifData);

        /* Update BPF filter to check for new transaction ID. */

        dhcpfilter [20].k = pLeaseData->xid;
        ioctl (pLeaseData->ifData.bpfDev, BIOCSETF, (u_int)&dhcpread);
        }

    dhcpcMsgOut.dhcp->xid = htonl (pLeaseData->xid);

    bcopy (pLeaseData->ifData.haddr.haddr, dhcpcMsgOut.dhcp->chaddr, 
           dhcpcMsgOut.dhcp->hlen);

    /* insert magic cookie */

    bcopy ( (char *)dhcpCookie, dhcpcMsgOut.dhcp->options, MAGIC_LEN);
    offopt = MAGIC_LEN;

    /* insert message type */

    dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_MSGTYPE_TAG;
    dhcpcMsgOut.dhcp->options [offopt++] = 1;
    dhcpcMsgOut.dhcp->options [offopt++] = DHCPDISCOVER;

    /* Insert requested IP address, if any. */

    if (pReqSpec->ipaddr.s_addr != 0)
        {
        dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_REQUEST_IPADDR_TAG;
        dhcpcMsgOut.dhcp->options [offopt++] = 4;
        bcopy ( (char *)&pReqSpec->ipaddr.s_addr,
                &dhcpcMsgOut.dhcp->options [offopt], 4);
        offopt += 4;
        }

    /* insert Maximum DHCP message size */

    dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_MAXMSGSIZE_TAG;
    dhcpcMsgOut.dhcp->options [offopt++] = 2;
    tmpus = htons (pReqSpec->maxlen);
    bcopy ( (char *)&tmpus, &dhcpcMsgOut.dhcp->options [offopt], 2);
    offopt += 2;

    /* Insert request list, if any. */

    if (pReqSpec->reqlist.len != 0)
        {
        dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_REQ_LIST_TAG;
        dhcpcMsgOut.dhcp->options [offopt++] = pReqSpec->reqlist.len;
        bcopy (pReqSpec->reqlist.list, &dhcpcMsgOut.dhcp->options [offopt],
               pReqSpec->reqlist.len);
        offopt += pReqSpec->reqlist.len;
        }

    /* Insert all other entries from custom options field, if any. */

    if (pReqSpec->pOptions != NULL)
        {
        bcopy (pReqSpec->pOptions, &dhcpcMsgOut.dhcp->options [offopt],
               pReqSpec->optlen);
        offopt += pReqSpec->optlen;
        }

    dhcpcMsgOut.dhcp->options[offopt] = _DHCP_END_TAG;

    msgsize = (DFLTDHCPLEN - DFLTOPTLEN) + offopt + 1;

    if (pLeaseData->oldFlag)
        {
        /*
         * This flag indicates that the client did not receive a response
         * to the initial set of discover messages. Older servers might
         * ignore messages less than the minimum length obtained with a
         * fixed options field, so pad the message to reach that length.
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
    tmpul = generate_xid (&pLeaseData->ifData);
    tmpul += (tmpul >> 16);
    /* dhcpcMsgOut.ip->ip_id = htons ( (u_short) (~tmpul));*/
    dhcpcMsgOut.ip->ip_id = (u_short) (~tmpul);
    dhcpcMsgOut.ip->ip_off = htons (IP_DF);                        /* XXX */
    dhcpcMsgOut.ip->ip_ttl = 0x20;                                 /* XXX */
    dhcpcMsgOut.ip->ip_p = IPPROTO_UDP;
    dhcpcMsgOut.ip->ip_src.s_addr = 0;
    dhcpcMsgOut.ip->ip_dst.s_addr = 0xffffffff;
    dhcpcMsgOut.ip->ip_sum = 0;

    msgsize += UDPHL + IPHL;

#if BSD<44
    /* tmpus = checksum ( (u_short *)snd.ip, (snd.ip->ip_v_hl & 0xf) << 2);
    snd.ip->ip_sum = htons (tmpus); */
  dhcpcMsgOut.ip->ip_sum = checksum ( (u_short *)dhcpcMsgOut.ip, 
                                      (dhcpcMsgOut.ip->ip_v_hl & 0xf) << 2);
#else
    /* tmpus = checksum ( (u_short *)snd.ip, snd.ip->ip_hl << 2);
    snd.ip->ip_sum = htons (tmpus); */
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
* required to request a lease. There is some variation in contents depending
* on whether the request is meant to establish a lease, renew a lease, or
* verify an existing lease. Also, when constructing a lease renewal message,
* the routine exits before any headers are are added so that the message may 
* be sent with send_unicast().
*
* Besides the five types of lease request messages, this routine constructs
* the DHCPINFORM messages added in RFC 2131 when the <type> argument is
* INFORMING. Those messages enable a host to obtain additional configuration
* parameters even though it has an externally configured address.
*
* .IP
* The <xidFlag> indicates whether a transaction ID should be generated. 
* Because multiple leases are supported, the contents of the transmit buffer 
* are not guaranteed to remain unchanged. Therefore, each message must be 
* rebuilt before it is sent. However, the transaction ID must not be changed 
* after the initial transmission.
*
* RETURNS: size of message if constructed succesfully, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int make_request
    (
    LEASE_DATA * 	pLeaseData, 	/* lease-specific data structures */
    int type,
    BOOL 		xidFlag 	/* generate a new transaction ID? */
    )
    {
    int offopt = 0;                   /* offset in options field */
    int msgsize; 	/* total size of DHCP message */
    int discoverSecs;   /* 'secs' field from DHCP discover message */
    u_long tmpul = 0;
    u_short tmpus = 0;
    struct dhcp_reqspec * 	pReqSpec = NULL;
    struct dhcp_param * 	paramp = NULL;

    pReqSpec = &pLeaseData->leaseReqSpec;
    paramp = pLeaseData->dhcpcParam;

    /* 
     * save value of 'secs' field from the previous DISCOVER message 
     * before dhcpcMsgOut is cleared. The 'secs' value is required for
     * the outgoing REQUEST message (RFC 2131).
     */

    discoverSecs = dhcpcMsgOut.dhcp->secs;

    /* construct dhcp part */

    bzero (sbuf.buf, sbuf.size);
    dhcpcMsgOut.dhcp->op = BOOTREQUEST;
    dhcpcMsgOut.dhcp->htype = pLeaseData->ifData.haddr.htype;
    dhcpcMsgOut.dhcp->hlen = pLeaseData->ifData.haddr.hlen;

    if (xidFlag)
        {
        pLeaseData->xid = generate_xid (&pLeaseData->ifData);

        /* Update BPF filter to check for new transaction ID. */

        dhcpfilter [20].k = pLeaseData->xid;
        ioctl (pLeaseData->ifData.bpfDev, BIOCSETF, (u_int)&dhcpread);
        }

    dhcpcMsgOut.dhcp->xid = htonl (pLeaseData->xid);

    /* set it to 'secs' value from DISCOVER message */

    dhcpcMsgOut.dhcp->secs = discoverSecs;  

    if (type == INFORMING)
        {
        /* Set externally assigned IP address. */

        dhcpcMsgOut.dhcp->ciaddr.s_addr = pReqSpec->ipaddr.s_addr;
        }
    else if (type == REQUESTING || type == REBOOTING)
        dhcpcMsgOut.dhcp->ciaddr.s_addr = 0;
    else
        dhcpcMsgOut.dhcp->ciaddr.s_addr = paramp->yiaddr.s_addr;

    bcopy (pLeaseData->ifData.haddr.haddr, dhcpcMsgOut.dhcp->chaddr, 
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

    if (type == REQUESTING || type == REBOOTING) 
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

    /* Insert server identifier when required. Omit when forbidden. */

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
    tmpus = htons (pReqSpec->maxlen);
    bcopy ( (char *)&tmpus, &dhcpcMsgOut.dhcp->options [offopt], 2);
    offopt += 2;

    /* Insert request list, if any. */

    if (pReqSpec->reqlist.len != 0)
        {
        dhcpcMsgOut.dhcp->options [offopt++] = _DHCP_REQ_LIST_TAG;
        dhcpcMsgOut.dhcp->options [offopt++] = pReqSpec->reqlist.len;
        bcopy (pReqSpec->reqlist.list, &dhcpcMsgOut.dhcp->options [offopt],
               pReqSpec->reqlist.len);
        offopt += pReqSpec->reqlist.len;
        }

    /* Insert all other entries from custom options field, if any. */

    if (pReqSpec->pOptions != NULL)
        {
        bcopy (pReqSpec->pOptions, &dhcpcMsgOut.dhcp->options [offopt],
               pReqSpec->optlen);
        offopt += pReqSpec->optlen;
        }

    dhcpcMsgOut.dhcp->options[offopt] = _DHCP_END_TAG;

    msgsize = (DFLTDHCPLEN - DFLTOPTLEN) + offopt + 1;

    if (pLeaseData->oldFlag)
        {
        /*
         * This flag indicates that the client did not receive a response
         * to the initial set of discover messages but did receive one
         * using the older message format. The (older) responding server
         * ignores messages less than the minimum length obtained with a
         * fixed options field, so pad the message to reach that length.
         */

        if (msgsize < DFLTDHCPLEN)
            msgsize = DFLTDHCPLEN;
        }

    if (type == RENEWING)    /* RENEWING is unicast with the normal socket */
        return (msgsize);

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
    tmpul = generate_xid (&pLeaseData->ifData);
    tmpul += (tmpul >> 16);
    dhcpcMsgOut.ip->ip_id = (u_short) (~tmpul);
    dhcpcMsgOut.ip->ip_off = htons (IP_DF);                        /* XXX */
    dhcpcMsgOut.ip->ip_ttl = 0x20;                                 /* XXX */
    dhcpcMsgOut.ip->ip_p = IPPROTO_UDP;

    msgsize += UDPHL + IPHL;

    switch (type) 
        {
        case REQUESTING:     /* fall-through */
        case REBOOTING:      /* fall-through */
        case VERIFYING:
            dhcpcMsgOut.ip->ip_src.s_addr = spudph.srcip.s_addr = 0;
            dhcpcMsgOut.ip->ip_dst.s_addr = spudph.dstip.s_addr = 0xffffffff;
            break;
        case REBINDING:
            dhcpcMsgOut.ip->ip_src.s_addr = spudph.srcip.s_addr = 
                                            paramp->yiaddr.s_addr;
            dhcpcMsgOut.ip->ip_dst.s_addr = spudph.dstip.s_addr = 0xffffffff;
            break;
        case INFORMING:
            dhcpcMsgOut.ip->ip_src.s_addr = spudph.srcip.s_addr = 
                                            pReqSpec->ipaddr.s_addr;
            dhcpcMsgOut.ip->ip_dst.s_addr = spudph.dstip.s_addr = 0xffffffff;
            break; 
        }
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
* send_unicast - send a DHCP message via unicast
*
* This routine sends a previously constructed DHCP message to the DHCP 
* server port on the specified IP address. It is used to send a request 
* for lease renewal or a DHCP release message to the appropriate DHCP server.
*
* RETURNS: 0 if message sent, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int send_unicast
    (
    struct in_addr *dstip,
    struct dhcp *sdhcp,
    int length 	/* length of DHCP message */
    )
    {
    struct sockaddr_in dst;
    struct msghdr msg;
    struct iovec bufvec[1];
    int bufsize = length;
    int status;

    status = setsockopt (dhcpcDataSock, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize,
		         sizeof (bufsize));
    if (status < 0) 
        {
        printf ("setsockopt error %d (%x).\n", status, errno);
        return (-1);
        }

    bzero ( (char *)&dst, sizeof (dst));
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = dstip->s_addr;
    dst.sin_port = dhcps_port;

    bufvec[0].iov_base = (char *)sdhcp;
    bufvec[0].iov_len = bufsize;

    bzero ( (char *)&msg, sizeof (msg));
    msg.msg_name = (caddr_t)&dst;
    msg.msg_namelen = sizeof (dst);
    msg.msg_iov = bufvec;
    msg.msg_iovlen = 1;
    status = sendmsg (dhcpcDataSock, &msg, 0);
    if (status < 0)
        {
        printf ("sendmsg error %d (%x).\n", status, errno);
        return (-1);
        }

    return(0);
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

int handle_ip
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

int handle_num
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

int handle_ips
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

int handle_str
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

int handle_bool
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

int handle_ippairs
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

int handle_nums
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

int handle_list
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

/* dhcps.c - WIDE project DHCP server routines */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01n,23apr02,wap  use dhcpTime() instead of time() (SPR #68900), also use
                 BPF_WORDALIGN() when traversing multiple packets in BPF
                 buffers (SPR #74215)
01m,08mar02,wap  Return sane lease renewal times for clients that request
                 leases without specifying their own lease times (SPR #73243)
01l,31oct01,vvv  allow low-priority tasks to run while server waits for 
		 ICMP reply (SPR #33132)
01k,12oct01,rae  merge from truestack ver 01s, base 01h
                 SPRs 70184, 69547, 34799, 66808
01j,24oct00,spm  fixed modification history after tor3_x merge; fixed invalid
                 socket reference (SPR #27246)
01i,23oct00,niq  merged from version 01j of tor3_x branch (base version 01h)
01h,01mar99,spm  corrected checksum calculation for ICMP requests (SPR #24745)
01g,06oct97,spm  removed reference to deleted endDriver global; replaced with
                 support for dynamic driver type detection; split interface
                 name into device name and unit number
01f,26aug97,spm  rearranged functions to consolidate lease selection routines
01e,02jun97,spm  changed DHCP option tags to prevent name conflicts (SPR #8667)
                 and updated man pages
01d,06may97,spm  changed memory access to align IP header on four byte 
                 boundary and corrected format of lease record
01c,28apr97,spm  allowed user to change DHCP_MAX_HOPS setting
01b,18apr97,spm  added conditional include DHCPS_DEBUG for displayed output
01a,07apr97,spm  created by modifying WIDE project DHCP implementation
*/

/*
DESCRIPTION
This library implements the server side of the Dynamic Host Configuration
Protocol (DHCP). DHCP is an extension of BOOTP. Like BOOTP, it allows a
target to configure itself dynamically by obtaining its IP address, a
boot file name, and the DHCP server's address over the network. Additionally,
DHCP provides for automatic reuse of network addresses by specifying
individual leases as well as many additional options. The compatible
message format allows DHCP participants to interoperate with BOOTP
participants.

INCLUDE FILES: dhcpsLib.h

SEE ALSO: RFC 1541, RFC 1533
*/

/* includes */

#include "dhcp/copyright_dhcp.h"
#include "vxWorks.h"
#include "net/bpf.h"
#include "logLib.h"
#include "vxLib.h" 	/* checksum() declaration */
#include "inetLib.h"
#include "sockLib.h"
#include "ioLib.h"
#include "wdLib.h"
#include "taskLib.h"
#include "sysLib.h"
#include "muxLib.h"

#include "netinet/ip.h"
#include "netinet/in_systm.h"
#include "netinet/ip_icmp.h" 

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "dhcp/dhcp.h"
#include "dhcp/common.h"
#include "dhcp/hash.h"
#include "dhcp/dhcps.h"
#include "dhcpsLib.h"
#include "dhcp/common_subr.h"

/* defines */

#define MEMORIZE 90 	/* Seconds of delay before re-using offered lease. */
#define E_NOMORE -2 	/* Error code: no more space in options field. */

#ifndef VIRTUAL_STACK
/* globals */

int nbind; 		/* Number of active or pending lease records. */
struct msg dhcpsMsgIn;	/* Pointers to components of incoming message. */
struct msg dhcpsMsgOut;	/* Pointers to outgoing message parts. */
char *dhcpsSendBuf;     /* Transmit buffer for outgoing messages. */
char *dhcpsOverflowBuf; /* Extra space (for larger messages) starts here. */

IMPORT struct if_info *dhcpsIntfaceList; 	/* Interfaces to monitor. */
struct iovec sbufvec[2]; 	/* Socket access to outgoing message. 
                                 * sbufvec[0] is standard message. 
                                 * sbufvec[1] contains message extensions if
                                 * client accepts longer messages. 
                                 */

IMPORT int dhcpsMaxSize; /* Size of transmit buffer. */
IMPORT int dhcpsBufSize; /* Size of receive buffer. */

IMPORT u_short dhcps_port;
IMPORT u_short dhcpc_port;

/* locals */

LOCAL unsigned char dhcpCookie[] = RFC1048_MAGIC; /* DHCP message indicator. */
LOCAL int rdhcplen;		/* Size of received DHCP message. */
LOCAL int overload; 		/* Options in sname or file fields? */
LOCAL int off_options; 		/* Index into options field. */
LOCAL int off_extopt; 		/* Index into any options in sbufvec[1]. */
LOCAL int maxoptlen;		/* Space available for options. */
LOCAL int off_file; 		/* Index into any options within file field. */
LOCAL int off_sname; 		/* Index into any options in sname field. */

#else
#include "netinet/vsLib.h"
#include "netinet/vsDhcps.h"
#endif /* VIRTUAL_STACK */

/* forward declarations */

LOCAL int icmp_check (int, struct in_addr *);

IMPORT STATUS dhcpsSend (struct ifnet *, char *, int,
                         struct sockaddr_in *, char *, int, BOOL);
IMPORT void dhcpServerRelay (struct if_info *);
IMPORT void dhcpClientRelay (struct if_info *, int, char *);
IMPORT void delarp (struct in_addr *, int);

/*******************************************************************************
*
* dhcpsStart - execute the DHCP server
*
* This routine receives DHCP requests from clients and generates replies. If
* configured to do so, it may also act as a relay agent and forward these 
* requests to other servers or transfer replies to clients. It is the entry 
* point for the DHCP server task and should only be called internally.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/
int dhcpsStart
    (
#ifndef VIRTUAL_STACK
    void
#else
    int stackNum
#endif /* VIRTUAL_STACK */
    )
    {
    struct if_info *ifp; 	/* pointer to receiving interface descriptor */
    int n = 0; 			/* number of bytes received */
    int msgtype; 		/* DHCP message type, or BOOTP indicator */
    char *option = NULL; 	/* pointer to access message options */
    struct bpf_hdr *    pMsgHdr;
    char *              pMsgData;
    int                 msglen;
    int                 curlen;
    int                 totlen;         /* Amount of data in BPF buffer. */

#ifdef VIRTUAL_STACK
    if (virtualStackNumTaskIdSet (stackNum) == ERROR)
        return (ERROR);
#endif /* VIRTUAL_STACK */

    /* Main loop - read and process incoming messages. */

    FOREVER 
        { 
        garbage_collect ();      /* remove expired leases from hash table. */

        /* select and read from interfaces */

        ifp = read_interfaces (dhcpsIntfaceList, &n, dhcpsBufSize);
        if (ifp == NULL)
            continue;

        /* Divide each DHCP message in buffer into protocol sections. */

        msglen = curlen = 0;
        totlen = n;
        pMsgHdr = (struct bpf_hdr *)ifp->buf;
        pMsgData = ifp->buf;
        while (curlen < totlen)
            {
            msglen = BPF_WORDALIGN(pMsgHdr->bh_hdrlen + pMsgHdr->bh_caplen);
            curlen += msglen;

            /* Set the IP pointer to skip the BPF and link level headers. */

            dhcpsMsgIn.ip = (struct ip *) (pMsgData + pMsgHdr->bh_hdrlen +
                                           pMsgHdr->bh_linklen);

            if ( (dhcpsMsgIn.ip->ip_dst.s_addr == 0xffffffff ||
                   dhcpsMsgIn.ip->ip_dst.s_addr == ifp->ipaddr.s_addr) &&
                  check_ipsum (dhcpsMsgIn.ip))
                dhcpsMsgIn.udp =
                (struct udphdr *)( (UCHAR *)dhcpsMsgIn.ip +
                                              dhcpsMsgIn.ip->ip_hl * WORD);
            else
                {
                /* Invalid IP header: ignore. */

                pMsgData = pMsgData + msglen;
                pMsgHdr = (struct bpf_hdr *)pMsgData;
                continue;
                }

            if (check_udpsum (dhcpsMsgIn.ip, dhcpsMsgIn.udp))
                dhcpsMsgIn.dhcp =
                (struct dhcp *)( (UCHAR *)dhcpsMsgIn.udp + UDPHL);
            else
                {
                /* Invalid UDP header: ignore. */

                pMsgData = pMsgData + msglen;
                pMsgHdr = (struct bpf_hdr *)pMsgData;
                continue;
                }

            /*
             * Perform function of relay agent for received server replies.
             * (Changes contents of input message IP and UDP headers while
             * sending to client).
             */

            dhcpMsgIn.ip = dhcpsMsgIn.ip;
            dhcpMsgIn.udp = dhcpsMsgIn.udp;
            dhcpMsgIn.dhcp = dhcpsMsgIn.dhcp;
            if (dhcpMsgIn.dhcp->op == BOOTREPLY)
                dhcpClientRelay (dhcpsIntfaceList, DHCPLEN (dhcpsMsgIn.udp),
                                 dhcpsSendBuf);

            if (dhcpsMsgIn.dhcp->op != BOOTREQUEST)
                {
                pMsgData = pMsgData + msglen;
                pMsgHdr = (struct bpf_hdr *)pMsgData;
                continue;
                }

            /* Process DHCP client message received at server port. */

            rdhcplen = DHCPLEN (dhcpsMsgIn.udp);

            msgtype = BOOTP;
            option = pickup_opt (dhcpsMsgIn.dhcp, rdhcplen, _DHCP_MSGTYPE_TAG);
            if (option != NULL)
                msgtype = (int) *OPTBODY(option);

#ifdef DHCPS_DEBUG
            if (msgtype < BOOTP || msgtype > DHCPRELEASE)
                logMsg ("dhcps: unknown message received.\n",
                         0, 0, 0, 0, 0, 0);
#endif
            if (msgtype >= BOOTP && msgtype <= DHCPRELEASE)
                if ( (process_msg [msgtype]) != NULL)
                    (*process_msg [msgtype]) (ifp);

            /*
             * Relay received client messages if target addresses available.
             * (Changes contents of input DHCP message when sending to
             * server or relay agent - must process first).
             */
            dhcpMsgIn.udp = dhcpsMsgIn.udp;
            dhcpMsgIn.dhcp = dhcpsMsgIn.dhcp;
            if (pDhcpTargetList != NULL)
                dhcpServerRelay (ifp);

            pMsgData = pMsgData + msglen;
            pMsgHdr = (struct bpf_hdr *)pMsgData;
            }
        }
    }

/*******************************************************************************
*
* haddrtos - convert hardware address to cache format
*
* This routine converts the given hardware address to the <type>:<value> pair
* used when adding lease record entries to permanent storage.
*
* RETURNS: Pointer to converted string
*
* ERRNO: N/A
*
* NOMANUAL
*/

char * haddrtos
    (
    struct chaddr *haddr 	/* pointer to parsed hardware address */
    )
    {
    int i;
    int fin;
    char tmp[3];
    static char result [MAX_HLEN * 2 + 8];     /* it seems enough */

    bzero (result, sizeof (result));
    fin = haddr->hlen;
    if (fin > MAX_HLEN)
        fin = MAX_HLEN;
    sprintf (result, "%d:0x", haddr->htype);
    for (i = 0; i < fin; i++) 
        {
        sprintf(tmp, "%.2x", haddr->haddr[i] & 0xff);
        strcat(result, tmp);
        }

    return (result);
    }

/*******************************************************************************
*
* cidtos - convert client identifier to cache format
*
* This routine converts the given client identifier to the <type>:<value> pair
* used when adding lease record entries to permanent storage. It also
* adds the current subnet number so that the server can deny inaccurate 
* leases caused by a client changing subnets. When used for user output, the
* subnet is not included in the converted string.
*
* RETURNS: Pointer to converted string
*
* ERRNO: N/A
*
* NOMANUAL
*/

char * cidtos
    (
    struct client_id *cid,	/* pointer to parsed client ID */
    int withsubnet 		/* flag for adding subnet to output string */
    )
    {
    int i = 0;
    static char result [MAXOPT * 2 + sizeof ("255.255.255.255") + 3];
    char tmp [sizeof ("255.255.255.255") + 1];

    sprintf (result, "%d:0x", cid->idtype);
    for (i = 0; i < cid->idlen; i++) 
        {
        sprintf (tmp, "%.2x", cid->id[i] & 0xff);
        strcat (result, tmp);
        }
    if (withsubnet) 
        {
        inet_ntoa_b (cid->subnet, tmp);
        strcat (result, ":");
        strcat (result, tmp);
        }

    return (result);
    }

/*******************************************************************************
*
* get_reqlease - Retrieve requested lease value 
*
* This routine extracts the desired lease duration from the options field of 
* the DHCP client request and converts it to host byte order.
*
* RETURNS: Requested lease duration, or 0 if none.
*
* ERRNO: N/A
*
* NOMANUAL
*/
  
static u_long get_reqlease
    (
    struct dhcp *msg, 		/* pointer to incoming message */
    int length 			/* length of incoming message */
    )
    {
    char *option = NULL;
    u_long retval = 0;

    if ( (option = pickup_opt (msg, length, _DHCP_LEASE_TIME_TAG)) != NULL) 
        retval = GETHL (OPTBODY (option));

    return (retval);
    }

/*******************************************************************************
*
* get_cid - Retrieve client identifier
*
* This routine extracts the client identifier from the options field of the
* DHCP client request and stores the <type>:<value> pair in the given 
* structure. If no explicit client ID is given, the routine uses the hardware
* address, as specified by RFC 1541.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

static void get_cid
    (
    struct dhcp *msg, 		/* pointer to incoming message */
    int length, 		/* length of incoming message */
    struct client_id *cid 	/* pointer to storage for parsed data */
    )
    {
    char *option = NULL;

    option = pickup_opt (msg, length, _DHCP_CLIENT_ID_TAG);
    if (option != NULL) 
        {
        cid->idlen = ( (int)DHCPOPTLEN (option)) - 1; /* -1 for ID type */
        bcopy (OPTBODY (option) + 1, cid->id, cid->idlen);
        cid->idtype = *OPTBODY (option);
        } 
    else 
        { 
        /* haddr is used to substitute for client identifier */
        cid->idlen = msg->hlen;
        cid->idtype = msg->htype;
        bcopy (msg->chaddr, cid->id, msg->hlen);
        }

    return;
    }

/*******************************************************************************
*
* get_maxoptlen - Calculate size of options field
*
* This routine determines the number of bytes available for DHCP options
* without overloading. For standard length messages of 548 bytes, it returns 
* the default length of 312 bytes. For longer messages, it returns 312 bytes 
* plus the excess bytes (beyond 548), unless the DHCP message length exceeds
* the MTU size. In that case, it returns the number of bytes in the MTU not
* needed for the IP header, UDP header, and the fixed-length portion of the
* DHCP messages, which require 264 bytes total.
*
* RETURNS: Size of variable-length options field
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int get_maxoptlen
    (
    struct dhcp *msg, 		/* pointer to incoming message */
    int length 			/* length of incoming message */
    )
    {
    char *option = NULL;
    int retval = DFLTOPTLEN;

    /* Calculate length of options field from maximum message size. */

    if ( (option = pickup_opt (msg, length, _DHCP_MAXMSGSIZE_TAG)) != NULL)
        retval = GETHS (OPTBODY (option)) - IPHL - UDPHL - DFLTDHCPLEN 
                        + DFLTOPTLEN;


    /* 
     * If requested maximum size exceeds largest supported value, return
     * value equal to portion of buffer not required for message headers
     * or fixed-size portion of DHCP message. 
     */
    if (retval - DFLTOPTLEN + DFLTDHCPLEN + UDPHL + IPHL > dhcpsMaxSize)
        retval = dhcpsMaxSize - IPHL - UDPHL - DFLTDHCPLEN + DFLTOPTLEN;

    return (retval);
    }

/*******************************************************************************
*
* get_subnet - Retrieve subnet number
*
* This routine determines the subnet number of the requesting client and
* stores it in the given structure. This value is determined using the
* subnet mask specified by the client, if present. Otherwise, it is formed
* from the last known subnet mask, if available, or the subnet mask of the
* receiving interface. The server will only issue leases for IP addresses 
* with the same subnet number as the requesting client.
*
* RETURNS: 0 if subnet number determined, or -1 otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int get_subnet
    (
    struct dhcp *msg, 		/* pointer to incoming message */
    int length, 		/* length of incoming message */
    struct in_addr *subn, 	/* pointer to storage for parsed data */
    struct if_info *ifp 	/* pointer to receiving interface descriptor */
    )
    {
    char *option = NULL;
    struct relay_acl *acl = NULL;
    struct dhcp_resource *res = NULL;
#ifdef DHCPS_DEBUG
    char output [INET_ADDR_LEN];
#endif

    if (msg->ciaddr.s_addr != 0)
        {
        if ( (option = pickup_opt (msg, length, _DHCP_SUBNET_MASK_TAG))
              != NULL) 
            {
            subn->s_addr = 
                         msg->ciaddr.s_addr & htonl (GETHL (OPTBODY (option)));
            return (0);
            }
        else 
            {
            res = (struct dhcp_resource *)
	          hash_find (&iphashtable, (char *)&msg->ciaddr.s_addr,
			     sizeof (u_long), resipcmp, &msg->ciaddr);
#ifdef DHCPS_DEBUG
            if (res == NULL)
                logMsg ("get_subnet can't find IP address in hash table.\n",
                         0, 0, 0, 0, 0, 0);
#endif
            if (res != NULL) 
                {
                subn->s_addr = msg->ciaddr.s_addr & res->subnet_mask.s_addr;
                return (0);
                }
            }
        }

    if (msg->giaddr.s_addr != 0) 
        {
        if ( (option = pickup_opt (msg, length, _DHCP_SUBNET_MASK_TAG)) 
              != NULL)
            {
            subn->s_addr = 
                         msg->giaddr.s_addr & htonl (GETHL (OPTBODY (option)));
            return (0);
            }
        else if ( (acl = (struct relay_acl *)
                 hash_find (&relayhashtable, (char *)&msg->giaddr,
                            sizeof (struct in_addr), relayipcmp,
                            &msg->giaddr)) == NULL) 
            {
#ifdef DHCPS_DEBUG
            inet_ntoa_b (msg->giaddr, output);          
            logMsg ("DHCP message sent from invalid relay agent(%s).\n", 
                     output, 0, 0, 0, 0, 0);
#endif
            return (-1);
            } 
        else 
            {
            subn->s_addr = (acl->relay.s_addr & acl->subnet_mask.s_addr);
            return (0);
            }
        }

    /* Client doesn't have IP address - form from received interface. */

    subn->s_addr = ifp->ipaddr.s_addr & ifp->subnetmask.s_addr;
    return (0);
    }

/*******************************************************************************
*
* get_snmk - Retrieve subnet mask
*
* This routine determines the subnet mask for the requesting client and
* stores it in the given structure. This value is determined from the 
* value specified for the relay agent, if the message was forwarded, or
* the subnet mask of the receiving interface. 
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int get_snmk
    (
    struct dhcp *msg, 		/* pointer to incoming message */
    int length, 		/* length of incoming message */
    struct in_addr *subn, 	/* pointer to storage for parsed data */
    struct if_info *ifp 	/* pointer to interface descriptor */
    )
    {
    struct relay_acl *acl = NULL;
#ifdef DHCPS_DEBUG
    char output [INET_ADDR_LEN];
#endif

    if (msg->giaddr.s_addr != 0) 
        {
        acl = (struct relay_acl *)hash_find(&relayhashtable, 
                                            (char *) &msg->giaddr,
		                            sizeof (struct in_addr),
                                            relayipcmp, &msg->giaddr);
        if (acl == NULL) 
            {
#ifdef DHCPS_DEBUG
            inet_ntoa_b (msg->giaddr, output);
            logMsg ("packet received from invalid relay agent(%s).\n", output, 
                     0, 0, 0, 0, 0);
#endif
            return (-1);
            } 
        else 
            {
            subn->s_addr = acl->subnet_mask.s_addr;
            return (0);
            }
        }
    subn->s_addr = ifp->subnetmask.s_addr;
    return (0);
    }

/*******************************************************************************
*
* available_res - Check resource availability
*
* This routine determines if the resource selected by the server may
* be offered to the client. The resource is available if it has not been
* assigned or offered to any client (res->binding == NULL), or the lease has 
* expired (expire_epoch < curr_epoch), or an outstanding offer was not 
* acknowledged within the time allotted (temp_epoch < curr_epoch). The
* resource is also available if it was  manually assigned to the given 
* client (binding->cid matches given client ID).
*
* RETURNS: TRUE if resource available, or FALSE otherwise. 
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int available_res 
    (
    struct dhcp_resource *res, 		/* pointer to lease descriptor */
    struct client_id *cid, 		/* pointer to client ID */
    time_t curr_epoch 			/* current time, in seconds */ 
    )
    {
    return (res->binding == NULL ||
	    (res->binding->expire_epoch != 0xffffffff &&
	     res->binding->expire_epoch < curr_epoch &&
	     res->binding->temp_epoch < curr_epoch) ||
	    cidcmp (&res->binding->cid, cid));
    }

/*******************************************************************************
*
* cidcopy - copy client identifier
*
* This routine copies the <type>:<value> client identifier pair from the
* source structure to the destination.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int cidcopy
    (
    struct client_id *src, 	/* source client identifier */
    struct client_id *dst 	/* destiniation client identifier */
    )
    {
    dst->subnet.s_addr = src->subnet.s_addr;
    dst->idtype = src->idtype;
    dst->idlen = src->idlen;

    bzero (dst->id, src->idlen);
    bcopy (src->id, dst->id, src->idlen);

    return (0);
    }

/*******************************************************************************
*
* choose_lease - determine lease duration
*
* This routine selects the lease duration for the offer to the client. If
* the resource is client-specific, the lease duration is infinite. Otherwise,
* the server provides the duration requested by the client, if available, or 
* the maximum available lease, whichever is less. If the client does not 
* request a lease duration, and has no active lease, the default lease value 
* is returned. For lease renewals or rebinding attempts, or if the lease
* has expired, the default lease value is also returned.
*
* RETURNS: Selected lease duration.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int choose_lease
    (
    int reqlease, 			/* requested lease duration (sec) */
    time_t curr_epoch, 			/* current time, in seconds */
    struct dhcp_resource *offer_res 	/* pointer to lease descriptor */
    )
    {
    u_long offer_lease = 0;

    /* Manual allocation - give an infinite lease to client. */

    if (ISSET (offer_res->valid, S_CLIENT_ID)) 
        offer_lease = 0xffffffff;

    /* Give requested lease, or maximum lease if request exceeds that value. */

    else if (reqlease != 0) 
        {
        if (reqlease <= offer_res->max_lease)
            offer_lease = reqlease;
        else
            offer_lease = offer_res->max_lease;
        }
    /* Initial request - give default lease. */

    else if (offer_res->binding == NULL) 
        offer_lease = offer_res->default_lease;

    /* Lease renewal or rebinding. */
    else 
        {
        /* "Renew" infinite lease. */

        if (offer_res->binding->expire_epoch == 0xffffffff) 
            offer_lease = 0xffffffff;

        /*
         * Lease expired (or being renewed) - give new lease
         * of default duration.
         */

        else 
            offer_lease = offer_res->default_lease;
        }

    return (offer_lease);
    }

/*******************************************************************************
*
* select_wcid - retrieve manually allocated leases
*
* This routine retrieves the dhcp_resource structure which holds the parameters
* from the address pool database specifically allocated to the client with
* the given client identifier, if any. These lease types have the highest
* priority when the server is selecting a lease for a client in response to
* a DHCP discover message.
*
* RETURNS: Manually allocated resource, or NULL if none or not available.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static struct dhcp_resource * select_wcid
    (
    int msgtype, 		/* DHCP message type */
    struct client_id *cid, 	/* pointer to client ID */
    time_t curr_epoch 		/* current time, in seconds */
    )
    {
    struct dhcp_binding *binding = NULL;

    binding = (struct dhcp_binding *)hash_find (&cidhashtable, cid->id, 
                                                cid->idlen, bindcidcmp, cid);
    if (binding != NULL) 
        {
        /*
         * Is the resource used ?
         */
        if (available_res (binding->res, cid, curr_epoch)) 
            {
            if (icmp_check (msgtype, &binding->res->ip_addr) == GOOD)
                {
	        return (binding->res);
                } 
            else 
                { 
                turnoff_bind (binding); 
                return (NULL); 
                } 
            } 
        } 
    return (NULL); 
    }

/*******************************************************************************
*
* select_wreqip - retrieve resource matching IP address
*
* This routine retrieves the dhcp_resource structure which provides the IP
* address requested by the client. The requested IP address must be on the
* same subnet as the requesting client. The client must also provide a
* matching client identifier, if included in the server's database entry.
*
* RETURNS: Matching resource, or NULL if none or not available.
*
* ERRNO: N/A
*
* NOMANUAL
*/
 
static struct dhcp_resource * select_wreqip 
    ( 
    int msgtype, 		/* DHCP message type */
    struct client_id *cid, 	/* pointer to client ID */
    time_t curr_epoch 		/* current time, in seconds */
    ) 
    { 
    char *option = NULL; 	/* pointer to access options field */
    char tmp [INET_ADDR_LEN]; 	/* temp IP address storage */
    struct dhcp_resource *res = NULL; 	/* access to lease descriptor data */
    struct in_addr reqip; 		/* value of desired IP address */

    bzero (tmp, sizeof (tmp));
    bzero ( (char *)&reqip, sizeof (reqip));

    option = pickup_opt (dhcpsMsgIn.dhcp, rdhcplen, _DHCP_REQUEST_IPADDR_TAG);
    if (option != NULL) 
        {
        reqip.s_addr = htonl (GETHL (OPTBODY (option)));
        res = (struct dhcp_resource *)hash_find (&iphashtable, 
                                                 (char *)&reqip.s_addr,
			                         sizeof (u_long), resipcmp, 
                                                 &reqip);
        if (res == NULL) 
            {
#ifdef DHCPS_DEBUG
            inet_ntoa_b (reqip, tmp);
            logMsg ("IP address %s is not in address pool", tmp, 
                     0, 0, 0, 0, 0);
#endif
            return (NULL);
            } 
        else 
            {   
            /* check the subnet number */

            if (cid->subnet.s_addr != 
                (res->ip_addr.s_addr & res->subnet_mask.s_addr)) 
                {
#ifdef DHCPS_DEBUG
                inet_ntoa_b (reqip, tmp);
	        logMsg (
          "DHCP%s(cid:\"%s\"): subnet mismatch for requested IP address %s.\n",
	       (int)((msgtype == DHCPDISCOVER) ? "DISCOVER" : "REQUEST"),
	                (int)cidtos (cid, 1), (int)tmp, 0, 0, 0);
#endif
	        return (NULL);
                }

            /* is it manual allocation ? */

            if (ISSET (res->valid, S_CLIENT_ID)) 
                {

	        /* is there corresponding binding ? */

	        if (res->binding == NULL) 
                    {
#ifdef DHCPS_DEBUG
                    inet_ntoa_b (reqip, tmp);
	            logMsg (
                         "DHCP%s(cid:\"%s\"): No corresponding binding for %s",
                    (int)((msgtype == DHCPDISCOVER) ? "DISCOVER" : "REQUEST"),
                            (int)cidtos (cid, 1), (int)tmp, 0, 0, 0);
#endif
                     return (NULL);
                    }

	        /* Check for matching client identifiers. */

	        if (cidcmp (&res->binding->cid, cid) != TRUE) 
                    {
#ifdef DHCPS_DEBUG
                    inet_ntoa_b (reqip, tmp);
	            logMsg (
        "DHCP%s(cid:\"%s\"): Client ID mismatch for requested IP address %s.\n",
                     (int)((msgtype == DHCPDISCOVER) ? "DISCOVER" : "REQUEST"),
                           (int)cidtos (cid, 1), (int)tmp, 0, 0, 0);
#endif
                    return (NULL);
	            }

	        /* Check if the address is already in use. */

	        if (icmp_check (msgtype, &res->ip_addr) == GOOD) 
	            return (res);
                else 
                    {
	            turnoff_bind (res->binding);
	            return (NULL);
	            }
                }

            /* If not manual allocation, is the requested lease available? */

            else if (available_res (res, cid, curr_epoch)) 
               {
	       if (icmp_check (msgtype, &res->ip_addr) == GOOD)
	           return (res);
	       else
                   { 
	           turnoff_bind (res->binding);
	           return (NULL);
	           }
               }
            }
        }
    return (NULL);    /* No requested IP address present in option list. */
    }

/*******************************************************************************
*
* select_newone - choose an entry from the available resources
*
* This routine retrieves a dhcp_resource structure for a client if no entry
* was found based on the requested IP address, if any, or the client 
* identifier. No addresses which are in use or on a different subnet than 
* the client are considered. For the remaining entries, it uses the following 
* criteria:
*
*      First, select entries which were never used in preference to used ones.
*
*      If not found, select the least recently used entry.
*
*      Next, give preference to entries which are unavailable to BOOTP clients.
*
*      Among unused or equally old entries, select the smallest possible 
*      maximum which exceeds the requested lease length. If not found, 
*      choose the resource with the largest maximum lease length.
*
*      The previous condition excludes resources with an infinite maximum
*      lease whenever possible, unless specifically requested by the client.
*
*      If not found (i.e. - all entries have same maximum lease length), 
*      select the first available entry in the database.
*
* RETURNS: Matching resource, or NULL if none or not available.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static struct dhcp_resource * select_newone
    (
    int msgtype, 		/* DHCP message type */
    struct client_id *cid, 	/* pointer to client ID */
    time_t curr_epoch, 		/* current time, in seconds */
    u_long reqlease 		/* requested lease duration (seconds) */
    )
    {
    struct dhcp_resource *res = NULL;
    struct dhcp_resource *best = NULL;
    struct hash_member *resptr = NULL;

    /* Examine each resource in list constructed from database. */

    resptr = reslist;
    while (resptr != NULL) 
       {
       res = (struct dhcp_resource *)resptr->data;

       if (res->ip_addr.s_addr == 0)      /* Skip dummy entries. */
           {
           resptr = resptr->next;
           continue;
           }

       /*
        * Check the resource subnet and availability. Skip entries which 
        * are not available or would change the subnet of the client.
        */

       if (cid->subnet.s_addr ==
             (res->ip_addr.s_addr & res->subnet_mask.s_addr) &&
	     available_res (res, cid, curr_epoch)) 
           {
           /*
            * choose the best entry from available resources on the same 
            * subnet. The criteria are listed in inverse priority order.
            */

           /* Lowest priority - take first available entry. */

           if (best == NULL) 
               {
	       if (icmp_check (msgtype, &res->ip_addr) == GOOD)
	           best = res;
               else
	           turnoff_bind (res->binding);
	       resptr = resptr->next;
	       continue;
               }

           /* Select unused entries in preference to used ones. */

           if (best->binding == NULL && res->binding != NULL) 
               {
               resptr = resptr->next;
               continue;
               }
           else if (best->binding != NULL && res->binding == NULL) 
               {
               if (icmp_check (msgtype, &res->ip_addr) == GOOD)
                   best = res;
               else
	           turnoff_bind (res->binding);
               resptr = resptr->next;
               continue;
               }
 
           /* Give preference to entries not available to BOOTP clients. */

           if (best->allow_bootp == FALSE && res->allow_bootp == TRUE) 
               {
               resptr = resptr->next;
               continue;
               }
           else if (best->allow_bootp == TRUE && res->allow_bootp == FALSE) 
               {
               if (icmp_check(msgtype, &res->ip_addr) == GOOD)
                   best = res;
               else
	           turnoff_bind (res->binding);
               resptr = resptr->next;
               continue;
               }

           /*
            * Tiebreaker conditionals for preferred entries (either both 
            * unused or which both qualify as least recently used). 
            * Overall, these conditionals select the resource whose maximum
            * lease exceeds the threshold of the requested lease length by 
            * the minimum amount. If there is no resource whose maximum lease
            * exceeds the requested lease length, the resource with the
            * largest maximum lease is chosen.
            *
            * NOTE: These conditionals also implement a preference for resource
            *       entries which do not provide an infinite lease, unless
            *       specifically requested by the client.
            */

           if ( (best->binding == NULL && res->binding == NULL) ||
                  (best->binding != NULL && res->binding != NULL &&
                   best->binding->expire_epoch == res->binding->expire_epoch)) 
               {
               /*
                * Select resource with larger maximum lease,
                * even if shorter than the requested lease length.
                * Combined with the third conditional, the maximum value
                * of the maximum lease will be selected, if the lease length
                * threshold is not exceeded.
                */

               if (reqlease >= res->max_lease && 
                   res->max_lease > best->max_lease) 
                   {
                   if (icmp_check (msgtype, &res->ip_addr) == GOOD)
                       best = res;
                   else
	               turnoff_bind (res->binding);
                   resptr = resptr->next;
                   continue;
                   }

               /*
                * Among resources whose maximum lease exceeds
                * the requested value, select the minimum.
                * This condition is never true until either the 
                * previous or next condition evaluated to true.
                */

               if (reqlease != INFINITY && reqlease <= res->max_lease &&
                   res->max_lease < best->max_lease) 
                   {
                   if (icmp_check (msgtype, &res->ip_addr) == GOOD)
                       best = res;
                   else
	               turnoff_bind (res->binding);
                   resptr = resptr->next;
                   continue;
                   }

                /*
                 * Accept entries longer than both the requested value 
                 * and the current maximum. This condition evaluates to
                 * true at most once. Once it does, neither it or the
                 * first conditional will ever evaluate to true again.
                 */

               if (reqlease != INFINITY && res->max_lease >= reqlease &&
                   reqlease > best->max_lease) 
                   {
                   if (icmp_check (msgtype, &res->ip_addr) == GOOD)
                       best = res;
                   else
	               turnoff_bind (res->binding);
                   resptr = resptr->next;
                   continue;
                   }
               resptr = resptr->next;
               continue;
               }
 
           /*
            * Among previously used entries, select those which expired 
            * earlier. (In the aggregate, implements a LRU algorithm).
            */

           if (best->binding != NULL && res->binding != NULL &&
               best->binding->expire_epoch > res->binding->expire_epoch) 
               {
               if (icmp_check (msgtype, &res->ip_addr) == GOOD)
                   best = res;
               else
                   turnoff_bind (res->binding);
               resptr = resptr->next;
               continue;
               } 
           else
               {
	       resptr = resptr->next;
	       continue;
               }
           }
        resptr = resptr->next;
        }

    return (best);
    }

/*******************************************************************************
*
* choose_res - select a resource for DHCP client
*
* This routine chooses a resource in response to an incoming DHCP discover 
* message. If the server database contains a manual entry matching the
* client identifier, the corresponding resource is returned. Otherwise,
* the database entry which provides the requested IP address, if any, is 
* chosen. If the discover message does not require a specific entry, the
* server selects a new entry using its own internal criteria, or NULL if
* no entry is available.
*
* RETURNS: Matching resource, or NULL if none available.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static struct dhcp_resource * choose_res
    (
    struct client_id *cid, 	/* pointer to client ID */
    time_t curr_epoch, 		/* current time, in seconds */
    u_long reqlease 		/* requested lease duration (seconds) */
    )
    {
    struct dhcp_resource *res = NULL;

    /* 1. select with client identifier, if  found. */

    if ( (res = select_wcid (DHCPDISCOVER, cid, curr_epoch)) != NULL)
        return (res);

    /* 2. select with requested IP address, if any. */

    if ( (res = select_wreqip (DHCPDISCOVER, cid, curr_epoch)) != NULL)
        return (res);

    /* 3. select an entry using internal criteria. */

    res = select_newone (DHCPDISCOVER, cid, curr_epoch, reqlease);
    if (res != NULL)
        return (res);

#ifdef DHCPS_DEBUG
    logMsg ("Warning: DHCPDISCOVER - No available addresses in the pool.\n",
             0, 0, 0, 0, 0, 0);
#endif

    return (NULL);
    }

/*******************************************************************************
*
* update_db - add entries to internal data structures to reflect client state
*
* This routine updates the binding list and corresponding hash tables when
* the server receives a DHCP discover message, or a DHCP or BOOTP request
* message from a client. The binding entry for the resource is marked
* unavailable for a short interval (for DHCP discover) or until the expiration
* of the lease.
*
* RETURNS: 0 if update completed, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int update_db
    (
    int msgtype, 		/* DHCP message type */
    struct client_id *cid, 	/* pointer to client ID */
    struct dhcp_resource *res, 	/* pointer to lease descriptor */
    u_long lease, 		/* lease duration, in seconds */
    time_t curr_epoch 		/* current time, in seconds */
    )
    {
    struct dhcp_binding *binding = NULL;

    /*
     * Ignore lease descriptors already offered (res->binding != NULL)
     * if also reserved to specific clients (STATIC_ENTRY flag set).
     */

    if (res->binding != NULL && (res->binding->flag & STATIC_ENTRY) != 0)
        return (0);

    /* Remove old client identifier association from prior lease record. */

    if (res->binding != NULL) 
        hash_del (&cidhashtable, res->binding->cid.id, res->binding->cid.idlen,
	          bindcidcmp, &res->binding->cid, free_bind);

    /* Create and assign new lease record entry. */

    binding = (struct dhcp_binding *)calloc (1, sizeof (struct dhcp_binding));
    if (binding == NULL) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: memory allocation error updating database.\n", 
                 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    if (cidcopy (cid, &binding->cid) != 0)
        return (-1);
    if (msgtype == DHCPDISCOVER) 
        binding->temp_epoch = curr_epoch + MEMORIZE;
    else if (lease == 0xffffffff) 
        binding->expire_epoch = 0xffffffff;
    else 
        binding->expire_epoch = curr_epoch + lease;

    /* Link lease record and lease descriptor. */

    binding->res = res;

    bcopy (res->entryname, binding->res_name, strlen (res->entryname));
    binding->res_name [strlen (res->entryname)] = '\0';

    res->binding = binding;

    /* Record client hardware address. */

    binding->haddr.htype = dhcpsMsgIn.dhcp->htype;
    binding->haddr.hlen = dhcpsMsgIn.dhcp->hlen;
    if (binding->haddr.hlen > MAX_HLEN)
        binding->haddr.hlen = MAX_HLEN;
    bcopy (dhcpsMsgIn.dhcp->chaddr, binding->haddr.haddr, binding->haddr.hlen);

    /* Add association of lease record and client identifier. */

    if ( (hash_ins (&cidhashtable, binding->cid.id, binding->cid.idlen, 
                    bindcidcmp, &binding->cid, binding) < 0)) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: hash table insertion with client ID failed.\n",
                 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    /* Store record of lease. */

    if (add_bind (binding) != 0)
        return (-1);

    return (0);
    }

/*******************************************************************************
*
* turnoff_bind - mark resource entry as unavailable
*
* This routine updates the binding list and corresponding hash tables when
* the server discovers (through an ICMP check or client decline) that an IP 
* address is unexpectedly in use. The corresponding resource is marked as an 
* active lease for the next half-hour.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

static void turnoff_bind
    (
    struct dhcp_binding *binding 	/* unavailable lease record */
    )
    {
    time_t curr_epoch = 0;
    int result;

    if (binding == NULL)
        return;

    if (dhcpTime (&curr_epoch) == -1) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: turnoff_bind() can't retrieve current time.\n",
                 0, 0, 0, 0, 0, 0);
#endif
        return;
        }

    /* Remove client ID from hash table entry for address in use. */

    binding->expire_epoch = binding->temp_epoch = curr_epoch + 1800;
    hash_del (&cidhashtable, binding->cid.id, binding->cid.idlen, bindcidcmp,
	      &binding->cid, free_fake);
    bzero (binding->cid.id, binding->cid.idlen);
    result = hash_ins (&cidhashtable, binding->cid.id, binding->cid.idlen, 
                       bindcidcmp, &binding->cid, binding);
#ifdef DHCPS_DEBUG
    if (result < 0) 
        logMsg ("Warning: couldn't alter hash table in turnoff_bind()", 
                 0, 0, 0, 0, 0, 0);
#endif

    binding->flag &= ~COMPLETE_ENTRY;

    return;
    }

/*******************************************************************************
*
* clean_sbuf - clean the message transmission buffers
*
* This routine clears the vectored buffers used to store outgoing DHCP 
* messages. The first buffer contains the Ethernet, IP and UDP headers, as well
* as the fixed-length portion of the DHCP message and the options which fit 
* within the default (312 byte) option field. The second buffer contains 
* overflow options, if any, for clients capable of receiving DHCP messages 
* longer than the default 548 bytes.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

static void clean_sbuf (void)
    {
    bzero (sbufvec[0].iov_base, sbufvec[0].iov_len);
    bzero (sbufvec[1].iov_base, sbufvec[1].iov_len);
    sbufvec[1].iov_len = 0;
    return;
    }

/*******************************************************************************
*
* construct_msg - make an outgoing DHCP message
*
* This routine creates all DHCP server responses to client requests, according
* to the behavior specified in RFC 1541. The message type parameter indicates
* whether to build a DHCP offer message, a DHCP ACK message, or a NAK reply.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

static void construct_msg
    (
    u_char msgtype, 		/* type of DHCP message to construct */
    struct dhcp_resource *res, 	/* lease descriptor describing contents */
    u_long lease, 		/* lease duration, in seconds */
    struct if_info *ifp 	/* descriptor of receiving interface */
    )
    {
    int i = 0;
    int reqoptlen = 0;
    u_long tmp = 0;
    char *reqopt = NULL;
    char inserted[32];
    char *option = NULL;
    struct client_id   paramId;   /* Key for additional parameters. */
    struct dhcp_resource * params;   /* Client- or class-specific options. */
    int result;

    bzero (inserted, sizeof (inserted));

    clean_sbuf ();            /* Zero out outgoing message buffer. */
    dhcpsMsgOut.dhcp->op = BOOTREPLY;
    dhcpsMsgOut.dhcp->htype = dhcpsMsgIn.dhcp->htype;
    dhcpsMsgOut.dhcp->hlen = dhcpsMsgIn.dhcp->hlen;
    dhcpsMsgOut.dhcp->hops = 0;
    dhcpsMsgOut.dhcp->xid = dhcpsMsgIn.dhcp->xid;
    dhcpsMsgOut.dhcp->secs = 0;
    dhcpsMsgOut.dhcp->flags = dhcpsMsgIn.dhcp->flags;
    dhcpsMsgOut.dhcp->giaddr.s_addr = dhcpsMsgIn.dhcp->giaddr.s_addr;
    bcopy (dhcpsMsgIn.dhcp->chaddr, dhcpsMsgOut.dhcp->chaddr,
           dhcpsMsgIn.dhcp->hlen);

    if (msgtype == DHCPACK)   /* ciaddr stays zero for all other types. */
        dhcpsMsgOut.dhcp->ciaddr.s_addr = dhcpsMsgIn.dhcp->ciaddr.s_addr;

    if (msgtype != DHCPNAK) 
        {
        dhcpsMsgOut.dhcp->yiaddr.s_addr = res->ip_addr.s_addr;
        if (ISSET (res->valid, S_SIADDR)) 
            {
            dhcpsMsgOut.dhcp->siaddr.s_addr = res->siaddr.s_addr;
            } 
        else 
            {
            dhcpsMsgOut.dhcp->siaddr.s_addr = 0;
            }
        overload = BOTH_AREOPT;
        if (ISSET (res->valid, S_SNAME)) 
            {
            strncpy (dhcpsMsgOut.dhcp->sname, res->sname, MAX_SNAME);
            dhcpsMsgOut.dhcp->sname [MAX_SNAME - 1] = '\0';
            overload -= SNAME_ISOPT;
            }
        if (ISSET (res->valid, S_FILE)) 
            {
            strncpy (dhcpsMsgOut.dhcp->file, res->file, MAX_FILE);
            dhcpsMsgOut.dhcp->file [MAX_FILE - 1] = '\0';
            overload -= FILE_ISOPT;
            }
        } 
    else 
        {
        dhcpsMsgOut.dhcp->yiaddr.s_addr = 0;
        dhcpsMsgOut.dhcp->siaddr.s_addr = 0;
        /* Refinement for draft RFC. */
        /* if (dhcpsMsgIn.giaddr.s_addr != 0)
            SETBRDCAST (dhcpsMsgOut.dhcp->flags); */
        }

    /* insert magic cookie */
    bcopy ((char *)dhcpCookie, dhcpsMsgOut.dhcp->options, MAGIC_LEN);
    off_options = MAGIC_LEN;
    off_extopt = 0;

    /* insert dhcp message type option */
    dhcpsMsgOut.dhcp->options [off_options++] = _DHCP_MSGTYPE_TAG;
    dhcpsMsgOut.dhcp->options [off_options++] = 1;
    dhcpsMsgOut.dhcp->options [off_options++] = msgtype;

    /* Insert client ID when permitted. (Only allowed under draft RFC). */

    if (msgtype == DHCPNAK) 
        {
        SETBRDCST (dhcpsMsgOut.dhcp->flags);
/*      option = pickup_opt (dhcpsMsgIn.dhcp, rdhcplen, _DHCP_CLIENT_ID_TAG);
        if (option != NULL) 
            {
            dhcpsMsgOut.dhcp->options [off_options++] = _DHCP_CLIENT_ID_TAG;
            dhcpsMsgOut.dhcp->options [off_options++] = DHCPOPTLEN(option);
            bcopy (option, &dhcpsMsgOut.dhcp->options [off_options],
                   DHCPOPTLEN (option));
            off_options += DHCPOPTLEN (option);
            } */
        return;
        }

    /* insert "server identifier" (required). */

    dhcpsMsgOut.dhcp->options [off_options++] = _DHCP_SERVER_ID_TAG;
    dhcpsMsgOut.dhcp->options [off_options++] = 4;
    bcopy ( (char *)&ifp->ipaddr.s_addr, 
            &dhcpsMsgOut.dhcp->options [off_options], 4);
    off_options += 4;

    /* insert "subnet mask" (permitted). */

    result = insert_opt (res, lease, _DHCP_SUBNET_MASK_TAG, inserted, PASSIVE);
#ifdef DHCPS_DEBUG
    if (result == E_NOMORE) 
        logMsg ("No space left in options field for DHCP%s",
                 (int)((msgtype == DHCPOFFER) ? "OFFER" : "ACK"), 
                 0, 0, 0, 0, 0);
#endif

    /* insert "lease duration" (required). */

    tmp = htonl (lease);
    dhcpsMsgOut.dhcp->options [off_options++] = _DHCP_LEASE_TIME_TAG;
    dhcpsMsgOut.dhcp->options [off_options++] = 4;
    bcopy ( (char *)&tmp, &dhcpsMsgOut.dhcp->options [off_options], 
            sizeof (u_long));
    off_options += 4;

    /* Insert "option overload" tag, if needed. */

    if (overload != 0) 
        {
        dhcpsMsgOut.dhcp->options[off_options++] = _DHCP_OPT_OVERLOAD_TAG;
        dhcpsMsgOut.dhcp->options[off_options++] = 1;
        dhcpsMsgOut.dhcp->options[off_options++] = overload;
        }

    /* insert the requested options */

    option = pickup_opt(dhcpsMsgIn.dhcp, rdhcplen, _DHCP_REQ_LIST_TAG);
    if (option != NULL) 
        {
        reqopt = OPTBODY (option);
        reqoptlen = DHCPOPTLEN (option);

        /* 
         * Handle requested parameters. The PASSIVE flag only inserts options
         * explicity configured into the resource entry. (Rule 1 of RFC 1541).
         * Because the implementation used "tblc=dflt" to force inclusion of
         * any missing parameters defined in the Host Requirements Document,
         * the PASSIVE flag will also include those settings if not already 
         * present. (Rule 2 of RFC 1541).
         */

        for (i = 0; i < reqoptlen; i++) 
            if (ISCLR (inserted, * (reqopt + i))) 
                {
	        result = insert_opt (res, lease, * (reqopt + i), inserted, 
                                     PASSIVE);
	        if (result == E_NOMORE)
                    {
#ifdef DHCPS_DEBUG
	            logMsg ("No space left in options field for DHCP%s",
		             (int)((msgtype == DHCPOFFER) ? "OFFER" : "ACK"), 
                             0, 0, 0, 0, 0);
#endif
	            break;
	            }
                }
        }

    /* 
     * Insert parameters which differ from the Host Requirements RFC defaults.
     * (The tags for these parameters are preceded by "!" in the server
     * configuration table).
     */

    for (i = 0; i < _DHCP_LAST_OPTION; i++) 
        if (ISCLR (inserted, i)) 
            if (insert_opt (res, lease, i, inserted, ACTIVE) == E_NOMORE) 
                {
#ifdef DHCPS_DEBUG
	        logMsg ("No space left in options field for DHCP%s",
	                 (int)((msgtype == DHCPOFFER) ? "OFFER" : "ACK"), 
                         0, 0, 0, 0, 0);
#endif
	        break;
                }

    /* Insert any client-specific options. */

       /* Insert any parameters associated with explicit client identifier. */

    tmp = 0;
    option = pickup_opt (dhcpsMsgIn.dhcp, rdhcplen, _DHCP_CLIENT_ID_TAG);
    if (option != NULL)
        {
        paramId.idlen = DHCPOPTLEN (option) - 1;
        paramId.idtype = *(char *)OPTBODY (option);
        bcopy (OPTBODY (option) + sizeof (char), paramId.id, paramId.idlen);

        params = hash_find (&paramhashtable, 
                            paramId.id, paramId.idlen, paramcidcmp, &paramId);

        /* Insert options from matching resource entry not already present. */

        if (params != NULL)
            {
            for (i = 0; i < _DHCP_LAST_OPTION; i++)
                if (ISCLR (inserted, i))
                    if (insert_opt (params, lease, i, inserted, PASSIVE) 
                           == E_NOMORE)
                        {
#ifdef DHCPS_DEBUG
                        logMsg ("No space left in options field for DHCP%s",
                               (int)((msgtype == DHCPOFFER) ? "OFFER" : "ACK"),
                               0, 0, 0, 0, 0);
#endif
                        break;
                        }
            tmp = 1;       /* Client-specific options found. */
            }

        }

    /*
     * If no client ID included, or no associated options found, check 
     * hardware address. 
     */

    if (tmp == 0)
        {
        paramId.idlen = dhcpsMsgIn.dhcp->hlen;
        paramId.idtype = dhcpsMsgIn.dhcp->htype;
        bcopy (dhcpsMsgIn.dhcp->chaddr, paramId.id, dhcpsMsgIn.dhcp->hlen);

        params = hash_find (&paramhashtable,
                            paramId.id, paramId.idlen, paramcidcmp, &paramId);

        /* Insert options from matching resource entry not already present. */

        if (params != NULL)
            {
            for (i = 0; i < _DHCP_LAST_OPTION; i++)
                if (ISCLR (inserted, i))
                    if (insert_opt (params, lease, i, inserted, PASSIVE)
                           == E_NOMORE)
                        {
#ifdef DHCPS_DEBUG
                        logMsg ("No space left in options field for DHCP%s",
                               (int)((msgtype == DHCPOFFER) ? "OFFER" : "ACK"),
                               0, 0, 0, 0, 0);
#endif
                        break;
                        }
            }
        }

    /* Insert any class-specific options. */

    option = pickup_opt (dhcpsMsgIn.dhcp, rdhcplen, _DHCP_CLASS_ID_TAG);
    if (option != NULL)
        {
        paramId.idlen = DHCPOPTLEN (option);
        paramId.idtype = 0;         /* Unused for class identifiers. */
        bcopy (OPTBODY (option), paramId.id, paramId.idlen);

        params = hash_find (&paramhashtable,
                            paramId.id, paramId.idlen, paramcidcmp, &paramId);

        /* Insert options from matching resource entry not already present. */

        if (params != NULL)
            {
            for (i = 0; i < _DHCP_LAST_OPTION; i++)
                if (ISCLR (inserted, i))
                    if (insert_opt (params, lease, i, inserted, PASSIVE)
                           == E_NOMORE)
                        {
#ifdef DHCPS_DEBUG
                        logMsg ("No space left in options field for DHCP%s",
                               (int)((msgtype == DHCPOFFER) ? "OFFER" : "ACK"),
                               0, 0, 0, 0, 0);
#endif
                        break;
                        }
            }
        }
    return;
    }

/*******************************************************************************
*
* select_wciaddr - retrieve resource with matching IP address
*
* This routine attempts to find an address pool entry whose IP address matches
* the value requested by the client. If the matching IP address is part of a
* manual lease, it also verifies that the client ID matches the required value.
* Otherwise, it checks if the requesting client received an offer from the
* server.
*
* RETURNS: Matching resource, or NULL if none or not available.
*
* ERRNO: N/A
*
* NOMANUAL
*/

/*
 * choose resource with ciaddr
 */
static struct dhcp_resource * select_wciaddr
    (
    struct client_id *cid, 	/* pointer to identifier of request client */
    time_t curr_epoch, 		/* current time, in seconds */
    int *nosuchaddr 		/* flag indicating if address foun in table */
    )
    {
    struct dhcp_resource *res = NULL;
#ifdef DHCPS_DEBUG
    char tmp [INET_ADDR_LEN];
#endif

    *nosuchaddr = FALSE;
    res = (struct dhcp_resource *)hash_find (&iphashtable, 
                                      (char *)&dhcpsMsgIn.dhcp->ciaddr.s_addr,
                                             sizeof (u_long), resipcmp, 
                                             &dhcpsMsgIn.dhcp->ciaddr);
    if (res == NULL) 
        {
        *nosuchaddr = TRUE;   /* Fatal error - expected entry not found. */
        return (NULL);
        } 
    else 
        {
        /* Check for subnet match. */
        if (cid->subnet.s_addr != 
             (res->ip_addr.s_addr & res->subnet_mask.s_addr)) 
            {
#ifdef DHCPS_DEBUG
            inet_ntoa_b (dhcpsMsgIn.dhcp->ciaddr, tmp);
            logMsg ("Subnet mismatch for DHCPREQUEST (cid: %s, ciaddr: %s).\n",
                     (int)cidtos (cid, 1), (int)tmp, 0, 0, 0, 0);
#endif
            return (NULL);
            }
        else if (ISSET(res->valid, S_CLIENT_ID))    /* Manual allocation. */
            {
            /* Fatal error if no binding present for manual allocation. */
            if (res->binding == NULL) 
                {
#ifdef DHCPS_DEBUG
                inet_ntoa_b (dhcpsMsgIn.dhcp->ciaddr, tmp);
	        logMsg ("DHCPREQUEST(cid:\"%s\"): No binding for %s",
                         (int)cidtos(cid, 1), (int)tmp, 0, 0, 0, 0);
#endif
	        *nosuchaddr = TRUE;
	        return (NULL);
                }
            /* Check that client ID of binding matches requesting client. */
            else if (res->binding->cid.idtype != cid->idtype ||
	             res->binding->cid.idlen != cid->idlen ||
	             bcmp (res->binding->cid.id, cid->id, cid->idlen) != 0) 
                    {
#ifdef DHCPS_DEBUG
                    inet_ntoa_b (dhcpsMsgIn.dhcp->ciaddr, tmp);
	            logMsg (
                  "DHCPREQUEST(cid:\"%s\", ciaddr:%s) - client ID mismatch.\n",
                               (int)cidtos(cid, 1), (int)tmp, 0, 0, 0, 0);
#endif
	            return (NULL);
                    }
            }
        /* Dynamic or automatic allocation. Fails if no binding present
         * (i.e. - unknown request), or lease has expired (i.e. - late
         * request), or if client ID doesn't match expected value from
         * the DHCP offer message.
         */
        else if (res->binding == NULL ||
	         (res->binding->expire_epoch != 0xffffffff &&
	          res->binding->expire_epoch <= curr_epoch) ||
	         res->binding->cid.idtype != cid->idtype ||
	         res->binding->cid.idlen != cid->idlen ||
	         bcmp (res->binding->cid.id, cid->id, cid->idlen) != 0) 
                 {
#ifdef DHCPS_DEBUG
                 inet_ntoa_b (dhcpsMsgIn.dhcp->ciaddr, tmp);
                 logMsg ("DHCPREQUEST(cid:\"%s\",ciaddr:%s): is unavailable.\n",
	                  (int)cidtos(cid, 1), (int)tmp, 0, 0, 0, 0);
#endif
                 return (NULL);
                 }
        }
    return (res);
    }

/*******************************************************************************
*
* discover - handle client discover messages
*
* This routine examines client discover messages, selects an available 
* resource (if any), calculates the length of the lease, and sends the 
* appropriate offer to the client.
*
* RETURNS: 0 if processing successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int discover
    (
    struct if_info *ifp    /* pointer to descriptor of receiving interface */
    )
    {
    struct dhcp_resource *offer_res = NULL; /* lease descriptor chosen */
    struct client_id cid;  			/* ID of requesting client */
    u_long offer_lease = 0; 			/* offered lease duration */
    u_long reqlease = 0; 			/* requested lease duration */
    time_t curr_epoch = 0; 			/* current time, in seconds */
    int result; 				/* error value, if any */

    bzero ((char *)&cid, sizeof (cid));

    if (dhcpTime (&curr_epoch) == -1) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: discover() couldn't retrieve current time.\n",
                 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    /* Extract requested lease from DHCP message. */
    reqlease = get_reqlease (dhcpsMsgIn.dhcp, rdhcplen);

    /* Determine maximum length available in DHCP message for options. */

    maxoptlen = get_maxoptlen (dhcpsMsgIn.dhcp, rdhcplen);

    /* Set pointers to access client ID within DHCP message options field. */

    get_cid (dhcpsMsgIn.dhcp, rdhcplen, &cid);

    /* Critical section with dhcpsLeaseEntryAdd(). */

    semTake (dhcpsMutexSem, WAIT_FOREVER);

    /* Retrieve subnet for incoming message. */

    if (get_subnet (dhcpsMsgIn.dhcp, rdhcplen, &cid.subnet, ifp) != 0)
        { 
        semGive (dhcpsMutexSem);
        return (-1);
        }

    /* Select an available lease descriptor. */

    if ( (offer_res = choose_res (&cid, curr_epoch, reqlease)) == NULL)
        {
        semGive (dhcpsMutexSem);
        return (-1);
        }

    /* Select a lease duration. */

    offer_lease = choose_lease (reqlease, curr_epoch, offer_res);

    /* Record lease offer in data structures. */

    result = update_db (DHCPDISCOVER, &cid, offer_res, offer_lease, curr_epoch);
    semGive (dhcpsMutexSem);

    if (result != 0)
        return (-1);

    /* Create outgoing DHCP offer message. */

    construct_msg (DHCPOFFER, offer_res, offer_lease, ifp);

    /*
     * xxx must be able to handle the fragments, but currently not implemented
     */

    /* Transfer message to receiving interface. */

    send_dhcp (ifp, DHCPOFFER);

    return (0);
    }

/*******************************************************************************
*
* request - handle client request messages
*
* This routine responds to request messages sent by clients in response
* to an offer from a DHCP server. If the server generated the offer, it
* sends the appropriate ACK or NAK message. Otherwise, it updates the 
* internal data structure to reflect the implicit decline from the client.
*
* RETURNS: 0 if processing successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int request
    (
    struct if_info *ifp    /* pointer to descriptor of receiving interface */
    )
    {
    BOOL reqforme = FALSE;
    BOOL nosuchaddr = FALSE;
    struct dhcp_resource *res = NULL;
    struct client_id cid;
    struct in_addr reqip;
    struct in_addr netmask;
    unsigned long offer_lease = 0;         /* offering lease */
    unsigned long reqlease = 0;            /* requested lease duration */
    char *option = NULL;
    int response;                 /* Accept request? */

#define EPOCH      "Thu Jan  1 00:00:00 1970\n"
#define BRDCSTSTR  "255.255.255.255"
    char datestr [sizeof (EPOCH)];
    char addrstr [sizeof (BRDCSTSTR)];
    time_t curr_epoch = 0;                 /* current epoch */

    bzero ((char *)&cid, sizeof (cid));
    bcopy (EPOCH, datestr, sizeof (EPOCH));
    bcopy (BRDCSTSTR, addrstr, sizeof (BRDCSTSTR));
    response = 2;            /* Response not determined. */

    if (dhcpTime (&curr_epoch) == -1)
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: Couldn't get timestamp when processing request.\n",
                 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    reqlease = get_reqlease (dhcpsMsgIn.dhcp, rdhcplen);
    maxoptlen = get_maxoptlen (dhcpsMsgIn.dhcp, rdhcplen);
    get_cid (dhcpsMsgIn.dhcp, rdhcplen, &cid);

    /* Critical section with dhcpsLeaseEntryAdd(). */

    semTake (dhcpsMutexSem, WAIT_FOREVER);
    if (get_subnet (dhcpsMsgIn.dhcp, rdhcplen, &cid.subnet, ifp) != 0)
        { 
        semGive (dhcpsMutexSem);
        return (-1);
        }

    /*
     * Check if this DHCP server is the request destination. 
     * (Option not present unless client is in SELECTING state).  
     */

    option = pickup_opt (dhcpsMsgIn.dhcp, rdhcplen, _DHCP_SERVER_ID_TAG);
    if (option != NULL) 
        if (htonl (GETHL (OPTBODY (option))) == ifp->ipaddr.s_addr)
            reqforme = TRUE;

    /*
     * Check the previously allocated network address sent by client
     * (i.e. - client is in RENEWING or REBINDING state). 
     */ 

    if (dhcpsMsgIn.dhcp->ciaddr.s_addr != 0) 
        {
        /* For client in RENEWING state, no relay agents are used. */
        
        if (get_snmk (dhcpsMsgIn.dhcp, rdhcplen, &netmask, ifp) == 0) 
            if (dhcpsMsgIn.dhcp->giaddr.s_addr != 0) 
                /* REBINDING state - check network of client. */
	        if ( (dhcpsMsgIn.dhcp->giaddr.s_addr & netmask.s_addr) !=
	             (dhcpsMsgIn.dhcp->ciaddr.s_addr & netmask.s_addr))
	            /* goto nak;   */               /* Different subnet. */
                    response = 0;          /* Send NAK to client. */

        
        if (response != 0)    /* Response still not determined. */
            {
            res = select_wciaddr (&cid, curr_epoch, &nosuchaddr);
            if (res == NULL) 
                {
                /* 
                 * If no entry present or manual entry present without a 
                 * matching binding the request is meant for another server
                 * (for renewal request) or cannot be satisfied (for rebinding).
                 */

                if (nosuchaddr == TRUE)
                    {
                    semGive (dhcpsMutexSem);
                    return (-1);
                    }

                /* deny request for subnet mismatch, client ID mismatch,
                 * expired lease, or missing binding for non-manual allocation.
                 * (Missing binding means no DISCOVER received for request). 
                 */

                 else
                     response = 0;
                } 
            else 
                /* goto ack; */
                response = 1;             /* Send ACK to client. */
            }
        }

    if (response == 2)       /* Response still undetermined. */
        {
        /* Requesting client has no IP address (i.e. - initial request
         * from SELECTING state or verification of cached lease from 
         * INIT-REBOOT state).
         */

        reqip.s_addr = 0;
        option = pickup_opt (dhcpsMsgIn.dhcp, rdhcplen, 
                             _DHCP_REQUEST_IPADDR_TAG);
        if (option != NULL)
            reqip.s_addr = htonl (GETHL (OPTBODY (option)));

        if (reqip.s_addr != 0) 
            {
            if (get_snmk (dhcpsMsgIn.dhcp, rdhcplen, &netmask, ifp) == 0) 
                {
                /*
                 * Deny request received if client is on the wrong network.
                 * (Suggested behavior from RFC 1541).
                 */
                if (dhcpsMsgIn.dhcp->giaddr.s_addr != 0) 
                    {
                    /* Deny request received from relay agent if requested IP 
                     * address is not on agent's subnet. 
                     */

	            if ( (dhcpsMsgIn.dhcp->giaddr.s_addr & netmask.s_addr) !=
	                 (reqip.s_addr & netmask.s_addr))
                        response = 0;         /* Send NAK to client. */
                    }
                else 
                    {
                    /* Deny request received directly if requested IP address
                     * on different subnet from receiving interface. 
                     */

	            if ( (ifp->ipaddr.s_addr & netmask.s_addr) !=
	                 (reqip.s_addr & netmask.s_addr))
	                response = 0;         /* Send NAK to client. */
                    }
                }

            if (response == 2)          /* Response still undetermined. */
                { 
                /* Subnets match - look for entry in address pool. */

                res = NULL;
                res = select_wcid (DHCPREQUEST, &cid, curr_epoch);
                if (res != NULL && res->ip_addr.s_addr != 0 &&
                     res->ip_addr.s_addr == reqip.s_addr)
                    response = 1;       /* Send ACK to client. */

                /* 
                 * Deny request if offered address is no longer available,
                 * or if requested address doesn't match offered address.
                 */

                else if (reqforme == TRUE)

                    /*
                     * Client's notion of IP address is incorrect -  
                     * deny the request. (Follows suggested behavior 
                     * of RFC 1541 for client in SELECTING state).
                     */

                    response = 0;       /* Send NAK to client. */
                else

                    /*
                     * No record of client, or another server was
                     * selected - ignore request message.
                     * Implements mandatory behavior from RFC 1541
                     * for client in INIT-REBOOT state.
                     */
                    {
                    semGive (dhcpsMutexSem);
                    return (-1);
                    }
                }
            }
   
        /* Ignore message if needed IP address not present. */ 

        if (response == 2)   /* Don't exit if response determined. */
            {
            semGive (dhcpsMutexSem);
            return (-1);
            }
        }

    if (response == 1)      /* A DHCP server will send ACK to client. */
        {
        offer_lease = choose_lease (reqlease, curr_epoch, res);
 
        if (update_db (DHCPREQUEST, &cid, res, offer_lease, curr_epoch) != 0) 
            {
            if (reqforme == TRUE)  /* Error in selected server's database. */
                response = 0;
            else
                {
                semGive (dhcpsMutexSem);
                return (-1);
                }
            }
        if (response == 1)      /* Lease selected and stored in database. */
            {
            semGive (dhcpsMutexSem);

            construct_msg (DHCPACK, res, offer_lease, ifp);

            /* send DHCPACK from the interface
             * xxx must be able to handle the fragments, but currently not 
             * implemented.
             */
            send_dhcp (ifp, DHCPACK);

            res->binding->flag |= COMPLETE_ENTRY;    /* binding is complete. */

            strcpy (datestr, ctime (&res->binding->expire_epoch));
            datestr [strlen (datestr) - 1] = '\0';
#ifdef DHCPS_DEBUG
            inet_ntoa_b (res->ip_addr, addrstr);
            logMsg ("Address %s assigned to client(cid: \"%s\") till \"%s\".",
	             (int)addrstr, (int)cidtos (&res->binding->cid, 1), 
                     (int)datestr, 0, 0, 0);
#endif
            }
        }

    /* Deny request with NAK if client or server error occurred. */

    if (response == 0)
        {
        semGive (dhcpsMutexSem);

        construct_msg (DHCPNAK, NULL, offer_lease, ifp);

        /*
         * send DHCPNAK from the interface
         * xxx must be able to handle fragments, but currently not implemented
         */

        send_dhcp (ifp, DHCPNAK);
        }
    return (0);
    }

/*******************************************************************************
*
* decline - handle client decline messages
*
* This routine responds to optional decline messages sent by clients which 
* have selected a different offer or detected that an offered address is 
* already in use. If the server generated the corresponding offer, it
* updates the internal data structure to indicate the address is unavailable.
* Otherwise, the decline message is ignored.
*
* RETURNS: 0 if processing successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int decline
    (
    struct if_info *ifp    /* pointer to descriptor of receiving interface */
    )
    {
    struct dhcp_binding *binding = NULL;
    struct dhcp_resource *res = NULL;
    struct client_id cid;
    char *option = NULL;
    char msg [255];
#ifdef DHCPS_DEBUG
    char output [INET_ADDR_LEN];
#endif

    bzero ((char *)&cid, sizeof (cid));

    /* DECLINE for another server */
    option = pickup_opt (dhcpsMsgIn.dhcp, rdhcplen, _DHCP_SERVER_ID_TAG);
    if (option == NULL ||
          htonl (GETHL (OPTBODY (option))) != ifp->ipaddr.s_addr) 
        return (0);

    get_cid (dhcpsMsgIn.dhcp, rdhcplen, &cid);

    /* Critical section with dhcpsLeaseEntryAdd(). */

    semTake (dhcpsMutexSem, WAIT_FOREVER);

    if (get_subnet (dhcpsMsgIn.dhcp, rdhcplen, &cid.subnet, ifp) != 0)
        { 
        semGive (dhcpsMutexSem);
        return (-1);
        }

    /* search with haddr (haddr has same format as client identifier) */

    binding = (struct dhcp_binding *)hash_find (&cidhashtable, cid.id, 
                                                cid.idlen, bindcidcmp, &cid);
    if (binding == NULL) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("DHCPDECLINE received from unknown client.\n", 
                 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        } 
    else 
        res = binding->res;

    /* Remove link between lease descriptor and record of (declined) offer. */

    if (binding->res != NULL) 
        turnoff_bind (binding);
    else 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Received invalid DHCPDECLINE.\n", 0, 0, 0, 0, 0, 0);
#endif
        semGive (dhcpsMutexSem);
        return (0);
        }

    semGive (dhcpsMutexSem);

#ifdef DHCPS_DEBUG
    inet_ntoa_b (binding->res->ip_addr, output);
    logMsg ("Received DHCP decline for entry %s (IP=%s).\n",
             (int)binding->res->entryname,
             output, 0, 0, 0, 0);
#endif

    option = pickup_opt (dhcpsMsgIn.dhcp, rdhcplen, _DHCP_ERRMSG_TAG);
    if (option != NULL) 
        {
        nvttostr (OPTBODY (option), msg, (int)DHCPOPTLEN (option));
#ifdef DHCPS_DEBUG
        if (msg[0] != '\0') 
            logMsg ("Client decline message: \"%s\".\n", (int)msg, 
                    0, 0, 0, 0, 0);
#endif
        }
    return (0);
    }

/*******************************************************************************
*
* release - handle client release messages
*
* This routine responds to release messages sent by clients to relinquish
* their lease before it expires. If the server holds a dynamic (i.e. - finite)
* lease for the originating client, it updates the internal data structure
* to mark the corresponding address pool entry as available.
*
* RETURNS: 0 if processing successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int release
    (
    struct if_info *ifp    /* pointer to descriptor of receiving interface */
    )
    {
    struct dhcp_binding *binding = NULL;
    struct dhcp_resource *res = NULL;
    char *option = NULL;
#ifdef DHCPS_DEBUG
    char output [INET_ADDR_LEN];
#endif

    /* release for another server */
    option = pickup_opt (dhcpsMsgIn.dhcp, rdhcplen, _DHCP_SERVER_ID_TAG);
    if (option == NULL ||
          htonl (GETHL (OPTBODY (option))) != ifp->ipaddr.s_addr) 
        return (0);

    /* Critical section with dhcpsLeaseEntryAdd(). */

    semTake (dhcpsMutexSem, WAIT_FOREVER);

    /* search with ciaddr */
    res = (struct dhcp_resource *)hash_find (&iphashtable, 
                                       (char *)&dhcpsMsgIn.dhcp->ciaddr.s_addr,
		                             sizeof (u_long), resipcmp, 
                                             &dhcpsMsgIn.dhcp->ciaddr);
    semGive (dhcpsMutexSem);

    if (res == NULL) 
        {
#ifdef DHCPS_DEBUG
        inet_ntoa_b (dhcpsMsgIn.dhcp->ciaddr, output);
        logMsg ("Received DHCPRELEASE from unknown client (IP:%s).\n",
	         (int)output, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }
    else 
        binding = res->binding;

    /* Verify retrieved entry matches requesting non-manual client. */

    if (binding != NULL && binding->res != NULL && binding->res == res && 
          ISCLR (binding->res->valid, S_CLIENT_ID) && 
          binding->haddr.htype == dhcpsMsgIn.dhcp->htype && 
          binding->haddr.hlen == dhcpsMsgIn.dhcp->hlen &&
          bcmp (binding->haddr.haddr, dhcpsMsgIn.dhcp->chaddr, 
                dhcpsMsgIn.dhcp->hlen) == 0) 
        binding->expire_epoch = 0;
    else 
        {
#ifdef DHCPS_DEBUG
        /* CLIENT_ID option is only present for manual (infinite) leases. */

        inet_ntoa_b (dhcpsMsgIn.dhcp->ciaddr, output);

        if (binding != NULL && binding->res != NULL &&
              ISSET (binding->res->valid, S_CLIENT_ID)) 
            logMsg ("DHCPRELEASE received for static entry(IP:%s).\n",
	             (int)output, 0, 0, 0, 0, 0);
        else 
            /* Hardware address mismatch with requesting client. */
            logMsg ("Received invalid DHCPRELEASE from client(IP:%s).\n",
                     (int)output, 0, 0, 0, 0, 0);
#endif
        return (0);
        }
    return (0);
    }

/*******************************************************************************
*
* send_dhcp - transmit a server response
*
* This routine sends messages formulated by an earlier construct_msg() call
* directly to the destination address, if available, or as a broadcast to 
* the client's subnet.
*
* RETURNS: 0 if message sent, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int send_dhcp
    (
    struct if_info *ifp, 	/* descriptor of transmission interface */
    int msgtype 		/* type of DHCP message */
    )
    {
    int msglen = 0;
    char devName [10];
    struct iovec bufvec[2];
    struct sockaddr_in dst;
    BOOL bcastFlag;
    int result;

    struct msghdr msg;
    struct ifnet *pIf;

    bzero ((char *)&dst, sizeof (dst));
    dst.sin_len = sizeof (struct sockaddr_in);
    dst.sin_family = AF_INET;

    if (overload & FILE_ISOPT) 
        dhcpsMsgOut.dhcp->file[off_file] = _DHCP_END_TAG;
    if (overload & SNAME_ISOPT) 
        dhcpsMsgOut.dhcp->sname[off_sname] = _DHCP_END_TAG;
    if (off_options < DFLTOPTLEN) 
        dhcpsMsgOut.dhcp->options[off_options] = _DHCP_END_TAG;
    else if (off_extopt > 0 && off_extopt < maxoptlen - DFLTOPTLEN)
        sbufvec[1].iov_base[off_extopt++] = _DHCP_END_TAG;

    if (off_extopt < sbufvec[1].iov_len) 
        sbufvec[1].iov_len = off_extopt;

    /*
     * Send message through socket if received from relay agent or client 
     * on a different subnet than the receiving network interface.
     */

    if (dhcpsMsgIn.dhcp->giaddr.s_addr != 0 ||
        (dhcpsMsgIn.dhcp->ciaddr.s_addr != 0 &&
         (dhcpsMsgIn.dhcp->ciaddr.s_addr & ifp->subnetmask.s_addr) !=
         (ifp->ipaddr.s_addr & ifp->subnetmask.s_addr))) 
        {
        if (dhcpsMsgIn.dhcp->ciaddr.s_addr != 0) 
            {
            dst.sin_port = dhcpc_port;
            bcopy ( (char *)&dhcpsMsgIn.dhcp->ciaddr, 
                    (char *)&dst.sin_addr, sizeof (u_long));
            }
        else if (dhcpsMsgIn.dhcp->giaddr.s_addr != 0) 
            {
            dst.sin_port = dhcps_port;
            bcopy ( (char *)&dhcpsMsgIn.dhcp->giaddr, 
                    (char *)&dst.sin_addr, sizeof (u_long));
            }

        bufvec[0].iov_base = (char *) dhcpsMsgOut.dhcp;
        msglen = bufvec[0].iov_len = DFLTDHCPLEN;
        if (sbufvec[1].iov_len == 0)
            bufvec[1].iov_base = NULL;
        else
            bufvec[1].iov_base = sbufvec[1].iov_base;
        bufvec[1].iov_len = sbufvec[1].iov_len;
        msglen += bufvec[1].iov_len;
        if (setsockopt (dhcpsIntfaceList->fd, SOL_SOCKET, SO_SNDBUF, 
                        (char *)&msglen, sizeof (msglen)) < 0) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: Couldn't set transmit buffer size for DHCP.\n",
                     0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }

        bzero ((char *)&msg, sizeof (msg));
        msg.msg_name = (caddr_t) &dst;
        msg.msg_namelen = sizeof (dst);
        msg.msg_iov = bufvec;
        if (bufvec[1].iov_base == NULL)
            msg.msg_iovlen = 1;
        else
            msg.msg_iovlen = 2;
        if (sendmsg (dhcpsIntfaceList->fd, &msg, 0) < 0) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: Couldn't send DHCP message.\n", 
                     0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }

        return (0);
        }

    /*
     * Message sent by client on same subnet as receiving interface.
     * The destination may not respond to ARP requests, so the
     * socket interface is unusable. Build and send the reply within
     * a complete link-level frame instead.
     */

    /* Set destination address and fill pseudo header to calculate checksum */

    bcopy ( (char *)&ifp->ipaddr.s_addr, 
            (char *)&dhcpsMsgOut.ip->ip_src, sizeof (u_long));

    if (dhcpsMsgOut.dhcp->yiaddr.s_addr != 0 &&
        !ISBRDCST (dhcpsMsgIn.dhcp->flags)) 
        {
        /* Send DHCPOFFER and DHCPACK messages to the client via unicast. */

        dhcpsMsgOut.ip->ip_dst.s_addr = dhcpsMsgOut.dhcp->yiaddr.s_addr;
        dst.sin_addr.s_addr = dhcpsMsgOut.dhcp->yiaddr.s_addr;
        bcastFlag = FALSE;
        }
    else
        {
        /*
         * Broadcast DHCPOFFER and DHCPACK messages if needed by the client.
         * Also broadcast all DHCPNAK messages with a non-zero 'giaddr'
         * field (because the 'yiaddr' field is always zero in this case).
         */

        dst.sin_addr.s_addr = dhcpsMsgOut.ip->ip_dst.s_addr = 0xffffffff;
        bcastFlag = TRUE;
        }
    dst.sin_port = dhcpc_port;

    dhcpsMsgOut.udp->uh_sport = dhcps_port;
    dhcpsMsgOut.udp->uh_dport = dhcpc_port;
    dhcpsMsgOut.udp->uh_ulen = htons (off_extopt + DFLTDHCPLEN + UDPHL);
    dhcpsMsgOut.udp->uh_sum = get_udpsum (dhcpsMsgOut.ip, dhcpsMsgOut.udp);

    dhcpsMsgOut.ip->ip_v = IPVERSION;
    dhcpsMsgOut.ip->ip_hl = IPHL >> 2;
    dhcpsMsgOut.ip->ip_tos = 0;
    dhcpsMsgOut.ip->ip_len = htons (off_extopt + DFLTDHCPLEN + UDPHL + IPHL);
    dhcpsMsgOut.ip->ip_id = dhcpsMsgOut.udp->uh_sum;
    dhcpsMsgOut.ip->ip_off = htons (IP_DF);    /* XXX */
    dhcpsMsgOut.ip->ip_ttl = 0x20;            /* XXX */
    dhcpsMsgOut.ip->ip_p = IPPROTO_UDP;
    dhcpsMsgOut.ip->ip_sum = get_ipsum (dhcpsMsgOut.ip);

    sprintf (devName, "%s%d", ifp->name, ifp->unit);
    pIf = ifunit (devName);
    if (pIf == NULL)
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: couldn't access network interface %s.\n", 
                 (int)devName, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    if (sbufvec[1].iov_len == 0) 
        {
        result = dhcpsSend (pIf,
                            dhcpsMsgOut.dhcp->chaddr, dhcpsMsgOut.dhcp->hlen,
                            &dst, (char *)dhcpsMsgOut.ip, sbufvec[0].iov_len,
                            bcastFlag);
        if (result != OK)
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: couldn't send DHCP message.\n", 
                     0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }
        } 
    else 
        {
        /* Send message which includes extra options (exceeds default size). */

        msglen = sbufvec[0].iov_len + sbufvec[1].iov_len;
        result = dhcpsSend (pIf,
                            dhcpsMsgOut.dhcp->chaddr, dhcpsMsgOut.dhcp->hlen,
                            &dst, (char *)dhcpsMsgOut.ip, msglen, bcastFlag);
        if (result != OK)
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: couldn't send DHCP message.\n", 
                     0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }
        }
    return (0);
    }

/*******************************************************************************
*
* available_forbootp - check resource for issuance to BOOTP client
*
* This routine determines if a resource entry may be granted to a BOOTP
* client. Only entries which are specifically marked as available for
* BOOTP (with "albp=true") will be granted. Among those entries, they
* must be unused, expired, or manually assigned to the BOOTP client.
*
* RETURNS: TRUE if resource available, or FALSE otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int available_forbootp
    (
    struct dhcp_resource *res, 	/* pointer to lease descriptor */
    struct client_id *cid, 	/* pointer to client identifier */
    time_t curr_epoch 		/* current time, in seconds */
    )
    {
    if (res->allow_bootp == FALSE)
        return (FALSE);

    /*
     * Allow resource use if currently unused, or in use by requesting client,
     * or if previous lease has expired. 
     */

    if (res->binding == NULL ||
        cidcmp (&res->binding->cid, cid) ||
        (res->binding->expire_epoch != 0xffffffff &&
         res->binding->expire_epoch < curr_epoch)) 
        return (TRUE);

    return (FALSE);
    }

/*******************************************************************************
*
* choose_forbootp - select resource for BOOTP client
*
* This routine retrieves a dhcp_resource structure which may be used
* for a BOOTP client. The corresponding IP address must be on the
* same subnet as the requesting client. The client must also provide a
* matching client identifier, if included in the server's database entry.
*
* RETURNS: Matching resource, or NULL if none or not available.
*
* ERRNO: N/A
*
* NOMANUAL
*/

/*
 * choose a new address for a bootp client
 */
static struct dhcp_resource * choose_forbootp
    (
    struct client_id *cid, 	/* pointer to client identifier */
    time_t curr_epoch 		/* current time, in seconds */
    ) 
    {
    struct dhcp_resource *res = NULL;
    struct dhcp_resource *offer = NULL;
    struct hash_member *resptr = NULL;

    resptr = reslist;
    while (resptr != NULL) 
       {
       res = (struct dhcp_resource *) resptr->data;

       /* if it is dummy entry, skip it */
       if (res->ip_addr.s_addr == 0) 
           {
           resptr = resptr->next;
           continue;
           }

       /* check the resource for valid subnet and availability. */

       if (cid->subnet.s_addr == 
                (res->ip_addr.s_addr & res->subnet_mask.s_addr) &&
	   available_forbootp (res, cid, curr_epoch)) 
           {

           if (dhcpsMsgIn.dhcp->ciaddr.s_addr != 0) 
               {
	       offer = res;
	       break;
               }

           /* Specify DHCPDISCOVER to force generation of ICMP request. */

           else if (icmp_check(DHCPDISCOVER, &res->ip_addr) == GOOD) 
               {
	       offer = res;
	       break;
               }
           else 
	       turnoff_bind (res->binding);

           }

       resptr = resptr->next;
       }
#ifdef DHCPS_DEBUG
    if (offer == NULL) 
        logMsg ("Warning: BOOTP - No available addresses in the pool.\n",
                 0, 0, 0, 0, 0, 0);
#endif
    return (offer);
    }

/*******************************************************************************
*
* construct_bootp - make an outgoing BOOTP message
*
* This routine creates a BOOTP reply if an address pool entry is found for
* a requesting BOOTP client. The corresponding resource is granted with an
* infinite lease, since BOOTP clients require statically configured 
* parameters.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

static void construct_bootp
    (
    struct dhcp_resource *res 	/* pointer to lease descriptor for offer */
    ) 
    { 
    int i = 0;
    char inserted [32];
    int result;

    bzero (inserted, sizeof (inserted));

    clean_sbuf ();
    overload = 0;
    dhcpsMsgOut.dhcp->op = BOOTREPLY;
    dhcpsMsgOut.dhcp->htype = dhcpsMsgIn.dhcp->htype;
    dhcpsMsgOut.dhcp->hlen = dhcpsMsgIn.dhcp->hlen;
    dhcpsMsgOut.dhcp->hops = 0;
    dhcpsMsgOut.dhcp->xid = dhcpsMsgIn.dhcp->xid;
    dhcpsMsgOut.dhcp->secs = 0;
    if (dhcpsMsgIn.dhcp->giaddr.s_addr != 0) 
        dhcpsMsgOut.dhcp->flags = dhcpsMsgIn.dhcp->flags;
    else
        dhcpsMsgOut.dhcp->flags = 0;

    dhcpsMsgOut.dhcp->giaddr.s_addr = dhcpsMsgIn.dhcp->giaddr.s_addr;
    bcopy (dhcpsMsgIn.dhcp->chaddr, dhcpsMsgOut.dhcp->chaddr, 
           dhcpsMsgIn.dhcp->hlen);

    dhcpsMsgOut.dhcp->yiaddr.s_addr = res->ip_addr.s_addr;
    if (ISSET (res->valid, S_SIADDR)) 
        dhcpsMsgOut.dhcp->siaddr.s_addr = res->siaddr.s_addr;
    else 
        dhcpsMsgOut.dhcp->siaddr.s_addr = 0;
    if (ISSET (res->valid, S_SNAME)) 
        {
        strncpy (dhcpsMsgOut.dhcp->sname, res->sname, MAX_SNAME);
        dhcpsMsgOut.dhcp->sname [MAX_SNAME - 1] = '\0';
        }
    if (ISSET (res->valid, S_FILE)) 
        {
        strncpy (dhcpsMsgOut.dhcp->file, res->file, MAX_FILE);
        dhcpsMsgOut.dhcp->file [MAX_FILE - 1] = '\0';
        }

    /* insert magic cookie */

    bcopy ((char *)dhcpCookie, dhcpsMsgOut.dhcp->options, MAGIC_LEN);
    off_options = MAGIC_LEN;
    off_extopt = 0;

    /* insert subnet mask */

    result = insert_opt (res, 0xffffffff, _DHCP_SUBNET_MASK_TAG, inserted, 
                         PASSIVE);
#ifdef DHCPS_DEBUG
    if (result == E_NOMORE) 
        logMsg ("No space left in BOOTP options field.\n", 0, 0, 0, 0, 0, 0);
#endif

    /*
     * insert any binding options that differ from "Host requirement RFC" 
     * defaults 
     */

    for (i = 0; i < _DHCP_LAST_OPTION; i++) 
        {
        if (ISCLR (inserted, i)) 
            if (insert_opt (res, 0xffffffff, i, inserted, PASSIVE) == E_NOMORE)
                {
#ifdef DHCPS_DEBUG
                logMsg ("No space left in BOOTP options field.\n",
                         0, 0, 0, 0, 0, 0);
#endif
                break;
                }
        }
    return;
    }

/*******************************************************************************
*
* bootp - handle BOOTP requests
*
* This routine examines requests from BOOTP clients, selects an available
* resource (if any), calculates the length of the lease, and sends the
* appropriate offer BOOTP reply to the client.
*
* RETURNS: 0 if processing successful, or -1 on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int bootp 
    (
    struct if_info *ifp    /* pointer to descriptor of receiving interface */
    )
    {
    char addrstr [sizeof (BRDCSTSTR)];
    struct client_id cid;
    struct dhcp_binding *binding = NULL;
    struct dhcp_resource *res = NULL;
    time_t curr_epoch = 0;

    bzero ( (char *)&cid, sizeof (cid));
    bcopy (BRDCSTSTR, addrstr, sizeof (BRDCSTSTR));

#ifdef NOBOOTP
  return (0);
#endif

    if (dhcpTime (&curr_epoch) == -1) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: couldn't get timestamp processing bootp message.\n",
                 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }
    get_cid (dhcpsMsgIn.dhcp, rdhcplen, &cid);

    /* Critical section with dhcpsLeaseEntryAdd(). */

    semTake (dhcpsMutexSem, WAIT_FOREVER);
    if (get_subnet (dhcpsMsgIn.dhcp, rdhcplen, &cid.subnet, ifp) != 0)
        { 
        semGive (dhcpsMutexSem);
        return (-1);
        }

    maxoptlen = BOOTPOPTLEN;

    /* search with haddr (haddr is same as client identfier) */

    binding = (struct dhcp_binding *)hash_find (&cidhashtable, cid.id, 
                                                cid.idlen, bindcidcmp, &cid);
    if (binding != NULL) 
        {
        if (cidcmp (&binding->cid, &cid))
            res = binding->res;
        }
    if (res == NULL && (res = choose_forbootp(&cid, curr_epoch)) == NULL)
        {
        semGive (dhcpsMutexSem);
        return (-1);
        }

    if (update_db (BOOTP, &cid, res, 0xffffffff, curr_epoch) != 0)
        {
        semGive (dhcpsMutexSem);
        return (-1);
        }

    semGive (dhcpsMutexSem);

    /* binding is complete. Update entry flags. */

    res->binding->flag |= (COMPLETE_ENTRY | BOOTP_ENTRY);

    construct_bootp (res);
    send_bootp (ifp);
    inet_ntoa_b (res->ip_addr,addrstr);
#ifdef DHCPS_DEBUG
    logMsg ("Sending BOOTP reply to client(IP:%s, cid:\"%s\").\n",
	     (int)addrstr, (int)cidtos (&res->binding->cid, 1), 0, 0, 0, 0);
#endif

    return (0);
    }

/*******************************************************************************
*
* send_bootp - transmit a BOOTP reply
*
* This routine sends messages formulated by an earlier construct_bootp() call
* directly to the destination address, if available, or as a link layeri
* broadcast on the client's subnet.
*
* RETURNS: 0 if message sent, or -1 otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int send_bootp
    (
    struct if_info *ifp 	/* descriptor for transmission interface */
    )
    {
    int buflen = 0;
    char devName [10];
    struct sockaddr_in srcaddr; 
    struct sockaddr_in dstaddr;
    struct ifnet *pIf;    
    BOOL bcastFlag;
    int result;

    bzero ((char *)&srcaddr, sizeof (srcaddr));
    bzero ((char *)&dstaddr, sizeof (dstaddr));

    dstaddr.sin_len = sizeof (struct sockaddr_in);
    dstaddr.sin_family = AF_INET;

    if (off_options < BOOTPOPTLEN) 
        dhcpsMsgOut.dhcp->options [off_options] = _DHCP_END_TAG;

    /* if received message was relayed from relay agent,
       send reply from normal socket */
    if (dhcpsMsgIn.dhcp->giaddr.s_addr != 0) 
        {
        if (dhcpsMsgIn.dhcp->ciaddr.s_addr == 0 ||
            dhcpsMsgIn.dhcp->ciaddr.s_addr != dhcpsMsgOut.dhcp->yiaddr.s_addr) 
            {
            dstaddr.sin_port = dhcps_port;
            bcopy( (char *)&dhcpsMsgIn.dhcp->giaddr, 
                   (char *)&dstaddr.sin_addr, sizeof (u_long));
            } 
        else 
            {
            dstaddr.sin_port = dhcpc_port;
            bcopy ( (char *)&dhcpsMsgOut.dhcp->yiaddr, 
                    (char *)&dstaddr.sin_addr, sizeof (u_long));
            }

        buflen = DFLTBOOTPLEN;
        if (setsockopt (dhcpsIntfaceList->fd, SOL_SOCKET, SO_SNDBUF, 
            (char *)&buflen, sizeof (buflen)) < 0) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: Couldn't set transmit buffer size for BOOTP.\n", 
                     0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }

        /*
         * This will send to any network interface, 
         * independent of the local address it is bound to.
         */

        if (sendto (dhcpsIntfaceList->fd, (caddr_t)dhcpsMsgOut.dhcp, buflen,
                    0, (struct sockaddr *)&dstaddr, sizeof (dstaddr)) < 0) 
            {
#ifdef DHCPS_DEBUG
            logMsg ("Warning: Couldn't send BOOTP message.\n", 
                     0, 0, 0, 0, 0, 0);
#endif
            return (-1);
            }
        return (0);
        }

    /* if directly received packet.... */

    /* Set destination address and fill pseudo header to calculate checksum */

    bcopy ( (char *)&ifp->ipaddr.s_addr, 
            (char *)&dhcpsMsgOut.ip->ip_src, sizeof (u_long));

    if (ISBRDCST (dhcpsMsgIn.dhcp->flags) || 
        dhcpsMsgIn.dhcp->ciaddr.s_addr != dhcpsMsgOut.dhcp->yiaddr.s_addr)
        {
        dstaddr.sin_addr.s_addr = dhcpsMsgOut.ip->ip_dst.s_addr = 0xffffffff;
        bcastFlag = TRUE;
        }
    else
        {
        dhcpsMsgOut.ip->ip_dst.s_addr = dhcpsMsgOut.dhcp->yiaddr.s_addr;
        dstaddr.sin_addr.s_addr = dhcpsMsgOut.dhcp->yiaddr.s_addr;
        bcastFlag = FALSE;
        }

    dhcpsMsgOut.udp->uh_sport = dhcps_port;
    dhcpsMsgOut.udp->uh_dport = dhcpc_port;
    dhcpsMsgOut.udp->uh_ulen = htons (DFLTBOOTPLEN + UDPHL);
    dhcpsMsgOut.udp->uh_sum = get_udpsum (dhcpsMsgOut.ip, dhcpsMsgOut.udp);

    dhcpsMsgOut.ip->ip_v = IPVERSION;
    dhcpsMsgOut.ip->ip_hl = IPHL >> 2;
    dhcpsMsgOut.ip->ip_tos = 0;
    dhcpsMsgOut.ip->ip_len = htons (DFLTBOOTPLEN + UDPHL + IPHL);
    dhcpsMsgOut.ip->ip_id = dhcpsMsgOut.udp->uh_sum;
    dhcpsMsgOut.ip->ip_off = htons (IP_DF);    /* XXX */
    dhcpsMsgOut.ip->ip_ttl = 0x20;            /* XXX */
    dhcpsMsgOut.ip->ip_p = IPPROTO_UDP;
    dhcpsMsgOut.ip->ip_sum = get_ipsum (dhcpsMsgOut.ip);

    buflen = DFLTBOOTPLEN + UDPHL + IPHL + ETHERHL;

    sprintf (devName, "%s%d", ifp->name, ifp->unit);
    pIf = ifunit (devName);
    if (pIf == NULL)
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: couldn't access network interface %s.\n", 
                 (int)devName, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    result = dhcpsSend (pIf,
                        dhcpsMsgOut.dhcp->chaddr, dhcpsMsgOut.dhcp->hlen,
                        &dstaddr, (char *)dhcpsMsgOut.ip, buflen, bcastFlag);
    if (result != OK)
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: couldn't send BOOTP message.\n", 0, 0, 0, 0, 0, 0);
#endif
        return (-1);
        }

    return (0);
    }

/*******************************************************************************
*
* ins_ip - insert options containing a single IP address
*
* This routine inserts any available options in the selected resource which
* consist of a single IP address as options in an outgoing DHCP message.
*
* RETURNS: 0 if option inserted, or negative value on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int ins_ip
    (
    struct dhcp_resource *res, 	/* pointer to lease descriptor */
    u_long lease, 		/* lease duration, in seconds */
    int tagnum, 		/* tag value of option */
    char *inserted, 		/* bitmap of inserted options */
    char flag 			/* if ACTIVE, marks non-default options */
    )
    {
    u_long *addr = 0;
    char option[6];
    int symbol = 0;
    int retval = 0;

    bzero (option, sizeof (option));

    /* Access offset for appropriate data. */

    switch (tagnum) 
        {
        case _DHCP_SUBNET_MASK_TAG:
            symbol = S_SUBNET_MASK; 
            addr = &res->subnet_mask.s_addr;
            break;
        case _DHCP_SWAP_SERVER_TAG:
            symbol = S_SWAP_SERVER;
            addr = &res->swap_server.s_addr;
            break;
        case _DHCP_BRDCAST_ADDR_TAG:
            symbol = S_BRDCAST_ADDR;
            addr = &res->brdcast_addr.s_addr;
            break;
        case _DHCP_ROUTER_SOLICIT_TAG:
            symbol = S_ROUTER_SOLICIT; 
            addr = &res->router_solicit.s_addr;
            break;
        default:
            return (-1);
        }
   
    /* Copy data if valid. */ 

    if ( (flag == PASSIVE && ISSET (res->valid, symbol)) ||
         (flag == ACTIVE && ISSET (res->active, symbol))) 
        {
        option[0] = tagnum;
        option[1] = 4;               /* Length of option data, in bytes. */
        bcopy ( (char *)addr, &option[2], 4);
        if ( (retval = insert_it (option)) == 0)
            SETBIT (inserted, tagnum);
        }
    return (retval);
    }

/*******************************************************************************
*
* ins_ips - insert options containing multiple IP addresses
*
* This routine inserts any available options in the selected resource which
* consist of one or more IP addresses as options in an outgoing DHCP message.
*
* RETURNS: 0 if option inserted, or negative value on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int ins_ips
    (
    struct dhcp_resource *res, 	/* pointer to lease descriptor */
    u_long lease, 		/* lease duration, in seconds */
    int tagnum, 		/* tag value of option */
    char *inserted, 		/* bitmap of inserted options */
    char flag 			/* if ACTIVE, marks non-default options */
    )
    {
    struct in_addrs *addr = NULL;
    char option [254];
    int symbol = 0;
    int retval = 0;
    int i = 0;

    bzero (option, sizeof (option));

    switch(tagnum) 
        {
        case _DHCP_ROUTER_TAG:
            symbol = S_ROUTER;
            addr = &res->router;
            break;
        case _DHCP_TIME_SERVER_TAG:
            symbol = S_TIME_SERVER;
            addr = &res->time_server;
            break;
        case _DHCP_NAME_SERVER_TAG:
            symbol = S_NAME_SERVER; 
            addr = &res->name_server;
            break;
        case _DHCP_DNS_SERVER_TAG:
            symbol = S_DNS_SERVER; 
            addr = &res->dns_server;
            break;
        case _DHCP_LOG_SERVER_TAG:
            symbol = S_LOG_SERVER; 
            addr = &res->log_server;
            break;
        case _DHCP_COOKIE_SERVER_TAG:
            symbol = S_COOKIE_SERVER; 
            addr = &res->cookie_server;
            break;
        case _DHCP_LPR_SERVER_TAG:
            symbol = S_LPR_SERVER; 
            addr = &res->lpr_server;
            break;
        case _DHCP_IMPRESS_SERVER_TAG:
            symbol = S_IMPRESS_SERVER; 
            addr = &res->impress_server;
            break;
        case _DHCP_RLS_SERVER_TAG:
            symbol = S_RLS_SERVER; 
            addr = &res->rls_server;
            break;
        case _DHCP_NIS_SERVER_TAG:
            symbol = S_NIS_SERVER; 
            addr = &res->nis_server;
            break;
        case _DHCP_NTP_SERVER_TAG:
            symbol = S_NTP_SERVER; 
            addr = &res->ntp_server;
            break;
        case _DHCP_NBN_SERVER_TAG:
            symbol = S_NBN_SERVER; 
            addr = &res->nbn_server;
            break;
        case _DHCP_NBDD_SERVER_TAG:
            symbol = S_NBDD_SERVER; 
            addr = &res->nbdd_server;
            break;
        case _DHCP_XFONT_SERVER_TAG:
            symbol = S_XFONT_SERVER; 
            addr = &res->xfont_server;
            break;
        case _DHCP_XDISPLAY_MANAGER_TAG:
            symbol = S_XDISPLAY_MANAGER; 
            addr = &res->xdisplay_manager;
            break;
        case _DHCP_NISP_SERVER_TAG:
            symbol = S_NISP_SERVER; 
            addr = &res->nisp_server;
            break;
        case _DHCP_MOBILEIP_HA_TAG:
            symbol = S_MOBILEIP_HA; 
            addr = &res->mobileip_ha;
            break;
        case _DHCP_SMTP_SERVER_TAG:
            symbol = S_SMTP_SERVER; 
            addr = &res->smtp_server;
            break;
        case _DHCP_POP3_SERVER_TAG:
            symbol = S_POP3_SERVER; 
            addr = &res->pop3_server;
            break;
        case _DHCP_NNTP_SERVER_TAG:
            symbol = S_NNTP_SERVER; 
            addr = &res->nntp_server;
            break;
        case _DHCP_DFLT_WWW_SERVER_TAG:
            symbol = S_DFLT_WWW_SERVER; 
            addr = &res->dflt_www_server;
            break;
        case _DHCP_DFLT_FINGER_SERVER_TAG:
            symbol = S_DFLT_FINGER_SERVER; 
            addr = &res->dflt_finger_server;
            break;
        case _DHCP_DFLT_IRC_SERVER_TAG:
            symbol = S_DFLT_IRC_SERVER; 
            addr = &res->dflt_irc_server;
            break;
        case _DHCP_STREETTALK_SERVER_TAG:
            symbol = S_STREETTALK_SERVER; 
            addr = &res->streettalk_server;
            break;
        case _DHCP_STDA_SERVER_TAG:
            symbol = S_STDA_SERVER; 
            addr = &res->stda_server;
            break;
        default:
            return (-1);
        }

    if ( (flag == PASSIVE && ISSET (res->valid, symbol)) ||
         (flag == ACTIVE && ISSET (res->active, symbol))) 
        {
        option[0] = tagnum;
        option[1] = addr->num * 4;   /* Length of data, in bytes. */
        for (i = 0; i < addr->num; i++)
            bcopy ((char *)&addr->addr[i].s_addr, &option [i * 4 + 2], 4);
        if ( (retval = insert_it (option)) == 0)
            SETBIT (inserted, tagnum);
        }
    return (retval);
    }

/*******************************************************************************
*
* ins_ippairs - insert options containing multiple IP address pairs
*
* This routine inserts any available options in the selected resource which
* consist of one or more pairs of IP addresses as options in an outgoing 
* DHCP message.
*
* RETURNS: 0 if option inserted, or negative value on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int ins_ippairs
    (
    struct dhcp_resource *res, 	/* pointer to lease descriptor */
    u_long lease, 		/* lease duration, in seconds */
    int tagnum, 		/* tag value of option */
    char *inserted, 		/* bitmap of inserted options */
    char flag 			/* if ACTIVE, marks non-default options */
    )
    {
    struct ip_pairs *pair = NULL;
    char option [254];
    int symbol = 0;
    int retval = 0;
    int i = 0;

    bzero (option, sizeof (option));

    switch (tagnum) 
        {
        case _DHCP_POLICY_FILTER_TAG:
            symbol = S_POLICY_FILTER;
            pair = &res->policy_filter;
            break;
        case _DHCP_STATIC_ROUTE_TAG:
            symbol = S_STATIC_ROUTE; 
            pair = &res->static_route;
            break;
        default:
            return (-1);
        }

    if ( (flag == PASSIVE && ISSET (res->valid, symbol)) ||
         (flag == ACTIVE && ISSET (res->active, symbol))) 
        {
        option[0] = tagnum;
        option[1] = pair->num * 8;
        for (i = 0; i < pair->num; i++) 
            {
            bcopy ( (char *)&pair->addr1[i].s_addr, &option [i * 8 + 2], 4);
            bcopy ( (char *)&pair->addr2[i].s_addr, &option [i * 8 + 6], 4);
            }
        if ( (retval = insert_it (option)) == 0)
            SETBIT (inserted, tagnum);
        }
    return (retval);
    }

/*******************************************************************************
*
* ins_long - insert options containing long integers
*
* This routine inserts any available options in the selected resource which
* consist of a long (4 byte) value as options in an outgoing DHCP message.
*
* RETURNS: 0 if option inserted, or negative value on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int ins_long
    (
    struct dhcp_resource *res, 	/* pointer to lease descriptor */
    u_long lease, 		/* lease duration, in seconds */
    int tagnum, 		/* tag value of option */
    char *inserted, 		/* bitmap of inserted options */
    char flag 			/* if ACTIVE, marks non-default options */
    )
    {
    long *num = NULL;
    char option[6];
    int symbol = 0;
    int retval = 0;

    bzero (option, sizeof (option));

    switch (tagnum) 
        {
        case _DHCP_TIME_OFFSET_TAG:
            symbol = S_TIME_OFFSET; 
            num = &res->time_offset;
            break;
        case _DHCP_MTU_AGING_TIMEOUT_TAG:
            symbol = S_MTU_AGING_TIMEOUT; 
            num = (long *)&res->mtu_aging_timeout;
            break;
        case _DHCP_ARP_CACHE_TIMEOUT_TAG:
            symbol = S_ARP_CACHE_TIMEOUT; 
            num = (long *)&res->arp_cache_timeout;
            break;
        case _DHCP_KEEPALIVE_INTERVAL_TAG:
            symbol = S_KEEPALIVE_INTER; 
            num = (long *)&res->keepalive_inter;
            break;
        default:
            return (-1);
        }
    
    if ( (flag == PASSIVE && ISSET (res->valid, symbol)) ||
         (flag == ACTIVE && ISSET (res->active, symbol)))
        {
        option [0] = tagnum;
        option [1] = 4;
        bcopy ( (char *)num, &option[2], 4);
        if ( (retval = insert_it (option)) == 0)
            SETBIT (inserted, tagnum);
        }
    return (retval);
    }

/*******************************************************************************
*
* ins_short - insert options containing short integers 
*
* This routine inserts any available options in the selected resource which
* consist of a short (2 byte) value as options in an outgoing DHCP message.
*
* RETURNS: 0 if option inserted, or negative value on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int ins_short
    (
    struct dhcp_resource *res, 	/* pointer to lease descriptor */
    u_long lease, 		/* lease duration, in seconds */
    int tagnum, 		/* tag value of option */
    char *inserted, 		/* bitmap of inserted options */
    char flag 			/* if ACTIVE, marks non-default options */
    )
    {
    short *num = NULL;
    char option[4];
    int symbol = 0;
    int retval = 0;

    bzero (option, sizeof (option));
 
    switch (tagnum) 
        {
        case _DHCP_BOOTSIZE_TAG:
            symbol = S_BOOTSIZE; 
            num = (short *)&res->bootsize;
            break;
        case _DHCP_MAX_DGRAM_SIZE_TAG:
            symbol = S_MAX_DGRAM_SIZE; 
            num = (short *) &res->max_dgram_size;
            break;
        case _DHCP_IF_MTU_TAG:
            symbol = S_IF_MTU; 
            num = (short *)&res->intf_mtu;
            break;
        default:
            return (-1);
        }

    if ( (flag == PASSIVE && ISSET (res->valid, symbol)) ||
         (flag == ACTIVE && ISSET (res->active, symbol))) 
        {
        option[0] = tagnum;
        option[1] = 2;
        bcopy ((char *)num, &option[2], 2);
        if ( (retval = insert_it (option)) == 0)
            SETBIT (inserted, tagnum);
        }
    return (retval);
    }

/*******************************************************************************
*
* ins_octet - insert options containing single bytes
*
* This routine inserts any available options in the selected resource which
* consist of a 1 byte value as options in an outgoing DHCP message.
*
* RETURNS: 0 if option inserted, or negative value on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int ins_octet
    (
    struct dhcp_resource *res, 	/* pointer to lease descriptor */
    u_long lease, 		/* lease duration, in seconds */
    int tagnum, 		/* tag value of option */
    char *inserted, 		/* bitmap of inserted options */
    char flag 			/* if ACTIVE, marks non-default options */
    )
    {
    char num = 0;
    char option[3];
    int symbol = 0;
    int retval = 0;

    bzero (option, sizeof (option));

    switch (tagnum) 
        {
        case _DHCP_IP_FORWARD_TAG:
            symbol = S_IP_FORWARD; 
            num = res->ip_forward;
            break;
        case _DHCP_NONLOCAL_SRCROUTE_TAG:
            symbol = S_NONLOCAL_SRCROUTE; 
            num = res->nonlocal_srcroute;
            break;
        case _DHCP_DEFAULT_IP_TTL_TAG:
            symbol = S_DEFAULT_IP_TTL; 
            num = res->default_ip_ttl;
            break;
        case _DHCP_ALL_SUBNET_LOCAL_TAG:
            symbol = S_ALL_SUBNET_LOCAL; 
            num = res->all_subnet_local;
            break;
        case _DHCP_MASK_DISCOVER_TAG:
            symbol = S_MASK_DISCOVER; 
            num = res->mask_discover;
            break;
        case _DHCP_MASK_SUPPLIER_TAG:
            symbol = S_MASK_SUPPLIER; 
            num = res->mask_supplier;
            break;
        case _DHCP_ROUTER_DISCOVER_TAG:
            symbol = S_ROUTER_DISCOVER; 
            num = res->router_discover;
            break;
        case _DHCP_TRAILER_TAG:
            symbol = S_TRAILER; 
            num = res->trailer;
            break;
        case _DHCP_ETHER_ENCAP_TAG:
            symbol = S_ETHER_ENCAP; 
            num = res->ether_encap;
            break;
        case _DHCP_DEFAULT_TCP_TTL_TAG:
            symbol = S_DEFAULT_TCP_TTL; 
            num = res->default_tcp_ttl;
            break;
        case _DHCP_KEEPALIVE_GARBAGE_TAG:
            symbol = S_KEEPALIVE_GARBA; 
            num = res->keepalive_garba;
            break;
        case _DHCP_NB_NODETYPE_TAG:
            symbol = S_NB_NODETYPE; 
            num = res->nb_nodetype;
            break;
        default:
            return (-1);
        }
    
    if ( (flag == PASSIVE && ISSET (res->valid, symbol)) ||
         (flag == ACTIVE && ISSET (res->active, symbol))) 
        {
        option[0] = tagnum;
        option[1] = 1;
        option[2] = num;
        if ( (retval = insert_it (option)) == 0)
            SETBIT(inserted, tagnum);
        }
    return (retval);
    }

/*******************************************************************************
*
* ins_str - insert options containing strings
*
* This routine inserts any available options in the selected resource which
* consist of a NULL terminated string as options in an outgoing DHCP message.
*
* RETURNS: 0 if option inserted, or negative value on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

/*
 * insert string
 */
/* ARGSUSED */
static int ins_str
    (
    struct dhcp_resource *res, 	/* pointer to lease descriptor */
    u_long lease, 		/* lease duration, in seconds */
    int tagnum, 		/* tag value of option */
    char *inserted, 		/* bitmap of inserted options */
    char flag 			/* if ACTIVE, marks non-default options */
    )
    {
    char *str = NULL;
    char option [258];
    int symbol = 0;
    int retval = 0;
    int i = 0;

    bzero (option, sizeof (option));
 
    switch (tagnum) 
        {
        case _DHCP_HOSTNAME_TAG:
            symbol = S_HOSTNAME; 
            str = res->hostname;
            break;
        case _DHCP_MERIT_DUMP_TAG:
            symbol = S_MERIT_DUMP; 
            str = res->merit_dump;
            break;
        case _DHCP_DNS_DOMAIN_TAG:
            symbol = S_DNS_DOMAIN; 
            str = res->dns_domain;
            break;
        case _DHCP_ROOT_PATH_TAG:
            symbol = S_ROOT_PATH; 
            str = res->root_path;
            break;
        case _DHCP_EXTENSIONS_PATH_TAG:
            symbol = S_EXTENSIONS_PATH; 
            str = res->extensions_path;
            break;
        case _DHCP_NIS_DOMAIN_TAG:
            symbol = S_NIS_DOMAIN; 
            str = res->nis_domain;
            break;
        case _DHCP_NB_SCOPE_TAG:
            symbol = S_NB_SCOPE; 
            str = res->nb_scope;
            break;
        case _DHCP_NISP_DOMAIN_TAG:
            symbol = S_NISP_DOMAIN; 
            str = res->nisp_domain;
            break;
        default:
            return (-1);
        }

    if ( (flag == PASSIVE && ISSET (res->valid, symbol)) ||
         (flag == ACTIVE && ISSET (res->active, symbol))) 
        {
        option[0] = tagnum;
        option[1] = ( (i = strlen (str)) > MAXOPT) ? MAXOPT : i;
        bcopy (str, &option[2], option[1]);
        if ( (retval = insert_it (option)) == 0)
            SETBIT (inserted, tagnum);
        }
    return (retval);
    }

/*******************************************************************************
*
* ins_dht - insert lease timers
*
* This routine inserts the values for T1 and T2 into the options of an
* outgoing DHCP message.
*
* RETURNS: 0 if options inserted, or negative value on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int ins_dht
    (
    struct dhcp_resource *res, 	/* pointer to lease descriptor */
    u_long lease, 		/* lease duration, in seconds */
    int tagnum, 		/* tag value of option */
    char *inserted, 		/* bitmap of inserted options */
    char flag 			/* if ACTIVE, marks non-default options */
    )
    {
    char option[6];
    int symbol = 0;
    int retval = 0;
    long num = 0;

    bzero (option, sizeof (option));

    switch (tagnum) 
        {
        case _DHCP_T1_TAG:
            symbol = S_DHCP_T1;
            num = htonl (lease * (res->dhcp_t1 + (rand () & 0x03)) / 1000);
            break;
        case _DHCP_T2_TAG:
            symbol = S_DHCP_T2;
            num = htonl (lease * (res->dhcp_t2 + (rand () & 0x03)) / 1000);
            break;
        default:
            return (-1);
        }
    
    if ( (flag == PASSIVE && ISSET (res->valid, symbol)) ||
         (flag == ACTIVE && ISSET (res->active, symbol))) 
        {
        option[0] = tagnum;
        option[1] = 4;
        bcopy ( (char *)&num, &option[2], option[1]);
        if ( (retval = insert_it (option)) == 0)
            SETBIT (inserted, tagnum);
        }
    return (retval);
    }

/*******************************************************************************
*
* ins_mtpt - insert MTU plateau table options
*
* This routine inserts the values specified for the MTU table as options in an 
* outgoing DHCP message.
*
* RETURNS: 0 if option inserted, or negative value on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

/*
 * insert mtu_plateau_table
 */
static int ins_mtpt
    (
    struct dhcp_resource *res, 	/* pointer to lease descriptor */
    u_long lease, 		/* lease duration, in seconds */
    int tagnum, 		/* tag value of option */
    char *inserted, 		/* bitmap of inserted options */
    char flag 			/* if ACTIVE, marks non-default options */
    )
    {
    char option [256];
    int retval = 0;
    int i = 0;

    bzero (option, sizeof (option));

    if (tagnum != _DHCP_MTU_PLATEAU_TABLE_TAG) 
        return (-1);

    if ( (flag == PASSIVE && ISSET (res->valid, S_MTU_PLATEAU_TABLE)) ||
         (flag == ACTIVE && ISSET (res->active, S_MTU_PLATEAU_TABLE))) 
        {
        option[0] = tagnum;
        option[1] = res->mtu_plateau_table.num * 2;
        for (i = 0; i < res->mtu_plateau_table.num; i++)
            bcopy ( (char *)&res->mtu_plateau_table.shorts[i], 
                   &option [i * 2 + 2], 2);
        if ( (retval = insert_it (option)) == 0)
            SETBIT (inserted, tagnum);
        }
    return (retval);
    }

/*******************************************************************************
*
* insert_opt - add an option to a outgoing message
*
* This routine multiplexes on the option type, then calls the appropriate
* insertion routine to store the given option in the outgoing message buffer.
*
* RETURNS: 0 if option inserted, or negative value on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int insert_opt
    (
    struct dhcp_resource *res, 	/* pointer to lease descriptor */
    u_long lease, 		/* lease duration, in seconds */
    int tagnum, 		/* tag value of option */
    char *inserted, 		/* bitmap of inserted options */
    char flag 			/* if ACTIVE, marks non-default options */
    )
    {
    if (tagnum < _DHCP_PAD_TAG || tagnum > _DHCP_LAST_OPTION || 
        ins_opt [tagnum] == NULL)
        return (-1);

    return ( (*ins_opt [tagnum]) (res, lease, tagnum, inserted, flag));
    }

/*******************************************************************************
*
* insert_it - transfer data to outgoing message buffer
*
* This routine is called by all the type-specific insertion routines defined
* above to store option data in the appropriate buffer for the outgoing
* message. If possible, the message is added to the options field of the
* standard DHCP message. If that field is full, the sname or file field are
* used to store the option, if available. Otherwise, the option is stored in
* an overflow buffer containing the excess bytes the client can receive, if
* any.
*
* RETURNS: 0 if option stored, or E_NOMORE if buffers not available.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int insert_it
    ( 
    char *opt 		/* pointer to buffer containing option */
    )
    {
    char len = 0;
    int done = 0;

    len = opt[1] + 2;   /* 2 == tag number and length field */
    if (off_options + len < maxoptlen && off_options + len < DFLTOPTLEN) 
        {
        bcopy (opt, &dhcpsMsgOut.dhcp->options [off_options], len);
        off_options += len;
        return (0);
        }
    else if ( (overload & FILE_ISOPT) != 0 && off_file + len < MAX_FILE) 
        {
        bcopy (opt, &dhcpsMsgOut.dhcp->file [off_file], len);
        off_file += len;
        return (0);
        }
    else if ((overload & SNAME_ISOPT) != 0 && off_sname + len < MAX_SNAME) 
        {
        bcopy (opt, &dhcpsMsgOut.dhcp->sname [off_sname], len);
        off_sname += len;
        return (0);
        }
    else if (len < maxoptlen - off_options - off_extopt) 
        {
        if (maxoptlen > DFLTOPTLEN) 
            {
            sbufvec[1].iov_len = maxoptlen - DFLTOPTLEN;

            /* off_options never exceeds DFLTOPTLEN. */

            done = DFLTOPTLEN - off_options;
            if (done > 0)           /* Only true for first overflow entry. */
                {
                bcopy (opt, &dhcpsMsgOut.dhcp->options[off_options], done);
                len -= done;
                off_options += done; /* off_options offset is now invalid. */
                }
            
            /* done = 0 for all but first overflow entries. */

            bcopy (&opt [done], &sbufvec[1].iov_base[off_extopt], len);
            off_extopt += len;
            return (0);
            }
        }

    /* Option not stored - no space found. */

    if ( (off_options + off_extopt >= maxoptlen) &&
         ((overload & FILE_ISOPT) == 0 || off_file >= MAX_FILE) &&
         ((overload & SNAME_ISOPT) == 0 || off_sname >= MAX_SNAME))
        return (E_NOMORE);


    return (-1);
    }

/*******************************************************************************
*
* cidcmp - compare client identifiers
*
* This routine checks if two client identifiers match. It is used before 
* assigning manual lease entries. The test fails if the identifiers do not
* match or if the client with the same identifier is on a new subnet.
*
* RETURNS: TRUE if identifier and subnet matches, or FALSE otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int cidcmp
    (
    struct client_id *cid1, 	/* pointer to assigned client ID */
    struct client_id *cid2 	/* pointer to requesting client ID */
    )
    {
    return (cid1->subnet.s_addr == cid2->subnet.s_addr &&
	    cid1->idtype == cid2->idtype && cid1->idlen == cid2->idlen &&
	    bcmp (cid1->id, cid2->id, cid1->idlen) == 0);
    }

/*******************************************************************************
*
* icmp_check - verify IP address not in use
*
* This routine attempts to verify a selected IP address is actually available
* before sending an offer to a client. It generates an ICMP request and 
* waits 1/2 of a second for a reply. If no reply is received, the test is 
* passed. At other times (after offers are sent), the test is automatically
* passed.
*
* RETURNS: GOOD if no response received or if test unneeded, or BAD otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int icmp_check
    (
    int msgtype, 		/* DHCP message type received */
    struct in_addr *ip 		/* IP address to test */
    )
    {
    struct sockaddr_in dst;
    struct sockaddr_in from;
    struct icmp *	sicmp = NULL;
    struct icmp *	ricmp = NULL;
    struct ip *		ipp = NULL;
    char rcvbuf [1024];
    char sndbuf [ICMP_MINLEN];
    int rlen = 0;
    int fromlen = 0;
    int i = 0;
    int sockfd = 0;
    u_short pid = 0;
#ifdef DHCPS_DEBUG
    char output [INET_ADDR_LEN];
#endif

#ifdef NOICMPCHK
    return (GOOD);
#endif
    if (msgtype != DHCPDISCOVER)
        return (GOOD);

    bzero ( (char *)&dst, sizeof (dst));
    bzero ( (char *)&from, sizeof (from));
    bzero (sndbuf, sizeof (sndbuf));
    bzero (rcvbuf, sizeof (rcvbuf));

    sicmp = (struct icmp *) sndbuf;
    pid = (short)taskIdSelf () & 0xffff;

    if ( (sockfd = socket (PF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: couldn't open socket in icmp_check()\n",
                 0, 0, 0, 0, 0, 0);
#endif
        return (GOOD);
        }

    delarp (ip, sockfd);
  
    i = 1;
    if (ioctl (sockfd, FIONBIO, (int)&i) < 0) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: couldn't set non-blocking I/O in icmp_check()\n",
                 0, 0, 0, 0, 0, 0);
#endif
        return(GOOD);
        }

    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = ip->s_addr;

    sicmp->icmp_type = ICMP_ECHO;
    sicmp->icmp_code = 0;
    sicmp->icmp_cksum = 0;
    sicmp->icmp_id = pid;
    sicmp->icmp_seq = 0;

    sicmp->icmp_cksum = checksum ( (u_short *)sndbuf, sizeof (sndbuf));
    fromlen = sizeof (from);

    i = sendto (sockfd, sndbuf, sizeof (sndbuf), 0,
                (struct sockaddr *) &dst, sizeof (dst));
    if (i < 0 || i != sizeof (sndbuf)) 
        {
#ifdef DHCPS_DEBUG
        logMsg ("Warning: Can't send icmp echo request.\n", 0, 0, 0, 0, 0, 0);
#endif
        return (GOOD);
        }

    /* Wait half a second for an ICMP reply */ 

    taskDelay (sysClkRateGet () / 2);

    FOREVER
        { 
        rlen = recvfrom (sockfd, rcvbuf, sizeof (rcvbuf), 0,
		         (struct sockaddr *)&from, &fromlen);
        if (rlen < 0)
            break;

        ipp = (struct ip *) rcvbuf;
        if (rlen < (ipp->ip_hl << 2) + ICMP_MINLEN) 
            {
            continue;
            }
        ricmp = (struct icmp *) (rcvbuf + (ipp->ip_hl << 2));
        if (ricmp->icmp_type != ICMP_ECHOREPLY) 
            {
            continue;
            }
        if (ricmp->icmp_id == pid) 
            {
            break;
            }
        }
    close (sockfd);

    if (ricmp != NULL && ricmp->icmp_id == pid) 
        {
        errno = 0;
#ifdef DHCPS_DEBUG
        inet_ntoa_b (*ip, output);
        logMsg ("Warning: IP address %s may be in use.\n", (int)output,
                 0, 0, 0, 0, 0);
#endif
        return (BAD);
        }

    return (GOOD);
    }


/*******************************************************************************
*
* free_bind - remove DHCP binding
*
* This routine removes the record of a lease binding from the internal linked
* list before either adding a new record (if an offer was accepted) or as a
* result of garbage collection, if the lease has expired.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int free_bind
    (
    struct hash_member *hash_m 		/* hash table entry to delete */
    )
    {
    struct dhcp_binding *bp = NULL;
    struct dhcp_binding *cbp = NULL;
    struct hash_member *current = NULL;
    struct hash_member *previous = NULL;

    bp = (struct dhcp_binding *)hash_m->data;
    previous = current = bindlist;

    /* Remove target element from binding database, if present. */

    while (current != NULL) 
       {
       cbp = (struct dhcp_binding *)current->data;
       if (cbp != NULL && cidcmp (&bp->cid, &cbp->cid) == TRUE)
           break;
       else 
           {
           previous = current;
           current = current->next;
           }
       }

    /* Mark end of list if given entry not found. */

    if (current == NULL) 
        {
        if (previous != NULL) 
            previous->next = NULL;
        } 

    /* Reset list pointers if given entry found. */

    else 
        {
        /* Set pointer for new head of list. */

        if (current == bindlist) 
            bindlist = current->next;
        else 
            /* Set pointer to skip target element within list. */

            if (previous != NULL) 
	        previous->next = current->next;
        free (current);             /* Remove target element. */
        }

    /* Remove target element from hash table. */

    if (bp->res != NULL) 
        {
        bp->res->binding = NULL;
        bp->res = NULL;
        }
    free (bp);
    free (hash_m);
    nbind--;

    return (0);
    }

/*******************************************************************************
*
* free_fake - simulate removal of DHCP binding
*
* This routine decreases the number of identified active bindings when the 
* server detects an IP address is unexpectedly in use. However, the record of 
* the active lease is not removed.
*
* RETURNS: 0, always.
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int free_fake
    (
    struct hash_member *hash_m
    )
    {
    nbind--;

    return (0);
    }

/*******************************************************************************
*
* garbage_collect - store a snapshot of known client states
*
* This timer-driven routine is called after each iteration of the 
* message-processing loop. If the time specified by GC_INTERVAL (currently
* 10 minutes) has elapsed since the previous call, the routine updates the
* server internal data structures. It accesses the user-supplied cache hook to 
* transfer a record of the active or offered leases to permanent storage. It 
* then removes all incomplete entries, and as many expired entries as necessary
* to reduce the internal table size below the maximum number of entries 
* (currently 500).
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

static void garbage_collect (void)
    {
#define MTOB(X)   ((struct dhcp_binding *)((X)->data))

    struct hash_member *bindptr = NULL,
                       *oldest = NULL;
    struct dhcp_binding *tmpptr;
    time_t curr_epoch = 0;
    static time_t prev_epoch = 0;

    if (dhcpTime (&curr_epoch) == -1) 
        return;

    /* Exit if interval has not expired. (Currently ten minutes). */
    if (curr_epoch - prev_epoch < GC_INTERVAL)
        return;

    prev_epoch = curr_epoch;

    /* Critical section with dhcpsLeaseEntryAdd(). */

    semTake (dhcpsMutexSem, WAIT_FOREVER);

    dump_bind_db ();    /* Write current bindings to permanent storage. */

    bindptr = bindlist;
    while (bindptr != NULL) 
       {
       tmpptr = MTOB (bindptr);
       if ((tmpptr->flag & COMPLETE_ENTRY) == 0 &&
	    tmpptr->temp_epoch < curr_epoch) 
           {
           hash_del (&cidhashtable, tmpptr->cid.id, tmpptr->cid.idlen, 
                     bindcidcmp, &tmpptr->cid, free_bind);
           }
       bindptr = bindptr->next;
       }

    /* Find and delete oldest expired entry if table exceeds size limit. */

    while (nbind > MAX_NBIND) 
        {
        bindptr = bindlist;
        while (bindptr != NULL) 
            {
            tmpptr = MTOB (bindptr);
            if ( (tmpptr->flag & COMPLETE_ENTRY) != 0 &&
	         tmpptr->expire_epoch != 0xffffffff &&
	         tmpptr->expire_epoch < curr_epoch &&
	         (oldest == NULL || 
                  tmpptr->expire_epoch < MTOB (oldest)->expire_epoch)) 
	        oldest = bindptr;
            bindptr = bindptr->next;
            }

        if (oldest == NULL)
            {
            semGive (dhcpsMutexSem);
            return;
            }
        else 
            {
            tmpptr = MTOB (oldest);
            hash_del (&cidhashtable, tmpptr->cid.id, tmpptr->cid.idlen, 
                      bindcidcmp, &tmpptr->cid, free_bind);
            oldest = NULL;
            }
        }
    semGive (dhcpsMutexSem);
    return;
    }

/*******************************************************************************
*
* nvttostr - convert NVT ASCII to strings
*
* This routine implements a limited NVT conversion which removes any embedded
* NULL characters from the input string.
*
* RETURNS: 0, always. 
*
* ERRNO: N/A
*
* NOMANUAL
*/

static int nvttostr
    (
    char *nvtstr, 	/* input buffer, possibly has embedded NULLs */
    char *str, 		/* output buffer for converted string */
    int length 		/* number of characters in input buffer */
    )
    {
    FAST int i = 0;
    FAST char *tmp = NULL;

    tmp = str;

    for (i = 0; i < length; i++) 
        if (nvtstr[i] != (char)NULL) 
            {
            *tmp = nvtstr[i];
            tmp++;
            }

    str [length] = '\0';
    return (0);
    }

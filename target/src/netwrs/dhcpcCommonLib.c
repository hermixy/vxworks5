/* dhcpcCommonLib.c - DHCP client interface shared code library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02a,26apr02,wap  Update documentation for dhcpcOptionAdd() routine (SPR
                 #73769)
01z,23apr02,wap  Use BPF_WORDALIGN() when retreiving multiple messages from a
                 BPF buffer (SPR #74215)
01y,06dec01,wap  Add NPT support
01x,16nov01,spm  added mod history fix from main branch, ver 01w, base 01v
01w,15oct01,rae  merge from truestack ver 01z, base 01q (SPR #65264)
01v,24may01,mil  Bump up dhcp read task stack size to 5000.
01u,17nov00,spm  added support for BSD Ethernet devices
01t,16nov00,spm  added required mutual exclusion when sending message
01s,24oct00,spm  fixed merge from tor3_x branch and updated mod history
01r,23oct00,niq  merged from version 01w of tor3_x branch (base version 01q);
                 upgrade to BPF replaces tagged frames support
01q,16mar99,spm  recovered orphaned code from tor1_0_1.sens1_1 (SPR #25770)
01p,06oct98,spm  fixed copying of parameters with IP address pairs (SPR #22416)
01o,04dec97,spm  added code review modifications
01n,06oct97,spm  removed reference to deleted endDriver global; replaced with
                 support for dynamic driver type detection
01m,30sep97,kbw  fixed minor spelling error in library man page
01l,02sep97,spm  modified handling of fatal errors - corrected conditions for 
                 disabling network interface and added event hook execution
01k,26aug97,spm  major overhaul: reorganized code and changed user interface
                 to support multiple leases at runtime
01j,06aug97,spm  removed parameters linked list to reduce memory required;
                 renamed class field of dhcp_reqspec structure to prevent C++
                 compilation errors (SPR #9079); corrected minor errors in
                 man pages introduced by 01i revision
01i,30jul97,kbw  fixed man page problems found in beta review
01h,15jul97,spm  cleaned up man pages
01g,10jun97,spm  moved length test to prevent buffer overflow and isolated 
                 incoming messages in state machine from input hooks
01f,02jun97,spm  changed DHCP option tags to prevent name conflicts (SPR #8667)
                 and updated man pages
01e,06may97,spm  changed memory access to align IP header on four byte boundary
01d,15apr97,kbw  fixing man page format and wording
01c,07apr97,spm  added code to use Host Requirements defaults and cleanup 
                 memory on exit, fixed bugs caused by Ethernet trailers, 
                 eliminated potential buffer overruns, rewrote documentation
01b,29jan97,spm  added END driver support and modified to fit coding standards
01a,14nov96,spm  created from shared functions of dhcpcLib.c and dhcpcBootLib.c
*/

/*
DESCRIPTION
This library contains the shared functions used by the both the run-time
and boot-time portions of the DHCP client.

INCLUDE FILES: dhcpcLib.h

SEE ALSO: dhcpcLib
*/

/* includes */

#include "dhcp/copyright_dhcp.h"
#include "vxWorks.h"
#include "wdLib.h"
#include "semLib.h"
#include "intLib.h"
#include "vxLib.h" 	/* checksum() declaration */
#include "tickLib.h" 	/* tickGet() declaration */
#include "muxLib.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
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

#include "dhcp/dhcpcCommonLib.h"
#include "dhcp/dhcpcStateLib.h"
#include "dhcp/dhcpc.h"
#include "net/bpf.h"
#include "ipProto.h"

/* defines */

#define _BYTESPERWORD 	4 	/* Conversion factor for IP header */

     /* Host requirements documents default values. */

#define _HRD_MAX_DGRAM  576     /* Default maximum datagram size. */
#define _HRD_IP_TTL     64      /* Default IP time-to-live (seconds) */
#define _HRD_MTU        576     /* Default interface MTU */
#define _HRD_ROUTER     0xffffffff /* Default router solication */
                                   /* address - 255.255.255.255 */
#define _HRD_ARP_TIME   60      /* Default ARP cache timeout (seconds) */
#define _HRD_TCP_TTL    64      /* Default TCP time-to-live (seconds) */
#define _HRD_TCP_TIME   7200    /* Default TCP keepalive interval (seconds) */

/* globals */

int dhcpcDataSock; 		/* Accesses runtime DHCP messages */
BOOL _dhcpcBootFlag = FALSE; 	/* Data socket available? (only at runtime) */

int dhcpcReadTaskId;                    /* DHCP message retrieval task */
LEASE_DATA **   dhcpcLeaseList;         /* List of available cookies. */
MESSAGE_DATA *  dhcpcMessageList;       /* Available DHCP messages. */

int dhcpcBufSize; 			/* Maximum supported message size */

int dhcpcMaxLeases; 		/*
                                 * Set externally when building runtime image
                                 * or to 1 when running boot image.
                                 */

int dhcpcMinLease; 		/* Set externally when building image. */

int _dhcpcReadTaskPriority  = 56;      /* Priority level of data retriever */
int _dhcpcReadTaskOptions   = 0;       /* Option settings of data retriever */
int _dhcpcReadTaskStackSize = 5000;    /* Stack size of data retriever */

    /* Berkeley Packet Filter instructions for catching DHCP messages. */

struct bpf_insn dhcpfilter[] = {
  BPF_STMT(BPF_LD+BPF_TYPE,0),                /* Save lltype in accumulator */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, ETHERTYPE_IP, 0, 22),  /* IP packet? */

  /*
   * The remaining statements use the (new) BPF_HLEN alias to avoid any
   * link-layer dependencies. The expected destination port and transaction
   * ID values are altered when necessary by the DHCP routines to match the
   * actual settings.
   */

  BPF_STMT(BPF_LD+BPF_H+BPF_ABS+BPF_HLEN, 6),    /* A <- IP FRAGMENT field */
  BPF_JUMP(BPF_JMP+BPF_JSET+BPF_K, 0x1fff, 20, 0),         /* OFFSET == 0 ? */

  BPF_STMT(BPF_LDX+BPF_HLEN, 0),          /* X <- frame data offset */
  BPF_STMT(BPF_LD+BPF_B+BPF_IND, 9),      /* A <- IP_PROTO field */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, IPPROTO_UDP, 0, 17),     /* UDP ? */

  BPF_STMT(BPF_LD+BPF_HLEN, 0),           /* A <- frame data offset */
  BPF_STMT(BPF_LDX+BPF_B+BPF_MSH+BPF_HLEN, 0), /* X <- IPHDR LEN field */
  BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0),     /* A <- start of UDP datagram */
  BPF_STMT(BPF_MISC+BPF_TAX, 0),          /* X <- start of UDP datagram */

  BPF_STMT(BPF_LD+BPF_H+BPF_IND, 2),      /* A <- UDP DSTPORT */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 68, 0, 11), /* check DSTPORT */

  BPF_STMT(BPF_LD+BPF_H+BPF_ABS+BPF_HLEN, 2),  /* A <- IP LEN field */
  BPF_JUMP(BPF_JMP+BPF_JGE+BPF_K, 0, 0, 9),    /* Correct IP length? */

  BPF_STMT(BPF_LD+BPF_H+BPF_IND, 4),      /* A <- UDP LEN field */
  BPF_JUMP(BPF_JMP+BPF_JGE+BPF_K, 0, 0, 7),    /* Correct UDP length? */

  BPF_STMT(BPF_LD+BPF_B+BPF_IND, 8),      /* A <- DHCP op field */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, BOOTREPLY, 0, 5),
  BPF_STMT(BPF_LD+BPF_W+BPF_IND, 12),      /* A <- DHCP xid field */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, -1, 0, 3),   /* -1 replaced with real xid */
  BPF_STMT(BPF_LD+BPF_W+BPF_IND, 244),    /* A <- DHCP options */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0x63825363, 0, 1),
                                                 /* Matches magic cookie? */

  BPF_STMT(BPF_RET+BPF_K+BPF_HLEN, DFLTDHCPLEN + UDPHL + IPHL),
                                           /*
                                            * ignore data beyond expected
                                            * size (some drivers add padding).
                                            */
  BPF_STMT(BPF_RET+BPF_K, 0)          /* unrecognized message: ignore frame */
  };

struct bpf_program dhcpread = {
    sizeof (dhcpfilter) / sizeof (struct bpf_insn),
    dhcpfilter
    };

RING_ID		dhcpcEventRing; 	/* Ring buffer of DHCP events */
SEM_ID		dhcpcEventSem; 		/* DHCP event notification */

/* forward declarations */

UCHAR * dhcpcOptionMerge (UCHAR, UCHAR *, UCHAR *, int);
UCHAR * dhcpcOptCreate (UCHAR, UCHAR *, UCHAR *, int);

void dhcpcRead (void);                   /* Retrieve incoming DHCP messages */

/*******************************************************************************
*
* dhcpcArpSend - transmit an outgoing ARP message
*
* This routine uses the MUX or netif interface to send a frame containing an
* ARP packet independently of the link level type. 
*
* RETURNS: OK, or ERROR if send fails.
*
* ERRNO: N/A
*
* NOMANUAL
*
* INTERNAL
* Ideally, the dhcpSend() routine would handle any link-level frame
* using the registered output routine (ipOutput in the ipProto.c module).
* However, constructing frames with the link-level broadcast address
* requires significant changes to the MUX/END interface and the ipOutput
* routine which are not backward compatible. This routine extracts
* relevant portions of the ipOutput routine so that only the driver
* layer interface is affected.
*/

STATUS dhcpcArpSend
    (
    struct ifnet *      pIf,    /* interface for sending message */
    char *              pData,  /* frame containing ARP packet */
    int                 size 	/* length of frame, in bytes */
    )
    {
    u_short etype;
    int s;
    struct mbuf * pMbuf;
    IP_DRV_CTRL* pDrvCtrl;
    struct arpcom *ac = (struct arpcom *)pIf;
    struct ether_header *eh;

    BOOL bsdFlag = FALSE;    /* BSD or END network device? */

    pDrvCtrl = (IP_DRV_CTRL *)pIf->pCookie;
    if (pDrvCtrl == NULL)
        bsdFlag = TRUE;         
    else if (pDrvCtrl->pIpCookie == NULL)
        return (ERROR);

    if ((pIf->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
        return (ERROR);

    pMbuf = bcopy_to_mbufs (pData, size, 0, pIf, NONE);
    if (pMbuf == NULL)
        return (ERROR);

    pIf->if_lastchange = tickGet();

    etype = htons (ETHERTYPE_ARP);

    if (bsdFlag)
        {
        /* Build an Ethernet header to the broadcast address. */

        M_PREPEND (pMbuf, sizeof (struct ether_header), M_DONTWAIT);
        if (pMbuf == 0)
            return (ERROR);

        eh = mtod (pMbuf, struct ether_header *);

        pMbuf->mBlkHdr.mFlags |= M_BCAST; /* Always broadcast ARP messages. */

        bcopy((caddr_t)&etype,(caddr_t)&eh->ether_type,
                sizeof(eh->ether_type));
        bcopy((caddr_t)etherbroadcastaddr, (caddr_t)eh->ether_dhost,
              sizeof (etherbroadcastaddr));
        bcopy((caddr_t)ac->ac_enaddr, (caddr_t)eh->ether_shost,
            sizeof(eh->ether_shost));
        } 
    else
        {
        /*
         * Use the MUX interface to build a frame header. We
         * need to handle both END and NPT devices here.
         */

        if (pDrvCtrl->nptFlag)
            {
            if (pIf->if_addrlen)
                {
                M_PREPEND(pMbuf, pIf->if_addrlen, M_DONTWAIT);
                if (pMbuf == NULL)
                    return(ERROR);
                ((M_BLK_ID)pMbuf)->mBlkPktHdr.rcvif = 0;

                /* Store the destination address. */

                bcopy (pDrvCtrl->pDstAddr, pMbuf->m_data, pIf->if_addrlen);
                }

            /* Save the network protocol type. */

            ((M_BLK_ID)pMbuf)->mBlkHdr.reserved = etype;
            }
        else
            {
            pDrvCtrl->pSrc->m_data = (char *)&ac->ac_enaddr;
            pDrvCtrl->pDst->mBlkHdr.reserved = etype;
            pDrvCtrl->pSrc->mBlkHdr.reserved = etype;

            pMbuf->mBlkHdr.mFlags |= M_BCAST; /* Always broadcast ARP messages. */

            if ((pMbuf = muxLinkHeaderCreate (pDrvCtrl->pIpCookie, pMbuf,
                                              pDrvCtrl->pSrc,
                                              pDrvCtrl->pDst, TRUE)) == NULL)
                return (ERROR);
            }
        }

    s = splimp();

    /*
     * Queue message on interface, and start output if interface
     * not yet active.
     */

    if (IF_QFULL(&pIf->if_snd))
        {
        IF_DROP(&pIf->if_snd);
        splx(s);
        netMblkClChainFree (pMbuf);
        return (ERROR);
        }
    IF_ENQUEUE(&pIf->if_snd, pMbuf);

    if ((pIf->if_flags & IFF_OACTIVE) == 0)
        (*pIf->if_start)(pIf);
    splx(s);
    pIf->if_obytes += pMbuf->mBlkHdr.mLen;
    if (pMbuf->m_flags & M_MCAST || pMbuf->m_flags & M_BCAST)
        pIf->if_omcasts++;

    return (OK);
    }

/*******************************************************************************
*
* dhcpSend - transmit an outgoing DHCP message
*
* This routine uses the MUX interface to send an IP packet containing a
* DHCP message independently of the link level type. It is derived from
* the ipOutput() routine in the ipProto.c module. It also transmits messages
* for a BSD Ethernet interface.
*
* RETURNS: OK, or ERROR if send fails.
*
* ERRNO: N/A
*
* INTERNAL
* Since the underlying processing performs address resolution, this transmit
* routine is only valid when sending messages to a DHCP participant (such
* as a server or relay agent) with a known IP address. It cannot send
* replies to clients since the hosts will not respond to resolution requests
* for the target IP address if it is not yet assigned.
*
* NOMANUAL
*/

STATUS dhcpSend
    (
    struct ifnet * 		pIf,    /* interface for sending message */
    struct sockaddr_in * 	pDest,  /* destination IP address of message */
    char * 			pData,  /* IP packet containing DHCP message */
    int 			size,   /* amount of data to send */
    BOOL 			broadcastFlag   /* broadcast packet? */
    )
    {
    struct mbuf * pMbuf;
    int result;
    int level;

    pMbuf = bcopy_to_mbufs (pData, size, 0, pIf, NONE);
    if (pMbuf == NULL)
        return (ERROR);

    if (broadcastFlag)
        pMbuf->mBlkHdr.mFlags |= M_BCAST;

    level = splnet ();
    result = pIf->if_output (pIf, pMbuf, (struct sockaddr *)pDest, NULL);
    splx (level);

    if (result)
        return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* dhcpcRead - handle all incoming DHCP messages
*
* This routine monitors all active BPF devices for new DHCP messages and
* signals the lease monitor task when a valid message arrives. It is the
* entry point for the data retrieval task created during the library
* initialization and should only be called internally.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void dhcpcRead (void)
    {
    STATUS 	result;

    EVENT_DATA 	newEvent;
    int key;

    char * 	pMsgData = NULL;

    BOOL 	dataFlag;
    fd_set 	readFds;
    int 	fileNum = 0;
    int 	maxFileNum;
    int 	slot = 0; 	/* Current buffer for holding messages. */
    int 	maxSlot = 10; 	/* Matches size of event ring. */
    int 	loop;
    int 	bufSize = 0;
    struct bpf_hdr * 	pMsgHdr;
    int 	msglen;
    int 	curlen;
    int 	totlen; 	/* Amount of data in BPF buffer. */
    struct timeval timeout;

    timeout.tv_sec = timeout.tv_usec = 0;
 
    FOREVER
        {
        /*
         * Wait until a message arrives or a new DHCP session is available.
         * The dhcpcInit() routine forces the read task to rebuild the list
         * of active devices by sending an empty message to the selected DHCP
         * client port so that the select routine exits.
         */

        FD_ZERO (&readFds);
        maxFileNum = 0;

        if (!_dhcpcBootFlag)
            {
            /*
             * Runtime DHCP sessions monitor a UDP socket bound to the
             * DHCP port to read and discard messages which were already
             * processed through the Berkeley Packet Filters. This socket
             * also allows the read loop to restart when a new DHCP session
             * begins. The boot time DHCP session is unable to use any
             * sockets since no fully configured devices exist.
             */
            
            FD_SET (dhcpcDataSock, &readFds);
            maxFileNum = dhcpcDataSock;
            }

        /* Create list of active BPF devices for monitoring. */

        for (loop = 0; loop < dhcpcMaxLeases; loop++)
            {
            if (dhcpcLeaseList [loop] != NULL &&
                    dhcpcLeaseList [loop]->initFlag)
                {
                fileNum = dhcpcLeaseList [loop]->ifData.bpfDev;

                FD_SET (fileNum, &readFds);

                if (fileNum > maxFileNum)
                    maxFileNum = fileNum;
                }
            }

        /*
         * No BPF devices will be active until a lease session starts
         * which uses an interface without an assigned IP address.
         */

        maxFileNum++;     /* Adjust maximum for select() routine. */

        /*
         * Check for an incoming DHCP message. None may be available
         * if a new session started.
         */

        result = select (maxFileNum, &readFds, NULL, NULL, NULL);
        if (result <= 0)
            continue;

        /*
         * Ignore the message if the current buffer is full. That
         * condition indicates an overflow of the message "ring".
         */

        if (dhcpcMessageList [slot].writeFlag == FALSE)
            continue;

        dhcpcMessageList [slot].writeFlag = FALSE;

        pMsgData = dhcpcMessageList [slot].msgBuffer;

        if (!_dhcpcBootFlag)
            {
            /*
             * For runtime DHCP sessions, read and discard any DHCP
             * messages which reach the socket layer. An (empty)
             * DHCP message is sent to the loopback address when a
             * new session begins. This operation also removes
             * messages which were already handled by the Berkeley
             * Packet Filter, but also reach the socket layer if
             * any device is configured with an IP address. Creating
             * a socket to receive them prevents the transmission of
             * ICMP errors.
             */

            dataFlag = FD_ISSET (dhcpcDataSock, &readFds);
            if (dataFlag)
                {
                read (dhcpcDataSock, pMsgData, dhcpcBufSize);
                dhcpcMessageList [slot].writeFlag = TRUE;
                continue;
                }
            }

        /* Read the first available DHCP message. */

        for (loop = 0; loop < dhcpcMaxLeases; loop++)
            {
            if (dhcpcLeaseList [loop]->initFlag)
                {
                fileNum = dhcpcLeaseList [loop]->ifData.bpfDev;
                dataFlag = FD_ISSET (fileNum, &readFds);
                if (dataFlag)
                    {
                    /*
                     * Any users must read an entire BPF buffer. The size
                     * is set during the initialization for each lease.
                     */

                    bufSize = dhcpcLeaseList [loop]->ifData.bufSize;
                    break;
                    }
                }
            }

        if (loop == dhcpcMaxLeases)
            {
            /* No DHCP sessions are using the BPF devices. */

            continue;
            }
 
        /*
         * Ignore any read errors. The DHCP protocol is designed to handle
         * any problems with false arrival notifications or invalid messages.
         */

        totlen = read (fileNum, pMsgData, bufSize);

        /* Signal lease monitor for each DHCP message in the BPF buffer. */

        newEvent.source = DHCP_AUTO_EVENT;
        newEvent.type = DHCP_MSG_ARRIVED;
        newEvent.leaseId = dhcpcLeaseList [loop];
        newEvent.slot = slot;

        msglen = curlen = 0;
        pMsgHdr = (struct bpf_hdr *)pMsgData;

        while (curlen < totlen)
            {
            msglen = BPF_WORDALIGN(pMsgHdr->bh_hdrlen + pMsgHdr->bh_caplen);
            curlen += msglen;

            /* Set the pointer to skip the BPF and link level headers. */

            newEvent.pMsg = pMsgData + pMsgHdr->bh_hdrlen +
                                pMsgHdr->bh_linklen;

            if (curlen < totlen)
                newEvent.lastFlag = FALSE;
            else
                newEvent.lastFlag = TRUE;

            /* Lock interrupts to prevent conflicts with timeout thread. */

            key = intLock ();
            rngBufPut (dhcpcEventRing, (char *) &newEvent, sizeof(newEvent));
            intUnlock (key);

            semGive (dhcpcEventSem);

            pMsgData = pMsgData + msglen;
            pMsgHdr = (struct bpf_hdr *)pMsgData;
            }

        /* Advance to next element in message ring. */

        slot = (slot + 1) % maxSlot;
        }
    }

/*******************************************************************************
*
* dhcpcOptionSet - add an option to the option request list
*
* This routine specifies which options the lease indicated by the <pCookie>
* parameter will request from a server.  The <option> parameter specifies an 
* option tag as defined in RFC 2132.  See the dhcp/dhcp.h include file for a
* listing of defined aliases for the available option tags. This routine will
* not accept the following <option> values, which are either used by the
* server for control purposes or only supplied by the client:
*
*     _DHCP_PAD_TAG
*     _DHCP_REQUEST_IPADDR_TAG 
*     _DHCP_LEASE_TIME_TAG  
*     _DHCP_OPT_OVERLOAD_TAG
*     _DHCP_MSGTYPE_TAG
*     _DHCP_SERVER_ID_TAG
*     _DHCP_REQ_LIST_TAG 
*     _DHCP_ERRMSG_TAG 
*     _DHCP_MAXMSGSIZE_TAG
*     _DHCP_CLASS_ID_TAG 
*     _DHCP_CLIENT_ID_TAG 
*     _DHCP_END_TAG
*
* This routine also will not accept <option> values 62 or 63, which are not
* currently defined.
*
* The maximum length of the option field in a DHCP message depends on the
* MTU size of the associated interface and the maximum DHCP message size set
* during the DHCP library initialization. Both the option request list and
* the options sent by the client through the dhcpcOptionAdd() routine share
* that field. Options which exceed the limit will not be stored.
*
* NOTE: The boot program automatically requests all options necessary for
* default target configuration. This routine is only necessary to support
* special circumstances in which additional options are required. Any
* options requested in that case may be retrieved after the runtime image
* has started.
* 
* NOTE: The DHCP specification forbids changing the option request list after
* a lease has been established.  Therefore, this routine must not be used
* after the dhcpcBind() call (in a runtime image) or the dhcpcBootBind() call
* (for a boot image).  Changing the request list at that point could have
* unpredictable results.
*
* NOTE: Options are added directly to outgoing DHCP messages, and numeric
* options (e.g. lease duration time) are expected to be provided in network
* byte order. Care must be taken on little-endian hosts to insure that
* numeric arguments are properly byte-swapped before being passed to this
* routine.
*
* RETURNS: OK if the option was set successfully, or ERROR if the option
* is invalid or storage failed.
*
* ERRNO: S_dhcpcLib_BAD_OPTION, S_dhcpcLib_OPTION_NOT_STORED
*
*/

STATUS dhcpcOptionSet
    (
    void * 	pCookie, 	/* identifier returned by dhcpcInit() */
    int 	option		/* RFC 2132 tag of desired option */
    )
    {
    LEASE_DATA *	 	pLeaseData;
    struct dhcp_reqspec * 	pReqSpec;
    struct dhcpcOpts * 		pOptList;

    int msglen = 0; 	/* Length of DHCP message after inserting option */

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;
    pReqSpec = &pLeaseData->leaseReqSpec;

    /* Check for restricted or undefined options. */

    switch (option)
        {
        case _DHCP_PAD_TAG: 		/* fall-through */
        case _DHCP_REQUEST_IPADDR_TAG: 	/* fall-through */
        case _DHCP_LEASE_TIME_TAG: 	/* fall-through */
        case _DHCP_OPT_OVERLOAD_TAG: 	/* fall-through */
        case _DHCP_MSGTYPE_TAG: 	/* fall-through */
        case _DHCP_SERVER_ID_TAG: 	/* fall-through */
        case _DHCP_REQ_LIST_TAG: 	/* fall-through */
        case _DHCP_ERRMSG_TAG: 		/* fall-through */
        case _DHCP_MAXMSGSIZE_TAG: 	/* fall-through */
        case _DHCP_CLASS_ID_TAG: 	/* fall-through */
        case _DHCP_CLIENT_ID_TAG: 	/* fall-through */
        case _DHCP_END_TAG:
            errno = S_dhcpcLib_BAD_OPTION;
            return (ERROR);
            break;
        default:
            break;
        }

    if (option == 62 || option == 63)
        {
        errno = S_dhcpcLib_BAD_OPTION;
        return (ERROR);
        }
        
    if (option < 0 || option > _DHCP_LAST_OPTION)
        {
        errno = S_dhcpcLib_BAD_OPTION;
        return (ERROR);
        }

    /*
     * Verify that the option won't exceed the MTU size for a message.
     * Start with an initial length equal to the UDP and IP headers and
     * the fixed-length portion of a DHCP message.
     */

    msglen = UDPHL + IPHL + (DFLTDHCPLEN - DFLTOPTLEN);

    /* Include size of existing options and option request list. */

    pOptList = pReqSpec->pOptList;

    if (pOptList)
        msglen += pReqSpec->pOptList->optlen;

    msglen += (pReqSpec->reqlist.len + 1);

    /*
     * Include space for required magic cookie (4 bytes), message
     * type option (3 bytes), maximum message size (4 bytes), and
     * server identifier option (6 bytes).
     */

    msglen += 17;

    /* Include space for required requested IP address option (6 bytes). */

    msglen += 6;

    if (msglen > pReqSpec->maxlen)
        {
        /*
         * Option field is full.  New option would either exceed MTU size
         * or overflow the transmit buffer.
         */

        errno = S_dhcpcLib_OPTION_NOT_STORED;
        return (ERROR);
        }

    pReqSpec->reqlist.list [pReqSpec->reqlist.len++] = option;

    return (OK);
    }

/*******************************************************************************
*
* dhcpcOptionAdd - add an option to the client messages
*
* This routine inserts option tags and associated values into the body of
* all outgoing messages for the lease indicated by the <pCookie> parameter.
* Each lease can accept option data up to the MTU size of the underlying
* interface, minus the link-level header size and the additional 283 bytes
* required for a minimum DHCP message (including mandatory options).
*
* The <option> parameter specifies an option tag defined in RFC 2132. See
* the dhcp/dhcp.h include file for a listing of defined aliases for the
* available option tags. This routine will not accept the following <option>
* values, which are used for control purposes and cannot be included
* arbitrarily:
*
*     _DHCP_PAD_TAG
*     _DHCP_OPT_OVERLOAD_TAG
*     _DHCP_MSGTYPE_TAG
*     _DHCP_SERVER_ID_TAG
*     _DHCP_MAXMSGSIZE_TAG
*     _DHCP_END_TAG
*
* This routine also will not accept <option> values 62 or 63, which are not
* currently defined.
*
* The <length> parameter indicates the number of bytes in the option body
* provided by the <pData> parameter. 
*
* The maximum length of the option field in a DHCP message depends on the
* MTU size of the associated interface and the maximum DHCP message size set
* during the DHCP library initialization. These option settings share that
* field with any option request list created through the dhcpcOptionSet()
* routine. Options which exceed the limit will not be stored.
*
* Each call to this routine with the same <option> value usually replaces
* the value of the existing option, if any. However, the routine will append
* the new data for the <option> values which contain variable length lists,
* corresponding to tags 3-11, 21, 25, 33, 41-45, 48-49, 55, 65, and 68-76.
*
* WARNING: The _DHCP_REQ_LIST_TAG <option> value (55) will replace
* any existing list created with the dhcpcOptionSet() routine.
*
* RETURNS: OK if the option was inserted successfully, or ERROR if the option
* is invalid or storage failed.
*
* ERRNO: S_dhcpcLib_BAD_OPTION, S_dhcpcLib_OPTION_NOT_STORED
*
*/

STATUS dhcpcOptionAdd
    (
    void * 	pCookie, 	/* identifier returned by dhcpcInit() */
    UCHAR 	option, 	/* RFC 2132 tag of desired option */
    int 	length, 	/* length of option data */
    UCHAR * 	pData 		/* option data */
    )
    {
    LEASE_DATA *                pLeaseData;
    struct dhcp_reqspec *       pReqSpec;
    char * 			pDest;
    struct dhcpcOpts * 		pOptList;

    int msglen = 0; 	/* Length of DHCP message after inserting option */

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;
    pReqSpec = &pLeaseData->leaseReqSpec;

    /* Check for restricted or invalid options. */

    switch (option)
        {
        case _DHCP_PAD_TAG:             /* fall-through */
        case _DHCP_OPT_OVERLOAD_TAG:    /* fall-through */
        case _DHCP_MSGTYPE_TAG:         /* fall-through */
        case _DHCP_SERVER_ID_TAG:       /* fall-through */
        case _DHCP_MAXMSGSIZE_TAG:      /* fall-through */
        case _DHCP_END_TAG:
            errno = S_dhcpcLib_BAD_OPTION;
            return (ERROR);
            break;
        default:
            break;
        }

    if (option == 62 || option == 63)
        {
        errno = S_dhcpcLib_BAD_OPTION;
        return (ERROR);
        }
        
    /*
     * Verify that the option won't exceed the MTU size for a message.
     * Start with an initial length equal to the UDP and IP headers and
     * the fixed-length portion of a DHCP message.
     */

    msglen = UDPHL + IPHL + (DFLTDHCPLEN - DFLTOPTLEN);

    /* Include size of existing options and option request list. */

    pOptList = pReqSpec->pOptList;

    if (pOptList)
        msglen += pReqSpec->pOptList->optlen;

    if (option == _DHCP_REQ_LIST_TAG)    /* Replacing request list? */
        msglen -= pReqSpec->reqlist.len;
    else
        msglen += pReqSpec->reqlist.len;

    /*
     * Include space for required magic cookie (4 bytes), message
     * type option (3 bytes), maximum message size (4 bytes), and
     * server identifier option (6 bytes).
     */

    msglen += 17;

    /* Include space for required requested IP address option (6 bytes). */

    msglen += 6;

    /* Add size of any other option (don't double-count IP address). */

    if (option != _DHCP_REQUEST_IPADDR_TAG)
        msglen += length + 2;    /* +2 includes tag and length values */

    if (msglen > dhcpcBufSize)
        {
        /* Option field is full.  New option would overflow transmit buffer. */

        errno = S_dhcpcLib_OPTION_NOT_STORED;
        return (ERROR);
        }

    if (msglen > pReqSpec->maxlen)
        {
        /*
         * Option field is full.  New option would either exceed MTU size
         * or overflow the transmit buffer.
         */

        errno = S_dhcpcLib_OPTION_NOT_STORED;
        return (ERROR);
        }

    if (pData == NULL)
        {
        errno = S_dhcpcLib_BAD_OPTION;
        return (ERROR);
        }

    /* Validate the length of each option. */

    switch (option)
        {
        /* 1 byte values */

        case _DHCP_IP_FORWARD_TAG: 		/* Option Tag 19 */
        case _DHCP_NONLOCAL_SRCROUTE_TAG: 	/* Option Tag 20 */
        case _DHCP_DEFAULT_IP_TTL_TAG: 		/* Option Tag 23 */
        case _DHCP_ALL_SUBNET_LOCAL_TAG: 	/* Option Tag 27 */
        case _DHCP_MASK_DISCOVER_TAG: 		/* Option Tag 29 */
        case _DHCP_MASK_SUPPLIER_TAG: 		/* Option Tag 30 */
        case _DHCP_ROUTER_DISCOVER_TAG: 	/* Option Tag 31 */
        case _DHCP_TRAILER_TAG: 		/* Option Tag 34 */
        case _DHCP_ETHER_ENCAP_TAG: 		/* Option Tag 36 */
        case _DHCP_DEFAULT_TCP_TTL_TAG: 	/* Option Tag 37 */
        case _DHCP_KEEPALIVE_GARBAGE_TAG: 	/* Option Tag 39 */
        case _DHCP_NB_NODETYPE_TAG: 		/* Option Tag 46 */
            if (length != 1)
                {
                errno = S_dhcpcLib_BAD_OPTION;
                return (ERROR);
                }
            break;

        /* 2 byte values */

        case _DHCP_BOOTSIZE_TAG: 		/* Option Tag 13 */
        case _DHCP_MAX_DGRAM_SIZE_TAG: 		/* Option Tag 22 */
        case _DHCP_IF_MTU_TAG: 			/* Option Tag 26 */
            if (length != 2)
                {
                errno = S_dhcpcLib_BAD_OPTION;
                return (ERROR);
                }
            break;

        /* 4 byte values */

        case _DHCP_SUBNET_MASK_TAG: 		/* Option Tag 1 */
        case _DHCP_TIME_OFFSET_TAG: 		/* Option Tag 2 */
        case _DHCP_SWAP_SERVER_TAG: 		/* Option Tag 16 */
        case _DHCP_MTU_AGING_TIMEOUT_TAG: 	/* Option Tag 24 */
        case _DHCP_BRDCAST_ADDR_TAG: 		/* Option Tag 28 */
        case _DHCP_ROUTER_SOLICIT_TAG: 		/* Option Tag 32 */
        case _DHCP_ARP_CACHE_TIMEOUT_TAG: 	/* Option Tag 35 */
        case _DHCP_KEEPALIVE_INTERVAL_TAG: 	/* Option Tag 38 */
        case _DHCP_REQUEST_IPADDR_TAG: 		/* Option Tag 50 */
        case _DHCP_LEASE_TIME_TAG: 		/* Option Tag 51 */
        case _DHCP_T1_TAG: 			/* Option Tag 58 */
        case _DHCP_T2_TAG: 			/* Option Tag 59 */
            if (length != 4)
                {
                errno = S_dhcpcLib_BAD_OPTION;
                return (ERROR);
                }
            break;

        /* Table entries (2 bytes each) */

        case _DHCP_MTU_PLATEAU_TABLE_TAG: 	/* Option Tag 25 */
            if (length % 2)
                {
                errno = S_dhcpcLib_BAD_OPTION;
                return (ERROR);
                }
            break;

        /* Address values (4 bytes each) */

        case _DHCP_ROUTER_TAG: 			/* Option Tag 3 */
        case _DHCP_TIME_SERVER_TAG: 		/* Option Tag 4 */
        case _DHCP_NAME_SERVER_TAG: 		/* Option Tag 5 */
        case _DHCP_DNS_SERVER_TAG: 		/* Option Tag 6 */
        case _DHCP_LOG_SERVER_TAG: 		/* Option Tag 7 */
        case _DHCP_COOKIE_SERVER_TAG: 		/* Option Tag 8 */
        case _DHCP_LPR_SERVER_TAG: 		/* Option Tag 9 */
        case _DHCP_IMPRESS_SERVER_TAG: 		/* Option Tag 10 */
        case _DHCP_RLS_SERVER_TAG: 		/* Option Tag 11 */
        case _DHCP_NIS_SERVER_TAG: 		/* Option Tag 41 */
        case _DHCP_NTP_SERVER_TAG: 		/* Option Tag 42 */
        case _DHCP_NBN_SERVER_TAG: 		/* Option Tag 44 */
        case _DHCP_NBDD_SERVER_TAG: 		/* Option Tag 45 */
        case _DHCP_XFONT_SERVER_TAG: 		/* Option Tag 48 */
        case _DHCP_XDISPLAY_MANAGER_TAG: 	/* Option Tag 49 */
        case _DHCP_NISP_SERVER_TAG: 		/* Option Tag 65 */
        case _DHCP_MOBILEIP_HA_TAG: 		/* Option Tag 68 */
        case _DHCP_SMTP_SERVER_TAG: 		/* Option Tag 69 */
        case _DHCP_POP3_SERVER_TAG: 		/* Option Tag 70 */
        case _DHCP_NNTP_SERVER_TAG: 		/* Option Tag 71 */
        case _DHCP_DFLT_WWW_SERVER_TAG: 	/* Option Tag 72 */
        case _DHCP_DFLT_FINGER_SERVER_TAG: 	/* Option Tag 73 */
        case _DHCP_DFLT_IRC_SERVER_TAG:  	/* Option Tag 74 */
        case _DHCP_STREETTALK_SERVER_TAG:  	/* Option Tag 75 */
        case _DHCP_STDA_SERVER_TAG: 		/* Option Tag 76 */
            if (length % 4)
                {
                errno = S_dhcpcLib_BAD_OPTION;
                return (ERROR);
                }
            break;

        /* Address pairs or address/mask values (8 bytes each). */

        case _DHCP_POLICY_FILTER_TAG: 		/* Option Tag 21 */
        case _DHCP_STATIC_ROUTE_TAG: 		/* Option Tag 33 */
            if (length % 8)
                {
                errno = S_dhcpcLib_BAD_OPTION;
                return (ERROR);
                }
            break;

        /* Variable-length values */

        case _DHCP_HOSTNAME_TAG: 		/* Option Tag 12 */
        case _DHCP_MERIT_DUMP_TAG: 		/* Option Tag 14 */
        case _DHCP_DNS_DOMAIN_TAG: 		/* Option Tag 15 */
        case _DHCP_ROOT_PATH_TAG: 		/* Option Tag 17 */
        case _DHCP_EXTENSIONS_PATH_TAG: 	/* Option Tag 18 */
        case _DHCP_NIS_DOMAIN_TAG: 		/* Option Tag 40 */
        case _DHCP_VENDOR_SPEC_TAG: 		/* Option Tag 43 */
        case _DHCP_NB_SCOPE_TAG: 		/* Option Tag 47 */
        case _DHCP_REQ_LIST_TAG: 		/* Option Tag 55 */
        case _DHCP_ERRMSG_TAG: 			/* Option Tag 56 */
        case _DHCP_CLASS_ID_TAG: 		/* Option Tag 60 */
        case _DHCP_CLIENT_ID_TAG: 		/* Option Tag 61 */
        case _DHCP_NISP_DOMAIN_TAG: 		/* Option Tag 64 */
        case _DHCP_TFTP_SERVERNAME_TAG: 	/* Option Tag 66 */
        case _DHCP_BOOTFILE_TAG: 		/* Option Tag 67 */
            if (length < 1 || length > _DHCP_MAX_OPTLEN)
                {
                errno = S_dhcpcLib_BAD_OPTION;
                return (ERROR);
                }
            break;
        }

    /* Store the option body in the space provided. */

    if (pOptList == NULL)
        {
        pOptList = malloc (sizeof (struct dhcpcOpts));
        if (pOptList == NULL)
            {
            errno = S_dhcpcLib_OPTION_NOT_STORED;
            return (ERROR);
            }
        bzero ( (char *)pOptList, sizeof (struct dhcpcOpts));
        pReqSpec->pOptList = pOptList;
        }

    switch (option)
        {
        /* 1 byte values */

        case _DHCP_IP_FORWARD_TAG: 		/* Option Tag 19 */
            pOptList->tag19 = *pData;
            break;    
        case _DHCP_NONLOCAL_SRCROUTE_TAG: 	/* Option Tag 20 */
            pOptList->tag20 = *pData;
            break;
        case _DHCP_DEFAULT_IP_TTL_TAG: 		/* Option Tag 23 */
            pOptList->tag23 = *pData;
            break;
        case _DHCP_ALL_SUBNET_LOCAL_TAG: 	/* Option Tag 27 */
            pOptList->tag27 = *pData;
            break;
        case _DHCP_MASK_DISCOVER_TAG: 		/* Option Tag 29 */
            pOptList->tag29 = *pData;
            break;
        case _DHCP_MASK_SUPPLIER_TAG: 		/* Option Tag 30 */
            pOptList->tag30 = *pData;
            break;
        case _DHCP_ROUTER_DISCOVER_TAG: 	/* Option Tag 31 */
            pOptList->tag31 = *pData;
            break;
        case _DHCP_TRAILER_TAG: 		/* Option Tag 34 */
            pOptList->tag34 = *pData;
            break;
        case _DHCP_ETHER_ENCAP_TAG: 		/* Option Tag 36 */
            pOptList->tag36 = *pData;
            break;
        case _DHCP_DEFAULT_TCP_TTL_TAG: 	/* Option Tag 37 */
            pOptList->tag37 = *pData;
            break;
        case _DHCP_KEEPALIVE_GARBAGE_TAG: 	/* Option Tag 39 */
            pOptList->tag39 = *pData;
            break;
        case _DHCP_NB_NODETYPE_TAG: 		/* Option Tag 46 */
            pOptList->tag46 = *pData;
            break;

        /* 2 byte values */

        case _DHCP_BOOTSIZE_TAG: 		/* Option Tag 13 */
            bcopy (pData, (char *)&pOptList->tag13, sizeof (USHORT));
            break;
        case _DHCP_MAX_DGRAM_SIZE_TAG: 		/* Option Tag 22 */
            bcopy (pData, (char *)&pOptList->tag22, sizeof (USHORT));
            break;
        case _DHCP_IF_MTU_TAG: 			/* Option Tag 26 */
            bcopy (pData, (char *)&pOptList->tag26, sizeof (USHORT));
            break;

        /* 4 byte values */

        case _DHCP_SUBNET_MASK_TAG: 		/* Option Tag 1 */
            bcopy (pData, (char *)&pOptList->tag1, sizeof (ULONG));
            break;
        case _DHCP_TIME_OFFSET_TAG: 		/* Option Tag 2 */
            bcopy (pData, (char *)&pOptList->tag2, sizeof (long));
            break;
        case _DHCP_SWAP_SERVER_TAG: 		/* Option Tag 16 */
            bcopy (pData, (char *)&pOptList->tag16, sizeof (ULONG));
            break;
        case _DHCP_MTU_AGING_TIMEOUT_TAG: 	/* Option Tag 24 */
            bcopy (pData, (char *)&pOptList->tag24, sizeof (ULONG));
            break;
        case _DHCP_BRDCAST_ADDR_TAG: 		/* Option Tag 28 */
            bcopy (pData, (char *)&pOptList->tag28, sizeof (ULONG));
            break;
        case _DHCP_ROUTER_SOLICIT_TAG: 		/* Option Tag 32 */
            bcopy (pData, (char *)&pOptList->tag32, sizeof (ULONG));
            break;
        case _DHCP_ARP_CACHE_TIMEOUT_TAG: 	/* Option Tag 35 */
            bcopy (pData, (char *)&pOptList->tag35, sizeof (ULONG));
            break;
        case _DHCP_KEEPALIVE_INTERVAL_TAG: 	/* Option Tag 38 */
            bcopy (pData, (char *)&pOptList->tag38, sizeof (ULONG));
            break;
        case _DHCP_REQUEST_IPADDR_TAG: 		/* Option Tag 50 */
            /*
             * This option is stored separately since it is not
             * always included in the client's initial message.
             */

            bcopy (pData, (char *)&pReqSpec->ipaddr.s_addr, sizeof (ULONG));
            break;
        case _DHCP_LEASE_TIME_TAG: 		/* Option Tag 51 */
            bcopy (pData, (char *)&pOptList->tag51, sizeof (ULONG));
            break;
        case _DHCP_T1_TAG: 			/* Option Tag 58 */
            bcopy (pData, (char *)&pOptList->tag58, sizeof (ULONG));
            break;
        case _DHCP_T2_TAG: 			/* Option Tag 59 */
            bcopy (pData, (char *)&pOptList->tag59, sizeof (ULONG));
            break;

        /*
         * Variable-length lists. Add new entry or merge.
         * Store tag value and length before option body.
         */

        case _DHCP_MTU_PLATEAU_TABLE_TAG: 	/* Option Tag 25 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag25, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag25 = pDest;
            break;

        case _DHCP_ROUTER_TAG: 			/* Option Tag 3 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag3, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag3 = pDest;
            break;
        case _DHCP_TIME_SERVER_TAG: 		/* Option Tag 4 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag4, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag4 = pDest;
            break;
        case _DHCP_NAME_SERVER_TAG: 		/* Option Tag 5 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag5, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag5 = pDest;
            break;
        case _DHCP_DNS_SERVER_TAG: 		/* Option Tag 6 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag6, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag6 = pDest;
            break;
        case _DHCP_LOG_SERVER_TAG: 		/* Option Tag 7 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag7, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag7 = pDest;
            break;
        case _DHCP_COOKIE_SERVER_TAG: 		/* Option Tag 8 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag8, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag8 = pDest;
            break;
        case _DHCP_LPR_SERVER_TAG: 		/* Option Tag 9 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag9, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag9 = pDest;
            break;
        case _DHCP_IMPRESS_SERVER_TAG: 		/* Option Tag 10 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag10, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag10 = pDest;
            break;
        case _DHCP_RLS_SERVER_TAG: 		/* Option Tag 11 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag11, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag11 = pDest;
            break;
        case _DHCP_NIS_SERVER_TAG: 		/* Option Tag 41 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag41, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag41 = pDest;
            break;
        case _DHCP_NTP_SERVER_TAG: 		/* Option Tag 42 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag42, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag42 = pDest;
            break;
        case _DHCP_NBN_SERVER_TAG: 		/* Option Tag 44 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag44, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag44 = pDest;
            break;
        case _DHCP_NBDD_SERVER_TAG: 		/* Option Tag 45 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag45, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag45 = pDest;
            break;
        case _DHCP_XFONT_SERVER_TAG: 		/* Option Tag 48 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag48, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag48 = pDest;
            break;
        case _DHCP_XDISPLAY_MANAGER_TAG: 	/* Option Tag 49 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag49, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag49 = pDest;
            break;
        case _DHCP_NISP_SERVER_TAG: 		/* Option Tag 65 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag65, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag65 = pDest;
            break;
        case _DHCP_MOBILEIP_HA_TAG: 		/* Option Tag 68 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag68, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag68 = pDest;
            break;
        case _DHCP_SMTP_SERVER_TAG: 		/* Option Tag 69 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag69, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag69 = pDest;
            break;
        case _DHCP_POP3_SERVER_TAG: 		/* Option Tag 70 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag70, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag70 = pDest;
            break;
        case _DHCP_NNTP_SERVER_TAG: 		/* Option Tag 71 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag71, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag71 = pDest;
            break;
        case _DHCP_DFLT_WWW_SERVER_TAG: 	/* Option Tag 72 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag72, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag72 = pDest;
            break;
        case _DHCP_DFLT_FINGER_SERVER_TAG: 	/* Option Tag 73 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag73, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag73 = pDest;
            break;
        case _DHCP_DFLT_IRC_SERVER_TAG:  	/* Option Tag 74 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag74, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag74 = pDest;
            break;
        case _DHCP_STREETTALK_SERVER_TAG:  	/* Option Tag 75 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag75, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag75 = pDest;
            break;
        case _DHCP_STDA_SERVER_TAG: 		/* Option Tag 76 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag76, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag76 = pDest;
            break;
        case _DHCP_POLICY_FILTER_TAG: 		/* Option Tag 21 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag21, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag21 = pDest;
            break;
        case _DHCP_STATIC_ROUTE_TAG: 		/* Option Tag 33 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag33, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag33 = pDest;
            break;
        case _DHCP_VENDOR_SPEC_TAG: 		/* Option Tag 43 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag43, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag43 = pDest;
            break;
        case _DHCP_REQ_LIST_TAG: 		/* Option Tag 55 */
            pDest = dhcpcOptionMerge (option, pOptList->pTag55, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                {
                /* Add new list. Ignore dhcpcOptionSet() routine results. */

                pReqSpec->reqlist.len = 0;
                pOptList->pTag55 = pDest;
                }
            break;

        /*
         * Variable-length values. Add new entry or replace existing one.
         * Store tag value and length before option body.
         */

        case _DHCP_HOSTNAME_TAG: 		/* Option Tag 12 */
            pDest = dhcpcOptCreate (option, pOptList->pTag12, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag12 = pDest;
            break;

        case _DHCP_MERIT_DUMP_TAG: 		/* Option Tag 14 */
            pDest = dhcpcOptCreate (option, pOptList->pTag14, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag14 = pDest;
            break;
        case _DHCP_DNS_DOMAIN_TAG: 		/* Option Tag 15 */
            pDest = dhcpcOptCreate (option, pOptList->pTag15, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag15 = pDest;
            break;
        case _DHCP_ROOT_PATH_TAG: 		/* Option Tag 17 */
            pDest = dhcpcOptCreate (option, pOptList->pTag17, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag17 = pDest;
            break;
        case _DHCP_EXTENSIONS_PATH_TAG: 	/* Option Tag 18 */
            pDest = dhcpcOptCreate (option, pOptList->pTag18, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag18 = pDest;
            break;
        case _DHCP_NIS_DOMAIN_TAG: 		/* Option Tag 40 */
            pDest = dhcpcOptCreate (option, pOptList->pTag40, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag40 = pDest;
            break;
        case _DHCP_NB_SCOPE_TAG: 		/* Option Tag 47 */
            pDest = dhcpcOptCreate (option, pOptList->pTag47, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag47 = pDest;
            break;
        case _DHCP_ERRMSG_TAG: 			/* Option Tag 56 */
            pDest = dhcpcOptCreate (option, pOptList->pTag56, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag56 = pDest;
            break;
        case _DHCP_CLASS_ID_TAG: 		/* Option Tag 60 */
            pDest = dhcpcOptCreate (option, pOptList->pTag60, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag60 = pDest;
            break;
        case _DHCP_CLIENT_ID_TAG: 		/* Option Tag 61 */
            /*
             * This option is also stored separately since it is included
             * independently in DECLINE and RELEASE messages.
             */

            if (pReqSpec->clid == NULL)
                {
                pReqSpec->clid = malloc (sizeof (struct client_id));
                if (pReqSpec->clid == NULL)
                    {
                    errno = S_dhcpcLib_OPTION_NOT_STORED;
                    return (ERROR);
                    }
                bzero ( (char *)pReqSpec->clid, sizeof (struct client_id));
                }

            if (pReqSpec->clid->id != NULL)
                free (pReqSpec->clid->id);

            pReqSpec->clid->id = malloc (length * sizeof (char));
            if (pReqSpec->clid->id == NULL)
                {
                free (pReqSpec->clid);
                pReqSpec->clid = NULL;
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }

            bcopy (pData, pReqSpec->clid->id, length);
            pReqSpec->clid->len = length;

            pDest = dhcpcOptCreate (option, pOptList->pTag61, pData, length);
            if (pDest == NULL)
                {
                free (pReqSpec->clid->id);
                free (pReqSpec->clid);
                pReqSpec->clid = NULL;
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag61 = pDest;
            break;
        case _DHCP_NISP_DOMAIN_TAG: 		/* Option Tag 64 */
            pDest = dhcpcOptCreate (option, pOptList->pTag64, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag64 = pDest;
            break;
        case _DHCP_TFTP_SERVERNAME_TAG: 	/* Option Tag 66 */
            pDest = dhcpcOptCreate (option, pOptList->pTag66, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag66 = pDest;
            break;
        case _DHCP_BOOTFILE_TAG: 		/* Option Tag 67 */
            pDest = dhcpcOptCreate (option, pOptList->pTag67, pData, length);
            if (pDest == NULL)
                {
                errno = S_dhcpcLib_OPTION_NOT_STORED;
                return (ERROR);
                }
            else
                pOptList->pTag67 = pDest;
            break;
        }

    pReqSpec->pOptList->optlen += length + 2;
    SETBIT (pOptList->optmask, option);

    return (OK);
    }

/*******************************************************************************
*
* dhcpcOptionMerge - combine options into a single body
*
* The dhcpcOptionAdd() routine uses this routine to extend an existing option
* body, if any, with new data. This merge operation is required when the same
* option is inserted more than once for a given tag. If no option currently
* exists, this routine just creates the new option subfield. The <pOrig>
* parameter contains the current option subfield, or NULL if none exists yet.
* The <pData> parameter contains <length> bytes of additional option data.
*
* RETURNS: Pointer to new option subfield, or NULL if error occurs.
*
* NOMANUAL
*/

UCHAR * dhcpcOptionMerge
    (
    UCHAR	option, 	/* RFC 2132 tag of desired option */
    UCHAR * 	pOrig, 		/* current option subfield, or NULL if none */
    UCHAR * 	pData, 		/* additional/first option body */
    int 	length 		/* amount of new data */
    )
    {
    int optlen; 	/* Size of current subfield */
    int newlen; 	/* Size of new subfield */

    UCHAR * pDest;

    if (pOrig != NULL)
        optlen = * (pOrig + 1);
    else
        optlen = 0;

    optlen += 2; 	/* Adjust for tag and length values. */

    newlen = optlen + length;

    if (newlen > 255) 	/* Merged option exceeds maximum length. */
        return (NULL);

    pDest = malloc (newlen);
    if (pDest != NULL)
        {
        if (pOrig != NULL)
            {
            bcopy (pOrig, pDest, optlen);
            free (pOrig);
            }
        else
            *pDest = option;

        * (pDest + 1) = newlen;    /* Update length field to new value. */

        bcopy (pData, pDest + optlen, length);
        }

    return (pDest);
    }

/*******************************************************************************
*
* dhcpcOptCreate - generate a new subfield for variable-length options
*
* The dhcpcOptionAdd() routine uses this routine to create or replace an
* option subfield. The <pOrig> parameter contains the current option subfield,
* or NULL if none exists yet. The <pData> parameter contains <length> bytes of
* option data for the new (or initial) subfield.
*
* RETURNS: Pointer to new option subfield, or NULL if error occurs.
*
* NOMANUAL
*/

UCHAR * dhcpcOptCreate
    (
    UCHAR	option, 	/* RFC 2132 tag of desired option */
    UCHAR * 	pOrig, 		/* current option subfield, or NULL if none */
    UCHAR * 	pData, 		/* additional/first option body */
    int 	length 		/* amount of new data */
    )
    {
    UCHAR * pDest;

    pDest = malloc (length + 2);    /* Adjusts for tag and length */
    if (pDest != NULL)
        {
        *pDest = option;
        * (pDest + 1) = length;
        bcopy (pData, pDest + 2, length);

        if (pOrig != NULL)
            free (pOrig);
        }

    return (pDest);
    }

/*******************************************************************************
*
* dhcpcOptFieldCreate - fill in the options field for outgoing messages
*
* The boot-time and run-time DHCP clients use this routine to fill the
* options field for all outgoing messages from the available entries in
* the <pOptList> structure. Any memory used for variable-length subfields
* within that structure is released. The <pOptions> parameter indicates the
* destination for the options to be included in all outgoing messages. This
* routine ignores the contents for the "requested IP address" option, if
* present, since that option cannot always be included in the initial message.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void dhcpcOptFieldCreate
    (
    struct dhcpcOpts * 	pOptList, 	/* Available options for message */
    UCHAR *	 	pOptions 	/* Resulting message field */
    )
    {
    int 	loop;
    int 	optlen = 0;
    UCHAR * 	pData = NULL;
    UCHAR * 	pDest;
    BOOL 	copyFlag;

    pDest = pOptions;

    for (loop = 0; loop < MAXTAGNUM; loop++)
        {
        if (ISSET (pOptList->optmask, loop))
            {
            copyFlag = FALSE;

            /* 
             * For fixed size options, copy option body after inserting
             * option tag and length.
             */

            switch (loop)
                {
                /* 1 byte values */

                case _DHCP_IP_FORWARD_TAG: 		/* Option Tag 19 */
                    optlen = 1;
                    pData = &pOptList->tag19;
                    copyFlag = TRUE; 
                    break;    
                case _DHCP_NONLOCAL_SRCROUTE_TAG: 	/* Option Tag 20 */
                    optlen = 1;
                    pData = &pOptList->tag20;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_DEFAULT_IP_TTL_TAG: 		/* Option Tag 23 */
                    optlen = 1;
                    pData = &pOptList->tag23;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_ALL_SUBNET_LOCAL_TAG: 	/* Option Tag 27 */
                    optlen = 1;
                    pData = &pOptList->tag27;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_MASK_DISCOVER_TAG: 		/* Option Tag 29 */
                    optlen = 1;
                    pData = &pOptList->tag29;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_MASK_SUPPLIER_TAG: 		/* Option Tag 30 */
                    optlen = 1;
                    pData = &pOptList->tag30;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_ROUTER_DISCOVER_TAG: 	/* Option Tag 31 */
                    optlen = 1;
                    pData = &pOptList->tag31;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_TRAILER_TAG: 		/* Option Tag 34 */
                    optlen = 1;
                    pData = &pOptList->tag34;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_ETHER_ENCAP_TAG: 		/* Option Tag 36 */
                    optlen = 1;
                    pData = &pOptList->tag36;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_DEFAULT_TCP_TTL_TAG: 	/* Option Tag 37 */
                    optlen = 1;
                    pData = &pOptList->tag37;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_KEEPALIVE_GARBAGE_TAG: 	/* Option Tag 39 */
                    optlen = 1;
                    pData = &pOptList->tag39;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_NB_NODETYPE_TAG: 		/* Option Tag 46 */
                    optlen = 1;
                    pData = &pOptList->tag46;
                    copyFlag = TRUE; 
                    break;

                /* 2 byte values */

                case _DHCP_BOOTSIZE_TAG: 		/* Option Tag 13 */
                    optlen = 2;
                    pData = (UCHAR *)&pOptList->tag13;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_MAX_DGRAM_SIZE_TAG: 		/* Option Tag 22 */
                    optlen = 2;
                    pData = (UCHAR *)&pOptList->tag22;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_IF_MTU_TAG: 			/* Option Tag 26 */
                    optlen = 2;
                    pData = (UCHAR *)&pOptList->tag26;
                    copyFlag = TRUE; 
                    break;

                /* 4 byte values */

                case _DHCP_SUBNET_MASK_TAG: 		/* Option Tag 1 */
                    optlen = 4;
                    pData = (UCHAR *)&pOptList->tag1;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_TIME_OFFSET_TAG: 		/* Option Tag 2 */
                    optlen = 4;
                    pData = (UCHAR *)&pOptList->tag2;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_SWAP_SERVER_TAG: 		/* Option Tag 16 */
                    optlen = 4;
                    pData = (UCHAR *)&pOptList->tag16;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_MTU_AGING_TIMEOUT_TAG: 	/* Option Tag 24 */
                    optlen = 4;
                    pData = (UCHAR *)&pOptList->tag24;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_BRDCAST_ADDR_TAG: 		/* Option Tag 28 */
                    optlen = 4;
                    pData = (UCHAR *)&pOptList->tag28;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_ROUTER_SOLICIT_TAG: 		/* Option Tag 32 */
                    optlen = 4;
                    pData = (UCHAR *)&pOptList->tag32;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_ARP_CACHE_TIMEOUT_TAG: 	/* Option Tag 35 */
                    optlen = 4;
                    pData = (UCHAR *)&pOptList->tag35;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_KEEPALIVE_INTERVAL_TAG: 	/* Option Tag 38 */
                    optlen = 4;
                    pData = (UCHAR *)&pOptList->tag38;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_LEASE_TIME_TAG: 		/* Option Tag 51 */
                    optlen = 4;
                    pData = (UCHAR *)&pOptList->tag51;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_T1_TAG: 			/* Option Tag 58 */
                    optlen = 4;
                    pData = (UCHAR *)&pOptList->tag58;
                    copyFlag = TRUE; 
                    break;
                case _DHCP_T2_TAG: 			/* Option Tag 59 */
                    optlen = 4;
                    pData = (UCHAR *)&pOptList->tag59;
                    copyFlag = TRUE; 
                    break;
                }

            if (copyFlag)
                {
                *pDest++ = (UCHAR)loop;
                *pDest++ = (UCHAR)optlen;
                bcopy (pData, pDest, optlen);
                pDest += optlen;
                continue;
                }

            /* 
             * For variable length options, copy the entry (which already
             * includes the option tag and length) then release the memory.
             */

            switch (loop)
                {
                case _DHCP_MTU_PLATEAU_TABLE_TAG: 	/* Option Tag 25 */
                    pData = pOptList->pTag25;
                    break;
                case _DHCP_ROUTER_TAG: 			/* Option Tag 3 */
                    pData = pOptList->pTag3;
                    break;
                case _DHCP_TIME_SERVER_TAG: 		/* Option Tag 4 */
                    pData = pOptList->pTag4;
                    break;
                case _DHCP_NAME_SERVER_TAG: 		/* Option Tag 5 */
                    pData = pOptList->pTag5;
                    break;
                case _DHCP_DNS_SERVER_TAG: 		/* Option Tag 6 */
                    pData = pOptList->pTag6;
                    break;
                case _DHCP_LOG_SERVER_TAG: 		/* Option Tag 7 */
                    pData = pOptList->pTag7;
                    break;
                case _DHCP_COOKIE_SERVER_TAG: 		/* Option Tag 8 */
                    pData = pOptList->pTag8;
                    break;
                case _DHCP_LPR_SERVER_TAG: 		/* Option Tag 9 */
                    pData = pOptList->pTag9;
                    break;
                case _DHCP_IMPRESS_SERVER_TAG: 		/* Option Tag 10 */
                    pData = pOptList->pTag10;
                    break;
                case _DHCP_RLS_SERVER_TAG: 		/* Option Tag 11 */
                    pData = pOptList->pTag11;
                    break;
                case _DHCP_NIS_SERVER_TAG: 		/* Option Tag 41 */
                    pData = pOptList->pTag41;
                    break;
                case _DHCP_NTP_SERVER_TAG: 		/* Option Tag 42 */
                    pData = pOptList->pTag42;
                    break;
                case _DHCP_NBN_SERVER_TAG: 		/* Option Tag 44 */
                    pData = pOptList->pTag44;
                    break;
                case _DHCP_NBDD_SERVER_TAG: 		/* Option Tag 45 */
                    pData = pOptList->pTag45;
                    break;
                case _DHCP_XFONT_SERVER_TAG: 		/* Option Tag 48 */
                    pData = pOptList->pTag48;
                    break;
                case _DHCP_XDISPLAY_MANAGER_TAG: 	/* Option Tag 49 */
                    pData = pOptList->pTag49;
                    break;
                case _DHCP_NISP_SERVER_TAG: 		/* Option Tag 65 */
                    pData = pOptList->pTag65;
                    break;
                case _DHCP_MOBILEIP_HA_TAG: 		/* Option Tag 68 */
                    pData = pOptList->pTag68;
                    break;
                case _DHCP_SMTP_SERVER_TAG: 		/* Option Tag 69 */
                    pData = pOptList->pTag69;
                    break;
                case _DHCP_POP3_SERVER_TAG: 		/* Option Tag 70 */
                    pData = pOptList->pTag70;
                    break;
                case _DHCP_NNTP_SERVER_TAG: 		/* Option Tag 71 */
                    pData = pOptList->pTag71;
                    break;
                case _DHCP_DFLT_WWW_SERVER_TAG: 	/* Option Tag 72 */
                    pData = pOptList->pTag72;
                    break;
                case _DHCP_DFLT_FINGER_SERVER_TAG: 	/* Option Tag 73 */
                    pData = pOptList->pTag73;
                    break;
                case _DHCP_DFLT_IRC_SERVER_TAG:  	/* Option Tag 74 */
                    pData = pOptList->pTag74;
                    break;
                case _DHCP_STREETTALK_SERVER_TAG:  	/* Option Tag 75 */
                    pData = pOptList->pTag75;
                    break;
                case _DHCP_STDA_SERVER_TAG: 		/* Option Tag 76 */
                    pData = pOptList->pTag76;
                    break;
                case _DHCP_POLICY_FILTER_TAG: 		/* Option Tag 21 */
                    pData = pOptList->pTag21;
                    break;
                case _DHCP_STATIC_ROUTE_TAG: 		/* Option Tag 33 */
                    pData = pOptList->pTag33;
                    break;
                case _DHCP_VENDOR_SPEC_TAG: 		/* Option Tag 43 */
                    pData = pOptList->pTag43;
                    break;
                case _DHCP_REQ_LIST_TAG: 		/* Option Tag 55 */
                    pData = pOptList->pTag55;
                    break;
                case _DHCP_HOSTNAME_TAG: 		/* Option Tag 12 */
                    pData = pOptList->pTag12;
                    break;
                case _DHCP_MERIT_DUMP_TAG: 		/* Option Tag 14 */
                    pData = pOptList->pTag14;
                    break;
                case _DHCP_DNS_DOMAIN_TAG: 		/* Option Tag 15 */
                    pData = pOptList->pTag15;
                    break;
                case _DHCP_ROOT_PATH_TAG: 		/* Option Tag 17 */
                    pData = pOptList->pTag17;
                    break;
                case _DHCP_EXTENSIONS_PATH_TAG: 	/* Option Tag 18 */
                    pData = pOptList->pTag18;
                    break;
                case _DHCP_NIS_DOMAIN_TAG: 		/* Option Tag 40 */
                    pData = pOptList->pTag40;
                    break;
                case _DHCP_NB_SCOPE_TAG: 		/* Option Tag 47 */
                    pData = pOptList->pTag47;
                    break;
                case _DHCP_ERRMSG_TAG: 			/* Option Tag 56 */
                    pData = pOptList->pTag56;
                    break;
                case _DHCP_CLASS_ID_TAG: 		/* Option Tag 60 */
                    pData = pOptList->pTag60;
                    break;
                case _DHCP_CLIENT_ID_TAG: 		/* Option Tag 61 */
                    pData = pOptList->pTag61;
                    break;
                case _DHCP_NISP_DOMAIN_TAG: 		/* Option Tag 64 */
                    pData = pOptList->pTag64;
                    break;
                case _DHCP_TFTP_SERVERNAME_TAG: 	/* Option Tag 66 */
                    pData = pOptList->pTag66;
                    break;
                case _DHCP_BOOTFILE_TAG: 		/* Option Tag 67 */
                    pData = pOptList->pTag67;
                    break;
                }

            optlen = *(pData + 1);
            optlen += 2; 	/* Adjust for tag and length value. */
            bcopy (pData, pDest, optlen);
            pDest += optlen;

            free (pData);
            }    /* END: if (ISSET (<option value>) */
        }    /* END: loop */
    return;
    }

/******************************************************************************
*
* dhcpcDefaultsSet - assign host requirements defaults for client
*
* This routine fills the client's parameters structure with the default
* values specified in the Host Requirements Documents (RFC's 1122 and 1123),
* the Path MTU Discovery description (RFC 1191), or the Router Discovery
* specification (RFC 1256). This data is assigned before processing a message 
* received from a DHCP server, so that it can override the defaults.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void dhcpcDefaultsSet
    (
    struct dhcp_param * 	pNewParam
    )
    {
    /* Default IP layer parameters, per host. */

    pNewParam->ip_forward = FALSE;
    pNewParam->nonlocal_srcroute = FALSE;
    pNewParam->max_dgram_size = _HRD_MAX_DGRAM;
    pNewParam->default_ip_ttl = _HRD_IP_TTL;

    /* Default IP layer parameters, per interface. */

    pNewParam->intf_mtu = _HRD_MTU;
    pNewParam->all_subnet_local = FALSE;
    pNewParam->mask_discover = FALSE;
    pNewParam->mask_supplier = FALSE;
    pNewParam->router_discover = TRUE;
    pNewParam->router_solicit.s_addr = _HRD_ROUTER;

    /* Default link layer parameters, per interface. */

    pNewParam->trailer = FALSE;
    pNewParam->arp_cache_timeout = _HRD_ARP_TIME;
    pNewParam->ether_encap = FALSE;

    /* Default link layer parameters, per host. */

    pNewParam->default_tcp_ttl = _HRD_TCP_TTL;
    pNewParam->keepalive_inter = _HRD_TCP_TIME;
    pNewParam->keepalive_garba = FALSE;
    }

/******************************************************************************
*
* dhcpcEventAdd - send event notification to monitor task
*
* This routine adds event descriptors to the event queue for later handling 
* by the DHCP client monitor task. 
*
* If the <source> parameter is set to DHCP_USER_EVENT, the routine was called 
* by one of the following API routines: dhcpcInformGet(), dhcpcBind(),
* dhcpcRelease(), dhcpcVerify(), or dhcpcShutdown(). The <type> parameter
* indicates the corresponding action: DHCP_USER_INFORM to get additional
* configuration parameters without establishing a lease, DHCP_USER_BIND to
* initiate the lease negotiation process, DHCP_USER_VERIFY to verify an
* active lease, DHCP_USER_RELEASE to relinquish an active lease or
* remove additional parameters, and DHCP_USER_SHUTDOWN to release all sets
* of configuration parameters and disable the DHCP client library. 
*
* If the <source> parameter is set to DHCP_AUTO_EVENT, the routine was called
* in response to a timeout in the DHCP client state machine and the
* <type> parameter is set to DHCP_TIMEOUT.
*
* The <pLeaseId> parameter identifies the configuration parameters (usually
* associated with a lease) for timeout events and the first four user events.
*
* The <intFlag> parameter indicates if this routine is executing at interrupt 
* level. If it is FALSE, then interrupts must be locked out to prevent
* write conflicts to the event ring buffer between user requests and watchdog 
* timers.
*
* RETURNS: OK if event added successfully, or ERROR otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/ 

STATUS dhcpcEventAdd
    (
    int 	source, 	/* automatic event or user request */
    int 	type, 		/* event identifier */
    void * 	pLeaseId,	/* internal lease identifier */
    BOOL 	intFlag 	/* executing at interrupt level? */
    )
    {
    EVENT_DATA 	newEvent;
    int 	status;
    int 	key = 0;

    newEvent.source = source;
    newEvent.type = type;
    newEvent.leaseId = pLeaseId;

    /*
     * Add the event to the monitor task's event list. 
     * Disable interrupts if necessary. 
     */

    if (!intFlag)
        key = intLock ();
    status = rngBufPut (dhcpcEventRing, (char *)&newEvent, sizeof (newEvent));
    if (!intFlag)
        intUnlock (key);

    if (status != sizeof (newEvent))
        return (ERROR);

    /* Signal the monitor task of the event arrival. */

    status = semGive (dhcpcEventSem);
    if (status == ERROR)
        return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* dhcpcParamsCopy - copy current network parameters.
*
* This routine copies the network parameters to the caller-supplied structure.
* A run-time session calls this routine internally with the dhcpcParamsGet()
* routine and the <pLeaseData> parameter identifies the target lease. The DHCP
* boot-time session also uses this routine within the dhcpcBootParamsGet()
* routine. In that case, the parameters accessed through the lease structure
* for compatibility with the interface are actually available directly.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*
* INTERNAL
* The data structure accessed by this routine was defined by the original
* public domain code used as a basis for the DHCP implementation. The 
* structure tag and field names are not required to follow the coding 
* standards (and they don't).
*/

void dhcpcParamsCopy
    (
    LEASE_DATA * 	pLeaseData,
    struct dhcp_param * pParamList 
    )
    {
    int loop;
    int limit = 0;
    struct dhcp_param * pDhcpcParam;

    /*
     * The DHCP client parameters received from the server are stored in a 
     * large structure which either contains the actual parameter value 
     * or a pointer to the data. Because the parameters received are 
     * determined by the server configuration and many parameters are
     * optional, any of the fields could use the default value determined
     * earlier. In most cases, the default value is an empty field (or
     * zero in the case of numeric values). However, some parameters contain 
     * default settings specified by the host requirements documents which are 
     * used if the server does not provide a value. The following code copies 
     * any available parameters into the provided target structure. If 
     * necessary, it checks for empty fields. The code is divided into logical 
     * blocks according to the various data types used by the DHCP parameters.
     */

    pDhcpcParam = pLeaseData->dhcpcParam;

    /* Process all string parameters. */

    bcopy (pDhcpcParam->got_option, pParamList->got_option, OPTMASKSIZE);

    if (pParamList->sname != NULL && pDhcpcParam->sname != NULL)
        strcpy (pParamList->sname, pDhcpcParam->sname);

    if (pParamList->file  != NULL && pDhcpcParam->file != NULL)
        strcpy (pParamList->file, pDhcpcParam->file);

    if (pParamList->hostname != NULL && pDhcpcParam->hostname != NULL)
        strcpy (pParamList->hostname, pDhcpcParam->hostname);

    if (pParamList->merit_dump != NULL && pDhcpcParam->merit_dump != NULL)
        strcpy (pParamList->merit_dump, pDhcpcParam->merit_dump);

    if (pParamList->dns_domain != NULL && pDhcpcParam->dns_domain != NULL)
        strcpy (pParamList->dns_domain, pDhcpcParam->dns_domain);

    if (pParamList->root_path != NULL && pDhcpcParam->root_path != NULL)
        strcpy (pParamList->root_path, pDhcpcParam->root_path);

    if (pParamList->extensions_path != NULL && 
        pDhcpcParam->extensions_path != NULL)
        strcpy (pParamList->extensions_path, pDhcpcParam->extensions_path);

    if (pParamList->nis_domain != NULL && pDhcpcParam->nis_domain != NULL)
        strcpy (pParamList->nis_domain, pDhcpcParam->nis_domain);

    if (pParamList->nb_scope != NULL && pDhcpcParam->nb_scope != NULL)
        strcpy (pParamList->nb_scope, pDhcpcParam->nb_scope);

    if (pParamList->errmsg != NULL && pDhcpcParam->errmsg != NULL)
        strcpy (pParamList->errmsg, pDhcpcParam->errmsg);

    if (pParamList->nisp_domain != NULL && pDhcpcParam->nisp_domain != NULL)
        strcpy (pParamList->nisp_domain, pDhcpcParam->nisp_domain);

    /* Process all boolean parameters. */

    pParamList->ip_forward = pDhcpcParam->ip_forward;
    pParamList->nonlocal_srcroute = pDhcpcParam->nonlocal_srcroute;
    pParamList->all_subnet_local = pDhcpcParam->all_subnet_local;
    pParamList->mask_discover = pDhcpcParam->mask_discover;
    pParamList->mask_supplier = pDhcpcParam->mask_supplier;
    pParamList->router_discover = pDhcpcParam->router_discover;
    pParamList->trailer = pDhcpcParam->trailer;
    pParamList->ether_encap = pDhcpcParam->ether_encap;
    pParamList->keepalive_garba = pDhcpcParam->keepalive_garba;

    /* Process all single numeric parameters. */

    pParamList->time_offset = pDhcpcParam->time_offset;
    pParamList->bootsize = pDhcpcParam->bootsize;
    pParamList->max_dgram_size = pDhcpcParam->max_dgram_size;
    pParamList->default_ip_ttl = pDhcpcParam->default_ip_ttl;
    pParamList->mtu_aging_timeout = pDhcpcParam->mtu_aging_timeout;
    pParamList->intf_mtu = pDhcpcParam->intf_mtu;
    pParamList->arp_cache_timeout = pDhcpcParam->arp_cache_timeout;
    pParamList->default_tcp_ttl = pDhcpcParam->default_tcp_ttl;
    pParamList->keepalive_inter = pDhcpcParam->keepalive_inter;
    pParamList->nb_nodetype = pDhcpcParam->nb_nodetype;
    pParamList->lease_origin = pDhcpcParam->lease_origin;
    pParamList->lease_duration = pDhcpcParam->lease_duration;
    pParamList->dhcp_t1 = pDhcpcParam->dhcp_t1;
    pParamList->dhcp_t2 = pDhcpcParam->dhcp_t2;

    /* Process multiple numeric parameters. */

    if (pParamList->mtu_plateau_table != NULL &&
        pDhcpcParam->mtu_plateau_table != NULL)
        {
        limit = 0;
        if (pParamList->mtu_plateau_table->shortnum != NULL)
            {
            limit = (pParamList->mtu_plateau_table->num <
                     pDhcpcParam->mtu_plateau_table->num) ?
                     pParamList->mtu_plateau_table->num :
                     pDhcpcParam->mtu_plateau_table->num;
            for (loop = 0; loop < limit; loop++)
                {
                pParamList->mtu_plateau_table->shortnum [loop] =
                pDhcpcParam->mtu_plateau_table->shortnum [loop];
                }
            }
        pParamList->mtu_plateau_table->num = limit;
        }

    /* Process single IP addresses. */

    pParamList->server_id = pDhcpcParam->server_id;
    pParamList->ciaddr = pDhcpcParam->ciaddr;
    pParamList->yiaddr = pDhcpcParam->yiaddr;
    pParamList->siaddr = pDhcpcParam->siaddr;
    pParamList->giaddr = pDhcpcParam->giaddr;

    if (pParamList->subnet_mask != NULL && pDhcpcParam->subnet_mask != NULL)
        bcopy ( (char *)pDhcpcParam->subnet_mask,
               (char *)pParamList->subnet_mask, sizeof (struct in_addr));

    if (pParamList->swap_server != NULL && pDhcpcParam->swap_server != NULL)
        bcopy ( (char *)pDhcpcParam->swap_server,
               (char *)pParamList->swap_server, sizeof (struct in_addr));

    if (pParamList->brdcast_addr != NULL &&
        pDhcpcParam->brdcast_addr != NULL)
        bcopy ( (char *)pDhcpcParam->brdcast_addr,
               (char *)pParamList->brdcast_addr, sizeof (struct in_addr));

    pParamList->router_solicit = pDhcpcParam->router_solicit;

    /* Process multiple IP addresses. */

    if (pParamList->router != NULL && pDhcpcParam->router != NULL)
        {
        limit = 0;
        if (pParamList->router->addr != NULL)
            {
            limit = (pParamList->router->num < pDhcpcParam->router->num) ?
                     pParamList->router->num : pDhcpcParam->router->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->router->addr [loop],
                       (char *)&pParamList->router->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->router->num = limit;
        }

    if (pParamList->time_server != NULL && pDhcpcParam->time_server != NULL)
        {
        limit = 0;
        if (pParamList->time_server->addr != NULL)
            {
            limit = (pParamList->time_server->num <
                     pDhcpcParam->time_server->num) ?
                     pParamList->time_server->num :
                     pDhcpcParam->time_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->time_server->addr [loop],
                       (char *)&pParamList->time_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->time_server->num = limit;
        }

    if (pParamList->name_server != NULL && pDhcpcParam->name_server != NULL)
        {
        limit = 0;
        if (pParamList->name_server->addr != NULL)
            {
            limit = (pParamList->name_server->num <
                     pDhcpcParam->name_server->num) ?
                     pParamList->name_server->num :
                     pDhcpcParam->name_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->name_server->addr [loop],
                       (char *)&pParamList->name_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->name_server->num = limit;
        }

    if (pParamList->dns_server != NULL && pDhcpcParam->dns_server != NULL)
        {
        limit = 0;
        if (pParamList->dns_server->addr != NULL)
            {
            limit = (pParamList->dns_server->num <
                     pDhcpcParam->dns_server->num) ?
                     pParamList->dns_server->num :
                     pDhcpcParam->dns_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->dns_server->addr [loop],
                       (char *)&pParamList->dns_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->dns_server->num = limit;
        }

    if (pParamList->log_server != NULL && pDhcpcParam->log_server != NULL)
        {
        limit = 0;
        if (pParamList->log_server->addr != NULL)
            {
            limit = (pParamList->log_server->num <
                     pDhcpcParam->log_server->num) ?
                     pParamList->log_server->num :
                     pDhcpcParam->log_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->log_server->addr [loop],
                       (char *)&pParamList->log_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->log_server->num = limit;
        }

    if (pParamList->cookie_server != NULL && 
        pDhcpcParam->cookie_server != NULL)
        {
        limit = 0;
        if (pParamList->cookie_server->addr != NULL)
            {
            limit = (pParamList->log_server->num <
                     pDhcpcParam->log_server->num) ?
                     pParamList->log_server->num :
                     pDhcpcParam->log_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->cookie_server->addr [loop],
                       (char *)&pParamList->cookie_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->cookie_server->num = limit;
        }

    if (pParamList->lpr_server != NULL && pDhcpcParam->lpr_server != NULL)
        {
        limit = 0;
        if (pParamList->lpr_server->addr != NULL)
            {
            limit = (pParamList->lpr_server->num <
                     pDhcpcParam->lpr_server->num) ?
                     pParamList->lpr_server->num :
                     pDhcpcParam->lpr_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy( (char *)&pDhcpcParam->lpr_server->addr [loop],
                      (char *)&pParamList->lpr_server->addr [loop],
                      sizeof (struct in_addr));
                }
            }
        pParamList->lpr_server->num = limit;
        }

    if (pParamList->impress_server != NULL &&
        pDhcpcParam->impress_server != NULL)
        {
        limit = 0;
        if (pParamList->impress_server->addr != NULL)
            {
            limit = (pParamList->impress_server->num <
                     pDhcpcParam->impress_server->num) ?
                     pParamList->impress_server->num :
                     pDhcpcParam->impress_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->impress_server->addr [loop],
                       (char *)&pParamList->impress_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->impress_server->num = limit;
        }

    if (pParamList->rls_server != NULL && pDhcpcParam->rls_server != NULL)
        {
        limit = 0;
        if (pParamList->rls_server->addr != NULL)
            {
            limit = (pParamList->rls_server->num <
                     pDhcpcParam->rls_server->num) ?
                     pParamList->rls_server->num :
                     pDhcpcParam->rls_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->rls_server->addr [loop],
                       (char *)&pParamList->rls_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->rls_server->num = limit;
        }

    if (pParamList->nis_server != NULL && pDhcpcParam->nis_server != NULL)
        {
        limit = 0;
        if (pParamList->nis_server->addr != NULL)
            {
            limit = (pParamList->nis_server->num <
                     pDhcpcParam->nis_server->num) ?
                     pParamList->nis_server->num :
                     pDhcpcParam->nis_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->nis_server->addr [loop],
                       (char *)&pParamList->nis_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->nis_server->num = limit;
        }

    if (pParamList->ntp_server != NULL && pDhcpcParam->ntp_server != NULL)
        {
        limit = 0;
        if (pParamList->ntp_server->addr != NULL)
            {
            limit = (pParamList->ntp_server->num <
                     pDhcpcParam->ntp_server->num) ?
                     pParamList->ntp_server->num :
                     pDhcpcParam->ntp_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->ntp_server->addr [loop],
                       (char *)&pParamList->ntp_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->ntp_server->num = limit;
        }

    if (pParamList->nbn_server != NULL && pDhcpcParam->nbn_server != NULL)
        {
        limit = 0;
        if (pParamList->nbn_server->addr != NULL)
            {
            limit = (pParamList->nbn_server->num <
                     pDhcpcParam->nbn_server->num) ?
                     pParamList->nbn_server->num :
                     pDhcpcParam->nbn_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->nbn_server->addr [loop],
                       (char *)&pParamList->nbn_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->nbn_server->num = limit;
        }

    if (pParamList->nbdd_server != NULL && pDhcpcParam->nbdd_server != NULL)
        {
        limit = 0;
        if (pParamList->nbdd_server->addr != NULL)
            {
            limit = (pParamList->nbdd_server->num <
                     pDhcpcParam->nbdd_server->num) ?
                     pParamList->nbdd_server->num :
                     pDhcpcParam->nbdd_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->nbdd_server->addr [loop],
                       (char *)&pParamList->nbdd_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->nbdd_server->num = limit;
        }

    if (pParamList->xfont_server != NULL &&
        pDhcpcParam->xfont_server != NULL)
        {
        limit = 0;
        if (pParamList->xfont_server->addr != NULL)
            {
            limit = (pParamList->xfont_server->num <
                     pDhcpcParam->xfont_server->num) ?
                     pParamList->xfont_server->num :
                     pDhcpcParam->xfont_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->xfont_server->addr [loop],
                       (char *)&pParamList->xfont_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->xfont_server->num = limit;
        }

    if (pParamList->xdisplay_manager != NULL &&
        pDhcpcParam->xdisplay_manager != NULL)
        {
        limit = 0;
        if (pParamList->xdisplay_manager->addr != NULL)
            {
            limit = (pParamList->xdisplay_manager->num <
                     pDhcpcParam->xdisplay_manager->num) ?
                     pParamList->xdisplay_manager->num :
                     pDhcpcParam->xdisplay_manager->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->xdisplay_manager->addr [loop],
                       (char *)&pParamList->xdisplay_manager->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->xdisplay_manager->num = limit;
        }

    if (pParamList->nisp_server != NULL && pDhcpcParam->nisp_server != NULL)
        {
        limit = 0;
        if (pParamList->nisp_server->addr != NULL)
            {
            limit = (pParamList->nisp_server->num <
                     pDhcpcParam->nisp_server->num) ?
                     pParamList->nisp_server->num :
                     pDhcpcParam->nisp_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->nisp_server->addr [loop],
                       (char *)&pParamList->nisp_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->nisp_server->num = limit;
        }

    if (pParamList->mobileip_ha != NULL && pDhcpcParam->mobileip_ha != NULL)
        {
        limit = 0;
        if (pParamList->mobileip_ha->addr != NULL)
            {
            limit = (pParamList->mobileip_ha->num <
                     pDhcpcParam->mobileip_ha->num) ?
                     pParamList->mobileip_ha->num :
                     pDhcpcParam->mobileip_ha->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->mobileip_ha->addr [loop],
                       (char *)&pParamList->mobileip_ha->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->mobileip_ha->num = limit;
        }

    if (pParamList->smtp_server != NULL && pDhcpcParam->smtp_server != NULL)
        {
        limit = 0;
        if (pParamList->smtp_server->addr != NULL)
            {
            limit = (pParamList->smtp_server->num <
                     pDhcpcParam->smtp_server->num) ?
                     pParamList->smtp_server->num :
                     pDhcpcParam->smtp_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->smtp_server->addr [loop],
                       (char *)&pParamList->smtp_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->smtp_server->num = limit;
        }

    if (pParamList->pop3_server != NULL && pDhcpcParam->pop3_server != NULL)
        {
        limit = 0;
        if (pParamList->pop3_server->addr != NULL)
            {
            limit = (pParamList->pop3_server->num <
                     pDhcpcParam->pop3_server->num) ?
                     pParamList->pop3_server->num :
                     pDhcpcParam->pop3_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->pop3_server->addr [loop],
                       (char *)&pParamList->pop3_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->pop3_server->num = limit;
        }

    if (pParamList->nntp_server != NULL && pDhcpcParam->nntp_server != NULL)
        {
        limit = 0;
        if (pParamList->nntp_server->addr != NULL)
            {
            limit = (pParamList->nntp_server->num <
                     pDhcpcParam->nntp_server->num) ?
                     pParamList->nntp_server->num :
                     pDhcpcParam->nntp_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->nntp_server->addr [loop],
                       (char *)&pParamList->nntp_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->nntp_server->num = limit;
        }

    if (pParamList->dflt_www_server != NULL &&
        pDhcpcParam->dflt_www_server != NULL)
        {
        limit = 0;
        if (pParamList->dflt_www_server->addr != NULL)
            {
            limit = (pParamList->dflt_www_server->num <
                     pDhcpcParam->dflt_www_server->num) ?
                     pParamList->dflt_www_server->num :
                     pDhcpcParam->dflt_www_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->dflt_www_server->addr [loop],
                       (char *)&pParamList->dflt_www_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->dflt_www_server->num = limit;
        }

    if (pParamList->dflt_finger_server != NULL &&
        pDhcpcParam->dflt_finger_server != NULL)
        {
        limit = 0;
        if (pParamList->dflt_finger_server->addr != NULL)
            {
            limit = (pParamList->dflt_finger_server->num <
                     pDhcpcParam->dflt_finger_server->num) ?
                     pParamList->dflt_finger_server->num :
                     pDhcpcParam->dflt_finger_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->dflt_finger_server->addr [loop],
                       (char *)&pParamList->dflt_finger_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->dflt_finger_server->num = limit;
        }

    if (pParamList->dflt_irc_server != NULL &&
        pDhcpcParam->dflt_irc_server != NULL)
        {
        limit = 0;
        if (pParamList->dflt_irc_server->addr != NULL)
            {
            limit = (pParamList->dflt_irc_server->num <
                     pDhcpcParam->dflt_irc_server->num) ?
                     pParamList->dflt_irc_server->num :
                     pDhcpcParam->dflt_irc_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->dflt_irc_server->addr [loop],
                       (char *)&pParamList->dflt_irc_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->dflt_irc_server->num = limit;
        }

    if (pParamList->streettalk_server != NULL &&
        pDhcpcParam->streettalk_server != NULL)
        {
        limit = 0;
        if (pParamList->streettalk_server->addr != NULL)
            {
            limit = (pParamList->streettalk_server->num <
                     pDhcpcParam->streettalk_server->num) ?
                     pParamList->streettalk_server->num :
                     pDhcpcParam->streettalk_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->streettalk_server->addr [loop],
                       (char *)&pParamList->streettalk_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->streettalk_server->num = limit;
        }

    if (pParamList->stda_server != NULL &&
        pDhcpcParam->stda_server != NULL)
        {
        limit = 0;
        if (pParamList->stda_server->addr != NULL)
            {
            limit = (pParamList->stda_server->num <
                     pDhcpcParam->stda_server->num) ?
                     pParamList->stda_server->num :
                     pDhcpcParam->stda_server->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->stda_server->addr [loop],
                       (char *)&pParamList->stda_server->addr [loop],
                       sizeof (struct in_addr));
                }
            }
        pParamList->stda_server->num = limit;
        }

    /* Handle multiple IP address pairs. */

    if (pParamList->policy_filter != NULL &&
        pDhcpcParam->policy_filter != NULL)
        {
        limit = 0;
        if (pParamList->policy_filter->addr != NULL)
            {
            limit = (pParamList->policy_filter->num <
                     pDhcpcParam->policy_filter->num) ?
                     pParamList->policy_filter->num :
                     pDhcpcParam->policy_filter->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy( (char *)&pDhcpcParam->policy_filter->addr [loop * 2],
                      (char *)&pParamList->policy_filter->addr [loop * 2],
                      2 * sizeof (struct in_addr));
                }
            }
        pParamList->policy_filter->num = limit;
        }

    if (pParamList->static_route != NULL &&
        pDhcpcParam->static_route != NULL)
        {
        limit = 0;
        if (pParamList->static_route->addr != NULL)
            {
            limit = (pParamList->static_route->num <
                     pDhcpcParam->static_route->num) ?
                     pParamList->static_route->num :
                     pDhcpcParam->static_route->num;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)&pDhcpcParam->static_route->addr [loop * 2],
                       (char *)&pParamList->static_route->addr [loop * 2],
                       2 * sizeof (struct in_addr));
                }
            }
        pParamList->static_route->num = limit;
        }

    /* Handle lists. */

    if (pParamList->vendlist != NULL && pDhcpcParam->vendlist != NULL)
        {
        limit = (pParamList->vendlist->len < pDhcpcParam->vendlist->len) ?
                pParamList->vendlist->len : pDhcpcParam->vendlist->len;
        bcopy (pDhcpcParam->vendlist->list, pParamList->vendlist->list,
               limit);
        pParamList->vendlist->len = limit;
        }

    return;
    }

/* dhcprLib.c - DHCP relay agent library */

/* Copyright 1984 - 2001 Wind River Systems, Inc.  */
#include "copyright_wrs.h"

/*
modification history
--------------------
01p,22mar02,wap  don't start DHCP relay task if the relay table is empty
                 (SPR #74456)
01o,16nov01,spm  fixed modification history following merge
01n,15oct01,rae  merge from truestack ver 01q, base 01lj (SPR #69981)
01m,17nov00,spm  added support for BSD Ethernet devices
01l,24oct00,spm  fixed modification history after merge from tor3_0_x branch
01k,23oct00,niq  merged from version 01l of tor3_0_x branch (base version 01j);
                 upgrade to BPF replaces tagged frames support
01j,04dec97,spm  added code review modifications
01i,06oct97,spm  split interface name into device name and unit number; removed
                 reference to deleted endDriver global; added stub routine to
                 support delayed startup
01h,25sep97,gnn  SENS beta feedback fixes
01g,02sep97,spm  removed excess debug message (SPR #9149); corrected removal
                 of target list in cleanup routine
01f,26aug97,spm  reorganized code and added support for UDP port selection
01e,12aug97,gnn  changes necessitated by MUX/END update.
01d,02jun97,spm  updated man pages and added ERRNO entries
01c,06may97,spm  changed memory access to align IP header on four byte boundary
01b,10apr97,kbw  changed title line to match actual file name 
01a,07apr97,spm  created by modifying WIDE project DHCP implementation
*/

/*
DESCRIPTION
This library implements a relay agent for the Dynamic Host Configuration
Protocol (DHCP).  DHCP is an extension of BOOTP.  Like BOOTP, it allows a 
target to configure itself dynamically by using the network to get 
its IP address, a boot file name, and the DHCP server's address.  The relay 
agent forwards DHCP messages between clients and servers resident on 
different subnets.  The standard DHCP server, if present on a subnet, can 
also forward messages across subnet boundaries.  The relay agent is needed 
only if there is no DHCP server running on the subnet.  The dhcprLibInit()
routine links this library into the VxWorks system.  This happens automatically
if INCLUDE_DHCPR is defined at the time the system is built, as long as 
INCLUDE_DHCPS is <not> also defined.

HIGH-LEVEL INTERFACE
The dhcprInit() routine initializes the relay agent automatically.  The 
relay agent forwards incoming DHCP messages to the IP addresses specified 
at build time in the 'dhcpTargetTbl[]' array.

INTERNAL
The core relay agent code, derived from code developed by the WIDE project,
is located in the dhcpr.c module in the directory /wind/river/target/src/dhcp.

INCLUDE FILES: dhcprLib.h

SEE ALSO: RFC 1541, RFC 1533
*/

/* includes */

#include "dhcp/copyright_dhcp.h"
#include "vxWorks.h"

#include <stdio.h>
#include <stdlib.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <sys/ioctl.h>

#include "end.h"
#include "ipProto.h"

#include "logLib.h"
#include "rngLib.h"
#include "muxLib.h"
#include "semLib.h"
#include "sockLib.h"
#include "ioLib.h"

#include "dhcprLib.h"
#include "dhcp/dhcp.h"
#include "dhcp/common.h"
#include "dhcp/common_subr.h"

#include "bpfDrv.h"

/* defines */

#define _DHCPR_MAX_DEVNAME 21 	/* "/bpf/dhcpr" + max unit number */

/* globals */

IMPORT int 	dhcpSPort; 	/* Port used by DHCP servers */
IMPORT int 	dhcpCPort; 	/* Port used by DHCP clients */
IMPORT int 	dhcpMaxHops;    /* Hop limit before message is discarded. */

IMPORT struct iovec sbufvec[2];            /* send buffer */
IMPORT struct msg dhcprMsgOut;

int dhcprBufSize; 	/* Size of buffer for BPF devices */
char * pDhcprSendBuf; 	/* Buffer for transmitting messages */

IMPORT u_short dhcps_port;     /* Server port */
IMPORT u_short dhcpc_port;     /* Client port */

void dhcprCleanup (int checkpoint);

/* locals */

LOCAL BOOL dhcprInitialized = FALSE;

struct if_info *dhcprIntfaceList = NULL;

    /* Berkeley Packet Filter instructions for catching DHCP messages. */

LOCAL struct bpf_insn dhcpfilter[] = {
  BPF_STMT(BPF_LD+BPF_TYPE,0),                /* Save lltype in accumulator */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, ETHERTYPE_IP, 0, 20),  /* IP packet? */

  /*
   * The remaining statements use the (new) BPF_HLEN alias to avoid any
   * link-layer dependencies. The expected length values are assigned to the
   * correct values during startup. The expected destination port is also
   * altered to match the actual value chosen.
   */

  BPF_STMT(BPF_LD+BPF_H+BPF_ABS+BPF_HLEN, 6),    /* A <- IP FRAGMENT field */
  BPF_JUMP(BPF_JMP+BPF_JSET+BPF_K, 0x1fff, 18, 0),         /* OFFSET == 0 ? */

  BPF_STMT(BPF_LDX+BPF_HLEN, 0),          /* X <- frame data offset */

  BPF_STMT(BPF_LD+BPF_H+BPF_IND, 2),      /* A <- IP_LEN field */
  BPF_JUMP(BPF_JMP+BPF_JGE+BPF_K, 0, 0, 15),     /* IP/UDP headers + DHCP? */

  BPF_STMT(BPF_LD+BPF_B+BPF_IND, 9),      /* A <- IP_PROTO field */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, IPPROTO_UDP, 0, 13),     /* UDP ? */

  BPF_STMT(BPF_LD+BPF_HLEN, 0),           /* A <- frame data offset */
  BPF_STMT(BPF_LDX+BPF_B+BPF_MSH+BPF_HLEN, 0), /* X <- IPHDR LEN field */
  BPF_STMT(BPF_ALU+BPF_ADD+BPF_X, 0),     /* A <- start of UDP datagram */
  BPF_STMT(BPF_MISC+BPF_TAX, 0),          /* X <- start of UDP datagram */

  BPF_STMT(BPF_LD+BPF_H+BPF_IND, 2),      /* A <- UDP DSTPORT */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 67, 0, 7), /* check DSTPORT */

  BPF_STMT(BPF_LD+BPF_H+BPF_IND, 4),      /* A <- UDP LENGTH */
  BPF_JUMP(BPF_JMP+BPF_JGE+BPF_K, 0, 0, 5), /* UDP header + DHCP? */

  BPF_STMT(BPF_LD+BPF_B+BPF_IND, 11),      /* A <- DHCP hops field */
  BPF_JUMP(BPF_JMP+BPF_JGT+BPF_K, -1, 3, 0),   /* -1 replaced with max hops */
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

LOCAL struct bpf_program dhcpread = {
    sizeof (dhcpfilter) / sizeof (struct bpf_insn),
    dhcpfilter
    };

/*******************************************************************************
*
* dhcprLibInit - initialize DHCP relay agent library
*
* This routine links the DHCP relay agent code into the runtime image and
* allocates storage for incoming DHCP messages. The <maxSize> parameter
* specifies the maximum length supported for any DHCP message, including
* the UDP and IP headers and the largest link level header for all supported
* devices. The smallest valid value for the <maxSize> parameter is 576 bytes,
* corresponding to the minimum IP datagram a host must accept. Larger values
* will allow the relay agent to handle longer DHCP messages.
*
* This routine must be called before calling any other library routines.
* The routine is called automatically if INCLUDE_DHCPR is defined at the
* time the system is built and assigns the maximum size to the value
* specified by the DHCPR_MAX_MSGSIZE constant.
* 
* RETURNS: OK, or ERROR if initialization fails.
*
* ERRNO: N/A
*
* NOMANUAL
*/

STATUS dhcprLibInit
    (
    int 	maxSize 	/* largest DHCP message supported, in bytes */
    )
    {
    if (dhcprInitialized)
        return (OK);

    if (maxSize < DFLTDHCPLEN + UDPHL + IPHL)
        return (ERROR);

    if (bpfDrv () == ERROR)
        return (ERROR);

    pDhcprSendBuf = (char *)memalign (4, maxSize);
    if (pDhcprSendBuf == NULL)
        return (ERROR);

    dhcprBufSize = maxSize + sizeof (struct bpf_hdr);

    dhcprInitialized = TRUE;

    return (OK);
    }

/*******************************************************************************
*
* dhcprInit - set up the DHCP relay agent parameters and data structures
*
* This routine creates the necessary data structures to monitor the specified
* network interfaces for incoming DHCP messages and forward the messages to
* the given Internet addresses. It may be called after the dhcprLibInit()
* routine has initialized the global data structures.
*
* The <ppIf> argument provides a list of devices which the relay agent will
* monitor to forward packets. The relay agent supports devices attached
* to the IP protocol with the MUX/END interface or BSD Ethernet devices
* attached to that protocol. Each device must be capable of sending broadcast
* messages and the MTU size of the interface must be large enough to receive
* a minimum IP datagram of 576 bytes.
* 
* RETURNS: OK, or ERROR if could not initialize.
*
* ERRNO: N/A
*
* NOMANUAL
*/

STATUS dhcprInit
    (
    struct ifnet **	ppIf, 		/* network devices used by server */
    int			numDev, 	/* number of devices */
    DHCP_TARGET_DESC *  pTargetTbl, 	/* table of receiving DHCP servers */
    int                 targetSize 	/* size of DHCP server table */
    )
    {
    struct if_info *pIf = NULL;          /* pointer to interface */
    char bpfDevName [_DHCPR_MAX_DEVNAME];  /* "/bpf/dhcpr" + max unit number */
    int bpfDev;

    int loop;
    int result;
    struct ifreq ifr;

    if (!dhcprInitialized)
        return (ERROR);

    if (ppIf == NULL)
        return (ERROR);

    if (numDev == 0)
        return (ERROR);

    if (bpfDevCreate ("/bpf/dhcpr", numDev, dhcprBufSize) == ERROR)
        return (ERROR);

    /* Validate the MTU size for each device. */

    for (loop = 0; loop < numDev; loop++)
        {
        if (ppIf[loop]->if_mtu == 0)    /* No MTU size given: not attached? */
            return (ERROR);

        if (ppIf[loop]->if_mtu < DHCP_MSG_SIZE)
            return (ERROR);
        }

    /*
     * Alter the filter program to check for the correct length values and
     * use the specified UDP destination port and maximum hop count.
     */

      /* IP length must at least equal a DHCP message with 4 option bytes. */
    dhcpfilter [6].k = (DFLTDHCPLEN - DFLTOPTLEN + 4) + UDPHL + IPHL;

    dhcpfilter [14].k = dhcpSPort;   /* Use actual destination port in test. */

       /* UDP length must at least equal a DHCP message with 4 option bytes. */
    dhcpfilter [16].k = (DFLTDHCPLEN - DFLTOPTLEN + 4) + UDPHL;

    dhcpfilter [18].k = dhcpMaxHops;   /* Use real maximum hop count. */

    /* Store the interface control information and get each BPF device. */

    for (loop = 0; loop < numDev; loop++)
        {
        pIf = (struct if_info *)calloc (1, sizeof (struct if_info));
        if (pIf == NULL) 
            {
            logMsg ("Memory allocation error.\n", 0, 0, 0, 0, 0, 0);
            dhcprCleanup (1);
            return (ERROR);
            }
        pIf->buf = (char *)memalign (4, dhcprBufSize);
        if (pIf->buf == NULL)
            {
            logMsg ("Memory allocation error.\n", 0, 0, 0, 0, 0, 0);
            dhcprCleanup (1);
            return (ERROR);
            }
        bzero (pIf->buf, DHCP_MSG_SIZE);

        sprintf (bpfDevName, "/bpf/dhcpr%d", loop);

        bpfDev = open (bpfDevName, 0, 0);
        if (bpfDev < 0)
            {
            logMsg ("BPF device creation error.\n", 0, 0, 0, 0, 0, 0);
            dhcprCleanup (1);
            return (ERROR);
            }

        /* Enable immediate mode for reading messages. */

        result = 1;
        result = ioctl (bpfDev, BIOCIMMEDIATE, (int)&result);
        if (result != 0)
            {
            logMsg ("BPF device creation error.\n", 0, 0, 0, 0, 0, 0);
            dhcprCleanup (1);
            return (ERROR);
            }

        result = ioctl (bpfDev, BIOCSETF, (int)&dhcpread);
        if (result != 0)
            {
            logMsg ("BPF filter assignment error.\n", 0, 0, 0, 0, 0, 0);
            dhcprCleanup (1);
            return (ERROR);
            }

        bzero ( (char *)&ifr, sizeof (struct ifreq));
        sprintf (ifr.ifr_name, "%s%d", ppIf[loop]->if_name,
                                       ppIf[loop]->if_unit);
        result = ioctl (bpfDev, BIOCSETIF, (int)&ifr);
        if (result != 0)
            {
            logMsg ("BPF interface attachment error.\n", 0, 0, 0, 0, 0, 0);
            dhcprCleanup (1);
            return (ERROR);
            }

        pIf->next = dhcprIntfaceList;
        dhcprIntfaceList = pIf;

        /* Fill in device name and hardware address. */
        sprintf (pIf->name, "%s", ppIf[loop]->if_name);
        pIf->unit = ppIf[loop]->if_unit;
        pIf->bpfDev = bpfDev;
        pIf->mtuSize = ppIf[loop]->if_mtu;
        }

    /* Access target DHCP server data. */

    pDhcpRelayTargetTbl = pTargetTbl;

  /* read database of DHCP servers */

    if (targetSize != 0)
        read_server_db (targetSize); 

    if (dhcpNumTargets == 0)
        {
        logMsg ("DHCP relay agent server table is empty -- aborting.\n",
                0, 0, 0, 0, 0, 0);
        dhcprCleanup(2);
        return (ERROR);
        }

    /* Use defined ports for client and server. */

    dhcps_port = htons (dhcpSPort);
    dhcpc_port = htons (dhcpCPort);

    /* Fill in subnet mask and IP address for each monitored interface. */

    pIf = dhcprIntfaceList;
    while (pIf != NULL)
        { 
        if (open_if (pIf) < 0) 
            {
            dhcprCleanup (2);
            return (ERROR);
            }
        pIf = pIf->next;
        }

    return (OK);
    }

/*******************************************************************************
*
* dhcprCleanup - remove data structures
*
* This routine frees all dynamically allocated memory obtained by the DHCP
* relay agent.  It is called at multiple points before the program exits due to
* an error occurring or manual shutdown.  The checkpoint parameter indicates 
* which data structures have been created.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void dhcprCleanup 
    (
    int checkpoint 	/* Progress identifier indicating created resources */
    )
    {
    int current = 0;
    struct if_info *pIf;
    DHCP_SERVER_DESC * 	pServer;

    int loop;

    /* Checkpoint 0 is empty. */

    current++;
    if (current > checkpoint)
        return;
    while (dhcprIntfaceList != NULL)         /* Checkpoint 1 */
        {
        pIf = dhcprIntfaceList;
        if (pIf->buf != NULL)
            {
            if (pIf->bpfDev >= 0)
                close (pIf->bpfDev);
            free (pIf->buf);
            }
        dhcprIntfaceList = dhcprIntfaceList->next;
        free (pIf);
        }

    bpfDevDelete ("/bpf/dhcpr");

    current++;
    if (current > checkpoint)
        return;

    /* Remove elements of circular list created by read_server_db(). */

                                             /* Checkpoint 2 */
    for (loop = 0; loop < dhcpNumTargets; loop++)
        {
        pServer = pDhcpTargetList;
        pDhcpTargetList = pDhcpTargetList->next;
        free (pServer);
        }
    current++;
    if (current > checkpoint)
        return;

    return;
    }

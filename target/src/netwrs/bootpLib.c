/* bootpLib.c - Bootstrap Protocol (BOOTP) client library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01x,07dec01,wap  Use htons() to set ip_len and ip_id fields in IP header,
                 and htonl() to set the XID field in the packet body/BPF filter
                 so that BOOTP works on little-endian targets (SPR #72059)
01w,15oct01,rae  merge from truestack ver 01z, base 01v
01v,17mar99,spm  added support for identical unit numbers (SPR #20913)
01u,04sep98,ham  corrected lack of params for etherInputHookAdd(),SPR#21909
01t,28aug98,n_s  corrected MAC address comparison in bootpInputHook. spr #20902
01s,17jul97,dgp  doc: correct unsupported interfaces per SPR 8940
02c,14dec97,jdi  doc: cleanup.
02b,10dec97,gnn  making man page fixes
02a,08dec97,gnn  END code review fixes
01z,03dec97,spm  corrected parameter description for bootpMsgSend (SPR #9401);
                 minor changes to man pages and code spacing
01y,03oct97,gnn  removed references to endDriver global.
01x,25sep97,gnn  SENS beta feedback fixes
01w,26aug97,spm  fixed bootpParamsGet - gateway not retrieved (SPR #9137)
01v,12aug97,gnn  changes necessitated by MUX/END update.
01u,30apr97,jag  man page edit for function bootParamsGet()
01t,07apr97,spm  changed BOOTP interface to DHCP style: all options supported
01s,17dec96,gnn  added code to handle the new etherHooks and END stuff.
01r,08nov96,spm  Updated example of bootpParamsGet() for SPR 7120
01q,22sep96,spm  Fixed SPR 7120: added support for gateways to bootpParamsGet()
01p,01feb96,gnn	 added the end of vendor data (0xff) to the request packet
		 we send
01o,16jan94,rhp  fix typo in library man page
01n,17oct94,rhp  remove docn reference to bootpdLib (SPR#2351)
01n,22sep92,jdi  documentation cleanup.
01m,14aug92,elh  documentation changes.
01l,11jun92,elh  modified parameters to bootpParamsGet.
01k,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01j,16apr92,elh	 moved routines shared by icmp to icmpLib.
01i,28feb92,elh  ansified.
01h,27aug91,elh  rewritten to use standard bootp protocol,
		 redesigned to be more modular, rewrote documentation.
01g,15aug90,dnw  added slot parameter to bootpParamsGet()
	   +hjb  fixed bug in bootpForwarder() not setting hw addr in arp ioctl
01f,12aug90,dnw  changed bootpParamsGet() to check every tick for reply message
                 instead of every 4 seconds, for faster response.
		 changed bootpParamsGet() to print '.' every time it broadcasts
                 request.
01e,12aug90,hjb  major redesign and implementation of the protocol
01d,07may90,hjb  made bootp IP checksum portable; modifications to the protocol
		 for better forwarding service.
01c,19apr90,hjb  minor fixups bootpRequestSend(), added protocol extension
		 to solve the routing problem when bootp forwarder is used.
01b,11apr90,hjb  de-linted.
01a,11mar90,hjb  written.
*/

/*
DESCRIPTION
This library implements the client side of the Bootstrap Protocol
(BOOTP).  This protocol allows a host to initialize automatically by
 obtaining its IP address, boot file name, and boot host's IP address over
a network. The bootpLibInit() routine links this library into the
VxWorks image. This happens automatically if INCLUDE_BOOTP is defined
at the time the image is built.

CONFIGURATION INTERFACE
When used during boot time, the BOOTP library attempts to retrieve
the required configuration information from a BOOTP server using
the interface described below. If it is successful, the remainder
of the boot process continues as if the information were entered manually.

HIGH-LEVEL INTERFACE
The bootpParamsGet() routine retrieves a set of configuration parameters
according to the client-server interaction described in RFC 951 and
clarified in RFC 1542. The parameter descriptor structure it accepts as
an argument allows the retrieval of any combination of the options described
in RFC 1533 (if supported by the BOOTP server and specified in the database).
During the default system boot process, the routine obtains the boot file, the
Internet address, and the host Internet address.  It also obtains the subnet
mask and the Internet address of an IP router, if available.

LOW-LEVEL INTERFACE
The bootpMsgGet() routine transmits an arbitrary BOOTP request message and
provides direct access to any reply. This interface provides a method for
supporting alternate BOOTP implementations which may not fully comply with
the recommended behavior in RFC 1542. For example, it allows transmission
of BOOTP messages to an arbitrary UDP port and provides access to the
vendor-specific field to handle custom formats which differ from the RFC 1533
implementation. The bootpParamsGet() routine already extracts all options
 which that document defines.

EXAMPLE
The following code fragment demonstrates use of the BOOTP library: 
.CS
    #include "bootpLib.h"

    #define _MAX_BOOTP_RETRIES 	1

    struct bootpParams 	bootParams;

    struct in_addr 	clntAddr;
    struct in_addr 	hostAddr;
    char 		bootFile [SIZE_FILE];
    int 		subnetMask;
    struct in_addr_list routerList;
    struct in_addr 	gateway;

    struct ifnet * 	pIf;

    /@ Retrieve the interface descriptor of the transmitting device. @/

    pIf = ifunit ("ln0");
    if (pIf == NULL)
        {
        printf ("Device not found.\n");
        return (ERROR);
        }

    /@ Setup buffers for information from BOOTP server. @/

    bzero ( (char *)&clntAddr, sizeof (struct in_addr));
    bzero ( (char *)&hostAddr, sizeof (struct in_addr));
    bzero (bootFile, SIZE_FILE);
    subnetMask  = 0;
    bzero ( (char *)&gateway, sizeof (struct in_addr));

    /@ Set all pointers in parameter descriptor to NULL. @/

    bzero ((char *)&bootParams, sizeof (struct bootpParams));

    /@ Set pointers corresponding to desired options. @/

    bootParams.netmask = (struct in_addr *)&subnetMask;
    routerlist.addr = &gateway;
    routerlist.num = 1;
    bootParams.routers = &routerlist;

    /@
     @ Send request and wait for reply, retransmitting as necessary up to
     @ given limit. Copy supplied entries into buffers if reply received.
     @/

    result = bootpParamsGet (pIf, _MAX_BOOTP_RETRIES,
                          &clntAddr, &hostAddr, NULL, bootFile, &bootParams);
    if (result != OK)
        return (ERROR);
.CE

INCLUDE FILES: bootpLib.h

SEE ALSO: RFC 951, RFC 1542, RFC 1533,

INTERNAL
The diagram below defines the structure chart of bootpLib.


                                   
   |             |               
   v             v                
bootpLibInit bootpParamsGet	
	     / 		\
	    |	 	 |
	    v		 v	     
      bootpMsgGet   bootpParamsFill
*/

/* includes */

#include "vxWorks.h"
#include "ioLib.h"
#include "bootpLib.h"
#include "m2Lib.h"      /* M2_blah  */
#include "bpfDrv.h"
#include "vxLib.h" 	/* checksum() declaration */
#include "netinet/ip.h"
#include "netinet/udp.h"
#include "netinet/if_ether.h"
#include "stdio.h" 	/* sprintf() declaration */
#include "stdlib.h" 	/* rand() declaration */
#include "end.h"
#include "muxLib.h"

/* defines */

#define _BOOTP_MAX_DEVNAME 	13 		/* "/bpf/bootpc0" */

/* locals */

LOCAL int       bootpConvert (int);
LOCAL BOOL 	bootpInitialized;

LOCAL struct
    {
    struct ip           ih;             /* IP header            */
    struct udphdr       uh;             /* UDP header           */
    BOOTP_MSG           bp;             /* Bootp message        */
    } bootpMsg;

LOCAL char * pMsgBuffer; 	/* Storage for BOOTP replies from BPF. */
LOCAL int bootpBufSize; 	/* Size of storage buffer. */

    /* Berkeley Packet Filter instructions for catching BOOTP messages. */

LOCAL struct bpf_insn bootpfilter[] = {
  BPF_STMT(BPF_LD+BPF_TYPE,0),                /* Save lltype in accumulator */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, ETHERTYPE_IP, 0, 22),  /* IP packet? */

  /*
   * The remaining statements use the (new) BPF_HLEN alias to avoid any
   * link-layer dependencies. The expected destination port and transaction
   * ID values are altered when necessary by the BOOTP routines to match the
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
  BPF_JUMP(BPF_JMP+BPF_JGE+BPF_K, BOOTPLEN + UDPHL + IPHL, 0, 9),
                                                 /* Correct IP length? */

  BPF_STMT(BPF_LD+BPF_H+BPF_IND, 4),      /* A <- UDP LEN field */
  BPF_JUMP(BPF_JMP+BPF_JGE+BPF_K, BOOTPLEN + UDPHL, 0, 7),
                                                 /* Correct UDP length? */

  BPF_STMT(BPF_LD+BPF_B+BPF_IND, 8),      /* A <- BOOTP op field */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, BOOTREPLY, 0, 5),
  BPF_STMT(BPF_LD+BPF_W+BPF_IND, 12),      /* A <- BOOTP xid field */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, -1, 0, 3),   /* -1 replaced with real xid */
  BPF_STMT(BPF_LD+BPF_W+BPF_IND, 244),    /* A <- BOOTP options */
  BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, 0x63825363, 0, 1),
                                                 /* Matches magic cookie? */

  BPF_STMT(BPF_RET+BPF_K+BPF_HLEN, BOOTPLEN + UDPHL + IPHL),
                                           /*
                                            * ignore data beyond expected
                                            * size (some drivers add padding).
                                            */
  BPF_STMT(BPF_RET+BPF_K, 0)          /* unrecognized message: ignore frame */
  };

LOCAL struct bpf_program bootpread = {
    sizeof (bootpfilter) / sizeof (struct bpf_insn),
    bootpfilter
    };

LOCAL u_char		magicCookie1048 [4] = VM_RFC1048;
							/* 1048 cookie type  */
#define VEOF_RFC1048 {255}

LOCAL u_char		endOfVend[1] = VEOF_RFC1048;

LOCAL void bootpParamsFill (struct in_addr *, struct in_addr *, char *,
                            char *, struct bootpParams *, BOOTP_MSG *);
LOCAL u_char * bootpTagFind (u_char *, int, int *);

/*******************************************************************************
*
* bootpLibInit - BOOTP client library initialization
*
* This routine creates and initializes the global data structures used by
* the BOOTP client library to obtain configuration parameters. The <maxSize>
* parameter specifies the largest link level header for all supported devices.
* This value determines the maximum length of the outgoing IP packet containing
* a BOOTP message.
*
* This routine must be called before using any other library routines. The
* routine is called automatically if INCLUDE_BOOTP is defined at the time
* the system is built and uses the BOOTP_MAXSIZE configuration setting
* for the <maxSize> parameter.
*
* RETURNS: OK, or ERROR if initialization fails.
*
* ERRNO: S_bootpLib_MEM_ERROR;
*
*/

STATUS bootpLibInit
    (
    int 	maxSize 	/* largest link-level header, in bytes */
    )
    {
    int bufSize; 	/* Size of receive buffer (BOOTP msg + BPF header) */

    if (bootpInitialized)
        return (OK);

    if (bpfDrv () == ERROR)
        {
        return (ERROR);
        }

    bufSize = maxSize + BOOTPLEN + UDPHL + IPHL + sizeof (struct bpf_hdr);
    if (bpfDevCreate ("/bpf/bootpc", 1, bufSize) == ERROR)
        {
        bpfDrvRemove ();
        return (ERROR);
        }

    /* Allocate receive buffer based on maximum message size. */

    pMsgBuffer = memalign (4, bufSize);
    if (pMsgBuffer == NULL)
        {
        bpfDevDelete ("/bpf/bootpc");
        bpfDrvRemove ();
        errno = S_bootpLib_MEM_ERROR;
        return (ERROR);
        }

    bootpBufSize = bufSize;

    bootpInitialized = TRUE;

    return (OK);
    }


/******************************************************************************
*
* bootpParamsGet - retrieve boot parameters using BOOTP
*
* This routine performs a BOOTP message exchange according to the process
* described in RFC 1542, so the server and client UDP ports are always
* equal to the defined values of 67 and 68.
*
* The <pIf> argument indicates the network device which will be used to send
* and receive BOOTP messages. The BOOTP client only supports devices attached
* to the IP protocol with the MUX/END interface. The MTU size must be large
* enough to receive an IP packet of 328 bytes (corresponding to the BOOTP
* message length of 300 bytes). The specified device also must be capable of
* sending broadcast messages, unless this routine sends the request messages
* directly to the IP address of a specific server.
*
* The <maxSends> parameter specifies the total number of requests before
* before this routine stops waiting for a reply. After the final request,
* this routine will wait for the current interval before returning error.
* The timeout interval following each request follows RFC 1542, beginning
* at 4 seconds and doubling until a maximum limit of 64 seconds.
*
* The <pClientAddr> parameter provides optional storage for the assigned
* IP address from the `yiaddr' field of a BOOTP reply. Since this routine
* can execute before the system is capable of accepting unicast datagrams
* or responding to ARP requests for a specific IP address, the corresponding
* `ciaddr' field in the BOOTP request message is equal to zero.
*
* The <pServerAddr> parameter provides optional storage for the IP address
* of the responding server (from the `siaddr' field of a BOOTP reply).
* This routine broadcasts the BOOTP request message unless this buffer
* is available (i.e. not NULL) and contains the explicit IP address of a
* BOOTP server as a non-zero value.
*
* The <pHostName> parameter provides optional storage for the server's
* host name (from the `sname' field of a BOOTP reply). This routine also
* copies any initial string in that buffer into the `sname' field of the
* BOOTP request (which restricts booting to a specified host).
*
* The <pBootFile> parameter provides optional storage for the boot file
* name (from the `file' field of a BOOTP reply). This routine also copies
* any initial string in that buffer into the `file' field of the BOOTP
* request message, which typically supplies a generic name to the server.
*
* The remaining fields in the BOOTP request message use the values which
* RFC 1542 defines. In particular, the `giaddr' field is set to zero and
* the suggested "magic cookie" is always inserted in the (otherwise empty)
* `vend' field.

* The <pBootpParams> argument provides access to any options defined in
* RFC 1533 using the following definition:
*
* .CS
*    struct bootpParams
*        {
*        struct in_addr *            netmask;
*        unsigned short *            timeOffset;
*        struct in_addr_list *       routers;
*        struct in_addr_list *       timeServers;
*        struct in_addr_list *       nameServers;
*        struct in_addr_list *       dnsServers;
*        struct in_addr_list *       logServers;
*        struct in_addr_list *       cookieServers;
*        struct in_addr_list *       lprServers;
*        struct in_addr_list *       impressServers;
*        struct in_addr_list *       rlpServers;
*        char *                      clientName;
*        unsigned short *            filesize;
*        char *                      dumpfile;
*        char *                      domainName;
*        struct in_addr *            swapServer;
*        char *                      rootPath;
*        char *                      extoptPath;
*        unsigned char *             ipForward;
*        unsigned char *             nonlocalSourceRoute;
*        struct in_addr_list *       policyFilter;
*        unsigned short *            maxDgramSize;
*        unsigned char *             ipTTL;
*        unsigned long *             mtuTimeout;
*        struct ushort_list *        mtuTable;
*        unsigned short *            intfaceMTU;
*        unsigned char *             allSubnetsLocal;
*        struct in_addr *            broadcastAddr;
*        unsigned char *             maskDiscover;
*        unsigned char *             maskSupplier;
*        unsigned char *             routerDiscover;
*        struct in_addr *            routerDiscAddr;
*        struct in_addr_list *       staticRoutes;
*        unsigned char *             arpTrailers;
*        unsigned long *             arpTimeout;
*        unsigned char *             etherPacketType;
*        unsigned char *             tcpTTL;
*        unsigned long *             tcpInterval;
*        unsigned char *             tcpGarbage;
*        char *                      nisDomain;
*        struct in_addr_list *       nisServers;
*        struct in_addr_list *       ntpServers;
*        char *                      vendString;
*        struct in_addr_list *       nbnServers;
*        struct in_addr_list *       nbddServers;
*        unsigned char *             nbNodeType;
*        char *                      nbScope;
*        struct in_addr_list *       xFontServers;
*        struct in_addr_list *       xDisplayManagers;
*        char *                      nispDomain;
*        struct in_addr_list *       nispServers;
*        struct in_addr_list *       ipAgents;
*        struct in_addr_list *       smtpServers;
*        struct in_addr_list *       pop3Servers;
*        struct in_addr_list *       nntpServers;
*        struct in_addr_list *       wwwServers;
*        struct in_addr_list *       fingerServers;
*        struct in_addr_list *       ircServers;
*        struct in_addr_list *       stServers;
*        struct in_addr_list *       stdaServers; 
*        };
* .CE
*
* This structure allows the retrieval of any BOOTP option specified in
* RFC 1533. The list of 2-byte (unsigned short) values is defined as:
*
* .CS
*    struct ushort_list
*        {
*        unsigned char       num;
*        unsigned short *    shortlist;
*        };
* .CE
*
* The IP address lists use the following similar definition:
*
* .CS
*    struct in_addr_list
*        {
*        unsigned char       num;
*        struct in_addr *    addrlist;
*        };
* .CE
*
* When these lists are present, the routine stores values retrieved from
* the BOOTP reply in the location indicated by the `shortlist' or `addrlist'
* members.  The amount of space available is indicated by the `num' member.
* When the routine returns, the `num' member indicates the actual number of
* entries retrieved.  In the case of `bootpParams.policyFilter.num' 
* and `bootpParams.staticRoutes.num', the `num' member value should be 
* interpreted as the number of IP address pairs requested and received.
*
* .LP
* The contents of the BOOTP parameter descriptor implicitly selects options
* for retrieval from the BOOTP server.  This routine attempts to retrieve the
* values for any options whose corresponding field pointers are non-NULL
* values.  To obtain these parameters, the BOOTP server must support the
* vendor-specific options described in RFC 1048 (or its successors) and the 
* corresponding parameters must be specified in the BOOTP server database. 
* Where meaningful, the values are returned in host byte order. 
*
* The BOOTP request issued during system startup with this routine attempts
* to retrieve a subnet mask for the boot device, in addition to the host and
* client addresses and the boot file name.
*
* RETURNS: OK, or ERROR if unsuccessful.
*
* SEE ALSO: bootLib, RFC 1048, RFC 1533
*/

STATUS bootpParamsGet
    (
    struct ifnet * 	pIf, 		/* network device used by client */
    u_int 		maxSends,	/* maximum transmit attempts */
    struct in_addr * 	pClientAddr, 	/* retrieved client address buffer */
    struct in_addr * 	pServerAddr, 	/* buffer for server's IP address */
    char * 		pHostName, 	/* 64 byte (max) host name buffer */
    char * 		pBootFile, 	/* 128 byte (max) file name buffer */
    struct bootpParams * 	pBootpParams 	/* parameters descriptor     */
    )
    {
    BOOTP_MSG		bootpMessage;	/* bootp message		*/
    struct in_addr	ipDest;		/* ip dest address 		*/
    int length;
    int htype;
    int maxSize; 	/* Largest BOOTP message available from BPF device. */

    if (!bootpInitialized)
        {
        errno = S_bootpLib_NOT_INITIALIZED;
        return (ERROR);
        }

    if (pIf == NULL)
        {
        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    if (muxDevExists (pIf->if_name, pIf->if_unit) == FALSE)
        {
        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    /* Verify interface data sizes are appropriate for message. */

    if (pIf->if_mtu == 0)
        {
        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    length = pIf->if_mtu;
    if (length < BOOTPLEN + UDPHL + IPHL)
        {
        /*
         * Devices must accept messages equal to an IP datagram
         * of 328 bytes, which corresponds to the fixed-size BOOTP
         * message with the minimum headers.
         */

        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    length = pIf->if_hdrlen + BOOTPLEN + UDPHL + IPHL;
    maxSize = bootpBufSize - sizeof (struct bpf_hdr);
    if (length > maxSize)
        {
        /* Link level header exceeds maximum supported value. */

        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    htype = bootpConvert (pIf->if_type);
    if (htype == ERROR)
        {
        /* Unknown device type. Can't encode with RFC 1700 value. */

        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    /* Fill in BOOTP request message. */

    bzero ((char *) &bootpMessage, sizeof (BOOTP_MSG));
    bootpMessage.bp_op = BOOTREQUEST;
    bootpMessage.bp_htype = htype;

    bootpMessage.bp_hlen = pIf->if_addrlen;

    if (bootpMessage.bp_hlen > SIZE_HLEN)
        bootpMessage.bp_hlen = SIZE_HLEN;

    bcopy ( (char *) ( (struct arpcom *)pIf)->ac_enaddr,
           bootpMessage.bp_chaddr, bootpMessage.bp_hlen);

    /* Check for specific boot host name. */

    if (pHostName != NULL)
        {
        length = strlen (pHostName);
        if (length >= SIZE_SNAME) 	/* Leave space for EOS character. */
            length = SIZE_SNAME - 1;
        (void) strncpy ( (char *) bootpMessage.bp_sname, pHostName, length);
        }

    /* Check for partial boot file. */

    if (pBootFile != NULL)
        {
        length = strlen (pBootFile);
        if (length >= SIZE_FILE) 	/* Leave space for EOS character. */
            length = SIZE_FILE - 1;
        (void) strncpy ( (char *) bootpMessage.bp_file, pBootFile, length);
        }

    /* Fill in RFC 1048 magic cookie. */

    bcopy ( (char *) magicCookie1048, (char *) bootpMessage.bp_vend,
            sizeof (magicCookie1048));
    bcopy ( (char *) endOfVend, (char *) bootpMessage.bp_vend +
           sizeof(magicCookie1048), sizeof(endOfVend));

    /* Check for specific BOOTP server's IP address, or use broadcast. */

    if (pServerAddr == NULL || pServerAddr->s_addr == 0)
        ipDest.s_addr = INADDR_BROADCAST;
    else
        ipDest.s_addr = pServerAddr->s_addr;

    /* Send BOOTP request and retrieve reply using the reserved ports. */

    if (bootpMsgGet (pIf, &ipDest, _BOOTP_CPORT, _BOOTP_SPORT,
                     &bootpMessage, maxSends) == ERROR)
        return (ERROR);

    /* Fill in any entries requested by user and provided by server. */

    bootpParamsFill (pClientAddr, pServerAddr, pHostName, pBootFile,
                     pBootpParams, &bootpMessage);

    return (OK);
    }

/******************************************************************************
*
* bootpParamsFill - copy requested BOOTP options
*
* This routine fills in the non-NULL fields in the given parameter descriptor
* with the corresponding entries from the received BOOTP message.  It is only
* called internally after bootpMsgGet() completes successfully.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL void bootpParamsFill
    (
    struct in_addr * 	pClientAddr,    /* buffer for client address */
    struct in_addr *    pServerAddr,    /* buffer for server's IP address */
    char *              pHostName,      /* 64 byte (max) host name buffer */
    char *              pBootFile,      /* 128 byte (max) file name buffer */
    struct bootpParams * pBootpParams, 	/* parameters descriptor */
    BOOTP_MSG * 	pBootpReply 	/* reply from BOOTP server */
    )
    {
    int 	loop;
    int 	limit;
    u_char * 	cp;
    int length;
    int number;

    /* Copy entries from message body if requested by user. */

        /* Fill in assigned IP address. */

    if (pClientAddr != NULL)
        *pClientAddr = pBootpReply->bp_yiaddr;

        /* Fill in boot server's IP address. */

    if (pServerAddr != NULL)
        *pServerAddr = pBootpReply->bp_siaddr;

        /* Fill in boot file. */

    if (pBootFile != NULL)
        {
        if (pBootpReply->bp_file [0] == EOS)
            {
            pBootFile[0] = EOS;
            }
        else
            strcpy (pBootFile, (char *)pBootpReply->bp_file);
        }
    
        /* Fill in server name. */

    if (pHostName != NULL)
        {
        if (pBootpReply->bp_sname[0] == EOS)
            pHostName[0] = EOS;
        else
            strcpy (pHostName, pBootpReply->bp_sname);
        }

    /* Fill in optional entries requested by user, if present in reply. */

        /* Retrieve subnet mask. */

    if (pBootpParams->netmask != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_SUBNET_MASK, &length);
        if (cp != NULL)
            bcopy ( (char *) cp, (char *)pBootpParams->netmask, length);
        else
            bzero ( (char *)pBootpParams->netmask, sizeof (struct in_addr));
        }

        /* Retrieve time offset. */

    if (pBootpParams->timeOffset != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_TIME_OFFSET, &length);
        if (cp != NULL)
            *pBootpParams->timeOffset = ntohs (*(unsigned short *)cp);
        else
            *pBootpParams->timeOffset = 0;
        }

        /* Retrieve IP addresses of IP routers, up to number requested. */

    if (pBootpParams->routers != NULL && 
        pBootpParams->routers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_GATEWAY, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->routers->num < number) ?
                     pBootpParams->routers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp, 
                       (char *)&pBootpParams->routers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->routers->num = limit;
        }

        /* Retrieve IP addresses of time servers, up to number requested. */

    if (pBootpParams->timeServers != NULL &&
        pBootpParams->timeServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_TIME_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->timeServers->num < number) ?
                     pBootpParams->timeServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->timeServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->timeServers->num = limit;
        }

        /* Retrieve IP addresses of name servers, up to number requested. */

    if (pBootpParams->nameServers != NULL &&
        pBootpParams->nameServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_NAME_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->nameServers->num < number) ?
                     pBootpParams->nameServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->nameServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->nameServers->num = limit;
        }

        /* Retrieve IP addresses of DNS servers, up to number requested. */

    if (pBootpParams->dnsServers != NULL && 
        pBootpParams->dnsServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_DNS_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->dnsServers->num < number) ?
                     pBootpParams->dnsServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->dnsServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->dnsServers->num = limit;
        }

        /* Retrieve IP addresses of log servers, up to number requested. */

    if (pBootpParams->logServers != NULL && 
        pBootpParams->logServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_LOG_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->logServers->num < number) ?
                     pBootpParams->logServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->logServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->logServers->num = limit;
        }

        /* Retrieve IP addresses of cookie servers, up to number requested. */

    if (pBootpParams->cookieServers != NULL && 
        pBootpParams->cookieServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_COOKIE_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->cookieServers->num < number) ?
                     pBootpParams->cookieServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->cookieServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->cookieServers->num = limit;
        }

        /* Retrieve IP addresses of LPR servers, up to number requested. */

    if (pBootpParams->lprServers != NULL && 
        pBootpParams->lprServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_LPR_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->lprServers->num < number) ?
                     pBootpParams->lprServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->lprServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->lprServers->num = limit;
        }

        /* Retrieve IP addresses of Impress servers, up to number requested. */

    if (pBootpParams->impressServers != NULL && 
        pBootpParams->impressServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_IMPRESS_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->impressServers->num < number) ?
                     pBootpParams->impressServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->impressServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->impressServers->num = limit;
        }

        /* Retrieve IP addresses of RLP servers, up to number requested. */

    if (pBootpParams->rlpServers != NULL && 
        pBootpParams->rlpServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_RLP_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->rlpServers->num < number) ?
                     pBootpParams->rlpServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->rlpServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->rlpServers->num = limit;
        }

        /* Retrieve hostname of client. */

    if (pBootpParams->clientName != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_HOSTNAME, &length);
        if (cp != NULL)
            {
            bcopy ( (char *)cp, pBootpParams->clientName, length);
            pBootpParams->clientName [length] = EOS;
            }
        else
            pBootpParams->clientName[0] = EOS;
        }

        /* Retrieve size of boot file. */

    if (pBootpParams->filesize != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_BOOTSIZE, &length);
        if (cp != NULL)
            *pBootpParams->filesize = ntohs (*(unsigned short *)cp);
        else
            *pBootpParams->filesize = 0;
        }

        /* Retrieve name of dump file. */

    if (pBootpParams->dumpfile != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_MERIT_DUMP, &length);
        if (cp != NULL)
            {
            bcopy ( (char *)cp, pBootpParams->dumpfile, length);
            pBootpParams->dumpfile [length] = EOS;
            }
        else
            pBootpParams->dumpfile[0] = EOS;
        }

        /* Retrieve name of DNS domain. */

    if (pBootpParams->domainName != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_DNS_DOMAIN, &length);
        if (cp != NULL)
            {
            bcopy ( (char *)cp, pBootpParams->domainName, length);
            pBootpParams->domainName [length] = EOS;
            }
        else
            pBootpParams->domainName[0] = EOS;
        }

        /* Retrieve IP address of swap server. */

    if (pBootpParams->swapServer != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_SWAP_SERVER, &length);
        if (cp != NULL)
            bcopy ( (char *)cp, (char *)pBootpParams->swapServer, length);
        else
            bzero ( (char *)pBootpParams->swapServer, sizeof (struct in_addr));
        }

        /* Retrieve pathname of root disk. */

    if (pBootpParams->rootPath != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_ROOT_PATH, &length);
        if (cp != NULL)
            {
            bcopy ( (char *)cp, pBootpParams->rootPath, length);
            pBootpParams->rootPath [length] = EOS;
            }
        else
            pBootpParams->rootPath[0] = EOS;
        }

        /* Retrieve pathname of extended options file. */

    if (pBootpParams->extoptPath != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_EXTENSIONS_PATH, &length);
        if (cp != NULL)
            {
            bcopy ( (char *)cp, pBootpParams->extoptPath, length);
            pBootpParams->extoptPath [length] = EOS;
            }
        else
            pBootpParams->extoptPath[0] = EOS;
        }

        /* Retrieve IP forwarding option. */

    if (pBootpParams->ipForward != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_IP_FORWARD, &length);
        if (cp != NULL)
            *pBootpParams->ipForward = *cp;
        else
            *pBootpParams->ipForward = 0;
        }

        /* Retrieve non-local source routing option. */

    if (pBootpParams->nonlocalSourceRoute != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_NONLOCAL_SRCROUTE, 
                           &length);
        if (cp != NULL)
            *pBootpParams->nonlocalSourceRoute = *cp;
        else
            *pBootpParams->nonlocalSourceRoute = 0;
        }

        /* Retrieve IP addresses and masks for policy filter option. */

    if (pBootpParams->policyFilter != NULL && 
        pBootpParams->policyFilter->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_POLICY_FILTER, &length);
        if (cp != NULL)
            {
            /* Find number of pairs to retrieve. */

            number = length / (2 * sizeof (struct in_addr));
            limit = (pBootpParams->policyFilter->num < number) ?
                     pBootpParams->policyFilter->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->policyFilter->addrlist[2 * loop],
                       2 * sizeof (struct in_addr));
                cp += 2 * sizeof (struct in_addr);
                }
            }
        pBootpParams->policyFilter->num = limit;
        }

        /* Retrieve size of maximum IP datagram. */

    if (pBootpParams->maxDgramSize != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_MAX_DGRAM_SIZE, &length);
        if (cp != NULL)
            *pBootpParams->maxDgramSize = ntohs (*(unsigned short *)cp);
        else
            *pBootpParams->maxDgramSize = 0;
        }

        /* Retrieve default IP time-to-live value. */

    if (pBootpParams->ipTTL != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_DEFAULT_IP_TTL, &length);
        if (cp != NULL)
            *pBootpParams->ipTTL = *cp;
        else
            *pBootpParams->ipTTL = 0;
        }

        /* Retrieve value for path MTU aging timeout. */

    if (pBootpParams->mtuTimeout != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_MTU_AGING_TIMEOUT, 
                           &length);
        if (cp != NULL)
            *pBootpParams->mtuTimeout = ntohs (*(unsigned long *)cp);
        else
            *pBootpParams->mtuTimeout = 0;
        }

        /* Retrieve table of MTU sizes. */

    if (pBootpParams->mtuTable != NULL && 
        pBootpParams->mtuTable->shortlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_MTU_PLATEAU_TABLE, 
                           &length);
        if (cp != NULL)
            {
            number = length / sizeof (unsigned short);
            limit = (pBootpParams->mtuTable->num < number) ?
                     pBootpParams->mtuTable->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                pBootpParams->mtuTable->shortlist [loop] = 
                    ntohs (*(unsigned short *)cp);
                cp += sizeof (unsigned short);
                }
            }
        pBootpParams->mtuTable->num = limit;
        }

        /* Retrieve interface MTU. */

    if (pBootpParams->intfaceMTU != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_IF_MTU, &length);
        if (cp != NULL)
            *pBootpParams->intfaceMTU = ntohs (*(unsigned short *)cp);
        else
            *pBootpParams->intfaceMTU = 0;
        }

        /* Retrieve all subnets local option. */

    if (pBootpParams->allSubnetsLocal != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_ALL_SUBNET_LOCAL, 
                           &length);
        if (cp != NULL)
            *pBootpParams->allSubnetsLocal = *cp;
        else
            *pBootpParams->allSubnetsLocal = 0;
        }

        /* Retrieve broadcast IP address. */

    if (pBootpParams->broadcastAddr != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_BRDCAST_ADDR, &length);
        if (cp != NULL)
            bcopy ( (char *) cp, (char *)pBootpParams->broadcastAddr, length);
        else
            bzero ( (char *)pBootpParams->broadcastAddr, 
                    sizeof (struct in_addr));
        }

        /* Retrieve mask discovery option. */

    if (pBootpParams->maskDiscover != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_MASK_DISCOVER, &length);
        if (cp != NULL)
            *pBootpParams->maskDiscover = *cp;
        else
            *pBootpParams->maskDiscover = 0;
        }

        /* Retrieve mask supplier option. */

    if (pBootpParams->maskSupplier != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_MASK_SUPPLIER, &length);
        if (cp != NULL)
            *pBootpParams->maskSupplier = *cp;
        else
            *pBootpParams->maskSupplier = 0;
        }

        /* Retrieve router discovery option. */

    if (pBootpParams->routerDiscover != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_ROUTER_DISCOVER, &length);
        if (cp != NULL)
            *pBootpParams->routerDiscover = *cp;
        else
            *pBootpParams->routerDiscover = 0;
        }

        /* Retrieve IP address for router solicitation. */

    if (pBootpParams->routerDiscAddr != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_ROUTER_SOLICIT, &length);
        if (cp != NULL)
            bcopy ( (char *) cp, (char *)pBootpParams->routerDiscAddr, length);
        else
            bzero ( (char *)pBootpParams->routerDiscAddr, 
                   sizeof (struct in_addr));
        }

        /* Retrieve static routing table. */

    if (pBootpParams->staticRoutes != NULL && 
        pBootpParams->staticRoutes->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_STATIC_ROUTE, &length);
        if (cp != NULL)
            {
            /* Find number of pairs to retrieve. */

            number = length / (2 * sizeof (struct in_addr));
            limit = (pBootpParams->staticRoutes->num < number) ?
                     pBootpParams->staticRoutes->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->staticRoutes->addrlist[2 * loop],
                       2 * sizeof (struct in_addr));
                cp += 2 * sizeof (struct in_addr);
                }
            }
        pBootpParams->staticRoutes->num = limit;
        }

        /* Retrieve ARP trailer encapsulation option. */

    if (pBootpParams->arpTrailers != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_TRAILER, &length);
        if (cp != NULL)
            *pBootpParams->arpTrailers = *cp;
        else
            *pBootpParams->arpTrailers = 0;
        }

        /* Retrieve value for ARP cache timeout. */

    if (pBootpParams->arpTimeout != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_ARP_CACHE_TIMEOUT, 
                           &length);
        if (cp != NULL)
            *pBootpParams->arpTimeout = ntohs (*(unsigned long *)cp);
        else
            *pBootpParams->arpTimeout = 0;
        }

        /* Retrieve Ethernet encapsulation option. */

    if (pBootpParams->etherPacketType != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_ETHER_ENCAP, &length);
        if (cp != NULL)
            *pBootpParams->etherPacketType = *cp;
        else
            *pBootpParams->etherPacketType = 0;
        }

        /* Retrieve default TCP time-to-live value. */

    if (pBootpParams->tcpTTL != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_DEFAULT_TCP_TTL, &length);
        if (cp != NULL)
            *pBootpParams->tcpTTL = *cp;
        else
            *pBootpParams->tcpTTL = 0;
        }

        /* Retrieve value for TCP keepalive interval. */

    if (pBootpParams->tcpInterval != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_KEEPALIVE_INTER, &length);
        if (cp != NULL)
            *pBootpParams->tcpInterval = ntohs (*(unsigned long *)cp);
        else
            *pBootpParams->tcpInterval = 0;
        }

        /* Retrieve value for TCP keepalive garbage option. */

    if (pBootpParams->tcpGarbage != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_KEEPALIVE_GARBA, &length);
        if (cp != NULL)
            *pBootpParams->tcpGarbage = *cp;
        else
            *pBootpParams->tcpGarbage = 0;
        }

        /* Retrieve NIS domain name. */

    if (pBootpParams->nisDomain != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_NIS_DOMAIN, &length);
        if (cp != NULL)
            {
            bcopy ( (char *)cp, pBootpParams->nisDomain, length);
            pBootpParams->nisDomain [length] = EOS;
            }
        else
            pBootpParams->nisDomain[0] = EOS;
        }

        /* Retrieve IP addresses of NIS servers, up to number requested. */

    if (pBootpParams->nisServers != NULL && 
        pBootpParams->nisServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_NIS_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->nisServers->num < number) ?
                     pBootpParams->nisServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->nisServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->nisServers->num = limit;
        }

        /* Retrieve IP addresses of NTP servers, up to number requested. */

    if (pBootpParams->ntpServers != NULL && 
        pBootpParams->ntpServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_NTP_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->ntpServers->num < number) ?
                     pBootpParams->ntpServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->ntpServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->ntpServers->num = limit;
        }

        /* Retrieve vendor specific information. */

    if (pBootpParams->vendString != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_VENDOR_SPEC, &length);
        if (cp != NULL)
            {
            bcopy ( (char *)cp, pBootpParams->vendString, length);
            pBootpParams->vendString [length] = EOS;
            }
        else
            pBootpParams->vendString[0] = EOS;
        }

        /* Retrieve IP addresses of NetBIOS name servers. */

    if (pBootpParams->nbnServers != NULL && 
        pBootpParams->nbnServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_NBN_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->nbnServers->num < number) ?
                     pBootpParams->nbnServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->nbnServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->nbnServers->num = limit;
        }

        /* Retrieve IP addresses of NetBIOS datagram distribution servers. */

    if (pBootpParams->nbddServers != NULL && 
        pBootpParams->nbddServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_NBDD_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->nbddServers->num < number) ?
                     pBootpParams->nbddServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->nbddServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->nbddServers->num = limit;
        }

        /* Retrieve value for NetBIOS Node Type option. */

    if (pBootpParams->nbNodeType != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_NB_NODETYPE, &length);
        if (cp != NULL)
            *pBootpParams->nbNodeType = *cp;
        else
            *pBootpParams->nbNodeType = 0;
        }

        /* Retrieve NetBIOS scope. */

    if (pBootpParams->nbScope != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_NB_SCOPE, &length);
        if (cp != NULL)
            {
            bcopy ( (char *)cp, pBootpParams->nbScope, length);
            pBootpParams->nbScope [length] = EOS;
            }
        else
            pBootpParams->nbScope[0] = EOS;
        }

        /* Retrieve IP addresses of X Window font servers. */

    if (pBootpParams->xFontServers != NULL && 
        pBootpParams->xFontServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_XFONT_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->xFontServers->num < number) ?
                     pBootpParams->xFontServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->xFontServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->xFontServers->num = limit;
        }

        /* Retrieve IP addresses of X Window Display Manager systems. */

    if (pBootpParams->xDisplayManagers != NULL && 
        pBootpParams->xDisplayManagers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_XDISPLAY_MANAGER, 
                           &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->xDisplayManagers->num < number) ?
                     pBootpParams->xDisplayManagers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->xDisplayManagers->addrlist[loop],
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->xDisplayManagers->num = limit;
        }

        /* Retrieve NIS+ domain name. */

    if (pBootpParams->nispDomain != NULL)
        {
        length = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_NISP_DOMAIN, &length);
        if (cp != NULL)
            {
            bcopy ( (char *)cp, pBootpParams->nispDomain, length);
            pBootpParams->nispDomain [length] = EOS;
            }
        else
            pBootpParams->nispDomain[0] = EOS;
        }

        /* Retrieve IP addresses of NIS+ servers. */

    if (pBootpParams->nispServers != NULL && 
        pBootpParams->nispServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_NISP_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->nispServers->num < number) ?
                     pBootpParams->nispServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->nispServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->nispServers->num = limit;
        }

        /* Retrieve IP addresses of Mobile IP Home Agents. */

    if (pBootpParams->ipAgents != NULL && 
        pBootpParams->ipAgents->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_MOBILEIP_HA, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->ipAgents->num < number) ?
                     pBootpParams->ipAgents->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->ipAgents->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->ipAgents->num = limit;
        }

        /* Retrieve IP addresses of SMTP servers. */

    if (pBootpParams->smtpServers != NULL && 
        pBootpParams->smtpServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_SMTP_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->smtpServers->num < number) ?
                     pBootpParams->smtpServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->smtpServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->smtpServers->num = limit;
        }

        /* Retrieve IP addresses of POP3 servers. */

    if (pBootpParams->pop3Servers != NULL && 
        pBootpParams->pop3Servers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_POP3_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->pop3Servers->num < number) ?
                     pBootpParams->pop3Servers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->pop3Servers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->pop3Servers->num = limit;
        }

        /* Retrieve IP addresses of NNTP servers. */

    if (pBootpParams->nntpServers != NULL && 
        pBootpParams->nntpServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_NNTP_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->nntpServers->num < number) ?
                     pBootpParams->nntpServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->nntpServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->nntpServers->num = limit;
        }

        /* Retrieve IP addresses of World Wide Web servers. */

    if (pBootpParams->wwwServers != NULL && 
        pBootpParams->wwwServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_WWW_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->wwwServers->num < number) ?
                     pBootpParams->wwwServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->wwwServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->wwwServers->num = limit;
        }

        /* Retrieve IP addresses of finger servers. */

    if (pBootpParams->fingerServers != NULL && 
        pBootpParams->fingerServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_FINGER_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->fingerServers->num < number) ?
                     pBootpParams->fingerServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->fingerServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->fingerServers->num = limit;
        }

        /* Retrieve IP addresses of Internet Relay Chat servers. */

    if (pBootpParams->ircServers != NULL && 
        pBootpParams->ircServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_IRC_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->ircServers->num < number) ?
                     pBootpParams->ircServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->ircServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->ircServers->num = limit;
        }

        /* Retrieve IP addresses of StreetTalk servers. */

    if (pBootpParams->stServers != NULL && 
        pBootpParams->stServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_ST_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->stServers->num < number) ?
                     pBootpParams->stServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->stServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->stServers->num = limit;
        }

        /* Retrieve IP addresses of STDA servers. */

    if (pBootpParams->stdaServers != NULL && 
        pBootpParams->stdaServers->addrlist != NULL)
        {
        length = 0;
        limit = 0;

        cp = bootpTagFind (pBootpReply->bp_vend, TAG_STDA_SERVER, &length);
        if (cp != NULL)
            {
            number = length / sizeof (struct in_addr);
            limit = (pBootpParams->stdaServers->num < number) ?
                     pBootpParams->stdaServers->num : number;
            for (loop = 0; loop < limit; loop++)
                {
                bcopy ( (char *)cp,
                       (char *)&pBootpParams->stdaServers->addrlist[loop], 
                       sizeof (struct in_addr));
                cp += sizeof (struct in_addr);
                }
            }
        pBootpParams->stdaServers->num = limit;
        }
    return;
    }

/******************************************************************************
*
* bootpMsgGet - send a BOOTP request message and retrieve reply
*
* This routine sends a BOOTP request using the network interface
* specified by <pIf> and waits for any reply. The <pIpDest> argument
* specifies the destination IP address.  It must be equal to either
* the broadcast address (255.255.255.255) or the IP address of a specific
* BOOTP server which is directly reachable using the given network interface.
* The given interface must support broadcasting in the first case.
*
* The <srcPort> and <dstPort> arguments support sending and receiving 
* BOOTP messages with arbitrary UDP ports. To receive replies, any BOOTP
* server must send those responses to the source port from the request.
* To comply with the RFC 1542 clarification, the request message must be
* sent to the reserved BOOTP server port (67) using the reserved BOOTP
* client port (68).
*
* Except for the UDP port numbers, this routine only sets the `bp_xid' and
* `bp_secs' fields in the outgoing BOOTP message. All other fields in that
* message use the values from the <pBootpMsg> argument, which later holds
* the contents of any BOOTP reply received.
*
* The <maxSends> parameter specifies the total number of requests to transmit
* if no reply is received. The retransmission interval starts at 4 seconds
* and doubles with each attempt up to a maximum of 64 seconds. Any subsequent
* retransmissions will occur at that maximum interval. To reduce the chances
* of network flooding, the timeout interval before each retransmission includes
* a randomized delay of plus or minus one second from the base value. After
* the final transmission, this routine will wait for the current interval to
* expire before returning a timeout error.
*
* NOTE: The target must be able to respond to an ARP request for any IP
*       address specified in the request template's `bp_ciaddr' field.
*
* RETURNS: OK, or ERROR.
*
* ERRNO
*  S_bootpLib_INVALID_ARGUMENT
*  S_bootpLib_NO_BROADCASTS
*  S_bootpLib_TIME_OUT
*/

STATUS bootpMsgGet
    (
    struct ifnet * 	pIf,          /* network device for message exchange */
    struct in_addr *	pIpDest,      /* destination IP address for request */
    USHORT		srcPort,      /* UDP source port for request */
    USHORT 		dstPort,      /* UDP destination port for request */
    BOOTP_MSG *		pBootpMsg,    /* request template and reply storage */
    u_int		maxSends      /* maximum number of transmit attempts */
    )
    {
    int result;
    int maxSize;

    char bpfDevName [_BOOTP_MAX_DEVNAME]; 	/* "/bpf/bootpc0" */
    int bpfDev;
    struct ifreq ifr;

    BOOL 	broadcastFlag;
    USHORT 	xid1; 	/* Hardware address portion of transaction ID. */
    USHORT 	xid2; 	/* Additional random portion of transaction ID. */

    int 	baseDelay; 	/* exponential backoff time (not randomized) */
    int 	retransmitSecs;	/* actual (randomized) timeout interval */
    struct in_addr	ipSrc;		/* source 			*/

    fd_set 	readFds;
    struct bpf_hdr *    pMsgHdr;
    char * 	pMsgData;
    struct ip * pIpHead;
    struct udphdr * pUdpHead;
    struct mbuf * pMbuf;
    struct sockaddr_in dest;

    int         totlen;         /* Amount of data in BPF buffer. */
    struct timeval timeout;
    int numSends = 0;
    int totalDelay = 0;
    int level;

    END_OBJ * 	pEnd;

    if (!bootpInitialized)
        {
        errno = S_bootpLib_NOT_INITIALIZED;
        return (ERROR);
        }

    if (pIf == NULL)
        {
        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    if (pBootpMsg == NULL)
        {
        errno = S_bootpLib_INVALID_ARGUMENT;
        return (ERROR);
        }

    pEnd = endFindByName (pIf->if_name, pIf->if_unit);
    if (pEnd == NULL)
        {
        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    /* Verify interface data sizes are appropriate for message. */

    if (pIf->if_mtu == 0)
        {
        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    maxSize = pIf->if_mtu;
    if (maxSize < BOOTPLEN + UDPHL + IPHL)
        {
        /*
         * Devices must accept messages equal to an IP datagram
         * of 328 bytes, which corresponds to the fixed-size BOOTP
         * message with the minimum headers.
         */

        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    maxSize = pIf->if_hdrlen + BOOTPLEN + UDPHL + IPHL;
    if (maxSize > (bootpBufSize - sizeof (struct bpf_hdr)) )
        {
        /* Link level header exceeds maximum supported value. */

        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    /* Get a BPF file descriptor to read/write BOOTP messages. */

    sprintf (bpfDevName, "/bpf/bootpc0");

    bpfDev = open (bpfDevName, 0, 0);
    if (bpfDev < 0)
        {
        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    /* Enable immediate mode for reading messages. */

    result = 1;
    result = ioctl (bpfDev, BIOCIMMEDIATE, (int)&result);
    if (result != 0)
        {
        close (bpfDev);
        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    /*
     * Add a filter for incoming BOOTP packets and assign the selected
     * interface to the BPF device.
     */

    bootpfilter [12].k = srcPort;
    result = ioctl (bpfDev, BIOCSETF, (int)&bootpread);
    if (result != 0)
        {
        close (bpfDev);
        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    bzero ( (char *)&ifr, sizeof (struct ifreq));
    sprintf (ifr.ifr_name, "%s%d", pIf->if_name, pIf->if_unit);

    result = ioctl (bpfDev, BIOCSETIF, (int)&ifr);
    if (result != 0)
        {
        close (bpfDev);
        errno = S_bootpLib_BAD_DEVICE;
        return (ERROR);
        }

    /*
     * Select the transmission method and set the IP source and destination
     * addresses. Verify the interface is capable of broadcasts if needed.
     */

    if (pIpDest->s_addr == INADDR_BROADCAST)
        {
        if ( (pIf->if_flags & IFF_BROADCAST) == 0)
            {
            close (bpfDev);
            errno = S_bootpLib_NO_BROADCASTS;
            return (ERROR);
            }
        broadcastFlag = TRUE;
        }
    else
        {
        broadcastFlag = FALSE;
        }

    ipSrc.s_addr = pBootpMsg->bp_ciaddr.s_addr;

    /* Initialize timeout values. */

    baseDelay = INIT_BOOTP_DELAY;
    retransmitSecs = (baseDelay - 1) + (rand () % 3);

    /* Copy the provided BOOTP (request) message and set the transaction ID. */

    bzero ( (char *)&bootpMsg, sizeof (bootpMsg));

    bootpMsg.bp 	= *pBootpMsg;

    xid1= checksum ( (u_short *)bootpMsg.bp.bp_chaddr, bootpMsg.bp.bp_hlen);
    xid2 = rand() & 0xffff;
    bootpMsg.bp.bp_xid = htonl ((xid1 << 16) + xid2);

    /* Fill in the UDP header. */

    bootpMsg.uh.uh_sport = htons ((u_short) srcPort);
    bootpMsg.uh.uh_dport = htons ((u_short) dstPort);
    bootpMsg.uh.uh_ulen = htons (sizeof (BOOTP_MSG) + UDPHL);
    bootpMsg.uh.uh_sum = 0;

    /* Fill in the IP header. */

    bootpMsg.ih.ip_v = IPVERSION;
    bootpMsg.ih.ip_hl = IPHL >> 2;
    bootpMsg.ih.ip_tos = 0;
    bootpMsg.ih.ip_len = htons (sizeof (bootpMsg));
    bootpMsg.ih.ip_id = htons ((u_short) (~ (bootpMsg.bp.bp_xid + xid1)));
    bootpMsg.ih.ip_off = 0;
    bootpMsg.ih.ip_ttl = MAXTTL;
    bootpMsg.ih.ip_p = IPPROTO_UDP;

    bootpMsg.ih.ip_src.s_addr = ipSrc.s_addr;
    bootpMsg.ih.ip_dst.s_addr = pIpDest->s_addr;
    bootpMsg.ih.ip_sum = 0;
    bootpMsg.ih.ip_sum = checksum ( (u_short *) &bootpMsg.ih,
                                   bootpMsg.ih.ip_hl << 2);

    /* Fill in the destination address structure and begin transmitting. */

    bzero ( (char *)&dest, sizeof (struct sockaddr_in));
    dest.sin_len = sizeof (struct sockaddr_in);
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = pIpDest->s_addr;

    while (numSends < maxSends)
        {
        /* Update BPF filter to check for current transaction ID. */

        bootpfilter [20].k = ntohl (bootpMsg.bp.bp_xid);
        ioctl (bpfDev, BIOCSETF, (u_int)&bootpread);

        pMbuf = bcopy_to_mbufs ( (char *)&bootpMsg.ih, sizeof (bootpMsg),
                                0, pIf, NONE);
        if (pMbuf == NULL)
            {
            result = ERROR;
            break;
            }

        if (broadcastFlag)
            pMbuf->mBlkHdr.mFlags |= M_BCAST;

        FD_ZERO (&readFds);

        level = splnet ();
        result = pIf->if_output (pIf, pMbuf, (struct sockaddr *)&dest, NULL);
        splx (level);

        if (result == ERROR)
            {
            break;
            }

        /* wait for a reply */

        FD_SET (bpfDev, &readFds);

        timeout.tv_sec = retransmitSecs;
        timeout.tv_usec = 0;

        result = select (bpfDev + 1, &readFds, NULL, NULL, &timeout);
        if (result == ERROR) 
            break;

        if (result)    /* got reply !! */
            {
            pMsgData = pMsgBuffer;
            totlen = read (bpfDev, pMsgData, bootpBufSize);

            pMsgHdr = (struct bpf_hdr *)pMsgData;

            /* Set the pointer to skip the BPF and link level headers. */

            pMsgData = pMsgData + pMsgHdr->bh_hdrlen + pMsgHdr->bh_linklen;

            /* Skip the IP and UDP headers and copy BOOTP message. */

            pIpHead = (struct ip *)pMsgData;
            pUdpHead = (struct udphdr *) (pMsgData + (pIpHead->ip_hl << 2));

            pMsgData = ( (u_char *)pUdpHead + sizeof (struct udphdr));

            bcopy (pMsgData, (char *)pBootpMsg, BOOTPLEN);

            result = OK;
 
            break;
            }
        else    /* Timeout - retransmit message if permitted. */
            {
            numSends++;
            totalDelay += retransmitSecs;

            if (baseDelay < MAX_BOOTP_DELAY)
                baseDelay *= 2;

            retransmitSecs = (baseDelay - 1) + (rand () % 3);

            /*
             * Set new transaction ID and update 'secs' field. No other
             * work needed since the message does not include a UDP checksum.
             */

            xid2 = rand() & 0xffff;
            bootpMsg.bp.bp_xid = htonl ((xid1 << 16) + xid2);

            bootpMsg.bp.bp_secs = htons (totalDelay);

            /*
             * Set result to indicate failure
             * (in case retransmit limit reached).
             */

            result = ERROR;
            }
        }

    close (bpfDev);

    return (result);
    }

/*******************************************************************************
*
* bootpTagFind - find data for a BOOTP options tag
*
* This routine finds the data associated with tag <tag> in the 
* vendor-specific member of a BOOTP message.  The <tag> parameter must 
* be a valid 1533 vendor-tag value.  Only <tag> values that have data 
* associated with them are considered valid (for example, TAG_END 
* and TAG_PAD are not valid <tag> values because they have no data).
* The <pVend> parameter is a pointer to the beginning of the 
* vendor-specific member in the BOOTP message.  If <tag> is found 
* in <pVend>, the length of the associated data gets placed in <pSize> 
* and a pointer to the data is returned.
*
* INTERNAL
* The vendor information field is divided into extendable tagged subfields.
* Tags that have no data, consist of a single tag octet and are one octet
* in length.  All other tags have a one tag octet, a length octet and length
* octets of data.  For a more complete description of the tags and how they
* are parsed, please refer to RFC 1048 or its successors. All tags defined
* through RFC 1533 are supported.
*
* RETURNS: A pointer to tag data if successful, otherwise NULL.
*
* ERRNO
*  S_bootpLib_INVALID_ARGUMENT
*  S_bootpLib_INVALID_COOKIE
*  S_bootpLib_INVALID_TAG
*  S_bootpLib_PARSE_ERROR
*
* NOMANUAL
*/

LOCAL u_char * bootpTagFind
    (
    u_char *		pVend,		/* vendor specific information  */
    int			tag,		/* tag to be located 		*/
    int *		pSize		/* return size of data		*/
    )
    {
    u_char *		cp;		/* character pointer 		*/
    u_char *		pData;		/* pointer to data		*/
    int 		sizeData;	/* size if data			*/

    if ( (pSize == NULL) || (pVend == NULL)) 	/* validate arguments */
        {
        errno = S_bootpLib_INVALID_ARGUMENT;
        return  (NULL);
        }

    if ( (tag <= TAG_PAD) || (tag >= TAG_END))	/* validate tag */
        {
        errno = S_bootpLib_INVALID_TAG;
        return (NULL);
        }

     /* validate RFC 1048 cookie */

    if (bcmp ( (char *)magicCookie1048, (char *)pVend,
               sizeof (magicCookie1048)) != 0)
        {
        errno = S_bootpLib_INVALID_COOKIE;
        return (NULL);
        }

    pData = pVend + sizeof (magicCookie1048);	/* move past cookie */
    sizeData = SIZE_VEND - sizeof (magicCookie1048);

    /* loop to find tag */

    for (cp = pData; (*cp != (u_char)TAG_END) && (cp < pData + sizeData); cp++)
        {
        if (*cp == (u_char) TAG_PAD)		/* do nothing -  pad */
            continue;

        if ( (cp + 1) >= (pData + sizeData))	/* no for length */
            {
            errno = S_bootpLib_PARSE_ERROR;
            break;
            }

        *pSize = *(cp + 1);

        /* no room for data */

        if ( (cp + 2 + *pSize) > (pData + sizeData))
            {
            errno = S_bootpLib_PARSE_ERROR;
            break;
            }

        if (*cp == (u_char)tag)		/* found desired tag */
            return (cp + 2);

        cp += *pSize + 1;			/* move past the data */
        }

    *pSize = 0;
    return (NULL);
    }

/*******************************************************************************
*
* bootpConvert - convert the hardware type from the MUX
*
* This routine converts the hardware type value which the MUX interface
* provides into the encoding that BOOTP uses. The MUX interface uses the
* RFC 1213 MIB values while BOOTP uses the interface encodings from the ARP
* section of the assigned numbers RFC (RFC 1700).
*
* NOMANUAL
*
* RETURNS: RFC 1700 encoding of type value, or ERROR if none.
*
*/

LOCAL int bootpConvert
    (
    int muxType         /* RFC 1213 interface type value */
    )
    {
    int bootpType = 0;

    switch (muxType)
        {
        default:
            return (ERROR);
            break;

        case M2_ifType_ethernet_csmacd:
            bootpType = ETHER;
            break;

        case M2_ifType_ethernet3Mbit:
            bootpType = EXPETHER;
            break;

        case M2_ifType_proteon10Mbit:     /* fall-through */
        case M2_ifType_proteon80Mbit:
            bootpType = PRONET;
            break;

        case M2_ifType_iso88023_csmacd:    /* fall-through */
        case M2_ifType_iso88024_tokenBus:  /* fall-through */
        case M2_ifType_iso88025_tokenRing: /* fall-through */
        case M2_ifType_iso88026_man:
            bootpType = IEEE802;
            break;

        case M2_ifType_hyperchannel:
            bootpType = HYPERCH;
            break;

        case M2_ifType_starLan:
            bootpType = LANSTAR;
            break;

        case M2_ifType_ultra:
            bootpType = ULTRALINK;
            break;

        case M2_ifType_frameRelay:
            bootpType = FRAMERELAY;
            break;

        case M2_ifType_propPointToPointSerial:    /* fall-through */
        case M2_ifType_ppp:                       /* fall-through */
        case M2_ifType_slip:
            bootpType = SERIAL;
            break;
        }
    return (bootpType);
    }

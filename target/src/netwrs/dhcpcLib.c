/* dhcpcLib.c - Dynamic Host Configuration Protocol (DHCP) run-time client API */

/* Copyright 1984 - 2002 Wind River Systems, Inc.  */
#include "copyright_wrs.h"

/*
modification history
--------------------
02k,25apr02,wap  Correct arguments passed to dhcpcOptionAdd() for
                 little-endian targets (SPR #73769)
02j,23apr02,wap  use dhcpTime() instead of time() (SPR #68900)
02i,08mar02,wap  Allow DHCP to work at boot time when the boot device is not a
                 network interface (SPR #73762)
02h,10dec01,wap  Fix bugs in dhcpcConfigSet() (SPR #72056)
02g,07dec01,vvv  test dhcpcEventAdd return value in dhcpcRelease
02f,03dec01,vvv  fixed interface configuration after dhcpcRelease (SPR #70026)
02e,05nov01,wap  dhcpcLeaseCleanup() fails to remove pLeaseData pointers from
                 global dhcpcLeaseList (SPR #68981)
02d,26oct01,vvv  fixed dhcpcBind doc
02c,15oct01,rae  merge from truestack ver 02f, base 01u (SPRs 28985, 65264,
                 67272, 65380, 65424, 29785)
02b,24may01,mil  Bump up dhcp state task stack size to 5000.
02a,17nov00,spm  added support for BSD Ethernet devices
01z,16nov00,spm  enabled new DHCP lease for runtime device setup (SPR #20438)
01y,16nov00,spm  fixed modification history after tor3_x merge
01x,13nov00,kbw  changed text of dhcpcParamsGet man page (SPR 28985)
01w,23oct00,niq  merged from version 01z of tor3_x branch (base version 01u)
01v,19oct00,dgr  docs: clarified sname/file retrieval with dhcpcParamsGet()
01u,17mar99,spm  fixed dhcpcRelease() and dhcpcVerify() routines (SPR #25482)
01t,05feb99,dgp  document errno values
01s,10dec97,kbw  fixed minor spelling issues in man pages
01r,04dec97,spm  added code review modifications; increased stack size for
                 monitor task to 50% over maximum for ss5 board
01q,27oct97,spm  corrected name field to contain device name only
01p,21oct97,kbw  fixed minor spelling issues in man pages
01o,06oct97,spm  removed reference to deleted endDriver global; replaced with
                 support for dynamic driver type detection
01n,25sep97,gnn  SENS beta feedback fixes
01m,02sep97,spm  removed unused global variable for event hook routine
01l,26aug97,spm  major overhaul: reorganized code and changed user interface
                 to support multiple leases at runtime
01k,12aug97,gnn  changes necessitated by MUX/END update.
01j,06aug97,spm  removed parameters linked list to reduce memory required;
                 corrected minor error in man pages introduced by 01i revision
01i,30jul97,kbw  fixed man page problems found in beta review
01h,15jul97,spm  fixed byte ordering of netmask from DHCP client (SPR #8739)
01g,10jun97,spm  fixed code layout and error in client lease indicator
01f,02jun97,spm  changed DHCP option tags to prevent name conflicts (SPR #8667)
                 and updated man pages
01e,30apr97,spm  changed dhcpcOptionGet() to return length; expanded man pages
01d,15apr97,kbw  fixed format problems in man page text, changed some wording
01c,07apr97,spm  altered to use current value of configAll.h defines, fixed 
                 user requests, inserted code previously in usrNetwork.c,
                 rewrote documentation
01b,29jan97,spm  added END driver support and modified to fit coding standards.
01a,03oct96,spm  created by modifying WIDE Project DHCP Implementation.
*/

/*
DESCRIPTION
This library implements the run-time access to the client side of the 
Dynamic Host Configuration Protocol (DHCP).  DHCP is an extension of BOOTP. 
Like BOOTP, the protocol allows a host to initialize automatically by obtaining
its IP address, boot file name, and boot host's IP address over a network. 
Additionally, DHCP provides a client with the complete set of parameters
defined in the Host Requirements RFCs and allows automatic reuse of network 
addresses by specifying individual leases for each set of configuration 
parameters.  The compatible message format allows DHCP participants to interact 
with BOOTP participants.  The dhcpcLibInit() routine links this library into 
the VxWorks image. This happens automatically if INCLUDE_DHCPC is defined at 
the time the image is built.

CONFIGURATION INTERFACE
When used during run time, the DHCP client library establishes and maintains
one or more DHCP leases.  Each lease provides access to a set of configuration 
parameters.  If requested, the parameters retrieved will be used to reconfigure 
the associated network interface, but may also be handled separately through 
an event hook.  The dhcpcEventHookAdd() routine specifies a function which is 
invoked whenever the lease status changes.  The dhcpcEventHookDelete() routine 
will disable that notification.  The automatic reconfiguration must be limited 
to one lease for a particular network interface.  Otherwise, multiple leases 
would attempt to reconfigure the same device, with unpredictable results.

HIGH-LEVEL INTERFACE
To access the DHCP client during run time, an application must first call 
the dhcpcInit() routine with a pointer to the network interface to be used 
for communication with a DHCP server.  Each call to the initialization 
routine returns a unique identifier to be used in subsequent calls to the 
DHCP client routines.  Next, the application must specify a client identifier 
for the lease using the dhcpcOptionSet() call.  Typically, the link-level 
hardware address is used for this purpose.  Additional calls to the option set 
routine may be used to request specific DHCP options.  After all calls to that 
routine are completed, a call to dhcpcBind() will retrieve a set of 
configuration parameters according to the client-server interaction detailed 
in RFC 1541.

Each sequence of the three function calls described above, if successful,
will retrieve a set of configuration parameters from a DHCP server.  The
dhcpcServerGet() routine retrieves the address of the server that provided a 
particular lease.  The dhcpcTimerGet() routine will retrieve the current values 
for both lease timers. 

Alternatively, the dhcpcParamsGet() and dhcpcOptionGet() routines will access 
any options provided by a DHCP server.  In addition to the lease identifier
obtained from the initialization routine, the dhcpcParamsGet() routine accepts 
a parameter descriptor structure that selects any combination of the options 
described in RFC 1533 for retrieval.  Similarly, the dhcpcOptionGet() routine 
retrieves the values associated with a single option.

LOW-LEVEL INTERFACE
This library also contains several routines which explicitly generate DHCP 
messages.  The dhcpcVerify() routine causes the client to renew a particular
lease, regardless of the time remaining.  The dhcpcRelease() routine 
relinquishes the specified lease.  The associated parameters are no longer 
valid.  If those parameters were used by the underlying network device, the 
routine also shuts off all network processing for that interface.  Finally,
the dhcpcShutdown() routine will release all active leases and disable all
the DHCP client library routines.

OPTIONAL INTERFACE
The dhcpcCacheHookAdd() routine registers a function that the client will
use to store and retrieve lease data.  The client can then re-use this 
information if it is rebooted.  The dhcpcCacheHookDelete() routine prevents 
the re-use of lease data.  Initially, a function to access permanent storage 
is not provided.

INTERNAL
The diagram below defines the structure chart of dhcpcLib.

     |            |              |
     v            v              v

 dhcpcLibInit  dhcpcSetup    dhcpcConfigSet -------\

     |                       /    |    \            \

     v                      |     |     |            |

  dhcpcMon                  v     |     v            |
(spawned task)
                        dhcpcInit | dhcpcBind        |

                                  |                  |

                                  |                  |

                                  |                  |

                                  v                  v

                            dhcpcOptionSet     dhcpcParamsGet

This library provides a wrapper for the WIDE project DHCP code found in the
directory /vobs/wpwr/target/src/dhcp, which contains the state machine and
other supporting functions.  The monitor task redirects incoming messages,
timeout events, and user requests to the appropriate state in the state
machine.  The input hook used to retrieve incoming messages and the
monitor task routines are both in the dhcpcCommonLib module in this directory.

The current RFC specification does not allow users to obtain parameters
with DHCP without also obtaining a new IP address.  This limitation requires
network shutdown if the lease used by an interface is not maintained.  It also 
artificially limits the use of this library to routines involving an active 
lease.  The draft RFC for the successor to RFC 1541 adds a new message which 
avoids this problem.  Once published, DHCP can safely be used within an 
application without risking network shutdown for any interface which was 
statically configured during system boot.  The code which shuts down the 
network should then be limited to execute conditionally on interfaces which
have no backup source of addressing information.

INCLUDE FILES: dhcpcLib.h

SEE ALSO: RFC 1541, RFC 1533
*/

/* includes */

#include "dhcp/copyright_dhcp.h"
#include "vxWorks.h"
#include "bootLib.h"
#include "sockLib.h"
#include "ioLib.h" 	/* ioctl() declaration */
#include "vxLib.h" 	/* checksum() declaration */
#include "sysLib.h"
#include "wdLib.h"
#include "semLib.h"
#include "inetLib.h"
#include "taskLib.h"

#ifdef DHCPC_DEBUG
#include "logLib.h"
#endif

#include "muxLib.h"

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"
#include "errno.h"
#include "signal.h"
#include "fcntl.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "net/if.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/if_ether.h"
#include "netinet/ip.h"
#include "netinet/udp.h"
#include "arpa/inet.h"
#include "time.h"

#include "dhcpcLib.h"
#include "dhcp/dhcpcStateLib.h"
#include "dhcp/dhcpcCommonLib.h"
#include "dhcp/dhcpcInternal.h"

#include "end.h"
#include "ipProto.h"

#include "bpfDrv.h"

/* defines */

#define _DHCP_MAX_OPTLEN 	255 	/* Max. number of bytes in an option */
#define _DHCPC_MAX_DEVNAME 	21 	/* "/bpf/dhcpc" + max unit number */

/* externals */

IMPORT RING_ID 	dhcpcEventRing;	/* Ring buffer of DHCP events */
IMPORT SEM_ID 	dhcpcEventSem; 	/* DHCP event notification */
IMPORT int dhcpcReadTaskId;	/* Identifier for data retrieval task */
IMPORT int _dhcpcReadTaskPriority;      /* Priority level of data retriever */
IMPORT int _dhcpcReadTaskOptions;       /* Option settings of data retriever */
IMPORT int _dhcpcReadTaskStackSize;     /* Stack size of data retriever */

IMPORT struct bpf_insn dhcpfilter[];    /* Needed to update client port. */
IMPORT struct bpf_program dhcpread;     /* Installs filter for DHCP messages */

IMPORT LEASE_DATA ** 	dhcpcLeaseList; 	/* List of available cookies. */
IMPORT MESSAGE_DATA * 	dhcpcMessageList; 	/* Available DHCP messages. */

IMPORT int 	dhcpcMaxLeases; 	/* configAll.h #define value */
IMPORT int 	dhcpcMinLease; 		/* configAll.h #define value */
IMPORT int 	dhcpcBufSize; 	/* sets size of transmit and receive buffers */

IMPORT int dhcpcDataSock; 			/* Receives DHCP messages */

/* globals */

void * 	pDhcpcBootCookie = NULL; 	/* Access to boot-time lease. */
BOOL dhcpcInitialized = FALSE;  /* Client initialized? */
SEM_ID 	dhcpcMutexSem; 		/* Protects the DHCP status indicator */
DHCP_LEASE_DATA 	dhcpcBootLease; 	/* Boot-time lease settings. */

int _dhcpcStateTaskPriority  = 56;      /* Priority level of lease monitor */
int _dhcpcStateTaskOptions   = 0;       /* Option settings of lease monitor */
int _dhcpcStateTaskStackSize = 5000;    /* Stack size of lease monitor */

LOCAL int dhcpcOfferTimeout;           /* configAll.h #define value */
LOCAL int dhcpcDefaultLease;           /* configAll.h #define value */
LOCAL FUNCPTR dhcpcCacheHookRtn; 	/* Data storage/retrieval function */

LOCAL int dhcpcSignalSock;              /* Indicates start of lease session */
LOCAL char dhcpcSignalData [UDPHL + IPHL + 1];    /* Contents of signal */
LOCAL int dhcpcSignalSize = UDPHL + IPHL + 1;

/* forward declarations */

LOCAL void dhcpcState (void);           /* Monitor pending/active leases */
LOCAL STATUS dhcpcEventGet (EVENT_DATA *);
LOCAL STATUS dhcpcEventHandle (EVENT_DATA *);
LOCAL void dhcpcCleanup (void);
LOCAL char *dhcpcOptionFind (LEASE_DATA *, int, int *);   /* Locate option */

IMPORT void dhcpcRead (void);            /* Retrieve incoming DHCP messages */
STATUS dhcpcVerify (void *);				/* Renew lease */

/*******************************************************************************
*
* dhcpcLibInit - DHCP client library initialization
*
* This routine creates and initializes the global data structures used by 
* the DHCP client library to maintain multiple leases, up to the limit 
* specified by the <maxLeases> parameter.  Every subsequent lease attempt will
* collect additional DHCP offers until the interval specified by <offerTimeout>
* expires and will request the lease duration indicated by <defaultLease>.
* The <maxSize> parameter specifies the maximum length supported for any DHCP
* message, including the UDP and IP headers and the largest link level header
* for all supported devices. The maximum length of the DHCP options field is
* based on this value or the MTU size for a lease's underlying interface,
* whichever is less. The smallest valid value for the <maxSize> parameter is
* 576 bytes, corresponding to the minimum IP datagram a host must accept.
* Larger values will allow the client to handle longer DHCP messages.
*
* This routine must be called before calling any other library routines.  The 
* routine is called automatically if INCLUDE_DHCPC is defined at the time the 
* system is built and assigns the global lease settings to the values specified
* by DHCPC_SPORT, DHCPC_CPORT, DHCPC_MAX_LEASES, DHCPC_MAX_MSGSIZE,
* DHCPC_DEFAULT_LEASE, and DHCPC_OFFER_TIMEOUT.
*
* RETURNS: OK, or ERROR if initialization fails.
*
* ERRNO: S_dhcpcLib_MEM_ERROR
* 
*/

STATUS dhcpcLibInit 
    (
    int 	serverPort, 	/* port used by DHCP servers (default 67) */
    int 	clientPort, 	/* port used by DHCP clients (default 68) */
    int         maxLeases, 	/* max number of simultaneous leases allowed */
    int 	maxSize, 	/* largest DHCP message supported, in bytes */
    int 	offerTimeout, 	/* interval to get additional DHCP offers */ 
    int 	defaultLease, 	/* default value for requested lease length */
    int 	minLease 	/* minimum accepted lease length */
    )
    {
    int offset;
    int loop;
    int bufSize; 	/* Size of receive buffer (maxSize + BPF header) */
    int stateTaskId; 	/* Identifier for client monitor task */
    STATUS result;
    struct sockaddr_in dstaddr;
    struct ip * 	pIph;
    struct udphdr * 	pUdph;

    if (dhcpcInitialized)
        return (OK);

    if (maxSize < DFLTDHCPLEN + UDPHL + IPHL)
        {
        errno = S_dhcpcLib_MEM_ERROR;
        return (ERROR);
        }

    /* Create a socket to signal the read task when a new session starts. */

    dhcpcSignalSock = socket (AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (dhcpcSignalSock < 0)
        return (ERROR);

    offset = 1;
    result = setsockopt (dhcpcSignalSock, IPPROTO_IP, IP_HDRINCL,
                         (char *)&offset, sizeof (int));
    if (result != OK)
        {
        close (dhcpcSignalSock);
        return (ERROR);
        }

    bzero ( (char *)&dstaddr, sizeof (dstaddr));
    dstaddr.sin_family = AF_INET;
    dstaddr.sin_addr.s_addr = htonl (0x7f000001);    /* 127.0.0.1: loopback */

    result = connect (dhcpcSignalSock, (struct sockaddr *) &dstaddr,
                          sizeof (dstaddr));
    if (result != OK)
        {
        close (dhcpcSignalSock);
        return (ERROR);
        }

    /* Create the signalling message (sent to loopback at DHCP client port). */

    bzero (dhcpcSignalData, UDPHL + IPHL + 1);
    pIph = (struct ip *)dhcpcSignalData;
    pUdph = (struct udphdr *) (dhcpcSignalData + IPHL);

    pIph->ip_v = IPVERSION;
    pIph->ip_hl = IPHL >> 2;
    pIph->ip_tos = 0;
    pIph->ip_len = UDPHL + IPHL + 1;
    pIph->ip_id = htons (0xF1C);
    pIph->ip_off = IP_DF;
    pIph->ip_ttl = 0x20;
    pIph->ip_p = IPPROTO_UDP;
    pIph->ip_src.s_addr = 0;
    pIph->ip_dst.s_addr = htonl(0x7f000001);    /* 127.0.0.1: loopback */
    pIph->ip_sum = checksum ( (u_short *)pIph, IPHL);

    pUdph->uh_sport = 0;
    pUdph->uh_dport = htons (clientPort);
    pUdph->uh_ulen = htons (UDPHL + 1);

    dhcpcSignalData [UDPHL + IPHL] = 'a';

    /* Create a socket to send and receive DHCP messages. */

    dhcpcDataSock = socket (AF_INET, SOCK_DGRAM, 0);
    if (dhcpcDataSock < 0)
        {
        close (dhcpcSignalSock);
        return (ERROR);
        }

    bzero ( (char *)&dstaddr, sizeof (dstaddr));
    dstaddr.sin_family = AF_INET;
    dstaddr.sin_addr.s_addr = htonl (INADDR_ANY);
    dstaddr.sin_port = htons (clientPort); 

    result = bind (dhcpcDataSock, (struct sockaddr *) &dstaddr,
                       sizeof (dstaddr));
    if (result != OK)
        {
        close (dhcpcSignalSock);
        close (dhcpcDataSock);
        return (ERROR);
        }

    if (bpfDrv () == ERROR)
        {
        close (dhcpcSignalSock);
        close (dhcpcDataSock);
        return (ERROR);
        }

    bufSize = maxSize + sizeof (struct bpf_hdr);
    if (bpfDevCreate ("/bpf/dhcpc", maxLeases, bufSize) == ERROR)
        {
        close (dhcpcSignalSock);
        close (dhcpcDataSock);
        bpfDrvRemove ();
        return (ERROR);
        }

    /* Create and initialize storage for active leases. */

    dhcpcLeaseList = malloc (maxLeases * sizeof (LEASE_DATA *));
    if (dhcpcLeaseList == NULL)
        {
        close (dhcpcSignalSock);
        close (dhcpcDataSock);
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        errno = S_dhcpcLib_MEM_ERROR;
        return (ERROR);
        }

    for (loop = 0; loop < maxLeases; loop++)
        dhcpcLeaseList [loop] = NULL;

    dhcpcMaxLeases = maxLeases;
    dhcpcDefaultLease = defaultLease;
    dhcpcOfferTimeout = offerTimeout;
    dhcpcMinLease = minLease;

    /*
     * Create protection semaphore for all lease status indicators.  User 
     * requests which require an active lease check this indicator before 
     * executing.  A single semaphore is used to reduce system resource 
     * requirements, even though this behavior may allow user requests to
     * slightly delay status updates for unrelated leases.
     */

    dhcpcMutexSem = semBCreate (SEM_Q_FIFO, SEM_FULL);
    if (dhcpcMutexSem == NULL)
        {
        close (dhcpcSignalSock);
        close (dhcpcDataSock);
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        free (dhcpcLeaseList);
        errno = S_dhcpcLib_MEM_ERROR;
        return (ERROR);
        }

    /* Create signalling semaphore for event notification ring.  */

    dhcpcEventSem = semCCreate (SEM_Q_FIFO, 0);
    if (dhcpcEventSem == NULL)
        {
        close (dhcpcSignalSock);
        close (dhcpcDataSock);
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        free (dhcpcLeaseList);
        semDelete (dhcpcMutexSem);
        errno = S_dhcpcLib_MEM_ERROR;
        return (ERROR);
        }

    /* Create event storage.  */

    dhcpcEventRing = rngCreate (EVENT_RING_SIZE);
    if (dhcpcEventRing == NULL)
        {
        close (dhcpcSignalSock);
        close (dhcpcDataSock);
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        free (dhcpcLeaseList);
        semDelete (dhcpcMutexSem);
        semDelete (dhcpcEventSem);
        errno = S_dhcpcLib_MEM_ERROR;
        return (ERROR);
        }

    /* Create message storage (matches number of elements in event ring). */

    dhcpcMessageList = malloc (10 * sizeof (MESSAGE_DATA));
    if (dhcpcMessageList == NULL)
        {
        close (dhcpcSignalSock);
        close (dhcpcDataSock);
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        free (dhcpcLeaseList);
        semDelete (dhcpcMutexSem);
        semDelete (dhcpcEventSem);
        rngDelete (dhcpcEventRing);
        errno = S_dhcpcLib_MEM_ERROR;
        return (ERROR);
        }

    /* Allocate receive buffers based on maximum message size. */

    for (loop = 0; loop < 10; loop++)
        {
        dhcpcMessageList [loop].msgBuffer = memalign (4, bufSize);
        if (dhcpcMessageList [loop].msgBuffer == NULL)
            break;

        dhcpcMessageList [loop].writeFlag = TRUE;
        }

    if (loop < 10)
        {
        for (offset = 0; offset < loop; offset++)
            free (dhcpcMessageList [offset].msgBuffer);

        close (dhcpcSignalSock);
        close (dhcpcDataSock);
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        free (dhcpcLeaseList);
        semDelete (dhcpcMutexSem);
        semDelete (dhcpcEventSem);
        rngDelete (dhcpcEventRing);
        free (dhcpcMessageList);
        errno = S_dhcpcLib_MEM_ERROR;
        return (ERROR);
        }
 
    /* Set up state machine and data structures for outgoing messages.  */

    result = dhcp_client_setup (serverPort, clientPort, maxSize);
    if (result == ERROR)
        {
        close (dhcpcSignalSock);
        close (dhcpcDataSock);
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        free (dhcpcLeaseList);
        semDelete (dhcpcMutexSem);
        semDelete (dhcpcEventSem);
        rngDelete (dhcpcEventRing);
        for (offset = 0; offset < 10; offset++)
            free (dhcpcMessageList [offset].msgBuffer);
        free (dhcpcMessageList);
        errno = S_dhcpcLib_MEM_ERROR;
        return (ERROR);
        }

    dhcpcCacheHookRtn = NULL;

    /*
     * Spawn the monitor task.  The entry routine will drive the client
     * state machine after processing all incoming DHCP messages and timer
     * related or user generated events.
     */

    result = taskSpawn ("tDhcpcStateTask", _dhcpcStateTaskPriority,
                        _dhcpcStateTaskOptions, _dhcpcStateTaskStackSize,
                        (FUNCPTR) dhcpcState, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (result == ERROR)
        {
        close (dhcpcSignalSock);
        close (dhcpcDataSock);
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        free (dhcpcLeaseList);
        semDelete (dhcpcMutexSem);
        semDelete (dhcpcEventSem);
        rngDelete (dhcpcEventRing);
        for (offset = 0; offset < 10; offset++)
            free (dhcpcMessageList [offset].msgBuffer);
        free (dhcpcMessageList);
        errno = S_dhcpcLib_MEM_ERROR;
        return (ERROR);
        }
    stateTaskId = result;

    /*
     * Spawn the data retrieval task.  The entry routine will read all 
     * incoming DHCP messages and signal the monitor task when a
     * valid one arrives.
     */

    result = taskSpawn ("tDhcpcReadTask", _dhcpcReadTaskPriority,
                        _dhcpcReadTaskOptions, _dhcpcReadTaskStackSize,
                        (FUNCPTR) dhcpcRead, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (result == ERROR)
        {
        close (dhcpcSignalSock);
        close (dhcpcDataSock);
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        free (dhcpcLeaseList);
        semDelete (dhcpcMutexSem);
        semDelete (dhcpcEventSem);
        rngDelete (dhcpcEventRing);
        for (offset = 0; offset < 10; offset++)
            free (dhcpcMessageList [offset].msgBuffer);
        free (dhcpcMessageList);
        taskDelete (stateTaskId);
        errno = S_dhcpcLib_MEM_ERROR;
        return (ERROR);
        }
    dhcpcReadTaskId = result;

    dhcpcBufSize = maxSize;

    /* Reset the filter to use the specified client port. */

    dhcpfilter [12].k = clientPort;

    /* Minimum IP and UDP packet lengths */

    dhcpfilter [14].k = (DFLTDHCPLEN - DFLTOPTLEN + 4) + UDPHL + IPHL;
    dhcpfilter [16].k = (DFLTDHCPLEN - DFLTOPTLEN + 4) + UDPHL;

    dhcpcInitialized = TRUE;

    return (OK);
    }

/*******************************************************************************
*
* dhcpcLeaseGet - read lease information from boot line
*
* If an automatic configuration protocol was used by the bootstrap loader
* (i.e. - SYSFLG_AUTOCONFIG was set), this routine is called to handle
* possible DHCP values in the address string.  If the target address indicated 
* by <pAddrString> contains both lease duration and lease origin values, it was
* assigned by a DHCP server and the routine sets the status flag accessed by 
* <pDhcpStatus> so that the lease will be verified or renewed.  This routine is 
* called when initializing the network during system startup and will not 
* function correctly if used in any other context.  Any lease information 
* attached to the address string is removed when this routine is executed.
*
* RETURNS: OK if setup completes, or ERROR if the address string is invalid.
*
* ERRNO: N/A
*
* NOMANUAL
*/

STATUS dhcpcLeaseGet
    (
    char * 	pAddrString, 	/* client address string from bootline */
    BOOL * 	pDhcpStatus 	/* DHCP lease values found? */
    )
    {
    char * 	pDelim;
    char * 	pOffset;
    int 	result;

    dhcpcBootLease.lease_duration = 0; 	/* Defaults to expired.  */
    dhcpcBootLease.lease_origin = 0; 	/* Defaults to invalid value.  */

    /* Read DHCP lease values and remove from address string.  */

    result = bootLeaseExtract (pAddrString, &dhcpcBootLease.lease_duration,
                               &dhcpcBootLease.lease_origin);
    if (result == 2)
        *pDhcpStatus = TRUE;    /* DHCP lease values read successfully.  */
    else
        {
        *pDhcpStatus = FALSE;    /* Valid DHCP lease values not present.  */

        if (result == 0)
            {
            /* 
             * The ":" field separator for either the netmask or duration was
             * not found, so no DHCP lease values are present. 
             */

            return (OK);
            }

        if (result == 1)
            {
            /* 
             * Only the lease duration field was present.
             * The DHCP lease values have been removed.
             */

           return (ERROR);
           }

        /* 
         * Parsing one of the lease values failed.  Remove
         * any DHCP lease data from the address string.
         */

        /* Find delimiter for netmask.  */

        pOffset = index (pAddrString, ':');

        /*
         * Start at the delimiter for DHCP lease values.  The netmask separator
         * is actually always present at this point, but check for it anyway.
         */

        if (pOffset != NULL)
            {
            pDelim = pOffset + 1;

            /* 
             * Find the lease duration tag.  This field separator is also
             * guaranteed to be present, or the extract routine would have
             * returned 0.
             */

            pOffset = index (pDelim, ':');

            /* Remove the DHCP lease values from string.  */

            if (pOffset != NULL)
                *pOffset = EOS;
            }
        return (ERROR);
        }
    return (OK);
    }

/*******************************************************************************
*
* dhcpcConfigSet - set system configuration according to active DHCP lease
*
* This routine verifies or renews any DHCP lease established during the system 
* boot.  If a DHCP lease was established, the dhcpcInit() call in this routine
* will setup the necessary data structures and create a cookie identifying the 
* boot lease.  The cookie is stored in the pDhcpcBootCookie global variable to
* provide users with access to the boot-time lease.  This routine is called when
* initializing the network during system startup and will not function
* correctly if executed in any other context.
*
* RETURNS: OK if configuration completed, or ERROR otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

STATUS dhcpcConfigSet
    (
    BOOT_PARAMS * 	pParams, 	/* structure for parsed bootline */
    char * 		pAddrString, 	/* specified IP address */
    int * 		pNetmask, 	/* specified subnet mask */
    BOOL *		pDhcpStatus, 	/* DHCP used to get address? */
    BOOL 		attachFlag 	/* Device configuration required? */
    )
    {
    char 		netDev [BOOT_DEV_LEN + 1];
    struct dhcp_param 	bootParams;

    BOOL 		dhcpStatus;
    u_long 		result;
    struct ifnet * 	pIf;
    LEASE_DATA * 	pLeaseData;
    char		bootFile[BOOT_FILE_LEN];
    int			netmask;

    dhcpStatus = *pDhcpStatus;

    if (dhcpStatus && !attachFlag)
        {
        /*
         * Fatal error: automatic configuration is required but not available.
         */

        return (ERROR);
        }

    if (dhcpStatus == FALSE && pAddrString [0] != EOS)
        {
        /* 
         * BOOTP reply was selected during system startup or the IP address
         * was assigned manually.  Lease renewal (or new lease) is not needed.
         */

        return (OK);
        }
    
    /* Verify DHCP lease obtained during boot or get a new one.  */

    bzero ( (char *)&bootParams, sizeof (struct dhcp_param));

    if (((strncmp (pParams->bootDev, "scsi", 4) == 0) ||
        (strncmp (pParams->bootDev, "ide", 3) == 0) ||
        (strncmp (pParams->bootDev, "ata", 3) == 0) ||
        (strncmp (pParams->bootDev, "fd", 2) == 0)  ||
        (strncmp (pParams->bootDev, "tffs", 4) == 0)) &&
        pParams->other [0] != EOS)
        sprintf (netDev, "%s%d", pParams->other, pParams->unitNum);
    else
        sprintf (netDev, "%s%d", pParams->bootDev, pParams->unitNum);

    if (pAddrString [0] != EOS)
        {
        /* Fill in client address obtained from bootline. */

        result = inet_addr (pAddrString);
        if (result == ERROR)
            {
            printf ("Invalid target address \"%s\"\n", pAddrString);
            return (ERROR);
            }
        dhcpcBootLease.yiaddr.s_addr = result;
        }

    pIf = ifunit (netDev);
    if (pIf == NULL)
        {
        printf ("Invalid device \"%s\"\n", netDev);
        return (ERROR);
        }

    /* 
     * Initialize required variables and get the lease identifier.  
     * The resulting lease will always apply the address information
     * to the specified network interface.  
     */

    pDhcpcBootCookie = dhcpcInit (pIf, TRUE);
    if (pDhcpcBootCookie == NULL)
        {
        printf ("Error initializing DHCP boot lease.\n");
        return (ERROR);
        }

    if (pAddrString [0] != EOS)
        {
        unsigned long duration;
        /*
         * Setup to renew the boot lease. Use the cookie to access the
         * lease-specific data structures.  For now, just typecast
         * the cookie.  This translation could be replaced with a more
         * sophisticated lookup at some point.
         */

        pLeaseData = (LEASE_DATA *)pDhcpcBootCookie;

        /* Set the lease descriptor to identify the boot lease.  */

        pLeaseData->leaseType = DHCP_AUTOMATIC;

        /* Make sure the lease duration is in network byte order. */

        duration = htonl(dhcpcBootLease.lease_duration);
        dhcpcOptionAdd (pDhcpcBootCookie, _DHCP_LEASE_TIME_TAG, 
                        4, (UCHAR *)&duration);
        }

    /*
     * Execute the bind call synchronously to verify any boot lease
     * or get the required IP address.
     */

    if (dhcpcBind (pDhcpcBootCookie, TRUE) != OK)
        {
        if (dhcpStatus == TRUE)
            printf ("Can't renew DHCP boot lease.\n");
        else
            printf ("Can't get IP address for device.\n");

        pDhcpcBootCookie = NULL;
        return (ERROR);
        }

    bootParams.file = bootFile;
    bootParams.subnet_mask = (struct in_addr *)&netmask;
    if (dhcpcParamsGet (pDhcpcBootCookie, &bootParams) == ERROR)
        {
        printf ("Can't get network data from DHCP lease.\n");
        pDhcpcBootCookie = NULL;
        return (ERROR);
        }

    /*
     * The netmask is returned to us in network byte order,
     * but the caller expects it in host order.
     */

    *pNetmask = ntohl(netmask);


    /* Fill in boot file */

    if (bootParams.file[0] != EOS)
	bcopy (bootFile, pParams->bootFile, BOOT_FILE_LEN);

    /* Fill in host address.  */

    if (bootParams.siaddr.s_addr)
        inet_ntoa_b (bootParams.siaddr, pParams->had);

    /* Fill in new or verified target address. */

    inet_ntoa_b (bootParams.yiaddr, pAddrString);

    /* Save timing information so later reboots will detect DHCP leases. */

    dhcpcBootLease.lease_duration = bootParams.lease_duration;
    dhcpcBootLease.lease_origin = bootParams.lease_origin;

    /* Update status in case IP address is newly assigned. */

    *pDhcpStatus = TRUE;

    return (OK);
    }

/*******************************************************************************
*
* dhcpcInit - assign network interface and setup lease request
*
* This routine creates the data structures used to obtain a set of parameters 
* with DHCP and must be called before each attempt at establishing a DHCP 
* lease, but after the dhcpcLibInit() routine has initialized the global data
* structures.
*
* The <pIf> argument indicates the network device which will be used for
* transmission and reception of DHCP messages during the lifetime of the
* lease.  The DHCP client supports devices attached to the IP protocol
* with the MUX/END interface.  The specified device must be capable of
* sending broadcast messages.  It also supports BSD Ethernet devices
* attached to the IP protocol. The MTU size of any interface must be large
* enough to receive a minimum IP datagram of 576 bytes. If the interface MTU
* size is less than the maximum message size set in the library initialization
* it also determines the maximum length of the DHCP options field.
*
* If the <autoConfig> parameter is set to TRUE, any address information
* obtained will automatically be applied to the specified interface. The
* <autoConfig> parameter also selects the default option request list for
* a lease.  If set to FALSE, no specific lease options are requested since
* any configuration parameters obtained are not intended for the underlying
* network device.  In that case, any specific options required may be added
* to the request list at any time before the corresponding dhcpcBind() call.
* If <autoConfig> is TRUE, this routine sets the configuration parameters to
* request the minimal address information (subnet mask and broadcast address)
* necessary for reconfiguring the network device specified by <pIf>.
*
* The internal lease identifier returned by this routine must be used in
* subsequent calls to the DHCP client library.
*
* NOTE
* This routine is called automatically during system startup if the DHCP 
* client was used to obtain the VxWorks boot parameters.  The resulting 
* lease will always reconfigure the network boot device.  Therefore, any
* further calls to this routine which specify the network boot device for 
* use in obtaining additional DHCP leases must set <autoConfig> to FALSE.
* Otherwise, that device will be unable to maintain a stable configuration.
* The global variable pDhcpcBootCookie provides access to the configuration 
* parameters for any DHCP lease created during system startup.
* 
* RETURNS: Lease handle for later use, or NULL if lease setup fails.
*
* ERRNO: S_dhcpcLib_NOT_INITIALIZED, S_dhcpcLib_BAD_DEVICE, S_dhcpcLib_BAD_OPTION,
*  S_dhcpcLib_MAX_LEASES_REACHED, S_dhcpcLib_MEM_ERROR
*
* SEE ALSO
* dhcpcOptionSet(), dhcpcEventHookAdd()
*/

void * dhcpcInit
    (
    struct ifnet *	pIf, 		/* network device used by client */
    BOOL 		autoConfig 	/* reconfigure network device? */
    )
    {
    void * 			pCookie;
    LEASE_DATA *	 	pLeaseData;
    struct dhcp_reqspec * 	pReqSpec;
    int offset;
    int result;
    int maxSize; 	/* Largest message, based on MTU and buffer sizes */
    int bufSize; 	/* Size of internal BPF buffer for lease */

    BOOL bsdFlag = FALSE;

    char bpfDevName [_DHCPC_MAX_DEVNAME];  /* "/bpf/dhcpc" + max unit number */
    int bpfDev;

    if (!dhcpcInitialized)
        {
        errno = S_dhcpcLib_NOT_INITIALIZED;
        return (NULL);
        }

    if (pIf == NULL)
        {
        errno = S_dhcpcLib_BAD_DEVICE;
        return (NULL);
        }

    if (muxDevExists (pIf->if_name, pIf->if_unit) == FALSE)
        {
        bsdFlag = TRUE; 
        }

    /* Verify interface data sizes are appropriate for message. */

    if (pIf->if_mtu == 0)
        {
        errno = S_dhcpcLib_BAD_DEVICE;
        return (NULL);
        }

    maxSize = pIf->if_mtu;
    if (maxSize < DHCP_MSG_SIZE)
        {
        /*
         * Devices must accept messages equal to the minimum IP datagram
         * of 576 bytes, which corresponds to a DHCP message with up to
         * 312 bytes of options.
         */

        errno = S_dhcpcLib_BAD_DEVICE;
        return (NULL);
        }

    /* Reduce maximum DHCP message to size of transmit buffer if necessary. */

    if (maxSize > dhcpcBufSize)
        maxSize = dhcpcBufSize;

    if (autoConfig != TRUE && autoConfig != FALSE)
        {
        errno = S_dhcpcLib_BAD_OPTION;
        return (NULL);
        }

    /* Find an unused entry in the array of lease-specific variables.  */

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] == NULL)
            break; 
   
    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_MAX_LEASES_REACHED;
        return (NULL);
        }

    /* Get a BPF file descriptor to read/write DHCP messages. */

    sprintf (bpfDevName, "/bpf/dhcpc%d", offset);

    bpfDev = open (bpfDevName, 0, 0);
    if (bpfDev < 0)
        {
        errno = S_dhcpcLib_BAD_DEVICE;
        return (NULL);
        }

    /* Enable immediate mode for reading messages. */

    result = 1;
    result = ioctl (bpfDev, BIOCIMMEDIATE, (int)&result);
    if (result != 0)
        {
        close (bpfDev);
        errno = S_dhcpcLib_BAD_DEVICE;
        return (NULL);
        }

    /*
     * To save space, reset the internal BPF buffer to a value based
     * on the interface MTU size, if it is less than the maximum receive
     * buffer size assigned during the library initialization. The
     * DHCP client's read task handles the possibility of multiple
     * messages per receive buffer.
     */

    bufSize = pIf->if_mtu + pIf->if_hdrlen;
    if (bufSize < dhcpcBufSize)
        {
        bufSize += sizeof (struct bpf_hdr);
        result = ioctl (bpfDev, BIOCSBLEN, (int)&bufSize);
        if (result != 0)
            {
            close (bpfDev);
            errno = S_dhcpcLib_BAD_DEVICE;
            return (NULL);
            }
        }
    else
        bufSize = dhcpcBufSize + sizeof (struct bpf_hdr);

    /* Allocate all lease-specific variables.  */
    
    pLeaseData = (LEASE_DATA *)calloc (1, sizeof (LEASE_DATA));
    if (pLeaseData == NULL)
        {
        close (bpfDev);
        errno = S_dhcpcLib_MEM_ERROR;
        return (NULL);
        }

    pLeaseData->initFlag = FALSE;

    pLeaseData->autoConfig = autoConfig;

    /* 
     * Use the current data storage hook routine (if any)
     * throughout the lifetime of this lease.
     */

    pLeaseData->cacheHookRtn = dhcpcCacheHookRtn;

    /* 
     * For now, use the lease data pointer as the unique lease identifier.
     * This could be changed later to shield the internal structures from
     * the user, but it allows fast data retrieval.
     */

    pCookie = (void *)pLeaseData;

    pReqSpec = &pLeaseData->leaseReqSpec;

    bzero ( (char *)pReqSpec, sizeof (struct dhcp_reqspec));
    bzero ( (char *)&pLeaseData->ifData, sizeof (struct if_info));

    /* Initialize WIDE project global containing network device data.  */ 

    sprintf (pLeaseData->ifData.name, "%s", pIf->if_name);
    pLeaseData->ifData.unit = pIf->if_unit;
    pLeaseData->ifData.iface = pIf;
    pLeaseData->ifData.bpfDev = bpfDev;
    pLeaseData->ifData.bufSize = bufSize;

    /*
     * For END devices, use hardware address information set when
     * driver attached to IP. Assume BSD devices use Ethernet frames.
     */

    if (bsdFlag)
        pLeaseData->ifData.haddr.htype = ETHER;
    else
        pLeaseData->ifData.haddr.htype = dhcpConvert (pIf->if_type);

    pLeaseData->ifData.haddr.hlen = pIf->if_addrlen;

    if (pLeaseData->ifData.haddr.hlen > MAX_HLEN)
        pLeaseData->ifData.haddr.hlen = MAX_HLEN;

    bcopy ( (char *) ( (struct arpcom *)pIf)->ac_enaddr,
           (char *)&pLeaseData->ifData.haddr.haddr, 
           pLeaseData->ifData.haddr.hlen); 

    /*
     * Set the maximum message length based on the MTU size and the
     * maximum message/buffer size specified during initialization.
     */

    pReqSpec->maxlen = maxSize;

    /* set duration of client's wait for multiple offers */

    pReqSpec->waitsecs = dhcpcOfferTimeout; 

    pReqSpec->reqlist.len = 0;    /* No options requested yet.  */

    /*
     * If executing with startup lease, or if automatic configuration is
     * requested, initialize request list with tags for options required 
     * by all network interfaces, using RFC 1533 tag values.
     */

    if (pLeaseData->autoConfig || pLeaseData->leaseType == DHCP_AUTOMATIC)
        {
        pReqSpec->reqlist.list [pReqSpec->reqlist.len++] =
                                                        _DHCP_SUBNET_MASK_TAG;
        pReqSpec->reqlist.list [pReqSpec->reqlist.len++] =
                                                        _DHCP_BRDCAST_ADDR_TAG;
        }

    /*
     * For now, create a separate watchdog timer for each lease.  Eventually,
     * a single watchdog timer will suffice for all leases, when combined
     * with a sorted queue containing relative firing times.  This enhancement
     * can wait.
     */

    /* Create event timer for state machine.  */

    pLeaseData->timer = wdCreate ();
    if (pLeaseData->timer == NULL)
        {
        free (pLeaseData);
        close (bpfDev);
        errno = S_dhcpcLib_MEM_ERROR;
        return (NULL);
        }

    /* Create signalling semaphore for completion of negotiation process.  */

    pLeaseData->leaseSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
    if (pLeaseData->leaseSem == NULL)
        {
        wdDelete (pLeaseData->timer);
        free (pLeaseData);
        close (bpfDev);
        errno = S_dhcpcLib_MEM_ERROR;
        return (NULL);
        }

    /* 
     * Manual leases will not reset the network interface unless specifically
     * requested to do so.  This lease type is replaced with DHCP_AUTOMATIC for 
     * the lease established during system startup.  The parameters for
     * an automatic lease are always applied to the underlying interface.
     */

    pLeaseData->leaseType = DHCP_MANUAL;

    pLeaseData->initFlag = TRUE;

    /* Store the lease-specific data in the global list.  */

    dhcpcLeaseList [offset] = pLeaseData;

    /* Notify the DHCP read task that a new session is available. */

    send (dhcpcSignalSock, dhcpcSignalData, dhcpcSignalSize, 0);

    return (pCookie);
    }

/*******************************************************************************
*
* dhcpcEventHookAdd - add a routine to handle configuration parameters
*
* This routine installs a hook routine to handle changes in the configuration
* parameters provided for the lease indicated by <pCookie>.  The hook provides 
* an alternate configuration method for DHCP leases and uses the following 
* interface:
* .CS
* void dhcpcEventHookRtn
*     (
*     int 	leaseEvent,	/@ new or expired parameters @/
*     void * 	pCookie 	/@ lease identifier from dhcpcInit() @/
*     )
* .CE
*
* The routine is called with the <leaseEvent> parameter set to DHCPC_LEASE_NEW
* whenever a lease is successfully established.  The DHCPC_LEASE_NEW event
* does not occur when a lease is renewed by the same DHCP server, since the
* parameters do not change in that case.  However, it does occur if the 
* client rebinds to a different DHCP server.  The DHCPC_LEASE_INVALID event
* indicates that the configuration parameters for the corresponding lease may
* no longer be used.  That event occurs when a lease expires or a renewal
* or verification attempt fails, and coincides with re-entry into the initial 
* state of the negotiation process.
*
* If the lease initialization specified automatic configuration of the
* corresponding network interface, any installed hook routine will be invoked
* after the new address information is applied.
*
* RETURNS: OK if notification hook added, or ERROR otherwise.
*
* ERRNO: S_dhcpcLib_BAD_COOKIE, S_dhcpcLib_NOT_INITIALIZED
*/

STATUS dhcpcEventHookAdd
    (
    void * 	pCookie, 	/* identifier returned by dhcpcInit() */
    FUNCPTR 	pEventHook	/* routine to handle lease parameters */
    )
    {
    LEASE_DATA * 	pLeaseData;
    int offset;

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL &&
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_BAD_COOKIE;
        return (ERROR);
        }

    if (!dhcpcInitialized || !pLeaseData->initFlag)
        {
        errno = S_dhcpcLib_NOT_INITIALIZED;
        return (ERROR);
        }

    /* Assign the event notification hook.  */

    pLeaseData->eventHookRtn = pEventHook;

    return (OK);
    }

/*******************************************************************************
*
* dhcpcEventHookDelete - remove the configuration parameters handler
*
* This routine removes the hook routine that handled changes in the 
* configuration parameters for the lease indicated by <pCookie>.
* If the lease initialization specified automatic configuration of the
* corresponding network interface, the assigned address could change
* without warning after this routine is executed.
*
* RETURNS: OK if notification hook removed, or ERROR otherwise.
*
* ERRNO: S_dhcpcLib_BAD_COOKIE, S_dhcpcLib_NOT_INITIALIZED
* 
*/

STATUS dhcpcEventHookDelete
    (
    void * 	pCookie 	/* identifier returned by dhcpcInit() */
    )
    {
    LEASE_DATA * 	pLeaseData;
    int offset;

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL &&
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_BAD_COOKIE;
        return (ERROR);
        }

    if (!dhcpcInitialized || !pLeaseData->initFlag)
        {
        errno = S_dhcpcLib_NOT_INITIALIZED;
        return (ERROR);
        }

    /* Remove the event notification hook. */

    pLeaseData->eventHookRtn = NULL;

    return (OK);
    }

/*******************************************************************************
*
* dhcpcCacheHookAdd - add a routine to store and retrieve lease data
*
* This routine adds a hook routine that is called at the bound state
* (to store the lease data) and during the INIT_REBOOT state (to re-use the
* parameters if the lease is still active).  The calling sequence of the 
* input hook routine is:
* .CS
* STATUS dhcpcCacheHookRtn
*     (
*     int command,			/@ requested cache operation @/
*     unsigned long *pTimeStamp,	/@ lease timestamp data @/
*     int *pDataLen,			/@ length of data to access @/
*     char *pBuffer			/@ pointer to data buffer @/
*     )
*
* .CE
* The hook routine should return OK if the requested operation is completed
* successfully, or ERROR otherwise.  All the supplied pointers reference memory 
* locations that are reused upon return from the hook.  The hook routine 
* must copy the data elsewhere.
*
* NOTE
* The setting of the cache hook routine during a dhcpcInit() call is
* recorded and used by the resulting lease throughout its lifetime.
* Since the hook routine is intended to store a single lease record,
* a separate hook routine should be specified before the dhcpcInit()
* call for each lease which will re-use its parameters across reboots.
*
* IMPLEMENTATION
* The <command> parameter specifies one of the following operations:
* .IP DHCP_CACHE_WRITE 25
* Save the indicated data.  The write operation must preserve the value
* referenced by <pTimeStamp> and the contents of <pBuffer>.  The <pDataLen> 
* parameter indicates the number of bytes in that buffer.
* .IP DHCP_CACHE_READ 
* Restore the saved data.  The read operation must copy the data from the
* most recent write operation into the location indicated by <pBuffer>,
* set the contents of <pDataLen> to the amount of data provided, and store 
* the corresponding timestamp value in <pTimeStamp>.
* .IP
* The read operation has very specific requirements.  On entry, the value 
* referenced by <pDataLen> indicates the maximum buffer size available at
* <pBuffer>.  If the amount of data stored by the previous write exceeds this 
* value, the operation must return ERROR.  A read must also return ERROR if the 
* saved timestamp value is 0.  Finally, the read operation must return ERROR if 
* it is unable to retrieve all the data stored by the write operation or if the
* previous write was unsuccessful.
* .IP DHCP_CACHE_ERASE 
* Ignore all stored data.  Following this operation, subsequent read operations
* must return ERROR until new data is written.  All parameters except 
* <command> are NULL.
* .LP
*
* RETURNS: OK, always.
*
* ERRNO: N/A
*/

STATUS dhcpcCacheHookAdd
    (
    FUNCPTR 	pCacheHookRtn 	/* routine to store/retrieve lease data */
    )
    {
    dhcpcCacheHookRtn = pCacheHookRtn;
    return (OK);
    }

/*******************************************************************************
*
* dhcpcCacheHookDelete - delete a lease data storage routine
*
* This routine deletes the hook used to store lease data, preventing
* re-use of the configuration parameters across system reboots for
* all subsequent lease attempts.  Currently active leases will continue
* to use the routine specified before the lease initialization.
*
* RETURNS: OK, always.
*
* ERRNO: N/A
*/

STATUS dhcpcCacheHookDelete (void)
    {
    dhcpcCacheHookRtn = NULL;
    return (OK);
    }

/*******************************************************************************
*
* dhcpcBind - obtain a set of network configuration parameters with DHCP
*
* This routine initiates a DHCP negotiation according to the process described 
* in RFC 1541.  The <pCookie> argument contains the return value of an earlier 
* dhcpcInit() call and is used to identify a particular lease.
*
* The <syncFlag> parameter specifies whether the DHCP negotiation started by 
* this routine will execute synchronously or asynchronously.  An asynchronous 
* execution will return after starting the DHCP negotiation, but a synchronous 
* execution will only return once the negotiation process completes.
*
* When a new lease is established, any event hook provided for the lease
* will be called to process the configuration parameters.  The hook is also 
* called when the lease expires or the negotiation process fails.  The results 
* of an asynchronous DHCP negotiation are not available unless an event hook 
* is installed.
*
* If automatic configuration of the underlying network interface was specified
* during the lease initialization, this routine will prevent all higher-level 
* protocols from accessing the underlying network interface used during the 
* initial lease negotiation until that process is complete.  In addition, any 
* addressing information obtained will be applied to that network interface, 
* which will remain disabled if the initial negotiation fails.  Finally, the
* interface will be disabled if the lease expires.
*
* NOTE
* If the DHCP client is used to obtain the VxWorks boot parameters, this 
* routine is called automatically during system startup using the automatic 
* reconfiguration.  Therefore, any calls to this routine which use the network 
* boot device for message transfer when the DHCP client was used at boot time 
* must not request automatic reconfiguration during initialization.  Otherwise, 
* the resulting lease settings will conflict with the configuration maintained 
* by the lease established during system startup. 
*
* RETURNS: OK if routine completes, or ERROR otherwise.
*
* ERRNO: S_dhcpcLib_BAD_COOKIE, S_dhcpcLib_NOT_INITIALIZED, 
*        S_dhcpcLib_BAD_OPTION, S_dhcpcLib_BAD_DEVICE
* 
*/

STATUS dhcpcBind 
    (
    void * 	pCookie, 	/* identifier returned by dhcpcInit() */
    BOOL 	syncFlag 	/* synchronous or asynchronous execution */
    )
    {
    int 		offset;
    STATUS		result = OK;
    LEASE_DATA * 	pLeaseData = NULL;
    struct dhcpcOpts * 	pOptList;
    char * 		pOptions;
    struct ifreq 	ifr;

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL &&
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_BAD_COOKIE;
        return (ERROR);
        }

    if (!dhcpcInitialized || !pLeaseData->initFlag)
        {
        errno = S_dhcpcLib_NOT_INITIALIZED;
        return (ERROR);
        }

    if (syncFlag != TRUE && syncFlag != FALSE)
        {
        errno = S_dhcpcLib_BAD_OPTION;
        return (ERROR);
        }

    /* Examine type of DHCP lease to determine action required.  */

    if (pLeaseData->leaseType == DHCP_BOOTP)
        {
        return (OK);    /* BOOTP leases are always valid.  */
        }

    /*
     * Allocate space for any options in outgoing messages
     * and fill in the options field.
     */

    pOptList = pLeaseData->leaseReqSpec.pOptList;
    if (pOptList != NULL)
        {
        pOptions = malloc (pOptList->optlen);
        if (pOptions == NULL)
            {
            errno = S_dhcpcLib_BAD_OPTION;
            return (ERROR);
            }
        pLeaseData->leaseReqSpec.optlen = pOptList->optlen;

        dhcpcOptFieldCreate (pOptList, pOptions);
        free (pOptList);
        pLeaseData->leaseReqSpec.pOptList = NULL;
        pLeaseData->leaseReqSpec.pOptions = pOptions;
        }

    /* Wait for results if the startup lease is being renewed.  */

    if (pLeaseData->leaseType == DHCP_AUTOMATIC)
        pLeaseData->waitFlag = TRUE;
    else
        pLeaseData->waitFlag = syncFlag;

    if (pLeaseData->leaseType == DHCP_MANUAL && pLeaseData->leaseGood) 
        {  
        /* If redundant bind is requested, change to verification.  */

        result = dhcpcVerify (pCookie); 
        return (result);
        } 
    else	/* Obtain initial lease or renew startup lease.  */ 
        {
        /*
         * Add a filter for incoming DHCP packets and assign the selected
         * interface to the BPF device.
         */

        result = ioctl (pLeaseData->ifData.bpfDev, BIOCSETF, (int)&dhcpread);
        if (result != 0)
            {
            errno = S_dhcpcLib_BAD_DEVICE;
            return (ERROR);
            }

        bzero ( (char *)&ifr, sizeof (struct ifreq));
        sprintf (ifr.ifr_name, "%s%d", pLeaseData->ifData.iface->if_name,
                                       pLeaseData->ifData.iface->if_unit);

        result = ioctl (pLeaseData->ifData.bpfDev, BIOCSETIF, (int)&ifr);
        if (result != 0)
            {
            errno = S_dhcpcLib_BAD_DEVICE;
            return (ERROR);
            }

        dhcp_client (pCookie, TRUE);    /* Perform bind process.  */

        /* Check results of synchronous lease attempt.  */

        if (pLeaseData->waitFlag)
            {
            pLeaseData->waitFlag = FALSE;    /* Disable further signals.  */
            semTake (dhcpcMutexSem, WAIT_FOREVER);
            if (pLeaseData->leaseGood)
                result = OK;
            else
                result = ERROR;
            semGive (dhcpcMutexSem);
            }
        else
            result = OK;
        }

    /* 
     * If waitFlag was TRUE, the negotiation has completed.  Otherwise, it
     * has begun, and the installed event hook routine will be called at the
     * appropriate time.
     */

    return (result);
    }

/*******************************************************************************
*
* dhcpcVerify - renew an established lease
*
* This routine schedules the lease identified by the <pCookie> parameter
* for immediate renewal according to the process described in RFC 1541.
* If the renewal is unsuccessful, the lease negotiation process restarts.
* The routine is valid as long as the lease is currently active.  The
* routine is also called automatically in response to a dhcpcBind() call
* for an existing lease.
*
* NOTE
* This routine is only intended for active leases obtained with the
* dhcpcBind() routine. It should not be used for parameters resulting
* from the dhcpcInformGet() routine.
*
* NOTE
* This routine will disable the underlying network interface if the 
* verification fails and automatic configuration was requested.  This may
* occur without warning if no event hook is installed.
*
* RETURNS: OK if verification scheduled, or ERROR otherwise.
*
* ERRNO: S_dhcpcLib_BAD_COOKIE, S_dhcpcLib_NOT_INITIALIZED, S_dhcpcLib_NOT_BOUND
* 
*/

STATUS dhcpcVerify 
    (
    void * 	pCookie 	/* identifier returned by dhcpcInit() */
    )
    {
    int offset;
    LEASE_DATA * 	pLeaseData = NULL;
    STATUS result = OK;

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL &&
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_BAD_COOKIE;
        return (ERROR);
        }

    if (!dhcpcInitialized || !pLeaseData->initFlag)
        {
        errno = S_dhcpcLib_NOT_INITIALIZED;
        return (ERROR);
        }

    semTake (dhcpcMutexSem, WAIT_FOREVER);
    if (!pLeaseData->leaseGood)
        result = ERROR;
    semGive (dhcpcMutexSem);

    if (result == ERROR)
        {
        errno = S_dhcpcLib_NOT_BOUND;
        return (ERROR);
        }

    /* Construct and send a verification request to the client monitor task.  */

    result = dhcpcEventAdd (DHCP_USER_EVENT, DHCP_USER_VERIFY,
                            pLeaseData, FALSE);

    return (result);
    }

/*******************************************************************************
*
* dhcpcRelease - relinquish specified lease
*
* This routine schedules the lease identified by the <pCookie> parameter
* for immediate release, regardless of time remaining, and removes all
* the associated data structures.  After the release completes, a new
* call to dhcpcInit() is required before attempting another lease.
*
* NOTE
* This routine will disable the underlying network interface if automatic 
* configuration was requested.  This may occur without warning if no event 
* hook is installed.
*
* RETURNS: OK if release scheduled, or ERROR otherwise.
*
* ERRNO: S_dhcpcLib_BAD_COOKIE, S_dhcpcLib_NOT_INITIALIZED
*
*/

STATUS dhcpcRelease
    (
    void * 	pCookie 	/* identifier returned by dhcpcInit() */
    )
    {
    int offset;
    LEASE_DATA *        pLeaseData = NULL;
    STATUS result = OK;

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL &&
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_BAD_COOKIE;
        return (ERROR);
        }

    if (!dhcpcInitialized || !pLeaseData->initFlag)
        {
        errno = S_dhcpcLib_NOT_INITIALIZED;
        return (ERROR);
        }

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    /* Construct and send a release request to the client monitor task.  */

    result = dhcpcEventAdd (DHCP_USER_EVENT, DHCP_USER_RELEASE,
                            pLeaseData, FALSE);

    /* Wait for lease cleanup to complete before returning */

    if (result == OK)
        while (dhcpcLeaseList [offset] != NULL)
            taskDelay (1);	

    return (result);
    }

/*******************************************************************************
*
* dhcpcInformGet - obtain additional configuration parameters with DHCP
*
* This routine uses DHCP to retrieve additional configuration parameters for
* a client with the externally configured network address given by the
* <pAddrString> parameter. It sends an INFORM message and waits for a reply
* following the process described in RFC 2131. The <pCookie> argument contains
* the return value of an earlier dhcpcInit() call and is used to access the
* resulting configuration. Unlike the dhcpcBind() call, this routine does not
* establish a lease with a server.
*
* The <syncFlag> parameter specifies whether the message exchange started by 
* this routine will execute synchronously or asynchronously.  An asynchronous 
* execution will return after sending the initial message, but a synchronous 
* execution will only return once the process completes.
*
* When a server responds with an acknowledgement message, any event hook
* provided will be called to process the configuration parameters.  The hook
* is also called if the message exchange fails.  The results of an asynchronous
* execution are not available unless an event hook is installed.
*
* NOTE
* This routine is designed as an alternative to the dhcpcBind() routine.
* It should not be used for any dhcpcInit() identifer corresponding to an
* active or pending lease.
*
* RETURNS: OK if routine completes, or ERROR otherwise.
*
* ERRNO: S_dhcpcLib_BAD_COOKIE, S_dhcpcLib_NOT_INITIALIZED, S_dhcpcLib_BAD_OPTION
* 
*/

STATUS dhcpcInformGet
    (
    void * 	pCookie, 	/* identifier returned by dhcpcInit() */
    char * 	pAddrString, 	/* known address assigned to client */
    BOOL 	syncFlag 	/* synchronous or asynchronous execution? */
    )
    {
    int 		offset;
    STATUS		result = OK;
    LEASE_DATA * 	pLeaseData = NULL;
    struct dhcpcOpts * 	pOptList;
    char * 		pOptions;

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL &&
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_BAD_COOKIE;
        return (ERROR);
        }

    if (!dhcpcInitialized || !pLeaseData->initFlag)
        {
        errno = S_dhcpcLib_NOT_INITIALIZED;
        return (ERROR);
        }

    if (syncFlag != TRUE && syncFlag != FALSE)
        {
        errno = S_dhcpcLib_BAD_OPTION;
        return (ERROR);
        }

    /* Set the requested IP address field to the externally assigned value. */

    pLeaseData->leaseReqSpec.ipaddr.s_addr = inet_addr (pAddrString);
    if (pLeaseData->leaseReqSpec.ipaddr.s_addr == -1)
        {
        errno = S_dhcpcLib_BAD_OPTION;
        return (ERROR);
        }

    /*
     * Allocate space for any options in outgoing messages
     * and fill in the options field.
     */

    pOptList = pLeaseData->leaseReqSpec.pOptList;
    if (pOptList != NULL)
        {
        pOptions = malloc (pOptList->optlen);
        if (pOptions == NULL)
            {
            errno = S_dhcpcLib_BAD_OPTION;
            return (ERROR);
            }
        pLeaseData->leaseReqSpec.optlen = pOptList->optlen;

        dhcpcOptFieldCreate (pOptList, pOptions);
        free (pOptList);
        pLeaseData->leaseReqSpec.pOptList = NULL;
        pLeaseData->leaseReqSpec.pOptions = pOptions;
        }

    /* Start the message exchange to get additional parameters. */

    pLeaseData->waitFlag = syncFlag;

    dhcp_client (pCookie, FALSE);

    /* Wait for results of synchronous execution.  */

    if (pLeaseData->waitFlag)
        {
        pLeaseData->waitFlag = FALSE;    /* Disable further signals.  */
        semTake (dhcpcMutexSem, WAIT_FOREVER);
        if (pLeaseData->leaseGood)
            result = OK;
        else
            result = ERROR;
        semGive (dhcpcMutexSem);
        }
    else
        result = OK;

    /* 
     * If waitFlag was TRUE, the message exchange has completed.  Otherwise,
     * it has begun, and the installed event hook routine will be called at
     * the appropriate time.
     */

    return (result);
    }

/*******************************************************************************
*
* dhcpcShutdown - disable DHCP client library
*
* This routine schedules the lease monitor task to clean up memory and exit,
* after releasing all currently active leases.  The network boot device
* will be disabled if the DHCP client was used to obtain the VxWorks boot 
* parameters and the resulting lease is still active.  Any other interfaces 
* using the addressing information from leases set for automatic configuration
* will also be disabled.  Notification of a disabled interface will not occur
* unless an event hook has been installed.  After the processing started by
* this request completes, the DHCP client library is unavailable until 
* restarted with the dhcpcLibInit() routine.
*
* RETURNS: OK if shutdown scheduled, or ERROR otherwise.
*
* ERRNO: S_dhcpcLib_NOT_INITIALIZED
* 
*/

STATUS dhcpcShutdown (void)
    {
    STATUS 	result;

    if (!dhcpcInitialized)
        {
        errno = S_dhcpcLib_NOT_INITIALIZED;
        return (ERROR);
        }

    /* Construct and send a shutdown request to the client monitor task.  */

    result = dhcpcEventAdd (DHCP_USER_EVENT, DHCP_USER_SHUTDOWN, NULL, FALSE);

    if (result == OK)
        {
        /*
	 * Library shutdown pending. Disable user API routines and
	 * remove (local) signal socket indicating start of new session.
	 * Wait for cleanup to complete before indicating that shutdown is
	 * done.
	 */

        close (dhcpcSignalSock);

	while (dhcpcInitialized)
	    taskDelay (1);
        }

    return (result);
    }

/*******************************************************************************
*
* dhcpcOptionGet - retrieve an option provided to a client and store in a buffer
*
* This routine retrieves the data for a specified option from a lease
* indicated by the <pCookie> parameter. The <option> parameter specifies
* an option tag as defined in RFC 2132. See the dhcp/dhcp.h include file
* for a listing of defined aliases for the available option tags. This
* routine will not accept the following <option> values, which are either
* used by the server for control purposes or only supplied by the client:
*
*     _DHCP_PAD_TAG
*     _DHCP_REQUEST_IPADDR_TAG
*     _DHCP_OPT_OVERLOAD_TAG
*     _DHCP_MSGTYPE_TAG
*     _DHCP_REQ_LIST_TAG
*     _DHCP_MAXMSGSIZE_TAG
*     _DHCP_CLASS_ID_TAG
*     _DHCP_CLIENT_ID_TAG
*     _DHCP_END_TAG
*
* If the option is found, the data is stored in the provided buffer, up to
* the limit specified in the <pLength> parameter.  The option is not available
* if the DHCP client is not in the bound state or if the server did not
* provide it.  After returning, the <pLength> parameter indicates the amount
* of data actually retrieved. The provided buffer may contain IP addresses
* stored in network byte order. All other numeric values are stored in host
* byte order.  See RFC 2132 for specific details on the data retrieved. 
* 
* RETURNS: OK if option available, or ERROR otherwise.
*
* ERRNO: S_dhcpcLib_BAD_COOKIE, S_dhcpcLib_NOT_INITIALIZED, S_dhcpcLib_NOT_BOUND,
*  S_dhcpcLib_OPTION_NOT_PRESENT
*
* SEE ALSO
* dhcpcOptionSet()
*/

STATUS dhcpcOptionGet
    (
    void * 	pCookie, 	/* identifier returned by dhcpcInit() */
    int 	option,		/* RFC 2132 option tag */
    int * 	pLength, 	/* size of provided buffer and data returned */
    char *	pBuf 		/* location for option data */
    )
    {
    char *	pData;
    int  	amount;
    int 	limit;
    int 	offset;
    LEASE_DATA * 	pLeaseData;

    limit = *pLength;

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL && 
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_BAD_COOKIE;
        return (ERROR);
        }

    if (!dhcpcInitialized || !pLeaseData->initFlag)
        {
        errno = S_dhcpcLib_NOT_INITIALIZED;
        return (ERROR);
        }

    /* Check if client is bound.  */

    semTake (dhcpcMutexSem, WAIT_FOREVER);
    if (!pLeaseData->leaseGood)
        {
        errno = S_dhcpcLib_NOT_BOUND;
        semGive (dhcpcMutexSem);
        return (ERROR);
        }
    semGive (dhcpcMutexSem);

    /* Check if requested parameter is available.  */

    if (!ISSET (pLeaseData->dhcpcParam->got_option, option))
        {
        errno = S_dhcpcLib_OPTION_NOT_PRESENT;
        return (ERROR);
        }

    /* Find the location of option data. Ignores unsupported tag values. */

    pData = dhcpcOptionFind (pLeaseData, option, &amount);
    if (pData == NULL)
        {
        errno = S_dhcpcLib_OPTION_NOT_PRESENT;
        return (ERROR);
        }

    if (amount == 0)
        {
        /* Empty option provided by server - no data returned.  */

        *pLength = 0;
        return (OK);
        }

    if (amount < limit)         /* Adjust for size of option data.  */
        limit = amount;
 
    bcopy (pData, pBuf, limit);
    *pLength = limit;

    return (OK);
    }

/*******************************************************************************
*
* dhcpcOptionFind - find the requested option
*
* This routine returns the address and length of a requested option
* for use by dhcpcOptionGet(). It should only be called internally.
* Options which that routine does not support are ignored.
*
* RETURNS: Pointer to start of data for option, or NULL if not present.
*
* ERRNO: N/A
*
* NOMANUAL
*
* INTERNAL
* The <pAmount> parameter is dereferenced unconditionally, but since this
* routine is only called internally, it is never NULL.
*/

LOCAL char *dhcpcOptionFind
    (
    LEASE_DATA *        pLeaseData,     /* lease-specific data structures */
    int                 option,         /* RFC 1533 option tag */
    int *               pAmount         /* Number of bytes for option */
    )
    {
    char *      pData = NULL;   /* Location of option data. */
    struct dhcp_param *         pDhcpcParam;

    pDhcpcParam = pLeaseData->dhcpcParam;

    /*
     * Only retrieve supported options. Ignore the following:
     *
     *     _DHCP_PAD_TAG (0)
     *     _DHCP_REQUEST_IPADDR_TAG (50)
     *     _DHCP_OPT_OVERLOAD_TAG (52)
     *     _DHCP_MSGTYPE_TAG (53)
     *     _DHCP_REQ_LIST_TAG (55)
     *     _DHCP_MAXMSGSIZE_TAG (57)
     *     _DHCP_CLASS_ID_TAG (60)
     *     _DHCP_CLIENT_ID_TAG (61)
     *     _DHCP_END_TAG (255)
     *
     * Also ignore the two undefined values (62 and 63) and all values
     * out of the expected range from 1 through 76.
     */

    switch (option)
        {
        case _DHCP_SUBNET_MASK_TAG:
            if (pDhcpcParam->subnet_mask != NULL)
                {
                *pAmount = sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->subnet_mask;
                }
            break;

        case _DHCP_TIME_OFFSET_TAG:
            *pAmount = sizeof (long);
            pData = (char *)&pDhcpcParam->time_offset;
            break;

        case _DHCP_ROUTER_TAG:
            if (pDhcpcParam->router != NULL)
                {
                *pAmount = pDhcpcParam->router->num * sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->router->addr;
                }
            break;

        case _DHCP_TIME_SERVER_TAG:
            if (pDhcpcParam->time_server != NULL)
                {
                *pAmount = pDhcpcParam->time_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->time_server->addr;
                }
            break;

        case _DHCP_NAME_SERVER_TAG:
            if (pDhcpcParam->name_server != NULL)
                {
                *pAmount = pDhcpcParam->name_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->name_server->addr;
                }
            break;

        case _DHCP_DNS_SERVER_TAG:
            if (pDhcpcParam->dns_server != NULL)
                {
                *pAmount = pDhcpcParam->dns_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->dns_server->addr;
                }
            break;

        case _DHCP_LOG_SERVER_TAG:
            if (pDhcpcParam->log_server != NULL)
                {
                *pAmount = pDhcpcParam->log_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->log_server->addr;
                }
            break;

        case _DHCP_COOKIE_SERVER_TAG:
            if (pDhcpcParam->cookie_server != NULL)
                {
                *pAmount = pDhcpcParam->cookie_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->cookie_server->addr;
                }
            break;

        case _DHCP_LPR_SERVER_TAG:
            if (pDhcpcParam->lpr_server != NULL)
                {
                *pAmount = pDhcpcParam->lpr_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->lpr_server->addr;
                }
            break;

        case _DHCP_IMPRESS_SERVER_TAG:
            if (pDhcpcParam->impress_server != NULL)
                {
                *pAmount = pDhcpcParam->impress_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->impress_server->addr;
                }
            break;

        case _DHCP_RLS_SERVER_TAG:
            if (pDhcpcParam->rls_server != NULL)
                {
                *pAmount = pDhcpcParam->rls_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->rls_server->addr;
                }
            break;

        case _DHCP_HOSTNAME_TAG:
            *pAmount = strlen (pDhcpcParam->hostname);
            pData = pDhcpcParam->hostname;
            break;

        case _DHCP_BOOTSIZE_TAG:
            *pAmount = sizeof (unsigned short);
            pData = (char *)&pDhcpcParam->bootsize;
            break;

        case _DHCP_MERIT_DUMP_TAG:
            *pAmount = strlen (pDhcpcParam->merit_dump);
            pData = pDhcpcParam->merit_dump;
            break;

        case _DHCP_DNS_DOMAIN_TAG:
            *pAmount = strlen (pDhcpcParam->dns_domain);
            pData = pDhcpcParam->dns_domain;
            break;

        case _DHCP_SWAP_SERVER_TAG:
            if (pDhcpcParam->swap_server != NULL)
                {
                *pAmount = sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->swap_server;
                }
            break;

        case _DHCP_ROOT_PATH_TAG:
            *pAmount = strlen (pDhcpcParam->root_path);
            pData = pDhcpcParam->root_path;
            break;

        case _DHCP_EXTENSIONS_PATH_TAG:
            *pAmount = strlen (pDhcpcParam->extensions_path);
            pData = pDhcpcParam->extensions_path;
            break;

        case _DHCP_IP_FORWARD_TAG:
            *pAmount = sizeof (unsigned char);
            pData = (char *)&pDhcpcParam->ip_forward;
            break;

        case _DHCP_NONLOCAL_SRCROUTE_TAG:
            *pAmount = sizeof (unsigned char);
            pData = (char *)&pDhcpcParam->nonlocal_srcroute;
            break;

        case _DHCP_POLICY_FILTER_TAG:
            if (pDhcpcParam->policy_filter != NULL)
                {
                *pAmount = pDhcpcParam->policy_filter->num *
                           2 * sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->policy_filter->addr;
                }
            break;

        case _DHCP_MAX_DGRAM_SIZE_TAG:
            *pAmount = sizeof (unsigned short);
            pData = (char *)&pDhcpcParam->max_dgram_size;
            break;

        case _DHCP_DEFAULT_IP_TTL_TAG:
            *pAmount = sizeof (unsigned char);
            pData = (char *)&pDhcpcParam->default_ip_ttl;
            break;

        case _DHCP_MTU_AGING_TIMEOUT_TAG:
            *pAmount = sizeof (unsigned long);
            pData = (char *)&pDhcpcParam->mtu_aging_timeout;
            break;

        case _DHCP_MTU_PLATEAU_TABLE_TAG:
            if (pDhcpcParam->mtu_plateau_table != NULL)
                {
                *pAmount = pDhcpcParam->mtu_plateau_table->num *
                           sizeof (unsigned short);
                pData = (char *)pDhcpcParam->mtu_plateau_table->shortnum;
                }
            break;

        case _DHCP_IF_MTU_TAG:
            *pAmount = sizeof (unsigned short);
            pData = (char *)&pDhcpcParam->intf_mtu;
            break;

        case _DHCP_ALL_SUBNET_LOCAL_TAG:
            *pAmount = sizeof (unsigned char);
            pData = (char *)&pDhcpcParam->all_subnet_local;
            break;

        case _DHCP_BRDCAST_ADDR_TAG:
            if (pDhcpcParam->brdcast_addr != NULL)
                {
                *pAmount = sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->brdcast_addr;
                }
            break;

        case _DHCP_MASK_DISCOVER_TAG:
            *pAmount = sizeof (unsigned char);
            pData = (char *)&pDhcpcParam->mask_discover;
            break;

        case _DHCP_MASK_SUPPLIER_TAG:
            *pAmount = sizeof (unsigned char);
            pData = (char *)&pDhcpcParam->mask_supplier;
            break;

        case _DHCP_ROUTER_DISCOVER_TAG:
            *pAmount = sizeof (unsigned char);
            pData = (char *)&pDhcpcParam->router_discover;
            break;

        case _DHCP_ROUTER_SOLICIT_TAG:
            if (pDhcpcParam->router_solicit.s_addr != 0)
                {
                *pAmount = sizeof (struct in_addr);
                pData = (char *)&pDhcpcParam->router_solicit;
                }
            break;

        case _DHCP_STATIC_ROUTE_TAG:
            if (pDhcpcParam->static_route != NULL)
                {
                *pAmount = pDhcpcParam->static_route->num *
                           2 * sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->static_route->addr;
                }
            break;

        case _DHCP_TRAILER_TAG:
            *pAmount = sizeof (unsigned char);
            pData = (char *)&pDhcpcParam->trailer;
            break;

        case _DHCP_ARP_CACHE_TIMEOUT_TAG:
            *pAmount = sizeof (unsigned long);
            pData = (char *)&pDhcpcParam->arp_cache_timeout;
            break;

        case _DHCP_ETHER_ENCAP_TAG:
            *pAmount = sizeof (unsigned char);
            pData = (char *)&pDhcpcParam->ether_encap;
            break;

        case _DHCP_DEFAULT_TCP_TTL_TAG:
            *pAmount = sizeof (unsigned char);
            pData = (char *)&pDhcpcParam->default_tcp_ttl;
            break;

        case _DHCP_KEEPALIVE_INTERVAL_TAG:
            *pAmount = sizeof (unsigned long);
            pData = (char *)&pDhcpcParam->keepalive_inter;
            break;

        case _DHCP_KEEPALIVE_GARBAGE_TAG:
            *pAmount = sizeof (unsigned char);
            pData = (char *)&pDhcpcParam->keepalive_garba;
            break;

        case _DHCP_NIS_DOMAIN_TAG:
            *pAmount = strlen (pDhcpcParam->nis_domain);
            pData = pDhcpcParam->nis_domain;
            break;

        case _DHCP_NIS_SERVER_TAG:
            if (pDhcpcParam->nis_server != NULL)
                {
                *pAmount = pDhcpcParam->nis_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->nis_server->addr;
                }
            break;

        case _DHCP_NTP_SERVER_TAG:
            if (pDhcpcParam->ntp_server != NULL)
                {
                *pAmount = pDhcpcParam->ntp_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->ntp_server->addr;
                }
            break;

        case _DHCP_VENDOR_SPEC_TAG:
            if (pDhcpcParam->vendlist != NULL)
                {
                *pAmount = pDhcpcParam->vendlist->len;
                pData = pDhcpcParam->vendlist->list;
                }
            break;

        case _DHCP_NBN_SERVER_TAG:
            if (pDhcpcParam->nbn_server != NULL)
                {
                *pAmount = pDhcpcParam->nbn_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->nbn_server->addr;
                }
            break;

        case _DHCP_NBDD_SERVER_TAG:
            if (pDhcpcParam->nbdd_server != NULL)
                {
                *pAmount = pDhcpcParam->nbdd_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->nbdd_server->addr;
                }
            break;

        case _DHCP_NB_NODETYPE_TAG:
            *pAmount = sizeof (unsigned char);
            pData = (char *)&pDhcpcParam->nb_nodetype;
            break;

        case _DHCP_NB_SCOPE_TAG:
            *pAmount = strlen (pDhcpcParam->nb_scope);
            pData = pDhcpcParam->nb_scope;
            break;

        case _DHCP_XFONT_SERVER_TAG:
            if (pDhcpcParam->xfont_server != NULL)
                {
                *pAmount = pDhcpcParam->xfont_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->xfont_server->addr;
                }
            break;

        case _DHCP_XDISPLAY_MANAGER_TAG:
            if (pDhcpcParam->xdisplay_manager != NULL)
                {
                *pAmount = pDhcpcParam->xdisplay_manager->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->xdisplay_manager->addr;
                }
            break;

        case _DHCP_LEASE_TIME_TAG:
            *pAmount = sizeof (unsigned long);
            pData = (char *)&pDhcpcParam->lease_duration;
            break;

        case _DHCP_SERVER_ID_TAG:
            *pAmount = sizeof (struct in_addr);
            pData = (char *)&pDhcpcParam->server_id;
            break;

        case _DHCP_ERRMSG_TAG:
            *pAmount = strlen (pDhcpcParam->errmsg);
            pData = pDhcpcParam->errmsg;
            break;

        case _DHCP_T1_TAG:
            *pAmount = sizeof (unsigned long);
            pData = (char *)&pDhcpcParam->dhcp_t1;
            break;

        case _DHCP_T2_TAG:
            *pAmount = sizeof (unsigned long);
            pData = (char *)&pDhcpcParam->dhcp_t2;
            break;

        case _DHCP_NISP_DOMAIN_TAG:
            *pAmount = strlen (pDhcpcParam->nisp_domain);
            pData = pDhcpcParam->nisp_domain;
            break;

        case _DHCP_NISP_SERVER_TAG:
            if (pDhcpcParam->nisp_server != NULL)
                {
                *pAmount = pDhcpcParam->nisp_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->nisp_server->addr;
                }
            break;
      
	case _DHCP_TFTP_SERVERNAME_TAG:
            *pAmount = sizeof (struct in_addr);
            pData = (char *)&pDhcpcParam->siaddr;
	    break;

	case _DHCP_BOOTFILE_TAG:
	    *pAmount = strlen (pDhcpcParam->file);
	    pData = pDhcpcParam->file;
	    break;

        case _DHCP_MOBILEIP_HA_TAG:
            if (pDhcpcParam->mobileip_ha != NULL)
                {
                *pAmount = pDhcpcParam->mobileip_ha->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->mobileip_ha->addr;
                }
            break;

        case _DHCP_SMTP_SERVER_TAG:
            if (pDhcpcParam->smtp_server != NULL)
                {
                *pAmount = pDhcpcParam->smtp_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->smtp_server->addr;
                }
            break;

        case _DHCP_POP3_SERVER_TAG:
            if (pDhcpcParam->pop3_server != NULL)
                {
                *pAmount = pDhcpcParam->pop3_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->pop3_server->addr;
                }
            break;

        case _DHCP_NNTP_SERVER_TAG:
            if (pDhcpcParam->nntp_server != NULL)
                {
                *pAmount = pDhcpcParam->nntp_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->nntp_server->addr;
                }
            break;

        case _DHCP_DFLT_WWW_SERVER_TAG:
            if (pDhcpcParam->dflt_www_server != NULL)
                {
                *pAmount = pDhcpcParam->dflt_www_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->dflt_www_server->addr;
                }
            break;

        case _DHCP_DFLT_FINGER_SERVER_TAG:
            if (pDhcpcParam->dflt_finger_server != NULL)
                {
                *pAmount = pDhcpcParam->dflt_finger_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->dflt_finger_server->addr;
                }
            break;

        case _DHCP_DFLT_IRC_SERVER_TAG:
            if (pDhcpcParam->dflt_irc_server != NULL)
                {
                *pAmount = pDhcpcParam->dflt_irc_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->dflt_irc_server->addr;
                }
            break;

        case _DHCP_STREETTALK_SERVER_TAG:
            if (pDhcpcParam->streettalk_server != NULL)
                {
                *pAmount = pDhcpcParam->streettalk_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->streettalk_server->addr;
                }
            break;

        case _DHCP_STDA_SERVER_TAG:
            if (pDhcpcParam->stda_server != NULL)
                {
                *pAmount = pDhcpcParam->stda_server->num *
                           sizeof (struct in_addr);
                pData = (char *)pDhcpcParam->stda_server->addr;
                }
            break;

        default:
            break;
        }
    return (pData);
    }

/*******************************************************************************
*
* dhcpcServerGet - retrieve the current DHCP server
*
* This routine returns the DHCP server that supplied the configuration
* parameters for the lease specified by the <pCookie> argument.  This 
* information is available only if the lease is in the bound state.
*
* RETURNS: OK if in bound state and server available, or ERROR otherwise.
*
* ERRNO: S_dhcpcLib_BAD_COOKIE, S_dhcpcLib_NOT_INITIALIZED, S_dhcpcLib_NOT_BOUND
*/

STATUS dhcpcServerGet
    (
    void * 		pCookie, 	/* identifier returned by dhcpcInit() */
    struct in_addr * 	pServerAddr 	/* location for address of server */
    )
    {
    int offset;
    LEASE_DATA * 	pLeaseData;
    STATUS 		result = OK;

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL &&
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_BAD_COOKIE;
        return (ERROR);
        }

    if (!dhcpcInitialized || !pLeaseData->initFlag)
        {
        errno = S_dhcpcLib_NOT_INITIALIZED;
        return (ERROR);
        }

    semTake (dhcpcMutexSem, WAIT_FOREVER);
    if (!pLeaseData->leaseGood)
        result = ERROR;
    semGive (dhcpcMutexSem);

    if (result == ERROR)
        {
        errno = S_dhcpcLib_NOT_BOUND;
        return (ERROR);
        }

    if (pLeaseData->dhcpcParam->server_id.s_addr != 0)
        bcopy ( (char *)&pLeaseData->dhcpcParam->server_id, 
               (char *)pServerAddr, sizeof (struct in_addr));
    else 
        result = ERROR;

    return (result);
    }

/*******************************************************************************
*
* dhcpcTimerGet - retrieve current lease timers
*
* This routine returns the number of clock ticks remaining on the timers
* governing the DHCP lease specified by the <pCookie> argument.  This 
* information is only available if the lease is in the bound state.
* Therefore, this routine will return ERROR if a BOOTP reply was accepted.
*
* RETURNS: OK if in bound state and values available, or ERROR otherwise.
*
* ERRNO: S_dhcpcLib_BAD_COOKIE, S_dhcpcLib_NOT_INITIALIZED, S_dhcpcLib_NOT_BOUND, 
*  S_dhcpcLib_OPTION_NOT_PRESENT, S_dhcpcLib_TIMER_ERROR
*/

STATUS dhcpcTimerGet
    (
    void * 	pCookie, 	/* identifier returned by dhcpcInit() */
    int * 	pT1, 		/* time until lease renewal */
    int * 	pT2 		/* time until lease rebinding */
    )
    {
    int 	offset;
    time_t	current = 0;
    long	timer1;
    long	timer2;
    LEASE_DATA *        pLeaseData;
    STATUS 		result = OK;

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL &&
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_BAD_COOKIE;
        return (ERROR);
        }

    if (!dhcpcInitialized || !pLeaseData->initFlag)
        {
        errno = S_dhcpcLib_NOT_INITIALIZED;
        return (ERROR);
        }

    if (pLeaseData->leaseType == DHCP_BOOTP)
        {
#ifdef DHCPC_DEBUG
        logMsg ("No timer values: BOOTP reply accepted.\n", 0, 0, 0, 0, 0, 0);
#endif
        errno = S_dhcpcLib_OPTION_NOT_PRESENT;
        return (ERROR);
        }

    if (dhcpTime (&current) == -1)
        {
#ifdef DHCPC_DEBUG
        logMsg ("time() error in dhcpTimerGet().\n", 0, 0, 0, 0, 0, 0);
#endif
        errno = S_dhcpcLib_TIMER_ERROR;
        return (ERROR);
        }

    semTake (dhcpcMutexSem, WAIT_FOREVER);
    if (!pLeaseData->leaseGood)
        result = ERROR;
    semGive (dhcpcMutexSem);

    if (result == ERROR)
        {
        errno = S_dhcpcLib_NOT_BOUND;
        return (ERROR);
        }

    timer1 = pLeaseData->dhcpcParam->lease_origin + 
                 pLeaseData->dhcpcParam->dhcp_t1 - current;
    timer2 = pLeaseData->dhcpcParam->lease_origin + 
                 pLeaseData->dhcpcParam->dhcp_t2 - current;

    if (timer1 < 0)
        timer1 = 0;   
    if (timer2 < 0)
        timer2 = 0;

    *pT1 = timer1 * sysClkRateGet();
    *pT2 = timer2 * sysClkRateGet();

    return (OK);
    }

/*******************************************************************************
*
* dhcpcParamsGet - retrieve current configuration parameters
*
* This routine copies the current configuration parameters for the lease
* specified by the <pCookie> argument to the user-supplied and allocated 
* 'dhcp_param' structure referenced in <pParamList>.  Within this structure,  
* defined in 'h/dhcp/dhcpc.h', you should supply buffer pointers for the 
* parameters that interest you.  Set all other structure members to zero.  
* When dhcpcParamsGet() returns, the buffers you specified in the submitted 
* 'dhcpc_param' structure will contain the information you requested.  
* This assumes that the specified lease is in the bound state and that 
* DHCP knows that the lease parameters are good.
*
* NOTE: The 'temp_sname' and 'temp_file' members of the 'dhcp_param' 
* structure are for internal use only.  They reference temporary buffers for 
* options that are passed using the 'sname' and 'file' members.  Do not 
* request either 'temp_sname' or 'temp_file'.  Instead, request either 
* 'sname' or 'file' if you want those parameters.  
*
* Many of the parameters within the user-supplied structure use one of the 
* following secondary data types: 'struct in_addrs', 'struct u_shorts', and 
* 'struct vendor_list'.  Each of those structures accepts a length designation 
* and a data pointer.  For the first two data types, the 'num' member indicates 
* the size of the buffer in terms of the number of underlying elements.  For 
* example, the STATIC_ROUTE option returns one or more IP address pairs.  Thus, 
* setting the 'num' member to 2 in the 'static_route' entry would indicate that 
* the corresponding buffer contained 16 bytes.  By contrast, the 'len' member 
* in the struct 'vendor_list' data type consists of the buffer size, in bytes.
* See RFC 1533 for specific details on the types of data for each option. 
* 
* On return, each of the length designators are set to indicate the amount of
* data returned.  For instance, the 'num' member in the static_route entry could
* be set to 1 to indicate that only one IP address pair of 8 bytes was 
* available.
*
* RETURNS: OK if in bound state, or ERROR otherwise.
*
* ERRNO: S_dhcpcLib_BAD_COOKIE, S_dhcpcLib_NOT_INITIALIZED, S_dhcpcLib_NOT_BOUND
*
*/

STATUS dhcpcParamsGet
    (
    void * 	pCookie, 	/* identifier returned by dhcpcInit() */
    struct dhcp_param *	pParamList	/* requested parameters */
    )
    {
    int offset;
    LEASE_DATA *        pLeaseData;
    STATUS 		result = OK;

    /*
     * Use the cookie to access the lease-specific data structures.  For now,
     * just typecast the cookie.  This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL &&
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        {
        errno = S_dhcpcLib_BAD_COOKIE;
        return (ERROR);
        }

    if (!dhcpcInitialized || !pLeaseData->initFlag)
        {
        errno = S_dhcpcLib_NOT_INITIALIZED;
        return (ERROR);
        }

    semTake (dhcpcMutexSem, WAIT_FOREVER);
    if (!pLeaseData->leaseGood)
        result = ERROR;
    semGive (dhcpcMutexSem);

    if (result == ERROR)
        {
        errno = S_dhcpcLib_NOT_BOUND;
        return (ERROR);
        }

    dhcpcParamsCopy (pLeaseData, pParamList);

    return (OK);
    }

/*******************************************************************************
*
* dhcpcState - monitor all DHCP client activity
*
* This routine establishes and monitors all runtime DHCP leases. It is not
* used by the DHCP client when executing at boot time. The routine receives
* all DHCP event notifications (message arrivals, timeouts, and user requests),
* and controls the event processing by invoking the appropriate routines.
* It is the entry point for the monitor task created during the library
* initialization and should only be called internally.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL void dhcpcState (void)
    {
    int offset;
    EVENT_DATA  newEvent;
    STATUS      result;

    FOREVER
        {
        /* Wait for an incoming DHCP message, user request, or timeout. */

        result = dhcpcEventGet (&newEvent);
        if (result == ERROR)
            {
            /* Invalid or missing event - ignore. */

            continue;
            }

        /* Handle dhcpcShutdown() requests. */

        if (newEvent.source == DHCP_USER_EVENT &&
                newEvent.type == DHCP_USER_SHUTDOWN)
            {
            /* Release all active leases, free all memory, and exit. */

            newEvent.type = DHCP_USER_RELEASE;

            for (offset = 0; offset < dhcpcMaxLeases; offset++)
                {
                if (dhcpcLeaseList [offset] != NULL)
                    {
                    newEvent.leaseId = dhcpcLeaseList [offset];
                    dhcpcEventHandle (&newEvent);
                    dhcpcLeaseList [offset] = NULL;
                    }
                }
            dhcpcCleanup ();

            break;
            }

        /* Process all other events in the context of the target lease. */

        result = dhcpcEventHandle (&newEvent);
        if (result == DHCPC_DONE)
            {
            /* Set the list entry to NULL when a lease is removed. */

            for (offset = 0; offset < dhcpcMaxLeases; offset++)
                {
                if (dhcpcLeaseList [offset] == newEvent.leaseId)
                    {
                    dhcpcLeaseList [offset] = NULL;
                    break;
                    }
                }
            }
        }

    /* The monitor task only returns in response to a dhcpcShutdown() call. */

    return;
    }

/*******************************************************************************
*
* dhcpcEventGet - wait for a DHCP event
*
* This routine retrieves DHCP events for processing by the monitor task.
* If the contents of the event descriptor are valid, it is stored in the
* buffer provided by the <pNewEvent> parameter. Automatic event descriptors
* are created when a DHCP message arrives or a timeout expires. Manual
* event descriptors are generated by calls to the dhcpBind(), dhcpVerify(),
* dhcpRelease() and dhcpShutdown() routines.
*
* RETURNS: OK if valid event retrieved, or ERROR otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL STATUS dhcpcEventGet
    (
    EVENT_DATA *        pNewEvent       /* pointer to event descriptor */
    )
    {
    STATUS      result;
    int         status;
    int         offset;
    LEASE_DATA *        pLeaseData;

    /* Wait for event occurrence. */

    semTake (dhcpcEventSem, WAIT_FOREVER);

    /* Retrieve event from message ring. */

    if (rngIsEmpty (dhcpcEventRing) == TRUE)
        {
#ifdef DHCPC_DEBUG
        logMsg ("dhcpcEventGet: Notification empty.\n", 0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }

    status = rngBufGet (dhcpcEventRing, (char *)pNewEvent, sizeof (EVENT_DATA));

    if (status != sizeof (EVENT_DATA))
        {
#ifdef DHCPC_DEBUG
        logMsg ("dhcpcEventGet: Notification error.\n", 0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }

    /*
     * Check event descriptor for valid data. The primary purpose of
     * this code is to catalog all the possible event descriptors
     * in one location, since invalid data could only result from a failure
     * in memory-to-memory copies by the ring buffers, which is extremely
     * unlikely. Even if that occurred, the DHCP protocol is designed to
     * handle any errors in message arrival notifications as a byproduct of
     * UDP processing, but corruption of timeout or user events could result
     * in unrecoverable errors.
     */

    result = OK;
    switch (pNewEvent->source)
        {
        case DHCP_AUTO_EVENT:   /* Validate automatic event types. */
            if (pNewEvent->type != DHCP_TIMEOUT &&
                    pNewEvent->type != DHCP_MSG_ARRIVED)
                {
                result = ERROR;
                }
            break;
        case DHCP_USER_EVENT:    /* Validate manual event types. */
            if (pNewEvent->type != DHCP_USER_INFORM &&
                    pNewEvent->type != DHCP_USER_BIND &&
                    pNewEvent->type != DHCP_USER_VERIFY &&
                    pNewEvent->type != DHCP_USER_RELEASE &&
                    pNewEvent->type != DHCP_USER_SHUTDOWN)
                {
                result = ERROR;
                }
            break;
        default:    /* Unknown event class. */
            result = ERROR;
            break;
        }

    if (result == ERROR)
        return (ERROR);

    /* Lease identifiers must be checked for all events except shutdown. */

    if (pNewEvent->source == DHCP_USER_EVENT &&
            pNewEvent->type == DHCP_USER_SHUTDOWN)
        return (OK);

    /*
     * Although not likely, a lease could be released between the
     * arrival of an event and the event processing. In that case,
     * the recorded lease identifier will be invalid. Ignore those events.
     */

    pLeaseData = pNewEvent->leaseId;

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if (dhcpcLeaseList [offset] != NULL &&
                dhcpcLeaseList [offset] == pLeaseData)
            break;

    if (offset == dhcpcMaxLeases)
        return (ERROR);

    return (result);
    }

/*******************************************************************************
*
* dhcpcEventHandle - process a DHCP event for a particular lease
*
* This routine executes a portion of the DHCP client finite state machine
* until a DHCP message is sent and/or the relevant timeouts are set. It
* processes all incoming DHCP messages, timeouts, and all user requests
* except for the dhcpcShutdown() routine. All handled events are processed
* in the context of the current state of a known lease. It is invoked by the
* monitor task when the events occur, and should only be called internally.
*
* RETURNS: OK if processing completed, or ERROR if it fails.
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL STATUS dhcpcEventHandle
    (
    EVENT_DATA *        pNewEvent       /* pointer to event descriptor */
    )
    {
    STATUS              result;
    LEASE_DATA *        pLeaseData;

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pNewEvent->leaseId;

    /*
     * For new messages, set the data pointer for the lease to access
     * the next DHCP message in the data buffer filled by the read task.
     */

    if (pNewEvent->source == DHCP_AUTO_EVENT &&
            pNewEvent->type == DHCP_MSG_ARRIVED)
        {
        pLeaseData->msgBuffer = pNewEvent->pMsg;
        }

    /*
     * Execute routines from the state machine until processing is complete.
     * In general, all processing is performed by a single routine. If
     * additional routines are needed, the called routine returns DHCPC_MORE,
     * and the next routine is executed immediately. Processing always stops
     * once the occurrence of a subsequent timeout or message arrival is
     * guaranteed. The processing routine returns OK at that point, allowing
     * the monitor task to handle the next event. If a routine returns
     * ERROR, the state machine for the lease is reset to the initial state.
     * Finally, a routine returns DHCPC_DONE after releasing its lease and
     * removing the data structures.
     */

    result = DHCPC_MORE;
    while (result == DHCPC_MORE)
        {
        result = (*fsm [pLeaseData->currState]) (pNewEvent);

        /*
         * Reset any lease message pointer to prevent further access to the
         * data buffer. Mark that buffer available after handling the last
         * DHCP message.
         */

        if (pNewEvent->source == DHCP_AUTO_EVENT &&
                pNewEvent->type == DHCP_MSG_ARRIVED)
            {
            pLeaseData->msgBuffer = NULL;

            if (pNewEvent->lastFlag)
                dhcpcMessageList [pNewEvent->slot].writeFlag = TRUE;
            }

        if (result == ERROR)
            {
#ifdef DHCPC_DEBUG
            logMsg ("Error in finite state machine.\n", 0, 0, 0, 0, 0, 0);
#endif

            if (pLeaseData->prevState == INFORMING)
                {
                /*
                 * Signal the (failed) completion of the message exchange
                 * if the dhcpcInformGet() routine is executing synchronously.
                 * No additional parameters are available.
                 */

                if (pLeaseData->waitFlag)
                    semGive (pLeaseData->leaseSem);

                /* Send a failure notification to any event hook routine. */

                if (pLeaseData->eventHookRtn != NULL)
                    result = (* pLeaseData->eventHookRtn) (DHCPC_LEASE_INVALID,
                                                           pNewEvent->leaseId);

                return (ERROR);
                }

            /* Lease negotiation failed - set to execute init() routine. */

            pLeaseData->prevState = DHCPC_ERROR;
            pLeaseData->currState = INIT;

            /* Disable the underlying network interface if necessary. */

            if (pLeaseData->autoConfig ||
                    pLeaseData->leaseType == DHCP_AUTOMATIC)
                {
                down_if (&pLeaseData->ifData);
                }

            /*
             * Signal the (failed) completion of the negotiation process
             * if the dhcpcBind() call is executing synchronously.
             */

            if (pLeaseData->waitFlag)
                semGive (pLeaseData->leaseSem);

            /* Send a notification of the failure to any event hook routine. */

            if (pLeaseData->eventHookRtn != NULL)
                result = (* pLeaseData->eventHookRtn) (DHCPC_LEASE_INVALID,
                                                       pNewEvent->leaseId);
            return (ERROR);
            }

        /*
         * When a lease entry is removed, return an indicator to the monitor
         * task so that is will reset the corresponding list entry to NULL.
         */

        if (result == DHCPC_DONE)
            return (DHCPC_DONE);

        if (pLeaseData->currState == BOUND && pLeaseData->prevState != BOUND)
            {
            /* Suspend filtering with the BPF device until timers expire. */

            ioctl (pLeaseData->ifData.bpfDev, BIOCSTOP, 0);

            /*
             * Signal the successful completion of the negotiation
             * process if the dhcpBind() call is executing synchronously.
             */

            if (pLeaseData->waitFlag)
                semGive (pLeaseData->leaseSem);

            /* Set the bound routine to exit after starting the timer. */

            pNewEvent->type = DHCPC_STATE_BEGIN;
            }
#ifdef DHCPC_DEBUG
        logMsg ("Next state= %d\n", pLeaseData->currState, 0, 0, 0, 0, 0);
#endif
        }
    return (OK);
    }

/******************************************************************************
*
* dhcpcLeaseCleanup - remove data structures used by a DHCP lease
*
* This routine removes all lease-specific data structures accessed by the
* <pLeaseData> parameter. It is called internally when a lease is relinquished
* by the user with the dhcpcRelease() routine and when the DHCP client
* library is disabled with dhcpcShutdown().
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void dhcpcLeaseCleanup
    (
    LEASE_DATA *        pLeaseData      /* lease-specific status information */
    )
    {
    int offset;

    wdCancel (pLeaseData->timer);
    wdDelete (pLeaseData->timer);

    close (pLeaseData->ifData.bpfDev);

    semDelete (pLeaseData->leaseSem);

    if (pLeaseData->leaseReqSpec.clid != NULL)
        {
        if (pLeaseData->leaseReqSpec.clid->id != NULL)
            free (pLeaseData->leaseReqSpec.clid->id);
        free (pLeaseData->leaseReqSpec.clid);
        }

    /* Remove custom options field, if any. */

    if (pLeaseData->leaseReqSpec.pOptions != NULL)
        free (pLeaseData->leaseReqSpec.pOptions);

    clean_param (pLeaseData->dhcpcParam);
    if (pLeaseData->dhcpcParam != NULL)
        free (pLeaseData->dhcpcParam);

    /* Clear the slot in the global list so it can be re-used. */

    for (offset = 0; offset < dhcpcMaxLeases; offset++)
        if ( dhcpcLeaseList [offset] == pLeaseData )
            dhcpcLeaseList [offset] = NULL;

    free (pLeaseData);

    return;
    }

/******************************************************************************
*
* dhcpcCleanup - remove data structures used by DHCP client library
*
* This routine removes all data structures used by the dhcp client after
* all the lease-specific data structures have been removed. It is called
* in response to a dhcpcShutdown() request just before the client monitor
* task exits.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL void dhcpcCleanup (void)
    {
    int offset;

    taskDelete (dhcpcReadTaskId);

    close (dhcpcDataSock);

    bpfDevDelete ("/bpf/dhcpc");

    free (dhcpcLeaseList);
 
    semDelete (dhcpcMutexSem);
    semDelete (dhcpcEventSem);
 
    rngDelete (dhcpcEventRing);

    for (offset = 0; offset < 10; offset++)
         free (dhcpcMessageList [offset].msgBuffer);

    free (dhcpcMessageList);

    dhcpcPrivateCleanup();    /* Remove data allocated in other libraries. */

    offset = bpfDrvRemove ();

#ifdef DHCPC_DEBUG
    if (offset == OK)
        logMsg ("dhcpcCleanup: successfully removed BPF driver.\n",
                0, 0, 0, 0, 0, 0);
    else
        logMsg ("dhcpcCleanup: unable to remove BPF driver (other users?).\n",
                0, 0, 0, 0, 0, 0);
#endif

    dhcpcInitialized = FALSE;
    return;
    }

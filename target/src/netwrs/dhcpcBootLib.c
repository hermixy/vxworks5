/* dhcpcBootLib.c - DHCP boot-time client library */

/* Copyright 1984 - 2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01x,27mar02,wap  make dhcpcBootParamsGet() return error if dhcpcBootParam
                 hasn't been initialized (SPR #73147)
01w,04dec01,wap  initialize dhcpcLeaseList correctly to avoid writing
                 to page 0 (NULL pointer dereference) (SPR #69523)
01v,17nov00,spm  added support for BSD Ethernet devices
01u,15nov00,spm  fixed incorrect pointer assignment in lease list (SPR #62274)
01t,24oct00,spm  fixed modification history after merge from tor3_0_x branch
01s,23oct00,niq  merged from version 01t of tor3_0_x branch (base version 01r);
                 upgrade to BPF replaces tagged frames support
01r,16mar99,spm  recovered orphaned code from tor2_0_x branch (SPR #25770)
01q,16mar99,spm  recovered orphaned code from tor1_0_1.sens1_1 (SPR #25770)
01p,01dec98,spm  added missing memory allocation (SPR #22881)
01p,06oct98,spm  fixed copying of parameters with IP address pairs (SPR #22416)
01o,14dec97,jdi  doc: cleanup.
01n,dec1097,kbw  fiddled manpage format
01m,04dec97,spm  added code review modifications
01l,06oct97,spm  removed reference to deleted endDriver global; replaced with
                 support for dynamic driver type detection
01k,25sep97,spm  restored man page for dhcpcBootOptionSet() routine
01j,25sep97,gnn  SENS beta feedback fixes
01i,02sep97,spm  removed name conflicts with runtime DHCP client (SPR #9241)
01h,26aug97,spm  major overhaul: included version 01j of dhcpcCommonLib.c
                 to retain single-lease library for boot time use
01g,12aug97,gnn  changes necessitated by MUX/END update.
01f,02jun97,spm  changed DHCP option tags to prevent name conflicts (SPR #8667)
                 and updated man pages
01e,18apr97,spm  added call to dhcp_boot_client to reduce boot ROM size
01d,15apr97,kbw  fixed manpage format
01c,07apr97,spm  altered to use current value of configAll.h defines,
                 rewrote documentation
01b,29jan97,spm  added END driver support and modified to fit coding standards
01a,14nov96,spm  created by removing unneeded functions from dhcpcLib.c
*/

/*
DESCRIPTION
This library contains the interface for the client side of the Dynamic Host
Configuration Protocol (DHCP) used during system boot. DHCP is an extension
of BOOTP, the bootstrap protocol.  Like BOOTP, the protocol allows automatic
system startup by providing an IP address, boot file name, and boot host's
IP address over a network.  Additionally, DHCP provides the complete set of
configuration parameters defined in the Host Requirements RFCs and allows
automatic reuse of network addresses by specifying a lease duration for a
set of configuration parameters.  This library is linked into the boot ROM 
image automatically if INCLUDE_DHCPC is defined at the time that image is 
constructed.

HIGH-LEVEL INTERFACE
The VxWorks boot program uses this library to obtain configuration parameters 
with DHCP according to the client-server interaction detailed in RFC 2131 using
the boot device specified in the boot parameters.  The DHCP client supports
devices attached to the IP protocol with the MUX/END interface. It also
supports BSD Ethernet devices attached to the IP protocol.

To use DHCP, first build a boot ROM image with INCLUDE_DHCPC defined and set
 the appropriate flag in the boot parameters before initiating booting with
the "@" command.  The DHCP client will attempt to retrieve entries for the
boot file name, and host IP address, as well as a subnet mask and broadcast
address for the boot device.  If a target IP address is not available, the
client will retrieve those parameters in the context of a lease. Otherwise,
it will search for permanent assignments using a simpler message exchange.
Any entries retrieved with either method will only be used if the corresponding
fields in the boot parameters are blank.

NOTE
After DHCP retrieves the boot parameters, the specified boot file is loaded
and the system restarts. As a result, the boot-time DHCP client cannot renew
any lease which may be associated with the assigned IP address.  To avoid
potential IP address conflicts while loading the boot file, the DHCPC_MIN_LEASE
value should be set to exceed the file transfer time.  In addition, the boot
file must also contain the DHCP client library so that the lease obtained
before the restart can be renewed. Otherwise, the network initialization using
the boot parameters will fail.  These restrictions do not apply if the target
IP address is entered manually since the boot parameters do not involve a
lease in that case.

INTERNAL
This module was created to reduce the size of the boot ROM image so that
the DHCP client could be used with target architectures such as the MV147
which have limited ROM capacity. The routines in this library are a modified
version of similar routines available to the user in the dhcpcLib library. 
These routines are only intended for use during boot time by the VxWorks 
system. Among other differences, they do not support multiple leases and 
they always apply configuration parameters to the underlying network device. 
They should only be called by the boot program.

INCLUDE FILES: dhcpcBootLib.h

SEE ALSO: dhcpcLib, RFC 1541, RFC 1533
*/

/* includes */

#include "dhcp/copyright_dhcp.h"
#include "vxWorks.h"
#include "wdLib.h"
#include "semLib.h"
#include "ioLib.h" 	/* ioctl() declaration */
#include "taskLib.h" 	/* taskSpawn() declaration */
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
#include "dhcpcBootLib.h"

#include "bpfDrv.h"

#include "end.h"
#include "ipProto.h"

/* defines */

#define _BYTESPERWORD 		4 	/* Conversion factor for IP header */
#define _DHCP_MAX_OPTLEN 	255 	/* Max. number of bytes in an option */
#define _IPADDRLEN 		4 	/* Length of address in an ARP reply */
#define _ETHER 			1 	/* Hardware type for Ethernet */
                                        /* from assigned numbers RFC */
#define _ETHERADDRLEN 		6 	/* Length of an Ethernet address */

     /* Host requirements documents default values. */

#define _HRD_MAX_DGRAM 	576 	/* Default maximum datagram size. */
#define _HRD_IP_TTL 	64 	/* Default IP time-to-live (seconds) */
#define _HRD_MTU 	576 	/* Default interface MTU */
#define _HRD_ROUTER 	0xffffffff /* Default router solication */
                                   /* address - 255.255.255.255 */
#define _HRD_ARP_TIME 	60 	/* Default ARP cache timeout (seconds) */
#define _HRD_TCP_TTL 	64 	/* Default TCP time-to-live (seconds) */
#define _HRD_TCP_TIME 	7200 	/* Default TCP keepalive interval (seconds) */

#define _DHCPC_MAX_DEVNAME      21      /* "/bpf/dhcpc" + max unit number */

IMPORT struct bpf_insn dhcpfilter[];    /* Needed to update client port. */
IMPORT struct bpf_program dhcpread;     /* Installs filter for DHCP messages */
IMPORT int dhcpcBufSize;
IMPORT BOOL _dhcpcBootFlag;

/* globals */

LEASE_DATA      dhcpcBootLeaseData;
LEASE_DATA * 	pDhcpcBootLeaseData;
int 		dhcpcBindType; 		/* Type of DHCP lease, if any. */

/* locals */

LOCAL int dhcpcOfferTimeout; 	/* Set externally when building image */
LOCAL int dhcpcDefaultLease; 	/* Set externally when building image */
LOCAL int dhcpcClientPort; 	/* Set externally when building image */
LOCAL int dhcpcServerPort; 	/* Set externally when building image */

/* forward declarations */

IMPORT void dhcpcRead (void); 	/* Retrieve incoming DHCP messages */
LOCAL void dhcpcBootCleanup (void);

/*******************************************************************************
*
* dhcpcBootInit - set up the DHCP client parameters and data structures
*
* This routine creates any necessary data structures and sets the client's 
* option request list to retrieve a subnet mask and broadcast address for the 
* network interface indicated by <pIf>.  The routine is executed automatically 
* by the boot program when INCLUDE_DHCPC is defined and the automatic 
* configuration option is set in the boot flags.  The network interface 
* specified by <pIf> is used to transmit and receive all DHCP messages during 
* the lease negotiation.  The DHCP client supports interfaces attached to the
* IP protocol using the MUX/END interface and BSD Ethernet devices attached
* to that protocol. The interface must be capable of sending broadcast
* messages. The <maxSize> parameter specifies the maximum length supported for
* any DHCP message, including the UDP and IP headers and the link level header.
* The maximum length of the DHCP options field is based on this value or the
* MTU size for the given interface, whichever is less. The smallest valid value
* for the <maxSize> parameter is 576 bytes, corresponding to the minimum IP
* datagram a host must accept. The MTU size of the network interface must be
* large enough to handle those datagrams.
*
* ERRNO: N/A
* 
* RETURNS: Lease handle for later use, or NULL if lease startup fails.
*/

void * dhcpcBootInit
    (
    struct ifnet * pIf, 	/* network device used by client */
    int 	serverPort,     /* port used by DHCP servers (default 67) */
    int 	clientPort,     /* port used by DHCP clients (default 68) */
    int 	maxSize, 	/* largest DHCP message supported, in bytes */
    int 	offerTimeout,   /* interval to get additional DHCP offers */
    int 	defaultLease,   /* default value for requested lease length */
    int 	minLease        /* minimum accepted lease length */
    )
    {
    struct dhcp_reqspec * 	pReqSpec;
    LEASE_DATA * 		pBootLeaseData;

    BOOL bsdFlag = FALSE;

    int loop;
    int offset = 0;
    int bufSize; 	/* Receive buffer size (maxSize or MTU + BPF header) */

    char bpfDevName [_DHCPC_MAX_DEVNAME];  /* "/bpf/dhcpc" + max unit number */
    int bpfDev;

    int result;

    dhcpcServerPort = serverPort;
    dhcpcClientPort = clientPort;

    if (pIf == NULL)
        {
        printf ("Error: network device not specified.\n");
        return (NULL);
        }

    if (muxDevExists (pIf->if_name, pIf->if_unit) == FALSE)
        {
        bsdFlag = TRUE; 
        }

    /* Verify interface data sizes are appropriate for message. */

    bufSize = pIf->if_mtu;
    if (bufSize == 0)
        {
        printf ("Error: unusable network device.\n");
        return (NULL);
        }

    if (bufSize < DHCP_MSG_SIZE)
        {
        /*
         * Devices must accept messages equal to the minimum IP datagram
         * of 576 bytes, which corresponds to a DHCP message with up to
         * 312 bytes of options.
         */

        printf ("Error: unusable network device.\n");
        return (NULL);
        }

    if (maxSize < DFLTDHCPLEN + UDPHL + IPHL)
        {
        /* The buffer size must also allow the minimum IP datagram. */

        return (NULL);
        }

    if (bpfDrv () == ERROR)
        {
        return (NULL);
        }

    /* Select buffer size for BPF device less than or equal to the MTU size. */

    bufSize += pIf->if_hdrlen;
    if (bufSize > maxSize)
        bufSize = maxSize;

    bufSize += sizeof (struct bpf_hdr);
    if (bpfDevCreate ("/bpf/dhcpc", 1, bufSize) == ERROR)
        {
        bpfDrvRemove ();
        return (NULL);
        }

    /* Create and initialize storage for boot lease. */

    pBootLeaseData = &dhcpcBootLeaseData;
    pDhcpcBootLeaseData = pBootLeaseData;
    dhcpcLeaseList = &pDhcpcBootLeaseData;

    dhcpcMaxLeases = 1;      /* Only one DHCP session, for boot device. */
    dhcpcDefaultLease = defaultLease;
    dhcpcOfferTimeout = offerTimeout;
    dhcpcMinLease = minLease;

    /* Create signalling semaphore for event notification ring. */

    dhcpcEventSem = semCCreate (SEM_Q_FIFO, 0);
    if (dhcpcEventSem == NULL)
        {
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        return (NULL);
        }

    /* Create event storage. */

    dhcpcEventRing = rngCreate (EVENT_RING_SIZE);
    if (dhcpcEventRing == NULL)
        {
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        semDelete (dhcpcEventSem);
        return (NULL);
        }

    /* Create message storage (matches number of elements in event ring). */

    dhcpcMessageList = malloc (10 * sizeof (MESSAGE_DATA));
    if (dhcpcMessageList == NULL)
        {
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        semDelete (dhcpcEventSem);
        rngDelete (dhcpcEventRing);
        return (NULL);
        }

    /* Allocate (aligned) receive buffers based on maximum message size. */

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

        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        semDelete (dhcpcEventSem);
        rngDelete (dhcpcEventRing);
        free (dhcpcMessageList);
        return (NULL);
        }

    /* Reset the message filter to use the specified client port. */

    dhcpfilter [12].k = clientPort;

    /* Get a BPF file descriptor to read/write DHCP messages. */

    sprintf (bpfDevName, "/bpf/dhcpc%d", offset);

    bpfDev = open (bpfDevName, 0, 0);
    if (bpfDev < 0)
        {
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        semDelete (dhcpcEventSem);
        rngDelete (dhcpcEventRing);
        for (offset = 0; offset < 10; offset++)
            free (dhcpcMessageList [offset].msgBuffer);
        free (dhcpcMessageList);

        return (NULL);
        }

    /* Enable immediate mode for reading messages. */

    result = 1;
    result = ioctl (bpfDev, BIOCIMMEDIATE, (int)&result);
    if (result != 0)
        {
        semDelete (dhcpcEventSem);
        rngDelete (dhcpcEventRing);
        for (offset = 0; offset < 10; offset++)
            free (dhcpcMessageList [offset].msgBuffer);
        free (dhcpcMessageList);
        close (bpfDev);
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        return (NULL);
        }

    /* Initialize the lease-specific variables. */

    bzero ( (char *)pBootLeaseData, sizeof (LEASE_DATA));

    pBootLeaseData->initFlag = FALSE;
    pBootLeaseData->autoConfig = TRUE;
    pBootLeaseData->cacheHookRtn = NULL;   /* Results are saved in bootline. */

    pReqSpec = &pBootLeaseData->leaseReqSpec;

    bzero ( (char *)pReqSpec, sizeof (struct dhcp_reqspec));
    bzero ( (char *)&pBootLeaseData->ifData, sizeof (struct if_info));

    /* Initialize WIDE project global containing network device data.  */

    sprintf (pBootLeaseData->ifData.name, "%s", pIf->if_name);
    pBootLeaseData->ifData.unit = pIf->if_unit;
    pBootLeaseData->ifData.iface = pIf;
    pBootLeaseData->ifData.bpfDev = bpfDev;
    pBootLeaseData->ifData.bufSize = bufSize;

    /*
     * For END devices, use hardware address information set when
     * driver attached to IP. Assume BSD devices use Ethernet frames.
     */

    if (bsdFlag)
        pBootLeaseData->ifData.haddr.htype = ETHER;
    else
        pBootLeaseData->ifData.haddr.htype = dhcpConvert (pIf->if_type);

    pBootLeaseData->ifData.haddr.hlen = pIf->if_addrlen;

    if (pBootLeaseData->ifData.haddr.hlen > MAX_HLEN)
        pBootLeaseData->ifData.haddr.hlen = MAX_HLEN;

    bcopy ( (char *) ( (struct arpcom *)pIf)->ac_enaddr,
           (char *)&pBootLeaseData->ifData.haddr.haddr,
           pBootLeaseData->ifData.haddr.hlen);

    /*
     * Set the maximum message length based on the MTU size and the
     * specified maximum message/buffer size.
     */

    bufSize = pIf->if_mtu;
    if (bufSize > maxSize)
        pReqSpec->maxlen = maxSize;
    else
        pReqSpec->maxlen = bufSize;

    dhcpcBufSize = maxSize;

    /* set duration of client's wait for multiple offers */

    pReqSpec->waitsecs = dhcpcOfferTimeout;

    /* 
     * Initialize request list with tags for options provided by server,
     * according to RFC 2132.
     */

    pReqSpec->reqlist.len = 0;
    pReqSpec->reqlist.list [pReqSpec->reqlist.len++] = _DHCP_SUBNET_MASK_TAG;
    pReqSpec->reqlist.list [pReqSpec->reqlist.len++] = _DHCP_BRDCAST_ADDR_TAG;

    /* Create event timer for state machine.  */

    pBootLeaseData->timer = wdCreate ();
    if (pBootLeaseData->timer == NULL)
        {
        semDelete (dhcpcEventSem);
        rngDelete (dhcpcEventRing);
        for (offset = 0; offset < 10; offset++)
            free (dhcpcMessageList [offset].msgBuffer);
        free (dhcpcMessageList);
        close (bpfDev);
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        return (NULL);
        }
    pBootLeaseData->initFlag = TRUE;

    _dhcpcBootFlag = TRUE;    /* Let read task ignore (unavailable) socket. */

    /*
     * Spawn the data retrieval task.  The entry routine will read all
     * incoming DHCP messages and signal the client state machine when a
     * valid one arrives.
     */

    result = taskSpawn ("tDhcpcReadTask", _dhcpcReadTaskPriority,
                        _dhcpcReadTaskOptions, _dhcpcReadTaskStackSize,
                        (FUNCPTR) dhcpcRead, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (result == ERROR)
        {
        semDelete (dhcpcEventSem);
        rngDelete (dhcpcEventRing);
        for (offset = 0; offset < 10; offset++)
            free (dhcpcMessageList [offset].msgBuffer);
        free (dhcpcMessageList);
        close (bpfDev);
        bpfDevDelete ("/bpf/dhcpc");
        bpfDrvRemove ();
        wdDelete (pBootLeaseData->timer);
        return (NULL);
        }
    dhcpcReadTaskId = result;

    return (pBootLeaseData);
    }

/*******************************************************************************
*
* dhcpcBootBind - initialize the network with DHCP at boot time
*
* This routine performs the client side of a DHCP negotiation according to 
* RFC 2131.  The negotiation uses the network device specified with the
* initialization call.  The addressing information retrieved is applied to that 
* network device.  Because the boot image is replaced by the downloaded target 
* image, the resulting lease cannot be renewed.  Therefore, the minimum lease 
* length specified by DHCPC_MIN_LEASE must be set so that the target image has 
* sufficient time to download and begin monitoring the lease.  This routine is 
* called automatically by the boot program when INCLUDE_DHCPC is defined and 
* the automatic configuration option is set in the boot flags and no target
* address is present.
*
* RETURNS: OK if negotiation is successful, or ERROR otherwise.
*
* ERRNO: N/A
*/

STATUS dhcpcBootBind (void)
    {
    STATUS result = OK;
    struct dhcpcOpts * 	pOptList;
    char * 		pOptions;
    struct ifreq 	ifr;

    /*
     * Allocate space for any options in outgoing messages
     * and fill in the options field.
     */

    pOptList = dhcpcBootLeaseData.leaseReqSpec.pOptList;
    if (pOptList != NULL)
        {
        pOptions = malloc (pOptList->optlen);
        if (pOptions == NULL)
            {
            return (ERROR);
            }
        dhcpcBootLeaseData.leaseReqSpec.optlen = pOptList->optlen;

        dhcpcOptFieldCreate (pOptList, pOptions);
        free (pOptList);
        dhcpcBootLeaseData.leaseReqSpec.pOptList = NULL;
        dhcpcBootLeaseData.leaseReqSpec.pOptions = pOptions;
        }

    /*
     * Add a filter for incoming DHCP packets and assign the selected
     * interface to the BPF device.
     */

    result = ioctl (dhcpcBootLeaseData.ifData.bpfDev, BIOCSETF, 
                    (int)&dhcpread);
    if (result != 0)
        {
        return (ERROR);
        }

    bzero ( (char *)&ifr, sizeof (struct ifreq));
    sprintf (ifr.ifr_name, "%s%d", dhcpcBootLeaseData.ifData.name,
                                   dhcpcBootLeaseData.ifData.unit);

    result = ioctl (dhcpcBootLeaseData.ifData.bpfDev, BIOCSETIF, (int)&ifr);
    if (result != 0)
        {
        return (ERROR);
        }

    dhcpcBindType = DHCP_MANUAL;
    result = dhcp_boot_client (& (dhcpcBootLeaseData.ifData), 
                               dhcpcServerPort, dhcpcClientPort, TRUE);

    dhcpcBootCleanup ();

    if (result != OK)
      return (ERROR);

    /* Bind was successful. */

    return (OK);
    }

/*******************************************************************************
*
* dhcpcBootInformGet - obtain additional configuration parameters with DHCP
*
* This routine uses DHCP to retrieve additional configuration parameters for
* a client with the externally configured network address given by the
* <pAddrString> parameter. It sends an INFORM message and waits for a reply
* following the process described in RFC 2131. The message exchange uses
* the network device specified with the initialization call. Any interface
* information retrieved is applied to that network device. Since this process
* does not establish a lease, the target address will not contain any
* timestamp information so that the runtime image will not attempt to verify 
* the configuration parameters. This routine is called automatically by the
* boot program when INCLUDE_DHCPC is defined and the automatic configuration
* option is set in the boot flags if a target address is already present.
*
* RETURNS: OK if negotiation is successful, or ERROR otherwise.
*
* ERRNO: N/A
*/

STATUS dhcpcBootInformGet
    (
    char * 	pAddrString 	/* known address assigned to client */
    )
    {
    STATUS result = OK;
    struct dhcpcOpts * 	pOptList;
    char * 		pOptions;

    /* Set the requested IP address field to the externally assigned value. */

    dhcpcBootLeaseData.leaseReqSpec.ipaddr.s_addr = inet_addr (pAddrString);
    if (dhcpcBootLeaseData.leaseReqSpec.ipaddr.s_addr == -1)
        return (ERROR);

    /*
     * Allocate space for any options in outgoing messages
     * and fill in the options field.
     */

    pOptList = dhcpcBootLeaseData.leaseReqSpec.pOptList;
    if (pOptList != NULL)
        {
        pOptions = malloc (pOptList->optlen);
        if (pOptions == NULL)
            {
            return (ERROR);
            }
        dhcpcBootLeaseData.leaseReqSpec.optlen = pOptList->optlen;

        dhcpcOptFieldCreate (pOptList, pOptions);
        free (pOptList);
        dhcpcBootLeaseData.leaseReqSpec.pOptList = NULL;
        dhcpcBootLeaseData.leaseReqSpec.pOptions = pOptions;
        }

    dhcpcBindType = DHCP_MANUAL;
    result = dhcp_boot_client (& (dhcpcBootLeaseData.ifData),
                               dhcpcServerPort, dhcpcClientPort, FALSE);

    dhcpcBootCleanup ();

    if (result != OK)
      return (ERROR);

    /* Additional configuration parameters retrieved. */

    return (OK);
    }

/*******************************************************************************
*
* dhcpcBootParamsGet - retrieve current network parameters
*
* This routine copies the configuration parameters obtained at boot time to 
* the caller-supplied structure.  That structure contains non-NULL pointers to 
* indicate the parameters of interest.  All other values within the structure 
* must be set to 0 before calling the routine.  This routine is called 
* internally at boot time when the DHCP client is in the BOUND state.
*
* RETURNS: OK if dhcpcBootParam is non-NULL, or ERROR otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

STATUS dhcpcBootParamsGet
    (
    struct dhcp_param*	pParamList 
    )
    {

    /*
     * If we were doing a DHCPINFORM and got no info, dhcpcBootParam
     * might still be NULL, in which case there are no parameters to
     * copy. Test for this here, otherwise dhcpcParamsCopy() will
     * blindly try to copy stuff from page 0.
     */
    if (dhcpcBootParam == NULL)
        return (ERROR);

    dhcpcBootLeaseData.dhcpcParam = dhcpcBootParam;

    dhcpcParamsCopy (&dhcpcBootLeaseData, pParamList);

    return (OK);
    }

/******************************************************************************
*
* dhcpcBootCleanup - remove data structures used by boot-time DHCP client
*
* This routine removes all data structures used by the dhcp client. It is 
* called internally when the client exits.
*
* RETURNS: N/A
*
* ERRNO:   N/A
*
* NOMANUAL
*/

LOCAL void dhcpcBootCleanup (void)
    {
    int loop;

    wdCancel (dhcpcBootLeaseData.timer);
    wdDelete (dhcpcBootLeaseData.timer);

    close (dhcpcBootLeaseData.ifData.bpfDev);

    taskDelete (dhcpcReadTaskId);

    semDelete (dhcpcEventSem);
    rngDelete (dhcpcEventRing);

    for (loop = 0; loop < 10; loop++)
         free (dhcpcMessageList [loop].msgBuffer);
    free (dhcpcMessageList);

    if (dhcpcBootLeaseData.leaseReqSpec.pOptions != NULL)
        {
        free (dhcpcBootLeaseData.leaseReqSpec.pOptions);
        }

    bpfDevDelete ("/bpf/dhcpc");

    bpfDrvRemove ();

    return;
    }

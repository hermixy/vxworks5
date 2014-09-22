/* dhcpsLib.c - Dynamic Host Configuration Protocol (DHCP) server library */

/* Copyright 1984 - 2001 Wind River Systems, Inc.  */
#include "copyright_wrs.h"

/*
modification history
--------------------
01v,16nov01,spm  fixed modification history
01u,29oct01,wap  Correct typo in documentation (SPR #34808)
01t,15oct01,rae  merge from truestack ver 01x, base 01p (VIRTUAL_STACK)
01s,17nov00,spm  added support for BSD Ethernet devices
01r,24oct00,spm  fixed modification history after merge from tor3_0_x branch;
                 corrected pathname in library description
01q,23oct00,niq  merged from version 01t of tor3_0_x branch (base version 01p);
                 upgrade to BPF replaces tagged frames support
01p,17jan98,kbw  removed NOMANUAL restriction from init function
01o,17dec97,spm  fixed byte order of address-based loop boundaries (SPR #20056)
01n,14dec97,kbw  made minor man page fixes
01m,10dec97,kbw  made minor man page fixes
01l,04dec97,spm  added code review modifications
01k,25oct97,kbw  made minor man page fixes
01j,06oct97,spm  removed reference to deleted endDriver global; replaced with
                 support for dynamic driver type detection
01i,25sep97,gnn  SENS beta feedback fixes
01h,26aug97,spm  added code review fixes and support for UDP port selection
01g,12aug97,gnn  changes necessitated by MUX/END update.
01f,30jul97,kbw  fixed man page problems found in beta review
01e,15jul97,spm  allowed startup without lease storage; changed module title
01d,02jun97,spm  updated man pages and added ERRNO entries
01c,06may97,spm  changed memory access to align IP header on four byte
                 boundary and corrected cleanup routine
01b,17apr97,kbw  fixed man page format, changed wording, spell checked doc
01a,07apr97,spm  created by modifying WIDE project DHCP implementation
*/

/*
DESCRIPTION
This library implements the server side of the Dynamic Host Configuration
Protocol (DHCP).  DHCP is an extension of BOOTP.  Like BOOTP, it allows a
target to configure itself dynamically by using the network to get 
its IP address, a boot file name, and the DHCP server's address.  Additionally,
DHCP provides for automatic reuse of network addresses by specifying
individual leases as well as many additional options.  The compatible
message format allows DHCP participants to interoperate with BOOTP
participants.  The dhcpsInit() routine links this library into the VxWorks 
image.  This happens automatically if INCLUDE_DHCPS is defined when the image 
is built.

PRIMARY INTERFACE
The dhcpsInit() routine initializes the server.  It reads the hard-coded server 
configuration data that is stored in three separate tables.
The first table contains entries as follows:
.CS
DHCPS_LEASE_DESC dhcpsLeaseTbl [] =
    {
    {"sample1", "90.11.42.24", "90.11.42.24", "clid=\e"1:0x08003D21FE90\e""},
    {"sample2", "90.11.42.25", "90.11.42.28", "maxl=90:dfll=60"},
    {"sample3", "90.11.42.29", "90.11.42.34", "maxl=0xffffffff:file=/vxWorks"},
    {"sample4", "90.11.42.24", "90.11.42.24", "albp=true:file=/vxWorks"}
    };
.CE
Each entry contains a name of up to eight characters, the starting and ending 
IP addresses of a range, and the parameters associated with the lease.  The
four samples shown demonstrate the four types of leases. 

Manual leases contain a specific client ID, and are issued only to that client, 
with an infinite duration.  The example shown specifies a MAC address, which is 
the identifier type used by the VxWorks DHCP client.

Dynamic leases specify a finite maximum length, and can be issued to any
requesting client.  These leases allow later re-use of the assigned IP address.
If not explicitly specified in the parameters field, these leases use the 
values of DHCPS_MAX_LEASE and DHCPS_DFLT_LEASE to determine the lease length.

Automatic leases are implied by the infinite maximum length.  Their IP addresses
are assigned permanently to any requesting client.

The last sample demonstrates a lease that is also available to BOOTP clients.
The infinite maximum length is implied, and any timing-related parameters are
ignored. 

The DHCP server supplies leases to DHCP clients according to the lease type in
the order shown above.  Manual leases have the highest priority and leases 
available to BOOTP clients the lowest.

Entries in the parameters field may be one of these types:

.IP `bool' 8
Takes values of "true" or "false", for example, ipfd=true.  Unrecognized 
values default to false. 
.IP 'str'
Takes a character string as a value, for example, hstn="clapton".
If the string includes a delimiter character, such as a colon, it 
should be enclosed in quotation marks.
.IP 'octet'
Takes an 8-bit integer in decimal, octal, or hexadecimal, for example,
8, 070, 0xff.
.IP 'short'
Takes a 16-bit integer.
.IP 'long'
Takes a 32-bit integer.
.IP 'ip'
Takes a string that is interpreted 
as a 32-bit IP address.  One of the following formats is
expected: a.b.c.d, a.b.c or a.b.  In the second
format, c is interpreted as a 16-bit value.  In
the third format, b is interpreted as a 24-bit
value, for example siad=90.11.42.1.  
.IP 'iplist'
Takes a list of IP addresses, separated by
white space, for example, rout=133.4.31.1 133.4.31.2 133.4.31.3.
.IP 'ippairs'
Takes a list of IP address pairs.  Each IP
address is separated by white space and grouped
in pairs, for example, strt=133.4.27.0  133.4.31.1
133.4.36.0 133.4.31.1.
.IP 'mtpt'
Takes a list of 16 bit integers,
separated by white space, for example, mtpt=1 2 3 4 6 8.
.IP 'clid'
Takes a client identifier as a value.
Client identifiers are represented by the quoted
string "<type>:<data>", where <type> is an
integer from 0 to 255, as defined by the IANA,
and <data> is a sequence of 8-bit values in hexadecimal.
The client ID is usually a MAC address, for example, 
clid="1:0x08004600e5d5".
.LP
The following table lists the option specifiers and descriptions for
every possible entry in the parameter list.  When available, the option
code from RFC 2132 is included.
.TS
tab(|);
lf3 lf3 lf3 lf3
l   l   l   l.
Name | Code | Type    | Description
_
snam | -    | str     | Optional server name.

file | -    | str     | Name of file containing the boot image.

siad | -    | ip      | Address of server that offers the boot image.

albp | -    | bool    | If true, this entry is also available
     |      |         | to BOOTP clients.  For entries using
     |      |         | static allocation, this value becomes
     |      |         | true by default and <maxl> becomes
     |      |         | infinity.

maxl | -    | long    | Maximum lease duration in seconds. 

dfll | -    | long    | Default lease duration in seconds.  If a
     |      |         | client does not request a specific lease
     |      |         | duration, the server uses this value.

clid | -    | clid    | This specifies a client identifier for
     |      |         | manual leases.  The VxWorks client uses
     |      |         | a MAC address as the client identifier.

pmid | -    | clid    | This specifies a client identifier for
     |      |         | client-specific parameters to be included
     |      |         | in a lease.  It should be present in separate
     |      |         | entries without IP addresses.

clas | -    | str     | This specifies a class identifier for
     |      |         | class-specific parameters to be included in 
     |      |         | a lease.  It should be present in separate entries
     |      |         | without IP addresses.

snmk | 1    | ip      | Subnet mask of the IP address to be
     |      |         | allocated.  The default is a natural mask
     |      |         | corresponding to the IP address.  The
     |      |         | server will not issue IP addresses to 
     |      |         | clients on different subnets.

tmof | 2    | long    | Time offset from UTC in seconds.

rout | 3    | iplist  | A list of routers on the same subnet as
     |      |         | the client.

tmsv |  4   | iplist  | A list of time servers (RFC 868).

nmsv |  5   | iplist  | A list of name servers (IEN 116).

dnsv |  6   | iplist  | A list of DNS servers (RFC 1035).

lgsv |  7   | iplist  | A list of MIT-LCS UDP log servers.

cksv |  8   | iplist  | A list of Cookie servers (RFC 865).

lpsv |  9   | iplist  | A list of LPR servers (RFC 1179).

imsv |  10  | iplist  | A list of Imagen Impress servers.

rlsv |  11  | iplist  | A list of Resource Location servers (RFC 887).

hstn |  12  | str     | Hostname of the client.

btsz |  13  | short   | Size of boot image.

mdmp |  14  | str     | Path name to which client dumps core.

dnsd |  15  | str     | Domain name for DNS.

swsv |  16  | ip      | IP address of swap server.

rpth |  17  | str     | Path name of root disk of the client.

epth |  18  | str     | Extensions Path (See RFC 1533).

ipfd |  19  | bool    | If true, the client performs IP
     |      |         | forwarding.

nlsr |  20  | bool    | If true, the client can perform non-local
     |      |         | source routing.

plcy |  21  | ippairs | Policy filter for non-local source
     |      |         | routing.  A list of pairs of
     |      |         | (Destination IP, Subnet mask).

mdgs |  22  | short   | Maximum size of IP datagram that the
     |      |         | client should be able to reassemble.

ditl |  23  | octet   | Default IP TTL.

mtat |  24  | long    | Aging timeout (in seconds) to be used
     |      |         | with Path MTU discovery (RFC 1191).

mtpt |  25  | mtpt    | A table of MTU sizes to be used with
     |      |         | Path MTU Discovery.

ifmt |  26  | short   | MTU to be used on an interface.

asnl |  27  | bool    | If true, the client assumes that all
     |      |         | subnets to which the client is connected
     |      |         | use the same MTU.

brda |  28  | ip      | Broadcast address in use on the client's
     |      |         | subnet.  The default is calculated from
     |      |         | the subnet mask and the IP address.

mskd |  29  | bool    | If true, the client should perform subnet
     |      |         | mask discovery using ICMP.

msks |  30  | bool    | If true, the client should respond to
     |      |         | subnet mask requests using ICMP.

rtrd |  31  | bool    | If true, the client should solicit
     |      |         | routers using Router Discovery defined
     |      |         | in RFC 1256.

rtsl |  32  | ip      | Destination IP address to which the
     |      |         | client sends router solicitation
     |      |         | requests.

strt |  33  | ippairs | A table of static routes for the client,
     |      |         | which are pairs of (Destination, Router).
     |      |         | It is illegal to specify default route
     |      |         | as a destination.

trlr |  34  | bool    | If true, the client should negotiate
     |      |         | the use of trailers with ARP (RFC 893).

arpt |  35  | long    | Timeout in seconds for ARP cache.

encp |  36  | bool    | If false, the client uses RFC 894
     |      |         | encapsulation.  If true, it uses
     |      |         | RFC 1042 (IEEE 802.3) encapsulation.

dttl |  37  | octet   | Default TTL of TCP.

kain |  38  | long    | Interval of the client's TCP keepalive
     |      |         | in seconds.

kagb |  39  | bool    | If true, the client should send TCP
     |      |         | keepalive messages with a octet of
     |      |         | garbage for compatibility.

nisd |  40  | str     | Domain name for NIS.

nisv |  41  | iplist  | A list of NIS servers.

ntsv |  42  | iplist  | A list of NTP servers.

nnsv |  44  | iplist  | A list of NetBIOS name server.
     |      |         | (RFC 1001, 1002)

ndsv |  45  | iplist  | A list of NetBIOS datagram distribution
     |      |         | servers (RFC 1001, 1002).

nbnt |  46  | octet   | NetBIOS node type (RFC 1001, 1002).

nbsc |  47  | str     | NetBIOS scope (RFC 1001, 1002).

xfsv |  48  | iplist  | A list of font servers of X Window system.

xdmn |  49  | iplist  | A list of display managers of X Window
     |      |         | system.

dht1 |  58  | short   | This value specifies when the client should
     |      |         | start RENEWING.  The default of 500 means
     |      |         | the client starts RENEWING after 50% of the
     |      |         | lease duration passes.

dht2 |  59  | short   | This value specifies when the client should
     |      |         | start REBINDING.  The default of 875 means
     |      |         | the client starts REBINDING after 87.5% of
     |      |         | the lease duration passes.
.TE
.LP
Finally, to function correctly, the DHCP server requires access to some form
of permanent storage.  The DHCPS_LEASE_HOOK constant specifies the name of a 
storage routine with the following interface:
.CS
    STATUS dhcpsStorageHook (int op, char *buffer, int datalen);
.CE
The storage routine is installed by a call to the dhcpsLeaseHookAdd() routine
The manual pages for dhcpsLeaseHookAdd() describe the parameters and required 
operation of the storage routine.

SECONDARY INTERFACE
In addition to the hard-coded entries, address entries may be added after the 
server has started by calling the following routine:
.CS
    STATUS dhcpsLeaseEntryAdd (char *name, char *start, char *end, char *config);
.CE
The parameters specify an entry name, starting and ending values for a block 
of IP addresses, and additional configuration information in the same format 
as shown above for the hard-coded entries.  Each parameter must be formatted 
as a NULL-terminated string. 

The DHCPS_ADDRESS_HOOK constant specifies the name of a storage routine, used 
to preserve address entries added after startup, which has the following 
prototype:
.CS
    STATUS dhcpsAddressStorageHook (int op,
				    char *name, char *start, char *end, 
				    char *params);
.CE
The storage routine is installed with the dhcpsAddressHookAdd() routine, and 
is fully described in the manual pages for that function.

OPTIONAL INTERFACE
The DHCP server can also receive messages forwarded from different subnets
by a relay agent.  To provide addresses to clients on different subnets, the
appropriate relay agents must be listed in the provided table in usrNetwork.c. 
A sample configuration is:
.CS
    DHCPS_RELAY_DESC dhcpsRelayTbl [] =
	{
	{"90.11.46.75", "90.11.46.0"}
	};
.CE
Each entry in the table specifies the address of a relay agent that will 
transmit the request and the corresponding subnet number.  To issue leases
successfully, the address pool must also contain IP addresses for the 
monitored subnets.

The following table allows a DHCP server to act as a relay agent in 
addition to its default function of processing messages.  It consists of 
a list of IP addresses.
.CS
    DHCP_TARGET_DESC dhcpTargetTbl [] =
	 {
	 {"90.11.43.2"},
	 {"90.11.44.1"}
	 };
.CE
Each IP address in this list receives a copy of any client messages
generated on the subnets monitored by the server.

INTERNAL
This library provides a wrapper for the WIDE project DHCP code found in the
directory .../target/src/dhcp, which contains all the message
processing functions.  This module uses the Berkeley Packet Filter services
to intercept incoming DHCP messages and signals their arrival to the WIDE
project code using a shared semaphore.

INCLUDE FILES: dhcpsLib.h

SEE ALSO: RFC 1541, RFC 1533
*/

/* includes */

#include "vxWorks.h"

#include "dhcp/copyright_dhcp.h"

#include "stdio.h"
#include "stdlib.h"
#include "netinet/if_ether.h"
#include "netinet/in.h"
#include "netinet/ip.h"
#include "netinet/udp.h"
#include "sys/ioctl.h"
#include "inetLib.h"

#include "end.h"
#include "ipProto.h"

#include "ioLib.h"             /* close() declaration. */
#include "logLib.h"
#include "rngLib.h"
#include "semLib.h"
#include "sockLib.h"
#include "taskLib.h"
#include "muxLib.h"

#include "dhcp/dhcp.h"
#include "dhcp/common.h"
#include "dhcp/hash.h"
#include "dhcp/common_subr.h"
#include "dhcpsLib.h"

#include "bpfDrv.h"

/* defines */

#define _DHCPS_MAX_DEVNAME 26 	/* "/bpf/dhcps" + max unit number + vs num */

/* globals for both the regular and virtual stacks */

#ifndef VIRTUAL_STACK

/* globals */
DHCPS_RELAY_DESC *      pDhcpsRelaySourceTbl;
DHCPS_LEASE_DESC *	pDhcpsLeasePool;

long    dhcps_dflt_lease; /* Default for default lease length */
long 	dhcps_max_lease;  /* Default maximum lease length */

SEM_ID  dhcpsMutexSem;             /* Synchronization for lease entry adds. */
FUNCPTR dhcpsLeaseHookRtn = NULL;  /* Accesses storage for lease bindings. */
FUNCPTR dhcpsAddressHookRtn = NULL; /* Preserves additional address entries. */

int dhcpsMaxSize; 	/* Transmit buffer size & largest supported message. */
int dhcpsBufSize; 	/* Size of buffer for BPF devices */

IMPORT struct iovec sbufvec[2];            /* send buffer */
IMPORT struct msg dhcpsMsgOut;
IMPORT struct hash_member *reslist;
IMPORT struct hash_member *bindlist;
IMPORT struct hash_tbl nmhashtable;
IMPORT struct hash_tbl iphashtable;
IMPORT struct hash_tbl cidhashtable;
IMPORT struct hash_tbl relayhashtable;
IMPORT struct hash_tbl paramhashtable;

IMPORT char *dhcpsSendBuf;
IMPORT char *dhcpsOverflowBuf;

IMPORT u_short dhcps_port;
IMPORT u_short dhcpc_port;
/* locals */

struct if_info *dhcpsIntfaceList = NULL;
LOCAL BOOL dhcpsInitialized = FALSE;

/* Berkeley Packet Filter instructions for catching DHCP messages. */

LOCAL struct bpf_insn dhcpfilter[] = DHCPS_FILTER_DEFINE;

LOCAL struct bpf_program dhcpread = {
    sizeof (dhcpfilter) / sizeof (struct bpf_insn),
    dhcpfilter
    };

#else
#include "netinet/vsLib.h"
#include "netinet/vsDhcps.h"
#endif  /* VIRTUAL_STACK */

/* forward declarations */

LOCAL STATUS alloc_sbuf (int);
void dhcpsCleanup (int);
void dhcpsFreeResource (struct dhcp_resource *);

IMPORT int resnmcmp();
IMPORT int resipcmp();
IMPORT int eval_symbol (void *, void *);

IMPORT int open_if();
IMPORT STATUS read_addrpool_file (void);
IMPORT void read_server_db (int);
IMPORT int process_entry (struct dhcp_resource *, DHCPS_ENTRY_DESC *);
IMPORT void set_default (struct dhcp_resource *);

/*******************************************************************************
*
* dhcpsInit - set up the DHCP server parameters and data structures
*
* This routine creates the necessary data structures, builds the
* server address pool, retrieves any lease or address information from 
* permanent storage through the user-provided hooks, and initializes the 
* network interfaces for monitoring.  It is called at system startup if 
* INCLUDE_DHCPS is defined at the time the VxWorks image is built.
*
* The <maxSize> parameter specifies the maximum length supported for any
* DHCP message, including the UDP and IP headers and the largest link
* level header for all supported devices. The smallest valid value is
* 576 bytes, corresponding to the minimum IP datagram a host must accept.
* Larger values will allow the server to handle longer DHCP messages.
* 
* RETURNS: OK, or ERROR if could not initialize.
*
*/

STATUS dhcpsInit
    (
    DHCPS_CFG_PARAMS  *pDhcpsCfg  	   /* configuration parameters */
    )
    {
    struct if_info *pIf = NULL;            /* pointer to interface */
    struct ifnet *pCIf = NULL;           /* pointer to current interface */
    char bpfDevName [_DHCPS_MAX_DEVNAME];  /* "/bpf/dhcps" + max unit number */
#ifdef VIRTUAL_STACK
    char temp [_DHCPS_MAX_DEVNAME];
#endif /* VIRTUAL_STACK */

    int bpfDev;

    struct ifreq ifreq;
    struct sockaddr_in send_addr;
    int loop;
    int result;
    STATUS status;

    if (pDhcpsCfg == (DHCPS_CFG_PARAMS  *)NULL)
	return (ERROR);

#ifdef VIRTUAL_STACK
    if (dhcpsVsInit () == ERROR)
        return (ERROR);
#endif /* VIRTUAL_STACK */

    /* It must return error, if it is already initialized */
    if (dhcpsInitialized)
        return (ERROR);

    if (pDhcpsCfg->dhcpMaxMsgSize < DFLTDHCPLEN + UDPHL + IPHL)
        return (ERROR);

    if (bpfDrv () == ERROR)
        return (ERROR);

    dhcpsMaxSize = pDhcpsCfg->dhcpMaxMsgSize;
    dhcpsBufSize = pDhcpsCfg->dhcpMaxMsgSize + sizeof (struct bpf_hdr);

    dhcps_dflt_lease = pDhcpsCfg->dhcpsDfltLease;
    dhcps_max_lease = pDhcpsCfg->dhcpsMaxLease;

    dhcpsLeaseHookAdd (pDhcpsCfg->pDhcpsLeaseFunc);
    dhcpsAddressHookAdd (pDhcpsCfg->pDhcpsAddrFunc);

#ifdef VIRTUAL_STACK
    sprintf(temp, "/bpf/vs%d_dhcps", myStackNum );
    if (bpfDevCreate (temp, pDhcpsCfg->numDev, dhcpsBufSize) == ERROR)
#else
    if (bpfDevCreate ("/bpf/dhcps", pDhcpsCfg->numDev, dhcpsBufSize) == ERROR)
#endif /* VIRTUAL_STACK */
        return (ERROR);

    dhcpsMutexSem = semBCreate (SEM_Q_FIFO, SEM_FULL);
    if (dhcpsMutexSem == NULL)
        return (ERROR);

    bzero ( (char *)&ifreq, sizeof (ifreq));
 
    srand (taskIdSelf ());

    /* Validate the MTU size. */

    for (loop = 0; loop < pDhcpsCfg->numDev; loop++)
        {
	pCIf = ifunit (pDhcpsCfg->pDhcpsIfTbl[loop].ifName);
	if (pCIf == NULL)
	    return (ERROR);

        if (pCIf->if_mtu == 0)    /* No MTU size given: not attached? */
            return (ERROR);

        if (pCIf->if_mtu < DHCP_MSG_SIZE)
            return (ERROR);
        }

    /*
     * Alter the filter program to check for the correct length values and
     * use the specified UDP destination port and maximum hop count.
     */

      /* IP length must at least equal a DHCP message with 4 option bytes. */
    dhcpfilter [6].k = (DFLTDHCPLEN - DFLTOPTLEN + 4) + UDPHL + IPHL;

    dhcpfilter [14].k = pDhcpsCfg->dhcpSPort;   /* Use actual destination port in test. */

       /* UDP length must at least equal a DHCP message with 4 option bytes. */
    dhcpfilter [16].k = (DFLTDHCPLEN - DFLTOPTLEN + 4) + UDPHL;

    dhcpfilter [18].k = pDhcpsCfg->dhcpMaxHops;   /* Use real maximum hop count. */

    /* Store the interface control information and get each BPF device. */

    for (loop = 0; loop < pDhcpsCfg->numDev; loop++)
        {
	pCIf = ifunit (pDhcpsCfg->pDhcpsIfTbl[loop].ifName);
        pIf = (struct if_info *)calloc (1, sizeof (struct if_info));
        if (pIf == NULL) 
            {
            logMsg ("Memory allocation error.\n", 0, 0, 0, 0, 0, 0);
            dhcpsCleanup (1); 
            return (ERROR);
            }
        pIf->buf = (char *)memalign (4, dhcpsBufSize);
        if (pIf->buf == NULL)
            {
            logMsg ("Memory allocation error.\n", 0, 0, 0, 0, 0, 0);
            dhcpsCleanup (1); 
            return (ERROR);
            }
        bzero (pIf->buf, dhcpsBufSize);

#ifdef VIRTUAL_STACK
        sprintf (bpfDevName, "/bpf/vs%d_dhcps%d", myStackNum, loop);
#else
        sprintf (bpfDevName, "/bpf/dhcps%d", loop);
#endif /* VIRTUAL_STACK */

        bpfDev = open (bpfDevName, 0, 0);
        if (bpfDev < 0)
            {
            logMsg ("BPF device creation error.\n", 0, 0, 0, 0, 0, 0);
            dhcpsCleanup (1);
            return (ERROR);
            }

        /* Enable immediate mode for reading messages. */

        result = 1;
        result = ioctl (bpfDev, BIOCIMMEDIATE, (int)&result);
        if (result != 0)
            {
            logMsg ("BPF device creation error.\n", 0, 0, 0, 0, 0, 0);
            dhcpsCleanup (1);
            return (ERROR);
            }

        result = ioctl (bpfDev, BIOCSETF, (int)&dhcpread);
        if (result != 0)
            {
            logMsg ("BPF filter assignment error.\n", 0, 0, 0, 0, 0, 0);
            dhcpsCleanup (1);
            return (ERROR);
            }

        sprintf (ifreq.ifr_name, "%s", pDhcpsCfg->pDhcpsIfTbl[loop].ifName );

        result = ioctl (bpfDev, BIOCSETIF, (int)&ifreq);
        if (result != 0)
            {
            logMsg ("BPF interface attachment error.\n", 0, 0, 0, 0, 0, 0);
            dhcpsCleanup (1);
            return (ERROR);
            }

        pIf->next = dhcpsIntfaceList;
        dhcpsIntfaceList = pIf;

        /*
         * Fill in device name and hardware address using the information
         * set when the driver attached to the IP network protocol.
         */

        sprintf (pIf->name, "%s", pCIf->if_name);
        pIf->unit = pCIf->if_unit;
        pIf->bpfDev = bpfDev;

        if (muxDevExists (pCIf->if_name, pCIf->if_unit) == FALSE)
            pIf->htype = ETHER;
        else
            pIf->htype = dhcpConvert (pCIf->if_type);

        pIf->hlen = pCIf->if_addrlen;

        bcopy ((char *) ((struct arpcom *)pCIf)->ac_enaddr,
               (char *)&pIf->haddr, pIf->hlen); 
        }

    pDhcpsLeasePool = pDhcpsCfg->pDhcpsLeaseTbl;

    pDhcpsRelaySourceTbl = pDhcpsCfg->pDhcpsRelayTbl;

    pDhcpRelayTargetTbl = pDhcpsCfg->pDhcpsTargetTbl;

    /* read address pool databasae */
    status = read_addrpool_db (pDhcpsCfg->dhcpsLeaseTblSize);
    if (status == ERROR) 
        {
        dhcpsCleanup (2); 
        return (ERROR);
        }

    status = read_addrpool_file ();  /* retrieve additional address entries */
    if (status == ERROR)
        {
        dhcpsCleanup (2); 
        return (ERROR);
        }
    status = read_bind_db ();        /* read binding database */
    if (status == ERROR)
        {
        dhcpsCleanup (3);
        return (ERROR);
        }
    /* read database of relay agents */ 
    if (pDhcpsCfg->dhcpsRelayTblSize != 0)
        read_relay_db (pDhcpsCfg->dhcpsRelayTblSize);

    /* read database of DHCP servers */
    if (pDhcpsCfg->dhcpTargetTblSize != 0)
        read_server_db (pDhcpsCfg->dhcpTargetTblSize);

    /* Always use default ports for client and server. */

    dhcps_port = htons (pDhcpsCfg->dhcpSPort);
    dhcpc_port = htons (pDhcpsCfg->dhcpCPort);

    /* Create a normal socket for sending messages across networks. */

    dhcpsIntfaceList->fd = socket (AF_INET, SOCK_DGRAM, 0);
    if (dhcpsIntfaceList->fd < 0)
        {
        dhcpsCleanup (4); 
        return (ERROR);
        }

    /* Read-only socket: disable receives to prevent buffer exhaustion. */

    result = 0;
    if (setsockopt (dhcpsIntfaceList->fd, SOL_SOCKET, SO_RCVBUF,
                    (char *)&result, sizeof (result)) != 0)
        {
        dhcpsCleanup (5); 
        return (ERROR);
        }

    bzero ( (char *)&send_addr, sizeof (send_addr));
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    send_addr.sin_port = dhcps_port;
    status = bind (dhcpsIntfaceList->fd, (struct sockaddr *) &send_addr, 
                   sizeof (send_addr));
    if (status < 0)
        {
        dhcpsCleanup (5);
        return (ERROR);
        }

    /* Fill in subnet mask and IP address for each monitored interface. */

    pIf = dhcpsIntfaceList;
    while (pIf != NULL)
        { 
        if (open_if (pIf) < 0) 
            {
            dhcpsCleanup (5);
            return (ERROR);
            }
        pIf = pIf->next;
        }

    /* Create buffers for outgoing messages. */

    if (alloc_sbuf (pDhcpsCfg->dhcpMaxMsgSize) != OK)
        {
        logMsg ("Error: Couldn't allocate memory.\n", 0, 0, 0, 0, 0, 0);
        dhcpsCleanup (5);  
        return (ERROR);
        }

    dhcpsInitialized = TRUE;

    return (OK);
    }

/*******************************************************************************
*
* dhcpsVsInit - Initializes the Virtual Stack Portion of the code
*
* dhcpsVsInit is used to configure the VS specific portion of the DHCP server.
*
* RETURNS: ERROR if it could not allocate the needed memory
*	   OK otherwise
*
* NOMANUAL
*/

#ifdef VIRTUAL_STACK
STATUS dhcpsVsInit ()
    {
    struct bpf_insn mydhcpfilter[] = DHCPS_FILTER_DEFINE;
    unsigned char mydhcpsCookie[] = RFC1048_MAGIC;

    if (vsTbl[myStackNum]->pDhcpsGlobals && (dhcpsInitialized == TRUE))
	 return (OK);

    /* Allocate memory to the dhcps globals in this virtual stack */
    vsTbl[myStackNum]->pDhcpsGlobals = (VS_DHCPS *) KHEAP_ALLOC (sizeof (VS_DHCPS));
    if (vsTbl[myStackNum]->pDhcpsGlobals == (VS_DHCPS *) NULL)
	 return (ERROR);

    bzero ((char *)vsTbl[myStackNum]->pDhcpsGlobals, sizeof (VS_DHCPS));

    /* Copy over filter variables */
    memcpy (dhcpfilter, mydhcpfilter, MAX_DHCPFILTERS * sizeof (struct bpf_insn));

    dhcpread.bf_len = MAX_DHCPFILTERS;
    dhcpread.bf_insns = dhcpfilter;

    /* Copy over the magic cookie */
    memcpy (dhcpCookie, mydhcpsCookie, MAGIC_LEN * sizeof (unsigned char));

    return (OK);
    }
#endif /* VIRTUAL_STACK */

/*******************************************************************************
*
* dhcpsCleanup - remove data structures
* 
* This routine frees all dynamically allocated memory obtained by the DHCP 
* server.  It is called at multiple points before the program exits due to
* an error occurring or manual shutdown.  The checkpoint parameter indicates 
* which data structures have been created.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void dhcpsCleanup 
    (
    int checkpoint 	/* Progress identifier indicating created resources */
    )
    {
    int current = 0;
    struct if_info *pIf;
    struct dhcp_resource * 	pResData;
    struct hash_member * 	pListElem;
    struct hash_member * 	pHashElem;
    struct hash_tbl * 		pHashTbl;
    struct dhcp_binding * 	pBindData;
    struct relay_acl * 		pRelayData;
    DHCP_SERVER_DESC * 		pSrvData;

    int loop;

    semDelete (dhcpsMutexSem);               /* Checkpoint 0 */
    current++;
    if (current > checkpoint)
        return;
                                             /* Checkpoint 1 */
    if (checkpoint >= 5)     /* Close socket descriptor if present.  */
        close (dhcpsIntfaceList->fd);

    while (dhcpsIntfaceList != NULL)
        {
        pIf = dhcpsIntfaceList;
        if (pIf->buf != NULL)
            {
            if (pIf->bpfDev >= 0)
                close (pIf->bpfDev);
            free (pIf->buf);
            }

        dhcpsIntfaceList = dhcpsIntfaceList->next;
        free (pIf);
        }
    current++;
    if (current > checkpoint)
        return;
   
    while (reslist != NULL)                  /* Checkpoint 2 */
        {
        pListElem = reslist;
        pResData = pListElem->data;
        reslist = reslist->next;
        dhcpsFreeResource (pResData);
        free (pListElem);
        }

    pHashTbl = &nmhashtable;
    for (loop = 0; loop < HASHTBL_SIZE; loop++)
        {
        pHashElem = (pHashTbl->head) [loop];

        while (pHashElem != NULL)
            {
            pListElem = pHashElem;
            pHashElem = pHashElem->next;
            free (pListElem);
            }
        }

    pHashTbl = &iphashtable;
    for (loop = 0; loop < HASHTBL_SIZE; loop++)
        {
        pHashElem = (pHashTbl->head) [loop];

        while (pHashElem != NULL)
            {
            pListElem = pHashElem;
            pHashElem = pHashElem->next;
            free (pListElem);
            }
        }

    /*
     * For the hash table holding additional parameters, also remove ID 
     * records (i.e. bindings), since these are not added to the bindlist.
     */

    pHashTbl = &paramhashtable;
    for (loop = 0; loop < HASHTBL_SIZE; loop++)
        {
        pHashElem = (pHashTbl->head) [loop];

        while (pHashElem != NULL)
            {
            pListElem = pHashElem;
            if (pHashElem->data != NULL)
                free (pHashElem->data);
            pHashElem = pHashElem->next;
            free (pListElem);
            }
        }
    current++;                      
    if (current > checkpoint)
        return;

    /* Checkpoint 3 */

    if (dhcpsAddressHookRtn != NULL)
        (* dhcpsAddressHookRtn) (DHCPS_STORAGE_STOP, NULL, NULL, NULL, NULL);

    while (bindlist != NULL)
        {
        pListElem = bindlist;
        pBindData = bindlist->data;
        bindlist = bindlist->next;
        free (pBindData);
        free (pListElem);
        }

    pHashTbl = &cidhashtable;
    for (loop = 0; loop < HASHTBL_SIZE; loop++)
        {
        pHashElem = (pHashTbl->head) [loop];

        while (pHashElem != NULL)
            {
            pListElem = pHashElem;
            pHashElem = pHashElem->next;
            free (pListElem);
            }
        }
    current++;
    if (current > checkpoint)
        return;

    pHashTbl = &relayhashtable;             /* Checkpoint 4 */
    for (loop = 0; loop < HASHTBL_SIZE; loop++)
        {
        pHashElem = (pHashTbl->head) [loop];

        while (pHashElem != NULL)
            {
            pListElem = pHashElem;
            pRelayData = pHashElem->data;
            pHashElem = pHashElem->next;
            free (pRelayData);
            free (pListElem);
            }
        }

    /* Remove elements of circular list created by read_server_db(). */

    pSrvData = pDhcpTargetList;
    for (loop = 0; loop < dhcpNumTargets; loop++)
        {
        pDhcpTargetList = pDhcpTargetList->next;
        free (pSrvData);
        }
    current++;
    if (current > checkpoint)
        return;

    /* Checkpoint 5 (close socket) contained in checkpoint 1. */
    current++;
    if (current > checkpoint)
        return;

    free (dhcpsSendBuf);                     /* Checkpoint 6 */
    current++;
    if (current > checkpoint)
        return;

    return;
    }

/*******************************************************************************
*
* void dhcpsFreeResource - remove allocated memory for resource entry
*
* This routine removes any dynamically allocated memory used by the given 
* lease descriptor.  It is called during cleanup before the server shuts down.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void dhcpsFreeResource 
    (
    struct dhcp_resource *pTarget    /* Pointer to resource to remove */
    )
    {
    if (pTarget->mtu_plateau_table.shorts != NULL)
        free (pTarget->mtu_plateau_table.shorts);

    if (pTarget->router.addr != NULL)
        free (pTarget->router.addr);

    if (pTarget->time_server.addr != NULL)
        free (pTarget->time_server.addr);

    if (pTarget->name_server.addr != NULL)
        free (pTarget->name_server.addr);

    if (pTarget->dns_server.addr != NULL)
        free (pTarget->dns_server.addr);

    if (pTarget->log_server.addr != NULL)
        free (pTarget->log_server.addr);

    if (pTarget->cookie_server.addr != NULL)
        free (pTarget->cookie_server.addr);

    if (pTarget->lpr_server.addr != NULL)
        free (pTarget->lpr_server.addr);

    if (pTarget->impress_server.addr != NULL)
        free (pTarget->impress_server.addr);

    if (pTarget->rls_server.addr != NULL)
        free (pTarget->rls_server.addr);

    if (pTarget->nis_server.addr != NULL)
        free (pTarget->nis_server.addr);

    if (pTarget->ntp_server.addr != NULL)
        free (pTarget->ntp_server.addr);

    if (pTarget->nbn_server.addr != NULL)
        free (pTarget->nbn_server.addr);

    if (pTarget->nbdd_server.addr != NULL)
        free (pTarget->nbdd_server.addr);

    if (pTarget->xfont_server.addr != NULL)
        free (pTarget->xfont_server.addr);

    if (pTarget->xdisplay_manager.addr != NULL)
        free (pTarget->xdisplay_manager.addr);

    if (pTarget->nisp_server.addr != NULL)
        free (pTarget->nisp_server.addr);

    if (pTarget->mobileip_ha.addr != NULL)
        free (pTarget->mobileip_ha.addr);

    if (pTarget->smtp_server.addr != NULL)
        free (pTarget->smtp_server.addr);

    if (pTarget->pop3_server.addr != NULL)
        free (pTarget->pop3_server.addr);

    if (pTarget->nntp_server.addr != NULL)
        free (pTarget->nntp_server.addr);

    if (pTarget->dflt_www_server.addr != NULL)
        free (pTarget->dflt_www_server.addr);

    if (pTarget->dflt_finger_server.addr != NULL)
        free (pTarget->dflt_finger_server.addr);

    if (pTarget->dflt_irc_server.addr != NULL)
        free (pTarget->dflt_irc_server.addr);

    if (pTarget->streettalk_server.addr != NULL)
        free (pTarget->streettalk_server.addr);

    if (pTarget->stda_server.addr != NULL)
        free (pTarget->stda_server.addr);

    if (pTarget->policy_filter.addr1 != NULL)
        free (pTarget->policy_filter.addr1);
    if (pTarget->policy_filter.addr2 != NULL)
        free (pTarget->policy_filter.addr2);

    if (pTarget->static_route.addr1 != NULL)
        free (pTarget->static_route.addr1);
    if (pTarget->static_route.addr2 != NULL)
        free (pTarget->static_route.addr2);

    free (pTarget);

    return;
    }

/*******************************************************************************
*
* dhcpsLeaseEntryAdd - add another entry to the address pool
*
* This routine allows the user to add new entries to the address pool without
* rebuilding the VxWorks image.  The routine requires a unique entry name of up
* to eight characters, starting and ending IP addresses, and a colon-separated 
* list of parameters.  Possible values for the parameters are listed in the 
* reference entry for dhcpsLib.  The parameters also determine the type of 
* lease, which the server uses to determine priority when assigning lease 
* addresses.  For examples of the possible lease types, see the reference 
* entry for dhcpsLib.
*
* RETURNS: OK if entry read successfully, or ERROR otherwise.
*
* ERRNO: N/A
*/

STATUS dhcpsLeaseEntryAdd
    (
    char *	pName, 		/* name of lease entry */
    char *	pStartIp, 	/* first IP address to assign */
    char * 	pEndIp, 	/* last IP address in assignment range */
    char *	pParams 	/* formatted string of lease parameters */
    )
    {
    struct dhcp_resource *pResData = NULL;
    DHCPS_ENTRY_DESC	entryData;
    struct hash_member *pListElem = NULL;
    int result;
    char tmp [10];  /* sizeof ("tblc=dlft") for host requirements defaults. */
    char entryName [BASE_NAME + 1];   /* User-given name for address range. */
    char newName [MAX_NAME + 1];    /* Unique name for each entry in range. */
    char newAddress [INET_ADDR_LEN];
    struct in_addr nextAddr;
    int len;
    char *pTmp;
    u_long start = 0;
    u_long end = 0;
    u_long loop;
    u_long limit;

    /* Ignore bad values for range. */

    if (pStartIp != NULL && pEndIp == NULL)
        return (ERROR);

    if (pStartIp == NULL && pEndIp != NULL)
        return (ERROR);

    /* If no address specified, just process parameters once. */

    if (pStartIp == NULL && pEndIp == NULL)
        {
        start = 0;
        end = 0;
        }
    else
        {
        start = inet_addr (pStartIp);
        end = inet_addr (pEndIp);
        if (start == ERROR || end == ERROR)
            return (ERROR);
        }

    entryData.pName = newName;
    entryData.pAddress = newAddress;
    entryData.pParams = pParams;

    len = strlen (pName);
    if (len > BASE_NAME)
        {
        bcopy (pName, entryName, BASE_NAME);
        entryName [BASE_NAME] = '\0';
        }
    else
        {
        bcopy (pName, entryName, len);
        entryName [len] = '\0';
        }

    limit = ntohl (end);
    for (loop = ntohl (start); loop <= limit; loop++)
        {
        pResData = (struct dhcp_resource *)calloc (1, 
                                                sizeof (struct dhcp_resource));
        if (pResData == NULL)
            {
            return (ERROR);
            }

        /* Generate a unique name by appending IP address value. */

        sprintf (entryData.pName, "%s%lx", entryName, loop);

        /* Assign current IP address in range. */

        nextAddr.s_addr = htonl (loop);
        inet_ntoa_b (nextAddr, entryData.pAddress);

        /* Critical section with message processing routines. */ 

        semTake (dhcpsMutexSem, WAIT_FOREVER);
 
        if (process_entry (pResData, &entryData) < 0)
            {
            semGive (dhcpsMutexSem);
            dhcpsFreeResource (pResData);
            return (ERROR);
            }

        /* 
         * Add Host Requirements defaults to resource entry. Do not add
         * to entries containing client-specific or class-specific 
         * parameters. 
         */

        if (ISCLR (pResData->valid, S_PARAM_ID) &&
            ISCLR (pResData->valid, S_CLASS_ID))
            {
            sprintf (tmp, "%s", "tblc=dflt");
            pTmp = tmp;
            eval_symbol (&pTmp, pResData);
            }

        /* Set default values for entry, if not already assigned. */

        if (ISSET (pResData->valid, S_IP_ADDR))
            set_default (pResData);

        /*
         * Append entries to lease descriptor list. Exclude entries which
         * specify additional client- or class-specific parameters.
         */

        if (ISCLR (pResData->valid, S_PARAM_ID) && 
            ISCLR (pResData->valid, S_CLASS_ID))
            {
            pListElem = reslist;
            while (pListElem->next != NULL)
                pListElem = pListElem->next; 
   
            pListElem->next =
                 (struct hash_member *)calloc (1, sizeof (struct hash_member));
            if (pListElem->next == NULL)
                {
                semGive (dhcpsMutexSem);
                dhcpsFreeResource (pResData);
                return (ERROR);
                }
            pListElem = pListElem->next;
            pListElem->next = NULL;
            pListElem->data = (void *)pResData;

            /* Add entryname to appropriate hash table. */

            result = hash_ins (&nmhashtable, pResData->entryname, 
                               strlen (pResData->entryname), resnmcmp, 
                               pResData->entryname, pResData);
            if (result < 0)
                {
                semGive (dhcpsMutexSem);
                return (ERROR);
                }

            if (ISSET (pResData->valid, S_IP_ADDR))
                if (hash_ins (&iphashtable, (char *)&pResData->ip_addr.s_addr,
                              sizeof (u_long), resipcmp, &pResData->ip_addr, 
                              pResData) < 0)
                    {
                    semGive (dhcpsMutexSem);
                    return (ERROR);
                    }
            }

        semGive (dhcpsMutexSem);
        }

    /* Save entry in permanent storage, if hook is provided. */

    if (dhcpsAddressHookRtn != NULL)
        (* dhcpsAddressHookRtn) (DHCPS_STORAGE_WRITE, pName,
                                 pStartIp, pEndIp, pParams);

    return (OK);
    }

/*******************************************************************************
* 
* dhcpsLeaseHookAdd - assign a permanent lease storage hook for the server
* 
* This routine allows the server to access some form of permanent storage
* that it can use to store current lease information across restarts.  The 
* only argument to dhcpsLeaseHookAdd() is a pointer to a storage routine 
* with the following interface: 
* .CS
*     STATUS dhcpsStorageHook (int op, char *buffer, int datalen);
* .CE
* The first parameter of the storage routine specifies one of the following 
* operations: 
* 
*     DHCPS_STORAGE_START
*     DHCPS_STORAGE_READ
*     DHCPS_STORAGE_WRITE
*     DHCPS_STORAGE_STOP
*     DHCPS_STORAGE_CLEAR
* 
* In response to START, the storage routine should prepare to return data or 
* overwrite data provided by earlier WRITEs.  For a WRITE the storage routine 
* must save the contents of the buffer to permanent storage.  For a READ, the 
* storage routine should copy data previously stored into the provided buffer 
* as a NULL-terminated string in FIFO order.  For a CLEAR, the storage routine 
* should discard currently stored data.  After a CLEAR, the READ operation
* must return ERROR until additional data is stored.  For a STOP, the storage 
* routine must handle cleanup.  After a STOP, READ and WRITE operations must
* return error until a START is received.  Each of these operations must 
* return OK if successful, or ERROR otherwise.
* 
* Before the server is initialized, VxWorks automatically calls 
* dhcpsLeaseHookAdd(), passing in the routine name defined by DHCPS_LEASE_HOOK.
* 
* RETURNS: OK, or ERROR if routine is NULL.
* 
* ERRNO: N/A
*/

STATUS dhcpsLeaseHookAdd
    (
    FUNCPTR pCacheHookRtn 	/* routine to store/retrieve lease records */
    )
    {
    if (pCacheHookRtn != NULL)
        {
        dhcpsLeaseHookRtn = pCacheHookRtn;
        return (OK);
        }
    return (ERROR);
    }

/*******************************************************************************
*
* dhcpsAddressHookAdd - assign a permanent address storage hook for the server
*
* This routine allows the server to access some form of permanent storage
* to preserve additional address entries across restarts.  This routine is
* not required, but leases using unsaved addresses are not renewed.  The 
* only argument provided is the name of a function with the following interface:
* .CS
*     STATUS dhcpsAddressStorageHook (int op,
*                                     char *name, char *start, char *end, 
*                                     char *params);
* .CE
* The first parameter of this storage routine specifies one of the following 
* operations: 
*
*     DHCPS_STORAGE_START
*     DHCPS_STORAGE_READ
*     DHCPS_STORAGE_WRITE
*     DHCPS_STORAGE_STOP 
*
* In response to a START, the storage routine should prepare to return
* data or overwrite data provided by earlier WRITE operations.  For a 
* WRITE, the storage routine must save the contents of the four buffers 
* to permanent storage.  Those buffers contain the NULL-terminated 
* strings received by the dhcpsLeaseEntryAdd() routine.  For a READ, the
* storage routine should copy previously stored data (as NULL-terminated 
* strings) into the provided buffers in the order received by earlier WRITE
* operations.  For a STOP, the storage routine should do any necessary cleanup. 
* After a STOP, the storage routine should return an ERROR for all operations 
* except START.  However, the STOP operation does not normally occur since
* the server only deliberately exits following an unrecoverable error.  This
* storage routine must not rely on that operation to handle READ, WRITE, or
* new START attempts.
* 
* The storage routine should return OK if successful, ERROR otherwise.
*
* Note that, unlike the lease storage routine, there is no CLEAR operation.
*
* Before the server is initialized, VxWorks calls this routine automatically 
* passing in the function named in DHCPS_ADDRESS_HOOK.
*
* RETURNS: OK, or ERROR if function pointer is NULL.
*
* ERRNO: N/A
*/

STATUS dhcpsAddressHookAdd
    (
    FUNCPTR pCacheHookRtn 	/* routine to store/retrieve lease entries */
    )
    {
    /* 
     * The hook routine is deliberately assigned before testing for NULL
     * so that this routine can be used to delete an existing address hook.
     * In that case, the user would just ignore the return value of ERROR.
     */

    dhcpsAddressHookRtn = pCacheHookRtn;
    if (pCacheHookRtn == NULL)
        return (ERROR);
    return (OK);
    }

/*******************************************************************************
*
* alloc_sbuf - create message transmission buffers
*
* This routine creates two transmission buffers for storing outgoing DHCP
* messages.  The first buffer has a fixed size equal to that of a standard 
* DHCP message as defined in the RFC.  The second buffer contains the
* remaining number of bytes needed to send the largest supported message.
* That buffer is only used if additional space for DHCP options is needed
* and if the requesting client accepts larger DHCP messages.  The two buffers
* are stored in a global "buffer vector" structure and are also accessible
* through individual global variables.
* 
* RETURNS: N/A
* 
* ERRNO: N/A
*
* NOMANUAL
*/

LOCAL STATUS alloc_sbuf
    (
    int 	maxSize 	/* largest supported message size, in bytes */
    )
    {
    char *pSendBuf;
    int sbufsize;

    /*
     * Allocate transmission buffer using aligned memory to provide 
     * IP header alignment needed by Sun BSP's. 
     */

    sbufsize = IPHL + UDPHL + DFLTDHCPLEN;
    pSendBuf = (char *)memalign (4, maxSize);
    if (pSendBuf == NULL)
        return (-1);

    dhcpsSendBuf = pSendBuf;    /* Buffer for default DHCP message. */
    bzero (dhcpsSendBuf, sbufsize);

    sbufvec[0].iov_len = IPHL + UDPHL + DFLTDHCPLEN;

    sbufvec[0].iov_base = dhcpsSendBuf;
    dhcpsMsgOut.ip = (struct ip *)dhcpsSendBuf;
    dhcpsMsgOut.udp = (struct udphdr *)&dhcpsSendBuf [IPHL];
    dhcpsMsgOut.dhcp = (struct dhcp *)&dhcpsSendBuf [IPHL + UDPHL];

    dhcpsOverflowBuf = &pSendBuf [sbufsize];  /* Buffer for larger messages */
    sbufvec[1].iov_base = dhcpsOverflowBuf;
    sbufvec[1].iov_len = 0;   /* Contains length of bytes in use, not size. */

    return (OK);
    }

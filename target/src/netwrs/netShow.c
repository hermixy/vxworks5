/* netShow.c - network information display routines */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03v,25jun02,ann  making a temporary change to the 64 bit counter lookups
03u,10may02,kbw  making man page edits
03t,22apr02,rae  inetstatShow warns if TCP or UDP show routine absent
                 (SPR #72838)
03s,28mar02,vvv  fixed problem with route show routines executed over the
		 network (SPR #69286/73759)
03r,07dec01,rae  merge from synth ver 03z (SPR #67790)
03q,15oct01,rae  merge from truestack ver 03x, base 03j (SPRs 69550,
                 67790, 67148, 65788 etc.)
03p,07feb01,spm  added internal documentation for parallel host shell routines
03o,15jan01,zhu  SPR#63298
03x,15aug01,vvv  added entries for all interface types in pIfTypes 
		 array (SPR #69550)
03w,13jul01,ann  eliminated the global mibStyle flag by incorporating it in
                 the END object's flags
03v,27jun01,spm  consolidated and updated route show routines; replaced use
                 of visibility flag for duplicate routes
03u,19jun01,rae  merged venus changes from tor3_x ver 03t
		 SPRs 67790 67148 65788
03t,01may01,niq  Correcting the mod history
03s,01may01,niq  Merging Fastpath changes from tor2_0.open_stack branch 
                 version 04i (netShow.c@@/main/tor2_0_x/tor2_0_0.sustaining/
                 tor2_0.open_stack/26)
03r,29mar01,spm  merged changes from version 04h of tor2_0.open_stack
                 branch (wpwr VOB, base 03q) for unified code base
03q,13mar01,rae  merged from 03p of tor2_0.barney branch (base: 03h)
		 SPR#63298, SPR#27395, RFC2233, etc.
03p,09mar01,ham  fixed output format in rtEntryPrint() routine (SPR #64321)
03o,24feb01,ham  fixed S_objLib_OBJ_ID_ERROR return (SPR 64383).
03n,20feb01,ham  fixed printf's format in rtEntryPrint() (SPR #63298).
03m,03nov00,ham  doc: added an example in netPoolShow.
03l,25oct00,ham  doc: cleanup for vxWorks AE 1.0.
03k,19apr00,ham  merged TOR2_0-NPT-FCS(03j,08nov99,pul).
                 fixed incorrect printf (SPR #26699).
03j,08nov99,pul  T2 cumulative patch 2
03i,10may99,spm  fixed arpShow routine to display proxy entries (SPR #24397)
03h,26aug98,fle  doc : added .CS in EXAMPLE of arpShow header
03g,14dec97,jdi  doc: cleanup.
03f,10dec97,kbw  minor man page fixes
03e,03dec97,vin  added netStackDataPoolShow() netStackSysPoolShow().
03d,26oct97,kbw  minor man page fixes
03c,26sep97,rjc  fix for SPR 9326
03b,26aug97,spm  removed compiler warnings (SPR #7866)
03a,12aug97,vin  added netpool specific stuff for mbufShow.
02z,12aug97,rjc  added if type to ifShow SPR7555.
02y,11aug97,vin  changed clPoolShow.
02x,01aug97,kbw  fix man page problems found in beta review
02w,07jul97,rjc  modifications for newer version of rtentry.
02v,30jun97,rjc  restored old rtentry structure size.
02u,03jun97,rjc  stopped converting all 0 mask to all 1.
02t,02jun97,spm  corrected modification history entry
02s,29may97,rjc  fixed mRouteShow byte order in mask.
02r,08mar97,vin  added changes to accomodate changes in pcb structure.
		 moved out igmp, icmp, udp, tcp code to igmpShow, icmpShow,
                 udpShow, tcpShow.
02q,07apr97,spm  removed DHCP client dependencies
02p,13feb97,rjc  added mRouteShow
02o,31jan97,vin  fixed a bug in clPoolShow().
02n,31jan97,spm  moved DHCP client show routines to dhcpShowLib.c
02m,29jan97,spm  made global variables for DHCP client unique.
02l,29jan97,spm  removed conditional includes from DHCP client code.
02k,07jan97,vin  added unknown protocol in ipstatShow.
02j,05dec96,vin  added new mbufTypes MT_IPMADDR,MT_IFMADDR,MT_MRTABLE.
02i,03dec96,spm  correcting errors in DHCP client show routines.
02h,27nov96,spm  added show routines for DHCP client.
02g,17nov96,vin	 modified show routines to include new clusters.
		 deleted cluster related fields from mbufShow().
02f,11sep96,vin  modified for BSD4.4.
02e,13mar95,dzb  changed tcpDebugShow() to call tcpReportRtn (SPR #3928).
02d,18jan95,rhp  add pointers to literature in library man page (SPR#3591)
02c,18jan95,jdi  doc cleanup for tcpDebugShow().
02b,11aug94,dzb  added tcpDebugShow() to get at tcp_debug.c:tcp_report().
                 warning cleanup.
02a,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01z,02feb93,jdi  documentation cleanup for 5.1.
01y,13nov92,dnw  added include of semLibP.h
01x,12oct92,jdi  fixed mangen problem in routestatShow().
01w,04sep92,jmm  fixed typo in printf in inpcbPrint()
                 changed field width in inetPrint() to 15 from 12 (spr 1470)
01v,29jul92,smb  changed putchar() to printf().
01u,27jul92,elh  Moved routeShow and hostShow here.
01t,04jul92,smb  added include vxWorks.h
01s,26jun92,wmd  modified inetPrint() to always print port address. 
01r,11jun92,elh	 moved arpShow here from arpLib.
01q,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01p,26may92,kdl  changed ifShow() to more robustly avoid dereferencing
		 null ptr (SPR #1389).
01o,01jan92,elh  changed arptabShow to call arpShow.
01n,13dec91,gae  ANSI fixes.
01m,17nov91,elh	 added routestatShow().
01l,07nov91,elh	 changed calls to inet_ntoa to inet_ntoa_b, so memory isn't lost.
01k,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01j,14aug91,jrb  (intel) improved instrumentation for udp in udpstatShow command
		 with the following fields: udptotal - total packets,
		 ipackets - input, opackets - output, fullsock - socket
		 overflows, noportbcast - broadcast port messages with no
		 listener. Taken from latest BSD reno udp.
01i,17jun91,del  added include of strLib.h.
01h,07may91,jdi	 documentation tweaks.
01g,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by elh.
01f,20feb91,jaa	 documentation.
01e,08oct90,hjb  moved include of "ip.h" above "tcp.h" to avoid redefinition of
		 IP_MSS.
01d,02oct90,hjb  deleted "netdb.h".  made ipstatShow() void.
01c,12aug90,hjb  added arptabShow().
01b,10aug90,kdl  added forward declarations for functions returning void.
01a,26jun90,hjb  created by moving Show routines out of various files.
*/

/*
DESCRIPTION
This library provides routines to show various network-related
statistics, such as configuration parameters for network interfaces,
protocol statistics, socket statistics, and so on.

Interpreting these statistics requires detailed knowledge of Internet
network protocols.  Information on these protocols can be found in
the following books:
.iP
.I "Internetworking with TCP/IP Volume III,"
by Douglas Comer and David Stevens
.iP
.I "UNIX Network Programming,"
by Richard Stevens
.iP
.I "The Design and Implementation of the 4.3 BSD UNIX Operating System,"
by Leffler, McKusick, Karels and Quarterman

.LP
The netShowInit() routine links the network show facility into the VxWorks
system.  This is performed automatically if INCLUDE_NET_SHOW is defined.
If you want inetstatShow() to display TCP socket status, then INCLUDE_TCP_SHOW
needs to be included.


INTERNAL
Some routines within this module are preempted by parallel versions
built in to the Tornado shell source code in the $WIND_BASE/host/src/windsh
directory. Changes to these routines must be duplicated within the shell to
maintain consistent output. The versions in this module can still be
accessed by prepending an "@" sign to the routine name (e.g. "@ifShow").

Currently, the following routines are preempted:

     ifShow, inetstatShow, ipstatShow, routestatShow, hostShow

SEE ALSO: ifLib, icmpShow, igmpShow, tcpShow, udpShow 

INTERNAL
Some routines within this module are preempted by parallel versions
built in to the Tornado shell source code in the $WIND_BASE/host/src/windsh
directory. Changes to these routines must be duplicated within the shell to
maintain consistent output. The versions in this module can still be
accessed by prepending an "@" sign to the routine name (e.g. "@ifShow").

Currently, the following routines are preempted:

     ifShow, inetstatShow, ipstatShow, routestatShow, hostShow     

*/

#include "vxWorks.h"
#include "sys/types.h"
#include "netinet/in.h"
#include "net/socketvar.h"
#include "sys/socket.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "net/route.h"
#include "net/radix.h"
#include "netinet/in_pcb.h"
#include "hostLib.h"
#include "net/if.h"
#include "net/if_dl.h"
#include "netinet/if_ether.h"
#include "netinet/in_var.h"
#include "errno.h"
#include "net/mbuf.h"
#include "inetLib.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "arpLib.h"
#include "private/semLibP.h"
#include "routeEnhLib.h"
#include "private/muxLibP.h"
#include "private/memPartLibP.h"
#include "end.h"
#include "ipProto.h"
#include "math.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsIp.h" 	/* for _in_ifaddr definition */
#include "netinet/vsShow.h" 	/* for _pTcpPcbHead and _pUdpPcbHead */
#include "netinet/vsHost.h" 	/* for hostList and hostListSem */
#include "netinet/vsRadix.h" 	/* for rt_tables definition */
#endif /* VIRTUAL_STACK */

/* externs */

#ifndef VIRTUAL_STACK
IMPORT struct rtstat	rtstat;		/* routing statistics */
IMPORT struct ifnet 	*ifnet;		/* list of all network interfaces */
IMPORT struct in_ifaddr *in_ifaddr; 	/* list of addr for net interfaces */
IMPORT struct ipstat	ipstat;
IMPORT struct radix_node_head *rt_tables[]; /* table of radix nodes */

IMPORT LIST 		hostList;
IMPORT SEM_ID 		hostListSem;
IMPORT NET_POOL_ID	_pNetDpool;

IMPORT BOOL              bufferedRtShow;         /* buffered route printing */
IMPORT UINT              rtMem;                  /* memory for route display */

/* globals */

struct inpcbhead *	_pUdpPcbHead  = NULL;	/* initialized in udpShow.c */
struct inpcbhead *	_pTcpPcbHead  = NULL;	/* initialized in tcpShow.c */
#endif /* VIRTUAL_STACK */

VOIDFUNCPTR		_pTcpPcbPrint =	NULL;	/* initialized in tcpShow.c */

/* locals */
LOCAL char *  pIfTypes [] =      /* interface types from mib-2 */
    {
    "UNDEFINED",
    "OTHER", 
    "REGULAR_1822", 
    "HDH1822", 
    "DDN_X25", 
    "RFC877_X25", 
    "ETHERNET_CSMACD", 
    "ISO88023_CSMACD", 
    "ISO88024_TOKENBUS", 
    "ISO88025_TOKENRING", 
    "ISO88026_MAN", 
    "STARLAN", 
    "PROTEON_10MBIT", 
    "PROTEON_80MBIT", 
    "HYPERCHANNEL", 
    "FDDI", 
    "LAPB", 
    "SDLC", 
    "DS1", 
    "E1", 
    "BASIC_ISDN", 
    "PRIMARY_ISDN", 
    "PROP_POINT_TO_POINT_SERIAL", 
    "PPP", 
    "SOFTWARE_LOOPBACK", 
    "EON", 
    "ETHERNET_3MBIT", 
    "NSIP", 
    "SLIP", 
    "ULTRA", 
    "DS3", 
    "SIP", 
    "FRAME_RELAY", 
    "RS232",
    "PARA",
    "ARCNET",
    "ARCNET_PLUS",
    "ATM",
    "MIOX25",
    "SONET",
    "X25PLE",
    "ISO88022LLC",
    "LOCAL_TALK",
    "SMDS_DXI",
    "FRAME_RELAY_SERVICE",
    "V35",
    "HSSI",
    "HIPPI",
    "MODEM",
    "AAL5",
    "SONET_PATH",
    "SONET_VT",
    "SMDS_ICIP",
    "PROP_VIRTUAL",
    "PROP_MULTIPLEXOR",
    "IEEE80212",
    "FIBRE_CHANNEL",
    "HIPPI_INTERFACE",
    "FRAME_RELAY_INTERCONNECT",
    "AFLANE8023",
    "AFLANE8025",
    "CCT_EMUL",
    "FAST_ETHER",
    "ISDN",
    "V11",
    "V36",
    "G703AT64K",
    "G703AT2MB",
    "QLLC",
    "FAST_ETHER_FX",
    "CHANNEL",
    "IEEE80211",
    "IBM370PAR_CHAN",
    "ESCON",
    "DLSW",
    "ISDNS",
    "ISDNU",
    "LAPD",
    "IP_SWITCH",
    "RSRB",
    "ATM_LOGICAL",
    "DS0",
    "DS0_BUNDLE",
    "BSC",
    "ASYNC",
    "CNR",
    "ISO88025_DTR",
    "EPLRS",
    "ARAP",
    "PROP_CNLS",
    "HOST_PAD",
    "TERM_PAD",
    "FRAME_RELAY_MPI",
    "X213",
    "ADSL",
    "RADSL",
    "SDSL",
    "VDSL",
    "ISO88025_CRFP_INT",
    "MYRINET",
    "VOICE_EM",
    "VOICE_FXO",
    "VOICE_FXS",
    "VOICE_ENCAP",
    "VOICE_OVER_IP",
    "ATM_DXI",
    "ATM_FUNI",
    "ATM_IMA",
    "PPP_MULTILINK_BUNDLE",
    "IP_OVER_CDLC",
    "IP_OVER_CLAW",
    "STACK_TO_STACK",
    "VIRTUAL_IP_ADDRESS",
    "MPC",
    "IP_OVER_ATM",
    "ISO88025_FIBER",
    "TDLC",
    "GIGABIT_ETHERNET",
    "HDLC",
    "LAPF",
    "V37",
    "X25MLP",
    "X25HUNT_GROUP",
    "TRASNP_HDLC",
    "INTERLEAVE",
    "FAST",
    "IP",
    "DOCS_CABLE_MACLAYER",
    "DOCS_CABLE_DOWNSTREAM",
    "DOCS_CABLE_UPSTREAM",
    "A12_MPP_SWITCH",
    "TUNNEL",
    "COFFEE",
    "CES",
    "ATM_SUB_INTERFACE",
    "L2VLAN",
    "L3IPVLAN",
    "L3IPXVLAN",
    "DIGITAL_POWERLINE",
    "MEDIA_MAIL_OVER_IP",
    "DTM",
    "DCN",
    "IP_FORWARD",
    "MSDSL",
    "IEEE1394",
    "IF_GSN",
    "DVB_RCC_MAC_LAYER",
    "DVB_RCC_DOWNSTREAM",
    "DVB_RCC_UPSTREAM",
    "ATM_VIRTUAL",
    "MPLS_TUNNEL",
    "SRP",
    "VOICE_OVER_ATM",
    "VOICE_OVER_FRAME_RELAY",
    "IDSL",
    "COMPOSITE_LINK",
    "SS7_SIG_LINK",
    "PMP",
     };

LOCAL int bufIndex;                 /* index into route buffer */
LOCAL char *routeBuf;               /* buffer to store routing table */

/* forward static functions */

static void ifAddrPrint (char *str, struct sockaddr *addr);
static void ifFlagPrint (u_short flag);
static void ifEtherPrint (u_char enaddr [ 6 ]);
static void inetPrint (struct in_addr *in, u_short port);
LOCAL BOOL routeEntryPrint (struct rtentry *, void *, BOOL);
static void routeTableShow (int rtType);
static void inpcbPrint (char *name, struct inpcb *pInpcb);
static int routeNodeShow (struct radix_node* rn, void* w);


#define RT_ENTRY_LEN    90
#define plural(num)	((num) > 1 ? "s": "")
#define RT_NET		0x001			/* print network route */
#define RT_HST		0x010			/* print host route */
#define RT_ARP		0x100			/* print arp route entry */


/*******************************************************************************
*
* ifShow - display the attached network interfaces
*
* This routine displays the attached network interfaces for debugging and
* diagnostic purposes.  If <ifName> is given, only the interfaces belonging
* to that group are displayed.  If <ifName> is omitted, all attached
* interfaces are displayed.
*
* For each interface selected, the following are shown: Internet
* address, point-to-point peer address (if using SLIP), broadcast address,
* netmask, subnet mask, Ethernet address, route metric, maximum transfer
* unit, number of packets sent and received on this interface, number of
* input and output errors, and flags (such as loopback, point-to-point,
* broadcast, promiscuous, ARP, running, and debug).
*
* EXAMPLE:
* The following call displays all interfaces whose names begin with "ln",
* (such as "ln0", "ln1", and "ln2"):
* .CS
*     -> ifShow "ln"
* .CE
* The following call displays just the interface "ln0":
* .CS
*     -> ifShow "ln0"
* .CE
*
* RETURNS: N/A
*
* SEE ALSO: routeShow(), ifLib
*
* INTERNAL
* When using the Tornado shell, this routine is only available if an
* "@" sign is prepended to the routine name. Otherwise, it is preempted
* by a built-in version.
*/

void ifShow
    (
    char *ifName                /* name of the interface to show */
    )
    {
    FAST struct ifnet 		*ifp;
    FAST struct ifaddr 		*ifa;
    FAST struct in_ifaddr 	*ia = 0;
    FAST int			 nameLen;
    BOOL 			 found;
    int				 ifUnit;
    IP_DRV_CTRL *                pDrvCtrl = NULL;
    END_OBJ *                    pEnd;

#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
#endif /* VIRTUAL_STACK */

    found = FALSE;

#ifdef VIRTUAL_STACK
    for (ifp = _ifnet;  ifp != NULL;  ifp = ifp->if_next)
#else
    for (ifp = ifnet;  ifp != NULL;  ifp = ifp->if_next)
#endif /* VIRTUAL_STACK */
	{
	nameLen = strlen (ifp->if_name);

	/*
	 * If no name is specified, every interface is a match.
	 */

	if (ifName != NULL)		/* if a name was specified */
	    {
	    if (strncmp (ifName, ifp->if_name, nameLen) != 0)
	    	continue;		/* no match - wrong name */

	    /*
	     * If the user specified unit ID in ifName compare the unit number
	     */
	    if (nameLen < strlen (ifName))
	    	{
	    	sscanf (&ifName [nameLen], "%d", &ifUnit);
	    	if (ifp->if_unit != ifUnit)
		    continue;		/* no match - wrong unit ID */
		}
	    }

	found = TRUE;
#ifdef VIRTUAL_STACK
	printf ("%s (unit number %d) (stack number %d):\n", ifp->if_name,
                ifp->if_unit, ifp->vsNum);
#else
	printf ("%s (unit number %d):\n", ifp->if_name, ifp->if_unit);
#endif /* VIRTUAL_STACK */
	ifFlagPrint (ifp->if_flags);
	printf ("     Type: %s\n", pIfTypes [ifp->if_type]);
	for (ifa = ifp->if_addrlist;  ifa != NULL;  ifa = ifa->ifa_next)
	    {
	    if (ifa->ifa_addr->sa_family == AF_INET)
		{
                /*
                 * Every element in the if_addrlist (type struct ifaddr) with
                 * family AF_INET is actually the first part of a larger
                 * in_ifaddr structure which includes the netmask and subnet
                 * mask. Access that structure to display those values.
                 */

                ia = (struct in_ifaddr *)ifa;

		ifAddrPrint ("Internet address", ifa->ifa_addr);

		if (ifp->if_flags & IFF_POINTOPOINT)
		    ifAddrPrint ("Destination Internet address",
                                  ifa->ifa_dstaddr);
		else if (ifp->if_flags & IFF_BROADCAST)
		    ifAddrPrint ("Broadcast address", ifa->ifa_broadaddr);

                printf ("     Netmask 0x%lx Subnetmask 0x%lx\n", 
                        ia->ia_netmask, ia->ia_subnetmask);
		}
	    }


        /*
	 * if the interface is not LOOPBACK or SLIP, print the link
	 * level address.
	 */
	if (!(ifp->if_flags & IFF_POINTOPOINT) &&
	    !(ifp->if_flags & IFF_LOOPBACK))
	    ifEtherPrint (((struct arpcom *)ifp)->ac_enaddr);

	printf("     Metric is %lu\n", ifp->if_metric);
	printf("     Maximum Transfer Unit size is %lu\n", ifp->if_mtu);

        pDrvCtrl = (IP_DRV_CTRL *)ifp->pCookie;
        /*
         * If the ifnet does not have a cookie then this is
         * a BSD 4.x  interface.
         */
        if (pDrvCtrl == NULL)
            {
            printf("     %lu packets received; %lu packets sent\n",
                   ifp->if_ipackets, ifp->if_opackets);
            printf("     %lu multicast packets received\n", ifp->if_imcasts);
            printf("     %lu multicast packets sent\n", ifp->if_omcasts);
            printf("     %lu input errors; %lu output errors\n",
                   ifp->if_ierrors, ifp->if_oerrors);
            }
        else /* END style interface. */
            {
            M2_DATA    *pIfMib;

            pEnd = PCOOKIE_TO_ENDOBJ(pDrvCtrl->pIpCookie);
            if (pEnd == NULL)
                continue;
            pIfMib = &pEnd->pMib2Tbl->m2Data;

            if (pEnd->flags & END_MIB_2233)

                {
	        printf ("     %lu octets received\n", pIfMib->mibIfTbl.ifInOctets);
	        printf ("     %lu octets sent\n", pIfMib->mibIfTbl.ifOutOctets);
	        printf ("     %lu unicast packets received\n", 
				            pIfMib->mibIfTbl.ifInUcastPkts);
	        printf ("     %lu unicast packets sent\n", 
				            pIfMib->mibIfTbl.ifOutUcastPkts);
	        printf ("     %lu non-unicast packets received\n", 
				            pIfMib->mibIfTbl.ifInNUcastPkts);
	        printf ("     %lu non-unicast packets sent\n", 
				            pIfMib->mibIfTbl.ifOutNUcastPkts);
	        printf ("     %lu multicast packets received\n", 
				            pIfMib->mibXIfTbl.ifInMulticastPkts);
	        printf ("     %lu multicast packets sent\n", 
				            pIfMib->mibXIfTbl.ifOutMulticastPkts);
	        printf ("     %lu broadcast packets received\n", 
				            pIfMib->mibXIfTbl.ifInBroadcastPkts);
	        printf ("     %lu broadcast packets sent\n", 
				            pIfMib->mibXIfTbl.ifOutBroadcastPkts);
	        printf ("     %lu incoming packets discarded\n", 
				            pIfMib->mibIfTbl.ifInDiscards);
	        printf ("     %lu outgoing packets discarded\n", 
				            pIfMib->mibIfTbl.ifOutDiscards);
	        printf ("     %lu incoming errors\n", 
				            pIfMib->mibIfTbl.ifInErrors);
	        printf ("     %lu outgoing errors\n", 
				            pIfMib->mibIfTbl.ifOutErrors);
	        printf ("     %lu unknown protos\n", 
				            pIfMib->mibIfTbl.ifInUnknownProtos);
		}
            else /* (RFC1213 style counters supported) XXX */
                {
                /*
                 * XXX This change was made in order to fix inconsistant
                 * statistics between MIB2 and ifnet due to END currently
                 * does not increment MIB2 statistics. However this is
                 * temporary fix and should be rolled back when ifnet{} 
                 * gets obsolete eventually.
                 */

                printf("     %lu octets received\n",
                       ifp->if_ipackets);
                printf("     %lu octets sent\n",
                       ifp->if_opackets);
                printf("     %lu packets received\n",
                       ifp->if_ipackets - ifp->if_ierrors);
                printf("     %lu packets sent\n",
                       ifp->if_opackets - ifp->if_oerrors);
                printf("     %lu non-unicast packets received\n",
                       ifp->if_imcasts);
                printf("     %lu non-unicast packets sent\n",
                       ifp->if_omcasts);
                printf("     %lu unicast packets received\n",
                       ifp->if_ipackets - ifp->if_imcasts);
                printf("     %lu unicast packets sent\n",
                       ifp->if_opackets - ifp->if_omcasts);
                printf("     %lu input discards\n",
                       ifp->if_iqdrops);
                printf("     %lu input unknown protocols\n",
                       ifp->if_noproto);
                printf("     %lu input errors\n",
                       ifp->if_ierrors);
                printf("     %lu output errors\n",
                       ifp->if_oerrors);
                }
            }

	    /* print collisions and drops for old BSD or iso88023_csmacd 
	     * or ethernet_csmacd
	     */
            if (pDrvCtrl == NULL  
		    || ifp->if_data.ifi_type==M2_ifType_iso88023_csmacd 
	            || ifp->if_data.ifi_type==M2_ifType_ethernet_csmacd )
	       {
                printf("     %ld collisions; %ld dropped\n",
                ifp->if_collisions, ifp->if_iqdrops);
	       }
        }
    if (found == FALSE)
	{
    	if (ifName != NULL)
	    printErr("\"%s\" - No such interface\n", ifName);
    	else
	    printErr("No network interface active.\n");
	}
    }

/*******************************************************************************
*
* ifAddrPrint - print Internet address for the interface
*
* Prints the Internet address (for the network interfaces)
* in readable format.
*
* RETURNS: N/A.
*/

LOCAL void ifAddrPrint
    (
    char                *str,   /* comment string to be printed */
    struct sockaddr     *addr   /* internet address for the interface */
    )
    {
    char 		addrString [INET_ADDR_LEN];

    inet_ntoa_b (((struct sockaddr_in *)addr)->sin_addr, addrString);
    printf("     %s: %s\n", str, addrString);
    }

/*******************************************************************************
*
* ifFlagPrint - print flags for the network interface
*
* ifFlagPrint prints each of the flags specified for the network
* interfaces in user-readable format.
*
* RETURNS: N/A.
*/

LOCAL void ifFlagPrint
    (
    FAST ushort flag                       /* network interface flags */
    )
    {
    printf("     Flags: (0x%x) ", flag);

    if (flag & IFF_UP)
	printf ("UP ");
    else
	/* shouldn't happen if the interface is deleted properly */
	printf ("DOWN ");

    if (flag & IFF_LOOPBACK)
	printf ("LOOPBACK ");

    if (flag & IFF_POINTOPOINT)
	printf ("POINT-TO-POINT ");

    if (flag & IFF_BROADCAST)
	printf ("BROADCAST ");

    if (flag & IFF_MULTICAST)
	printf ("MULTICAST "); 

    if (flag & IFF_PROMISC)
	printf ("PROMISCUOUS ");

    if (!(flag & IFF_NOTRAILERS))
	printf ("TRAILERS ");

    if (!(flag & IFF_NOARP))
	printf ("ARP ");

    if (flag & IFF_RUNNING)
	printf ("RUNNING ");

    if (flag & IFF_DEBUG)
	printf ("DEBUG ");

    if (flag & IFF_FP_ENABLE)
	printf ("FASTPATH ");

    printf ("\n");
    }

/*******************************************************************************
*
* ifEtherPrint - print Ethernet address in ":" notation
*
* RETURNS: N/A.
*/

LOCAL void ifEtherPrint
    (
    u_char enaddr[6]            /* ethernet address */
    )
    {
    unsigned char *en = (unsigned char *)enaddr;

    printf("     Ethernet address is %02x:%02x:%02x:%02x:%02x:%02x\n",
	   en [0], en [1], en [2], en [3], en [4], en [5]);
    }

/*******************************************************************************
*
* inetPrint - Pretty print an Internet address (net address + port).
*
* RETURNS: N/A.
*/

LOCAL void inetPrint
    (
    FAST struct in_addr *in,
    u_short port
    )
    {
    char line[80];
    char *cp;
    int width;
    char addrString [INET_ADDR_LEN];

    inet_ntoa_b ((*in), addrString);
    (void) sprintf (line, "%.*s.", 15, addrString);
    cp = index (line, '\0');

    (void) sprintf (cp, "%d", ntohs((u_short)port));

    width = 21;
    printf (" %-*.*s", width, width, line);
    }

/*******************************************************************************
*
* inetstatShow - display all active connections for Internet protocol sockets
*
* This routine displays a list of all active Internet protocol sockets in a
* format similar to the UNIX `netstat' command.
*
* If you want inetstatShow() to display TCP socket status, then
* INCLUDE_TCP_SHOW needs to be included.
*
* RETURNS: N/A
*
* INTERNAL
* When using the Tornado shell, this routine is only available if an
* "@" sign is prepended to the routine name. Otherwise, it is preempted
* by a built-in version.
*/

void inetstatShow (void)
    {
#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
    printf("Active Internet connections (including servers) (stack number %d)\n", myStackNum);
#else
    printf("Active Internet connections (including servers)\n");
#endif /* VIRTUAL_STACK */
    printf("%-8.8s %-5.5s %-6.6s %-6.6s  %-18.18s %-18.18s %s\n",
	   "PCB", "Proto", "Recv-Q", "Send-Q",
	   "Local Address", "Foreign Address", "(state)");
    printf("%-8.8s %-5.5s %-6.6s %-6.6s  %-18.18s %-18.18s %s\n",
	   "--------", "-----", "------", "------", "------------------",
    	   "------------------", "-------");
    if (_pTcpPcbHead != NULL)
        inpcbPrint ("TCP", _pTcpPcbHead->lh_first);
    else
        printf("No TCP show routine included.  TCP connections not shown.\n");
    
    if (_pUdpPcbHead != NULL)
        inpcbPrint ("UDP", _pUdpPcbHead->lh_first);
    else
        printf("No UDP show routine included.  UDP connections not shown.\n");
#ifdef VIRTUAL_STACK
    inpcbPrint ("RAW", ripcb.lh_first);
#endif /* VIRTUAL_STACK */

    }

/*******************************************************************************
*
* inpcbPrint - print a list of protocol control blocks for an Internet protocol
*
* Prints a list of active protocol control blocks for the specified
* Internet protocols in a nice and pretty format.
*
* RETURNS: N/A.
*/

LOCAL void inpcbPrint
    (
    char *name,
    struct inpcb *pInpcb
    )
    {
    FAST struct inpcb *inp;
    struct socket *pSock;
    BOOL isTcp = 0;

    if (strcmp (name, "TCP") == 0)
        isTcp = 1;

    for (inp = pInpcb; inp != NULL; inp = inp->inp_list.le_next)
	{
	pSock = inp->inp_socket;

	/*
	 * Print "inp" here to allow manual deletion of the stale
	 * PCB's by using in_pcbdetach (inp).
	 */
	printf ("%-8x ", (u_int) inp);

	printf("%-5.5s %6ld %6ld ", name, pSock->so_rcv.sb_cc,
	       pSock->so_snd.sb_cc);
	inetPrint (&(inp->inp_laddr), inp->inp_lport);
	inetPrint (&(inp->inp_faddr), inp->inp_fport);

        /* if tcp is included then only print the tcp information */
        
        if (isTcp && (_pTcpPcbPrint != NULL))
            (*_pTcpPcbPrint) (inp); 

	(void) printf ("\n");
	}
    }

/*******************************************************************************
*
* ipstatShow - display IP statistics
*
* This routine displays detailed statistics for the IP protocol.
*
* RETURNS: N/A
*
* INTERNAL
* When using the Tornado shell, this routine is only available if an
* "@" sign is prepended to the routine name. Otherwise, it is preempted
* by a built-in version.
*/

void ipstatShow
    (
    BOOL zero           /* TRUE = reset statistics to 0 */
    )
    {
    static char *ipstat_name[] =
	{
	"total",           "badsum",    "tooshort",     "toosmall",
	"badhlen",         "badlen",    "infragments",  "fragdropped", 
	"fragtimeout",     "forward",   "cantforward",  "redirectsent", 
	"unknownprotocol", "nobuffers",  "reassembled", "outfragments", 
	"noroute"
	};

    char *fmt = "%20s %4d\n";
    int ix = 0;

#ifdef VIRTUAL_STACK
    virtualStackIdCheck();

    printf (fmt, "stack number",
	myStackNum);                    /* stack number */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_total);             /* total packets received */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_badsum);            /* checksum bad */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_tooshort);          /* packet too short */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_toosmall);          /* not enough data */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_badhlen);           /* ip header length < data size */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_badlen);            /* ip length < ip header length */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_fragments);         /* fragments received */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_fragdropped);       /* frags dropped (dups, out of space) */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_fragtimeout);       /* fragments timed out */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_forward);           /* packets forwarded */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_cantforward);       /* packets rcvd for unreachable dest */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_redirectsent);      /* packets forwarded on same net */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_noproto);	        /* pkts recieved with unknown protos */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_odropped);	        /* pkts dropped for no buffers */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_reassembled);	/* total packets reassembled ok */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_ofragments);        /* output fragments created */
    printf (fmt, ipstat_name [ix++],
	_ipstat.ips_noroute);	        /* packets discarded due to no route */
    printf ("\n");

    if (zero)
	bzero ((char*)&_ipstat, sizeof (_ipstat));
#else
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_total);              /* total packets received */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_badsum);             /* checksum bad */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_tooshort);           /* packet too short */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_toosmall);           /* not enough data */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_badhlen);            /* ip header length < data size */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_badlen);             /* ip length < ip header length */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_fragments);          /* fragments received */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_fragdropped);        /* frags dropped (dups, out of space) */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_fragtimeout);        /* fragments timed out */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_forward);            /* packets forwarded */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_cantforward);        /* packets rcvd for unreachable dest */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_redirectsent);       /* packets forwarded on same net */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_noproto);	        /* pkts recieved with unknown protos */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_odropped);	        /* pkts dropped for no buffers */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_reassembled);	/* total packets reassembled ok */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_ofragments);	        /* output fragments created */
    printf (fmt, ipstat_name [ix++],
	ipstat.ips_noroute);	        /* packets discarded due to no route */

    printf ("\n");

    if (zero)
	bzero ((char*)&ipstat, sizeof (ipstat));
#endif /* VIRTUAL_STACK */

    }

/*******************************************************************************
*
* clPoolShow - show cluster pool information
*
* This function shows cluster pool information.
*
* NOMANUAL
*
* RETURNS: N/A
*/

LOCAL void clPoolShow
    (
    NET_POOL_ID	pNetPool		/* pointer the netPool */
    )
    {
    UCHAR 	clType; 
    CL_POOL_ID	pClPool;

    printf ("__________________\n"); 
    printf ("CLUSTER POOL TABLE\n"); 
    printf ("_______________________________________________________________________________\n");
    printf ("size     clusters  free      usage\n"); 
    printf ("-------------------------------------------------------------------------------\n");
    for (clType = pNetPool->clLg2Min; clType <= pNetPool->clLg2Max; clType++)
	{
	if ((pClPool = netClPoolIdGet (pNetPool, CL_LOG2_TO_CL_SIZE(clType),
                                        TRUE)) != NULL)
	    {
	    printf ("%-9d", pClPool->clSize); 
	    printf ("%-10d", pClPool->clNum); 
	    printf ("%-10d", pClPool->clNumFree);
	    printf ("%-14d\n", pClPool->clUsage); 
	    }
	}
    printf ("-------------------------------------------------------------------------------\n");
    }

/*******************************************************************************
*
* netPoolShow - show pool statistics
*
* This routine displays the distribution of `mBlk's and clusters in a given
* network pool ID.
*
* EXAMPLE:
*
* .CS
* void endPoolShow
*    (
*    char * devName,    /@ The inteface name: "dc", "ln" ...@/
*    int    unit        /@ the unit number: usually 0       @/
*    )
*    {
*    END_OBJ * pEnd;
* 
*    if ((pEnd = endFindByName (devName, unit)) != NULL)
*        netPoolShow (pEnd->pNetPool);
*    else
*        printf ("Could not find device %s\n", devName);
*    return;
*    }
* .CE
*
* RETURNS: N/A
*/

void netPoolShow
    (
    NET_POOL_ID pNetPool
    )
    {
    static int mt_types [NUM_MBLK_TYPES] =
	{ MT_FREE, 	MT_DATA, 	MT_HEADER, 	MT_SOCKET,
	  MT_PCB,	MT_RTABLE, 	MT_HTABLE, 	MT_ATABLE, 
	  MT_SONAME, 	MT_ZOMBIE,	MT_SOOPTS, 	MT_FTABLE,
	  MT_RIGHTS, 	MT_IFADDR,	MT_CONTROL,	MT_OOBDATA,
	  MT_IPMOPTS,	MT_IPMADDR,	MT_IFMADDR,	MT_MRTABLE
	};
    static char mt_names [NUM_MBLK_TYPES][10] =
	{ "FREE", 	"DATA", 	"HEADER", 	"SOCKET",
	  "PCB",	"RTABLE", 	"HTABLE", 	"ATABLE", 
	  "SONAME",	"ZOMBIE",	"SOOPTS",	"FTABLE",
	  "RIGHTS",	"IFADDR",	"CONTROL", 	"OOBDATA",
	  "IPMOPTS",	"IPMADDR",	"IFMADDR",	"MRTABLE"
	};
    int totalMbufs = 0;
    FAST int ix;

    if (pNetPool == NULL || pNetPool->pPoolStat == NULL)
        return ; 

    printf ("type        number\n");
    printf ("---------   ------\n");

    for (ix = 0; ix < NUM_MBLK_TYPES; ix++)
	{
	printf ("%-8s:    %3ld\n", mt_names [ix],
		pNetPool->pPoolStat->mTypes [mt_types [ix]]);
	totalMbufs += pNetPool->pPoolStat->m_mtypes [mt_types [ix]];
	}

    printf ("%-8s:    %3d\n", "TOTAL", totalMbufs);

    printf ("number of mbufs: %ld\n", pNetPool->pPoolStat->mNum);
    printf ("number of times failed to find space: %ld\n",
            pNetPool->pPoolStat->mDrops);
    printf ("number of times waited for space: %ld\n",
            pNetPool->pPoolStat->mWait);
    printf ("number of times drained protocols for space: %ld\n",
	    pNetPool->pPoolStat->mDrain);
    clPoolShow (pNetPool); 
    }
    
/*******************************************************************************
*
* netStackDataPoolShow - show network stack data pool statistics
*
* This routine displays the distribution of `mBlk's and clusters in a
* the network data pool.  The network data pool is used only for data
* transfer through the network stack.
*
* The "clusters" column indicates the total number of clusters of that size
* that have been allocated.  The "free" column indicates the number of
* available clusters of that size (the total number of clusters minus those
* clusters that are in use).  The "usage" column indicates the number of times
* clusters have been allocated (not, as you might expect, the number of
* clusters currently in use).
*
* RETURNS: N/A
*
* SEE ALSO: netStackSysPoolShow(), netBufLib
*/

void netStackDataPoolShow (void)
    {
#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
    printf("stack number %d\n", myStackNum);
#endif /* VIRTUAL_STACK */
    netPoolShow (_pNetDpool);
    }

/*******************************************************************************
*
* netStackSysPoolShow - show network stack system pool statistics
*
* This routine displays the distribution of `mBlk's and clusters in a
* the network system pool.  The network system pool is used only for system
* structures such as sockets, routes, interface addresses, protocol control
* blocks, multicast addresses, and multicast route entries.
*
* The "clusters" column indicates the total number of clusters of that size
* that have been allocated.  The "free" column indicates the number of
* available clusters of that size (the total number of clusters minus those
* clusters that are in use).  The "usage" column indicates the number of times
* clusters have been allocated (not, as you might expect, the number of
* clusters currently in use).
*
* RETURNS: N/A
*
* SEE ALSO: netStackDataPoolShow(), netBufLib
*/

void netStackSysPoolShow (void)
    {
#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
    printf("stack number %d\n", myStackNum);
#endif /* VIRTUAL_STACK */
    netPoolShow (_pNetSysPool);
    }

/*******************************************************************************
*
* mbufShow - report mbuf statistics
*
* This routine displays the distribution of mbufs in the network.
*
* RETURNS: N/A
*/

void mbufShow (void)
    {
#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
    printf("stack number %d\n", myStackNum);
#endif /* VIRTUAL_STACK */
    netPoolShow (_pNetDpool);
    }

/******************************************************************************
*
* netShowInit - initialize network show routines
*
* This routine links the network show facility into the VxWorks system.
* These routines are included automatically if INCLUDE_NET_SHOW
* is defined.
*
* RETURNS: N/A
*/

void netShowInit (void)
    {
    if (bufferedRtShow)
        routeBuf = KHEAP_ALLOC (rtMem);
    }

/*******************************************************************************
*
* arpShow - display entries in the system ARP table
*
* This routine displays the current Internet-to-Ethernet address mappings 
* in the ARP table.
*
* EXAMPLE
* .CS
*     -> arpShow
*
*     LINK LEVEL ARP TABLE
*     Destination      LL Address        Flags  Refcnt Use        Interface
*     ---------------------------------------------------------------------
*     90.0.0.63        08:00:3e:23:79:e7 0x405  0      82         lo0
*     ---------------------------------------------------------------------
* .CE
*
* Some configuration is required when this routine is to be used remotely over
* the network eg. through a telnet session or through the host shell using 
* WDB_COMM_NETWORK. If more than 5 entries are expected in the table the 
* parameter RT_BUFFERED_DISPLAY should be set to TRUE to prevent a possible
* deadlock. This requires a buffer whose size can be set with RT_DISPLAY_MEMORY.
* It will limit the number of entries that can be displayed (each entry requires
* approx. 70 bytes).
*
* RETURNS: N/A
*/

void arpShow (void)
    {
    char *dashLine = "---------------------------------------------------------------------\n";
    char *topLine =  "Destination      LL Address        Flags  Refcnt Use        Interface\n";

#ifdef VIRTUAL_STACK
    printf("stack number %d\n", myStackNum);
#endif /* VIRTUAL_STACK */

    printf ("\nLINK LEVEL ARP TABLE\n");
    printf ("%s", topLine);
    printf ("%s", dashLine);

    routeTableShow (RT_ARP); 		/* show ARP routes */

    printf ("%s", dashLine);
    }

/*******************************************************************************
*
* arptabShow - display the known ARP entries
*
* This routine displays current Internet-to-Ethernet address mappings in the
* ARP table.
*
* RETURNS: N/A
*
* INTERNAL
* This just calls arpShow.  It is provided for compatablity.
* Migrating to arpShow to be more compliant with WRS naming.
*/

void arptabShow (void)
    {
    arpShow ();
    }


/*****************************************************************************
*
* routestatShow - display routing statistics
*
* This routine displays routing statistics.
*
* RETURNS: N/A
*
* INTERNAL
* When using the Tornado shell, this routine is only available if an
* "@" sign is prepended to the routine name. Otherwise, it is preempted
* by a built-in version.
*/

void routestatShow (void)
    {
#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
    printf ("routing: (stack number %d)\n", myStackNum);
#else
    printf ("routing:\n");
#endif /* VIRTUAL_STACK */

    printf ("\t%u bad routing redirect%s\n", rtstat.rts_badredirect,
	    plural (rtstat.rts_badredirect));

    printf ("\t%u dynamically created route%s\n", rtstat.rts_dynamic,
	    plural (rtstat.rts_dynamic));

    printf ("\t%u new gateway%s due to redirects\n", rtstat.rts_newgateway,
	    plural (rtstat.rts_newgateway));

    printf ("\t%u destination%s found unreachable\n", rtstat.rts_unreach,
	    plural (rtstat.rts_unreach));

    printf ("\t%u use%s of a wildcard route\n", rtstat.rts_wildcard,
	    plural (rtstat.rts_wildcard));
    }

/*******************************************************************************
*
* routeShow - display all IP routes (summary information)
*
* This routine displays the list of destinations in the routing table
* along with the next-hop gateway and associated interface for each entry.
* It separates the routes into network routes and host-specific entries,
* but does not display the netmask for a route since it was created for
* class-based routes which used predetermined values for that field.
*
* The IP forwarding process will only use the first route entry to a 
* destination. When multiple routes exist to the same destination with
* the same netmask (which is not shown), the first route entry uses the
* lowest administrative weight. The remaining entries (listed as additional
* routes) use the same destination address. One of those entries will
* replace the primary route if it is deleted.
*
* EXAMPLE
* .CS
*     -> routeShow
*
*     ROUTE NET TABLE
*     Destination      Gateway          Flags  Refcnt Use        Interface
*     --------------------------------------------------------------------
*     90.0.0.0         90.0.0.63        0x1    1      142        enp0
*     10.0.0.0         90.0.0.70        0x1    1      142        enp0
*       Additional routes to 10.0.0.0:
*                      80.0.0.70        0x1    0      120        enp1
*     --------------------------------------------------------------------
*
*     ROUTE HOST TABLE
*     Destination      Gateway          Flags  Refcnt Use        Interface
*     --------------------------------------------------------------------
*     127.0.0.1        127.0.0.1        0x101  0      82         lo0
*     --------------------------------------------------------------------
* .CE
*
* The flags field represents a decimal value of the flags specified
* for a given route.  The following is a list of currently available
* flag values:
*
* .TS
* tab(|);
* l l .
*     0x1     | - route is usable (that is, "up")
*     0x2     | - destination is a gateway
*     0x4     | - host specific routing entry
*     0x8     | - host or net unreachable
*     0x10    | - created dynamically (by redirect)
*     0x20    | - modified dynamically (by redirect)
*     0x40    | - message confirmed
*     0x80    | - subnet mask present
*     0x100   | - generate new routes on use
*     0x200   | - external daemon resolves name
*     0x400   | - generated by ARP
*     0x800   | - manually added (static)
*     0x1000  | - just discard packets (during updates)
*     0x2000  | - modified by management protocol
*     0x4000  | - protocol specific routing flag
*     0x8000  | - protocol specific routing flag
* .TE
*
* In the above display example, the entry in the ROUTE NET TABLE has a 
* flag value of 1, which indicates that this route is "up" and usable and 
* network specific (the 0x4 bit is turned off).  The entry in the ROUTE 
* HOST TABLE has a flag value of 5 (0x1 OR'ed with 0x4), which indicates 
* that this route is "up" and usable and host-specific.
*
* Some configuration is required when this routine is to be used remotely over
* the network eg. through a telnet session or through the host shell using 
* WDB_COMM_NETWORK. If more than 5 routes are expected in the table the 
* parameter RT_BUFFERED_DISPLAY should be set to TRUE to prevent a possible
* deadlock. This requires a buffer whose size can be set with RT_DISPLAY_MEMORY.
* It will limit the number of routes that can be displayed (each route requires
* approx. 70 bytes).
*
* RETURNS: N/A
*/

void routeShow (void)
    {
    char *dashLine = "--------------------------------------------------------------------\n";
    char *topLine =  "Destination      Gateway          Flags  Refcnt Use        Interface\n";

#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
    printf("stack number %d\n", myStackNum);
#endif /* VIRTUAL_STACK */

    printf ("\nROUTE NET TABLE\n");
    printf ("%s", topLine);
    printf ("%s", dashLine);

    routeTableShow (RT_NET);		/* show network routes */

    printf ("%s", dashLine);
    printf ("\nROUTE HOST TABLE\n");
    printf ("%s", topLine);
    printf ("%s", dashLine);

    routeTableShow (RT_HST);		/* show host routes */

    printf ("%s", dashLine);
    }

/****************************************************************************
*
* routeNodeShow - print all route entries attached to a node in the tree
*
* This routine displays every route entry with a particular destination
* and netmask. Only the initial entry attached to the tree is visible to
* the IP forwarding process. The remaining entries are duplicates.
*
* The return value of 0 continues the tree traversal to access the next
* element.
*
* RETURNS: OK on success and ERROR on failure
*/

LOCAL int routeNodeShow
    (
    struct radix_node * pRoute,
    void* w
    )
    {
    ROUTE_ENTRY * pHead;    /* Start of route list for a protocol. */
    ROUTE_ENTRY * pNext;

    BOOL printFlag;    /* Available route actually displayed? */

    struct sockaddr * pAddress;
    char   addressString [INET_ADDR_LEN];
    int    len;

    pHead = (ROUTE_ENTRY *)pRoute;

    /* Print the route entry which is visible to IP. */

    printFlag = routeEntryPrint ( (struct rtentry *)pHead, w, TRUE);

    if (printFlag == FALSE)
        {
        /*
         * The primary route entry did not match the requested type. Ignore
         * any duplicate routes, which also will not meet the criteria.
         */

        return (OK);
        }

    /* Get the next entry to display, if any. */

    pNext = pHead->sameNode.pFrwd;
    if (pNext == NULL)
        {
        pHead = pNext = pHead->diffNode.pFrwd;
        }

    if (pNext != NULL)
        {
        /* Additional entries exist. Print the appropriate subheading. */

        pAddress = rt_key ( (struct rtentry *)pRoute);
        inet_ntoa_b ( ((struct sockaddr_in *)pAddress)->sin_addr,
                     addressString);

	if (!bufferedRtShow)
            printf ("  Additional routes to %s:\n", addressString);
	else
	    {
	    len = sprintf (routeBuf + bufIndex, "  Additional routes to %s:\n", 
			   addressString);
	    bufIndex += len;
	    }
        }

    /* Print any duplicate routes for the visible node. */

    while (pHead)
        {
        while (pNext)
            {
	    /* check for buffer space */

            if (bufferedRtShow && (rtMem - bufIndex < RT_ENTRY_LEN))
	        return (ERROR);
            routeEntryPrint ( (struct rtentry *)pNext, w, FALSE);

            pNext = pNext->sameNode.pFrwd;
            }
        pHead = pNext = pHead->diffNode.pFrwd;
        }

    /* check for buffer space */

    if (bufferedRtShow && (rtMem - bufIndex < RT_ENTRY_LEN))
	return (ERROR);

    return OK;
    }

/*******************************************************************************
*
* routeEntryPrint - print one route entry
*
* This function executes (when walking the route tree) for each route entry,
* including duplicate routes not visible to the IP forwarding process.
* The public routines set the <type> argument to RT_NET, RT_HST, or RT_ARP
* to select categories of route information from the routing table. The
* RT_NET value only displays IP routes which can reach multiple hosts.
* The RT_HST value only displays IP routes which can reach a specific host,
* and the RT_ARP value displays host-specific routes which contain link-level
* information in the gateway field.
*
* Older show routines only select one of the available route types. The
* newer alternative sets both RT_NET and RT_HST to display all IP routes.
* That routine supports classless routing, so it includes the netmask value
* in the output. It also prints the new type-of-service field and the
* protocol number which indicates the route's creator.
*
* RETURNS: TRUE if entry printed, or FALSE for mismatched type
*/

LOCAL BOOL routeEntryPrint
    (
    struct rtentry *	pRt,		/* pointer to the route entry */
    void *		pRtType, 	/* pointer to the route type */
    BOOL 		ipRouteFlag 	/* visible to IP forwarding? */
    )
    {
    FAST struct sockaddr *	destAddr = NULL;
    FAST struct sockaddr *	gateAddr;
    FAST UCHAR * 		pChr;
    FAST int			rtType; 
    char 			aStr [INET_ADDR_LEN];
    ULONG 			mask;
    char                        buf [RT_ENTRY_LEN];
    int                         index = 0;
    int                         len;

    rtType = *((int *)pRtType); 
    
    /*
     * Ignore host routes (including ARP entries) if only
     * network routes should be printed.
     */

    if ( (rtType == RT_NET) && (pRt->rt_flags & RTF_HOST))
	return (FALSE); 

    destAddr = rt_key(pRt);		/* get the destination address */
    gateAddr = pRt->rt_gateway;		/* get the gateway address */


    /* if what we want is a host route and not an arp entry then return */

    if ((rtType & RT_HST) && (gateAddr->sa_family == AF_LINK) &&
                              (pRt->rt_flags & RTF_HOST))
	return (FALSE);

    /* If only host routes are desired, skip any network route. */

    if ((rtType == RT_HST) && (!(pRt->rt_flags & RTF_HOST)))
	return (FALSE); 

    /* if what we want is an arp entry and if it is not then return */

    if ((rtType & RT_ARP) && ((gateAddr->sa_family != AF_LINK) ||
			      (((struct sockaddr_dl *)gateAddr)->sdl_alen == 0)
			      ))
	return (FALSE); 

    /* if what we want is a network route and the gateway family is AF_LINK */

    if ((rtType & RT_NET) && (gateAddr->sa_family == AF_LINK))
	gateAddr = pRt->rt_ifa->ifa_addr; 

    /*
     * Print destination address for visible entries and whitespace
     * for duplicate routes.
     */

    if (ipRouteFlag)
        {
        inet_ntoa_b (((struct sockaddr_in *)destAddr)->sin_addr, aStr);

	if (!bufferedRtShow)
            printf ("%-16s ", (destAddr->sa_family == AF_INET) ?
		              aStr : "not AF_INET");
	else
	    {
	    len = sprintf (buf, "%-16s ", (destAddr->sa_family == AF_INET) ? 
					   aStr : "not AF_INET");
	    index += len;
	    }
        }
    else
        {
	if (!bufferedRtShow)
            printf ("                 ");
	else
	    {
	    len = sprintf (buf, "                 ");
	    index += len;
	    }
        }

    /*
     * When displaying all IP routes, print the netmask for the visible
     * route entry. Show the new TOS values and the value of the routing
     * protocol which created the entry for the primary routes and any
     * duplicates. Only one of these type flags is set when using the
     * older class-based show routines.
     */

    if ( (rtType & RT_NET) && (rtType & RT_HST))
        {
        /* Print the (common) netmask value with the primary route entry. */

        if (ipRouteFlag)
            {
            if (rt_mask (pRt) == NULL)
                {
                mask = 0;
                }
            else
                {
                mask = ((struct sockaddr_in *) rt_mask (pRt))->sin_addr.s_addr;
                mask = ntohl (mask);
                }
        
	    if (!bufferedRtShow)
                printf ("%#-10lx ", mask);
	    else
		{
		len = sprintf (buf + index, "%#-10lx ", mask);
		index += len;
		}
            }
        else
            {
            /* Print whitespace when displaying duplicate routes. */

	    if (!bufferedRtShow)
                printf ("           ");    /* 11 spaces: empty netmask field */
	    else
		{
		len = sprintf (buf + index, "           ");
		index += len;
		}
            }

	if (!bufferedRtShow)
	    {
            /* print routing protocol value */

            printf ("%-4d  ", RT_PROTO_GET (destAddr));

            /* print type of service */

            printf ("%#-4x ", TOS_GET (destAddr));
            }
        else
	    {
	    len = sprintf (buf + index, "%-4d  %#-4x ", RT_PROTO_GET (destAddr),
	                   TOS_GET (destAddr));
	    index += len;
	    }
        }

    /* print the gateway address which internet or linklevel */

    if (gateAddr->sa_family == AF_LINK)
	{
	pChr = (UCHAR *)(LLADDR((struct sockaddr_dl *)gateAddr));

	if (!bufferedRtShow)
	    printf ("%02x:%02x:%02x:%02x:%02x:%02x ",
		    pChr[0], pChr[1], pChr[2], pChr[3], pChr[4], pChr[5]); 
	else
	    {
	    len = sprintf (buf + index, "%02x:%02x:%02x:%02x:%02x:%02x ",
			   pChr[0], pChr[1], pChr[2], pChr[3], pChr[4], pChr[5]);
            index += len;
	    }
	}
    else
	{
	inet_ntoa_b (((struct sockaddr_in *)gateAddr)->sin_addr, aStr);

	if (!bufferedRtShow)
	    printf ("%-16s ", (gateAddr->sa_family == AF_INET) ?
		    aStr :"not AF_INET"); 
	else
	    {
	    len = sprintf (buf + index, "%-16s ", 
			   (gateAddr->sa_family == AF_INET) ? aStr :
			   "not AF_INET");
	    index += len;
	    }
	}

    if (!bufferedRtShow)
	{
        printf ("%#-6hx ",	pRt->rt_flags);
        printf ("%-5d  ",	pRt->rt_refcnt);
        printf ("%-10ld ",	pRt->rt_use);
        printf ("%s%d\n",  pRt->rt_ifp->if_name, pRt->rt_ifp->if_unit);
	}
    else
	{
	len = sprintf (buf + index, "%#-6hx %-5d  %-10ld %s%d\n", pRt->rt_flags,
		       pRt->rt_refcnt, pRt->rt_use, pRt->rt_ifp->if_name,
		       pRt->rt_ifp->if_unit);
        index += len;

	bcopy (buf, (char *) (routeBuf + bufIndex), index);
	bufIndex += index;
	}

    return (TRUE); 
    }

/*******************************************************************************
*
* routeTableShow - print a subset of entries in the routing table
*
* This routine uses internal interfaces to walk the internal tree which
* contains all route information and display particular types of route
* entries. It provides a common entry point for three individual routines
* which print the ARP information, route entries assuming class-based
* netmasks (further separated into network and host categories), and routes
* which can use arbitrary netmasks.
*
* RETURNS: N/A
*/

LOCAL void routeTableShow
    (
    int 	rtType		/* route type network, host or arp */
    )
    {
    int                         s;
    struct radix_node_head *	rnh;
    int				type;

    type = rtType;
    rnh = rt_tables[AF_INET];

    if (rnh)
        {
	if (bufferedRtShow)
	    {
	    if (routeBuf == NULL)
		return;

	    bufIndex = 0;
            }

	s = splnet ();
	rn_walktree(rnh, routeNodeShow, &type);
        splx (s);

	if (bufferedRtShow)
	    {
	    routeBuf [bufIndex] = '\0';
	    printf ("%s", routeBuf);
	    }
        }

    return;
    }

/*******************************************************************************
*
* hostShow - display the host table
*
* This routine prints a list of remote hosts, along with their Internet
* addresses and aliases.
*
* RETURNS: N/A
*
* SEE ALSO: hostAdd()
*
* INTERNAL
* When using the Tornado shell, this routine is only available if an
* "@" sign is prepended to the routine name. Otherwise, it is preempted
* by a built-in version.
*
* This just calls arpShow.  It is provided for compatablity.
* Migrating to arpShow to be more compliant with WRS naming.
*/

void hostShow (void)
    {
    char inetBuf [INET_ADDR_LEN];
    FAST HOSTNAME *pHostName;
    FAST HOSTENTRY *pHostEntry;

#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
    printf("stack number %d\n", myStackNum);
#endif /* VIRTUAL_STACK */

    printf ("hostname         inet address       aliases\n");
    printf ("--------         ------------       -------\n");

    semTake (hostListSem, WAIT_FOREVER);

    for (pHostEntry = (HOSTENTRY *)lstFirst (&hostList);
	 pHostEntry != NULL;
	 pHostEntry = (HOSTENTRY *)lstNext (&pHostEntry->node))
	{
	inet_ntoa_b (pHostEntry->netAddr, inetBuf);
	printf ("%-16s %-18s", pHostEntry->hostName.name, inetBuf);

	for (pHostName = pHostEntry->hostName.link;
	     pHostName != NULL;
	     pHostName = pHostName->link)
	    {
	    printf ("%s ", pHostName->name);
	    }

	printf ("\n");
	}

    semGive (hostListSem);
    }

/*******************************************************************************
*
* mRouteShow - display all IP routes (verbose information)
*
* This routine displays the list of destinations in the routing table
* along with the next-hop gateway and associated interface. It also
* displays the netmask for a route (to handle classless routes which
* use arbitrary values for that field) and the value which indicates
* the route's creator, as well as any type-of-service information.
*
* When multiple routes exist to the same destination with the same netmask,
* the IP forwarding process only uses the first route entry with the lowest
* administrative weight. The remaining entries (listed as additional routes)
* use the same address and netmask. One of those entries will replace the
* primary route if it is deleted.
*
* Some configuration is required when this routine is to be used remotely over
* the network eg. through a telnet session or through the host shell using 
* WDB_COMM_NETWORK. If more than 5 routes are expected in the table the 
* parameter RT_BUFFERED_DISPLAY should be set to TRUE to prevent a possible
* deadlock. This requires a buffer whose size can be set with RT_DISPLAY_MEMORY.
* It will limit the number of routes that can be displayed (each route requires
* approx. 90 bytes).
*
* RETURNS: N/A
*/

void mRouteShow (void)
    {
    int type = RT_HST | RT_NET;    /* print all IP route entries */

#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
    printf("stack number %d\n", myStackNum);
#endif /* VIRTUAL_STACK */

    printf ("%-16s %-10s %-6s%-4s", "Destination", "Mask", "Proto", "TOS");
    printf ("%-16s %-6s %-7s%-11s", "Gateway", "Flags", "RefCnt", "Use");
    printf ("%s\n", "Interface");

    printf ("-------------------------------------------------------------");
    printf ("----------------------------\n");

    routeTableShow (type);
    }


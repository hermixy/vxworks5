/* proxyArpLib.c - proxy Address Resolution Protocol (ARP) server library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01p,07may02,kbw  man page edits
01o,26mar02,vvv  fixed proxy broadcast storm (SPR #74518)
01n,07dec01,vvv  fixed proxy broadcast forwarding enabled by proxyPortFwdOn
		 (SPR #71656)
01m,05nov01,vvv  fixed transfer of broadcasts across networks (SPR #70768)
01l,15oct01,rae  merge from truestack ver 01p, base o1h (cleanup)
01k,26feb01,spm  fixed transfer of broadcasts across networks (SPR #31718)
0ij,07feb01,spm  added merge record for 30jan01 update from version 01i of
                 tor2_0_x branch (base 01h) and fixed modification history;
                 removed excess code from merge
01i,30jan01,ijm  merged SPR# 28602 fixes (proxy ARP services are obsolete);
                 added permanent flag to proxy ARP entries and fixed handling
                 of ARP requests for local addresses
01h,09feb99,fle  doc: put the library description chart inside .CS / .CE
                 markups
		 + and removed dash line to stay in the INTERNAL field
01g,14dec97,jdi  doc: cleanup.
01f,25oct97,kbw  fixed man page problems found in beta review
01e,15apr97,kbw  fixed minor format problems in man page
01d,29jan97,vin  replaced routeCmd with proxyRouteCmd which adds an 
		 explicit network mask of 0xffffffff. Always added
		 the route before adding the arp entry.
01c,22nov96,vin  added cluster support replaced m_gethdr(..) with 
		 mHdrClGet(..).
01h,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01g,13nov92,dnw  added include of semLibP.h
01f,21sep92,jdi  Documentation cleanup.
01e,16aug92,elh  documentation changes.
01d,15jun92,elh  changed parameters, general cleanup. 
01c,04jun92,ajm  quieted ansi warnings on netTypeAdd parameters
01b,26may92,rrr  the tree shuffle
		 -changed includes to have absolute path from h/
01a,20sep91,elh	 written.
*/

/*
DESCRIPTION
This library implements a proxy ARP server that uses the Address Resolution
Protocol (ARP) to make physically distinct networks appear as one logical
network (that is, the networks share the same address space). The server
forwards ARP messages between the separate networks so that hosts on the
main network can access hosts on the proxy network without altering their
routing tables.

The proxyArpLibInit() initializes the server and adds this library to the
VxWorks image.  This happens automatically if INCLUDE_PROXY_SERVER is
defined at the time the image is built.  The proxyNetCreate() and
proxyNetDelete() routines will enable and disable the forwarding of ARP
messages between networks. The proxyNetShow() routine displays the current
set of proxy networks and the main network and known clients for each.

By default, this server automatically adds a client when it first detects
an ARP message from that host. A VxWorks target can also register as
a client with the proxyReg() routine and remove that registration with
the proxyUnreg() routine. See the proxyLib manual pages for details.

To minimize traffic on the main network, the proxy server will only forward
broadcast packets to the specified destination ports visible with the
proxyPortShow() routine.  The proxyPortFwdOn() and proxyPortFwdOff() routines
will alter the current settings. Initially, broadcast forwarding is not
active for any ports.

INCLUDE FILES: proxyArpLib.h

SEE ALSO: proxyLib, RFC 925, RFC 1027, RFC 826

INTERNAL

Proxy Server Structures

The proxy server must hold information about each of the proxy networks.
A proxy network is defined by a proxy network structure (PROXY_NET) which
contains the local interface addresses for each network and a list of the
current proxy clients. The proxyNetList variable stores all of the proxy
network information.

Proxy Clients

A proxy client corresponds to a host on a proxy network.  The proxy server
stores information about each proxy client in the PROXY_CLNT structure.
By default, the server adds a client when it detects the initial ARP message
from the host. Clients may also be added and deleted explicitly with
proxyLib routines.

The proxy client information is available as both a hash table and a linked
list. The hash table (called clientTbl) provides fast lookup using the
client IP address as the key.  Each proxy network structure provides a
third method for accessing a particular proxy client on that network through
the clientList element of the structure.

The combined collection of proxy ARP structures (i.e the proxyNetList, and
the clientTbl) are protected by a single mutual exclusion semaphore.

Registering and Unregistering Proxy Clients

A vxWorks target can explicitly register and unregister itself as a 
proxy client by generating and sending messages of an unused Ethernet
type to the server. These messages use the format given by the PROXY_MSG
structure and have either operation type PROXY_REG or PROXY_UNREG.  If
the proxy server receives one of these messages it performs the appropriate
action by either adding or deleting the node as a proxy client.  The server
notifies the client on completion by sending back an ACK message. 

By default, the proxy server registers a client when it detects the initial
ARP message from a host and never removes that registration. As a result,
the proxy client component is normally not required. The explicit registration
which that library allows is only necessary for protocols such as BOOTP
that may require proxy ARP services before configuring an interface with
an IP address (which sends a gratuitous ARP message).

There are two main hooks into this library from the network modules.  The
in_arpinput() routine in if_ether.c uses the first hook to handle special
ARP processing.  When the server is active, that routine processes all valid
ARP messages as follows:

                    if (proxyArpHook == NULL ||
                        (* proxyArpHook)
                         ( (struct arpcom *)m->m_pkthdr.rcvif, m) == FALSE)
                        in_arpinput(m);

The proxyArpHook function pointer uses the proxyArpInput() routine to forward
any ARP requests or replies as appropriate. That routine typically returns
FALSE since the standard ARP processing handles most cases correctly.

The ipintr() routine in ip_input.c uses a similar function pointer
(proxyBroadcastHook) so that the proxy ARP server may forward selected
broadcast packets to and from the main network with the proxyBroadcastInput
routine.

VXWORKS AE PROTECTION DOMAINS

Under VxWorks AE, the functions you assign for either 'proxyArpHook' 
or 'proxyBroadcastHook' must be valid within the kernel protection domain. 
This restriction does not apply under non-AE versions of VxWorks.  

Structure Chart:

.CS
				 v	      	  v
		   	    proxyArpLibInit  proxyNetShow
	       (in_arpinput)
		     v
 ----------------proxyArpInput------------------------------------------
/     /			|  	        \ \      	        \        \
      v			|		|  |  			 |	  |
| proxyArpReplyFwd------|---------------|--|---------------------|-------->
|             |         v		|  |  	                 |        |
v	      | proxyArpRequestFwd---- -|--|---------------------|-------->
proxyArpReply |  / 	 		|  |     	         |        |
       |      | | 	 		|  |     	         |        |
       v      v v  		        |  |                     |        |
      proxyArpSend    	    		|  |     	         |        |
					|  |     	         |        |
					|  |  		   	 |        |
	(ipintr)			|  |     	         |        |
           v				|  |     	         |        |
  --proxyBroadcastInput-----------------|-->     	         |        |
 /           |		   \	        |  |     	         |        |
 v     	     ------   	    <-----------/  |     (do_protocol)	 |        |
proxyBroadcast |	    |		   |      	v        |        |
               |	    |		   |   proxyMsgInput----->        |
             |----/    	    |		   |   	       	       | |        |
             |		    |		   |                   | |        |
             |	            |		   v            |      | |        |
             |              |		proxyIsAMainNet |      | |        |
             |	    v       |	        		| /---/  |        |
	     |proxyNetCreate|   	       		| |      |        |
	     |      v	    | 		    	  	| |      v        |
	     |proxyNetDelete|       ----------------------|-proxyClientAdd|
	     |	    v 	    v      / 			| v     v	  |
	     |	    proxyNetFind<-/			|proxyClientDelete|
	     |						v       v         |
	     \						proxyClientFind<-/
	      ------------\
     v	  		  |		v		   v
proxyPortFwdOn	          |        proxyPortFwdOff	proxyPortShow
      |	                  |         |     		  |    |
      \------------------>|<-------/<--------------------/     v
       			  v	   			 proxyPortPrint
        	  proxyPortTblFind

.CE
*/

/* includes */

#include "vxWorks.h"
#include "netinet/if_ether.h"
#include "proxyArpLib.h"
#include "sys/socket.h"
#include "net/if_arp.h"
#include "net/route.h"
#include "net/if.h"
#include "net/mbuf.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "sys/ioctl.h"
#include "netinet/in.h"
#include "errno.h"
#include "hashLib.h"
#include "netinet/ip_var.h"
#include "netinet/udp.h"
#include "netinet/udp_var.h"
#include "inetLib.h"
#include "string.h"
#include "stdlib.h"
#include "logLib.h"
#include "arpLib.h"
#include "routeLib.h"
#include "stdio.h"
#include "net/unixLib.h"
#include "net/if_subr.h"
#include "private/semLibP.h"
#include "memPartLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsProxyArp.h"
#endif

/* defines */

#define CLNT_TBL_SZ		8	/* default client table sz 2^8	*/
#define PORT_TBL_SZ		8	/* default port table sz 2^8	*/

/* useful macros */

#define proxyIsAproxyNet(pAddr)	((proxyNetFind (pAddr) == NULL) ? 	\
					(FALSE) : (TRUE))

#define proxyIsAClient(pAddr)	((proxyClientFind ((pAddr)) == NULL) ?	\
					(FALSE) : (TRUE))

#define NODE_TO_CLIENT(pNode)	(PROXY_CLNT *) 			\
			((u_long)(pNode + 1) - sizeof (PROXY_CLNT))




					/* fill in sockaddr_in structure */
#define SIN_FILL(pSin, family, addr, port)				\
    {									\
    bzero ((caddr_t) (pSin), sizeof (struct sockaddr_in));		\
    (pSin)->sin_len		= sizeof(struct sockaddr_in);		\
    (pSin)->sin_family		= (family);				\
    (pSin)->sin_addr.s_addr	= (addr);				\
    (pSin)->sin_port		= (port);				\
    }


/* globals */
#ifndef VIRTUAL_STACK

/* debug variables */

BOOL		arpDebug              = FALSE;	/* ARP debug messages */
BOOL		proxyArpVerbose	      = FALSE;	/* proxy ARP messages */
BOOL		proxyBroadcastVerbose = FALSE;	/* broadcast messages */

/* enabling variables */

BOOL		proxyBroadcastFwd     = TRUE;	/* forward broadcasts */
BOOL		arpRegister	      = TRUE;	/* use ARP message to */
					 	/* register clients */

int		clientTblSizeDefault  = CLNT_TBL_SZ; /* default size  */
int		portTblSizeDefault    = PORT_TBL_SZ; /* default size  */

/* locals */

/*
 * proxy semahore & semid and options and structures
 * protected by proxySem (clientTbl, proxyNetList)
 */
LOCAL SEMAPHORE	proxySem;			/* proxy ARP semphore */
LOCAL SEM_ID	proxySemId 	    = &proxySem;
int		proxySemOptions	    = (SEM_Q_FIFO | SEM_DELETE_SAFE );

LOCAL HASH_ID	clientTbl	    = NULL;	/* hashed clients     */
LOCAL LIST	proxyNetList;			/* proxy network list */

/* port table sem semid and options, and port table*/

LOCAL SEMAPHORE	portTblSem;
LOCAL SEM_ID    portTblSemId	    = &portTblSem;
int		portTblSemOptions   = (SEM_Q_FIFO | SEM_DELETE_SAFE );
LOCAL HASH_ID	portTbl	      	    = NULL;	/* hashed ports	      */

LOCAL BOOL	proxyLibInitialized = FALSE;	/* server initialized */
LOCAL int	hashKeyArg 	    = 425871; 	/* hash key 	      */

#endif /* VIRTUAL_STACK */

/* forward declarations */

IMPORT FUNCPTR		proxyArpHook;
IMPORT FUNCPTR		proxyBroadcastHook;
IMPORT int		in_cksum ();
IMPORT int		in_broadcast ();


/* locals */
							/* hook routines */

LOCAL void proxyBroadcastInput (struct mbuf *pMbuf, struct ifnet * pIf);
LOCAL BOOL proxyArpInput (struct arpcom * pArpcom, struct mbuf * pMbuf);
LOCAL void proxyMsgInput (struct arpcom * pArpcom, struct mbuf * pMbuf, 
			  int len);
LOCAL void proxyArpReply (struct arpcom * pArpcom, struct mbuf * pMbuf,
			  int proto);
LOCAL void proxyArpRequestFwd (struct in_addr * pClientAddr, 
			       struct mbuf * pMbuf);
LOCAL void proxyArpReplyFwd (PROXY_CLNT * pClient, struct mbuf * pMbuf);
LOCAL void proxyArpSend (struct ifnet * pIf, int op, int proto,
		         u_char * srcProtoAddr, u_char * dstHwAddr,
			 u_char * dstProtoAddr);

LOCAL void proxyBroadcast (struct ifnet * pIf, struct mbuf * pMbuf, int ttl);
LOCAL PROXY_NET * proxyNetFind (struct in_addr * pProxyAddr);
LOCAL PROXY_CLNT * proxyClientFind (struct in_addr * clientAddr);
LOCAL BOOL proxyIsAMainNet (struct in_addr * pMainAddr);
LOCAL PORT_NODE * portTblFind (int port);
LOCAL BOOL proxyPortPrint (PORT_NODE * pPort);

LOCAL STATUS proxyRouteCmd (int destInetAddr, int gateInetAddr, int ioctlCmd); 
LOCAL STATUS proxyClientAdd (struct arpcom * pArpcom,
                             struct in_addr * pClientAddr,
                             u_char * pClientHwAddr);
LOCAL STATUS proxyClientDelete (struct in_addr * pClientAddr);

LOCAL void proxyClientRemove (PROXY_CLNT * pClient);

LOCAL M_BLK_ID proxyPacketDup (M_BLK_ID pMblk);

/*******************************************************************************
*
* proxyArpLibInit - initialize proxy ARP
*
* This routine starts the proxy ARP server by initializing the required data
* structures and installing the necessary input hooks.  It should be called
*  only once; subsequent calls have no effect. The <clientSizeLog2> and
* <portSizeLog2> parameters specify the internal hash table sizes. Each
* must be equal to a power of two, or zero to use a default size value.
*
* RETURNS: OK, or ERROR if unsuccessful.
*/

STATUS proxyArpLibInit
    (
    int		clientSizeLog2,		/* client table size as power of two */
    int		portSizeLog2		/* port table size as power of two   */
    )
    {
    if (proxyLibInitialized)		/* already initialized */
	return (OK);

#ifdef VIRTUAL_STACK
/* need to initialize vars */
arpDebug              = FALSE;  /* ARP debug messages */
proxyArpVerbose       = FALSE;  /* proxy ARP messages */
proxyBroadcastVerbose = FALSE;  /* broadcast messages */

/* enabling variables */

proxyBroadcastFwd     = TRUE;   /* forward broadcasts */
arpRegister           = TRUE;   /* use ARP message to */
				/* to register clients */

clientTblSizeDefault  = CLNT_TBL_SZ; /* default size  */
portTblSizeDefault    = PORT_TBL_SZ; /* default size  */

proxySemId          = &proxySem;
proxySemOptions     = (SEM_Q_FIFO | SEM_DELETE_SAFE );

clientTbl           = NULL;     /* hashed clients     */

/* port table sem semid and options, and port table*/

portTblSemId        = &portTblSem;
portTblSemOptions   = (SEM_Q_FIFO | SEM_DELETE_SAFE );
portTbl             = NULL;     /* hashed ports       */

proxyLibInitialized = FALSE;    /* server initialized */
hashKeyArg          = 425871;   /* hash key           */

#endif /* VIRTUAL_STACK */

    /* use default values if zero passed */

    clientSizeLog2 = (clientSizeLog2 == 0) ? clientTblSizeDefault :
					     clientSizeLog2;
    portSizeLog2   = (portSizeLog2 == 0) ? portTblSizeDefault : portSizeLog2;

    lstInit (&proxyNetList);

    if ((proxySemId = semMCreate (proxySemOptions)) == NULL) 
	return (ERROR);

    if ((portTblSemId = semMCreate (portTblSemOptions)) == NULL)
	{
	semDelete (proxySemId);
	return (ERROR);
	}

    if ((clientTbl = hashTblCreate (clientSizeLog2, hashKeyCmp, hashFuncModulo,
				    hashKeyArg)) == NULL)
	{
	semDelete (proxySemId);
	semDelete (portTblSemId);
	return (ERROR);
	}

    if ((portTbl = hashTblCreate (portSizeLog2, hashKeyCmp, hashFuncModulo,
				  hashKeyArg)) == NULL)
	{
	semDelete (proxySemId);
	semDelete (portTblSemId);
	hashTblDelete (clientTbl);
	return (ERROR);
	}

    proxyBroadcastHook = (FUNCPTR) proxyBroadcastInput;
    netTypeAdd (PROXY_TYPE, (FUNCPTR) proxyMsgInput);
    proxyArpHook = (FUNCPTR) proxyArpInput;
    proxyLibInitialized = TRUE;
    return (OK);
    }

/*******************************************************************************
*
* proxyArpInput - proxy ARP input hook
*
* The proxyArpInput routine completes the gateway transparency between a
* proxy network and the main network, allowing the physically distinct
* hosts to share the same logical subnet. The standard ARP processing is
* able to manage all ARP messages except requests from a proxy client for
* a host on the main network. The arpintr routine in the if_ether.c module
* calls this routine to relay those requests to the destination network and
* return the replies. The routine returns TRUE for those messages to prevent
* redundant (and usually incorrect) processing by the in_arpinput routine.
*
* The <pArpcom> parameter identifies the interface which received the
* ARP message contained in the <pMbuf> mbuf chain.
*
* RETURNS: TRUE if message handled, or FALSE otherwise.
*
* NOMANUAL
*/

LOCAL BOOL proxyArpInput
    (
    struct arpcom *		pArpcom,	/* arpcom structure 	*/
    struct mbuf *		pMbuf		/* mbuf chain		*/
    )
    {
    struct ether_arp * 	pArp; 		/* ARP message */
    struct in_addr 	srcAddr; 	/* source address */
    struct in_addr 	dstAddr; 	/* destination address */
    u_short 		arpOp; 		/* ARP operation */
    PROXY_CLNT * 	pClient; 	/* registration of proxy client */
    PROXY_NET *         pNet;           /* proxy network */

    /*
     * Extract information from the ARP message which the arpintr()
     * routine has validated.
     */

    pArp = mtod (pMbuf, struct ether_arp *);
    arpOp = ntohs (pArp->arp_op);
    bcopy (pArp->arp_spa, (char *)&srcAddr, sizeof (srcAddr));
    bcopy (pArp->arp_tpa, (char *)&dstAddr, sizeof (dstAddr));

    if (arpDebug)				/* ARP debugging info */
	{
    	logMsg ("op:%s (if 0x%x) src: 0x%x [%s] dst: 0x%x ",
		(arpOp == ARPOP_REQUEST) ? (int) "request" : (int) "reply",
		ntohl (pArpcom->ac_ipaddr.s_addr), ntohl (srcAddr.s_addr),
		(int) ether_sprintf (pArp->arp_sha), ntohl (dstAddr.s_addr), 0);

	if (arpOp != ARPOP_REQUEST)
	    logMsg ("[%s]", (int) ether_sprintf (pArp->arp_tha), 0, 0, 0, 0 ,0);
	logMsg ("\n", 0, 0, 0, 0, 0, 0);
	}

    /* Ignore any messages containing a broadcast address as the source. */

    if (!(bcmp (pArp->arp_sha, etherbroadcastaddr, sizeof (pArp->arp_sha))))
        {
        m_freem (pMbuf);
        return (TRUE);    /* Invalid message - skip standard ARP processing. */
        }
    semTake (proxySemId, WAIT_FOREVER);

    switch (arpOp)
	{
	case ARPOP_REQUEST:
            /*
             * Handle any ARP requests which must cross a network boundary
             * that the proxy server hides. The standard ARP processing
             * cannot manage those situations. It will handle all requests
             * from a main network since proxy ARP entries exist for all
             * hosts on the proxy network.
             */

            if ((pNet = proxyNetFind (&pArpcom->ac_ipaddr)) != NULL)
                {
                /*
                 * Received an ARP request from a proxy network. 
                 * Add new client if enabled by arpRegister setting.
                 */

                if (arpRegister && !proxyIsAClient (&srcAddr))
                    (void) proxyClientAdd (pArpcom, &srcAddr, pArp->arp_sha);

                pClient = proxyClientFind (&dstAddr);
                if (pClient)
                    {
                    /*
                     * The destination of the request is also a proxy client.
                     * If it is on the same proxy network, ignore the request
                     * since the client will reply for itself. If it is on
                     * a different proxy network, the standard ARP processing
                     * will reply correctly based on the proxy ARP entry.
                     */

                    if (pClient->pNet->proxyAddr.s_addr
                            == pArpcom->ac_ipaddr.s_addr)
                        {
                        /* Client will answer - avoid proxy reply. */

                        semGive (proxySemId);

                        m_freem (pMbuf);
                        return (TRUE);
                        }
                    }
                else
                    {
                    /*
                     * The destination is not a proxy client. These ARP
                     * requests from a proxy client for a different host on
                     * the main network cannot be handled by the standard
                     * ARP processing. If the destination's hardware address
                     * is available, send a reply giving ours as an alias.
                     * Otherwise, forward the request to the main network.
                     */

                    if (arpCmd (SIOCGARP, &dstAddr, (u_char *)NULL,
                                (int *) NULL) == OK ||
				pNet->proxyAddr.s_addr == dstAddr.s_addr ||
				pNet->mainAddr.s_addr == dstAddr.s_addr)
                        proxyArpReply (pArpcom, pMbuf,
                                       ntohs (pArp->arp_pro));
                    else
                        proxyArpRequestFwd (&srcAddr, pMbuf);

                    semGive (proxySemId);

                    m_freem (pMbuf);
                    return (TRUE);    /* Skip standard ARP processing. */
                    }
                }
            break;

	case ARPOP_REPLY:
    	    /*
     	     * Received an ARP reply.  If it is a response to an earlier
             * (forwarded) request, generate a reply to the proxy client
             * supplying our hardware address as an alias. Once the 
             * hardware address of the actual destination is in the
             * ARP cache, the proxy server will be able to transparently
             * forward packets from the proxy network to that host. 
     	     */

            pClient = proxyClientFind (&dstAddr);
            if (pClient && (pClient->pNet->mainAddr.s_addr ==
                                pArpcom->ac_ipaddr.s_addr))
                {
                /*
                 * Note: the second condition of the previous test is a
                 * sanity check preventing the forwarding of replies
                 * between proxy networks, although those messages should
                 * never be sent since requests are not forwarded between
                 * proxy networks in the first place.
                 */

                proxyArpReplyFwd (pClient, pMbuf);

                /*
                 * The standard ARP processing would only refresh an existing
                 * entry, not create a new one, so add the entry for the
                 * actual destination of the original ARP request.
                 */

                arpCmd (SIOCSARP, &srcAddr, pArp->arp_sha, (int *) NULL);

                semGive (proxySemId);

                m_freem (pMbuf);
                return (TRUE); 	/* Skip (redundant) standard ARP processing. */
                }

            break;

	default:
	    break;
	}

    semGive (proxySemId);
    return (FALSE); 	/* Standard ARP processing handles remaining cases. */
    }


/*******************************************************************************
*
* proxyArpReply - generate an ARP reply
*
* This routine responds to ARP request messages which would ordinarily
* require forwarding across a network boundary hidden by the proxy server.
* The server advertises the hardware address of its local interface instead
* of the actual hardware address of the destination host. It will forward
* the incoming data to the correct destination on the separate network.
*
* This routine usually handles ARP requests from the proxy network for a host
* on the main network. The standard ARP processing is capable of responding
* to requests from the main network since registering the proxy clients
* creates the necessary proxy ARP entries, but hosts on a main network are
* not registered.
*
* The <pArpcom> parameter identifies the interface which received the
* ARP message contained in the <pMbuf> mbuf chain. The <proto> parameter
* identifies the type of the protocol information for the new ARP message.
* Currently, it is always ETHERTYPE_IP (0x800). 
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void proxyArpReply
    (
    FAST struct arpcom *		pArpcom,	/* arpcom structure   */
    FAST struct mbuf *			pMbuf,		/* mbuf chain 	      */
    int 			 	proto		/* format of protocol */
    )
    {
    struct ether_arp *			pArp;		/* ARP message	      */
    struct in_addr			srcAddr;	/* source address     */

    pArp = mtod (pMbuf, struct ether_arp *);

    /*
     * Add the original source to the ARP cache so that the server can
     * transparently forward data from the proxy client to that host.
     * Ignore failures caused if an entry already exists.
     */

    bcopy (pArp->arp_spa, (char *)&srcAddr, sizeof (srcAddr));
    arpCmd (SIOCSARP, &srcAddr, (u_char *)pArp->arp_sha,  (int *)NULL);

    /*  switch source and target addresses */

    proxyArpSend ( (struct ifnet *) pArpcom, ARPOP_REPLY, proto,
                  pArp->arp_tpa, pArp->arp_sha, pArp->arp_spa);
    }

/*******************************************************************************
*
* proxyArpRequestFwd - forward an ARP request
*
* This routine relays an ARP request message from a proxy network to a
* main network when a proxy client attempts to contact a host across the
* network boundary which the proxy server conceals. The server supplies
* the hardware address of the local interface to the host on the main
* network so that all traffic will be transparently forwarded to the
* proxy client. The <pMbuf> parameter contains the original ARP request
* sent by the proxy client using the <pClientAddr> address.
*
* RETURNS:  N/A
*
* NOMANUAL
*/

LOCAL void proxyArpRequestFwd
    (
    struct in_addr *		pClientAddr,		/* proxy client addr */
    struct mbuf * 		pMbuf			/* mbuf chain	     */
    )
    {
    PROXY_CLNT *		pClient;		/* proxy client	     */
    struct ether_arp *		pArp;			/* ARP message 	     */
    struct sockaddr_in		sin;			/* interface sin     */
    struct ifaddr *		pIfa;			/* interface address */

    /* find the client's main network interface */

    if ((pClient = proxyClientFind (pClientAddr)) == NULL)
	return;

    SIN_FILL (&sin, AF_INET, pClient->pNet->mainAddr.s_addr, 0);

    if ((pIfa = ifa_ifwithaddr ((struct sockaddr *) &sin)) == NULL)
	return;

    if (proxyArpVerbose)			/* print debugging info */
	logMsg ("(forwarding request to if 0x%x) ", ntohl (pClient->pNet->mainAddr.s_addr),
		0, 0, 0, 0, 0);

    /*
     * Leave the client as the source protocol address so when the reply
     * comes back we know where to forward the reply.
     */
    pArp = mtod (pMbuf, struct ether_arp *);
    proxyArpSend (pIfa->ifa_ifp, ntohs (pArp->arp_op), ntohs (pArp->arp_pro),
		  pArp->arp_spa, pArp->arp_tha, pArp->arp_tpa);
    }

/*******************************************************************************
*
* proxyArpReplyFwd - forward an ARP reply
*
* This routine relays an ARP reply message from a main network to a
* proxy network following an earlier transfer in the opposite direction.
* The server supplies the hardware address of the local interface instead
* of the actual destination host on the main network so that all traffic
* from the proxy client will be transparently forwarded. The <pMbuf>
* mbuf chain contains the original ARP reply intended for the  proxy client
* indicated by the <pClient> parameter.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void proxyArpReplyFwd
    (
    PROXY_CLNT * 	pClient,    /* proxy client registration data */
    struct mbuf * 	pMbuf       /* original ARP message */
    )
    {
    struct sockaddr_in		sin;			/* interface sin     */
    struct ifaddr *		pIfa;			/* interface address */
    struct ether_arp *		pArp;			/* ARP message 	     */
    u_short 			arpProto;		/* ARP type	     */

    /* Find the interface connected to the proxy client. */

    SIN_FILL (&sin, AF_INET, pClient->pNet->proxyAddr.s_addr, 0);

    if ((pIfa = ifa_ifwithaddr ((struct sockaddr *) &sin)) == NULL)
	return;

    if (proxyArpVerbose)			/* print debugging info */
	logMsg ("(forwarding reply to if 0x%x)", ntohl (pClient->pNet->proxyAddr.s_addr),
		0, 0, 0, 0, 0);

    pArp = mtod (pMbuf, struct ether_arp *);

    /*
     * Replace (deprecated) trailer packets with the standard type value. It
     * is theoretically possible to consume those messages since trailer
     * responses are additional replies, but forward them anyway for safety.
     */

    if (ntohs (pArp->arp_pro) == ETHERTYPE_TRAIL)
        arpProto = ETHERTYPE_IP;
    else
        arpProto = ntohs (pArp->arp_pro);

    /*
     * The client's registration data contains a snapshot of the hardware
     * address at that time. Since all clients must register, the lack of
     * dynamic updates (available with an ARP entry) is unimportant.
     */

    proxyArpSend (pIfa->ifa_ifp, ntohs (pArp->arp_op), arpProto,
                  pArp->arp_spa, pClient->hwaddr, pArp->arp_tpa);
    }

/*******************************************************************************
*
* proxyArpSend - generate and send an ARP message
*
* This routine constructs and sends an ARP message over the network interface
* specified by <pIf>.  <op> specifies the ARP operation, <proto> is the format
* of protocol address.  <srcProtoAddr> is the source protocol address,
* <dstHwAddr> is the destination hardware address and <dstProtoAddr> is the
* destination protocol addresses.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void proxyArpSend
    (
    struct ifnet *		pIf,		/* interface pointer 	*/
    int				op,		/* ARP op		*/
    int				proto,		/* ARP protocol		*/
    u_char *			srcProtoAddr,	/* src ip address	*/
    u_char *			dstHwAddr,	/* dest hw address	*/
    u_char *			dstProtoAddr	/* dest ip address	*/
    )

    {
    FAST struct mbuf *		pMbuf;		/* mbuf chain		*/
    FAST struct ether_arp *	pArp;		/* ARP message		*/
    struct ether_header *	pEh;		/* ether header		*/
    struct sockaddr 		sa;		/* sockaddr structure	*/

    if ((pMbuf = mHdrClGet(M_DONTWAIT, MT_DATA, sizeof(*pArp), TRUE)) == NULL)
	return;
    pMbuf->m_len = sizeof(*pArp);
    pMbuf->m_pkthdr.len = sizeof(*pArp);
    MH_ALIGN(pMbuf, sizeof(*pArp));

    pArp = mtod(pMbuf, struct ether_arp *);	/* fill in ARP message */
    bzero ((caddr_t) pArp, sizeof (struct ether_arp));
    pArp->arp_hrd = htons (ARPHRD_ETHER);
    pArp->arp_pro = htons (proto);
    pArp->arp_hln = sizeof (pArp->arp_sha);
    pArp->arp_pln = sizeof (pArp->arp_spa);
    pArp->arp_op  = htons (op);

    bcopy ((caddr_t) ((struct arpcom *) pIf)->ac_enaddr,
	   (caddr_t) pArp->arp_sha, sizeof (pArp->arp_sha));
    bcopy ((caddr_t) srcProtoAddr, (caddr_t) pArp->arp_spa,
	   sizeof (pArp->arp_spa));

    bcopy ((caddr_t) dstProtoAddr, (caddr_t) pArp->arp_tpa,
	   sizeof (pArp->arp_tpa));

    bzero ((caddr_t) &sa, sizeof (sa));
    sa.sa_family = AF_UNSPEC;

    pEh = (struct ether_header *) sa.sa_data;
    pEh->ether_type = ETHERTYPE_ARP;		/* switched in ether_output */

    if (op == ARPOP_REQUEST)
	{
    	bcopy ((caddr_t) etherbroadcastaddr, (caddr_t) pEh->ether_dhost,
	       sizeof (pEh->ether_dhost));
	}
    else
	{
    	bcopy ((caddr_t) dstHwAddr, (caddr_t) pArp->arp_tha,
	       sizeof (pArp->arp_tha));
    	bcopy ((caddr_t) dstHwAddr, (caddr_t) pEh->ether_dhost,
	       sizeof (pEh->ether_dhost));
	}

    if (proxyArpVerbose)
	{
    	logMsg ("%s (%x): src: 0x%x [%s] dst : 0x%x ", (op == ARPOP_REQUEST) ?
		(int) "request" : (int) "reply", proto,
		*((int *) pArp->arp_spa), (int) ether_sprintf (pArp->arp_sha),
		*((int *) pArp->arp_tpa), 0);

	if (op != ARPOP_REQUEST)
	    logMsg ("[%s]", (int) ether_sprintf (pArp->arp_tha), 0, 0, 0, 0, 0);

	logMsg ("\n", 0, 0, 0, 0, 0, 0);
	}

    (*pIf->if_output) (pIf, pMbuf, &sa, (struct rtentry *) NULL);
    }

/*******************************************************************************
*
* proxyNetCreate - create a proxy ARP network
*
* This routine activates proxy services between the proxy network connected
* to the interface with the <proxyAddr> IP address and the main network
* connected to the interface with the <mainAddr> address. Once registration
* is complete, the proxy server will disguise the physically separated
* networks as a single logical network. 
*
* The corresponding interfaces must be attached and configured with IP
* addresses before calling this routine. If the proxy network shares the
* same logical subnet number as the main network, the corresponding interface
* to the proxy network must use a value of 255.255.255.255 for the netmask.
*
* RETURNS: OK, or ERROR if unsuccessful.
*
* ERRNO:
*  S_proxyArpLib_INVALID_ADDRESS
*/

STATUS proxyNetCreate
    (
    char * 	proxyAddr,	/* address of proxy network interface */
    char * 	mainAddr	/* address of main network interface */
    )
    {
    struct sockaddr_in 	proxyAddrKey;
    struct sockaddr_in 	mainAddrKey;
    PROXY_NET * 	pNet;
    struct ifaddr * 	pIfa;
    int 		flags;

    /*
     * Verify that local interfaces can reach the addresses of the
     * proxy and main networks.
     */

    SIN_FILL (&proxyAddrKey, AF_INET, inet_addr (proxyAddr), 0);
    SIN_FILL (&mainAddrKey, AF_INET, inet_addr (mainAddr), 0);

    if ( (ifa_ifwithaddr ( (struct sockaddr *)&mainAddrKey) == NULL) ||
        ( (pIfa = ifa_ifwithaddr ( (struct sockaddr *)&proxyAddrKey)) == NULL))
        {
        errno = S_proxyArpLib_INVALID_ADDRESS;
        return (ERROR);
        }

    if ((pNet = (PROXY_NET *) calloc (1, sizeof (PROXY_NET))) == NULL)
	return (ERROR);

    semTake (proxySemId, WAIT_FOREVER);

    /* Replace any existing proxy network entry with the new values. */

    proxyNetDelete (proxyAddr);

    pNet->proxyAddr    =  proxyAddrKey.sin_addr;
    pNet->mainAddr     =  mainAddrKey.sin_addr;
    lstInit (&pNet->clientList);
    lstAdd (&proxyNetList, &pNet->netNode);

    /*
     * Add a proxy ARP entry allowing the standard ARP processing
     * to reply to requests for this local interface from hosts on
     * other networks. The ATF_PROXY flag indicates a "wildcard" hardware
     * address. Ignore failures caused if an entry already exists.
     */

    flags = ATF_PUBL | ATF_PROXY | ATF_PERM;
    arpCmd (SIOCSARP, &pNet->proxyAddr, NULL, &flags);

    semGive (proxySemId);
    return (OK);
    }

/*******************************************************************************
*
* proxyNetDelete - delete a proxy network
*
* This routine deletes the proxy network specified by <proxyAddr>.  It also
* removes all the proxy clients that exist on that network.
*
* RETURNS: OK, or ERROR if unsuccessful.
*
*/

STATUS proxyNetDelete
    (
    char *  			proxyAddr	/* proxy net address 	*/
    )

    {
    PROXY_NET *			pNet;		/* proxy network	*/
    NODE *			pNode;		/* client node		*/
    PROXY_CLNT *		pClient;	/* proxy client		*/
    STATUS 			status = ERROR;	/* return value		*/
    struct in_addr		proxyAddrIn;

    semTake (proxySemId, WAIT_FOREVER);

    proxyAddrIn.s_addr = inet_addr (proxyAddr);
    if ((pNet = proxyNetFind (&proxyAddrIn)) != NULL)
	{					/* clear out the clients */
        for (pNode = lstFirst (&pNet->clientList); pNode != NULL;)
	    {
	    pClient = NODE_TO_CLIENT (pNode);
	    proxyClientRemove (pClient);
	    pNode = lstNext (pNode);
	    KHEAP_FREE((caddr_t)pClient);
	    }

    	lstDelete (&proxyNetList, &pNet->netNode);
    	KHEAP_FREE((caddr_t)pNet);
	status = OK;
	}

    semGive (proxySemId);
    return (status);
    }

/*******************************************************************************
*
* proxyNetFind - find a proxy network structure
*
* This routine finds the proxy network structure associated 
* with <proxyAddr>.  The `proxySemId' semaphore must already 
* be taken before calling this routine.
*
* RETURNS: A PROXY_NET pointer if found, otherwise NULL.
*
* ERRNO: 
*  S_proxyArpLib_INVALID_NET
*
* NOMANUAL
*/

LOCAL PROXY_NET * proxyNetFind
    (
    struct in_addr *		pProxyAddr	/* proxy address */
    )


    {
    PROXY_NET *			pNet;		/* proxy network */

    for (pNet = (PROXY_NET *) lstFirst (&proxyNetList); (pNet != NULL);
	 pNet = (PROXY_NET *) lstNext (&pNet->netNode))
	if (pNet->proxyAddr.s_addr == pProxyAddr->s_addr)
	    return (pNet);

    errno = S_proxyArpLib_INVALID_PROXY_NET;
    return (NULL);
    }

/*******************************************************************************
*
* proxyIsAMainNet - is the interface a main network ?
*
* This routine determines if <mainAddr> is a main network.  
* `proxySemId' must already be taken before this routine is called.
*
* RETURNS: TRUE if a main network, FALSE otherwise.
*
* NOMANUAL
*/

LOCAL BOOL proxyIsAMainNet
    (
    struct in_addr *		pMainAddr	/* main address */
    )

    {
    PROXY_NET *			pNet;		/* proxy network */

    for (pNet = (PROXY_NET *) lstFirst (&proxyNetList); (pNet != NULL);
	 pNet = (PROXY_NET *) lstNext (&pNet->netNode))
	if (pNet->mainAddr.s_addr == pMainAddr->s_addr)
	   return (TRUE);

    return (FALSE);
    }

/*******************************************************************************
*
* proxyNetShow - show proxy ARP networks
*
* This routine displays the proxy networks and their associated clients.
*
* EXAMPLE
* .CS
*     -> proxyNetShow
*     main interface 147.11.1.182 proxy interface 147.11.1.183
*        client 147.11.1.184
* .CE
*
* RETURNS: N/A
*
* INTERNAL
* calls printf while semaphore taken.
*/

void proxyNetShow (void)

    {
    PROXY_NET *		pNet;			/* proxy net		*/
    PROXY_CLNT *	pClient;		/* proxy client		*/
    NODE *		pNode;			/* pointer to node	*/
						/* address in ascii	*/
    char 		asciiAddr [ INET_ADDR_LEN ];

    semTake (proxySemId, WAIT_FOREVER);

    for (pNet = (PROXY_NET *) lstFirst (&proxyNetList);
	 pNet != NULL; pNet = (PROXY_NET *) lstNext (&pNet->netNode))

	{
	inet_ntoa_b (pNet->mainAddr, asciiAddr);
	printf ("main interface %s ", asciiAddr);

	inet_ntoa_b (pNet->proxyAddr, asciiAddr);
	printf ("proxy interface %s\n", asciiAddr);

	for (pNode = lstFirst (&pNet->clientList); pNode != NULL;
	     pNode = lstNext (pNode))
	    {
	    pClient = NODE_TO_CLIENT (pNode);
	    inet_ntoa_b (pClient->ipaddr, asciiAddr);
	    printf ("    client:%s\n", asciiAddr);
	    }
	}
    semGive (proxySemId);
    }

/*******************************************************************************
*
* proxyClientAdd - add a proxy client
*
* This routine adds the client, whose ip address is <pArpcom>.ac_ipaddr, onto 
* the proxy network identified by <pProxyNetAddr>.
*
* RETURNS: OK, or ERROR if unsuccessful.
*
* ERRNO:
*  S_proxyArpLib_INVALID_ADDRESS
*
* NOMANUAL
*/

LOCAL STATUS proxyClientAdd
    (
    struct arpcom * 	pArpcom, 	/* interface on proxy network */
    struct in_addr * 	pClientAddr, 	/* client internet address */
    u_char * 		pClientHwAddr 	/* client hardware address */
    )
    {
    PROXY_NET * 	pNet; 		/* proxy net		*/
    PROXY_CLNT * 	pClient; 	/* proxy client 	*/
    struct sockaddr_in 	sin;		/* sin structure 	*/
    struct ifaddr * 	pIfa;		/* interface address	*/
    struct in_addr * 	pProxyNetAddr;	/* proxy net address	*/
    int flags; 				/* ARP flags */

    /* validate addresses */
    pProxyNetAddr = &pArpcom->ac_ipaddr;

    if ((in_netof (*pProxyNetAddr) != in_netof (*pClientAddr)) ||
        in_broadcast (*pClientAddr, (struct ifnet *)pArpcom))
	{
	errno = S_proxyArpLib_INVALID_ADDRESS;
	return (ERROR);
	}

    if ((pClient = (PROXY_CLNT *) KHEAP_ALLOC(sizeof(PROXY_CLNT))) == NULL)
	return (ERROR);
    bzero ((char *)pClient, sizeof(PROXY_CLNT));

    semTake (proxySemId, WAIT_FOREVER);

    if (!proxyIsAproxyNet (pClientAddr) &&
        ((pNet = (PROXY_NET *) proxyNetFind (pProxyNetAddr)) != NULL))
	{
						/* find main interface */
    	SIN_FILL (&sin, AF_INET, pNet->mainAddr.s_addr, 0);
    	if ((pIfa = ifa_ifwithaddr ((struct sockaddr *) &sin)) != NULL)
	    {
	    (void) proxyClientDelete (pClientAddr);

	    /*
	     * Create a cloneable route (which will ultimately produce a
	     * standard ARP entry for the client) and add the client to
             * the active list.
	     */

	    if (proxyRouteCmd (pClientAddr->s_addr, pProxyNetAddr->s_addr, 
			  SIOCADDRT) == OK)
		{
		pClient->pNet	= pNet;
		pClient->ipaddr	= *pClientAddr;
	        bcopy (pClientHwAddr, pClient->hwaddr,
                       sizeof (struct ether_addr));
              
		hashTblPut (clientTbl, &pClient->hashNode);
		lstAdd (&pNet->clientList, &pClient->clientNode);

                /*
                 * Send a gratuitous ARP request to update any entries
                 * on the main network.
                 */

	        proxyArpSend (pIfa->ifa_ifp, ARPOP_REQUEST, ETHERTYPE_IP,
			      (u_char *) pClientAddr, (u_char *) NULL,
			      (u_char *) pClientAddr);

                /*
                 * Add a proxy ARP entry allowing the standard ARP processing
                 * to reply to requests for this client from hosts on other
                 * networks. Ignore failures caused if an entry already exists.
                 */

                flags = ATF_PUBL | ATF_PROXY | ATF_PERM;
                arpCmd (SIOCSARP, pClientAddr, pClientHwAddr, &flags);

	        if (proxyArpVerbose)
		    logMsg ("added client 0x%x proxyNet 0x%x\n",
			    ntohl (pClientAddr->s_addr), 
			    ntohl (pProxyNetAddr->s_addr), 0, 0, 0, 0);

		semGive (proxySemId);
		return (OK);
		}
	    }
	else
	    {
	    if (proxyArpVerbose)
		logMsg ("proxyClientAdd: no main if 0x%x\n",
			ntohl (pNet->mainAddr.s_addr), 0, 0, 0, 0, 0);
	    }
	}

    semGive (proxySemId);
    KHEAP_FREE((caddr_t)pClient);
    return (ERROR);
    }

/*******************************************************************************
*
* proxyClientDelete - delete a proxy client
*
* This routines deletes the proxy client specified by <pClientAddr>.
*
* RETURNS: OK, or ERROR if unsuccessful.
*
* NOMANUAL
*/

LOCAL STATUS proxyClientDelete
    (
    struct in_addr *		pClientAddr		/* client address   */
    )
    {
    PROXY_CLNT *		pClient;		/* proxy client     */
    STATUS 			status = ERROR;		/* return status    */

    semTake (proxySemId, WAIT_FOREVER);

    if ((pClient = proxyClientFind (pClientAddr)) != NULL)
	{
	proxyClientRemove (pClient);
	lstDelete (&pClient->pNet->clientList, &pClient->clientNode);
    	if (proxyArpVerbose)
	    logMsg ("deleted client 0x%x\n", ntohl (pClientAddr->s_addr), 0, 
		    0, 0, 0, 0);
	status = OK;
	}

    semGive (proxySemId);

    if (pClient != NULL)
	KHEAP_FREE((caddr_t)pClient);

    return (status);
    }

/*******************************************************************************
*
* proxyClientFind - find a proxy client
*
* This routine finds the proxy client structure associated with <pClientAddr>.
* Caller must have taken `proxySemId' before this routine is called.
*
* RETURNS: Pointer to PROXY_CLNT if found, NULL otherwise.
*
* ERRNO:
*  S_proxyArpLib_INVALID_CLIENT
*
* NOMANUAL
*/

LOCAL PROXY_CLNT * proxyClientFind
    (
    struct in_addr *		pClientAddr		/* client address    */
    )

    {
    PROXY_CLNT			client;			/* client to find    */
    PROXY_CLNT *		pClient;		/* pointer to client */

    client.ipaddr.s_addr = pClientAddr->s_addr;
    if ((pClient = (PROXY_CLNT *)
	   hashTblFind (clientTbl, &client.hashNode, 0)) == NULL)
    	errno = S_proxyArpLib_INVALID_CLIENT;

    return (pClient);
    }

/*******************************************************************************
*
* proxyPortFwdOn - enable broadcast forwarding for a particular port
*
* This routine enables broadcasts destined for the port, <port>, to be 
* forwarded to and from the proxy network.  To enable all ports, specify 
* zero for <port>.
*
* RETURNS: OK, or ERROR if unsuccessful.
*/

STATUS proxyPortFwdOn
    (
    int			port			/* port number 		*/
    )
    {
    PORT_NODE *		pPort;			/* port node 		*/

    if ((pPort = (PORT_NODE *) KHEAP_ALLOC(sizeof(PORT_NODE))) == NULL)
	return (ERROR);
    bzero ((char *)pPort, sizeof(PORT_NODE));

    semTake (portTblSemId, WAIT_FOREVER);

    if (portTblFind (port) != NULL)	/* already enabled so ok */
	{
	semGive (portTblSemId);
	KHEAP_FREE((caddr_t)pPort);
	return (OK);
	}

    pPort->port = (int) port;
    (void) hashTblPut (portTbl, &pPort->hashNode);
    semGive (portTblSemId);
    return (OK);
    }

/*******************************************************************************
*
* proxyPortFwdOff - disable broadcast forwarding for a particular port
*
* This routine disables broadcast forwarding on port number <port>.  To
* disable the (previously enabled) forwarding of all ports via
* proxyPortFwdOn(), specify zero for <port>.
*
* RETURNS: OK, or ERROR if unsuccessful.
*/

STATUS proxyPortFwdOff
    (
    int			port			/* port number		*/
    )
    {
    PORT_NODE *		pPort;			/* port node 		*/

    semTake (portTblSemId, WAIT_FOREVER);

    if ((pPort = portTblFind (port)) == NULL)
	{				/* not in port table so ok */
	semGive (portTblSemId);
	return (OK);
	}

    (void) hashTblRemove (portTbl, &pPort->hashNode);
    semGive (portTblSemId);
    KHEAP_FREE((caddr_t)pPort);
    return (OK);
    }

/*******************************************************************************
*
* portTblFind - find the port node
*
* This routine finds the port node associated with port number <port>.
*
* RETURNS: A pointer to the port node, or NULL if not found.
*
* NOMANUAL
*/

LOCAL PORT_NODE * portTblFind
    (
    int		port			/* port number		*/
    )
    {
    PORT_NODE		portNode;		/* port node		*/

    portNode.port = (int) port;
    return ((PORT_NODE *) hashTblFind (portTbl, &portNode.hashNode, 0));
    }

/*******************************************************************************
*
* proxyPortShow - show ports enabled for broadcast forwarding
*
* This routine displays the destination ports for which the proxy ARP server
* will forward broadcast messages between the physically separate networks.
*
* EXAMPLE
* .CS
*     -> proxyPortShow
*     enabled ports:
*        port 67
* .CE
*
* RETURNS: N/A
*
* INTERNAL
* calls printf() while semaphore taken.
*/

void proxyPortShow (void)
    {
    semTake (portTblSemId, WAIT_FOREVER);

    printf ("enabled ports:\n");
    if (portTblFind (0) != NULL)
	printf ("    all ports enabled\n");
    else
	hashTblEach (portTbl, proxyPortPrint, 0);

    semGive (portTblSemId);
    }

/*******************************************************************************
*
* proxyPortPrint - print ports enabled
*
* This routine prints the list of enabled ports.
*
* RETURNS: TRUE (always)
*
* NOMANUAL
*/

LOCAL BOOL proxyPortPrint
    (
    PORT_NODE *		pPort				/* port node 	*/
    )

    {
    printf ("    port %d\n", pPort->port);
    return (TRUE);
    }

/*******************************************************************************
*
* proxyBroadcastInput - hook routine for broadcasts
*
* This routine is the hook routine that forwards a broadcast datagram
* <pMbuf> which was received on interface <pIf> to and from proxy interfaces.
* As a measure of control, it only forwards broadcasts that are destined for
* ports that have been previously enabled (with proxyPortFwdOn()).
* proxyBroadcastInput() gets called from routine ipintr(), in file ip_input.c.
* It assumes the datagram is whole (i.e., if the datagram was fragmented,
* then it was previously reassembled).
*
* If the broadcast datagram came from a main network, then forward the
* broadcast to all proxy network's that have the input interface as their
* main network.  If the broadcast came from a proxy network, then forward
* the broadcast to all other proxy networks that have that proxy network's main
* network as their main network (and forward it to the main network as well).
*
* INTERNAL
* Would we ever want to forward anything beyond UDP broadcasts (what
* about ICMP?)? The old backplane driver makes a local copy of broadcasts
* so we'll get an extra copy of broadcast packets if we are using the old
* backplane driver.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void proxyBroadcastInput
    (
    struct mbuf *			pMbuf,		/* mbuf chain	     */
    struct ifnet *			pIf		/* interface pointer */
    )

    {
    struct in_addr			inputAddr;	/* input interface   */
    PROXY_NET *				pNet;		/* proxy network     */
    struct in_addr			mainAddr;	/* main interface    */
    struct ifaddr *			pIfa;		/* if address	     */
    struct sockaddr_in			sin;		/* sin structure     */
    struct ip *				pIP;		/* ip header pointer */
    int					hlen;		/* ip header length  */
    struct udphdr *			pUDP;		/* udp header 	     */

    if (!proxyBroadcastFwd) 			/* broadcasts got turned off */
	return;

    if (pMbuf->m_len < sizeof (struct ip) &&
	(pMbuf = m_pullup(pMbuf, sizeof (struct ip))) == 0)
	return;
    
    pIP = mtod (pMbuf, struct ip *);		/* pulled tight in ip_input */

    /*
     * Only forward UDP broadcast packets.
     */

    if (pIP->ip_p != IPPROTO_UDP)
	return;

    hlen = pIP->ip_hl << 2;

    /*
     * make sure IP header (with options) and UDP header fit into the
     * first mbuf.
     */
    if (pMbuf->m_len < (hlen + sizeof (struct udphdr)))
	{
        if ((pMbuf = m_pullup (pMbuf, hlen + sizeof (struct udphdr))) == 0)
	    return;
    	pIP = mtod (pMbuf, struct ip *);
	}

    pUDP = (struct udphdr *) ((u_char *) pIP + hlen);

    if (!portTblFind (ntohs (pUDP->uh_dport)) && !portTblFind ((u_short) 0))
	return;

    semTake (proxySemId, WAIT_FOREVER);

    inputAddr.s_addr  =  ((struct arpcom *) pIf)->ac_ipaddr.s_addr;

    /* Validate the interface which received the original broadcast message. */

    pNet = proxyNetFind (&inputAddr);

    if (pNet == NULL && !proxyIsAMainNet (&inputAddr))
        {
        /*
         * The receiving interface is not part of any proxy network or
         * main network. Don't forward the broadcast message.
         */

        semGive (proxySemId);
        return;
        }
    
    if (pNet)
	{
	/*
	 * Input interface was a proxy network.  Find main network associated
	 * with that proxy network (and the interface to the main network)
	 * and forward the datagram onto it.
	 */

	mainAddr.s_addr = pNet->mainAddr.s_addr;
    	SIN_FILL(&sin, AF_INET, pNet->mainAddr.s_addr, 0);
    	if ((pIfa = ifa_ifwithaddr ((struct sockaddr *) &sin)) != NULL)
	    proxyBroadcast (pIfa->ifa_ifp, pMbuf, pIP->ip_ttl - 1);
	}
    else
	{
    	mainAddr.s_addr = inputAddr.s_addr;
	}

    for (pNet = (PROXY_NET *) lstFirst (&proxyNetList); (pNet != NULL);
	 pNet = (PROXY_NET *) lstNext (&pNet->netNode))
	{
    	/*
	 * Forward packet to all appropriate proxy networks. (A copy is not
         * sent to any proxy network which contained the original broadcast).
     	 */

	if ((pNet->mainAddr.s_addr == mainAddr.s_addr)  &&
	    (pNet->proxyAddr.s_addr != inputAddr.s_addr))
	    {
	    /* find proxy interface and ship it off */

    	    SIN_FILL(&sin, AF_INET, pNet->proxyAddr.s_addr, 0);
    	    if ((pIfa = ifa_ifwithaddr ((struct sockaddr *) &sin)) != NULL)
		proxyBroadcast (pIfa->ifa_ifp, pMbuf, 1);
	    }
	}

    semGive (proxySemId);
    }

/*******************************************************************************
*
* proxyBroadcast - send a broadcast
*
* This routine sends a copy of the broadcast message <pMbuf> on the interface
* <pIf> so that all (physically separated) hosts on a logical subnet receive
* the expected broadcasts.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void proxyBroadcast
    (
    FAST struct ifnet *		pIf,		/* output interface pointer */
    struct mbuf *		pMbuf,  	/* mbuf chain		    */
    int                         ttl             /* time to live */
    )
    {
    struct mbuf *		pMbufCopy;	/* mbuf chain (copy)	    */
    struct sockaddr_in  	sin;		/* address 		    */
    struct ip *			pIP;		/* ip header pointer	    */

    /* 
     * Copy the message to protect forwarded packet from changes made by 
     * upper layers.
     */

    if ((pMbufCopy = (struct mbuf *) proxyPacketDup (pMbuf)) == NULL)
	return;
    pMbufCopy->mBlkHdr.mFlags |= M_PROXY;

    pIP = mtod (pMbufCopy, struct ip *);

    if (proxyBroadcastVerbose)
    	logMsg ("forward broadcast (if:0x%x) src 0x%x [%d] dst 0x%x [%d]\n",
	        ((struct arpcom *) pIf)->ac_ipaddr.s_addr,
		pIP->ip_src.s_addr, ((struct udpiphdr *) pIP)->ui_sport,
		pIP->ip_dst.s_addr, ((struct udpiphdr *) pIP)->ui_dport, 0);

    SIN_FILL(&sin, AF_INET, pIP->ip_dst.s_addr, 0);

    /* Adjust the IP packet length (modified in receive processing). */

    pIP->ip_len +=  pIP->ip_hl << 2;

    /* Convert appropriate header fields to network order for transmission. */

    pIP->ip_id  = htons ((u_short) pIP->ip_id);	
    pIP->ip_len = htons ((u_short) pIP->ip_len);
    pIP->ip_off = htons ((u_short) pIP->ip_off);
    pIP->ip_ttl = ttl;

    pIP->ip_sum = 0;
    pIP->ip_sum = in_cksum (pMbufCopy, pIP->ip_hl << 2); 

    (*pIf->if_output)(pIf, pMbufCopy, (struct sockaddr *) &sin, 
		      (struct rtentry *) NULL);
    }

/*******************************************************************************
*
* proxyMsgInput - input routine for proxy messages
*
* This routine is the input hook routine for proxy messages.  It handles
* the probe, client register and client unregister messages.  For a probe
* message it just sends an ACK.  PROXY_REG and PROXY_UNREG messages cause
* the proxy server to add the client (via proxyClientAdd()) and delete the
* (via proxyClientDelete()), respectively then reply with an ACK.
*
* <pArpcom> is the arpcom structure for the input interface.  <pMbuf> is the
* mbuf chain.  <len> is the length of the message.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void proxyMsgInput
    (
    FAST struct arpcom *	pArpcom,	/* arpcom structure 	*/
    FAST struct mbuf *		pMbuf,         	/* mbuf chain       	*/
    int                         len             /* length           	*/
    )

    {
    PROXY_MSG *             	pMsg;		/* proxy message	*/
    int				op;		/* operation 		*/
    BOOL                        sendACK = FALSE;/* send client ack	*/

    if (pMbuf->m_len < sizeof (PROXY_MSG))      /* check message length */
        {
        m_freem (pMbuf);
        return;
        }

    pMsg = mtod (pMbuf, PROXY_MSG *);
    op = ntohl (pMsg->op);

    if (proxyArpVerbose)
    	logMsg ("(%d) from 0x%x [%s]\n", op, ntohl (pMsg->clientAddr.s_addr),
                (int) ether_sprintf (pMsg->clientHwAddr), 0, 0, 0);

    if (proxyLibInitialized)
        {
    	if (op == PROXY_PROBE)       /* probe message        */
	    sendACK = TRUE;
	
        semTake (proxySemId, WAIT_FOREVER);

        if (op == PROXY_REG)    /* add client message   */
	    {
            if (proxyIsAClient (&pMsg->clientAddr))
                sendACK = TRUE;    /* Acknowledge duplicate registrations. */
	    else if (proxyClientAdd (pArpcom, &pMsg->clientAddr,
                pMsg->clientHwAddr) == OK)
                sendACK = TRUE;
	    }

        if (op == PROXY_UNREG) /* delete client message */
            {
            (void) proxyClientDelete (&pMsg->clientAddr);
            sendACK = TRUE;
            }

        semGive (proxySemId);

	if (sendACK)
	    {
	    struct ether_header *	pEh;    /* ether header         */
	    struct sockaddr		sa;     /* sockaddr structure   */
	    struct ifnet *		pIf;	/* interface pointer  	*/

	    /* fill in proxy message */

            pMsg->op = htonl (PROXY_ACK);
            pMsg->serverAddr.s_addr = pArpcom->ac_ipaddr.s_addr;
            bcopy ((caddr_t) pArpcom->ac_enaddr, (caddr_t) pMsg->serverHwAddr,
                   sizeof (pMsg->serverHwAddr));

	    if (proxyArpVerbose)
                logMsg ("proxy ACK sent to 0x%x\n", 
			ntohl (pMsg->clientAddr.s_addr), 0, 0, 0, 0, 0);

	    bzero ((caddr_t) &sa, sizeof (sa));
	    sa.sa_family = AF_UNSPEC;

	    /* fill in ethernet header */

	    pEh = (struct ether_header *) sa.sa_data;
	    pEh->ether_type = PROXY_TYPE;
	    pIf = (struct ifnet *) pArpcom;
	    bcopy ((caddr_t) pMsg->clientHwAddr, (caddr_t) pEh->ether_dhost,
 	           sizeof (pEh->ether_dhost));

	    (*pIf->if_output) (pIf, pMbuf, &sa, (struct rtentry *) NULL);
	    return;
            }
        }

    m_freem (pMbuf);
    }

/*******************************************************************************
*
* proxyRouteCmd - add and delete a proxy client routes
*
* This routine adds and deletes routes to the proxy clients with the netmask
* of 0xffffffff so that arp route entries to the proxy clients can be cloned
* from the route added by this routine.
*
* RETURNS: OK, or ERROR if unsuccessful.
*
* NOMANUAL
*/

LOCAL STATUS proxyRouteCmd
    (
    int		destInetAddr,		/* destination adrs */
    int		gateInetAddr,		/* gateway adrs */
    int		ioctlCmd		/* route command */
    )
    {
    struct sockaddr 	destAddr;
    struct sockaddr 	gateWayAddr;
    struct sockaddr 	netMask;
    
    bzero((char *)(&destAddr), sizeof(struct sockaddr));

    destAddr.sa_family	= AF_INET;
    destAddr.sa_len	= sizeof(struct sockaddr_in);	
    ((struct sockaddr_in *)&destAddr)->sin_addr.s_addr = (u_long)destInetAddr;

    /* zero out sockaddr_in, fill in gateway info */

    bzero ((char *)&gateWayAddr, sizeof(struct sockaddr));
    gateWayAddr.sa_family	= AF_INET;
    gateWayAddr.sa_len	= sizeof(struct sockaddr_in);	
    ((struct sockaddr_in *)&gateWayAddr)->sin_addr.s_addr = 
     (u_long)gateInetAddr;

    /* initialize the netmask to 0xffffffff */

    netMask.sa_family 	= AF_INET;
    netMask.sa_len	= 8; 
    ((struct sockaddr_in *)&netMask)->sin_addr.s_addr = 0xffffffff;
    in_socktrim ((struct sockaddr_in *)&netMask); 

    return (rtrequest (((ioctlCmd == SIOCADDRT) ? RTM_ADD : RTM_DELETE), 
	    &destAddr, &gateWayAddr, &netMask, RTF_CLONING, NULL)); 
    }

/*******************************************************************************
*
* proxyClientRemove - clear client structures
*
* This routine deletes the client routes from the route table.
*
* NOMANUAL
*/

LOCAL void proxyClientRemove
    (
    PROXY_CLNT * pClient
    )
    {
    int flags = ATF_PROXY;

    (void) hashTblRemove (clientTbl, &pClient->hashNode);

    /* Remove proxy ARP entry explicitly */

    (void) arpCmd (SIOCDARP, &pClient->ipaddr, (u_char *)  NULL,
                   &flags);

    /* Remove regular ARP entry */

    (void) arpCmd (SIOCDARP, &pClient->ipaddr, (u_char *)  NULL,
				   (int *) NULL);

    (void) proxyRouteCmd (pClient->ipaddr.s_addr, 
                     pClient->pNet->proxyAddr.s_addr, SIOCDELRT);
    }


/*******************************************************************************
*
* proxyPacketDup - copies IP header, dups the rest of the packet
*
* This routine makes a new mbuf chain copying the IP header of the
* original packet to a newly allocated cluster and copying by reference
* the rest of the packet.  This kind of copying is necessary if the IP
* header must be altered to reflect a new TTL when broadcast packets
* are forwarded.
*
* RETURNS: An M_BLK_ID for the new mbuf chain if successful, NULL
*          otherwise.
*
* NOMANUAL
*/

LOCAL M_BLK_ID proxyPacketDup
    (
    M_BLK_ID pMblk      /* pointer to source mBlk */
    )
    {
    M_BLK_ID pNewMblk = NULL;
    M_BLK_ID pPktMblk = NULL;
    int ipHeaderSize = sizeof (struct ip);
    int bytesDuped = 0;

    if (pMblk == NULL)
        return (NULL);

    /* First mbuf must have enough room for link layer header */

    if ((pNewMblk = netTupleGet (_pNetDpool, max_linkhdr + ipHeaderSize,
				 M_DONTWAIT, MT_DATA, TRUE)) == NULL)
	return (NULL);

    /* Copy IP header which is in the first mbuf (done by m_pullup) */

    pNewMblk->mBlkHdr.mData += max_linkhdr; /* Leave room */

    bcopy (pMblk->mBlkHdr.mData, pNewMblk->mBlkHdr.mData, ipHeaderSize);
    pNewMblk->mBlkHdr.mLen = ipHeaderSize;

    if (pMblk->mBlkHdr.mFlags & M_PKTHDR)
        {
        /* Copy packet header information */

	pNewMblk->mBlkPktHdr = pMblk->mBlkPktHdr;
	pNewMblk->mBlkHdr.mFlags = pMblk->mBlkHdr.mFlags;
	}

    /* Copy by reference the rest of the packet */

    bytesDuped = pMblk->mBlkPktHdr.len - ipHeaderSize;

    pPktMblk = netMblkChainDup (_pNetDpool, pMblk, ipHeaderSize,
				bytesDuped, M_DONTWAIT);

    if (pPktMblk == NULL)
        {
        netMblkClChainFree(pNewMblk);
        return (NULL);
        }

    /* Make chain */

    pNewMblk->mBlkPktHdr.len = pMblk->mBlkPktHdr.len;
    pNewMblk->mBlkHdr.mNext = pPktMblk;

    return (pNewMblk);
    }


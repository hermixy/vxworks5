/* wdbUdpSockLib.c - WDB communication interface for UDP sockets */

/* Copyright 1984-1994 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,17mar98,jmb merge from HPSIM, jmb patch 20mar97, rtalloc fails if port
                number is not zero in socket address.
01c,27sep95,ms  put netif MTU in global variable (SPR #4645).
01b,03jun95,ms	use new commIf structure (sendto/rvcfrom replaces read/write).
01a,22dec94,ms  written.
*/

/*
DESCRIPTION

The WDB agent communicates with the host using sendto/rcvfrom function
pointers contained in a WDB_COMM_IF structure.
This library iniitalizes these functions to be callouts to the standard
UDP socket routines.
It supports only "interrupt" mode (meaning that packets arrive for
the agent asynchronously to this library). "Polled" mode is not supported,
because the external mode agent requires that polling routines not
make any calls to the kernel (so simply doing non-blocking I/O is not
enough). As a result, this communication library can only support
a task mode agent.

During agent initialization (in the file target/src/config/usrWdb.c), this
libraries wdbUdpSockIfInit() routine is called to initializes a WDB_COMM_IF
structure. A pointer to this structure is then passed to the agent's
wdbInstallCommIf().

Note: This particular library is not required by the WDB target agent.
Other communication libraries can be used instead simply by initializing
a WDB_COMM_IF structure and installing it in the agent.
*/

#include "vxWorks.h"
#include "stdio.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "net/socketvar.h"
#include "net/route.h"
#include "net/if.h"
#include "netinet/in_systm.h"
#include "netinet/in.h"
#include "netinet/ip.h"
#include "netinet/udp.h"
#include "arpa/inet.h"
#include "sockLib.h"
#include "string.h"
#include "selectLib.h"
#include "ioLib.h"
#include "pipeDrv.h"
#include "netLib.h"
#include "intLib.h"
#include "iv.h"
#include "wdb/wdb.h"
#include "wdb/wdbSvcLib.h"
#include "wdb/wdbCommIfLib.h"
#include "wdb/wdbLibP.h"

/* external functions */

extern void rtalloc();
extern void rtfree();

/* static variables */

static int	wdbUdpSock;		/* for the agent to read from */
static int	wdbUdpCancelSock;	/* to "cancel read" to wdbUdpSock */

/* foward declarations */

static int  wdbUdpSockRcvfrom	(WDB_COMM_ID commId, caddr_t addr, uint_t len,
				 struct sockaddr_in *pAddr, struct timeval *tv);
static int  wdbUdpSockSendto	(WDB_COMM_ID commId, caddr_t addr, uint_t len,
				 struct sockaddr_in *pAddr);
static int  wdbUdpSockModeSet	(WDB_COMM_ID commId, uint_t newMode);
static int  wdbUdpSockCancel	(WDB_COMM_ID commId);

/******************************************************************************
*
* wdbUdpSockIfInit - Initialize the WDB communication function pointers.
*/

STATUS wdbUdpSockIfInit
    (
    WDB_COMM_IF *	pWdbCommIf
    )
    {
    struct sockaddr_in srvAddr;

    pWdbCommIf->rcvfrom	= wdbUdpSockRcvfrom;
    pWdbCommIf->sendto	= wdbUdpSockSendto;
    pWdbCommIf->modeSet = wdbUdpSockModeSet;
    pWdbCommIf->cancel  = wdbUdpSockCancel;
    pWdbCommIf->hookAdd = NULL;
    pWdbCommIf->notifyHost = NULL;

    /* Create the wdbUdpSocket */

    if ((wdbUdpSock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        {
        printErr ("Socket failed\n");
        return (ERROR);
        }

    /* Bind to WDB agents port number */

    bzero ((char *)&srvAddr, sizeof(srvAddr));
    srvAddr.sin_family      = AF_INET;
    srvAddr.sin_port        = htons(WDBPORT);
    srvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind (wdbUdpSock, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0 )
        {
        printErr ("Bind failed\n");
        return (ERROR);
        }

    /* Create the wdbUdpCancelSocket */

    if ((wdbUdpCancelSock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        {
        printErr ("Socket failed\n");
        return (ERROR);
	}

    /* connect it (for writes) to the wdbUdpSocket */

    bzero ((char *)&srvAddr, sizeof(srvAddr));
    srvAddr.sin_family      = AF_INET;
    srvAddr.sin_port        = htons(WDBPORT);
    srvAddr.sin_addr.s_addr = inet_addr ("127.0.0.1");

    if (connect (wdbUdpCancelSock, (struct sockaddr *)&srvAddr, sizeof(srvAddr))
				== ERROR)
	{
        printErr ("Connect failed\n");
	return (ERROR);
	}

    return (OK);
    }


/******************************************************************************
*
* wdbUdpSockModeSet - Set the communication mode (only INT is supported).
*/

static int wdbUdpSockModeSet
    (
    WDB_COMM_ID commId,
    uint_t	newMode
    )
    {
    if (newMode != WDB_COMM_MODE_INT)
	return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* wdbUdpSockRcvfrom - read data from the UDP socket with a timeout.
*/

static int wdbUdpSockRcvfrom
    (
    WDB_COMM_ID commId,
    caddr_t	addr,
    uint_t	len,
    struct sockaddr_in *pSockAddr,
    struct timeval *tv
    )
    {
    uint_t		nBytes;
    int 		addrLen = sizeof (struct sockaddr_in);
    struct fd_set	readFds;
    static struct route	theRoute;
    struct sockaddr_in 	sockAddr;

    /* wait for data with a timeout */

    FD_ZERO (&readFds);
    FD_SET  (wdbUdpSock, &readFds);

    if (select (wdbUdpSock + 1, &readFds, NULL, NULL, tv) < 0)
        {
        printErr ("wdbUdpSockLib: select failed!\n");
        return (0);
        }

    if (!FD_ISSET (wdbUdpSock, &readFds))
        {
        return (0);             /* select timed out */
        }

    /* read the data */

    nBytes   = recvfrom (wdbUdpSock, addr, len, 0,
			(struct sockaddr  *)pSockAddr, &addrLen);

    if (nBytes < 4)
	return (0);

    /* 
     * The following is a fix for SPR #4645 (the agent does
     * not report the correct MTU to the host). We lower
     * the value of the global variable wdbCommMtu if a packet
     * arrives via an interface with a smaller MTU.
     *
     * XXX This fix has some problems:
     *   1) it uses rtalloc, which is not an approved interface.
     *      Note: rtalloc doesn't seem to like having the port number
     *            in the socket address.
     *   2) it assumes all agent packets arrive via the same netif.
     *   3) it doesn't take into account intermediate MTUs in a WAN.
     * The right fix for (3) would be to use an MTU-discovey
     * algorithm (e.g. send larger and larger IP datagrams with the
     * "don't fragment" flag until an ICMP error is returned).
     */

    if (theRoute.ro_rt == NULL)
	{
	sockAddr = *pSockAddr;
	sockAddr.sin_port = 0;
	theRoute.ro_dst = * (struct sockaddr *) &sockAddr;
	rtalloc (&theRoute);
	if (theRoute.ro_rt != NULL)
	    {
	    if (wdbCommMtu > theRoute.ro_rt->rt_ifp->if_mtu)
		wdbCommMtu = theRoute.ro_rt->rt_ifp->if_mtu;
	    rtfree (theRoute.ro_rt);
	    }
	}

    return (nBytes);
    }

/******************************************************************************
*
* wdbUdpSockCancel - cancel a wdbUdpSockRcvfrom
*/

static int wdbUdpSockCancel
    (
    WDB_COMM_ID commId
    )
    {
    char 		dummy;

    netJobAdd (write, wdbUdpCancelSock, (int)&dummy, 1, 0, 0);

    return (OK);
    }

/******************************************************************************
*
* wdbUdpSockSendto - write data to a UDP socket.
*/

static int wdbUdpSockSendto
    (
    WDB_COMM_ID commId,
    caddr_t addr,
    uint_t len,
    struct sockaddr_in * pRemoteAddr
    )
    {
    uint_t		nBytes;

    nBytes = sendto (wdbUdpSock, addr, len, 0, (struct sockaddr *)pRemoteAddr,
			sizeof (*pRemoteAddr));

    return (nBytes);
    }


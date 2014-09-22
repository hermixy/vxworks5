/* wdbUdpLib.c - WDB communication interface UDP datagrams */

/* Copyright 1994-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,14sep01,jhw  Fixed warnings from compiling with gnu -pedantic flag
01g,22jul98,dbt  in udpSendTo(), test wdbComMode rather thant wdbMode (with
                 wdbIsNowTasking) to choose if we must use semTake and
                 semGive.
01f,07apr97,gnn  turned version number into a #define
01e,07apr97,gnn  really added version number, was not really there.
01d,22nov96,vin  made modifications required for BSD44 cluster code.
		 added udpIpHdrBuf.
01c,29feb96,ms   added IP version number to IP header (SPR #5867).
01b,03jun95,ms	 use new commIf structure (sendto/rvcfrom replaces read/write).
01a,06oct94,ms   written.
*/

/*
DESCRIPTION

The WDB agent communicates with the host using sendto/rcvfrom function
pointers contained in a WDB_COMM_IF structure.
This library implements these methods using a lightweight UDP/IP stack
as the transport.
It supports two modes of operation; interrupt and polled, and the
mode can be dynamically toggled.

During agent initialization (in the file target/src/config/usrWdb.c), this
libraries udpCommIfInit() routine is called to initializes a WDB_COMM_IF
structure. A pointer to this structure is then passed to the agent's
wdbInstallCommIf().

Note: This particular library is not required by the WDB target agent.
Other communication libraries can be used instead simply by initializing
a WDB_COMM_IF structure and installing it in the agent.
*/

#include "wdb/wdb.h"
#include "wdb/wdbCommIfLib.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbRtIfLib.h"
#include "wdb/wdbUdpLib.h"
#include "wdb/wdbMbufLib.h"
#include "string.h"
#include "netinet/in_systm.h"
#include "netinet/in.h"
#include "netinet/ip.h"
#include "netinet/udp.h"

#define IP_HDR_SIZE	20
#define UDP_HDR_SIZE	8
#define UDP_IP_HDR_SIZE IP_HDR_SIZE + UDP_HDR_SIZE

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/* State Information */

static u_int    commMode;		/* POLL or INT */
static int	agentIp;		/* in network byte order */

/* This is a volatile pointer. Not a pointer to volatile data. */

static struct mbuf * volatile pInputMbufs;

static WDB_DRV_IF * pWdbDrvIf;

static void	(*udpHookRtn) (u_int arg);
static u_int	udpHookArg;

static void *	readSyncSem;
static void *	writeSyncSem;
static char 	udpIpHdrBuf [128]; 	/* 128 byte buffer for header */

/* foward declarations */

static int  udpRcvfrom	(WDB_COMM_ID commId, caddr_t addr, u_int len,
			 struct sockaddr_in *pAddr, struct timeval *tv);
static int  udpSendto	(WDB_COMM_ID commId, caddr_t addr, u_int len,
			 struct sockaddr_in *pAddr);
static int  udpModeSet	(WDB_COMM_ID commId, u_int newMode);
static int  udpCancel	(WDB_COMM_ID commId);
static int  udpHookAdd	(WDB_COMM_ID commId, void (*rout)(), u_int arg);

extern int wdbCksum (char * pData, int nBytes);

/******************************************************************************
*
* udpCommIfInit - Initialize the WDB communication function pointers.
*
* The configlette for the selected WDB_COMM_TYPE calls this function
* from its wdbCommDevInit routine. This function can only be called once.
*
*/

STATUS udpCommIfInit
    (
    WDB_COMM_IF * pWdbCommIf,		/* function pointers to install */
    WDB_DRV_IF *  pDrvIf
    )
    {
    pWdbCommIf->rcvfrom	= udpRcvfrom;
    pWdbCommIf->sendto	= udpSendto;
    pWdbCommIf->modeSet = udpModeSet;
    pWdbCommIf->cancel  = udpCancel;
    pWdbCommIf->hookAdd = udpHookAdd;
    pWdbCommIf->notifyHost = NULL;

    pWdbDrvIf		= pDrvIf;

    commMode = WDB_COMM_MODE_INT;	/* default communication mode */

    if (pWdbRtIf->semCreate != NULL)
	{
	readSyncSem = pWdbRtIf->semCreate();
	writeSyncSem = pWdbRtIf->semCreate();
	}

    return (OK);
    }


/******************************************************************************
*
* udpModeSet - Set the communication mode to POLL or INT.
*
* RETURNS: OK on success, or ERROR if the new mode is not supported.
*/

static int udpModeSet
    (
    WDB_COMM_ID commId,
    u_int newMode
    )
    {
    if (pWdbDrvIf->modeSetRtn == NULL)
	return (ERROR);

    if ((*pWdbDrvIf->modeSetRtn)(pWdbDrvIf->devId, newMode) == ERROR)
	return (ERROR);

    commMode = newMode;

    return (OK);
    }

/******************************************************************************
*
* udpRcv - receive a UDP/IP datagram from the driver.
*/

void udpRcv
    (
    struct mbuf * pMbuf		/* mbuf chain received */
    )
    {
    /* store away the data for the next agent read */

    if (pInputMbufs != NULL)		/* only allow one queued request */
	{
	wdbMbufChainFree (pInputMbufs);
	return;
	}
    pInputMbufs        = pMbuf;

    /* interrupt on first packet mode? Then call the input hook */

    if ((commMode == WDB_COMM_MODE_INT) && (udpHookRtn != NULL))
	{
	(*udpHookRtn) (udpHookArg);
	return;
	}

    /* normal tasking mode? Then unblock the task */

    if (commMode == WDB_COMM_MODE_INT)
	{
	pWdbRtIf->semGive (readSyncSem);
	}
    }

/******************************************************************************
*
* udpRcvfrom - read a UDP/IP datagram (after removing the UDP/IP headers).
*
* This routine assumes that the mbuf chain received from the driver
* has a UDP/IP header in the first mbuf.
*
* In polled mode operation, there is no timeout. This needs to be fixed 
* because it could cause an infinite loop waiting for data.
*
*/

static int udpRcvfrom
    (
    WDB_COMM_ID	commId,
    caddr_t	addr,
    u_int	len,
    struct sockaddr_in *pAddr,
    struct timeval *tv
    )
    {
    struct udphdr *	pUdpHdr;
    struct ip *		pIpHdr;
    u_int		nBytes;
   
    /* WDB_COMM_MODE_INT means "task mode". Use semaphore to wait for data */

    if (commMode == WDB_COMM_MODE_INT)
	pWdbRtIf->semTake (readSyncSem, tv);

    /* 
     * Otherwise poll the driver until data arrives or a timeout occurs.
     * Note that presently the timeout is not implemented
     */

    else
	{
	while (pInputMbufs == NULL)
	    (*pWdbDrvIf->pollRtn) (pWdbDrvIf->devId);
	}

    /* Any data available ? */

    if (pInputMbufs == NULL)
	{
	return (0);
	}

    /* If so, break the packet up into components */

    pIpHdr  = mtod (pInputMbufs, struct ip *);
    pUdpHdr = (struct udphdr *)((int)pIpHdr + IP_HDR_SIZE);

    /* If this packet is not for the agent, ignore it */

    if ((pIpHdr->ip_p      != IPPROTO_UDP) ||
	(pUdpHdr->uh_dport != htons(WDBPORT)) ||
	(pInputMbufs->m_len < UDP_IP_HDR_SIZE))
	{
	nBytes = 0;
        goto done;
	}

    /* Save away reply address info */

    agentIp		= pIpHdr->ip_dst.s_addr;
    pAddr->sin_port	= pUdpHdr->uh_sport;
    pAddr->sin_addr	= pIpHdr->ip_src;

    /* copy the RPC data into the uses buffer */

    pInputMbufs->m_len -= UDP_IP_HDR_SIZE;
    pInputMbufs->m_data += UDP_IP_HDR_SIZE;
    wdbMbufDataGet (pInputMbufs, addr, len, &nBytes);

done:
    /* free the mbuf chain */

    wdbMbufChainFree (pInputMbufs);
    pInputMbufs = NULL;

    return (nBytes);
    }

/******************************************************************************
*
* udpCancel - cancel a udpRcvfrom
*/

static int udpCancel
    (
    WDB_COMM_ID commId
    )
    {
    pWdbRtIf->semGive (readSyncSem);

    return (OK);
    }

/******************************************************************************
*
* udpSendto - send a reply packet (after adding a UDP/IP header).
*/

static int udpSendto
    (
    WDB_COMM_ID commId,
    caddr_t addr,
    u_int len,
    struct sockaddr_in *pAddr
    )
    {
    static u_short	ip_id;
    struct udphdr *     pUdpHdr;
    struct ip *         pIpHdr;
    STATUS		status;
    struct mbuf *	pDataMbuf;
    struct mbuf *	pHeaderMbuf;

    /* allocate a couple of mbufs */

    pDataMbuf = wdbMbufAlloc();
    if (pDataMbuf == NULL)
	return (0);

    pHeaderMbuf = wdbMbufAlloc ();
    if (pHeaderMbuf == NULL)
	{
	wdbMbufFree (pDataMbuf);
	return (0);
	}

    /* put the reply data in an mbuf cluster */

    wdbMbufClusterInit (pDataMbuf, addr, len, NULL, 0);

    if (commMode == WDB_COMM_MODE_INT)
	{
	pDataMbuf->m_extFreeRtn	= pWdbRtIf->semGive;
	pDataMbuf->m_extArg1	= (int)writeSyncSem;
	}

    /* put the header data in the other mbuf */
    wdbMbufClusterInit (pHeaderMbuf, udpIpHdrBuf, len, NULL, 0); 

    pHeaderMbuf->m_data +=	40;
    pHeaderMbuf->m_len =	UDP_IP_HDR_SIZE;
    pHeaderMbuf->m_type =	MT_DATA;
    pHeaderMbuf->m_next  =	pDataMbuf;

    pIpHdr	= mtod (pHeaderMbuf, struct ip *);
    pUdpHdr	= (struct udphdr *)((int)pIpHdr + IP_HDR_SIZE);

    pIpHdr->ip_v        = IPVERSION;
    pIpHdr->ip_hl	= 0x45;
    pIpHdr->ip_tos	= 0;
    pIpHdr->ip_off	= 0;
    pIpHdr->ip_ttl	= 0x20;
    pIpHdr->ip_p	= IPPROTO_UDP;
    pIpHdr->ip_src.s_addr = agentIp;
    pIpHdr->ip_dst.s_addr = pAddr->sin_addr.s_addr;
    pIpHdr->ip_len	= htons (UDP_IP_HDR_SIZE + len);
    pIpHdr->ip_id	= ip_id++;
    pIpHdr->ip_sum	= 0;
    pIpHdr->ip_sum	= ~wdbCksum ((char *)pIpHdr, IP_HDR_SIZE);

    pUdpHdr->uh_sport	= htons(WDBPORT);
    pUdpHdr->uh_dport	= pAddr->sin_port;
    pUdpHdr->uh_sum	= 0;
    pUdpHdr->uh_ulen 	= htons (UDP_HDR_SIZE + len);

    /* tell the driver to output the data */

    if (pWdbDrvIf == NULL)
	return (ERROR);
    status = (*pWdbDrvIf->pktTxRtn) (pWdbDrvIf->devId, pHeaderMbuf);
    if (status == ERROR)
	return (ERROR);

    /* wait for the driver to finish transmition before returning */

    if (commMode == WDB_COMM_MODE_INT)
	pWdbRtIf->semTake (writeSyncSem, NULL);

    return (UDP_IP_HDR_SIZE + len);
    }

/******************************************************************************
*
* udpHookAdd -
*/

static int udpHookAdd
    (
    WDB_COMM_ID commId,
    void	(*rout)(),
    u_int	arg
    )
    {
    udpHookRtn = rout;
    udpHookArg = arg;

    return (OK);
    }


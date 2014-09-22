/* wdbEndPktDrv.c - END based packet driver for lightweight UDP/IP */

/* Copyright 1996-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
02c,10apr02,wap  Set the M_PKTHDR flag for mBlks which do in fact contain
                 packet headers. (SPR #73331)
02b,30oct01,jhw  Copied version TOR3_1-RC-1. Fixed ARPOP_REPLY.
02a,11sep00,elg  Update documentation.
01z,20jul00,dbt  Fixed some error cases where mBlk were incorrectly freed.
01y,14jun00,dbt  Fixed problem with Little Endian targets.
01x,12jun00,ham  removed reference to etherLib.
01w,31may00,dbt  Modified wdbEndInt() to test if a packet is fragmented (SPR
                 #30194).
01v,19apr00,ham  merged TOR2_0-NPT-FCS(01s,05mar99,sj-01u,20sep99,sj).
01t,13apr00,dbt  Adapted to new MUX layer.
01u,20sep99,sj   htons(ETHERTYPE_IP)
01t,29apr99,pul  Upgraded NPT phase3 code to tor2.0.0
01s,05mar99,sj   added support for NPT drivers
01s,02aug99,dbt  added support for standalone agent.
		 Added support for ARP requests (SPR #25685).
		 Added IP address verification (SPR #27815).
01r,02mar99,dbt  correctly initialize mBlkPktHdr.len field in send routine
                 (SPR #25330).
01q,11dec97,vin  optimized some code and additional fixes to SPR9563.
01p,08dec97,gnn  fixed spr#9563 to get MBLK chains.
01o,25nov97,gnn  fixed spr#9620 polled mode receive
01n,19oct97,vin  fixed wdbEndInt, wdbEndPoll(), cleanup.
01m,17oct97,vin  changed wdbEndInt for muxPacketDataGet fixes.
01l,09oct97,vin  fixed wdbEndInt various error conditions, added htons
01k,03oct97,gnn  adjusted muxBind call to be in line with new prototype
01j,25sep97,gnn  SENS beta feedback fixes
01i,25aug97,gnn  fixed remaining polled mode problems.
01h,22aug97,gnn  changes due to new buffering scheme.
01g,12aug97,gnn  changes necessitated by MUX/END update.
01f,08apr97,map  Free mbuf cluster in wdbEndTx() when output is busy.
01e,21jan97,gnn  Added code to handle new addressles sending.
                 Changed the way that transmit is done and free'd.
01d,22oct96,gnn  Name changes to follow the coding standards.
01c,22oct96,gnn  Removed netVectors and replaced with netBuffers.
                 Replaced the 0 for promiscous protocol with a constant
                 MUX_PROTO_PROMISC.
01b,23sep96,gnn	 Added new buffering scheme information.
01a,1jul96,gnn   written.
*/

/*
DESCRIPTION

This is an END based driver for the WDB system.  It uses the MUX and END
based drivers to allow for interaction between the target and target
server.

USAGE

The driver is typically only called only from the configlette wdbEnd.c.
The only directly callable routine in this module is wdbEndPktDevInit().
To use this driver, just select the component INCLUDE_WDB_COMM_END in the
folder SELECT_WDB_COMM_TYPE. This is the default selection.
To modify the MTU, change the value of parameter WDB_END_MTU in component
INCLUDE_WDB_COMM_END.

DATA BUFFERING

The drivers only need to handle one input packet at a time because
the WDB protocol only supports one outstanding host-request at a time.
If multiple input packets arrive, the driver can simply drop them.
The driver then loans the input buffer to the WDB agent, and the agent
invokes a driver callback when it is done with the buffer.

For output, the agent will pass the driver a chain of mbufs, which
the driver must send as a packet. When it is done with the mbufs,
it calls wdbMbufChainFree() to free them.
The header file wdbMbufLib.h provides the calls for allocating, freeing,
and initializing mbufs for use with the lightweight UDP/IP interpreter.
It ultimately makes calls to the routines wdbMbufAlloc and wdbMbufFree, which
are provided in source code in the configlette usrWdbCore.c.

INCLUDE FILES: drv/wdb/wdbEndPktDrv.h
*/

/* includes */

#include "string.h"
#include "stdio.h"
#include "errno.h"
#include "sioLib.h"
#include "intLib.h"
#include "logLib.h"
#include "end.h"
#include "muxLib.h"
#include "muxTkLib.h"
#include "endLib.h"
#include "private/muxLibP.h"
#include "stdlib.h"
#include "wdb/wdbMbufLib.h"
#include "drv/wdb/wdbEndPktDrv.h"
#include "net/if.h"
#include "net/if_llc.h"
#include "netinet/ip.h"
#include "netinet/udp.h"
#include "netinet/if_ether.h"
#include "inetLib.h"

/* defines */

#define IP_HDR_SIZE	20

#define WDB_NPT_CAPABLE

/* globals */

WDB_END_PKT_DEV * pEndPktDev;
int wdbEndDebug = 0;
char *pInPkt;

/* pseudo-macros */

#define DRIVER_INIT_HARDWARE(pDev) {}
#define DRIVER_MODE_SET(pDev, mode)
#define DRIVER_PACKET_IS_READY(pDev) FALSE
#define DRIVER_RESET_INPUT(pDev) {pDev->inputBusy=FALSE;}
#define DRIVER_RESET_OUTPUT(pDev) {pDev->outputBusy=FALSE;}
#define DRIVER_GET_INPUT_PACKET(pDev, pBuf) (50)
#define DRIVER_DATA_FROM_MBUFS(pDev, pBuf) {}
#define DRIVER_POLL_TX(pDev, pBuf) {}
#define DRIVER_TX_START(pDev) {}

/* forward declarations */

LOCAL STATUS	wdbEndPoll (void *pDev);
LOCAL STATUS	wdbEndTx   (void *pDev, struct mbuf * pMbuf);
LOCAL STATUS	wdbEndModeSet (void *pDev, uint_t newMode);
LOCAL void	wdbEndInputFree (void *pDev);
LOCAL void	wdbEndOutputFree (void *pDev);
LOCAL M_BLK_ID	wdbEndMblkClGet (END_OBJ * pEnd, int size);
LOCAL int	wdbEndInt (void* pCookie, long type, M_BLK_ID pMblk, 
			LL_HDR_INFO *, void * pNull);
LOCAL BOOL	wdbEndPollArpReply (struct mbuf * pMblk, void * pCookie);

#ifdef WDB_NPT_CAPABLE
int wdbNptInt ( void *, long type, M_BLK_ID pMblk, void *);
void wdbNptShutdown ( void * callbackId );
#endif WDB_NPT_CAPABLE

/******************************************************************************
*
* wdbEndPktDevInit - initialize an END packet device
*
* This routine initializes an END packet device. It is typically called
* from configlette wdbEnd.c when the WDB agent's lightweight END
* communication path (INCLUDE_WDB_COMM_END) is selected.
*
* RETURNS: OK or ERROR
*/

STATUS wdbEndPktDevInit
    (
    WDB_END_PKT_DEV *	pPktDev,	/* device structure to init */
    void		(*stackRcv) (),	/* receive packet callback (udpRcv) */
    char *		pDevice,	/* Device (ln, ie, etc.) that we */
    					/* wish to bind to. */
    int         	unit            /* unit number (0, 1, etc.) */
    )
    {
    END_OBJ *	pEnd;
    char	ifname [20];
    char	inetAdrs [24];

    /* initialize the wdbDrvIf field with driver info */

    pPktDev->wdbDrvIf.mode	= WDB_COMM_MODE_POLL| WDB_COMM_MODE_INT;
    pPktDev->wdbDrvIf.mtu	= WDB_END_PKT_MTU;
    pPktDev->wdbDrvIf.stackRcv	= stackRcv;		/* udpRcv */
    pPktDev->wdbDrvIf.devId	= (WDB_END_PKT_DEV *)pPktDev;
    pPktDev->wdbDrvIf.pollRtn	= wdbEndPoll;
    pPktDev->wdbDrvIf.pktTxRtn	= wdbEndTx;
    pPktDev->wdbDrvIf.modeSetRtn = wdbEndModeSet;

    /* initialize the device specific fields in the driver structure */

    pPktDev->inputBusy		= FALSE;
    pPktDev->outputBusy		= FALSE;

#ifndef	STANDALONE_AGENT
    /*
     * Here is where we bind to the lower layer.
     * We do not, as yet, provide for a shutdown routine, but perhaps
     * later.
     * We are a promiscous protocol.
     * The Int routine a fakeout.  Interrupts are handled by the lower
     * layer but we use a similar mechanism to check packets for 
     * the proper type and hand them off to the WDB agent from
     * the "interrupt" routine if it's appropriate to do so.
     */

#ifdef WDB_NPT_CAPABLE
    if (muxTkDrvCheck (pDevice) == TRUE)
	{
	if ((pPktDev->pCookie = muxTkBind (pDevice, unit, wdbNptInt,
					   (FUNCPTR)wdbNptShutdown,
                                           NULL, NULL, MUX_PROTO_SNARF,
					   "Wind Debug Agent",
					   NULL, NULL, NULL)) == NULL)
	    {
	    if (wdbEndDebug)
		logMsg ("Could not bind to NPT Device %s, loading...\n",
			(int)pDevice, 2, 3, 4, 5, 6);
	    return (ERROR);
	    }
	}
    else  /* END */
	{
	if (wdbEndDebug)
	    logMsg ("Not a NPT device! %s\n", (int)pDevice,2,3,4,5,6);
#endif /* WDB_NPT_CAPABLE */
	if ((pPktDev->pCookie = muxBind (pDevice, unit, wdbEndInt, NULL,
					 NULL, NULL, MUX_PROTO_SNARF,
			 		 "Wind Debug Agent", NULL)) == NULL)
	    {
	    if (wdbEndDebug)
		logMsg ("Could not bind to %s, loading...\n",
			(int)pDevice, 2, 3, 4, 5, 6);
	    return (ERROR);
	    }
#ifdef WDB_NPT_CAPABLE
	}
#endif /* WDB_NPT_CAPABLE */

    pEnd = PCOOKIE_TO_ENDOBJ(pPktDev->pCookie);


#else	/* STANDALONE_AGENT */
    /*
     * for standalone agent, we simply need to get the address of the 
     * of the device.
     */

     if ((pPktDev->pCookie = endFindByName (pDevice, unit)) == NULL)
	return (ERROR); 
#endif	/* STANDALONE_AGENT */

    /* build interface name */

    sprintf (ifname, "%s%d", pDevice, unit);

    /* get interface inet address */

    if (ifAddrGet (ifname, inetAdrs) != OK)
    	{
	if (wdbEndDebug)
	    logMsg ("Could not get inet address of %s interface...\n",
	    	(int) ifname, 2, 3, 4, 5, 6);
	return (ERROR);
	}

    pPktDev->ipAddr.s_addr = inet_addr (inetAdrs);

    pEnd = PCOOKIE_TO_ENDOBJ(pPktDev->pCookie);

    if ((pInPkt = memalign (4,pEnd->mib2Tbl.ifMtu)) == NULL)
        return (ERROR);
   
    if ((pPktDev->pInBlk = wdbEndMblkClGet (pEnd, pEnd->mib2Tbl.ifMtu))
        == NULL)
	return (ERROR);
    pPktDev->pInBlk->mBlkHdr.mFlags |= M_PKTHDR;

    if ((pPktDev->pOutBlk = wdbEndMblkClGet (pEnd, pEnd->mib2Tbl.ifMtu))
        == NULL)
	return (ERROR);
    pPktDev->pOutBlk->mBlkHdr.mFlags |= M_PKTHDR;

    if ((pPktDev->lastHAddr = wdbEndMblkClGet (pEnd, pEnd->mib2Tbl.ifMtu))
        == NULL)
	return (ERROR);

    if ((pPktDev->srcAddr = wdbEndMblkClGet (pEnd, pEnd->mib2Tbl.ifMtu))
        == NULL)
	return (ERROR);

    /* Set the length to the size of the buffer just allocated. */
    pPktDev->pInBlk->mBlkHdr.mLen = pPktDev->pInBlk->pClBlk->clSize;

    pPktDev->lastHAddr->mBlkHdr.mLen = pEnd->mib2Tbl.ifPhysAddress.addrLength;

    memset (pPktDev->lastHAddr->mBlkHdr.mData, 0xff,
            pPktDev->lastHAddr->mBlkHdr.mLen);

    /*
     * Create a source address structure so we can send fully
     * qualified packets.
     */
     
    muxIoctl (pPktDev->pCookie, EIOCGADDR, pPktDev->srcAddr->mBlkHdr.mData);

    
    pEndPktDev = pPktDev;

    return (OK);
    }

/******************************************************************************
*
* wdbEndInt - END driver interrupt handler
*
* This is really a MUX receive routine but we let it look like
* an interrupt handler to make this interface look as much like the
* template as possible.
*
* RETURNS: TRUE if the packet was for us, FALSE otherwise.
*
* NOMANUAL
*/

int wdbEndInt
    (
    void * 		pCookie,
    long 		type, 
    M_BLK_ID 		pMblk,
    LL_HDR_INFO * 	pLinkHdrInfo, 
    void * 		pMode
    )
    {
    struct mbuf * 	pMbuf;
    int			size;
    struct udphdr * 	pUdpHdr;
    struct ip *		pIpHdr;
    WDB_END_PKT_DEV * 	pPktDev = pEndPktDev;

    /* input buffer already in use - drop this packet */

    if (wdbEndDebug)
	logMsg ("Got a packet!\n", 1, 2, 3, 4, 5, 6);

    if (pPktDev->inputBusy)
	{
        if (wdbEndDebug)
	    logMsg ("Input busy!\n", 1, 2, 3, 4, 5, 6);

	/*
	 * free the packet only if the driver is in polling mode. If not, the
	 * packet will be freed by the MUX layer.
	 */

	if ((pMode != NULL) && (*((int *)pMode) == WDB_COMM_MODE_POLL))
	     netMblkClFree (pMblk);
	return (FALSE);
	}

    pPktDev->inputBusy = TRUE;

    /* Check the type before doing anything expensive. */

    if ((type == 0x806) && (pMode != NULL) &&
    				(*((int *)pMode) == WDB_COMM_MODE_POLL))
    	{
	/*
	 * In polling mode we need to answer ARP request so that
	 * communication becomes or remains possible.
	 */

        if (wdbEndDebug)
            logMsg ("Type == 0x806\n", 1, 2, 3, 4, 5, 6);
	return (wdbEndPollArpReply (pMblk, pCookie));
	}
	
    if (type != 0x800)
        {
        if (wdbEndDebug)
            logMsg ("Type != 0x800 && Type!= 0x806\n", 1, 2, 3, 4, 5, 6);
        goto wdbEndIntError;
        }

    size = pLinkHdrInfo->dataOffset + IP_HDR_SIZE + sizeof(struct udphdr);

    if (!(pMblk->mBlkHdr.mFlags & M_PKTHDR) || (pMblk->mBlkPktHdr.len <
                                                size))
        {
        goto wdbEndIntError; 
        }

    if (pMblk->mBlkHdr.mLen < size)
        {
        if (netMblkOffsetToBufCopy (pMblk, pLinkHdrInfo->dataOffset, pInPkt,
                                    size, NULL) == 0)
            goto wdbEndIntError;

	pIpHdr = (struct ip *) pInPkt;
        pUdpHdr = (struct udphdr *)(pInPkt + IP_HDR_SIZE);
        }
    else
        {
	pIpHdr = (struct ip *) (pMblk->mBlkHdr.mData +
				pLinkHdrInfo->dataOffset);
        pUdpHdr = (struct udphdr *)(pMblk->mBlkHdr.mData +
                                    pLinkHdrInfo->dataOffset +
                                    IP_HDR_SIZE);
        }

    /* If this packet is not for the agent, ignore it */

    if ((pIpHdr->ip_p != IPPROTO_UDP) ||
	(pUdpHdr->uh_dport != htons(WDBPORT)))
        goto wdbEndIntError;

    if (pPktDev->ipAddr.s_addr != pIpHdr->ip_dst.s_addr)
    	goto wdbEndIntError;

    /*
     * Check to see whether the packet is fragmented.  WDB does not
     * handle fragmented packets.
     */

    if (pIpHdr->ip_off & htons(IP_MF|IP_OFFMASK))
    	{
	if (wdbEndDebug)
	    logMsg ("Fragmented packet\n", 0, 0, 0, 0, 0, 0);
	goto wdbEndIntError;
	}

    if ((size = netMblkOffsetToBufCopy (pMblk, pLinkHdrInfo->dataOffset,
                                        pInPkt, M_COPYALL, NULL)) == 0)
        goto wdbEndIntError;
    
    bcopy ((char *)pMblk->mBlkHdr.mData + pLinkHdrInfo->srcAddrOffset,
          (char *)pPktDev->lastHAddr->mBlkHdr.mData, 
          pLinkHdrInfo->srcSize);

    /* We're always going to send an IP packet. */

    pPktDev->lastHAddr->mBlkHdr.reserved = htons (0x800);
    
    /*
     * Fill the input buffer with the packet. Use an mbuf cluster 
     * to pass the packet on to the agent.
     */
    
    pMbuf = wdbMbufAlloc ();
    if (pMbuf == NULL)
        goto wdbEndIntError;

    wdbMbufClusterInit (pMbuf, pInPkt, size, (int (*)())wdbEndInputFree,
                        (int)pPktDev);
    if (wdbEndDebug)
        logMsg ("Passing up a packet!\n", 1, 2, 3, 4, 5, 6);
    (*pPktDev->wdbDrvIf.stackRcv) (pMbuf);  /* invoke callback */

    netMblkClFree (pMblk);
    return (TRUE); 

    wdbEndIntError:
   	 {
         /*
          * drop all non wdb packets received through the poll routine
          * When call from muxReceive pMode is actually the spare pointer
          * which is initialized in the NET_PROTOCOL structure, the spare
          * pointer is passed as the last argument to the stackRcvRtn.
          * When called from wdbEndPoll it passes a mode in the void pointer
          * This mode is used to find out whether this routine is called from
          * wdbEndPoll or muxReceive. You will not be happy camper if the
          * spare pointer is removed from NET_PROTOCOL or if the stackRcvRtn's
          * API is changed.
          */

         if ((pMode != NULL) && (*((int *)pMode) == WDB_COMM_MODE_POLL))
	     netMblkClFree (pMblk);
         
         DRIVER_RESET_INPUT(pPktDev);
         return (FALSE); 
         }
    }

/******************************************************************************
*
* wdbEndTx - transmit a packet.
*
* The packet is realy a chain of mbufs. We may have to just queue up
* this packet is we are already transmitting.
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

STATUS wdbEndTx
    (
    void *	pDev,
    struct mbuf * pMbuf
    )
    {
    WDB_END_PKT_DEV *pPktDev = pDev;
    struct mbuf* pFirstMbuf = pMbuf;

#ifdef WDB_NPT_CAPABLE
    int sendStatus = 0;
    char * dstMacAddr = NULL;
#else
    int len = 0;
#endif WDB_NPT_CAPABLE

    if (wdbEndDebug)
	logMsg ("entering send\n", 1, 2, 3, 4, 5, 6);

    if (pMbuf == NULL)
	return (ERROR);

    if (pPktDev->outputBusy)
        {
        wdbMbufChainFree (pFirstMbuf);
        return (EAGAIN);
        }
    else
        pPktDev->outputBusy = TRUE;

    if (wdbEndDebug)
	logMsg ("About to enter the copy loop!\n", 1, 2, 3, 4, 5, 6);

    /* Make sure we know that the outgoing buffer is clean/empty */
    pPktDev->pOutBlk->mBlkHdr.mLen  = 0;
    pPktDev->pOutBlk->mBlkHdr.mData = pPktDev->pOutBlk->pClBlk->clNode.pClBuf;

    /* increment the data pointer so that muxAddressForm can prepend header */
    pPktDev->pOutBlk->mBlkHdr.mData +=	SIZEOF_ETHERHEADER;

#ifndef WDB_NPT_CAPABLE
    /* First place the addressing information into the packet. */
    muxAddressForm (pPktDev->pCookie, pPktDev->pOutBlk,
                    pPktDev->srcAddr, pPktDev->lastHAddr);

    len = pPktDev->pOutBlk->mBlkHdr.mLen;
    
    if (wdbEndDebug)
	logMsg ("About to check length!\n", 1, 2, 3, 4, 5, 6);

    if (len == 0)
        {
        wdbMbufChainFree (pFirstMbuf);
        pPktDev->outputBusy = FALSE;
        if (wdbEndDebug)
            logMsg ("Wrong length!\n", 1, 2, 3, 4, 5, 6);
        return (EAGAIN);
        }
    
    if (wdbEndDebug)
        logMsg ("hdr length:%d\n", len, 2, 3, 4, 5, 6);
    pPktDev->pOutBlk->mBlkHdr.mLen = len + 
        netMblkToBufCopy (pMbuf, pPktDev->pOutBlk->mBlkHdr.mData + len, NULL);
    pPktDev->pOutBlk->mBlkPktHdr.len = pPktDev->pOutBlk->mBlkHdr.mLen;


    if (wdbEndDebug)
        logMsg ("OutBlk: %x:%x:%x:%x:%x:%x\n", 
                pPktDev->pOutBlk->mBlkHdr.mData[0],
                pPktDev->pOutBlk->mBlkHdr.mData[1],
                pPktDev->pOutBlk->mBlkHdr.mData[2],
                pPktDev->pOutBlk->mBlkHdr.mData[3],
                pPktDev->pOutBlk->mBlkHdr.mData[4],
                pPktDev->pOutBlk->mBlkHdr.mData[5]);
#else
    pPktDev->pOutBlk->mBlkHdr.mLen =
        netMblkToBufCopy (pMbuf, pPktDev->pOutBlk->mBlkHdr.mData, NULL);
    pPktDev->pOutBlk->mBlkPktHdr.len = pPktDev->pOutBlk->mBlkHdr.mLen;
#endif /* WDB_NPT_CAPABLE */

    if (wdbEndDebug)
        logMsg ("Data copied !\n", 1, 2, 3, 4, 5, 6);

    wdbMbufChainFree (pFirstMbuf); 
    
    if (wdbEndDebug)
	logMsg ("About to send a packet!\n", 1, 2, 3, 4, 5, 6);

#ifdef WDB_NPT_CAPABLE
    dstMacAddr = pPktDev->lastHAddr->mBlkHdr.mData;
#endif /* WDB_NPT_CAPABLE */

#ifndef	STANDALONE_AGENT
    /* if we are in polled mode, transmit the packet in a loop */

    if (pPktDev->mode == WDB_COMM_MODE_POLL)
	{
#ifndef WDB_NPT_CAPABLE
        muxPollSend (pPktDev->pCookie, pPktDev->pOutBlk);
#else
        sendStatus = muxTkPollSend (pPktDev->pCookie, pPktDev->pOutBlk,
			            dstMacAddr, htons(ETHERTYPE_IP), NULL);

        if (sendStatus != OK)
	    {
	    pPktDev->outputBusy = FALSE;
	    if (wdbEndDebug)
		logMsg ("Wrong length!\n", 1, 2, 3, 4, 5, 6);
	    return (EAGAIN);
	    }
#endif /* WDB_NPT_CAPABLE */
        if (wdbEndDebug)
            logMsg ("Sent polled packet!\n", 1, 2, 3, 4, 5, 6);
	}
    else
        {
#ifndef WDB_NPT_CAPABLE
        if (muxSend (pPktDev->pCookie, pPktDev->pOutBlk) == OK)
#else
        if (muxTkSend (pPktDev->pCookie, pPktDev->pOutBlk, dstMacAddr,
		       htons(ETHERTYPE_IP), NULL) == OK)
#endif /* WDB_NPT_CAPABLE */
            {
	    END_OBJ *	pEnd;

            if (wdbEndDebug)
                logMsg ("Sent regular packet!\n", 1, 2, 3, 4, 5, 6);

            /*pEnd = (END_OBJ *)pPktDev->pCookie;*/
	    pEnd = PCOOKIE_TO_ENDOBJ(pPktDev->pCookie);
            
            if ((pPktDev->pOutBlk =
                 wdbEndMblkClGet (pEnd, pEnd->mib2Tbl.ifMtu)) == NULL)
                return (ERROR);
            pPktDev->pOutBlk->mBlkHdr.mFlags |= M_PKTHDR;
            }
        else
            {
            if (wdbEndDebug)
                logMsg ("Could not send regular packet!\n", 1, 2, 3, 4, 5, 6);
            }
        }
#else	/* STANDALONE_AGENT */
    if (muxPollSend (pPktDev->pCookie, pPktDev->pOutBlk) == ERROR)
    	{
    	if (wdbEndDebug);
	    logMsg ("Could not send regular packet!\n", 1, 2, 3, 4, 5, 6);
	}
    else
    	{
        if (wdbEndDebug)
            logMsg ("Sent polled packet!\n", 1, 2, 3, 4, 5, 6);
	}
#endif	/* STANDALONE_AGENT */
    
    wdbEndOutputFree (pPktDev);
    
    return (OK);
    }

/******************************************************************************
*
* wdbEndOutputFree - free the output buffer
*
* This is the callback used to let us know the that the device is done with the
* output buffer we loaned it.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void wdbEndOutputFree
    (
    void *	pDev
    )
    {
    WDB_END_PKT_DEV *	pPktDev = pDev;

    DRIVER_RESET_OUTPUT(pPktDev);
    }

/******************************************************************************
*
* wdbEndInputFree- free the input buffer
*
* This is the callback used to let us know the agent is done with the
* input buffer we loaded it.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void wdbEndInputFree
    (
    void *	pDev
    )
    {
    WDB_END_PKT_DEV *	pPktDev = pDev;

    DRIVER_RESET_INPUT(pPktDev);
    }

/******************************************************************************
*
* wdbEndModeSet - switch driver modes
*
* RETURNS: OK for a supported mode, else ERROR
*
* NOMANUAL
*/

LOCAL STATUS wdbEndModeSet
    (
    void *	pDev,
    uint_t	newMode
    )
    {
    WDB_END_PKT_DEV * pPktDev = pDev;

    if (newMode == WDB_COMM_MODE_INT)
	{
	muxIoctl (pPktDev->pCookie, EIOCPOLLSTOP, NULL);
	pPktDev->mode = newMode;
	return (OK);
	}
    else if (newMode == WDB_COMM_MODE_POLL)
	{
	muxIoctl (pPktDev->pCookie, EIOCPOLLSTART, NULL);
	pPktDev->mode = newMode;
	return (OK);
	}
    else
	return (ERROR);

    return (ERROR);
    }

/******************************************************************************
*
* wdbEndPollArpReply - END driver ARP reply handler
*
* WDB agent normally only manages IP packets that are sent to a dedicated
* port. However it is the only protocol bound to the MUX, we must also reply
* to ARP request so that communication becomes possible.
*
* RETURNS: TRUE if the packet was for us, FALSE otherwise.
*
* NOMANUAL
*/

LOCAL BOOL wdbEndPollArpReply
    (
    struct mbuf *	pMblk,
    void *		pCookie
    )
    {
    struct ether_header * pEh;
    struct ether_arp *	  pEa;
    WDB_END_PKT_DEV * 	  pPktDev = pEndPktDev;
    int			  op;

    pEh = mtod(pMblk, struct ether_header *);
    pEa = (struct ether_arp *) ((char *)pEh + sizeof (struct ether_header));
    op = ntohs(pEa->arp_op);

    /* test if the request is for us */

    if ((bcmp ((caddr_t)&pPktDev->ipAddr.s_addr,
    		(caddr_t)&pEa->arp_tpa, pEa->arp_pln) == 0) &&
		(op == ARPOP_REQUEST))
	{
        /* build the reply */

	/* fill the ethernet header */

	bcopy (pEh->ether_shost, pEh->ether_dhost, sizeof(pEh->ether_shost));
	bcopy (pPktDev->srcAddr->mBlkHdr.mData, pEh->ether_shost,
               sizeof(pEh->ether_shost));
	
	/* fill the ARP Frame */

        bcopy (pEh->ether_dhost, pEa->arp_tha, pEa->arp_hln);
	bcopy (pEa->arp_spa, pEa->arp_tpa, pEa->arp_pln);
	bcopy (pEh->ether_shost, pEa->arp_sha, pEa->arp_hln);
	bcopy ((char *)&pPktDev->ipAddr.s_addr, pEa->arp_spa, pEa->arp_pln);
	pEa->arp_op = htons (ARPOP_REPLY);

        /* send the reply */

	muxPollSend (pCookie, pMblk);

	netMblkClFree (pMblk);
	DRIVER_RESET_INPUT(pPktDev);
	return (TRUE);
	}

    netMblkClFree (pMblk);
    DRIVER_RESET_INPUT(pPktDev);
    return (FALSE);
    }

/******************************************************************************
*
* wdbEndPoll - poll for a packet
*
* This routine polls for a packet. If a packet has arrived it invokes
* the agents callback.
*
* RETURNS: OK if a packet has arrived, else ERROR.
*
* NOMANUAL
*/ 

LOCAL STATUS wdbEndPoll
    (
    void *	pDev
    )
    {
    long 		type;
    M_BLK_ID 		pMblk;
    WDB_END_PKT_DEV * 	pPktDev = pDev;
    LL_HDR_INFO  	llHdrInfo;
    int			mode = WDB_COMM_MODE_POLL; 
    END_OBJ * 		pEnd = PCOOKIE_TO_ENDOBJ(pPktDev->pCookie);

    /* Reset the size to the maximum. */

    pPktDev->pInBlk->mBlkHdr.mLen = pPktDev->pInBlk->pClBlk->clSize;
    pPktDev->pInBlk->mBlkHdr.mData = pPktDev->pInBlk->pClBlk->clNode.pClBuf;

#ifdef WDB_NPT_CAPABLE
    if (muxTkPollReceive (pPktDev->pCookie, pPktDev->pInBlk, NULL) == OK)
#else
    if (muxPollReceive (pPktDev->pCookie, pPktDev->pInBlk) == OK)
#endif /* WDB_NPT_CAPABLE */
	{
        if ((pMblk = mBlkGet (pEnd->pNetPool, M_DONTWAIT, MT_DATA)) == NULL)
            return (ERROR);

        /* duplicate the received mBlk so that wdbEndInt can free it */

        pMblk = netMblkDup (pPktDev->pInBlk, pMblk);

#ifndef WDB_NPT_CAPABLE
        if (muxPacketDataGet (pPktDev->pCookie, pMblk, &llHdrInfo) == ERROR)
	    {
	    return (ERROR);
	    }
#else
	if ((pEnd->mib2Tbl.ifType == M2_ifType_ethernet_csmacd) ||
	    (pEnd->mib2Tbl.ifType == M2_ifType_iso88023_csmacd))
	    {
	    if (endEtherPacketDataGet (pMblk, &llHdrInfo) == ERROR)
		{
		return (ERROR);
		}
	    }
	else
	    return ERROR;
#endif /* WDB_NPT_CAPABLE */

        type = llHdrInfo.pktType;

        if (wdbEndInt (pPktDev->pCookie, type, pMblk, &llHdrInfo,
                       (void *)&mode) == TRUE)
            return (OK);
        return (ERROR);
        }
    else
        {
        return (ERROR);
        }
    }

/*******************************************************************************
* wdbEndMblkClGet - get an mBlk/clBlk/cluster 
*
* This routine returns an mBlk which points to a clBlk which points to a
* cluster. This routine allocates the triplet from the pNetPool in the
* end object.
*
* RETURNS: M_BLK_ID / NULL
*
* NOMANUAL
*/ 

LOCAL M_BLK_ID wdbEndMblkClGet
    (
    END_OBJ * 	pEnd,		/* endobject in which the netpool belongs */
    int		size		/* size of the cluster */
    )
    {
    M_BLK_ID 	pMblk;

    if ((pMblk = netTupleGet (pEnd->pNetPool, size, M_DONTWAIT, MT_DATA,
                              FALSE)) == NULL)
        {
	if (wdbEndDebug)
	    logMsg ("wdbEndMblkClGet:Could not get mBlk...\n",
                   1, 2, 3, 4, 5, 6);
        }

    return (pMblk);
    }

#ifdef WDB_NPT_CAPABLE
/******************************************************************************
*
* wdbNptInt - NPT driver interrupt handler
*
* This is really a MUX receive routine but we let it look like
* an interrupt handler to make this interface look as much like the
* template as possible.
*
* RETURNS: TRUE if the packet was for us, FALSE otherwise.
*
* NOMANUAL
*/

int wdbNptInt
    (
    void * 	callbackId, 	/* call back Id supplied during muxTkBind */
    long 	type, 		/* our network service type */
    M_BLK_ID 	pMblk,		/* the packet */
    void * 	pSpareData	/* any spare data; we ignore it */
    )
    {
    END_OBJ * pEnd = PCOOKIE_TO_ENDOBJ(pEndPktDev->pCookie);
    LL_HDR_INFO llHdrInfo;

    switch (pEnd->mib2Tbl.ifType)
        {
        case M2_ifType_ethernet_csmacd:
        case M2_ifType_iso88023_csmacd:
	    if (wdbEndDebug)
		logMsg ("wdbNptInt: Received an Ethernet packet\n",1,2,3,4,5,6);
	    endEtherPacketDataGet (pMblk,&llHdrInfo);
	    return (wdbEndInt (pEndPktDev->pCookie, type, pMblk,
                               &llHdrInfo,callbackId));
	    break;

	default:
	    if (wdbEndDebug)
		logMsg ("wdbNptInt: NOT an Ethernet packet\n",1,2,3,4,5,6);

	    return (FALSE);
	}
    }

/********************************************************************************
* wdbNptShutdown - dummy shutdown routine for muxTkBind 
*
* RETURNS: N/A
*
* NOMANUAL
*/

void wdbNptShutdown
    (
    void * 	callbackId 	/* call back Id supplied during muxTkBind */
    )
    {
    }
#endif /* WDB_NPT_CAPABLE */

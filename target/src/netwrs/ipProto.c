/* ipProto.c - an interface between the BSD IP protocol and the MUX */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
03k,15may02,tcr  Make WV_NETEVENT_ETHEROUT_NOBUFS match uses elsewhere
03j,24apr02,rae  Fixed muxTxRestart race condition (SPR #74565)
03i,19apr02,wap  call ip_mloopback() rather than calling looutput() directly
                 (SPR #72246)
03h,26mar02,vvv  fixed proxy broadcast storm (SPR #74518)
03g,02jan02,vvv  changed ipAttach failure message when device not found
03f,01nov01,rae  ipError frees buffers on END_ERR_NO_BUF (SPR #70545)
03e,15oct01,rae  merge from truestack ver 03r, base 02r(1) (SPRs 69573, 69112,
                 29668, ROUTER_STACK etc.)
03d,24jul01,r_s  changed code to be ANSI compatible so that it compiles with
                 diab. made asm macro changes for diab
03f,12jun01,rae  ipDetach checks for npt
03e,01jun01,rae  fix problem with last checkin
03d,08may01,rae  limited ARP binding to appropriate device types (SPR #33657)
03c,07feb01,spm  fixed modification history for branches and merges
03b,22nov00,rae  fixed problem with zeros in ethernet address (SPR# 29152)
03a,14nov00,rae  fix SIOC[ADD,DEL]MULTI for non-ethernet devices (SPR# 35459)
02z,13nov00,niq  merged from version 03o of tor2_0.open_stack-f1 brranh:
                 added RFC 2233 support
02y,10nov00,ham  enhanced sanity check for memory allocation (SPR #62262)
02x,07nov00,spm  moved nptFlag from END_OBJ for T2 END binary compatibility
02w,23oct00,spm  removed duplicate free (corrects SPR #22324 fix) (SPR #30465)
02v,17oct00,spm  removed modification history from tor2_0_0.toolkit branch;
                 merged from ver. 02z of tor3_0_x branch (base 02t): corrected
                 attach routine (SPR #31351), added backward compatibility for
                 END devices, and included general code cleanup
02u,17oct00,spm  updated for new if_attach: reports memory allocation failures
02t,17mar99,spm  added support for identical unit numbers (SPR #20913)
02s,16mar99,spm  recovered orphaned code from version 02q of tor2_0_x branch
                 (base version 02n) (SPR #25570)
02r,03mar99,spm  eliminated buffer overflow when attaching device (SPR #22679);
                 cleaned up mod history list
02q,03mar99,pul  update the driver flag during ipAttach(), SPR# 24287
02p,02mar99,pul  update ifp->if_baudrate during ipAttach(): SPR# 24256
02o,25feb99,pul  fixed multicast mapping to invoke resolution func spr#24255
02n,08aug98,n_s  fixed ipReceiveRtn to handle M_BLK chains (SPR #22324)
02m,26aug98,fle  doc : put library description next line
02l,24aug98,ann  fixed copy of ifSpeed field in ipIoctl() spr # 22198
02k,21aug98,n_s  fixed if_omcast increment in ipOutout (). spr # 21074
02j,16jul98,n_s  fixed ipIoctl () SIOCSIFFLAGS. SPR 21124
02i,15jun98,n_s  seperated ipDetach () into ipShutdownRtn () and 
                 arpShutdownRtn ().  spr # 21545
02h,11dec97,gnn  removed IFF_SCAT and IFF_LOAN references.
02g,08dec97,gnn  END code review fixes.
02f,17oct97,vin  changes reflecting protocol recieveRtn changes.
02e,09oct97,vin  added break statements to ipIoctl(). fixed ipReceive().
02d,03oct97,gnn  added an error routine
02c,25sep97,gnn  SENS beta feedback fixes
02b,03sep97,gnn  implemented dropping of packets if the queue is full
02a,29aug97,jag  fixed bad parameter causing failure in txRestart routine.
01z,25aug97,gnn  documentation fixes for mangen.
01y,25aug97,gnn  added a check for attached in the TxRestart routine.
01x,19aug97,gnn  changes due to new buffering scheme.
01w,12aug97,gnn  name change and changes necessitated by MUX/END update.
01v,06jun97,vin  made enhancements, fixed arpReceive for multiple units.
01u,02jun97,gnn  put in stuff to deal with memory widths.
01t,20may97,gnn  fixed SPR 8627 so that multiple units are possible.
01s,15may97,gnn  modified muxUnbind to follow new prototype.
01r,30apr97,gnn  put a logMsg behind a debug check.
                 cleaned up warnings.
01q,25apr97,gnn  fixed SPR 8461 about SNMP ioctls.
01p,09apr97,gnn  pass through for IFFLAGS.
01o,20mar97,map  Fixed problems in buffer loaning.
                 Added gratuitous arp on SIOCSIFADDR.
01n,03feb97,gnn  Changed the way in which muxBufAlloc is used.
01m,30jan97,gnn  Fixed a bug in the call to build_cluster.
01l,24jan97,gnn  Modified vector code to handle holes in mbuf chains.
01k,24jan97,gnn  Fixed the bug where we were not checking the length of
                 data in an mbuf as well as the next pointer.
                 Fixed a bug in initialising for buffer loaning.
01j,22jan97,gnn  Added new private flags that is seperate from if flags.
01i,21jan97,gnn  Added code to handle scatter/gather.
                 Changed the way send is done to deal with freeing buffers.
                 Made receive routines return a value for SNARFing.
01h,20dec96,vin  fixed problems in send (added semGive, m_freem) 
                 and recv routines (cleaned up). cleanup ifdefs.
01j,17dec96,gnn  added new drvCtrl structures.
01i,27nov96,gnn  added MIB 2 if_type handling.
01h,07nov96,gnn  fixed the bug where we were getting the flags wrong.
01g,23oct96,gnn  removed bcopy hack in initialization.
01f,22oct96,gnn  name changes to follow coding standards.
                 cleanup of some compilation warnings.
01e,22oct96,gnn  removed ENET_HDR definition.
01d,22oct96,gnn  Changed all netVectors to netBuffers.
                 Removed all private buffer loaning stuff.  That's all now
                 in the MUX.
01c,23sep96,gnn  Added new buffering scheme information.
01b,13sep96,vin  fixed a bug in ipEtherIoctl for SIOCADDMULTI & SIOCDELMULTI 
01a,16may96,gnn	 written.
*/
 
/*
DESCRIPTION
This library provides an interface between the Berkeley protocol stack
and the MUX interface for both NPT and END devices. The ipAttach() routine
binds the IP protocol to a specific device. It is called automatically
during network initialization if INCLUDE_END is defined. The ipDetach()
routine removes an existing binding to an END device.

NOTE: The library can only transmit data to link-level destination addresses
      less than or equal to 64 bytes in length.

INCLUDE FILES: end.h muxLib.h etherMultiLib.h sys/ioctl.h
*/

/* includes */

#include "vxWorks.h"
#include "stdio.h"
#include "logLib.h"
#include "semLib.h"
#include "errnoLib.h"
#include "tickLib.h"
#include "intLib.h"
#include "m2Lib.h"
#include "private/m2LibP.h"
#include "end.h"
#include "muxLib.h"
#include "private/muxLibP.h"
#include "muxTkLib.h"
#include "etherMultiLib.h"

#include "sys/ioctl.h"

#include "net/protosw.h"
#include "sys/socket.h"
#include "errno.h"

#include "netinet/in_systm.h"
#include "netinet/in_var.h"
#include "netinet/ip.h"
#include "netinet/if_ether.h"
#include "net/if_subr.h"
#include "net/if_dl.h"
#include "net/if_llc.h"
#include "net/mbuf.h"
#include "ipProto.h"
#include "netinet/ip_var.h"
#include "stdlib.h"
#include "memPartLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif /* VIRTUAL_STACK */

#ifdef ROUTER_STACK
#include "wrn/fastPath/fastPathLib.h"
#endif /* ROUTER_STACK */

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif /* INCLUDE_WVNET */
#endif /* WV_INSTRUMENTATION */

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_IPPROTO_MODULE;   /* Value for ipProto.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif	  /* WV_INSTRUMENTATION */

/* defines */

#define MAX_ADDRLEN 64 	/* Largest link-level destination for NPT devices. */

#define senderr(e) { error = (e); goto bad;}

/* typedefs */

typedef struct arpcom IDR;
typedef struct mbuf MBUF;
typedef struct ifnet IFNET;                 /* real Interface Data Record */
typedef struct sockaddr SOCK;

/* externs */

IMPORT void if_dettach(struct ifnet *ifp);

IMPORT void ip_mloopback(struct ifnet *, struct mbuf *, struct sockaddr_in *,
                    struct rtentry *rt);

/* globals */

#ifdef ROUTER_STACK

/*
 * Pointer to the Fastpath library function table. This is set to
 * point to the function table inside of ffLibInit() when it is called
 */

FF_LIB_FUNCTIONS *	pFFLibFuncs = NULL;
#endif /* ROUTER_STACK */


/* locals */

/* forward declarations */
LOCAL BOOL ipReceiveRtn (void * pCookie, long type, M_BLK_ID pBuff,
                         LL_HDR_INFO * pLinkHdrInfo, void * pSpare);
LOCAL BOOL ipTkReceiveRtn (void * callbackId ,long type, M_BLK_ID pBuff,
                          void * pSpareData);
LOCAL STATUS ipShutdownRtn (void * pCookie, void * pSpare);
LOCAL STATUS ipTkShutdownRtn (void * callbackId);
LOCAL STATUS arpShutdownRtn (void * pCookie, void * pSpare);
LOCAL STATUS arpTkShutdownRtn (void * callbackId);
LOCAL void ipTxStartup (IP_DRV_CTRL* pDrvCtrl);
LOCAL int ipIoctl (IDR * ifp, int cmd, caddr_t data );
LOCAL int ipTxRestart (void * pCookie, IP_DRV_CTRL* pDrvCtrl);
LOCAL int ipTkTxRestart (void * pSpare);

LOCAL int ipOutput (register struct ifnet *ifp, struct mbuf *m0,
                        struct sockaddr *dst, struct rtentry *rt0);
LOCAL void ipError (END_OBJ* pEnd, END_ERR* pError, void * pSpare);
LOCAL int ipOutputResume (struct mbuf*, struct sockaddr*, void*, void*);
LOCAL int ipMcastResume (struct mbuf* pMbuf, struct sockaddr* ipDstAddr,
                             void* pIfp, void* rt);

#ifdef IP_DEBUG
int ipDebug 	=	FALSE;
#endif

/******************************************************************************
*
* ipReceiveRtn - Send a packet from the MUX to the Protocol
* 
* This routine deals with  calling the upper layer protocol.
*
* RETURNS: always TRUE.
*
* NOMANUAL
*/

BOOL ipReceiveRtn
    (
    void * 		pCookie,   /* protocol/device binding from muxBind() */
    long                type,           /* Protocol type.  */
    M_BLK_ID 		pMblk,	        /* The whole packet. */
    LL_HDR_INFO *	pLinkHdrInfo,	/* pointer to link level header info */
    void * 		pSpare 		/* ip Drv ctrl */
    )
    {
    END_OBJ * 		pEnd;
    struct ifnet * 	pIfp;
    M_BLK_ID            pMblkOrig;
    IP_DRV_CTRL * 	pDrvCtrl = (IP_DRV_CTRL *)pSpare;

#ifdef ROUTER_STACK
    struct ip* pIpHdr;
    ULONG ipAddr=0;
    struct sockaddr_in dstIpAddr;
#endif  /* ROUTER_STACK */

    if (pDrvCtrl == NULL)
        {
        logMsg ("ipProto: unknown device\n", 0, 0, 0, 0, 0, 0);
        goto ipReceiveError;
        }
    
    pIfp = &pDrvCtrl->idr.ac_if;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
    WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_NOTICE, 4, 13,
                    WV_NETEVENT_ETHERIN_START, WV_NET_RECV, pIfp)
#endif  /* INCLUDE_WVNET */
#endif

    if ((pIfp->if_flags & IFF_UP) == 0)
        {
        pIfp->if_ierrors++;
        goto ipReceiveError;
        }

    pIfp->if_ipackets++;          /* bump statistic */
    pIfp->if_lastchange = tickGet();
    pIfp->if_ibytes += pMblk->mBlkPktHdr.len;

    /* Make sure the entire Link Hdr is in the first M_BLK */

    pMblkOrig = pMblk;
    if (pMblk->mBlkHdr.mLen < pLinkHdrInfo->dataOffset
        && (pMblk = m_pullup (pMblk, pLinkHdrInfo->dataOffset)) == NULL)
        {
        pMblk = pMblkOrig;
        goto ipReceiveError;
        }

    switch (pIfp->if_type)
        {
        case M2_ifType_ethernetCsmacd:
        case M2_ifType_iso88023Csmacd:
        case M2_ifType_iso88024_tokenBus:
        case M2_ifType_iso88025_tokenRing:
        case M2_ifType_iso88026_man:
        case M2_ifType_fddi:                  		
            if (pMblk->mBlkHdr.mData[0] & 1)
                {
                if (bcmp((caddr_t)etherbroadcastaddr,
                         (caddr_t)pMblk->mBlkHdr.mData,
                         sizeof(etherbroadcastaddr)) == 0)
                    pMblk->mBlkHdr.mFlags |= M_BCAST;
                else
                    pMblk->mBlkHdr.mFlags |= M_MCAST;
                pIfp->if_imcasts++;
                }
            
            if (pIfp->if_flags & IFF_PROMISC)
                {

#ifdef ROUTER_STACK		/* UNNUMBERED_SUPPORT */
		/* No point to reject if we are on an unnumbered P2P network */
		if (pIfp->if_flags & IFF_UNNUMBERED)
		    break;
#endif /* ROUTER_STACK */

                /*
                 * do not hand over the non multicast packets to the ip stack
                 * if they are not destined to us, orelse it confuses the
                 * ip forwarding logic and keeps sending unnecessary redirects.
                 * If packets destined for other hosts have to be snooped they
                 * have to done through driver level hooks.
                 */
                
                if (!(pMblk->mBlkHdr.mFlags & (M_BCAST | M_MCAST)))
                    {
                    pEnd = PCOOKIE_TO_ENDOBJ(pCookie);

                    if (pEnd->flags & END_MIB_2233)
                        {
                        if (bcmp ((char *)END_ALT_HADDR (pEnd),
                                  (caddr_t)pMblk->mBlkHdr.mData,
                                  END_ALT_HADDR_LEN (pEnd)) != 0)
                            goto ipReceiveError;
                        }
                    else /* (RFC1213 style of counters supported) XXX */
                        {
                        if (bcmp ((char *)END_HADDR (pEnd),
                                  (caddr_t)pMblk->mBlkHdr.mData,
                                  END_HADDR_LEN (pEnd)) != 0)
                            goto ipReceiveError;
                        }
                    }
                }
            break;
        default:
            break;
        }

    /* point to the ip header, and adjust the length */
    pMblk->mBlkHdr.mData 	+= pLinkHdrInfo->dataOffset;
    pMblk->mBlkHdr.mLen  	-= pLinkHdrInfo->dataOffset;
    pMblk->mBlkPktHdr.len   	-= pLinkHdrInfo->dataOffset;
    pMblk->mBlkPktHdr.rcvif 	= &pDrvCtrl->idr.ac_if;

#ifdef VIRTUAL_STACK
     /* Set the virtual stack ID so the received packet
      * will be processed by the correct stack.
      */

      virtualStackNumTaskIdSet(pMblk->mBlkPktHdr.rcvif->vsNum);
#endif /* VIRTUAL_STACK */

#ifdef ROUTER_STACK
    /*
     * The algorithm explained...
     * Check if Fastpath is enabled and this is an Ip packet.
     * If so hand the packet over to the Fastpath code.
     * If Fastpath is not enabled or if the Fastpath packet send function
     * returns false, hand over the packet to tNetTask for normal processing.
     */

    if (SHOULD_GIVE_PACKET_TO_FF (GET_IPV4_FF_ID, pIfp) && 
        type == ETHERTYPE_IP)
        {
        pIpHdr=mtod(pMblk, struct ip*);
        ipAddr=pIpHdr->ip_dst.s_addr;
        bzero((char*)&dstIpAddr, sizeof(struct sockaddr_in));
        dstIpAddr.sin_len = sizeof(struct sockaddr_in);
        dstIpAddr.sin_family=AF_INET;
        dstIpAddr.sin_addr.s_addr=(ipAddr);

        if (FF_NC_CALL (GET_IPV4_FF_ID, ffPktSend, (GET_IPV4_FF_ID, (struct sockaddr*)&dstIpAddr, pMblk, ETHERTYPE_IP, pIfp)) == OK)
            {
#ifdef DEBUG
            logMsg("ipReceiveRtn: packet forwarded by Fastpath\n",0,0,0,0,0,0);
#endif /* DEBUG */
            return (TRUE);
            }
        }
#endif /* ROUTER_STACK */

    do_protocol_with_type (type, pMblk , &pDrvCtrl->idr, 
                           pMblk->mBlkPktHdr.len);
    return (TRUE);

ipReceiveError:
    if (pMblk != NULL)
        netMblkClChainFree (pMblk);

    return (TRUE);
    }

/******************************************************************************
*
* ipTkReceiveRtn - send a packet to the protocol from an NPT device
*
* This routine transfers data to IP when an NPT device receives a packet
* from a toolkit driver. The data contained in the <pMblk> argument is an
* IP packet (without a link-level header). A toolkit driver updates the
* flags in the mBlk to indicate a multicast or broadcast link-level frame.
*
* RETURNS: TRUE, always.
*
* NOMANUAL
*/

LOCAL BOOL ipTkReceiveRtn
    (
    void * ipCallbackId,      /* Sent down in muxTkBind call. */
    long         type,        /* Protocol type.  */
    M_BLK_ID     pMblk,       /* The whole packet. */
    void *       pSpareData   /* out of band data */
    )
    {
    IP_DRV_CTRL * pDrvCtrl = (IP_DRV_CTRL *)ipCallbackId;
    struct ifnet *  pIfp;

#ifdef ROUTER_STACK
    struct ip* pIpHdr;
    ULONG ipAddr=0;
    struct sockaddr_in dstIpAddr;
#endif  /* ROUTER_STACK */

    if (pDrvCtrl == NULL)
        {
        logMsg ("ipProto: unknown device\n", 0, 0, 0, 0, 0, 0);
        goto ipReceiveError;
        }

#ifdef IP_DEBUG
    if (ipDebug)
        logMsg("start of ipTkReceiveRtn!\n", 0, 0, 0, 0, 0, 0);
#endif /* IP_DEBUG */

    pIfp = (struct ifnet *)&pDrvCtrl->idr;

    if ((pIfp->if_flags & IFF_UP) == 0)
        {
        pIfp->if_ierrors++;
        goto ipReceiveError;
        }

    /* bump statistic */

    pIfp->if_ipackets++;        
    pIfp->if_lastchange = tickGet();
    pIfp->if_ibytes += pMblk->mBlkPktHdr.len;

    if(pMblk->mBlkHdr.mFlags & M_MCAST)
        pIfp->if_imcasts++;

    pMblk->mBlkPktHdr.rcvif     = pIfp;

    /* hand off the packet to ip layer */

#ifdef ROUTER_STACK
    /* Check to see if Fastpath is enabled on the received interface */

    if (SHOULD_GIVE_PACKET_TO_FF (GET_IPV4_FF_ID, pIfp) && 
        type == ETHERTYPE_IP)
        {
        pIpHdr=mtod(pMblk, struct ip*);
        ipAddr=pIpHdr->ip_dst.s_addr;
        bzero((char*)&dstIpAddr, sizeof(struct sockaddr_in));
        dstIpAddr.sin_len = sizeof(struct sockaddr_in);
        dstIpAddr.sin_family=AF_INET;
        dstIpAddr.sin_addr.s_addr=NTOHL(ipAddr);

        if (FF_NC_CALL (GET_IPV4_FF_ID, ffPktSend, \
                        (GET_IPV4_FF_ID, (struct sockaddr*)&dstIpAddr, \
                         pMblk, ETHERTYPE_IP, pIfp)) == OK)
            {
#ifdef DEBUG
            logMsg("ipTkReceiveRtn: packet forwarded by Fastpath\n",0,0,0,0,0,0);
#endif /* DEBUG */
            return (TRUE);
            }
        }
#endif /* ROUTER_STACK */

    do_protocol_with_type (type, pMblk , (struct arpcom* )pIfp, 
                           pMblk->mBlkHdr.mLen);

    return (TRUE);

    ipReceiveError:
         {
         if (pMblk != NULL)
             netMblkClChainFree (pMblk);
         }

    return (TRUE);
    }

/******************************************************************************
*
* ipShutdownRtn - Shutdown the NULL protocol stack gracefully.
* 
* This routine is called by the lower layer upon muxDevUnload or a similar
* condition to tell the protocol to shut itself down.
*
* RETURNS: OK, or ERROR if muxUnbind routine fails.
*
* NOMANUAL
*/

LOCAL STATUS ipShutdownRtn
    (
    void * 	pCookie, 	/* protocol/device binding from muxBind() */
    void * 	pSpare 		/* spare pointer from muxBind() */
    )
    {
    IP_DRV_CTRL * 	pDrvCtrl = (IP_DRV_CTRL *)pSpare;

    if (!pDrvCtrl->attached)
	return (ERROR);

    if_dettach(&pDrvCtrl->idr.ac_if);

    KHEAP_FREE(pDrvCtrl->idr.ac_if.if_name);

    netMblkFree (_pNetDpool, pDrvCtrl->pSrc);
    netClFree (_pNetDpool, pDrvCtrl->pDstAddr);
    netMblkFree (_pNetDpool, pDrvCtrl->pDst);

    if (muxUnbind (pDrvCtrl->pIpCookie, ETHERTYPE_IP, ipReceiveRtn) != OK)
	{
	logMsg ("Could not un-bind from the IP MUX!", 0, 0, 0, 0, 0, 0);
	return (ERROR);
	}

    if (pDrvCtrl->pArpCookie == NULL)
        {
        pDrvCtrl->attached = FALSE;
        }

    pDrvCtrl->pIpCookie = NULL;

    return (OK);
    }

/******************************************************************************
*
* arpShutdownRtn - Shutdown the NULL protocol stack gracefully.
* 
* This routine is called by the lower layer upon muxDevUnload or a similar
* condition to tell the protocol to shut itself down.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

LOCAL STATUS arpShutdownRtn
    (
    void * 	pCookie, 	/* protocol/device binding from muxBind() */
    void * 	pSpare 		/* spare pointer from muxBind() */
    )
    {
    IP_DRV_CTRL * pDrvCtrl = (IP_DRV_CTRL *)pSpare;

    if (pDrvCtrl == NULL)
        return (ERROR);
            
    if (!pDrvCtrl->attached)
	return (ERROR);

    if (muxUnbind (pDrvCtrl->pArpCookie, ETHERTYPE_ARP, ipReceiveRtn) != OK)
	{
	logMsg ("Could not un-bind from the ARP MUX!", 0, 0, 0, 0, 0, 0);
	return (ERROR);
	}

    if (pDrvCtrl->pIpCookie == NULL)
        {
        pDrvCtrl->attached = FALSE;
        }

    pDrvCtrl->pArpCookie = NULL;

    return (OK);
    }

/******************************************************************************
*
* ipTkShutdownRtn - Shutdown the protocol stack bound to a toolkit driver
*
* This routine is called by the lower layer upon muxDevUnload or a similar
* condition to tell the protocol to shut itself down.
*
* RETURNS: OK or ERROR depending on ipDetach.
*
* NOMANUAL
*/

LOCAL STATUS ipTkShutdownRtn
    (
    void * ipCallbackId       /* protocol/device binding from muxTkBind() */
    )
    {
    IP_DRV_CTRL * pDrvCtrl = (IP_DRV_CTRL *)ipCallbackId;

    if (pDrvCtrl == NULL)
        return (ERROR);
    
    if (!pDrvCtrl->attached)
        return (ERROR);

    if_dettach(&pDrvCtrl->idr.ac_if);

    KHEAP_FREE(pDrvCtrl->idr.ac_if.if_name);

    netClFree (_pNetDpool, pDrvCtrl->pDstAddr);

    if (muxUnbind(pDrvCtrl->pIpCookie, ETHERTYPE_IP, ipTkReceiveRtn) != OK)
        {
        logMsg("Could not un-bind from the IP MUX!", 1, 2, 3, 4, 5, 6);
        return (ERROR);
        }

    if (pDrvCtrl->pArpCookie == NULL)
        {
        pDrvCtrl->attached = FALSE;
        }

    pDrvCtrl->pIpCookie=NULL;

    return (OK);
    }

/******************************************************************************
*
* arpTkShutdownRtn - shutdown the protocol stack bound to a toolkit driver. 
*
* This routine is called by the lower layer upon muxDevUnload or a similar
* condition to tell the protocol to shut itself down.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

LOCAL STATUS arpTkShutdownRtn
    (
    void * ipCallbackId       /* Sent down in muxTkBind call. */
    )
    {
    IP_DRV_CTRL * pDrvCtrl = (IP_DRV_CTRL *)ipCallbackId;

    if (pDrvCtrl == NULL)
        return (ERROR);

    if (!pDrvCtrl->attached)
        return (ERROR);

    if (muxUnbind(pDrvCtrl->pArpCookie, ETHERTYPE_ARP, ipTkReceiveRtn) != OK)
        {
        logMsg("Could not un-bind from the ARP MUX!", 1, 2, 3, 4, 5, 6);
        return (ERROR);
        }

    if (pDrvCtrl->pIpCookie == NULL)
        {
        pDrvCtrl->attached = FALSE;
        }

    pDrvCtrl->pArpCookie=NULL;

    return (OK);
    }

/******************************************************************************
*
* ipTkError - a routine to deal with toolkit device errors
*
* This routine is called by the lower layer to handle a toolkit device error.
*
* RETURNS: N/A
*
* NOMANUAL
*
*/

void ipTkError
    (
    void * ipCallbackId,  /* Sent down in muxTkBind call. */
    END_ERR* pError       /* Error message */
    )
    {
    IP_DRV_CTRL * pDrvCtrl = (IP_DRV_CTRL *)ipCallbackId;
    END_OBJ * pEnd  = NULL;

    if (pDrvCtrl)
	 pEnd = PCOOKIE_TO_ENDOBJ (pDrvCtrl->pIpCookie);

    if (pDrvCtrl == NULL || pEnd == NULL)
        return;

    ipError (pEnd, pError, pDrvCtrl);

    return;
    }

/******************************************************************************
*
* ipError - a routine to deal with device errors
*
* This routine handles all errors from NPT and END drivers.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void ipError
    (
    END_OBJ * 	pEnd, 		/* END reporting the error */
    END_ERR * 	pError, 	/* the error message */
    void * 	pSpare 		/* spare pointer from muxBind() */
    )
    {
    int s;
    IP_DRV_CTRL * pDrvCtrl = (IP_DRV_CTRL *)pSpare;
    struct ifnet * pIfp;
    USHORT newFlags;

    if (pEnd == NULL || pDrvCtrl == NULL)
    	return;

    pIfp = (struct ifnet *)&pDrvCtrl->idr;

    switch (pError->errCode)
        {
#ifdef IP_DEBUG
        case END_ERR_INFO:
            if (ipDebug && pError->pMesg != NULL)
                logMsg ("INFO: Device: %s Unit: %d Msg: %s\n",
                        (int)pEnd->devObject.name, pEnd->devObject.unit,
                        (int)pError->pMesg, 0, 0, 0);
                break;
        case END_ERR_WARN:
            if (ipDebug && pError->pMesg != NULL)
                logMsg ("WARN: Device: %s Unit: %d Msg: %s\n",
                        (int)pEnd->devObject.name, pEnd->devObject.unit,
                        (int)pError->pMesg, 0, 0, 0);
                break;
#endif /* IP_DEBUG */

        case END_ERR_RESET:
#ifdef IP_DEBUG
            if (ipDebug && pError->pMesg != NULL)
                logMsg ("RESET: Device: %s Unit: %d Msg: %s\n",
                        (int)pEnd->devObject.name, pEnd->devObject.unit,
                        (int)pError->pMesg, 0, 0, 0);
#endif /* IP_DEBUG */

            pDrvCtrl->idr.ac_if.if_lastchange = tickGet();
            break;

        case END_ERR_UP:
#ifdef IP_DEBUG
            if (ipDebug && pError->pMesg != NULL)
                logMsg ("UP: Device: %s Unit: %d Msg: %s\n",
                        (int)pEnd->devObject.name, pEnd->devObject.unit,
                        (int)pError->pMesg, 0, 0, 0);
#endif /* IP_DEBUG */

            if ( (pIfp->if_flags & IFF_UP) == 0)
                {
                s = splimp ();
                if_up (pIfp);
                splx (s);
                }

            pDrvCtrl->idr.ac_if.if_lastchange = tickGet();
            ((IFNET *)&pDrvCtrl->idr)->if_flags |= (IFF_UP| IFF_RUNNING);
            break;
        case END_ERR_DOWN:
#ifdef IP_DEBUG
            if (ipDebug && pError->pMesg != NULL)
                logMsg ("DOWN: Device: %s Unit: %d Msg: %s\n",
                        (int)pEnd->devObject.name, pEnd->devObject.unit,
                        (int)pError->pMesg, 0, 0, 0);
#endif /* IP_DEBUG */

            if ( (pIfp->if_flags & IFF_UP))
                {
                s = splimp ();
                if_down (pIfp);
                splx (s);
                }

            pDrvCtrl->idr.ac_if.if_lastchange = tickGet();
            ((IFNET *)&pDrvCtrl->idr)->if_flags &= ~(IFF_UP| IFF_RUNNING);
            break;

        case END_ERR_FLAGS:
#ifdef IP_DEBUG
            if (ipDebug && pError->pMesg != NULL)
                logMsg ("ipError:Msg from device %s Unit: %d Msg: %s\n",
                        (int)pEnd->devObject.name, pEnd->devObject.unit,
                        (int)pError->pMesg, 0, 0, 0);
#endif /* IP_DEBUG */

            newFlags = (USHORT) ( (UINT32)pError->pSpare);

            if ( (pIfp->if_flags & IFF_UP) ^ (newFlags & IFF_UP))
                {
                s = splimp ();

                if ( (pIfp->if_flags & IFF_UP) == 0)
                    if_up (pIfp);
                else
                    if_down (pIfp);

                splx (s);
                }

            /* Set interface flags to new values, if allowed. */

            newFlags = newFlags & ~IFF_CANTCHANGE;
            pIfp->if_flags &= IFF_CANTCHANGE;
            pIfp->if_flags = pIfp->if_flags | newFlags;

            break;
        case END_ERR_NO_BUF:
#ifdef IP_DEBUG
            if (ipDebug && pError->pMesg != NULL)
                logMsg ("NO_BUF: Device: %s Unit: %d Msg: %s\n",
                        (int)pEnd->devObject.name, pEnd->devObject.unit,
                        (int)pError->pMesg, 0, 0, 0);
#endif /* IP_DEBUG */
            ip_drain();                      /* empty the fragment Q */

            break;
        default:
#ifdef IP_DEBUG
            if (ipDebug && pError->pMesg != NULL)
                logMsg ("UNKOWN ERROR: Device: %s Unit: %d Msg: %s\n",
                        (int)pEnd->devObject.name, pEnd->devObject.unit,
                        (int)pError->pMesg, 0, 0, 0);
#endif /* IP_DEBUG */
            
		break;
        }
    }

/******************************************************************************
*
* ipAttach - a generic attach routine for the TCP/IP network stack
*
* This routine takes the unit number and device name of an END or NPT
* driver (e.g., "ln0", "ei0", etc.) and attaches the IP protocol to
* the corresponding device. Following a successful attachment IP will
* begin receiving packets from the devices.
*
* RETURNS: OK or ERROR
*/

int ipAttach
    (
    int unit,                   /* Unit number  */
    char *pDevice		/* Device name (i.e. ln, ei etc.). */
    )
    {
    long flags;
    M2_INTERFACETBL mib2Tbl;
    M2_ID * pM2ID; 
    IP_DRV_CTRL* pDrvCtrl;
#ifndef VIRTUAL_STACK
    IMPORT int          ipMaxUnits;
#endif
    struct ifnet* pIfp = NULL;
    char *pName = NULL;
    END_OBJ* pEnd;
    struct ifaddr *pIfa;
    struct sockaddr_dl *pSdl;
    CL_POOL_ID pClPoolId;
    int ifHdrLen;
    
    int count;
    int limit = ipMaxUnits;
    int freeslot = -1;
    
    pEnd = endFindByName (pDevice, unit);
    if (pEnd == NULL)
        {
        logMsg ("ipAttach: Can't attach %s (unit %d). It is not an END device.\n",
                (int)pDevice, unit, 0, 0, 0, 0);
        return (ERROR);
        }
    
    for (count = 0; count < limit; count++)
        {
        pDrvCtrl = &ipDrvCtrl [count];
        
        if (pDrvCtrl->attached)
            {
            /* Already attached to requested device? */
            
            if (PCOOKIE_TO_ENDOBJ (pDrvCtrl->pIpCookie) == pEnd)
                return (OK);
            }
        else if (freeslot == -1)
            freeslot = count;
        }
    
    if (freeslot == -1)
        {
        /* Too many attach attempts. */
        
        logMsg ("Protocol is out of space. Increase IP_MAX_UNITS.\n",
                0, 0, 0, 0, 0, 0);
        
        return (ERROR);
        }
    
    pDrvCtrl = &ipDrvCtrl [freeslot];
    bzero ( (char *)pDrvCtrl, sizeof (IP_DRV_CTRL));
    
    /*
     * Bind IP to the device using the appropriate routines for the driver
     * type. NPT devices use a different receive routine since those drivers
     * remove the link-level header before handing incoming data to the MUX
     * layer.
     */
    
    pDrvCtrl->nptFlag = FALSE;
    
    if (pEnd->pFuncTable->ioctl)
        {
        if ( (pEnd->pFuncTable->ioctl (pEnd, EIOCGNPT, NULL)) == OK)
            {
            /* NPT device. */
            pDrvCtrl->nptFlag = TRUE;
            }
        }
    
    if (pDrvCtrl->nptFlag)
        {
        /* NPT device */
        pDrvCtrl->pIpCookie = muxTkBind (pDevice, unit, ipTkReceiveRtn,
                                         ipTkShutdownRtn, ipTkTxRestart,
                                         ipTkError, ETHERTYPE_IP, 
                                         "IP 4.4 TCP/IP",
                                         pDrvCtrl, NULL, NULL);
        
        if (pDrvCtrl->pIpCookie == NULL)
            {
            logMsg ("TCP/IP could not bind to the MUX: muxTkBind! %s",
                    (int)pDevice, 0, 0, 0, 0, 0);
            goto ipAttachError;
            }
        }
    else
        {
        /* END device */
        pDrvCtrl->pIpCookie = muxBind (pDevice, unit, (FUNCPTR) ipReceiveRtn,
                                       (FUNCPTR) ipShutdownRtn, 
                                       (FUNCPTR) ipTxRestart,
                                       (VOIDFUNCPTR) ipError, ETHERTYPE_IP,
                                       "IP 4.4 TCP/IP", (void *) pDrvCtrl);
        if (pDrvCtrl->pIpCookie == NULL)
            {
            logMsg ("TCP/IP could not bind to the MUX! %s", (int)pDevice,
                    0, 0, 0, 0, 0);
            goto ipAttachError;
            }
        
        
        /*
         * END devices require additional memory to construct a
         * link-level header for outgoing data.
         */
        
        pDrvCtrl->pSrc = netMblkGet (_pNetDpool, M_DONTWAIT, MT_DATA);
        if (pDrvCtrl->pSrc == NULL)
            {
            goto ipAttachError;
            }
        
        pDrvCtrl->pDst = netMblkGet (_pNetDpool, M_DONTWAIT, MT_DATA);
        if (pDrvCtrl->pDst == NULL)
            {
            goto ipAttachError;
            }
        }
        
    /* Get storage for link-level destination addresses. */
    
    pClPoolId = netClPoolIdGet (_pNetDpool, MAX_ADDRLEN, TRUE);
    if (pClPoolId == NULL)
        {
        goto ipAttachError;
        }
    
    pDrvCtrl->pDstAddr = netClusterGet (_pNetDpool, pClPoolId);
    if (pDrvCtrl->pDstAddr == NULL)
        {
        goto ipAttachError;
        }
    
    /* Set the internal mBlk to access the link-level information. */
    
    if (!pDrvCtrl->nptFlag)
        pDrvCtrl->pDst->m_data = pDrvCtrl->pDstAddr;
    
    pName = KHEAP_ALLOC( (strlen (pDevice) + 1));    /* +1: EOS character */
    if (pName == NULL)
        {
        goto ipAttachError;
        }
    bzero (pName, strlen (pDevice) + 1);
    strcpy (pName, pDevice);

    /* Get all necessary lower-level device information. */
    
    muxIoctl (pDrvCtrl->pIpCookie, EIOCGFLAGS, (caddr_t)&flags);
    
    if (pEnd->flags & END_MIB_2233)
        {
        if (muxIoctl(pDrvCtrl->pIpCookie, EIOCGMIB2233,
                     (caddr_t)&pM2ID) == ERROR)
	    {
	    logMsg("ipAttach: could not get MIB2 Table for device %s #%d\n",
                   (int)pDevice,unit,3,4,5,6);
            
	    goto ipAttachError;
	    }
        
        memcpy(&mib2Tbl, &(pM2ID->m2Data.mibIfTbl), sizeof(M2_INTERFACETBL));
        }
    else /* (RFC1213 style of counters supported) XXX */
        {
        if (muxIoctl(pDrvCtrl->pIpCookie, EIOCGMIB2,
                     (caddr_t)&mib2Tbl) == ERROR)
	    {
	    logMsg("ipAttach: could not get MIB2 Table for device %s #%d\n",
                   (int)pDevice,unit,3,4,5,6);
            
	    goto ipAttachError;
	    }
        }

    pIfp = (IFNET *)&pDrvCtrl->idr;
    
    /* Hook up the interface pointers etc. */
    
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_VERBOSE event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_VERBOSE, 14, 17, 
                     WV_NETEVENT_ETHERATTACH_START, pIfp)
#endif  /* INCLUDE_WVNET */
#endif
        
        bzero ((char *) pIfp, sizeof (*pIfp));
    
#ifdef VIRTUAL_STACK
    virtualStackIdCheck();  /* make sure the myStackNum exists */
    pIfp->vsNum = myStackNum;
#endif /* VIRTUAL_STACK */
    
    pIfp->if_unit   = unit;
    pIfp->if_name   = pName;
    pIfp->if_mtu    = mib2Tbl.ifMtu;
    pIfp->if_baudrate = mib2Tbl.ifSpeed;
    pIfp->if_init   = NULL;
    pIfp->if_ioctl  = (FUNCPTR) ipIoctl;
    pIfp->if_output = ipOutput;
    pIfp->if_reset  = NULL;
    pIfp->if_resolve = muxAddrResFuncGet (mib2Tbl.ifType, ETHERTYPE_IP);
    pIfp->if_flags  = flags;
    pIfp->if_type = mib2Tbl.ifType;
    pIfp->if_addrlen = mib2Tbl.ifPhysAddress.addrLength;
    pIfp->pCookie = (void *)pDrvCtrl;

    /*
     * Store address information for supported devices (802.x addressing)
     * and bind ARP to the device using the appropriate receive routine.
     */

    switch (mib2Tbl.ifType)
        {
        case M2_ifType_ethernet_csmacd:
        case M2_ifType_iso88023_csmacd:
        case M2_ifType_iso88024_tokenBus:
        case M2_ifType_iso88025_tokenRing:
        case M2_ifType_iso88026_man:
        case M2_ifType_fddi:

            bcopy (mib2Tbl.ifPhysAddress.phyAddress, pDrvCtrl->idr.ac_enaddr,
                   pIfp->if_addrlen);
            
            if (pDrvCtrl->nptFlag)
                {
                pDrvCtrl->pArpCookie = muxTkBind (pDevice, unit,
                                                  ipTkReceiveRtn,
                                                  arpTkShutdownRtn,
                                                  ipTkTxRestart,
                                                  NULL, ETHERTYPE_ARP, 
                                                  "IP 4.4 ARP",
                                                  pDrvCtrl, NULL, NULL);
                if (pDrvCtrl->pArpCookie == NULL)
                    {
                    logMsg ("ARP could not bind to the MUX: muxTkBind! %s",
                            (int)pDevice, 0, 0, 0, 0, 0);
                    goto ipAttachError;
                    }
                }
            else     /* END device */
                {
                pDrvCtrl->pArpCookie = muxBind (pDevice, unit,
                                                (FUNCPTR) ipReceiveRtn,
                                                (FUNCPTR) arpShutdownRtn, 
                                                (FUNCPTR) ipTxRestart,
                                                NULL, ETHERTYPE_ARP, 
                                                "IP 4.4 ARP",
                                                pDrvCtrl);
                if (pDrvCtrl->pArpCookie == NULL)
                    {
                    logMsg ("ARP could not bind to the MUX! %s", (int)pDevice,
                            0, 0, 0, 0, 0);
                    goto ipAttachError;
                    }
                }
            break;

        default:
            break;
        }
    
    /* Get the length of the header. */

    if (muxIoctl (pDrvCtrl->pIpCookie, EIOCGHDRLEN, (caddr_t)&ifHdrLen) != OK)
        pIfp->if_hdrlen = 0;
    else
        pIfp->if_hdrlen = (short)ifHdrLen;

    
    /* Attach the interface. */
    /* NOTE: if_attach must come before "common duties" below.  see spr# 29152 */
    
    if (if_attach (pIfp) == ERROR)
        goto ipAttachError;
    
    /*
     * Perform common duties while attaching to interface list
     */

    for (pIfa = pIfp->if_addrlist; pIfa; pIfa = pIfa->ifa_next)
        {
        if ((pSdl = (struct sockaddr_dl *)pIfa->ifa_addr) &&
            pSdl->sdl_family == AF_LINK)
            {
            pSdl->sdl_type = pIfp->if_type;
            pSdl->sdl_alen = pIfp->if_addrlen;
            bcopy (mib2Tbl.ifPhysAddress.phyAddress,
                   LLADDR (pSdl), pSdl->sdl_alen);
            break;
            }
        }
    
    pIfp->if_start = (FUNCPTR)ipTxStartup;
    
    pDrvCtrl->attached = TRUE;
    
    return (OK);

    ipAttachError:
    
    /* Cleanup by removing any bound protocols and/or allocated memory. */
    
    if (pDrvCtrl->pDstAddr)
        netClFree (_pNetDpool, pDrvCtrl->pDstAddr);
    
    if (pDrvCtrl->nptFlag)
        {
        if (pDrvCtrl->pIpCookie)
            muxUnbind (pDrvCtrl->pIpCookie, ETHERTYPE_IP, ipTkReceiveRtn);
        
        if (pDrvCtrl->pArpCookie)
            muxUnbind (pDrvCtrl->pArpCookie, ETHERTYPE_IP, ipTkReceiveRtn);
        }
    else
        {
        if (pDrvCtrl->pIpCookie)
            muxUnbind (pDrvCtrl->pIpCookie, ETHERTYPE_IP, ipReceiveRtn);
        
        if (pDrvCtrl->pArpCookie)
            muxUnbind (pDrvCtrl->pArpCookie, ETHERTYPE_IP, ipReceiveRtn);
        }
    
    if (pDrvCtrl->pSrc)
        netMblkFree (_pNetDpool, pDrvCtrl->pSrc);
    
    if (pDrvCtrl->pDst)
        netMblkFree (_pNetDpool, pDrvCtrl->pDst);
    
    return (ERROR);
    }

/*******************************************************************************
*
* ipTxRestart - restart routine 
*
* NOMANUAL
*/

LOCAL int ipTxRestart
    (
    void * pCookie,	/* device identifier (from muxDevLoad) */
    IP_DRV_CTRL * pDrvCtrl
    )
    {
    if (pDrvCtrl == NULL)
        return (ERROR);
    
    ipTxStartup (pDrvCtrl);
    
    return (OK);
    }

/*******************************************************************************
*
* ipTkTxRestart - restart routine registered with a toolkit driver. 
*
* NOMANUAL
*/

LOCAL int ipTkTxRestart
    (
    void * ipCallbackId       /* Sent down in muxTkBind call. */
    )
    {
    IP_DRV_CTRL * pDrvCtrl = (IP_DRV_CTRL *)ipCallbackId;

    if (pDrvCtrl == NULL)
        return (ERROR);

    ipTxStartup (pDrvCtrl);

    return (OK);
    }

/*******************************************************************************
*
* ipTxStartup - start output on the lower layer
*
* Looks for any action on the queue, and begins output if there is anything
* there.  This routine is called from several possible threads.  Each will be
* described below.
*
* The first, and most common thread, is when a user task requests the
* transmission of data.  This will cause muxSend() to be called, which will
* cause ether_output() to be called, which will cause this routine to be
* called (usually).  This routine will not be called if ether_output() finds
* that our interface output queue is full.  In this case, the outgoing data
* will be thrown out.
*
* The second, and most obscure thread, is when the reception of certain
* packets causes an immediate (attempted) response.  For example, ICMP echo
* packets (ping), and ICMP "no listener on that port" notifications.  All
* functions in this driver that handle the reception side are executed in the
* context of netTask().  Always.  So, in the case being discussed, netTask() 
* will receive these certain packets, cause IP to be stimulated, and cause the
* generation of a response to be sent.  We then find ourselves following the
* thread explained in the second example, with the important distinction that
* the context is that of netTask().
*
* This routine is also called from ipTxRestart() and ipTkTxRestart().
*
* NOMANUAL
*/

void ipTxStartup
    (
    IP_DRV_CTRL * pDrvCtrl		/* pointer to the drv control */
    )
    {
    M_BLK_ID      pMblk;
    STATUS        status = OK;

    struct ifnet * 	pIf;
    BOOL 		nptFlag;

    USHORT netType = 0;
    char dstAddrBuf [MAX_ADDRLEN];
    M_BLK_ID pNetPkt = NULL;
    int s;

    if (pDrvCtrl == NULL)
        return;

    if (!pDrvCtrl->attached)
        {
#ifdef IP_DEBUG
        if (ipDebug)
            logMsg ("ipTxStartup not attached!\n", 0, 0, 0, 0, 0, 0);
#endif /* IP_DEBUG */
        return;
        }
    s = splnet();
    nptFlag = pDrvCtrl->nptFlag;

    pIf = (struct ifnet *)&pDrvCtrl->idr;

    /*
     * Loop until there are no more packets ready to send or we
     * have insufficient resources left to send another one.
     */

    while (pIf->if_snd.ifq_head)
        {
        /* Dequeue a packet. */

        IF_DEQUEUE (&pIf->if_snd, pMblk);  

#ifdef ETHER_OUTPUT_HOOKS
        if ((etherOutputHookRtn != NULL) &&
            (* etherOutputHookRtn)
          (&pDrvCtrl->idr, (struct ether_header *)pMblk->m_data,pMblk->m_len))
            {
            continue;
            }
#endif /* ETHER_OUTPUT_HOOKS */

        if (nptFlag)
            {
            /*
             * For NPT devices, the output processing adds the network packet
             * type (and the link-level destination address, if any) to the
             * start of the packet. Retrieve that information and pass it to
             * the NPT transmit routine.
             */

            netType = pMblk->mBlkHdr.reserved;
            bzero (dstAddrBuf, MAX_ADDRLEN);
            pNetPkt = pMblk;

            if (pIf->if_addrlen)
                {
                /* Save the dest. address to protect it from driver changes. */

                bcopy (pMblk->mBlkHdr.mData, dstAddrBuf, pIf->if_addrlen);

                /* Restore original network packet data pointers and length. */

                m_adj (pMblk, pIf->if_addrlen);

                if (pMblk->mBlkHdr.mLen == 0)
                    {
                    /*
                     * The destination address used a new mBlk in the chain.
                     * Access the next entry containing the network packet
                     * and restore the header flag setting.
                     */

                    pNetPkt = pMblk->mBlkHdr.mNext;

                    if (pMblk->mBlkHdr.mFlags & M_PKTHDR)
                        {
                        pNetPkt->mBlkHdr.mFlags |= M_PKTHDR;
                        }
                    }
                }

            /* Attempt to send the packet. */

            status = muxTkSend (pDrvCtrl->pIpCookie, pNetPkt, dstAddrBuf,
                                netType, NULL);
            }
        else
            {
            /*
             * For END devices, the transmit queue contains a complete
             * link-level frame. Attempt to send that data.
             */

            status = muxSend (pDrvCtrl->pIpCookie, pMblk);
            }

        if (status == END_ERR_BLOCK)
            {
            /*
             * The device is currently unable to transmit. Return the
             * packet or frame to the queue to retry later.
             */

#ifdef IP_DEBUG
            if (ipDebug)
                logMsg ("Transmit error!\n", 0, 0, 0, 0, 0, 0);
#endif /* IP_DEBUG */

            if (nptFlag && pIf->if_addrlen)
                {
                /*
                 * For NPT devices, restore the link-level destination
                 * address, in case the driver processing altered it.
                 */

                if (pMblk == pNetPkt)
                    {
                    /*
                     * The destination address used the initial mBlk in the
                     * chain. Change the data pointer to provide copy space.
                     */

                    pMblk->mBlkHdr.mData -= pIf->if_addrlen;
                    }

                bcopy (dstAddrBuf, pMblk->mBlkHdr.mData, pIf->if_addrlen);
                pMblk->mBlkHdr.mLen += pIf->if_addrlen;

                if (pMblk->mBlkHdr.mFlags & M_PKTHDR)
                    pMblk->mBlkPktHdr.len += pIf->if_addrlen;
                }

	    /* If we are transmitting make a new copy and PREPEND it. */

	    if (IF_QFULL(&pDrvCtrl->idr.ac_if.if_snd))
	        {
	        IF_DROP(&pDrvCtrl->idr.ac_if.if_snd);
	        netMblkClChainFree (pMblk);
	        ++pDrvCtrl->idr.ac_if.if_oerrors;          
                }
	    else
	        IF_PREPEND (&pDrvCtrl->idr.ac_if.if_snd, pMblk);
            break;
            }
        else     /* Successful handoff to driver for transmission. */
           {
           ++pDrvCtrl->idr.ac_if.if_opackets;

            /* For NPT devices, free any Mblk prepended for the link-level
             * destination address.  */
           if (nptFlag && (pMblk != pNetPkt))
               {
               netMblkClFree(pMblk);
               }
           }
        
#ifdef IP_DEBUG
        if (ipDebug)
            logMsg("ipTxStartup done!\n", 0, 0, 0, 0, 0, 0);
#endif /* IP_DEBUG */
        }
    splx(s);
    }

/*******************************************************************************
*
* ipDetach - a generic detach routine for the TCP/IP network stack
*
* This routine removes the TCP/IP stack from the MUX. If completed
* successfully, the IP protocol will no longer receive packets
* from the named END driver. 
*
* RETURNS: OK or ERROR
*/

STATUS ipDetach
    (
    int unit,                   /* Unit number  */
    char *pDevice		/* Device name (i.e. ln, ei etc.). */
    )
    {
    END_OBJ * 	pEnd;
    IP_DRV_CTRL* pDrvCtrl = NULL;
    int error;                 /* return error status */
    int count;
#ifndef VIRTUAL_STACK
    IMPORT int          ipMaxUnits;
#endif
    int limit = ipMaxUnits;
    char        ifName[IFNAMSIZ];               /* if name, e.g. "en0" */


    /* Check if device is valid. */

    pEnd = endFindByName (pDevice, unit);
    if (pEnd == NULL)
        return (ERROR);

    for (count = 0; count < limit; count++)
        {
        pDrvCtrl = &ipDrvCtrl [count];

        if (pDrvCtrl->attached &&
                PCOOKIE_TO_ENDOBJ (pDrvCtrl->pIpCookie) == pEnd)
            break;
        }

    if (count == limit)
        {
        /* Device not attached. */

        return (ERROR);
        }

    /* Now bring down the interface */
 
    sprintf (ifName,"%s%d",pDevice, unit);
    ifFlagChange(ifName, IFF_UP, 0);
 
    /* Now remove the interface route */
 
    ifRouteDelete(pDevice, unit);
    
    if (pDrvCtrl->nptFlag)       /* npt device, call npt routines */
        {
        error = ipTkShutdownRtn (pDrvCtrl);
        if (pDrvCtrl->pArpCookie)
            error |= arpTkShutdownRtn (pDrvCtrl);
        }
    else
        {
        error = ipShutdownRtn (pDrvCtrl->pIpCookie, pDrvCtrl);
        if (pDrvCtrl->pArpCookie)
            error |= arpShutdownRtn (pDrvCtrl->pArpCookie, pDrvCtrl);
        }
    
    return (error);
    }

/*******************************************************************************
*
* ipIoctl - the IP I/O control routine
*
* Process an ioctl request.
*
* NOMANUAL
*/

LOCAL int ipIoctl
    (
    IDR  *ifp,
    int            cmd,
    caddr_t        data
    )
    {
    int error = OK;

    struct ifreq *ifr;
    IP_DRV_CTRL* pDrvCtrl;
    register struct in_ifaddr* pIa = 0;
    M2_INTERFACETBL mib2Tbl;
    struct sockaddr* inetaddr;
    char   mapMCastBuff[128];
    long flagsMask;
    register M2_NETDRVCNFG *pm2DrvCnfg = (M2_NETDRVCNFG *)data;
    register M2_NETDRVCNTRS *pm2DrvCntr = (M2_NETDRVCNTRS *)data;
    u_long dt_saddr = ((struct in_ifaddr *)data)->ia_addr.sin_addr.s_addr;
    struct mBlk* pMblk;

    pDrvCtrl = (IP_DRV_CTRL *)(ifp->ac_if.pCookie);

    if (pDrvCtrl->pIpCookie == NULL)
        return (EINVAL);

    switch ((u_int) cmd)
	{
	case SIOCSIFADDR:
#ifdef VIRTUAL_STACK
            for (pIa = _in_ifaddr; pIa; pIa = pIa->ia_next)
#else
            for (pIa = in_ifaddr; pIa; pIa = pIa->ia_next)
#endif /* VIRTUAL_STACK */
                if ((pIa->ia_ifp == (struct ifnet *)ifp) &&
                    (pIa->ia_addr.sin_addr.s_addr == dt_saddr))
                    break;

            /*
             * For devices with 802.x addressing, setup ARP processing
             * for the IP address.
             */

            switch (ifp->ac_if.if_type)
                {
                case M2_ifType_ethernet_csmacd:
                case M2_ifType_iso88023_csmacd:
                case M2_ifType_iso88024_tokenBus:
                case M2_ifType_iso88025_tokenRing:
                case M2_ifType_iso88026_man:
                case M2_ifType_fddi:

#ifdef ROUTER_STACK		/* UNNUMBERED_SUPPORT */ 
	            if (pIa->ia_ifp->if_flags & IFF_UNNUMBERED)
		        {
		        pIa->ia_ifa.ifa_rtrequest = NULL;
		        pIa->ia_ifa.ifa_flags &= ~RTF_CLONING;
		        ifp->ac_ipaddr = IA_SIN (data)->sin_addr;
		        break;
		        }
#endif /* ROUTER_STACK */

                    pIa->ia_ifa.ifa_rtrequest = arp_rtrequest;
                    pIa->ia_ifa.ifa_flags |= RTF_CLONING;
                    ifp->ac_ipaddr = IA_SIN (data)->sin_addr;

                    /* Generate gratuitous ARP requests only for ethernet devices. */
                    arpwhohas ((struct arpcom*) ifp, &IA_SIN (data)->sin_addr);

                    break;

                default:
                    break;
                }

	    break;

        case SIOCGMIB2CNFG:
	    error = muxIoctl (pDrvCtrl->pIpCookie, EIOCGMIB2,
                              (caddr_t)&mib2Tbl);
            if (error != OK)
                return (EINVAL);
            pm2DrvCnfg->ifType = mib2Tbl.ifType;		/* ppp type */
            if (mib2Tbl.ifSpecific.idLength)
                {
                pm2DrvCnfg->ifSpecific.idLength = mib2Tbl.ifSpecific.idLength;
                memcpy (&pm2DrvCnfg->ifSpecific.idArray[0],
                            &mib2Tbl.ifSpecific.idArray[0],
                            mib2Tbl.ifSpecific.idLength * sizeof (long));
                }
            else
                pm2DrvCnfg->ifSpecific.idLength = 0;

            break;
            
        case SIOCGMIB2CNTRS:				/* fill in counters */
	    error = muxIoctl (pDrvCtrl->pIpCookie, EIOCGMIB2,
                              (caddr_t)&mib2Tbl);
            if (error != OK)
                return (EINVAL);
	    pm2DrvCntr->ifSpeed = mib2Tbl.ifSpeed;
            pm2DrvCntr->ifInOctets = mib2Tbl.ifInOctets;
            pm2DrvCntr->ifInNUcastPkts = mib2Tbl.ifInNUcastPkts;
            pm2DrvCntr->ifInDiscards = mib2Tbl.ifInDiscards;
            pm2DrvCntr->ifInUnknownProtos = mib2Tbl.ifInUnknownProtos;
            pm2DrvCntr->ifOutOctets = mib2Tbl.ifOutOctets;
            pm2DrvCntr->ifOutNUcastPkts = mib2Tbl.ifOutNUcastPkts;
            pm2DrvCntr->ifOutDiscards = mib2Tbl.ifOutDiscards;
            break;

        case SIOCGMIB2233:
            error = muxIoctl (pDrvCtrl->pIpCookie, EIOCGMIB2233, data);
            if (error != OK)
                return (EINVAL);
            break;

        case SIOCSMIB2233:
            error = muxIoctl (pDrvCtrl->pIpCookie, EIOCSMIB2233, data);
            if (error != OK)
                return (EINVAL);
            break;
            
        case SIOCADDMULTI:
            /* Don't allow group membership on non-multicast
               interfaces. */
            if ((ifp->ac_if.if_flags & IFF_MULTICAST) == 0)
                return EOPNOTSUPP;
            
            if (ifp->ac_if.if_resolve)
                {
                pDrvCtrl = (IP_DRV_CTRL*)ifp->ac_if.pCookie;
                ifr = (struct ifreq *)data;
                inetaddr = &ifr->ifr_addr;

                pMblk = KHEAP_ALLOC(sizeof(M_BLK));
                if (pMblk == NULL)
                    {
                    error = ENOBUFS;
                    break;
                    }
                bzero ( (char *)pMblk, sizeof (M_BLK));

                pMblk->mBlkHdr.mFlags |= M_MCAST;

                /*
                 * call the registered address resolution function
                 * to do the mapping
                 */

                if (ifp->ac_if.if_resolve (&ipMcastResume, pMblk, inetaddr,
                                           &ifp->ac_if, SIOCADDMULTI, &mapMCastBuff) == 0)
		    {
		    KHEAP_FREE( (char *)pMblk);
		    break; /* not yet resolved */
		    }
                KHEAP_FREE( (char *)pMblk);

                /* register the mapped multicast MAC address */

                error = muxMCastAddrAdd (pDrvCtrl->pIpCookie,
                                         (char *)&mapMCastBuff);
                }
            break;           /* if no resolve function, return OK */

        case SIOCDELMULTI:
            /* Don't allow group membership on non-multicast interfaces. */
            if ((ifp->ac_if.if_flags & IFF_MULTICAST) == 0)
                return EOPNOTSUPP;
            
            if (ifp->ac_if.if_resolve)
                {
                pDrvCtrl = ifp->ac_if.pCookie;
                ifr = (struct ifreq *)data;
                inetaddr = (&ifr->ifr_addr);
                
                pMblk = KHEAP_ALLOC(sizeof (M_BLK));
                if (pMblk == NULL)
                    {
                    error = ENOBUFS;
                    break;
                    }
                bzero ( (char *)pMblk, sizeof (M_BLK));

                pMblk->mBlkHdr.mFlags |= M_MCAST;

                /*
                 * call the registered address resolution function to do
                 * the mapping
                 */
                
                if (ifp->ac_if.if_resolve (&ipMcastResume, pMblk, inetaddr,
                                     &ifp->ac_if, SIOCDELMULTI, &mapMCastBuff) == 0)
		    {
		    KHEAP_FREE( (char *)pMblk);
		    break; /* not yet resolved */
		    }
		KHEAP_FREE( (char *)pMblk);
                
		/* delete the mapped multicast MAC address */
                
		error = muxMCastAddrDel (pDrvCtrl->pIpCookie,
                                         (char *)&mapMCastBuff);
		}
            break;            /* if no resolve routine, return OK */
            
	case SIOCGETMULTI:
	    break;
            
	case SIOCSIFFLAGS:
	    ifr = (struct ifreq *) data;
			    
	    /* 
	     * Turn off all flags that are disabled in the request 
	     * correcting for the conversion from short to long 
	     */
	    
	    flagsMask = (unsigned short) (~ ifr->ifr_flags);

	    /* Two's complement used to disable flags by muxIoctl () */
	    
	    flagsMask = -flagsMask - 1;
	    error = muxIoctl (pDrvCtrl->pIpCookie, EIOCSFLAGS,
                              (caddr_t) flagsMask);

	    /* 
	     * Next set all flags that are set in request correcting for 
	     * the conversion from short to long. 
	     */

	    flagsMask = (unsigned short) ifr->ifr_flags;

	    error |= muxIoctl (pDrvCtrl->pIpCookie, EIOCSFLAGS,
                               (caddr_t) flagsMask);
	    break;

        case SIOCGMCASTLIST:
            error = muxIoctl (pDrvCtrl->pIpCookie, EIOCGMCASTLIST, data);
            if (error != OK)
                return (EINVAL);
            break;

	default:
	    error = muxIoctl (pDrvCtrl->pIpCookie, cmd, data);
	    break;
	}

    return (error);
    }

/******************************************************************************
*
* ipOutput - transmit routine for IP packets using END or NPT devices.
*
* This routine handles the final processing before transferring a
* network packet (contained in the <m0> argument) to the transmit
* device.
*
* For END devices, it encapsulates a packet in the link-level frame
* before transferring it to the driver. The output processing constructs
* Ethernet frames unless the device provides a custom routine to form the
* appropriate frame header.
*
* For NPT devices, the routine just inserts the destination link-level
* address at the front of the mBlk chain. Later processing hands that
* address to the driver along with an mBlk chain containing the network
* packet (with no link-level header).
*
* NOTE: It assumes that ifp is actually a pointer to an arpcom structure.
*
* NOMANUAL
*/

int ipOutput
    (
    register struct ifnet *ifp,
    struct mbuf *m0,
    struct sockaddr *dst,
    struct rtentry *rt0
    )
    {
    u_short etype = 0;
    int s, error = 0;
    register struct mbuf *m = m0;
    register struct rtentry *rt;
    int off;
    struct arpcom *ac = (struct arpcom *)ifp;
    struct ether_header* eh;
    IP_DRV_CTRL* pDrvCtrl;

#ifdef ROUTER_STACK
    struct ip* pIpHdr;
    END_OBJ* pEnd;
    struct sockaddr_in* pNetMask;
#endif  /* ROUTER_STACK */
 
    pDrvCtrl = (IP_DRV_CTRL *)ifp->pCookie;
    if (pDrvCtrl->pIpCookie == NULL)
        senderr (EINVAL);

    if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
        {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 2, 3, 
                        WV_NETEVENT_ETHEROUT_IFDOWN, WV_NET_SEND, ifp)
#endif  /* INCLUDE_WVNET */
#endif

        senderr(ENETDOWN);
        }

    ifp->if_lastchange = tickGet();
    if ((rt = rt0))
        {
        if ((rt->rt_flags & RTF_UP) == 0)
            {
            if ((rt0 = rt = rtalloc1(dst, 1)))
                rt->rt_refcnt--;
            else
                {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
                WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 4, 5, 
                                WV_NETEVENT_ETHEROUT_NOROUTE, WV_NET_SEND,
                                ((struct sockaddr_in *)dst)->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

                senderr(EHOSTUNREACH);
                }
            }
        if (rt->rt_flags & RTF_GATEWAY)
            {
            if (rt->rt_gwroute == 0)
                goto lookup;
            if (((rt = rt->rt_gwroute)->rt_flags & RTF_UP) == 0)
                {
                rtfree(rt); rt = rt0;
                lookup: rt->rt_gwroute = rtalloc1(rt->rt_gateway, 1);
                if ((rt = rt->rt_gwroute) == 0)
                    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
                    WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 4, 5, 
                                    WV_NETEVENT_ETHEROUT_NOROUTE, WV_NET_SEND,
                     ((struct sockaddr_in *)rt->rt_gateway)->sin_addr.s_addr)
#endif  /* INCLUDE_WVNET */
#endif

                    senderr(EHOSTUNREACH);
                    }
                }
            }
        if (rt->rt_flags & RTF_REJECT)
            {
            if (rt->rt_rmx.rmx_expire == 0 ||
                tickGet() < rt->rt_rmx.rmx_expire)
                {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
                WV_NET_EVENT_0 (NET_CORE_EVENT, WV_NET_CRITICAL, 5, 6, 
                                WV_NETEVENT_ETHEROUT_RTREJECT, WV_NET_SEND)
#endif  /* INCLUDE_WVNET */
#endif

                senderr(rt == rt0 ? EHOSTDOWN : EHOSTUNREACH);
                }
            }
        } /* if (rt = rt0) */

    switch (dst->sa_family)
        {
        case AF_INET:
            if (ifp->if_resolve != NULL)
                if (!ifp->if_resolve (&ipOutputResume, m, dst, ifp, rt,
                                      pDrvCtrl->pDstAddr))
                    return (0);	/* if not yet resolved */

#ifdef ROUTER_STACK
            if ((m->m_flags & M_FORWARD)) 
                {

                /*
                 * If we are forwarding this packet, and Fastpath is
                 * enabled both at the module and interface level,
                 * supply the Fastpath module with the additional
                 * forwarding information (the MAC address etc.)
                 */

                if (SHOULD_MAKE_CALLS_TO_FF (GET_IPV4_FF_ID) && 
                    FF_MOD_ENABLED (GET_IPV4_FF_ID) &&
                    FF_INTF_ENABLED (GET_IPV4_FF_ID, ifp))
                    {
                    pEnd = PCOOKIE_TO_ENDOBJ(pDrvCtrl->pIpCookie);
                    pIpHdr= mtod(m, struct ip *);
                    pNetMask=(struct sockaddr_in*)(rt_mask(rt0));

#ifdef DEBUG
                    logMsg ("ipOutput: dst IP addr %x\n", pIpHdr->ip_dst.s_addr,
                            0, 0, 0, 0, 0);
                    logMsg ("ipOutput: rt0 is %x. rt is %x.\n", rt0, rt,
                            0, 0, 0, 0);
#endif /* DEBUG */

                    /*
                     * if indirect route pass the 'key' of the
                     * routing entry as the destination IP address.
                     * If not pass the host
                     * IP address as the destination IP address.
                     */

                    if (rt0->rt_flags & RTF_GATEWAY)
                        {
                        if (rt0->rt_flags & RTF_CLONED) 
                            rt0 = rt0->rt_parent;
                        if (rt0) 
                            FFL_CALL (ffLibRouteMoreInfo, \
                                      (GET_IPV4_FF_ID, rt_key(rt0), \
                                       rt_mask(rt0), rt0->rt_gateway, \
                                       pDrvCtrl->pDstAddr, ifp->if_addrlen, \
                                       rt0->rt_flags, ifp->if_mtu, \
                                       ifp->if_index, ifp));
                        }
                    else 
                        {
                        FFL_CALL (ffLibRouteMoreInfo, \
                                  (GET_IPV4_FF_ID, dst, \
                                   0, dst, \
                                   pDrvCtrl->pDstAddr, ifp->if_addrlen, \
                                   rt0->rt_flags, ifp->if_mtu, \
                                   ifp->if_index, ifp));
                        }
                    }
                }
#endif /* ROUTER_STACK */

            /* If broadcasting on a simplex interface, loopback a copy */

            if ((m->m_flags & M_BCAST) && (ifp->if_flags & IFF_SIMPLEX) &&
		!(m->m_flags & M_PROXY))
                ip_mloopback (ifp, m, (struct sockaddr_in *)dst,
                              (struct rtentry*)rt);

            off = m->m_pkthdr.len - m->m_len;
            etype = ETHERTYPE_IP;
            break;
        case AF_UNSPEC:
            /*
             * WARNING: At the moment this code ONLY handles 14 byte
             * headers of the type like Ethernet.
             */

            switch (ifp->if_type)
                {
                case M2_ifType_ethernet_csmacd:
                case M2_ifType_iso88023_csmacd:
                case M2_ifType_iso88024_tokenBus:
                case M2_ifType_iso88025_tokenRing:
                case M2_ifType_iso88026_man:
                case M2_ifType_fddi:

                    eh = (struct ether_header *)dst->sa_data;
		    bcopy((caddr_t)eh->ether_dhost, 
		          (caddr_t)pDrvCtrl->pDstAddr, ifp->if_addrlen);
                    etype = eh->ether_type;
                    break;

                default:
                    error=ERROR;
		}
             
             if(!error)
                 break;
             /* fall-through */

        default:
            logMsg ("%s%d: can't handle af%d\n", (int)ifp->if_name,
                    ifp->if_unit, dst->sa_family, 0, 0, 0);
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_ERROR event */
            WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_ERROR, 3, 7, 
                            WV_NETEVENT_ETHEROUT_AFNOTSUPP, WV_NET_SEND,
                            dst->sa_family)
#endif  /* INCLUDE_WVNET */
#endif

            senderr(EAFNOSUPPORT);
        }
    
    etype = htons(etype);

    if (pDrvCtrl->nptFlag)
        {
        /*
         * For NPT devices, insert the destination address (if any) and
         * network protocol type at the beginning of the network packet
         * for later use.
         */

        if (ifp->if_addrlen)
            {
            M_PREPEND(m, ifp->if_addrlen, M_DONTWAIT);

            if (m == NULL)
                {
                senderr(ENOBUFS);
                }

            ((M_BLK_ID)m)->mBlkPktHdr.rcvif = 0;

            /* Store the destination address. */

            bcopy (pDrvCtrl->pDstAddr, m->m_data, ifp->if_addrlen);
            }
    
        /* Save the network protocol type. */

        ((M_BLK_ID)m)->mBlkHdr.reserved = etype;
        }
    else
        {
        /*
         * For END devices, build and prepend the complete frame instead.
         * NOTE: currently, the ac_enaddr field is only valid for specific
         * device types. (See the ipAttach routine).
         */

        pDrvCtrl->pSrc->m_data = (char *)&ac->ac_enaddr;
        pDrvCtrl->pDst->mBlkHdr.reserved = etype;
        pDrvCtrl->pSrc->mBlkHdr.reserved = etype;

        m = muxAddressForm (pDrvCtrl->pIpCookie, m,
                            pDrvCtrl->pSrc, pDrvCtrl->pDst);
        if (m == NULL)
            {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
            WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 3, 4, 
                            WV_NETEVENT_ETHEROUT_NOBUFS, WV_NET_SEND, ifp)
#endif  /* INCLUDE_WVNET */
#endif
            senderr(ENOBUFS);
            }
        }

    s = splimp();

    /*
     * Queue message on interface, and start output if interface
     * not yet active.
     */

    if (IF_QFULL(&ifp->if_snd))
        {
        IF_DROP(&ifp->if_snd);
        splx(s);
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_CRITICAL event */
        WV_NET_EVENT_1 (NET_CORE_EVENT, WV_NET_CRITICAL, 3, 4, 
                        WV_NETEVENT_ETHEROUT_NOBUFS, WV_NET_SEND, ifp)
#endif  /* INCLUDE_WVNET */
#endif

        senderr(ENOBUFS);
        }
    IF_ENQUEUE(&ifp->if_snd, m);

    (*ifp->if_start)(ifp);

    splx(s);
    ifp->if_obytes += m->mBlkHdr.mLen;
    if (m->m_flags & M_MCAST || m->m_flags & M_BCAST)
        ifp->if_omcasts++;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_NOTICE event */
    WV_NET_EVENT_2 (NET_CORE_EVENT, WV_NET_NOTICE, 3, 12, 
                    WV_NETEVENT_ETHEROUT_FINISH, WV_NET_SEND, ifp, error)
#endif  /* INCLUDE_WVNET */
#endif

    return (error);
    
    bad:
    if (m)
        netMblkClChainFree(m);
    return (error);

    }

/*******************************************************************************
*
* ipOutputResume - transmit routine called after delayed address resolution
*
* When sending an IP packet, all address resolution functions responsible
* for converting an IP address to a link-layer address receive a pointer to
* this routine as their first argument. If a resolution routine is unable to
* supply the required link-level address immediately from a cache, it may 
* calls this routine after finishing the necessary network transactions (such
* as an ARP request and reply) to resume the transmit process.
*
* RETURNS: results of ipOutput() routine.
*
* INTERNAL
* This callback routine is actually never used. The ipOutput() routine is
* already capable of resuming the transmit process. The arpresolve() routine
* (which is the default resolution routine for this library) calls ipOutput
* directly. This callback routine is only included to comply with the existing
* NPT interface. The prototype for resolution routines should be changed to
* eliminate the callback method.
*
* NOMANUAL
*/

int ipOutputResume
     (
     struct mbuf* pMBuf,
     struct sockaddr* ipDstAddr,
     void* ifp,
     void* rt
     )
     {
     return (
         ipOutput ( (struct ifnet*)ifp,
                   pMBuf, ipDstAddr, (struct rtentry*)rt)
            );
     }
 
/*******************************************************************************
*
* ipMcastResume - internal routine called after delayed mcast address mapping
*
* All IP multicast addresses also correspond to a particular link-level
* address.  The registered address resolution routine is responsible for
* performing that mapping.
*
* All address resolution routines receive a pointer to this routine as their
* first argument when determining a link-level multicast address. If the
* resolution routine is unable to determine the link-level address immediately,
* it calls this routine once the address is available.
*
* INTERNAL
* This callback routine is actually never used, which is fortunate since the
* <rt> parameter is not valid for the ipIoctl routine it executes.
* 
* The multicast conversions for Ethernet addresses are statically mapped, so
* they are always available immediately and the arpresolve() routine (which
* is the default resolution routine for this library) does not require the
* callback argument. This callback routine is only included to comply with
* the existing NPT interface. The prototype for resolution routines should
* be changed to eliminate the callback method.
*
* RETURNS: results of ipIoctl routine
*
* NOMANUAL
*/

LOCAL int ipMcastResume
    (
    struct mbuf* pMbuf,
    struct sockaddr* ipDstAddr,
    void* pIfp,
    void* rt
    )
    {
    struct ifreq ifr;

    bcopy(ipDstAddr->sa_data, ifr.ifr_addr.sa_data, ipDstAddr->sa_len);
    ifr.ifr_addr.sa_family=ipDstAddr->sa_family;

    return(ipIoctl ((IDR *) pIfp, (int) rt, (caddr_t) &ifr));
    }

/****************************************************************************
*
* ipMuxCookieGet - Given an interface index returns the MUX cookie
*
* This routine returns the MUX cookie associated with an interface described
* by the <ifIndex> parameter. <ifIndex> is the interface index.
* This routine is called by the Fastpath module to retrieve the MUX cookie
* needed for sending packets out if Fastpath is done in software.
*
* <ifIndex>	specifies the interface index whose MUX cookie is being
*		retrieved.
*
* RETURNS: The MUX cookie if the interface exists and is bound; NULL otherwise
*
* NOMANUAL
*/

void * ipMuxCookieGet 
    (
    unsigned short 	ifIndex	/* Interface index whose MUX cookie */
    				/* is being retrieved */
    )
    {
    FAST struct ifnet *		ifp;

    if ((ifp = ifIndexToIfp (ifIndex)) == NULL)
        return (NULL);
    return (((IP_DRV_CTRL *)ifp->pCookie)->pIpCookie);
    }

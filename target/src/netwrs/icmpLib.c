/* icmpLib.c - VxWorks library for ICMP routines */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01q,29jan02,vvv  fixed icmpMaskGet to use unfreed mblk when retransmitting
		 (SPR #72867)
01p,15oct01,rae  merge from truestack ver 01t, base 01o (no ether hooks)
01o,17mar99,spm  added support for identical unit numbers (SPR #20913)
01n,17nov98,n_s  fixed icmpMaskGet for non-END devices. spr 23005.
01m,08dec97,gnn  END code review fixes.
01l,05oct97,vin  added header file ip_var.h
01k,03oct97,gnn  removed necessity for endDriver global
01j,25sep97,gnn  SENS beta feedback fixes
01i,12aug97,gnn  changes necessitated by MUX/END update.
01h,20jan97,vin  added icmpLibInit for scaling.
01g,17dec96,gnn  added code to handle the new etherHooks and END stuff.
01f,05aug94,dzb  set IP address of interface in arpcom struct (SPR #2706).
01e,30jun92,jmm  moved checksum() to vxLib
01d,11jun92,elh	 changed parameters to ipHeaderCreate.
01c,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01b,16apr92,elh	 moved routines shared by bootpLib here.
01a,11mar91,elh	 written.
*/

/*
DESCRIPTION
icmpLib contains routines that use ICMP.  icmpMaskGet is currently the only 
routine in this library.  icmpMaskGet generates and sends an ICMP address
mask request to obtain the subnet mask of the network.

The routine icmpLibInit() is responsible for configuring the ICMP protocol
with various parameters.

To use this feature, include the following component:
INCLUDE_ICMP
*/

/* includes */

#include "vxWorks.h"
#include "net/protosw.h"
#include "net/domain.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/in_pcb.h"
#include "netinet/ip_var.h"
#include "netinet/ip.h"
#include "netinet/ip_icmp.h"
#include "netLib.h"
#include "errno.h"
#include "icmpLib.h"
#include "sysLib.h"
#include "string.h"
#include "taskLib.h"
#include "stdio.h"
#include "tickLib.h"
#include "inetLib.h"
#include "vxLib.h"
#include "end.h"
#include "ipProto.h"
#include "muxLib.h"
#include "muxTkLib.h"
#include "private/muxLibP.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif

/* externs */

#ifndef VIRTUAL_STACK
IMPORT int		_protoSwIndex;
IMPORT struct protosw 	inetsw [IP_PROTO_NUM_MAX]; 
IMPORT int		icmpmaskrepl;
#endif

/* defines */

#define ICMP_REXMT_DELAY	1	/* retransmit delay (secs) */
#define ICMP_MAX_SEND		2	/* maximum requests */

/* locals */

#ifndef VIRTUAL_STACK
LOCAL	BOOL	      maskReplyReceived = FALSE; /* recv mask reply 	*/
LOCAL	int	      icmpMask = 0;		/* replied icmp mask    */

LOCAL struct
    {
    struct ip	ih;				/* IP header		*/
    struct icmp	icmph;				/* ICMP header		*/
    } icmpMsg;
#endif

LOCAL BOOL icmpMaskEndInput (void * pCookie, long type, M_BLK_ID pBuff,
			     LL_HDR_INFO * pLinkHdrInfo, void * pSpare);

LOCAL BOOL icmpMaskNptInput (void * callbackId ,long type, M_BLK_ID pBuff,
			     void * pSpareData);

/* forward declarations */

IMPORT int in_broadcast ();

/******************************************************************************
 *
 * icmpLibInit - initialize icmpLib 
 *
 * This routine initializes icmpLib.
 *
 * RETURNS: OK if successful, otherwise ERROR.
 *
 * NOMANUAL
 */

STATUS icmpLibInit 
    (
    ICMP_CFG_PARAMS * 	icmpCfg		/* icmp configuration parameters */
    )
    {
    FAST struct protosw	* pProtoSwitch; 

    if (_protoSwIndex >= sizeof(inetsw)/sizeof(inetsw[0]))
	return (ERROR) ;

    pProtoSwitch = &inetsw [_protoSwIndex]; 

    if (pProtoSwitch->pr_domain != NULL)
	return (OK); 				/* already initialized */

    pProtoSwitch->pr_type   	=  SOCK_RAW;
    pProtoSwitch->pr_domain   	=  &inetdomain;
    pProtoSwitch->pr_protocol   =  IPPROTO_ICMP;
    pProtoSwitch->pr_flags	=  PR_ATOMIC | PR_ADDR;
    pProtoSwitch->pr_input	=  icmp_input;
    pProtoSwitch->pr_output	=  rip_output;
    pProtoSwitch->pr_ctlinput	=  0;
    pProtoSwitch->pr_ctloutput	=  rip_ctloutput;
    pProtoSwitch->pr_usrreq	=  rip_usrreq;
    pProtoSwitch->pr_init	=  icmp_init;
    pProtoSwitch->pr_fasttimo	=  0;
    pProtoSwitch->pr_slowtimo	=  0;
    pProtoSwitch->pr_drain	=  0;
    pProtoSwitch->pr_sysctl	=  0;

    _protoSwIndex++; 

    /* initialize icmp configuration parameters */

    icmpmaskrepl = (icmpCfg->icmpCfgFlags & ICMP_DO_MASK_REPLY) ? TRUE : FALSE;

    return (OK); 
    }

/******************************************************************************
*
* icmpMaskGet - obtain the subnet mask 
*
* icmpMask broadcasts an ICMP Address Mask Request over the network
* interface specified by <ifName> to obtain the subnet mask of that
* network.
* This interface must have been previously attached and initialized.
*
* <src> specifies the source IP address, which must be set.
*
* <dst> specifies destination of ICMP request.  A NULL value for
* <dst> results in the request being broadcasted.  However, because
* ICMP mask request/reply behaves differently on each ICMP
* implementation, thus the ICMP mask reply by broadcasting ICMP mask
* request is not guranteed.
*
* The subnet mask gets returned in <pSubnet> in host byte order.
*
* NOTE: This routine can be used for END or NPT driver only.
*
* RETURNS: OK if successful, otherwise ERROR.
*
* ERRNO
*   S_icmpLib_NO_BROADCAST
*   S_icmpLib_INVALID_INTERFACE
*   S_icmpLib_TIMEOUT
*
*/

STATUS icmpMaskGet
    (
    char *		ifName,		/* network interface name */
    char * 		src,		/* optional src address */
    char *		dst,		/* optional dst address */
    int	 *		pSubnet		/* return subnet mask 	*/
    )
    {
    FAST int 		retransmitSecs;	/* retransmit time 	*/
    FAST int		tickCount;
    int			ix;		/* index 		*/
    struct in_addr	srcAddr;	/* source address 	*/
    struct sockaddr_in	dstAddr;	/* destination address	*/
    struct ifnet *	pIf;		/* pointer to interface */
    IP_DRV_CTRL *	pDrvCtrl;
    struct mbuf *	pMbuf;
    void *		pCookie = NULL;
    FUNCPTR		pBoundRtn = NULL;
    int 		level;
    int 		result;

    /* reset flags */

    maskReplyReceived = FALSE;
    icmpMask = 0;

    /* verify arguments and the I/F if attached END/NPT */

    if (pSubnet == NULL                                      ||
        (pIf = ifunit (ifName)) == NULL                      ||
	pIf->if_output == NULL                               ||
	(endFindByName (pIf->if_name, pIf->if_unit)) == NULL ||
	(pDrvCtrl = (IP_DRV_CTRL *)pIf->pCookie) == NULL)
	{
	errno = S_icmpLib_INVALID_INTERFACE;
	goto icmpMaskGetError;
	}


    /* build IP address */

    if (src == NULL)
        {
        errno = S_icmpLib_INVALID_ARGUMENT;
	goto icmpMaskGetError;
        }

    /* bind IP address to ac */

    srcAddr.s_addr = inet_addr (src);
    ((struct arpcom *) pIf)->ac_ipaddr.s_addr = srcAddr.s_addr;

    bzero ((char *)&dstAddr, sizeof (struct sockaddr_in));

    if (dst == NULL)
	{
        if ((pIf->if_flags & IFF_BROADCAST) == 0)
	    {
	    errno = S_icmpLib_NO_BROADCAST;	/* no broadcasts */
	    goto icmpMaskGetError;
	    }
	dstAddr.sin_addr.s_addr = htonl (INADDR_BROADCAST);
	}
    else
	dstAddr.sin_addr.s_addr = inet_addr (dst);

    dstAddr.sin_len = sizeof (struct sockaddr_in);
    dstAddr.sin_family = AF_INET;

    pIf->if_flags |= (IFF_UP | IFF_RUNNING);

    retransmitSecs	= ICMP_REXMT_DELAY; 	/* set delay value */
    maskReplyReceived	= FALSE;


    /* fill in ICMP message and put it in an mbuf */

    bzero ((char *) &icmpMsg, sizeof (icmpMsg));
    icmpMsg.icmph.icmp_type  = ICMP_MASKREQ;
    icmpMsg.icmph.icmp_code  = 0;
    icmpMsg.icmph.icmp_cksum = 0;
    icmpMsg.icmph.icmp_cksum = checksum ((u_short *) &icmpMsg.icmph, 
					 ICMP_MASKLEN);

    ipHeaderCreate (IPPROTO_ICMP, &srcAddr, &dstAddr.sin_addr,
		    &icmpMsg.ih, sizeof (struct ip) + ICMP_MASKLEN);


    /* bind icmpMaskHook as SNARF */

    if (muxTkDrvCheck (pIf->if_name))
	{
	pCookie = muxTkBind (pIf->if_name, pIf->if_unit,
			     (FUNCPTR)icmpMaskNptInput, NULL, NULL, NULL,
			     MUX_PROTO_SNARF, "ICMP HOOK NPT",
			     pDrvCtrl, NULL, NULL);
	pBoundRtn = (FUNCPTR) icmpMaskNptInput;
	}
    else
	{
	pCookie = muxBind (pIf->if_name, pIf->if_unit,
			   (FUNCPTR)icmpMaskEndInput, NULL, NULL, NULL,
			   MUX_PROTO_SNARF, "ICMP HOOK END",
			   pDrvCtrl);
	pBoundRtn = (FUNCPTR) icmpMaskEndInput;
	}

    if (pCookie == NULL)
	goto icmpMaskGetError;


    for (ix = 0; ix < ICMP_MAX_SEND;  ix++)
	{
        if ((pMbuf = bcopy_to_mbufs ((u_char *) &icmpMsg, sizeof (icmpMsg),
				     0, pIf, NONE)) == NULL)
            goto icmpMaskGetError;

        if (dst == NULL)   			/* if broadcasting */
	    pMbuf->mBlkHdr.mFlags |= M_BCAST;


	/* send message */

        level = splnet ();
	result = (* pIf->if_output) ( (struct ifnet *)&pDrvCtrl->idr, pMbuf,
			             (struct sockaddr *)&dstAddr, NULL);
        splx (level);

	if (result)
	    goto icmpMaskGetError;

	/* wait for reply */

	tickCount = retransmitSecs * sysClkRateGet ();

	while (tickCount-- > 0)
	    {
	    if (maskReplyReceived)
		{
		*pSubnet = icmpMask;
		muxUnbind (pCookie, MUX_PROTO_SNARF, pBoundRtn);
		return (OK);
		}

	    taskDelay (1);
	    }
	}

    errno = S_icmpLib_TIMEOUT;

    icmpMaskGetError:
        if (pCookie != NULL && pBoundRtn != NULL)
	    muxUnbind (pCookie, MUX_PROTO_SNARF, pBoundRtn);
    return (ERROR);				/* no subnet */
    }

/******************************************************************************
*
* icmpMaskEndInput - END version of input filter for ICMP Mask Reply
*
* This routine filters out the ICMP Address Mask Reply message from
* incoming link frame.  It is bound to MUX by muxBind as a SNARF
* protocol.
*
* RETURNS:
* TRUE indicating the packet was consumed by this routine and no further
* processing needs to be done.
* FALSE indicating the packet was not consumed by this routine and
* further processing needs to be done.
*/

LOCAL BOOL icmpMaskEndInput
    (
    void *              pCookie,        /* device identifier from driver */
    long                type,           /* Protocol type.  */
    M_BLK_ID            pMblk,          /* The whole packet. */
    LL_HDR_INFO *       pLinkHdrInfo,   /* pointer to link level header info */
    void *              pSpare          /* pointer to IP_DRV_CTRL */
    )
    {
    IP_DRV_CTRL *	pDrvCtrl = (IP_DRV_CTRL *) pSpare;
    struct ifnet *      pIf = NULL;
    struct icmp *	pIcmp;
    M_BLK_ID            pMblkOrig;
    M_BLK		tmpM;


    /* already got a reply, or unknown device */

    if (maskReplyReceived || pDrvCtrl == NULL || pMblk == NULL ||
	(pIf = (struct ifnet *)&pDrvCtrl->idr) == NULL)
	return (FALSE);

    if ((pIf->if_flags & IFF_UP) == 0)
        {
        pIf->if_ierrors++;
        return (FALSE);
        }

    /* Make sure the entire Link Hdr is in the first M_BLK */
 
    pMblkOrig = pMblk;
    if (pMblk->mBlkHdr.mLen < pLinkHdrInfo->dataOffset
        && (pMblk = m_pullup (pMblk, pLinkHdrInfo->dataOffset)) == NULL)
        {
        pMblk = pMblkOrig;
        return (FALSE);
	}
 
    /* point to the ip header, and adjust the length */

    bzero ((char *)&tmpM, sizeof (M_BLK));
    bcopy ((char *)pMblk, (char *)&tmpM, sizeof (M_BLK));
    tmpM.mBlkHdr.mData        += pLinkHdrInfo->dataOffset;
    tmpM.mBlkHdr.mLen         -= pLinkHdrInfo->dataOffset;
    tmpM.mBlkPktHdr.len       -= pLinkHdrInfo->dataOffset;
    tmpM.mBlkPktHdr.rcvif     = &pDrvCtrl->idr.ac_if;
 
    /* get and verify icmp */

    pIcmp = (struct icmp *)
	    ipHeaderVerify ((struct ip *) tmpM.mBlkHdr.mData,
			    tmpM.mBlkHdr.mLen, IPPROTO_ICMP);

    if ((pIcmp == NULL) ||
        (checksum ((u_short *) pIcmp, ICMP_MASKLEN) != 0) ||
        (pIcmp->icmp_type != ICMP_MASKREPLY))
        return (FALSE);

    pIf->if_ipackets++;          /* bump statistic */
    pIf->if_lastchange = tickGet();
    pIf->if_ibytes += pMblk->mBlkPktHdr.len;
  
    maskReplyReceived = TRUE;
    icmpMask = ntohl (pIcmp->icmp_mask);
    netMblkClChainFree (pMblk);
    return (TRUE);
    }

/******************************************************************************
*
* icmpMaskNptInput - NPT version of input filter for ICMP Mask Reply
*
* This routine filters out the ICMP Address Mask Reply message from
* incoming link frame.  It is bound to MUX by muxBind as a SNARF
* protocol.
*
* RETURNS:
* TRUE indicating the packet was consumed by this routine and no further
* processing needs to be done.
* FALSE indicating the packet was not consumed by this routine and
* further processing needs to be done.
*/

LOCAL BOOL icmpMaskNptInput
    (
    void *    ipCallbackId,  /* Sent down in muxTkBind call. */
    long      type,          /* Protocol type.  */
    M_BLK_ID  pMblk,         /* The whole packet. */
    void *    pSpareData     /* out of band data */
    )
    {
    IP_DRV_CTRL *	pDrvCtrl = (IP_DRV_CTRL *)ipCallbackId;
    struct ifnet *	pIf = NULL;
    struct icmp *	pIcmp;
    M_BLK_ID            pMblkOrig;
    M_BLK		tmpM;


    /* already got a reply, or unknown device */

    if (maskReplyReceived || pDrvCtrl == NULL || pMblk == NULL ||
	(pIf = (struct ifnet *)&pDrvCtrl->idr) == NULL)
	return (FALSE);

    if ((pIf->if_flags & IFF_UP) == 0)
        {
        pIf->if_ierrors++;
        return (FALSE);
        }


    /* Make sure the entire interface header is in the first M_BLK */
 
    pMblkOrig = pMblk;
    if (pMblk->mBlkHdr.mLen < pIf->if_hdrlen
        && (pMblk = m_pullup (pMblk, pIf->if_hdrlen)) == NULL)
        {
        pMblk = pMblkOrig;
        return (FALSE);
	}
 

    /* point to the network service header, and adjust the length */

    bzero ((char *)&tmpM, sizeof (M_BLK));
    bcopy ((char *)pMblk, (char *)&tmpM, sizeof (M_BLK));
    tmpM.mBlkHdr.mData        += pIf->if_hdrlen;
    tmpM.mBlkHdr.mLen         -= pIf->if_hdrlen;
    tmpM.mBlkPktHdr.len       -= pIf->if_hdrlen;
    tmpM.mBlkPktHdr.rcvif     = &pDrvCtrl->idr.ac_if;


    /* get and verify icmp */

    pIcmp = (struct icmp *)
	    ipHeaderVerify ((struct ip *) tmpM.mBlkHdr.mData,
			    tmpM.mBlkHdr.mLen, IPPROTO_ICMP);

    if (pIcmp == NULL ||
        (checksum ((u_short *) pIcmp, ICMP_MASKLEN) != 0) ||
        (pIcmp->icmp_type != ICMP_MASKREPLY))
        return (FALSE);

    pIf->if_ipackets++;          /* bump statistic */
    pIf->if_lastchange = tickGet();
    pIf->if_ibytes += pMblk->mBlkPktHdr.len;
  
    maskReplyReceived = TRUE; 
    icmpMask = ntohl (pIcmp->icmp_mask);
    netMblkClChainFree (pMblk);
    return (TRUE);
    }

/******************************************************************************
*
* ipHeaderCreate - create a simple IP header
*
* This generates a simple IP header (with no options) and checksums it.
* <proto> specifies the protocol type. <pSrcAddr> and <pDstAddr> define
* the source and destination internet addresses, respectively.  Both should
* be specified in network byte order.  <pih> is a pointer to an internet
* datagram whose length is <length>.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void ipHeaderCreate
    (
    int			proto,		/* protocol number	*/
    struct in_addr *	pSrcAddr,	/* source ip address 	*/
    struct in_addr *	pDstAddr,	/* dest ip address	*/
    struct ip *		pih,		/* internet header 	*/
    int			length		/* datagram size 	*/
    )

    {
    					/* fill in the IP header */
    pih->ip_v = IPVERSION;
    pih->ip_hl = (sizeof (struct ip) >> 2) & 0xf;  
    pih->ip_len	= htons ((u_short) length);
    pih->ip_id	= (u_short) (tickGet () & 0xffff);
    pih->ip_ttl	= MAXTTL;
    pih->ip_p	= (u_char) proto;
    pih->ip_src.s_addr	= pSrcAddr->s_addr;
    pih->ip_dst.s_addr	= pDstAddr->s_addr;

    pih->ip_sum	= 0;	/* zero out the checksum while computing it */
    pih->ip_sum = checksum ((u_short *) pih, (pih->ip_hl << 2));
    }

/*******************************************************************************
*
* ipHeaderVerify - simple IP header verification
*
* ipHeaderVerify performs simple IP header verification.  It does
* sanity checks and verifies the checksum.  <pih> points to an internet
* datagram of length <length>.  <proto> is the expected protocol.
*
* RETURNS: a pointer to the IP data, or NULL if not a valid IP datagram.
*
* NOMANUAL
*/

u_char * ipHeaderVerify
    (
    struct ip *		pih, 		/* internet header	*/
    int			length,		/* length of datagram 	*/
    int			proto  		/* protocol */
    )

    {
    FAST int		options;	/* options offset 	*/

    /* if not minimum size, or right protocol bail */

    if ((length < sizeof (struct ip)) || (pih->ip_p != (u_char) proto))
	return (NULL);
					/* verify checksum */
    if (checksum ((u_short *) pih, (pih->ip_hl << 2)) != 0)
	return (NULL);

    if ((pih->ip_v != IPVERSION) ||
	(ntohs ((u_short) pih->ip_off) & 0x3FFF))
	return (NULL);
					/* ip_hl is in words */
    options = (pih->ip_hl << 2) - sizeof (struct ip);

    return ((u_char *) pih + sizeof (struct ip) + options);
    }

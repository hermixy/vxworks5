/* proxyLib.c - proxy Address Resolution Protocol (ARP) client library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
/*
modification history
--------------------
01k,10may02,kbw  making man page edits
01j,15oct01,rae  merge from truestack ver 01k, base 01f (no ether hooks)
01i,07feb01,spm  added merge record for 30jan01 update from version 01g of
                 tor2_0_x branch (base 01f) and fixed modification history
01h,30jan01,ijm  corrected parameters for etherInputHookDelete
01g,30jan01,ijm  merged SPR #28602 fixes: proxy ARP services are obsolete
01f,26aug97,spm  removed compiler warnings (SPR #7866)
01e,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
01d,22sep92,jdi  documentation cleanup.
01c,15jun92,elh  changed parameters to proxyReg & proxyUnreg,
		 renamed to proxyLib, general cleanup.
01b,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01a,20sep91,elh	  written.
*/

/*
DESCRIPTION
This library implements the client side of the proxy Address Resolution
Protocol (ARP).  It allows a VxWorks target to register itself as a proxy
client by calling proxyReg() and to unregister itself by calling
proxyUnreg().

Both commands take an interface name and an IP address as arguments.  
The interface, <ifName>, specifies the interface through which to send the
message.  <ifName> must be a backplane interface.  <proxyAddr> is the
IP address associated with the interface <ifName>.

To use this feature, include INCLUDE_PROXY_CLIENT.

INCLUDE FILES: proxyArpLib.h

SEE ALSO: proxyArpLib

INTERNAL
This registration process is pretty simple.  Basically the client broadcasts
a PROXY_MSG message on the proxy network.  The client retries sending
the message until it either times out or it receives an ACK from the server.
There are currently three types of messages: PROXY_REG, to add the client
as a proxy client, PROXY_UNREG to delete the client as a proxy client,
and a PROXY_PROBE message, to probe the server.


    proxyReg 	proxyUnreg			v
	|	  |		 proxyAckEndRecv/proxyAckNptRecv
	v	  v                             |                        
     proxyMsgSend                               v
                                           proxyAckCheck
*/

/* includes */

#include "vxWorks.h"
#include "proxyArpLib.h"
#include "netinet/in.h"
#include "sys/socket.h"
#include "net/if_arp.h"
#include "net/if.h"
#include "netinet/if_ether.h"
#include "errno.h"
#include "arpLib.h"
#include "stdio.h"
#include "string.h"
#include "sysLib.h"
#include "inetLib.h"
#include "muxLib.h"
#include "taskLib.h"
#include "muxLib.h"
#include "muxTkLib.h"
#include "ipProto.h"

/* globals */

int			proxyXmitMax 	 = XMIT_MAX;	/* max rexmits      */
int			proxyVerbose     = FALSE;	/* debug messages   */

/* locals */

LOCAL BOOL		proxyAckReceived = FALSE;	/* got ack	    */
LOCAL PROXY_MSG 	proxyMsg;			/* proxy message    */

/* forward declarations */

STATUS proxyMsgSend (char * ifName, char * proxyAddr, int op);
LOCAL BOOL proxyAckCheck (M_BLK_ID pMblk);
LOCAL BOOL proxyAckEndRecv (void * pCookie, long type, M_BLK_ID pBuff,
			    LL_HDR_INFO * pLinkHdrInfo, void * pSpare);
LOCAL BOOL proxyAckNptRecv (void * callbackId ,long type, M_BLK_ID pBuff,
			    void * pSpareData);

/* imports */

IMPORT struct ifnet * ifunit ();

/*******************************************************************************
*
* proxyReg - register a proxy client
*
* This routine sends a message over the network interface <ifName> to register
* <proxyAddr> as a proxy client. 
*
* RETURNS: OK, or ERROR if unsuccessful.
*/

STATUS proxyReg
    (
    char *			ifName,		/* interface name */
    char *  			proxyAddr	/* proxy address  */
    )

    {
    if (ifName == NULL || proxyAddr == NULL)
	{
	errno = S_proxyArpLib_INVALID_PARAMETER;	
	return (ERROR);
	}
    if (proxyMsgSend (ifName, proxyAddr, PROXY_REG) == ERROR)
	return (ERROR);

    arpFlush ();
    return (OK);
    }

/*******************************************************************************
*
* proxyUnreg - unregister a proxy client
*
* This routine sends a  message over the network interface <ifName> to
* unregister <proxyAddr> as a proxy client.
*
* RETURNS: OK, or ERROR if unsuccessful.
*/

STATUS proxyUnreg
    (
    char *			ifName,		/* interface name */
    char * 			proxyAddr	/* proxy address  */
    )

    {
    if (ifName == NULL || proxyAddr == NULL)
	{
	errno = S_proxyArpLib_INVALID_PARAMETER;
	return (ERROR);
	}
    if (proxyMsgSend (ifName, proxyAddr, PROXY_UNREG) == ERROR)
	return (ERROR);

    arpFlush ();
    return (OK);
    }

/*******************************************************************************
*
* proxyMsgSend - send a proxy message
*
* This routine creates a proxy message with operation type <op>. It then
* broadcasts this message over the proxy interface identified by <proxyAddr>.
*
* NOMANUAL
*
* RETURNS: OK, or ERROR if unsuccessful.
*
* ERRNO
*   S_proxyArpLib_INVALID_PARAMETER
*   S_proxyArpLib_INVALID_INTERFACE
*   S_proxyArpLib_INVALID_ADDRESS
*   S_proxyArpLib_TIMEOUT
*
* INTERNAL
* This routine is global but no manual because it is useful for debugging
* purposes.
*/

STATUS proxyMsgSend
    (
    char *			ifName,		/* interface name */
    char * 			proxyAddr,	/* proxy interface addr */
    int				op 		/* operation		*/
    )

    {
    struct ether_header 	eh; 		/* ethernet header	*/
    int				ix;		/* index variable	*/
    int				delay;		/* xmit delay value 	*/
    struct ifnet *		pIf;
    struct mbuf *		pMbuf;
    struct sockaddr		dst;
    IP_DRV_CTRL *		pDrvCtrl;
    void *			pCookie;
    FUNCPTR			pBoundRtn = NULL;
    int 			level;
    int 			result;

    if (op < PROXY_PROBE || op >= PROXY_ACK)
	{
	errno = S_proxyArpLib_INVALID_PARAMETER;
	return (ERROR);				/* invalid op */
	}

    if ((pIf = ifunit (ifName)) == NULL      ||
	(pIf->if_output == NULL)             ||
	(pIf->if_flags & IFF_BROADCAST) == 0 ||
	(pDrvCtrl = (IP_DRV_CTRL *)pIf->pCookie) == NULL)
	{
	errno = S_proxyArpLib_INVALID_INTERFACE;
	return (ERROR);                 	/* interface not attached */
	}

    /* fill in proxy message */

    bzero ((caddr_t ) &proxyMsg, sizeof (proxyMsg));
    proxyMsg.op = htonl (op);
    if ((proxyMsg.clientAddr.s_addr = inet_addr (proxyAddr)) == ERROR)
	{
	errno = S_proxyArpLib_INVALID_ADDRESS;
	return (ERROR);
	}
    bcopy ((caddr_t) ((struct arpcom *)pIf)->ac_enaddr,
	   (caddr_t) proxyMsg.clientHwAddr, sizeof (proxyMsg.clientHwAddr));

    /* fill in ethernet header */

    bzero ((caddr_t) &eh, sizeof (eh));
    eh.ether_type = PROXY_TYPE;		/* htons is done in ipOutput */
    bcopy ((char *) etherbroadcastaddr, (char *) eh.ether_dhost,
	   sizeof (etherbroadcastaddr));

    proxyAckReceived = FALSE;

    bzero ((caddr_t)&dst, sizeof (dst));
    dst.sa_len = sizeof (dst);
    dst.sa_family = AF_UNSPEC;
    bcopy((caddr_t)&eh, (caddr_t)&dst.sa_data, sizeof(eh));

    /* bind proxyAckInput as SNARF */

    if (muxTkDrvCheck (pIf->if_name))
	{
	pCookie = muxTkBind (pIf->if_name, pIf->if_unit,
			     (FUNCPTR)proxyAckNptRecv, NULL, NULL, NULL,
			     MUX_PROTO_SNARF, "PROXY ACK NPT",
			     pDrvCtrl, NULL, NULL);
	pBoundRtn = (FUNCPTR)proxyAckNptRecv;
	}
    else
	{
	pCookie = muxBind (pIf->if_name, pIf->if_unit,
			   (FUNCPTR)proxyAckEndRecv, NULL, NULL, NULL,
			   MUX_PROTO_SNARF, "PROXY ACK END",
			   pDrvCtrl);
	pBoundRtn = (FUNCPTR)proxyAckEndRecv;
	}


    if (proxyVerbose)
    	printf ("sending <%d> client %s\n", op, proxyAddr);

    for (ix = 0; ix < proxyXmitMax; ix++)
        {
	printf (".");
        /* form an ARP packet */

        if ((pMbuf = bcopy_to_mbufs ((caddr_t)&proxyMsg, sizeof (PROXY_MSG),
				 0, pIf, NONE)) == NULL)
            return (ERROR);

        level = splnet ();
	
        result = pIf->if_output (pIf, pMbuf, &dst, NULL);

        splx (level);

	if (result)
	    break;

	delay = sysClkRateGet () * XMIT_DELAY;		/* rexmit delay */
	while (delay-- > 0)
	    {
	    if (proxyAckReceived)
		{
		muxUnbind (pCookie, MUX_PROTO_SNARF, pBoundRtn);
		return (OK);
		}

	    taskDelay (1);
	    }
	}

    if (ix == proxyXmitMax)
       errno = S_proxyArpLib_TIMEOUT;

    muxUnbind (pCookie, MUX_PROTO_SNARF, pBoundRtn);
    return (ERROR);
    }

/*******************************************************************************
*
* proxyAckCheck - check proxy ack
* 
* This routine checks given link frame if it is an expected proxy arp ACK.
* If it is the case, this routine consumed the pMblk and frees it.
*
* RETURNS:TRUE if received an expected arp ACK, or FALSE
*/

LOCAL BOOL proxyAckCheck
    (
    M_BLK_ID	pMblk				/* incoming link frame */
    )
    {
    struct ether_header *  eh;           	/* ethernet header      */
    PROXY_MSG *		   pMsg;		/* pointer to message	*/
    char		   inputBuffer [2176];  /* frame buffer 	*/
    UINT	           length;		/* packet length	*/


    /* check proxy message type in ethernet header */

    eh = (struct ether_header *)pMblk->mBlkHdr.mData;
    length = pMblk->mBlkHdr.mLen;

    if (length <= SIZEOF_ETHERHEADER ||
	(ntohs (eh->ether_type)) != PROXY_TYPE)
        return (FALSE);


    /* get proxy arp message */
     
    bzero (inputBuffer, sizeof (inputBuffer));
    bcopyBytes ((caddr_t)((u_char *)pMblk->mBlkHdr.mData + SIZEOF_ETHERHEADER),
                inputBuffer, length - SIZEOF_ETHERHEADER);

    pMsg = (PROXY_MSG *)inputBuffer;


    /*  check to make sure I sent out the original message  */

    if ((ntohl (pMsg->op) == PROXY_ACK) &&
        (pMsg->clientAddr.s_addr == proxyMsg.clientAddr.s_addr) &&
        (bcmp ((caddr_t) pMsg->clientHwAddr, (caddr_t) proxyMsg.clientHwAddr,
               sizeof (pMsg->clientHwAddr)) == 0))
        {
        proxyAckReceived = TRUE;
        netMblkClChainFree (pMblk);
        return (TRUE);
        }

    return (FALSE);
    }

/*******************************************************************************
*
* proxyAckEndRecv - End version of a proxy ACK receive routine
*
* This module is the SNARF input routine used to receive the ACK
* for a previous operation sent to the proxy server via proxyMsgSend.
*
* RETURNS: See proxyAckCheck().
*/

LOCAL BOOL proxyAckEndRecv
    (
    void *              pCookie,        /* device identifier from driver */
    long                type,           /* Protocol type.  */
    M_BLK_ID            pMblk,          /* The whole packet. */
    LL_HDR_INFO *       pLinkHdrInfo,   /* pointer to link level header info */
    void *              pSpare          /* pointer to IP_DRV_CTRL */
    )
    {
    /* already got a reply, or unknown device */

    if (proxyAckReceived || pMblk == NULL)
	return (FALSE);

    return (proxyAckCheck (pMblk));
    }

/*******************************************************************************
*
* proxyAckNptRecv - NPT version of a proxy ACK receive routine
*
* This module is the SNARF input routine used to receive the ACK
* for a previous operation sent to the proxy server via proxyMsgSend.
*
* RETURNS: See proxyAckCheck().
*/

LOCAL BOOL proxyAckNptRecv
    (
    void *    ipCallbackId,  /* Sent down in muxTkBind call. */
    long      type,          /* Protocol type.  */
    M_BLK_ID  pMblk,         /* The whole packet. */
    void *    pSpareData     /* out of band data */
    )
    {
    if (proxyAckReceived || pMblk == NULL)
        return (FALSE);

    return (proxyAckCheck (pMblk));
    }

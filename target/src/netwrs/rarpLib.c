/* rarpLib.c - Reverse Address Resolution Protocol (RARP) client library */

/* Copyright 1999 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)if_ether.c  8.2 (Berkeley) 9/26/94
 */

/*
modification history
--------------------
01i,17dec01,vvv  fixed doc build errors
01h,05nov01,vvv  fixed compilation warnings
01g,29mar01,spm  file creation: copied from version 01f of tor2_0.open_stack
                 branch (wpwr VOB) for unified code base; fixed published
                 routine name for coding conventions
01f,29jun00,spm  updated to handle NPT version of MUX interface
01e,11may00,brx  fixed error returns to set errno and return ERROR
01d,11may00,ead  added calls to virtualStackIdCheck() in the various RARP
                 commands
01c,25apr00,spm  made virtual stack support optional (enabled by default)
01b,18apr00,ead  modified to be multi-instance
01a,28dec99,brx  created from RARP code found in if_ether.c

*/

/*
DESCRIPTION
This library is an implementation of the Reverse Address Resolution Protocol.
This protocol allows devices such as diskless workstations request an IP 
address at boot-time from a dedicated server on the local network.

The routine rarpGet() broadcasts a RARP request on the local ethernet and
returns a response.  If a RARP server does not respond after the specified
timeout interval, rarpGet() returns with an error code set in errno.
 
*/


#include "vxWorks.h"
#include "stdio.h"
#include "taskLib.h"
#include "end.h"
#include "muxLib.h"
#include "ipProto.h"
#include "sysLib.h"
#include "logLib.h"
#include "errnoLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif /* VIRTUAL_STACK */

/* defines */
#define SAMPLE_RATE 6

/* define ERROR codes */
#define S_rarpLib_TIMEOUT           (M_rarpLib | 1)
#define S_rarpLib_NETDOWN           (M_rarpLib | 2)
#define S_rarpLib_INPROGRESS        (M_rarpLib | 3)
#define S_rarpLib_NORESPONSE        (M_rarpLib | 4)


/* typedefs */

/* externs */

/* locals */

#ifndef VIRTUAL_STACK
LOCAL SEM_ID	revarpInProgress = NULL;
#endif /* VIRTUAL_STACK */

/* revarp state */

#ifndef VIRTUAL_STACK
typedef struct
    {
    BOOL		ipAddrInitialized; 
    struct in_addr 	ipAddr;
    END_OBJ		*pEndId;
    BOOL		rarpDebug;
    } RARPSTATE;

LOCAL RARPSTATE rarpState;
#endif /* VIRTUAL_STACK */

/* forward declarations */

void rarpSetDebug( int );
int rarpGet ( char *, int, struct in_addr *, int);		
void rarpIn ( END_OBJ *, long, M_BLK_ID, LL_HDR_INFO *, IP_DRV_CTRL *);
int revarpwhoami(struct in_addr *, struct ifnet *, int);
void revarprequest(struct ifnet *);
void in_revarpinput(struct ether_arp *);

/******************************************************************************
*
* rarpLibInit - Initialize rarp client
*
* This routine links the RARP facility into the VxWorks system.
* These routines are included automatically if INCLUDE_RARP is defined.
* 
*
* Returns: N/A
*
*/

STATUS rarpLibInit(void)
    {
#ifndef VIRTUAL_STACK
    if (revarpInProgress == NULL){
#endif /* VIRTUAL_STACK */
        revarpInProgress = semBCreate(SEM_Q_PRIORITY, SEM_FULL);
        if (revarpInProgress == NULL)
    	    return ERROR;
        return OK;
#ifndef VIRTUAL_STACK
    }
    else
        return OK;
#endif /* VIRTUAL_STACK */

#ifdef VIRTUAL_STACK
    rarpState.ipAddrInitialized = FALSE;
    rarpState.ipAddr.s_addr = 0;
    rarpState.pEndId = 0;
    rarpState.rarpDebug = FALSE;
    return OK;
#endif /* VIRTUAL_STACK */
    }

/******************************************************************************
*
* rarpDebugSet - Set the debug facility in RARP
*
* This routine sets the RARP debug facility.  If the argument is non-zero, 
* debug messages will be sent to the logging task.  If the argument is zero
* RARP operates quietly.  Default is quiet operation.
*
* Returns: N/A
*
*/

void rarpDebugSet
    (
    int flag
    )
    {
#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
#endif /* VIRTUAL_STACK */

    logMsg("Setting rarpDebug to %d\n", flag, 0, 0, 0, 0, 0);
    rarpState.rarpDebug = (flag != 0) ? TRUE : FALSE;
    }



/*******************************************************************************
*
* rarpGet - broadcast a RARP packet and wait for a reply
*
* rarpGet() maps ethernet hardware addresses to IP protocol addresses.
* This routine generates an ethertype 0x8035 packet and 
* broadcasts it on the device local network.  If a RARP server is on the 
* network, it replies with an IP address that corresponds to the device 
* hardware address. 
*
* .IP <IFname>
* Expects a pointer to the character string which contains the name of the 
* interface.
* .IP <IFunit>
* Expects a number which is the unit of the device named in IFname.
* .IP <Addr> 
* Expects a pointer to a struct in_addr. The IP address of the device will
* be placed in this buffer after it is received from the RARP server.
* .IP <Tmo>
* Expects an integer for timeout value in seconds.
*
* Returns: OK, S_rarpLib_NETDOWN if IFname is not a valid device, 
*          S_rarpLib_INPROGRESS if another RARP request is in progress, 
*          S_rarpLib_NORESPONSE if there is no response from a server.
*/

int rarpGet
    (
    char *IFname, 
    int IFunit, 
    struct in_addr *Addr, 
    int tmo
    )

    {
    struct ifnet *pIf;
    END_OBJ * pEnd;
    void *pCookie;
    int errorFlag = 0;
    char cName[16];

#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
#endif /* VIRTUAL_STACK */

    semTake(revarpInProgress, WAIT_FOREVER);

    rarpState.ipAddrInitialized = FALSE;
    rarpState.ipAddr.s_addr = 0;

    pEnd = endFindByName (IFname, IFunit);

    sprintf(cName,"%s%d", IFname, IFunit);
    pIf = (struct ifnet *)ifunit(cName);
    if (pIf == NULL || pEnd == NULL) 
        {
	if (rarpState.rarpDebug)
            logMsg ("RARP: Can't find interface %s!\n", (int)cName,
                    0, 0, 0, 0, 0);
	semGive(revarpInProgress);
        errnoSet(S_rarpLib_NETDOWN);
	return ERROR;
        }

    if ((pCookie = muxBind(IFname, IFunit, (FUNCPTR)rarpIn, NULL, 
	NULL, NULL, 0x8035, "IP 4.4 RARP", NULL )) == NULL)
        {
	 if (rarpState.rarpDebug)
              logMsg("RARP: Can't bind to MUX\n",0,0,0,0,0,0);
	 semGive(revarpInProgress);
         errnoSet(S_rarpLib_NETDOWN);
	 return ERROR;
        }
    rarpState.pEndId = pEnd;

    /* broadcast  RARP request, wait for answer */
    errorFlag = revarpwhoami(Addr, pIf, tmo); 

    if( muxUnbind( pCookie, 0x8035, (FUNCPTR)rarpIn) != OK)
	if (rarpState.rarpDebug)
	    logMsg("RARP: Error in MUX unbind\n",0,0,0,0,0,0);

    semGive(revarpInProgress);

    if (errorFlag != 0)
        {
	if (rarpState.rarpDebug)
	    logMsg("RARP: no response\n",0,0,0,0,0,0);
        errnoSet(errorFlag);
	return ERROR;	
        }
    else 
    return OK;

    }

/*******************************************************************************
*
* revarpwhoami - issues RARP request and waits for a reply from a server
*
*
* RETURNS: OK if the IP has been received, S_rarpLib_NORESPONSE if no
*          response has been received before tmo seconds.
*
*
* NOMANUAL
*/

int
revarpwhoami
    (
    struct in_addr *pClientIn,
    struct ifnet *pIfp,
    int tmo
    )

    {
    int count;
	
    if (rarpState.ipAddrInitialized) 
        return OK;

    if (rarpState.rarpDebug)
	logMsg("RARP: sending request\n",0,0,0,0,0,0);

    revarprequest(pIfp);
    for(count=0; count < SAMPLE_RATE*tmo; count++)
        {
        if (rarpState.ipAddrInitialized == FALSE)
            taskDelay (sysClkRateGet()/SAMPLE_RATE); 
        else 
	    {
            bcopy((caddr_t)&rarpState.ipAddr, (char *)pClientIn, 
		   sizeof(*pClientIn));

            if (rarpState.rarpDebug)
	        logMsg("RARP: received reply: 0x%x\n",
		        (unsigned int)rarpState.ipAddr.s_addr, 0, 0, 0, 0, 0);

            return OK;
            } 
        }

    /* no reply before time out */
    if (rarpState.rarpDebug)
        logMsg("RARP: did not receive reply after %d seconds\n",
	        tmo, 0, 0, 0, 0, 0);

    errnoSet(S_rarpLib_NORESPONSE);
    return ERROR;
    }
   
/*******************************************************************************
*
* revarprequest - form RARP ether packet and transmit
*
* Send a RARP request for the ip address of the specified interface.
*
* RETURNS: NA
*
* NOMANUAL
*/
void
revarprequest
    (
    struct ifnet *pIfp
    )

    {
	struct sockaddr sa;
	struct mbuf *pMbuf;
	struct ether_header *eh;
	struct ether_arp *ea;
	struct arpcom *ac = (struct arpcom *)pIfp;

	if ((pMbuf = mHdrClGet(M_DONTWAIT, MT_DATA, sizeof(*ea), TRUE)) == NULL)
	    return; 
	pMbuf->m_len = sizeof(*ea);
	pMbuf->m_pkthdr.len = sizeof(*ea);
	MH_ALIGN(pMbuf, sizeof(*ea));
	ea = mtod(pMbuf, struct ether_arp *);
	eh = (struct ether_header *)sa.sa_data;
	bzero((caddr_t)ea, sizeof(*ea));
	bcopy((caddr_t)etherbroadcastaddr, (caddr_t)eh->ether_dhost,
	      sizeof(eh->ether_dhost));
	eh->ether_type = ETHERTYPE_REVARP; /* htons is done in ipOutput */
	ea->arp_hrd = htons(ARPHRD_ETHER);
	ea->arp_pro = htons(ETHERTYPE_IP);
	ea->arp_hln = sizeof(ea->arp_sha);	/* hardware address length */
	ea->arp_pln = sizeof(ea->arp_spa);	/* protocol address length */
	ea->arp_op = htons(ARPOP_REVREQUEST);
	bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_sha,
	   sizeof(ea->arp_sha));
	bcopy((caddr_t)ac->ac_enaddr, (caddr_t)ea->arp_tha,
	   sizeof(ea->arp_tha));
	sa.sa_family = AF_UNSPEC;
	sa.sa_len = sizeof(sa);
	pIfp->if_output(pIfp, pMbuf, &sa, (struct rtentry *)0);
    }

/*******************************************************************************
*
* rarpIn -  callback routine bound to MUX 
*
* This routine is called by the mux when a packet is ready to be processed
*
* RETURNS: N/A
*
*/
void rarpIn        
    (
    END_OBJ *           pCookie,        /* Returned by muxBind() call. */
    long                type,           /* Protocol type.  */
    M_BLK_ID            m,          	/* The whole packet. */
    LL_HDR_INFO *       pLinkHdrInfo,   /* pointer to linklevel header*/
    IP_DRV_CTRL *       pDrvCtrl        /* ip Drv ctrl */
    )
    {
    struct arphdr *pArphdr;
    struct ether_arp *ea;
    char   *pChar;


    /* verify correct interface */
    if (rarpState.pEndId != pCookie)
        goto out;
        
    /* if a previous reply has already answered, we're done */
    if (rarpState.ipAddrInitialized)
        goto out;

    if (m->m_len < sizeof(struct arphdr))
	goto out;
    pChar = mtod(m, char *);

    pArphdr = (struct arphdr *)(pChar + pLinkHdrInfo->dataOffset);

    /* verify supported RARP values per RFC 903 */
    ea = (struct ether_arp *)pArphdr;
    if (ntohs(pArphdr->ar_hrd) != ARPHRD_ETHER)
	goto out;

    if (ntohs(pArphdr->ar_pro) != ETHERTYPE_IP)
	goto out;

    if (ntohs(ea->arp_op) != ARPOP_REVREPLY)
	goto out;

    if (m->m_len < sizeof(struct arphdr) + 
        2 * (pArphdr->ar_hln + pArphdr->ar_pln))
	goto out;

	/** copy address into global variable **/
    bcopy((caddr_t)ea->arp_tpa, (caddr_t)&rarpState.ipAddr, 
	   sizeof(rarpState.ipAddr));

    rarpState.ipAddrInitialized = TRUE;

    out:
	m_freem(m);
    }



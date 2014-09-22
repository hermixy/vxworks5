/* muxTkLib.c - MUX toolkit Network Interface Library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
02k,05nov01,vvv  fixed compilation warning
02j,15oct01,rae  merge from truestack ver 02m, base 02i (SPR #70512 etc)
02i,09nov00,spm  removed pNptCookie from END_OBJ for binary compatibility;
                 fixed handling of output protocols
02h,29oct00,ham  doc: removed reference to old SENS docs(SPR #35134).
02h,24oct00,niq  Merging in RFC2233 changes from tor2_0.open_stack-f1 branch
                 02f,18oct00,ann  applyingthe fix for Jitterbug #25 regarding
                 mac address not being filled correctly
                 02e,16may00,ann merging from post R1 on openstack branch to
                 incorporate RFC2233 implementation
                 02c,10mar00,ead fixed MIB-2 counter updates to use new
                 RFC 2233 m2IfLib
02g,18oct00,niq  Correct the order in which protocols are inserted in the
                 muxTkBind call
02f,16oct00,spm  merged version 02g from tor3_0_x branch (base version 02b):
                 adds multiple SNARF protocol support, code cleanup, and
                 backward compatibility fixes; overrides 02d and 02e changes
02e,25apr00,pul  fixing SPR 30584
02d,17apr00,pul  fixed TSR 153280, TSR 156655 which are part of the NPT patch
02c,12apr00,pul  modified _muxTkSend() to free an MBlk that could be allocated
                 by the addressForm() routine when the driver send() routine
                 returns an error
02b,06oct99,pul  removing references to mCastMapFunc and rtRequestFunc
02a,04oct99,pul  adding a new semaphore to protect the Bib entry traversal 
	 	 + fixed SPR # 28427
01z,07jul99,pul  To modify description for man page generation
01y,07jun99,sj   resurrect network layer pkt in _muxTkSend if endSend fails 
01x,29apr99,pul  Upgraded NPT phase3 code to tor2.0.0
01w,29mar99,pul  altered bind query, member name
01v,29mar99,pul  altered muxTkPollReceive to return correct error.
01u,26mar99,pul  removed all muxSvcFunc related stuff.
01t,25mar99,sj   muxEndRcvRtn undoes MAC header strip after calling outputFilter
01s,24mar99,sj   MUX_ID is installed as pSpare by muxTkBind + hook re-org
01r,19mar99,sj   copy protocol type in to the BIB in muxTkBindUpdate
01q,18mar99,sj   some clean up: collocated future release code at the end 
01p,09mar99,sj   muxTkBind can bind to ENDs
01o,05mar99,sj   eliminated hooks; not cleanly though. next version will do that
01n,22feb99,sj 	 added muxTkSend and muxTkPollSend and muxTkPollReceive
01m,26jan99,sj 	 moved BIB entry definition from here to muxLibP.h
01l,10Nov98,sj 	 more doc fixes
01k,09Nov98,sj 	 made logMsgs in muxTkLibInit conditional on muxTkDebug
01j,09Nov98,sj 	 return OK from muxTkLibInit
01i,08Nov98,sj   populate hooks only if not done already
01h,06Nov98,sj   forward declaration for nptHookRtn
01g,03Nov98,pul  modified muxMCastMapFuncGet to accept net svc type and net
                 drv type 
01f,03Nov98,pul  added muxMCastMapFuncGet, muxMCastMapFuncDel and comments
01e,03Nov98,pul  moved definition of MUX_BIND_ENTRY and MUX_SVC_FUNCS here.
		 modified muxTkLibInit to populate nptHook. Allocated
                 muxBindBase array during muxTkBibInit().
01d,02Nov98,sj   doc update
01c,27Oct98,pul  removed muxTkSend for future release 
01b,10Oct98,pul  Introduced hooks for ipProto module
01a,05Oct98,pul  Written
*/

/*
DESCRIPTION
This library provides additional APIs offered by the Network Protocol
Toolkit (NPT) architecture.  These APIs extend the original release of
the MUX interface.

A NPT driver is an enhanced END but retains all of the END's functionality.
NPT also introduces the term "network service sublayer" or simply "service
sublayer" which is the component that interfaces between the network service
(or network protocol) and the MUX.  This service sublayer may be built in
to the network service or protocol rather than being a separate component.

INTERNAL
The muxLib routines for binding protocols to END drivers also use certain
routines from this library so that the BIB accurately reflects all bindings
between protocols and drivers. Since all information in the BIB is also
stored elsewhere, the BIB array should be removed to avoid the predefined
limit it imposes on the number of active bindings.

INCLUDE FILES: vxWorks.h, taskLib.h, stdio.h, errno.herrnoLib.h, lstlib.h,
logLib.h, string.h, m2Lib.h, net/if.h, bufLib.h, semlib.h, end.h, muxLib.h,
muxTkLib.h, netinet/if_ether.h, net/mbuf.h
*/

/* includes */
#include "vxWorks.h"
#include "taskLib.h"
#include "stdio.h"
#include "errno.h"
#include "errnoLib.h"
#include "lstLib.h"
#include "logLib.h"
#include "string.h"
#include "m2Lib.h"
#include "net/if.h"             /* Needed for IFF_LOAN flag. */
#include "bufLib.h"
#include "semLib.h"

#include "end.h"		/* Necessary for any END as well as the MUX */
#include "muxLib.h"
#include "private/muxLibP.h"
#include "muxTkLib.h"
#include "netinet/if_ether.h"
#include "net/mbuf.h"
#include "memPartLib.h"


#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif    /* VIRTUAL_STACK */

/* defines */

#define STREQ(A, B) (strcmp ( (A), (B)) == 0)

#define TK_DRV_CHECK(pBib) ((((MUX_ID)pBib)->flags & BIB_TK_DRV) ? TRUE : FALSE)
#define TK_DRV_SET(pBib)   (((MUX_ID)pBib)->flags |= BIB_TK_DRV)
#define TK_DRV_CLEAR(pBib) (((MUX_ID)pBib)->flags &= ~BIB_TK_DRV)

#define DRV_ENTRY_CHECK(pBib) ((((MUX_ID)pBib)->flags & BIB_DRV_ENTRY) ? TRUE :\
				    FALSE)
#define PROTO_ENTRY_CHECK(pBib) ((((MUX_ID)pBib)->flags & BIB_PROTO_ENTRY) ?\
				TRUE : FALSE)

/* externs */

/* globals */

SEM_ID muxBibLock = NULL; /* To protect the traversal of Bib entries */
int muxTkDebug = 0;
int muxMaxBinds = 8;    /* Default (unless overridden by project facility). */

/* locals */

LOCAL MUX_ID muxBindBase;

/* forward declarations */

LOCAL int  muxTkBibEntryGet (END_OBJ * pEnd);
LOCAL void muxTkBibEntryFill (int index, int netSvcType, void * netCallbackId);
LOCAL void muxTkBibEntryFree (MUX_ID muxId);

LOCAL int  muxEndRcvRtn (void *, long, M_BLK_ID, LL_HDR_INFO *, void *);

/*****************************************************************************
*
* muxTkDrvCheck - checks if the device is an NPT or an END interface
*
* This function returns 1 if the driver indicated by <pDevName> is of the
* Toolkit (NPT) paradigm, and 0 (zero) if it is an END.  This routine is
* called by the network service sublayer so that it can discover the driver
* type before it binds to it via the MUX.
*
* INTERNAL:
* All network drivers having the same device name but different unit numbers
* are of the same type (NPT or END), so passing just the device name as an
* argument is sufficient
*
* .IP <pDevName>
* Expects a pointer to a string containing the name of the device
*
* RETURNS: 1 for an NPT driver, 0 for an END or other driver, or ERROR (-1) 
* if no device is found with the given name
*
* SEE ALSO: muxTkBind(), muxBind()
*/

int muxTkDrvCheck
    (
    char * pDevName	/* device name */
    )
    {
    int    count;
    BOOL   found = FALSE;

    /* find the BIB entry for the device and return the tkFlag status */

    semTake (muxBibLock, WAIT_FOREVER);
    for (count = 0; count < muxMaxBinds; count++)
        {
        if (STREQ(muxBindBase[count].devName, pDevName))
	    {
	    found = TRUE;
            break;
	    }
        }
    semGive (muxBibLock);

    if (found)
	return (TK_DRV_CHECK(&muxBindBase[count]));
    else
	return (ERROR);
    }

/*****************************************************************************
*
* muxTkCookieGet - returns the cookie for a device
*
* This routine returns the cookie for a device.
*
* RETURNS: a cookie to the device or NULL if unsuccessful
*
*/

void *muxTkCookieGet
    (
    char *pName,		/* Device Name */
    int unit			/* Device Unit */
    )
    {
    int    count = 0;
    END_OBJ *pEnd;

    pEnd = endFindByName (pName, unit);

    if (pEnd == NULL)
	{
	errnoSet (S_muxLib_NO_DEVICE);
	return (NULL);
	}

    /* find the BIB entry for the device */

    for (; (count < muxMaxBinds) && (muxBindBase[count].pEnd != pEnd); count++);

    /* if there is a match, return it */

    if (muxBindBase[count].pEnd == pEnd)
	return (&muxBindBase[count]);

    return (NULL);

    }
  
/******************************************************************************
*
* muxTkBind - bind an NPT protocol to a driver
*
* A network protocol, network service, or service sublayer uses this routine
* to bind to a specific driver.  This bind routine is valid both for END and
* NPT drivers, but the specified stack routine parameters must use the NPT
* function prototypes, and are somewhat different from those used with
* muxBind().
*
* The driver is specified by the <pName> and <unit> arguments, (for example,
* ln and 0, ln and 1, or ei and 0).
*
* .IP <pName> 20
* Expects a pointer to a character string that contains the name of the
* device that this network service wants to use to send and receive packets.
* .IP <unit>
* Expects the unit number of the device of the type indicated by <pName>.
* .IP <stackRcvRtn>
* Expects a pointer to the function that the MUX will call when it
* wants to pass a packet up to the network service.  For a description of
* how to write this routine, see the  
* .I "WindNet TCP/IP Network Programmer's Guide"
* .IP <stackShutdownRtn>
* Expects a pointer to the function that the MUX will call to
* shutdown the network service.  For a description of how to write such
* a routine, see the 
* .I "WindNet TCP/IP Network Programmer's Guide"
* .IP <stackTxRestartRtn>
* Expects a pointer to the function that the MUX will call after packet
* transmission has been suspended, to tell the network service that it can
* continue transmitting packets.  For a description of how to write this
* routine, see the 
* .I "WindNet TCP/IP Network Programmer's Guide"
* .IP <stackErrorRtn>
* Expects a pointer to the function that the MUX will call to give errors
* to the network service.  For a description of how to write this routine,
* see the section
* .I "WindNet TCP/IP Network Programmer's Guide"
* .IP <type>
* Expects a value that indicates the protocol type.  The MUX uses this type
* to prioritize a network service as well as to modify its capabilities.  For
* example, a network service of type MUX_PROTO_SNARF has the highest priority
* (see the description of protocol prioritizing provided in
* .I "WindNet TCP/IP Network Programmer's Guide."
* Aside from MUX_PROTO_SNARF and MUX_PROTO_PROMISC, valid network service
* types include any of the values specified in RFC 1700, or can be
* user-defined.
*
* The <stackRcvRtn> is called whenever the MUX has a packet of the specified
* type.  If the type is MUX_PROTO_PROMISC, the protocol is considered
* promiscuous and will get all of the packets that have not been consumed
* by any other protocol.  If the type is MUX_PROTO_SNARF, it will get all of
* the packets that the MUX sees.
*
* If the type is MUX_PROTO_OUTPUT, this network service is an output protocol
* and all packets that are to be output on this device are first passed to
* <stackRcvRtn> routine rather than being sent to the device.  This can be
* used by a network service that needs to send packets directly to another
* network service, or in a loop-back test.  If the <stackRcvRtn> returns OK,
* the packet is consumed and as no longer available.  The <stackRcvRtn> for an
* output protocol may return ERROR to indicate that it wants to look at the
* packet without consuming it.
* .IP <pProtoName>
* Expects a pointer to a character string for the name of this
* network service.  This string can be NULL, in which case a network service
* name is assigned internally.
* .IP <pNetCallbackId>
* Expects a pointer to a structure defined by the protocol.  This argument
* is passed up to the protocol as the first argument of all the callbacks.
* This argument corresponds to the <pSpare> argument in muxBind()
* .IP <pNetSvcInfo>
* Reference to an optional structure specifying network service layer
* information needed by the driver
* .IP <pNetDrvInfo>
* Reference to an optional structure specifying network driver information
* needed by the network protocol, network service, or service sublayer
*
* RETURNS: A cookie that uniquely represents the binding instance, or NULL
* if the bind fails.
*
* ERRNO: S_muxLib_NO_DEVICE, S_muxLib_END_BIND_FAILED, S_muxLib_NO_TK_DEVICE,
* S_muxLib_NOT_A_TK_DEVICE, S_muxLib_ALREADY_BOUND, S_muxLib_ALLOC_FAILED
*
* SEE ALSO: muxBind()
*
*/

void * muxTkBind
    (
    char * pName,		/* interface name, for example, ln, ei,... */
    int    unit,		/* unit number */
    BOOL   (*stackRcvRtn) (void*,long, M_BLK_ID, void *),
                                /* receive function to be called. */
    STATUS (*stackShutdownRtn) (void *),
                                /* routine to call to shutdown the stack */
    STATUS (*stackTxRestartRtn) (void *),
                                /* routine to tell the stack it can transmit */
    void   (*stackErrorRtn) (void*, END_ERR*),
                                /* routine to call on an error. */
    long   type,		/* protocol type from RFC1700 and many */
                                /* other sources (for example, 0x800 is IP) */
    char * pProtoName,          /* string name for protocol  */
    void * pNetCallbackId,      /* returned to network service sublayer during recv */
    void * pNetSvcInfo,         /* reference to netSrvInfo structure */
    void * pNetDrvInfo          /* reference to netDrvInfo structure */
    )
    {
    NET_PROTOCOL * pNewProto = NULL;
    NET_PROTOCOL * pNode = NULL;
    NODE *         pFinal = NULL;    /* Last snarf protocol, if any. */
    END_OBJ *      pEnd;
    int            index = 0;
    FUNCPTR        ioctlRtn = NULL;
    BOOL           tkFlag = FALSE;

    if (muxTkDebug)
	logMsg ("Start of muxTkBind\n", 1, 2, 3, 4, 5, 6);

    /* Check to see if we have a valid device. */

    pEnd = endFindByName (pName, unit);

    if (pEnd == NULL)
        {
        errnoSet (S_muxLib_NO_DEVICE);
        return (NULL);
        }

    if (muxTkDebug)
	logMsg ("Found the END object\n", 1, 2, 3, 4, 5, 6);

    /*
     * Query for the END bind routine and call it if available;
     * if the driver rejects the bind there's no need to proceed
     */

    if((ioctlRtn = pEnd->pFuncTable->ioctl) != NULL)
        {
	FUNCPTR   bindRtn = NULL;
	END_QUERY endQuery = {END_BIND_QUERY, 4, {0,0,0,0} };

	if ((*ioctlRtn) (pEnd,EIOCQUERY, &endQuery) == OK)
	    {
	    bindRtn = *(FUNCPTR *)endQuery.queryData;

	    if ((*bindRtn) (pEnd, pNetSvcInfo,
			    pNetDrvInfo, type) == ERROR)
		{
		if (muxTkDebug)
		    logMsg ("muxTkBind: EndBind failed\n", 1, 2, 3, 4, 5, 6);

		return (NULL);
		}
	    }
	}

    /* check if we are allowed to continue with this protocol type */

    if (type == MUX_PROTO_OUTPUT)
	{
	/*
	 * Check to see if this is an OUTPUT protocol and allow only
	 * one output filter per END
	 */

        if (pEnd->outputFilter)
            {
            errnoSet (S_muxLib_ALREADY_BOUND);
            return (NULL);
            }
	}
    else
	{
        /*
         * Get the appropriate place in the protocol list if necessary.
         * For standard protocols, also check if the protocol number
         * is available.
         */
 
        if (type != MUX_PROTO_PROMISC)
            {
            /* Get the first standard protocol and last snarf protocol, if any. */

            if (pEnd->snarfCount)
                {
                pFinal = lstNth (&pEnd->protocols, pEnd->snarfCount);
                pNode = (NET_PROTOCOL *)lstNext (pFinal);
                }
            else
                pNode = (NET_PROTOCOL *)lstFirst (&pEnd->protocols);
            }

        if (type != MUX_PROTO_SNARF && type != MUX_PROTO_PROMISC)
            {
            for ( ; pNode != NULL;
                  pNode = (NET_PROTOCOL *)lstNext (&pNode->node) )
                {
                if (pNode->type == type)
                    {
                    errnoSet (S_muxLib_ALREADY_BOUND);
                    return (NULL);
                    }
                }
            }
	}

   /* find an empty slot in the BIB */

   if ((index = muxTkBibEntryGet (pEnd)) < 0)
        {
	if (muxTkDebug)
	    logMsg ("muxTkBind: No more Bind entries", 1, 2, 3, 4, 5, 6);

	return (NULL);
        }
    else
	{
        tkFlag = TK_DRV_CHECK(&muxBindBase[index]);
	}

    /* Allocate a new protocol structure and bind the protocol to the END. */

    pNewProto = (NET_PROTOCOL *)KHEAP_ALLOC(sizeof (NET_PROTOCOL));

    if (pNewProto == NULL)
        {
        /* free BIB entry */

        bzero ((void *)&muxBindBase[index], sizeof(MUX_BIND_ENTRY));

        logMsg ("muxBind cannot allocate protocol node", 0, 0, 0, 0, 0, 0);
        errnoSet (S_muxLib_ALLOC_FAILED);
        return (NULL);
        }

    bzero ( (char *)pNewProto, sizeof(NET_PROTOCOL));

    /* bind the protocol to the END */

    if (type != MUX_PROTO_OUTPUT)
	{

	/* populate all fields in the new protocol node */

	pNewProto->stackShutdownRtn = stackShutdownRtn;
	pNewProto->stackTxRestartRtn = stackTxRestartRtn;
	pNewProto->stackErrorRtn = stackErrorRtn;
        }

    pNewProto->type = type;

    /* Indicate that muxTkBind() was used. */

    pNewProto->nptFlag = TRUE;

    if (tkFlag != TRUE && type != MUX_PROTO_OUTPUT)
        {
        /* Use a wrapper receive routine when sending data with END devices. */

        pNewProto->stackRcvRtn = muxEndRcvRtn;
        }
    else
        {
        pNewProto->stackRcvRtn = stackRcvRtn;
        }

    /*
     * For all input protocols bound using muxTkBind,
     * install the BIB entry pointer as the call back ID.
     */

    pNewProto->pSpare = &muxBindBase[index];

    if (type != MUX_PROTO_OUTPUT)
        {
	/* copy the protocol name or generate one if not available */

	if (pProtoName != NULL)
	    {
	    strncpy (pNewProto->name, pProtoName, 32);
	    }
	else
	    {
	    sprintf (pNewProto->name, "Protocol %d",
		     lstCount (&pEnd->protocols));
	    }
        }

    /* Insert the protocol in the END_OBJ protocol list */

    switch (type)
        {
        case MUX_PROTO_OUTPUT:
        case MUX_PROTO_PROMISC:
            lstAdd (&pEnd->protocols, &pNewProto->node);
            break;

        default:
            /* Add a standard or snarf protocol after any existing ones. */

            lstInsert(&pEnd->protocols, pFinal, &pNewProto->node);
            break;
        }

    if (type == MUX_PROTO_OUTPUT)
	{

	/* For an OUTPUT protocol place the filter in the END object itself. */

        pEnd->outputFilter = pNewProto;
        pEnd->pOutputFilterSpare = pNetCallbackId;
	muxBindBase[index].pEnd->outputFilter->stackRcvRtn = muxEndRcvRtn;
	pEnd->outputFilter->stackRcvRtn = stackRcvRtn;
	}

    muxBindBase[index].netStackRcvRtn = stackRcvRtn;

    /* fill the rest of the BIB entry */

    muxTkBibEntryFill (index, type, pNetCallbackId);
    if (type == MUX_PROTO_SNARF)     /* Increase the count. */
	(pEnd->snarfCount)++;
    pNewProto->pNptCookie = &muxBindBase[index];
    return (&muxBindBase[index]);
    }

/*****************************************************************************
*
* muxTkReceive - receive a packet from a NPT driver
*
* This is the routine that the NPT driver calls to hand a packet to the MUX.
* This routine forwards the received 'mBlk' chain to the network service
* sublayer by calling its registered stackRcvRtn().
*
* Typically, a driver includes an interrupt handling routine to process
* received packets.  It should keep processing to a minimum during interrupt
* context and then arrange for processing of the received packet within
* task context.
*
* Once the frame has been validated, the driver should pass it to the MUX
* with the 'receiveRtn' member of its END_OBJ structure.  This routine has
* the same prototype as (and typically is) muxTkReceive().
*
* Depending on the protocol type (for example, MUX_PROTO_SNARF or
* MUX_PROTO_PROMISC), this routine either forwards the received packet chain
* unmodified or it changes the data pointer in the 'mBlk' to strip off the
* frame header before forwarding the packet.
*
* .IP <pCookie>
* Expects the END_OBJ pointer returned by the driver's endLoad() or
* nptLoad() function
*
* .IP <pMblk>
* Expects a pointer to the 'mBlk' structure containing the packet that has
* been received
*
* .IP <netSvcOffset>
* Expects an offset into the frame to the point where the data field (the
* network service layer header) begins
*
* .IP <netSvcType>
* Expects the network service type of the service for which the packet is
* destined (typically this value can be found in the header of the received
* frame)
*
* .IP <uniPromiscuous>
* Expects a boolean set to TRUE when driver is in promiscuous mode and
* receives a unicast or a multicast packet not intended for this device.
* When TRUE the packet is not handed over to network services other than
* those registered as SNARF or PROMISCUOUS.
*
* .IP <pSpareData>
* Expects a pointer to any spare data the driver needs to pass up to the
* network service layer, or NULL
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_muxLib_NO_DEVICE
*/

STATUS muxTkReceive
    (
    void *    pCookie,		/* cookie passed in endLoad() call */
    M_BLK_ID  pMblk,		/* a buffer passed to us. */
    long      netSvcOffset,	/* offset to network datagram in the packet */
    long      netSvcType,	/* network service type */
    BOOL      uniPromiscuous,	/* TRUE when driver is in promiscuous mode */ 
    void *    pSpareData	/* out of band data */
    )
    {
    NET_PROTOCOL *      pProto;
    END_OBJ *           pEnd;
    long                type;
    M_BLK_ID            pMblkOrig;
    int 		count;

    pEnd = (END_OBJ *)pCookie;

    if (pEnd == NULL)
        {
        errnoSet (S_muxLib_NO_DEVICE);
        if (pMblk)
            netMblkClChainFree (pMblk);
        return (ERROR);
        }

    /* Grab a new block and parse out the header information. */

    count = lstCount (&pEnd->protocols);
    if (count <= 0)
        {
        if (pMblk)
            netMblkClChainFree (pMblk);
        return (FALSE);
        }

    pProto = (NET_PROTOCOL *)lstFirst (&pEnd->protocols);
    if (pProto == NULL)
        {
        if (pMblk)
            netMblkClChainFree (pMblk);
        return (ERROR);
        }

    /* Ignore incoming data if an output protocol is the only entry. */

    if (count == 1 && pProto->type == MUX_PROTO_OUTPUT)
        {
        if (pMblk)
            netMblkClChainFree (pMblk);
        return (ERROR);
        }

    /*
     * Loop through the protocol list.
     * If a protocol's receive routine returns TRUE, that means it has
     * consumed the packet.
     * The protocol's receive routine should always return FALSE if other
     * protocols have to see the packet.
     * SNARF protocols have the priority over every other protocol.
     * PROMISC protocols have the least priority.
     */

    type = netSvcType;

    for (; pProto != NULL; pProto = (NET_PROTOCOL *)lstNext (&pProto->node))
        {
        MUX_ID muxId = (MUX_ID)(pProto->pSpare);
	void * pNetCallbackId = NULL;

        if (pProto->type == MUX_PROTO_OUTPUT)
            continue;

        if (muxId)
            pNetCallbackId = muxId->pNetCallbackId;

        if (pProto->type == MUX_PROTO_SNARF)
            {
            if ((pProto->stackRcvRtn) && (*pProto->stackRcvRtn) 
		 (pNetCallbackId, type, pMblk, pSpareData) == TRUE)
                return (OK);
            }

        else if (pProto->type == type && !uniPromiscuous)
            {

            /* Make sure the entire Link Hdr is in the first M_BLK */

            pMblkOrig = pMblk;
            if (pMblk->mBlkHdr.mLen < netSvcOffset &&
                (pMblk = m_pullup (pMblk, netSvcOffset)) == NULL)
                {
                pMblk = pMblkOrig;
                break;
                }
  
            /* remove the MAC header and set the packet length accordingly */ 

            pMblk->mBlkHdr.mData += netSvcOffset;
            pMblk->mBlkHdr.mLen  -= netSvcOffset;
            pMblk->mBlkPktHdr.len  -= netSvcOffset;
            if ((pProto->stackRcvRtn) &&
                (((*pProto->stackRcvRtn) (pNetCallbackId, type, pMblk,
                                          pSpareData)) == TRUE))
                {
                return (OK);
                }
            }

        else if (pProto->type == MUX_PROTO_PROMISC)
            {
            if ((pProto->stackRcvRtn) &&
                (((*pProto->stackRcvRtn) (pNetCallbackId, type, pMblk,
                                          pSpareData)) == TRUE))
                {
                return (OK);
                }
            }
        }

    /* if the flow comes here then pMblk is not freed */

    if (pMblk)
        netMblkClChainFree (pMblk);

    return (OK);
    }


/*****************************************************************************
*
* _muxTkSend - A private function common to both muxTkSend and muxTkPollSend
*
* This routine does all of the processing required prior to a muxTkSend() or
* muxTkPollSend() and calls the driver's send() or pollSend() routine; a
* pointer to which is passed as the last argument. All other arguments are
* the same as muxTkSend() or muxTkPollSend()
*
* RETURNS: OK or ERROR
*
* SEE ALSO: muxTkSend(), muxTkPollSend, muxSend(), muxPollSend
*
* NOMANUAL 
*/
LOCAL STATUS _muxTkSend
    (
    MUX_ID    muxId,		/* cookie returned by muxTkBind */
    M_BLK_ID  pNBuff,		/* data to be sent */
    char *    dstMacAddr,	/* destination MAC address */
    USHORT    netType,		/* network protocol that is calling us: redundant? */
    void *    pSpareData,	/* spare data passed on each send */
    FUNCPTR   sendRtn		/* the driver's send routine: poll or regular */
    )
    {
    LL_HDR_INFO    llHdrInfo; 
    END_OBJ *      pEnd = muxId->pEnd;
    NET_PROTOCOL * pOut = NULL;

    /* we need these two M_BLKs for endAddressForm */

    M_BLK srcAddr = {{NULL,NULL,NULL,0,MT_IFADDR,0,0}, {NULL,0}, NULL};
    M_BLK dstAddr = {{NULL,NULL,NULL,0,MT_IFADDR,0,0}, {NULL,0}, NULL};

    if (pEnd == NULL)
        return (ERROR);

    /* bump MIB2 statistic */

    if (pNBuff != NULL &&
           (pNBuff->mBlkHdr.mFlags & M_BCAST ||
              pNBuff->mBlkHdr.mFlags & M_MCAST))
        {
        if (pEnd->flags & END_MIB_2233)
            {
            /*
             * Do nothing as ifOutNUcastPkts is updated by the
             * m2PktCountRtn function specified in the END objects
             * M2_ID.
             */
            }
        else /* (RFC1213 style counters supported) XXX */
            {
            ++(pEnd->mib2Tbl.ifOutNUcastPkts);
            }
        }

    if (pEnd->outputFilter)
        {
        pOut = pEnd->outputFilter;

        if (pOut->stackRcvRtn (pEnd->pOutputFilterSpare, netType, pNBuff, 
                               pSpareData) == TRUE)
            {
            return (OK);
            }
        }

    if (!TK_DRV_CHECK(muxId))
	{
	M_BLK_ID  pSrcAddr = &srcAddr;
	M_BLK_ID  pDstAddr = &dstAddr;
	M_BLK_ID  pMacFrame = NULL;
	char      srcMacAddr[64];

        /* copy END MAC addr into a temporary buffer */

        bzero (srcMacAddr, 64);
        if (pEnd->flags & END_MIB_2233)
            bcopy(END_ALT_HADDR(pEnd),srcMacAddr,END_ALT_HADDR_LEN(pEnd));
        else
            bcopy(END_HADDR(pEnd),srcMacAddr,END_HADDR_LEN(pEnd));

	pDstAddr->mBlkHdr.mData = dstMacAddr;
	pDstAddr->mBlkHdr.mLen = END_HADDR_LEN(pEnd);
	pDstAddr->mBlkHdr.reserved = netType;

	pSrcAddr->mBlkHdr.mData = srcMacAddr; 
	pSrcAddr->mBlkHdr.mLen = END_HADDR_LEN(pEnd);

	if (pEnd->pFuncTable->formAddress &&
	    ((pMacFrame = pEnd->pFuncTable->formAddress (pNBuff, pSrcAddr,
							 pDstAddr, FALSE))
	      != NULL))
	    {
	    STATUS status;

	    /*
	     * if we get here, we send the fully formed MAC frame to 
	     * the END driver
	     */

	    status = sendRtn (pEnd, pMacFrame);

	    if (status != OK)
		{

		/*
		 * resurrect the network layer packet
		 */

		if (pMacFrame == pNBuff)
		    {
		    if (pEnd->pFuncTable->packetDataGet (pNBuff, &llHdrInfo)
			== ERROR)
                        {
                        if (pNBuff)
                            netMblkClChainFree (pNBuff);
                        return ERROR;
                        }

		    /* point to the network pkt header, and adjust the length */

		    pNBuff->mBlkHdr.mData += llHdrInfo.dataOffset;
		    pNBuff->mBlkHdr.mLen  -= llHdrInfo.dataOffset;

		    if(pNBuff->mBlkHdr.mFlags & M_PKTHDR)
			pNBuff->mBlkPktHdr.len -= llHdrInfo.dataOffset;

		    #if 0
		    logMsg("_muxTkSendError\n",1,2,3,4,5,6);
		    #endif
		    }
                else
		    {
		    if(pMacFrame)
			{
			netMblkClFree(pMacFrame);
			}
                    pNBuff->mBlkHdr.mFlags |= M_PKTHDR;
                    }
		}
	    return(status);
	    }
	else
            {
            if (pNBuff)
                netMblkClChainFree (pNBuff);
            return ERROR;
            }
	}
    else /* NPT driver */
	{
	/* if we get here we send the packet to the driver */

	return (sendRtn (pEnd, pNBuff, dstMacAddr, netType, pSpareData));
	}
    }

/*****************************************************************************
*
* muxTkSend - send a packet out on a Toolkit or END network interface
*
* This routine uses <pCookie> to find a specific network interface and
* uses that driver's send routine to transmit a packet.
* 
* The transmit entry point for an NPT driver uses the following prototype:
* .CS
* STATUS nptSend
*     (
*     END_OBJ * pEND, 	        /@ END object @/
*     M_BLK_ID 	pPkt, 		/@ network packet to transmit @/
*     char * 	pDstAddr, 	/@ destination MAC address @/
*     int 	netType 	/@ network service type @/
*     void * 	pSpareData 	/@ optional network service data @/
*     )
* .CE
*
* The transmit entry point for an END driver the following prototype:
* .CS
* STATUS endSend
*     (
*     void * 	pEND, 	/@ END object @/
*     M_BLK_ID 	pPkt,	/@ Network packet to transmit @/
*     )
* .CE
*
* An END driver must continue to provide the addressForm() entry point to
* construct the appropriate link-level header. The <pDst> and <pSrc> M_BLK
* arguments to that routine supply the link-level addresses with the
* 'mData' and 'mLen' fields. The reserved field of the destination M_BLK
* contains the network service type. Both arguments \f2must\fP
* be treated as read-only.
*
* To send a fully formed physical layer frame to a device using an NPT
* driver (which typically forms the frame itself), set the M_L2HDR flag
* in the 'mBlk' header.
*
* A driver may be written so that it returns the error END_ERR_BLOCK if
* the driver has insufficient resources to transmit data.  The network
* service sublayer can use this feedback to establish a flow control
* mechanism by holding off on making any further calls to muxTkSend()
* until the device is ready to restart transmission, at which time the
* device should call muxTxRestart() which will call the service sublayer's
* stackRestartRtn() that was registered for the interface at bind time.
*
* .IP <pCookie>
* Expects the cookie returned from muxTkBind().  This Cookie
* identifies the device to which the MUX has bound this protocol.
*
* .IP <pNBuff>
* The network packet to be sent, formed into an 'mBlk' chain.
*
* .IP <dstMacAddr>
* Destination MAC address to which packet is to be sent, determined
* perhaps by calling the address resolution function that was registered
* for this service/device interface.
*
* .IP <netType>
* Network service type of the sending service.  This will be used to
* identify the payload type in the MAC frame.
*
* .IP <pSpareData>
* Reference to any additional data the network service wants to pass to the
* driver during the send operation.
*
* NOTE: A driver may return END_ERR_BLOCK if it is temporarily unable
* to complete the send, and then call muxTxRestart() to indicate that it
* is again able to send data.  If the driver has been written in this way,
* muxTkSend() will pass the ERR_END_BLOCK back as its own return value and
* the service can wait for its stackRestartRtn() callback routine to be
* called before trying the send operation again.
*
* RETURNS: OK; ENETDOWN, if <pCookie> doesn't represent a valid device;
* or an error, if the driver's send() routine fails.
*
* ERRNO: S_muxLib_NO_DEVICE
*/

STATUS muxTkSend
    (
    void *    pCookie,		/* returned by muxTkBind()*/
    M_BLK_ID  pNBuff,		/* data to be sent */
    char *    dstMacAddr,	/* destination MAC address */
    USHORT    netType,		/* network protocol that is calling us */
                                /** is netType redundant? **/
    void *    pSpareData	/* spare data passed on each send */
    )
    {
    MUX_ID    muxId = (MUX_ID)pCookie;
    END_OBJ * pEnd = muxId->pEnd;

    if (pEnd == NULL)
        {
        errnoSet (S_muxLib_NO_DEVICE);
        if (pNBuff)
            netMblkClChainFree (pNBuff);
        return (ENETDOWN);
        }
    else
        {
	FUNCPTR sendRtn = pEnd->pFuncTable->send;
	return (_muxTkSend (muxId, pNBuff, dstMacAddr, netType, pSpareData,
			    sendRtn));
        }
    }

/*****************************************************************************
*
* muxTkPollSend - send a packet out in polled mode to an END or NPT interface
*
* This routine uses <pCookie> to find a specific network interface and
* use that driver's pollSend() routine to transmit a packet.
*
* This routine replaces the muxPollSend() routine for both END and NPT
* drivers.
*
* When using this routine, the driver does not need to call muxAddressForm()
* to complete the packet, nor does it need to prepend an 'mBlk' of type
* MF_IFADDR containing the destination address.
*
* An NPT driver's pollSend() entry point is called based on this prototype:
* .CS
* STATUS nptPollSend
*     (
*     END_OBJ * pEND, 	        /@ END object @/
*     M_BLK_ID 	pPkt, 		/@ network packet to transmit @/
*     char * 	pDstAddr, 	/@ destination MAC address @/
*     long 	netType 	/@ network service type @/
*     void * 	pSpareData 	/@ optional network service data @/
*     )
* .CE
*
* The pollSend() entry point for an END uses this prototype:
* .CS
* STATUS endPollSend
*     (
*     END_OBJ * pEND, 	/@ END object @/
*     M_BLK_ID 	pPkt, 	/@ network packet to transmit @/
*     )
* .CE
*
* An END driver must provide the addressForm() entry point to
* construct the appropriate link-level header. The <pDst> and <pSrc> M_BLK
* arguments to that routine supply the link-level addresses with the
* mData and mLen fields. The reserved field of the destination M_BLK
* contains the network service type. Both arguments \f2must\fP be treated as
* read-only.
*
* .IP <pCookie>
* Expects the cookie returned from muxBind() or muxTkBind().  This cookie
* identifies the device to which the MUX has bound this protocol.
*
* .IP <pNBuff>
* The network packet to be sent.
*
* .IP <dstMacAddr>
* Destination MAC address to which packet is to be sent
*
* .IP <netType>
* Network service type that will be used to identify the payload data in
* the MAC frame.
*
* .IP <pSpareData>
* Reference to any additional data the network service wants to pass to the
* driver during the send operation.
*
* RETURNS: OK, ENETDOWN if <pCookie> doesn't represent a valid device,
* or an error if the driver's pollSend() routine fails.
*
* ERRNO: S_muxLib_NO_DEVICE
*/

STATUS muxTkPollSend
    (
    void *    pCookie,		/* returned by muxTkBind()*/
    M_BLK_ID  pNBuff,		/* data to be sent */
    char *    dstMacAddr,	/* destination MAC address */
    USHORT    netType,		/* network protocol that is calling us */
                                /** is netType redundant? **/
    void *    pSpareData	/* spare data passed on each send */
    )
    {
    MUX_ID    muxId = (MUX_ID)pCookie;
    END_OBJ * pEnd = muxId->pEnd;

    if (pEnd == NULL)
        {
        if (pNBuff)
            netMblkClChainFree (pNBuff);
        errnoSet (S_muxLib_NO_DEVICE);
        return (ENETDOWN);
        }
    else
        {
	FUNCPTR sendRtn = pEnd->pFuncTable->pollSend;
	return (_muxTkSend (muxId, pNBuff, dstMacAddr, netType,
			    pSpareData, sendRtn));
        }
    }

/*****************************************************************************
* 
* muxTkPollReceive - poll for a packet from a NPT or END driver
*
* This is the routine that an upper layer can call to poll for a packet.
* Any service type retrieved from the MAC frame is passed via the reserved
* member of the M_BLK header.
*
* This API effectively replaces muxPollReceive() for both END and NPT drivers.
*
* For an NPT driver its pollReceive() entry point is called based on the new
* prototype:
* .CS
* STATUS nptPollReceive
*     (
*     END_OBJ * pEND, 	        /@ END object @/
*     M_BLK_ID 	pPkt, 		/@ network packet buffer @/
*     long * 	pNetSvc, 	/@ service type from MAC frame @/
*     long * 	pNetOffset, 	/@ offset to network packet @/
*     void * 	pSpareData 	/@ optional network service data @/
*     )
* .CE
*
* The pollReceive() entry point for an END driver uses the original prototype:
* .CS
* STATUS endPollRcv
*     (
*     END_OBJ * pEND, 	/@ END object @/
*     M_BLK_ID 	pPkt, 	/@ network packet buffer @/
*     )
* .CE
*
* An END driver must continue to provide the packetDataGet() entry point
*
* .IP <pCookie>
* Expects the cookie that was returned from muxBind() or muxTkBind().
* This "cookie" identifies the driver. 
*
* .IP <pNBuff>
* Expects a pointer to a buffer chain into which incoming data will be put.
*
* .IP <pSpareData>
* A pointer to any optional spare data provided by a NPT driver. Always
* NULL with an END driver.
*
* RETURNS: OK; EAGAIN, if no packet was available; ENETDOWN, if the 'pCookie'
* does not represent a loaded driver; or an error value returned from the 
* driver's registered pollReceive() function.
*
* ERRNO: S_muxLib_NO_DEVICE
*/
STATUS muxTkPollReceive
    (
    void *    pCookie,	/* cookie from muxTkBind routine */
    M_BLK_ID  pNBuff,	/* a vector of buffers passed to us */
    void *    pSpare	/* a reference to  spare data is returned here */
    )
    {
    int          error        = OK;
    MUX_ID       muxId        = (MUX_ID)pCookie;
    END_OBJ *    pEnd         = muxId->pEnd;
    FUNCPTR      pollRcvRtn;
    LL_HDR_INFO  llHdrInfo; 
    int          netPktOffset = 0;
    int          netSvcType   = 0;

    if (pEnd == NULL)
	{
        errnoSet (S_muxLib_NO_DEVICE);
	return (ENETDOWN);
	}

    pollRcvRtn = pEnd->pFuncTable->pollRcv;

    if (!TK_DRV_CHECK(muxId))
	{

	/* call END poll receive routine */

	if ((error = pollRcvRtn (pEnd, pNBuff)) == OK)
	    {
	    if (pEnd->pFuncTable->packetDataGet (pNBuff,&llHdrInfo) == ERROR)
		return (ERROR);
	    }

	netPktOffset = llHdrInfo.dataOffset;
	netSvcType = llHdrInfo.pktType;

	if (pSpare)
	    *(UINT32 *)pSpare = (UINT32)NULL;
	}
    else
	{
	error = pollRcvRtn (pEnd, pNBuff,&netSvcType,
			    &netPktOffset, pSpare);

        if (error != OK)
	    return error;
	}

    /* restore the network layer packet for a non SNARF protocol */

    if (muxId->netSvcType != MUX_PROTO_SNARF)
	{
	pNBuff->mBlkHdr.mData += netPktOffset;
	pNBuff->mBlkHdr.mLen -= netPktOffset;
	}

    /* Save the service type returned by the driver in the reserved field. */

    pNBuff->mBlkHdr.reserved = (USHORT)netSvcType;

    return (error);
    }
/******************************************************************************
*
* muxTkPollReceive2 - muxTkPollReceive + muxTkReceive
*
* Combines muxTkPollReceive and muxTkReceive.
*
* RETURNS: OK if the packet is passed up the stack
* 
* NOMANUAL
*/

STATUS muxTkPollReceive2
    (
    void *    pCookie,  /* cookie from muxTkBind routine */
    M_BLK_ID  pNBuff
    )
    {
    MUX_ID       muxId        = (MUX_ID)pCookie;
    END_OBJ *    pEnd         = muxId->pEnd;
    FUNCPTR      pollRcvRtn;
    int          netPktOffset = 0;
    int          netSvcType   = 0;
    
    pollRcvRtn = pEnd->pFuncTable->pollRcv;

    if (pollRcvRtn (pEnd, pNBuff, &netSvcType, &netPktOffset, NULL) == ERROR)
	return (ERROR);
    
    /* restore the network layer packet for a non SNARF protocol */

    if (((MUX_ID)pCookie)->netSvcType != MUX_PROTO_SNARF)
	{
	pNBuff->mBlkHdr.mData += netPktOffset;
	pNBuff->mBlkHdr.mLen -= netPktOffset;
	}

    /* Save the service type returned by the driver in the reserved field. */

    pNBuff->mBlkHdr.reserved = (USHORT)netSvcType;

    muxTkReceive (pCookie, pNBuff, netPktOffset, netSvcType,
	((END_OBJ *)pCookie)->flags & IFF_PROMISC, NULL);

    return (OK);
    }

/******************************************************************************
*
* muxEndRcvRtn - wrapper stack receive routine for END drivers
* 
* This routine is installed as the stackRcvRoutine() for all END drivers bound
* to with the muxTkBind() call. This allows muxTkBind() to support both END
* and NPT drivers and thus eliminates the need to call muxTkDrvCheck().
*
* For incoming data, the <pCookie> argument is equal to the value from the
* muxBind() routine (if it had been used). For an NPT output filter bound
* to an END device, the <pCookie> argument is equal to NULL.
*
* The <pSpare> argument is equal to the address of the BIB entry for the
* protocol/device binding.
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

int muxEndRcvRtn
    (
    void *        pCookie,       	/* muxReceive cookie, or NULL */
    long          type,			/* network service type */
    M_BLK_ID      pMblk,		/* MAC frame */
    LL_HDR_INFO * pLinkHdrInfo,		/* link level information */
    void *        pSpare		/* pSpare argument (from muxBind) */
    )
    {
    BOOL      retVal = FALSE;
    MUX_ID    muxId  = (MUX_ID) pSpare;
    END_OBJ * pEnd   = muxId->pEnd;

    if (pLinkHdrInfo == NULL || pEnd == NULL)
	return ERROR;

    /* point to network packet and adjust the length */

    pMblk->mBlkHdr.mData        += pLinkHdrInfo->dataOffset;
    pMblk->mBlkHdr.mLen         -= pLinkHdrInfo->dataOffset;
    pMblk->mBlkPktHdr.len       -= pLinkHdrInfo->dataOffset;

    /* call the registered network service's stack recv routine */

    retVal = muxId->netStackRcvRtn (muxId->pNetCallbackId,type,pMblk,NULL);

    if (muxId->netSvcType == MUX_PROTO_OUTPUT && retVal == FALSE)
	{

	/*
	 * if we are called via the output filter mechanism and the
	 * filter does not consume the packet we need to undo the stripping
	 * of the MAC header
	 */

	pMblk->mBlkHdr.mData        -= pLinkHdrInfo->dataOffset;
	pMblk->mBlkHdr.mLen         += pLinkHdrInfo->dataOffset;
	pMblk->mBlkPktHdr.len       += pLinkHdrInfo->dataOffset;
	}
    return (retVal);
    }


/******************************************************************************
*
* muxTkBibEntryGet - find the next available slot in the BIB 
*
* This is a internal function called to find the next available entry in
* the BIB. For the first protocol that binds to a particular device the
* entry that was partially filled at the time of driver load, using
* muxDevLoad(), is used. For the second protocol onwards a new entry is
* claimed
*
* RETURNS : muxId or NULL.
* NOMANUAL
*/
int muxTkBibEntryGet
    (
    END_OBJ * pEnd	/* the END the protocol is trying to bind to */
    )
    {
    int            multBinds = 0;
    int            count = 0;
    int            index = -1;
    NET_PROTOCOL * pProto;

   /*
    * mutual exclusion begins here and we keep it till we have a valid
    * BIB index 
    */

    if (semTake (muxBibLock, WAIT_FOREVER) == ERROR)
       return (-1);

    /* are one or more protocols already bound to this device ? */

    multBinds = lstCount (&pEnd->protocols); 

    /*
     * KLUDGE to accomodate muxBind
     */

     if (multBinds == 1)
	 {
	 pProto = (NET_PROTOCOL *)lstFirst (&pEnd->protocols);
	 if (! pProto->nptFlag)
	     multBinds = 0;
	 }
    /*
     * if this is the first protocol being registered with this device,
     * we simply use search for and use the BIB slot in which the driver
     * was loaded using muxDevLoad(); else we find the next available slot
     * in the BIB
     */  

    if (!multBinds)
	{
	for (count = 0; count < muxMaxBinds; count++)
	    {
	    if (STREQ(muxBindBase[count].devName, pEnd->devObject.name) &&
		muxBindBase[count].unitNo == pEnd->devObject.unit)
	       {
	       index = count;
	       break;
	       }
	    }
	}
    else
        {
        for (count = 0; (count < muxMaxBinds) && 
		    (muxBindBase[count].pEnd != NULL); count++)
		    ;
	/*
	 * these fields in the BIB entry need to be filled only when 
	 * more than one protocol is bound to the same device
	 */

	if (count < muxMaxBinds)
	    {
	    index = count;

	    /* reserve this slot for us */

	    muxBindBase[index].pEnd = pEnd;

	    muxBindBase[index].unitNo = pEnd->devObject.unit;
	    strcpy(muxBindBase[count].devName, pEnd->devObject.name);

	    if ((END_IOCTL(pEnd)(pEnd, EIOCGNPT, NULL)) == OK)
		{
		TK_DRV_SET(&muxBindBase[count]);
		}
	    else
		{
		TK_DRV_CLEAR(&muxBindBase[count]);
		}
	    }
        }

    /* say that a protocol has taken this slot */

    muxBindBase[index].flags |= BIB_PROTO_ENTRY;

    /* we have the BIB index and we are clear to release muxLock */

    semGive (muxBibLock);

    return (index);
    }

/******************************************************************************
*
* muxTkBibEntryFill - fill and complete the BIB entry
*
* This is a internal function called to complete the BIB entry that was
* claimed using muxTkBibEntryGet()
*
* RETURNS : muxId or NULL.
* NOMANUAL
*/
void muxTkBibEntryFill
    (
    int    index,		/* BIB entry pointer */
    int    netSvcType,		/* protocol that is being bound */
    void * pNetCallbackId	/* protocols callback Id */
    )
    {
    M2_INTERFACETBL mib2Tbl;
    M2_ID * pM2ID;
    long            netDrvType = 0;

    /* get the MAC type from the END MIB table */
    if (muxBindBase[index].pEnd->flags & END_MIB_2233)
        {
        muxIoctl(&muxBindBase[index], EIOCGMIB2233, (caddr_t)&pM2ID);
        netDrvType = pM2ID->m2Data.mibIfTbl.ifType;
        }
    else /* (RFC1213 style counters supported) XXX */
        {
        muxIoctl(&muxBindBase[index], EIOCGMIB2, (caddr_t)&mib2Tbl);
        netDrvType = mib2Tbl.ifType;
        }

    /* populate the rest of the BIB entry */

    muxBindBase[index].netSvcType = netSvcType;
    muxBindBase[index].netDrvType = netDrvType;
    muxBindBase[index].pNetCallbackId = pNetCallbackId;

    /* populate the BIB entry with the registered network service functions */

    muxBindBase[index].addrResFunc = muxAddrResFuncGet (netDrvType,netSvcType);

    }

/******************************************************************************
*
* muxTkBibEntryFree - free the BIB entry
*
* This is a internal function called to free the said BIB entry
*
* RETURNS : N/A
* NOMANUAL
*/
LOCAL void muxTkBibEntryFree
    (
    MUX_ID muxId	/* BIB entry pointer */
    )
    {
    if (muxId == NULL)
	return;

    /*
     * we bzero the entry if it is a driver load entry and no protocols
     * are bound OR if it is not a driver load entry and a protocol is
     * bound at this entry
     */

    if (((DRV_ENTRY_CHECK(muxId) == TRUE) &&
	 (PROTO_ENTRY_CHECK(muxId)== FALSE)) ||
        ((DRV_ENTRY_CHECK(muxId) == FALSE) &&
	 (PROTO_ENTRY_CHECK(muxId)== TRUE)))
	{
	bzero ((void *)muxId, sizeof(MUX_BIND_ENTRY));
	}
    else if ((PROTO_ENTRY_CHECK(muxId) == TRUE) && 
	     (DRV_ENTRY_CHECK(muxId) == TRUE))
	{

	/* here we simply clean up the protocol information */

    	muxId->flags &= ~BIB_PROTO_ENTRY;
	muxId->addrResFunc = NULL;
	muxId->netSvcType = 0;
	muxId->netDrvType = 0;
	muxId->pNetCallbackId = NULL;
	}
    }

/******************************************************************************
*
* muxTkBibShow - show the BIB entries
*
* This is an unpublished BIB show routine 
*
* RETURNS : N/A
* NOMANUAL
*/
void muxTkBibShow (void)
    {
    int    i;
    int    count = 0;
    MUX_ID muxId = NULL;

    for (i = 0; i < muxMaxBinds; i++)
	{
	if (muxBindBase[i].pEnd == NULL)
	    {
	    continue;
	    }
	else if (++count == 1)
	    {
	    /* print banner */
	    printf("\n%5s   %8s  %8s  %8s  %8s \n", "Slot#",
		    "Svc Type", "Drv Name", "Drv Unit", "DRV_ENT");
	    printf("%4s   %8s  %8s  %8s  %8s \n\n", "----",
		    "--------", "--------", "--------", "--------");
	    }

	muxId = &muxBindBase[i];
	printf ("%5d  0x%x  %8s  %8d  %8s \n", i,
	        (UINT)muxId->netSvcType, muxId->devName, muxId->unitNo,
	        (char *)(DRV_ENTRY_CHECK(muxId) ? "DRV_ENTRY" : "    "));
	}
    }

/*
 * The functions below are called by the SENS MUX API implementation to upgrade
 * to NPT infrastructure using the BIB
 */

/******************************************************************************
*
* muxTkBibInit - Initialize the BIB
*
* This initializes the MUX BIB used by NPT to store binding information. This
* routine also initializes the list to store the MUX service functions. 
*
* Note: This routine is called by muxLibInit()
*
* RETURNS: OK  or ERROR
* NOMANUAL
*/

STATUS muxTkBibInit (void)
    {
    int count;

    if (muxBibLock != NULL)
	return (OK);

    muxBibLock = semBCreate (SEM_Q_PRIORITY, SEM_FULL);
    if (muxBibLock == NULL)
	return (ERROR);

    /* allocate the BIB table */
    muxBindBase = (MUX_ID) KHEAP_ALLOC(muxMaxBinds * sizeof (MUX_BIND_ENTRY));
    if (muxBindBase == NULL)
	return (ERROR);


    /* Initialize the BIB table */
    for (count = 0; count < muxMaxBinds; count++)
        bzero ((char*) &muxBindBase[count], sizeof(MUX_BIND_ENTRY));

    return (OK);
    }

/*******************************************************************************
*
* muxTkDevLoadUpdate - Hook routine called from muxDevLoad
*
* This is called from muxDevLoad() to load the driver into the BIB.
* The driver is queried for NPT/END mode of operation and the receive
* routine in the END_OBJ is set accordingly.
*
* RETURNS: muxId or NULL.
* NOMANUAL
*/

void * muxTkDevLoadUpdate
    (
    END_OBJ * pEnd
    )
    {
    int count = 0;  /* index for BIB */

    if (pEnd == NULL)
        return (NULL);

    /* Take the Bib semaphore and create an Entry in the BIB table */

    semTake (muxBibLock, WAIT_FOREVER);
    while ( (muxBindBase[count].pEnd != NULL) && (count < muxMaxBinds))
	{
	count++;
	}
    semGive (muxBibLock);

    if (count == muxMaxBinds)
        return (NULL);

    muxBindBase[count].unitNo = pEnd->devObject.unit;
    strcpy (muxBindBase[count].devName, pEnd->devObject.name);
    muxBindBase[count].pEnd = pEnd;
    muxBindBase[count].flags = BIB_DRV_ENTRY;

    /* set the toolkit flag and the MUX receive routine  */

    if ((END_IOCTL(pEnd)(pEnd, EIOCGNPT, NULL)) == OK)
        {
        TK_DRV_SET(&muxBindBase[count]);
        pEnd->receiveRtn = (FUNCPTR)muxTkReceive;
        }
    else
        {
        TK_DRV_CLEAR(&muxBindBase[count]);
        pEnd->receiveRtn = (FUNCPTR)muxReceive;
        }
    return (&muxBindBase[count]);
    }


/*******************************************************************************
*
* muxTkUnbindUpdate - free the BIB entry upon unbind
*
* This routine is called by muxUnbind()
*
* RETURNS : N/A
* NOMANUAL
*/
void muxTkUnbindUpdate
    (
    void * pCookie	/* bind cookie */
    )
    {
    MUX_ID muxId = (MUX_ID) pCookie;
    muxTkBibEntryFree (muxId);
    }

/*******************************************************************************
*
* muxTkBindUpdate - Hook routine is called from muxBind
*
* This hook is called from muxBind() to update the BIB introduced by NPT
* Finds an entry in the mux BIB and fills in the relevant information
*
* RETURNS : muxId or NULL.
* NOMANUAL
*/

void * muxTkBindUpdate
    (
    END_OBJ *      pEnd,	/* the END being bound to */
    NET_PROTOCOL * pProto	/* the protocol node added to the END */
    )
    {
    int    index          = -1;
    BOOL   tkFlag         = FALSE;
    int    netSvcType     = 0;
    void * pNetCallbackId = NULL;

    if (pEnd == NULL)
        return (NULL);

    /* find and get a BIB entry */

    if ((index = muxTkBibEntryGet (pEnd)) < 0)
	return (NULL);
    else
	{
	if ((tkFlag = TK_DRV_CHECK(&muxBindBase[index])) == TRUE)
	    {

	    /*
	     * cannot bind to a NPT driver using muxBind; free BIB entry
	     * and return
	     */

            muxTkBibEntryFree (&muxBindBase[index]);
	    return (NULL);
	    }
	}

    if (pProto != NULL)
	{
	netSvcType = pProto->type;
	pNetCallbackId = pProto->pSpare;
	muxBindBase[index].netStackRcvRtn = pProto->stackRcvRtn;
	}

    /* complete the BIB entry */

    muxTkBibEntryFill (index, netSvcType,pNetCallbackId);

    return (&muxBindBase[index]);
    }

/******************************************************************************
*
* muxTkUnloadUpdate - delete the entries in the BIB
*
* This hook is called from muxDevUnload() when a device is being unloaded and
* the corresponding entry in the BIB is to be removed
*
* RETURNS STATUS
* NOMANUAL
*/

STATUS muxTkUnloadUpdate
    (
    END_OBJ * pEnd
    )
    {
    int  count = 0;     /* index */
    BOOL error = FALSE;

    /* delete the corresponding entry in the BIB table */

    semTake (muxBibLock, WAIT_FOREVER); 
    for (count = 0; count < muxMaxBinds; count++)
        {
        /* commented out since T2 NPT
	if (STREQ(muxBindBase[count].devName, pEnd->devObject.name) &&
	    muxBindBase[count].unitNo==pEnd->devObject.unit &&
	    (DRV_ENTRY_CHECK(&muxBindBase[count]) == TRUE))
	*/
	if (muxBindBase[count].pEnd == pEnd)
	    {
	    if (muxBindBase[count].netSvcType == MUX_PROTO_OUTPUT)
		muxTkBibEntryFree(&muxBindBase[count]);

	    if (DRV_ENTRY_CHECK(&muxBindBase[count]) == TRUE)
		{
		muxTkBibEntryFree (&muxBindBase[count]);
		error = TRUE;
		}
	    }
        }
 
    semGive (muxBibLock);
    return (error);
    }

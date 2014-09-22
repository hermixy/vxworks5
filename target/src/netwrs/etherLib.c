/* etherLib.c - Ethernet raw I/O routines and hooks */

/* Copyright 1984 - 2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02w,16oct00,spm  fixed etherInputHookAdd and etherInputHookDelete (SPR #30166)
02v,16oct00,spm  merged documentation updates from tor2_0_x branch (SPR #21383)
02u,27may99,pul  etherInputHookAdd changes for NPT driver and 
		 tagged Ethernet frames support
02t,27apr99,sj   Merged post SENS1.1 fixes done for Tor2.0
02s,29mar99,dat  documentation, removed refs to obsolete drivers (26119)
02r,16mar99,spm  recovered orphaned code from tor1_0_1.sens1_1 (SPR #25770)
02q,15dec98,jmb  SENS merge, (01w-02p)
02p,06oct98,spm  fixed buffer overrun in END input hook routine (SPR #22363)
02o,27aug98,n_s  set unit number in fakeIF for endEtherInputHook. spr #22222.
02n,14dec97,jdi  doc: cleanup.
02m,08dec97,gnn  END code review fixes.
02l,28oct97,gnn  fixed SPR 9606, caused by problems in the endEtherInputHook
02k,27oct97,spm  corrected input hooks to handle mixed device types; added
                 support for multiple hooks per END device
02j,25oct97,kbw  making minor man page fixes
02i,17oct97,vin  changes reflecting protocol recieveRtn changes.
02h,09oct97,spm  fixed memory leak in END driver input hook handler
02g,03oct97,gnn  removed necessity for endDriver global
02f,25sep97,gnn  SENS beta feedback fixes
02e,26aug97,spm  removed compiler warnings (SPR #7866)
02d,19aug97,gnn  changes due to new buffering scheme.
02c,12aug97,gnn  changes necessitated by MUX/END update.
02b,02jun97,spm  corrected invalid mem width causing m_devget panic (SPR #8640)
02a,15may97,gnn  removed some warnings.
                 modified muxUnbind.
01z,20mar97,spm  fixed memory leak in Ethernet hook routines
01y,21jan97,gnn  changed MUX_PROTO_PROMISC to MUX_PROTO_SNARF.
01x,17dec96,gnn  added code to handle the new etherHooks and END stuff.
01w,22oct96,gnn  added etherTypeGet routine to heuristically figure out the
                 proper ethertype for a packet given a pointer to it.
                 This handles 802.3 addressing.
01w,15apr96,hk   added awai-san's etherOutput() fix for SH.
01v,01nov95,jdi  doc: updated list of supported drivers (SPR 4545).
01u,20jan93,jdi  documentation cleanup for 5.1.
01t,13nov92,dnw  added includes of unixLib.h and hostLib.h
01s,15aug92,elh  fixed bug in etherAddrResolve.
01r,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01q,19nov91,rrr  shut up some ansi warnings.
01p,15oct91,elh  added ether{Input,Output}HookDelete
01o,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed copyright notice
01n,30apr91,jdi	 documentation tweaks.
01m,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01l,04feb91,jaa	 documentation cleanup.
01k,05oct90,dnw  documentation tweaks.
01j,11apr90,hjb  de-linted.
01i,19mar90,jdi  documentation cleanup.
01h,20aug88,gae  documentation.
01g,22jun88,dnw  name tweaks.
01f,30may88,dnw  changed to v4 names.
01e,05jan88,rdc  added include of systm.h
01d,18nov87,ecs  documentation.
01c,11nov87,dnw  added etherAddOutputHook().
		 changed calling sequence to input hook routine.
01b,05nov87,jlf  moved from netsrc to src
		 documentation
01a,28aug87,dnw  written
*/

/*
DESCRIPTION
This library provides utilities that give direct access to Ethernet packets.
Raw packets can be output directly to an interface using etherOutput().
Incoming and outgoing packets can be examined or processed using the hooks
etherInputHookAdd() and etherOutputHookAdd().  The input hook can be used to
receive raw packets that are not part of any of the supported network
protocols.  The input and output hooks can also be used to build network
monitoring and testing tools.

Normally, the network should be accessed through the higher-level socket
interface provided in sockLib.  The routines in etherLib should rarely,
if ever, be necessary for applications.

CAVEAT
All VxWorks Ethernet drivers using the generic MUX/END model support both
the input hook and output hook routines. Both hook types are also supported 
by the following BSD model drivers:

    if_cpm - Motorola CPM Ethernet driver
    if_eex - Intel EtherExpress 16 network interface driver
    if_ei - Intel 82596 Ethernet driver
    if_eihk - Intel 82596 Ethernet driver (for hkv3500)
    if_elc - SMC 8013WC Ethernet driver
    if_elt - 3Com 3C509 Ethernet driver
    if_ene - Novell/Eagle NE2000 network driver
    if_esmc - Ampro Ethernet2 SMC-91c9x Ethernet driver
    if_fei - Intel 82557 Ethernet driver
    if_fn - Fujitsu MB86960 NICE Ethernet driver
    if_ln - Advanced Micro Devices Am7990 LANCE Ethernet driver
    if_mbc - Motorola 68EN302 Ethernet driver 
    if_sm - shared memory backplane network interface driver
    if_sn - National Semiconductor DP83932B SONIC Ethernet driver
    if_ultra - SMC Elite Ultra Ethernet driver

The following BSD drivers support only the input hook routines:

    if_cs - Crystal Semiconductor CS8900 network interface driver
    if_dc - DEC 21x4x Ethernet LAN network interface driver
    if_nicEvb - National Semiconductor ST-NIC Ethernet driver (for evb403)
    if_sl - Serial Line IP (SLIP) network interface driver

The following BSD drivers support only the output hook routines:

    if_ulip - network interface driver for User Level IP (VxSim)

The following BSD drivers do not support either the input hook or
output hook routines:

    if_loop - software loopback network interface driver

INCLUDE FILES: etherLib.h

SEE ALSO:
.pG "Network"
*/

#include "vxWorks.h"
#include "lstLib.h"
#include "net/mbuf.h"
#include "net/if.h"
#include "net/route.h"
#include "netinet/if_ether.h"
#include "net/if_llc.h"
#include "net/systm.h"
#include "inetLib.h"
#include "net/unixLib.h"
#include "hostLib.h"
#include "end.h"
#include "muxLib.h"
#include "netBufLib.h"

#include "muxTkLib.h"
#include "private/muxLibP.h"

IMPORT unsigned char etherbroadcastaddr [];

FUNCPTR etherInputHookRtn  = NULL;	/* NULL = none */
FUNCPTR etherOutputHookRtn = NULL;	/* NULL = none */

LOCAL BOOL endEtherInputHookRtn (void * pCookie, long type, M_BLK_ID pMblk, 
                                 LL_HDR_INFO * pLinkHdrInfo, void * pSpare);
LOCAL BOOL nptEtherInputHookRtn (void * pCallBackId, long type, M_BLK_ID pMblk, 
                                 void * pSpareData);
LOCAL BOOL etherInputHook (struct ifnet * pIf, char * buffer, int length);
LOCAL BOOL etherOutputHook (struct ifnet * pIf, char * buffer, int length);

/* locals */

LOCAL LIST inputHookList;
LOCAL LIST outputHookList;

LOCAL BOOL etherInputHookActive = FALSE;

typedef struct hook_entry
    {
    NODE node; 		/* Attachment point for linked list. */
    FUNCPTR routine;
    void * pCookie; 	/* For use with END devices, NULL for BSD devices. */
    char name[8]; 	/* Device name (END devices only). */
    int unit; 		/* Device unit number (END devices only). */
    } HOOK_ENTRY;

/* defints */

#define STREQ(A, B) (strcmp(A, B) == 0 ? 1 : 0)

/* RFC 894 Header. */
typedef struct enet_hdr
    {
    char dst [6];
    char src [6];
    USHORT type;
    } ENET_HDR;

/*******************************************************************************
*
* etherOutput - send a packet on an Ethernet interface
*
* This routine sends a packet on the specified Ethernet interface by
* calling the interface's output routine directly.
*
* The first argument <pIf> is a pointer to a variable of type `struct ifnet'
* which contains some useful information about the network interface.  A
* routine named ifunit() can retrieve this pointer from the system in the
* following way:
* .CS
*     struct ifnet *pIf;
*     ...
*     pIf = ifunit ("ln0");
* .CE
* If ifunit() returns a non-NULL pointer, it is a valid pointer to
* the named network interface device structure of type `struct ifnet'.
* In the above example, <pIf> points to the data structure that
* describes the first LANCE network interface device if ifunit() is
* successful.
*
* The second argument <pEtherHeader> should contain a valid Ethernet address
* of the machine for which the message contained in the argument <pData> is
* intended.  If the Ethernet address of this machine is fixed and well-known
* to the user, filling in the structure `ether_header' can be accomplished by
* using bcopy() to copy the six-byte Ethernet address into the `ether_dhost'
* field of the structure `ether_header'.  Alternatively, users can make use of
* the routine etherAddrResolve() which will use ARP (Address Resolution
* Protocol) to resolve the Ethernet address for a specified Internet
* address.
*
* RETURNS: OK, or ERROR if the routine runs out of mbufs.
*
* SEE ALSO:
* etherAddrResolve()
*/

STATUS etherOutput
    (
    struct ifnet *pIf,                  /* interface on which to send */
    struct ether_header *pEtherHeader,  /* Ethernet header to send    */
    FAST char *pData,                   /* data to send               */
    FAST int dataLength                 /* # of bytes of data to send */
    )
    {
    FAST struct mbuf *m;	/* ptr to current mbuf piece */
#if (CPU_FAMILY==SH)
    FAST int i;
#endif /* CPU_FAMILY==SH */
    struct sockaddr dst;	/* destination address */
    int oldLevel;

    /* construct dst sockaddr required by interface output routine;
     * all Ethernet drivers interpret address family AF_UNSPEC as raw
     * Ethernet header (this is used by arp) */

    dst.sa_family = AF_UNSPEC;
    dst.sa_len = sizeof(dst);

#if (CPU_FAMILY==SH)
    for ( i=0; i<SIZEOF_ETHERHEADER; i+=2 )
        *((UINT16 *)((int)&(dst.sa_data[0])+i)) =
                                        *((UINT16 *)((int)pEtherHeader+i));
#else /* CPU_FAMILY==SH */
    *((struct ether_header *) dst.sa_data) = *pEtherHeader;
#endif /* CPU_FAMILY==SH */

    if ((m = bcopy_to_mbufs (pData, dataLength, 0, pIf, NONE)) == NULL)
	return (ERROR);

    /* call interface's output routine */

    oldLevel = splnet ();
    (* pIf->if_output) (pIf, m, &dst, (struct rtentry *)NULL);
    splx (oldLevel);

    return (OK);
    }

/*******************************************************************************
*
* etherInputHookAdd - add a routine to receive all Ethernet input packets
*
* This routine adds a hook routine that will be called for every Ethernet
* packet that is received.
*
* The calling sequence of the input hook routine is:
* .CS
*     BOOL inputHook
*          (
*          struct ifnet *pIf,    /@ interface packet was received on @/
*          char         *buffer, /@ received packet @/
*          int          length   /@ length of received packet @/
*          )
* .CE
* The hook routine should return TRUE if it has handled the input packet and
* no further action should be taken with it.  It should return FALSE if it
* has not handled the input packet and normal processing (for example, 
* Internet) should take place.
*
* The packet is in a temporary buffer when the hook routine is called.
* This buffer will be reused upon return from the hook.  If the hook
* routine needs to retain the input packet, it should copy it elsewhere.
*
* IMPLEMENTATION
* A call to the function pointed to by the global function pointer
* `etherInputHookRtn' should be invoked in the receive routine of every
* network driver providing this service.  For example:
* .CS
*     ...
*     #include "etherLib.h"
*     ...
*     xxxRecv ()
*     ...
*     /@ call input hook if any @/
*
*         if ((etherInputHookRtn != NULL) &&
*             (* etherInputHookRtn) (&ls->ls_if, (char *)eh, len))
*             {
*             return; /@ input hook has already processed this packet @/
*             }
* .CE
*
* RETURNS: OK if the hook was added, or ERROR otherwise.
*/

STATUS etherInputHookAdd
    (
    FUNCPTR inputHook,   /* routine to receive Ethernet input */
    char* pName,         /* name of device if MUX/END is being used */
    int unit             /* unit of device if MUX/END is being used */
    )
    {
    HOOK_ENTRY *pHookEnt;
    HOOK_ENTRY *pHookCurr;
    void * pBinding = NULL;  /* Existing END device binding, if any. */

    BOOL tkDevice=FALSE;

    if (pName != NULL) /* We are dealing with an END. */
        {
        if (etherInputHookActive == FALSE)     /* First END driver hook? */
            {
            if (etherInputHookRtn == NULL)
                {
                /* Initialize list - first network driver of either type. */
 
                lstInit (&inputHookList);
                }
            etherInputHookActive = TRUE;
            }

        /* Check if bind is necessary and eliminate duplicate hook routine. */

        for (pHookCurr = (HOOK_ENTRY *)lstFirst(&inputHookList); 
             pHookCurr != NULL;
             pHookCurr = (HOOK_ENTRY *)lstNext(&pHookCurr->node))
            {
            /* Ignore BSD device hook entries. */

            if (pHookCurr->pCookie == NULL)
                continue;

            if (STREQ(pHookCurr->name, pName) && (pHookCurr->unit == unit))
                {
                if (pHookCurr->routine == inputHook)
                    return (ERROR);

                /* Additional hook for same device - reuse binding. */

                pBinding = pHookCurr->pCookie;
                }
            }
        pHookEnt = malloc (sizeof (HOOK_ENTRY));
        if (pHookEnt == NULL)
            return (ERROR);
        bzero ( (char *)pHookEnt, sizeof (HOOK_ENTRY));

        if (pBinding == NULL) 	/* No hook entry for this END device? */
            {
            /* Attach Ethernet input hook handler for this device. */

	    tkDevice = muxTkDrvCheck (pName);
            if (tkDevice)
                {
                pBinding = muxTkBind (pName, unit, nptEtherInputHookRtn, 
                                      NULL, NULL, NULL, MUX_PROTO_SNARF, 
                                      "etherInputHook", pHookEnt, NULL, NULL);
                }
            else
                {
                pBinding = muxBind (pName, unit, endEtherInputHookRtn, 
                                    NULL, NULL, NULL, MUX_PROTO_SNARF, 
                                    "etherInputHook", pHookEnt);
                }

            if (pBinding == NULL)
                {
                free (pHookEnt);
                return (ERROR);
                }
            }

        /*
         * Assign (new or existing) handler attachment for the device,
         * allowing hook deletion in any order.
         */

        pHookEnt->pCookie = pBinding;

        strcpy (pHookEnt->name, pName);
        pHookEnt->unit = unit;
        pHookEnt->routine = inputHook;
        lstAdd (&inputHookList, &pHookEnt->node);
        }
    else               /* Old style driver. */
        {
        /* Check for duplicate hook routine. */

        for (pHookCurr = (HOOK_ENTRY *)lstFirst(&inputHookList);
             pHookCurr != NULL;
             pHookCurr = (HOOK_ENTRY *)lstNext(&pHookCurr->node))
            {
            if (pHookCurr->pCookie)    /* Ignore END device hook entries. */
                continue;

            if (pHookCurr->routine == inputHook)
                return (ERROR);
            }

        pHookEnt = malloc(sizeof(HOOK_ENTRY));
        if (pHookEnt == NULL)
            return (ERROR);
        bzero ( (char *)pHookEnt, sizeof (HOOK_ENTRY));

        if (etherInputHookRtn == NULL)    /* First BSD driver hook? */
            {
            etherInputHookRtn = etherInputHook;

            if (!etherInputHookActive)
                {
                /* Initialize list - first network driver of either type. */

                lstInit (&inputHookList);
                }
            }
        pHookEnt->routine = inputHook;
        lstAdd(&inputHookList, &pHookEnt->node);
        }
    return (OK);
    }

/*******************************************************************************
*
* etherInputHookDelete - delete a network interface input hook routine
*
* This routine deletes a network interface input hook.
*
* RETURNS: N/A
*/

void etherInputHookDelete
    (
    FUNCPTR inputHook,
    char *pName,
    int unit
    )
    {
    HOOK_ENTRY *pHookEnt;
    HOOK_ENTRY *pTarget = NULL;

    BOOL unbindFlag = TRUE;    /* Remove handler if hook found? */

    if (pName != NULL)                       /* END case */
        {
        for (pHookEnt = (HOOK_ENTRY *)lstFirst(&inputHookList);
             pHookEnt != NULL;
             pHookEnt = (HOOK_ENTRY *)lstNext(&pHookEnt->node))
            {
            /* Ignore BSD device hook entries. */

            if (pHookEnt->pCookie == NULL)
                continue;

            if (STREQ(pHookEnt->name, pName) && (pHookEnt->unit == unit))
                {
                if (pHookEnt->routine == inputHook)
                    {
                    /*
                     * Found matching hook entry to remove. Keep searching
                     * for other hooks on this device if needed.
                     */

                    pTarget = pHookEnt;
                    if (!unbindFlag)    /* Another hook already found? */
                        break;
                    }
                else
                    {
                    /*
                     * Different hook on same driver - do not unbind.
                     * Stop searching if target hook entry already found.
                     */

                    unbindFlag = FALSE;
                    if (pTarget)
                        break;
                    }
                }
            }

        if (pTarget)    /* Remove hook entry if match found. */
            {
            if (unbindFlag)   /* Remove binding if last hook for device. */
                {
                if (muxTkDrvCheck (pName))
                    {
                    muxUnbind (pTarget->pCookie, MUX_PROTO_SNARF,
                               nptEtherInputHookRtn);
                    }
                else
                    {
                    muxUnbind (pTarget->pCookie, MUX_PROTO_SNARF,
                               endEtherInputHookRtn);
                    }
                }
            lstDelete (&inputHookList, &pTarget->node);
            free (pTarget);
            }
        }
    else                                     /* 4.4 case */
        {
        for (pHookEnt = (HOOK_ENTRY *)lstFirst(&inputHookList);
             pHookEnt != NULL; pHookEnt = pTarget)
            {
            if (pHookEnt->pCookie)    /* Ignore END device hook entries. */
                continue;

            pTarget = (HOOK_ENTRY *)lstNext(&pHookEnt->node);
            if (pHookEnt->routine == inputHook)
                {
                lstDelete(&inputHookList, &pHookEnt->node);
                free(pHookEnt);
                }
            }
        }
    
    if (lstCount(&inputHookList) <= 0)
        {
        etherInputHookActive = FALSE;     /* No END driver hooks installed. */
        etherInputHookRtn = NULL;         /* No BSD driver hooks installed. */
        lstFree (&inputHookList);
        }
    }


/*******************************************************************************
*
* etherOutputHookAdd - add a routine to receive all Ethernet output packets
*
* This routine adds a hook routine that will be called for every Ethernet
* packet that is transmitted.
*
* The calling sequence of the output hook routine is:
* .CS
*     BOOL outputHook
*         (
*         struct ifnet *pIf,    /@ interface packet will be sent on @/
*         char         *buffer, /@ packet to transmit               @/
*         int          length   /@ length of packet to transmit     @/
*         )
* .CE
* The hook is called immediately before transmission.  The hook routine
* should return TRUE if it has handled the output packet and no further
* action should be taken with it.  It should return FALSE if it has not
* handled the output packet and normal transmission should take place.
*
* The Ethernet packet data is in a temporary buffer when the hook routine
* is called.  This buffer will be reused upon return from the hook.  If
* the hook routine needs to retain the output packet, it should be copied
* elsewhere.
*
* IMPLEMENTATION
* A call to the function pointed to be the global function pointer
* `etherOutputHookRtn' should be invoked in the transmit routine of every
* network driver providing this service.  For example:
* .ne 14
* .CS
*     ...
*     #include "etherLib.h"
*     ...
*     xxxStartOutput ()
*     /@ call output hook if any @/
*     if ((etherOutputHookRtn != NULL) &&
*          (* etherOutputHookRtn) (&ls->ls_if, buf0, len))
*         {
*         /@ output hook has already processed this packet @/
*         }
*     else
*     ...
* .CE
*
* RETURNS: OK if the hook was added, or ERROR otherwise.
*/

STATUS etherOutputHookAdd
    (
    FUNCPTR outputHook  /* routine to receive Ethernet output */
    )
    {
    HOOK_ENTRY *pHookEnt;
    
    pHookEnt = malloc(sizeof(HOOK_ENTRY));
    if (pHookEnt == NULL)
        return (ERROR);

    if (etherOutputHookRtn == NULL)
        {
        etherOutputHookRtn = etherOutputHook;
        lstInit(&outputHookList);
        }
    pHookEnt->routine = outputHook;
    lstAdd(&outputHookList, &pHookEnt->node);
    return (OK);
    }

/*******************************************************************************
*
* etherOutputHookDelete - delete a network interface output hook routine
*
* This routine deletes a network interface output hook, which must be supplied
* as the only argument.
*
* RETURNS: N/A
*/

void etherOutputHookDelete
    (
    FUNCPTR outputHook
    )
    {

    HOOK_ENTRY *pHookEnt;
    extern LIST outputHookList;
    NODE index;

    for (pHookEnt = (HOOK_ENTRY *)lstFirst(&outputHookList); 
         pHookEnt != NULL;
         pHookEnt = (HOOK_ENTRY *)lstNext(&index))
        {
        index = pHookEnt->node;
        if (pHookEnt->routine == outputHook)
            {
            lstDelete(&outputHookList, &pHookEnt->node);
            free(pHookEnt);
            }
        }
    
    if (lstCount(&outputHookList) <= 0)
        {
        etherOutputHookRtn = NULL;
        lstFree(&outputHookList);
        }
    
    }

/*******************************************************************************
*
* etherAddrResolve - find an Ethernet address for a specified Internet address
*
* This routine uses the Address Resolution Protocol (ARP) and internal ARP
* cache to resolve the Ethernet address of a machine that owns the Internet
* address given in <targetAddr>.
*
* The first argument <pIf> is a pointer to a variable of type `struct ifnet'
* which identifies the network interface through which the ARP request messages
* are to be sent out.  The routine ifunit() is used to retrieve this pointer
* from the system in the following way:
* .CS
*     struct ifnet *pIf;
*     ...
*     pIf = ifunit ("ln0");
* .CE
* If ifunit() returns a non-NULL pointer, it is a valid pointer to
* the named network interface device structure of type `struct ifnet'.
* In the above example, <pIf> will be pointing to the data structure that
* describes the first LANCE network interface device if ifunit() is
* successful.
*
* The six-byte Ethernet address is copied to <eHdr>, if the resolution of
* <targetAddr> is successful.  <eHdr> must point to a buffer of at least
* six bytes.
*
* RETURNS:
* OK if the address is resolved successfully, or ERROR if <eHdr> is NULL,
* <targetAddr> is invalid, or address resolution is unsuccessful.
*
* SEE ALSO:
* etherOutput()
*/

STATUS etherAddrResolve
    (
    struct ifnet        *pIf,           /* interface on which to send ARP req */
    char                *targetAddr,    /* name or Internet address of target */
    char                *eHdr,          /* where to return the Ethernet addr */
    int                 numTries,       /* number of times to try ARPing */
    int                 numTicks        /* number of ticks between ARPing */
    )
    {
    struct sockaddr_in  sockInetAddr;
    unsigned long	addr;
    int			retVal = 0;

    if (eHdr == NULL)		/* user messed up */
        return (ERROR);

    /* the 'targetAddr' can either be the hostname or the actual Internet
     * address.
     */
    if ((addr = (unsigned long) hostGetByName (targetAddr)) == ERROR &&
	(addr = inet_addr (targetAddr)) == ERROR)
	return (ERROR);

    sockInetAddr.sin_len = sizeof(struct sockaddr_in);
    sockInetAddr.sin_family = AF_INET;
    sockInetAddr.sin_addr.s_addr = addr; 

    /* return 0xffffffffffff for broadcast Internet address */

    if (in_broadcast (sockInetAddr.sin_addr, pIf))
	{
        bcopy ((char *) etherbroadcastaddr, eHdr, sizeof (etherbroadcastaddr));

	return (OK);
	}

    /* 
     * Try to resolve the Ethernet address by calling arpresolve() which
     * may send ARP request messages out onto the Ethernet wire.
     */

    while ((numTries == -1 || numTries-- > 0) &&
	   (retVal = arpresolve ((struct arpcom *) pIf, 
				 (struct rtentry *)NULL, 
				 (struct mbuf *) NULL,
				 (struct sockaddr *)&sockInetAddr,
				 (UCHAR *)eHdr))
	   == 0)
    	taskDelay (numTicks);

    if (retVal == 0)		/* unsuccessful resolution */
        return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* etherTypeGet - get the type from an ethernet packet
*
* This routine returns a short that is the ethertype (defined in RFC
* 1700) from either an 802.3 addressed packet or an RFC 894 packet.
* Most packets are encoded as described in RFC 894 but 802.3 addressing
* is also recognized.
* 
* RETURNS: A USHORT value that is the ethertype, or 0 on error.
*
* SEE ALSO:
* .I "RFC 894, TCP/IP Illustrated,"
* Volume 1, by Richard Stevens.
*/

USHORT etherTypeGet
    (
    char *pPacket		/* pointer to the beginning of the packet */
    )
    {

    ENET_HDR* pEnetHdr;
    struct llc* pLLCHdr;
    USHORT ether_type;

    /* Try for RFC 894 first as it's the most common. */
    pEnetHdr = (ENET_HDR *)pPacket;
    ether_type = ntohs(pEnetHdr->type);

    /* Deal with 802.3 addressing. */

    /* Here is the algorithm. */
    /* If the ether_type is less than the MTU then we know that */
    /* this is an 802.x address from RFC 1700. */
    if (ether_type < ETHERMTU)
	{
	pLLCHdr = (struct llc *) pPacket;

	/* Now it may be IP over 802.x so we check to see if the */
	/* destination SAP is IP, if so we snag the ethertype from the */
	/* proper place. */

	/* Now if it's NOT IP over 802.x then we just used the DSAP as */
	/* the ether_type.  */

	if (pLLCHdr->llc_dsap == LLC_SNAP_LSAP)
	    ether_type = ntohs(pLLCHdr->llc_un.type_snap.ether_type);
	else
	    ether_type = pLLCHdr->llc_dsap;
	}

    /* Finally!  Now, who says we should have standards comittees? */
    /* Wouldn't it be easier, and cheaper just to shoot these losers? */

    return (ether_type);

    }

/******************************************************************************
*
* etherInputHook - the system etherInputHook
*
* This routine multiplexes packets to multiple input hooks.
*
* RETURNS: TRUE if one of the routines that it called returns TRUE indicating
* that the hook routine "ate" the packet, otherwise it returns FALSE.
*
* NOMANUAL
*
*/

LOCAL BOOL etherInputHook
    (
    struct ifnet *pIf, 
    char *buffer, 
    int length
    )
    {

    HOOK_ENTRY *pHookEnt;
    extern LIST inputHookList;

    for (pHookEnt = (HOOK_ENTRY *)lstFirst(&inputHookList); 
         pHookEnt != NULL; pHookEnt = (HOOK_ENTRY *)lstNext(&pHookEnt->node))
	{
        if ((* pHookEnt->routine) (pIf, buffer, length))
            return (TRUE);
	}

    return (FALSE);

    }

/******************************************************************************
*
* etherOutputHook - the system etherOutputHook
*
* This routine is used to multiplex access to output hooks.
*
* RETURNS: TRUE if one of its callee consumed the packet otherwise returns
* FALSE.
*
* NOMANUAL
*/

LOCAL BOOL etherOutputHook
    (
    struct ifnet *pIf, 
    char *buffer, 
    int length
    )
    {

    HOOK_ENTRY* pHookEnt;
    extern LIST outputHookList;

    for (pHookEnt = (HOOK_ENTRY *)lstFirst(&outputHookList); 
         pHookEnt != NULL; pHookEnt = (HOOK_ENTRY *)lstNext(&pHookEnt->node))
	{
        if ((* pHookEnt->routine) (pIf, buffer, length))
            return (TRUE);
	}

    return (FALSE);

    }

LOCAL BOOL endEtherInputHookRtn
    (
    void * 	pCookie,
    long 	type,
    M_BLK_ID 	pMblk, 		/* Received frame (with link-level header). */
    LL_HDR_INFO * pLinkHdrInfo,
    void * 	pSpare 		/* Extra protocol data. Unused. */
    )
    {
    HOOK_ENTRY * pHookEnt = (HOOK_ENTRY*) pSpare;
    char devName[32];
    char packetData [ETHERMTU + SIZEOF_ETHERHEADER];
    struct ifnet fakeIF;
    extern LIST inputHookList;
    int flags;
    int len;
    END_OBJ* pEnd=(END_OBJ*)pCookie;
    int ifHdrLen=0;

    bzero ( (char *)&fakeIF, sizeof (fakeIF));
    fakeIF.if_name = &devName[0];
    strcpy (fakeIF.if_name, pEnd->devObject.name);
    fakeIF.if_unit = pEnd->devObject.unit;
    muxIoctl (pHookEnt->pCookie, EIOCGFLAGS, (caddr_t)&flags);
    fakeIF.if_flags = flags;
    
    /* Get the Link Level header length . */

    if (muxIoctl (pHookEnt->pCookie, EIOCGHDRLEN, (caddr_t)&ifHdrLen) != OK)
        fakeIF.if_hdrlen = 0;
    else
        fakeIF.if_hdrlen = (UCHAR)ifHdrLen;

    len = netMblkToBufCopy(pMblk, (char *)packetData, NULL);
    for (pHookEnt = (HOOK_ENTRY *)lstFirst (&inputHookList); 
         pHookEnt != NULL; pHookEnt = (HOOK_ENTRY *)lstNext (&pHookEnt->node))
        {
        if ( (* pHookEnt->routine) (&fakeIF, packetData, len))
            {
            netMblkClChainFree (pMblk);	/* hook has consumed the packet */
            return (TRUE);
            }
        }
    return (FALSE);
    }

/********************************************************************************
*
* nptEtherInputHookRtn- EtherInputHook Receive Routine
*
* This is the receive routine for the etherInputHook while binding to an NPT 
* device. This is very similar to endEtherInputHook except that it takes a
* different arguments.
*
*/

LOCAL BOOL nptEtherInputHookRtn
    (
    void *      pCallBackId,    /* Sent down in muxTkBind(); same as pSpare 
                                   as in muxBind() 
                                 */
    long        type,           /* SNARF */
    M_BLK_ID    pMblk,          /* Received frame (with link-level header). */
    void *      pSpareData      /* Extra protocol data */
    )
    {
    HOOK_ENTRY * pHookEnt = (HOOK_ENTRY*) pCallBackId;
    char devName[32];
    char packetData [ETHERMTU + SIZEOF_ETHERHEADER];
    struct ifnet fakeIF;
    extern LIST inputHookList;
    int flags;
    int len;
    END_OBJ* pEnd=PCOOKIE_TO_ENDOBJ(pHookEnt->pCookie);
    int ifHdrLen=0;


    bzero ( (char *)&fakeIF, sizeof (fakeIF));
    fakeIF.if_name = &devName[0];
    strcpy (fakeIF.if_name, pEnd->devObject.name);
    fakeIF.if_unit = pEnd->devObject.unit;
    muxIoctl (pHookEnt->pCookie, EIOCGFLAGS, (caddr_t)&flags);
    fakeIF.if_flags = flags;

    /* Get the Link Level header length . */

    if (muxIoctl (pHookEnt->pCookie, EIOCGHDRLEN, (caddr_t)&ifHdrLen) != OK)
        fakeIF.if_hdrlen = 0;
    else
        fakeIF.if_hdrlen = (UCHAR)ifHdrLen;

    len = netMblkToBufCopy(pMblk, (char *)packetData, NULL);
    for (pHookEnt = (HOOK_ENTRY *)lstFirst (&inputHookList);
         pHookEnt != NULL; pHookEnt = (HOOK_ENTRY *)lstNext (&pHookEnt->node))
        {
        if ( (* pHookEnt->routine) (&fakeIF, packetData, len))
            {
            netMblkClChainFree (pMblk); /* hook has consumed the packet */
            return (TRUE);
            }
        }
    return (FALSE);
    }


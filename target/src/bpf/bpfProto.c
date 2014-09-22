/* bpfProto.c - Berkeley Packet Filter (BPF) protocol interface */

/* Copyright 1999 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*-
 * Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995, 1996
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from the Stanford/CMU enet packet filter,
 * (net/enet.c) distributed as part of 4.3BSD, and code contributed
 * to Berkeley by Steven McCanne and Van Jacobson both of Lawrence
 * Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 *	@(#)bpf.c	7.5 (Berkeley) 7/15/91
 *
 * static char rcsid[] =
 * "$Header: bpf.c,v 1.67 96/09/26 22:00:52 leres Exp $";
 */

/*
modification history
--------------------
01e,13nov01,wap  added support for NPT Drivers and allow for qtag support.
01d,26jan01,ijm  corrected leak in bpfBsdProtoInput
01c,17nov00,spm  added support for BSD Ethernet devices
01b,26oct00,spm  fixed merge from tor3_x branch and updated mod history
01a,23oct00,niq  created from version 01c of tor3_x branch
*/

#define INCLUDE_IP     /* hack to have ipProto filter out promisc pkts */

/* includes */

#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"

#include "lstLib.h"
#include "logLib.h"

#include "netLib.h"
#include "muxLib.h"
#include "muxTkLib.h"
#include "netBufLib.h"
#include "end.h"
#include "etherLib.h"

#include "private/muxLibP.h"    /* PCOOKIE_TO_ENDOBJ conversion macro */

#include "private/bpfLibP.h"
#include "net/bpf.h"

#ifdef INCLUDE_IP
#include "ifLib.h"
#endif

/* defines */


#define BPF_PROTO_NAME "Berkeley Packet Filter"

#define DEBUG

#ifdef DEBUG
#undef LOCAL
#define LOCAL
#endif  /* DEBUG */

/* locals */

LOCAL BOOL bpfProtoInitDone = FALSE;   /* TRUE if BPF MUX proto */
                                       /* has been initialized */
LOCAL LIST lstProtoInUse;    /* attached network interfaces */

LOCAL int bpfDltTbl [] =         /* Translation tbl from m2 ifType to DLT */
    {
	DLT_NULL,
	DLT_NULL,      /* M2_ifType_other */
	DLT_NULL,      /* M2_ifType_regular1822 */
	DLT_NULL,      /* M2_ifType_hdh1822 */
	DLT_AX25,      /* M2_ifType_ddn_x25 */
	DLT_AX25,      /* M2_ifType_rfc877_x25 */
	DLT_EN10MB,    /* M2_ifType_ethernet_csmacd */
	DLT_IEEE802,   /* M2_ifType_iso88023_csmacd */
	DLT_IEEE802,   /* M2_ifType_iso88024_tokenBus */
	DLT_IEEE802,   /* M2_ifType_iso88025_tokenRing */
	DLT_IEEE802,   /* M2_ifType_iso88026_man */
	DLT_NULL,      /* M2_ifType_starLan */
	DLT_PRONET,    /* M2_ifType_proteon_10Mbit */
	DLT_PRONET,    /* M2_ifType_proteon_80Mbit */
	DLT_NULL,      /* M2_ifType_hyperchannel */
	DLT_FDDI,      /* M2_ifType_fddi */
	DLT_NULL,      /* M2_ifType_lapb */
	DLT_NULL,      /* M2_ifType_sdlc */
	DLT_NULL,      /* M2_ifType_ds1 */
	DLT_NULL,      /* M2_ifType_e1 */
	DLT_NULL,      /* M2_ifType_basicISDN */
	DLT_NULL,      /* M2_ifType_primaryISDN */
	DLT_NULL,      /* M2_ifType_propPointToPointSerial */
	DLT_PPP,       /* M2_ifType_ppp */
	DLT_NULL,      /* M2_ifType_softwareLoopback */
	DLT_NULL,      /* M2_ifType_eon */
	DLT_EN3MB,     /* M2_ifType_ethernet_3Mbit */
	DLT_NULL,      /* M2_ifType_nsip */
	DLT_SLIP,      /* M2_ifType_slip */
	DLT_NULL,      /* M2_ifType_ultra */
	DLT_NULL,      /* M2_ifType_ds3 */
	DLT_NULL,      /* M2_ifType_sip */
	DLT_NULL,      /* M2_ifType_frame_relay */
    };

LOCAL int bpfDltTblSize = NELEMENTS (bpfDltTbl);  /* size of BPF DLT   */
                                                  /* translation table */

LOCAL int bpfBsdDevCount = 0;  /* BPF devices attached to BSD devices. */

/* forward declarations */

LOCAL BOOL bpfProtoInput (void * pCookie, long type, M_BLK_ID pMBlk,
			  LL_HDR_INFO * pLlHdrInfo, 
			  void * pProtoCtrl);
LOCAL BOOL bpfBsdProtoInput (struct ifnet * pIf, char * pBuffer, int length);

LOCAL STATUS bpfProtoShutdown (void * pCookie, void * pProtoCtrl);

LOCAL STATUS bpfTkProtoShutdown (void * pProtoCtrl);

LOCAL BOOL bpfTkProtoInput (void * pProtoCtrl, long type, M_BLK_ID pMBlk,
                            void * pSpare);
    /*
     * Called with global BPF lock to prevent conflicts with attach routine
     * and promiscuous setting. This routine removes the last attachment
     * of a BPF device from a network interface and also detaches the
     * corresponding BPF MUX protocols.
     */

STATUS _bpfProtoDetach
    (
    BPF_DEV_CTRL * pBpfDev 	/* BPF device control structure */
    )
    {
    BPF_PROTO_CTRL * pNew;      /* BPF units from other devices, if any. */

    BOOL bsdFlag; 	/* Attached to BSD or END device? */

    bsdFlag = pBpfDev->pNetDev->bsdFlag;

    /*
     * Find a BPF device using the same
     * network interface as this unit.
     */

    for (pNew = (BPF_PROTO_CTRL *)lstFirst (&lstProtoInUse);
         pNew != NULL;
         pNew = (BPF_PROTO_CTRL *)lstNext ((NODE *)pNew))
        {
        if (bsdFlag)
            {
            if (pNew->pCookie == pBpfDev->pNetDev->pCookie)
                break;
            }
        else if (PCOOKIE_TO_ENDOBJ(pNew->pCookie) ==
                    PCOOKIE_TO_ENDOBJ(pBpfDev->pNetDev->pCookie))
            break;
        }

    if (pNew == NULL)    /* No such device attached? (Can't really occur). */
        return (ERROR);

    if (pNew != pBpfDev->pNetDev)
        {
        /*
         * The given BPF unit is part of a BPF device which shares the
         * same network interface as an earlier BPF device. Just remove
         * it from the private list.
         */

        lstDelete (& (pNew->bpfDevList), (NODE *) (pBpfDev->pNetDev));
        }
    else
        {
        /*
         * The given BPF unit is part of the first BPF device which
         * used the network interface. Update the global list if other
         * BPF devices are using the same network interface.
         */

        if (lstCount (& (pNew->bpfDevList)) != 0)
            {
            /* Move the first shared device element to the global list. */

            pNew = (BPF_PROTO_CTRL *)lstGet (& (pBpfDev->pNetDev->bpfDevList));
            if (pNew != NULL)
                {
                lstInsert (&lstProtoInUse, (NODE *) (pBpfDev->pNetDev),
                           (NODE *)pNew);

                /* Move other attached devices to the (new) private list. */

                lstConcat (& (pNew->bpfDevList),
                           & (pBpfDev->pNetDev->bpfDevList));
                }
            }

        /* Remove the (old) root of the private list from the global list. */

        lstDelete (&lstProtoInUse, (NODE *) (pBpfDev->pNetDev));
        }

    /*
     * No BPF units from the old BPF device are using the
     * network interface. Remove any attached MUX protocols.
     * Alternatively, remove the Ethernet input hook if no
     * other BPF devices are using any BSD network interface.
     */

    if (pBpfDev->pNetDev->bsdFlag == TRUE)
        {
        bpfBsdDevCount--;
        if (bpfBsdDevCount == 0)
            etherInputHookDelete (bpfBsdProtoInput, NULL, 0);
        }
    else if (pBpfDev->pNetDev->pCookie != NULL)
        {

        int deviceType = muxTkDrvCheck (pBpfDev->pNetDev->devName);

        if (deviceType != ERROR)
            {
            if ( deviceType == 0 )
                muxUnbind (pBpfDev->pNetDev->pCookie, MUX_PROTO_SNARF,
                       (FUNCPTR) bpfProtoInput);
            else
                muxUnbind (pBpfDev->pNetDev->pCookie, MUX_PROTO_SNARF,
                       (FUNCPTR) bpfTkProtoInput);
            }
        }
    return (OK);
    }

LOCAL STATUS bpfProtoShutdown 
    (
    void * pCookie,
    void * pProtoCtrl
    )
    {
    BPF_PROTO_CTRL * pBpfProto;
    BPF_DEV_CTRL * pBpfDev;

    pBpfProto = (BPF_PROTO_CTRL *) pProtoCtrl;

    for (pBpfDev = (BPF_DEV_CTRL *) lstFirst (& (pBpfProto->bpfUnitList));
         pBpfDev != NULL;
         pBpfDev = (BPF_DEV_CTRL *) lstNext ((NODE *) pBpfDev))
        {
        _bpfDevDetach (pBpfDev);
        }
    
    return (OK);
    }

    /*
     * Check incoming frame against every BPF unit attached to a network
     * device using the MUX/END interface. The BPF device containing the
     * attachment data must still exist or the MUX would not call this
     * routine. Always return FALSE to allow normal stack processing after
     * applying any filters.
     */

LOCAL BOOL bpfProtoInput 
    (
    void * pCookie, 
    long type,
    M_BLK_ID pMBlk,
    LL_HDR_INFO * pLlHdrInfo, 
    void * pProtoCtrl
    )
    {
    BPF_PROTO_CTRL * pNetDev;

    /* Access a BPF unit's attachment to the network interface. */

    pNetDev = (BPF_PROTO_CTRL *)pProtoCtrl;
    if (pNetDev->pCookie != pCookie)
        {
        /* Mismatched binding? Should never occur. */

        return (FALSE);
        }

    /* Apply the current filters and save matching frames. */

    _bpfPacketTap (& (pNetDev->bpfUnitList), type, pMBlk,
                    pLlHdrInfo->dataOffset);

    return (FALSE);
    }

    /*
     * Check incoming frame against any BPF unit attached to the
     * network device using the BSD interface. Always return FALSE
     * to allow normal stack processing after applying any filters.
     */

LOCAL BOOL bpfBsdProtoInput 
    (
    struct ifnet * pIf, 	/* interface which received frame */
    char * pBuffer, 		/* pointer to received frame */
    int length 			/* length of received frame */
    )
    {
    BPF_PROTO_CTRL * pFirst;
    BPF_PROTO_CTRL * pNext;
    M_BLK_ID pMbuf;

    /* Find any BPF units attached to the network interface. */

    for (pFirst = (BPF_PROTO_CTRL *)lstFirst (&lstProtoInUse);
         pFirst != NULL;
         pFirst = (BPF_PROTO_CTRL *)lstNext ((NODE *)pFirst))
        {
        /* Ignore attachments to END network interfaces. */

        if (pFirst->bsdFlag == FALSE)
            continue;

        /* Ignore attachments to different BSD network interfaces. */

        if (pFirst->pCookie == pIf)
            break;
        }

     /*
      * Exit if no BPF device is using this BSD network interface.
      * (This situation can occur frequently since all devices call
      * this routine with incoming data as long as any input hook is
      * installed).
      */

    if (pFirst == NULL)
        return (FALSE);

    /*
     * Build an mBlk chain containing the data.
     * (This step is wasteful since the driver does the same thing,
     * but the Ethernet hook interface doesn't allow access to that
     * mBlk and the BPF implementation requires one).
     */

    pMbuf = bcopy_to_mbufs (pBuffer, length, 0, pIf, NONE);
    if (pMbuf == NULL)
        return (FALSE);

    /* Apply the current filters and save matching frames. */

    _bpfPacketTap (& (pFirst->bpfUnitList), ETHERTYPE_IP, pMbuf,
                    SIZEOF_ETHERHEADER);

    for (pNext = (BPF_PROTO_CTRL *)lstFirst (& (pFirst->bpfDevList));
         pNext != NULL;
         pNext = (BPF_PROTO_CTRL *)lstNext ((NODE *)pNext))
        {
        _bpfPacketTap (& (pNext->bpfUnitList), ETHERTYPE_IP, pMbuf,
                        SIZEOF_ETHERHEADER);
        }

    /* Free the buffer */

    netMblkClChainFree(pMbuf);

    return (FALSE);
    }

/*
 * Make pBpfDev listen on named interface. 
 * Must be called by a routine holding the global BPF lock.
 */

STATUS _bpfProtoAttach
    (
    BPF_DEV_CTRL * pBpfDev,
    char * 	pDevName, 	/* Name of network interface device. */
    int 	devUnit, 	/* Unit number of network interface. */
    struct ifnet * pIf 		/* Interface handle for BSD device. */
    )
    {
    M2_INTERFACETBL m2Tbl;
   
    BPF_DEV_CTRL *   pBpfUnit;

    BPF_PROTO_CTRL * pNetIfCtrl;
    BPF_PROTO_CTRL * pFirst;
    BPF_PROTO_CTRL * pLast = NULL;
    NODE tmpNode;

    /* Check if a BPF device is already attached to the network interface. */

    for (pNetIfCtrl = (BPF_PROTO_CTRL *)lstFirst (&lstProtoInUse);
         pNetIfCtrl != NULL;
         pNetIfCtrl = (BPF_PROTO_CTRL *) lstNext ((NODE *) pNetIfCtrl))
        {
        if (devUnit == pNetIfCtrl->devUnit &&
            strncmp (pDevName, pNetIfCtrl->devName, END_NAME_MAX) == 0)
            break;
        }

    if (pNetIfCtrl == NULL)
        {
        /* Add this BPF unit's attachment data to the global list. */

        pFirst = pBpfDev->pNetIfCtrl;
        if (pFirst == NULL)
            return (ENOBUFS);

        lstAdd (&lstProtoInUse, (NODE *)pFirst);
        pLast = pFirst;
        }
    else
        {
        /*
         * At least one BPF device is already attached to the interface.
         * Find the attached parent device for this BPF unit, if any.
         */

        pFirst = pNetIfCtrl;
        pBpfUnit = (BPF_DEV_CTRL *)lstFirst ( &(pNetIfCtrl->bpfUnitList));
        if (pBpfUnit == NULL)
            return (ENOBUFS);

        if (pBpfDev->bpfDevId != pBpfUnit->bpfDevId)
            {
            /*
             * This BPF unit is not from the first attached device.
             * Search for a matching shared device.
             */

            pNetIfCtrl = (BPF_PROTO_CTRL *)lstFirst (& (pFirst->bpfDevList));
            while (pNetIfCtrl != NULL)
                {
            pBpfUnit = (BPF_DEV_CTRL *)lstFirst ( &(pNetIfCtrl->bpfUnitList));
            if (pBpfUnit == NULL)
                return (ENOBUFS);

                if (pBpfDev->bpfDevId == pBpfUnit->bpfDevId)
                    break;

                pLast = pNetIfCtrl;
                pNetIfCtrl = (BPF_PROTO_CTRL *)lstNext ((NODE *) pNetIfCtrl);
                }
            }

        if (pNetIfCtrl != NULL)
            {
            /*
             * Parent device found. Save the network interface attachment in
             * the BPF unit entry, which will be appended to the existing list.
             */

            pBpfDev->pNetDev = pNetIfCtrl;

            return (OK);
            }

        /*
         * No other BPF unit from this device is attached to the
         * given network interface. Add the unit's entry to the
         * (private) list of BPF devices which share a network interface.
         */

        if (pBpfDev->pNetIfCtrl == NULL)
            return (ENOBUFS);

        /* Append the new attachment data to the private list. */

        lstInsert (& (pFirst->bpfDevList), (NODE *)pLast,
                   (NODE *)pBpfDev->pNetIfCtrl);
        pLast = pBpfDev->pNetIfCtrl;
        }

    /*
     * Attach the BPF MUX protocols and initialize the attachment data
     * for a network interface attached to the global or a private list.
     * Alternatively, use the existing Ethernet input hook attachment
     * or add the input hook if no other BPF devices are using any BSD
     * network interface.
     */

    /* Delete the data in this node, but don't clobber list pointers. */

    tmpNode.next = pLast->bpfProtoLstNode.next;
    tmpNode.previous = pLast->bpfProtoLstNode.previous;
    bzero ( (char *)pLast, sizeof (BPF_PROTO_CTRL));
    pLast->bpfProtoLstNode.next = tmpNode.next;
    pLast->bpfProtoLstNode.previous = tmpNode.previous;

    if (pIf)
        {
        /* Attaching a BPF device to a BSD network interface. */

        bpfBsdDevCount++;
        pLast->bsdFlag = TRUE;
        pLast->pCookie = pIf;
        }
    else
        {

        /* Attach to MUX to receive incoming packets */

        int deviceType = muxTkDrvCheck (pDevName);

        if (deviceType == ERROR)
            goto bpfProtoAttachErr;
/*logMsg("BPF PROTO ATTACH: %d\n", deviceType, 0, 0, 0, 0, 0);*/
        pLast->bsdFlag = FALSE;
        if (deviceType == 0)
            pLast->pCookie = muxBind (pDevName, devUnit, bpfProtoInput, 
                                      bpfProtoShutdown, NULL, NULL, 
                                      MUX_PROTO_SNARF, BPF_PROTO_NAME, 
                                      pLast);
        else
            pLast->pCookie = muxTkBind (pDevName, devUnit, bpfTkProtoInput, 
                                        bpfTkProtoShutdown, NULL, NULL, 
                                        MUX_PROTO_SNARF, BPF_PROTO_NAME, pLast,
                                        NULL, NULL);

        if (pLast->pCookie == NULL)
            {
            goto bpfProtoAttachErr;
            }
        }

    /* Initialize list of attached BPF units and store interface name. */

    lstInit (& (pLast->bpfUnitList));
    strncpy (pLast->devName, pDevName, END_NAME_MAX);
    pLast->devUnit = devUnit;

    /* Reset the private attachment list for possible later use. */

    lstInit (& (pLast->bpfDevList));

    /* Copy remaining interface settings if possible. */

    if (pFirst != pLast)
        {
        /*
         * More than one BPF device is sharing the same network interface.
         * Copy the common values from the first attachment.
         */

        pLast->mtuSize = pFirst->mtuSize;
        pLast->dataLinkType = pFirst->dataLinkType;
        }
    else
        {
        /*
         * The BPF unit is the first attachment to the network interface.
         * Retrieve and save the interface information.
         * Add the Ethernet input hook if no other BSD device is in use.
         */

        if (pLast->bsdFlag)
            {
            if (bpfBsdDevCount == 1)
                {
                if (etherInputHookAdd (bpfBsdProtoInput, NULL, 0) == ERROR)
                    goto bpfProtoAttachErr;
                }
            pLast->mtuSize = ETHERMTU;
            pLast->dataLinkType = DLT_EN10MB;
            }
        else
            {
            if (muxIoctl (pLast->pCookie, EIOCGMIB2, (char *)&m2Tbl) != OK)
                {
                goto bpfProtoAttachErr;
                }

            pLast->mtuSize = m2Tbl.ifMtu;
            if (m2Tbl.ifType < 0 || m2Tbl.ifType >= bpfDltTblSize)
                {
                pLast->dataLinkType = DLT_NULL;
                }
            else
                {
                pLast->dataLinkType = bpfDltTbl [m2Tbl.ifType];
                }
            }
        }

    /* Save the new network interface attachment in the BPF unit entry. */

    pBpfDev->pNetDev = pLast;

    return (OK);

    /*
     * Remove any linked BPF MUX protocols if an error occurred
     * while attaching to the network interface.
     */

bpfProtoAttachErr:
    if ( (pLast->bsdFlag == FALSE) && pLast->pCookie != NULL)
        {

        int deviceType = muxTkDrvCheck (pLast->devName);

        if (deviceType != ERROR)
            {
            if (deviceType == 0)
                muxUnbind (pLast->pCookie, MUX_PROTO_SNARF,
                       (FUNCPTR) bpfProtoInput);
            else
                muxUnbind (pLast->pCookie, MUX_PROTO_SNARF,
                       (FUNCPTR) bpfTkProtoInput);
            }
        }
    return (EINVAL);
    }

/* INTERNAL: Each device contains a table for binding BPF file
 * descriptors to network interfaces. Each BPF unit adds an element
 * to that table so that the associated file descriptor can use any
 * network interface. That element is unused if the file descriptor
 * uses the same network interface as another BPF unit of the same
 * device, but this approach allows users to create BPF devices without
 * limiting the number of unique network interfaces allowed.
 * This routine allows every BPF device access to a list of the network
 * interfaces which are currently in use so that the MUX bindings are
 * appropriately established and maintained.
 */

void _bpfProtoInit (void)
    {
    if (!bpfProtoInitDone)
        {
        lstInit (&lstProtoInUse);
        bpfProtoInitDone = TRUE;
        }
    return;
    }

STATUS _bpfProtoPromisc
    (
    BPF_PROTO_CTRL * pBpfProto,
    BOOL           on
    )
    {
    ULONG endFlags;
    int error = OK;
    BPF_PROTO_CTRL * pFirst;    /* First attachment to network interface. */

#ifdef INCLUDE_IP
    char devName [IFNAMSIZ];
#endif  /* INCLUDE_IP */

    if (pBpfProto->bsdFlag)
        return (EINVAL);

    if (on)
        {
        error = muxIoctl (pBpfProto->pCookie, EIOCGFLAGS, (char *) &endFlags);
        if (error != OK)
            {
            return (EINVAL);
            }

        /* Enable promiscuous mode if not already enabled */ 

        if ( (endFlags & IFF_PROMISC) == 0)
            {
            endFlags = IFF_PROMISC;
            error = muxIoctl (pBpfProto->pCookie, EIOCSFLAGS, 
                              (char *) endFlags);
            if (error != OK)
                {
                return (EINVAL);
                }

#ifdef INCLUDE_IP
            sprintf (devName, "%s%d", pBpfProto->devName, 
                     pBpfProto->devUnit); 
            ifFlagChange (devName, IFF_PROMISC, TRUE);
#endif  /* INCLUDE_IP */
            }
 
        pBpfProto->bpfPromiscEnabled = TRUE;
        pBpfProto->numPromisc ++;
        return (OK);
        }

    pBpfProto->numPromisc --;

    /*
     * Disable the network interface's promiscuous mode if no other BPF
     * units from any BPF devices are using it.
     */

    if (pBpfProto->numPromisc == 0)
        {
        /* This BPF device no longer uses promiscuous mode. */

        pBpfProto->bpfPromiscEnabled = FALSE;

        /* Find the start of the attachment list for this network interface. */

        for (pFirst = (BPF_PROTO_CTRL *)lstFirst (&lstProtoInUse);
             pFirst != NULL;
             pFirst = (BPF_PROTO_CTRL *)lstNext ((NODE *)pFirst))
            {
            if (pFirst->bsdFlag == TRUE)    /* Ignore BSD network devices. */
                continue;

            if (PCOOKIE_TO_ENDOBJ(pFirst->pCookie) ==
                PCOOKIE_TO_ENDOBJ(pBpfProto->pCookie))
                break; 
            }

        if (pFirst == NULL)    /* No device attached? (Can't really occur). */
            return (OK);

        if (pFirst->numPromisc)
            {
            /*
             * The first BPF device attached to the network
             * interface is using the promiscuous mode setting.
             */

            return (OK);
            }
        else
            {
            /*
             * Check the promiscuous mode setting for any
             * BPF devices sharing a network interface.
             */

            pFirst = (BPF_PROTO_CTRL *)lstFirst (& (pFirst->bpfDevList));
            while (pFirst != NULL)
                {
                if (pFirst->numPromisc)
                    break;

                pFirst = (BPF_PROTO_CTRL *)lstNext ( (NODE *)pFirst);
                }

            /* Leave interface setting unchanged if promiscuous mode found. */

            if (pFirst != NULL)
                return (OK);
            }

        endFlags = IFF_PROMISC;

        /* Set mask which causes muxIoctl() routine to disable flags. */

        endFlags = - endFlags - 1;

        error = muxIoctl (pBpfProto->pCookie, EIOCSFLAGS, 
                          (char *) endFlags);
        if (error != OK)
            error = EINVAL;
#ifdef INCLUDE_IP
        sprintf (devName, "%s%d", pBpfProto->devName,
                 pBpfProto->devUnit); 
        ifFlagChange (devName, IFF_PROMISC, FALSE);
#endif  /* INCLUDE_IP */
        }
    return (error);
    }

STATUS _bpfProtoSend
    (
    BPF_PROTO_CTRL * pBpfProto,
    char * pBuf,
    int nBytes,
    BOOL nonBlocking
    )
    {
    M_BLK_ID pMBlk;
    END_OBJ * pEnd;

    if (pBpfProto->bsdFlag)
        return (ERROR);

    pEnd = PCOOKIE_TO_ENDOBJ(pBpfProto->pCookie);
    
    pMBlk = netTupleGet (pEnd->pNetPool, nBytes,
                         (nonBlocking ? M_DONTWAIT : M_WAIT), MT_DATA, TRUE);
    if (pMBlk == NULL)
	{
	netErrnoSet (ENOBUFS);
	return (ERROR);
	}

    return (muxSend (pBpfProto->pCookie, pMBlk));
    }

/*****************************************************************************
* 
* bpfTkProtoInput - Check incoming frame against every BPF unit
*
* This routine checks incoming frame against every BPF unit attached to a
* NPT device using the muxTk/END interface. The BPF device containing the
* attachment data must still exist or the MUX would not call this
* routine. Always return FALSE to allow normal stack processing after
* applying any filters.
*
* RETURNS: FALSE
*/

LOCAL BOOL bpfTkProtoInput 
    (
    void *      pProtoCtrl,
    long        type,
    M_BLK_ID    pMBlk,
    void *      pSpare
    )
    {
    BPF_PROTO_CTRL * pBpfProto;
    ulong_t          dataOffset;

    if (pProtoCtrl == NULL)
        return (FALSE);

    /* Access a BPF unit's attachment to the network interface. */

    pBpfProto = (BPF_PROTO_CTRL *) pProtoCtrl;

    dataOffset = 0;
    if (muxIoctl (pBpfProto->pCookie, EIOCGHDRLEN,
                  (caddr_t)&dataOffset) != OK)
        return (FALSE);

    /* Apply the current filters and save matching frames. */

    _bpfPacketTap (&(pBpfProto->bpfUnitList), type, pMBlk, dataOffset);

    return (FALSE);
    }

/*****************************************************************************
* 
* bpfTkProtoShutdown - shutdown routine
*
* This is the shut down routine passed to muxTkBind call
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS bpfTkProtoShutdown 
    (
    void * pProtoCtrl
    )
    {

    if (pProtoCtrl != NULL)
        {
        return (bpfProtoShutdown (NULL, pProtoCtrl));
        }

    return (OK);
    }

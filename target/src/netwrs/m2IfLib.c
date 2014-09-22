/* m2IfLib.c - MIB-II interface-group API for SNMP agents */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01w,25jun02,ann  making a temporary fix to the counter update code
01v,10may02,kbw  making man page edits
01u,07may02,kbw  making man page edits
01t,25apr02,vvv  added semaphore protection in m2IfTableUpdate (SPR #74394)
01s,05dec01,vvv  fixed stack table search for high index values (SPR #71316) 
01r,15oct01,rae  merge from truestack
01q,14jun01,vvv  fixed compilation warnings
01p,30mar01,rae  allow m2 stuff to be excluded (SPR# 65428)
01o,23jan01,spm  fixed modification history after RFC 2233 merge
01n,13nov00,niq  merged version 01p of tor2_0.open_stack-f1 branch (base 01l):
                 added RFC 2233 support
01m,03nov00,zhu  fix SPR#30082: ifIndex should remain constant
01l,24mar99,ead  Finally fixed the ifLastChange problem!
                 Created the m2SetIfLastChange() function which is called
		 from ifioctl() in if.c. (SPR #23290)
01k,16mar99,ead  additional fixes to SPR #23290, not indexing in pm2IfTable
                 also zeroed each instance of ifLastChange in m2IfInit so
                 the correct zero value is set during a reboot
01j,16mar99,spm  recovered orphaned code from tor1_0_1.sens1_1 (SPR #25770)
01i,11mar99,ead  ifLastChange was not getting updated properly (SPR #23290)
01h,06oct98,ann  corrected the flag check in m2IfTblEntryGet() for
                 ifOperStatus (SPR #22275)
01g,26aug97,spm  removed compiler warnings (SPR #7866)
01f,03apr96,rjc  set the m2InterfaceSem semaphore to NULL in m2IfDelete
01e,25jan95,jdi  doc cleanup.
01d,11nov94,rhp  edited man pages some more
01c,11nov94,rhp  edited man pages
01b,22feb94,elh  added extern to quiet compiler.
01a,08dec93,jag  written
*/
/*
DESCRIPTION
This library provides MIB-II services for the interface group.  It
provides routines to initialize the group, access the group scalar
variables, read the table interfaces and change the state of the interfaces.
For a broader description of MIB-II services, see the manual entry for m2Lib.

To use this feature, include the following component:
INCLUDE_MIB2_IF

USING THIS LIBRARY
This library can be initialized and deleted by calling m2IfInit() and
m2IfDelete() respectively, if only the interface group's services are
needed.  If full MIB-II support is used, this group and all other groups
can be initialized and deleted by calling m2Init() and m2Delete().

The interface group supports the Simple Network Management Protocol (SNMP)
concept of traps, as specified by RFC 1215.  The traps supported by this
group are "link up" and "link down."  This library enables an application
to register a hook routine and an argument.  This hook routine can be
called by the library when a "link up" or "link down" condition is
detected.  The hook routine must have the following prototype:
.CS
void TrapGenerator (int trapType,  /@ M2_LINK_DOWN_TRAP or M2_LINK_UP_TRAP @/ 
		    int interfaceIndex, 
		    void * myPrivateArg);
.CE

The trap routine and argument can be specified at initialization time as
input parameters to the routine m2IfInit() or to the routine m2Init().

The interface-group global variables can be accessed as follows:
.CS
M2_INTERFACE   ifVars;

if (m2IfGroupInfoGet (&ifVars) == OK)
    /@ values in ifVars are valid @/
.CE

An interface table entry can be retrieved as follows:
.CS
M2_INTERFACETBL  interfaceEntry;

/@ Specify zero as the index to get the first entry in the table @/

interfaceEntry.ifIndex = 2;     /@ Get interface with index 2 @/

if (m2IfTblEntryGet (M2_EXACT_VALUE, &interfaceEntry) == OK)
    /@ values in interfaceEntry are valid @/
.CE

An interface entry operational state can be changed as follows:
.CS
M2_INTERFACETBL ifEntryToSet;

ifEntryToSet.ifIndex = 2; /@ Select interface with index 2     @/
                          /@ MIB-II value to set the interface @/
                          /@ to the down state.                @/

ifEntryToSet.ifAdminStatus = M2_ifAdminStatus_down;

if (m2IfTblEntrySet (&ifEntryToSet) == OK)
    /@ Interface is now in the down state @/
.CE

INCLUDE FILES: m2Lib.h
 
SEE ALSO: m2Lib, m2SysLib, m2IpLib, m2IcmpLib, m2UdpLib, m2TcpLib
*/

/* includes */

#include "vxWorks.h"
#include "stdio.h"
#include "stdlib.h"
#include "m2IfLib.h"
#include "private/m2LibP.h"
#include "ioctl.h"
#include "semLib.h"
#include "string.h"
#include "tickLib.h"
#include "sysLib.h"
#include "errnoLib.h"
#include "limits.h"
#include "float.h"
#include "memPartLib.h"
#include "ipProto.h"
#include "private/muxLibP.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif    /* VIRTUAL_STACK */

/* defines */

#define END_OBJ_MIB        1            /* Used in ifnetToEndFieldsGet() */
#define END_OBJ_FLAGS      2            /* Used in ifnetToEndFieldsGet() */

/* imports */

IMPORT FUNCPTR  _m2SetIfLastChange;     /* def'd in if.c for scalability */
IMPORT FUNCPTR  _m2IfTableUpdate;

/* globals */

static const UI64 U64_MAX  = { UINT_MAX, UINT_MAX };

#ifndef VIRTUAL_STACK

AVL_TREE      pM2IfRoot    = NULL;
AVL_TREE *    pM2IfRootPtr = &pM2IfRoot;
M2_IFINDEX *  pm2IfTable;     /* Network I/F table (Allocated Dynamically) */

int           m2IfCount;      /* Number of network interfaces */
int           ifsInList;      /* Actual number of interfaces in our list */
LOCAL FUNCPTR pM2TrapRtn;     /* Pointer to trap routine supplied by the user */
LOCAL void *  pM2TrapRtnArg;  /* Pointer to trap routine argument supplied by */
			      /* the user */

/* last time a row was added/removed from the ifTable/ifXTable */
LOCAL ULONG ifTableLastChange;

/* last time a row was added/removed from the ifStackTable or when the
 * ifStackStatus was last changed */
LOCAL ULONG ifStackLastChange;

/* Semaphore used to protect the Interface group */

SEM_ID m2InterfaceSem;

/* 
 * Global variable used to keep the group startup time in hundreds of a second. 
 * The functionality here is the same as the functionality provided in the 
 * module m2SysLib.c.  It is duplicated to allow for both modules to exist
 * independently.
 */

LOCAL unsigned long startCentiSecs;	/* Hundred of Seconds at start */

#endif    /* VIRTUAL_STACK */

/*
 * The zero object id is used throught out the MIB-II library to fill OID 
 * requests when an object ID is not provided by a group variable.
 */

LOCAL M2_OBJECTID ifZeroObjectId = { 2, {0,0} };

extern int ifioctl ();

LOCAL void   m2IfIncr32Bit (M2_DATA *, ULONG *, ULONG);
LOCAL void   m2IfIncr64Bit (M2_DATA *, UI64 *, ULONG);
LOCAL void   m2IfDefaultValsGet (M2_DATA *, M2_IFINDEX *);
LOCAL void   m2IfCommonValsGet (M2_DATA *, M2_IFINDEX *);
LOCAL STATUS rcvEtherAddrGet (struct ifnet *, M2_IFINDEX *);
LOCAL STATUS rcvEtherAddrAdd (M2_IFINDEX *, unsigned char *);
LOCAL unsigned long centiSecsGet (void);

LOCAL BOOL   stackEntryIsTop(int);
LOCAL BOOL   stackEntryIsBottom(int);
LOCAL void * ifnetToEndFieldsGet (struct ifnet *, int);


/***************************************************************************
*
* m2IfAlloc - allocate the structure for the interface table
*
* This routine is called by the driver during initialization of the interface.
* The memory for the interface table is allocated here.  We also set the
* default update routines in the M2_ID struct. These fields can later be
* overloaded using the installed routines in the M2_ID.  Once this function
* returns, it is the driver's responsibility to set the pMib2Tbl pointer in
* the END object to the new M2_ID.
*
* When this call returns, the calling routine must set the END_MIB_2233 
* bit of the flags field in the END object.
*
* RETURNS: Pointer to the M2_ID structure that was allocated.
*/

M2_ID * m2IfAlloc
    (
    ULONG        ifType,          /* If type of the interface */
    UCHAR *      pEnetAddr,       /* Physical address of interface */
    ULONG        addrLen,         /* Address length */
    ULONG        mtuSize,         /* MTU of interface */
    ULONG        speed,           /* Speed of the interface */
    char *       pName,           /* Name of the device */
    int          unit             /* Unit number of the device */
    )
    {
    M2_ID *             pM2Id;
    M2_INTERFACETBL *   pM2IfTbl;
    M2_2233TBL *        pM2233Tbl;

    /* Allocate memory for the interface table */

    pM2Id = KHEAP_ALLOC(sizeof(M2_ID));
    if (!pM2Id)
        return NULL;

    memset(pM2Id, 0, sizeof(M2_ID));

    /* Enter the values provided by the driver */

    pM2IfTbl = &pM2Id->m2Data.mibIfTbl;
    pM2233Tbl = &pM2Id->m2Data.mibXIfTbl;

    pM2IfTbl->ifType  = ifType;
    pM2IfTbl->ifMtu   = mtuSize;
    pM2IfTbl->ifSpeed = speed;

    memcpy (&pM2IfTbl->ifPhysAddress.phyAddress[0], pEnetAddr, addrLen);
    pM2IfTbl->ifPhysAddress.addrLength = addrLen;

    sprintf (&pM2233Tbl->ifName[0], "%s%d", pName, unit);

    /* Set the default update routines */

    pM2Id->m2PktCountRtn  = m2IfGenericPacketCount;
    pM2Id->m2CtrUpdateRtn = m2IfCounterUpdate;
    pM2Id->m2VarUpdateRtn = m2IfVariableUpdate;

    return (pM2Id);
    }


/***************************************************************************
*
* m2IfFree - free an interface data structure
*
* This routine frees the given M2_ID.  Note if the driver is not an RFC 2233
* driver then the M2_ID is NULL and this function simply returns.
*
* RETURNS: OK if successful, ERROR otherwise
*/
STATUS m2IfFree
    (
    M2_ID * pId      /* pointer to the driver's M2_ID object */
    )
    {
    if (pId == NULL)
        {
        return ERROR;
        }
    else
        {
        KHEAP_FREE((char *)pId);
        return OK;
        }
    }


/***************************************************************************
*
* m2IfIncr32Bit - increment a 32 bit interface counter
*
* This function is used to increment an interface counter.  The counter
* to update is pointed to by pStat and amount specifies how much to increment
* the counter.  If the counter would roll-over if incremented by the amount,
* the the ifCounterDiscontinuityTime is set to the system uptime.
*
* RETURNS: N/A
*/
LOCAL void m2IfIncr32Bit
    (
    M2_DATA * pMib2Data,        /* pointer to interface stat struct */
    ULONG *   pStat,            /* pointer to the counter to update */
    ULONG     amount            /* amount to update the counter */
    )
    {
#ifdef notdef
    if (*pStat > UINT_MAX)
        {
        *pStat = UINT_MAX;
        }

    if ((UINT_MAX - *pStat) < amount)
        {
        *pStat = UINT_MAX;
        pMib2Data->mibXIfTbl.ifCounterDiscontinuityTime = centiSecsGet();
        }
    else
        {
        *pStat += amount;
        }
#endif
        *pStat += amount;

    }



/***************************************************************************
*
* m2IfIncr64Bit - increment a 64 bit interface counter
*
* This function is used to increment an interface counter.  The counter
* to update is pointed to by pStat and amount specifies how much to increment
* the counter.  If the counter would roll-over if incremented by the amount,
* the the ifCounterDiscontinuityTime is set to the system uptime.
*
* RETURNS: N/A
*/
LOCAL void m2IfIncr64Bit
    (
    M2_DATA *   pMib2Data,  /* pointer to interface stat struct */
    UI64 *      pStat,      /* pointer to the counter to update */
    ULONG       amount      /* amount to update the counter */
    )
    {
    UI64 u64Diff;
    UI64 u64Amount;

    if (UI64_COMP(pStat, &U64_MAX) >= 0)
        {
        UI64_COPY(pStat, &U64_MAX);
        }

    UI64_ZERO(&u64Diff);
    UI64_ZERO(&u64Amount);

    u64Amount.low = amount;

    UI64_SUB64(&u64Diff, &U64_MAX, pStat);

    if (UI64_COMP(&u64Diff, &u64Amount) < 0)
        {
        UI64_COPY(pStat, &U64_MAX);
        pMib2Data->mibXIfTbl.ifCounterDiscontinuityTime = centiSecsGet();
        }
    else
        {
        UI64_ADD32(pStat, amount);
        }
    }



/***************************************************************************
*
* m2IfGenericPacketCount - increment the interface packet counters 
*
* This function updates the basic interface counters for a packet. It knows 
* nothing of the underlying media.  Thus, so only the 'ifInOctets', 
* 'ifHCInOctets', 'ifOutOctets', 'ifHCOutOctets', and 
* 'ifCounterDiscontinuityTime' variables are incremented.  The <ctrl> 
* argument specifies whether the packet is being sent or just received 
* (M2_PACKET_IN or M2_PACKET_OUT).
*
* RETURNS: ERROR if the M2_ID is NULL, OK if the counters were updated.
*/
STATUS m2IfGenericPacketCount 
    (
    M2_ID *   pId,      /* The pointer to the device M2_ID object */
    UINT      ctrl,     /* Update In or Out counters */
    UCHAR *   pPkt,     /* The incoming/outgoing packet */
    ULONG     pktLen    /* Length of the packet */
    )
    {
    M2_DATA * pMib2Data;

    if (pId == NULL)
        return ERROR;

    pMib2Data = &(pId->m2Data);

    switch (ctrl)
        {
        case M2_PACKET_IN:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifInOctets),
                          pktLen);
            break;

        case M2_PACKET_OUT:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifOutOctets),
                          pktLen);
            break;

        default:
            return ERROR;
        }

    return OK;
    }


/***************************************************************************
* 
* m2If8023PacketCount - increment the packet counters for an 802.3 device 
*
* This function is used to update basic interface counters for a packet.  The
* ctrl argument specifies whether the packet is being sent or just received
* (M2_PACKET_IN or M2_PACKET_OUT).  This function only works for 802.3
* devices as it understand the Ethernet packet format.  The following counters
* are updated:
* \ml
* \m - 
* ifInOctets
* \m - 
* ifInUcastPkts
* \m - 
* ifInNUcastPkts
* \m - 
* ifOutOctets
* \m - 
* ifOutUcastPkts
* \m - 
* ifOutNUcastPkts
* \m - 
* ifInMulticastPkts
* \m - 
* ifInBroadcastPkts
* \m - 
* ifOutMulticastPkts
* \m - 
* ifOutBroadcastPkts
* \m - 
* ifHCInOctets
* \m - 
* ifHCInUcastPkts
* \m - 
* ifHCOutOctets
* \m - 
* ifHCOutUcastPkts
* \m - 
* ifHCInMulticastPkts
* \m - 
* ifHCInBroadcastPkts
* \m - 
* ifHCOutMulticastPkts
* \m - 
* ifHCOutBroadcastPkts
* \m - 
* ifCounterDiscontinuityTime
* \me
* This function should be called right after the netMblkToBufCopy() function
* has been completed.  The first 6 bytes in the resulting buffer must contain 
* the destination MAC address and the second 6 bytes of the buffer must 
* contain the source MAC address.
* 
* The type of MAC address (i.e. broadcast, multicast, or unicast) is
* determined by the following:
* \ml
* \m broadcast address:  
*  ff:ff:ff:ff:ff:ff
* \m multicast address:  
*  first bit is set
* \m unicast address:  
*  any other address not matching the above
* \me
* RETURNS: ERROR, if the M2_ID is NULL, or the ctrl is invalid; OK, if
* the counters were updated.
*/
STATUS m2If8023PacketCount
    (
    M2_ID *   pId,      /* The pointer to the device M2_ID object */
    UINT      ctrl,     /* Update In or Out counters */
    UCHAR *   pPkt,     /* The incoming/outgoing packet */
    ULONG     pktLen    /* Length of the packet */
    )
    {
    M2_DATA * pMib2Data;
    UCHAR pBcastAddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    UCHAR mcastBit = 0x01;
    USHORT *pSrc, *pDst;

    if (pId == NULL)
        return ERROR;

    pMib2Data = &(pId->m2Data);
    pDst = (USHORT *)pBcastAddr;
    pSrc = (USHORT *)pPkt;

    switch (ctrl)
        {
        case M2_PACKET_IN:
            {
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifInOctets),
                          pktLen);

            /* the first 6 bytes is the destination MAC address */
            if (*pPkt & mcastBit) /* multicast address */
                {
                if (pSrc[0] == pDst[0] &&
                    pSrc[1] == pDst[1] &&
                    pSrc[2] == pDst[2])
                    m2IfIncr32Bit(pMib2Data, /* broadcast address */
                                  &(pMib2Data->mibXIfTbl.ifInBroadcastPkts), 1);
                else
                    m2IfIncr32Bit(pMib2Data,
                                  &(pMib2Data->mibXIfTbl.ifInMulticastPkts), 1);

                m2IfIncr32Bit(pMib2Data,
                              &(pMib2Data->mibIfTbl.ifInNUcastPkts), 1);
                }
            else /* unicast address */
                {
                m2IfIncr32Bit(pMib2Data,
                              &(pMib2Data->mibIfTbl.ifInUcastPkts), 1);
                }

            break;
            }

        case M2_PACKET_OUT:
            {
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifOutOctets),
                          pktLen);

            /* the first 6 bytes is the destination MAC address */
            if (*pPkt & mcastBit) /* multicast address */
                {
                if (pSrc[0] == pDst[0] &&
                    pSrc[1] == pDst[1] &&
                    pSrc[2] == pDst[2])
                    m2IfIncr32Bit(pMib2Data,
                                  &(pMib2Data->mibXIfTbl.ifOutBroadcastPkts), 1);
                else
                    m2IfIncr32Bit(pMib2Data,
                                  &(pMib2Data->mibXIfTbl.ifOutMulticastPkts), 1);
                m2IfIncr32Bit(pMib2Data,
                              &(pMib2Data->mibIfTbl.ifOutNUcastPkts), 1);
                }
            else /* unicast address */
                {
                m2IfIncr32Bit(pMib2Data,
                              &(pMib2Data->mibIfTbl.ifOutUcastPkts), 1);
                }

            break;
            }

        default:
            return ERROR;
        }

    return OK;
    }


/***************************************************************************
*
* m2IfCounterUpdate - increment interface counters 
*
* This function is used to directly update an interface counter.  The counter
* is specified by <ctrId> and the amount to increment it is specified by value.
* If the counter would roll over then the 'ifCounterDiscontinuityTime' is
* updated with the current system uptime.
*
* RETURNS: ERROR if the M2_ID is NULL, OK if the counter was updated.
*/
STATUS m2IfCounterUpdate 
    (
    M2_ID *   pId,    /* The pointer to the device M2_ID object */
    UINT      ctrId,  /* Counter to update */
    ULONG     value   /* Amount to update the counter by */
    )
    {
    M2_DATA * pMib2Data;

    if (pId == NULL)
        return ERROR;

    pMib2Data = &(pId->m2Data);

    switch (ctrId)
        {
        case M2_ctrId_ifInOctets:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifInOctets),
                          value);
            break;

        case M2_ctrId_ifInUcastPkts:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifInUcastPkts),
                          value);
            break;

        case M2_ctrId_ifInNUcastPkts:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifInNUcastPkts),
                          value);
            break;

        case M2_ctrId_ifInDiscards:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifInDiscards),
                          value);
            break;

        case M2_ctrId_ifInErrors:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifInErrors),
                          value);
            break;

        case M2_ctrId_ifInUnknownProtos:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifInUnknownProtos),
                          value);
            break;

        case M2_ctrId_ifOutOctets:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifOutOctets),
                          value);
            break;

        case M2_ctrId_ifOutUcastPkts:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifOutUcastPkts),
                          value);
            break;

        case M2_ctrId_ifOutNUcastPkts:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifOutNUcastPkts),
                          value);
            break;

        case M2_ctrId_ifOutDiscards:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifOutDiscards),
                          value);
            break;

        case M2_ctrId_ifOutErrors:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibIfTbl.ifOutErrors),
                          value);
            break;

        case M2_ctrId_ifInMulticastPkts:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibXIfTbl.ifInMulticastPkts),
                          value);
            break;

        case M2_ctrId_ifInBroadcastPkts:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibXIfTbl.ifInBroadcastPkts),
                          value);
            break;

        case M2_ctrId_ifOutMulticastPkts:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibXIfTbl.ifOutMulticastPkts),
                          value);
            break;

        case M2_ctrId_ifOutBroadcastPkts:
            m2IfIncr32Bit(pMib2Data, &(pMib2Data->mibXIfTbl.ifOutBroadcastPkts),
                          value);
            break;

        case M2_ctrId_ifHCInOctets:
            m2IfIncr64Bit(pMib2Data, &(pMib2Data->mibXIfTbl.ifHCInOctets),
                          value);
            break;

        case M2_ctrId_ifHCInUcastPkts:
            m2IfIncr64Bit(pMib2Data, &(pMib2Data->mibXIfTbl.ifHCInUcastPkts),
                          value);
            break;

        case M2_ctrId_ifHCInMulticastPkts:
            m2IfIncr64Bit(pMib2Data,
                          &(pMib2Data->mibXIfTbl.ifHCInMulticastPkts), value);
            break;

        case M2_ctrId_ifHCInBroadcastPkts:
            m2IfIncr64Bit(pMib2Data,
                          &(pMib2Data->mibXIfTbl.ifHCInBroadcastPkts), value);
            break;

        case M2_ctrId_ifHCOutOctets:
            m2IfIncr64Bit(pMib2Data,
                          &(pMib2Data->mibXIfTbl.ifHCOutOctets), value);
            break;

        case M2_ctrId_ifHCOutUcastPkts:
            m2IfIncr64Bit(pMib2Data,
                          &(pMib2Data->mibXIfTbl.ifHCOutUcastPkts), value);
            break;

        case M2_ctrId_ifHCOutMulticastPkts:
            m2IfIncr64Bit(pMib2Data,
                          &(pMib2Data->mibXIfTbl.ifHCOutMulticastPkts), value);
            break;

        case M2_ctrId_ifHCOutBroadcastPkts:
            m2IfIncr64Bit(pMib2Data,
                          &(pMib2Data->mibXIfTbl.ifHCOutBroadcastPkts), value);
            break;

        default:
            return ERROR;
        }

    return OK;
    }


/***************************************************************************
*
* m2IfVariableUpdate - update the contents of an interface non-counter object 
*
* This function is used to update an interface variable.  The variable is
* specified by varId and the data to use is specified by pData.  Note that
* different variable expect different types of data.  Here is a list of the
* variables and the type of data expected.  Therefore, pData will be cast to 
* the type listed below for each variable.
* \ts
* Variable               | Casted to Type
* ----------------------------------------
* ifDescr                | char *
* ifType                 | UINT
* ifMtu                  | ULONG
* ifSpeed                | ULONG
* ifPhysAddress          | M2_PHYADDR *
* ifAdminStatus          | ULONG
* ifOperStatus           | ULONG
* ifLastChange           | ULONG
* ifOutQLen              | ULONG
* ifSpecific             | M2_OBJECTID *
* ifName                 | char *
* ifLinkUpDownTrapEnable | UINT
* ifHighSpeed            | ULONG
* ifPromiscuousMode      | UINT
* ifConnectorPresent     | UINT
* ifAlias                | char *
* \te
* RETURNS: ERROR, if the M2_ID is NULL; OK, if the variable was updated.
*/
STATUS m2IfVariableUpdate 
    (
    M2_ID *   pId,    /* The pointer to the device M2_ID object */
    UINT      varId,  /* Variable to update */
    caddr_t   pData   /* Data to use */
    )
    {
    M2_DATA * pMib2Data;

    if (pId == NULL)
        return ERROR;

    pMib2Data = &(pId->m2Data);

    switch (varId)
        {
        case M2_varId_ifDescr:
            strcpy(pMib2Data->mibIfTbl.ifDescr, (char *)pData);
            break;

        case M2_varId_ifType:
            pMib2Data->mibIfTbl.ifType = (UINT)pData;
            break;

        case M2_varId_ifMtu:
            pMib2Data->mibIfTbl.ifMtu = (ULONG)pData;
            break;

        case M2_varId_ifSpeed:
            pMib2Data->mibIfTbl.ifSpeed = (ULONG)pData;
            break;

        case M2_varId_ifPhysAddress:
            memcpy(&(pMib2Data->mibIfTbl.ifPhysAddress), (M2_PHYADDR *)pData,
                   sizeof(M2_PHYADDR));
            break;

        case M2_varId_ifAdminStatus:
            pMib2Data->mibIfTbl.ifAdminStatus = (ULONG)pData;
            break;

        case M2_varId_ifOperStatus:
            pMib2Data->mibIfTbl.ifOperStatus = (ULONG)pData;
            break;

        case M2_varId_ifLastChange:
            pMib2Data->mibIfTbl.ifLastChange = (ULONG)pData;
            break;

        case M2_varId_ifOutQLen:
            pMib2Data->mibIfTbl.ifOutQLen = (ULONG)pData;
            break;

        case M2_varId_ifSpecific:
            memcpy(&(pMib2Data->mibIfTbl.ifSpecific), (M2_OBJECTID *)pData,
                   sizeof(M2_OBJECTID));
            break;

        case M2_varId_ifName:
            strcpy(pMib2Data->mibXIfTbl.ifName, (char *)pData);
            break;

        case M2_varId_ifLinkUpDownTrapEnable:
            pMib2Data->mibXIfTbl.ifLinkUpDownTrapEnable = (UINT)pData;
            break;

        case M2_varId_ifHighSpeed:
            pMib2Data->mibXIfTbl.ifHighSpeed = (ULONG)pData;
            break;

        case M2_varId_ifPromiscuousMode:
            pMib2Data->mibXIfTbl.ifPromiscuousMode = (UINT)pData;
            break;

        case M2_varId_ifConnectorPresent:
            pMib2Data->mibXIfTbl.ifConnectorPresent = (UINT)pData;
            break;

        case M2_varId_ifAlias:
            strcpy(pMib2Data->mibXIfTbl.ifAlias, (char *)pData);
            break;

        default:
            return ERROR;
        }

    return OK;
    }


/***************************************************************************
*
* m2IfPktCountRtnInstall - install an interface packet counter routine
*
* This function installs a routine in the M2_ID.  This routine is a packet
* counter which is able to update all the interface counters.
*
* RETURNS: ERROR if the M2_ID is NULL, OK if the routine was installed.
*/
STATUS m2IfPktCountRtnInstall
    (
    M2_ID *           pId,
    M2_PKT_COUNT_RTN  pRtn
    )
    {
    if (pId == NULL)
        return ERROR;

    pId->m2PktCountRtn = pRtn;
    return OK;
    }


/***************************************************************************
*
* m2IfCtrUpdateRtnInstall - install an interface counter update routine
*
* This function installs a routine in the M2_ID.  This routine is able to
* update a single specified interface counter.
*
* RETURNS: ERROR if the M2_ID is NULL, OK if the routine was installed.
*/
STATUS m2IfCtrUpdateRtnInstall
    (
    M2_ID *            pId,
    M2_CTR_UPDATE_RTN  pRtn
    )
    {
    if (pId == NULL)
        return ERROR;

    pId->m2CtrUpdateRtn = pRtn;
    return OK;
    }




/***************************************************************************
*
* m2IfVarUpdateRtnInstall - install an interface variable update routine
*
* This function installs a routine in the M2_ID.  This routine is able to
* update a single specified interface variable.
*
* RETURNS: ERROR if the M2_ID is NULL, OK if the routine was installed.
*/
STATUS m2IfVarUpdateRtnInstall
    (
    M2_ID *            pId,
    M2_VAR_UPDATE_RTN  pRtn
    )
    {
    if (pId == NULL)
        return ERROR;

    pId->m2VarUpdateRtn = pRtn;
    return OK;
    }




/******************************************************************************
*
* centiSecsGet - get hundreds of a second
*
* The number of hundreds of a second that have passed since this routine was
* first called.
*
* RETURNS: Hundreds of a second since the group was initialized.
*
* SEE ALSO: N/A
*/
LOCAL unsigned long centiSecsGet (void)
    {
    unsigned long currCentiSecs;
    unsigned long clkRate = sysClkRateGet ();

    if (startCentiSecs == 0)
	startCentiSecs = (tickGet () * 100) / clkRate;

    currCentiSecs = (tickGet () * 100) / clkRate;

    return (currCentiSecs - startCentiSecs);
    }


/******************************************************************************
*
* m2IfInit - initialize MIB-II interface-group routines
*
* This routine allocates the resources needed to allow access to the 
* MIB-II interface-group variables.  This routine must be called before any 
* interface variables can be accessed.  The input parameter <pTrapRtn> is an
* optional pointer to a user-supplied SNMP trap generator.  The input parameter
* <pTrapArg> is an optional argument to the trap generator.  Only one trap
* generator is supported.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call m2IfInit() from within the kernel 
* protection domain only, and the data referenced in the <pTrapRtn> and  
* <pTrapArg> parameters must reside in the kernel protection domain.  
* This restriction does not apply to non-AE versions of VxWorks.  
*
* RETURNS: OK, if successful; ERROR, if an error occurred.
*
* ERRNO:
* S_m2Lib_CANT_CREATE_IF_SEM
*
* SEE ALSO: 
* m2IfGroupInfoGet(), m2IfTblEntryGet(), m2IfTblEntrySet(), m2IfDelete()
*/
STATUS m2IfInit 
    (
    FUNCPTR 	pTrapRtn, 	/* pointer to user trap generator */
    void * 	pTrapArg	/* pointer to user trap generator argument */
    )
    {

    /* add hooks for if.c */
    _m2SetIfLastChange = (FUNCPTR) m2SetIfLastChange;
    _m2IfTableUpdate   = (FUNCPTR) m2IfTableUpdate;

    /* Create the Interface semaphore */
    if (m2InterfaceSem == NULL)
        {
        m2InterfaceSem = semMCreate (SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                   SEM_DELETE_SAFE);
        if (m2InterfaceSem == NULL)
            {
	    errnoSet (S_m2Lib_CANT_CREATE_IF_SEM);
            return (ERROR);
            }
        }

    /* Take the interface semaphore, and initialize group global variables */
 
    semTake (m2InterfaceSem, WAIT_FOREVER);

    pM2TrapRtn    = pTrapRtn;
    pM2TrapRtnArg = pTrapArg;

#ifdef VIRTUAL_STACK
    _ifTableLastChange = 0;
    _ifStackLastChange = 0;
#else    /* VIRTUAL_STACK */
    ifTableLastChange = 0;
    ifStackLastChange = 0;
#endif   /* VIRTUAL_STACK */
    
    semGive (m2InterfaceSem);

    (void) centiSecsGet ();     /* Initialize group time reference */

    return (OK);
    }




/***************************************************************************
*
* m2IfTableUpdate - insert or remove an entry in the ifTable
*
* This routine is called by if_attach and if_detach to insert/remove
* an entry from the local m2IfLib ifTable.  The status can be either
* M2_IF_TABLE_INSERT or M2_IF_TABLE_REMOVE.  The ifIndex that is searched
* for in the AVL tree is specified in given the ifnet struct.
* <if_ioctl> is a function pointer to change the flags on the interface.
* <addr_get> is a function pointer to add the interface's addresses to
* ifRcvAddressTable.  Ethernet interfaces can use NULL for both function
* pointers, other interfaces will need to pass an appropriate function.
*
* RETURNS: ERROR if entry does not exist, OK if the entry was deleted
*/
STATUS m2IfTableUpdate
    (
    struct ifnet *  pIfNet,
    UINT            status,   /* attaching or detaching */
    int             (*if_ioctl) /* protocol specific ioctl or null for default (ethernet) */
		      (struct socket*,u_long,caddr_t),
    STATUS          (*addr_get) /* func to grab the interface's addrs, null for default (ethernet) */
		      (struct ifnet*, M2_IFINDEX*)
    )
    {
    UINT              index;
    M2_IFINDEX *      pIfIndex;
    M2_IFSTACKTBL *   pIfStackTbl;
    M2_IFSTACKTBL *   pTempS;
    M2_IFRCVADDRTBL * pIfRcvAddrTbl;
    M2_IFRCVADDRTBL * pTempR;
    M2_ID *           pM2Id = NULL;
    long *            pFlags = NULL;

    index = pIfNet->if_index;

    switch (status)
        {
        case M2_IF_TABLE_INSERT:
            {
            pIfIndex = KHEAP_ALLOC(sizeof(M2_IFINDEX));
            if (!pIfIndex)
                return ERROR;

            memset(pIfIndex, 0, sizeof(M2_IFINDEX));

            pIfIndex->ifIndex  = index;
            pIfIndex->pIfStats = pIfNet;
     
            /* Set the MIB style flag */

            pFlags = (long *) ifnetToEndFieldsGet (pIfNet, END_OBJ_FLAGS);
            if (pFlags)
                {
                if (*pFlags & END_MIB_2233)
                    pIfIndex->mibStyle = TRUE;
                else
                    pIfIndex->mibStyle = FALSE;
                }

	    if (if_ioctl)      /* use supplied ioctl */
		{
		pIfIndex->ifIoctl = if_ioctl;
		}
            else               /* default */
		{
		pIfIndex->ifIoctl = ifioctl;
		}

            if (addr_get)
		{
		pIfIndex->rcvAddrGet = addr_get;
		}
            else
		{
		pIfIndex->rcvAddrGet = rcvEtherAddrGet;
		}

            /* Set Interface OID to default */
            memcpy(pIfIndex->ifOid.idArray, ifZeroObjectId.idArray,
                   ifZeroObjectId.idLength * sizeof(long));
            pIfIndex->ifOid.idLength = ifZeroObjectId.idLength;

            /* Fill in the unicast and the multicast addresses */
	    (*pIfIndex->rcvAddrGet)(pIfNet, pIfIndex);

            semTake (m2InterfaceSem, WAIT_FOREVER);

            /* Insert the node in the tree */
            if (avlInsert(pM2IfRootPtr, pIfIndex, (GENERIC_ARGUMENT)index,
                          nextIndex) != OK)
                {
		semGive (m2InterfaceSem);
                KHEAP_FREE((char *)pIfIndex);
                return ERROR;
                }

            m2IfCount++;
            ifsInList = index;
#ifdef VIRTUAL_STACK
            _ifTableLastChange = centiSecsGet();
#else    /* VIRTUAL_STACK */
            ifTableLastChange = centiSecsGet();
#endif   /* VIRTUAL_STACK */          

            semGive (m2InterfaceSem);

            if (pM2TrapRtn != NULL)
                {
                (*pM2TrapRtn)(M2_LINK_UP_TRAP, 
                                  pIfIndex->ifIndex, pM2TrapRtnArg);
                }

            return OK;
            }

        case M2_IF_TABLE_REMOVE:
            {
	    semTake (m2InterfaceSem, WAIT_FOREVER);

            /* Remove the node from the tree */
            if ((pIfIndex = (M2_IFINDEX *)avlDelete(pM2IfRootPtr,
                                                    (GENERIC_ARGUMENT)index,
                                                    nextIndex)) == NULL)
		{
		semGive (m2InterfaceSem);
                return ERROR;
		}

            pIfStackTbl = pIfIndex->pNextLower;
            while (pIfStackTbl != NULL)
                {
                pTempS = pIfStackTbl;
                pIfStackTbl = pIfStackTbl->pNextLower;
                KHEAP_FREE((char *)pTempS);
                }

            pIfRcvAddrTbl = pIfIndex->pRcvAddr;
            while (pIfRcvAddrTbl != NULL)
                {
                pTempR = pIfRcvAddrTbl;
                pIfRcvAddrTbl = pIfRcvAddrTbl->pNextEntry;
                KHEAP_FREE((char *)pTempR);
                }

            KHEAP_FREE((char *)pIfIndex);

            m2IfCount--;
#ifdef VIRTUAL_STACK
            _ifTableLastChange = centiSecsGet();
#else    /* VIRTUAL_STACK */            
            ifTableLastChange = centiSecsGet();
#endif   /* VIRTUAL_STACK */            

            semGive (m2InterfaceSem);

            if ( (pM2TrapRtn != NULL) && (pIfNet->if_ioctl != NULL) &&
                     ((*pIfNet->if_ioctl)(pIfNet, SIOCGMIB2233,
                                                  (caddr_t)&pM2Id) == 0) )
                {
                if (pM2Id->m2Data.mibXIfTbl.ifLinkUpDownTrapEnable 
                                           == M2_LINK_UP_DOWN_TRAP_ENABLED)
                    {
                    (*pM2TrapRtn)(M2_LINK_DOWN_TRAP, 
                                      pIfIndex->ifIndex, pM2TrapRtnArg);
                    }
                }

            return OK;
            }

        default:
            return ERROR;
        }
    }





/******************************************************************************
*
* rcvEtherAddrGet - populate the rcvAddr fields for the ifRcvAddressTable
*
* This function needs to be called to add all physical addresses for which an
* interface may receive or send packets. This includes unicast and multicast
* addresses. The address is inserted into the linked list maintained in the
* AVL node corresponding to the interface. Given the ifnet struct and the
* AVL node corresponding to the interface, this function goes through all the
* physical addresses associated with this interface and adds them into the
* linked list.
*
* RETURNS: OK, if successful; ERROR, otherwise. 
*/
STATUS rcvEtherAddrGet
    (
    struct ifnet *     pIfNet,       /* pointer to the interface's ifnet */
    M2_IFINDEX *       pIfIndexEntry /* avl node */
    )
    {
    struct arpcom *      pIfArpcom = (struct arpcom*) pIfNet;
    unsigned char *      pEnetAddr = pIfArpcom->ac_enaddr;
    LIST *               pMCastList = NULL;
    ETHER_MULTI *        pEtherMulti = NULL;

    /* Copy the list of mcast addresses */

    if ( (pIfArpcom->ac_if.if_ioctl != NULL) &&
             ((*pIfArpcom->ac_if.if_ioctl)((struct ifnet *)pIfArpcom,
                               SIOCGMCASTLIST, (caddr_t)&pMCastList) != 0) )
        {
        return (ERROR);
        }
    
    /* First create the entry for the unicast address */

    rcvEtherAddrAdd (pIfIndexEntry, pEnetAddr);

    /* Loop thru the mcast linked list and add the addresses */

    for ( pEtherMulti = (ETHER_MULTI *) lstFirst (pMCastList);
             pEtherMulti != NULL;
                 pEtherMulti = (ETHER_MULTI *) lstNext ((NODE *)pEtherMulti) )
        {
        rcvEtherAddrAdd (pIfIndexEntry, (unsigned char *)pEtherMulti->addr);
        }

    return (OK);
    }




/****************************************************************************
*
* rcvEtherAddrAdd - add a physical address into the linked list
*
* This function is a helper function for rcvEtherAddrGet.  It is called to
* add a single physical address into the linked list of addresses maintained
* by the AVL node.
*
* RETURNS: OK, if successful; ERROR, otherwise.
*/
STATUS rcvEtherAddrAdd
    (
    M2_IFINDEX *    pIfIndexEntry,    /* the avl node */
    unsigned char * pEnetAddr         /* the addr to be added */
    )
    {
    M2_IFRCVADDRTBL *    pIfRcvAddr = NULL;
    M2_IFRCVADDRTBL **   ppTemp = &pIfIndexEntry->pRcvAddr;

    if (pEnetAddr)
        {
        pIfRcvAddr = KHEAP_ALLOC(sizeof (M2_IFRCVADDRTBL));
        if (!pIfRcvAddr)
            return ERROR;

        bcopy (pEnetAddr, pIfRcvAddr->ifRcvAddrAddr.phyAddress, ETHERADDRLEN);
        pIfRcvAddr->ifRcvAddrAddr.addrLength = ETHERADDRLEN;
        pIfRcvAddr->ifRcvAddrStatus = ROW_ACTIVE;
        pIfRcvAddr->ifRcvAddrType = STORAGE_NONVOLATILE;
        pIfRcvAddr->pNextEntry = NULL;

        if (pIfIndexEntry->pRcvAddr)
            {
            while (*ppTemp)
                {
                if (memcmp (pEnetAddr, (*ppTemp)->ifRcvAddrAddr.phyAddress,
                                                         ETHERADDRLEN) > 0)
                    {
                    ppTemp = &(*ppTemp)->pNextEntry;
                    continue;
                    }
                else
                    {
                    pIfRcvAddr->pNextEntry = *ppTemp;
                    break;
                    }            
        
                *ppTemp = pIfRcvAddr;
                }
            }
        else
            {
            pIfIndexEntry->pRcvAddr = pIfRcvAddr;
            }

        return (OK);
        }
    
    return (ERROR);
    }



/******************************************************************************
*
* m2IfTblEntryGet - get a MIB-II interface-group table entry
*
* This routine maps the MIB-II interface index to the system's internal
* interface index.  The internal representation is in the form of a balanced
* AVL tree indexed by ifIndex of the interface. The <search> parameter is 
* set to either M2_EXACT_VALUE or M2_NEXT_VALUE; for a discussion of its use, see
* the manual entry for m2Lib. The interface table values are returned in a
* structure of type M2_DATA, which is passed as the second argument to this 
* routine.
*
* RETURNS: 
* OK, or ERROR if the input parameter is not specified, or a match is not found.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*
* SEE ALSO:
* m2Lib, m2IfInit(), m2IfGroupInfoGet(), m2IfTblEntrySet(), m2IfDelete()
*/
STATUS m2IfTblEntryGet
    (
    int       search,       /* M2_EXACT_VALUE or M2_NEXT_VALUE */
    void *    pIfReqEntry   /* pointer to requested interface entry */
    )
    {
    M2_IFINDEX *   pIfIndexEntry = NULL;
    struct ifnet * pIfNetEntry = NULL;
    int            index;
    M2_ID *        pM2Id = NULL;
    M2_DATA *      pM2Entry = (M2_DATA *)pIfReqEntry;
    M2_NETDRVCNFG  ifDrvCnfg;
    
    /* Validate pointer to requeste structure */
 
    if (pIfReqEntry == NULL)
	{
	errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
	}

    semTake (m2InterfaceSem, WAIT_FOREVER);

    /*
     * For a M2_EXACT_VALUE search, look up the index provided in the
     * request and get the node from our internal tree. For a
     * M2_NEXT_VALUE search, look up the successor of this node.
     */

    /* First, Validate the bounds of the requested index  */

    index = pM2Entry->mibIfTbl.ifIndex;
    if (index > ifsInList || index < 0)
        {
        semGive (m2InterfaceSem);
	errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        return (ERROR);
        }

    /* Check to see if it is a EXACT search or a next search */

    if (search == M2_EXACT_VALUE)
        {
        pIfIndexEntry = (M2_IFINDEX *) avlSearch (pM2IfRoot,
                                  (GENERIC_ARGUMENT)index, nextIndex);
        }
    else
        {
        pIfIndexEntry = (M2_IFINDEX *) avlSuccessorGet (pM2IfRoot,
                                 (GENERIC_ARGUMENT)index, nextIndex);
        }

    if (!pIfIndexEntry)
        {
        semGive (m2InterfaceSem);
	errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        return (ERROR);
        }

    /* Get the pointer to the ifnet struct for this ifIndex */
    
    pIfNetEntry = (struct ifnet *) pIfIndexEntry->pIfStats;

    /* Get the interface data from the driver via the Ioctl */

    if (pIfIndexEntry->mibStyle == TRUE)
        {
        /* Call the RFC 2233 Ioctl */

        if ( (pIfNetEntry->if_ioctl != NULL) &&
             ((*pIfNetEntry->if_ioctl)(pIfNetEntry, SIOCGMIB2233,
                                                  (caddr_t)&pM2Id) != 0) )
            {
            /* ioctl failed, copy default values and return */
            
            m2IfDefaultValsGet (pM2Entry, pIfIndexEntry);
            pM2Entry->mibIfTbl.ifIndex = pIfIndexEntry->ifIndex;
            m2IfCommonValsGet (pM2Entry, pIfIndexEntry);
	    semGive(m2InterfaceSem);
            return (OK);
            }

        memcpy ((char *)pM2Entry, (char *)&pM2Id->m2Data, sizeof (M2_DATA));
        pM2Entry->mibIfTbl.ifIndex = pIfIndexEntry->ifIndex;
        semGive (m2InterfaceSem);
        return (OK);
        }
    else
        {
        /* Declare variables for RFC 1213 support */
        
        M2_INTERFACETBL * pM2IntTblEntry = &pM2Entry->mibIfTbl;
        M2_NETDRVCNTRS    m2DrvCounters;

        bzero ((char *)pM2IntTblEntry, sizeof (M2_INTERFACETBL));

        /* Enter some of the values that we already know */
    
        pM2IntTblEntry->ifIndex = pIfIndexEntry->ifIndex;    
        
        /* Do what we used to do before for the RFC 1213 only counters */

        if ( (pIfNetEntry->if_ioctl != NULL) &&
                ((*pIfNetEntry->if_ioctl)(pIfNetEntry, SIOCGMIB2CNTRS,
                                         (caddr_t)&m2DrvCounters) != 0) )
            {
            m2IfDefaultValsGet (pM2Entry, pIfIndexEntry);
            }
        else
            {
	    /* Fill parameters with driver counters */

            m2IfDefaultValsGet (pM2Entry, pIfIndexEntry);
            pM2IntTblEntry->ifSpeed           = m2DrvCounters.ifSpeed;
            pM2IntTblEntry->ifInOctets        = m2DrvCounters.ifInOctets;
            pM2IntTblEntry->ifInDiscards      = m2DrvCounters.ifInDiscards;
            pM2IntTblEntry->ifInUnknownProtos = m2DrvCounters.ifInUnknownProtos;
            pM2IntTblEntry->ifOutOctets       = m2DrvCounters.ifOutOctets;
            pM2IntTblEntry->ifOutDiscards     = m2DrvCounters.ifOutDiscards;

            /* We can now do the second ioctl to get drv cnfg params */
            
            if ( (*pIfNetEntry->if_ioctl) &&
                 ((*pIfNetEntry->if_ioctl)(pIfNetEntry, SIOCGMIB2CNFG,
                                          (caddr_t)&ifDrvCnfg) == 0) )
                {
                /* Copy the ifType and the ifSpecific values */
                
                pM2IntTblEntry->ifType  = ifDrvCnfg.ifType;

                if (ifDrvCnfg.ifSpecific.idLength > 0)
                    {
	            bcopy (((char *) ifDrvCnfg.ifSpecific.idArray), 
		           ((char *) pM2IntTblEntry->ifSpecific.idArray),
		           ifDrvCnfg.ifSpecific.idLength * sizeof (long));
 
                    pM2IntTblEntry->ifSpecific.idLength =
                                        ifDrvCnfg.ifSpecific.idLength;
                    }
                }
            else
                {
                /*
                 * no info from ioctl, make an educated guess at
                 * the driver configuration
                 */

                if (strncmp(pIfNetEntry->if_name, "lo", 2) == 0)
                    {
                    /* IP Loop back interface. */

                    pM2IntTblEntry->ifType  =
                                     M2_ifType_softwareLoopback;
                    pM2IntTblEntry->ifSpeed = 0;
                    }
                else                      
                    if (strncmp(pIfNetEntry->if_name, "sl", 2) == 0)
                        {
                        /* Slip Network Interface */
 
                        pM2IntTblEntry->ifType  = M2_ifType_slip;
                        pM2IntTblEntry->ifSpeed = 19200; /* Baud Rate ??? */
                        }
                else
                    if (strncmp(pIfNetEntry->if_name, "ppp", 3) == 0)
                        {
                        /* PPP Network Interface */

                        pM2IntTblEntry->ifType  = M2_ifType_ppp;
                        pM2IntTblEntry->ifSpeed = 19200; /* Baud Rate ??? */
                        }
                else
                    {
                    /* Use Ethernet parameters as defaults */

                    pM2IntTblEntry->ifType  =
                                      M2_ifType_ethernet_csmacd;
                    pM2IntTblEntry->ifSpeed = 10000000;
                    }
                }
            }
        
        /* Fill in the other parameters */

        m2IfCommonValsGet (pM2Entry, pIfIndexEntry);        
        }

    semGive (m2InterfaceSem);
    return (OK);    
    }


/******************************************************************************
*
* m2IfDefaultValsGet - get the default values for the counters
*
* This function fills the given struct with the default values as
* specified in the RFC. We will enter this routine only if the ioctl
* to the driver fails.
*
* RETURNS: n/a
*/
void m2IfDefaultValsGet
    (
    M2_DATA *    pM2Data,        /* The requested entry */
    M2_IFINDEX * pIfIndexEntry   /* The ifindex node */
    )
    {
    struct ifnet * pIfNetEntry = (struct ifnet *)pIfIndexEntry->pIfStats;

    /* We will check only for loopback here */
    if (strncmp(pIfNetEntry->if_name, "lo", 2) == 0)
        pM2Data->mibIfTbl.ifType = M2_ifType_softwareLoopback;
    else
        pM2Data->mibIfTbl.ifType = M2_ifType_other;

    if (pIfIndexEntry->mibStyle == TRUE)
        {
        pM2Data->mibIfTbl.ifInOctets        = 0;
        pM2Data->mibIfTbl.ifInUcastPkts     = 0;
        pM2Data->mibIfTbl.ifInNUcastPkts    = 0;
        pM2Data->mibIfTbl.ifInDiscards      = 0;
        pM2Data->mibIfTbl.ifInErrors        = 0;
        pM2Data->mibIfTbl.ifInUnknownProtos = 0;

        pM2Data->mibIfTbl.ifOutOctets       = 0;
        pM2Data->mibIfTbl.ifOutUcastPkts    = 0;
        pM2Data->mibIfTbl.ifOutNUcastPkts   = 0;
        pM2Data->mibIfTbl.ifOutDiscards     = 0;
        pM2Data->mibIfTbl.ifOutErrors       = 0;

        pM2Data->mibIfTbl.ifOutQLen         = 0;
        }

    pM2Data->mibXIfTbl.ifInMulticastPkts = 0;
    pM2Data->mibXIfTbl.ifInBroadcastPkts = 0;

    pM2Data->mibXIfTbl.ifOutMulticastPkts = 0;
    pM2Data->mibXIfTbl.ifOutBroadcastPkts = 0;

    memset(&(pM2Data->mibXIfTbl.ifHCInOctets), 0, sizeof(UI64)); 
    memset(&(pM2Data->mibXIfTbl.ifHCInUcastPkts), 0, sizeof(UI64));
    memset(&(pM2Data->mibXIfTbl.ifHCInMulticastPkts), 0, sizeof(UI64));
    memset(&(pM2Data->mibXIfTbl.ifHCInBroadcastPkts), 0, sizeof(UI64));

    memset(&(pM2Data->mibXIfTbl.ifHCOutOctets), 0, sizeof(UI64));
    memset(&(pM2Data->mibXIfTbl.ifHCOutUcastPkts), 0, sizeof(UI64));
    memset(&(pM2Data->mibXIfTbl.ifHCOutMulticastPkts), 0, sizeof(UI64));
    memset(&(pM2Data->mibXIfTbl.ifHCOutBroadcastPkts), 0, sizeof(UI64));

    memset(pM2Data->mibXIfTbl.ifName, 0, M2DISPLAYSTRSIZE);

    pM2Data->mibXIfTbl.ifLinkUpDownTrapEnable = M2_LINK_UP_DOWN_TRAP_DISABLED;

    pM2Data->mibXIfTbl.ifHighSpeed = 0;

    pM2Data->mibXIfTbl.ifPromiscuousMode = M2_PROMISCUOUS_MODE_OFF; 

    pM2Data->mibXIfTbl.ifConnectorPresent = M2_CONNECTOR_NOT_PRESENT;

    memset(pM2Data->mibXIfTbl.ifAlias, 0, M2DISPLAYSTRSIZE);

    pM2Data->mibXIfTbl.ifCounterDiscontinuityTime = 0;

    return;
    }




/******************************************************************************
*
* m2IfCommonValsGet - get the common values
*
* This function updates the requested struct with all the data that is
* independent of the driver ioctl. This information can be obtained
* from the ifnet structures.
*
* RETURNS: n/a
*/
void m2IfCommonValsGet
    (
    M2_DATA *    pM2Data,        /* The requested struct */
    M2_IFINDEX * pIfIndexEntry   /* The ifindex node */
    )
    {
    struct ifnet *    pIfNetEntry    = (struct ifnet *)pIfIndexEntry->pIfStats;
    M2_INTERFACETBL * pM2IntTblEntry = &pM2Data->mibIfTbl;
    struct arpcom *   pArpcomEntry   = (struct arpcom *)pIfNetEntry;
    
    sprintf (pM2IntTblEntry->ifDescr, "%s%d",
                     pIfNetEntry->if_name, pIfNetEntry->if_unit);

    if (!pM2IntTblEntry->ifSpecific.idLength)
        {
        memcpy (pM2IntTblEntry->ifSpecific.idArray, pIfIndexEntry->ifOid.idArray,
            ((int) pIfIndexEntry->ifOid.idLength * sizeof (long)));

        pM2IntTblEntry->ifSpecific.idLength = pIfIndexEntry->ifOid.idLength;
        }

    pM2IntTblEntry->ifMtu = pIfNetEntry->if_mtu;

    /* If the device does not have a hardware address set it to zero */
 
    if (pIfNetEntry->if_flags &
                        (IFF_LOOPBACK | IFF_POINTOPOINT | IFF_NOARP))
        {
        memset(&(pM2IntTblEntry->ifPhysAddress), 0, sizeof(M2_PHYADDR));
        }
    else
        {
        bcopy (((char *) pArpcomEntry->ac_enaddr),
              ((char *) pM2IntTblEntry->ifPhysAddress.phyAddress),
                                                          ETHERADDRLEN);

        pM2IntTblEntry->ifPhysAddress.addrLength = ETHERADDRLEN;
        }

    /* Get the ifAdminStatus, ifLastChange, and ifOperStatus  */

    pM2IntTblEntry->ifAdminStatus = (((pIfNetEntry->if_flags & IFF_UP) != 0) ?
                                     M2_ifAdminStatus_up :
                                     M2_ifAdminStatus_down);

    /*
    pNetIf->netIfAdminStatus  = pIfReqEntry->ifAdminStatus;
    pM2IntTblEntry->ifLastChange = pIfNetEntry->netIfLastChange;
    */

    pM2IntTblEntry->ifOperStatus = (((pIfNetEntry->if_flags & IFF_UP) != 0) ?
                                    M2_ifOperStatus_up : M2_ifOperStatus_down);

    if (pIfIndexEntry->mibStyle != TRUE)
        {
        pM2IntTblEntry->ifInUcastPkts  = pIfNetEntry->if_ipackets -
                                         pIfNetEntry->if_imcasts;
        pM2IntTblEntry->ifInNUcastPkts = pIfNetEntry->if_imcasts;
        pM2IntTblEntry->ifInErrors     = pIfNetEntry->if_ierrors;
        pM2IntTblEntry->ifOutUcastPkts = pIfNetEntry->if_opackets -
                                         pIfNetEntry->if_omcasts;
        pM2IntTblEntry->ifOutNUcastPkts= pIfNetEntry->if_omcasts;
        pM2IntTblEntry->ifOutErrors    = pIfNetEntry->if_oerrors;
	pM2IntTblEntry->ifOutQLen      = pIfNetEntry->if_snd.ifq_len;

        /*
         * XXX Due to inconsistant increment between ifInUnknownProtos by
         * muxReceive and if_noproto by do_protocol_with_type, this routine
         * will synchronize both statistics here. In future, these increments
         * should be consistantly updated at the same place.
         */

        pIfNetEntry->if_noproto = pM2IntTblEntry->ifInUnknownProtos; /* XXX */
        }

    return;
    }




/******************************************************************************
*
* m2IfTblEntrySet - set the state of a MIB-II interface entry to UP or DOWN
*
* This routine selects the interface specified in the input parameter
* <pIfReqEntry> and sets the interface parameters to the requested state.  
* It is the responsibility of the calling routine to set the interface
* index, and to make sure that the state specified in the
* `ifAdminStatus' field of the structure at <pIfTblEntry> is a valid
* MIB-II state, up(1) or down(2).
*
* The fields that can be modified by this routine are the following:
* ifAdminStatus, ifAlias, ifLinkUpDownTrapEnable and ifName.
*
* RETURNS: OK, or ERROR if the input parameter is not specified, an interface
* is no longer valid, the interface index is incorrect, or the ioctl() command
* to the interface fails.
*
* ERRNO:
*   S_m2Lib_INVALID_PARAMETER
*   S_m2Lib_ENTRY_NOT_FOUND
*   S_m2Lib_IF_CNFG_CHANGED
*
* SEE ALSO:
* m2IfInit(), m2IfGroupInfoGet(), m2IfTblEntryGet(), m2IfDelete()
*/

STATUS m2IfTblEntrySet
    (
    void *        pIfReqEntry   /* pointer to requested entry to change */
    )
    {
    int             index;
    M2_IFINDEX *    pIfIndexEntry = NULL;
    struct ifnet *  pIfNetEntry = NULL;
    struct ifreq    ifReqChange;
    IF_SETENTRY *   pM2Entry = (IF_SETENTRY *)pIfReqEntry;
    M2_ID *         pM2Id = NULL;
    
    semTake (m2InterfaceSem, WAIT_FOREVER);

    /* Validate the bounds of the requested index  */

    index = pM2Entry->ifIndex;
    if (index < 0 || index > ifsInList)
        {
	errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        semGive (m2InterfaceSem);
        return (ERROR);
        }

    /* Get the node associated with the index */

    pIfIndexEntry = (M2_IFINDEX *) avlSearch (pM2IfRoot,
                                       (GENERIC_ARGUMENT)index, nextIndex);
    if (!pIfIndexEntry)
	{
	semGive (m2InterfaceSem);
        return (ERROR);
        }

    pIfNetEntry = (struct ifnet *) pIfIndexEntry->pIfStats;

    /*
     * The only supported state changes for the interface are UP and DOWN.
     * The TEST case is not supported by the drivers.
     */

    if (pM2Entry->varToSet & M2_varId_ifAdminStatus)
        {
        if (pM2Entry->ifAdminStatus == M2_ifAdminStatus_up)
            {
            /*
             * If the driver is in the UP state, there is no need to send the
             * IOCTL command, otherwise format the IOCTL request.
             */

            if ( (pIfNetEntry->if_flags & IFF_UP) == 0)
                ifReqChange.ifr_flags = pIfNetEntry->if_flags | IFF_UP;
            else
                pM2Entry->varToSet =
                     pM2Entry->varToSet & ~M2_varId_ifAdminStatus;
            }
        else
            {
            /* 
	     * If the driver is in the DOWN state, there is no need to send the
	     * IOCTL command, otherwise format the IOCTL request. 
	     */
 
            if ((pIfNetEntry->if_flags & IFF_UP) != 0)
                ifReqChange.ifr_flags = pIfNetEntry->if_flags & ~IFF_UP;
            else
                pM2Entry->varToSet = 
                      pM2Entry->varToSet & ~M2_varId_ifAdminStatus;
            }

        /* Stop if no work is needed. */

        if (pM2Entry->varToSet == 0)
	    {
	    semGive (m2InterfaceSem);
            return (OK);
	    }
        }

    /* Check to see if we have to set the promiscuous mode */

    if (pM2Entry->varToSet & M2_varId_ifPromiscuousMode)
        {
        if (pM2Entry->ifPromiscuousMode == M2_ifPromiscuousMode_on)
            ifReqChange.ifr_flags = pIfNetEntry->if_flags | IFF_PROMISC;
        else 
            ifReqChange.ifr_flags = pIfNetEntry->if_flags & ~IFF_PROMISC;
        }

    /* Do we need to call the Ioctl as a result of any of the above */

    if (pM2Entry->varToSet & (M2_varId_ifAdminStatus |
                                 M2_varId_ifPromiscuousMode))
        {
        /*
         * The network semaphore is not taken for this operation because the
         * called routine takes the network semphore.  Issue the IOCTL to the
         * driver routine, and verify that the request is honored.
         */

        sprintf (ifReqChange.ifr_name,"%s%d", pIfNetEntry->if_name,
						pIfNetEntry->if_unit);

        if ((*pIfIndexEntry->ifIoctl)(0, SIOCSIFFLAGS, (char *)&ifReqChange) != 0)
            {
            semGive (m2InterfaceSem);
            return (ERROR);
            }

        if ( (pIfNetEntry->if_ioctl != NULL) &&
             ((*pIfNetEntry->if_ioctl)(pIfNetEntry, SIOCGMIB2233,
                                       (caddr_t)&pM2Id) == OK) )
            {
	    if (pM2Entry->varToSet & M2_varId_ifAdminStatus)
		{
		(*pM2Id->m2VarUpdateRtn)(pM2Id, M2_varId_ifAdminStatus,
					(caddr_t)pM2Entry->ifAdminStatus);
                }
            if (pM2Entry->varToSet & M2_varId_ifPromiscuousMode)
		{
		(*pM2Id->m2VarUpdateRtn)(pM2Id, M2_varId_ifPromiscuousMode,
					(caddr_t)pM2Entry->ifPromiscuousMode);
                }
            }
        }
    else 
        {
        /* 
         * For the others, we have to get a pointer to the interface 
         * table, so we call the SIOCSMIB2233 IOCTL and let the driver
         * call our appropriate set routine. In this case, the set
         * routine is m2IfSnmpSet().
         */

        if ( (pIfNetEntry->if_ioctl) &&
             ((*pIfNetEntry->if_ioctl) (pIfNetEntry, SIOCSMIB2233, 
                                              (caddr_t)pIfReqEntry) != 0) )
            {
            semGive (m2InterfaceSem);
            return (ERROR);
            }
        }

    semGive (m2InterfaceSem);
    return (OK);    
    }




/******************************************************************************
*
* m2IfGroupInfoGet -  get the MIB-II interface-group scalar variables
*
* This routine fills the interface-group structure at <pIfInfo> with
* the values of MIB-II interface-group global variables.
*
* RETURNS: OK, or ERROR if <pIfInfo> is not a valid pointer.
*
* ERRNO:
* S_m2Lib_INVALID_PARAMETER
*
* SEE ALSO:
* m2IfInit(), m2IfTblEntryGet(), m2IfTblEntrySet(), m2IfDelete()
*/

STATUS m2IfGroupInfoGet
    (
    M2_INTERFACE * pIfInfo	/* pointer to interface group structure */
    )
    {
    semTake (m2InterfaceSem, WAIT_FOREVER);
 
    /* Validate the input parameter pointer */

    if (pIfInfo == NULL)
        {
        semGive (m2InterfaceSem);
	errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
        }

    /*
     * Number of network interfaces in the system independent of
     * their state
     */
 
    pIfInfo->ifNumber = m2IfCount;  
#ifdef VIRTUAL_STACK
    pIfInfo->ifTableLastChange = _ifTableLastChange;
    pIfInfo->ifStackLastChange = _ifStackLastChange;
#else    /* VIRTUAL_STACK */
    pIfInfo->ifTableLastChange = ifTableLastChange;
    pIfInfo->ifStackLastChange = ifStackLastChange;
#endif   /* VIRTUAL_STACK */
 
    semGive (m2InterfaceSem);
    return OK;
    }





/******************************************************************************
*
* m2IfStackTblUpdate - update the relationship between the sub-layers
*
* This function must be called to setup the relationship between the
* ifIndex values for each sub-layer. This information is required to 
* support the ifStackTable for RFC 2233. Using this data, we can easily
* determine which sub-layer runs on top of which other. 
*
* <action> is either M2_STACK_TABLE_INSERT or M2_STACK_TABLE_REMOVE.
*
* Each AVL node keeps a linked list of all the layers that are 
* directly beneath it. Thus by walking through the AVL nodes in an
* orderly way, we can understand the relationships between all the 
* interfaces.
*
* RETURNS: OK upon successful addition
*          ERROR otherwise.
*/

STATUS m2IfStackTblUpdate
    (
    UINT    lowerIndex,        /* The ifIndex of the lower sub-layer */
    UINT    higherIndex,        /* The ifIndex of the higher sub-layer */
    int     action             /* insert or remove */
    )
    {
    M2_IFINDEX *      pIfIndexLow = NULL;
    M2_IFINDEX *      pIfIndexHigh = NULL;
    M2_IFSTACKTBL *   pIfStackPtr = NULL;
    M2_IFSTACKTBL **  ppTempPtr = NULL;    

    /* Check to see if the indexes are within the limits */

    if ( (lowerIndex < (UINT) 0) || (lowerIndex > (UINT) ifsInList) ||
         (higherIndex < (UINT) 0) || (higherIndex > (UINT) ifsInList) )
        {
        return (ERROR);
        }

    semTake(m2InterfaceSem, WAIT_FOREVER);

    /* Get the nodes associated with the interfaces */

    pIfIndexLow = (M2_IFINDEX *) avlSearch (pM2IfRoot,
                                 (GENERIC_ARGUMENT)lowerIndex, nextIndex);
    pIfIndexHigh = (M2_IFINDEX *) avlSearch (pM2IfRoot,
                                 (GENERIC_ARGUMENT)higherIndex, nextIndex);
    if ( (!pIfIndexLow) || (!pIfIndexHigh) )
	{
	semGive(m2InterfaceSem);
        return (ERROR);
	};

    /* Check the slot to insert a new index */

    pIfStackPtr = pIfIndexHigh->pNextLower;
    ppTempPtr = &pIfIndexHigh->pNextLower;

    if (action == M2_STACK_TABLE_INSERT)
        {
	while (pIfStackPtr && pIfStackPtr->index < lowerIndex)
	    {
	    ppTempPtr = &pIfStackPtr->pNextLower;
	    pIfStackPtr = pIfStackPtr->pNextLower;
	    }
        *ppTempPtr = (M2_IFSTACKTBL*) malloc(sizeof(M2_IFSTACKTBL));
        (*ppTempPtr)->index = lowerIndex;
	(*ppTempPtr)->pNextLower = pIfStackPtr;
	(*ppTempPtr)->status = ROW_ACTIVE;
	}
    else
	{
	while (pIfStackPtr && pIfStackPtr->index < lowerIndex)
	    {
	    ppTempPtr = &pIfStackPtr->pNextLower;
	    pIfStackPtr = pIfStackPtr->pNextLower;
	    }
        if (pIfStackPtr && pIfStackPtr->index == lowerIndex)
	    {
	    *ppTempPtr = pIfStackPtr->pNextLower;
	    free(pIfStackPtr);
	    }
        }
    
#ifdef VIRTUAL_STACK
    _ifStackLastChange = centiSecsGet();
#else    /* VIRTUAL_STACK */
    ifStackLastChange = centiSecsGet();
#endif   /* VIRTUAL_STACK */

    semGive(m2InterfaceSem);

    return (OK);
    }


/******************************************************************************
*
* stackEntryIsTop - test if an ifStackTable interface has no layers above
*
* This routine returns TRUE if an interface is not below any other interface.
* That is, it returns TRUE if the given interface is topmost on a stack.  This
* helper function is not exported.
*
* RETURNS: 
*  TRUE is interface is topmost
*  FALSE otherwise or for errors
*
*/
BOOL stackEntryIsTop
    (
    int index  /* the interface to examine */
    )
    {
    M2_IFINDEX *     pIfIndexEntry;
    M2_IFSTACKTBL *  pIfStackEntry;
    UINT i = 0;

    while ((pIfIndexEntry = (M2_IFINDEX*) avlSuccessorGet (pM2IfRoot,
	(GENERIC_ARGUMENT)i, nextIndex)))
	{
	pIfStackEntry = pIfIndexEntry->pNextLower;
	while (pIfStackEntry)
	    {
	    if (pIfStackEntry->index == (UINT) index)
		{
		return FALSE;
		}
            pIfStackEntry = pIfStackEntry->pNextLower;
	    }
        i = pIfIndexEntry->ifIndex;
	}
    return TRUE;
    }

/******************************************************************************
*
* stackEntryIsBottom - test if an interface has no layers beneath it
*
* This routine returns TRUE if an interface has no layers beneath it.  This
* helper function is not exported.
*
* RETURNS: 
*  TRUE if the interface is the bottom-most layer in a stack
*  FALSE otherwise or on error
*
*/
BOOL stackEntryIsBottom
    (
    int index   /* interface to examine */
    )
    {
    M2_IFINDEX *     pIfIndexEntry;
    pIfIndexEntry = (M2_IFINDEX*) avlSearch (pM2IfRoot,
	(GENERIC_ARGUMENT)index, nextIndex);
    if (pIfIndexEntry == 0 || pIfIndexEntry->pNextLower != 0)
	{
	return FALSE;
	}
    return TRUE;
    }

/******************************************************************************
*
* m2IfStackEntryGet -  get a MIB-II interface-group table entry
*
* This routine maps the given high and low indexes to the interfaces in the
* AVL tree. Using the <high> and <low> indexes, we retrieve the nodes in
* question and walk through their linked lists to get to the right relation.
* Once we get to the correct node, we can return the values based on the
* M2_EXACT_VALUE and the M2_NEXT_VALUE searches.
*
* RETURNS: 
* OK, or ERROR if the input parameter is not specified, or a match is not found.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*
*/
STATUS m2IfStackEntryGet
    (
    int             search,       /* M2_EXACT_VALUE or M2_NEXT_VALUE */
    int *           pHighIndex,   /* the higher layer's ifIndex */
    M2_IFSTACKTBL * pIfReqEntry   /* pointer to the requested entry */
    )
    {
    M2_IFINDEX *    pIfIndexEntry = NULL;
    M2_IFSTACKTBL * pIfStackEntry = NULL;
    
    /* Validate the pointer to the requested structure */
    
    if (pIfReqEntry == NULL)
        {
        errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
        }

    semTake (m2InterfaceSem, WAIT_FOREVER);

    /*
     *In addition to the relationships created by m2IfStackTblUpdate()
     * there exists up to 2 virtual relationships for each interface.
     * These virtual relationships are in the form 0.x and x.0
     * The virtual relationship 0.x is present if interface x is not
     * below a real interface.  The virtual relationship x.0 is present
     * if interface x has not real interfaces below it.
     *
     * These virtual relationships have to be handled as
     * special cases because they are not represented in the stack table
     * data structure.
     */

    if (*pHighIndex == 0)
        {
	if (search == M2_EXACT_VALUE)
	    {
	    pIfIndexEntry = (M2_IFINDEX*) avlSearch (pM2IfRoot,
			      (GENERIC_ARGUMENT)pIfReqEntry->index, nextIndex);
            if (pIfIndexEntry)
		{
		if (stackEntryIsTop(pIfReqEntry->index) == FALSE)
		    {
		    goto errorReturn;
		    }
		pIfReqEntry->index = pIfIndexEntry->ifIndex;
		pIfReqEntry->status = ROW_ACTIVE;
		goto goodReturn;
                }
            goto errorReturn;
	    }
        else
	    {
	    pIfIndexEntry = (M2_IFINDEX*) avlSuccessorGet (pM2IfRoot,
			      (GENERIC_ARGUMENT) pIfReqEntry->index,
			      nextIndex);
            while (pIfIndexEntry != 0 && stackEntryIsTop(pIfIndexEntry->ifIndex)
		  == FALSE)
	        {
		pIfIndexEntry = (M2_IFINDEX*) avlSuccessorGet (pM2IfRoot,
				    (GENERIC_ARGUMENT) pIfIndexEntry->ifIndex,
				    nextIndex);
                }
            if (pIfIndexEntry)
		{
		pIfReqEntry->index = pIfIndexEntry->ifIndex;
		pIfReqEntry->status = ROW_ACTIVE;
		goto goodReturn;
                }

            pIfIndexEntry = (M2_IFINDEX *) avlSuccessorGet
                    (pM2IfRoot, (GENERIC_ARGUMENT)0, nextIndex);

            if (!pIfIndexEntry)
                goto errorReturn;

            pIfReqEntry->index = 0;
            pIfReqEntry->status = ROW_ACTIVE;
            *pHighIndex = pIfIndexEntry->ifIndex;
	    if (stackEntryIsBottom(pIfIndexEntry->ifIndex))
		{
		goto goodReturn;
		}
	    }
        }

    pIfIndexEntry = (M2_IFINDEX *) avlSearch (pM2IfRoot,
                                  (GENERIC_ARGUMENT)*pHighIndex, nextIndex);
    if (pIfIndexEntry == 0)
	goto errorReturn;

    /*
     * Doing a next match.  The next relation after <interface>.<last_lower> is
     * the virtual relation <next_interface>.0 if <next_interface> has no
     * lower layers.
     */

    if (search != M2_EXACT_VALUE)
        {
	while (1)
	    {
            pIfStackEntry = pIfIndexEntry->pNextLower;

            /* Doing a next match */
            while (pIfStackEntry)
                {
                if (pIfStackEntry->index > pIfReqEntry->index)
                    {
                    /* We found a next within the same high index node */
                
                    pIfReqEntry->index = pIfStackEntry->index;
                    pIfReqEntry->status = pIfStackEntry->status;
                    goto goodReturn;
                    }

                /*
                 * Check to see if another lower layer index is available
                 * for this index. If it is, loop through again, else
	         * start searching the next interface.
                 */
            
                pIfStackEntry = pIfStackEntry->pNextLower;
                }

            pIfIndexEntry = (M2_IFINDEX *) avlSuccessorGet
                (pM2IfRoot, (GENERIC_ARGUMENT)*pHighIndex, nextIndex);

            if (pIfIndexEntry == 0)
		goto errorReturn;

            *pHighIndex = pIfIndexEntry->ifIndex;

            if (stackEntryIsBottom(pIfIndexEntry->ifIndex) == TRUE)
		{
		pIfReqEntry->index = 0;
		pIfReqEntry->status = ROW_ACTIVE;
		goto goodReturn;
		}
            }
        }

    /*
     * Doing an exact match case.
     */

    if (pIfReqEntry->index == 0 && stackEntryIsBottom(*pHighIndex))
        {
        pIfReqEntry->status = ROW_ACTIVE;
        goto goodReturn;
        }

    while (pIfStackEntry)
        {
        if (pIfStackEntry->index == pIfReqEntry->index)
            {
            pIfReqEntry->status = pIfStackEntry->status;
            goto goodReturn;
            }
        pIfStackEntry = pIfStackEntry->pNextLower;
        }

    goto errorReturn;

errorReturn:
    semGive (m2InterfaceSem);
    errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
    return (ERROR);

goodReturn:
    semGive (m2InterfaceSem);
    return (OK);
    }



/******************************************************************************
*
* m2IfStackEntrySet - modify the status of a relationship
*
* This routine selects the interfaces specified in the input parameters
* <pIfReqEntry> and <highIndex> and sets the interface's status to the 
* requested state.  
*
* RETURNS: OK, or ERROR if the input parameter is not specified, an interface
* is no longer valid, or the interface index is incorrect.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*  S_m2Lib_IF_CNFG_CHANGED
*
*/
STATUS m2IfStackEntrySet
    (
    int             highIndex,        /* The higher layer's ifIndex */
    M2_IFSTACKTBL * pIfReqEntry       /* The requested entry */
    )
    {
    M2_IFINDEX *    pIfIndexEntry = NULL;
    M2_IFSTACKTBL * pIfStackEntry = NULL;
    M2_IFSTACKTBL **ppTemp = NULL;

    /* Check for insert before anything else because m2IfStackTblUpdate()
     * does all the necessary checking.
     */
    if (pIfReqEntry->status == ROW_ACTIVE)
	{
	return m2IfStackTblUpdate(pIfReqEntry->index, highIndex, M2_STACK_TABLE_INSERT);
	}
    
    if (pIfReqEntry->status == ROW_DESTROY || pIfReqEntry->status == ROW_NOTINSERVICE)
	{
	return m2IfStackTblUpdate(pIfReqEntry->index, highIndex, M2_STACK_TABLE_REMOVE);
	}

    semTake (m2InterfaceSem, WAIT_FOREVER);

    /* Validate the bounds of the requested index  */

    if (highIndex < 0 || highIndex >= ifsInList)
        {
	errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        semGive (m2InterfaceSem);
        return (ERROR);
        }

    /*
     * Get the node associated with the higher index, because this
     * is where we will be making the change.
     */

    pIfIndexEntry = (M2_IFINDEX *) avlSearch (pM2IfRoot,
                                 (GENERIC_ARGUMENT)highIndex, nextIndex);
    if (!pIfIndexEntry)
	{
	errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
	semGive(m2InterfaceSem);
        return (ERROR);
	}
    
    pIfStackEntry = pIfIndexEntry->pNextLower;
    ppTemp = &pIfIndexEntry->pNextLower;
    
    while (pIfStackEntry)
        {
        if (pIfStackEntry->index == pIfReqEntry->index)
	    break;

        ppTemp = &pIfStackEntry->pNextLower;
        pIfStackEntry = pIfStackEntry->pNextLower;
        }

    if (pIfStackEntry == 0)
	{
	errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
	semGive(m2InterfaceSem);
	return ERROR;
	}

#ifdef VIRTUAL_STACK
    _ifStackLastChange = centiSecsGet();
#else    /* VIRTUAL_STACK */
    ifStackLastChange = centiSecsGet();
#endif   /* VIRTUAL_STACK */

    semGive(m2InterfaceSem);
    return (OK);
    }




/******************************************************************************
*
* m2IfRcvAddrEntryGet - get the rcvAddress table entries for a given address
*
* This function returns the exact or the next value in the ifRcvAddressTable
* based on the value of the search parameter. In order to identify the
* appropriate entry, this function needs two identifiers - the ifIndex of the
* interface and the physical address for which the status or the type is
* being requested. For a M2_EXACT_VALUE search, this function returns the
* status and the type of the physical address in the instance. For a 
* M2_NEXT_VALUE search, it returns the type and status of the lexicographic
* successor of the physical address seen in the instance.
*
* RETURNS: OK, or ERROR if the input parameter is not specified, an interface
* is no longer valid, or the interface index is incorrect.
*
* ERRNO:
*   S_m2Lib_INVALID_PARAMETER
*   S_m2Lib_ENTRY_NOT_FOUND
*   S_m2Lib_IF_CNFG_CHANGED
*
*/
STATUS m2IfRcvAddrEntryGet
    (
    int               search,         /* exact search or next search */
    int *             pIndex,         /* pointer to the ifIndex */
    M2_IFRCVADDRTBL * pIfReqEntry     /* struct for the values */
    )
    {
    int               index;
    int               cmp;
    M2_IFINDEX *      pIfIndexEntry = NULL;
    M2_IFRCVADDRTBL * pRcvAddrEntry = NULL;
    
    /* Validate the pointer to the requested structure */

    if (pIfReqEntry == NULL)
        {
        errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
        }

    index = *pIndex;
    
    semTake (m2InterfaceSem, WAIT_FOREVER);

    if ( (search == M2_NEXT_VALUE) && (index == 0) )
        {
        do
            {
            pIfIndexEntry = (M2_IFINDEX *) avlSuccessorGet (pM2IfRoot,
                                  (GENERIC_ARGUMENT)index, nextIndex);

            if (!pIfIndexEntry)
                goto errorReturn;

            pRcvAddrEntry = pIfIndexEntry->pRcvAddr;
            index = pIfIndexEntry->ifIndex;
            } while (!pRcvAddrEntry);
        }
    else
        {
        pIfIndexEntry = (M2_IFINDEX *) avlSearch (pM2IfRoot,
                                  (GENERIC_ARGUMENT)index, nextIndex);

        pRcvAddrEntry = pIfIndexEntry->pRcvAddr;

        /*
         * If index is not zero, even though we have a next search,
         * we would still need to get the node associated with the
         * current index. The algorithm is as follows:
         * - Get the node associated with the current index.
         * - Look thru the list of addresses to see if we have a match.
         * - For an exact search, we return error upon no match, else
         *   we just pick up the next address, if any.
         * - If no address lexicographically greater is present in this
         *   node, we get the next node and do the above.
         */

        while (pRcvAddrEntry)
            {
            /* compare the physical addresses */
            cmp = bcmp (pIfReqEntry->ifRcvAddrAddr.phyAddress,
                    pRcvAddrEntry->ifRcvAddrAddr.phyAddress, ETHERADDRLEN);

            if (cmp == 1)
                /* the reqested entry is greater than any entry in our base */
                goto errorReturn;

            if ( ((cmp == 0) && (search == M2_NEXT_VALUE)) || (cmp == -1) )
                {
                /*
                 * we come here only in one of the two cases:
                 * either we encountered a match and are looking for a next
                 * OR we still haven't found a match
                 */
                pRcvAddrEntry = pRcvAddrEntry->pNextEntry;
                if (!pRcvAddrEntry)
                    {
                    /* no more addresses in this avl node, get next index */
                    if (search == M2_EXACT_VALUE)
                        goto errorReturn;

                    /* For a next search, search the next avl node */
                    do
                        {
                        index = pIfIndexEntry->ifIndex;
                        pIfIndexEntry = (M2_IFINDEX *) avlSuccessorGet
                            (pM2IfRoot, (GENERIC_ARGUMENT)index, nextIndex);

                        if (!pIfIndexEntry)
                            goto errorReturn;                        
                        } while (!pIfIndexEntry->pRcvAddr);
                    
                    pRcvAddrEntry = pIfIndexEntry->pRcvAddr;
                    }

                /* Continue with the while loop if there is a valid addr */
                continue;
                }

            /* Found a perfect match and search is M2_EXACT_VALUE */
            break;
            }
        }
    

    /* found a match, copy the values */

    *pIndex = pIfIndexEntry->ifIndex;
    bcopy ((char *)pRcvAddrEntry->ifRcvAddrAddr.phyAddress,
               (char *)pIfReqEntry->ifRcvAddrAddr.phyAddress, ETHERADDRLEN);
    pIfReqEntry->ifRcvAddrAddr.addrLength = ETHERADDRLEN;
    pIfReqEntry->ifRcvAddrStatus = pRcvAddrEntry->ifRcvAddrStatus;
    pIfReqEntry->ifRcvAddrType = pRcvAddrEntry->ifRcvAddrType;

    semGive (m2InterfaceSem);
    return OK;

errorReturn:
    semGive (m2InterfaceSem);
    errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
    return (ERROR);
    }




/******************************************************************************
*
* m2IfRcvAddrEntrySet - modify the entries of the rcvAddressTable
*
* This function modifies the status and type fields of a given receive address
* associated with a given interface. <varToSet> identifies the fields for 
* which the change is being requested.  We can also add multicast addresses
* by creating a new row in the table. The physical address is stripped from
* the instance value of the SNMP request. This routine does not allow the
* deletion of a unicast address. Neither does it allow the unicast address
* to be modified or created.
*
* RETURNS: OK, or ERROR if the input parameter is not specified, an interface
* is no longer valid, the interface index is incorrect, or the ioctl() command
* to the interface fails.
*
* ERRNO:
*   S_m2Lib_INVALID_PARAMETER
*   S_m2Lib_ENTRY_NOT_FOUND
*   S_m2Lib_IF_CNFG_CHANGED
*
* SEE ALSO:
* m2IfInit(), m2IfGroupInfoGet(), m2IfTblEntryGet(), m2IfDelete()
*/
STATUS m2IfRcvAddrEntrySet
    (
    int               varToSet,       /* entries that need to be modified */
    int               index,          /* search type */
    M2_IFRCVADDRTBL * pIfReqEntry     /* struct containing the new values */
    )
    {
    int               cmp;
    M2_IFINDEX *      pIfIndexEntry = NULL;
    M2_IFRCVADDRTBL * pRcvAddrEntry = NULL;
    M2_IFRCVADDRTBL **ppTemp = NULL;
    LIST *            pMCastList = NULL;
    ETHER_MULTI *     pEtherMulti = NULL;
    struct arpcom *   pIfArpcom = NULL;

    if (pIfReqEntry == NULL)
        {
        errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
        }

    semTake (m2InterfaceSem, WAIT_FOREVER);

    pIfIndexEntry = (M2_IFINDEX *) avlSearch (pM2IfRoot,
                                  (GENERIC_ARGUMENT)index, nextIndex);
    pIfArpcom = (struct arpcom *)pIfIndexEntry->pIfStats;

    pRcvAddrEntry = pIfIndexEntry->pRcvAddr;
    ppTemp = &pIfIndexEntry->pRcvAddr;
    
    while (pRcvAddrEntry)
        {
        cmp = bcmp ((char *)pIfReqEntry->ifRcvAddrAddr.phyAddress,
                (char *)pRcvAddrEntry->ifRcvAddrAddr.phyAddress, ETHERADDRLEN);

        if (cmp == 0)
            break;

        if (cmp == -1)
            goto errorReturn;

        ppTemp = &pRcvAddrEntry->pNextEntry;; 
        pRcvAddrEntry = pRcvAddrEntry->pNextEntry;
        }

    /* get the list pointer for the mcast address */

    if ( (pIfArpcom->ac_if.if_ioctl != NULL) &&
             ((*pIfArpcom->ac_if.if_ioctl)
              ((struct ifnet *)pIfArpcom, SIOCGMCASTLIST,
                                                (caddr_t)&pMCastList) != 0) )
        {
        goto errorReturn;        
        }
    
    if ( (!pRcvAddrEntry) &&
         ((pIfReqEntry->ifRcvAddrStatus == ROW_CREATEANDGO) ||
          (pIfReqEntry->ifRcvAddrStatus == ROW_CREATEANDWAIT)) )
        {
        /* request to create a new entry */

        pEtherMulti = KHEAP_ALLOC(sizeof (ETHER_MULTI));
        if (!pEtherMulti)
            goto errorReturn;

        bcopy ((char *)pIfReqEntry->ifRcvAddrAddr.phyAddress,
                                   pEtherMulti->addr, ETHERADDRLEN);
        pEtherMulti->refcount = 0;

        if (rcvEtherAddrAdd (pIfIndexEntry,
                      pIfReqEntry->ifRcvAddrAddr.phyAddress) == OK)
            {
            lstAdd (pMCastList, (NODE *)pEtherMulti) ;
            }
        else
            goto errorReturn;
        }

    /* perform the set operation */

    if (pRcvAddrEntry)
        {
        if (varToSet & M2_IFRCVADDRSTATUS)
            {
            switch (pIfReqEntry->ifRcvAddrStatus)
                {
                case ROW_NOTINSERVICE:
                case ROW_NOTREADY:
                case ROW_DESTROY:
                    *ppTemp = pRcvAddrEntry->pNextEntry;
                    KHEAP_FREE((char *)pRcvAddrEntry);
                    break;

                case ROW_ACTIVE:
                default:
                    break;
                }
            }

        if (varToSet & M2_IFRCVADDRTYPE)
            {
            pRcvAddrEntry->ifRcvAddrType = pIfReqEntry->ifRcvAddrType;
            }
        }
  
    semGive(m2InterfaceSem);
    return OK;

errorReturn:
    semGive (m2InterfaceSem);
    errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
    return (ERROR);
    }




/*******************************************************************************
*
* m2IfDelete - delete all resources used to access the interface group
*
* This routine frees all the resources allocated at the time the group was
* initialized.  The interface group should not be accessed after this routine 
* has been called.
*
* RETURNS: OK, always.
*
* SEE ALSO:
* m2IfInit(), m2IfGroupInfoGet(), m2IfTblEntryGet(), m2IfTblEntrySet()
*/
STATUS m2IfDelete (void)
    {
    /* Take the interface semaphore to delete the entries */

    semTake (m2InterfaceSem, WAIT_FOREVER);
    avlTreeErase (pM2IfRootPtr); 
    semGive (m2InterfaceSem);

    /* Release the interface semaphore */

    if (m2InterfaceSem != NULL)
        {
        semDelete (m2InterfaceSem);
        m2InterfaceSem = NULL;
        }

    /* Free Network I/F Table Struct */
 
    if (pm2IfTable != NULL)
        {
        KHEAP_FREE((char *)pm2IfTable);
        pm2IfTable = NULL;
        }

    /* Clear trap routine variables */

    pM2TrapRtn    = NULL;
    pM2TrapRtnArg = NULL;

    return (OK);
    }




/******************************************************************************
*
* nextIndex - the comparison routine for the AVL tree
*
* This routine compares the two indexes and returns a code based on wether
* the index, in question, is lesser than, equal to or greater than the one
* being compared.
*
* RETURNS
* -1, if the given index is lesser; 0, if equal; and 1, if greater.
*/
int nextIndex
    (
    void *            pAvlNode,        /* The node to compare with */
    GENERIC_ARGUMENT  key              /* The given index */
    )
    {
    UINT             index;
    M2_IFINDEX *    pTmpIfIndex;

    pTmpIfIndex = (M2_IFINDEX *)pAvlNode;
    index = pTmpIfIndex->ifIndex;

    return ((key.u < index) ? -1 : (key.u == index) ? 0 : 1); 
    }





/***************************************************************************
 *
 * m2SetIfLastChange - set the ifLastChange MIB variable for the interface
 *
 * This routine sets the ifLastChange and ifAdminStatus for the interface
 * specified by the device and unit arguments.
 *
 * NOMANUAL
 *
 * RETURNS: OK, or ERROR if the interface isn't found in pm2IfTable
 *
 * SEE ALSO:
 * m2IfInit(), m2IfGroupInfoGet(), m2IfTblEntryGet(), m2IfTblEntrySet()
 */
STATUS m2SetIfLastChange
    (
    int    ifIndex
    )
    {
    long           adminStatus;
    M2_ID *        pM2Id = NULL;
    struct ifnet * pIfNetEntry = NULL;
    M2_IFINDEX *   pIfIndexEntry = NULL;

    semTake (m2InterfaceSem, WAIT_FOREVER);

    /* 
     * As the ifnet flags has changed, we must change the ifAdminStatus
     * change the ifAdminStatus in the MIB table. To do this, we have to 
     * request the driver to provide a pointer to the MIB data and that is
     * possible only via an IOCTL. So, we must first get the ifnet structure.
     */

    pIfIndexEntry = (M2_IFINDEX *) avlSearch (pM2IfRoot, 
                                          (GENERIC_ARGUMENT)ifIndex, nextIndex);

    if (pIfIndexEntry == NULL)
        {
        semGive (m2InterfaceSem);
        errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        return (ERROR);
        }

    /* 
     * In order to issue an IOCTL, we must know the type of MIB table that
     * the driver supports ... RFC2233 or RFC1213.
     */

    pIfNetEntry = (struct ifnet *) pIfIndexEntry->pIfStats;
    adminStatus = ((pIfNetEntry->if_flags & IFF_UP) != 0) ? 
                                    M2_ifAdminStatus_up : M2_ifAdminStatus_down;
        
    if (pIfIndexEntry->mibStyle == TRUE)
        {
        /* 
         * RFC 2233 style ... If IOCTL fails, there is nothing we can do, so
         * we simply return saying OK. 
         */

        if ( (pIfNetEntry->if_ioctl != NULL) &&
                                   ((*pIfNetEntry->if_ioctl)(pIfNetEntry, 
                                         SIOCGMIB2233, (caddr_t)&pM2Id) == 0) )
            {
            if (pM2Id != NULL)
                {
                pM2Id->m2VarUpdateRtn (pM2Id, M2_varId_ifAdminStatus, 
                                                         (caddr_t)adminStatus);
                pM2Id->m2VarUpdateRtn (pM2Id, M2_varId_ifLastChange, 
                                                      (caddr_t)centiSecsGet());
                }
            }
        }
    else
        {
        /* 
         * RFC 1213 style
         * We get hold of the END_OBJ so that we can update the ifLastChange
         * field in the mib2Tbl structure. Ugh.... This is a very bad way
         * of doing it, but there is no other way currently. Once all
         * drivers support RFC 2233, this can be removed.
         */

        M2_INTERFACETBL * pMib2Tbl = NULL;

        pMib2Tbl = (M2_INTERFACETBL *) 
                                ifnetToEndFieldsGet (pIfNetEntry, END_OBJ_MIB);

        if (pMib2Tbl == NULL)
            {
            semGive (m2InterfaceSem);
            return (ERROR);
            }

        pMib2Tbl->ifLastChange = centiSecsGet ();
        pMib2Tbl->ifAdminStatus = adminStatus;
        }

    semGive (m2InterfaceSem);

    return (OK);
    }




/***************************************************************************
 *
 * ifnetToEndFieldsGet - get the END object's various fields given the ifnet 
 *                       pointer
 *
 * This routine returns a pointer to the requested field in the END_OBJ
 * structure. The values for the <field> parameter are defined at the 
 * beginning of this module.
 *
 * NOMANUAL
 *
 * RETURNS: pointer to the requested field of the END_OBJ structure.
 *          NULL if the field requested is not defined in this module or
 *               if a NULL pointer was encountered somewhere.
 *
 */

LOCAL void * ifnetToEndFieldsGet
    (
    struct ifnet   *pIfNet,       /* ifnet pointer */
    int             field         /* field of the END_OBJ */
    )
    {
    END_OBJ     *pEnd = NULL;
    IP_DRV_CTRL *pDrvCtrl = NULL;

    if (pIfNet)
        {
        pDrvCtrl = (IP_DRV_CTRL *) pIfNet->pCookie;

        if ( (pDrvCtrl) && 
                ((pEnd = PCOOKIE_TO_ENDOBJ (pDrvCtrl->pIpCookie)) != NULL) )
            {
            switch (field)
                {
                case END_OBJ_MIB:
                    return (&pEnd->mib2Tbl);

                case END_OBJ_FLAGS:
                    return (&pEnd->flags);

                default:
                    break;
                }
            }
        }

    return (NULL);
    }

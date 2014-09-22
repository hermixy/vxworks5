/* muxLib.c - MUX network interface library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
03z,10may02,wap  Remove unnecessary redeclaration of endList (SPR #74201)
03y,07may02,kbw  man page edits
03x,25apr02,vvv  return on muxTkBindUpdate error in muxBind (SPR #74042)
03w,22apr02,wap  Don't call muxTkUnbindUpdate() on invalid/free()'ed pointer
                 (SPR #74861)
03v,05nov01,vvv  fixed compilation warnings
03u,26oct01,vvv  added NULL check for END object in muxEndFlagsNotify
		 (SPR #69540)
03t,15oct01,rae  merge from truestack ver 03o, base 02n(?) (SPRs 69573,70722,
                 64471, 64473, 64406, 32626, ROUTER_STACK, etc.)
03s,14jun01,vvv  fixed compilation warnings
03r,08jun01,vvv  added missing closing for-loop closing bracket in 
		 muxReceive
03q,10apr01,rae  muxReceive only calls interested protocols (SPR #65557)
03p,09nov00,spm  removed pNptCookie from END_OBJ for binary compatibility
03o,07nov00,spm  removed nptFlag from END_OBJ for T2 END binary compatibility
03n,30oct00,kbw  putting back the NOMANUAL per request of dgross
03m,30oct00,kbw  removing NOMANUAL assignemnt to muxReceive, OK'd by GNN
03m,24oct00,niq  Merging in RFC2233 changes fron tor2_0.open_stack-f1 branch
                 03n,24oct00,ann  fixed a bug in the EIOCGMCASTLIST part of
                 muxIoctl
                 03m,16may00,ann  merging from post R1 on openstack branch to
                 incorporate RFC2233 implementation
                 03k,10mar00,ead  fixed MIB-2 counter updates to use new 
                 RFC 2233 m2IfLib
03l,17oct00,niq  Update pEnd->snarfCount only when protocol addition is
                 complete
03k,16oct00,spm  merged from version 03a of tor3_0_x branch (base version 02n):
                 adds multiple SNARF protocol support, bug fixes and cleanup,
                 and backward compatibility fixes
03i,07jul99,pul  modify description for man page generation
03h,30apr99,pul  muxIoctl: check for error before calling muxEndFlagsNotify 
03g,29apr99,pul  muxPollSend: Call endpacketDataGet directly instead of 
		 muxPacketDataGet 
03f,29apr99,pul  Upgraded NPT phase3 code to tor2.0.0
03e,31mar99,spm  removed \040 codes from muxMCastAddrGet() entry (SPR #26268)
03d,30mar99,pul  removed all references to muxSvcFunc and muxMCastFunc
03c,25mar99,sj   detect if the end.flags actually changed before notifying all
03b,25mar99,sj   muxDevUnload does not free pEnd->devObject.pDevice + misc
03a,24mar99,sj   before calling Error,Restart or Shutdown callbacks check for
              	 MUX_BIND or MUX_TK_BIND
02z,19mar99,sj   no need to notify the protocol that changed the END flags
02y,18mar99,sj   in muxLibInit return status returned by muxTkBibInit
02x,18mar99,sj   added notification to protocols when END flags change
02w,05mar99,sj   eliminated hooks; not cleanly though. next version will do that
02v,03mar99,pul  fix for SPR# 24285
02u,02mar99,pul  fixed the return value for muxUnbind(): SPR# 24293
02t,13nov98,n_s  added muxDevStopAll function.  Added reboot hook in
                 muxLibInit so that muxDevStop all is called prior to reboot. 
                 spr #23229
02s,10nov98,sj   more doc changes for NPT
02r,03nov98,sj   added check for NPT driver in muxSend + doc fixes
02q,12oct98,pul  modified the NPT hooks for compatibility with SENS 
02p,08oct98,sj   making DOC fixes for NPT
02o,07oct98,sj   added npt hooks
02n,09sep98,ham  corrected the comprison of ifTypes and MUX_MAX_IFTYPE.
02m,09sep98,ham  cont'd SPR#22298.
02l,08sep98,ham  moved MUX_MAX_TYPE to h/muxLib.h, SPR#22298.
02k,21aug98,n_s  fixed ifInUnknownProtos update in muxReceive, ifOutNUcastPkts
                 update in muxSend (). spr 21074.
02j,16jul98,n_s  Added semGive () to ERROR condition for driver unload call in
                 muxDevUnload ().
02i,13jul98,n_s  fixed muxDevLoad () to limit device name strcpy () to 
                 END_NAME_MAX.  spr # 21642
02h,13jul98,n_s  fixed muxBind () to clear malloc'd NET_PROTOCOL. spr # 21644
02g,24jun98,n_s  fixed muxUnbind () to check NET_PROTOCOL node for type 
02f,15jun98,n_s  fixed muxDevUnload () and muxUnbind (). spr # 21542
02e,13feb98,n_s  fixed endFindByName () to handle multiple devices. 
                 spr # 21055.
02d,02feb98,spm  removed unneeded '&' operator and corrected spacing
02c,17jan98,gnn  fixed a bug in the output filter with passing LL_HDR_INFO.
02b,17jan98,gnn  changed output routines so they can be BOOL like receive
                 routines.
02a,17jan98,kbw  made man page fixes
01z,14dec97,kbw  made man page fixes
01y,10dec97,kbw  made man page fixes
01x,08dec97,gnn  END code review fixes.
01w,21oct97,kbw  made man page fixes
01v,08oct97,vin  fixed muxPacketAddrGet().
01u,07oct97,vin  fixed a problem with calling muxIoctl, removed IFF_LOAN
01t,03oct97,gnn  added error routine and cleaned up function prototypes
01s,25sep97,gnn  SENS beta feedback fixes
01r,25aug97,gnn  documenatation changs to make mangen happy.
01q,25aug97,gnn  fixed a bug in the restart routine.
01p,22aug97,gnn  updated polled mode support.
01o,19aug97,gnn  changes due to new buffering scheme.
01n,12aug97,gnn  changes necessitated by MUX/END update.
01m,31jul97,kbw  fixed man page bug that broke build, comment in comment
01l,31jul97,kbw  fixed man page problems found in beta review
01k,15may97,gnn  removed many warnings.
                 added code to handle MUX_PROTO_OUTPUT.
01j,30apr97,jag  man edits for funtion muxBufInit()
01i,17apr97,gnn  removed muxDevStart from muxDevLoad.
01h,07apr97,gnn  added errnoSet calls
                 added muxDevNameGet.
                 modified the way muxDevLoad works.
01g,12mar97,gnn  fixed a bug in muxReceive's calling API.
01f,03feb97,gnn  Modified muxBuf code to be more generic and support other,
                 non-TMD systems.
01e,29jan97,gnn  Removed the code to start the tNetTask.
01d,21jan97,gnn  Added muxBuf* code to handle buffering system.
                 Added code to handle the new SNARF option.
                 Removed all loaning and reference counting stuff.
                 Removed TxBuf stuff.
01e,23oct96,gnn  name changes to follow coding standards.
                 removed some compiler warnings.
01d,22oct96,gnn  added routines to start and stop drivers.
                 added code to handle buffer loaning startup requests on
                 both the protocol and device side.
                 replaced netVectors with netBuffers.
01c,23sep96,gnn	 moved some generic code to here from the driver.
01b,22Apr96,gnn	 filling in with real code.
01a,21Mar96,gnn	 written.
*/
 
/*
DESCRIPTION
This library provides the routines that define the MUX interface, a facility
that handles communication between the data link layer and the network
protocol layer.  Using the MUX, the VxWorks network stack has decoupled the
data link and network layers.  Drivers and services no longer need 
knowledge of each other's internals.  This independence makes it much easier 
to add new drivers or services.  For example, if you add a new MUX-based 
"END" driver, all existing MUX-based services can use the new driver.  
Likewise, if you add a new MUX-based service, any existing END can use the 
MUX to access the new service.  

INCLUDE FILES: errno.h, lstLib.h, logLib.h, string.h, m2Lib.h, bufLib.h, if.h,
end.h, muxLib.h, vxWorks.h, taskLib.h, stdio.h, errnoLib.h, if_ether.h,
netLib.h, semLib.h, rebootLib.h

To use this feature, include the following component:
INCLUDE_MUX


SEE ALSO
.I "VxWorks AE Network Programmer's Guide"
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
#include "net/if.h"		/* Needed for IFF_LOAN flag. */
#include "netinet/if_ether.h"
#include "netLib.h"		/* Needed for netJobAdd */
#include "bufLib.h"
#include "semLib.h"

#include "end.h"                /* Necessary for any END as well as the MUX */
#include "muxLib.h"
#include "muxTkLib.h"
#include "private/muxLibP.h"
#include "rebootLib.h"
#include "memPartLib.h"

#ifdef VIRTUAL_STACK 
#include "netinet/vsLib.h"
#endif    /* VIRTUAL_STACK */

/* defines */

#define STREQ(A, B) (strcmp(A, B) == 0 ? 1 : 0)

#define NET_TASK_NAME "tNetTask"
#define TK_DRV_CHECK(pBib) ((((MUX_ID)pBib)->flags & BIB_TK_DRV) ? TRUE : FALSE)
#define GET_IFMTU(a) (((a)->flags & END_MIB_2233) ? \
	    (a)->pMib2Tbl->m2Data.mibIfTbl.ifMtu : (a)->mib2Tbl.ifMtu)

/* externs */

/* globals */

/* locals */

LOCAL muxLib muxLibState;

LOCAL LIST endList;

#ifndef	STANDALONE_AGENT
LOCAL LIST addrResList[MUX_MAX_IFTYPE + 1]; /* IFT_xxx begins from 1 */
#endif	/* STANDALONE_AGENT */

/* forward declarations */
LOCAL M_BLK_ID 	*pMuxPollMblkRing;
LOCAL void 	**ppMuxPollDevices;
LOCAL int 	muxPollDevCount;
LOCAL int 	muxPollDevMax;

IMPORT STATUS muxTkReceive (void *, M_BLK_ID, long, long, BOOL, void *);
LOCAL void    muxEndFlagsNotify (void * pCookie, long endFlags); 
LOCAL STATUS  muxDevStopAllImmediate (void);


/*******************************************************************************
*
* muxLibInit - initialize global state for the MUX
*
* This routine initializes all global states for the MUX.
*
* RETURNS: OK or ERROR.
*/

STATUS muxLibInit (void)
    {
#ifndef	STANDALONE_AGENT
    int count;
#endif 	/* STANDALONE_AGENT */
    STATUS status = OK;

    if (muxLibState.lock != NULL)
        return (OK);

    muxLibState.debug = FALSE;
    muxLibState.lock = NULL;
    muxLibState.mode = MUX_MODE_NORM;
    muxLibState.priority = MUX_POLL_TASK_PRIORITY; /* Lame */
    muxLibState.taskDelay = MUX_POLL_TASK_DELAY; /* Lame */
    muxLibState.taskID = 0;

    muxLibState.lock = semBCreate(SEM_Q_PRIORITY, SEM_FULL);

    if (muxLibState.lock == NULL)
        return (ERROR);

#ifndef	STANDALONE_AGENT
    /*
     * Initialize our lists to empty.
     * addrResList[0] will not be used because IFT_xxx begins from 1
     */

    for (count = 0; count <= MUX_MAX_IFTYPE; count++)
        lstInit (&addrResList[count]);
#endif 	/* STANDALONE_AGENT */
    
    /* initialize END list */

    lstInit (&endList);
    
    /* Add hook to stop all ENDs when the system reboots */

    if (rebootHookAdd ((FUNCPTR) muxDevStopAllImmediate) != OK)
	{
	return (ERROR);
	}

    /* Initialize BIB. */

    status = muxTkBibInit();

    if (muxLibState.debug)
        logMsg ("End of muxLibInit\n", 0, 0, 0, 0, 0, 0);

    return (status);
    }
    
/******************************************************************************
* 
* muxDevLoad - load a driver into the MUX
* 
* The muxDevLoad() routine loads a network driver into the MUX.  Internally, 
* this routine calls the specified <endLoad> routine to initialize the
* software state of the device.  After the device is initialized,
* you must call muxDevStart() to start the device. 
* .IP <unit> 15
* Expects the unit number of the device. 
* .IP <endLoad> 
* Expects a pointer to the network driver's endLoad() or nptLoad() entry
* point. 
* .IP <pInitString> 
* Expects a pointer to an initialization string, typically a colon-delimited
* list of options.  The muxDevLoad() routine passes this along blindly to 
* the <endLoad> function.
* .IP <loaning> 
* Currently unused.
* .IP <pBSP>
* The MUX blindly passes this argument to the driver, which may or may not 
* use it.  Some BSPs use this parameter to pass in tables of functions that 
* the diver can use to deal with the particulars of the BSP. 
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxDevLoad() from within the kernel 
* protection domain only, and the data referenced in the <endLoad> and  
* <pBSP> parameters must reside in the kernel protection domain.  In 
* addition, the returned void pointer is valid in the kernel protection 
* domain only. This restriction does not apply under non-AE versions of 
* VxWorks.  
* 
* RETURNS: A cookie representing the new device, or NULL if an error occurred.
*
* ERRNO: S_muxLib_LOAD_FAILED
*/

void * muxDevLoad
    (
    int       unit,                      /* unit number of device */
    END_OBJ * (*endLoad) (char*, void*), /* load function of the driver  */
    char *    pInitString,		 /* init string for this driver  */
    BOOL      loaning,		         /* we loan buffers  */
    void *    pBSP                       /* for BSP group  */
    )
    {
    END_OBJ *     pNew;
    END_TBL_ROW * pNode;
    char          initString [END_INIT_STR_MAX];
    char          devName [END_NAME_MAX];
    BOOL          found = FALSE;
    void *        pCookie = NULL;

    if (muxLibState.debug)
        logMsg ("Start of muxDevLoad\n", 0, 0, 0, 0, 0, 0);

    bzero ( (char *)initString, END_INIT_STR_MAX);
    bzero ( (char *)devName, END_NAME_MAX);

    /* Let's mutually exclude here, wouldn't you say? */
    semTake (muxLibState.lock, WAIT_FOREVER);
    
    /*
     * Loading a device is a two pass algorithm.
     *
     * This is Pass 1.
     *
     * In the first pass we ask the device what its name is.  If that name
     * exists in our table then we add the new node to the end of the
     * already existing list.  If not then we create a new row in the
     * table, and place the new node as the zero'th (0) element in the node's
     * list.
     */

    if (endLoad ( (char *)devName, NULL) != 0)
        {
        goto muxLoadErr;
        }

    if (endFindByName ( (char *)devName, unit) != NULL)
        {
        goto muxLoadErr;
        }

    for (pNode = (END_TBL_ROW *)lstFirst(&endList); pNode != NULL; 
	pNode = (END_TBL_ROW *)lstNext(&pNode->node))
	{
	if (STREQ (pNode->name, (char *)devName))
            {
            found = TRUE;
            break;
            }
	}

    /*  If there is no row for this device then add it. */
    
    if (!found)
        {
        pNode = KHEAP_ALLOC (sizeof(END_TBL_ROW));
        if (pNode == NULL)
            {
            goto muxLoadErr;
            }
        bzero ((char *)pNode, sizeof(END_TBL_ROW));
        strncpy(pNode->name, devName, END_NAME_MAX - 1);
	pNode->name [END_NAME_MAX - 1] = EOS;
        lstAdd(&endList, &pNode->node);
        }

    /*
     * This is Pass 2.
     *
     * Now that we can determine a unique number we assign that number to
     * the device and actually load it.
     */
    
    sprintf ( (char *)initString, "%d:%s", unit, pInitString);

    pNew = (END_OBJ *)endLoad ( (char *)initString, pBSP);

    if (pNew == NULL)
        {
        goto muxLoadErr;
        }

    /* 
     * Leave this stuff last to prevent a race condition.  The condition
     * would be that the driver could call the receive routine 
     * (it should always of course check to make sure there is one)
     * before the buffer pool was set up.  This would be Bad.
     */
    lstAdd(&pNode->units, &pNew->node);

#ifndef	STANDALONE_AGENT
    /*
     * no receive routine for standalone agent (only polling mode
     * is supported).
     */

    /* Determine if driver uses END or NPT interface. Default is END. */

    pNew->receiveRtn = muxReceive;
 
    if (pNew->pFuncTable->ioctl)
        {
        if ( (pNew->pFuncTable->ioctl (pNew, EIOCGNPT, NULL)) == OK)
            {
            /* NPT device. */

            pNew->receiveRtn = (FUNCPTR)muxTkReceive;
             }
        }
#endif	/* STANDALONE_AGENT */

    pCookie = muxTkDevLoadUpdate (pNew);
    if (pCookie == NULL)
        {
        goto muxLoadErr;
        }

    semGive (muxLibState.lock);

    return (pCookie);

    muxLoadErr:
    errnoSet (S_muxLib_LOAD_FAILED);
    semGive (muxLibState.lock);
    return (NULL);
    }

/*****************************************************************************
* 
* muxDevStart - start a device by calling its start routine
*
* This routine starts a device that has already been initialized and loaded
* into the MUX with muxDevLoad().  muxDevStart() activates the network
* interfaces for a device, and calls the device's endStart() or nptStart()
* routine, which registers the driver's interrupt service routine and does
* whatever else is needed to allow the device to handle receiving and
* transmitting.  This call to endStart() or nptStart() puts the device into
* a running state. 
* 
* .IP <pCookie>
* Expects the pointer returned as the function value of the muxDevLoad()
* call for this device.  This pointer identifies the device.
* .LP
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxDevStart() from within the kernel 
* protection domain only, and the data referenced in the <pCookie> 
* parameter must reside in the kernel protection domain.  
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK; ENETDOWN, if <pCookie> does not represent a valid device;
* or ERROR, if the start routine for the device fails.
*
* ERRNO: S_muxLib_NO_DEVICE
*/

STATUS muxDevStart
    (
    void * pCookie    /* device identifier from muxDevLoad() routine */
    )
    {
    int       error;
    END_OBJ * pEnd = PCOOKIE_TO_ENDOBJ(pCookie);

    if (pEnd == NULL)
	{
        errnoSet (S_muxLib_NO_DEVICE);
	error = ENETDOWN;
	}
    else
	{
	error = pEnd->pFuncTable->start(pEnd);
	}

    if (error == OK)
        {
        if (pEnd->flags & END_MIB_2233)
            {
            if ((pEnd->pMib2Tbl != NULL) &&
                (pEnd->pMib2Tbl->m2VarUpdateRtn != NULL))
                {
                pEnd->pMib2Tbl->m2VarUpdateRtn(pEnd->pMib2Tbl,
                                               M2_varId_ifAdminStatus,
                                               (void *)M2_ifAdminStatus_up);
                pEnd->pMib2Tbl->m2VarUpdateRtn(pEnd->pMib2Tbl,
                                               M2_varId_ifOperStatus,
                                               (void *)M2_ifOperStatus_up);
                }
            }
        else /* (RFC1213 style of counters is supported) XXX */
            {
            pEnd->mib2Tbl.ifAdminStatus = M2_ifAdminStatus_up;
            pEnd->mib2Tbl.ifOperStatus = M2_ifOperStatus_up;
            }
        }
    
    return (error);
    }

#ifndef	STANDALONE_AGENT
/*****************************************************************************
* 
* muxDevStop - stop a device by calling its stop routine
*
* This routine stops the device specified in <pCookie>.
* muxDevStop() calls the device's endStop() or nptStop() routine.
* 
* .IP <pCookie> 
* Expects the cookie returned as the function value of the muxDevLoad() 
* call for this device.  This cookie identifies the device.
* .LP
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxDevStop() from within the kernel 
* protection domain only, and the data referenced in the <pCookie> 
* parameter must reside in the kernel protection domain.  
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK; ENETDOWN, if <pCookie> does not represent a valid device; or
* ERROR, if the endStop() or nptStop() routine for the device fails.
*
* ERRNO: S_muxLib_NO_DEVICE
*/

STATUS muxDevStop
    (
    void * pCookie    /* device identifier from muxDevLoad() routine */
    )
    {
    int error;
    END_OBJ * pEnd = PCOOKIE_TO_ENDOBJ(pCookie);

    if (pEnd == NULL)
	{
        errnoSet (S_muxLib_NO_DEVICE);
	error = ENETDOWN;
	}
    else
	{
	error = pEnd->pFuncTable->stop(pEnd);
	}

    return (error);
    }

/******************************************************************************
*
* muxShow - display configuration of devices registered with the MUX
*
* If the <pDevName> and <unit> arguments specify an existing device, this
* routine reports the name and type of each protocol bound to it. Otherwise,
* if <pDevName> is NULL, the routine displays the entire list of existing
* devices and their associated protocols.
*
* .IP <pDevName> 
* A string that contains the name of the device, or a null pointer to 
* indicate "all devices."
* .IP <unit> 
* Specifies the unit number of the device (if <pDevName> is not a null 
* pointer).
*
* RETURNS: N/A
*/

void muxShow
    (
    char * 	pDevName, 	/* pointer to device name, or NULL for all */
    int 	unit 		/* unit number for a single device */
    )
    {
    int curr;
    NET_PROTOCOL* pProto;
    END_OBJ* pEnd; 
    END_TBL_ROW* pNode;
    
    /* Display the current packet handling mode. */

    printf("Current mode: ");
    switch (muxLibState.mode)
        {
        case MUX_MODE_POLL:
            printf ("POLLING\n");
            break;
        case MUX_MODE_NORM:
            printf ("NORMAL\n");
            break;
        default:
            printf ("ERROR UNKNOWN MODE\n");
            break;
        }
    
    /* If the name is not NULL then find the specific interface to show. */

    if (pDevName != NULL)
	{
	pEnd = endFindByName (pDevName, unit);
	if (pEnd == NULL)
	    {
	    printf ("Device %s unit %d not found.\n", pDevName, unit);
	    return;
	    }
	printf ("Device: %s Unit: %d\n", pEnd->devObject.name,
                pEnd->devObject.unit);
        printf ("Description: %s\n", pEnd->devObject.description);

        /* See if there is an output protocol. */

        if (pEnd->outputFilter != NULL)
            {
	    printf ("Output protocol: Recv 0x%lx\n",
                    (unsigned long)pEnd->outputFilter->stackRcvRtn);
            }

        /* List the rest of the regular protocols for the device. */

	curr = 0;
	for (pProto = (NET_PROTOCOL *)lstFirst (&pEnd->protocols);
	     pProto != NULL; 
	     pProto = (NET_PROTOCOL *)lstNext(&pProto->node))
	    {
	    FUNCPTR netStackRcvRtn = NULL;

            if (pProto->type == MUX_PROTO_OUTPUT)
                continue;

	    if (pProto->nptFlag)
                {
                MUX_ID muxId = (MUX_ID)pProto->pSpare;
                if (muxId)
                    netStackRcvRtn = muxId->netStackRcvRtn;
                }
            else
                netStackRcvRtn = pProto->stackRcvRtn;

	    printf ("Protocol: %s\tType: %ld\tRecv 0x%lx\tShutdown 0x%lx\n", 
                    pProto->name, pProto->type,
                    (unsigned long)netStackRcvRtn,
		    (unsigned long)pProto->stackShutdownRtn);
	    curr++;
	    }
	}
    else  /* Show ALL configured ENDs. */
	{
	for (pNode = (END_TBL_ROW *)lstFirst (&endList); pNode != NULL; 
             pNode = (END_TBL_ROW *)lstNext (&pNode->node))
            {
            for (pEnd = (END_OBJ *)lstFirst(&pNode->units); pEnd != NULL; 
                 pEnd = (END_OBJ *)lstNext(&pEnd->node))
                {
                printf ("Device: %s Unit: %d\n", pEnd->devObject.name,
                        pEnd->devObject.unit);
                printf ("Description: %s\n", pEnd->devObject.description);
                if (pEnd->outputFilter != NULL)
                    {
                    printf ("Output protocol: Recv 0x%lx\n",
                            (unsigned long)pEnd->outputFilter->stackRcvRtn);
                    }
                curr = 0;
                for (pProto = (NET_PROTOCOL *)lstFirst (&pEnd->protocols);
                     pProto != NULL; 
                     pProto = (NET_PROTOCOL *)lstNext (&pProto->node))
                    {
		    FUNCPTR netStackRcvRtn = NULL;

                    if (pProto->type == MUX_PROTO_OUTPUT)
                        continue;

	            if (pProto->nptFlag)
                        {
                        MUX_ID muxId = (MUX_ID)pProto->pSpare;
                        if (muxId)
                            netStackRcvRtn = muxId->netStackRcvRtn;
                        }
                    else
                        netStackRcvRtn = pProto->stackRcvRtn;

                    printf ("Protocol: %s\tType: %ld\tRecv 0x%lx\tShutdown 0x%lx\n", 
                            pProto->name, pProto->type,
			    (unsigned long)netStackRcvRtn, 
                            (unsigned long)pProto->stackShutdownRtn);
                    curr++;
                    }
                }
	    }
	}
    }

/******************************************************************************
*
* muxBind - create a binding between a network service and an END
*
* A network service uses this routine to bind to an END specified by the
* <pName> and <unit> arguments (for example, ln and 0, ln and 1, or ei and 0).
*
* NOTE: This routine should only be used to bind to drivers that use the
* old END driver callback function prototypes.  NPT drivers, or END drivers
* that use the newer callback function prototypes, should use muxTkBind()
* instead.  See the
* .I "Network Protocol Toolkit Programmer's Guide"
* for more information on when to use muxBind() and muxTkBind().
*
* The <type> argument assigns a network service to one of several classes.
* Standard services receive the portion of incoming data associated
* with <type> values from RFC 1700.  Only one service for each RFC 1700
* type value may be bound to an END.
*
* Services with <type> MUX_PROTO_SNARF provide a mechanism for bypassing
* the standard services for purposes such as firewalls. These services
* will get incoming packets before any of the standard services.
*
* Promiscuous services with <type> MUX_PROTO_PROMISC receive any packets
* not consumed by the snarf or standard services. 
*
* The MUX allows multiple snarf and promiscuous services but does not
* coordinate between them. It simply delivers available packets to each
* service in FIFO order. Services that consume packets may prevent
* "downstream" services from receiving data if the desired packets overlap.
*
* An output service (with <type> MUX_PROTO_OUTPUT) receives outgoing data
* before it is sent to the device. This service type allows two network
* services to communicate directly and provides a mechanism for loop-back
* testing. Only one output service is supported for each driver.
*
* The MUX calls the registered <stackRcvRtn> whenever it receives a packet
* of the appropriate type. If that routine returns TRUE, the packet is
* not offered to any remaining services (or to the driver in the case of
* output services). A service (including an output service) may return
* FALSE to examine a packet without consuming it. See the description of a
* stackRcvRtn() in the
* .I "Network Protocol Toolkit Programmer's Guide"
* for additional information about the expected behavior of that routine.
*
* The <stackShutdownRtn> argument provides a function that the MUX can use
* to shut down the service.  See the
* .I "Network Protocol Toolkit Programmer's Guide"
* for a description of how to write such a routine.
* 
* The <pProtoName> argument provides the name of the service as a character
* string.  A service name is assigned internally if the argument is NULL.
*
* The <pSpare> argument registers a pointer to data defined by the service.
* The MUX includes this argument in calls to the call back routines from 
* this service.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxBind() from within the kernel protection 
* domain only, and the data referenced in the <stackRcvRtn>, 
* <stackShutdownRtn>, <stackTxRestartRtn>, <stackErrorRtn> and <pSpare> 
* parameters must reside in the kernel protection domain.  In addition, the 
* returned void pointer is valid in the kernel protection domain only. This 
* restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: A cookie identifying the binding between the service and driver;
* or NULL, if an error occurred.
*
* ERRNO: S_muxLib_NO_DEVICE, S_muxLib_ALREADY_BOUND, S_muxLib_ALLOC_FAILED
*/

void * muxBind
    (
    char * pName,		/* interface name, for example, ln, ei,... */
    int    unit,                /* unit number */
    BOOL   (*stackRcvRtn) (void*, long, M_BLK_ID, LL_HDR_INFO *, void*),
                                /* receive function to be called. */
    STATUS (*stackShutdownRtn) (void*, void*),
                                /* routine to call to shutdown the stack */
    STATUS (*stackTxRestartRtn) (void*, void*),
                                /* routine to tell the stack it can transmit */
    void   (*stackErrorRtn) (END_OBJ*, END_ERR*, void*),
                                /* routine to call on an error. */
    long   type,		/* protocol type from RFC1700 and many */
			        /* other sources (for example, 0x800 is IP) */
    char * pProtoName,		/* string name for protocol  */
    void * pSpare               /* per protocol spare pointer  */
    )
    {
    NET_PROTOCOL * pNew;
    NET_PROTOCOL * pNode = NULL;
    NODE *         pFinal = NULL;    /* Last snarf protocol, if any. */
    END_OBJ *      pEnd; 
    void *         pCookie = NULL;

    /* Check to see if we have a valid device. */

    pEnd = endFindByName(pName, unit);

    if (pEnd == NULL)
        {
        errnoSet(S_muxLib_NO_DEVICE);
	return (NULL);
        }

    /* 
     * Check to see if an output protocol already exists. */

    if (type == MUX_PROTO_OUTPUT)
        {
        if (pEnd->outputFilter)
            {
            errnoSet(S_muxLib_ALREADY_BOUND);
            return (NULL);
            }
        }

    /*
     * Get the appropriate place in the protocol list if necessary.
     * For standard protocols, also check if the protocol number
     * is available.
     */

    if (type != MUX_PROTO_PROMISC && type != MUX_PROTO_OUTPUT)
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

    if (type != MUX_PROTO_SNARF && type != MUX_PROTO_PROMISC && 
            type != MUX_PROTO_OUTPUT)
        {
        for ( ; pNode != NULL; pNode = (NET_PROTOCOL *)lstNext (&pNode->node))
            {
            if (pNode->type == type)
                {
                errnoSet (S_muxLib_ALREADY_BOUND);
                return (NULL);
                }
            }
        }

    /* Now we can allocate a new protocol structure. */

    pNew = (NET_PROTOCOL *) KHEAP_ALLOC (sizeof (NET_PROTOCOL));
    if (pNew == NULL)
	{
        if (muxLibState.debug)
	    logMsg ("muxBind cannot allocate protocol node", 0, 0, 0, 0, 0, 0);

        errnoSet (S_muxLib_ALLOC_FAILED);
	return (NULL);
	}

    bzero ( (char *)pNew, sizeof (NET_PROTOCOL));
    pNew->stackRcvRtn = stackRcvRtn;

    if (type != MUX_PROTO_OUTPUT)
        {
        pNew->stackShutdownRtn = stackShutdownRtn;
        pNew->stackTxRestartRtn = stackTxRestartRtn;
        pNew->stackErrorRtn = stackErrorRtn;
        }

    pNew->pEnd = pEnd;
    pNew->type = type;
    pNew->nptFlag = FALSE;      /* Protocol uses original MUX prototypes. */ 
    pNew->pSpare = pSpare;
    
    if (type != MUX_PROTO_OUTPUT)
        {
        if (pProtoName != NULL)
	    {
	    strncpy(pNew->name, pProtoName, 32);
	    }
        else 
	    {
	    sprintf(pNew->name, "Protocol %d", lstCount(&pEnd->protocols));
	    }
        }

    switch (type)
        {
        case MUX_PROTO_OUTPUT:
        case MUX_PROTO_PROMISC:
            lstAdd(&pEnd->protocols, &pNew->node);
            break;

        default:
             /* Add a standard or snarf protocol after any existing ones. */

            if (pEnd->snarfCount)
                {
                lstInsert(&pEnd->protocols, pFinal, &pNew->node);
                }
            else
                {
                lstInsert(&pEnd->protocols, NULL, &pNew->node);
                }
            break;
        }

    /* update the BIB. */

    if((pCookie = muxTkBindUpdate(pEnd,pNew)) == NULL)
	{
	/* unbind the protocol */

	lstDelete(&pEnd->protocols, &pNew->node);
	KHEAP_FREE((char *)pNew);
	return (NULL);
	}
    else if (type == MUX_PROTO_SNARF)     /* Increase the count. */
        (pEnd->snarfCount)++;

    if (type == MUX_PROTO_OUTPUT)
        {
        pEnd->outputFilter = pNew;
        pEnd->pOutputFilterSpare = pSpare;
        }

    pNew->pNptCookie = pCookie;

    return (pCookie);
    }

/*****************************************************************************
* 
* muxSend - send a packet out on a network interface
*
* This routine transmits a packet for the service specified by <pCookie>.
* You got this cookie from a previous bind call that bound the service to 
* a particular interface. This muxSend() call uses this bound interface to 
* transmit the packet.
*
* .IP <pCookie>
* Expects the cookie returned from muxBind().  This cookie identifies a
* particular service-to-interface binding.
* .IP <pNBuff>
* Expects a pointer to the buffer that contains the packet you want 
* to transmit.  Before you call muxSend(), you need to put the 
* addressing information at the head of the buffer.  To do this, call 
* muxAddressForm(). 
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxSend() from within the kernel 
* protection domain only, and the data referenced in the <pCookie> and  
* <pNBuff> parameters must reside in the kernel protection domain.  
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK; ENETDOWN, if <pCookie> does not represent a valid binding;
* or ERROR, if the driver's endSend() routine fails.
*
* ERRNO: S_muxLib_NO_DEVICE
*/

STATUS muxSend
    (
    void *   pCookie, 	/* protocol/device binding from muxBind() */
    M_BLK_ID pNBuff	/* data to be sent */
    )
    {
    int            error;
    END_OBJ *      pEnd;
    LL_HDR_INFO    llHdrInfo; 
    long           type;
    NET_PROTOCOL * pOut = NULL;

    pEnd = PCOOKIE_TO_ENDOBJ (pCookie);

    if (pEnd == NULL)
	{
        errnoSet (S_muxLib_NO_DEVICE);
	error = ENETDOWN;
	}
    else
	{
	if (pNBuff != NULL && 
	    (pNBuff->mBlkHdr.mFlags & M_BCAST 
	     || pNBuff->mBlkHdr.mFlags & M_MCAST))
	    {
            if (pEnd->flags & END_MIB_2233)
                {
                /*
                 * Do nothing as ifOutNUcastPkts is updated by the
                 * m2PktCountRtn function specified in the END object's M2_ID.
                 */
                }
            else /* RFC1213 style counters supported) XXX */
                {
	        ++(pEnd->mib2Tbl.ifOutNUcastPkts);
                }
	    }

        if (pEnd->outputFilter)
            {
            pOut = pEnd->outputFilter;

            if (muxPacketDataGet (pCookie, pNBuff, &llHdrInfo) == ERROR)
                return (ERROR);
            type = llHdrInfo.pktType;
            if (pOut->stackRcvRtn (pOut->pNptCookie, type, pNBuff, &llHdrInfo,
                                   pEnd->pOutputFilterSpare) == TRUE)
		{
		return (OK);
		}
            }
	error = pEnd->pFuncTable->send (pEnd, pNBuff);
	}

    return (error);
    }

/*****************************************************************************
* 
* muxReceive - handle a packet from a device driver
*
* An END calls this routine to transfer a packet to one or more bound
* services.
*
* NOTE: The stack receive routine installed by a protocol must not alter
* the packet if it returns FALSE.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxReceive() from within the kernel 
* protection domain only, and the data referenced in the <pCookie> and  
* <pMblk> parameters must reside in the kernel protection domain.  
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_muxLib_NO_DEVICE
*
* NOMANUAL
*/

STATUS muxReceive
    (
    void *   pCookie, 	/* device identifier from driver's load routine */
    M_BLK_ID pMblk 	/* buffer containing received frame */
    )
    {
    NET_PROTOCOL * 	pProto;
    END_OBJ * 		pEnd;
    long 		type;
    LL_HDR_INFO  	llHdrInfo; 
    int 		count;

    pEnd = (END_OBJ *)pCookie;

    if (pEnd == NULL)
        {
        errnoSet(S_muxLib_NO_DEVICE);
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
        return (ERROR);
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

    if (muxPacketDataGet (pProto->pNptCookie, pMblk, &llHdrInfo) == ERROR)
        {
        if (pEnd->flags & END_MIB_2233)
            {
            if ((pEnd->pMib2Tbl != NULL) &&
                (pEnd->pMib2Tbl->m2CtrUpdateRtn != NULL))
                {
                pEnd->pMib2Tbl->m2CtrUpdateRtn(pEnd->pMib2Tbl,
                                               M2_ctrId_ifInErrors, 1);
                }
            }
        else /* (RFC1213 style of counters supported) */
            {
            pEnd->mib2Tbl.ifInErrors++;
            }
        if (pMblk)
            netMblkClChainFree (pMblk);
        return (ERROR);
        }

    type = llHdrInfo.pktType;

    /*
     * Loop through the protocol list.
     * If a service's receive routine returns TRUE, it has consumed the
     * packet. The receive routine must return FALSE for other services
     * to see the packet.
     *
     * The order of services in the list (established during the bind)
     * determines their priority. SNARF services have the priority over
     * every other service type. PROMISC services have the lowest priority.
     * If multiple SNARF or PROMISC services exist, the order is first-come,
     * first-served, which could prevent some services from seeing matching
     * data if an earlier entry consumes it.
     */

    for (;pProto != NULL; pProto = (NET_PROTOCOL *)lstNext (&pProto->node))
	{
        if (pProto->type == MUX_PROTO_OUTPUT)    /* Ignore output service. */
            continue;

        /*
         * Handoff the data to any SNARF or promiscuous protocols, or to
         * a standard protocol which registered for the frame type.
         */

        if (pProto->type == MUX_PROTO_SNARF ||
            pProto->type == MUX_PROTO_PROMISC ||
            pProto->type == type)
            {
            if ( (*pProto->stackRcvRtn) (pProto->pNptCookie, type, pMblk,
                                         &llHdrInfo, pProto->pSpare) == TRUE)
                return (OK);
            }
        }

    /* Unknown service. Free the packet. */

    if (pEnd->flags & END_MIB_2233)
        {
        if ((pEnd->pMib2Tbl != NULL) &&
            (pEnd->pMib2Tbl->m2CtrUpdateRtn != NULL))
            {
            pEnd->pMib2Tbl->m2CtrUpdateRtn(pEnd->pMib2Tbl,
                                           M2_ctrId_ifInUnknownProtos, 1);
            }
        }
    else /* (RFC1213 style counters supported) XXX */
        {
        pEnd->mib2Tbl.ifInUnknownProtos++;
        }
    
    if (pMblk)
        netMblkClChainFree (pMblk);
    
    return OK;
    }
#endif	/* STANDALONE_AGENT */

/*****************************************************************************
* 
* muxPollSend - now 'deprecated', see muxTkPollSend()
*
* This routine transmits a packet for the service specified by <pCookie>.
* You got this cookie from a previous bind call that bound the service to 
* a particular interface. This muxPollSend() call uses this bound interface 
* to transmit the packet.  The <pNBuff> argument is a buffer ('mBlk') chain 
* that contains the packet to be sent.
*
* RETURNS: OK; ENETDOWN, if <pCookie> does not represent a valid device;
* ERROR, if the device type is not recognized; or an error value from the 
* device's registered endPollSend() routine. 
*
* ERRNO: S_muxLib_NO_DEVICE
*/

STATUS muxPollSend
    (
    void *   pCookie, 	/* binding instance from muxBind() */
    M_BLK_ID pNBuff	/* data to be sent */
    )
    {
    int            error;
    END_OBJ *      pEnd;
    LL_HDR_INFO    llHdrInfo; 
    long           type;
    NET_PROTOCOL * pOut;

    pEnd = PCOOKIE_TO_ENDOBJ (pCookie);

    if (pEnd == NULL)
	{
        errnoSet (S_muxLib_NO_DEVICE);
	error = ENETDOWN;
	}
    else
	{
        if (pEnd->outputFilter)
            {
            pOut = pEnd->outputFilter;

            if (muxPacketDataGet (pCookie, pNBuff, &llHdrInfo) == ERROR)
                return (ERROR);
            type = llHdrInfo.pktType;
            if (pOut->stackRcvRtn (pOut->pNptCookie, type, pNBuff, &llHdrInfo,
                                   pEnd->pOutputFilterSpare) == TRUE)
                {
                return (OK);
                }
            }
	error = pEnd->pFuncTable->pollSend (pEnd, pNBuff);
	}

    return (error);
    }

/*****************************************************************************
* 
* muxPollReceive - now 'deprecated', see muxTkPollReceive()
*
* NOTE: This routine has been deprecated in favor of muxTkPollReceive()
*
* Upper layers can call this routine to poll for a packet.
*
* .IP <pCookie>
* Expects the cookie that was returned from muxBind().
* This cookie indicates which driver to query for available data.
*
* .IP <pNBuff>
* Expects a pointer to a buffer chain into which to receive data.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxPollReceive() from within the kernel 
* protection domain only, and the data referenced in the <pCookie> and  
* <pNBuff> parameters must reside in the kernel protection domain.  
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK; ENETDOWN, if the <pCookie> argument does not represent a 
* loaded driver; or an error value returned from the driver's registered 
* endPollReceive() function.
*
* ERRNO: S_muxLib_NO_DEVICE
*/

STATUS muxPollReceive
    (
    void *   pCookie, 	/* binding instance from muxBind() */
    M_BLK_ID pNBuff	/* a vector of buffers passed to us */
    )
    {
    int       error;
    END_OBJ * pEnd;

    pEnd = PCOOKIE_TO_ENDOBJ (pCookie);

    if (pEnd == NULL)
	{
        errnoSet(S_muxLib_NO_DEVICE);
	error = ENETDOWN;
	}
    else
	{
	error = pEnd->pFuncTable->pollRcv (pEnd, pNBuff);
	}

    return (error);
    }

/******************************************************************************
*
* muxIoctl - send control information to the MUX or to a device
*
* This routine gives the service access to the network driver's control 
* functions.  The MUX itself can implement some of the standard control
* functions, so not all commands necessarily pass down to the device.  
* Otherwise, both command and data pass to the device without modification.
*
* Typical uses of muxIoctl() include commands to start, stop, or reset the
* network interface, or to add or configure MAC and network addresses.
*
* .IP <pCookie>
* Expects the cookie returned from muxBind() or muxTkBind().  This
* cookie indicates the device to which this service is bound. 
* .IP <cmd>
* Expects a value indicating the control command you want to execute. 
* For valid <cmd> values, see the description of the endIoctl() and
* nptIoctl() routines provided in the
* .I "Network Protocol Toolkit Programmer's Guide".
* .IP <data>
* Expects the data or a pointer to the data needed to carry out the command
* specified in <cmd>. 
* 
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxIoctl() from within the kernel 
* protection domain only, and the data referenced in the <pCookie> and  
* <data> parameters must reside in the kernel protection domain.  
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK; ENETDOWN, if <pCookie> does not represent a bound device;
* or ERROR, if the command fails.
*
* ERRNO: S_muxLib_NO_DEVICE
*/

STATUS muxIoctl
    (
    void *  pCookie, 	/* service/device binding from muxBind()/muxTkBind() */
    int     cmd,	/* command to pass to ioctl */
    caddr_t data	/* data need for command in cmd */
    )
    {
    int           error;
    END_OBJ *     pEnd;
    IF_SETENTRY * pIfSetentry;

    UINT32 endFlags = 0;    /* Current flag settings (before any change). */

    pEnd = PCOOKIE_TO_ENDOBJ (pCookie);

    if (pEnd == NULL)
	{
        errnoSet (S_muxLib_NO_DEVICE);
	error = ENETDOWN;
	}
    else
	{
	switch ((u_int) cmd)
	    {
	    case EIOCMULTIADD:
		error = pEnd->pFuncTable->mCastAddrAdd (pEnd, (char *)data);
		break;

	    case EIOCMULTIDEL:
		error =
                    pEnd->pFuncTable->mCastAddrDel (pEnd, (char *)data);
		break;

	    case EIOCMULTIGET:
		error =
                    pEnd->pFuncTable->mCastAddrGet (pEnd, (MULTI_TABLE *)data);
		break;

            case EIOCGMCASTLIST:
                *(LIST **)data = &pEnd->multiList;
		error = OK;
                break;

            case EIOCSMIB2233:
                {
                pIfSetentry = (IF_SETENTRY *)data;

                /* check the four possible IF variable to set */

                if (pIfSetentry->varToSet & M2_varId_ifAdminStatus)
                    {
                    if (pEnd->flags & END_MIB_2233)
                        {
                        if ((pEnd->pMib2Tbl != NULL) &&
                            (pEnd->pMib2Tbl->m2VarUpdateRtn != NULL))
                            {
                            pEnd->pMib2Tbl->m2VarUpdateRtn(pEnd->pMib2Tbl,
                                           M2_varId_ifAdminStatus,
                                           (void *)pIfSetentry->ifAdminStatus);
                            pEnd->pMib2Tbl->m2VarUpdateRtn(pEnd->pMib2Tbl,
                                           M2_varId_ifOperStatus,
                                           (void *)pIfSetentry->ifAdminStatus);
                            }
                        }
                    else /* (RFC1213 style of counters supported) XXX */
                        {
                        pEnd->mib2Tbl.ifAdminStatus =
                            pIfSetentry->ifAdminStatus;
                        pEnd->mib2Tbl.ifOperStatus =
                            pIfSetentry->ifAdminStatus;
                        }
                    }

                if ((pEnd->flags & END_MIB_2233) &&
                    (pEnd->pMib2Tbl != NULL) &&
                    (pEnd->pMib2Tbl->m2VarUpdateRtn != NULL) &&
                    (pIfSetentry->varToSet & M2_varId_ifLinkUpDownTrapEnable))
                    {
                    pEnd->pMib2Tbl->m2VarUpdateRtn(pEnd->pMib2Tbl,
                                  M2_varId_ifLinkUpDownTrapEnable,
                                  (void *)pIfSetentry->ifLinkUpDownTrapEnable);
                    }

                if ((pEnd->flags & END_MIB_2233) &&
                    (pEnd->pMib2Tbl != NULL) &&
                    (pEnd->pMib2Tbl->m2VarUpdateRtn != NULL) &&
                    (pIfSetentry->varToSet & M2_varId_ifPromiscuousMode))
                    {
                    pEnd->pMib2Tbl->m2VarUpdateRtn(pEnd->pMib2Tbl,
                                  M2_varId_ifLinkUpDownTrapEnable,
                                  (void *)pIfSetentry->ifLinkUpDownTrapEnable);
                    }

                if ((pEnd->flags & END_MIB_2233) &&
                    (pEnd->pMib2Tbl != NULL) &&
                    (pEnd->pMib2Tbl->m2VarUpdateRtn != NULL) &&
                    (pIfSetentry->varToSet & M2_varId_ifAlias))
                    {
                    pEnd->pMib2Tbl->m2VarUpdateRtn(pEnd->pMib2Tbl,
                                                 M2_varId_ifAlias,
                                                 (void *)pIfSetentry->ifAlias);
                    }

                error = OK;
                break;
                }

	    case EIOCSFLAGS:
		endFlags = pEnd->flags;
		/* fall-through */

	    default:
		error = pEnd->pFuncTable->ioctl (pEnd, cmd, data);
                break;
	    }

        /* All services must be notified if the device flags change. */

        if (cmd == (int) EIOCSFLAGS && error == OK && (endFlags ^ pEnd->flags))
            {
            netJobAdd ( (FUNCPTR)muxEndFlagsNotify, (int)pCookie,
                       (int)pEnd->flags, 0, 0, 0);
            }
        }

    return (error);
    }

#ifndef STANDALONE_AGENT
/******************************************************************************
*
* muxMCastAddrAdd - add a multicast address to a device's multicast table 
*
* This routine adds an address to the multicast table maintained by a device. 
* This routine calls the driver's endMCastAddrAdd() or nptMCastAddrAdd()
* routine to accomplish this.
* 
* If the device does not support multicasting, muxMCastAddrAdd() will return
* ERROR and 'errno' will be set to ENOTSUP (assuming the driver has been
* written properly).
*
* .IP <pCookie> 13
* Expects the cookie returned from the muxBind() or muxTkBind() call.  This
* cookie identifies the device to which the MUX has bound this service. 
*
* .IP <pAddress>
* Expects a pointer to a character string containing the address you 
* want to add. 
* 
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxMCastAddrAdd() from within the kernel 
* protection domain only, and the data referenced in the <pCookie> 
* parameter must reside in the kernel protection domain.  
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK; ENETDOWN, if <pCookie> does not represent a valid device;
* or ERROR, if the device's endMCastAddrAdd() function fails.
*
* ERRNO: S_muxLib_NO_DEVICE
*
*/

STATUS muxMCastAddrAdd
    (
    void * pCookie, 	/* binding instance from muxBind() or muxTkBind() */
    char * pAddress	/* address to add to the table */
    )
    {
    int       error;
    END_OBJ * pEnd;

    pEnd = PCOOKIE_TO_ENDOBJ (pCookie);

    if (pEnd == NULL)
	{
        errnoSet (S_muxLib_NO_DEVICE);
	error = ENETDOWN;
	}
    else 
	{
	error = pEnd->pFuncTable->mCastAddrAdd (pEnd, pAddress);
	}

    return (error);
    }

/******************************************************************************
*
* muxMCastAddrDel - delete a multicast address from a device's multicast table
*
* This routine deletes an address from the multicast table maintained by a 
* device by calling that device's endMCastAddrDel() or nptMCastAddrDel()
* routine.
*
* If the device does not support multicasting, muxMCastAddrAdd() will return
* ERROR and 'errno' will be set to ENOTSUP (assuming the driver has been
* written properly).
*
* .IP <pCookie> 13
* Expects the cookie returned from muxBind() or muxTkBind() call.  This
* cookie identifies the device to which the MUX bound this service. 
* .IP <pAddress>
* Expects a pointer to a character string containing the address you 
* want to delete. 
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxMCastAddrDell() from within the kernel 
* protection domain only, and the data referenced in the <pCookie> 
* parameter must reside in the kernel protection domain.  
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK; ENETDOWN, if <pCookie> does not represent a valid driver;
* or ERROR, if the driver's registered endMCastAddrDel() or nptMCastAddrDel() 
* functions fail.
*
* ERRNO: S_muxLib_NO_DEVICE
*/

STATUS muxMCastAddrDel
    (
    void * pCookie, 	/* binding instance from muxBind() or muxTkBind() */
    char * pAddress     /* Address to delete from the table. */
    )
    {
    int       error;
    END_OBJ * pEnd;

    pEnd = PCOOKIE_TO_ENDOBJ (pCookie);

    if (pEnd == NULL)
	{
        errnoSet (S_muxLib_NO_DEVICE);
	error = ENETDOWN;
	}
    else 
	{
	error = pEnd->pFuncTable->mCastAddrDel (pEnd, pAddress);
	}

    return (error);
    }

/******************************************************************************
*
* muxMCastAddrGet - get the multicast address table from the MUX/Driver
*
* This routine writes the list of multicast addresses for a specified
* device into a buffer.  To get this list, it calls the driver's own
* endMCastAddrGet() or nptMCastAddrGet() routine. 
*
* .IP <pCookie>
* Expects the cookie returned from muxBind() or muxTkBind()
* call.  This cookie indicates the device to which the MUX
* has bound this service. 
* .IP <pTable>
* Expects a pointer to a MULTI_TABLE structure.  You must have allocated 
* this structure at some time before the call to muxMCastAddrGet(). 
* The MULTI_TABLE structure is defined in end.h as: 
* 
* .CS
*    typedef struct multi_table
*        {
*        int     tableLen;  /@ length of table in bytes @/
*        char *  pTable;    /@ pointer to entries @/
*        } MULTI_TABLE;
* .CE
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxMCastAddrGet() from within the kernel 
* protection domain only, and the data referenced in the <pCookie> 
* parameter must reside in the kernel protection domain.  
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK; ENETDOWN, if <pCookie> does not represent a valid driver;
* or ERROR, if the driver's registered endMCastAddrGet() or nptMCastAddrGet() 
* routines fail.
*
* ERRNO: S_muxLib_NO_DEVICE
*/

int muxMCastAddrGet
    (
    void *        pCookie, /* binding instance from muxBind() or muxTkBind() */
    MULTI_TABLE * pTable   /* pointer to a table to be filled and returned. */
    )
    {
    int       error;
    END_OBJ * pEnd;

    pEnd = PCOOKIE_TO_ENDOBJ (pCookie);

    if (pEnd == NULL)
	{
        errnoSet(S_muxLib_NO_DEVICE);
	error = ENETDOWN;
	}
    else 
	{
	error = pEnd->pFuncTable->mCastAddrGet(pEnd, pTable);
	}

    return (error);
    }

/******************************************************************************
*
* muxUnbind - detach a network service from the specified device
*
* This routine disconnects a network service from the specified device.  The
* <pCookie> argument indicates the service/device binding returned by the
* muxBind() or muxTkBind() routine. The <type> and <stackRcvRtn> arguments
* must also match the values given to the original muxBind() or muxTkBind()
* call.
*
* NOTE: If muxUnbind() returns ERROR, and 'errno' is set to EINVAL, this
* indicates that the device is not bound to the service.
*
* RETURNS: OK; or ERROR, if muxUnbind() fails.
* 
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxUnBind() from within the kernel 
* protection domain only, and the data referenced in the <stackRcvRtn> and  
* <pCookie> parameters must reside in the kernel protection domain.  
* This restriction does not apply under non-AE versions of VxWorks.  
*
* ERRNO: EINVAL, S_muxLib_NO_DEVICE
*/

STATUS muxUnbind
    (
    void *  pCookie, 	/* binding instance from muxBind() or muxTkBind() */
    long    type, 	/* type passed to muxBind() or muxTkBind() call */
    FUNCPTR stackRcvRtn /* pointer to stack receive routine */
    )
    {
    int error;

    END_OBJ * 	   pEnd;
    NET_PROTOCOL * pNode;
    FUNCPTR        netStackRcvRtn = NULL;

    pEnd = PCOOKIE_TO_ENDOBJ (pCookie);

    if (pEnd == NULL)
	{
        errnoSet (S_muxLib_NO_DEVICE);
	error = EINVAL;
	return error;
	}
    else 
        {
        /* Find and remove the matching binding. */

	for (pNode = (NET_PROTOCOL *)lstFirst (&pEnd->protocols); 
	     pNode != NULL; 
	     pNode = (NET_PROTOCOL *)lstNext(&pNode->node))
	    {
            if (pNode->pNptCookie != pCookie)
                continue;

            /*
             * If necessary, fetch the actual stack receive routine
             * instead of the wrapper installed for END devices.
             */

            if (pNode->nptFlag)
                {
                MUX_ID muxId = (MUX_ID)pNode->pSpare;
                if (muxId)
                    netStackRcvRtn = muxId->netStackRcvRtn;
                }
            else
                netStackRcvRtn = pNode->stackRcvRtn;

            /* This test should always succeed since the entry matches. */

            if (netStackRcvRtn == stackRcvRtn && pNode->type == type)
                {
                if (pNode->type == MUX_PROTO_SNARF)
                    (pEnd->snarfCount)--;

                if (pNode->type == MUX_PROTO_OUTPUT)
                    pEnd->outputFilter = NULL;

                lstDelete (&pEnd->protocols, &pNode->node);
                muxTkUnbindUpdate (pNode->pNptCookie);
                KHEAP_FREE( (char *)pNode);

                return (OK);
                }
            }
        }

    errnoSet (EINVAL);
    return (ERROR);
    }
    
/******************************************************************************
*
* muxDevUnload - unloads a device from the MUX
*
* This routine unloads a device from the MUX.  This breaks any network
* connections that use the device.  When this routine is called, each service
* bound to the device disconnects from it with the stackShutdownRtn() routine
* that was registered by the service.  The stackShutdownRtn() should call
* muxUnbind() to detach from the device.  Then, muxDevUnload() calls the
* device's endUnload() or nptUnload() routine.
*
* .IP <pName>
* Expects a pointer to a string containing the name of the device, for example
* 'ln' or 'ei'
*
* .IP <unit>
* Expects the unit number of the device indicated by <pName>
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxDevUnLoad() from within the kernel 
* protection domain only. This restriction does not apply under non-AE 
* versions of VxWorks.  
*
* RETURNS: OK, on success; ERROR, if the specified device was not found
* or some other error occurred; or the error value returned by the driver's
* unload() routine.
*
* ERRNO: S_muxLib_UNLOAD_FAILED, S_muxLib_NO_DEVICE
*
*/

STATUS muxDevUnload
    (
    char * pName, 	/* a string containing the name of the device */
			/* for example, ln or ei */
    int    unit         /* the unit number */
    )
    {
    int            error;
    END_OBJ *      pEnd = NULL;
    NET_PROTOCOL * pNetNode;
    NET_PROTOCOL * pNetNodeNext;
    END_TBL_ROW *  pNode;
    void * pTemp = NULL;

    pEnd = endFindByName (pName, unit);

    if (pEnd != NULL)
	{
        semTake (muxLibState.lock, WAIT_FOREVER);

        /* 
         * Find which row we're in. We know it's here because 
         * the endFindByName() routine was successful.
         */

        for (pNode = (END_TBL_ROW *)lstFirst (&endList); pNode != NULL; 
             pNode = (END_TBL_ROW *)lstNext (&pNode->node))
            {
            if (STREQ(pNode->name, pName))
                break;
            }

        /* First we shutdown all of the protocols. */

        for (pNetNode = (NET_PROTOCOL *) lstFirst (&pEnd->protocols);
	     pNetNode != NULL; pNetNode = pNetNodeNext) 
            {
	    /* Save pointer to next node as shutdown rtn will free pNetNode */

	    pNetNodeNext = (NET_PROTOCOL *) lstNext (&pNetNode->node);

            if (pNetNode->stackShutdownRtn != NULL)
		{
		/*
		 * stackShutdownRtn() is expected to call muxUnbind() which 
		 * will free the NET_PROTOCOL node, so that operation is not
                 * necessary here. The END and NPT devices use different
                 * shutdown prototypes. Select the appropriate format.
                 */

                if (pNetNode->nptFlag)
                    {
                    MUX_ID muxId = (MUX_ID)pNetNode->pSpare;
                    void * pSpare = NULL;

                    pSpare = (muxId) ? muxId->pNetCallbackId : NULL;
                    pNetNode->stackShutdownRtn(pSpare);
                    }
                else
                    pNetNode->stackShutdownRtn (pEnd, pNetNode->pSpare);
		}
	    else
		{
		/*
		 * If no shutdown rtn then muxUnbind () will not be called and
		 * we need to free the pNetNode ourselves.
		 */

		lstDelete (&pEnd->protocols, &pNetNode->node);
		KHEAP_FREE ((char *)pNetNode);
		}
	    }
   
	/* Release BIB and delete node before unloading device driver */

        muxTkUnloadUpdate (pEnd);
        lstDelete (&pNode->units, &pEnd->node);
 
	/* remember the device specific value for backward compatibility purpose */

	pTemp = pEnd->devObject.pDevice;

        /* Unload (destroy) the end object */

	error = pEnd->pFuncTable->unload (pEnd);
 
	/*
	 * XXX Freeing END_OBJ(or DRV_CTRL) should be done by END, not here.
         * In futre, this code will be removed.
	 */

	if (pTemp == pEnd)
            {
            /* for backward compatibility, free the end_object */

            KHEAP_FREE ((char *)pTemp);
            }
 
       /*
        * XXX I don't see how 'unload' can really fail. There's nothing the mux
	* can do to recover or continue using the END_OBJ.  All the protocols are
	* already gone.
        */

        if (error != OK)
            {
            errnoSet (S_muxLib_UNLOAD_FAILED);
            semGive (muxLibState.lock);
            return (ERROR);
            }
 
        /* Is this row empty?  If so, remove it. */

        if (lstCount (&pNode->units) == 0)
            {
            lstDelete (&endList, &pNode->node);
            KHEAP_FREE ((char *)pNode);
            }
        error = OK;
        semGive (muxLibState.lock);
        }
    else
        {
        errnoSet (S_muxLib_NO_DEVICE);
        error = ERROR;
        }
    
    return (error);
    }
#endif	/* STANDALONE_AGENT */

/******************************************************************************
*
* muxLinkHeaderCreate - attach a link-level header to a packet
*
* This routine constructs a link-level header using the source address
* of the device indicated by the <pCookie> argument as returned from the
* muxBind() routine.
*
* The <pDstAddr> argument provides an M_BLK_ID buffer containing the
* link-level destination address.
* Alternatively, the <bcastFlag> argument, if TRUE, indicates that the
* routine should use the link-level broadcast address, if available for
* the device. Although other information contained in the <pDstAddr>
* argument must be accurate, the address data itself is ignored in that case.
*
* The <pPacket> argument contains the contents of the resulting link-level
* frame.  This routine prepends the new link-level header to the initial
* 'mBlk' in that network packet if space is available or allocates a new
* 'mBlk'-'clBlk'-cluster triplet and prepends it to the 'mBlk' chain. 
* When construction of the header is complete, it returns an M_BLK_ID that 
* points to the initial 'mBlk' in the assembled link-level frame.
*
* RETURNS: M_BLK_ID or NULL.
*
* ERRNO: S_muxLib_INVALID_ARGS
*/

M_BLK_ID muxLinkHeaderCreate
    (
    void * 	pCookie, 	/* protocol/device binding from muxBind() */
    M_BLK_ID 	pPacket, 	/* structure containing frame contents */
    M_BLK_ID 	pSrcAddr,	/* structure containing source address */
    M_BLK_ID 	pDstAddr, 	/* structure containing destination address */
    BOOL 	bcastFlag 	/* use broadcast destination (if available)? */
    )
    {
    END_OBJ * 	pEnd;
    M_BLK_ID	pMblkHdr = NULL;

    pEnd = PCOOKIE_TO_ENDOBJ (pCookie);
    if (pEnd == NULL)
        {
        errnoSet(S_muxLib_INVALID_ARGS);
        return (NULL); 
        }

    if (pEnd->pFuncTable->formAddress != NULL)
        {
        pMblkHdr = pEnd->pFuncTable->formAddress (pPacket, pSrcAddr,
                                                  pDstAddr, bcastFlag);
        }
    else
        {
        switch (pEnd->mib2Tbl.ifType)
            {
            case M2_ifType_ethernet_csmacd:
            case M2_ifType_iso88023_csmacd:
                M_PREPEND(pPacket, SIZEOF_ETHERHEADER, M_DONTWAIT);
                if ( pPacket == NULL)
                    return NULL;
                if (bcastFlag)
                    bcopy (etherbroadcastaddr, pPacket->m_data, 6);
                else
                    bcopy(pDstAddr->m_data, pPacket->m_data, pDstAddr->m_len);

                bcopy(pSrcAddr->m_data, pPacket->m_data + pDstAddr->m_len,
                      pSrcAddr->m_len);
                ((struct ether_header *)pPacket->m_data)->ether_type =
                      pDstAddr->mBlkHdr.reserved;
                pMblkHdr = pPacket;
                break;
            default:
                break;
            }
        }
    return (pMblkHdr);
    }

/******************************************************************************
*
* muxAddressForm - form a frame with a link-layer address
*
* Use this routine to create a frame with an appropriate link-layer address.  
* As input, this function expects the source address, the destination address, 
* and the data you want to include in the frame. When control returns from the 
* muxAddressForm() call, the <pMblk> parameter references a frame ready for 
* transmission.  Internally, muxAddressForm() either prepended the link-layer 
* header to the data buffer supplied in <pMblk> (if there was enough room) or 
* it allocated a new 'mBlk'-'clBlk'-cluster and prepended the new 'mBlk' to 
* the 'mBlk' chain supplied in <pMblk>.
*
* NOTE: You should set the 'pDstAddr.mBlkHdr.reserved' field to the network
* service type.
*
* .IP <pCookie> 13
* Expects the cookie returned from the muxBind().  This cookie 
* indicates the device to which the MUX has bound this protocol. 
* .IP <pMblk>
* Expects a pointer to the 'mBlk' structure that contains the packet. 
* .IP <pSrcAddr>
* Expects a pointer to the 'mBlk' that contains the source address.
* .IP <pDstAddr>
* Expects a pointer to the 'mBlk' that contains the destination address.
*
* NOTE: This routine is used only with ENDs, and is not needed for NPT
* drivers.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxAddressForm() from within the kernel 
* protection domain only, and the data referenced in the <pCookie> 
* parameter must reside in the kernel protection domain. In addition, the 
* returned M_BLK_ID is valid in the kernel protection domain only.
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: M_BLK_ID or NULL.
*
* ERRNO: S_muxLib_NO_DEVICE
*/

M_BLK_ID muxAddressForm
    (
    void * 	pCookie, 	/* protocol/device binding from muxBind() */
    M_BLK_ID 	pMblk,		/* structure to contain packet */
    M_BLK_ID 	pSrcAddr,	/* structure containing source address */
    M_BLK_ID 	pDstAddr	/* structure containing destination address */
    )
    {
    END_OBJ *   pEnd;
    M_BLK_ID	pMblkHdr = NULL;
    ULONG       ifType;

    pEnd = PCOOKIE_TO_ENDOBJ (pCookie);
    
    if (pEnd == NULL)
        {
        errnoSet (S_muxLib_NO_DEVICE);
        return (NULL); 
        }
    
    if (pEnd->pFuncTable->formAddress != NULL)
        {
        pMblkHdr = pEnd->pFuncTable->formAddress (pMblk, pSrcAddr,
                                                  pDstAddr, FALSE);
        }
    else
        {
        if (pEnd->flags & END_MIB_2233)
            {
            if (pEnd->pMib2Tbl != NULL)
                ifType = pEnd->pMib2Tbl->m2Data.mibIfTbl.ifType;
            else
                return NULL;
            }
        else /* (RFC1213 style of counters supported) XXX */
            {
            ifType = pEnd->mib2Tbl.ifType;
            }

        switch (ifType)
            {
            case M2_ifType_ethernet_csmacd:
            case M2_ifType_iso88023_csmacd:
                M_PREPEND(pMblk, SIZEOF_ETHERHEADER, M_DONTWAIT);
                if (pMblk == NULL)
                    return NULL;
                bcopy(pDstAddr->m_data, pMblk->m_data, pDstAddr->m_len);
                bcopy(pSrcAddr->m_data, pMblk->m_data + pDstAddr->m_len,
                      pSrcAddr->m_len);
                ((struct  ether_header *)pMblk->m_data)->ether_type =
                    pDstAddr->mBlkHdr.reserved;
                pMblkHdr = pMblk;
                break;
            default:
                break;
            }
        }
    return (pMblkHdr);
    }

/******************************************************************************
*
* muxPacketDataGet - return the data from a packet
*
* Any service bound to a driver may use this routine to extract the
* packet data and remove the link-level header information. This
* routine copies the header information from the packet referenced in
* <pMblk> into the LL_HDR_INFO structure referenced in <pLinkHdrInfo>.
*
* .IP <pCookie>
* Expects the cookie returned from the muxBind() call.  This
* cookie indicates the device to which the MUX bound this service.
*
* .IP <pMblk>
* Expects a pointer to an 'mBlk' or 'mBlk' cluster representing a packet
* containing the data to be returned
*
* .IP <pLinkHdrInfo>
* Expects a pointer to an LL_HDR_INFO structure into which the packet header
* information is copied from the incoming 'mBlk'
* 
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxPacketDataGet() from within the kernel 
* protection domain only, and the data referenced in the parameters must   
* reside in the kernel protection domain.  This restriction does not 
* apply under non-AE versions of VxWorks.  
*
* RETURNS: OK; or ERROR, if the device type is not recognized.
*
* ERRNO: S_muxLib_NO_DEVICE
*/

STATUS muxPacketDataGet
    (
    void *        pCookie, 	 /* protocol/device binding from muxBind() */
    M_BLK_ID      pMblk, 	 /* returns the packet data */
    LL_HDR_INFO * pLinkHdrInfo   /* returns the packet header information */
    )
    {
    int       error = OK;
    END_OBJ * pEnd;

    pEnd = PCOOKIE_TO_ENDOBJ(pCookie);

    if (pEnd == NULL)
        {
        errnoSet(S_muxLib_NO_DEVICE);
        return (ERROR);
        }
    
    if (pEnd->pFuncTable->packetDataGet != NULL)
        {
        error = pEnd->pFuncTable->packetDataGet(pMblk, pLinkHdrInfo);
        }
    else
        {
        return (ERROR);
        }

    return (error);
    }

/******************************************************************************
*
* muxPacketAddrGet - get addressing information from a packet
*
* The routine returns the immediate source, immediate destination, ultimate 
* source, and ultimate destination addresses from the packet pointed to in 
* the first M_BLK_ID.  This routine makes no attempt to extract that 
* information from the packet directly.  Instead, it passes the packet 
* through to the driver routine that knows how to interpret the packets that 
* it has received.
*
* .IP <pCookie>
* Expects the cookie returned from the muxBind() call.  This
* cookie indicates the device to which the MUX bound this service.
*
* .IP <pMblk>
* Expects an M_BLK_ID representing packet data from which the addressing
* information is to be extracted
*
* .IP <pSrcAddr>
* Expects NULL or an M_BLK_ID which will hold the local source address
* extracted from the packet
*
* .IP <pDstAddr>
* Expects NULL or an M_BLK_ID which will hold the local destination address
* extracted from the packet
*
* .IP <pESrcAddr>
* Expects NULL or an M_BLK_ID which will hold the end source address
* extracted from the packet
*
* .IP <pEDstAddr>
* Expects NULL or an M_BLK_ID which will hold the end destination address
* extracted from the packet
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxPacketAddrGet() from within the kernel 
* protection domain only, and the data referenced in the parameters must 
* reside in the kernel protection domain.  This restriction does not apply 
* under non-AE versions of VxWorks.  
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_muxLib_NO_DEVICE
*/

STATUS muxPacketAddrGet
    (
    void *   pCookie, 		/* protocol/device binding from muxBind() */
    M_BLK_ID pMblk,		/* structure to contain packet */
    M_BLK_ID pSrcAddr,   	/* structure containing source address */
    M_BLK_ID pDstAddr,          /* structure containing destination address */
    M_BLK_ID pESrcAddr,         /* structure containing the end source */
    M_BLK_ID pEDstAddr          /* structure containing the end destination */
    )
    {
    int       error = OK;
    END_OBJ * pEnd;

    pEnd = PCOOKIE_TO_ENDOBJ (pCookie);

    if (pEnd == NULL)
        {
        errnoSet(S_muxLib_NO_DEVICE);
        return (ERROR);
        }
    
    if (pEnd->pFuncTable->formAddress != NULL)
        {
        if (pEnd->pFuncTable->addrGet != NULL)
            {  
            error = pEnd->pFuncTable->addrGet (pMblk, pSrcAddr, pDstAddr,
                                               pESrcAddr, pEDstAddr);
            }
        }
    else
        {
        error = ERROR;
        }

    return (error);
    }

/******************************************************************************
*
* endFindByName - find a device using its string name
*
* This routine takes a string name and a unit number and finds the
* device that has that name/unit combination.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call endFindByName() from within the kernel 
* protection domain only, and the data referenced in the <pName> 
* parameter must reside in the kernel protection domain. In addition, the 
* returned END_OBJ is valid in the kernel protection domain only. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: A pointer to an END_OBJ; or NULL, if the device is not found.
*
*/

END_OBJ * endFindByName
    (
    char * pName,			/* device name to search for */
    int    unit
    )
    {
    BOOL          found = FALSE;
    END_TBL_ROW * pNode;
    END_OBJ *     pEnd;
    
    if (pName == NULL)
	return (NULL);

    for (pNode = (END_TBL_ROW *)lstFirst(&endList); pNode != NULL; 
	pNode = (END_TBL_ROW *)lstNext(&pNode->node))
	{
	if (STREQ(pNode->name, pName))
	    {
            found = TRUE;
            break;
	    }
	}

    if (found)
        {
        for (pEnd = (END_OBJ *)lstFirst(&pNode->units); pEnd != NULL; 
             pEnd = (END_OBJ *)lstNext(&pEnd->node))
            {
            if (pEnd->devObject.unit == unit)
                {
                return (pEnd);
                }
            }
        }    

    errnoSet (S_muxLib_NO_DEVICE);
    return (NULL);

    }

/******************************************************************************
*
* muxDevExists - tests whether a device is already loaded into the MUX
*
* This routine takes a string device name (for example, ln or ei) and a unit
* number.  If this device is already known to the MUX, it returns TRUE.
* Otherwise, this routine returns FALSE. 
*
* .IP <pName>
* Expects a pointer to a string containing the device name
*
* .IP <unit>
* Expects the unit number of the device
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxDevExists() from within the kernel 
* protection domain only, and the data referenced in the <pName> 
* parameter must reside in the kernel protection domain.  
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: TRUE, if the device exists; else FALSE.
*
*/

BOOL muxDevExists
    (
    char * pName,	/* string containing a device name (ln, ei, ...)*/
    int    unit         /* unit number */
    )
    {
    if (endFindByName(pName, unit) == NULL)
        return (FALSE);

    return (TRUE);
    }

/******************************************************************************
*
* muxTxRestart - notify a service that it can restart its transmitter
*
* A device uses this routine to notify all the services bound to it that
* they can resume transmitting.  This after the device blocked transmission
* by returning END_ERR_BLOCK from endSend() or nptSend().
*
* .IP <pCookie>
* The device identifier (END_OBJ) returned from the driver's endLoad() or
* nptLoad() routine.
*
* RETURNS: N/A
*
* ERRNO: S_muxLib_NO_DEVICE
*
* NOMANUAL
*/

void muxTxRestart
    (
    void * pCookie    /* device identifier from driver's load routine */
    )
    {
    NET_PROTOCOL* pProto;
    END_OBJ* pEnd;
    
    pEnd = (END_OBJ *)pCookie;

    if (pEnd == NULL)
        {
        errnoSet (S_muxLib_NO_DEVICE);
        return;
        }

    if (muxLibState.debug)
        logMsg ("muxTxRestart\n", 0, 0, 0, 0, 0, 0);

    /*
     * Loop through the service list.
     */

    for (pProto = (NET_PROTOCOL *)lstFirst(&pEnd->protocols); 
	pProto != NULL; pProto = (NET_PROTOCOL *)lstNext(&pProto->node))
	{
	if (pProto->stackTxRestartRtn != NULL)
            {
	    /* 
	     * the call back prototype differs depending on whether muxBind()
	     * or muxTkBind was used to bind the service to the device.
	     */

            if (pProto->nptFlag)
                {
                MUX_ID muxId = (MUX_ID)pProto->pSpare;
                void * pSpare = NULL;

                pSpare = (muxId) ? muxId->pNetCallbackId : NULL;
                pProto->stackTxRestartRtn(pSpare);
                }
            else
                pProto->stackTxRestartRtn (pCookie, pProto->pSpare);
            }
        }
    }

/******************************************************************************
*
* muxError - notify a service that an error occurred.
*
* A driver uses this routine to notify all of the services bound to it that
* an error has occurred.  A set of error values are defined in end.h.
*
* .IP <pCookie>
* Expects the cookie that was returned by the driver's endLoad() or nptLoad()
* routine, a pointer to the driver's END_OBJ structure.
*
* .IP <pError>
* Expects an error structure as defined in end.h
*
* RETURNS: N/A
*
* ERRNO: S_muxLib_NO_DEVICE
*
* NOMANUAL
*/

void muxError
    (
    void *    pCookie,                        /* END_OBJ */
    END_ERR * pError                          /* Error structure. */
    )
    {
    NET_PROTOCOL* pProto;
    END_OBJ* pEnd;
    
    pEnd = (END_OBJ *)pCookie;

    if (pEnd == NULL)
        {
        errnoSet(S_muxLib_NO_DEVICE);
        return;
        }

    if (muxLibState.debug)
        logMsg ("muxError\n", 0, 0, 0, 0, 0, 0);

    /*
     * Loop through the service list.
     */

    for (pProto = (NET_PROTOCOL *)lstFirst (&pEnd->protocols); 
	pProto != NULL; pProto = (NET_PROTOCOL *)lstNext (&pProto->node))
	{
	if (pProto->stackErrorRtn != NULL)
            {
	    /* 
	     * the call back prototype differs depending on whether muxBind()
	     * or muxTkBind() was used to bind the service to this device.
	     */

            if (pProto->nptFlag)
                {
                MUX_ID muxId = (MUX_ID)pProto->pSpare;
                void * pSpare = NULL;

                pSpare = (muxId) ? muxId->pNetCallbackId : NULL;
                pProto->stackErrorRtn (pSpare,pError);
                }
            else
		pProto->stackErrorRtn (pCookie, pError, pProto->pSpare);
            }
        }
    }

#ifndef	STANDALONE_AGENT
/******************************************************************************
*
* muxAddrResFuncAdd - replace the default address resolution function
*
* Use muxAddrResFuncAdd() to register an address resolution function for an 
* interface-type/protocol pair. You must call muxAddrResFuncAdd() prior to 
* calling the protocol's <protocol>Attach() routine. If the driver registers 
* itself as an Ethernet driver, you do not need to call muxAddrResFuncAdd(). 
* VxWorks automatically assigns arpresolve() to registered Ethernet devices. 
* The muxAddrResFuncAdd() functionality is intended for those who want to use
* VxWorks network stack with non-Ethernet drivers that require address 
* resolution. 
*
* .IP <ifType>
* Expects a media interface or network driver type, such as can be found in
* m2Lib.h. If using the END model, the <ifType> argument is restricted to 
* the values in m2Lib.h. In the NPT model, this restriction does not apply.
*
* .IP <protocol>
* Expects a network service or protocol type, such as can be found in RFC
* 1700. Look for the values under ETHER TYPES. For example, Internet IP
* would be identified as 2048 (0x800 hexadecimal). If using the END model, 
* <protocol> is restricted to the values in RFC 1700. In the NPT model,
* this restriction does not apply.
*
* .IP <addrResFunc>
* Expects a pointer to an address resolution function for this combination
* of driver type and service type. The prototype of your replacement address 
* resolution function must match that of arpresolve():
* 
* .CS
* int arpresolve
*    (
*    struct arpcom *   ac,
*    struct rtentry *  rt,
*    struct mbuf *     m,
*    struct sockaddr * dst,
*    u_char *          desten
*    )
* .CE
* 
* This function returns one upon success, which indicates that <desten> has 
* been updated with the necessary data-link layer information and that the 
* IP sublayer output function can transmit the packet. 
* 
* This function returns zero if it cannot resolve the address immediately. 
* In the default arpresolve() implementation, resolving the address 
* immediately means arpresolve() was able to find the address in its 
* table of results from previous ARP requests. Returning zero indicates 
* that the table did not contain the information but that the packet 
* has been stored and that an ARP request has been queued. 
* 
* If the ARP request times out, the packet is dropped. If the ARP request 
* completes successfully, processing that event updates the local ARP table 
* and resubmits the packet to the IP sublayer's output function for 
* transmission. This time, the arpresolve() call will return one. 
*
* What is essential to note here is that arpresolve() did not wait for 
* the ARP request to complete before returning. If you replace the default 
* arpresolve() function, you must make sure your function returns as soon 
* as possible and that it never blocks. Otherwise, you block the IP sublayer 
* from transmitting other packets out through the interface for which this 
* packet was queued. You must also make sure that your arpresolve() function 
* takes responsibility for the packet if it returns zero. Otherwise, the 
* packet is dropped. 
*
* RETURNS: OK, or ERROR.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxAddrResFuncAdd() from within the kernel 
* protection domain only, and the data referenced in the <addrResFunc> 
* parameter must reside in the kernel protection domain.  
* This restriction does not apply under non-AE versions of VxWorks.  
*
*/

STATUS muxAddrResFuncAdd
    (
    long    ifType,      /* Media interface type, typically from m2Lib.h */
    long    protocol,    /* Service type, for instance from RFC 1700 */
    FUNCPTR addrResFunc  /* Function to call. */
    )
    {
    BOOL           found = FALSE;
    MUX_ADDR_REC * pNode;
    
    if (ifType > MUX_MAX_IFTYPE)
        return (ERROR);

    if (&addrResList[ifType] == NULL)
        {
        lstInit (&addrResList[ifType]);
        pNode = KHEAP_ALLOC (sizeof (MUX_ADDR_REC));
        if (pNode == NULL)
            {
            return (ERROR);
            }
        pNode->addrResFunc = addrResFunc;
        pNode->protocol = protocol;
        lstAdd(&addrResList[ifType], &pNode->node);
        return (OK);
        }
    else
        {
        for (pNode = (MUX_ADDR_REC *)lstFirst (&addrResList[ifType]);
             pNode != NULL; 
             pNode = (MUX_ADDR_REC *)lstNext (&pNode->node))
            {
            if (pNode->protocol == protocol)
                {
                found = TRUE;
                break;
                }
            }
        }
    if (found)
        {
        return (ERROR);
        }
    else
        {
        pNode = KHEAP_ALLOC (sizeof (MUX_ADDR_REC));
        if (pNode == NULL)
            {
            return (ERROR);
            }
        pNode->addrResFunc = addrResFunc;
        pNode->protocol = protocol;
        lstAdd(&addrResList[ifType], &pNode->node);
        return (OK);
        }
    }

/******************************************************************************
*
* muxAddrResFuncGet - get the address resolution function for ifType/protocol
*
* This routine gets a pointer to the registered address resolution function 
* for the specified interface-protocol pair.  If no such function exists, 
* muxAddResFuncGet() returns NULL.  
*
* .IP <ifType>
* Expects a media interface or network driver type, such as those found
* in m2Lib.h. If using the END model, the <ifType> argument is restricted to 
* the m2Lib.h values. In the NPT model, this restriction does not apply.
*
* .IP <protocol>
* Expects a network service or protocol type such as those found in RFC 1700.
* Look for the values under ETHER TYPES.  For example, Internet IP would 
* be identified as 2048 (0x800 hexadecimal). If using the END model, the 
* <protocol> argument is restricted to the RFC 1700 values.  In the NPT model,
* this restriction does not apply.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxAddrResFuncGet() from within the kernel 
* protection domain only.  In addition, the returned FUNCPTR is valid in the 
* kernel protection domain only.  This restriction does not apply under 
* non-AE versions of VxWorks.  
*
* RETURNS: FUNCPTR to the routine or NULL.
*/

FUNCPTR muxAddrResFuncGet
    (
    long ifType,                             /* ifType from m2Lib.h    */
    long protocol                            /* protocol from RFC 1700 */
    )
    {
    MUX_ADDR_REC * pNode;

    if (ifType > MUX_MAX_IFTYPE)
        return (NULL);

    if (&addrResList[ifType] == NULL)
        return (NULL);

    for (pNode = (MUX_ADDR_REC *)lstFirst (&addrResList[ifType]);
         pNode != NULL; 
         pNode = (MUX_ADDR_REC *)lstNext (&pNode->node))
        {
        if (pNode->protocol == protocol)
            {
            return (pNode->addrResFunc);
            }
        }

    return (NULL);
    }
    
/******************************************************************************
*
* muxAddrResFuncDel - delete an address resolution function
*
* This function deletes the address resolution function registered for the
* specified interface-protocol pair. If using the NPT architecture, the 
* <ifType> and <protocol> arguments are not restricted to the m2Lib.h or 
* RFC 1700 values.
*
* .IP <ifType>
* Expects a media interface or network driver type. For and END driver, 
* use the values specified in m2Lib.h.
*
* .IP <protocol>
* Expects a network service or protocol type. For example, Internet IP would 
* be identified as 2048 (0x800 hexadecimal). This value can be found in RFC 
* 1700 under the heading, ETHER TYPES.  
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call muxAddrResFuncDel() from within the kernel 
* protection domain only.  This restriction does not apply under non-AE 
* versions of VxWorks.  
*
* RETURNS: OK or ERROR.
*/
    
STATUS muxAddrResFuncDel
    (
    long ifType,    /* ifType of function you want to delete */
    long protocol   /* protocol from which to delete the function */
    )
    {
    MUX_ADDR_REC* pNode;

    if (ifType > MUX_MAX_IFTYPE)
        return (ERROR);

    if (&addrResList[ifType] == NULL)
        return (ERROR);

    for (pNode = (MUX_ADDR_REC *)lstFirst (&addrResList[ifType]);
         pNode != NULL; 
         pNode = (MUX_ADDR_REC *)lstNext (&pNode->node))
        {
        if (pNode->protocol == protocol)
            {
            lstDelete (&addrResList[ifType], &pNode->node);
            KHEAP_FREE ((char *)pNode);
            return (OK);
            }
        }

    return (ERROR);
    }
#endif	/* STANDALONE_AGENT */

/******************************************************************************
*
* muxDevStopAll - stop all drivers that have been loaded into the MUX
*
* This routine calls the "stop" entry point for all drivers loaded into the
* MUX.
*
* WARNING:  Calling this routine with a timeout that is not WAIT_FOREVER
*           is a violation of mutual exclusion and will cause problems in
*           a running system.
*
*           This option should only be used if the system is being rebooted
*           (i.e. when called from a reboot hook) or if the application has
*           very specific knowledge of the state of all drivers in the system.
*
* RETURNS: OK or ERROR
*
* SEE ALSO
* muxDevStop
*
* NOMANUAL
*/

STATUS muxDevStopAll
    (
    int timeout /* Number of ticks to wait before blowing away the devices. */
    )
    {
    END_TBL_ROW * pEndTblRow;    
    END_OBJ *     pEnd;

    /* Mutual exclusion is necessary to protect <endList> access */

    semTake (muxLibState.lock, timeout);
    
    /* Each Table row contains a list of ENDs for a particular driver */

    for (pEndTblRow = (END_TBL_ROW *)lstFirst (&endList); 
	 pEndTblRow != NULL; 
	 pEndTblRow = (END_TBL_ROW *)lstNext (&pEndTblRow->node))
	{

	/* Loop through all ENDs in the table row and call the stop routine */

        for (pEnd = (END_OBJ *)lstFirst (&pEndTblRow->units); 
	     pEnd != NULL; 
             pEnd = (END_OBJ *)lstNext (&pEnd->node))
            {
            if (pEnd->pFuncTable->stop != NULL)
                {
                (void) pEnd->pFuncTable->stop (pEnd);
                }
	    }    
	}

    semGive (muxLibState.lock);

    return (OK);
    }

/******************************************************************************
*
* muxDevStopAllImmediate - muxDevStopAll for rebootHook use only
*  
* RETURNS: OK or ERROR
*
* SEE ALSO
* muxDevStopAll
*
* NOMANUAL
*/

LOCAL STATUS muxDevStopAllImmediate (void)
    {
    return (muxDevStopAll (0));
    }

/******************************************************************************
*
* muxEndFlagsNotify - notify all services of the new driver flags value
*
* This function executes in the context of tNetTask and its sole purpose
* is to traverse the protocol list in the driver, notifying all the bound
* services of a change in the device flags, except the service that
* changed the flags value in the first place.
* 
* RETURNS: N/A
* NOMANUAL
*/

LOCAL void muxEndFlagsNotify
    (
    void * 	pCookie, 	/* binding instance cookie */
    long 	endFlags 	/* new END flag settings */
    )
    {
    END_ERR endErr;
    MUX_ID  muxId = (MUX_ID)pCookie;
    NET_PROTOCOL * pProto;

    char msgBuffer[128];

    if ((muxId == NULL) || (muxId->pEnd == NULL))
        return;

    bzero (msgBuffer,128);

    /* create a message */

    sprintf (msgBuffer,
             "END FLAGS changed by protocol ID 0x%x for: %s Unit %d\n",
             (UINT)muxId->netSvcType, muxId->pEnd->devObject.name,
             muxId->pEnd->devObject.unit);

    /* create the error notification */

    endErr.errCode = END_ERR_FLAGS;
    endErr.pMesg = msgBuffer;
    (UINT32)(endErr.pSpare) = endFlags;

    /*
     * notify all protocols that are bound to this END, except the one that
     * changed the flag settings originally.
     */

    for (pProto = (NET_PROTOCOL *)lstFirst(&muxId->pEnd->protocols); 
	 pProto != NULL; pProto = (NET_PROTOCOL *)lstNext (&pProto->node))
	{
        if (pProto->type != muxId->netSvcType && pProto->stackErrorRtn != NULL)
	    {
	    /* 
	     * The call back prototype differs depending on whether muxBind()
	     * or muxTkBind() was used to bind the protocol to the device.
	     */

	    if (pProto->nptFlag)    /* muxTkBind: new NPT prototype. */
                {
                MUX_ID protoMuxId = (MUX_ID)pProto->pSpare;
                void * pSpare = NULL;

                pSpare = (protoMuxId) ? protoMuxId->pNetCallbackId : NULL;
                pProto->stackErrorRtn(pSpare, &endErr);
                }
            else                   /* muxBind: original prototype. */
		pProto->stackErrorRtn (muxId->pEnd, &endErr, pProto->pSpare);
            }
        }

    return;
    }

/******************************************************************************
*
* muxDevStartAll - start all ENDs that have been loaded into the MUX
*
* This routine calls the "start" entry point for all ENDs loaded into the MUX.
*
* RETURNS: OK or ERROR
*
* SEE ALSO
* muxDevStopAll
*
* NOMANUAL
*/

STATUS muxDevStartAll
    (
    )
    {
    END_TBL_ROW* pEndTblRow;    
    END_OBJ* pEnd;

    /* Mutual exclusion is necessary to protect <endList> access */

    semTake (muxLibState.lock, WAIT_FOREVER);
    
    /* Each Table row contains a list of ENDs for a particular driver */

    for (pEndTblRow = (END_TBL_ROW *)lstFirst(&endList); 
	 pEndTblRow != NULL; 
	 pEndTblRow = (END_TBL_ROW *)lstNext(&pEndTblRow->node))
	{

	/* Loop through all ENDs in the table row and call the stop routine */

        for (pEnd = (END_OBJ *)lstFirst(&pEndTblRow->units); 
	     pEnd != NULL; 
             pEnd = (END_OBJ *)lstNext(&pEnd->node))
            {
            if (pEnd->pFuncTable->start != NULL)
		{
		(void) pEnd->pFuncTable->start(pEnd->devObject.pDevice);
		}
	    }    
	}

    semGive (muxLibState.lock);

    return (OK);

    }


/******************************************************************************
*
* muxPollTask - the routine that runs as the MUX polling task.
*
* This routine does the actual polling.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void muxPollTask ()
    {
    int i;
    END_OBJ *pEnd;

    for(;;)
        {
	semTake (muxLibState.lock, WAIT_FOREVER);
	for (i = 0; i < muxPollDevCount; i++)
	     {
	     pEnd = PCOOKIE_TO_ENDOBJ (ppMuxPollDevices[i]);

	     /* We check to see if there is already a mblk, if not get one */
	     pMuxPollMblkRing[i] = pMuxPollMblkRing[i] ? pMuxPollMblkRing[i] :
		 netTupleGet2 (pEnd->pNetPool, GET_IFMTU(pEnd), FALSE);

	     if(!pMuxPollMblkRing[i])
		 continue;

	     if (!TK_DRV_CHECK (ppMuxPollDevices[i]))
	         {
	         if (pEnd->pFuncTable->pollRcv (pEnd, pMuxPollMblkRing[i]) == OK)
		     {
                     muxReceive (pEnd, pMuxPollMblkRing[i]);
		     pMuxPollMblkRing[i] = NULL;
		     }
	         }		
             else
	         {
		 if (muxTkPollReceive2 (pEnd, pMuxPollMblkRing[i]) == OK);
		     pMuxPollMblkRing[i] = NULL;
	         }		

            }
        semGive (muxLibState.lock);
        taskDelay (muxLibState.taskDelay);
        }

    }

/*****************************************************************************
*
* muxTaskDelaySet - set the inter-cycle delay on the polling task
*
* This routine sets up a delay (measured in ticks) that is inserted at the 
* end of each run through the list of devices polled by 'tMuxPollTask'.
*
* RETURNS: OK; or ERROR, if you specify a delay less than zero. 
*
*/
STATUS muxTaskDelaySet
    (
    int delay
    )
    {
    if (delay < 0)
        return (ERROR);
    
    muxLibState.taskDelay = delay;

    return (OK);
    }

/*****************************************************************************
*
* muxTaskDelayGet - get the delay on the polling task
*
* This routine returns the amount of delay (in ticks) that is inserted 
* between the polling runs of 'tMuxPollTask'.  This value is written to
* the location specified by <pDelay>.
*
* RETURNS: OK; or ERROR, if NULL is passed in the <pDelay> variable. 
*
*/

STATUS muxTaskDelayGet
    (
    int* pDelay
    )
    {

    if (pDelay == NULL)
        return (ERROR);
    
    *pDelay = muxLibState.taskDelay;

    return (OK);
    }
    
/*****************************************************************************
*
* muxTaskPrioritySet - reset the priority of 'tMuxPollTask'
*
* This routine resets the priority of a running 'tMuxPollTask'.  Valid 
* task priorities are values between zero and 255 inclusive.  However, 
* do not set the priority of 'tMuxPollTask' to exceed that of 'tNetTask'. 
* Otherwise, you will shut out 'tNetTask' from getting any processor time. 
*
* RETURNS: OK; or ERROR, if you specify a non-valid priority value. 
*
*/

STATUS muxTaskPrioritySet
    (
    int priority
    )
    {
    if (priority < 0 || priority > 255)
        return (ERROR);
    
    muxLibState.priority = priority;

    /*
     * If we're in polling mode then the task must be running.
     * So, find the task and update it's priority.
     */
     if (muxLibState.taskID)
        if (taskPrioritySet (muxLibState.taskID, muxLibState.priority) == ERROR)
            {
            if (muxLibState.debug)
                {
                logMsg("Invalid muxPollTask taskID\n", 1, 2, 3, 4, 5, 6);
                }
            }
    return (OK);
    }

/*****************************************************************************
*
* muxTaskPriorityGet - get the priority of 'tMuxPollTask'
*
* This routine returns the current priority of 'tMuxPollTask'. This value
* is returned to the location specified by the <pPriority> parameter. 
*
* RETURNS: OK; or ERROR, if NULL is passed in the <pPriority> parameter.
*
*/

STATUS muxTaskPriorityGet
    (
    int* pPriority
    )
    {

    if (pPriority == NULL)
        return (ERROR);
    
    *pPriority = muxLibState.priority;

    return (OK);
    }
   
/*****************************************************************************
*
* muxPollStart - initialize and start the MUX poll task
*
* This routine initializes and starts the MUX poll task, 'tMuxPollTask'.  
* This task runs an infinite loop in which it polls each of the interfaces 
* referenced on a list of network interfaces. To add or remove devices 
* from this list, use muxPollDevAdd() and muxPollDevDel(). Removing all 
* devices from the list automatically triggers a call to muxPollEnd(), 
* which shuts down 'tMuxPollTask'.
* 
* Using the <priority> parameter, you assign the priority to 'tMuxPollTask'.
* Valid values are between 0 and 255, inclusive. However, you must not set
* the priority of 'tMuxPollTask' to exceed that of 'tNetTask'. Otherwise, 
* you risk shutting 'tNetTask' out from getting processor time. To reset 
* the 'tMuxPollTask' priority after launch, use muxTaskPrioritySet(). 
*
* Using the <delay> parameter, you can set up a delay at the end of each 
* trip though the poll list. To reset the value of this delay after the 
* launch of 'tNetTask', call muxTaskDelaySet(). 
*
* To shut down 'tMuxPollTask', call muxPollEnd(). 
* 
* RETURNS: OK or ERROR
*
*/


STATUS muxPollStart
    (
    int numDev,     /* Maximum number of devices to support poll mode. */
    int priority,   /* tMuxPollTask priority, not to exceed tNetTask. */
    int delay       /* Delay, in ticks, at end of each polling cycle. */
    )
    {
    int i;

    if (!(numDev > 0))
	return (ERROR);

    if (priority < 0 || priority > 255 || delay < 0)
        return (ERROR);
	   
    muxLibState.priority = priority;
    muxLibState.taskDelay = delay;

    if (!muxLibState.lock)
	return (ERROR);

    ppMuxPollDevices = (void **) KHEAP_ALLOC ((numDev + 1) * sizeof (void *));
    if (!ppMuxPollDevices)
	return (ERROR);
    for (i = 0; i <= numDev; i++)
	  ppMuxPollDevices[i] = NULL;

    pMuxPollMblkRing = (M_BLK_ID *) KHEAP_ALLOC ((numDev + 1) * sizeof (M_BLK_ID));
    if (!pMuxPollMblkRing)
	{
	KHEAP_FREE ((char *)ppMuxPollDevices);
	return (ERROR);
        }
    for (i = 0; i <= numDev; i++)
	  pMuxPollMblkRing[i] = NULL;

    muxPollDevMax = numDev;

    muxPollDevCount = 0;

    muxLibState.taskID = taskSpawn ("tMuxPollTask", priority, 0, 0x4000, 
			   (FUNCPTR)muxPollTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (muxLibState.taskID == ERROR)
	{
        semDelete (muxLibState.lock);
	KHEAP_FREE ((char *)ppMuxPollDevices);
	KHEAP_FREE ((char *)pMuxPollMblkRing);
	muxLibState.taskID = 0;
	return (ERROR);
	}

    return (OK);
    }

/*****************************************************************************
*
* muxPollEnd - shuts down 'tMuxPollTask' and returns devices to interrupt mode
*
* This routine shuts down 'tMuxPollTask' and returns network devices to run in 
* their interrupt-driven mode. 
*
* RETURNS: OK or ERROR
*
*/

STATUS muxPollEnd ()
    {
    int i;
    void **pList;

    if (!muxLibState.taskID)
	return (ERROR);

    semTake (muxLibState.lock, WAIT_FOREVER);
    taskDelete (muxLibState.taskID);
    muxLibState.taskID = 0;

    for (pList = ppMuxPollDevices; *pList; pList++)
         muxIoctl(*pList, EIOCPOLLSTOP, NULL);
         
    KHEAP_FREE ((char *)ppMuxPollDevices);

    for (i = 0; i < muxPollDevCount; i++)
	 if (pMuxPollMblkRing[i])
	      netMblkClChainFree (pMuxPollMblkRing[i]);

    KHEAP_FREE ((char *)pMuxPollMblkRing);

    semGive (muxLibState.lock);

    return (OK);
    }


/*****************************************************************************
*
* muxPollDevAdd - adds a device to list polled by 'tMuxPollTask'
*
* This routine adds a device to list of devices polled by 'tMuxPollTask'. It 
* assumes that you have already called muxPollStart() and that 'tMuxPollTask' 
* is still running.  
*
* NOTE: You cannot use a device for WDB_COMM_END type debugging while that 
* device is on the 'tMuxPollTask' poll list. 
*
* RETURNS: OK or ERROR
*
*/

STATUS muxPollDevAdd
    (
    int unit,			/* Device unit number */
    char *pName			/* Device name */
    )
    {
    void *pCookie = NULL;
    END_OBJ *pEnd = NULL;
     
    pCookie = muxTkCookieGet (pName, unit);

    if (muxPollDevStat (unit, pName) == TRUE)
	return (ERROR);
	
    if (!muxLibState.taskID || !pCookie)
        return (ERROR);

    pEnd = PCOOKIE_TO_ENDOBJ(pCookie);
    if (pEnd == NULL)
        return (ERROR);
    
    if ( (pEnd->flags & END_MIB_2233) && (pEnd->pMib2Tbl == NULL) )
	return (ERROR);

    semTake (muxLibState.lock, WAIT_FOREVER);

    if (muxPollDevCount == muxPollDevMax)
	{
        semGive (muxLibState.lock);
	return (ERROR);
        } 

    
    if (muxIoctl (pCookie, EIOCPOLLSTART, NULL) == ERROR)
	{
        semGive (muxLibState.lock);
	return (ERROR);
        } 
	

    ppMuxPollDevices[muxPollDevCount++] = pCookie;

    semGive (muxLibState.lock);

    return (OK);
    }


/*****************************************************************************
*
* muxPollDevDel - removes a device from the list polled by 'tMuxPollTask'
*
* This routine removes a device from the list of devices polled by 
* 'tMuxPollTask'. If you remove the last device on the list, a call to 
* muxPollDevDel() also makes an internal call to muxPollEnd().  This shuts 
* down 'tMuxPollTask' completely.  
*
* RETURNS: OK or ERROR
*
*/

STATUS muxPollDevDel
    (
    int unit,			/* Device unit number */
    char *pName			/* Device name */
    )
    {
    int i;
    void *pCookie;
    void **pList;

    pCookie = muxTkCookieGet (pName, unit);

    if (!muxLibState.taskID || !pCookie)
        return (ERROR);

    semTake (muxLibState.lock, WAIT_FOREVER);

    for (i = 0; (i < muxPollDevCount) && (ppMuxPollDevices[i] != pCookie); i++);

    if ((pList = (ppMuxPollDevices + i)) != pCookie)
	{
	semGive (muxLibState.lock);
	return (ERROR);
	}

    muxIoctl (pCookie, EIOCPOLLSTOP, NULL);
    if (pMuxPollMblkRing[i])
         netMblkClChainFree (pMuxPollMblkRing[i]);
    pMuxPollMblkRing[i] = NULL;

    /* 
     * We don't care about order! So when a device is removed,
     * we take the last device on the list and fill in the spot
     * that was just cleared up. Notice that this is harmless
     * if one device is left.
     */

    if (--muxPollDevCount > 0)
	{
        pList = ppMuxPollDevices + muxPollDevCount;
	ppMuxPollDevices[muxPollDevCount] = NULL;
	pMuxPollMblkRing[i] = pMuxPollMblkRing[muxPollDevCount];
	}
    else
	{
	semGive (muxLibState.lock);
	/* Remove muxPollTask if there are no devices left */
	return (muxPollEnd ());
	}

    semGive (muxLibState.lock);
    return (OK);
    }

/*****************************************************************************
*
* muxPollDevStat - reports whether device is on list polled by 'tMuxPollTask'
*
* This routine returns true or false depending on whether the specified 
* device is on the list of devices polled by 'tMuxPollTask'. 
*
* RETURNS: TRUE, if it is; or FALSE.
*
*/

BOOL muxPollDevStat
    (
    int unit,			/* Device unit number */
    char *pName			/* Device name */
    )
    {
    int i;
    void *pCookie;
    
    pCookie = muxTkCookieGet (pName, unit);

    if (!muxLibState.taskID || !pCookie)
        return (FALSE);

    semTake (muxLibState.lock, WAIT_FOREVER);

    for (i = 0; (i < muxPollDevCount) && (ppMuxPollDevices[i] != pCookie); i++);

    if (ppMuxPollDevices[i] != pCookie)
	{
	semGive (muxLibState.lock);
	return (FALSE);
	}

    semGive (muxLibState.lock);
    return (TRUE);
    }

#ifdef ROUTER_STACK

/****************************************************************************
*
* muxProtoPrivDataGet - Returns the private data for the protocol specified
*
* This routine returns private data (specified by the <pSpare> pointer in the
* muxBind() call) for the specified protocol, <proto>, bound to the
* END device <pEnd>.
*
* RETURNS: The protocol specific private data if the protocol is bound to
* the MUX; NULL, otherwise.
*
* ERRNO:
* S_muxLib_NO_DEVICE
*
* NOMANUAL
* NOTE: Currently this is used only by Fastpath
*/

void * muxProtoPrivDataGet 
    (
    END_OBJ *	pEnd,	/* Points to the END device */
    int		proto	/* The protocol whose private data is desired */
    )
    {
    NET_PROTOCOL *	pProto;

    if (pEnd == NULL)
        {
        errno = S_muxLib_NO_DEVICE;
        return (NULL);
        }

    /* Loop through the protocol list, looking for the IP protocol binding. */

    for (pProto = (NET_PROTOCOL *)lstFirst (&pEnd->protocols); 
         pProto != NULL; pProto = (NET_PROTOCOL *)lstNext (&pProto->node))
	{
        if (pProto->type == proto)   
            {
            return (pProto->pSpare);
            }
        }

    return (NULL);
    }
#endif /* ROUTER_STACK */

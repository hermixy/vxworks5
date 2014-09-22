/* wdbBdmPktDrv.c - Bdm packet driver for lightweight UDP/IP */

/* Copyright 1998-2001 Wind River Systems, Inc. */

/*
This is an unsupported release of the Background Debug Mode (BDM) Back
End. It is provided AS IS with no warranties of any kind.
*/

#include "copyright_wrs.h"

/*
modification history
--------------------
01b,15sep01,dat  Use of WRS_ASM macro
01a,16jun98,vrd  written, inspired from wdbNetromPktDrv.c version 01h.
*/

/*
DESCRIPTION

OVERVIEW
This is a driver for WDB system which uses the BDM line and protocol to
communicate with the HOST.

USAGE
The driver is typically only called only from usrWdb.c. The only directly
callable routine in this module is wdbBdmPktDevInit().  Your configAll.h
file will have to be modified so that WDB_COMM_TYPE is defined as
WDB_COMM_BDM.


DATA BUFFERING
The drivers only need to handle one input packet at a time because
the WDB protocol only supports one outstanding host-request at a time.
If multiple input packets arrive, the driver can simply drop them.
The driver then loans the input buffer to the WDB agent, and the agent
invokes a driver callback when it is done with the buffer.

For output, the agent will pass the driver a chain of mbufs, which
the driver must send as a packet. When it is done with the mbufs,
it calls wdbMbufChainFree() to free them.
The header file wdbMbuflib.h provides the calls for allocating, freeing,
and initializing mbufs for use with the lightweight UDP/IP interpreter.
It ultimatly makes calls to the routines wdbMbufAlloc() and wdbMbufFree(),
which are provided in source code in usrWdb.c.
*/

/* includes */

#include "vxWorks.h"
#include "string.h"
#include "errno.h"
#include "intLib.h"
#include "wdb/wdbLib.h"
#include "wdb/wdbCommIfLib.h"
#include "wdb/wdbMbufLib.h"
#include "drv/wdb/wdbBdmPktDrv.h"

/* defines */

#define MIN min

/* typedefs */

typedef struct
    {
    UINT8	recvSem;
    UINT8	padding;
    UINT16	recvCount;
    UINT8	recvBuffer [WDB_BDM_PKT_MTU];
    UINT8	sendSem;
    UINT8	padding2;
    UINT16	sendCount;
    UINT8	sendBuffer [WDB_BDM_PKT_MTU];
    } BDM_COMM_BUFF;

/* locals */

LOCAL BDM_COMM_BUFF	bdmCommBuff;
LOCAL BDM_COMM_BUFF *	pBdmCommBuff = &bdmCommBuff;
LOCAL int		pollDelay = 2;	/* polling delay */

/* forward declarations */

LOCAL STATUS	wdbBdmPoll		(void *pDev);
LOCAL STATUS	wdbBdmTx		(void *pDev, struct mbuf * pMbuf);
LOCAL STATUS	wdbBdmModeSet		(void *pDev, uint_t newMode);
LOCAL void	wdbBdmFree		(void *pDev);
LOCAL void	wdbBdmPollTask		(void * pDev);
LOCAL void	wdbBdmTargetStop	(void);
LOCAL STATUS	wdbBdmInitHardware	(void);

/******************************************************************************
*
* wdbBdmTargetStop - stop the CPU
*
* This routine contains CPU specific code to put the CPU in BDM mode.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void wdbBdmTargetStop (void)
    {
#if 	(CPU == CPU32)
    WRS_ASM ("nop");
    WRS_ASM (".word	0x4afa");
#endif	/* (CPU == CPU32) */

#if 	(CPU == PPC860)
    WRS_ASM ("sc");			/* sc instruction */
#if	FALSE
    WRS_ASM ("tw	31,0,0");	/* trap instruction */
#endif	/* FALSE */
#endif	/* (CPU == PPC860) */
    }

/******************************************************************************
*
* wdbBdmInitHardware - initialize the BDM communiction buffer
*
* This routine computes the input and the output buffer address.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL STATUS wdbBdmInitHardware (void)
    {

    /* reset memory */
    
    bdmCommBuff.recvSem = 0;
    bdmCommBuff.recvCount = 0;
    bdmCommBuff.sendSem = 0;
    bdmCommBuff.sendCount = 0;

    /*
     * Here you must put the IOBuff address in the first register (the first
     * one for the Macraigor's ReadRegister routine and then put the CPU in
     * background. The backend will restart the CPU.
     */

#if 	(CPU == CPU32)
    WRS_ASM ("move.l	_pBdmCommBuff,D3");
    WRS_ASM ("nop");
    WRS_ASM (".word	0x4afa");
#elif 	(CPU == PPC860)
    WRS_ASM ("addi	3, 0, bdmCommBuff@ha");	/* move IOBuff@ha to R3 */
    WRS_ASM ("rlwinm	3, 3, 16, 0, 15");	/* move IOBuff@ha to R3@ha */
    WRS_ASM ("addi	3, 3, bdmCommBuff@l");	/* move IOBuff@l to R3 */

    WRS_ASM ("sc");			/* sc instruction */
#if 	FALSE
    WRS_ASM ("tw	31, 0, 0");	/* trap instruction */
#endif	/* FALSE */
#endif 	/* CPU == CPU32 */

    return(OK);
    }

/******************************************************************************
*
* wdbBdmPktDevInit - initialize a bdm packet device.
*
* RETRUNS: N/A
*
* NOMANUAL
*/

STATUS wdbBdmPktDevInit
    (
    WDB_BDM_PKT_DEV	*pPktDev,	/* bdm device structure to init */
    void		(*stackRcv)()	/* receive packet callback (udpRcv) */
    )
    {

    /* initialize the wdbDrvIf field with driver info */

    pPktDev->wdbDrvIf.mode	= WDB_COMM_MODE_POLL | WDB_COMM_MODE_INT;
    pPktDev->wdbDrvIf.mtu	= WDB_BDM_PKT_MTU;
    pPktDev->wdbDrvIf.stackRcv	= stackRcv;		/* udpRcv */
    pPktDev->wdbDrvIf.devId	= (WDB_BDM_PKT_DEV *)pPktDev;
    pPktDev->wdbDrvIf.pollRtn	= wdbBdmPoll;
    pPktDev->wdbDrvIf.pktTxRtn	= wdbBdmTx;
    pPktDev->wdbDrvIf.modeSetRtn = wdbBdmModeSet;

    /*
     * Put your hardware initialization code here. It should initialize
     * the device structure's register addresses and initialize
     * the hardware.
     */

    if (wdbBdmInitHardware() != OK)
	return(ERROR);

    /* put the device in a quiescent state */

    wdbBdmModeSet (pPktDev, WDB_COMM_MODE_INT);

    /* start the polling task if the OS is initialized */

    if(taskIdCurrent)
	{
	taskSpawn ("tBdmPoll", 200, 0, 5000, (int (*)()) wdbBdmPollTask,
		   (int) pPktDev, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}

    return(OK);
    }

/******************************************************************************
*
* wdbBdmTx - transmit a packet.
*
* The packet is realy a chain of mbufs. We may have to just queue up
* this packet is we are already transmitting.
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

LOCAL STATUS wdbBdmTx
    (
    void *		pDev,
    struct mbuf *	pMbuf
    )
    {
    UINT32		packetSize = 0;

    /* copy transmit data to buffer */

    wdbMbufDataGet (pMbuf, bdmCommBuff.sendBuffer,
                    WDB_BDM_PKT_MTU, &packetSize);

    /* send packet */

    bdmCommBuff.sendCount = (UINT16) htons (packetSize);
    bdmCommBuff.sendSem = 0xFF;
    wdbBdmTargetStop ();

    /* free mbuf chain */

    wdbMbufChainFree (pMbuf);

    return (OK);
    }

/******************************************************************************
*
* wdbBdmFree - free the input buffer
*
* This is the callback used to let us know the agent is done with the
* input buffer we loaded it.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void wdbBdmFree
    (
    void *	pDev		/* unused */
    )
    {
    bdmCommBuff.recvCount = 0;
    bdmCommBuff.recvSem = 0;
    }

/******************************************************************************
*
* wdbBdmModeSet - switch driver modes
*
* RETURNS: OK for a supported mode, else ERROR
*
* NOMANUAL
*/

LOCAL STATUS wdbBdmModeSet
    (
    void *	pDev,
    uint_t	newMode
    )
    {
    WDB_BDM_PKT_DEV * pPktDev = pDev;

    if (newMode == WDB_COMM_MODE_INT)
	pPktDev->mode = WDB_COMM_MODE_INT;
    else if (newMode == WDB_COMM_MODE_POLL)
	pPktDev->mode = WDB_COMM_MODE_POLL;
    else
	return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* wdbBdmPoll - poll for a packet
*
* This routine polls for a packet. If a packet has arrived it invokes
* the agents callback.
*
* RETURNS: OK if a packet has arrived, else ERROR.
*
* NOMANUAL
*/ 

LOCAL STATUS wdbBdmPoll
    (
    void *	pDev
    )
    {
    WDB_BDM_PKT_DEV *	pPktDev = pDev;
    struct mbuf *	pMbuf;
    UINT16		packetSize = 0;

    if (bdmCommBuff.recvSem)
	{

	/*
	 * Fill the input buffer with the packet. Use an mbuf cluster to pass
	 * the packet on to the agent.
	 */
	
	packetSize = ntohs (bdmCommBuff.recvCount);
	pMbuf = wdbMbufAlloc();

	if ((pMbuf == NULL) || (packetSize == 0))
	    {
	    bdmCommBuff.recvSem = 0;
	    return (ERROR);
	    }

	wdbMbufClusterInit (pMbuf, bdmCommBuff.recvBuffer, (UINT32) packetSize,
			    (int (*) ())wdbBdmFree, (int) pPktDev);
	(*pPktDev->wdbDrvIf.stackRcv) (pMbuf);  /* invoke callback */
	return (OK);
	}

    return (ERROR);
    }

/******************************************************************************
*
* wdbBdmPollTask - poll for a packet
*
* This routine polls for a packet. If a packet has arrived it invokes
* the agents callback.
*
* RETURNS: OK if a packet has arrived, else ERROR.
*
* NOMANUAL
*/ 

LOCAL void wdbBdmPollTask
    (
    void *	pDev
    )
    {
    for (;;)
	{
	taskDelay (pollDelay);
	wdbBdmPoll (pDev);
	}
    }

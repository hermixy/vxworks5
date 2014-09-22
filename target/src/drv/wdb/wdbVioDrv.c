/* wdbVioDrv.c - virtual tty I/O driver for the WDB agent */

/* Copyright 1994-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,18oct01,jhw  Fixed documentation build errors.
01i,26feb99,jmp  canceled fixed for SPR #23898, reinstate the outputDone
		 mechanism.
01h,27jan99,dbt  modified event dequeue routine to never send NULL size VIO
                 events (SPR #23898). Fixed the close routine (SPR #24654).
01g,09aug96,ms   close now waits for output complete (SPR 6211).
01f,24oct95,ms   don't send up more data than wdbCommMtu (SPR #5228).
01e,14oct95,jdi  doc: cleanup.
01d,01jun95,ms	 fixed vioInput
01c,24may95,ms	 keep calling tyITx until it returns ERROR.
01b,19may95,ms   one device, multiplexed on open().
01a,08nov94,ms   written.
*/

/*
DESCRIPTION
This library provides a psuedo-tty driver for use with the WDB debug
agent.  I/O is performed on a virtual I/O device just like it is on
a VxWorks serial device.  The difference is that the data is not
moved over a physical serial channel, but rather over a virtual
channel created between the WDB debug agent and the Tornado host
tools.

The driver is installed with wdbVioDrv().  Individual virtual I/O channels
are created by opening the device (see wdbVioDrv for details).  The
virtual I/O channels are defined as follows:

.TS
tab(|);
lf3 lf3
l   l.
Channel		| Usage
_
0		| Virtual console
1-0xffffff	| Dynamically created on the host
>= 0x1000000	| User defined
.TE

Once data is written to a virtual I/O channel on the target, it is sent
to the host-based target server.  The target server allows this data to be
sent to another host tool, redirected to the "virtual console," or
redirected to a file.  For details see the
.I Tornado User's Guide.
*/

#include "vxWorks.h"
#include "ioLib.h"
#include "iosLib.h"
#include "tyLib.h"
#include "stdlib.h"
#include "stdio.h"
#include "intLib.h"
#include "rngLib.h"
#include "taskLib.h"

#include "wdb/wdb.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbEvtLib.h"
#include "wdb/wdbVioLib.h"
#include "wdb/wdbLibP.h"

#define INBUF_SIZE		512
#define OUTBUF_SIZE		512
#define MAX_VIO_XFER_SIZE	512
#define MAX_FLUSH_TRIES		60

typedef struct
    {
    DEV_HDR	devHdr;			/* device header */
    char        buf[MAX_VIO_XFER_SIZE]; /* temporary buffer */
    } VIO_DEV;

typedef struct
    {
    TY_DEV	tyDev;			/* input/output buffers, etc */
    WDB_VIO_NODE vioNode;		/* VIO device list node */
    WDB_EVT_NODE eventNode;		/* RPC event list node */
    VIO_DEV *	pVioDev;		/* device handle */
    BOOL	outputDone;		/* done with output */
    BOOL	vioChannelIsClosed;	/* VIO channel is closed */
    } VIO_CHANNEL_DESC;

static int vioDrvNum;           /* driver number assigned to this driver */

/* forward declarations */

static int     vioOpen		 (VIO_DEV *pDev, char * name, int mode);
static STATUS  vioClose		 (VIO_CHANNEL_DESC *pChannelDesc);
static STATUS  vioIoctl		 (VIO_CHANNEL_DESC *pChannelDesc, int request,
					int arg);
static void    vioStartup	 (VIO_CHANNEL_DESC *pChannelDesc);
static int     vioInput		 (WDB_VIO_NODE *pNode, char *pData,
					uint_t nBytes);
static void    vioWriteEventGet  (void * pChannelDesc, WDB_EVT_DATA *pEvtData);
static void    vioWriteEventDeq  (void * pChannelDesc);


/*******************************************************************************
*
* wdbVioDrv - initialize the tty driver for a WDB agent
*
* This routine initializes the VxWorks virtual I/O driver and creates
* a virtual I/O device of the specified name.
*
* This routine should be called exactly once, before any reads, writes, or
* opens.  Normally, it is called by usrRoot() in usrConfig.c,
* and the device name created is "/vio".
*
* After this routine has been called, individual virtual I/O channels
* can be open by appending the channel number to the virtual I/O
* device name.  For example, to get a file descriptor for virtual I/O
* channel 0x1000017, call open() as follows:
* .CS
*     fd = open ("/vio/0x1000017", O_RDWR, 0)
* .CE
*
* RETURNS: OK, or ERROR if the driver cannot be installed.
*/

STATUS wdbVioDrv
    (
    char *name
    )
    {
    static VIO_DEV	vioDev;			/* virtual I/O device */

    /* check if driver already installed */

    if (vioDrvNum > 0)
	return (OK);

    vioDrvNum = iosDrvInstall (vioOpen, (FUNCPTR) NULL, vioOpen,
                                (FUNCPTR) vioClose, tyRead, tyWrite, vioIoctl);

    if (vioDrvNum <= 0)
        return (ERROR);

    /* Add the device to the I/O systems device list */

    return (iosDevAdd (&vioDev.devHdr, name, vioDrvNum));
    }

/*******************************************************************************
*
* vioOpen - open a virtual I/O channel
*
* RETURNS: vioDv handle
*/

static int vioOpen
    (
    VIO_DEV *	pDev,
    char *	name,
    int		mode
    )
    {
    VIO_CHANNEL_DESC *	pChannelDesc;
    int			channel;

    /* parse the channel number from the name */

    if (name[0]=='/')
	name = &name[1];
    channel = (int) strtol (name, (char **)NULL, 10);
    if ((channel == 0) && (name[0] != '0'))
	return (ERROR);

    /* create a channel descriptor */

    pChannelDesc = (VIO_CHANNEL_DESC *) malloc (sizeof (VIO_CHANNEL_DESC));
    if (pChannelDesc == NULL)
	return (ERROR);

    /* initialize the channel descriptor */

    if (tyDevInit (&pChannelDesc->tyDev, INBUF_SIZE, OUTBUF_SIZE,
                    (FUNCPTR)vioStartup) != OK)
        return (ERROR);

    wdbEventNodeInit (&pChannelDesc->eventNode, vioWriteEventGet,
			vioWriteEventDeq, (void *)pChannelDesc);

    pChannelDesc->pVioDev		 = pDev;
    pChannelDesc->vioNode.channel	 = channel;
    pChannelDesc->vioNode.pVioDev	 = &pChannelDesc->vioNode;
    pChannelDesc->vioNode.inputRtn	 = vioInput;
    pChannelDesc->vioChannelIsClosed	 = FALSE;

    if (wdbVioChannelRegister (&pChannelDesc->vioNode) == ERROR)
	return (ERROR);

    return ((int) pChannelDesc);
    }

/******************************************************************************
*
* vioClose - close a VIO channel
*/ 

static STATUS vioClose
    (
    VIO_CHANNEL_DESC *	pChannelDesc	/* chanel to close */
    )
    {
    int numTries;

    /* unregister VIO channel to prevent further input from host */

    wdbVioChannelUnregister (&pChannelDesc->vioNode);

    /*
     * Dequeue the event node to prevent further output to the host.
     * But first try waiting a bit to allow pending data to be flushed.
     */

    for (numTries = 0; numTries < MAX_FLUSH_TRIES; numTries++)
	if (pChannelDesc->eventNode.onQueue == TRUE)
	    taskDelay (1);

    /* indicate that the VIO channel is closed */

    pChannelDesc->vioChannelIsClosed = TRUE;

    /* dequeue the event node */

    wdbEventDeq (&pChannelDesc->eventNode);

    /* free all the malloc'ed memory */

    rngDelete (pChannelDesc->tyDev.wrtBuf);
    rngDelete (pChannelDesc->tyDev.rdBuf);
    free (pChannelDesc);

    return (OK);
    }

/*******************************************************************************
*
* vioIoctl - special device control
*
* This routine passes all requests to tyIoctl().
*
* RETURNS: whatever tyIoctl() returns.
*/

static STATUS vioIoctl
    (
    VIO_CHANNEL_DESC *	pChannelDesc,	/* device to control */
    int 		request,	/* request code */
    int 		arg		/* some argument */
    )
    {
    STATUS status;

    switch (request)
        {
        default:
            status = tyIoctl (&pChannelDesc->tyDev, request, arg);
            break;
        }
    return (status);
    }

/*******************************************************************************
*
* vioStartup - transmitter startup routine
*
* Call interrupt level character output routine.
*
* RETURNS: N/A
*/

static void vioStartup
    (
    VIO_CHANNEL_DESC *pChannelDesc	/* ty device to start up */
    )
    {
    wdbEventPost (&pChannelDesc->eventNode);
    }

/******************************************************************************
*
* vioWriteEventGet - retrieve the WDB_VIO_WRITE event data
*/

static void vioWriteEventGet
    (
    void *	   arg,
    WDB_EVT_DATA * pEvtData
    )
    {
    uint_t	nBytes;
    VIO_CHANNEL_DESC *	pChannelDesc;
    uint_t	lockKey;
    char *	outBuf;
    int		maxBytes;

    pChannelDesc = (VIO_CHANNEL_DESC *)arg;
    pChannelDesc->outputDone = FALSE;

    /* if the VIO channel is closed, return */

    if (pChannelDesc->vioChannelIsClosed)
    	return;

    outBuf = pChannelDesc->pVioDev->buf;
    maxBytes = wdbCommMtu - 90;
    if (maxBytes > MAX_VIO_XFER_SIZE)
	maxBytes = MAX_VIO_XFER_SIZE;

    for (nBytes = 0; nBytes < maxBytes; nBytes++)
	{
	lockKey = intLock();
	if (tyITx (&pChannelDesc->tyDev, &outBuf[nBytes]) == ERROR)
	    {
	    pChannelDesc->outputDone = TRUE;
	    intUnlock (lockKey);
	    break;
	    }
	intUnlock (lockKey);
	}

    pEvtData->evtType = WDB_EVT_VIO_WRITE;
    pEvtData->eventInfo.vioWriteInfo.destination =
				(TGT_ADDR_T)pChannelDesc->vioNode.channel;
    pEvtData->eventInfo.vioWriteInfo.numBytes   = nBytes;
    pEvtData->eventInfo.vioWriteInfo.source	= outBuf;
    }

/******************************************************************************
*
* vioWriteEventDeq - delete the WDB_VIO_WRITE event
*/

static void vioWriteEventDeq
    (
    void *	arg
    )
    {
    uint_t		lockKey;
    VIO_CHANNEL_DESC *	pChannelDesc = (VIO_CHANNEL_DESC *)arg;

    /* if the VIO channel is closed, return */

    if (pChannelDesc->vioChannelIsClosed)
    	return;

    /* any data remaining? if so, post the event */

    lockKey = intLock();

    if (pChannelDesc->outputDone == FALSE)
	wdbEventPost (&pChannelDesc->eventNode);

    intUnlock (lockKey);
    }

/******************************************************************************
*
* vioInput - handle incoming VIO packets
*/

static int vioInput
    (
    WDB_VIO_NODE *	pVioNode,
    char *		pData,
    uint_t		maxBytes
    )
    {
    int		nBytes;
    uint_t	lockKey;
    TY_DEV *	pTyDev;

#undef	OFFSET
#define OFFSET(type, field) ((int)&((type *)0)->field)

    pTyDev	= (TY_DEV *)((int)pVioNode - OFFSET (VIO_CHANNEL_DESC, vioNode)
				+ OFFSET (VIO_CHANNEL_DESC, tyDev));

    for (nBytes = 0; nBytes < maxBytes; nBytes++)
	{
        lockKey = intLock();
	if (wdbIsNowExternal())
	    intCnt++;		/* fake an ISR context */

	if (tyIRd (pTyDev, pData[nBytes]) == ERROR)
	    {
	    intUnlock (lockKey);
	    break;
	    }

	if (wdbIsNowExternal())
	    intCnt--;		/* fake an ISR exit */
	intUnlock (lockKey);
	}

    return (nBytes);
    }


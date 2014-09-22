/* wdbUlipPktDrv.c - WDB communication interface for the ULIP driver */

/* Copyright 1994-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,18oct01,jhw  Fixed documentation build errors.
01f,02dec96,dvs changed u_write to WriteUlip of SIMSPARCSOLARIS
01e,09nov96,pr  added simsolaris support.
01d,14oct95,jdi  doc: cleanup.
01c,15aug95,ms  locked ints around u_read. Added wdbUlipRcvFlush.
01b,27jun95,ms	moved from BSP. wdbUlipPktDevInit now takes a device name.
01a,06oct94,ms  written.
*/

/*
DESCRIPTION
This is a lightweight ULIP driver that interfaces with the WDB agent's
UDP/IP interpreter.  It is the lightweight equivalent of the ULIP netif
driver.  This module provides a communication path which supports
both a task mode and an external mode WDB agent.
*/

#include "vxWorks.h"

#if (CPU_FAMILY==SIMSPARCSUNOS) || (CPU_FAMILY==SIMHPPA) || (CPU_FAMILY==SIMSPARCSOLARIS)

#include "simLib.h"
#include "u_Lib.h"
#include "ioLib.h"
#include "intLib.h"
#include "iv.h"
#include "wdb/wdbCommIfLib.h"
#include "wdb/wdbMbufLib.h"
#include "drv/wdb/wdbUlipPktDrv.h"

/******************************************************************************
*
* wdbUlipPktFree - reset the receive packet state machine 
*/

static void wdbUlipPktFree
    (
    WDB_ULIP_PKT_DEV *	pDrv,
    caddr_t		buf
    )
    {
    pDrv->inBufLocked = FALSE;
    }

/******************************************************************************
*
* wdbUlipRcvFlush - flush ULIP receive bufer
*/ 

static void wdbUlipRcvFlush
    (
    int	ulipFd			/* ulip file desc */
    )
    {
    char tmpBuf [ULIP_MTU];
    int		nBytes;

    do
	{
	nBytes = u_read (ulipFd, tmpBuf, ULIP_MTU);
	}
    while (nBytes > 0);
    }

/******************************************************************************
*
* wdbUlipIntRcv - receive an input packet in INT mode
*/

static STATUS wdbUlipIntRcv
    (
    WDB_ULIP_PKT_DEV *	pDrv
    )
    {
    int		nBytes;
    struct mbuf * pMbuf;

    /* if input buffer is full, drop all ULIP packets */

    if (pDrv->inBufLocked || !(pMbuf = wdbMbufAlloc()))
	{
	wdbUlipRcvFlush (pDrv->ulipFd);
        return (0);
	}

    /* get the input packet */

    nBytes = u_read (pDrv->ulipFd, pDrv->inBuf, ULIP_MTU);

    /* flush the input queue to ensure the next SIGIO is sent properly */

    if (nBytes > 0)
	wdbUlipRcvFlush (pDrv->ulipFd);

    wdbMbufClusterInit (pMbuf, pDrv->inBuf, nBytes, (int (*)())wdbUlipPktFree,
			pDrv);

    if (nBytes <= 0)
	{
	wdbMbufFree (pMbuf);
	return (0);
	}

    pDrv->inBufLocked = TRUE;

    (*pDrv->wdbDrvIf.stackRcv) (pMbuf);

    return (nBytes);
    }

/******************************************************************************
*
* wdbUlipModeSet - Set the communication mode to POLL or INT.
*/

static int wdbUlipModeSet
    (
    WDB_ULIP_PKT_DEV *	pDrv,
    uint_t 		newMode
    )
    {
    return (OK);
    }

/******************************************************************************
*
* wdbUlipWrite - write to the ULIP device
*/

static int wdbUlipWrite
    (
    WDB_ULIP_PKT_DEV *	pDrv,
    struct mbuf * 	pMbuf
    )
    {
    struct mbuf *	pFirstMbuf = pMbuf;
    char		data [ULIP_MTU];
    int			len = 0;

    while (pMbuf != NULL)
	{
	bcopy (mtod (pMbuf, char *), &data[len], pMbuf->m_len);
	len += pMbuf->m_len;
	pMbuf = pMbuf->m_next;
	}

    wdbMbufChainFree (pFirstMbuf);
    
#if     ( CPU==SIMSPARCSOLARIS )
    return (WriteUlip (pDrv->ulipFd, data, len));
#else /* ( CPU==SIMSPARCSOLARIS ) */
    return (u_write (pDrv->ulipFd, data, len));
#endif /* ( CPU==SIMSPARCSOLARIS ) */
    }

/******************************************************************************
*
* wdbUlipPktDevInit - initialize the communication functions for ULIP
*
* This routine initializes a ULIP device for use by the WDB debug agent.
* It provides a communication path to the debug agent which can be
* used with both a task and an external mode agent.
* It is typically called by usrWdb.c when the WDB agent's lightweight
* ULIP communication path is selected.
*
* RETURNS: N/A
*/

void wdbUlipPktDevInit
    (
    WDB_ULIP_PKT_DEV * pDev,	/* ULIP packet device to initialize */
    char * ulipDev,		/* name of UNIX device to use */
    void (*stackRcv)()		/* routine to call when a packet arrives */
    )
    {
    pDev->wdbDrvIf.mode		= WDB_COMM_MODE_POLL | WDB_COMM_MODE_INT;
    pDev->wdbDrvIf.mtu		= ULIP_MTU;
    pDev->wdbDrvIf.stackRcv	= stackRcv;
    pDev->wdbDrvIf.devId	= (void *)pDev;
    pDev->wdbDrvIf.pollRtn	= wdbUlipIntRcv;
    pDev->wdbDrvIf.pktTxRtn	= wdbUlipWrite;
    pDev->wdbDrvIf.modeSetRtn	= wdbUlipModeSet;

    pDev->inBufLocked 		= FALSE;
    pDev->ulipFd		= u_open (ulipDev, O_RDWR, 0);

    intConnect (FD_TO_IVEC (pDev->ulipFd), (void (*)())wdbUlipIntRcv,
		(int)pDev);

    s_fdint (pDev->ulipFd, 1);
    }

#endif	/* (CPU_FAMILY==SIMSPARCSUNOS) || (CPU_FAMILY==SIMHPPA) || (CPU_FAMILY==SIMSPARCSOLARIS) */


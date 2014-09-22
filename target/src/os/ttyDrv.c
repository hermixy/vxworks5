/* ttyDrv.c - provide terminal device access to serial channels */

/* Copyright 1984-1995 Wind River systems, Inc. */

/*
modification history
--------------------
01h,19mar99,cn  added check on pSioChan in ttyDevCreate () (SPR# 25839).
01g,06may97,db  added ttyClose and modified ttyOpen to keep track of count of
		open paths to device and provide HUPCL modem control. Added
		arg flags to ttyOpen for compatibility with iosOpen(SPR #7637).
01f,19nov96,ms  FIOBAUDRATE ioctl now returns OK or ERROR (SPR 5487).
01e,29oct96,dgp doc: correct spelling per SPR 5904
01d,20jun95,ms	fixed comments for mangen
01c,15jun95,ms	updated for new serial driver
01b,21feb95,ms	revised to be generic.
01a,29dec94,ms	written.
*/

/*
DESCRIPTION
This library provides the OS-dependent functionality of a serial device,
including canonical processing and the interface to the VxWorks I/O system.

The BSP provides "raw" serial channels which are accessed
via an SIO_CHAN data structure. These raw devices provide only low
level access to the devices to send and receive characters.
This library builds on that functionality by allowing the
serial channels to be accessed via the VxWorks I/O system using
the standard read/write interface. It also provides the canonical
processing support of tyLib.

The routines in this library are typically called by usrRoot()
in usrConfig.c to create VxWorks serial devices at system startup time.

INCLUDE FILES: ttyLib.h

SEE ALSO: tyLib, sioLib.h
*/

#include "vxWorks.h"
#include "iv.h"
#include "ioLib.h"
#include "iosLib.h"
#include "tyLib.h"
#include "intLib.h"
#include "errnoLib.h"
#include "sioLib.h"
#include "stdlib.h"

/* data types */

typedef struct 	/* TYCO_DEV */
    {
    TY_DEV	tyDev;
    SIO_CHAN *	pSioChan;
    } TYCO_DEV;

/* local variables */

static int ttyDrvNum;           /* driver number assigned to this driver */

/* forward declarations */

LOCAL int    ttyOpen ();
LOCAL int    ttyClose (TYCO_DEV *  pTyCoDev); 
LOCAL STATUS ttyIoctl ();
LOCAL void   ttyStartup ();

/*******************************************************************************
*
* ttyDrv - initialize the tty driver
*
* This routine initializes the tty driver, which is the OS interface
* to core serial channel(s). Normally, it is called by usrRoot()
* in usrConfig.c.
*
* After this routine is called, ttyDevCreate() is typically called
* to bind serial channels to VxWorks devices.
*
* RETURNS: OK, or ERROR if the driver cannot be installed.
*/

STATUS ttyDrv (void)

    {
    /* check if driver already installed */

    if (ttyDrvNum > 0)
        return (OK);

    ttyDrvNum = iosDrvInstall (ttyOpen, (FUNCPTR) NULL, ttyOpen,
                                ttyClose, tyRead, tyWrite, ttyIoctl);

    return (ttyDrvNum == ERROR ? ERROR : OK);
    }

/*******************************************************************************
*
* ttyDevCreate - create a VxWorks device for a serial channel
*
* This routine creates a device on a specified serial channel.  Each channel
* to be used should have exactly one device associated with it by calling
* this routine.
*
* For instance, to create the device "/tyCo/0", with buffer sizes of 512 bytes,
* the proper call would be:
* .CS
*     ttyDevCreate ("/tyCo/0", pSioChan, 512, 512);
* .CE
* Where pSioChan is the address of the underlying SIO_CHAN serial channel
* descriptor (defined in sioLib.h).
* This routine is typically called by usrRoot() in usrConfig.c
*
* RETURNS: OK, or ERROR if the driver is not installed, or the
* device already exists.
*/


STATUS ttyDevCreate
    (
    char *      name,           /* name to use for this device      */
    SIO_CHAN *	pSioChan,	/* pointer to core driver structure */
    int         rdBufSize,      /* read buffer size, in bytes       */
    int         wrtBufSize      /* write buffer size, in bytes      */
    )
    {
    TYCO_DEV *pTyCoDev;

    if (ttyDrvNum <= 0)
        {
        errnoSet (S_ioLib_NO_DRIVER);
        return (ERROR);
        }

    if (pSioChan == (SIO_CHAN *) ERROR)
	{
        return (ERROR);
	}

    /* allocate memory for the device */

    if ((pTyCoDev = (TYCO_DEV *) malloc (sizeof (TYCO_DEV))) == NULL)
        return (ERROR);

    /* initialize the ty descriptor */

    if (tyDevInit (&pTyCoDev->tyDev, rdBufSize, wrtBufSize,
                   (FUNCPTR) ttyStartup) != OK)
        {
	free (pTyCoDev);
        return (ERROR);
        }

    /* initialize the SIO_CHAN structure */

    pTyCoDev->pSioChan	= pSioChan;
    sioCallbackInstall (pSioChan, SIO_CALLBACK_GET_TX_CHAR,
			tyITx, (void *)pTyCoDev);
    sioCallbackInstall (pSioChan, SIO_CALLBACK_PUT_RCV_CHAR,
			tyIRd, (void *)pTyCoDev);

    /* start the device cranking */

    sioIoctl (pSioChan, SIO_MODE_SET, (void *)SIO_MODE_INT);

    /* add the device to the I/O system */

    return (iosDevAdd (&pTyCoDev->tyDev.devHdr, name, ttyDrvNum));
    }

/*******************************************************************************
*
* ttyOpen - open a ttyDrv serial device.
*
* Increments a counter that holds the number of open paths to device. 
*/

LOCAL int ttyOpen
    (
    TYCO_DEV *	pTyCoDev,	/* device to control */
    char     *	name,		/* device name */
    int		flags,		/* flags */
    int        	mode		/* mode selected */
    )
    {
    pTyCoDev->tyDev.numOpen++;  /* increment number of open paths */
    sioIoctl (pTyCoDev->pSioChan, SIO_OPEN, NULL);
    return ((int) pTyCoDev);
    }

/*******************************************************************************
*
* ttyClose - close a ttyDrv serial device.
*
* Decrements the counter of open paths to device and alerts the driver 
* with an ioctl call when the count reaches zero. This scheme is used to
* implement the HUPCL(hang up on last close).      
*/

LOCAL int ttyClose
    (
    TYCO_DEV *	pTyCoDev	/* device to control */
    )
    {

    if (!(--pTyCoDev->tyDev.numOpen))
	sioIoctl (pTyCoDev->pSioChan, SIO_HUP, NULL);
    return ((int) pTyCoDev);
    }

/*******************************************************************************
*
* ttyIoctl - special device control
*
* RETURNS: depends on the function invoked.
*/

LOCAL int ttyIoctl
    (
    TYCO_DEV *	pTyCoDev,	/* device to control */
    int		request,	/* request code */
    void *	arg		/* some argument */
    )
    {
    int status;

    if (request == FIOBAUDRATE)
	return (sioIoctl (pTyCoDev->pSioChan, SIO_BAUD_SET, arg) == OK ?
		OK : ERROR);

    status = sioIoctl (pTyCoDev->pSioChan, request, arg);

    if (status == ENOSYS)
	return (tyIoctl (&pTyCoDev->tyDev, request, (int)arg));

    return (status);
    }

/*******************************************************************************
*
* ttyStartup - transmitter startup routine
*
* Call interrupt level character output routine.
*/

LOCAL void ttyStartup
    (
    TYCO_DEV *pTyCoDev          /* ty device to start up */
    )
    {
    sioTxStartup (pTyCoDev->pSioChan);
    }


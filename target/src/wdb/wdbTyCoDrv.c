/* wdbTyCoDrv.c - WDB agent interface over a VxWorks TTY device */

/* Copyright 1984-1996 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,23jan96,tpr  added casts to compile with Diab Data tools.
01b,15jun95,ms	 updated for new serial driver structure.
01a,21dec94,ms   written.
*/

/*
DESCRIPTION
This library uses a VxWorks tyCo device to emulate a raw SIO_CHAN.
*/

#include "vxWorks.h"
#include "ioLib.h"
#include "sioLib.h"
#include "wdb/wdbCommIfLib.h"
#include "wdb/wdbTyCoDrv.h"
#include "stdio.h"

/* locals */

static SIO_DRV_FUNCS wdbTyCoDrvFuncs;

/******************************************************************************
*
* unimp - unimplemented function.
*/ 

static int unimp (void)
    {
    return (ENOSYS);
    }

/******************************************************************************
*
* wdbTyCoIoctl - 
*
* RETURNS: OK on success, EIO on device error, ENOSYS on unsupported
*          request.
*/ 

static int wdbTyCoIoctl
    (
    SIO_CHAN *	pSioChan,
    int		cmd,
    void *	arg
    )
    {
    switch (cmd)
	{
	case SIO_MODE_GET:
	case SIO_AVAIL_MODES_GET:
	    *(int *)arg = SIO_MODE_INT;
	    return (OK);

	case SIO_MODE_SET: 
	    return (arg == (void *)SIO_MODE_INT ? OK : EIO);

	case SIO_BAUD_SET:
	    return (ioctl (((WDB_TYCO_SIO_CHAN *)pSioChan)->fd, FIOBAUDRATE,
			(int)arg) == OK ? OK : EIO);

	case SIO_BAUD_GET:
	case SIO_HW_OPTS_SET:
	case SIO_HW_OPTS_GET:
	    /* should be supported but can't be for tyCoDrv */
	default:
	    return (ENOSYS);
	}
    }

/******************************************************************************
*
* wdbTyCoStartup - start transmitting over the serial device.
*/

static int wdbTyCoStartup
    (
    SIO_CHAN *	pSioChan
    )
    {
    char thisChar;
    WDB_TYCO_SIO_CHAN * pChan = (WDB_TYCO_SIO_CHAN *)pSioChan;

    while ((*pChan->getTxChar) (pChan->getTxArg, &thisChar) != ERROR)
	write (((WDB_TYCO_SIO_CHAN *)pSioChan)->fd, &thisChar, 1);

    return (OK);
    }

/******************************************************************************
*
* wdbTyCoCallbackInstall - install callbacks.
*/ 

static int wdbTyCoCallbackInstall
    (
    SIO_CHAN *	pSioChan,
    int		callbackType,
    STATUS	(*callback)(),
    void *	callbackArg
    )
    {
    WDB_TYCO_SIO_CHAN * pChan = (WDB_TYCO_SIO_CHAN *)pSioChan;

    switch (callbackType)
	{
	case SIO_CALLBACK_GET_TX_CHAR:
	    pChan->getTxChar	= callback;
	    pChan->getTxArg	= callbackArg;
	    return (OK);
	case SIO_CALLBACK_PUT_RCV_CHAR:
	    pChan->putRcvChar	= callback;
	    pChan->putRcvArg	= callbackArg;
	    return (OK);
	default:
	    return (ENOSYS);
	}
    }

/******************************************************************************
*
* wdbTyCoRcv - receive a character over the serial channel.
*/

static BOOL wdbTyCoRcv
    (
    SIO_CHAN *	pSioChan,
    int 	inChar
    )
    {
    char thisChar;
    WDB_TYCO_SIO_CHAN * pChan = (WDB_TYCO_SIO_CHAN *)pSioChan;

    thisChar = inChar & 0xff;

    (*pChan->putRcvChar)(pChan->putRcvArg, thisChar);

    return (TRUE);		/* no more processing by the tyCo device */
    }

/******************************************************************************
*
* wdbTyCoDevInit - initialize a tyCo SIO_CHAN
*/

STATUS wdbTyCoDevInit
    (
    WDB_TYCO_SIO_CHAN *pChan,	/* channel structure to initialize */
    char *	device,		/* name of underlying tyCo device to use */
    int		baudRate	/* baud rate */
    )
    {
    /* make sure driver functions are initialized */

    if (wdbTyCoDrvFuncs.ioctl == NULL)
	{
	wdbTyCoDrvFuncs.ioctl		= wdbTyCoIoctl;
	wdbTyCoDrvFuncs.txStartup	= wdbTyCoStartup;
	wdbTyCoDrvFuncs.callbackInstall	= wdbTyCoCallbackInstall;
	wdbTyCoDrvFuncs.pollInput	= (int (*) (SIO_CHAN *, char *)) unimp;
	wdbTyCoDrvFuncs.pollOutput	= (int (*) (SIO_CHAN *, char ))  unimp;
	}

    /* open and initialize the tyCoDev device */

    pChan->fd = open (device, O_RDWR, 0);

    if (pChan->fd <= 0)
	{
	printErr ("wdbTyCoDrv: open of %s failed\n", device);
	return (ERROR);
	}

    if (ioctl (pChan->fd, FIOISATTY, 0) != TRUE)
	{
	printErr ("%s is not a TTY device\n");
	close (pChan->fd);
	return (ERROR);
	}

    if (ioctl (pChan->fd, FIOBAUDRATE, baudRate) == ERROR)
	{
	printErr ("wdbTyCoDrv: seting baud rate to %d failed\n", baudRate);
	close (pChan->fd);
	return (ERROR);
	}

    if (ioctl (pChan->fd, FIOFLUSH, 0)  == ERROR ||
        ioctl (pChan->fd, FIOOPTIONS, OPT_RAW)  == ERROR ||
        ioctl (pChan->fd, FIOPROTOHOOK, (int)wdbTyCoRcv) == ERROR ||
        ioctl (pChan->fd, FIOPROTOARG, (int)pChan) == ERROR)
	{
	printErr ("wdbTyCoDrv: ioctl failed\n");
	close (pChan->fd);
	return (ERROR);
	}

    /* initialize the SIO_CHAN structure */

    pChan->pDrvFuncs	= &wdbTyCoDrvFuncs;

    return (OK);
    }


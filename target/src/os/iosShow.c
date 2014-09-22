/* iosShow.c - I/O system show routines */

/* Copyright 1984-1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,10oct95,jdi  doc: added .tG Shell to SEE ALSOs.
01c,02feb93,jdi  documentation tweaks.
01b,02oct92,jdi  documentation cleanup.
01a,23aug92,jcf  extracted from iosLib.c.
*/

/*
DESCRIPTION
This library contains I/O system information display routines.

The routine iosShowInit() links the I/O system information show
facility into the VxWorks system.  It is called automatically when
\%INCLUDE_SHOW_ROUTINES is defined in configAll.h.

SEE ALSO: intLib, ioLib,
.pG "I/O System,"
windsh,
.tG "Shell"
*/

#include "vxWorks.h"
#include "dllLib.h"
#include "fioLib.h"
#include "stdio.h"
#include "private/iosLibP.h"


/******************************************************************************
*
* iosShowInit - initialize the I/O system show facility
*
* This routine links the I/O system show facility into the VxWorks system.
* It is called automatically when \%INCLUDE_SHOW_ROUTINES is defined in
* configAll.h.
*
* RETURNS: N/A
*/

void iosShowInit (void)
    {
    }

/*******************************************************************************
*
* iosDrvShow - display a list of system drivers
*
* This routine displays a list of all drivers in the driver list.
*
* RETURNS: N/A
*
* SEE ALSO:
* .pG "I/O System,"
* windsh,
* .tG "Shell"
*/

void iosDrvShow (void)
    {
    FAST int i;

    printf ("%3s %9s  %9s  %9s  %9s  %9s  %9s  %9s\n",
	"drv", "create", "delete", "open", "close", "read", "write", "ioctl");

    for (i = 1; i < maxDrivers; i++)
	{
	if (drvTable[i].de_inuse)
	    {
	    printf ("%3d %9x  %9x  %9x  %9x  %9x  %9x  %9x\n", i,
		drvTable[i].de_create, drvTable[i].de_delete,
		drvTable[i].de_open, drvTable[i].de_close,
		drvTable[i].de_read, drvTable[i].de_write,
		drvTable[i].de_ioctl);
	    }
	}
    }
/*******************************************************************************
*
* iosDevShow - display the list of devices in the system
*
* This routine displays a list of all devices in the device list.
*
* RETURNS: N/A
*
* SEE ALSO: devs(),
* .pG "I/O System,"
* windsh,
* .tG "Shell"
*/

void iosDevShow (void)
    {
    FAST DEV_HDR *pDevHdr;

    printf ("%3s %-20s\n", "drv", "name");

    for (pDevHdr = (DEV_HDR *) DLL_FIRST (&iosDvList); pDevHdr != NULL;
				pDevHdr = (DEV_HDR *) DLL_NEXT (&pDevHdr->node))
	printf ("%3d %-20s\n", pDevHdr->drvNum, pDevHdr->name);
    }
/*******************************************************************************
*
* iosFdShow - display a list of file descriptor names in the system
*
* This routine displays a list of all file descriptors in the system.
*
* RETURNS: N/A
*
* SEE ALSO: ioctl(),
* .pG "I/O System,"
* windsh,
* .tG "Shell"
*/

void iosFdShow (void)
    {
    char *stin;
    char *stout;
    char *sterr;
    FD_ENTRY *pFdEntry;
    int fd;
    int xfd;

    printf ("%3s %-20s %3s\n", "fd", "name", "drv");

    for (fd = 0; fd < maxFiles; fd++)
	{
	pFdEntry = &fdTable [fd];
	if (pFdEntry->inuse)
	    {
	    xfd = STD_FIX(fd);

	    stin  = (xfd == ioGlobalStdGet (STD_IN))  ? "in"  : "";
	    stout = (xfd == ioGlobalStdGet (STD_OUT)) ? "out" : "";
	    sterr = (xfd == ioGlobalStdGet (STD_ERR)) ? "err" : "";

	    printf ("%3d %-20s %3d %s %s %s\n",
		    xfd,
		    (pFdEntry->name == NULL) ? "(unknown)" : pFdEntry->name,
		    pFdEntry->pDevHdr->drvNum, stin, stout, sterr);
	    }
	}
    }

/* usrFd.c - floppy disk initialization */

/* Copyright 1992-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,21sep01,jkf  cbio API changes.
01g,21jun00,rsh  upgrade to dosFs 2.0
01f,04nov98,lrn  fixed return value checking for dosFsDevCreate()
01e,14oct98,lrn  modified for DosFs 2.0

01d,28jun95,hdn  doc change.
01c,24jan95,jdi  doc cleanup.
01b,25oct94,hdn  swapped 1st and 2nd parameter of fdDevCreate() and
		 usrFdConfig().
01a,25oct93,hdn  written.
*/

/*
DESCRIPTION
This file is used to configure and initialize the VxWorks floppy disk support.
This file is included by the prjConfig.c configuration file created by thge Project Manager.

NOMANUAL
*/

#include "vxWorks.h"
#include "dosFsLib.h"
#include "dpartCbio.h"
#include "dcacheCbio.h"
#include "usrFdiskPartLib.h"

/* forward declaration */

/* macro's */
#ifndef FD_CACHE_SIZE
#define FD_CACHE_SIZE 0x0
#endif /* !FD_CACHE_SIZE */


/*******************************************************************************
*
* usrFdConfig - mount a DOS file system from a floppy disk
*
* This routine mounts a DOS file system from a floppy disk device.
*
* The <drive> parameter is the drive number of the floppy disk;
* valid values are 0 to 3.
*
* The <type> parameter specifies the type of diskette, which is described
* in the structure table `fdTypes[]' in sysLib.c.  <type> is an index to
* the table.  Currently the table contains two diskette types:
* .iP "" 4
* A <type> of 0 indicates the first entry in the table (3.5" 2HD, 1.44MB);
* .iP
* A <type> of 1 indicates the second entry in the table (5.25" 2HD, 1.2MB).
* .LP
*
* The <fileName> parameter is the mount point, e.g., `/fd0/'.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* .pG "I/O System, Local File Systems, Intel i386/i486 Appendix"
*/

STATUS usrFdConfig
    (
    int     drive,	/* drive number of floppy disk (0 - 3) */
    int     type,	/* type of floppy disk */
    char *  fileName	/* mount point */
    )
    {
    BLK_DEV *pBootDev;
    CBIO_DEV_ID cbio ;
    char bootDir [BOOT_FILE_LEN];

    if( type == NONE)
	return OK;

    if ((UINT)drive >= FD_MAX_DRIVES)
	{
	printErr ("drive is out of range (0-%d).\n", FD_MAX_DRIVES - 1);
	return (ERROR);
	}

    /* create a block device spanning entire disk (non-distructive!) */

    if ((pBootDev = fdDevCreate (drive, type, 0, 0)) == NULL)
	{
        printErr ("fdDevCreate failed.\n");
        return (ERROR);
	}

    /* create a disk cache to speed up Floppy operation */
    cbio = dcacheDevCreate( (CBIO_DEV_ID) pBootDev, NULL, 
                           FD_CACHE_SIZE, bootDir );

    if( cbio == NULL )
	{
	/* insufficient memory, will avoid the cache */
	cbio = cbioWrapBlkDev (pBootDev);
	}

    /* split off boot device from boot file */

    devSplit (fileName, bootDir);

    /* initialize device as a dosFs device named <bootDir> */

    if (dosFsDevCreate (bootDir, cbio, 20, NONE) == ERROR)
	{
        printErr ("dosFsDevCreate failed.\n");
        return (ERROR);
	}

    return (OK);
    }


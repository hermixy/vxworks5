/* usrAta.c - ATA/ATAPI initialization */

/* Copyright 1992-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,02mar02,jkf  SPR#73841, fixed (cdrom) trailing paran indexing error.
                 fixed warning: pn unitialized if INCLUDE_CDROMFS.
01k,26nov01,jac  added file system type check to allow support for specifying
                 CDROM filesystem.
01j,02ocy01,jkf  fixing SPR#70377, usrAtaConfig altered char * arg, this 
                 modifies .text seg and caused checksum error for tgtsvr. 
01i,21sep01,jkf  cbio API changes.
01h,21jun00,rsh  upgrade to dosFs 2.0
01g,14oct98,lrn  modified for DosFs 2.0
01f,30jul99,jkf  SPR#4429.
01e,02jun98,ms   created configlette.
01d,30oct97,db   added call to ATA_SWAP in usrAtaPartition. 
01c,30oct97,dat  added #include pccardLib.h and ataDrv.h
01b,14jul97,dgp  doc: update to match hard-copy
01a,11may95,hdn  re-written for ATA driver.
*/

/*
DESCRIPTION
This file is used to configure and initialize the VxWorks ATA support.
This file is included by the prjConfig.c configuration file created by the
Project Manager.


SEE ALSO: usrExtra.c

NOMANUAL
*/

#include "vxWorks.h"
#include "string.h"
#include "dosFsLib.h"
#include "dcacheCbio.h"
#include "dpartCbio.h"
#include "usrFdiskPartLib.h"
#include "drv/hdisk/ataDrv.h"

#ifdef INCLUDE_CDROMFS
#include "cdromFsLib.h"
#endif

/* forward declaration */

/* macro's */

#ifndef ATA_CACHE_SIZE
#define ATA_CACHE_SIZE 0x0
#endif /* !ATA_CACHE_SIZE */

/*******************************************************************************
*
* usrAtaConfig - mount a DOS file system from an ATA hard disk or a CDROM
*                file system from an ATAPI CDROM drive
*
* This routine mounts a DOS file system from an ATA hard disk. Parameters:
*
* .IP <drive> 
* the drive number of the hard disk; 0 is `C:' and 1 is `D:'.
* .IP <devName>
* the mount point for all partitions which are expected to be present
* on the disk, separated with commas, for example "/ata0,/ata1" or "C:,D:".
* Blanks are not allowed in this string.  If the drive is an ATAPI CDROM
* drive, then the CDROM filesystem is specified by appending "(cdrom)"
* after the mount point name.  For example, a CDROM drive could be specified
* as "/cd(cdrom)".
* .LP
*
* NOTE: Because VxWorks does not support creation of partition tables,
* hard disks formatted
* and initialized on VxWorks are not compatible with DOS machines.  This
* routine does not refuse to mount a hard disk that was initialized on
* VxWorks. Up to 8 disk partitions are supported.
*
*
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* `src/config/usrAta.c',
* .pG "I/O System, Local File Systems, Intel i386/i486/Pentium"
*/

STATUS usrAtaConfig
    (
    int     ctrl,	 /* 0: primary address, 1: secondary address */
    int     drive,	 /* drive number of hard disk (0 or 1) */
    char    *devNames    /* mount points for each partition */
    )
    {
    BLK_DEV *pBlkDev;
    CBIO_DEV_ID cbio, masterCbio;
    STATUS  stat = OK;
    char    *tmp; 
    char    *devNamesCopy;
    char    *freePtr;
    char    *sysType;
    char    *devName[PART_MAX_ENTRIES];
    int     pn; 
    int     numPart = 0;

    /* check argument sanity */

    if( NULL == devNames || EOS == *devNames )
        {
	printErr ("usrAtaConfig: Invalid device name.\n");

	return (ERROR);
        }

    if (ATA_MAX_DRIVES <= (UINT) drive)
	{
	printErr ("usrAtaConfig: drive is out of range (0-%d).\n", 
                  ATA_MAX_DRIVES - 1);

	return (ERROR);
	}

    if (ATA_MAX_CTRLS <= (UINT) ctrl)
        {
        printErr ("usrAtaConfig: controller is out of range (0-%d).\n",
                  ATA_MAX_CTRLS - 1);

        return (ERROR);
        }

    /* 
     * make private copy of the devNames, SPR#70337 
     * strlen does not count EOS, so we add 1 to malloc.
     */

    devNamesCopy = malloc (1 + (int) (strlen (devNames)));   

    /* ensure malloc suceeded    */

    if (NULL == devNamesCopy)       
        {
	printErr ("usrAtaConfig: malloc returned NULL\n");

        return (ERROR); 
        }

    /* store the pointer for free, since devNamesCopy is modified */
   
    freePtr = devNamesCopy;

    /* copy string, include EOS  */

    strcpy (devNamesCopy, devNames); 
    
    /* Check for file system spec */

    sysType = index (devNamesCopy, '(');

    if (sysType != NULL)
        {
        *sysType = '\0';
        sysType++;
        tmp = index( sysType, ')' );

        if (tmp != NULL)
            {
            *tmp = '\0';
            }
	}
    else
        {
        sysType = "dos";
        }

    /* Parse the partition device name string */

    for (numPart = 0; numPart < PART_MAX_ENTRIES; numPart++)
	{

	if (EOS == *devNamesCopy)
	    break;

	tmp = devNamesCopy ;
	devName[numPart] = devNamesCopy ;
	tmp = index (tmp, ',');

	if (NULL == tmp)
	    {
	    numPart++;

	    break;
	    }

	*tmp = EOS ;
	devNamesCopy = tmp+1;
	}

    /* create block device for the entire disk, */

    if ((pBlkDev = ataDevCreate (ctrl, drive, 0, 0)) == (BLK_DEV *) NULL)
        {
        printErr ("Error during ataDevCreate: %x\n", errno);
        free (freePtr);

        return (ERROR);
        }

    if (strcmp (sysType, "dos") == 0)
        {

        /* create disk cache for the entire drive */

        cbio = dcacheDevCreate ((CBIO_DEV_ID) pBlkDev, NULL, ATA_CACHE_SIZE, freePtr);

        if (NULL == cbio)
            {
            /* insufficient memory, will avoid the cache */

            printErr ("WARNING: Failed to create %d bytes of disk cache"
                      " ATA disk %s configured without cache\n",
                      ATA_CACHE_SIZE, devNames);
            cbio = cbioWrapBlkDev (pBlkDev);
            }

        /* create partition manager */

        masterCbio = dpartDevCreate (cbio, numPart, usrFdiskPartRead);

        if (NULL == masterCbio)
            {
            printErr ("Error creating partition manager: %x\n", errno);
            free (freePtr);

            return (ERROR);
            }

        /* Create a DosFs device for each partition required */

        for (pn = 0; pn < numPart; pn ++)
            {
            stat = dosFsDevCreate (devName[pn], dpartPartGet (masterCbio, pn),
                                   NUM_DOSFS_FILES, NONE);

            if (stat == ERROR)
                {
                printErr ("Error creating dosFs device %s, errno=%x\n",
                          devName[pn], errno);
                free (freePtr);

                return (ERROR);
                }
            }
        }
#ifdef INCLUDE_CDROMFS

    else if (strcmp (sysType, "cdrom") == 0)
        {

        if (cdromFsDevCreate (devName[0], pBlkDev) == NULL)
            {
            printErr ("Error creating cdromFs device %s, errno=%x\n",
                      devName[0], errno);

            free (freePtr);

            return (ERROR);
            }
        }
#endif

    else
        {
        printErr ("Unknown or un-included filesystem type: \"%s\"\n", sysType);
        free (freePtr);

        return (ERROR);
        }

    free (freePtr);

    return (OK);
    }

/******************************************************************************
*
* usrAtaInit - intialize the hard disk driver
*
* This routine is called from usrConfig.c to initialize the hard drive.
*/

void usrAtaInit (void)
    {
    int ix;
    ATA_RESOURCE *pAtaResource;

    for (ix = 0; ix < ATA_MAX_CTRLS; ix++)
        {
        pAtaResource = &ataResources[ix];

        if (pAtaResource->ctrlType == IDE_LOCAL)

            if ((ataDrv (ix, pAtaResource->drives, pAtaResource->intVector,
                   pAtaResource->intLevel, pAtaResource->configType,
                   pAtaResource->semTimeout, pAtaResource->wdgTimeout))
               == ERROR)
                {
                printf ("ataDrv returned ERROR from usrRoot.\n");
                }
        }
    }


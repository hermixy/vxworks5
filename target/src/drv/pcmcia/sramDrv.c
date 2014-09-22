/* sramDrv.c - PCMCIA SRAM device driver */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,21jun00,rsh  upgrade to dosFs 2.0
01d,16jan97,hdn  added pCtrl->memBase.
01c,28mar96,jdi  doc: cleaned up language and format.
01b,22feb96,hdn  cleaned up
01a,28feb95,hdn  written based on memDrv.c and ramDrv.c.
*/

/*
DESCRIPTION
This is a device driver for the SRAM PC card.  The memory location and size
are specified when the "disk" is created.

USER-CALLABLE ROUTINES
Most of the routines in this driver are accessible only through the I/O
system.  However, two routines must be called directly:  sramDrv() to
initialize the driver, and sramDevCreate() to create block devices.
Additionally, the sramMap() routine is called directly to map the PCMCIA
memory onto the ISA address space.  Note that this routine does not use
any mutual exclusion or synchronization mechanism; thus, special care must
be taken in the multitasking environment.

Before using this driver, it must be initialized by calling sramDrv().  This
routine should be called only once, before any reads, writes, or calls to
sramDevCreate() or sramMap().  It can be called from usrRoot() in usrConfig.c
or at some later point.

SEE ALSO:
.pG "I/O System"

LINTLIBRARY
*/

#include "vxWorks.h"
#include "ioLib.h"
#include "memLib.h"
#include "errnoLib.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "sysLib.h"
#include "logLib.h"
#include "private/semLibP.h"
#include "drv/pcmcia/pcmciaLib.h"
#include "drv/pcmcia/sramDrv.h"


/* defines */

#define	SRAM_WINDOW	1


/* imports */

IMPORT PCMCIA_CTRL	pcmciaCtrl;
IMPORT SRAM_RESOURCE	sramResources[];


/* globals */

SRAM_CTRL		sramCtrl;
int			sramResourceNumEnt;


/* locals */

LOCAL BOOL		sramDrvInstalled	= FALSE;
LOCAL PCMCIA_MEMWIN	sramMemwin		= {1,0,0,0,0,0};


/* forward declarations */

LOCAL STATUS sramRead		(SRAM_DEV *pSramDev, int startBlk, int numBlks, 
			 	 char *pChar);
LOCAL STATUS sramWrite		(SRAM_DEV *pSramDev, int startBlk, int numBlks,
			 	 char *pChar);
LOCAL STATUS sramIoctl		(SRAM_DEV *pSramDev, int function, int arg);
LOCAL STATUS sramStatusChk 	(SRAM_DEV *pSramDev);


/*******************************************************************************
*
* sramDrv - install a PCMCIA SRAM memory driver
*
* This routine initializes a PCMCIA SRAM memory driver.  It must be called once,
* before any other routines in the driver.
*
* RETURNS:
* OK, or ERROR if the I/O system cannot install the driver.
*/

STATUS sramDrv
    (
    int sock			/* socket no. */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    SEM_ID muteSemID		= &sramCtrl.muteSem[sock];
    SEM_ID syncSemID		= &sramCtrl.syncSem[sock];
    int ix;


    if ((!pChip->installed) || (!pCard->detected))
	return (ERROR);

    if (!sramDrvInstalled)
	{
	for (ix = 0; ix < MAX_SOCKETS; ix++)
	    {
            semBInit (&sramCtrl.syncSem[ix], SEM_Q_FIFO, SEM_EMPTY);
            semMInit (&sramCtrl.muteSem[ix], SEM_Q_PRIORITY |
		      SEM_DELETE_SAFE | SEM_INVERSION_SAFE);
	    }

        sramDrvInstalled = TRUE;
	}

    if (!pCard->installed)
	{
        semBInit (syncSemID, SEM_Q_FIFO, SEM_EMPTY);
        semMInit (muteSemID, SEM_Q_PRIORITY | SEM_DELETE_SAFE |
		  SEM_INVERSION_SAFE);
	}

    return (OK);
    }

/*******************************************************************************
*
* sramMap - map PCMCIA memory onto a specified ISA address space
*
* This routine maps PCMCIA memory onto a specified ISA address space.
*
* RETURNS:
* OK, or ERROR if the memory cannot be mapped.
*/

STATUS sramMap
    (
    int sock,			/* socket no. */
    int type,			/* 0: common  1: attribute */
    int start,			/* ISA start address */
    int stop,			/* ISA stop address */
    int offset,			/* card offset address */
    int extraws			/* extra wait state */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_MEMWIN memwin;


    if ((!pChip->installed) || (sock >= pChip->socks))
	return (ERROR);

    memwin.window	= SRAM_WINDOW;
    memwin.flags	= MAP_ACTIVE | MAP_16BIT;
    if (type == 1)
        memwin.flags	= MAP_ACTIVE | MAP_16BIT | MAP_ATTRIB;
    memwin.extraws	= extraws;
    memwin.start	= start;
    memwin.stop		= stop;
    memwin.cardstart	= offset & 0xfffff000;
    if ((* pChip->memwinSet)(sock, &memwin) != OK)
        return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* sramDevCreate - create a PCMCIA memory disk device
*
* This routine creates a PCMCIA memory disk device.
*
* RETURNS:
* A pointer to a block device structure (BLK_DEV), or NULL if memory cannot
* be allocated for the device structure.
*
* SEE ALSO: ramDevCreate()
*/

BLK_DEV *sramDevCreate
    (
    int sock,			/* socket no. */
    int	bytesPerBlk,		/* number of bytes per block */
    int	blksPerTrack,		/* number of blocks per track */
    int	nBlocks,		/* number of blocks on this device */
    int	blkOffset		/* no. of blks to skip at start of device */
    )
					 
    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_CARD *pCard		= &pCtrl->card[sock];
    SRAM_RESOURCE *pSram	= &sramResources[sock];
    PCCARD_RESOURCE *pResource	= &pSram->resource;
    SRAM_DEV	*pSramDev;	/* ptr to created SRAM_DEV struct */
    BLK_DEV	*pBlkDev;	/* ptr to BLK_DEV struct in SRAM_DEV */


    if ((!pChip->installed) || (!pCard->installed) ||
	(sock >= sramResourceNumEnt))
        return (NULL);

    /* Set up defaults for any values not specified */

    if (bytesPerBlk == 0)
	bytesPerBlk = DEFAULT_SEC_SIZE;

    if (nBlocks == 0)
	nBlocks = DEFAULT_DISK_SIZE / bytesPerBlk;

    if (blksPerTrack == 0)
	blksPerTrack = nBlocks;

    if ((bytesPerBlk * nBlocks) >= pResource->memLength)
        return (NULL);

    /* Allocate a SRAM_DEV structure for device */

    pSramDev = (SRAM_DEV *) malloc (sizeof (SRAM_DEV));
    if (pSramDev == NULL)
	return (NULL);					/* no memory */


    /* Initialize BLK_DEV structure (in SRAM_DEV) */

    pBlkDev = &pSramDev->blkDev;

    pBlkDev->bd_nBlocks      = nBlocks;		/* number of blocks */
    pBlkDev->bd_bytesPerBlk  = bytesPerBlk;	/* bytes per block */
    pBlkDev->bd_blksPerTrack = blksPerTrack;	/* blocks per track */

    pBlkDev->bd_nHeads       = 1;		/* one "head" */
    pBlkDev->bd_removable    = TRUE;		/* removable */
    pBlkDev->bd_retry	     = 1;		/* retry count */
    pBlkDev->bd_readyChanged = TRUE;		/* new ready status */
    pBlkDev->bd_mode	     = O_RDWR;		/* initial mode for device */
    pBlkDev->bd_blkRd	     = sramRead;	/* read block function */
    pBlkDev->bd_blkWrt	     = sramWrite;	/* write block function */
    pBlkDev->bd_ioctl	     = sramIoctl;	/* ioctl function */
    pBlkDev->bd_reset	     = NULL;		/* no reset function */
    pBlkDev->bd_statusChk    = sramStatusChk;	/* check-status function */
    pBlkDev->bd_statusChk    = NULL;		/* check-status function */


    /* Initialize remainder of device struct */

    pSramDev->blkOffset	= blkOffset;		/* block offset */
    pSramDev->sock	= sock;			/* socket no. */

    return (&pSramDev->blkDev);
    }

/*******************************************************************************
*
* sramRead - read one or more blocks from a PCMCIA memory disk volume
*
* This routine reads one or more blocks from the specified volume,
* starting with the specified block number.  The byte offset is
* calculated and the PCMCIA memory disk data is copied to the specified buffer.
*
* If any block offset was specified during sramDevCreate(), it is added
* to <startBlk> before the transfer takes place.
*
* RETURNS: OK, or ERROR if mapping or read fails.
*/

LOCAL STATUS sramRead
    (
    SRAM_DEV *pSramDev,		/* pointer to device desriptor */
    int      startBlk,		/* starting block number to read */
    int      numBlks,		/* number of blocks to read */
    char     *pChar		/* pointer to buffer to receive data */
    )
    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_CARD *pCard		= &pCtrl->card[pSramDev->sock];
    SRAM_RESOURCE *pSram	= &sramResources[pSramDev->sock];
    PCCARD_RESOURCE *pResource	= &pSram->resource;
    SEM_ID muteSemID		= &sramCtrl.muteSem[pSramDev->sock];
    SEM_ID syncSemID		= &sramCtrl.syncSem[pSramDev->sock];
    u_int bytesPerBlk		= pSramDev->blkDev.bd_bytesPerBlk;
    u_int windowSize		= pResource->memStop - pResource->memStart + 1;
    u_int nbytes		= bytesPerBlk * numBlks;
    u_int offset		= (u_int)pResource->memOffset + 
				  ((startBlk + pSramDev->blkOffset) * 
				  bytesPerBlk);
    u_int copiedBytes		= 0;
    u_int length;
    PCMCIA_MEMWIN memwin;


    /* sanity check */

    if ((!pCard->installed) ||
        ((block_t)(startBlk + numBlks) > pSramDev->blkDev.bd_nBlocks) ||
	(offset >= (u_int)pResource->memLength))
	return (ERROR);

#ifdef	SRAM_DEBUG
    printf ("sramRead: startBlk=%d numBlks=%d p=0x%x\n",
	    startBlk, numBlks, pChar);	
#endif

    if (pCard->cardStatus & PC_BATDEAD)
	{
	(void) errnoSet (S_pcmciaLib_BATTERY_DEAD);
	return (ERROR);
	}

    if (pCard->cardStatus & PC_BATWARN)
	{
	(void) errnoSet (S_pcmciaLib_BATTERY_WARNING);
	return (ERROR);
	}

    if (pCard->cardStatus & PC_WRPROT)
        pSramDev->blkDev.bd_mode = O_RDONLY;

    /* Read the block(s) */

    if (semTake (muteSemID, WAIT_FOREVER) == ERROR)
	return (ERROR);

    while (copiedBytes < nbytes)
	{
	memwin.window	= SRAM_WINDOW;
	memwin.flags	= MAP_ACTIVE | MAP_16BIT;
	memwin.extraws	= pResource->memExtraws;
	memwin.start	= pResource->memStart;
	memwin.stop	= pResource->memStop;
	memwin.cardstart= offset & 0xfffff000;
	if ((* pChip->memwinSet)(pSramDev->sock, &memwin) != OK)
	    {
	    semGive (muteSemID);
	    return (ERROR);
	    }

	if (((* pChip->status)(pSramDev->sock) & PC_READY) == 0)
	    if (semTake(syncSemID, sysClkRateGet()) != OK)
		{
		semGive (muteSemID);
		return (ERROR);
		}

	length = min (nbytes - copiedBytes, (windowSize - (offset & 0xfff)));
	bcopy ((char *)memwin.start + (offset & 0xfff) + pCtrl->memBase,
	       pChar, length);

	offset		+= length;
	copiedBytes 	+= length;
	pChar 		+= length;
	}

    if ((* pChip->memwinSet)(pSramDev->sock, &sramMemwin) != OK)
	{
	semGive (muteSemID);
	return (ERROR);
	}

    semGive (muteSemID);
    return (OK);
    }

/*******************************************************************************
*
* sramWrite - write one or more blocks to a PCMCIA memory disk volume
*
* This routine writes one or more blocks to the specified volume,
* starting with the specified block number.  The byte offset is
* calculated and the buffer data is copied to the PCMCIA memory disk.
*
* If any block offset was specified during sramDevCreate(), it is added
* to <startBlk> before the transfer takes place.
*
* RETURNS: OK, or ERROR if mapping or write fails.
*/

LOCAL STATUS sramWrite
    (
    SRAM_DEV *pSramDev,		/* pointer to device desriptor */
    int	     startBlk,		/* starting block number to write */
    int	     numBlks,		/* number of blocks to write */
    char     *pChar 		/* pointer to buffer of data to write */
    )
    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_CARD *pCard		= &pCtrl->card[pSramDev->sock];
    SRAM_RESOURCE *pSram	= &sramResources[pSramDev->sock];
    PCCARD_RESOURCE *pResource	= &pSram->resource;
    SEM_ID muteSemID		= &sramCtrl.muteSem[pSramDev->sock];
    SEM_ID syncSemID		= &sramCtrl.syncSem[pSramDev->sock];
    u_int bytesPerBlk		= pSramDev->blkDev.bd_bytesPerBlk;
    u_int windowSize		= pResource->memStop - pResource->memStart + 1;
    u_int nbytes		= bytesPerBlk * numBlks;
    u_int offset		= (u_int)pResource->memOffset + 
				  ((startBlk + pSramDev->blkOffset) *
				  bytesPerBlk);
    u_int copiedBytes		= 0;
    u_int length;
    PCMCIA_MEMWIN memwin;


    /* sanity check */

    if ((!pCard->installed) ||
        ((block_t)(startBlk + numBlks) > pSramDev->blkDev.bd_nBlocks) ||
	(offset >= (u_int)pResource->memLength))
	return (ERROR);

#ifdef	SRAM_DEBUG
    printf ("sramWrite: startBlk=%d numBlks=%d p=0x%x\n",
	    startBlk, numBlks, pChar);	
#endif

    if (pCard->cardStatus & PC_BATDEAD)
	{
	(void) errnoSet (S_pcmciaLib_BATTERY_DEAD);
	return (ERROR);
	}

    if (pCard->cardStatus & PC_BATWARN)
	{
	(void) errnoSet (S_pcmciaLib_BATTERY_WARNING);
	return (ERROR);
	}

    if (pCard->cardStatus & PC_WRPROT)
	{
	(void) errnoSet (S_ioLib_WRITE_PROTECTED);
        pSramDev->blkDev.bd_mode = O_RDONLY;
	return (ERROR);
	}

    /* Write the block(s) */

    if (semTake (muteSemID, WAIT_FOREVER) == ERROR)
	return (ERROR);

    while (copiedBytes < nbytes)
	{
	memwin.window	= SRAM_WINDOW;
	memwin.flags	= MAP_ACTIVE | MAP_16BIT;
	memwin.extraws	= pResource->memExtraws;
	memwin.start	= pResource->memStart;
	memwin.stop	= pResource->memStop;
	memwin.cardstart= offset & 0xfffff000;
	if ((* pChip->memwinSet)(pSramDev->sock, &memwin) != OK)
	    {
	    semGive (muteSemID);
	    return (ERROR);
	    }

	if (((* pChip->status)(pSramDev->sock) & PC_READY) == 0)
	    if (semTake(syncSemID, sysClkRateGet()) != OK)
		{
		semGive (muteSemID);
		return (ERROR);
		}

	length = min (nbytes - copiedBytes, (windowSize - (offset & 0xfff)));
	bcopy (pChar, (char *)memwin.start + (offset & 0xfff) + pCtrl->memBase,
	       length);

	offset		+= length;
	copiedBytes 	+= length;
	pChar 		+= length;
	}

    if ((* pChip->memwinSet)(pSramDev->sock, &sramMemwin) != OK)
	{
	semGive (muteSemID);
	return (ERROR);
	}

    semGive (muteSemID);
    return (OK);
    }

/*******************************************************************************
*
* sramIoctl - do device specific control function
*
* This routine is called when the file system cannot handle an ioctl()
* function.
*
* The FIODISKFORMAT function always returns OK, since a PCMCIA memory disk does
* not require formatting.  All other requests return ERROR.
*
* RETURNS:  OK, or ERROR if there is an error.
*
* ARGSUSED1
*/

LOCAL STATUS sramIoctl
    (
    SRAM_DEV *pSramDev,		/* device structure pointer */
    int	     function,		/* function code */
    int	     arg 		/* some argument */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CARD *pCard		= &pCtrl->card[pSramDev->sock];
    int	status;			/* returned status value */


    if (!pCard->installed)
	return (ERROR);

#ifdef	SRAM_DEBUG
    printf ("sramIoctl: function=0x%x arg=0x%x\n", function, arg);	
#endif

    switch (function)
	{
	case FIODISKFORMAT:
	    status = OK;
	    break;

	default:
	    errnoSet (S_ioLib_UNKNOWN_REQUEST);
	    status = ERROR;
	}

    return (status);
    }

/*******************************************************************************
*
* sramStatusChk - check the status
*
* This routine is called by the file system to check the status.
*
* RETURNS:  OK, or ERROR if there is an error.
*
*/

LOCAL STATUS sramStatusChk
    (
    SRAM_DEV *pSramDev		/* device structure pointer */
    )

    {
    PCMCIA_CTRL *pCtrl		= &pcmciaCtrl;
    PCMCIA_CHIP *pChip		= &pCtrl->chip;
    PCMCIA_CARD *pCard		= &pCtrl->card[pSramDev->sock];
    SEM_ID muteSemID		= &sramCtrl.muteSem[pSramDev->sock];
    BLK_DEV *pBlkDev		= &pSramDev->blkDev;


    if (!pCard->installed)
	return (ERROR);	

#ifdef	SRAM_DEBUG
    printf ("sramStatusChk: \n");	
#endif

    if (pCard->changed)
	{
	pBlkDev->bd_readyChanged = TRUE;
	pCard->changed		 = FALSE;
	}

    if (semTake (muteSemID, WAIT_FOREVER) == ERROR)
	return (ERROR);

    pCard->cardStatus = (* pChip->status) (pSramDev->sock);

    semGive (muteSemID);

    if (pCard->cardStatus & PC_BATDEAD)
	{
	(void) errnoSet (S_pcmciaLib_BATTERY_DEAD);
	return (ERROR);
	}

    if (pCard->cardStatus & PC_BATWARN)
	{
	(void) errnoSet (S_pcmciaLib_BATTERY_WARNING);
	return (ERROR);
	}

    if (pCard->cardStatus & PC_WRPROT)
        pBlkDev->bd_mode     = O_RDONLY;
    else
        pBlkDev->bd_mode     = O_RDWR;

    return (OK);
    }

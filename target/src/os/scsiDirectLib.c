/* scsiDirectLib.c - SCSI library for direct access devices (SCSI-2) */

/* Copyright 1989-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01g,10mar99,dat  SPR 25196, return error if numsecs == 0
01f,28aug97,dds  SPR 3425: "scsiFormatUnit" command, times out for disks which 
                 take a long time to format.
01e,29oct96,dgp  doc: editing for newly published SCSI libraries
01d,23jul96,dds  SPR 6718: added support for transfers above 16 MB.
01c,06may96,jds  more doc tweaks
01b,20sep95,jdi  doc tweak.
01a,24jul95,jds  written by extracting from scsi2Lib.
*/

/*
DESCRIPTION
This library contains commands common to all direct-access SCSI devices.
These routines are separated from scsi2Lib in order to create an additional
layer for better support of all SCSI direct-access devices.

Commands in this library include:

.TS
tab(|);
lf3 lf3
l l.
Command                 | Op Code
_
FORMAT UNIT             | (0x04)
READ (6)                | (0x08)
READ (10)               | (0x28)
READ CAPACITY           | (0x25)
RELEASE                 | (0x17)
RESERVE                 | (0x16)
MODE SELECT (6)         | (0x15)
MODE SELECT (10)        | (0x55)
MODE SENSE (6)          | (0x1a)
MODE SENSE (10)         | (0x5a)
START STOP UNIT         | (0x1b)
WRITE (6)               | (0x0a)
WRITE (10)              | (0x2a)
.TE

INCLUDE FILES
scsiLib.h, scsi2Lib.h

SEE ALSO: dosFsLib, rt11FsLib, rawFsLib, scsi2Lib,
.pG "I/O System, Local File Systems"
*/

#define  INCLUDE_SCSI2
#define  SCSI_BLOCK_ADDRESS_SIZE  0x1fffff  /* 21 Bit logical block address */

#include "vxWorks.h"
#include "ioLib.h"
#include "intLib.h"
#include "ctype.h"
#include "cacheLib.h"
#include "stdlib.h"
#include "errnoLib.h"
#include "taskLib.h"
#include "lstLib.h"
#include "logLib.h"
#include "msgQLib.h"
#include "string.h"
#include "stdio.h"
#include "sysLib.h"
#include "scsiLib.h"
#include "wdLib.h"


/* imported globals */

IMPORT BOOL scsiErrors;			/* enable SCSI error messages */
IMPORT BOOL scsiDebug;			/* enable task level debug messages */
IMPORT BOOL scsiIntsDebug;		/* enable int level debug messages */

/* global functions */

void scsiDirectLibTblInit ();

/*
 * Backward compatability functions localised
 */

LOCAL STATUS          scsi2ReadCapacity (SCSI_PHYS_DEV *, int *, int *);
LOCAL STATUS          scsi2RdSecs (SCSI_BLK_DEV *, int, int, char *);
LOCAL STATUS          scsiSectorRead (SCSI_BLK_DEV *, int, int, char *);
LOCAL STATUS          scsi2WrtSecs (SCSI_BLK_DEV *, int, int, char *);
LOCAL STATUS          scsiSectorWrite (SCSI_BLK_DEV *, int, int, char *);
LOCAL STATUS          scsi2FormatUnit (SCSI_PHYS_DEV *, BOOL, int, int, int,
                                                                char *,int);
LOCAL STATUS          scsi2ModeSelect (SCSI_PHYS_DEV *, int, int, char *, int);
LOCAL STATUS          scsi2ModeSense (SCSI_PHYS_DEV *, int, int, char *, int);

/* Block Device functions */

LOCAL BLK_DEV *       scsi2BlkDevCreate (SCSI_PHYS_DEV *, int, int);
LOCAL void            scsi2BlkDevInit (SCSI_BLK_DEV *, int, int);
LOCAL void            scsi2BlkDevShow (SCSI_PHYS_DEV *);
LOCAL STATUS 	      scsiStatusCheck (BLK_DEV *pBlkDev);
LOCAL STATUS          scsiBlkDevIoctl (SCSI_BLK_DEV *pScsiBlkDev, 
							int function, int arg);


/*******************************************************************************
*
* scsiDirectLibTblInit - initialize direct access functions in table 
*
* Initialisation of array of function pointers for SCSI1 and SCSI2 switching
*
* NOMANUAL
*/

void scsiDirectLibTblInit ()
    {
    pScsiIfTbl->scsiBlkDevCreate  = (FUNCPTR) scsi2BlkDevCreate;
    pScsiIfTbl->scsiBlkDevInit    = (FUNCPTR) scsi2BlkDevInit;
    pScsiIfTbl->scsiBlkDevShow    = (FUNCPTR) scsi2BlkDevShow;
    pScsiIfTbl->scsiFormatUnit    = (FUNCPTR) scsi2FormatUnit;
    pScsiIfTbl->scsiModeSelect    = (FUNCPTR) scsi2ModeSelect;
    pScsiIfTbl->scsiModeSense     = (FUNCPTR) scsi2ModeSense;
    pScsiIfTbl->scsiReadCapacity  = (FUNCPTR) scsi2ReadCapacity;
    pScsiIfTbl->scsiRdSecs        = (FUNCPTR) scsi2RdSecs;
    pScsiIfTbl->scsiWrtSecs       = (FUNCPTR) scsi2WrtSecs;
    }

/*******************************************************************************
*
* scsi2BlkDevCreate - define a logical partition on a SCSI block device
*
* This routine creates and initializes a BLK_DEV structure, which
* describes a logical partition on a SCSI physical block device.  A logical
* partition is an array of contiguously addressed blocks; it can be completely
* described by the number of blocks and the address of the first block in
* the partition.  In normal configurations, partitions do not overlap, although
* such a condition is not an error.
*
* NOTE:
* If `numBlocks' is 0, the rest of device is used.
*
* RETURNS: A pointer to the created BLK_DEV, or NULL if parameters exceed
* physical device boundaries, the physical device is not a block device, or
* memory is insufficient for the structures.
*/

LOCAL BLK_DEV *scsi2BlkDevCreate
    (
    SCSI_PHYS_DEV *pScsiPhysDev, /* ptr to SCSI physical device info */
    int numBlocks,               /* number of blocks in block device */
    int blockOffset              /* address of first block in volume */
    )
    {
    SCSI_BLK_DEV      *pScsiBlkDev;	/* ptr to SCSI block dev struct */
    SCSI_BLK_DEV_NODE *pScsiBlkDevNode;	/* ptr to SCSI block dev node struct */

    /* check parameters for validity */

    if ((pScsiPhysDev == NULL) ||
        (numBlocks < 0) || (blockOffset < 0) ||
	((blockOffset + numBlocks) > pScsiPhysDev->numBlocks))
	{
	errnoSet (S_scsiLib_ILLEGAL_PARAMETER);
	SCSI_DEBUG_MSG ("scsiBlkDevCreate: Invalid input parameter(s).\n",
			0, 0, 0, 0, 0, 0);
	return ((BLK_DEV *) NULL);
	}

    /* return NULL if sequential access (or other non-block) device */

    if (!((pScsiPhysDev->scsiDevType == SCSI_DEV_DIR_ACCESS) ||
    	  (pScsiPhysDev->scsiDevType == SCSI_DEV_WORM) ||
    	  (pScsiPhysDev->scsiDevType == SCSI_DEV_RO_DIR_ACCESS)))
	{
	errnoSet (S_scsiLib_ILLEGAL_OPERATION);
	SCSI_DEBUG_MSG ("scsiBlkDevCreate:", 0, 0, 0, 0, 0, 0);
	SCSI_DEBUG_MSG ("Physical device is not a block device.\n",
			0, 0, 0, 0, 0, 0);
	return ((BLK_DEV *) NULL);
	}

    /* disallow multiple partitions on removable media */

    if (pScsiPhysDev->removable && (lstCount (&pScsiPhysDev->blkDevList) != 0))
	{
	printErr ("scsiBlkDevCreate: ");
	printErr ("Can't create multiple partitions on removable media.\n");
	return ((BLK_DEV *) NULL);
	}

    /* create a SCSI block device node structure */

    pScsiBlkDevNode =
	(SCSI_BLK_DEV_NODE *) calloc (1, sizeof (SCSI_BLK_DEV_NODE));

    if (pScsiBlkDevNode == NULL)
        return ((BLK_DEV *) NULL);

    pScsiBlkDev = &pScsiBlkDevNode->scsiBlkDev;

    /* fill in the member data */

    pScsiBlkDev->blkDev.bd_blkRd  = (FUNCPTR) scsiRdSecs;
    pScsiBlkDev->blkDev.bd_blkWrt = (FUNCPTR) scsiWrtSecs;
    pScsiBlkDev->blkDev.bd_ioctl  = (FUNCPTR) scsiBlkDevIoctl;
    pScsiBlkDev->blkDev.bd_reset  = (FUNCPTR) NULL;

    if (pScsiPhysDev->removable)
	pScsiBlkDev->blkDev.bd_statusChk = (FUNCPTR) scsiStatusCheck;
    else
	pScsiBlkDev->blkDev.bd_statusChk = (FUNCPTR) NULL;

    pScsiBlkDev->blkDev.bd_removable = pScsiPhysDev->removable;

    pScsiBlkDev->blkDev.bd_nBlocks = (ULONG)
	(numBlocks == 0 ? pScsiPhysDev->numBlocks - blockOffset : numBlocks);

    pScsiBlkDev->blkDev.bd_bytesPerBlk = (ULONG) pScsiPhysDev->blockSize;

    pScsiBlkDev->blkDev.bd_retry = 1;
    pScsiBlkDev->blkDev.bd_mode = O_RDWR;
    pScsiBlkDev->blkDev.bd_readyChanged = TRUE;
    pScsiBlkDev->pScsiPhysDev = pScsiPhysDev;

    pScsiBlkDev->blockOffset = blockOffset;
    pScsiBlkDev->numBlocks   = (int) pScsiBlkDev->blkDev.bd_nBlocks;

    /* add block device to list created on the physical device */

    semTake (pScsiPhysDev->mutexSem, WAIT_FOREVER);

    lstAdd (&pScsiPhysDev->blkDevList, &pScsiBlkDevNode->blkDevNode);

    semGive (pScsiPhysDev->mutexSem);

    return (&pScsiBlkDev->blkDev);
    }

/*******************************************************************************
*
* scsi2BlkDevInit - initialize fields in a SCSI logical partition
*
* This routine specifies the disk geometry parameters required by certain
* file systems (e.g., dosFs).  It should be called after a SCSI_BLK_DEV
* structure is created via scsiBlkDevCreate(), but before a file system
* initialization routine.  It is generally required only for removable media
* devices.
*
* RETURNS: N/A
*/

LOCAL void scsi2BlkDevInit
    (
    SCSI_BLK_DEV *pScsiBlkDev,  /* ptr to SCSI block dev. struct */
    int blksPerTrack,           /* blocks per track */
    int nHeads                  /* number of heads */
    )
    {
    pScsiBlkDev->blkDev.bd_blksPerTrack = (ULONG) blksPerTrack;
    pScsiBlkDev->blkDev.bd_nHeads = (ULONG) nHeads;
    }

/*******************************************************************************
*
* scsi2BlkDevShow - show the BLK_DEV structures on a specified physical device
*
* This routine displays all of the BLK_DEV structures created on a specified
* physical device.  This routine is called by scsiShow(), but may also be
* invoked directly, usually from the shell.
*
* RETURNS: N/A
*/

LOCAL void scsi2BlkDevShow
    (
    SCSI_PHYS_DEV *pScsiPhysDev  /* ptr to SCSI physical device info */
    )
    {
    SCSI_BLK_DEV_NODE *pScsiBlkDevNode;
    int ix = 0;

    printf ("Block Device #       physical address     size (blocks)\n");
    printf ("--------------       ----------------     -------------\n");

    if (lstCount (&pScsiPhysDev->blkDevList) == 0)
	return;

    semTake (pScsiPhysDev->mutexSem, WAIT_FOREVER);

    for (pScsiBlkDevNode = (SCSI_BLK_DEV_NODE *)
			       lstFirst (&pScsiPhysDev->blkDevList);
         pScsiBlkDevNode != NULL;
         pScsiBlkDevNode = (SCSI_BLK_DEV_NODE *)
			       lstNext (&pScsiBlkDevNode->blkDevNode))
        {
	printf ("%8d              %8d                %8d\n", ix++,
		pScsiBlkDevNode->scsiBlkDev.blockOffset,
		pScsiBlkDevNode->scsiBlkDev.numBlocks);
	}

    semGive (pScsiPhysDev->mutexSem);
    }

/*******************************************************************************
*
* scsi2FormatUnit - issue a FORMAT_UNIT command to a SCSI device
*
* This routine issues a FORMAT_UNIT command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi2FormatUnit
    (
    SCSI_PHYS_DEV *pScsiPhysDev,/* ptr to SCSI physical device */
    BOOL cmpDefectList,         /* whether defect list is complete */
    int defListFormat,          /* defect list format */
    int vendorUnique,           /* vendor unique byte */
    int interleave,             /* interleave factor */
    char *buffer,               /* ptr to input data buffer */
    int bufLength               /* length of buffer in bytes */
    )
    {
    SCSI_COMMAND formatUnitCommand;	/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */

    SCSI_DEBUG_MSG ("scsiFormatUnit:\n", 0, 0, 0, 0, 0, 0);

    formatUnitCommand[0] = SCSI_OPCODE_FORMAT_UNIT;
    formatUnitCommand[1] = (UINT8) ((pScsiPhysDev->scsiDevLUN & 0x7) << 5);
    if (buffer != (char *) NULL)
	{
	formatUnitCommand[1] |= SCSI_FORMAT_DATA_BIT;
	if (cmpDefectList)
	    formatUnitCommand[1] |= SCSI_COMPLETE_LIST_BIT;
	formatUnitCommand[1] |= (defListFormat & 0x07);
	}
    formatUnitCommand[2]  = (UINT8) vendorUnique;
    formatUnitCommand[3]  = (UINT8) ((interleave >> 8) & 0xff);
    formatUnitCommand[4]  = (UINT8) ((interleave     ) & 0xff);
    formatUnitCommand[5]  = (UINT8) 0;

    scsiXaction.cmdAddress    = formatUnitCommand;
    scsiXaction.cmdLength     = SCSI_GROUP_0_CMD_LENGTH;
    scsiXaction.dataAddress   = (UINT8 *) buffer;
    scsiXaction.dataDirection = O_WRONLY;
    scsiXaction.dataLength    = bufLength;
    scsiXaction.addLengthByte = NONE;
    scsiXaction.cmdTimeout    = WAIT_FOREVER;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
	    (pScsiPhysDev, &scsiXaction));
    }

/*******************************************************************************
*
* scsi2ModeSelect - issue a MODE_SELECT command to a SCSI device
*
* This routine issues a MODE_SELECT command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi2ModeSelect
    (
    SCSI_PHYS_DEV *pScsiPhysDev,/* ptr to SCSI physical device */
    int pageFormat,             /* value of the page format bit (0-1) */
    int saveParams,             /* value of the save parameters bit (0-1) */
    char *buffer,               /* ptr to output data buffer */
    int bufLength               /* length of buffer in bytes */
    )
    {
    SCSI_COMMAND modeSelectCommand;	/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */
    int tempLBAField;			/* "logical block address" field */

    SCSI_DEBUG_MSG ("scsiModeSelect:\n", 0, 0, 0, 0, 0, 0);

    tempLBAField = (pageFormat ? (1 << 20) : 0) | (saveParams ? (1 << 16) : 0);

    if (scsiCmdBuild (modeSelectCommand, &scsiXaction.cmdLength,
	SCSI_OPCODE_MODE_SELECT, pScsiPhysDev->scsiDevLUN, FALSE,
	tempLBAField, min (0xff, bufLength), (UINT8) 0)
	== ERROR)
    	return (ERROR);

    scsiXaction.cmdAddress    = modeSelectCommand;
    scsiXaction.dataAddress   = (UINT8 *) buffer;
    scsiXaction.dataDirection = O_WRONLY;
    scsiXaction.dataLength    = min (0xff, bufLength);
    scsiXaction.addLengthByte = NONE;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
	    (pScsiPhysDev, &scsiXaction));
    }

/*******************************************************************************
*
* scsi2ModeSense - issue a MODE_SENSE command to a SCSI device
*
* This routine issues a MODE_SENSE command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi2ModeSense
    (
    SCSI_PHYS_DEV *pScsiPhysDev,/* ptr to SCSI physical device */
    int pageControl,            /* value of the page control field (0-3) */
    int pageCode,               /* value of the page code field (0-0x3f) */
    char *buffer,               /* ptr to input data buffer */
    int bufLength               /* length of buffer in bytes */
    )
    {
    SCSI_COMMAND modeSenseCommand;	/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */

    SCSI_DEBUG_MSG ("scsiModeSense:\n", 0, 0, 0, 0, 0, 0);

    if (scsiCmdBuild (modeSenseCommand, &scsiXaction.cmdLength,
	SCSI_OPCODE_MODE_SENSE, pScsiPhysDev->scsiDevLUN, FALSE,
	0, min (0xff, bufLength), (UINT8) 0)
	== ERROR)
    	return (ERROR);

    modeSenseCommand [2] = (UINT8) ((pageControl << 6) | pageCode);

    scsiXaction.cmdAddress    = modeSenseCommand;
    scsiXaction.dataAddress   = (UINT8 *) buffer;
    scsiXaction.dataDirection = O_RDONLY;
    scsiXaction.dataLength    = min (0xff, bufLength);
    scsiXaction.addLengthByte = MODE_SENSE_ADD_LENGTH_BYTE;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
	    (pScsiPhysDev, &scsiXaction));
    }

/*******************************************************************************
*
* scsi2ReadCapacity - issue a READ_CAPACITY command to a SCSI device
*
* This routine issues a READ_CAPACITY command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi2ReadCapacity
    (
    SCSI_PHYS_DEV *pScsiPhysDev,/* ptr to SCSI physical device */
    int *pLastLBA,              /* where to return last logical block adrs */
    int *pBlkLength             /* where to return block length */
    )
    {
    SCSI_COMMAND readCapCommand;	/* SCSI command byte array */
    RD_CAP_DATA readCapData;		/* data structure for results */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */

    SCSI_DEBUG_MSG ("scsiReadCapacity:\n", 0, 0, 0, 0, 0, 0);

    if (scsiCmdBuild (readCapCommand, &scsiXaction.cmdLength,
	SCSI_OPCODE_READ_CAPACITY, pScsiPhysDev->scsiDevLUN, FALSE,
	0, 0, (UINT8) 0)
	== ERROR)
    	return (ERROR);

    scsiXaction.cmdAddress    = readCapCommand;
    scsiXaction.dataAddress   = (UINT8 *) &readCapData;
    scsiXaction.dataDirection = O_RDONLY;
    scsiXaction.dataLength    = sizeof (readCapData);
    scsiXaction.addLengthByte = NONE;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    if ((*pScsiPhysDev->pScsiCtrl->scsiTransact) (pScsiPhysDev, &scsiXaction)
	== ERROR)
	return (ERROR);
    else
	{
	*pLastLBA   = readCapData.lastLogBlkAdrs;
	*pBlkLength = readCapData.blkLength;
	SCSI_SWAB (pLastLBA, sizeof (*pLastLBA));
	SCSI_SWAB (pBlkLength, sizeof (*pBlkLength));
	return (OK);
	}
    }

/******************************************************************************
*
* scsi2RdSecs - write sector(s) to a SCSI block device
*
* This routine checks if the data to be transferred is greater than the
* maximum transfer length permitted by the device. If the data to be 
* transferred is greater than the maximum transfer length it breaks the
* data into permissible sizes and loops until the transfer is done.
*
* RETURNS: OK, or ERROR if the sector cannot be written, or if numSecs
* is zero.
*/

LOCAL STATUS scsi2RdSecs
    (
    SCSI_BLK_DEV *pScsiBlkDev,  /* ptr to SCSI block device info */
    int sector,                 /* sector number to be read */
    int numSecs,                /* total sectors to be read */
    char *buffer                /* ptr to input data buffer */
    )
    {
    SCSI_PHYS_DEV *pScsiPhysDev = pScsiBlkDev->pScsiPhysDev;
    int dataToXfer;             /* Total Transfer size */
    int sectorNum;               /* sector number to be read */
    int numbSecs;               /* total sectors to be read */
    char *xferBuf;              /* ptr to input data buffer    */
    STATUS xferStatus = OK;

    dataToXfer = numSecs * pScsiPhysDev->blockSize;
    xferBuf = buffer;
    sectorNum = sector;

    if (numSecs == 0)
	return ERROR;

    if (dataToXfer <= (pScsiPhysDev->pScsiCtrl->maxBytesPerXfer))
	xferStatus = scsiSectorRead (pScsiBlkDev,sector,numSecs,buffer);
    else
	{
	while (dataToXfer > 0)
	    {
	    /* determine the number of sectors to read */
	    
	    if (dataToXfer > (pScsiPhysDev->pScsiCtrl->maxBytesPerXfer))
		numbSecs = (pScsiPhysDev->pScsiCtrl->maxBytesPerXfer) / 
		     	    pScsiPhysDev->blockSize;
	    else
		numbSecs = dataToXfer / pScsiPhysDev->blockSize;

	    /* read the sectors */

	    xferStatus = scsiSectorRead (pScsiBlkDev,sectorNum,numbSecs,
					 xferBuf);

	    if (xferStatus == OK)
		{
		/* 
		 * increment the sector no, buffer pointers & update the
		 * bytes left to be transferred
		 */

		sectorNum += numbSecs;
		xferBuf += (numbSecs * pScsiPhysDev->blockSize);
		dataToXfer -= (numbSecs * pScsiPhysDev->blockSize);
		}
	    else
		return (xferStatus);
	    }
	}
    return (xferStatus);
    }

/*******************************************************************************
*
* scsiSectorRead - read sector(s) from a SCSI block device
*
* This routine reads the specified physical sector(s) from a specified
* physical device.
*
* RETURNS: OK, or ERROR if the sector cannot be read.
*/

LOCAL STATUS scsiSectorRead
    (
    SCSI_BLK_DEV *pScsiBlkDev,  /* ptr to SCSI block device info */
    int sector,                 /* sector number to be read */
    int numSecs,                /* total sectors to be read */
    char *buffer                /* ptr to input data buffer */
    )
    {
    SCSI_COMMAND readCommand;		/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */
    SCSI_PHYS_DEV *pScsiPhysDev = pScsiBlkDev->pScsiPhysDev;
    int startSec = sector + pScsiBlkDev->blockOffset;
    
    SCSI_DEBUG_MSG ("scsiRdSecs:\n", 0, 0, 0, 0, 0, 0);
    
    if (startSec <= SCSI_BLOCK_ADDRESS_SIZE && numSecs <= 256)
        {
        /* build a 21 bit logical block address 'READ' command */
	
	if (scsiCmdBuild (readCommand, &scsiXaction.cmdLength,
			  SCSI_OPCODE_READ, pScsiPhysDev->scsiDevLUN, FALSE,
			  startSec, (numSecs == 256 ? 0 : numSecs), (UINT8) 0)
	    == ERROR)
	    return (ERROR);
        }
    else
        {
        /* build a 32 bit logical block address 'READ_EXTENDED' command */
	
	if (scsiCmdBuild (readCommand, &scsiXaction.cmdLength, 
			  SCSI_OPCODE_READ_EXT, pScsiPhysDev->scsiDevLUN, 
			  FALSE, startSec, numSecs, (UINT8) 0) == ERROR)
	    return (ERROR);
        }
    
    scsiXaction.cmdAddress    = readCommand;
    scsiXaction.dataAddress   = (UINT8 *) buffer;
    scsiXaction.dataDirection = O_RDONLY;
    scsiXaction.dataLength    = numSecs * pScsiPhysDev->blockSize;
    scsiXaction.addLengthByte = NONE;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;
    
    if (numSecs < 2000)
	scsiXaction.cmdTimeout = SCSI_TIMEOUT_5SEC +
				 (SCSI_TIMEOUT_1SEC * numSecs);
    else
	scsiXaction.cmdTimeout = SCSI_TIMEOUT_FULL;

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
	    (pScsiPhysDev, &scsiXaction));
    }

/*******************************************************************************
*
* scsi2WrtSecs - write sector(s) to a SCSI block device
*
* This routine checks if the data to be transferred is greater than the
* maximum transfer length permitted by the device. If the data to be 
* transferred is greater than the maximum transfer length it breaks the
* data into permissible sizes and loops until the transfer is done.
*
* RETURNS: OK, or ERROR if the sector cannot be written, or if the numSecs
* is zero.
*/

LOCAL STATUS scsi2WrtSecs
    (
    SCSI_BLK_DEV *pScsiBlkDev,  /* ptr to SCSI block device info */
    int sector,                 /* sector number to be read */
    int numSecs,                /* total sectors to be read */
    char *buffer                /* ptr to input data buffer */
    )
    {
    SCSI_PHYS_DEV *pScsiPhysDev = pScsiBlkDev->pScsiPhysDev;
    int dataToXfer;             /* Total Transfer size */
    int sectorNum;               /* sector number to be written */
    int numbSecs;               /* total sectors to be written */
    char *xferBuf;              /* ptr to input data buffer    */
    STATUS xferStatus = OK;

    dataToXfer = numSecs * pScsiPhysDev->blockSize;
    xferBuf = buffer;
    sectorNum = sector;

    if (numSecs == 0)
	return ERROR;

    if (dataToXfer <= (pScsiPhysDev->pScsiCtrl->maxBytesPerXfer))
	xferStatus = scsiSectorWrite (pScsiBlkDev,sector,numSecs,buffer);
    else
	{
	while (dataToXfer > 0)
	    {
	    /* determine the number of sectors to written */
	    
	    if (dataToXfer > (pScsiPhysDev->pScsiCtrl->maxBytesPerXfer))
		numbSecs = (pScsiPhysDev->pScsiCtrl->maxBytesPerXfer) / 
			    pScsiPhysDev->blockSize;
	    else
		numbSecs = dataToXfer / pScsiPhysDev->blockSize;
	    
	    /* write the sectors */
	    
	    xferStatus = scsiSectorWrite (pScsiBlkDev,sectorNum,numbSecs,
					  xferBuf);
	    
	    if (xferStatus == OK)
		{
		/* 
		 * increment the sector no, buffer pointers & update the
		 * bytes left to be transferred
		 */
		
		sectorNum += numbSecs;
		xferBuf += (numbSecs * pScsiPhysDev->blockSize);
		dataToXfer -= (numbSecs * pScsiPhysDev->blockSize);
		}
	    else
		return (xferStatus);
	    }
	}
    return (xferStatus);
    }

/*******************************************************************************
*
* scsiSectorWrite - write sector(s) to a SCSI block device
*
* This routine writes the specified physical sector(s) to a specified physical
* device.
*
* RETURNS: OK, or ERROR if the sector cannot be written.
*/

LOCAL STATUS scsiSectorWrite
    (
    SCSI_BLK_DEV *pScsiBlkDev,  /* ptr to SCSI block device info */
    int sector,                 /* sector number to be read */
    int numSecs,                /* total sectors to be read */
    char *buffer                /* ptr to input data buffer */
    )
    {
    SCSI_COMMAND writeCommand;		/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */
    SCSI_PHYS_DEV *pScsiPhysDev = pScsiBlkDev->pScsiPhysDev;
    int startSec = sector + pScsiBlkDev->blockOffset;

    SCSI_DEBUG_MSG ("scsiWrtSecs:\n", 0, 0, 0, 0, 0, 0);

    if (startSec <= SCSI_BLOCK_ADDRESS_SIZE && numSecs <= 256)
        {
        /* build a 21 bit logical block address 'WRITE' command */

	if (scsiCmdBuild (writeCommand, &scsiXaction.cmdLength,
			  SCSI_OPCODE_WRITE, pScsiPhysDev->scsiDevLUN, FALSE,
			  startSec, (numSecs == 256 ? 0 : numSecs), (UINT8) 0)
	    == ERROR)
	    {
	    return (ERROR);
	    }
        }
    else
        {
        /* build a 32 bit logical block address 'WRITE_EXTENDED' command */
	
	if (scsiCmdBuild (writeCommand, &scsiXaction.cmdLength,
			  SCSI_OPCODE_WRITE_EXT, pScsiPhysDev->scsiDevLUN,
			  FALSE, startSec, numSecs, (UINT8) 0) == ERROR)
	    return (ERROR);
        }
    
    scsiXaction.cmdAddress    = writeCommand;
    scsiXaction.dataAddress   = (UINT8 *) buffer;
    scsiXaction.dataDirection = O_WRONLY;
    scsiXaction.dataLength    = numSecs * pScsiPhysDev->blockSize;
    scsiXaction.addLengthByte = NONE;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;
    
    if (numSecs < 2000)
	scsiXaction.cmdTimeout = SCSI_TIMEOUT_5SEC +
				 (SCSI_TIMEOUT_1SEC * numSecs);
    else
	scsiXaction.cmdTimeout = SCSI_TIMEOUT_FULL;
    
    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
	    (pScsiPhysDev, &scsiXaction));
    }

/*******************************************************************************
*
* scsiStatusCheck - called by filesystems before doing open()'s or creat()'s
*
* This routine issues a TEST_UNIT_READY command to a SCSI device to detect a
* medium change.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS scsiStatusCheck
    (
    BLK_DEV *pBlkDev                    /* ptr to a block dev */
    )
    {
    SCSI_PHYS_DEV *pScsiPhysDev;        /* ptr to SCSI physical device */
    SCSI_COMMAND testUnitRdyCommand;    /* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;       /* info on a SCSI transaction */

    pScsiPhysDev = ((SCSI_BLK_DEV *) pBlkDev)->pScsiPhysDev;

    if (scsiCmdBuild (testUnitRdyCommand, &scsiXaction.cmdLength,
        SCSI_OPCODE_TEST_UNIT_READY, pScsiPhysDev->scsiDevLUN, FALSE,
        0, 0, (UINT8) 0) == ERROR)
        {
        return (ERROR);
        }

    scsiXaction.cmdAddress    = testUnitRdyCommand;
    scsiXaction.dataAddress   = NULL;
    scsiXaction.dataDirection = NONE;
    scsiXaction.dataLength    = 0;
    scsiXaction.addLengthByte = NONE;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    (*pScsiPhysDev->pScsiCtrl->scsiTransact) (pScsiPhysDev, &scsiXaction);

    if ((pScsiPhysDev->lastSenseKey != SCSI_SENSE_KEY_NO_SENSE) &&
        (pScsiPhysDev->lastSenseKey != SCSI_SENSE_KEY_UNIT_ATTENTION))
        {
        SCSI_DEBUG_MSG ("scsiStatusCheck returning ERROR, last Sense = %x\n",
                        pScsiPhysDev->lastSenseKey, 0, 0, 0, 0, 0);
        return (ERROR);
        }
    else
        return (OK);
    }

/*******************************************************************************
*
* scsiBlkDevIoctl - call scsiIoctl() with pointer to SCSI physical device
*                   associated with the specified SCSI block device
*
* RETURNS: Status of scsiIoctl call.
*/

LOCAL STATUS scsiBlkDevIoctl
    (
    SCSI_BLK_DEV *pScsiBlkDev,  /* ptr to SCSI block device info */
    int function,               /* function code */
    int arg                     /* argument to pass called function */
    )
    {
    return (scsiIoctl (pScsiBlkDev->pScsiPhysDev, function, arg));
    }

/*******************************************************************************
*
* scsiStartStopUnit - issue a START_STOP_UNIT command to a SCSI device
*
* This routine issues a START_STOP_UNIT command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiStartStopUnit
    (
    SCSI_PHYS_DEV *pScsiPhysDev,  /* ptr to SCSI physical device */
    BOOL start                    /* TRUE == start, FALSE == stop */
    )
    {
    SCSI_COMMAND startStopCommand;      /* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;       /* info on a SCSI transaction */

    SCSI_DEBUG_MSG ("scsiStartStopUnit:\n", 0, 0, 0, 0, 0, 0);

    if (scsiCmdBuild (startStopCommand, &scsiXaction.cmdLength,
        SCSI_OPCODE_START_STOP_UNIT, pScsiPhysDev->scsiDevLUN, FALSE,
        0, 1, (UINT8) 0)
        == ERROR)
        {
        return (ERROR);
        }

    scsiXaction.cmdAddress    = startStopCommand;
    scsiXaction.dataDirection = O_RDONLY;
    scsiXaction.dataLength    = 0;
    scsiXaction.addLengthByte = NONE;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
            (pScsiPhysDev, &scsiXaction));
    }

/*******************************************************************************
*
* scsiReserve - issue a RESERVE command to a SCSI device
*
* This routine issues a RESERVE command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiReserve
    (
    SCSI_PHYS_DEV *pScsiPhysDev         /* ptr to SCSI physical device */
    )
    {
    SCSI_COMMAND reserveCommand;        /* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;       /* info on a SCSI transaction */

    SCSI_DEBUG_MSG ("scsiReserve:\n", 0, 0, 0, 0, 0, 0);

    if (scsiCmdBuild (reserveCommand, &scsiXaction.cmdLength,
        SCSI_OPCODE_RESERVE, pScsiPhysDev->scsiDevLUN, FALSE,
        0, 0, (UINT8) 0)
        == ERROR)
        {
        return (ERROR);
        }

    scsiXaction.cmdAddress    = reserveCommand;
    scsiXaction.dataDirection = O_RDONLY;
    scsiXaction.dataLength    = 0;
    scsiXaction.addLengthByte = NONE;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
            (pScsiPhysDev, &scsiXaction));
    }

/*******************************************************************************
*
* scsiRelease - issue a RELEASE command to a SCSI device
*
* This routine issues a RELEASE command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiRelease
    (
    SCSI_PHYS_DEV *pScsiPhysDev         /* ptr to SCSI physical device */
    )
    {
    SCSI_COMMAND releaseCommand;        /* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;       /* info on a SCSI transaction */

    SCSI_DEBUG_MSG ("scsiRelease:\n", 0, 0, 0, 0, 0, 0);

    if (scsiCmdBuild (releaseCommand, &scsiXaction.cmdLength,
        SCSI_OPCODE_RELEASE, pScsiPhysDev->scsiDevLUN, FALSE,
        0, 0, (UINT8) 0)
        == ERROR)
        {
        return (ERROR);
        }

    scsiXaction.cmdAddress    = releaseCommand;
    scsiXaction.dataDirection = O_RDONLY;
    scsiXaction.dataLength    = 0;
    scsiXaction.addLengthByte = NONE;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
            (pScsiPhysDev, &scsiXaction));
    }


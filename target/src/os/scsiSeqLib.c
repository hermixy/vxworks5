/* scsiSeqLib.c - SCSI sequential access device library (SCSI-2) */

/* Copyright 1989-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02k,21feb99,jdi  doc: listed errnos.
02j,09jul97,dgp  doc: correct fonts per SPR 7853
02i,06mar97,dds  SPR 8120: fixed scsiRdTape to return the actual number of
                 bytes or blocks read, 0 if end of file, or ERROR.
02h,29oct96,dgp  doc: editing for newly published SCSI libraries
02g,16sep96,dds  removed compiler warnings.
02f,23jul96,dds  SPR 6718: added support for tranfers above 16 MB.
02e,06may96,jds  more doc tweaks
02d,01may96,jds  doc tweaks
02c,20sep95,jds  changed scsiStatusCheck to scsiSeqStatusCheck and made it
		 global
02b,26jul95,jds  maxVarBlockLimit to be set by scsiPhysDevCreate (Wrt & Rd) 
02a,10may95,jds  reworked for use with enhanced SCSI-2 library; added tagType,
		 and priority ; removed the idea of retrying command upon
		 UNIT_ATTENTION which should be managed by higher level layer.
01e,28jun94,ccc  doc tweaks.
01d,21jun94,ccc  changed dataAddress from NONE to NULL.
01c,27apr94,jdi	 doc tweaks.
01b,20apr94,jds	 enhanced scsiWrtTape and scsiRdTape to correctly handle fixed
		 block transfers and variable block transfers with maxBlockLimit
01a,24jan94,ccc  created to support sequential access SCSI devices.
*/

/*
DESCRIPTION
This library contains commands common to all sequential-access SCSI devices.
Sequential-access SCSI devices are usually SCSI tape devices.
These routines are separated from scsi2Lib in order to create an additional
layer for better support of all SCSI sequential devices.

SCSI commands in this library include:

.TS
tab(|);
lf3 lf3
l l.
Command			| Op Code
_
ERASE			| (0x19)
MODE SELECT (6)		| (0x15)
MODE_SENSE (6)		| (0x1a)
READ (6)		| (0x08)
READ BLOCK LIMITS	| (0x05)
RELEASE UNIT		| (0x17)
RESERVE UNIT		| (0x16)
REWIND			| (0x01)
SPACE			| (0x11)
WRITE (6)		| (0x0a)
WRITE FILEMARKS		| (0x10)
LOAD/UNLOAD		| (0x1b)
.TE

The SCSI routines implemented here operate mostly on a SCSI_SEQ_DEV
structure.  This structure acts as an interface between this library
and a higher-level layer.  The SEQ_DEV structure is analogous to the
BLK_DEV structure for block devices.  

The scsiSeqDevCreate() routine creates a SCSI_SEQ_DEV structure whose first 
element is a SEQ_DEV, operated upon by higher layers.  This routine publishes
all functions to be invoked by
higher layers and maintains some state information (for example, block size)
for tracking SCSI-sequential-device information.

INCLUDE FILES
scsiLib.h, scsi2Lib.h

SEE ALSO: tapeFsLib, scsi2Lib,
.pG "I/O System, Local File Systems"
*/

#define  INCLUDE_SCSI2
#include "vxWorks.h"
#include "ioLib.h"
#include "ctype.h"
#include "stdlib.h"
#include "errnoLib.h"
#include "taskLib.h"
#include "logLib.h"
#include "string.h"
#include "stdio.h"
#include "scsiLib.h"
#include "sysLib.h"

#define MODE_BUF_LENGTH 	  0xff
#define SCSI_MAX_XFER_BLOCKS  0xffffff
#define SCSI_READ             0x00
#define SCSI_WRITE            0x01

/* globals */

IMPORT BOOL scsiDebug;			/* enable task level debug messages */
IMPORT BOOL scsiIntsDebug;		/* enable int level debug messages */

/* select timeout to use when creating a SCSI_PHYS_DEV */

IMPORT UINT32 scsiSelectTimeout;

IMPORT SCSI_CTRL *pSysScsiCtrl;		/* ptr to default SCSI_CTRL struct */

LOCAL VOID scsiXactionFill (SCSI_TRANSACTION *, SCSI_COMMAND, BOOL, int, UINT,
			    int, UINT8 *);
LOCAL VOID scsiCmdFill (SCSI_SEQ_DEV *, SCSI_COMMAND , BOOL, BOOL, int);
LOCAL int scsiRdTapeFixedBlocks (SCSI_SEQ_DEV *, UINT, char * );
LOCAL int scsiRdTapeVariableBlocks (SCSI_SEQ_DEV *, UINT, char * );
LOCAL int scsiCalcDataRead (SCSI_SEQ_DEV *, UINT);

/*******************************************************************************
*
* scsiSeqDevCreate - create a SCSI sequential device
*
* This routine creates a SCSI sequential device and saves a pointer to this
* SEQ_DEV in the SCSI physical device. The following functions are
* initialized in this structure:
*
* .TS
* tab(|);
* l l.
* sd_seqRd           | -  scsiRdTape()
* sd_seqWrt          | -  scsiWrtTape()
* sd_ioctl           | -  scsiIoctl() (in scsiLib)
* sd_seqWrtFileMarks | -  scsiWrtFileMarks()
* sd_statusChk       | -  scsiSeqStatusCheck()
* sd_reset           | -  (not used)
* sd_rewind          | -  scsiRewind()
* sd_reserve         | -  scsiReserve()
* sd_release         | -  scsiRelease()
* sd_readBlkLim      | -  scsiSeqReadBlockLimits()
* sd_load            | -  scsiLoadUnit()
* sd_space           | -  scsiSpace()
* sd_erase           | -  scsiErase()
* .TE
*
* Only one SEQ_DEV per SCSI_PHYS_DEV is allowed, unlike BLK_DEVs where an 
* entire list is maintained. Therefore, this routine can be called only 
* once per creation of a sequential device.
*
* RETURNS: A pointer to the SEQ_DEV structure, or NULL if the command fails. 
* 
*/
SEQ_DEV *scsiSeqDevCreate
    (
    SCSI_PHYS_DEV *pScsiPhysDev  /* ptr to SCSI physical device info */
    )
    {
    SCSI_SEQ_DEV      *pScsiSeqDev;     /* ptr to SCSI sequential dev struct */

    /* check parameter for validity */

    if (pScsiPhysDev == NULL)
	{
	errnoSet (S_scsiLib_ILLEGAL_PARAMETER);
	SCSI_DEBUG_MSG ("scsiSeqDevCreate: Invalid input parameter(s).\n",
				0, 0, 0, 0, 0, 0);
	return ((SEQ_DEV *) NULL);
	}

    /* Check if sequential device alread exists */

    if (pScsiPhysDev->pScsiSeqDev != NULL)
	{
        errnoSet (S_scsiLib_ILLEGAL_OPERATION);
        SCSI_DEBUG_MSG ("scsiSeqDevCreate: SEQ_DEV already exists.\n", 
							0, 0, 0, 0, 0, 0);
        return ((SEQ_DEV *) NULL);
        }

    /* return NULL if not a sequential access device */

    if (pScsiPhysDev->scsiDevType != SCSI_DEV_SEQ_ACCESS)
        {
        errnoSet (S_scsiLib_ILLEGAL_OPERATION);
        SCSI_DEBUG_MSG ("scsiSeqDevCreate:", 0, 0, 0, 0, 0, 0);
        SCSI_DEBUG_MSG ("Physical device is not a sequential device.\n",
                        0, 0, 0, 0, 0, 0);
        return ((SEQ_DEV *) NULL);
        }

    if (!pScsiPhysDev->removable)
        SCSI_DEBUG_MSG ("scsiSeqDevCreate: Odd! Non removable tape!!\n",
						0, 0, 0, 0, 0, 0);

    pScsiSeqDev = (SCSI_SEQ_DEV *) calloc (1, sizeof (SCSI_SEQ_DEV));

    if (pScsiSeqDev == NULL)
	return ((SEQ_DEV *) NULL);

    pScsiSeqDev->seqDev.sd_seqRd           = (FUNCPTR) scsiRdTape;
    pScsiSeqDev->seqDev.sd_seqWrt          = (FUNCPTR) scsiWrtTape;
    pScsiSeqDev->seqDev.sd_ioctl           = (FUNCPTR) scsiIoctl;
    pScsiSeqDev->seqDev.sd_seqWrtFileMarks = (FUNCPTR) scsiWrtFileMarks;
    pScsiSeqDev->seqDev.sd_rewind 	   = (FUNCPTR) scsiRewind;
    pScsiSeqDev->seqDev.sd_reserve 	   = (FUNCPTR) scsiReserveUnit;
    pScsiSeqDev->seqDev.sd_release 	   = (FUNCPTR) scsiReleaseUnit;
    pScsiSeqDev->seqDev.sd_readBlkLim	   = (FUNCPTR) scsiSeqReadBlockLimits;
    pScsiSeqDev->seqDev.sd_load 	   = (FUNCPTR) scsiLoadUnit;
    pScsiSeqDev->seqDev.sd_space 	   = (FUNCPTR) scsiSpace;
    pScsiSeqDev->seqDev.sd_erase 	   = (FUNCPTR) scsiErase;
    pScsiSeqDev->seqDev.sd_statusChk       = (FUNCPTR) scsiSeqStatusCheck;
    pScsiSeqDev->seqDev.sd_reset           = (FUNCPTR) NULL;
    pScsiSeqDev->seqDev.sd_readyChanged    = TRUE;

    pScsiSeqDev->pScsiPhysDev = pScsiPhysDev;

    pScsiPhysDev->pScsiSeqDev = pScsiSeqDev;

    /* Note: sd_blkSize and sd_mode are left uninitialized */

    /* this should be the same as returning (SEQ_DEV *) pScsiSeqDev ?? */

    return (&pScsiSeqDev->seqDev);
    }
    
/*******************************************************************************
*
* scsiErase - issue an ERASE command to a SCSI device
*
* This routine issues an ERASE command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiErase
    (
    SCSI_PHYS_DEV *pScsiPhysDev,	/* ptr to SCSI physical device */
    BOOL longErase			/* TRUE for entire tape erase  */
    )
    {
    SCSI_COMMAND eraseCommand;		/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */
    STATUS status;			/* status of transaction */

    SCSI_DEBUG_MSG ("scsiErase:\n", 0, 0, 0, 0, 0, 0);

    /*
     * Build the SCSI command. Do not use scsiCmdBuild, because that function
     * is used only for direct access commands.
     */

    eraseCommand[0] = SCSI_OPCODE_ERASE;
    eraseCommand[1] = (UINT8) ((pScsiPhysDev->scsiDevLUN & 0x7) << 5);
    if (longErase)
	eraseCommand [1] |= 0x01;	/* set long bit */
    eraseCommand[2] = (UINT8) 0;
    eraseCommand[3] = (UINT8) 0;
    eraseCommand[4] = (UINT8) 0;
    eraseCommand[5] = (UINT8) 0;

    scsiXaction.cmdAddress    = eraseCommand;
    scsiXaction.cmdLength     = SCSI_GROUP_0_CMD_LENGTH;
    scsiXaction.dataAddress   = (UINT8 *) NULL;
    scsiXaction.dataDirection = O_WRONLY;
    scsiXaction.dataLength    = 0;
    scsiXaction.addLengthByte = NULL;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_FULL;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
	      (pScsiPhysDev, &scsiXaction);

    return (status);
    }

/*******************************************************************************
*
* scsiTapeModeSelect - issue a MODE_SELECT command to a SCSI tape device
*
* This routine issues a MODE_SELECT command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiTapeModeSelect
    (
    SCSI_PHYS_DEV *pScsiPhysDev,/* ptr to SCSI physical device            */
    int pageFormat,             /* value of the page format bit (0-1)     */
    int saveParams,             /* value of the save parameters bit (0-1) */
    char *buffer,               /* ptr to output data buffer              */
    int bufLength               /* length of buffer in bytes              */
    )
    {
    SCSI_COMMAND scsiCommand;		/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */
    STATUS status;			/* status of transaction */

    SCSI_DEBUG_MSG ("scsiTapeModeSelect:\n", 0, 0, 0, 0, 0, 0);

    scsiCommand[0] = SCSI_OPCODE_MODE_SELECT;
    scsiCommand[1] = (UINT8) ((pScsiPhysDev->scsiDevLUN & 0x7) << 5);
    scsiCommand[1] |= ((UINT8) ((pageFormat & 1) << 4) |
		       (UINT8) (saveParams & 1));
    scsiCommand[2] = (UINT8) 0;
    scsiCommand[3] = (UINT8) 0;
    scsiCommand[4] = (UINT8) (bufLength & 0xff);
    scsiCommand[5] = (UINT8) 0;

    scsiXaction.cmdAddress    = scsiCommand;
    scsiXaction.cmdLength     = SCSI_GROUP_0_CMD_LENGTH;
    scsiXaction.dataAddress   = (UINT8 *) buffer;
    scsiXaction.dataDirection = O_WRONLY;
    scsiXaction.dataLength    = min (0xff, bufLength);
    scsiXaction.addLengthByte = NULL;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
	      (pScsiPhysDev, &scsiXaction);

    return (status);
    }

/*******************************************************************************
*
* scsiTapeModeSense - issue a MODE_SENSE command to a SCSI tape device
*
* This routine issues a MODE_SENSE command to a specified SCSI tape device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiTapeModeSense
    (
    SCSI_PHYS_DEV *pScsiPhysDev,/* ptr to SCSI physical device           */
    int pageControl,            /* value of the page control field (0-3) */
    int pageCode,               /* value of the page code field (0-0x3f) */
    char *buffer,               /* ptr to input data buffer              */
    int bufLength               /* length of buffer in bytes             */
    )
    {
    SCSI_COMMAND scsiCommand;	/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */
    STATUS status;			/* status of transaction */

    SCSI_DEBUG_MSG ("scsiModeSense: cmdAddress 0x%x\n", (int) scsiCommand,
								0, 0, 0, 0, 0);

    scsiCommand[0] = SCSI_OPCODE_MODE_SENSE;
    scsiCommand[1] = (UINT8) ((pScsiPhysDev->scsiDevLUN & 0x7) << 5);
    scsiCommand[2] = (UINT8) ((pageControl << 6) | pageCode);
    scsiCommand[3] = (UINT8) 0;
    scsiCommand[4] = (UINT8) (bufLength & 0xff);
    scsiCommand[5] = (UINT8) 0;

    scsiXaction.cmdAddress    = scsiCommand;
    scsiXaction.cmdLength     = SCSI_GROUP_0_CMD_LENGTH;
    scsiXaction.dataAddress   = (UINT8 *) buffer;
    scsiXaction.dataDirection = O_RDONLY;
    scsiXaction.dataLength    = min (0xff, bufLength);
    scsiXaction.addLengthByte = MODE_SENSE_ADD_LENGTH_BYTE;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
	      (pScsiPhysDev, &scsiXaction);

    return (status);
    }

/*******************************************************************************
*
* scsiReadBlockLimits - issue a READ_BLOCK_LIMITS command to a SCSI device
*
* This routine issues a READ_BLOCK_LIMITS command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*
* NOMANUAL
*/

STATUS scsiReadBlockLimits
    (
    SCSI_PHYS_DEV * pScsiPhysDev,/* ptr to SCSI physical device          */
    int    *pMaxBlockLength,     /* where to return maximum block length */
    UINT16 *pMinBlockLength      /* where to return minimum block length */
    )
    {
    SCSI_COMMAND readBlockLimitCommand;	/* SCSI command byte array */
    RD_BLOCK_LIMIT_DATA readBlkLimitData; /* data structure for results */
    SCSI_TRANSACTION scsiXaction;	  /* info on a SCSI transaction */
    STATUS status;			  /* status of the transaction */


    SCSI_DEBUG_MSG ("scsiReadBlockLimit:\n", 0, 0, 0, 0, 0, 0);

    readBlockLimitCommand[0] = SCSI_OPCODE_READ_BLOCK_LIMITS;
    readBlockLimitCommand[1] = (UINT8) ((pScsiPhysDev->scsiDevLUN & 0x7) << 5);
    readBlockLimitCommand[2] = (UINT8) 0;
    readBlockLimitCommand[3] = (UINT8) 0;
    readBlockLimitCommand[4] = (UINT8) 0;
    readBlockLimitCommand[5] = (UINT8) 0;

    scsiXaction.cmdAddress    = readBlockLimitCommand;
    scsiXaction.cmdLength     = SCSI_GROUP_0_CMD_LENGTH;
    scsiXaction.dataAddress   = (UINT8 *) &readBlkLimitData;
    scsiXaction.dataDirection = O_RDONLY;
    scsiXaction.dataLength    = sizeof (readBlkLimitData);
    scsiXaction.addLengthByte = NULL;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
	      (pScsiPhysDev, &scsiXaction);

    if (status == ERROR)
	return (ERROR);

    *pMaxBlockLength = readBlkLimitData.maxBlockLength;
    *pMinBlockLength = readBlkLimitData.minBlockLength;
    SCSI_SWAB (pMaxBlockLength, sizeof (*pMaxBlockLength));
    SCSI_SWAB (pMinBlockLength, sizeof (*pMinBlockLength));

    return (OK);
    }

/*******************************************************************************
*
* scsiSeqReadBlockLimits - issue a READ_BLOCK_LIMITS command to a SCSI device
*
* This routine issues a READ_BLOCK_LIMITS command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiSeqReadBlockLimits
    (
    SCSI_SEQ_DEV * pScsiSeqDev,  /* ptr to SCSI sequential device        */
    int    *pMaxBlockLength,     /* where to return maximum block length */
    UINT16 *pMinBlockLength      /* where to return minimum block length */
    )
    {
    SCSI_PHYS_DEV * pScsiPhysDev = pScsiSeqDev->pScsiPhysDev;

    return (scsiReadBlockLimits (pScsiPhysDev, pMaxBlockLength, 
							pMinBlockLength));
    }

/*******************************************************************************
*
* scsiCmdFill - fill the SCSI_COMMAND structure.
*
* RETURNS: N/A.
*/

LOCAL VOID scsiCmdFill 
    ( 
    SCSI_SEQ_DEV * pScsiSeqDev,
    SCSI_COMMAND   scsiCommand,  /* scsi command structure to be filled */
    BOOL           commandType,  /* read or write command               */
    BOOL           fixed,        /* variable or fixed                   */
    int            xferLength    /* total bytes or blocks to xfer       */
    )
    {
    SCSI_PHYS_DEV *pScsiPhysDev; /* ptr to SCSI physical device info */

    pScsiPhysDev = pScsiSeqDev->pScsiPhysDev;

    if ( commandType == SCSI_READ )
	scsiCommand[0] = SCSI_OPCODE_READ;
    else
	scsiCommand[0] = SCSI_OPCODE_WRITE;


    scsiCommand[1] = (UINT8) ((pScsiPhysDev->scsiDevLUN & 0x7) << 5);

    if (fixed)
	scsiCommand[1] |= 0x01;	/* set for fixed block size */

    scsiCommand[2] = (UINT8) ((xferLength >> 16) & 0xff);
    scsiCommand[3] = (UINT8) ((xferLength >>  8) & 0xff);
    scsiCommand[4] = (UINT8) (xferLength & 0xff);
    scsiCommand[5] = (UINT8) 0;
    }

/*******************************************************************************
*
* scsiXactionFill - fill the SCSI_TRANSACTION structure.
*
* RETURNS: N/A.
*/

LOCAL VOID scsiXactionFill 
    ( 
    SCSI_TRANSACTION * scsiXaction, /* scsi Transaction structure */
    SCSI_COMMAND   scsiCommand,     /* scsi command structure     */
    BOOL           commandType,     /* read or write command      */
    int            cmdLength,       /* scsi command length        */
    UINT           cmdTimeout,      /* scsi command timeout       */
    int            dataXferLen,     /* data transfer length       */
    UINT8 *        bufAdrs          /* scsi data address          */
    )
    {
    if ( commandType == SCSI_READ )
	scsiXaction->dataDirection = O_RDONLY;
    else
	scsiXaction->dataDirection = O_WRONLY;

    scsiXaction->cmdAddress  = scsiCommand;
    scsiXaction->cmdLength   = cmdLength;
    scsiXaction->dataAddress = bufAdrs;
    scsiXaction->dataLength  = dataXferLen;
    scsiXaction->addLengthByte = NULL;
    scsiXaction->cmdTimeout = cmdTimeout;
    scsiXaction->tagType = SCSI_TAG_DEFAULT;
    scsiXaction->priority = SCSI_THREAD_TASK_PRIORITY;
    }

/*******************************************************************************
*
* scsiRdTapeFixedBlocks - reads the number of blocks specified
*
* This routine reads the specified number of blocks from a specified
* physical device.  If `numBlocks' is greater than the `maxBytesLimit' field
* defined in the `pScsiPhysDev' structure, then more than one SCSI transaction
* is used to transfer the data.
*
* RETURNS: Number of blocks actually read, 0 if EOF, or ERROR.
*/

LOCAL int scsiRdTapeFixedBlocks
    (
    SCSI_SEQ_DEV *pScsiSeqDev,    /* ptr to SCSI sequential device info */
    UINT numBlocks,                /* total # of blocks to be read       */
    char * buffer                 /* ptr to iput data buffer            */    
    )
    {
    SCSI_PHYS_DEV *pScsiPhysDev;  /* ptr to SCSI physical device info   */
    SCSI_COMMAND readCommand;	  /* SCSI command byte array            */
    SCSI_TRANSACTION scsiXaction; /* info on a SCSI transaction         */
    UINT8 * bufPtr;               /* ptr to input data buffer           */
    UINT timeout;                 /* scsi command timeout               */
    int  xferLength;              /* transfer length                    */
    int  nBlocks = 0;             /* number of blocks read              */
    int  xferBlocks;              /* number of blocks to be xferred     */
    UINT blocksRead = 0;          /* actual number of blocks read       */

    pScsiPhysDev = pScsiSeqDev->pScsiPhysDev;
    
    if (numBlocks > SCSI_MAX_XFER_BLOCKS)
	return (ERROR);
    
    /*
     * Check if the number of blocks to be transferred is less than 
     * the max permissible size 
     */
    
    if (numBlocks <= (pScsiPhysDev->pScsiCtrl->maxBytesPerXfer / 
		      pScsiSeqDev->seqDev.sd_blkSize))
	{
	scsiCmdFill (pScsiSeqDev, readCommand, SCSI_READ, TRUE,
		     numBlocks);
	bufPtr = (UINT8 *) buffer;
	xferLength = numBlocks * pScsiSeqDev->seqDev.sd_blkSize;
	
	/* 
	 * timeout value is based on 100kB/sec xfer and a 5 
	 * sec threshold 
	 */
	
	timeout = SCSI_TIMEOUT_5SEC * 50 + (10 * numBlocks *
					    pScsiPhysDev->blockSize);
	
	scsiXactionFill (&scsiXaction, readCommand, SCSI_READ,
			 SCSI_GROUP_0_CMD_LENGTH, timeout, xferLength,
			 bufPtr);

        if (((*pScsiPhysDev->pScsiCtrl->scsiTransact) 
	     (pScsiPhysDev, &scsiXaction)) == ERROR)
	    return (ERROR);
	
	if ((blocksRead = scsiCalcDataRead (pScsiSeqDev, numBlocks)) == ERROR)
	    return (ERROR);
	
	return (blocksRead);
	}
    else
	{
	/* determine the max number of blocks that can be transferred */
	
	xferBlocks = (pScsiPhysDev->pScsiCtrl->maxBytesPerXfer) /
		     (pScsiSeqDev->seqDev.sd_blkSize);

	while (numBlocks > 0)
	    {
	    scsiCmdFill (pScsiSeqDev, readCommand, SCSI_READ, TRUE, 
			 xferBlocks);
	    bufPtr = (UINT8 *) buffer;
	    xferLength = xferBlocks * pScsiSeqDev->seqDev.sd_blkSize;
	    
	    /* 
	     * timeout value is based on 100kB/sec xfer and a 5 
	     * sec threshold 
	     */
	    
	    timeout = SCSI_TIMEOUT_5SEC * 50 + (10 * xferBlocks * 
						pScsiPhysDev->blockSize);
	    
	    scsiXactionFill (&scsiXaction, readCommand, SCSI_READ,
			     SCSI_GROUP_0_CMD_LENGTH, timeout, xferLength,
			     bufPtr);

	    if (((*pScsiPhysDev->pScsiCtrl->scsiTransact) 
		 (pScsiPhysDev, &scsiXaction)) == ERROR)
  	        return (ERROR);
	    
	    if ((nBlocks = scsiCalcDataRead (pScsiSeqDev, numBlocks)) == ERROR)
	        return (ERROR);
	    
	    numBlocks -= xferBlocks;
	    buffer += (xferBlocks * pScsiSeqDev->seqDev.sd_blkSize);
	    
	    if (numBlocks <= 
		(pScsiPhysDev->pScsiCtrl->maxBytesPerXfer /
		 pScsiSeqDev->seqDev.sd_blkSize))
		xferBlocks = numBlocks;
	    
	    blocksRead += nBlocks;
	    }
	return (blocksRead);
	}
    }

/*******************************************************************************
*
* scsiRdTapeVariableBlocks - reads the number of bytes specified
*
* This routine reads the specified number of bytes from a specified
* physical device.  If `numBytes' is greater than the `maxBytesLimit' field
* defined in the `pScsiPhysDev' structure, then more than one SCSI transaction
* is used to transfer the data.
*
* RETURNS: Number of bytes actually read, 0 if EOF, or ERROR.
*/

LOCAL int scsiRdTapeVariableBlocks
    (
    SCSI_SEQ_DEV *pScsiSeqDev,    /* ptr to SCSI sequential device info */
    UINT numBytes,                 /* total # of bytes to be read        */
    char * buffer                 /* ptr to input data buffer           */    
    )
    {
    SCSI_PHYS_DEV *pScsiPhysDev;  /* ptr to SCSI physical device info   */
    SCSI_COMMAND readCommand;	  /* SCSI command byte array            */
    SCSI_TRANSACTION scsiXaction; /* info on a SCSI transaction         */
    UINT8 * bufPtr;               /* ptr to input data buffer           */
    UINT timeout;                 /* scsi command timeout               */
    int  xferLength;              /* transfer length                    */
    int  nBytes;                  /* number of bytes read               */
    int  maxVarBlockLimit;        /* maximum variable block limit       */
    UINT bytesRead = 0;           /* actual number of bytes read        */
    int  readBytes;               /* for multiple read transactions     */
    
    
    pScsiPhysDev = pScsiSeqDev->pScsiPhysDev;
    
    /* get the maximum variable block size for the device */    
    
    maxVarBlockLimit = pScsiPhysDev->maxVarBlockLimit;
    
    if (maxVarBlockLimit > pScsiPhysDev->pScsiCtrl->maxBytesPerXfer)
	maxVarBlockLimit = pScsiPhysDev->pScsiCtrl->maxBytesPerXfer;
    
    /* multiple transactions needed if numBytes > maxVarBlockLimit */
    
    for (readBytes=0; numBytes > maxVarBlockLimit; 
	 numBytes -= maxVarBlockLimit, readBytes += maxVarBlockLimit)
        {
	scsiCmdFill (pScsiSeqDev, readCommand, SCSI_READ, FALSE, 
		     maxVarBlockLimit);
	
	bufPtr = (UINT8 *) buffer + readBytes;
	xferLength = maxVarBlockLimit;
	timeout = SCSI_TIMEOUT_5SEC * 50 + (maxVarBlockLimit * 10);
	
	scsiXactionFill (&scsiXaction, readCommand, SCSI_READ,
			 SCSI_GROUP_0_CMD_LENGTH, timeout, xferLength,
			 bufPtr);
	
        if (((*pScsiPhysDev->pScsiCtrl->scsiTransact) 
	     (pScsiPhysDev, &scsiXaction)) == ERROR)
	    return (ERROR);
	
	if ((nBytes = scsiCalcDataRead (pScsiSeqDev, xferLength)) == ERROR)
	    return (ERROR);
	
	bytesRead += nBytes;
        }
    
    if (numBytes > 0)
	{
	scsiCmdFill (pScsiSeqDev, readCommand, SCSI_READ, FALSE, numBytes);

	bufPtr = (UINT8 *) buffer + readBytes;
	timeout = SCSI_TIMEOUT_5SEC * 50 + (numBytes * 10);
	
	scsiXactionFill (&scsiXaction, readCommand, SCSI_READ,
			 SCSI_GROUP_0_CMD_LENGTH, timeout, numBytes,
			 bufPtr);
	
        if (((*pScsiPhysDev->pScsiCtrl->scsiTransact)
	     (pScsiPhysDev, &scsiXaction)) == ERROR)
            return (ERROR);
	
	if ((nBytes = scsiCalcDataRead (pScsiSeqDev, numBytes)) == ERROR)
 	    return (ERROR);
	
	bytesRead  += nBytes;
	}
    return (bytesRead);
    }

/*******************************************************************************
*
* scsiRdTape - read bytes or blocks from a SCSI tape device
*
* This routine reads the specified number of bytes or blocks from a specified
* physical device.  If the boolean <fixedSize> is true, then <numBytes>
* represents the number of blocks of size <blockSize>, defined in the 
* `pScsiPhysDev' structure.  If variable block sizes are used
* (<fixedSize> = FALSE), then <numBytes> represents the actual number of bytes
* to be read. 
*
* RETURNS: Number of bytes or blocks actually read, 0 if EOF, or ERROR.
*/

int scsiRdTape
    (
    SCSI_SEQ_DEV *pScsiSeqDev,    /* ptr to SCSI sequential device info */
    UINT count,                   /* total bytes or blocks to be read   */
    char *buffer,                 /* ptr to input data buffer           */
    BOOL fixedSize		  /* if variable size blocks            */
    )
    {
    SCSI_PHYS_DEV *pScsiPhysDev;  /* ptr to SCSI physical device info   */
    int            dataCount;     /* blocks or bytes read               */
    
    pScsiPhysDev = pScsiSeqDev->pScsiPhysDev;
    
    SCSI_DEBUG_MSG ("scsiRdTape:\n", 0, 0, 0, 0, 0, 0);
    
    /* Invalidate the request sense data */
    
    pScsiPhysDev->pReqSenseData[0] &= 0x7f;
    
    if (fixedSize)
	dataCount = scsiRdTapeFixedBlocks (pScsiSeqDev, count, buffer);
    else
	dataCount = scsiRdTapeVariableBlocks (pScsiSeqDev, count, buffer);
    
    return (dataCount);
    }

/*******************************************************************************
*
* scsiCalcDataRead - calculates the actual # of bytes read
*
* This routine calculates the actual number of bytes or blocks read after
* a read operation, by examining the request sense data if it is valid.
*
* RETURNS: bytes or blocks read, 0 if EOF or ERROR if tape move was 
* unsuccessful.
*          
* NOMANUAL
*/

LOCAL int scsiCalcDataRead 
    (
    SCSI_SEQ_DEV *pScsiSeqDev,    /* ptr to SCSI sequential device info */
    UINT numDataUnits              /* total bytes or blocks to be read   */
    )
    {
    SCSI_PHYS_DEV *pScsiPhysDev;  /* ptr to SCSI physical device info */
    int  residue;                 /* diff between requested & xfered data */
    UINT unitsRead = 0;           /* actual number of bytes read          */
    
    pScsiPhysDev = pScsiSeqDev->pScsiPhysDev;
    
    /* if request sense data is valid */
    
    if (pScsiPhysDev->pReqSenseData[0] & 0x80)
	{
	residue = (pScsiPhysDev->pReqSenseData[3] << 24) +
		  (pScsiPhysDev->pReqSenseData[4] << 16) +
		  (pScsiPhysDev->pReqSenseData[5] <<  8) +
		  (pScsiPhysDev->pReqSenseData[6]);
	
	if (residue < 0)
	    unitsRead = numDataUnits;
	else
	    unitsRead = numDataUnits - residue;
	
	/* Invalidate the request sense data */
	
	pScsiPhysDev->pReqSenseData[0] = pScsiPhysDev->pReqSenseData[0] & 0x7f;
	
        /*
	 * Check the request sense data to see if End of Filemark
	 * or End of Medium was encountered.
	 */
	
	if ((pScsiPhysDev->pReqSenseData[2] & 0x40) ||
	    (pScsiPhysDev->pReqSenseData[2] & 0x80))
	    {
            /* move tape head to the beginning of filemark */
	    
	    if ((scsiSpace (pScsiSeqDev, -1, SPACE_CODE_FILEMARK)) == ERROR)
	        {
		printf ("Backward space unsuccessful\n");
	        return (ERROR);
		}
  	    }
	return (unitsRead);
	}
    return (numDataUnits);
    }

/*******************************************************************************
*
* scsiWrtTape - write data to a SCSI tape device
*
* This routine writes data to the current block on a specified physical
* device.  If the boolean <fixedSize> is true, then <numBytes>
* represents the number of blocks of size <blockSize>,  
* defined in the `pScsiPhysDev' structure.  If variable block sizes are used
* (<fixedSize> = FALSE), then <numBytes> represents the actual number of bytes
* to be written.  If <numBytes> is greater than the `maxBytesLimit' field
* defined in the `pScsiPhysDev' structure, then more than one SCSI transaction
* is used to transfer the data.
*
* RETURNS: OK, or ERROR if the data cannot be written or zero bytes are 
* written.
*/

STATUS scsiWrtTape
    (
    SCSI_SEQ_DEV *pScsiSeqDev,    /* ptr to SCSI sequential device info  */
    int numBytes,                 /* total bytes or blocks to be written */
    char *buffer,                 /* ptr to input data buffer            */
    BOOL fixedSize		  /* if variable size blocks             */
    )
    {
    SCSI_COMMAND scsiCommand;	  /* SCSI command byte array          */
    SCSI_TRANSACTION scsiXaction; /* info on a SCSI transaction       */
    SCSI_PHYS_DEV *pScsiPhysDev;  /* ptr to SCSI physical device info */
    STATUS status = ERROR;	  /* status of transaction            */
    UINT8 * bufPtr;               /*  ptr to input data buffer        */
    UINT timeout;                 /* scsi command timeout             */
    int  xferLength;              /* transfer length                  */
    int  numBlocks;
    int  maxVarBlockLimit;
    int  writeBytes;
    int  xferBlocks;

    SCSI_DEBUG_MSG ("scsiWrtTape:\n", 0, 0, 0, 0, 0, 0);

    pScsiPhysDev = pScsiSeqDev->pScsiPhysDev;

    /*
     * Fixed block size transfer. The block size must be defined in 
     * pScsiPhysDev prior to calling this function 
     */

    if (fixedSize)
        {
	numBlocks = numBytes; /* numBytes means numBlocks for fixed blk size */
	
	if (numBlocks > SCSI_MAX_XFER_BLOCKS)
	    return (ERROR);

	/*
	 * Check if the number of blocks to be transferred
	 * is less than the max permissible size 
	 */

	if (numBlocks <= (pScsiPhysDev->pScsiCtrl->maxBytesPerXfer / 
			  pScsiSeqDev->seqDev.sd_blkSize))
   	    {
	    scsiCmdFill (pScsiSeqDev, scsiCommand, SCSI_WRITE, TRUE, 
			 numBlocks);

	    bufPtr = (UINT8 *) buffer;
	    xferLength = numBlocks * pScsiSeqDev->seqDev.sd_blkSize;

	    /* 
	     * timeout value is based on 100kB/sec xfer and a 5 
	     * sec threshold 
	     */

	    timeout = SCSI_TIMEOUT_5SEC * 50 + (10 * numBlocks *
						pScsiPhysDev->blockSize);
	    scsiXactionFill (&scsiXaction, scsiCommand, SCSI_WRITE,
			     SCSI_GROUP_0_CMD_LENGTH, timeout, xferLength,
			     bufPtr);	    
	    status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
		     (pScsiPhysDev, &scsiXaction);

	    return (status);
	    }

	else
	    {
	    /* determine the max number of blocks that can be transferred */

            xferBlocks = (pScsiPhysDev->pScsiCtrl->maxBytesPerXfer) /
	                 (pScsiSeqDev->seqDev.sd_blkSize);

	    while (numBlocks > 0)
                {   
		scsiCmdFill (pScsiSeqDev, scsiCommand, SCSI_WRITE, TRUE, 
			     xferBlocks);
		bufPtr = (UINT8 *) buffer;
		xferLength = xferBlocks * pScsiSeqDev->seqDev.sd_blkSize;
		
		/* 
		 * timeout value is based on 100kB/sec xfer and a 5 
		 * sec threshold 
		 */

		timeout = SCSI_TIMEOUT_5SEC * 50 + (10 * xferBlocks *
						    pScsiPhysDev->blockSize);
		scsiXactionFill (&scsiXaction, scsiCommand, SCSI_WRITE,
				 SCSI_GROUP_0_CMD_LENGTH, timeout, xferLength,
				 bufPtr);

		status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
		  (pScsiPhysDev, &scsiXaction);

		if (status == OK )
		    {
		     numBlocks -= xferBlocks;
		     buffer += (xferBlocks * pScsiSeqDev->seqDev.sd_blkSize);
		     if (numBlocks <= 
			 (pScsiPhysDev->pScsiCtrl->maxBytesPerXfer /
			  pScsiSeqDev->seqDev.sd_blkSize))
		       xferBlocks = numBlocks; 
		    }
		else
		    return (status);   
		}
	    return (status);
	    }
	}

    /* get the maximum variable block size for the device */    
    
    maxVarBlockLimit = pScsiPhysDev->maxVarBlockLimit;

    if ( maxVarBlockLimit > pScsiPhysDev->pScsiCtrl->maxBytesPerXfer)
	maxVarBlockLimit = pScsiPhysDev->pScsiCtrl->maxBytesPerXfer;

    /* multiple transactions needed if numBytes > maxVarBlockLimit */

    for (writeBytes=0; numBytes > maxVarBlockLimit;
	        numBytes -= maxVarBlockLimit, writeBytes += maxVarBlockLimit)
        {
	scsiCmdFill (pScsiSeqDev, scsiCommand, SCSI_WRITE, FALSE, 
		     maxVarBlockLimit);

	bufPtr = (UINT8 *) buffer + writeBytes;
	xferLength = maxVarBlockLimit;
	timeout = SCSI_TIMEOUT_5SEC * 50 + (maxVarBlockLimit * 10);

	scsiXactionFill (&scsiXaction, scsiCommand, SCSI_WRITE,
			 SCSI_GROUP_0_CMD_LENGTH, timeout, xferLength,
			 bufPtr);

        status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
    	           (pScsiPhysDev, &scsiXaction);

        if (status == ERROR)
	    return (status);
        }

    if (numBytes > 0)
	{
	scsiCmdFill (pScsiSeqDev, scsiCommand, SCSI_WRITE, FALSE, numBytes);

	bufPtr = (UINT8 *) buffer + writeBytes;
	xferLength = numBytes;
	timeout = SCSI_TIMEOUT_5SEC * 50 + (numBytes * 10);

	scsiXactionFill (&scsiXaction, scsiCommand, SCSI_WRITE,
			 SCSI_GROUP_0_CMD_LENGTH, timeout, xferLength,
			 bufPtr);

        status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
    	           (pScsiPhysDev, &scsiXaction);
        }

    return (status);
    }

/*******************************************************************************
*
* scsiRewind - issue a REWIND command to a SCSI device
*
* This routine issues a REWIND command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiRewind
    (
    SCSI_SEQ_DEV *pScsiSeqDev	/* ptr to SCSI Sequential device */
    )
    {
    SCSI_COMMAND rewindCommand;		/* SCSI command byte array     */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction  */
    SCSI_PHYS_DEV * pScsiPhysDev;    	/* ptr to SCSI physical device */
    STATUS status;			/* holds status of transaction */

    SCSI_DEBUG_MSG ("scsiRewind:\n", 0, 0, 0, 0, 0, 0);

    pScsiPhysDev = pScsiSeqDev->pScsiPhysDev;

    rewindCommand[0] = SCSI_OPCODE_REWIND;
    rewindCommand[1] = (UINT8) ((pScsiPhysDev->scsiDevLUN & 0x7) << 5);
    rewindCommand[2] = (UINT8) 0;
    rewindCommand[3] = (UINT8) 0;
    rewindCommand[4] = (UINT8) 0;
    rewindCommand[5] = (UINT8) 0;

    scsiXaction.cmdAddress    = rewindCommand;
    scsiXaction.cmdLength     = SCSI_GROUP_0_CMD_LENGTH;
    scsiXaction.dataAddress   = (UINT8 *) NULL;
    scsiXaction.dataDirection = O_WRONLY;
    scsiXaction.dataLength    = 0;
    scsiXaction.addLengthByte = NULL;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_FULL;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
	      (pScsiPhysDev, &scsiXaction);

    return (status);
    }

/*******************************************************************************
*
* scsiReserveUnit - issue a RESERVE UNIT command to a SCSI device
*
* This routine issues a RESERVE UNIT command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiReserveUnit
    (
    SCSI_SEQ_DEV *pScsiSeqDev	/* ptr to SCSI sequential device */
    )
    {
    SCSI_COMMAND reserveUnitCommand;	/* SCSI command byte array     */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction  */
    SCSI_PHYS_DEV *pScsiPhysDev;        /* ptr to SCSI physical device */
    STATUS status;			/* status of transaction       */

    SCSI_DEBUG_MSG ("scsiReserveUnit:\n", 0, 0, 0, 0, 0, 0);

    pScsiPhysDev = pScsiSeqDev->pScsiPhysDev;

    reserveUnitCommand[0] = SCSI_OPCODE_RESERVE;
    reserveUnitCommand[1] = (UINT8) 0;
    reserveUnitCommand[2] = (UINT8) 0;
    reserveUnitCommand[3] = (UINT8) 0;
    reserveUnitCommand[4] = (UINT8) 0;
    reserveUnitCommand[5] = (UINT8) 0;

    scsiXaction.cmdAddress    = reserveUnitCommand;
    scsiXaction.cmdLength     = SCSI_GROUP_0_CMD_LENGTH;
    scsiXaction.dataAddress   = (UINT8 *) NULL;
    scsiXaction.dataDirection = O_WRONLY;
    scsiXaction.dataLength    = 0;
    scsiXaction.addLengthByte = NULL;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
	      (pScsiPhysDev, &scsiXaction);

    return (status);
    }

/*******************************************************************************
*
* scsiReleaseUnit - issue a RELEASE UNIT command to a SCSI device
*
* This routine issues a RELEASE UNIT command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiReleaseUnit
    (
    SCSI_SEQ_DEV *pScsiSeqDev	/* ptr to SCSI sequential device */
    )
    {
    SCSI_COMMAND releaseUnitCommand;	/* SCSI command byte array     */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction  */
    SCSI_PHYS_DEV *pScsiPhysDev;        /* ptr to SCSI physical device */
    STATUS status;			/* status of transaction       */

    SCSI_DEBUG_MSG ("scsiReleaseUnit:\n", 0, 0, 0, 0, 0, 0);

    pScsiPhysDev = pScsiSeqDev->pScsiPhysDev;

    releaseUnitCommand[0] = SCSI_OPCODE_RELEASE;
    releaseUnitCommand[1] = (UINT8) 0;
    releaseUnitCommand[2] = (UINT8) 0;
    releaseUnitCommand[3] = (UINT8) 0;
    releaseUnitCommand[4] = (UINT8) 0;
    releaseUnitCommand[5] = (UINT8) 0;

    scsiXaction.cmdAddress    = releaseUnitCommand;
    scsiXaction.cmdLength     = SCSI_GROUP_0_CMD_LENGTH;
    scsiXaction.dataAddress   = (UINT8 *) NULL;
    scsiXaction.dataDirection = O_WRONLY;
    scsiXaction.dataLength    = 0;
    scsiXaction.addLengthByte = NULL;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
	      (pScsiPhysDev, &scsiXaction);

    return (status);
    }

/*******************************************************************************
*
* scsiLoadUnit - issue a LOAD/UNLOAD command to a SCSI device
*
* This routine issues a LOAD/UNLOAD command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiLoadUnit
    (
    SCSI_SEQ_DEV * pScsiSeqDev,	        /* ptr to SCSI physical device */
    BOOL           load,		/* TRUE=load, FALSE=unload     */
    BOOL	   reten,		/* TRUE=retention and unload   */
    BOOL	   eot			/* TRUE=end of tape and unload */
    )
    {
    SCSI_COMMAND loadCommand;		/* SCSI command byte array     */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction  */
    SCSI_PHYS_DEV *pScsiPhysDev;	/* ptr to SCSI physical device */
    STATUS status;			/* status of transaction       */

    SCSI_DEBUG_MSG ("scsiLoadUnit:\n", 0, 0, 0, 0, 0, 0);

    pScsiPhysDev = pScsiSeqDev->pScsiPhysDev;

    loadCommand[0] = SCSI_OPCODE_LOAD_UNLOAD;
    loadCommand[1] = (UINT8) 0;
    loadCommand[2] = (UINT8) 0;
    loadCommand[3] = (UINT8) 0;

    /* 
     * Check for load, retension and eot (TRUE/FALSE) conditions.
     * Byte4 bit2 of CDB: eot
     * Byte4 bit1 of CDB: reten
     * Byte4 bit0 of CDB: load
     */

    if (load && eot) 		/* invalid condition */
	return (ERROR);

    if (load)
        loadCommand[4] = (UINT8) 1;

    else /* unload */
	{
        loadCommand[4] = (UINT8) 0;
        if (eot)
	    loadCommand[4] |= (0x1 << 2);
        }

    if (reten)
    	loadCommand[4] |= (0x1 << 1);

    loadCommand[5] = (UINT8) 0;

    scsiXaction.cmdAddress    = loadCommand;
    scsiXaction.cmdLength     = SCSI_GROUP_0_CMD_LENGTH;
    scsiXaction.dataAddress   = (UINT8 *) NULL;
    scsiXaction.dataDirection = O_WRONLY;
    scsiXaction.dataLength    = 0;
    scsiXaction.addLengthByte = NULL;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC * 10;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
	      (pScsiPhysDev, &scsiXaction);

    return (status);
    }

/*******************************************************************************
*
* scsiWrtFileMarks - write file marks to a SCSI sequential device
*
* This routine writes file marks to a specified physical device.
*
* RETURNS: OK, or ERROR if the file mark cannot be written.
*/

STATUS scsiWrtFileMarks
    (
    SCSI_SEQ_DEV * pScsiSeqDev,   /* ptr to SCSI sequential device info */
    int            numMarks,      /* number of file marks to write      */
    BOOL           shortMark	  /* TRUE to write short file mark      */
    )
    {
    SCSI_COMMAND     scsiCommand;	/* SCSI command byte array          */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction       */
    SCSI_PHYS_DEV *  pScsiPhysDev;      /* ptr to SCSI physical device info */
    STATUS           status;		/* status of transactions           */

    SCSI_DEBUG_MSG ("scsiWrtFileMarks:\n", 0, 0, 0, 0, 0, 0);

    pScsiPhysDev = pScsiSeqDev->pScsiPhysDev;

    scsiCommand[0] = SCSI_OPCODE_WRITE_FILEMARKS;
    scsiCommand[1] = (UINT8) ((pScsiPhysDev->scsiDevLUN & 0x7) << 5);
    scsiCommand[2] = (UINT8) ((numMarks >> 16) & 0xff);
    scsiCommand[3] = (UINT8) ((numMarks >>  8) & 0xff);
    scsiCommand[4] = (UINT8) (numMarks & 0xff);
    if (shortMark)
	scsiCommand [5] = 0x80;
    else
        scsiCommand[5] = (UINT8) 0;

    scsiXaction.cmdAddress    = scsiCommand;
    scsiXaction.cmdLength     = SCSI_GROUP_0_CMD_LENGTH;
    scsiXaction.dataAddress   = NULL;
    scsiXaction.dataDirection = O_WRONLY;
    scsiXaction.dataLength    = NULL;
    scsiXaction.addLengthByte = NULL;
    if (numMarks < 500)
        scsiXaction.cmdTimeout = SCSI_TIMEOUT_5SEC * (numMarks + 10);
    else
	scsiXaction.cmdTimeout = SCSI_TIMEOUT_FULL;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
	      (pScsiPhysDev, &scsiXaction);

    return (status);
    }

/*******************************************************************************
*
* scsiSpace - move the tape on a specified physical SCSI device
*
* This routine moves the tape on a specified SCSI physical device.
* There are two types of space code that are mandatory in SCSI; currently
* these are the only two supported:
*
* .TS
* tab(|);
* lf3 lf3 cf3
* l l c.
* Code | Description | Support
* _
* 000 | Blocks		      | Yes
* 001 | File marks            | Yes
* 010 | Sequential file marks | No
* 011 | End-of-data           | No
* 100 | Set marks             | No
* 101 | Sequential set marks  | No
* .TE	
*
* RETURNS: OK, or ERROR if an error is returned by the device.
*
* ERRNO: S_scsiLib_ILLEGAL_REQUEST
*/

STATUS scsiSpace
    (
    SCSI_SEQ_DEV * pScsiSeqDev,  /* ptr to SCSI sequential device info */
    int count,                   /* count for space command            */
    int spaceCode		 /* code for the type of space command */
    )
    {
    SCSI_PHYS_DEV *pScsiPhysDev;	/* ptr to SCSI physical device info */
    SCSI_COMMAND scsiCommand;		/* SCSI command byte array          */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction       */
    STATUS status;			/* status of transaction            */

    SCSI_DEBUG_MSG ("scsiSpace:\n", 0, 0, 0, 0, 0, 0);

    pScsiPhysDev = pScsiSeqDev->pScsiPhysDev;

    if ((count == 0) || ((spaceCode != SPACE_CODE_DATABLK) && 
			 (spaceCode != SPACE_CODE_FILEMARK)))
        {
	errno = S_scsiLib_ILLEGAL_REQUEST;
	return (ERROR);
	}

    scsiCommand[0] = SCSI_OPCODE_SPACE;
    scsiCommand[1] = (UINT8) (((pScsiPhysDev->scsiDevLUN & 0x7) << 5) |
			      (spaceCode));
    scsiCommand[2] = (UINT8) ((count >> 16) & 0xff);
    scsiCommand[3] = (UINT8) ((count >>  8) & 0xff);
    scsiCommand[4] = (UINT8) (count & 0xff);
    scsiCommand[5] = (UINT8) 0;

    scsiXaction.cmdAddress    = scsiCommand;
    scsiXaction.cmdLength     = SCSI_GROUP_0_CMD_LENGTH;
    scsiXaction.dataAddress   = NULL;
    scsiXaction.dataDirection = O_RDONLY;
    scsiXaction.dataLength    = NULL;
    scsiXaction.addLengthByte = NULL;
    if (count < 500)
        scsiXaction.cmdTimeout = SCSI_TIMEOUT_5SEC * count * 10;
    else
	scsiXaction.cmdTimeout = SCSI_TIMEOUT_FULL;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    status = (*pScsiPhysDev->pScsiCtrl->scsiTransact)
	      (pScsiPhysDev, &scsiXaction);

    return (status);
    }

/*******************************************************************************
*
* scsiSeqStatusCheck - detect a change in media
*
* This routine issues a TEST_UNIT_READY command to a SCSI device to detect a
* change in media. It is called by file systems before executing open() or
* creat().
*
* INTERNAL
* This function is a duplicate of that in scsiDirectLib, except for the fact
* that this function operates on a SEQ_DEV.
* 
* RETURNS: OK or ERROR.
*/

STATUS scsiSeqStatusCheck
    (
    SCSI_SEQ_DEV *pScsiSeqDev          /* ptr to a sequential dev */
    )
    {
    SCSI_PHYS_DEV *pScsiPhysDev;        /* ptr to SCSI physical device */
    SCSI_COMMAND testUnitRdyCommand;    /* SCSI command byte array     */
    SCSI_TRANSACTION scsiXaction;       /* info on a SCSI transaction  */
    char             modeBuf[0xff];     /* get mode sense data array   */
    int		     modeBufLen;
    int 	     pageControl;
    int              pageCode;


    pScsiPhysDev =  pScsiSeqDev->pScsiPhysDev;

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

    if ((*pScsiPhysDev->pScsiCtrl->scsiTransact) (pScsiPhysDev, &scsiXaction)
							             == ERROR)
	{
        SCSI_DEBUG_MSG ("scsiSeqStatusCheck returning ERROR, last Sense = %x\n",
                        pScsiPhysDev->lastSenseKey, 0, 0, 0, 0, 0);
        return (ERROR);
	}

    pageControl = 0x0;  /* current values     */
    pageCode    = 0x0;  /* no page formatting */

    /* (Mode param hdr len) + (mode param block descriptor len) */

    modeBufLen  = 4 + 8;

    if (scsiModeSense (pScsiPhysDev, pageControl, pageCode, modeBuf,
		       modeBufLen
		       ) == ERROR)
        return (ERROR);

    /* Set the mode of the device */

    pScsiSeqDev->seqDev.sd_mode =
	( modeBuf [SCSI_MODE_DEV_SPECIFIC_PARAM] &
	  (UINT8) SCSI_DEV_SPECIFIC_WP_MASK
        ) ? O_RDONLY : O_RDWR;

    return (OK);
   
    }


/*******************************************************************************
*
* scsiSeqIoctl - perform an I/O control function for sequential access devices
*
* This routine issues scsiSeqLib commands to perform sequential
* device-specific I/O control operations.
*
* RETURNS: OK or ERROR.
*
* ERRNO: S_scsiLib_INVALID_BLOCK_SIZE
*/
int scsiSeqIoctl
    (
    SCSI_SEQ_DEV *  pScsiSeqDev,    /* ptr to SCSI sequential device */
    int		    function,	    /* ioctl function code */
    int		    arg 	    /* argument to pass to called function */
    )
    {
    char modeBuf[MODE_BUF_LENGTH];
    int  modeBufLen;
    int  pageControl;
    int  pageCode;
    int  pageFormat;
    int  savePages;
    int  blkSize;
    SCSI_PHYS_DEV * pScsiPhysDev;	/* ptr to SCSI physical device struct */


    if ((pScsiPhysDev = pScsiSeqDev->pScsiPhysDev) == NULL)
	{
	/* XXX errno */
	SCSI_DEBUG_MSG ("scsiSeqIoctl: pScsiSeqDev ptr NULL\n",0,0,0,0,0,0);
	return (ERROR);
	}

    switch (function)
	{
    	case FIODENSITYSET:


            /* Execute a MODE SENSE to get the right buffer values */

	    pageControl = 0x0;  /* current values     */
	    pageCode    = 0x0; 	/* no page formatting */

            /* (Mode param hdr len) + (mode param block descriptor len) */

    	    modeBufLen  = 4 + 8;

	    if (scsiModeSense (pScsiPhysDev, pageControl, pageCode, modeBuf,
			       modeBufLen
			      ) == ERROR)
		return (ERROR);
            

	    /* Execute a MODE SELECT to set the density value */

	    modeBuf[0] = 0x0;	/* mode data len not valid for mode select   */
	    /* modeBuf[1] is reserved; medium type not valid for seq devices */
	    modeBuf[2] = 0x0;   /* dev-specific param not defined for md sel */
	    modeBuf[4] = arg;   /* set density code                          */
            pageFormat = 0x0;   /* no formatted pages                        */
	    savePages  = 0x1;   /* save page values                          */

	    if (scsiModeSelect (pScsiPhysDev, pageFormat, savePages, modeBuf,
				modeBufLen) == ERROR)
	        return (ERROR);


            /* Check that the density was set correctly by issuing MODE SENSE */
            
	    if (scsiModeSense (pScsiPhysDev, pageControl, pageCode, modeBuf,
			       modeBufLen
			      ) == ERROR)
		return (ERROR);

            if (modeBuf[4] != arg)
		{
		/* XXX set errno */
		return (ERROR);
		}

	    pScsiSeqDev->seqDev.sd_density = arg;

	    return (OK);


        case FIODENSITYGET:


            /* Execute a MODE SENSE to get the right buffer values */
            
            pageControl = 0x0;  /* current values     */
            pageCode    = 0x0;  /* no page formatting */

            /* (Mode param hdr len) + (mode param block descriptor len) */

            modeBufLen  = 4 + 8;

            if (scsiModeSense (pScsiPhysDev, pageControl, pageCode, modeBuf,
                               modeBufLen
                              ) == ERROR)
                return (ERROR);

            * ((int *) arg) = modeBuf[4];	/* density */

	    return (OK);


        case FIOBLKSIZESET:


            /* Execute a MODE SENSE to get the right buffer values */
            
            pageControl = 0x0;  /* current values     */
            pageCode    = 0x0;  /* no page formatting */

            /* (Mode param hdr len) + (mode param block descriptor len) */

            modeBufLen  = 4 + 8;

            if (scsiModeSense (pScsiPhysDev, pageControl, pageCode, modeBuf,
                               modeBufLen
                              ) == ERROR)
                return (ERROR);

            /* Execute a MODE SELECT to set the blkSize value */

            modeBuf[0]  = 0x0;   /* mode data len not valid for mode select   */

            /* modeBuf[1] is reserved; medium type not valid for seq devices  */

	    modeBuf[1] = 0x0;
            modeBuf[2] = 0x0;    /* dev-specific param not defined for md sel */

            /* Set the block size */

            modeBuf[9]  = (UINT8) ((arg >> 16) & 0xff);
            modeBuf[10] = (UINT8) ((arg >>  8) & 0xff);
            modeBuf[11] = (UINT8) ( arg & 0xff); 

            pageFormat = 0x1;   /* no formatted pages                        */
            savePages  = 0x0;   /* save page values                          */

            if (scsiModeSelect (pScsiPhysDev, pageFormat, savePages, modeBuf,
                                modeBufLen) == ERROR)
                return (ERROR);


            /* Check that the block size was set correctly */

            blkSize = scsiSeqIoctl (pScsiSeqDev, FIOBLKSIZEGET, 0);

            if (blkSize != arg)
                {
                errno = S_scsiLib_INVALID_BLOCK_SIZE;
                return (ERROR);
                }

            pScsiSeqDev->seqDev.sd_blkSize = blkSize;

            return (OK);


        case FIOBLKSIZEGET:


            /* Execute a MODE SENSE to get the right buffer values */
            
            pageControl = 0x0;  /* current values     */
            pageCode    = 0x0;  /* no page formatting */

            /* (Mode param hdr len) + (mode param block descriptor len) */

            modeBufLen  = 4 + 8;

            if (scsiModeSense (pScsiPhysDev, pageControl, pageCode, modeBuf,
                               modeBufLen
                              ) == ERROR)
                return (ERROR);

            /* Construct the block size from the buffer */

            blkSize  = 0;
            blkSize  = (modeBuf[9]  << 16);
            blkSize |= (modeBuf[10] <<  8);
            blkSize |=  modeBuf[11];

            return (blkSize);


        default:


	    SCSI_DEBUG_MSG ("scsiSeqIoctl: bad IOCTL function\n",0,0,0,0,0,0);
            return (ERROR);


        } /* switch */

    }


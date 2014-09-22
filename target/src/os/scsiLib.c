/* scsiLib.c - Small Computer System Interface (SCSI) library */

/* Copyright 1989-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01h,09jul97,dgp  doc: correct fonts per SPR 7853
01g,29oct96,dgp  doc: editing for newly published SCSI libraries
01f,01may96,jds  doc tweaks
01e,13nov95,jds  minor documentation tweaks
01d,08nov95,jds  main description modification
01c,20sep95,jdi  doc tweaks.
01b,11feb95,jdi  doc: fixed style for global variable pSysScsiCtrl.
01a,24aug94,jds  Written. Interface and documentation remain the same. However,
	         routines moved to different SCSI1 or SCSI2 libraries.
*/

/*
DESCRIPTION
The purpose of this library is to switch SCSI function calls (the common 
SCSI-1 and SCSI-2 calls listed above) to either scsi1Lib or scsi2Lib, 
depending upon the SCSI configuration in the Board Support Package (BSP).
The normal usage is to configure SCSI-2. However, SCSI-1 is configured
when device incompatibilities exist. VxWorks can be configured with 
either SCSI-1 or SCSI-2, but not both SCSI-1 and SCSI-2 simultaneously.

For more information about SCSI-1 functionality, refer to scsi1Lib. 
For more information about SCSI-2, refer to scsi2Lib.


INCLUDE FILES
scsiLib.h, scsi1Lib.h, scsi2Lib.h

SEE ALSO: dosFsLib, rt11FsLib, rawFsLib, scsi1Lib, scsi2Lib,
.pG "I/O System, Local File Systems"
*/

#include "vxWorks.h"
#include "ioLib.h"
#include "ctype.h"
#include "stdlib.h"
#include "errnoLib.h"
#include "taskLib.h"
#include "logLib.h"
#include "string.h"
#include "stdio.h"
#include "sysLib.h"
#include "scsiLib.h"

/* globals */

BOOL scsiErrors;			/* enable SCSI error messages */
BOOL scsiDebug;				/* enable task level debug messages */
BOOL scsiIntsDebug;			/* enable int level debug messages */
SCSI_FUNC_TBL * pScsiIfTbl;             /* pointer to scsiLib interface table */

    /* select timeout to use when creating a SCSI_PHYS_DEV */

UINT32 scsiSelectTimeout    = SCSI_DEF_SELECT_TIMEOUT;
int blkDevListMutexOptions  = SEM_Q_PRIORITY |
			      SEM_INVERSION_SAFE |
			      SEM_DELETE_SAFE;
int scsiCtrlMutexOptions    = SEM_Q_PRIORITY |
			      SEM_INVERSION_SAFE |
			      SEM_DELETE_SAFE;
int scsiCtrlSemOptions      = SEM_Q_FIFO;
int scsiPhysDevMutexOptions = SEM_Q_PRIORITY |
			      SEM_INVERSION_SAFE |
			      SEM_DELETE_SAFE;
int scsiPhysDevSemOptions   = SEM_Q_FIFO;

SCSI_CTRL * pSysScsiCtrl;		/* ptr to default SCSI_CTRL struct */


/******************************************************************************
*
* scsiCtrlInit - initialize generic fields of a SCSI_CTRL structure
*
* This routine should be called by the SCSI controller libraries' xxCtrlCreate
* routines, which are responsible for initializing any fields not herein
* initialized.  It is NOT intended to be called directly by a user.
*
* NOTE
* As a matter of good practice, the SCSI_CTRL structure should be
* calloc()'ed by the xxCtrlCreate() routine, so that all fields are
* initially zero.  If this is done, some of the work of this routine will be
* redundant.
*
* RETURNS: OK, or ERROR if a semaphore initialization fails.
*
* NOMANUAL
*/

STATUS scsiCtrlInit
    (
    SCSI_CTRL *pScsiCtrl        /* ptr to SCSI_CTRL struct to initialize */
    )
    {
    if (pScsiIfTbl->scsiCtrlInit != NULL)
	return ((STATUS) (pScsiIfTbl->scsiCtrlInit) (pScsiCtrl));
    else
	return (ERROR);

    }

/******************************************************************************
*
* scsiPhysDevDelete - delete a SCSI physical-device structure
*
* This routine deletes a specified SCSI physical-device structure.
*
* RETURNS: OK, or ERROR if `pScsiPhysDev' is NULL or SCSI_BLK_DEVs have
* been created on the device.
*/

STATUS scsiPhysDevDelete
    (
    SCSI_PHYS_DEV *pScsiPhysDev    /* ptr to SCSI physical device info */
    )
    {
    if (pScsiIfTbl->scsiPhysDevDelete != NULL)
	return ((STATUS) (pScsiIfTbl->scsiPhysDevDelete) (pScsiPhysDev));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiPhysDevCreate - create a SCSI physical device structure
*
* This routine enables access to a SCSI device and must be the first routine
* invoked. It must be called once for each physical device on the SCSI bus.
*
* If <reqSenseLength> is NULL (0), one or more REQUEST_SENSE
* commands are issued to the device to determine the number of bytes of
* sense data it typically returns.  Note that if the device returns variable
* amounts of sense data depending on its state, you must consult the device
* manual to determine the maximum amount of sense data that can be
* returned.
*
* If <devType> is NONE (-1), an INQUIRY command is issued to
* determine the device type; as an added benefit, it acquires the device's
* make and model number.  The scsiShow() routine displays this information.
* Common values of <devType> can be found in scsiLib.h or in the SCSI
* specification.
*
* If <numBlocks> or <blockSize> are specified as NULL (0), a READ_CAPACITY
* command is issued to determine those values.  This occurs
* only for device types that support READ_CAPACITY.
*
* RETURNS: A pointer to the created SCSI_PHYS_DEV structure, or NULL if the
* routine is unable to create the physical-device structure.
*/

SCSI_PHYS_DEV * scsiPhysDevCreate
    (
    SCSI_CTRL * pScsiCtrl, /* ptr to SCSI controller info */
    int  devBusId,         /* device's SCSI bus ID */
    int  devLUN,           /* device's logical unit number */
    int  reqSenseLength,   /* length of REQUEST SENSE data dev returns */
    int  devType,          /* type of SCSI device */
    BOOL removable,        /* whether medium is removable */
    int  numBlocks,        /* number of blocks on device */
    int  blockSize         /* size of a block in bytes */
    )
    {
    if (pScsiIfTbl->scsiPhysDevCreate != NULL)
	return ((SCSI_PHYS_DEV *) (pScsiIfTbl->scsiPhysDevCreate) 
	        (pScsiCtrl, devBusId, devLUN, reqSenseLength, devType, 
					      removable, numBlocks, blockSize));
    else
	return (NULL);

    }

/*******************************************************************************
*
* scsiPhysDevIdGet - return a pointer to a SCSI_PHYS_DEV structure
*
* This routine returns a pointer to the SCSI_PHYS_DEV structure of the SCSI
* physical device located at a specified bus ID (<devBusId>) and logical
* unit number (<devLUN>) and attached to a specified SCSI controller
* (<pScsiCtrl>).
*
* RETURNS: A pointer to the specified SCSI_PHYS_DEV structure, or NULL if the
* structure does not exist.
*/

SCSI_PHYS_DEV * scsiPhysDevIdGet
    (
    SCSI_CTRL * pScsiCtrl,       /* ptr to SCSI controller info  */
    int         devBusId,        /* device's SCSI bus ID         */
    int         devLUN           /* device's logical unit number */
    )
    {
    if (pScsiIfTbl->scsiPhysDevIdGet != NULL)
	return ((SCSI_PHYS_DEV *) (pScsiIfTbl->scsiPhysDevIdGet) 
						(pScsiCtrl, devBusId, devLUN));
    else
	return (NULL);

    }

/*******************************************************************************
*
* scsiAutoConfig - configure all devices connected to a SCSI controller
*
* This routine cycles through all valid SCSI bus IDs and logical unit
* numbers (LUNs), attempting a scsiPhysDevCreate() with default parameters
* on each.  All devices which support the INQUIRY command are
* configured.  The scsiShow() routine can be used to find the system table
* of SCSI physical devices attached to a specified SCSI controller.  In
* addition, scsiPhysDevIdGet() can be used programmatically to get a
* pointer to the SCSI_PHYS_DEV structure associated with the device at a
* specified SCSI bus ID and LUN.
*
* RETURNS: OK, or ERROR if <pScsiCtrl> and the global variable `pSysScsiCtrl'
* are both NULL.
*/

STATUS scsiAutoConfig
    (
    SCSI_CTRL *pScsiCtrl       /* ptr to SCSI controller info */
    )
    {
    if (pScsiIfTbl->scsiAutoConfig != NULL)
	return ((STATUS) (pScsiIfTbl->scsiAutoConfig) (pScsiCtrl));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiShow - list the physical devices attached to a SCSI controller
*
* This routine displays the SCSI bus ID, logical unit number (LUN), vendor ID,
* product ID, firmware revision (rev.), device type, number of blocks,
* block size in bytes, and a pointer to the associated SCSI_PHYS_DEV
* structure for each physical SCSI device known to be attached to a specified
* SCSI controller.
*
* NOTE:
* If <pScsiCtrl> is NULL, the value of the global variable `pSysScsiCtrl'
* is used, unless it is also NULL.
*
* RETURNS: OK, or ERROR if both <pScsiCtrl> and `pSysScsiCtrl' are NULL.
*/

STATUS scsiShow
    (
    SCSI_CTRL *pScsiCtrl           /* ptr to SCSI controller info */
    )
    {
    if (pScsiIfTbl->scsiShow != NULL)
	return ((STATUS) (pScsiIfTbl->scsiShow) (pScsiCtrl));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiBlkDevCreate - define a logical partition on a SCSI block device
*
* This routine creates and initializes a BLK_DEV structure, which
* describes a logical partition on a SCSI physical-block device.  A logical
* partition is an array of contiguously addressed blocks; it can be completely
* described by the number of blocks and the address of the first block in
* the partition.  In normal configurations partitions do not overlap, although
* such a condition is not an error.
*
* NOTE:
* If <numBlocks> is 0, the rest of device is used.
*
* RETURNS: A pointer to the created BLK_DEV, or NULL if parameters exceed
* physical device boundaries, if the physical device is not a block device, or
* if memory is insufficient for the structures.
*/

BLK_DEV * scsiBlkDevCreate
    (
    SCSI_PHYS_DEV * pScsiPhysDev,/* ptr to SCSI physical device info */
    int             numBlocks,   /* number of blocks in block device */
    int             blockOffset  /* address of first block in volume */
    )
    {
    if (pScsiIfTbl->scsiBlkDevCreate != NULL)
	return ((BLK_DEV *) (pScsiIfTbl->scsiBlkDevCreate) (pScsiPhysDev, 
	                                              numBlocks, blockOffset));
    else
	return (NULL);

    }

/*******************************************************************************
*
* scsiBlkDevInit - initialize fields in a SCSI logical partition
*
* This routine specifies the disk-geometry parameters required by certain
* file systems (for example, dosFs).  It is called after a SCSI_BLK_DEV
* structure is created with scsiBlkDevCreate(), but before calling a file system
* initialization routine.  It is generally required only for removable-media
* devices.
*
* RETURNS: N/A
*/

void scsiBlkDevInit
    (
    SCSI_BLK_DEV * pScsiBlkDev, /* ptr to SCSI block dev. struct */
    int            blksPerTrack,/* blocks per track */
    int            nHeads       /* number of heads */
    )
    {
    if (pScsiIfTbl->scsiBlkDevInit != NULL)
	(pScsiIfTbl->scsiBlkDevInit) (pScsiBlkDev, blksPerTrack, nHeads);

    }

/*******************************************************************************
*
* scsiBlkDevShow - show the BLK_DEV structures on a specified physical device
*
* This routine displays all of the BLK_DEV structures created on a specified
* physical device.  This routine is called by scsiShow() but may also be
* invoked directly, usually from the shell.
*
* RETURNS: N/A
*
* SEE ALSO: scsiShow()
*/

void scsiBlkDevShow
    (
    SCSI_PHYS_DEV * pScsiPhysDev  /* ptr to SCSI physical device info */
    )
    {
    if (pScsiIfTbl->scsiBlkDevShow != NULL)
        (pScsiIfTbl->scsiBlkDevShow) (pScsiPhysDev);

    }

/*******************************************************************************
*
* scsiBusReset - pulse the reset signal on the SCSI bus
*
* This routine calls a controller-specific routine to reset a specified
* controller's SCSI bus.  If no controller is specified (<pScsiCtrl> is 0),
* the value in the global variable `pSysScsiCtrl' is used.
*
* RETURNS: OK, or ERROR if there is no controller or controller-specific
* routine.
*/

STATUS scsiBusReset
    (
    SCSI_CTRL * pScsiCtrl	/* ptr to SCSI controller info */
    )
    {
    if (pScsiIfTbl->scsiBusReset != NULL)
	return ((STATUS) (pScsiIfTbl->scsiBusReset) (pScsiCtrl));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiPhaseNameGet - get the name of a specified SCSI phase
*
* This routine returns a pointer to a string which is the name of the SCSI
* phase input as an integer.  It's primarily used to improve readability of
* debugging messages.
*
* RETURNS: A pointer to a string naming the SCSI phase input
*
* NOMANUAL
*/

char * scsiPhaseNameGet
    (
    int scsiPhase               /* phase whose name to look up */
    )
    {
    if (pScsiIfTbl->scsiPhaseNameGet != NULL)
	return ((char *) (pScsiIfTbl->scsiPhaseNameGet) (scsiPhase));
    else
	return (NULL);

    }

/*******************************************************************************
*
* scsiCmdBuild - fills in the fields of a SCSI command descriptor block
*
* Typically, this routine is not called directly by the user, but by other
* routines in scsiLib.  It fills in fields of a SCSI-command descriptor block
* based on the input parameters.  The field layouts vary based on the command
* group, which is determined from the `opCode'.
*
* RETURNS: ERROR if vendor-unique command group or out-of-bounds parameter,
* otherwise OK.
*
* NOMANUAL
*/

STATUS scsiCmdBuild
    (
    SCSI_COMMAND scsiCmd,       /* command to be built */
    int *        pCmdLength,    /* ptr to command length variable */
    UINT8        opCode,        /* SCSI opCode for command */
    int          LUN,           /* logical unit number for command */
    BOOL         relAdrs,       /* whether to set relative address bit */
    int          logBlockAdrs,  /* logical block address */
    int          xferLength,    /* number of blocks or bytes to xfer */
    UINT8        controlByte    /* control byte for command */
    )
    {
    if (pScsiIfTbl->scsiCmdBuild != NULL)
	return ((STATUS) (pScsiIfTbl->scsiCmdBuild) (scsiCmd, pCmdLength, 
		opCode, LUN, relAdrs, logBlockAdrs, xferLength, controlByte));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiTransact - obtain exclusive use of SCSI controller for a transaction
*
* This routine calls scsiPhaseSequence() to execute the command specified.
* If there are physical path management errors, then this routine returns
* ERROR.  If not, then the status returned from the command is checked.  If
* it is "Check Condition", then a "Request Sense" CCS command is executed
* and the sense key is examined.  An indication of the success of the
* command is returned (OK or ERROR).
*
* RETURNS: OK, or ERROR if a path management error occurs
* or the status or sense information indicates an error.
*
* NOMANUAL
*/

STATUS scsiTransact
    (
    SCSI_PHYS_DEV *     pScsiPhysDev,        /* ptr to the target device    */
    SCSI_TRANSACTION  * pScsiXaction      /* ptr to the transaction info */
    )
    {
    if (pScsiIfTbl->scsiTransact != NULL)
	return ((STATUS) (pScsiIfTbl->scsiTransact) (pScsiPhysDev, 
								pScsiXaction));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiIoctl - perform a device-specific I/O control function
*
* This routine performs a specified `ioctl' function using a specified SCSI 
* block device.
*
* RETURNS: The status of the request, or ERROR if the request is unsupported.
*/

STATUS scsiIoctl
    (
    SCSI_PHYS_DEV * pScsiPhysDev,/* ptr to SCSI block device info */
    int             function,    /* function code */
    int             arg          /* argument to pass called function */
    )
    {
    if (pScsiIfTbl->scsiIoctl != NULL)
	return ((STATUS) (pScsiIfTbl->scsiIoctl) (pScsiPhysDev, function, arg));
    else
	return (ERROR);

    }


/*******************************************************************************
*
* scsiFormatUnit - issue a FORMAT_UNIT command to a SCSI device
*
* This routine issues a FORMAT_UNIT command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiFormatUnit
    (
    SCSI_PHYS_DEV * pScsiPhysDev,  /* ptr to SCSI physical device */
    BOOL            cmpDefectList, /* whether defect list is complete */
    int             defListFormat, /* defect list format */
    int             vendorUnique,  /* vendor unique byte */
    int             interleave,    /* interleave factor */
    char *          buffer,        /* ptr to input data buffer */
    int             bufLength      /* length of buffer in bytes */
    )
    {
    if (pScsiIfTbl->scsiFormatUnit != NULL)
	return ((STATUS) (pScsiIfTbl->scsiFormatUnit) (pScsiPhysDev, 
		cmpDefectList, defListFormat, vendorUnique, interleave, 
							buffer, bufLength));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiModeSelect - issue a MODE_SELECT command to a SCSI device
*
* This routine issues a MODE_SELECT command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiModeSelect
    (
    SCSI_PHYS_DEV * pScsiPhysDev, /* ptr to SCSI physical device            */
    int             pageFormat,   /* value of the page format bit (0-1)     */
    int             saveParams,   /* value of the save parameters bit (0-1) */
    char *          buffer,       /* ptr to output data buffer              */
    int             bufLength     /* length of buffer in bytes              */
    )
    {
    if (pScsiIfTbl->scsiModeSelect != NULL)
	return ((STATUS) (pScsiIfTbl->scsiModeSelect) (pScsiPhysDev, pageFormat,
		                               saveParams, buffer, bufLength));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiModeSense - issue a MODE_SENSE command to a SCSI device
*
* This routine issues a MODE_SENSE command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiModeSense
    (
    SCSI_PHYS_DEV * pScsiPhysDev, /* ptr to SCSI physical device */
    int             pageControl,  /* value of the page control field (0-3) */
    int             pageCode,     /* value of the page code field (0-0x3f) */
    char *          buffer,       /* ptr to input data buffer */
    int             bufLength     /* length of buffer in bytes */
    )
    {
    if (pScsiIfTbl->scsiModeSense != NULL)
	return ((STATUS) (pScsiIfTbl->scsiModeSense) (pScsiPhysDev, pageControl,
		                               pageCode, buffer, bufLength));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiReadCapacity - issue a READ_CAPACITY command to a SCSI device
*
* This routine issues a READ_CAPACITY command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiReadCapacity
    (
    SCSI_PHYS_DEV * pScsiPhysDev, /* ptr to SCSI physical device */
    int *           pLastLBA,   /* where to return last logical block address */
    int *           pBlkLength    /* where to return block length */
    )
    {
    if (pScsiIfTbl->scsiReadCapacity != NULL)
	return ((STATUS) (pScsiIfTbl->scsiReadCapacity) (pScsiPhysDev, pLastLBA,
		                                                   pBlkLength));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiRdSecs - read sector(s) from a SCSI block device
*
* This routine reads the specified physical sector(s) from a specified
* physical device.
*
* RETURNS: OK, or ERROR if the sector(s) cannot be read.
*/

STATUS scsiRdSecs
    (
    SCSI_BLK_DEV * pScsiBlkDev,  /* ptr to SCSI block device info */
    int             sector,       /* sector number to be read */
    int             numSecs,      /* total sectors to be read */
    char *          buffer        /* ptr to input data buffer */
    )
    {
    if (pScsiIfTbl->scsiRdSecs != NULL)
	return ((STATUS) (pScsiIfTbl->scsiRdSecs) (pScsiBlkDev, sector, numSecs,
		                                                      buffer));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiWrtSecs - write sector(s) to a SCSI block device
*
* This routine writes the specified physical sector(s) to a specified physical
* device.
*
* RETURNS: OK, or ERROR if the sector(s) cannot be written.
*/

STATUS scsiWrtSecs
    (
    SCSI_BLK_DEV * pScsiBlkDev,  /* ptr to SCSI block device info */
    int             sector,       /* sector number to be written */
    int             numSecs,      /* total sectors to be written */
    char *          buffer        /* ptr to input data buffer */
    )
    {
    if (pScsiIfTbl->scsiWrtSecs != NULL)
	return ((STATUS) (pScsiIfTbl->scsiWrtSecs) (pScsiBlkDev, sector, 
							numSecs, buffer));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiTestUnitRdy - issue a TEST_UNIT_READY command to a SCSI device
*
* This routine issues a TEST_UNIT_READY command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiTestUnitRdy
    (
    SCSI_PHYS_DEV * pScsiPhysDev         /* ptr to SCSI physical device */
    )
    {
    if (pScsiIfTbl->scsiTestUnitRdy != NULL)
	return ((STATUS) (pScsiIfTbl->scsiTestUnitRdy) (pScsiPhysDev));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiInquiry - issue an INQUIRY command to a SCSI device
*
* This routine issues an INQUIRY command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiInquiry
    (
    SCSI_PHYS_DEV * pScsiPhysDev, /* ptr to SCSI physical device */
    char *          buffer,       /* ptr to input data buffer */
    int             bufLength     /* length of buffer in bytes */
    )
    {
    if (pScsiIfTbl->scsiInquiry != NULL)
	return ((STATUS) (pScsiIfTbl->scsiInquiry) (pScsiPhysDev, buffer, 
								bufLength));
    else
	return (ERROR);

    }

/*******************************************************************************
*
* scsiReqSense - issue a REQUEST_SENSE command to a SCSI device and read results
*
* This routine issues a REQUEST_SENSE command to a specified SCSI device and
* reads the results.
*
* RETURNS: OK, or ERROR if the command fails.
*/

STATUS scsiReqSense
    (
    SCSI_PHYS_DEV * pScsiPhysDev, /* ptr to SCSI physical device */
    char *          buffer,       /* ptr to input data buffer */
    int             bufLength     /* length of buffer in bytes */
    )
    {
    if (pScsiIfTbl->scsiReqSense != NULL)
	return ((STATUS) (pScsiIfTbl->scsiReqSense) (pScsiPhysDev, buffer, 
								   bufLength));
    else
	return (ERROR);

    }

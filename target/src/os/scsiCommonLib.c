/* scsiCommonLib.c - SCSI library common commands for all devices (SCSI-2) */

/* Copyright 1989-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,06may96,jds  yet more doc tweaks...
01c,13nov95,jds  more doc tweaks
01b,20sep95,jdi  doc tweak.
01a,24jul95,jds  written by extracting from scsi2Lib.
*/

/*
DESCRIPTION
This library contains commands common to all SCSI devices. The content of 
this library is separated from the other SCSI libraries in order to create
an additional layer for better support of all SCSI devices.

Commands in this library include:

.TS
tab(|);
lf3 lf3
l l.
Command         | Op Code
_
INQUIRY         | (0x12)
REQUEST SENSE   | (0x03)
TEST UNIT READY | (0x00)
.TE

INCLUDE FILES
scsiLib.h, scsi2Lib.h

SEE ALSO: dosFsLib, rt11FsLib, rawFsLib, tapeFsLib, scsi2Lib,
.pG "I/O System, Local File Systems"
*/

#define  INCLUDE_SCSI2
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


/* forward static functions */

void scsiCommonLibTblInit ();


/*
 * Backward compatability functions localised
 */

LOCAL STATUS          scsi2TestUnitRdy (SCSI_PHYS_DEV *);
LOCAL STATUS          scsi2Inquiry (SCSI_PHYS_DEV *, char *, int);
LOCAL STATUS          scsi2ReqSense (SCSI_PHYS_DEV *, char *, int);


/*******************************************************************************
*
* scsiCommonLibTblInit -
*
* Initialises the SCSI common commands interface.
*
* NOMANUAL
*/
void scsiCommonLibTblInit ()
    {
    pScsiIfTbl->scsiTestUnitRdy   = (FUNCPTR) scsi2TestUnitRdy;
    pScsiIfTbl->scsiInquiry       = (FUNCPTR) scsi2Inquiry;
    pScsiIfTbl->scsiReqSense      = (FUNCPTR) scsi2ReqSense;
    }


/*******************************************************************************
*
* scsi2TestUnitRdy - issue a TEST_UNIT_READY command to a SCSI device
*
* This routine issues a TEST_UNIT_READY command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi2TestUnitRdy
    (
    SCSI_PHYS_DEV *pScsiPhysDev         /* ptr to SCSI physical device */
    )
    {
    SCSI_COMMAND testUnitRdyCommand;	/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */

    SCSI_DEBUG_MSG ("scsiTestUnitRdy:\n", 0, 0, 0, 0, 0, 0);

    if (scsiCmdBuild (testUnitRdyCommand, &scsiXaction.cmdLength,
	SCSI_OPCODE_TEST_UNIT_READY, pScsiPhysDev->scsiDevLUN, FALSE,
	0, 0, (UINT8) 0)
	== ERROR)
    	return (ERROR);

    scsiXaction.cmdAddress    = testUnitRdyCommand;
    scsiXaction.dataAddress   = NULL;
    scsiXaction.dataDirection = NONE;
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
* scsi2Inquiry - issue an INQUIRY command to a SCSI device
*
* This routine issues an INQUIRY command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi2Inquiry
    (
    SCSI_PHYS_DEV *pScsiPhysDev,/* ptr to SCSI physical device */
    char *buffer,               /* ptr to input data buffer */
    int bufLength               /* length of buffer in bytes */
    )
    {
    SCSI_COMMAND inquiryCommand;	/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */

    SCSI_DEBUG_MSG ("scsiInquiry:\n", 0, 0, 0, 0, 0, 0);

    if (scsiCmdBuild (inquiryCommand, &scsiXaction.cmdLength,
	SCSI_OPCODE_INQUIRY, pScsiPhysDev->scsiDevLUN, FALSE,
	0, bufLength, (UINT8) 0)
	== ERROR)
    	return (ERROR);

    scsiXaction.cmdAddress    = inquiryCommand;
    scsiXaction.dataAddress   = (UINT8 *) buffer;
    scsiXaction.dataDirection = O_RDONLY;
    scsiXaction.dataLength    = bufLength;
    scsiXaction.addLengthByte = INQUIRY_ADD_LENGTH_BYTE;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
	    (pScsiPhysDev, &scsiXaction));
    }

/*******************************************************************************
*
* scsi2ReqSense - issue a REQUEST_SENSE command to a device and read the results
*
* This routine issues a REQUEST_SENSE command to a specified SCSI device and
* read the results.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi2ReqSense
    (
    SCSI_PHYS_DEV *pScsiPhysDev,/* ptr to SCSI physical device */
    char *buffer,               /* ptr to input data buffer */
    int bufLength               /* length of buffer in bytes */
    )
    {
    SCSI_COMMAND reqSenseCommand;	/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */

    SCSI_DEBUG_MSG ("scsiReqSense:\n", 0, 0, 0, 0, 0, 0);

    if (scsiCmdBuild (reqSenseCommand, &scsiXaction.cmdLength,
		      SCSI_OPCODE_REQUEST_SENSE, pScsiPhysDev->scsiDevLUN,
		      FALSE, 0, bufLength, (UINT8) 0)
        == ERROR)
        return (ERROR);

    scsiXaction.cmdAddress    = reqSenseCommand;
    scsiXaction.dataAddress   = (UINT8 *) buffer;
    scsiXaction.dataDirection = O_RDONLY;
    scsiXaction.dataLength    = bufLength;
    scsiXaction.addLengthByte = REQ_SENSE_ADD_LENGTH_BYTE;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
	    (pScsiPhysDev, &scsiXaction));
    }

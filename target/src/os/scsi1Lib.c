/* scsi1Lib.c - Small Computer System Interface (SCSI) library (SCSI-1) */

/* Copyright 1989-1994 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02e,09jul97,dgp  doc: correct fonts per SPR 7853
02d,29oct96,dgp  doc: editing for newly published SCSI libraries
02c,06may96,jds  and more doc fixes...
02b,01may96,jds  yet more doc fixes...
02a,13nov95,jds  more doc tweaks
01z,08nov95,jds  doc tweaks
01y,10oct94,jds  fixed for SCSI1 and SCSI2 backward compatability
01x,28feb93,jdi  doc: changed comment in scsiWrtSecs() from "read" to "write".
01w,27nov92,jdi  documentation cleanup, including SPR 1415.
01v,15sep92,ccc  reworked select timeout, fixed extended message to work
		 with new drive, documentation changes.
01u,10aug92,ccc  added timeouts to scsiXaction for each scsi command.
01t,28jul92,rrr  fixed decl of scsiSyncTarget
01s,22jul92,gae  added correct number of parameters to logMsg().
01r,20jul92,eve  Remove conditional INCLUDE_SYNC_SCSI compilation.
01q,18jul92,smb  Changed errno.h to errnoLib.h.
01p,13jul92,eve  added init of the current pScsiXaction in scsiPhaseSequence().
01o,08jul92,eve  supplies a reqSense buffer if there is no user buffer 
		 in scsiTransact routine.
01n,03jul92,eve  added new sync feature with NOMANUAL
01m,26may92,rrr  the tree shuffle
01l,16dec91,gae  added includes for ANSI; added parameters to logMsg() calls.
01k,19nov91,rrr  shut up some ansi warnings.
01j,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY and ...
		  -changed VOID to void
		  -changed copyright notice
01i,13mar91,jcc  misc. clean-up and enhancements.
01h,25oct90,jcc  lint.
01g,18oct90,jcc  removed call to printErrno() in scsiAutoConfig().
01f,02oct90,jcc  UTINY became UINT8; added scsiAutoConfig(), scsiPhysDevIdGet(),
		 and scsiIoctl() option FIODISKFORMAT;
		 changed all malloc()'s to calloc()'s in xxCreate() routines;
		 made changes associated with SEM_ID's becoming SEMAPHORE's
		 in created structures;
		 enhanced scsiPhysDevCreate() to determine amount and type
		 of REQUEST SENSE data returned by the device; miscellaneous.
01e,10aug90,dnw  fixed warnings caused by changing scsiBusReset to VOIDFUNCPTR.
01d,18jul90,jcc  made semTake() calls 5.0 compatible; clean-up.
01c,08jun90,jcc  modified incoming message handling; generalized scsiTransact
		 calls to a procedure variable to allow off-board SCSI
		 controller drivers; documentation.
01b,23mar90,jcc  lint.
01a,05may89,jcc  written.
*/

/*
DESCRIPTION
This library implements the Small Computer System Interface (SCSI)
protocol in a controller-independent manner.  It implements only the SCSI
initiator function; the library does not support a VxWorks target
acting as a SCSI target.  Furthermore, in the current implementation, a
VxWorks target is assumed to be the only initiator on the SCSI bus,
although there may be multiple targets (SCSI peripherals) on the bus.

The implementation is transaction based.  A transaction is defined as the
selection of a SCSI device by the initiator, the issuance of a SCSI command,
and the sequence of data, status, and message phases necessary to perform
the command.  A transaction normally completes with a "Command Complete" 
message from
the target, followed by disconnection from the SCSI bus.  If
the status from the target is "Check Condition," the transaction continues;
the initiator issues a "Request Sense" command to gain more information
on the exception condition reported.

Many of the subroutines in scsi1Lib facilitate the transaction of
frequently used SCSI commands.  Individual command fields are passed as
arguments from which SCSI Command Descriptor Blocks are constructed, and
fields of a SCSI_TRANSACTION structure are filled in appropriately.  This
structure, along with the SCSI_PHYS_DEV structure associated with the
target SCSI device, is passed to the routine whose address is indicated by
the 'scsiTransact' field of the SCSI_CTRL structure associated with the
relevant SCSI controller.

The function variable 'scsiTransact' is set by the individual SCSI
controller driver.  For off-board SCSI controllers, this
routine rearranges the fields of the SCSI_TRANSACTION structure into
the appropriate structure for the specified hardware, which then carries out
the transaction through firmware control.  Drivers for an on-board
SCSI-controller chip can use the scsiTransact() routine in scsiLib (which
invokes the scsi1Transact() routine in scsi1Lib), as long as they provide the 
other functions specified in the SCSI_CTRL structure.

Note that no disconnect/reconnect capability is currently supported.

SUPPORTED SCSI DEVICES
The scsi1Lib library supports use of SCSI peripherals conforming to the 
standards specified in 
.I "Common Command Set (CCS) of the SCSI, Rev. 4.B."
Most SCSI peripherals currently offered support CCS.  
While an attempt has been made
to have scsi1Lib support non-CCS peripherals, not all commands or features
of this library are guaranteed to work with them.  For example,
auto-configuration may be impossible with non-CCS devices, if they do not
support the INQUIRY command.

Not all classes of SCSI devices are supported.  However, the scsiLib library
provides the capability to transact any SCSI command on any SCSI device
through the FIOSCSICOMMAND function of the scsiIoctl() routine.

Only direct-access devices (disks) are supported by a file system.  For
other types of devices, additional, higher-level software is necessary to
map user-level commands to SCSI transactions.

CONFIGURING SCSI CONTROLLERS
The routines to create and initialize a specific SCSI controller are
particular to the controller and normally are found in its library
module.  The normal calling sequence is:
.ne 4
.CS
    xxCtrlCreate (...); /@ parameters are controller specific @/
    xxCtrlInit (...);   /@ parameters are controller specific @/
.CE
The conceptual difference between the two routines is that xxCtrlCreate()
calloc's memory for the xx_SCSI_CTRL data structure and initializes
information that is never expected to change (for example, clock rate).  The
remaining fields in the xx_SCSI_CTRL structure are initialized by
xxCtrlInit() and any necessary registers are written on the SCSI
controller to effect the desired initialization.  This
routine can be called multiple times, although this is rarely required.
For example, the bus ID of the SCSI
controller can be changed without rebooting the VxWorks system.

CONFIGURING PHYSICAL SCSI DEVICES
Before a device can be used, it must be "created," that is, declared.
This is done with scsiPhysDevCreate() and can only be done after a
SCSI_CTRL structure exists and has been properly initialized.

.CS
SCSI_PHYS_DEV *scsiPhysDevCreate
    (
    SCSI_CTRL * pScsiCtrl,/@ ptr to SCSI controller info @/
    int  devBusId,        /@ device's SCSI bus ID @/
    int  devLUN,          /@ device's logical unit number @/
    int  reqSenseLength,  /@ length of REQUEST SENSE data dev returns @/
    int  devType,         /@ type of SCSI device @/
    BOOL removable,       /@ whether medium is removable @/
    int  numBlocks,       /@ number of blocks on device @/
    int  blockSize        /@ size of a block in bytes @/
    )
.CE

Several of these parameters can be left unspecified, as follows:
.iP <reqSenseLength>
If 0, issue a REQUEST_SENSE to determine a request sense length.
.iP <devType>
If -1, issue an INQUIRY to determine the device type.
.iP "<numBlocks>, <blockSize>"
If 0, issue a READ_CAPACITY to determine the number of blocks.
.LP
The above values are recommended, unless the device does not support the
required commands, or other non-standard conditions prevail.

LOGICAL PARTITIONS ON BLOCK DEVICES
It is possible to have more than one logical partition on a SCSI block
device.  This capability is currently not supported for removable media
devices.  A partition is an array of contiguously addressed blocks
with a specified starting block address and a specified number of blocks.
The scsiBlkDevCreate() routine is called once for each block device
partition.  Under normal usage, logical partitions should not overlap.
.ne 8
.CS
SCSI_BLK_DEV *scsiBlkDevCreate
    (
    SCSI_PHYS_DEV *  pScsiPhysDev,  /@ ptr to SCSI physical device info @/
    int              numBlocks,     /@ number of blocks in block device @/
    int              blockOffset    /@ address of first block in volume @/
    )
.CE

Note that if <numBlocks> is 0, the rest of the device is used.

ATTACHING FILE SYSTEMS TO LOGICAL PARTITIONS
Files cannot be read or written to a disk partition until a file system
(such as dosFs or rt11Fs) has been initialized on the partition.  For more
information, see the documentation in dosFsLib or rt11FsLib.

TRANSMITTING ARBITRARY COMMANDS TO SCSI DEVICES
The scsi1Lib library provides routines that implement many common SCSI
commands.  Still, there are situations that require commands that are not
supported by scsi1Lib (for example, writing software to control non-direct 
access devices).  Arbitrary commands are handled with the FIOSCSICOMMAND 
option to scsiIoctl().  The <arg> parameter for FIOSCSICOMMAND is a pointer to 
a valid SCSI_TRANSACTION structure.  Typically, a call to scsiIoctl() is 
written as a subroutine of the form:
.CS
STATUS myScsiCommand
    (
    SCSI_PHYS_DEV *  pScsiPhysDev,  /@ ptr to SCSI physical device     @/
    char *           buffer,        /@ ptr to data buffer              @/
    int              bufLength,     /@ length of buffer in bytes       @/
    int              someParam      /@ param. specifiable in cmd block @/
    )

    {
    SCSI_COMMAND myScsiCmdBlock;        /@ SCSI command byte array @/
    SCSI_TRANSACTION myScsiXaction;     /@ info on a SCSI transaction @/

    /@ fill in fields of SCSI_COMMAND structure @/

    myScsiCmdBlock [0] = MY_COMMAND_OPCODE;     /@ the required opcode @/
    .
    myScsiCmdBlock [X] = (UINT8) someParam;     /@ for example @/
    .
    myScsiCmdBlock [N-1] = MY_CONTROL_BYTE;     /@ typically == 0 @/

    /@ fill in fields of SCSI_TRANSACTION structure @/

    myScsiXaction.cmdAddress    = myScsiCmdBlock;
    myScsiXaction.cmdLength     = <# of valid bytes in myScsiCmdBlock>;
    myScsiXaction.dataAddress   = (UINT8 *) buffer;
    myScsiXaction.dataDirection = <O_RDONLY (0) or O_WRONLY (1)>;
    myScsiXaction.dataLength    = bufLength;
    myScsiXaction.cmdTimeout    = timeout in usec;

    /@ if dataDirection is O_RDONLY, and the length of the input data is
     * variable, the following parameter specifies the byte # (min == 0)
     * of the input data which will specify the additional number of
     * bytes available
     @/

    myScsiXaction.addLengthByte = X;

    if (scsiIoctl (pScsiPhysDev, FIOSCSICOMMAND, &myScsiXaction) == OK)
        return (OK);
    else
        /@ optionally perform retry or other action based on value of
         *  myScsiXaction.statusByte
         @/
        return (ERROR);
    }
.CE

INCLUDE FILES
scsiLib.h, scsi1Lib.h

SEE ALSO: dosFsLib, rt11FsLib,
.I  "American National Standards for Information Systems - Small Computer" 
.I  "System Interface (SCSI), ANSI X3.131-1986,"
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
#include "scsiLib.h"

/* forward static functions */

LOCAL BOOL            strIsPrintable (char *);
LOCAL STATUS          scsiDevSelect (SCSI_PHYS_DEV *, SCSI_TRANSACTION *);
LOCAL STATUS          scsiStatusCheck (BLK_DEV *);
LOCAL STATUS          scsiPhaseSequence (SCSI_PHYS_DEV *, SCSI_TRANSACTION *);
LOCAL STATUS          scsiBlkDevIoctl (SCSI_BLK_DEV *, int, int);
LOCAL STATUS          scsiSyncTarget (SCSI_PHYS_DEV *, int, int, 
						       SCSI_SYNC_AGREEMENT *);
LOCAL STATUS          scsi1CtrlInit (SCSI_CTRL *);
LOCAL STATUS          scsi1TestUnitRdy (SCSI_PHYS_DEV *);
LOCAL SCSI_PHYS_DEV * scsi1PhysDevIdGet (SCSI_CTRL *, int, int);
LOCAL STATUS          scsi1ReqSense (SCSI_PHYS_DEV *, char *, int);
LOCAL STATUS          scsi1Inquiry (SCSI_PHYS_DEV *, char *, int);
LOCAL STATUS          scsi1ReadCapacity (SCSI_PHYS_DEV *, int *, int *);
LOCAL STATUS          scsi1RdSecs (SCSI_BLK_DEV *, int, int, char *);
LOCAL STATUS          scsi1WrtSecs (SCSI_BLK_DEV *, int, int, char *);
LOCAL STATUS          scsi1PhysDevDelete (FAST SCSI_PHYS_DEV *);
LOCAL SCSI_PHYS_DEV * scsi1PhysDevCreate (SCSI_CTRL *, int, int, int, int, BOOL,
                                        int, int);
LOCAL STATUS          scsi1AutoConfig (SCSI_CTRL *);
LOCAL BLK_DEV *       scsi1BlkDevCreate (SCSI_PHYS_DEV *, int, int);
LOCAL void            scsi1BlkDevInit (SCSI_BLK_DEV *, int, int);
LOCAL void            scsi1BlkDevShow (SCSI_PHYS_DEV *);
LOCAL STATUS          scsi1Show (FAST SCSI_CTRL *);
LOCAL STATUS          scsi1BusReset (SCSI_CTRL *);
LOCAL STATUS          scsi1CmdBuild (SCSI_COMMAND, int *, UINT8, int, BOOL, int,
         							int, UINT8);
LOCAL STATUS          scsi1Transact (SCSI_PHYS_DEV *, SCSI_TRANSACTION *);
LOCAL STATUS          scsi1Ioctl (SCSI_PHYS_DEV *, int, int);
LOCAL STATUS          scsi1FormatUnit (SCSI_PHYS_DEV *, BOOL, int, int, int, 
								char *,int);
LOCAL STATUS          scsi1ModeSelect (SCSI_PHYS_DEV *, int, int, char *, int);
LOCAL STATUS          scsi1ModeSense (SCSI_PHYS_DEV *, int, int, char *, int);
LOCAL char *          scsi1PhaseNameGet (int);

SCSI_FUNC_TBL scsi1IfTbl =
    {
    (FUNCPTR) scsi1CtrlInit,
    (FUNCPTR) scsi1BlkDevInit,
    (FUNCPTR) scsi1BlkDevCreate,
    (FUNCPTR) scsi1BlkDevShow,
    (FUNCPTR) scsi1PhaseNameGet,
    (FUNCPTR) scsi1PhysDevCreate,
    (FUNCPTR) scsi1PhysDevDelete,
    (FUNCPTR) scsi1PhysDevIdGet,
    (FUNCPTR) scsi1AutoConfig,
    (FUNCPTR) scsi1Show,
    (FUNCPTR) scsi1BusReset,
    (FUNCPTR) scsi1CmdBuild,
    (FUNCPTR) scsi1Transact,
    (FUNCPTR) scsi1Ioctl,
    (FUNCPTR) scsi1FormatUnit,
    (FUNCPTR) scsi1ModeSelect,
    (FUNCPTR) scsi1ModeSense,
    (FUNCPTR) scsi1ReadCapacity,
    (FUNCPTR) scsi1RdSecs,
    (FUNCPTR) scsi1WrtSecs,
    (FUNCPTR) scsi1TestUnitRdy,
    (FUNCPTR) scsi1Inquiry,
    (FUNCPTR) scsi1ReqSense
    };

/*******************************************************************************
*
* scsi1IfInit - initialize the SCSI1 interface to scsiLib
*
* NOMANUAL 
*/

void scsi1IfInit ()
    {
    pScsiIfTbl = &scsi1IfTbl;

    pScsiIfTbl->scsiCtrlInit      = (FUNCPTR) scsi1CtrlInit;
    pScsiIfTbl->scsiPhysDevDelete = (FUNCPTR) scsi1PhysDevDelete;
    pScsiIfTbl->scsiPhysDevCreate = (FUNCPTR) scsi1PhysDevCreate;
    pScsiIfTbl->scsiAutoConfig    = (FUNCPTR) scsi1AutoConfig;
    pScsiIfTbl->scsiBlkDevCreate  = (FUNCPTR) scsi1BlkDevCreate;
    pScsiIfTbl->scsiPhysDevIdGet  = (FUNCPTR) scsi1PhysDevIdGet;
    pScsiIfTbl->scsiBlkDevInit    = (FUNCPTR) scsi1BlkDevInit;
    pScsiIfTbl->scsiBlkDevShow    = (FUNCPTR) scsi1BlkDevShow;
    pScsiIfTbl->scsiShow          = (FUNCPTR) scsi1Show;
    pScsiIfTbl->scsiBusReset      = (FUNCPTR) scsi1BusReset;
    pScsiIfTbl->scsiCmdBuild      = (FUNCPTR) scsi1CmdBuild;
    pScsiIfTbl->scsiTransact      = (FUNCPTR) scsi1Transact;
    pScsiIfTbl->scsiIoctl         = (FUNCPTR) scsi1Ioctl;
    pScsiIfTbl->scsiFormatUnit    = (FUNCPTR) scsi1FormatUnit;
    pScsiIfTbl->scsiModeSelect    = (FUNCPTR) scsi1ModeSelect;
    pScsiIfTbl->scsiModeSense     = (FUNCPTR) scsi1ModeSense;
    pScsiIfTbl->scsiPhaseNameGet  = (FUNCPTR) scsi1PhaseNameGet;
    pScsiIfTbl->scsiReadCapacity  = (FUNCPTR) scsi1ReadCapacity;
    pScsiIfTbl->scsiRdSecs        = (FUNCPTR) scsi1RdSecs;
    pScsiIfTbl->scsiWrtSecs       = (FUNCPTR) scsi1WrtSecs;
    pScsiIfTbl->scsiTestUnitRdy   = (FUNCPTR) scsi1TestUnitRdy;
    pScsiIfTbl->scsiInquiry       = (FUNCPTR) scsi1Inquiry;
    pScsiIfTbl->scsiReqSense      = (FUNCPTR) scsi1ReqSense;
    }

/*******************************************************************************
*
* scsi1CtrlInit - initialize generic fields of a SCSI_CTRL structure
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

LOCAL STATUS scsi1CtrlInit
    (
    SCSI_CTRL *pScsiCtrl        /* ptr to SCSI_CTRL struct to initialize */
    )
    {
    int ix;			/* loop index */

    /* initialize controller mutual exclusion semaphore */

    if (semMInit (&pScsiCtrl->ctrlMutexSem, scsiCtrlMutexOptions) == ERROR)
        {
        printErr ("scsi1CtrlInit: semMInit of ctrlMutexSem failed.\n");
        return (ERROR);
        }

    /* initialize controller interrupt waiting semaphore */

    if (semBInit (&pScsiCtrl->ctrlSyncSem, scsiCtrlSemOptions,
		  SEM_EMPTY) == ERROR)
	{
	printErr ("scsi1CtrlInit: semBInit of ctrlSyncSem failed.\n");
	return (ERROR);
	}

    /* initialize the scsiBusReset to NULL (set by individual drivers) */

    pScsiCtrl->scsiBusReset = (VOIDFUNCPTR) NULL;

    /* initialize the scsiPriority to NONE (set by individual drivers) */

    pScsiCtrl->scsiPriority = NONE;

    /* initialize array of ptrs to SCSI_PHYS_DEV structures to NULL */

    for (ix = 0; ix < MAX_SCSI_PHYS_DEVS; ix++)
	{
	pScsiCtrl->physDevArr [ix] = (SCSI_PHYS_DEV *) NULL;
	}

    return (OK);
    }

/*******************************************************************************
*
* scsi1PhysDevDelete - delete a SCSI physical device structure
*
* This routine deletes a specified SCSI physical device structure.
*
* RETURNS: OK, or ERROR if `pScsiPhysDev' is NULL or SCSI_BLK_DEVs have
* been created on the device.
*/

LOCAL STATUS scsi1PhysDevDelete
    (
    FAST SCSI_PHYS_DEV *pScsiPhysDev    /* ptr to SCSI physical device info */
    )
    {
    FAST SCSI_CTRL *pScsiCtrl;
    STATUS status;

    if ((pScsiPhysDev == (SCSI_PHYS_DEV *) NULL) ||
	(pScsiPhysDev->pScsiBlkDev != (SCSI_BLK_DEV *) NULL))
	return (ERROR);

    /* Reset target if the sync capacity is existing */

    if ( pScsiPhysDev->syncXfer == TRUE )
        {
        SCSI_SYNC_AGREEMENT syncAgreement;

        /* Send a sync message for async protocol */
        status = scsiSyncTarget(pScsiPhysDev, pScsiPhysDev->syncXferPeriod
				, 0, &syncAgreement);

        /* Clear the next check status */
        status = scsiTestUnitRdy(pScsiPhysDev);
        }

    pScsiCtrl = pScsiPhysDev->pScsiCtrl;
    pScsiCtrl->physDevArr [(pScsiPhysDev->scsiDevBusId << 3) |
			   pScsiPhysDev->scsiDevLUN] = (SCSI_PHYS_DEV *) NULL;

    if (pScsiPhysDev->pReqSenseData != NULL)
        (void) free ((char *) pScsiPhysDev->pReqSenseData);

    (void) free ((char *) pScsiPhysDev);

    return (OK);
    }

/*******************************************************************************
*
* scsi1PhysDevCreate - create a SCSI physical device structure
*
* This routine enables access to a SCSI device and must be invoked first.
* It should be called once for each physical device on the SCSI bus.
*
* If `reqSenseLength' is specified as NULL (0), one or more REQUEST_SENSE
* commands are issued to the device to determine the number of bytes of
* sense data it typically returns.  Note that if the device returns variable
* amounts of sense data depending on its state, consult the device manual 
* to determine the maximum amount of sense data that can be returned.
*
* If `devType' is specified as NONE (-1), an INQUIRY command is issued to
* determine the device type, with the added benefit of acquiring the device's
* make and model number.  The scsiShow() routine displays this information.
* Common values of `devType' can be found in scsiLib.h or in the SCSI
* specification.
*
* If `numBlocks' or `blockSize' are specified as NULL (0), a READ_CAPACITY
* command is issued to determine those values.  This will occur
* only for device types which support READ_CAPACITY.
*
* RETURNS: A pointer to the created SCSI_PHYS_DEV structure, or NULL if the
* routine is unable to create the physical device structure.
*/

LOCAL SCSI_PHYS_DEV *scsi1PhysDevCreate
    (
    SCSI_CTRL *pScsiCtrl,       /* ptr to SCSI controller info */
    int devBusId,               /* device's SCSI bus ID */
    int devLUN,                 /* device's logical unit number */
    int reqSenseLength,         /* length of REQUEST SENSE data dev returns */
    int devType,                /* type of SCSI device */
    BOOL removable,             /* whether medium is removable */
    int numBlocks,              /* number of blocks on device */
    int blockSize               /* size of a block in bytes */
    )
    {
    SCSI_PHYS_DEV *pScsiPhysDev;	/* ptr to SCSI physical dev. struct */
					/* REQ SENSE data for auto-sizing */
    UINT8 reqSenseData [REQ_SENSE_ADD_LENGTH_BYTE + 1];
    SCSI_PHYS_DEV *pPhysDev;    /* use to check if it's already created */
     

    /* check bus ID and LUN are within limits */

    if ((devBusId < SCSI_MIN_BUS_ID) ||
	(devBusId > SCSI_MAX_BUS_ID) ||
	(devLUN < SCSI_MIN_LUN) ||
	(devLUN > SCSI_MAX_LUN))
	{
	errnoSet (S_scsiLib_ILLEGAL_PARAMETER);
	return ((SCSI_PHYS_DEV *) NULL);
	}


    /* Check if this device was already create and if it's support sync 
     * capacity.
     */

       pPhysDev = scsiPhysDevIdGet (pScsiCtrl,devBusId,devLUN);
		
    if (pPhysDev != (SCSI_PHYS_DEV *)NULL)
        {
        errnoSet (S_scsiLib_DEVICE_EXIST);
        return ((SCSI_PHYS_DEV *) NULL);
	}

    /* create a SCSI physical device structure */

    pScsiPhysDev = (SCSI_PHYS_DEV *) calloc (1, sizeof (*pScsiPhysDev));

    if (pScsiPhysDev == NULL)
        return ((SCSI_PHYS_DEV *) NULL);

    /* initialize device mutual exclusion semaphore */

    if (semMInit (&pScsiPhysDev->devMutexSem, scsiPhysDevMutexOptions) == ERROR)
        {
        SCSI_DEBUG_MSG ("scsiPhysDevCreate: semMInit of devMutexSem failed.\n",
			0, 0, 0, 0, 0, 0);
	(void) free ((char *) pScsiPhysDev);
        return ((SCSI_PHYS_DEV *) NULL);
        }

    /* initialize device interrupt waiting semaphore */

    if (semBInit (&pScsiPhysDev->devSyncSem, scsiPhysDevSemOptions,
		  SEM_EMPTY) == ERROR)
        {
        SCSI_DEBUG_MSG ("scsiPhysDevCreate: semBInit of devSyncSem failed.\n",
			0, 0, 0, 0, 0, 0);
	(void) free ((char *) pScsiPhysDev);
        return ((SCSI_PHYS_DEV *) NULL);
        }

    /* initialize miscellaneous fields in the SCSI_PHYS_DEV struct */

    pScsiPhysDev->scsiDevBusId	= devBusId;
    pScsiPhysDev->scsiDevLUN	= devLUN;
    pScsiPhysDev->devStatus 	= IDLE;

    (*pScsiCtrl->scsiSelTimeOutCvt) (pScsiCtrl, scsiSelectTimeout,
				     &pScsiPhysDev->selTimeOut);

    pScsiPhysDev->pScsiCtrl = pScsiCtrl;
    pScsiCtrl->physDevArr [(devBusId << 3) | devLUN] = pScsiPhysDev;

    /* initialize block device list */

    semMInit (&pScsiPhysDev->blkDevList.listMutexSem, blkDevListMutexOptions);
    lstInit (&pScsiPhysDev->blkDevList.blkDevNodes);

    if (reqSenseLength == 0)
	{
        /* determine if device uses Extended Sense Data Format */

        if (scsiReqSense (pScsiPhysDev, (char *) reqSenseData, 1) == ERROR)
	    {
	    SCSI_DEBUG_MSG ("scsiPhysDevCreate: REQUEST SENSE failed.\n",
			    0, 0, 0, 0, 0, 0);
            (void) scsiPhysDevDelete (pScsiPhysDev);
	    return ((SCSI_PHYS_DEV *) NULL);
	    }

        SCSI_DEBUG_MSG ("scsiPhysDevCreate: reqSenseData[0] = %x\n",
		        reqSenseData[0], 0, 0, 0, 0, 0);

        if ((reqSenseData[0] & SCSI_SENSE_DATA_CLASS) != SCSI_EXT_SENSE_CLASS)
	    {
	    /* device uses Nonextended Sense Data Format */

	    pScsiPhysDev->extendedSense      = FALSE;
	    pScsiPhysDev->reqSenseDataLength = NON_EXT_SENSE_DATA_LENGTH;
	    }
        else if ((reqSenseData[0] & SCSI_SENSE_DATA_CODE) !=
		  SCSI_EXT_SENSE_CODE)
	    {
	    /* device uses Unknown Sense Data Format */

	    errnoSet (S_scsiLib_DEV_UNSUPPORTED);
	    SCSI_DEBUG_MSG ("scsiPhysDevCreate: Unknown Sense Data Format ",
			    0, 0, 0, 0, 0, 0);
	    SCSI_DEBUG_MSG ("(device not supported)\n", 0, 0, 0, 0, 0, 0);

            (void) scsiPhysDevDelete (pScsiPhysDev);
	    return ((SCSI_PHYS_DEV *) NULL);
	    }
        else
	    {
	    /* device uses Extended Sense Data Format */

	    if (scsiReqSense (pScsiPhysDev, (char *) reqSenseData,
	        REQ_SENSE_ADD_LENGTH_BYTE + 1) == ERROR)
	        {
	        SCSI_DEBUG_MSG ("scsiPhysDevCreate: REQUEST SENSE failed.\n",
				0, 0, 0, 0, 0, 0);
                (void) scsiPhysDevDelete (pScsiPhysDev);
	        return ((SCSI_PHYS_DEV *) NULL);
	        }

	    SCSI_DEBUG_MSG ("scsiPhysDevCreate: reqSenseData[7] = %x\n",
			    reqSenseData[REQ_SENSE_ADD_LENGTH_BYTE],
			    0, 0, 0, 0, 0);

	    pScsiPhysDev->extendedSense      = TRUE;
	    pScsiPhysDev->reqSenseDataLength = REQ_SENSE_ADD_LENGTH_BYTE +
	        (int) reqSenseData [REQ_SENSE_ADD_LENGTH_BYTE] + 1;
	    }
	}
    else
	{
	pScsiPhysDev->reqSenseDataLength = reqSenseLength;
	if (reqSenseLength == 4)
	    pScsiPhysDev->extendedSense = FALSE;
	else
	    pScsiPhysDev->extendedSense = TRUE;
	}

    if ((pScsiPhysDev->pReqSenseData =
	(UINT8 *) calloc (pScsiPhysDev->reqSenseDataLength,
			  sizeof (UINT8))) == NULL)
	{
        (void) scsiPhysDevDelete (pScsiPhysDev);
	return ((SCSI_PHYS_DEV *) NULL);
	}

    /* transact an INQUIRY command if devType is unspecified */

    if ((devType == NONE) || (removable == NONE))
        {
	UINT8 inquiryData [DEFAULT_INQUIRY_DATA_LENGTH];
	int ix;

        /* do an INQUIRY command */

	for (ix = 0; ix < DEFAULT_INQUIRY_DATA_LENGTH; ix++)
	    inquiryData[ix] = (UINT8) 0;

inquiryRetry:

	if ((scsiInquiry (pScsiPhysDev, (char *) inquiryData,
			 sizeof (inquiryData)) == OK) &&
	    (inquiryData[0] != SCSI_LUN_NOT_PRESENT))
	    {
	    pScsiPhysDev->scsiDevType = inquiryData[0];
	    pScsiPhysDev->removable = (BOOL) (inquiryData[1] &
					      INQUIRY_REMOVABLE_MED_BIT);

	    bcopy ((char *) &inquiryData[8], pScsiPhysDev->devVendorID,
		   VENDOR_ID_LENGTH);
	    bcopy ((char *) &inquiryData[16], pScsiPhysDev->devProductID,
		   PRODUCT_ID_LENGTH);
	    bcopy ((char *) &inquiryData[32], pScsiPhysDev->devRevLevel,
		   REV_LEVEL_LENGTH);

	    pScsiPhysDev->devVendorID [VENDOR_ID_LENGTH] = EOS;
	    pScsiPhysDev->devProductID [PRODUCT_ID_LENGTH] = EOS;
	    pScsiPhysDev->devRevLevel [REV_LEVEL_LENGTH] = EOS;
	    }
	else if (pScsiPhysDev->resetFlag == TRUE)
	    {
	    pScsiPhysDev->resetFlag = FALSE;
	    SCSI_DEBUG_MSG ("retrying scsiInquiry...\n", 0, 0, 0, 0, 0, 0);
	    goto inquiryRetry;
	    }
	else
	    {
	    if (inquiryData[0] == SCSI_LUN_NOT_PRESENT)
		{
		SCSI_DEBUG_MSG ("scsiPhysDevCreate: LUN not present.\n",
				0, 0, 0, 0, 0, 0);
		errnoSet (S_scsiLib_LUN_NOT_PRESENT);
		}

	    (void) scsiPhysDevDelete (pScsiPhysDev);

	    return ((SCSI_PHYS_DEV *) NULL);
	    }
        }
    else
        {
	pScsiPhysDev->scsiDevType = (UINT8) devType;
	pScsiPhysDev->removable = removable;
        }

    /* record numBlocks and blockSize in physical device */

    if (((pScsiPhysDev->scsiDevType == SCSI_DEV_DIR_ACCESS) ||
    	 (pScsiPhysDev->scsiDevType == SCSI_DEV_WORM) ||
    	 (pScsiPhysDev->scsiDevType == SCSI_DEV_RO_DIR_ACCESS)) &&
	((numBlocks == 0) || (blockSize == 0)))
        {
	int lastLogBlkAdrs;
	int blkLength;

        /* do a READ_CAPACITY command */

readCapRetry:

	if (scsiReadCapacity (pScsiPhysDev, &lastLogBlkAdrs, &blkLength) == OK)
	    {
	    pScsiPhysDev->numBlocks = lastLogBlkAdrs + 1;
	    pScsiPhysDev->blockSize = blkLength;
	    }
	else if (pScsiPhysDev->resetFlag == TRUE)
	    {
	    pScsiPhysDev->resetFlag = FALSE;
	    SCSI_DEBUG_MSG ("retrying scsiReadCapacity...\n",
			    0, 0, 0, 0, 0, 0);
	    goto readCapRetry;
	    }
	else
	    {
            (void) scsiPhysDevDelete (pScsiPhysDev);
	    return ((SCSI_PHYS_DEV *) NULL);
	    }
        }
    else
        {
	pScsiPhysDev->numBlocks = numBlocks;
	pScsiPhysDev->blockSize = blockSize;
        }

    return (pScsiPhysDev);
    }

/*******************************************************************************
*
* scsi1PhysDevIdGet - return a pointer to a SCSI_PHYS_DEV structure
*
* This routine returns a pointer to the SCSI_PHYS_DEV structure of the SCSI
* physical device located at a specified bus ID (`devBusId') and logical
* unit number (`devLUN') and attached to a specified SCSI controller
* (`pScsiCtrl').
*
* RETURNS: A pointer to the specified SCSI_PHYS_DEV structure, or NULL if the
* structure does not exist.
*/

LOCAL SCSI_PHYS_DEV * scsi1PhysDevIdGet
    (
    SCSI_CTRL *pScsiCtrl,       /* ptr to SCSI controller info */
    int devBusId,               /* device's SCSI bus ID */
    int devLUN                  /* device's logical unit number */
    )
    {
    /* check for valid ptr to SCSI_CTRL */

    if (pScsiCtrl == NULL)
	{
	if (pSysScsiCtrl != NULL)
	    pScsiCtrl = pSysScsiCtrl;
	else
	    {
	    errnoSet (S_scsiLib_NO_CONTROLLER);
	    SCSI_DEBUG_MSG ("No SCSI controller specified.\n",
			    0, 0, 0, 0, 0, 0);
	    return ((SCSI_PHYS_DEV *) NULL);
	    }
	}

    return (pScsiCtrl->physDevArr [(devBusId << 3) | devLUN]);
    }

/*******************************************************************************
*
* scsi1AutoConfig - configure all devices connected to a SCSI controller
*
* This routine cycles through all legal SCSI bus IDs (and logical unit
* numbers (LUNs)), attempting a scsiPhysDevCreate() with default parameters
* on each.  All devices which support the INQUIRY routine are
* configured.  The scsiShow() routine can be used to find the system's table
* of SCSI physical devices attached to a specified SCSI controller.  In
* addition, scsiPhysDevIdGet() can be used programmatically to get a
* pointer to the SCSI_PHYS_DEV structure associated with the device at a
* specified SCSI bus ID and LUN.
*
* RETURNS: OK, or ERROR if `pScsiCtrl' and `pSysScsiCtrl' are both NULL.
*/

LOCAL STATUS scsi1AutoConfig
    (
    SCSI_CTRL *pScsiCtrl       /* ptr to SCSI controller info */
    )
    {
    int busId, lun;		/* loop indices */
    SCSI_PHYS_DEV *pScsiPhysDev; /* ptr to SCSI physical device info */

    /* check for valid input parameters */

    if (pScsiCtrl == (SCSI_CTRL *) NULL)
	{
	if (pSysScsiCtrl == (SCSI_CTRL *) NULL)
	    {
	    errnoSet (S_scsiLib_NO_CONTROLLER);
	    printErr ("No SCSI controller specified.\n");
	    return (ERROR);
	    }

	pScsiCtrl = pSysScsiCtrl;
	}

    /* loop through all SCSI bus ID's and LUN's (logical units); if a given
     * bus ID times out during selection, do not test for other LUN's at
     * that bus ID, since there cannot be any.
     */

    for (busId = SCSI_MIN_BUS_ID; busId <= SCSI_MAX_BUS_ID; busId++)
	{
	if (busId != pScsiCtrl->scsiCtrlBusId)
	    {
            for (lun = SCSI_MIN_LUN; lun <= SCSI_MAX_LUN; lun++)
	        {
		SCSI_DEBUG_MSG ("scsiAutoConfig: bus ID = %d, LUN = %d\n",
			        busId, lun, 0, 0, 0, 0);

	        if ((pScsiPhysDev = scsiPhysDevCreate
					(pScsiCtrl, busId, lun, 0, NONE, 0,
					 0, 0)) == (SCSI_PHYS_DEV *) NULL)
		    {
		    if (errnoGet () == S_scsiLib_SELECT_TIMEOUT)
		        break;
		    }
	        }
	    }
	}

    return (OK);
    }

/*******************************************************************************
*
* strIsPrintable - determine whether a string contains all printable chars
*
* RETURNS: TRUE | FALSE.
*/

LOCAL BOOL strIsPrintable
    (
    FAST char *pString          /* ptr to string to be tested */
    )
    {
    FAST char ch;

    while ((ch = *pString++) != EOS)
	{
	if (!isprint (ch))
	    return (FALSE);
	}

    return (TRUE);
    }

/*******************************************************************************
*
* scsi1Show - list the physical devices attached to a SCSI controller
*
* This routine displays the SCSI bus ID, logical unit number (LUN), vendor ID,
* product ID, firmware revision (rev.), device type, number of blocks,
* block size in bytes, and a pointer to the associated SCSI_PHYS_DEV
* structure for each physical SCSI device known to be attached to a specified
* SCSI controller.
*
* NOTE:
* If `pScsiCtrl' is NULL, the value of the global variable `pSysScsiCtrl'
* is used, unless it is also NULL.
*
* RETURNS: OK, or ERROR if both `pScsiCtrl' and `pSysScsiCtrl' are NULL.
*/

LOCAL STATUS scsi1Show
    (
    FAST SCSI_CTRL *pScsiCtrl           /* ptr to SCSI controller info */
    )
    {
    FAST SCSI_PHYS_DEV *pScsiPhysDev;	/* SCSI physical device info */
    FAST int ix;			/* loop variable */

    if (pScsiCtrl == NULL)
	{
	if (pSysScsiCtrl != NULL)
	    pScsiCtrl = pSysScsiCtrl;
	else
	    {
	    errnoSet (S_scsiLib_NO_CONTROLLER);
	    SCSI_DEBUG_MSG ("No SCSI controller specified.\n",
			    0, 0, 0, 0, 0, 0);
	    return (ERROR);
	    }
	}

    printf ("ID LUN VendorID    ProductID     Rev. ");
    printf ("Type  Blocks  BlkSize pScsiPhysDev \n");
    printf ("-- --- -------- ---------------- ---- ");
    printf ("---- -------- ------- ------------\n");

    for (ix = 0; ix < MAX_SCSI_PHYS_DEVS; ix++)
	if ((pScsiPhysDev = pScsiCtrl->physDevArr[ix]) !=
	    (SCSI_PHYS_DEV *) NULL)
	    {
	    printf ("%2d " , pScsiPhysDev->scsiDevBusId);
	    printf ("%2d " , pScsiPhysDev->scsiDevLUN);

	    printf (" %8s" , strIsPrintable (pScsiPhysDev->devVendorID) ?
			     pScsiPhysDev->devVendorID : "        ");
	    printf (" %16s", strIsPrintable (pScsiPhysDev->devProductID) ?
			     pScsiPhysDev->devProductID : "                ");
	    printf (" %4s ", strIsPrintable (pScsiPhysDev->devRevLevel) ?
			     pScsiPhysDev->devRevLevel : "    ");

	    printf ("%3d"  , pScsiPhysDev->scsiDevType);
	    printf (pScsiPhysDev->removable ? "R" : " ");
	    printf (" %7d " , pScsiPhysDev->numBlocks);
	    printf (" %5d " , pScsiPhysDev->blockSize);
	    printf ("   0x%08x ", (int) pScsiPhysDev);
	    printf ("\n");
	    }

    return (OK);
    }

/*******************************************************************************
*
* scsi1BlkDevCreate - define a logical partition on a SCSI block device
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

LOCAL BLK_DEV *scsi1BlkDevCreate
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
	SCSI_DEBUG_MSG ("scsi1BlkDevCreate: Invalid input parameter(s).\n",
			0, 0, 0, 0, 0, 0);
	return ((BLK_DEV *) NULL);
	}

    /* return NULL if sequential access (or other non-block) device */

    if (!((pScsiPhysDev->scsiDevType == SCSI_DEV_DIR_ACCESS) ||
    	  (pScsiPhysDev->scsiDevType == SCSI_DEV_WORM) ||
    	  (pScsiPhysDev->scsiDevType == SCSI_DEV_RO_DIR_ACCESS)))
	{
	errnoSet (S_scsiLib_ILLEGAL_OPERATION);
	SCSI_DEBUG_MSG ("scsi1BlkDevCreate:", 0, 0, 0, 0, 0, 0);
	SCSI_DEBUG_MSG ("Physical device is not a block device.\n",
			0, 0, 0, 0, 0, 0);
	return ((BLK_DEV *) NULL);
	}

    /* disallow multiple partitions on removable media */

    if ((pScsiPhysDev->pScsiBlkDev != NULL) &&
	pScsiPhysDev->removable)
	{
	printErr ("scsi1BlkDevCreate: ");
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

    semTake (&pScsiPhysDev->blkDevList.listMutexSem, WAIT_FOREVER);

    lstAdd (&pScsiPhysDev->blkDevList.blkDevNodes,
	    &pScsiBlkDevNode->blkDevNode);

    semGive (&pScsiPhysDev->blkDevList.listMutexSem);

    return (&pScsiBlkDev->blkDev);
    }

/*******************************************************************************
*
* scsi1BlkDevInit - initialize fields in a SCSI logical partition
*
* This routine specifies the disk geometry parameters required by certain
* file systems (e.g., dosFs).  It should be called after a SCSI_BLK_DEV
* structure is created via scsiBlkDevCreate(), but before a file system
* initialization routine.  It is generally required only for removable media
* devices.
*
* RETURNS: N/A
*/

LOCAL void scsi1BlkDevInit
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
* scsi1BlkDevShow - show the BLK_DEV structures on a specified physical device
*
* This routine displays all of the BLK_DEV structures created on a specified
* physical device.  This routine is called by scsiShow(), but may also be
* invoked directly, usually from the shell.
*
* RETURNS: N/A
*/

LOCAL void scsi1BlkDevShow
    (
    SCSI_PHYS_DEV *pScsiPhysDev  /* ptr to SCSI physical device info */
    )
    {
    SCSI_BLK_DEV_NODE *pScsiBlkDevNode;
    int ix = 0;

    printf ("Block Device #       physical address     size (blocks)\n");
    printf ("--------------       ----------------     -------------\n");

    if (lstCount (&pScsiPhysDev->blkDevList.blkDevNodes) == 0)
	return;

    semTake (&pScsiPhysDev->blkDevList.listMutexSem, WAIT_FOREVER);

    for (pScsiBlkDevNode = (SCSI_BLK_DEV_NODE *)
			       lstFirst (&pScsiPhysDev->blkDevList.blkDevNodes);
         pScsiBlkDevNode != NULL;
         pScsiBlkDevNode = (SCSI_BLK_DEV_NODE *)
			       lstNext (&pScsiBlkDevNode->blkDevNode))
        {
	printf ("%8d              %8d                %8d\n", ix++,
		pScsiBlkDevNode->scsiBlkDev.blockOffset,
		pScsiBlkDevNode->scsiBlkDev.numBlocks);
	}

    semGive (&pScsiPhysDev->blkDevList.listMutexSem);
    }

/*******************************************************************************
*
* scsi1BusReset - pulse the reset signal on the SCSI bus
*
* This routine calls a controller-specific routine to reset a specified
* controller's SCSI bus.  If no controller is specified (`pScsiCtrl' is 0),
* the value in `pSysScsiCtrl' is used.
*
* RETURNS: OK, or ERROR if there is no controller or controller-specific
* routine.
*/

LOCAL STATUS scsi1BusReset
    (
    SCSI_CTRL *pScsiCtrl	/* ptr to SCSI controller info */
    )
    {
    if (pScsiCtrl->scsiBusReset == (VOIDFUNCPTR) NULL)
	return (ERROR);
    else
	{
	if (pScsiCtrl == (SCSI_CTRL *) NULL)
	    {
	    if (pSysScsiCtrl != (SCSI_CTRL *) NULL)
	        pScsiCtrl = pSysScsiCtrl;
	    else
		{
	        errnoSet (S_scsiLib_NO_CONTROLLER);
	        printErr ("No SCSI controller specified.\n");
		return (ERROR);
		}
	    }
	(pScsiCtrl->scsiBusReset) (pScsiCtrl);
	return (OK);
	}
    }

/*******************************************************************************
*
* scsiDevSelect - call the scsiDevSelect routine in the SCSI_CTRL structure
*
* This routine is not intended to be called directly by users, but rather
* by other routines in scsiLib.  It merely calls a driver-specific routine
* to select the desired SCSI physical device.
*
* RETURNS: OK if device was successfully selected, otherwise ERROR.
*/

LOCAL STATUS scsiDevSelect
    (
    SCSI_PHYS_DEV *pScsiPhysDev,        /* ptr to SCSI physical device */
    SCSI_TRANSACTION *pScsiXaction      /* ptr to SCSI transaction info */
    )
    {
    return ((*pScsiPhysDev->pScsiCtrl->scsiDevSelect)
	        (pScsiPhysDev, pScsiXaction));
    }

/*******************************************************************************
*
* scsi1PhaseNameGet - get the name of a specified SCSI phase
*
* This routine returns a pointer to a string which is the name of the SCSI
* phase input as an integer.  It's primarily used to improve readability of
* debugging messages.
*
* RETURNS: A pointer to a string naming the SCSI phase input
*
* NOMANUAL
*/

LOCAL char *scsi1PhaseNameGet
    (
    int scsiPhase               /* phase whose name to look up */
    )
    {
    static char *phaseNameArray [] =
	{
	"DATA_OUT",
	"DATA_IN ",
	"COMMAND ",
	"STATUS  ",
	"UNDEF(4)",
	"UNDEF(5)",
	"MSG_OUT ",
	"MSG_IN  "
	};

    return ((scsiPhase < SCSI_DATA_OUT_PHASE ||
	     scsiPhase > SCSI_MSG_IN_PHASE) ?
	     "UNKNOWN " : phaseNameArray [scsiPhase]);
    }

/*******************************************************************************
*
* scsi1CmdBuild - fills in the fields of a SCSI command descriptor block
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

LOCAL STATUS scsi1CmdBuild
    (
    SCSI_COMMAND scsiCmd,       /* command to be built */
    int *pCmdLength,            /* ptr to command length variable */
    UINT8 opCode,               /* SCSI opCode for command */
    int LUN,                    /* logical unit number for command */
    BOOL relAdrs,               /* whether to set relative address bit */
    int logBlockAdrs,           /* logical block address */
    int xferLength,             /* number of blocks or bytes to xfer */
    UINT8 controlByte           /* control byte for command */
    )
    {
    FAST int groupCode = (int) (opCode >> 5);
    FAST int cmdLength;

    /* array with the length of a SCSI command indexed by its group code
     * (NONE == vendor unique)
     */

    LOCAL int scsiCmdLength [8] =
        {
        SCSI_GROUP_0_CMD_LENGTH,
        SCSI_GROUP_1_CMD_LENGTH,
        NONE,
        NONE,
        NONE,
        SCSI_GROUP_5_CMD_LENGTH,
        NONE,
        NONE
        };

    if ((*pCmdLength = cmdLength = scsiCmdLength [groupCode]) == NONE)
	return (ERROR);

    if ((groupCode == 0) && (logBlockAdrs > 0x1fffff || xferLength > 0xff))
	return (ERROR);
    else if (xferLength > 0xffff)
	return (ERROR);

    scsiCmd[0] = opCode;
    scsiCmd[1] = (UINT8) ((LUN & 0x7) << 5);

    switch (groupCode)
	{
	case 0:
	    scsiCmd[1] |= (UINT8) ((logBlockAdrs >> 16) & 0x1f);
	    scsiCmd[2]  = (UINT8) ((logBlockAdrs >>  8) & 0xff);
	    scsiCmd[3]  = (UINT8) ((logBlockAdrs      ) & 0xff);
	    scsiCmd[4]  = (UINT8) xferLength;
	    scsiCmd[5]  = controlByte;
	    break;
	case 1:
	case 5:
	    scsiCmd[1] |= (UINT8) (relAdrs ? 1 : 0);
	    scsiCmd[2]  = (UINT8) ((logBlockAdrs >> 24) & 0xff);
	    scsiCmd[3]  = (UINT8) ((logBlockAdrs >> 16) & 0xff);
	    scsiCmd[4]  = (UINT8) ((logBlockAdrs >>  8) & 0xff);
	    scsiCmd[5]  = (UINT8) ((logBlockAdrs      ) & 0xff);
	    scsiCmd[6]  = (UINT8) 0;
	    if (groupCode == 5)
		{
	        scsiCmd[7]  = (UINT8) 0;
	        scsiCmd[8]  = (UINT8) 0;
		}
	    scsiCmd [cmdLength - 3] = (UINT8) ((xferLength >> 8) & 0xff);
	    scsiCmd [cmdLength - 2] = (UINT8) ((xferLength     ) & 0xff);
	    scsiCmd [cmdLength - 1] = controlByte;
	    break;
	}

    return (OK);
    }

/*******************************************************************************
*
* scsi1TestUnitRdy - issue a TEST_UNIT_READY command to a SCSI device
*
* This routine issues a TEST_UNIT_READY command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi1TestUnitRdy
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
    SCSI_PHYS_DEV *pScsiPhysDev;	/* ptr to SCSI physical device */
    SCSI_COMMAND testUnitRdyCommand;	/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */

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
* scsi1FormatUnit - issue a FORMAT_UNIT command to a SCSI device
*
* This routine issues a FORMAT_UNIT command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi1FormatUnit
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
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_FULL;

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
	    (pScsiPhysDev, &scsiXaction));
    }

/*******************************************************************************
*
* scsi1Inquiry - issue an INQUIRY command to a SCSI device
*
* This routine issues an INQUIRY command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi1Inquiry
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

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
	    (pScsiPhysDev, &scsiXaction));
    }

/*******************************************************************************
*
* scsi1ModeSelect - issue a MODE_SELECT command to a SCSI device
*
* This routine issues a MODE_SELECT command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi1ModeSelect
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

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
	    (pScsiPhysDev, &scsiXaction));
    }

/*******************************************************************************
*
* scsi1ModeSense - issue a MODE_SENSE command to a SCSI device
*
* This routine issues a MODE_SENSE command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi1ModeSense
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

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
	    (pScsiPhysDev, &scsiXaction));
    }

/*******************************************************************************
*
* scsi1ReadCapacity - issue a READ_CAPACITY command to a SCSI device
*
* This routine issues a READ_CAPACITY command to a specified SCSI device.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi1ReadCapacity
    (
    SCSI_PHYS_DEV *pScsiPhysDev,/* ptr to SCSI physical device */
    int *pLastLBA,              /* where to return last logical block address */
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

/*******************************************************************************
*
* scsi1RdSecs - read sector(s) from a SCSI block device
*
* This routine reads the specified physical sector(s) from a specified
* physical device.
*
* RETURNS: OK, or ERROR if the sector cannot be read.
*/

LOCAL STATUS scsi1RdSecs
    (
    SCSI_BLK_DEV *pScsiBlkDev,  /* ptr to SCSI block device info */
    int sector,                 /* sector number to read */
    int numSecs,                /* total sectors to read */
    char *buffer                /* ptr to input data buffer */
    )
    {
    SCSI_COMMAND readCommand;		/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */
    SCSI_PHYS_DEV *pScsiPhysDev = pScsiBlkDev->pScsiPhysDev;
    int startSec = sector + pScsiBlkDev->blockOffset;

    SCSI_DEBUG_MSG ("scsiRdSecs:\n", 0, 0, 0, 0, 0, 0);

    if (startSec <= 0x1FFFFF && numSecs <= 256)
        {
        /* build a 21 bit logical block address 'O_RDONLY' command */

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
			  SCSI_OPCODE_READ_EXT, pScsiPhysDev->scsiDevLUN, FALSE,
			  startSec, numSecs, (UINT8) 0)
	    == ERROR)
	    return (ERROR);
        }

    scsiXaction.cmdAddress    = readCommand;
    scsiXaction.dataAddress   = (UINT8 *) buffer;
    scsiXaction.dataDirection = O_RDONLY;
    scsiXaction.dataLength    = numSecs * pScsiPhysDev->blockSize;
    scsiXaction.addLengthByte = NONE;
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
* scsi1WrtSecs - write sector(s) to a SCSI block device
*
* This routine writes the specified physical sector(s) to a specified physical
* device.
*
* RETURNS: OK, or ERROR if the sector cannot be written.
*/

LOCAL STATUS scsi1WrtSecs
    (
    SCSI_BLK_DEV *pScsiBlkDev,  /* ptr to SCSI block device info */
    int sector,                 /* sector number to write */
    int numSecs,                /* total sectors to write */
    char *buffer                /* ptr to input data buffer */
    )
    {
    SCSI_COMMAND writeCommand;		/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction;	/* info on a SCSI transaction */
    SCSI_PHYS_DEV *pScsiPhysDev = pScsiBlkDev->pScsiPhysDev;
    int startSec = sector + pScsiBlkDev->blockOffset;

    SCSI_DEBUG_MSG ("scsiWrtSecs:\n", 0, 0, 0, 0, 0, 0);

    if (startSec <= 0x1FFFFF && numSecs <= 256)
        {
        /* build a 21 bit logical block address 'O_WRONLY' command */

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
			  FALSE, startSec, numSecs, (UINT8) 0)
	    == ERROR)
	    return (ERROR);
        }

    scsiXaction.cmdAddress    = writeCommand;
    scsiXaction.dataAddress   = (UINT8 *) buffer;
    scsiXaction.dataDirection = O_WRONLY;
    scsiXaction.dataLength    = numSecs * pScsiPhysDev->blockSize;
    scsiXaction.addLengthByte = NONE;
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
* scsi1ReqSense - issue a REQUEST_SENSE command to a device and read the results
*
* This routine issues a REQUEST_SENSE command to a specified SCSI device and
* read the results.
*
* RETURNS: OK, or ERROR if the command fails.
*/

LOCAL STATUS scsi1ReqSense
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

    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
	    (pScsiPhysDev, &scsiXaction));
    }

/*******************************************************************************
*
* scsiPhaseSequence - manage all phases of a single SCSI bus sequence
*
* This routine is called by scsiTransact after a device has been successfully
* selected.  It repeated calls the appropriate routine to detect the current
* SCSI bus phase.  It takes whatever action is specified in the supplied
* SCSI_TRANSACTION structure, usually calling the appropiate routines to
* input or output bytes to the device.
*
* RETURNS: OK, or ERROR if not successful for any reason.
*/

LOCAL STATUS scsiPhaseSequence
    (
    SCSI_PHYS_DEV *pScsiPhysDev,        /* ptr to the target device */
    SCSI_TRANSACTION *pScsiXaction      /* ptr to the transaction info */
    )
    {
    SCSI_CTRL *pScsiCtrl;	/* SCSI controller info for device */
    STATUS status;		/* routine return status */
    int scsiPhase;		/* passed to scsiBusPhaseGet routine */
    UINT8 msgIn [MAX_MSG_IN_BYTES]; /* message returned from the SCSI device */
    UINT8 *pMsgIn;		/* ptr to next dest. for a message in */
    BOOL transactionComplete;	/* set TRUE upon 'disconn' or 'comm complete' */

    pScsiCtrl = pScsiPhysDev->pScsiCtrl;

    /* save current command context requested */
    pScsiPhysDev->pScsiXaction = pScsiXaction;

    transactionComplete = FALSE;

    if ((status = scsiDevSelect (pScsiPhysDev, pScsiXaction)) == ERROR)
	{
	SCSI_DEBUG_MSG ("scsiPhaseSequence: Unable to select device.\n",
			0, 0, 0, 0, 0, 0);
	return (ERROR);
	}

    do
	{
	if ((*pScsiCtrl->scsiBusPhaseGet) (pScsiCtrl, pScsiXaction->cmdTimeout,
					   &scsiPhase) != OK)
	    {
	    SCSI_DEBUG_MSG ("scsiBusPhaseGet returned ERROR.\n",
			    0, 0, 0, 0, 0, 0);
	    return (ERROR);
	    }

	phaseSwitch:

        switch (scsiPhase)
	    {
	    case SCSI_DATA_OUT_PHASE:
	        status = (*pScsiCtrl->scsiBytesOut)
			     (pScsiPhysDev, pScsiXaction->dataAddress,
			      pScsiXaction->dataLength, scsiPhase);
		if (status == ERROR)
		    {
		    return (ERROR);
		    }
	        break;
	    case SCSI_DATA_IN_PHASE:
		{
		int addLengthByte = pScsiXaction->addLengthByte;
		int dataLength = pScsiXaction->dataLength;
		UINT8 *dataAddress = pScsiXaction->dataAddress;

		if ((addLengthByte == NONE) ||
		    (dataLength <= (addLengthByte + 1)))
		    {
	            status = (*pScsiCtrl->scsiBytesIn)
			         (pScsiPhysDev, dataAddress,
			          dataLength, scsiPhase);
		    if (status == ERROR)
		        {
		        return (ERROR);
		        }
		    }
		else
		    {
	            status = (*pScsiCtrl->scsiBytesIn)
			         (pScsiPhysDev, dataAddress,
			          addLengthByte + 1, scsiPhase);
		    if (status == ERROR)
		        {
		        return (ERROR);
		        }

	    if ((*pScsiCtrl->scsiBusPhaseGet) (pScsiCtrl, 0, &scsiPhase) != OK)
	        {
		SCSI_DEBUG_MSG ("scsiBusPhaseGet returned ERROR.\n",
				0, 0, 0, 0, 0, 0);
	        return (ERROR);
	        }

		if (scsiPhase != SCSI_DATA_IN_PHASE)
		    goto phaseSwitch;

	            status = (*pScsiCtrl->scsiBytesIn)
			         (pScsiPhysDev,
				  dataAddress + addLengthByte + 1,
				  min (dataAddress [addLengthByte],
				       dataLength - (addLengthByte + 1)),
				  scsiPhase);
		    if (status == ERROR)
		        {
		        return (ERROR);
		        }
		    }
	        break;
		}
	    case SCSI_COMMAND_PHASE:
		status = (*pScsiCtrl->scsiBytesOut)
			     (pScsiPhysDev, pScsiXaction->cmdAddress,
			      pScsiXaction->cmdLength, scsiPhase);
		if (status != OK)
		    {
		    return (ERROR);
		    }
		break;
	    case SCSI_STATUS_PHASE:
	        status = (*pScsiCtrl->scsiBytesIn)
			     (pScsiPhysDev, &pScsiXaction->statusByte,
			      sizeof (pScsiXaction->statusByte), scsiPhase);
		if (status == ERROR)
		    {
		    return (ERROR);
		    }
	        break;
	    case SCSI_MSG_OUT_PHASE:
	        status = (*pScsiCtrl->scsiBytesOut)
			     (pScsiPhysDev, pScsiPhysDev->msgOutArray,
			      pScsiPhysDev->msgLength, scsiPhase);
		if (status == ERROR)
		    {
		    return (ERROR);
		    }
		else
		    pScsiPhysDev->msgLength = 0;
	        break;
	    case SCSI_MSG_IN_PHASE:
		pMsgIn = msgIn;

	        status = (*pScsiCtrl->scsiBytesIn)
			     (pScsiPhysDev, pMsgIn++, sizeof (UINT8),
			      scsiPhase);
		if (status == ERROR)
		    {
		    return (ERROR);
		    }
		switch (msgIn[0])
		    {
		    case SCSI_MSG_COMMAND_COMPLETE:
			SCSI_DEBUG_MSG ("Command Complete message received.\n",
					0, 0, 0, 0, 0, 0);
			break;

		    case SCSI_MSG_EXTENDED_MESSAGE:

		        if ((*pScsiCtrl->scsiBusPhaseGet) (pScsiCtrl,
				pScsiXaction->cmdTimeout, &scsiPhase) != OK)
            		    {
            		    SCSI_DEBUG_MSG ("scsiBusPhaseGet returned ERROR.\n",
                            		    0, 0, 0, 0, 0, 0);
            		    return (ERROR);
            		    }

			if (scsiPhase != SCSI_MSG_IN_PHASE)
			    break;

	        	status = (*pScsiCtrl->scsiBytesIn)
			          (pScsiPhysDev, pMsgIn++, sizeof (UINT8),
			           scsiPhase);
			if (status == ERROR)
		    	    {
		    	    return (ERROR);
		    	    }

		        if ((*pScsiCtrl->scsiBusPhaseGet) (pScsiCtrl,
				pScsiXaction->cmdTimeout, &scsiPhase) != OK)
            		    {
            		    SCSI_DEBUG_MSG ("scsiBusPhaseGet returned ERROR.\n",
                            		    0, 0, 0, 0, 0, 0);
            		    return (ERROR);
            		    }

			if (scsiPhase != SCSI_MSG_IN_PHASE)
			    break;

	        	status = (*pScsiCtrl->scsiBytesIn)
			          (pScsiPhysDev, pMsgIn, (int) msgIn[1],
			           scsiPhase);
			if (status == ERROR)
		    	    {
		    	    return (ERROR);
		    	    }

			switch (msgIn[2])
			    {
/*XXX
SYNC transfer not supported for 5.1 version.
			    case SCSI_EXT_MSG_SYNC_XFER_REQ:
				pScsiPhysDev->syncXferPeriod = msgIn[3] * 4;
				if (msgIn[4] > 0)
				    {
				    pScsiPhysDev->syncXferOffset = msgIn[4];
				    pScsiPhysDev->syncXfer = (TBOOL) TRUE;
				    }
				break;
XXX*/
			    default:
				SCSI_DEBUG_MSG ("unknown extended message\n",
						0, 0, 0, 0, 0, 0);
			    }
			break;
		    case SCSI_MSG_SAVE_DATA_POINTER:
			SCSI_DEBUG_MSG ("Save Data Pointer message in\n",
					0, 0, 0, 0, 0, 0);
			break;
		    case SCSI_MSG_RESTORE_POINTERS:
			SCSI_DEBUG_MSG ("Restore Pointers message in\n",
					0, 0, 0, 0, 0, 0);
			break;
		    case SCSI_MSG_DISCONNECT:
			SCSI_DEBUG_MSG ("Disconnect message in\n",
					0, 0, 0, 0, 0, 0);
			break;
		    case SCSI_MSG_MESSAGE_REJECT:
			SCSI_DEBUG_MSG ("Message Reject message in\n",
					0, 0, 0, 0, 0, 0);
			break;
		    case SCSI_MSG_NO_OP:
			SCSI_DEBUG_MSG ("No Operation message in\n",
					0, 0, 0, 0, 0, 0);
			break;
		    default:
			if (msgIn[0] & SCSI_MSG_IDENTIFY)
			    {
			    SCSI_DEBUG_MSG ("Identify message in\n",
					    0, 0, 0, 0, 0, 0);
			    }
			else
			    {
			    SCSI_DEBUG_MSG ("message (%x) in\n",
					    msgIn[0], 0, 0, 0, 0, 0);
			    }
			break;
		    }
	        break;
	    case SCSI_BUS_FREE_PHASE:
		transactionComplete = TRUE;
		status = OK;
	        break;
	    }
	} while (!transactionComplete);

    return (status);
    }

/*******************************************************************************
*
* scsi1Transact - obtain exclusive use of SCSI controller for a transaction
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

LOCAL STATUS scsi1Transact
    (
    SCSI_PHYS_DEV *pScsiPhysDev,        /* ptr to the target device */
    SCSI_TRANSACTION *pScsiXaction      /* ptr to the transaction info */
    )
    {
    SCSI_CTRL *pScsiCtrl;	/* SCSI controller info for device */
    SCSI_BLK_DEV_NODE *pBlkDevNode; /* ptr for looping through BLK_DEV list */
    STATUS status;		/* routine return status */
    int senseKey;		/* extended sense key from target */
    int addSenseCode;		/* additional sense code from target */
    int taskId = taskIdSelf ();	/* calling task's ID */
    int taskPriority;		/* calling task's current priority */

    pScsiCtrl = pScsiPhysDev->pScsiCtrl;

    /* set task priority to SCSI controller's (if != NONE) */

    if (pScsiCtrl->scsiPriority != NONE)
	{
	taskPriorityGet (taskId, &taskPriority);
	taskPrioritySet (taskId, pScsiCtrl->scsiPriority);
	}

    /* take the device and controller semaphores */

    semTake (&pScsiPhysDev->devMutexSem, WAIT_FOREVER);
    semTake (&pScsiCtrl->ctrlMutexSem, WAIT_FOREVER);

    if ((status = scsiPhaseSequence (pScsiPhysDev, pScsiXaction)) == ERROR)
	{
	SCSI_DEBUG_MSG ("scsiTransact: scsiPhaseSequence ERROR.\n",
			0, 0, 0, 0, 0, 0);
	goto cleanExit;
	}

    /* check device status and take appropriate action */

    switch (pScsiXaction->statusByte & SCSI_STATUS_MASK)
	{
	case SCSI_STATUS_GOOD:
	    status = OK;
	    pScsiPhysDev->lastSenseKey = SCSI_SENSE_KEY_NO_SENSE;
	    pScsiPhysDev->lastAddSenseCode = (UINT8) 0;
	    goto cleanExit;

	case SCSI_STATUS_CHECK_CONDITION:
	    {
	    SCSI_COMMAND reqSenseCmd;		/* REQUEST SENSE command */
	    SCSI_TRANSACTION reqSenseXaction;	/* REQUEST SENSE xaction */
						/* REQUEST SENSE buffer  */
            UINT8 reqSenseData [REQ_SENSE_ADD_LENGTH_BYTE + 1];

	    /* build a REQUEST SENSE command and transact it */

	    (void) scsiCmdBuild (reqSenseCmd, &reqSenseXaction.cmdLength,
			  	 SCSI_OPCODE_REQUEST_SENSE,
				 pScsiPhysDev->scsiDevLUN, FALSE, 0,
				 pScsiPhysDev->reqSenseDataLength, (UINT8) 0);

	    reqSenseXaction.cmdAddress    = (UINT8 *) reqSenseCmd;

	    /* if there is no user request sense buffer ,supply it */

	    if ( pScsiPhysDev->pReqSenseData == (UINT8 *)NULL )
		{
                reqSenseXaction.dataAddress  = reqSenseData;

		if (!pScsiPhysDev->extendedSense)
		    reqSenseXaction.dataLength    = 4;
                else
		    reqSenseXaction.dataLength = REQ_SENSE_ADD_LENGTH_BYTE + 1;
                } 
            else
		{
	        reqSenseXaction.dataAddress   = pScsiPhysDev->pReqSenseData;
	        reqSenseXaction.dataLength    = 
		                           pScsiPhysDev->reqSenseDataLength;
                }

	    reqSenseXaction.dataDirection = O_RDONLY;
	    reqSenseXaction.addLengthByte = REQ_SENSE_ADD_LENGTH_BYTE;
	    reqSenseXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;

	    SCSI_DEBUG_MSG ("scsiTransact: issuing a REQUEST SENSE command.\n",
			    0, 0, 0, 0, 0, 0);

	    if ((status = scsiPhaseSequence (pScsiPhysDev, &reqSenseXaction))
		== ERROR)
		{
		SCSI_DEBUG_MSG ("scsiTransact: scsiPhaseSequence ERROR.\n",
				0, 0, 0, 0, 0, 0);
		goto cleanExit;
		}

	    /* REQUEST SENSE command status != GOOD indicates fatal error */

	    if (reqSenseXaction.statusByte != SCSI_STATUS_GOOD)
		{
		SCSI_DEBUG_MSG ("scsiTransact: non-zero REQ SENSE status.\n",
				0, 0, 0, 0, 0, 0);
		errnoSet (S_scsiLib_REQ_SENSE_ERROR);
		status = ERROR;
		goto cleanExit;
		}

	    /* if device uses Nonextended Sense Data Format, return now */

	    if (!pScsiPhysDev->extendedSense)
		{
		status = ERROR;
		goto cleanExit;
		}

	    /* check sense key and take appropriate action */

	    pScsiPhysDev->lastSenseKey =
		(pScsiPhysDev->pReqSenseData)[2] & SCSI_SENSE_KEY_MASK;

	    pScsiPhysDev->lastAddSenseCode = (pScsiPhysDev->pReqSenseData)[12];
	    addSenseCode = (int) pScsiPhysDev->lastAddSenseCode;

	    switch (senseKey = (int) pScsiPhysDev->lastSenseKey)
	        {
	        case SCSI_SENSE_KEY_NO_SENSE:
	            {
	            SCSI_DEBUG_MSG ("scsiTransact: No Sense\n",
				    0, 0, 0, 0, 0, 0);
	            status = OK;
	            goto cleanExit;
	            }
	        case SCSI_SENSE_KEY_RECOVERED_ERROR:
	            {
		    SCSI_DEBUG_MSG ("scsiTransact: Recovered Error Sense,",
				    0, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
				    addSenseCode, 0, 0, 0, 0, 0);
	            status = OK;
	            goto cleanExit;
	            }
	        case SCSI_SENSE_KEY_NOT_READY:
	            {
		    SCSI_DEBUG_MSG ("scsiTransact: Not Ready Sense,",
				    0, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
				    addSenseCode, 0, 0, 0, 0, 0);
	            errnoSet (S_scsiLib_DEV_NOT_READY);
	            status = ERROR;
	            goto cleanExit;
	            }
	        case SCSI_SENSE_KEY_UNIT_ATTENTION:
	            {
		    SCSI_DEBUG_MSG ("scsiTransact: Unit Attention Sense,",
				    0, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
				    addSenseCode, 0, 0, 0, 0, 0);
		    if (addSenseCode == 0x28)
			{
    			semTake (&pScsiPhysDev->blkDevList.listMutexSem, WAIT_FOREVER);

    			for (pBlkDevNode = (SCSI_BLK_DEV_NODE *)
			         lstFirst
				     (&pScsiPhysDev->blkDevList.blkDevNodes);
         		     pBlkDevNode != NULL;
         		     pBlkDevNode = (SCSI_BLK_DEV_NODE *)
				 lstNext (&pBlkDevNode->blkDevNode))
		 	    {
			    pBlkDevNode->scsiBlkDev.blkDev.bd_readyChanged =
			        TRUE;
       		  	    }

    			semGive (&pScsiPhysDev->blkDevList.listMutexSem);
			}
		    else if (addSenseCode == 0x29)
			{
			pScsiPhysDev->resetFlag = TRUE;
			}

		    /* issue a MODE SENSE command */

    		    {
    		    UINT8 modeSenseHeader [4];    /* mode sense data header */
    		    SCSI_COMMAND modeSenseCommand;/* SCSI command byte array */
    		    SCSI_TRANSACTION scsiXaction; /* info on a SCSI xaction */

    		    if (scsiCmdBuild (modeSenseCommand, &scsiXaction.cmdLength,
				      SCSI_OPCODE_MODE_SENSE,
				      pScsiPhysDev->scsiDevLUN, FALSE,
				      0, sizeof (modeSenseHeader), (UINT8) 0)
			== ERROR)
			goto cleanExit;

    		    scsiXaction.cmdAddress    = modeSenseCommand;
    		    scsiXaction.dataAddress   = modeSenseHeader;
    		    scsiXaction.dataDirection = O_RDONLY;
    		    scsiXaction.dataLength    = sizeof (modeSenseHeader);
    		    scsiXaction.addLengthByte = MODE_SENSE_ADD_LENGTH_BYTE;
		    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;

	    	    SCSI_DEBUG_MSG ("scsiTransact: issuing a MODE SENSE cmd.\n",
				    0, 0, 0, 0, 0, 0);

	    	    if ((status = scsiPhaseSequence (pScsiPhysDev,
						     &scsiXaction)) == ERROR)
			{
			SCSI_DEBUG_MSG ("scsiPhaseSequence returned ERROR\n",
					0, 0, 0, 0, 0, 0);
			goto cleanExit;
			}

	    	    /* MODE SENSE command status != GOOD indicates
		     * fatal error
		     */

	    	    if (scsiXaction.statusByte != SCSI_STATUS_GOOD)
			{
			SCSI_DEBUG_MSG ("scsiTransact: bad MODE SELECT stat.\n",
					0, 0, 0, 0, 0, 0);
			status = ERROR;
			goto cleanExit;
			}
	    	    else
			{
    			semTake (&pScsiPhysDev->blkDevList.listMutexSem, WAIT_FOREVER);

    			for (pBlkDevNode = (SCSI_BLK_DEV_NODE *)
			         lstFirst
				     (&pScsiPhysDev->blkDevList.blkDevNodes);
         		     pBlkDevNode != NULL;
         		     pBlkDevNode = (SCSI_BLK_DEV_NODE *)
				 lstNext (&pBlkDevNode->blkDevNode))
		 	    {
			    pBlkDevNode->scsiBlkDev.blkDev.bd_mode =
		    	        (modeSenseHeader [2] & (UINT8) 0x80) ? O_RDONLY
								     : O_RDWR;
       		  	    }

    			semGive (&pScsiPhysDev->blkDevList.listMutexSem);

			SCSI_DEBUG_MSG ("Write-protect bit = %x.\n",
			    	        (modeSenseHeader [2] & (UINT8) 0x80),
					0, 0, 0, 0, 0);
			}
    		    }
		    errnoSet (S_scsiLib_UNIT_ATTENTION);
	            status = ERROR;
	            goto cleanExit;
	            }
	        case SCSI_SENSE_KEY_DATA_PROTECT:
	            {
		    SCSI_DEBUG_MSG ("scsiTransact: Data Protect Sense,",
				    0, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
				    addSenseCode, 0, 0, 0, 0, 0);
		    if (addSenseCode == 0x27)
			errnoSet (S_scsiLib_WRITE_PROTECTED);
	            status = ERROR;
	            goto cleanExit;
	            }
	        default:
		    {
		    SCSI_DEBUG_MSG ("scsiTransact: Sense = %x,",
				    senseKey, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
				    addSenseCode, 0, 0, 0, 0, 0);
	            status = ERROR;
	            goto cleanExit;
	            }
	        }
	    }

	case SCSI_STATUS_BUSY:
	    errnoSet (S_scsiLib_DEV_NOT_READY);
	    SCSI_DEBUG_MSG ("device returned busy status.\n", 0, 0, 0, 0, 0, 0);
	    status = ERROR;
	    break;

	default:
	    status = ERROR;
	}

cleanExit:

    /* give back controller and device mutual exclusion semaphores */

    semGive (&pScsiCtrl->ctrlMutexSem);
    semGive (&pScsiPhysDev->devMutexSem);

    /* restore task's normal priority, if previously altered */

    if (pScsiCtrl->scsiPriority != NONE)
        taskPrioritySet (taskId, taskPriority);

    return (status);
    }

/*******************************************************************************
*
* scsi1Ioctl - perform a device-specific control function
*
* This routine performs a specified function using a specified SCSI block 
* device.
*
* RETURNS: The status of the request, or ERROR if the request is unsupported.
*/

LOCAL STATUS scsi1Ioctl
    (
    SCSI_PHYS_DEV *pScsiPhysDev,/* ptr to SCSI block device info */
    int function,               /* function code */
    int arg                     /* argument to pass called function */
    )
    {
    switch (function)
        {
        case FIOSCSICOMMAND:
	    return ((*pScsiPhysDev->pScsiCtrl->scsiTransact)
		    (pScsiPhysDev, (SCSI_TRANSACTION *) arg));

        case FIODISKFORMAT:
	    /* issue a FORMAT UNIT command with default parameters */
	    return (scsiFormatUnit (pScsiPhysDev, 0, 0, 0, 0,
				    (char *) NULL, 0));

        default:
	    errnoSet (S_ioLib_UNKNOWN_REQUEST);
	    return (ERROR);
        }
    }

/*******************************************************************************
*
* scsiBlkDevIoctl - call scsiIoctl() with pointer to SCSI physical device
*		    associated with the specified SCSI block device
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
*  scsiBuildByteMsgOut - Build a single byte message out. 
*
* 
* Typically, this routine is not called directly by the user, but by other
* routines in scsiLib.  It fills an array supplied by the user with the message
* code and check if it's a legal message for an initiator.
* If the `scsiMsgType' is SCSI_MSG_IDENT_DISCONNECT or SCSI_MSG_IDENTIFY a 
* status ERROR will be returned.  The disconnect and identify message have to be 
* built by the driver. 
*
* RETURNS:OK  ,otherwise ERROR if pMsgout is NULL or the scsiMsgType is not
* supported or not correct for an initiator.
*
* NOMANUAL
*/

STATUS scsiBuildByteMsgOut
    (
    UINT8 *pMsgOut,			/* ptr to message array */
    UINT8 scsiMsgType 			/* Single byte message code to send */
    )

    {
    if (pMsgOut == NULL )
        return (ERROR);

    /* Check for a legal Message */

    switch ( scsiMsgType )
	{
        case SCSI_MSG_COMMAND_COMPLETE:
        case SCSI_MSG_EXTENDED_MESSAGE: 
	case SCSI_MSG_SAVE_DATA_POINTER:
	case SCSI_MSG_RESTORE_POINTERS: 
	case SCSI_MSG_DISCONNECT:
	case SCSI_MSG_LINK_CMD_COMPLETE:
	case SCSI_MSG_LINK_CMD_FLAG_COMP:
        case SCSI_MSG_IDENT_DISCONNECT:
        case SCSI_MSG_IDENTIFY:
            /* That could not be send by an initiator or that 's handle by the 
               driver ( identify message ) */
            return ( ERROR);
        
	case SCSI_MSG_INITOR_DETECT_ERR:
	case SCSI_MSG_ABORT:
	case SCSI_MSG_MESSAGE_REJECT:
	case SCSI_MSG_NO_OP:
	case SCSI_MSG_MSG_PARITY_ERR:
	case SCSI_MSG_BUS_DEVICE_RESET:
            *pMsgOut = scsiMsgType;
            break;

	default:
            /* unknown message */
            return (ERROR);
	}
    return (OK);
    }

/*******************************************************************************
*
*  scsiBuildExtMsgOut - Build a skeleton for an extended message out. 
*
* 
* Typically, this routine is not called directly by the user, but by other
* routines in scsiLib.  It fills an array supplied by the user with the message
* code and checks if it's a legal message.  The `scsiMsgType' is the extended
* message type code.  It fills only the fixed fields in the message out 
* array.  The specific values for the extended message have to be filled by
* the caller. 
*    
*
* RETURNS:OK  ,otherwise ERROR if pMsgout is NULL or the scsiMsgType is not 
* supported.
*
* NOMANUAL
*/

STATUS scsiBuildExtMsgOut
    (
    UINT8 *pMsgOut,			/* ptr to message array */
    UINT8 scsiMsgType 			/* Single byte message code to send */
    )

    {
    if (pMsgOut == NULL )
        return (ERROR);

    /* Check for a legal Message */

    switch ( scsiMsgType )
	{
	case SCSI_EXT_MSG_SYNC_XFER_REQ:
	    *pMsgOut++ = SCSI_MSG_EXTENDED_MESSAGE;
            *pMsgOut++ = SCSI_SYNC_XFER_REQ_MSG_LENGTH;
            *pMsgOut   = SCSI_EXT_MSG_SYNC_XFER_REQ;
	    break;

	case SCSI_EXT_MSG_WIDE_XFER_REQ:
	    *pMsgOut++ = SCSI_MSG_EXTENDED_MESSAGE;
            *pMsgOut++ = SCSI_WIDE_XFER_REQ_MSG_LENGTH;
            *pMsgOut++ = SCSI_EXT_MSG_WIDE_XFER_REQ;
	    break;

	default:
            /* unknown message or not supportted by an initiator */
            return (ERROR);
	}
    return (OK);
    }


/*******************************************************************************
*
*  scsiSyncTarget - initiate an agreement for a synchronous protocol
*
* This routine could be used to setup a synchronous protocol between the 
* initiator and a scsi target.  It uses the scsi test unit ready command with 
* a selection with attention to negotiate a pair of extended sync messages.
* It builds an extended sync message out with the `syncVal' and `offsetVal' 
* round up to the closest value supported by the controller.
*  
*.CS
* `pScsiPhysDev' 	pointer to the SCSI_PHYS_DEV info.
*
* `syncVal' 		wished scsi REQ/ACK period value in 4ns step 
*			(e.g 40 means 160ns).
*
* `offsetVal' 		wished offset value.
*
* `pSyncVal'		pointer used to return the controller values used and 
*			the target response values in a SCSI_SYNC_AGREEMENT 
*			structure.
*.CE
*
* The last parameter allows the caller to know the round up values sent 
* in the initiator message out and what are the values returned by the 
* target.   
*.CS
*
* typedef struct 
*     {  
*     int syncPeriodCtrl;   /@ round up period available with the controller @/
*     int syncOffsetCtrl;   /@ round up offset available with the controller @/
*     int syncPeriodTarget; /@ Period value returned by the target @/
*     int syncOffsetTarget; /@ Offset value returned by the target @/
*     } SCSI_SYNC_AGREEMENT;
*
* NOTE: As this routine is setting up and using the SCSI_PHYS_DEV structure, it
* must be used when no scsi activity occurs on the device.
* A good matter to set up the synchronous protocol is to send this command 
* after a scsiPhysDevCreate and before any other scsi initialisation to be 
* sure that no other program is using this device.
* 
* RETURNS: OK means that the agreement sync is OK with the target values 
* except if you get back an offset and/or sync equal to 0 
* (Async agreement value or reject message).
* If the driver does not support any synchronous capability, an error status 
* will be also returned with an `S_scsiLib_SYNC_UNSUPPORTED' error code.
*
* NOMANUAL
*/

static STATUS scsiSyncTarget
    (
    SCSI_PHYS_DEV *pScsiPhysDev,    /* ptr to phys dev info */
    int syncVal,		    /* Period value wishes */
    int offsetVal,		    /* Offset value wishes */
    SCSI_SYNC_AGREEMENT *pSyncVal   /* where to return ctrl and target values */
    )

    {
    UINT8 *pMsgOut;	/* pointer to msg out array */
    STATUS status;
    SCSI_CTRL *pCtrl;

    if ((pScsiPhysDev == (SCSI_PHYS_DEV *) NULL) ||
        (pSyncVal     == NULL ))	
	{
        errnoSet (S_scsiLib_ILLEGAL_PARAMETER);
        return (ERROR);
	}

    pMsgOut = SCSI_GET_PTR_MSGOUT(pScsiPhysDev);
    pCtrl   = (SCSI_CTRL *)pScsiPhysDev->pScsiCtrl;

    /* Clear return parameter */
    pSyncVal->syncPeriodCtrl = 0;
    pSyncVal->syncOffsetCtrl = 0;
    pSyncVal->syncPeriodTarget = 0;
    pSyncVal->syncOffsetTarget = 0;

    /* Set Value for the device in SCSI_PHYS_DEV info,that will be used by 
     * the scsiTransact routine to set up the values in a message out.
     */

    pScsiPhysDev->syncXferPeriod = syncVal * SCSI_SYNC_STEP_PERIOD;
    pScsiPhysDev->syncXferOffset = offsetVal;

    /* Call the driver routine to find out the closest controller period/offset
     * values, just to send back to the caller the round up values possible
     * for the SCSI controller.  That checks also if the driver is supporting
     * a such functionality.
     */

    pMsgOut[3] = (UINT8) syncVal;
    pMsgOut[4] = (UINT8) offsetVal;

    if (pCtrl->scsiSyncMsgConvert == (FUNCPTR) NULL)
       {
       errnoSet (S_scsiLib_SYNC_UNSUPPORTED);
       return (ERROR);
       }

    status = (*pCtrl->scsiSyncMsgConvert) (pScsiPhysDev,pMsgOut,
                        &pSyncVal->syncPeriodCtrl,&pSyncVal->syncOffsetCtrl);
    if (status == ERROR)
        return (ERROR);

    /* Set useMsgout */

    pScsiPhysDev->useMsgout = SCSI_SYNC_MSGOUT;

    /* send an scsi command to negotiate for sync transfer */
    status = scsiTestUnitRdy(pScsiPhysDev);

    /* Reset useMsgout */

    pScsiPhysDev->useMsgout = SCSI_NO_MSGOUT;

    /* If the status is in error the target have been reset to be sure to keep 
     * the target in assync mode because the sync value can't be matched by the 
     * scsi controller or the target does not support sync protocol and
     * answer a reject message.  In case of reset ,the target will 
     * be send a sense key like "unit attention" on the next command 
     * , we send a new request to clear it on the target.
     */
    if (status == ERROR ) 
        {
        status = scsiTestUnitRdy(pScsiPhysDev);
        status = ERROR;
        }
    
    /* copy target value returned in structure */

    pSyncVal->syncPeriodTarget  = (pScsiPhysDev->syncXferPeriod)
						/SCSI_SYNC_STEP_PERIOD;
    pSyncVal->syncOffsetTarget = pScsiPhysDev->syncXferOffset; 

    SCSI_DEBUG_MSG("scsiSyncTarget:target syncXferPeriod =%d\n",
				pScsiPhysDev->syncXferPeriod, 0, 0, 0, 0, 0);

    SCSI_DEBUG_MSG("scsiSyncTarget:target syncXferOffset =%d\n",
				pScsiPhysDev->syncXferOffset, 0, 0, 0, 0, 0);
    return(status);
    }

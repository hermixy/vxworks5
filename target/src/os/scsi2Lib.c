/* scsi2Lib.c - Small Computer System Interface (SCSI) library (SCSI-2) */

/* Copyright 1989-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
04l,07mar02,pmr  SPR 73383: no division-by-zero in scsiTargetOptionsShow().
04k,16oct01,pmr  SPR 67310: turning off message-using options when messaging
                 is turned off in scsiTargetOptionsSet().
04j,08oct01,rcs  tested for removable and medium not present in 
                 scsiPhysDevCreate() SPR# 32203
04i,05oct01,rcs  added display of ASCQ (Additional Sense Code Qualifier) to 
                 scsi2Transact() SCSI_DEBUG_MSGs. SPR# 29269  
04h,03oct01,rcs  clarified description of xferWidth in scsiTargetOptionsSet()
                 SPR# 69003 
04g,26sep01,rcs  fixed scsi2PhysDevCreate() to use user specified `numBlocks' 
                 or `blockSize' SPR# 22635
04f,26sep01,rcs  fixed scsiTargetOptionsSet() to allow for narrow width
                 to be explicitly set for a narrow target. SPR# 28262 
04e,25sep01,rcs  added routine scsiTargetOptionsShow() SPR# 69223
04d,24sep01,rcs  set errno for MEDIUM ERROR, HARDWARE ERROR, ILLEGAL REQUEST, 
                 BLANK CHECK, ABORTED COMMAND, and VOLUME OVERFLOW in switch
                 statement in scsi2Transact(). SPR# 21236 
04c,05oct98,jmp  doc: cleanup.
04b,28aug97,dds  SPR 3425: "scsiFormatUnit" command, times out for disks which 
                 take a long time to format.
04a,14aug97,dds  Rework of SPR 8219.
03z,28jul97,dds  SPR 8219: switching from synch to async mode cause target
                 to hang.
03y,24jul97,dds  SPR 8323 / 7838: scsiTargetOptionsGet returns "xferWidth"
03x,10jul97,dds  added library support for adaptec chips.
03w,09jul97,dgp  doc: correct fonts per SPR 7853
03v,28mar97,dds  SPR 8220: added check for parity errors.
03u,28oct96,dgp  doc: editing for newly published SCSI libraries
03t,21oct96,dds  removed NOMANUAL for functions called from the 
                 driver interface.
03s,14sep96,dds  removed the scsiMgr and scsiCtrl functions. These 
                 functions are found in scsiMgrLib.c and scsiCtrlLib.c
03r,30jul96,jds  fixed scsiCacheSynchronize() to act on cache lines only
03q,29jul96,jds  added support for hardware snooping 
03p,06may96,jds  more doc tweaks
03o,01may96,jds  doc tweaks
03n,22apr96,jds  doc tweaks to describe WIDE transfers
03m,20apr96,jds  added WIDE data transfer support
03l,25jan96,jds  added scsiCtrl routine modifications based on SPARC/5390
03k,13nov95,jds  minor document changes
03j,08nov95,jds  more doc cleanup
03i,20sep95,jdi  doc cleanup.
03h,30aug95,jds  bug fixes and cleanup. Bugs include several instances where
		 function was returning with semaphore taken.
03g,26jul95,jds  added scsiReadBlockLimits in scsiPhysDevCreate for SEQ devs
03f,25jul95,jds  added support in scsiTransact for sequential devices. Created
                 chkMedChangeAndWP (). Made appropriate changes in other
		 routines as well.e.g scsiPhysDevCreate ()
03e,19jul95,jds  removed all SCSI direct access commands and put them in 
                 scsiDirectLib, also moved blkDev routines.
03d,21mar95,ihw  minor bug fixes in "scsiCtrl*()" routines
03c,07oct94,ihw  errno now set if sense data is unrecognised
		 error message logged if max number of threads is reached
03b,27may94,ihw  documented prior to release
03a,30mar94,ihw  modified for enhanced SCSI library: tagged command queueing
                    major architectural changes ...
                 ABORT rather than REJECT used to reject invalid reselection
		 removed redundant code for additional length in data in xfer
		 fixed incorrect handling of ext msg in with zero length byte
02b,25feb94,ihw  fixed synchronous transfer message-in diagnostics
02a,14jan94,ihw  modified for enhanced SCSI library: multiple initiators,
    	    	    disconnect/reconnect and synchronous transfer supported
		 "scsiSyncXferNegotiate()" made non-local for NCR driver
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
initiator function as defined in the SCSI-2 ANSI specification. This library 
does not support a VxWorks target acting as a SCSI target.

The implementation is transaction based.  A transaction is defined as the
selection of a SCSI device by the initiator, the issuance of a SCSI command,
and the sequence of data, status, and message phases necessary to perform
the command.  A transaction normally completes with a "Command Complete"
message from the target, followed by disconnection from the SCSI bus.  If
the status from the target is "Check Condition," the transaction
continues; the initiator issues a "Request Sense" command to gain more
information on the exception condition reported.

Many of the subroutines in scsi2Lib facilitate the transaction of
frequently used SCSI commands.  Individual command fields are passed as
arguments from which SCSI Command Descriptor Blocks are constructed, and
fields of a SCSI_TRANSACTION structure are filled in appropriately.  This
structure, along with the SCSI_PHYS_DEV structure associated with the
target SCSI device, is passed to the routine whose address is indicated by
the 'scsiTransact' field of the SCSI_CTRL structure associated with the
relevant SCSI controller. The above mentioned structures are defined in 
scsi2Lib.h.

The function variable 'scsiTransact' is set by the individual SCSI
controller driver.  For off-board SCSI controllers, this
routine rearranges the fields of the SCSI_TRANSACTION structure into
the appropriate structure for the specified hardware, which then carries out
the transaction through firmware control.  Drivers for an on-board
SCSI-controller chip can use the scsiTransact() routine in scsiLib (which
invokes the scsi2Transact() routine in scsi2Lib), as long as they provide the 
other functions specified in the SCSI_CTRL structure.

SCSI TRANSACTION TIMEOUT
Associated with each transaction is a time limit (specified in microseconds,
but measured with the resolution of the system clock).  If the transaction
has not completed within this time limit, the SCSI library aborts it; the
called routine fails with a corresponding error code.  The timeout period
includes time spent waiting for the target device to become free to accept
the command.

The semantics of the timeout should guarantee that the caller waits no 
longer than the transaction timeout period, but in practice this
may depend on the state of the SCSI bus and the connected target device when
the timeout occurs.  If the target behaves correctly according to the SCSI
specification, proper timeout behavior results.  However, in certain
unusual cases--for example, when the target does not respond to an asserted 
ATN signal--the caller may remain blocked for longer than the timeout period.

If the transaction timeout causes problems in your system, you can set the
value of either or both the global variables "scsi{Min,Max}Timeout".  These
specify (in microseconds) the global minimum and maximum timeout periods,
which override (clip) the value specified for a transaction.  They may be
changed at any time and affect all transactions issued after the new
values are set.  The range of both these variable is 0 to 0xffffffff (zero to
about 4295 seconds).

SCSI TRANSACTION PRIORITY
Each transaction also has an associated priority used by the SCSI
library when selecting the next command to issue when the SCSI system is
idle.  It chooses the highest priority transaction that can be dispatched
on an available physical device.  If there are several equal-priority
transactions available, the SCSI library uses a simple round-robin
scheme to avoid favoring the same physical device.

Priorities range from 0 (highest) to 255 (lowest), which is the same as task
priorities.  The priority SCSI_THREAD_TASK_PRIORITY can be used to give the
transaction the same priority as the calling task (this is the method used
internally by this SCSI-2 library).

SUPPORTED SCSI DEVICES
This library requires peripherals that conform to the SCSI-2 ANSI
standard; in particular, the INQUIRY, REQUEST SENSE, and
TEST UNIT READY commands must be supported as specified by this standard.
In general, the SCSI library is self-configuring to work with any device
that meets these requirements.

Peripherals that support identification and the SCSI message protocol are
strongly recommended as these provide maximum performance.

In theory, all classes of SCSI devices are supported.  The scsiLib library
provides the capability to transact any SCSI command on any SCSI device
through the FIOSCSICOMMAND function of the scsiIoctl() routine (which 
invokes the scsi2Ioctl() routine in scsi2Lib).

Only direct-access devices (disks) are supported by file systems like dosFs, 
rt11Fs and rawFs. These file systems employ routines in 
scsiDirectLib (most of which are described in scsiLib but defined in 
scsiDirectLib). In the case of sequential-access devices (tapes), higher-level
tape file systems, like tapeFs,  make use of scsiSeqLib. For other types of 
devices, additional, higher-level software is necessary to map user-level
commands to SCSI transactions.

DISCONNECT/RECONNECT SUPPORT
The target device can be disconnected from the SCSI bus while it carries
out a SCSI command; in this way, commands to multiple SCSI devices can
be overlapped to improve overall SCSI throughput.  There are no
restrictions on the number of pending, disconnected commands or the
order in which they are resumed.  The SCSI library serializes access to the
device according to the capabilities and status of the device (see the
following section).

Use of the disconnect/reconnect mechanism is invisible to users of the SCSI
library. It can be enabled and disabled separately for each target device
(see scsiTargetOptionsSet()).  Note that support for disconnect/reconnect
depends on the capabilities of the controller and its driver (see below).

TAGGED COMMAND QUEUEING SUPPORT
If the target device conforms to the ANSI SCSI-2 standard and indicates
(using the INQUIRY command) that it supports command queuing, the SCSI
library allows new commands to be started on the device whenever the SCSI
bus is idle.  That is, it executes multiple commands concurrently on the
target device.  By default, commands are tagged with a SIMPLE QUEUE TAG
message.  Up to 256 commands can be executing concurrently.

The SCSI library correctly handles contingent allegiance conditions that
arise while a device is executing tagged commands.  (A contingent
allegiance condition exists when a target device is maintaining sense data that
the initiator should use to correctly recover from an error condition.)  It
issues an untagged REQUEST SENSE command, and stops issuing tagged commands
until the sense recovery command has completed.

For devices that do not support command queuing, the SCSI library only
issues a new command when the previous one has completed.  These devices
can only execute a single command at once.

Use of tagged command queuing is normally invisible to users of the SCSI
library.  If necessary, the default tag type and maximum number of tags may
be changed on a per-target basis, using scsiTargetOptionsSet().

SYNCHRONOUS TRANSFER PROTOCOL SUPPORT
If the SCSI controller hardware supports the synchronous transfer
protocol, scsiLib negotiates with the target device to determine
whether to use synchronous or asynchronous transfers.  Either VxWorks
or the target device may start a round of negotiation.  Depending on the
controller hardware, synchronous transfer rates up to the maximum allowed
by the SCSI-2 standard (10 Mtransfers/second) can be used.

Again, this is normally invisible to users of the SCSI library, but
synchronous transfer parameters may be set or disabled on a per-target basis
by using scsiTargetOptionsSet().

WIDE DATA TRANSFER SUPPORT
If the SCSI controller supports the wide data transfer protocol, scsiLib
negotiates wide data transfer parameters with the target device, if
that device also supports wide transfers. Either VxWorks or the target device
may start a round of negotiation. Wide data transfer parameters are 
negotiated prior to the synchronous data transfer parameters, as 
specified by the SCSI-2 ANSI specification. In conjunction with
synchronous transfer, up to a maximum of 20MB/sec. can be attained.

Wide data transfer negotiation is invisible to users of this library,
but it is possible to enable or disable wide data transfers and the
parameters on a per-target basis by using scsiTargetOptionsSet().

SCSI BUS RESET
The SCSI library implements the ANSI "hard reset" option.  Any transactions
in progress when a SCSI bus reset is detected fail with an error code
indicating termination due to bus reset.  Any transactions waiting to start
executing are then started normally.

CONFIGURING SCSI CONTROLLERS
The routines to create and initialize a specific SCSI controller are
particular to the controller and normally are found in its library
module.  The normal calling sequence is:
.ne 4
.CS
    xxCtrlCreate (...);	/@ parameters are controller specific @/
    xxCtrlInit (...);	/@ parameters are controller specific @/
.CE
The conceptual difference between the two routines is that xxCtrlCreate()
calloc's memory for the xx_SCSI_CTRL data structure and initializes
information that is never expected to change (for example, clock rate).  The
remaining fields in the xx_SCSI_CTRL structure are initialized by
xxCtrlInit() and any necessary registers are written on the SCSI
controller to effect the desired initialization.  This routine can be
called multiple times, although this is rarely required.  For example, the
bus ID of the SCSI controller can be changed without rebooting the VxWorks
system.

CONFIGURING PHYSICAL SCSI DEVICES
Before a device can be used, it must be "created," that is, declared.
This is done with scsiPhysDevCreate() and can only be done after a
SCSI_CTRL structure exists and has been properly initialized.

.CS
SCSI_PHYS_DEV *scsiPhysDevCreate
    (
    SCSI_CTRL * pScsiCtrl,/@ ptr to SCSI controller info @/
    int  devBusId,	  /@ device's SCSI bus ID @/
    int  devLUN,	  /@ device's logical unit number @/
    int  reqSenseLength,  /@ length of REQUEST SENSE data dev returns @/
    int  devType,	  /@ type of SCSI device @/
    BOOL removable,	  /@ whether medium is removable @/
    int  numBlocks,	  /@ number of blocks on device @/
    int  blockSize	  /@ size of a block in bytes @/
    )
.CE

Several of these parameters can be left unspecified, as follows:
.iP <reqSenseLength>
If 0, issue a REQUEST_SENSE to determine a request sense length.
.iP <devType>
This parameter is ignored: an INQUIRY command is used to ascertain the
device type.  A value of NONE (-1) is the recommended placeholder.
.iP "<numBlocks>, <blockSize>"
If 0, issue a READ_CAPACITY to determine the number of blocks.
.LP

The above values are recommended, unless the device does not support the
required commands, or other non-standard conditions prevail.

LOGICAL PARTITIONS ON DIRECT-ACCESS BLOCK DEVICES
It is possible to have more than one logical partition on a SCSI block
device.  This capability is currently not supported for removable media
devices.  A partition is an array of contiguously addressed blocks
with a specified starting block address and specified number of blocks.
The scsiBlkDevCreate() routine is called once for each block device
partition.  Under normal usage, logical partitions should not overlap.
.ne 8
.CS
SCSI_BLK_DEV *scsiBlkDevCreate
    (
    SCSI_PHYS_DEV *  pScsiPhysDev,  /@ ptr to SCSI physical device info @/
    int              numBlocks,	    /@ number of blocks in block device @/
    int              blockOffset    /@ address of first block in volume @/
    )
.CE

Note that if <numBlocks> is 0, the rest of the device is used.

ATTACHING DISK FILE SYSTEMS TO LOGICAL PARTITIONS
Files cannot be read or written to a disk partition until a file system (for
example, dosFs, rt11Fs, or rawFs) has been initialized on the partition.
For more information, see the relevant documentation in dosFsLib,
rt11FsLib, or rawFsLib.

USING A SEQUENTIAL-ACCESS BLOCK DEVICE
The entire volume (tape) on a sequential-access block device is treated as 
a single raw file. This raw file is made available to higher-level layers
like tapeFs by the scsiSeqDevCreate() routine, described in scsiSeqLib. The
scsiSeqDevCreate() routine is called once for a given SCSI physical 
device.
.CS
SEQ_DEV *scsiSeqDevCreate
    (
    SCSI_PHYS_DEV *pScsiPhysDev  /@ ptr to SCSI physical device info @/
    )
.CE

TRANSMITTING ARBITRARY COMMANDS TO SCSI DEVICES
The scsi2Lib, scsiCommonLib, scsiDirectLib, and scsiSeqLib libraries 
collectively provide routines that implement all mandatory SCSI-2 
direct-access and sequential-access commands. Still, there are situations that
require commands that are not supported by these libraries (for example, 
writing software that needs to use an optional SCSI-2 command).
Arbitrary commands are handled with the FIOSCSICOMMAND option to
scsiIoctl().  The <arg> parameter for FIOSCSICOMMAND is a pointer to a
valid SCSI_TRANSACTION structure.  Typically, a call to scsiIoctl() is written
as a subroutine of the form:
.CS
STATUS myScsiCommand
    (
    SCSI_PHYS_DEV *  pScsiPhysDev,  /@ ptr to SCSI physical device     @/
    char *	     buffer,	    /@ ptr to data buffer              @/
    int		     bufLength,	    /@ length of buffer in bytes       @/
    int		     someParam	    /@ param. specifiable in cmd block @/
    )

    {
    SCSI_COMMAND myScsiCmdBlock;	/@ SCSI command byte array @/
    SCSI_TRANSACTION myScsiXaction;	/@ info on a SCSI transaction @/

    /@ fill in fields of SCSI_COMMAND structure @/

    myScsiCmdBlock [0] = MY_COMMAND_OPCODE;	/@ the required opcode @/
    .
    myScsiCmdBlock [X] = (UINT8) someParam;	/@ for example @/
    .
    myScsiCmdBlock [N-1] = MY_CONTROL_BYTE;	/@ typically == 0 @/

    /@ fill in fields of SCSI_TRANSACTION structure @/

    myScsiXaction.cmdAddress    = myScsiCmdBlock;
    myScsiXaction.cmdLength     = <# of valid bytes in myScsiCmdBlock>;
    myScsiXaction.dataAddress   = (UINT8 *) buffer;
    myScsiXaction.dataDirection = <O_RDONLY (0) or O_WRONLY (1)>;
    myScsiXaction.dataLength    = bufLength;
    myScsiXaction.addLengthByte = 0;	    	/@ no longer used @/
    myScsiXaction.cmdTimeout    = <timeout in usec>;
    myScsiXaction.tagType       = SCSI_TAG_{DEFAULT,UNTAGGED,
					    SIMPLE,ORDERED,HEAD_OF_Q};
    myScsiXaction.priority      = [ 0 (highest) to 255 (lowest) ];

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
scsiLib.h, scsi2Lib.h

SEE ALSO: dosFsLib, rt11FsLib, rawFsLib, tapeFsLib, scsiLib, scsiCommonLib,
scsiDirectLib, scsiSeqLib, scsiMgrLib, scsiCtrlLib,
.I  "American National Standard for Information Systems - Small Computer" 
.I  "System Interface (SCSI-2), ANSI X3T9,"
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

/* globals */

extern int scsiPhysDevMutexOptions;

IMPORT BOOL scsiErrors;			/* enable SCSI error messages */
IMPORT BOOL scsiDebug;			/* enable task level debug messages */
IMPORT BOOL scsiIntsDebug;		/* enable int level debug messages */

int scsiCtrlMutexSemOptions = SEM_Q_PRIORITY |
			      SEM_INVERSION_SAFE |
			      SEM_DELETE_SAFE;

int scsiMaxNumThreads       = SCSI_DEF_MAX_THREADS;
int scsiAllocNumThreads     = SCSI_DEF_ALLOC_THREADS;


UINT scsiMinTimeout         = SCSI_DEF_MIN_TIMEOUT;
UINT scsiMaxTimeout         = SCSI_DEF_MAX_TIMEOUT;

int scsiCacheFlushThreshold = SCSI_DEF_CACHE_FLUSH_THRESHOLD;

UINT scsiTestUnitRdyTrys = 5;           /* Times to retry scsiTestUnitReady() */

IMPORT SCSI_CTRL * pSysScsiCtrl;	/* ptr to default SCSI_CTRL struct */

VOID               scsiTargetReset (SCSI_CTRL * pScsiCtrl, UINT busId);
VOID               scsiThreadListShow   (LIST * pList);
VOID               scsiThreadListIdShow (LIST * pList);

/* forward static functions */


/* Backward compatability functions localised */

LOCAL STATUS          scsi2CtrlInit (SCSI_CTRL *);
LOCAL SCSI_PHYS_DEV * scsi2PhysDevIdGet (SCSI_CTRL *, int, int);

LOCAL STATUS          scsi2PhysDevDelete (FAST SCSI_PHYS_DEV *);
LOCAL SCSI_PHYS_DEV * scsi2PhysDevCreate (SCSI_CTRL *, int, int, int, int, BOOL,
                                        int, int);
LOCAL STATUS          scsi2AutoConfig (SCSI_CTRL *);
LOCAL STATUS          scsi2Show (FAST SCSI_CTRL *);
LOCAL STATUS          scsi2BusReset (SCSI_CTRL *);
LOCAL STATUS          scsi2CmdBuild (SCSI_COMMAND, int *, UINT8, int, BOOL, int,
                                                                int, UINT8);
LOCAL STATUS          scsi2Transact (SCSI_PHYS_DEV *, SCSI_TRANSACTION *);
LOCAL STATUS          scsi2Ioctl (SCSI_PHYS_DEV *, int, int);
LOCAL char *          scsi2PhaseNameGet (int);

/* Miscellaneous support routines */

LOCAL void          scsiTargetInit  (SCSI_CTRL * pScsiCtrl, UINT busId);
LOCAL BOOL   	    strIsPrintable (char *pString);

LOCAL UINT          scsiTimeoutCvt  (UINT uSecs);
LOCAL SCSI_PRIORITY scsiPriorityCvt (UINT priority);
LOCAL SCSI_THREAD * scsiThreadCreate (SCSI_PHYS_DEV    * pScsiPhysDev,
				      SCSI_TRANSACTION * pScsiXaction);
LOCAL void          scsiThreadDelete (SCSI_THREAD * pThread);
LOCAL SCSI_THREAD * scsiThreadAllocate (SCSI_CTRL * pScsiCtrl);
LOCAL void          scsiThreadDeallocate (SCSI_CTRL * pScsiCtrl,
					  SCSI_THREAD * pThread);
LOCAL char *        scsiThreadArrayCreate (SCSI_CTRL * pScsiCtrl, int nThreads);
LOCAL STATUS        scsiThreadExecute (SCSI_THREAD * pThread);
LOCAL STATUS        scsiCommand (SCSI_PHYS_DEV * pScsiPhysDev,
				 SCSI_TRANSACTION * pScsiXaction);
LOCAL void 	    chkMedChangeAndWP (int addSenseCode, 
						  SCSI_PHYS_DEV * pScsiPhysDev);

/* Imported functions from other scsi*Lib modules */

IMPORT void scsiDirectLibTblInit ();
IMPORT void scsiCommonLibTblInit ();

/*******************************************************************************
*
* scsi2IfInit - initialize the SCSI-2 interface to scsiLib
*
* This routine initializes the SCSI-2 function interface by adding all the 
* routines in scsi2Lib plus those in scsiDirectLib and scsiCommonLib. It is
* invoked by usrConfig.c if the macro INCLUDE_SCSI2 is defined in
* config.h.  The calling interface remains the same between SCSI-1 and SCSI-2;
* this routine simply sets the calling interface function pointers to 
* the SCSI-2 functions.
*
* RETURNS: N/A
*/

void scsi2IfInit ()
    {
    /* Allocate memory for the interface table */

    pScsiIfTbl = (SCSI_FUNC_TBL *) calloc (1,sizeof (SCSI_FUNC_TBL));

    if (pScsiIfTbl == NULL)
	return;

    /* Initialisation of functions in this module */

    pScsiIfTbl->scsiCtrlInit      = (FUNCPTR) scsi2CtrlInit;
    pScsiIfTbl->scsiPhaseNameGet  = (FUNCPTR) scsi2PhaseNameGet;
    pScsiIfTbl->scsiPhysDevCreate = (FUNCPTR) scsi2PhysDevCreate;
    pScsiIfTbl->scsiPhysDevDelete = (FUNCPTR) scsi2PhysDevDelete;
    pScsiIfTbl->scsiPhysDevIdGet  = (FUNCPTR) scsi2PhysDevIdGet;
    pScsiIfTbl->scsiAutoConfig    = (FUNCPTR) scsi2AutoConfig;
    pScsiIfTbl->scsiShow          = (FUNCPTR) scsi2Show;
    pScsiIfTbl->scsiBusReset      = (FUNCPTR) scsi2BusReset;
    pScsiIfTbl->scsiCmdBuild      = (FUNCPTR) scsi2CmdBuild;
    pScsiIfTbl->scsiTransact      = (FUNCPTR) scsi2Transact;
    pScsiIfTbl->scsiIoctl         = (FUNCPTR) scsi2Ioctl;

    /* Initialisation of functions in other modules */

    scsiDirectLibTblInit ();
    scsiCommonLibTblInit ();
    }


/*******************************************************************************
*
* scsi2CtrlInit - initialize generic fields of a SCSI_CTRL structure
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
* Many of the fields in the SCSI_CTRL structure are used to set up virtual
* functions and configuration data for the controller drivers.  If these
* fields are left set to zero, default values are used.  These values assume
* that a bus-level controller driver is used in conjunction with the
* generic thread-level driver in this source file (see "scsiCtrlxxx()").
*
* INTERNAL
* This function is really initialising too many things and should probably
* be broken into several functions such as: scsiMgrInit, scsiCtrlInit (i.e.
* the "generic" controller), and an overall initialisation function that
* can be called by a driver create routine, as the current routine is.
*
* RETURNS: OK, or ERROR if a semaphore creation fails.
*
* NOMANUAL
*/

LOCAL STATUS scsi2CtrlInit
    (
    SCSI_CTRL * pScsiCtrl       /* ptr to SCSI_CTRL struct to initialize */
    )
    {
    int ix;			/* loop index */

    /*
     *	Initialize virtual functions and configuration info.
     *
     *	If the driver has left the default value (0), set to the values
     *	for the generic thread-level driver.
     */
    /* structure sizes */
    
    if (pScsiCtrl->eventSize == 0)
    	pScsiCtrl->eventSize = sizeof (SCSI_EVENT);

    if (pScsiCtrl->threadSize == 0)
    	pScsiCtrl->threadSize = sizeof (SCSI_THREAD);

    /* virtual functions bus-level driver */

    if (pScsiCtrl->scsiTransact == 0)
    	pScsiCtrl->scsiTransact = scsiTransact;

    if (pScsiCtrl->scsiEventProc == 0)
    	pScsiCtrl->scsiEventProc = scsiCtrlEvent;

    if (pScsiCtrl->scsiThreadInit == 0)
    	pScsiCtrl->scsiThreadInit = scsiCtrlThreadInit;

    if (pScsiCtrl->scsiThreadActivate == 0)
    	pScsiCtrl->scsiThreadActivate = scsiCtrlThreadActivate;

    if (pScsiCtrl->scsiThreadAbort == 0)
    	pScsiCtrl->scsiThreadAbort = scsiCtrlThreadAbort;
    
    /* create controller mutual exclusion semaphore */

    if ((pScsiCtrl->mutexSem = semMCreate (scsiCtrlMutexSemOptions)) == NULL)
        {
        printErr ("scsiCtrlInit: semMCreate of mutexSem failed.\n");
        goto failed;
        }

    /* create sync semaphore for controller events and client requests */

    if ((pScsiCtrl->actionSem = semBCreate (scsiMgrActionSemOptions,
					    SEM_EMPTY)) == NULL)
        {
        printErr ("scsiCtrlInit: semBCreate of actionSem failed.\n");
        goto failed;
        }

    /* create ring buffers for controller events and client requests */
    
    if ((pScsiCtrl->eventQ = rngCreate (scsiMgrEventQSize *
					pScsiCtrl->eventSize)) == NULL)
	{
	printErr ("scsiCtrlInit: rngCreate of eventQ failed\n");
        goto failed;
	}
    
    if ((pScsiCtrl->timeoutQ = rngCreate (scsiMgrTimeoutQSize *
					  sizeof (SCSI_TIMEOUT))) == NULL)
	{
	printErr ("scsiCtrlInit: rngCreate of timeoutQ failed\n");
        goto failed;
	}

    if ((pScsiCtrl->requestQ = rngCreate (scsiMgrRequestQSize *
					  sizeof (SCSI_REQUEST))) == NULL)
	{
	printErr ("scsiCtrlInit: rngCreate of requestQ failed\n");
        goto failed;
	}

    if ((pScsiCtrl->replyQ = rngCreate (scsiMgrReplyQSize *
					sizeof (SCSI_REPLY))) == NULL)
	{
	printErr ("scsiCtrlInit: rngCreate of replyQ failed\n");
        goto failed;
	}

    /* create thread used for incoming identification */

    if ((pScsiCtrl->pIdentThread = scsiCtrlIdentThreadCreate (pScsiCtrl)) == 0)
	{
	printErr ("scsiCtrlInit: can't create identification thread.\n");
	goto failed;
	}

    /* initialize the list of free threads */

    lstInit (&pScsiCtrl->freeThreads);

    /* initialize support for disconnect and sync. xfer to yes */
    /* (may be reset by driver) */

    pScsiCtrl->disconnect = TRUE;
    pScsiCtrl->syncXfer   = TRUE;

    /* 
     * The default behaviour is assumed to be to disallow wide transfers.
     * However, this default can be modified by the controller driver and
     * must be tested first.
     */

    if (pScsiCtrl->wideXfer != TRUE)
        pScsiCtrl->wideXfer   = FALSE;

    /* initialise controller state variables */

    pScsiCtrl->active   = FALSE;
    pScsiCtrl->nThreads = 0;
    pScsiCtrl->pThread  = 0;
    pScsiCtrl->nextDev  = 0;

    pScsiCtrl->msgInState  = SCSI_MSG_IN_NONE;
    pScsiCtrl->msgOutState = SCSI_MSG_OUT_NONE;

    /* initialize array of target information */

    for (ix = 0; ix < SCSI_MAX_TARGETS; ix++)
	{
	pScsiCtrl->targetArr [ix].scsiDevBusId = ix;
	scsiTargetInit (pScsiCtrl, ix);
	}
	
    /* initialize array of ptrs to SCSI_PHYS_DEV structures to NULL */

    for (ix = 0; ix < SCSI_MAX_PHYS_DEVS; ix++)
	{
	pScsiCtrl->physDevArr [ix] = (SCSI_PHYS_DEV *) NULL;
	}

    /* by default hardware snooping is disabled */

    pScsiCtrl->cacheSnooping = FALSE;

    return (OK);

failed:
    if (pScsiCtrl->mutexSem != 0)
	(void) semDelete (pScsiCtrl->mutexSem);

    if (pScsiCtrl->actionSem != 0)
	(void) semDelete (pScsiCtrl->actionSem);

    if (pScsiCtrl->eventQ != 0)
	(void) rngDelete (pScsiCtrl->eventQ);

    if (pScsiCtrl->timeoutQ != 0)
	(void) rngDelete (pScsiCtrl->timeoutQ);

    if (pScsiCtrl->requestQ != 0)
	(void) rngDelete (pScsiCtrl->requestQ);

    return (ERROR);
    }

/*******************************************************************************
*
* scsiTargetInit - initialise a SCSI target structure
*
* Set all per-target configurable parameters to their default values.
* Initialise all the target's state variables.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void scsiTargetInit
    (
    FAST SCSI_CTRL *pScsiCtrl,		/* ptr to SCSI controller info   */
    UINT busId				/* SCSI bus ID of target to init */
    )
    {
    FAST SCSI_TARGET *pScsiTarget = &pScsiCtrl->targetArr [busId];
    
    /* initialise options (also see "scsiTargetOptionsSet()") */

    pScsiTarget->selTimeOut = SCSI_DEF_SELECT_TIMEOUT;
    pScsiTarget->messages   = TRUE;
    pScsiTarget->disconnect = TRUE;
    pScsiTarget->xferOffset = SCSI_SYNC_XFER_ASYNC_OFFSET;
    pScsiTarget->xferPeriod = SCSI_SYNC_XFER_ASYNC_PERIOD;
    pScsiTarget->maxOffset  = SCSI_SYNC_XFER_MAX_OFFSET;
    pScsiTarget->minPeriod  = SCSI_SYNC_XFER_MIN_PERIOD;
    pScsiTarget->tagType    = SCSI_DEF_TAG_TYPE;
    pScsiTarget->maxTags    = SCSI_MAX_TAGS;
    if (pScsiCtrl->wideXfer)
        pScsiTarget->xferWidth  = SCSI_WIDE_XFER_SIZE_DEFAULT;
    else 
        pScsiTarget->xferWidth  = SCSI_WIDE_XFER_SIZE_NARROW;

    /* initialize wide / sync support for each target to be FALSE */

    pScsiTarget->wideSupport   = FALSE;
    pScsiTarget->syncSupport   = FALSE;

    scsiTargetReset (pScsiCtrl, busId);
    }

/*******************************************************************************
*
* scsiTargetReset - reset the state of a SCSI target
*
* Reset the target's state variables, but do not change its configurable
* parameters.  Typically called when a target is initialised, and when a
* SCSI bus reset occurs.
*
* RETURNS: N/A
*
* NOMANUAL
*/

VOID scsiTargetReset
    (
    FAST SCSI_CTRL *pScsiCtrl,		/* ptr to SCSI controller info    */
    UINT busId				/* SCSI bus ID of target to reset */
    )
    {
    FAST SCSI_TARGET *pScsiTarget = &pScsiCtrl->targetArr [busId];
    
    /* intialise synchronous transfer fsm */

    scsiSyncXferNegotiate (pScsiCtrl, pScsiTarget, SYNC_XFER_RESET);

    /* similarly, intialise the wide data transfer fsm */

    scsiWideXferNegotiate (pScsiCtrl, pScsiTarget, WIDE_XFER_RESET);
    }

/*******************************************************************************
*
* scsiTargetOptionsSet - set options for one or all SCSI targets
*
* This routine sets the options defined by the bitmask `which' for the
* specified target (or all targets if `devBusId' is SCSI_SET_OPT_ALL_TARGETS).
*
* The bitmask `which' can be any combination of the following, bitwise
* OR'd together (corresponding fields in the SCSI_OPTIONS structure are
* shown in parentheses):
*
* .TS
* tab(|);
* l l l.
* SCSI_SET_OPT_TIMEOUT    | 'selTimeOut' | select timeout period, microseconds
* 
* SCSI_SET_OPT_MESSAGES   | 'messages'   | FALSE to disable SCSI messages
*
* SCSI_SET_OPT_DISCONNECT | 'disconnect' | FALSE to disable discon/recon
*
* SCSI_SET_OPT_XFER_PARAMS| 'maxOffset,' | max sync xfer offset, 0=>async
*                         |  'minPeriod' | min sync xfer period, x 4 nsec.
*
* SCSI_SET_OPT_TAG_PARAMS | 'tagType,'   | default tag type (SCSI_TAG_*)
*                         |  'maxTags'   | max cmd tags available
*
* SCSI_SET_OPT_WIDE_PARAMS| 'xferWidth'  | data transfer width setting.
*					   xferWidth = 0 ;  8 bits wide 
*					   xferWidth = 1 ; 16 bits wide
*
* .TE
*
* NOTE
* This routine can be used after the target device has already been used; 
* in this case, however, it is not possible to change the tag parameters. 
* This routine must not be used while there is any SCSI activity on the 
* specified target(s).
*
* RETURNS: OK, or ERROR if the bus ID or options are invalid.
*/

STATUS scsiTargetOptionsSet
    (
    SCSI_CTRL    *pScsiCtrl,		/* ptr to SCSI controller info   */
    int           devBusId,		/* target to affect, or all      */
    SCSI_OPTIONS *pOptions,		/* buffer containing new options */
    UINT          which			/* which options to change       */
    )
    {
    int i;

    /* verify bus ID of target, and validity of "which" bitmask */
    
    if (((devBusId < SCSI_MIN_BUS_ID) || (devBusId > SCSI_MAX_BUS_ID)) &&
	(devBusId != SCSI_SET_OPT_ALL_TARGETS))
	{
	errnoSet (S_scsiLib_ILLEGAL_BUS_ID);
	return (ERROR);
	}

    if ((which & SCSI_SET_OPT_BITMASK) != which)
	{
	errnoSet (S_scsiLib_ILLEGAL_PARAMETER);
	return (ERROR);
	}

    /* set options for one or all targets (loop is easiest approach) */
    
    for (i = 0; i < SCSI_MAX_TARGETS; ++i)
	{
	if ((devBusId == i) || (devBusId == SCSI_SET_OPT_ALL_TARGETS))
	    {
	    SCSI_TARGET *pScsiTarget = &pScsiCtrl->targetArr [i];

	    /* support for variable select timeout period (us) */
	    
	    if (which & SCSI_SET_OPT_TIMEOUT)
		pScsiTarget->selTimeOut = pOptions->selTimeOut;

	    /* support for messages (other than COMMAND COMPLETE) */
	    
	    if (which & SCSI_SET_OPT_MESSAGES)
		pScsiTarget->messages = pOptions->messages;

	    /* if messages are off, force options to SCSI-1 defaults */

            if (!pScsiTarget->messages)
		{
		pScsiTarget->disconnect = FALSE; 
		pScsiTarget->maxOffset = 0;
		pScsiTarget->xferWidth = SCSI_WIDE_XFER_SIZE_NARROW; 
		}

	    /* support for disconnect / reconnect (requires messages) */
	    
	    if (which & SCSI_SET_OPT_DISCONNECT)
		{
		if (pOptions->disconnect &&
		    (!pScsiCtrl->disconnect || !pScsiTarget->messages))
		    {
		    errnoSet (S_scsiLib_ILLEGAL_PARAMETER);
		    return (ERROR);
		    }

		pScsiTarget->disconnect = pOptions->disconnect;
		}
            
	    /* support for synchronous data transfer (requires messages) */
	    
	    if (which & SCSI_SET_OPT_XFER_PARAMS)
		{
		if ((pOptions->maxOffset != SCSI_SYNC_XFER_ASYNC_OFFSET) &&
		    (!pScsiCtrl->syncXfer || !pScsiTarget->messages)) 
		    {
		    errnoSet (S_scsiLib_ILLEGAL_PARAMETER);
		    return (ERROR);
		    }

                if (pOptions->minPeriod < 1)
		    {
		    errnoSet (S_scsiLib_ILLEGAL_PARAMETER);
		    return (ERROR);
		    }
		
		pScsiTarget->maxOffset = pOptions->maxOffset;
		pScsiTarget->minPeriod = pOptions->minPeriod;

    	    	/* re-intialise synchronous transfer fsm */

    	    	scsiSyncXferNegotiate (pScsiCtrl, pScsiTarget, SYNC_XFER_RESET);
		}

	    /* support for tagged commands (requires messages) */
	    
	    if (which & SCSI_SET_OPT_TAG_PARAMS)
		{
		BOOL valid;
		
		switch (pOptions->tagType)
		    {
		    case SCSI_TAG_UNTAGGED:
		    	valid = (pOptions->maxTags == 0);
			break;

		    case SCSI_TAG_SIMPLE:
		    case SCSI_TAG_ORDERED:
		    case SCSI_TAG_HEAD_OF_Q:
			valid = pScsiTarget->messages    &&
			    	(pOptions->maxTags >  0) &&
			        (pOptions->maxTags <= SCSI_MAX_TAGS);
			break;
			
		    default:
		    	valid = FALSE;
			break;
		    }

		if (!valid)
		    {
		    errnoSet (S_scsiLib_ILLEGAL_PARAMETER);
		    return (ERROR);
		    }

    	    	pScsiTarget->tagType = pOptions->tagType;
		pScsiTarget->maxTags = pOptions->maxTags;
		}

            /* support for wide data transfer (requires messages) */

            if (which & SCSI_SET_OPT_WIDE_PARAMS)
	        {
		if (pOptions->xferWidth != SCSI_WIDE_XFER_SIZE_NARROW &&
		    pOptions->xferWidth != SCSI_WIDE_XFER_SIZE_DEFAULT) 
		    {
		    errnoSet (S_scsiLib_ILLEGAL_PARAMETER);
		    return (ERROR);
		    }

		if (pScsiTarget->messages)
		    {
                    pScsiTarget->xferWidth = pOptions->xferWidth;

                    /* re-initialise wide data xfer fsm */

    	    	    scsiWideXferNegotiate (pScsiCtrl, pScsiTarget, 
					   WIDE_XFER_RESET);
                    }
		else  
		    {

		    /* if messages off, only accept NARROW */

		    if (pOptions->xferWidth != SCSI_WIDE_XFER_SIZE_NARROW)
		        {
		        errnoSet (S_scsiLib_ILLEGAL_PARAMETER);
		        return (ERROR);
		        }
                    }
		}
	    }
	}
    
    return (OK);
    }

/*******************************************************************************
*
* scsiTargetOptionsGet - get options for one or all SCSI targets
*
* This routine copies the current options for the specified target into the
* caller's buffer.
*
* RETURNS: OK, or ERROR if the bus ID is invalid.
*/

STATUS scsiTargetOptionsGet
    (
    SCSI_CTRL    *pScsiCtrl,		/* ptr to SCSI controller info */
    int           devBusId,		/* target to interrogate       */
    SCSI_OPTIONS *pOptions		/* buffer to return options    */
    )
    {
    SCSI_TARGET *pScsiTarget;

    if ((devBusId < SCSI_MIN_BUS_ID) || (devBusId > SCSI_MAX_BUS_ID))
	{
	errnoSet (S_scsiLib_ILLEGAL_BUS_ID);
	return (ERROR);
	}

    pScsiTarget = &pScsiCtrl->targetArr [devBusId];

    pOptions->selTimeOut = pScsiTarget->selTimeOut;
    pOptions->messages   = pScsiTarget->messages;
    pOptions->disconnect = pScsiTarget->disconnect;
    pOptions->maxOffset  = pScsiTarget->xferOffset;
    pOptions->minPeriod  = pScsiTarget->xferPeriod;
    pOptions->tagType    = pScsiTarget->tagType;
    pOptions->maxTags    = pScsiTarget->maxTags;
    pOptions->xferWidth  =  pScsiTarget->xferWidth;

    return (OK);
    }

/*******************************************************************************
*
* scsiTargetOptionsShow - display options for specified SCSI target
*
* This routine displays the current target options for the specified target in
* the following format:
* 
* Target Options (id <scsi bus ID>): 
*     selection TimeOut: <timeout> nano secs
*      messages allowed: TRUE or FALSE
*    disconnect allowed: TRUE or FALSE
*        REQ/ACK offset: <negotiated offset> 
*       transfer period: <negotiated period> 
*        transfer width: 8 or 16 bits
* maximum transfer rate: <peak transfer rate> MB/sec
*              tag type: <tag type>
*          maximum tags: <max tags>
*
* RETURNS: OK, or ERROR if the bus ID is invalid.
*/

STATUS scsiTargetOptionsShow
    (
    SCSI_CTRL    *pScsiCtrl,		/* ptr to SCSI controller info */
    int           devBusId		/* target to interrogate       */
    )
    {
    SCSI_OPTIONS options;		/* buffer for returned options    */

    if ((devBusId < SCSI_MIN_BUS_ID) || (devBusId > SCSI_MAX_BUS_ID))
	{
	errnoSet (S_scsiLib_ILLEGAL_BUS_ID);
	return (ERROR);
	}

    if (scsiTargetOptionsGet (pScsiCtrl, devBusId, &options) == ERROR)
        {
        return (ERROR);
        }
    printf("Target Options (id %d):\n", devBusId);
    printf("     selection TimeOut: %d nano secs\n", options.selTimeOut);
    printf("      messages allowed: %s\n", 
                       (options.messages ? "TRUE" : "FALSE"));
    printf("    disconnect allowed: %s\n",
                       (options.disconnect ? "TRUE" : "FALSE"));
    printf("        REQ/ACK offset: %d\n", options.maxOffset);
    printf("       transfer period: %d\n", (options.minPeriod * 4));
    printf("        transfer width: %s bits\n", 
                       (options.xferWidth ? "16" : "8"));
    printf(" maximum transfer rate: %d MB/sec\n", 
              (options.minPeriod == 0 ? 0 : (options.xferWidth ? 
              (250 * 2 / options.minPeriod) : (250 / options.minPeriod))));
    printf("              tag type: 0x%x\n", options.tagType);
    printf("          maximum tags: %d\n", options.maxTags);

    return (OK);
    }

/*******************************************************************************
*
* scsi2PhysDevDelete - delete a SCSI physical device structure
*
* This routine deletes a specified SCSI physical device structure.
*
* RETURNS: OK, or ERROR if `pScsiPhysDev' is NULL or SCSI_BLK_DEVs have
* been created on the device.
*/

LOCAL STATUS scsi2PhysDevDelete
    (
    FAST SCSI_PHYS_DEV *pScsiPhysDev    /* ptr to SCSI physical device info */
    )
    {
    FAST SCSI_CTRL *pScsiCtrl;

    if ((pScsiPhysDev == (SCSI_PHYS_DEV *) NULL)        ||
	(lstCount (&pScsiPhysDev->blkDevList)     != 0) ||
	(lstCount (&pScsiPhysDev->activeThreads)  != 0) ||
	(lstCount (&pScsiPhysDev->waitingThreads) != 0))
	return (ERROR);

    pScsiCtrl = pScsiPhysDev->pScsiCtrl;
    pScsiCtrl->physDevArr [(pScsiPhysDev->pScsiTarget->scsiDevBusId << 3) |
			   pScsiPhysDev->scsiDevLUN] = (SCSI_PHYS_DEV *) NULL;

    if (pScsiPhysDev->pReqSenseData != NULL)
        (void) free ((char *) pScsiPhysDev->pReqSenseData);

    if (pScsiPhysDev->pTagInfo != 0)
	(void) free ((char *) pScsiPhysDev->pTagInfo);

    if (pScsiPhysDev->mutexSem != 0)
	(void) semDelete (pScsiPhysDev->mutexSem);

    (void) free ((char *) pScsiPhysDev);

    return (OK);
    }

/*******************************************************************************
*
* scsi2PhysDevCreate - create a SCSI physical device structure
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
* An INQUIRY command is issued to determine information about the device
* including its type, make and model number, and its ability to accept
* SCSI-2 features such as tagged commands.  The scsiShow() routine displays
* this information.
*
* If the type device is a Direct Access, a Write Once Read Multiple, or a
* Opticial Memory Device then a READ_CAPACITY command is issued to determine 
* the values of `numBlocks' and `blockSize'.  For these types of devices, 
* values passed into `numBlocks' and `blockSize' are ignored.
*
* For Sequential Access devices the values specified in  `numBlocks' and 
* `blockSize' are entered into the SCSI_PHYS_DEV structure.
*
* RETURNS: A pointer to the created SCSI_PHYS_DEV structure, or NULL if the
* routine is unable to create the physical device structure.
*
* NOTE: the `devType' and `removable' arguments are ignored.
*/

LOCAL SCSI_PHYS_DEV *scsi2PhysDevCreate
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
    SCSI_TARGET   *pScsiTarget;	    	/* ptr to SCSI target structure */
					/* REQ SENSE data for auto-sizing */
    UINT8 reqSenseData [REQ_SENSE_ADD_LENGTH_BYTE + 1];
    UINT8 inquiryData  [DEFAULT_INQUIRY_DATA_LENGTH];

    /* check bus ID and LUN are within limits */

    if ((devBusId < SCSI_MIN_BUS_ID) ||
	(devBusId > SCSI_MAX_BUS_ID) ||
	(devLUN < SCSI_MIN_LUN) ||
	(devLUN > SCSI_MAX_LUN))
	{
	errnoSet (S_scsiLib_ILLEGAL_PARAMETER);
	return ((SCSI_PHYS_DEV *) NULL);
	}

    /* Check if this device was already created */

    if (scsiPhysDevIdGet (pScsiCtrl, devBusId, devLUN) != NULL)
        {
        errnoSet (S_scsiLib_DEVICE_EXIST);
        return ((SCSI_PHYS_DEV *) NULL);
	}

    /* create a SCSI physical device structure */

    pScsiPhysDev = (SCSI_PHYS_DEV *) calloc (1, sizeof (*pScsiPhysDev));

    if (pScsiPhysDev == NULL)
        return ((SCSI_PHYS_DEV *) NULL);

    /* create device mutual exclusion semaphore */

    if ((pScsiPhysDev->mutexSem = semMCreate (scsiPhysDevMutexOptions)) == NULL)
        {
        SCSI_DEBUG_MSG ("scsiPhysDevCreate: semMCreate of mutexSem failed.\n",
			0, 0, 0, 0, 0, 0);
	goto failed;
        }

    /* initialize miscellaneous fields in the SCSI_PHYS_DEV struct */

    pScsiTarget = &pScsiCtrl->targetArr [devBusId];
    
    pScsiPhysDev->pScsiCtrl   = pScsiCtrl;
    pScsiPhysDev->pScsiTarget = pScsiTarget;
    pScsiPhysDev->scsiDevLUN  = devLUN;
    
    pScsiCtrl->physDevArr [(devBusId << 3) | devLUN] = pScsiPhysDev;

    /* initialize block device list */

    lstInit (&pScsiPhysDev->blkDevList);

    /* initialize sequential dev ptr */

    pScsiPhysDev->pScsiSeqDev = NULL;

    /* initialize lists of active and waiting threads */

    lstInit (&pScsiPhysDev->activeThreads);
    lstInit (&pScsiPhysDev->waitingThreads);

    /* initialize state variables */

    pScsiPhysDev->connected = FALSE;
    pScsiPhysDev->pendingCA = FALSE;

    pScsiPhysDev->curTag       = SCSI_TAG_NONE;
    pScsiPhysDev->nexus        = SCSI_NEXUS_NONE;
    pScsiPhysDev->savedNexus   = SCSI_NEXUS_NONE;
    pScsiPhysDev->nTaggedNexus = 0;
    
    /* initialise tags: do not use tags until after the INQUIRY command ! */

    pScsiPhysDev->tagType   = SCSI_TAG_UNTAGGED;
    pScsiPhysDev->nTags     = 0;
    pScsiPhysDev->pTagInfo  = 0;

    scsiMgrPhysDevTagInit (pScsiPhysDev);

    /*
     *	Issue a Request Sense command to establish length of sense data,
     *	if not specified by caller.
     */
    if (reqSenseLength != 0)
	{
	pScsiPhysDev->reqSenseDataLength = reqSenseLength;

	if (reqSenseLength <= NON_EXT_SENSE_DATA_LENGTH)
	    pScsiPhysDev->extendedSense = FALSE;
	else
	    pScsiPhysDev->extendedSense = TRUE;
	}
    else
	{
        /* determine if device uses Extended Sense Data Format */

        if (scsiReqSense (pScsiPhysDev, (char *) reqSenseData,
			  REQ_SENSE_ADD_LENGTH_BYTE + 1) == ERROR)
	    {
	    SCSI_DEBUG_MSG ("scsiPhysDevCreate: REQUEST SENSE failed.\n",
			    0, 0, 0, 0, 0, 0);
	    goto failed;
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

	    goto failed;
	    }
        else
	    {
	    /* device uses Extended Sense Data Format */

	    SCSI_DEBUG_MSG ("scsiPhysDevCreate: reqSenseData[7] = %x\n",
			    reqSenseData[REQ_SENSE_ADD_LENGTH_BYTE],
			    0, 0, 0, 0, 0);

	    pScsiPhysDev->extendedSense      = TRUE;
	    pScsiPhysDev->reqSenseDataLength = REQ_SENSE_ADD_LENGTH_BYTE +
	        (int) reqSenseData [REQ_SENSE_ADD_LENGTH_BYTE] + 1;
	    }
	}

    if ((pScsiPhysDev->pReqSenseData =
	(UINT8 *) calloc (pScsiPhysDev->reqSenseDataLength,
			  sizeof (UINT8))) == NULL)
	{
	goto failed;
	}

    /*
     *	Issue an INQUIRY command: retry while a reset condition persists
     */
    bzero ((char *) inquiryData, sizeof (inquiryData));

    while (scsiInquiry (pScsiPhysDev, (char *) inquiryData,
			               sizeof  (inquiryData)) != OK)
	{
    	if (!pScsiPhysDev->resetFlag)
	    goto failed;

	pScsiPhysDev->resetFlag = FALSE;
	SCSI_DEBUG_MSG ("retrying scsiInquiry...\n", 0, 0, 0, 0, 0, 0);
	}

    /*
     *	The INQUIRY command was successful: is there a device on this LUN ?
     */
    if (inquiryData[SCSI_INQUIRY_DEV_TYPE] == SCSI_LUN_NOT_PRESENT)
	{
	SCSI_DEBUG_MSG ("scsiPhysDevCreate: LUN not present.\n",
			0, 0, 0, 0, 0, 0);
	errnoSet (S_scsiLib_LUN_NOT_PRESENT);

	goto failed;
	}

    /*
     *	There is a supported device on this LUN
     */
    {
    int  devType      = inquiryData[SCSI_INQUIRY_DEV_TYPE] &
	    	    	    	    SCSI_INQUIRY_DEV_TYPE_MASK;
    
    BOOL removable    = inquiryData[SCSI_INQUIRY_DEV_MODIFIER] &
	                            SCSI_INQUIRY_REMOVABLE_MASK;
    
    int  ansiVersion  = inquiryData[SCSI_INQUIRY_VERSION] &
	                            SCSI_INQUIRY_ANSI_VSN_MASK;
    
    BOOL supportsTags = inquiryData[SCSI_INQUIRY_FLAGS] &
	                            SCSI_INQUIRY_CMD_QUEUE_MASK;

    BOOL syncSupport  = inquiryData[SCSI_INQUIRY_FLAGS] & 0x10;

    BOOL wideSupport  = (inquiryData[SCSI_INQUIRY_FLAGS] & 
                         SCSI_INQUIRY_WIDE_16_MASK) |
                        (inquiryData[SCSI_INQUIRY_FLAGS] & 
                         SCSI_INQUIRY_WIDE_32_MASK);

    /* 
     * set the appropriate fields to indicate if the target
     * supports wide / synchronous transfers.
     */

    pScsiTarget->wideSupport   = wideSupport;
    pScsiTarget->syncSupport   = syncSupport;
    
    pScsiPhysDev->scsiDevType = devType;
    
    pScsiPhysDev->removable   = removable;

    pScsiPhysDev->tagType = (ansiVersion >= 2) && supportsTags
	    	    	  ? pScsiTarget->tagType
			  : SCSI_TAG_UNTAGGED;

    pScsiPhysDev->nTags   = (pScsiPhysDev->tagType != SCSI_TAG_UNTAGGED)
	    	    	  ? pScsiTarget->maxTags
			  : 0;

    if (pScsiPhysDev->nTags != 0)
	{
	/*
	 *  Allocate tag info. table; re-initialise tag system
	 */
	if ((pScsiPhysDev->pTagInfo = malloc (pScsiPhysDev->nTags *
					      sizeof (SCSI_TAG_INFO))) == 0)
	    {
	    SCSI_DEBUG_MSG ("scsiPhysDevCreate: can't allocate tag info.\n",
			    0, 0, 0, 0, 0, 0);

	    goto failed;
	    }

	scsiMgrPhysDevTagInit (pScsiPhysDev);
	}
    
    /*
     *  Save product info. strings in physical device structure
     */
    bcopy ((char *) &inquiryData[SCSI_INQUIRY_VENDOR_ID],
	   pScsiPhysDev->devVendorID,
	   SCSI_INQUIRY_VENDOR_ID_LENGTH);
    
    bcopy ((char *) &inquiryData[SCSI_INQUIRY_PRODUCT_ID],
	   pScsiPhysDev->devProductID,
	   SCSI_INQUIRY_PRODUCT_ID_LENGTH);
    
    bcopy ((char *) &inquiryData[SCSI_INQUIRY_REV_LEVEL],
	   pScsiPhysDev->devRevLevel,
	   SCSI_INQUIRY_REV_LEVEL_LENGTH);
    
    pScsiPhysDev->devVendorID  [SCSI_INQUIRY_VENDOR_ID_LENGTH]  = EOS;
    pScsiPhysDev->devProductID [SCSI_INQUIRY_PRODUCT_ID_LENGTH] = EOS;
    pScsiPhysDev->devRevLevel  [SCSI_INQUIRY_REV_LEVEL_LENGTH]  = EOS;
    }

    /* record numBlocks and blockSize in physical device */

    if ((pScsiPhysDev->scsiDevType == SCSI_DEV_DIR_ACCESS) ||
    	 (pScsiPhysDev->scsiDevType == SCSI_DEV_WORM) ||
    	 (pScsiPhysDev->scsiDevType == SCSI_DEV_RO_DIR_ACCESS)) 
        {     
	int lastLogBlkAdrs;
	int blkLength;

        /*
	 * Issue a READ_CAPACITY command: retry while reset condition occurs
	 */

	while (scsiReadCapacity (pScsiPhysDev, &lastLogBlkAdrs, &blkLength) 
	       != OK)
    	    {
            /* Test for removable & additional sense code MEDIUM NOT PRESENT */ 

            if (pScsiPhysDev->removable && 
                (pScsiPhysDev->lastAddSenseCode == SCSI_ADD_SENSE_NO_MEDIUM))
                {
	        pScsiPhysDev->numBlocks = 0;
	        pScsiPhysDev->blockSize = 0;
                break;
                }
            else
                {      
	        /* if command failed, then the device must have been reset */

	        if (!pScsiPhysDev->resetFlag)
	            goto failed;
	    
	        pScsiPhysDev->resetFlag = FALSE;
	        SCSI_DEBUG_MSG ("retrying scsiReadCapacity...\n",
	           		        0, 0, 0, 0, 0, 0);
                }
	    }

	    pScsiPhysDev->numBlocks = lastLogBlkAdrs + 1;
	    pScsiPhysDev->blockSize = blkLength;
        }

    else if (pScsiPhysDev->scsiDevType == SCSI_DEV_SEQ_ACCESS)
        {
	int    pMaxBlockLength;     /* where to return maximum block length */
	UINT16 pMinBlockLength;


        while (scsiReadBlockLimits (pScsiPhysDev, &pMaxBlockLength, 
					  	     &pMinBlockLength) != OK)
	    {
	    /* if the command failed, then the device must have been reset */

	    if (!pScsiPhysDev->resetFlag)
	        goto failed;
            pScsiPhysDev->resetFlag = FALSE;
	    SCSI_DEBUG_MSG ("retrying scsiReadBlockLimits...\n", 
							0, 0, 0, 0, 0, 0);
	    }

        pScsiPhysDev->maxVarBlockLimit = pMaxBlockLength;

        /* for sequential access devices, these fields are not used */

	pScsiPhysDev->numBlocks = numBlocks;
	pScsiPhysDev->blockSize = blockSize;
        }

    return (pScsiPhysDev);

failed:
    if (pScsiPhysDev->mutexSem != 0)
	(void) semDelete (pScsiPhysDev->mutexSem);

    if (pScsiPhysDev->pTagInfo != 0)
	(void) free ((char *) pScsiPhysDev->pTagInfo);

    if (pScsiPhysDev->pReqSenseData != 0)
	(void) free ((char *) pScsiPhysDev->pReqSenseData);

    (void) free ((char *) pScsiPhysDev);

    pScsiCtrl->physDevArr [(devBusId << 3) | devLUN] = NULL;

    return (NULL);
    }

/*******************************************************************************
*
* scsi2PhysDevIdGet - return a pointer to a SCSI_PHYS_DEV structure
*
* This routine returns a pointer to the SCSI_PHYS_DEV structure of the SCSI
* physical device located at a specified bus ID (`devBusId') and logical
* unit number (`devLUN') and attached to a specified SCSI controller
* (`pScsiCtrl').
*
* RETURNS: A pointer to the specified SCSI_PHYS_DEV structure, or NULL if the
* structure does not exist.
*/

LOCAL SCSI_PHYS_DEV * scsi2PhysDevIdGet
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
* scsi2AutoConfig - configure all devices connected to a SCSI controller
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

LOCAL STATUS scsi2AutoConfig
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
* scsi2Show - list the physical devices attached to a SCSI controller
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

LOCAL STATUS scsi2Show
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

    for (ix = 0; ix < SCSI_MAX_PHYS_DEVS; ix++)
	if ((pScsiPhysDev = pScsiCtrl->physDevArr[ix]) !=
	    (SCSI_PHYS_DEV *) NULL)
	    {
	    printf ("%2d " , pScsiPhysDev->pScsiTarget->scsiDevBusId);
	    printf ("%2d " , pScsiPhysDev->scsiDevLUN);

	    printf (" %8s" , strIsPrintable (pScsiPhysDev->devVendorID) ?
			     pScsiPhysDev->devVendorID : "        ");
	    printf (" %16s", strIsPrintable (pScsiPhysDev->devProductID) ?
			     pScsiPhysDev->devProductID : "                ");
	    printf (" %4s ", strIsPrintable (pScsiPhysDev->devRevLevel) ?
			     pScsiPhysDev->devRevLevel : "    ");

	    printf ("%3d"  , pScsiPhysDev->scsiDevType);
	    printf (pScsiPhysDev->removable ? "R" : " ");
	    printf (" %8d " , pScsiPhysDev->numBlocks);
	    printf (" %5d " , pScsiPhysDev->blockSize);
	    printf ("   0x%08x ", (int) pScsiPhysDev);
	    printf ("\n");
	    }

    return (OK);
    }


/*******************************************************************************
*
* scsiThreadShow - show information for a SCSI thread
*
* Display the role, priority, tag type (and number, if appropriate) and
* state of a SCSI thread.  May be called at any time, but note that the
* state is a "snapshot".
*
* RETURNS: N/A
*
* NOMANUAL
*/
void scsiThreadShow
    (
    SCSI_THREAD * pThread,		/* thread to be displayed  */
    BOOL          noHeader		/* do not print title line */
    )
    {
    char * role;
    char * tagType;
    char * state;
    
    switch (pThread->role)
	{
	case SCSI_ROLE_INITIATOR:  role = "init"; break;
	case SCSI_ROLE_TARGET:     role = "targ"; break;
	case SCSI_ROLE_IDENT_INIT: role = "id-i"; break;
	case SCSI_ROLE_IDENT_TARG: role = "id-t"; break;
	default:                   role = "????"; break;
	}

    switch (pThread->tagType)
	{
	case SCSI_TAG_DEFAULT:    tagType = "default";  break;
	case SCSI_TAG_UNTAGGED:       tagType = "untagged"; break;
	case SCSI_TAG_SENSE_RECOVERY: tagType = "recovery"; break;
	case SCSI_TAG_SIMPLE:         tagType = "simple";   break;
	case SCSI_TAG_ORDERED:        tagType = "ordered";  break;
	case SCSI_TAG_HEAD_OF_Q:      tagType = "queue hd"; break;
	default:                      tagType = "????????"; break;
	}

    switch (pThread->state)
	{
    	case SCSI_THREAD_INACTIVE:     state = "inact"; break;
	case SCSI_THREAD_WAITING:      state = "wait";  break;
	case SCSI_THREAD_DISCONNECTED: state = "disc."; break;
	default:                       state = "conn."; break;
	}
    
    if (!noHeader)
	{
    	printf ("Thread  ID  PhysDev ID  Role  Pri  Tag Type  (#)   State\n");
    	printf ("----------  ----------  ----  ---  --------------  -----\n");
	}

    printf ("0x%08x  0x%08x  %4s  %3d  %-8s (%3d)  %5s\n",
	    (int) pThread,
	    (int) pThread->pScsiPhysDev,
	    role,
	    pThread->priority,
	    tagType,
	    pThread->tagNumber,
	    state);
    }

/*******************************************************************************
*
* scsiPhysDevShow - show status information for a physical device
*
* This routine shows the state, the current nexus type, the current tag
* number, the number of tagged commands in progress, and the number of
* waiting and active threads for a SCSI physical device.  Optionally, it shows
* the IDs of waiting and active threads, if any.  This routine may be called
* at any time, but note that all of the information displayed is volatile.
*
* RETURNS: N/A
*/
void scsiPhysDevShow
    (
    SCSI_PHYS_DEV * pScsiPhysDev,	/* physical device to be displayed */
    BOOL            showThreads,    	/* show IDs of associated threads  */
    BOOL            noHeader		/* do not print title line         */
    )
    {
    char * nexus;
    char * state;
    int    nWaiting = lstCount (&pScsiPhysDev->waitingThreads);
    int    nActive  = lstCount (&pScsiPhysDev->activeThreads);

    switch (pScsiPhysDev->nexus)
	{
	case SCSI_NEXUS_NONE: nexus = "none"; break;
	case SCSI_NEXUS_IT:   nexus = "IT"  ; break;
	case SCSI_NEXUS_ITL:  nexus = "ITL" ; break;
	case SCSI_NEXUS_ITLQ: nexus = "ITLQ"; break;
	default:              nexus = "????"; break;
        }

    state = pScsiPhysDev->connected ? "conn." : "disc.";
    
    if (!noHeader)
	{
    	printf ("PhysDev ID  State  Nexus  Tag  # Tags  # Threads (w/a)\n");
    	printf ("----------  -----  -----  ---  ------  ---------------\n");
	}

    printf ("0x%08x  %-5s  %-4s   %3d   %3d       %3d   %3d\n",
	    (int) pScsiPhysDev,
	    state,
	    nexus,
	    pScsiPhysDev->curTag,
	    pScsiPhysDev->nTaggedNexus,
	    nWaiting,
	    nActive);

    if (showThreads && (nWaiting != 0))
	{
	printf ("\nWaiting threads:\n");

	scsiThreadListIdShow (&pScsiPhysDev->waitingThreads);
	}

    if (showThreads && (nActive != 0))
	{
	printf ("\nActive threads:\n");

	scsiThreadListIdShow (&pScsiPhysDev->activeThreads);
	}
    }

/*******************************************************************************
*
* scsiThreadListIdShow - show IDs of all threads in a list
*
* Step through a list of threads, printing IDs of all threads in the list.
* A maximum of 6 IDs are printed per line of output.
*
* NOTE: this routine is naive about the list being updated while it is
* traversed, so is best used when there is no SCSI activity.
*
* RETURNS: N/A
*
* NOMANUAL
*/
VOID scsiThreadListIdShow
    (
    LIST * pList
    )
    {
    NODE * pThread;
    int n;
	
    n = 0;
	
    for (pThread = lstFirst (pList); pThread != 0; pThread = lstNext (pThread))
	{
	printf ("0x%08x  ", (int) pThread);

	if ((++n % 6) == 0)
	    {
	    printf ("\n");

	    n = 0;
	    }
	}

    if (n != 0)
	printf ("\n");
    }

/*******************************************************************************
*
* scsiThreadListShow - show info for all threads in a list
*
* Step through a list of threads, showing details of all threads in the list.
*
* NOTE: this routine is naive about the list being updated while it is
* traversed, so is best used when there is no SCSI activity.
*
* RETURNS: N/A
*
* NOMANUAL
*/
void scsiThreadListShow
    (
    LIST * pList
    )
    {
    NODE * pThread;
    BOOL   noHeader;

    noHeader = FALSE;

    for (pThread = lstFirst (pList); pThread != 0; pThread = lstNext (pThread))
	{
	scsiThreadShow ((SCSI_THREAD *) pThread, noHeader);
	
	noHeader = TRUE;
	}
    }

/*******************************************************************************
*
* scsi2BusReset - pulse the reset signal on the SCSI bus
*
* This routine calls a controller-specific routine to reset a specified
* controller's SCSI bus.  If no controller is specified (`pScsiCtrl' is 0),
* the value in `pSysScsiCtrl' is used.
*
* RETURNS: OK, or ERROR if there is no controller or controller-specific
* routine.
*
* INTERNAL
*
* The SCSI manager must be notified that a SCSI bus reset has occurred.  This
* is normally done by the ISR for the SCSI controller hardware, assuming it
* can detect a bus reset.  If this is not the case "scsiBusResetNotify ()"
* must be called explicitly, perhaps by a board-specific ISR.
*/

LOCAL STATUS scsi2BusReset
    (
    SCSI_CTRL *pScsiCtrl	/* ptr to SCSI controller info */
    )
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

    (pScsiCtrl->scsiBusControl) (pScsiCtrl, SCSI_BUS_RESET);

    return (OK);
    }

/*******************************************************************************
*
* scsiBusResetNotify - notify the SCSI library that a bus reset has occurred.
*
* This function should not normally be called by application code.  It may be
* called from interrupt service routines (typically in the controller- or
* BSP-specific code which handles a SCSI bus reset interrupt).
*
* INTERNAL
* SCSI controller interrupts must be disabled while the synthesised
* bus reset event is posted to the controller, to ensure that the posting
* operation (which is not inherently atomic) is not interrupted by the ISR.
*
* NOMANUAL
*/

void scsiBusResetNotify
    (
    SCSI_CTRL *pScsiCtrl		/* ctrlr for bus which was reset */
    )
    {
    SCSI_EVENT event;
    int        key;

    event.type = SCSI_EVENT_BUS_RESET;

    key = intLock ();
    
    scsiMgrEventNotify (pScsiCtrl, &event, sizeof (event));

    intUnlock (key);
    }

/*******************************************************************************
*
* scsi2PhaseNameGet - get the name of a specified SCSI phase
*
* This routine returns a pointer to a string which is the name of the SCSI
* phase input as an integer.  It's primarily used to improve readability of
* debugging messages.
*
* RETURNS: A pointer to a string naming the SCSI phase input
*/

LOCAL char *scsi2PhaseNameGet
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
* scsi2CmdBuild - fills in the fields of a SCSI command descriptor block
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

LOCAL STATUS scsi2CmdBuild
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



/*
 *   
 *     Controller-independent SCSI support routines 
 *  
 */

/*******************************************************************************
*
* scsiCacheSynchronize - synchronize the caches for data coherency
*
* This routine performs whatever cache action is necessary to ensure cache
* coherency with respect to the various buffers involved in a SCSI command.
*
* The process is as follows:
*
* .iP 1. 4
* The buffers for command, identification, and write data,
* which are simply written to SCSI, are flushed before the command.
* .iP 2.
* The status buffer, which is written and then read, is cleared
* (flushed and invalidated) before the command.
* .iP 3.
* The data buffer for a read command, which is only read, is
* cleared before the command.
* .LP
*
* The data buffer for a read command is cleared before the command rather
* than invalidated after it because it may share dirty cache lines with data
* outside the read buffer.  DMA drivers for older versions of the SCSI
* library have flushed the first and last bytes of the
* data buffer before the command.  However, this approach is not sufficient
* with the enhanced SCSI library because the amount of data transferred into
* the buffer may not fill it, which would cause dirty cache lines which
* contain correct data for the un-filled part of the buffer to be lost
* when the buffer is invalidated after the command.
*
* To optimize the performance of the driver in supporting different caching
* policies, the routine uses the CACHE_USER_FLUSH macro when flushing the
* cache.  In the absence of a CACHE_USER_CLEAR macro, the following steps are 
* taken:
* .iP 1. 5
* If there is a non-NULL flush routine in the `cacheUserFuncs' structure, 
* the cache is cleared.
* .iP 2.
* If there is a non-NULL invalidate routine, the cache is invalidated. 
* .iP 3.
* Otherwise nothing is done; the cache is assumed to be coherent
* without any software intervention.
* .LP
*
* Finally, since flushing (clearing) cache line entries for a large data
* buffer can be time-consuming, if the data buffer is larger
* than a preset (run-time configurable) size, the entire cache is flushed.
*
* RETURNS: N/A
*/

void scsiCacheSynchronize
    (
    SCSI_THREAD *     pThread,		/* ptr to thread info    */
    SCSI_CACHE_ACTION action		/* cache action required */
    )
    {
    int     direction = pThread->dataDirection;
    UINT8 * dataAddr  = pThread->dataAddress;
    UINT    dataSize  = pThread->dataLength;

    FUNCPTR clearRtn = (cacheUserFuncs.flushRtn != NULL)
	    	     	 ? cacheClear
	                 : cacheUserFuncs.invalidateRtn;

    /* 
     * If hardware snooping is enabled then don't do anything. By default
     * cacheSnooping is disabled, unless enabled by the BSP
     */

    if (pThread->pScsiCtrl->cacheSnooping == TRUE)
	return;

    /*
     * Invalidating the entire cache is incorrect. However, flushing the
     * entire cache is correct. It is safest to invalidate and flush
     * by cache lines, although there is a performance penalty to be 
     * paid.
     */
#if FALSE
    if (dataSize >= scsiCacheFlushThreshold)
	{
	dataSize = ENTIRE_CACHE;
	dataAddr = 0;
	}
#endif

    switch (action)
	{
    	case SCSI_CACHE_PRE_COMMAND:

#if (CPU == SPARC)
            /*
             *  The "cacheClear ()" function, called by CACHE_USER_FLUSH,
             *  occasionally crashes on microSPARC when asked to clear a
             *  small area of cache (try "repeat (0, cacheClear, 1, 0, 100)").
             *  It appears to work reliably when clearing the entire cache.
             *
             *  To avoid this, if there is a non-null flush routine or a non-
             *  null invalidation routine, just clear the whole cache.
             *  This is a superset of everything normally done.
             *
             *  If both flush and invalidation routines are null, there is
             *  nothing to do.
             */
            if ((cacheUserFuncs.flushRtn      != 0) ||
                (cacheUserFuncs.invalidateRtn != 0))
                {
                cacheClear (DATA_CACHE, 0, ENTIRE_CACHE);
                }
            break;
#endif

    	    /*
	     *	Flush command and identify buffers; clear status buffer.
	     */
	    CACHE_USER_FLUSH (pThread->identMsg,   pThread->identMsgLength);
    	    CACHE_USER_FLUSH (pThread->cmdAddress, pThread->cmdLength);

	    if (clearRtn != 0)
	    	(* clearRtn) (DATA_CACHE, pThread->statusAddress,
			                  pThread->statusLength);

	    /*
	     *	Flush (write) or clear (read) data buffer.
	     */
	    if (direction == O_WRONLY)
		CACHE_USER_FLUSH (dataAddr, dataSize);

	    else if (clearRtn != 0)
		{
#if (CPU == MC68040)
	    	/*
	    	 *  "cacheInvalidate (DATA_CACHE, 0, ENTIRE_CACHE)" crashes
	    	 *  on the 68040 (for some reason).  Use "cacheClear" instead
		 *  in this case.
	    	 */
		if (dataSize == ENTIRE_CACHE)
		    clearRtn = cacheClear;
#endif
		(* clearRtn) (DATA_CACHE, dataAddr, dataSize);
	    	}
	    break;
	    
    	case SCSI_CACHE_POST_COMMAND:
	    /*
	     *	No action is required.  For a read, we cleared the cache
	     *	before the command and therefore don't need to invalidate
	     *	it here.
	     */
	    break;
	    
    	default:
	    logMsg ("scsiCacheSynchronize: invalid action (%d).\n",
		    action, 0, 0, 0, 0, 0);
	    break;
	}
    }


/*******************************************************************************
*
* scsiIdentMsgBuild - build an identification message
*
* This routine builds an identification message in the caller's buffer,
* based on the specified physical device, tag type, and tag number.
*
* If the target device does not support messages, there is no identification
* message to build.
*
* Otherwise, the identification message consists of an IDENTIFY byte plus an
* optional QUEUE TAG message (two bytes), depending on the type of tag used.
*
* NOTE:
* This function is not intended for use by application programs.
*
* RETURNS: The length of the resulting identification message in bytes or -1 
* for ERROR.
*/
int scsiIdentMsgBuild
    (
    UINT8 *         msg,
    SCSI_PHYS_DEV * pScsiPhysDev,
    SCSI_TAG_TYPE   tagType,
    UINT            tagNumber
    )
    {
    SCSI_TARGET * pScsiTarget = pScsiPhysDev->pScsiTarget;
    
    int msgLen = 0;

    if (!pScsiTarget->messages)
	return (0);

    msg[0] = SCSI_MSG_IDENTIFY | (UINT8) pScsiPhysDev->scsiDevLUN;
	
    if (pScsiTarget->disconnect)
    	msg[0] |= SCSI_MSG_IDENT_DISCONNECT;

    switch (tagType)
    	{
    	case SCSI_TAG_UNTAGGED:
    	case SCSI_TAG_SENSE_RECOVERY:
    	    msgLen = 1;
    	    break;

	case SCSI_TAG_SIMPLE:
	    msg[1] = SCSI_MSG_SIMPLE_Q_TAG;
    	    msg[2] = tagNumber;
    	    msgLen = 3;
    	    break;

    	case SCSI_TAG_ORDERED:
    	    msg[1] = SCSI_MSG_ORDERED_Q_TAG;
    	    msg[2] = tagNumber;
    	    msgLen = 3;
    	    break;

    	case SCSI_TAG_HEAD_OF_Q:
    	    msg[1] = SCSI_MSG_HEAD_OF_Q_TAG;
    	    msg[2] = tagNumber;
    	    msgLen = 3;
    	    break;

	default:
	    logMsg ("scsiIdentMsgBuild: invalid tag type (%d)\n",
		    tagType, 0, 0, 0, 0, 0);
	    return (-1);
        }

    return (msgLen);
}


/*******************************************************************************
*
* scsiIdentMsgParse - parse an identification message
*
* This routine scans a (possibly incomplete) identification message,
* validating it in the process.  If there is an IDENTIFY message, it
* identifies the corresponding physical device.
*
* If the physical device is currently processing an untagged (ITL) nexus,
* identification is complete. Otherwise, the identification is complete only
* if there is a complete QUEUE TAG message.
*
* If there is no physical device corresponding to the IDENTIFY message, or
* if the device is processing tagged (ITLQ) nexuses and the tag does not
* correspond to an active thread (it may have been aborted by a timeout, for
* example), then the identification sequence fails.
*
* The caller's buffers for physical device and tag number (the results
* of the identification process) are always updated.  This is required by
* the thread event handler (see scsiMgrThreadEvent().)
*
* NOTE: This function is not intended for use by application programs.
*
* RETURNS: The identification status (incomplete, complete, or rejected).
*/
SCSI_IDENT_STATUS scsiIdentMsgParse
    (
    SCSI_CTRL      * pScsiCtrl,
    UINT8          * msg,
    int              msgLength,
    SCSI_PHYS_DEV ** ppScsiPhysDev,
    SCSI_TAG       * pTagNum
    )
    {
    SCSI_PHYS_DEV   * pScsiPhysDev = 0;	/* initialize to avoid warning */
    SCSI_IDENT_STATUS status;
    BOOL              tagged = (msgLength >= 1 + 2);	/* id (1) + tag (2) */

    /*
     *	If no IDENTIFY message has been received, always incomplete.
     */
    if (msgLength < 1)
	{
	status = SCSI_IDENT_INCOMPLETE;
	}

    /*
     *	First byte should be an IDENTIFY message: check it
     */
    else if ((msg[0] & ~SCSI_MSG_IDENT_LUN_MASK) != SCSI_MSG_IDENTIFY)
	{
	SCSI_ERROR_MSG ("scsiIdentMsgParse: invalid IDENTIFY message (0x%x)\n",
			msg[0], 0, 0, 0, 0, 0);
	
	status = SCSI_IDENT_FAILED;
	}
    
    /*
     *	Second byte, if present, should be a SIMPLE QUEUE TAG message: check it
     */
    else if (tagged && (msg[1] != SCSI_MSG_SIMPLE_Q_TAG))
	{
	SCSI_ERROR_MSG ("scsiIdentMsgParse: not a queue tag message (0x%x)\n",
			msg[1], 0, 0, 0, 0, 0);

	status = SCSI_IDENT_FAILED;
	}

    /*
     *	IDENTIFY message has been received: get the physical device.
     */
    else if ((pScsiPhysDev = scsiPhysDevIdGet (pScsiCtrl,
					       pScsiCtrl->peerBusId,
				      msg[0] & SCSI_MSG_IDENT_LUN_MASK)) == 0)
	{
	status = SCSI_IDENT_FAILED;	/* invalid IDENTIFY message */
	}

    /*
     *	Physical device has been found: if using tagged commands, ident.
     *	is complete iff a tag message has been received.
     */
    else if (pScsiPhysDev->nexus == SCSI_NEXUS_ITLQ)
	{
	status = tagged ? SCSI_IDENT_COMPLETE : SCSI_IDENT_INCOMPLETE;
	}

    /*
     *	Device is not using tagged commands: there can only be a single
     *	thread, so identification is complete.  Error if a tag message
     *	has been received.
     */
    else if (!tagged)
	{
	status = SCSI_IDENT_COMPLETE;
	}
    else
	{
	SCSI_ERROR_MSG ("scsiIdentMsgParse: unexpected tag "
		        "(phys dev = 0x%08x)\n",
			(int) pScsiPhysDev, 0, 0, 0, 0, 0);

	tagged = FALSE;
	status = SCSI_IDENT_FAILED;
	}

    *ppScsiPhysDev = pScsiPhysDev;
    *pTagNum       = tagged ? msg[2] : NONE;

    return (status);
    }


/******************************************************************************
*
* scsiMsgOutComplete - perform post-processing after a SCSI message is sent
*
* This routine parses the complete message and takes any necessary action.
*
* NOTE:
* This function is intended for use only by SCSI controller drivers.
*
* RETURNS: OK, or ERROR if the message is not supported.
*/

STATUS scsiMsgOutComplete
    (
    SCSI_CTRL   *pScsiCtrl,		/* ptr to SCSI controller info */
    SCSI_THREAD *pThread    	    	/* ptr to thread info          */
    )
    {
    int msgType;			/* SCSI message type code */

    msgType = pScsiCtrl->msgOutBuf[0];

    switch (msgType)
	{
	case SCSI_MSG_ABORT:
	    SCSI_DEBUG_MSG ("ABORT message out\n", 0, 0, 0, 0, 0, 0);
	    break;

	case SCSI_MSG_ABORT_TAG:
	    SCSI_DEBUG_MSG ("ABORT TAG message out\n", 0, 0, 0, 0, 0, 0);
	    break;
	    
        case SCSI_MSG_MESSAGE_REJECT:
            SCSI_DEBUG_MSG ("MESSAGE REJECT message out\n", 0, 0, 0, 0, 0, 0);
            break;

        case SCSI_MSG_NO_OP:
            SCSI_DEBUG_MSG ("NO OP message out\n", 0, 0, 0, 0, 0, 0);
            break;

        case SCSI_MSG_EXTENDED_MESSAGE:
	    msgType = pScsiCtrl->msgOutBuf[SCSI_EXT_MSG_TYPE_BYTE];

	    switch (msgType)
		{
    	        case SCSI_EXT_MSG_SYNC_XFER_REQ:
                    SCSI_DEBUG_MSG ("SYNCHRONOUS TRANSFER REQUEST message out\n"
	    	    	    "    offset = %d, period = %d\n",
			    pScsiCtrl->msgOutBuf[SCSI_SYNC_XFER_MSG_OFFSET],
			    pScsiCtrl->msgOutBuf[SCSI_SYNC_XFER_MSG_PERIOD],
			    0, 0, 0, 0);

	            scsiSyncXferNegotiate (pScsiCtrl,
				   pThread->pScsiTarget,
				   SYNC_XFER_MSG_OUT);
	            break;

                case SCSI_EXT_MSG_WIDE_XFER_REQ:
                    SCSI_DEBUG_MSG ("WIDE DATA TRANSFER REQUEST message out\n"
	    	    	    "    xferWidth = %d where 0 => 8 bits 1 => 16...\n",
			    pScsiCtrl->msgOutBuf[SCSI_WIDE_XFER_MSG_WIDTH],
			    0, 0, 0, 0, 0);

	            scsiWideXferNegotiate (pScsiCtrl,
				   pThread->pScsiTarget,
				   WIDE_XFER_MSG_OUT);
	            break;

                default:
		    SCSI_MSG ("Unsupported extended message (0x%02x) out !\n", 
			      msgType, 0, 0, 0, 0, 0);
		    return (ERROR);
                }

            break;

    	default:
	    if (msgType & SCSI_MSG_IDENTIFY)
		{
		SCSI_DEBUG_MSG ("IDENTIFY message (0x%02x) out\n", msgType,
				0, 0, 0, 0, 0);
		}
	    else
		{
		SCSI_MSG ("Unsupported message (0x%02x) out !\n", msgType,
			  0, 0, 0, 0, 0);
		return (ERROR);
		}
	    break;
	}

    return (OK);
    }

/******************************************************************************
*
* scsiMsgOutReject - perform post-processing when an outgoing message is rejected
*
* NOTE:
* This function is intended for use only by SCSI controller drivers.
*
* RETURNS: OK, or ERROR if the message is not supported.
*/

void scsiMsgOutReject
    (
    SCSI_CTRL   *pScsiCtrl,		/* ptr to SCSI controller info */
    SCSI_THREAD *pThread    	    	/* ptr to thread info          */
    )
    {
    int msgType;			/* SCSI message type code */

    pScsiCtrl->msgOutState = SCSI_MSG_OUT_NONE;

    /*
     *	Handle rejection of the message according to type
     */
    if ((msgType = pScsiCtrl->msgOutBuf[0]) == SCSI_MSG_EXTENDED_MESSAGE)
        msgType  = pScsiCtrl->msgOutBuf[SCSI_EXT_MSG_TYPE_BYTE];

    switch (msgType)
	{
    	case SCSI_EXT_MSG_SYNC_XFER_REQ:
            SCSI_DEBUG_MSG ("SYNCHRONOUS TRANSFER REQUEST rejected\n",
			    0, 0, 0, 0, 0, 0);

	    scsiSyncXferNegotiate (pScsiCtrl,
				   pThread->pScsiTarget,
				   SYNC_XFER_MSG_REJECTED);
	    break;

    	case SCSI_EXT_MSG_WIDE_XFER_REQ:
            SCSI_DEBUG_MSG ("WIDE DATA TRANSFER REQUEST rejected\n",
			    0, 0, 0, 0, 0, 0);

	    scsiWideXferNegotiate (pScsiCtrl,
				   pThread->pScsiTarget,
				   WIDE_XFER_MSG_REJECTED);
	    break;

    	default:
	    break;
	}
    }

/******************************************************************************
*
* scsiMsgInComplete - handle a complete SCSI message received from the target
*
* This routine parses the complete message and takes any necessary action,
* which may include setting up an outgoing message in reply.  If the message
* is not understood, the routine rejects it and returns an ERROR status.
*
* NOTE:
* This function is intended for use only by SCSI controller drivers.
*
* RETURNS: OK, or ERROR if the message is not supported.
*/

STATUS scsiMsgInComplete
    (
    SCSI_CTRL   *pScsiCtrl,		/* ptr to SCSI controller info */
    SCSI_THREAD *pThread    	    	/* ptr to thread info          */
    )
    {
    int  msgType;			/* SCSI message type code        */
    BOOL msgReject = FALSE;		/* will we send MSG REJECT msg ? */

    msgType = pScsiCtrl->msgInBuf[0];

    switch (msgType)
	{
    	case SCSI_MSG_COMMAND_COMPLETE:
	    SCSI_DEBUG_MSG ("COMMAND COMPLETE message in\n", 0, 0, 0, 0, 0, 0);

	    pThread->state = SCSI_THREAD_WAIT_COMPLETE;
	    break;
	    
        case SCSI_MSG_DISCONNECT:
	    SCSI_DEBUG_MSG ("DISCONNECT message in\n", 0, 0, 0, 0, 0, 0);

	    pThread->state = SCSI_THREAD_WAIT_DISCONNECT;
	    break;
	    
	    
	case SCSI_MSG_SAVE_DATA_POINTER:
	    SCSI_DEBUG_MSG ("SAVE DATA POINTER message in\n", 0, 0, 0, 0, 0, 0);
	    pThread->savedDataAddress = pThread->activeDataAddress;
	    pThread->savedDataLength  = pThread->activeDataLength;
	    break;
	    
	case SCSI_MSG_RESTORE_POINTERS:
	    SCSI_DEBUG_MSG ("RESTORE POINTERS message in\n", 0, 0, 0, 0, 0, 0);

	    pThread->activeDataAddress = pThread->savedDataAddress;
	    pThread->activeDataLength  = pThread->savedDataLength;
	    break;
	    
	case SCSI_MSG_MESSAGE_REJECT:
	    SCSI_DEBUG_MSG ("MESSAGE REJECT message in\n", 0, 0, 0, 0, 0, 0);

	    scsiMsgOutReject (pScsiCtrl, pThread);
	    break;
	    
	case SCSI_MSG_NO_OP:
	    SCSI_DEBUG_MSG ("NO OP message in\n", 0, 0, 0, 0, 0, 0);
	    break;
	    
        case SCSI_MSG_EXTENDED_MESSAGE:

            msgType = pScsiCtrl->msgInBuf[SCSI_EXT_MSG_TYPE_BYTE];

	    switch (msgType)
		{
	        case SCSI_EXT_MSG_SYNC_XFER_REQ:
	            SCSI_DEBUG_MSG ("SYNCHRONOUS TRANSFER REQUEST message in\n"
			            "    offset = %d, period = %d\n",
			        pScsiCtrl->msgInBuf[SCSI_SYNC_XFER_MSG_OFFSET],
			        pScsiCtrl->msgInBuf[SCSI_SYNC_XFER_MSG_PERIOD],
			        0, 0, 0, 0);
	    
	            scsiSyncXferNegotiate (pScsiCtrl,
				   pThread->pScsiTarget,
				   SYNC_XFER_MSG_IN);
	            break;

                case SCSI_EXT_MSG_WIDE_XFER_REQ:
                    SCSI_DEBUG_MSG ("WIDE DATA TRANSFER REQUEST message in\n"
	    	    	    "    xferWidth = %d where 0 => 8 bits 1 => 16...\n",
			    pScsiCtrl->msgInBuf[SCSI_WIDE_XFER_MSG_WIDTH],
			    0, 0, 0, 0, 0);

	            scsiWideXferNegotiate (pScsiCtrl,
				   pThread->pScsiTarget,
				   WIDE_XFER_MSG_IN);
	            break;

                default:
	            SCSI_ERROR_MSG ("unsupported extended message in" 
				    "(type = 0x%02x)\n",
			    	    msgType, 0, 0, 0, 0, 0);
	            msgReject = TRUE;
	            break;
		}
	    
	    break;

	default:
	    SCSI_ERROR_MSG ("unsupported message in (type = 0x%02x)\n",
			    msgType, 0, 0, 0, 0, 0);
	    
	    msgReject = TRUE;
	    break;
	}
    
    /*
     *	If the message is to be rejected, set up a MESSAGE REJECT message out.
     */
    if (msgReject)
	{
	pScsiCtrl->msgOutState  = SCSI_MSG_OUT_PENDING;
	pScsiCtrl->msgOutBuf[0] = SCSI_MSG_MESSAGE_REJECT;
	pScsiCtrl->msgOutLength = 1;
	}
    
    return (msgReject ? ERROR : OK);
    }


/*******************************************************************************
*
* scsiSyncXferNegotiate - initiate or continue negotiating transfer parameters
*
* This routine manages negotiation by means of a finite-state machine which
* is driven by "significant events" such as incoming and outgoing messages.
* Each SCSI target has its own independent state machine.
*
* NOTE:
* If the controller does not support synchronous transfer or if the
* target's maximum REQ/ACK offset is zero, attempts to initiate a round of
* negotiation are ignored.
*
* This function is intended for use only by SCSI controller drivers.
*
* RETURNS: N/A
*/

void scsiSyncXferNegotiate
    (
    SCSI_CTRL           *pScsiCtrl,	/* ptr to SCSI controller info  */
    SCSI_TARGET         *pScsiTarget,	/* ptr to SCSI target info      */
    SCSI_SYNC_XFER_EVENT eventType	/* tells what has just happened */
    )
    {
    SCSI_SYNC_XFER_STATE state = pScsiTarget->syncXferState;
    UINT8 offset;
    UINT8 period;
    BOOL  setValues = FALSE;
    BOOL  sendMsg   = FALSE;

    switch (eventType)
	{
	case SYNC_XFER_RESET:
	    state = SYNC_XFER_NOT_NEGOTIATED;
	    break;

    	case SYNC_XFER_NEW_THREAD:
	    offset = pScsiTarget->maxOffset;
	    period = pScsiTarget->minPeriod;

	    if (pScsiCtrl->syncXfer &&
                ((offset !=  pScsiTarget->xferOffset) ||
		 (period !=  pScsiTarget->xferPeriod)) &&
		(state  != SYNC_XFER_NEGOTIATION_COMPLETE))
		{
		/* initiate a round of negotiation */
		
		sendMsg = TRUE;
		state   = SYNC_XFER_REQUEST_PENDING;
		}
	    else
		state = SYNC_XFER_NEGOTIATION_COMPLETE;
    	    break;

    	case SYNC_XFER_MSG_IN:
	    offset = pScsiCtrl->msgInBuf[SCSI_SYNC_XFER_MSG_OFFSET];
	    period = pScsiCtrl->msgInBuf[SCSI_SYNC_XFER_MSG_PERIOD];

	    if (state == SYNC_XFER_REQUEST_SENT)
		{
		/* target is replying to our initial request */
		
		setValues = TRUE;
		state     = SYNC_XFER_NEGOTIATION_COMPLETE;
		}
	    else
		{
		/* target is making unsolicited request */
		
		sendMsg = TRUE;
    	    	state   = SYNC_XFER_REPLY_PENDING;
		}
    	    break;

    	case SYNC_XFER_MSG_OUT:
	    offset = pScsiCtrl->msgOutBuf[SCSI_SYNC_XFER_MSG_OFFSET];
	    period = pScsiCtrl->msgOutBuf[SCSI_SYNC_XFER_MSG_PERIOD];

	    if (state == SYNC_XFER_REQUEST_PENDING)
		{
		/* we have sent initial request (need target's reply) */
		
		state = SYNC_XFER_REQUEST_SENT;
		}
	    else if (state == SYNC_XFER_REPLY_PENDING)
		{
		/* we have replied to target's unsolicited request */
		
		setValues = TRUE;
		state     = SYNC_XFER_NEGOTIATION_COMPLETE;
		}
	    else
		SCSI_ERROR_MSG ("scsiSyncXferNegotiate: unexpected msg out\n",
				0, 0, 0, 0, 0, 0);
    	    break;

    	case SYNC_XFER_MSG_REJECTED:
	    offset = SCSI_SYNC_XFER_ASYNC_OFFSET;
	    period = SCSI_SYNC_XFER_ASYNC_PERIOD;
	    
	    if (state == SYNC_XFER_REQUEST_SENT)
		{
		/* target has rejected our initial request */
		
		setValues = TRUE;
		state     = SYNC_XFER_NEGOTIATION_COMPLETE;
		}
    	    break;

    	default:
	    SCSI_MSG ("scsiSyncXferNegotiate: invalid event type (%d)\n",
		      eventType, 0, 0, 0, 0, 0);
	    break;
	}

    if (sendMsg)
    	{
	UINT8 *msg = pScsiCtrl->msgOutBuf;
	
	if (pScsiCtrl->scsiSpecialHandler != NULL)
	    {
	    if ((pScsiTarget->syncSupport) && (pScsiTarget->syncXferState !=
					      SYNC_XFER_NEGOTIATION_COMPLETE))
		{
		(*pScsiCtrl->scsiXferParamsQuery) (pScsiCtrl, &offset, 
						   &period);

		if ((*pScsiCtrl->scsiXferParamsSet) (pScsiCtrl, offset, period)
		    != OK)
		    SCSI_ERROR_MSG ("syncXferNego: can't set xfer params.\n",
				    0, 0, 0, 0, 0, 0);
		state = SYNC_XFER_NEGOTIATION_COMPLETE;
		}
	    }
	else
	    {
	    (*pScsiCtrl->scsiXferParamsQuery) (pScsiCtrl, &offset, &period);
	    
	    msg[0]                         = SCSI_MSG_EXTENDED_MESSAGE;
	    msg[SCSI_EXT_MSG_LENGTH_BYTE]  = SCSI_SYNC_XFER_REQ_MSG_LENGTH;
	    msg[SCSI_EXT_MSG_TYPE_BYTE]    = SCSI_EXT_MSG_SYNC_XFER_REQ;
	    msg[SCSI_SYNC_XFER_MSG_PERIOD] = period;
	    msg[SCSI_SYNC_XFER_MSG_OFFSET] = offset;
	    
	    pScsiCtrl->msgOutState  = SCSI_MSG_OUT_PENDING;
	    pScsiCtrl->msgOutLength = SCSI_EXT_MSG_HDR_LENGTH +
				      SCSI_SYNC_XFER_REQ_MSG_LENGTH;
	    }
	}

    if (setValues)
    	{
	pScsiTarget->xferOffset = offset;
	pScsiTarget->xferPeriod = period;

	if ((*pScsiCtrl->scsiXferParamsSet) (pScsiCtrl, offset, period) != OK)
	    {
	    SCSI_ERROR_MSG ("scsiSyncXferNegotiate: can't set xfer params.\n",
			    0, 0, 0, 0, 0, 0);
	    }
    	}

    pScsiTarget->syncXferState = state;
    }


/*******************************************************************************
*
* scsiWideXferNegotiate - initiate or continue negotiating wide parameters
*
* This routine manages negotiation means of a finite-state machine which is
* driven by "significant events" such as incoming and outgoing messages.
* Each SCSI target has its own independent state machine.
*
* NOTE:
* If the controller does not support wide transfers or the
* target's transfer width is zero, attempts to initiate a round of
* negotiation are ignored; this is because zero is the default narrow transfer.
*
* This function is intended for use only by SCSI controller drivers.
*
* RETURNS: N/A
*/

void scsiWideXferNegotiate
    (
    SCSI_CTRL           *pScsiCtrl,	/* ptr to SCSI controller info  */
    SCSI_TARGET         *pScsiTarget,	/* ptr to SCSI target info      */
    SCSI_WIDE_XFER_EVENT eventType	/* tells what has just happened */
    )
    {
    SCSI_WIDE_XFER_STATE state = pScsiTarget->wideXferState;
    UINT8 xferWidth;
    BOOL  setValues = FALSE;
    BOOL  sendMsg   = FALSE;

    switch (eventType)
	{
	case WIDE_XFER_RESET:

	    state = WIDE_XFER_NOT_NEGOTIATED;
	    break;

    	case WIDE_XFER_NEW_THREAD:
	    xferWidth = pScsiTarget->xferWidth;

	    if (pScsiCtrl->wideXfer                     &&
		(xferWidth != SCSI_WIDE_XFER_SIZE_NARROW) &&
		(state  != WIDE_XFER_NEGOTIATION_COMPLETE))
		{
		/* initiate a round of negotiation */
		
		sendMsg = TRUE;
		state   = WIDE_XFER_REQUEST_PENDING;

		/* 
		 * Set the synchronous state to "complete". This state 
		 * should be reset when the wide negotiation is over. This
		 * is because a wide negotiation must occur before a 
		 * sync xfer negotiation.
		 */

		if (pScsiCtrl->scsiSpecialHandler == NULL)
		    pScsiTarget->syncXferState = SYNC_XFER_NEGOTIATION_COMPLETE;
		}
	    else
		{
		if ((pScsiCtrl->scsiSpecialHandler != NULL) &&
		    (state != WIDE_XFER_NEGOTIATION_COMPLETE))
		    sendMsg = TRUE;
		else 
		    state = WIDE_XFER_NEGOTIATION_COMPLETE;
		}
    	    break;

    	case WIDE_XFER_MSG_IN:
	    xferWidth = pScsiCtrl->msgInBuf[SCSI_WIDE_XFER_MSG_WIDTH];

	    if (state == WIDE_XFER_REQUEST_SENT)
		{
		/* target is replying to our initial request */
		
		setValues = TRUE;
		state     = WIDE_XFER_NEGOTIATION_COMPLETE;

                /* 
		 * After a wide negotiation is complete, a synchronous
		 * negotiation must be activated when the next new
		 * thread begins.
		 */

		pScsiTarget->syncXferState = SYNC_XFER_NOT_NEGOTIATED;
		}
	    else
		{
		/* target is making unsolicited request */
		
		sendMsg = TRUE;
    	    	state   = WIDE_XFER_REPLY_PENDING;
		}
    	    break;

    	case WIDE_XFER_MSG_OUT:
	    xferWidth = pScsiCtrl->msgOutBuf[SCSI_WIDE_XFER_MSG_WIDTH];

	    if (state == WIDE_XFER_REQUEST_PENDING)
		{
		/* we have sent initial request (need target's reply) */
		
		state = WIDE_XFER_REQUEST_SENT;
		}
	    else if (state == WIDE_XFER_REPLY_PENDING)
		{
		/* we have replied to target's unsolicited request */
		
		setValues = TRUE;
		state     = WIDE_XFER_NEGOTIATION_COMPLETE;

		/* Allow a synchronous negotiation to occur */

		pScsiTarget->syncXferState = SYNC_XFER_NOT_NEGOTIATED;
		}
	    else
		SCSI_ERROR_MSG ("scsiWideXferNegotiate: unexpected msg out\n",
				0, 0, 0, 0, 0, 0);
    	    break;

    	case WIDE_XFER_MSG_REJECTED:
	    xferWidth = SCSI_WIDE_XFER_SIZE_NARROW;
	    
	    if (state == WIDE_XFER_REQUEST_SENT)
		{
		/* target has rejected our initial request */
		
		setValues = TRUE;
		state     = WIDE_XFER_NEGOTIATION_COMPLETE;

		/* Allow a synchronous negotiation to occur */

		pScsiTarget->syncXferState = SYNC_XFER_NOT_NEGOTIATED;
		}
    	    break;

    	default:
	    SCSI_MSG ("scsiWideXferNegotiate: invalid event type (%d)\n",
		      eventType, 0, 0, 0, 0, 0);
	    break;
	}

    if (sendMsg)
    	{
	UINT8 *msg = pScsiCtrl->msgOutBuf;
	
	/*
         * Make sure that we are sending the correct params according to the
	 * controller driver.
	 */

	if (pScsiCtrl->scsiSpecialHandler != NULL)
	    {
	    if ((pScsiTarget->wideSupport) && (pScsiTarget->wideXferState != 
					       WIDE_XFER_NEGOTIATION_COMPLETE))
		{
		if ((*pScsiCtrl->scsiWideXferParamsSet) (pScsiCtrl, xferWidth) 
		    != OK)
		    SCSI_ERROR_MSG ("syncXferNego: can't set xfer params.\n",
				    0, 0, 0, 0, 0, 0);
		state = WIDE_XFER_NEGOTIATION_COMPLETE;
		pScsiTarget->syncXferState = SYNC_XFER_NOT_NEGOTIATED;
		}
	    }
	else
	    {
	    (*pScsiCtrl->scsiWideXferParamsQuery) (pScsiCtrl, &xferWidth);

	    msg[0]                         = SCSI_MSG_EXTENDED_MESSAGE;
	    msg[SCSI_EXT_MSG_LENGTH_BYTE]  = SCSI_WIDE_XFER_REQ_MSG_LENGTH;
	    msg[SCSI_EXT_MSG_TYPE_BYTE]    = SCSI_EXT_MSG_WIDE_XFER_REQ;
	    msg[SCSI_WIDE_XFER_MSG_WIDTH]  = xferWidth;
	    
	    pScsiCtrl->msgOutState  = SCSI_MSG_OUT_PENDING;
	    pScsiCtrl->msgOutLength = SCSI_EXT_MSG_HDR_LENGTH +
				      SCSI_WIDE_XFER_REQ_MSG_LENGTH;
	    }
	}
		
    if (setValues)
    	{
	pScsiTarget->xferWidth = xferWidth;

	if ((*pScsiCtrl->scsiWideXferParamsSet) (pScsiCtrl, xferWidth) != OK)
	    {
	    SCSI_ERROR_MSG ("scsiWideXferNegotiate: can't set xfer params.\n",
			    0, 0, 0, 0, 0, 0);
	    }
    	}

    pScsiTarget->wideXferState = state;
    }

/*******************************************************************************
*
* scsiTimeoutCvt - convert timeout in microseconds to system clock ticks
*
* Clip the specified timeout to be within the allowed range, then convert
* to system clock ticks.
*
* NOTE: this is non-trivial simply because it must avoid problems with
* overflow or underflow during the conversion.  A quick-and-dirty approach
* would use floating point maths, but that needlessly drags in another
* (possibly large) library.
*
* RETURNS: timeout in system clock ticks
*/
LOCAL UINT scsiTimeoutCvt
    (
    UINT uSecs				/* timeout in microseconds */
    )
    {
    UINT ticksPerSec = sysClkRateGet ();
    UINT secs;
    UINT Mticks;
    
    /*
     *	Clip timeout (if necessary) to be within current allowed range
     */
    if (uSecs > scsiMaxTimeout)
	uSecs = scsiMaxTimeout;

    if (uSecs < scsiMinTimeout)
	uSecs = scsiMinTimeout;

    /*
     *	Convert timeout from usec to system clock ticks
     */
    if (uSecs < 0xffffffff / ticksPerSec)
	{
	Mticks = uSecs * ticksPerSec;
	
	return (Mticks / 1000000);
	}
    else
	{
	secs = uSecs / 1000000;
	
    	return (secs * ticksPerSec);
	}
    }


/*******************************************************************************
*
* scsiPriorityCvt - convert priority for a SCSI transaction
*
* Converts a priority as specified by the application to one used by the
* SCSI manager task.  If the application specifies a default priority, the
* current task priority is used.  Otherwise, the specified value is used.
*
* NOTE:
* This routine should trap out-of-range priorities and return an error.
*
* RETURNS: SCSI transaction priority
*/
LOCAL SCSI_PRIORITY scsiPriorityCvt
    (
    UINT priority
    )
    {
    if (priority == SCSI_THREAD_TASK_PRIORITY)
	{
	int taskPriority;
	
    	taskPriorityGet (0, &taskPriority);

	priority = taskPriority;
	}

    return (priority);
    }


/*******************************************************************************
*
* scsiThreadCreate - create and initialise a new SCSI thread context
*
* Use the thread allocator to allocate a thread structure.  Initialise the
* thread context from the transaction it will execute.
*
* RETURNS: thread ptr, or 0 if an error occurs
*/
LOCAL SCSI_THREAD * scsiThreadCreate
    (
    SCSI_PHYS_DEV    * pScsiPhysDev,   	/* physical device used by thread  */
    SCSI_TRANSACTION * pScsiXaction	/* transaction thread will execute */
    )
    {
    SCSI_THREAD * pThread;
    SCSI_CTRL *   pScsiCtrl = pScsiPhysDev->pScsiCtrl;
    
    /*
     *	Allocate a thread structure
     */
    if ((pThread = scsiThreadAllocate (pScsiCtrl)) == 0)
	{
	SCSI_DEBUG_MSG ("scsiThreadCreate: can't allocate thread.\n",
			0, 0, 0, 0, 0, 0);
	return (0);
	}

    /*
     *	Initialise thread structure for this transaction
     */
    pThread->pScsiCtrl     =  pScsiCtrl;
    pThread->pScsiPhysDev  =  pScsiPhysDev;
    pThread->pScsiTarget   =  pScsiPhysDev->pScsiTarget;
    
    pThread->role          =  SCSI_ROLE_INITIATOR;
    pThread->state         =  SCSI_THREAD_INACTIVE;

    if (pScsiXaction->cmdTimeout == WAIT_FOREVER)
	pThread->timeout = WAIT_FOREVER;
    else
	pThread->timeout   =  scsiTimeoutCvt  (pScsiXaction->cmdTimeout);
    pThread->priority	   =  scsiPriorityCvt (pScsiXaction->priority);

    pThread->dataDirection =  pScsiXaction->dataDirection;
    pThread->tagType       =  pScsiXaction->tagType;
    pThread->cmdAddress    =  pScsiXaction->cmdAddress;
    pThread->cmdLength     =  pScsiXaction->cmdLength;
    pThread->dataAddress   =  pScsiXaction->dataAddress;
    pThread->dataLength    =  pScsiXaction->dataLength;
    pThread->statusAddress = &pScsiXaction->statusByte;
    pThread->statusLength  =  1;

    return (pThread);
    }
    
/*******************************************************************************
*
* scsiThreadDelete - delete a SCSI thread context
*
* Deallocate the thread structure.
*
* NOTE:
* This function is provided mainly for symmetry with "scsiThreadCreate()",
* and to provide a place-holder for future extensions.
*
* RETURNS: N/A
*/
LOCAL void scsiThreadDelete
    (
    SCSI_THREAD * pThread   	    	/* thread to be destroyed */
    )
    {
    /*
     *	Deallocate the thread structure
     */
    scsiThreadDeallocate (pThread->pScsiCtrl, pThread);
    }
    

/*******************************************************************************
*
* scsiThreadAllocate - allocate and initialise a SCSI thread structure
*
* To avoid dynamic creation and deletion of threads (and the objects they
* contain) for every SCSI transaction, a pool of available thread structures
* is automatically built up and maintained for each SCSI controller.
*
* Thus, the allocator first checks the pool (list) of free threads and takes
* a thread from this pool if possible.  Otherwise (there are no free threads)
* it allocates storage for and initialises a number of thread structures,
* reserves one to be returned to the caller, and adds the rest to the free
* list.
*
* Two globals control the allocation policy:
*
*   scsiAllocNumThreads	- how many threads to allocate in one go, and
*
*   scsiMaxNumThreads	- the maximum number of threads to allow per ctrlr
*
* NOTE:
* If a new thread cannot be allocated, this routine returns an error rather
* than waiting for a thread to become free.
*
* Access to the free thread list must be serialised because this routine is
* called in the context of the SCSI client task(s).
*
* RETURNS: thread ptr, or 0 if none can be allocated
*/
LOCAL SCSI_THREAD * scsiThreadAllocate
    (
    SCSI_CTRL * pScsiCtrl
    )
    {
    SCSI_THREAD * pThread;

    semTake (pScsiCtrl->mutexSem, WAIT_FOREVER);
    
    if ((pThread = (SCSI_THREAD *) lstGet (&pScsiCtrl->freeThreads)) == 0)
	{
	/*
	 *  No free threads: create more if possible.
    	 */
    	if (pScsiCtrl->nThreads >= scsiMaxNumThreads)
	    {
	    SCSI_DEBUG_MSG ("scsiThreadAllocate: max threads (%d) reached\n",
			    scsiMaxNumThreads, 0, 0, 0, 0, 0);

	    errnoSet (S_scsiLib_NO_MORE_THREADS);
	    }
	else
	    {
	    char * pArray;
	    int    i;
	    int    nThreads = min (scsiAllocNumThreads,
				   scsiMaxNumThreads - pScsiCtrl->nThreads);

	    if ((pArray = scsiThreadArrayCreate (pScsiCtrl, nThreads)) == 0)
		{
		SCSI_DEBUG_MSG ("scsiThreadAllocate: can't create array\n",
				0, 0, 0, 0, 0, 0);
                semGive (pScsiCtrl->mutexSem);
		return (NULL);
		}

	    pScsiCtrl->nThreads += nThreads;

	    /*
	     *  Grab the first one; put the rest onto the free queue
	     */
	    pThread = (SCSI_THREAD *) pArray;

	    for (i = 1; i < nThreads; ++i)
    	    	lstAdd (&pScsiCtrl->freeThreads,
			(NODE *) (pArray + i * pScsiCtrl->threadSize));
	    }
	}

    semGive (pScsiCtrl->mutexSem);
    
    return (pThread);
    }


/*******************************************************************************
*
* scsiThreadDeallocate - deallocate a thread structure
*
* Deallocation consists of returning the thread structure to the free thread
* list: once allocated, threads are never "really freed".
*
* NOTE:
* Access to the free thread list must be serialised because this routine is
* called in the context of the SCSI client task(s).
*
* RETURNS: N/A
*/
LOCAL void scsiThreadDeallocate
    (
    SCSI_CTRL *   pScsiCtrl,
    SCSI_THREAD * pThread
    )
    {
    semTake (pScsiCtrl->mutexSem, WAIT_FOREVER);
    
    lstAdd (&pScsiCtrl->freeThreads, (NODE *) pThread);

    semGive (pScsiCtrl->mutexSem);
    }


/*******************************************************************************
*
* scsiThreadArrayCreate - allocate and initialise storage for N threads
*
* Allocate storage for an array of `nThreads' thread structures, each of the
* controller-specific size.  Step through the array calling the controller-
* specific initialisation routine for each thread structure.
*
* RETURNS: ptr to array of thread structures, or 0 if an error occurs
*/
LOCAL char * scsiThreadArrayCreate
    (
    SCSI_CTRL * pScsiCtrl,
    int         nThreads
    )
    {
    char * pArray;
    int    i;

    /*
     *	Allocate memory for an array of thread structures
     */
    if ((pArray = malloc (nThreads * pScsiCtrl->threadSize)) == 0)
	{
	SCSI_DEBUG_MSG ("scsiThreadArrayCreate: can't allocate threads.\n",
			0, 0, 0, 0, 0, 0);
	return (0);
	}

    /*
     *	Initialise (controller-specific) each thread structure in array
     */
    for (i = 0; i < nThreads; ++i)
	{
	SCSI_THREAD * pThread = (SCSI_THREAD *) (pArray +
						 i * pScsiCtrl->threadSize);
	
    	if ((*pScsiCtrl->scsiThreadInit) (pScsiCtrl, pThread) != OK)
	    {
	    SCSI_DEBUG_MSG ("scsiThreadArrayCreate: can't initialise thread\n",
			    0, 0, 0, 0, 0, 0);
	    free (pArray);
	    return (0);
	    }
	}

    return (pArray);
    }


/*******************************************************************************
*
* scsiThreadInit - perform generic SCSI thread initialization
*
* This routine initializes the controller-independent parts of a thread 
* structure, which are specific to the SCSI manager.
*
* NOTE:
* This function should not be called by application programs.  It is intended
* to be used by SCSI controller drivers.
*
* RETURNS: OK, or ERROR if the thread cannot be initialized.
*/
STATUS scsiThreadInit
    (
    SCSI_THREAD * pThread
    )
    {
    if ((pThread->replyQ = msgQCreate (1, sizeof (SCSI_REPLY),
				          scsiThreadReplyQOptions)) == 0)
	{
	SCSI_DEBUG_MSG ("scsiThreadInit: can't create thread's reply queue.\n",
			0, 0, 0, 0, 0, 0);
	return (ERROR);
	}

    if ((pThread->wdog = wdCreate ()) == 0)
	{
	SCSI_DEBUG_MSG ("scsiThreadInit: can't create thread's watchdog.\n",
			0, 0, 0, 0, 0, 0);
	return (ERROR);
	}
    
    return (OK);
    }


/*******************************************************************************
*
* scsiThreadExecute - execute the specified thread
*
* Build an activation request and execute it via the SCSI manager.  Parse the
* return status in the SCSI manager's reply message.
*
* RETURNS: OK, or ERROR if the thread could not be executed, or failed
*/
LOCAL STATUS scsiThreadExecute
    (
    SCSI_THREAD * pThread   	    	/* thread to be executed  */
    )
    {
    SCSI_CTRL *  pScsiCtrl;  	    	/* ptr to controller info */
    SCSI_REQUEST request;	    	/* thread activation request msg */
    SCSI_REPLY   reply; 	    	/* thread completion reply   msg */

    pScsiCtrl = pThread->pScsiCtrl;

    /*
     *	Build thread activation request
     */
    request.type   = SCSI_REQUEST_ACTIVATE;
    request.thread = pThread;

    /*
     *  Execute thread activation request
     */
   if (scsiMgrRequestExecute (pScsiCtrl, &request, &reply) != OK)
	{
	SCSI_DEBUG_MSG ("scsiThreadExecute: error executing request.\n",
			0, 0, 0, 0, 0, 0);
	return (ERROR);
	}

    /*
     *  Check reply type, set errno if necessary
     */
    switch (reply.type)
	{
	case SCSI_REPLY_COMPLETE:
	    break;

	default:
	    logMsg ("scsiThreadExecute: invalid reply type (%d)\n",
		    reply.type, 0, 0, 0, 0, 0);
	    return (ERROR);
	}

    if (reply.status != OK)
	errnoSet (reply.errNum);

    return (reply.status);
    }


/*******************************************************************************
*
* scsiCommand - execute a single SCSI command
*
* This routine executes a single SCSI command.  It retries while the device
* is unable to execute the command (busy or queue full status), and returns
* when the target has completed (or failed) the command, at which time the
* status byte is available in the transaction structure.
*
* RETURNS: OK, or ERROR if not successful for any reason.
*/

LOCAL STATUS scsiCommand
    (
    SCSI_PHYS_DEV *    pScsiPhysDev,	/* ptr to physical device info */
    SCSI_TRANSACTION * pScsiXaction	/* ptr to transaction info */
    )
    {
    SCSI_THREAD * pThread;   
    SCSI_CTRL *   pScsiCtrl;		/* ptr to SCSI controller info */
    STATUS        status;		/* return status */
    BOOL          retry;		/* TRUE if command must be retried */

    pScsiCtrl = pScsiPhysDev->pScsiCtrl;

    /*
     *	Validate target device's bus ID
     */
    if (pScsiPhysDev->pScsiTarget->scsiDevBusId == pScsiCtrl->scsiCtrlBusId)
    	{
	errnoSet (S_scsiLib_ILLEGAL_BUS_ID);

       	return (ERROR);
	}

    if ((pThread = scsiThreadCreate (pScsiPhysDev, pScsiXaction)) == 0)
	{
	SCSI_DEBUG_MSG ("scsiCommand: can't create thread.\n",
			0, 0, 0, 0, 0, 0);
	return (ERROR);
	}

    do
        {
	retry = FALSE;
	
	if ((status = scsiThreadExecute (pThread)) != OK)
	    {
	    SCSI_DEBUG_MSG ("scsiCommand: thread execution failed\n",
			    0, 0, 0, 0, 0, 0);
	    }
	else
	    {
	    switch (pScsiXaction->statusByte)
	    	{
	    	case SCSI_STATUS_BUSY:
	    	    SCSI_DEBUG_MSG ("scsiCommand: device busy - retrying\n",
			    	0, 0, 0, 0, 0, 0);
		    retry = TRUE;
		    break;

	    	case SCSI_STATUS_QUEUE_FULL:
	    	    SCSI_DEBUG_MSG ("scsiCommand: queue full - retrying\n",
			    	    0, 0, 0, 0, 0, 0);
		    retry = TRUE;
		    break;

	    	default:
		    break;
		}
	    }
        }
    while (retry);

    (void) scsiThreadDelete (pThread);

    return (status);
    }


/*******************************************************************************
*
* scsi2Transact - execute a SCSI transaction
*
* This routine calls scsiCommand() to execute the command specified.
* If there are physical path management errors, then this routine returns
* ERROR.  If not, then the status returned from the command is checked.  If
* it is "Check Condition", then a "Request Sense" CCS command is executed
* and the sense key is examined.  An indication of the success of the
* command is returned (OK or ERROR).
*
* RETURNS: OK, or ERROR if a path management error occurs
* or the status or sense information indicates an error.
*/

LOCAL STATUS scsi2Transact
    (
    SCSI_PHYS_DEV *pScsiPhysDev,        /* ptr to the target device    */
    SCSI_TRANSACTION *pScsiXaction      /* ptr to the transaction info */
    )
    {
    STATUS status;		    /* routine return status                */
    int senseKey;		    /* extended sense key from target       */
    int addSenseCode;		    /* additional sense code from target    */
    int addSenseCodeQual;           /* additional sense code qualifier      */

    SCSI_DEBUG_MSG ("scsi2Transact:\n",0, 0, 0, 0, 0, 0);

    if ((status = scsiCommand (pScsiPhysDev, pScsiXaction)) == ERROR)
	{
	SCSI_DEBUG_MSG ("scsiTransact: scsiCommand ERROR.\n",
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

	    reqSenseXaction.cmdAddress = (UINT8 *) reqSenseCmd;

	    /* if there is no user request sense buffer, supply it */

	    if ( pScsiPhysDev->pReqSenseData == (UINT8 *)NULL )
		{
                reqSenseXaction.dataAddress = reqSenseData;

		if (!pScsiPhysDev->extendedSense)
		    reqSenseXaction.dataLength = NON_EXT_SENSE_DATA_LENGTH;
                else
		    reqSenseXaction.dataLength = REQ_SENSE_ADD_LENGTH_BYTE + 1;
                } 
            else
		{
	        reqSenseXaction.dataAddress = pScsiPhysDev->pReqSenseData;
	        reqSenseXaction.dataLength  = 
		                           pScsiPhysDev->reqSenseDataLength;
                }

	    reqSenseXaction.dataDirection = O_RDONLY;
	    reqSenseXaction.addLengthByte = REQ_SENSE_ADD_LENGTH_BYTE;
	    reqSenseXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
	    reqSenseXaction.tagType       = SCSI_TAG_SENSE_RECOVERY;
    	    reqSenseXaction.priority      = SCSI_THREAD_MAX_PRIORITY;

	    SCSI_DEBUG_MSG ("scsiTransact: issuing a REQUEST SENSE command.\n",
			    0, 0, 0, 0, 0, 0);

	    if ((status = scsiCommand (pScsiPhysDev, &reqSenseXaction))
		== ERROR)
		{
		SCSI_DEBUG_MSG ("scsiTransact: scsiCommand ERROR.\n",
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

            /* store additional sense code qualifier */

	    addSenseCodeQual = (pScsiPhysDev->pReqSenseData)[13];

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
		    SCSI_DEBUG_MSG ("ASCQ = 0x%02x\n",
				    addSenseCodeQual, 0, 0, 0, 0, 0);
	            status = OK;
	            goto cleanExit;
	            }
	        case SCSI_SENSE_KEY_NOT_READY:
	            {
		    SCSI_DEBUG_MSG ("scsiTransact: Not Ready Sense,",
				    0, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
				    addSenseCode, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("ASCQ = 0x%02x\n",
				    addSenseCodeQual, 0, 0, 0, 0, 0);
	            errnoSet (S_scsiLib_DEV_NOT_READY);
	            status = ERROR;
	            goto cleanExit;
	            }
                case SCSI_SENSE_KEY_MEDIUM_ERROR:
                    {
                    SCSI_DEBUG_MSG ("scsiTransact: Not Ready Sense,",
                                    0, 0, 0, 0, 0, 0);
                    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
                                    addSenseCode, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("ASCQ = 0x%02x\n",
				    addSenseCodeQual, 0, 0, 0, 0, 0);
                    errnoSet (S_scsiLib_MEDIUM_ERROR);
                    status = ERROR;
                    goto cleanExit;
                    }
                case SCSI_SENSE_KEY_HARDWARE_ERROR:
                    {
                    SCSI_DEBUG_MSG ("scsiTransact: Hardware Error Sense,",
                                    0, 0, 0, 0, 0, 0);
                    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
                                    addSenseCode, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("ASCQ = 0x%02x\n",
				    addSenseCodeQual, 0, 0, 0, 0, 0);
                    errnoSet (S_scsiLib_HARDWARE_ERROR);
                    status = ERROR;
                    goto cleanExit;
                    }
                case SCSI_SENSE_KEY_ILLEGAL_REQUEST:
                    {
                    SCSI_DEBUG_MSG ("scsiTransact: Illegal Request Sense,",
                                    0, 0, 0, 0, 0, 0);
                    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
                                    addSenseCode, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("ASCQ = 0x%02x\n",
				    addSenseCodeQual, 0, 0, 0, 0, 0);
                    errnoSet (S_scsiLib_ILLEGAL_REQUEST);
                    status = ERROR;
                    goto cleanExit;
                    }
                /* 
		 * A UNIT ATTENTION occurs because of the following main
		 * conditions:
		 *	1. Device reset (SCSI command or hard reset)
		 *	2. Removable medium has been changed
		 *	3. Another initiator changed mode params or cleared
		 *         tagged commnads
		 *	4. Others
		 * A medium change has to be reported the BLK_DEV or SEQ_DEV
		 * structure.
		 */

	        case SCSI_SENSE_KEY_UNIT_ATTENTION:
	            {
		    SCSI_DEBUG_MSG ("scsiTransact: Unit Attention Sense,",
				    0, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
				    addSenseCode, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("ASCQ = 0x%02x\n",
				    addSenseCodeQual, 0, 0, 0, 0, 0);

                    /* check for medium change and WP bit */

                    chkMedChangeAndWP (addSenseCode, pScsiPhysDev);

		    errnoSet (S_scsiLib_UNIT_ATTENTION);
	            status = ERROR;

                   /* retry the command sequence */
                   
                   if (addSenseCode == SCSI_ADD_SENSE_DEVICE_RESET)
                     status = scsiCommand (pScsiPhysDev, pScsiXaction);
 
	            goto cleanExit;
                    }

                /* 
		 * Indicates that a command that reads or writes the medium
		 * was attempted on a block that is protected from this 
		 * operation. The read/write operation is not performed.
		 * In simple terms, we get this sense for write protected 
		 * media
		 */

	        case SCSI_SENSE_KEY_DATA_PROTECT:
	            {
		    SCSI_DEBUG_MSG ("scsiTransact: Data Protect Sense,",
				    0, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
				    addSenseCode, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("ASCQ = 0x%02x\n",
				    addSenseCodeQual, 0, 0, 0, 0, 0);

		    if (addSenseCode == SCSI_ADD_SENSE_WRITE_PROTECTED)
			errnoSet (S_scsiLib_WRITE_PROTECTED);
	            status = ERROR;
	            goto cleanExit;
	            }
                case SCSI_SENSE_KEY_BLANK_CHECK:
                    {
                    SCSI_DEBUG_MSG ("scsiTransact: Blank Check Sense,",
                                    0, 0, 0, 0, 0, 0);
                    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
                                    addSenseCode, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("ASCQ = 0x%02x\n",
				    addSenseCodeQual, 0, 0, 0, 0, 0);
                    errnoSet (S_scsiLib_BLANK_CHECK);
                    status = ERROR;
                    goto cleanExit;
                    }
                case SCSI_SENSE_KEY_ABORTED_COMMAND:
                    {
                    SCSI_DEBUG_MSG ("scsiTransact:  Aborted Command Sense,",
                                    0, 0, 0, 0, 0, 0);
                    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
                                    addSenseCode, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("ASCQ = 0x%02x\n",
				    addSenseCodeQual, 0, 0, 0, 0, 0);
                    errnoSet (S_scsiLib_ABORTED_COMMAND);
                    status = ERROR;
                    goto cleanExit;
                    }
                case SCSI_SENSE_KEY_VOLUME_OVERFLOW:
                    {
                    SCSI_DEBUG_MSG ("scsiTransact: Volume Overflow Sense,",
                                    0, 0, 0, 0, 0, 0);
                    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
                                    addSenseCode, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("ASCQ = 0x%02x\n",
				    addSenseCodeQual, 0, 0, 0, 0, 0);
                    errnoSet (S_scsiLib_VOLUME_OVERFLOW);
                    status = ERROR;
                    goto cleanExit;
                    }
	        default:
		    {
		    SCSI_DEBUG_MSG ("scsiTransact: Sense = %x,",
				    senseKey, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("Additional Sense Code = 0x%02x\n",
				    addSenseCode, 0, 0, 0, 0, 0);
		    SCSI_DEBUG_MSG ("ASCQ = 0x%02x\n",
				    addSenseCodeQual, 0, 0, 0, 0, 0);
	            status = ERROR;
		    errnoSet (S_scsiLib_UNKNOWN_SENSE_DATA);
	            goto cleanExit;
	            }
	        }
	    }

	case SCSI_STATUS_BUSY:
	case SCSI_STATUS_QUEUE_FULL:
	    /* NOTE: should never occur - see "scsiCommand()" */
	    /* FALLTHROUGH */
	default:
    	    SCSI_ERROR_MSG ("scsiTransact: unsupported status (0x%02x)\n",
    	    	    	    pScsiXaction->statusByte & SCSI_STATUS_MASK,
			    0, 0, 0, 0, 0);
	    status = ERROR;
	    break;
	}

cleanExit:

    return (status);
    }

/*******************************************************************************
*
* chkMedChangeAndWP - check if medium has changed and if it is write protected
*
* Look at the sense information from a request sense command and determine if 
* the medium has been changed. Issue a MODE SENSE command, in order to check
* if the medium is write protected. Set the appropriate values in either a
* BLK_DEV structure or a SEQ_DEV structure, depending upon the type of device.
*
* RETURN: N/A
*/
LOCAL void chkMedChangeAndWP 
    (
    int             addSenseCode, 	/* additional sense code */
    SCSI_PHYS_DEV * pScsiPhysDev	/* ptr to a SCSI physcial device */
    )
    {
    SCSI_BLK_DEV_NODE *pBlkDevNode; /* ptr for looping through BLK_DEV list */
    SCSI_SEQ_DEV      *pScsiSeqDev; /* ptr to SCSI_SEQ_DEV                  */
    UINT8 modeSenseHeader [4];    /* mode sense data header */
    SCSI_COMMAND modeSenseCommand;/* SCSI command byte array */
    SCSI_TRANSACTION scsiXaction; /* info on a SCSI xaction */
    int status = OK;


    /* Has there been a change in removable medium ? */

    if (addSenseCode == SCSI_ADD_SENSE_MEDIUM_CHANGED)
	{

    	semTake (pScsiPhysDev->mutexSem, WAIT_FOREVER);

	if (pScsiPhysDev->scsiDevType == SCSI_DEV_SEQ_ACCESS)
	    {
	    pScsiSeqDev = pScsiPhysDev->pScsiSeqDev;
	    pScsiSeqDev->seqDev.sd_readyChanged = TRUE;
	    }
        else 	/* it is a block device */
	    {
    	    for (pBlkDevNode = (SCSI_BLK_DEV_NODE *)
			                   lstFirst (&pScsiPhysDev->blkDevList);
                 pBlkDevNode != NULL;
                 pBlkDevNode = (SCSI_BLK_DEV_NODE *)
			   	             lstNext (&pBlkDevNode->blkDevNode))
	        {
		pBlkDevNode->scsiBlkDev.blkDev.bd_readyChanged = TRUE;
       	        }
	    }

    	semGive (pScsiPhysDev->mutexSem);
	}

    /* Was there some kind of a device reset ? */

    else if (addSenseCode == SCSI_ADD_SENSE_DEVICE_RESET) 
	{
	pScsiPhysDev->resetFlag = TRUE;
	}

    /* issue a MODE SENSE command */

    /* The objective of issuing a MODE SENSE command is to
     * determine if the media was write protected. 
     *
     * XXX - This may not be the best way to tell if we have
     *       read-only or read-write media. Sense key 0x7 and
     *       additionaly sense key of 0x27 could do the job
     *       as well. This should be verified.
     */

    if (scsiCmdBuild (modeSenseCommand, &scsiXaction.cmdLength,
		      SCSI_OPCODE_MODE_SENSE, pScsiPhysDev->scsiDevLUN, FALSE,
		      0, sizeof (modeSenseHeader), (UINT8) 0)
        == ERROR)
	return;

    scsiXaction.cmdAddress    = modeSenseCommand;
    scsiXaction.dataAddress   = modeSenseHeader;
    scsiXaction.dataDirection = O_RDONLY;
    scsiXaction.dataLength    = sizeof (modeSenseHeader);
    scsiXaction.addLengthByte = MODE_SENSE_ADD_LENGTH_BYTE;
    scsiXaction.cmdTimeout    = SCSI_TIMEOUT_5SEC;
    scsiXaction.tagType       = SCSI_TAG_DEFAULT;
    scsiXaction.priority      = SCSI_THREAD_TASK_PRIORITY;

    SCSI_DEBUG_MSG ("scsiTransact: issuing a MODE SENSE cmd.\n",
				                             0, 0, 0, 0, 0, 0);

    if ((status = scsiCommand (pScsiPhysDev, &scsiXaction)) == ERROR)
	{
	SCSI_DEBUG_MSG ("scsiCommand returned ERROR\n", 0, 0, 0, 0, 0, 0);
	return;
	}

    /* MODE SENSE command status != GOOD indicates
     * fatal error
     */

    if (scsiXaction.statusByte != SCSI_STATUS_GOOD)
	{
	SCSI_DEBUG_MSG ("scsiTransact: bad MODE SELECT stat.\n",
					 		      0, 0, 0, 0, 0, 0);
	return;
	}

    else 	/* MODE SENSE returned successfully */
	{

    	semTake (pScsiPhysDev->mutexSem, WAIT_FOREVER);

	/*
         * if the WP bit of the device specific parameter
	 * of the Mode parameter header is set then the
	 * medium is write protected 
	 */

        if (pScsiPhysDev->scsiDevType == SCSI_DEV_SEQ_ACCESS)
            {

            pScsiSeqDev = pScsiPhysDev->pScsiSeqDev;
	    pScsiSeqDev->seqDev.sd_mode = 
                ( modeSenseHeader [SCSI_MODE_DEV_SPECIFIC_PARAM] & 
	          (UINT8) SCSI_DEV_SPECIFIC_WP_MASK
                ) ? O_RDONLY : O_RDWR;
            }
            else    /* it is a block device */
            {

    	    for (pBlkDevNode = (SCSI_BLK_DEV_NODE *)
			                   lstFirst (&pScsiPhysDev->blkDevList);
                 pBlkDevNode != NULL;
         	 pBlkDevNode = (SCSI_BLK_DEV_NODE *)
				            lstNext (&pBlkDevNode->blkDevNode))
		{
		pBlkDevNode->scsiBlkDev.blkDev.bd_mode =
		    ( modeSenseHeader [SCSI_MODE_DEV_SPECIFIC_PARAM] & 
                      (UINT8) SCSI_DEV_SPECIFIC_WP_MASK
                    ) ? O_RDONLY : O_RDWR;
                }
       	    }

    	semGive (pScsiPhysDev->mutexSem);

	SCSI_DEBUG_MSG ("Write-protect bit = %x.\n",
		           ( modeSenseHeader [SCSI_MODE_DEV_SPECIFIC_PARAM] & 
                             (UINT8) SCSI_DEV_SPECIFIC_WP_MASK
                           ), 0, 0, 0, 0, 0);
	}
    }

/*******************************************************************************
*
* scsi2Ioctl - perform a device-specific control function
*
* This routine performs a specified function using a specified SCSI physical
* device.
*
* RETURNS: The status of the request, or ERROR if the request is unsupported.
*/
LOCAL STATUS scsi2Ioctl
    (
    SCSI_PHYS_DEV *pScsiPhysDev,/* ptr to SCSI physical device info */
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
        case FIOERASE:
	    /* issue a tape erase command */
	    return (scsiErase (pScsiPhysDev, arg));

        case FIODENSITYSET:
        case FIODENSITYGET:
        case FIOBLKSIZESET:
        case FIOBLKSIZEGET:
	     {
	     int tmp;


             tmp = (scsiSeqIoctl ((SCSI_SEQ_DEV *)pScsiPhysDev, function, arg));   
	     logMsg ("scsiIoctl: blkSize: %d\n", tmp,0,0,0,0,0);
	     return tmp;
	     }

        default:
	    errnoSet (S_ioLib_UNKNOWN_REQUEST);
	    return (ERROR);
        }
    }

/*******************************************************************************
*
* scsiCacheSnoopEnable - inform SCSI that hardware snooping of caches is enabled
*
* This routine informs the SCSI library that hardware snooping is enabled
* and that scsi2Lib need not execute any cache coherency code.
* In order to make scsi2Lib aware that hardware
* snooping is enabled, this routine should be called after all SCSI-2
* initializations, especially after scsi2CtrlInit().
* 
* RETURNS: N/A
*/

void scsiCacheSnoopEnable 
    (
    SCSI_CTRL * pScsiCtrl 	/* pointer to a SCSI_CTRL structure */
    )
    {

    pScsiCtrl->cacheSnooping = TRUE;
    }

/*******************************************************************************
*
* scsiCacheSnoopDisable - inform SCSI that hardware snooping of caches is disabled
*
* This routine informs the SCSI library that hardware snooping is disabled
* and that scsi2Lib should execute any neccessary cache coherency code.
* In order to make scsi2Lib aware that hardware
* snooping is disabled, this routine should be called after all SCSI-2
* initializations, especially after scsi2CtrlInit().
* 
* RETURNS: N/A
*/

void scsiCacheSnoopDisable 
    (
    SCSI_CTRL * pScsiCtrl 	/* pointer to a SCSI_CTRL structure */
    )
    {

    pScsiCtrl->cacheSnooping = FALSE;
    }

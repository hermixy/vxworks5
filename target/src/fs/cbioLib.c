/* cbioLib.c - Cached Block I/O library */

/* Copyright 1999-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01y,03jun02,jkf  fixed SPR#78162, cbioLib has INTERNAL comment visable
01x,29apr02,jkf  SPR#76013, blkWrapBlkRWbuf() shall semGive what it semTake's,
                 also removed cbioWrapBlkDev check for bytesPerBlk since the 
                 media may not be present at cdromFs mount time; improved 
                 error message to mention the BLK_DEV * value that fails.
01w,14jan02,jkf  SPR#72533, added cbioRdyChk(), which is used in cbioShow().
01v,12dec01,jkf  fixing diab build warnings.
01u,10dec01,jkf  SPR#72039, cbioShow should use strcmp.
01t,09dec01,jkf  SPR#71637, fix for SPR#68387 caused ready changed bugs.
01s,15nov01,jkf  SPR#71720, avoid unaligned pointer access.
01r,20sep01,jkf  SPR#69031, common code for both AE & 5.x.
01q,01aug01,jyo  Fixed SPR#68387: readyChanged bit is not correctly checked to
                 verify the change in the media, SPR#69411: Change in media's
                 readyChanged bit is not being propogated appropriately to the
                 layers above.
01p,14jun01,jyo  Fixed SPR#67729 and SPR#68150: Media's state information is
                 read correctly from the lowest layer and tidied the code to
                 use the new member subDev instead of pDc of CBIO_DEV
                 structure.
01o,08sep00,nrj  Fixed SPR#33934,33925: avoid using cbioBlkDevPtrLimit
01n,19apr00,dat  doc fixup
01m,07dec99,jkf  changed OBJ_CORE to HANDLE, KHEAP_ALLOC, made mode int
                 changes for cbioDevVerify(), cbioWrapBlkDev().
01l,15sep99,jkf  changes for public CBIO API.
01k,31jul99,jkf  T2 merge, tidiness, & spelling.
01j,14dec98,lrn  make pointer range limit configurable (SPR#23977)
01i,22nov98,lrn  added statusChk verification after reset
01h,04nov98,lrn  refined error handling with Floppy
01g,29oct98,lrn  fixed BLK_DEV replacement for PCMCIA, w/ CBIO_RESET 3rd arg   
01f,20oct98,lrn  removed the previous mode because of non-ATA block drivers.
01e,20oct98,lrn  added capacity adjustment to work around ATA driver bug
01d,20oct98,lrn  fixed SPR#22553, SPR#22731
01c,10sep98,lrn  add CBIO_RESET for blk dev replacement, add error handling
01b,03sep98,lrn  add block device wrapper
01a,15jan98,lrn  written, preliminary
*/

/*
DESCRIPTION

This library provides the Cached Block Input Output Application 
Programmers Interface (CBIO API).  Libraries such as dosFsLib, 
rawFsLib, and usrFdiskPartLib use the CBIO API for I/O operations 
to underlying devices.

This library also provides generic services for CBIO modules.  
The libraries dpartCbio, dcacheCbio, and ramDiskCbio are examples 
of CBIO modules that make use of these generic services.

This library also provides a CBIO module that converts blkIo driver 
BLK_DEV (blkIo.h) interface into CBIO API compliant interface using
minimal memory overhead.  This lean module is known as the basic 
BLK_DEV to CBIO wrapper module.   


CBIO MODULES AND DEVICES

A CBIO module contains code for supporting CBIO devices.  
The libraries cbioLib, dcacheCbio, dpartCbio, and ramDiskCbio 
are examples of CBIO modules.   

A CBIO device is a software layer that provide its master control
of I/O to it subordinate.  CBIO device layers typicaly reside 
logically below a file system and above a storage device.  CBIO 
devices conform to the CBIO API on their master (upper) interface.

CBIO modules provide a CBIO device creation routine used to instantiate 
a CBIO device.  The CBIO modules device creation routine returns 
a CBIO_DEV_ID handle.  The CBIO_DEV_ID handle is used to uniquely 
identify the CBIO device layer instance.  The user of the CBIO device 
passes this handle to the CBIO API routines when accessing the device.

The libraries dosFsLib, rawFsLib, and usrFdiskPartLib are considered 
users of CBIO devices because they use the CBIO API on their subordinate
(lower) interface.  They do not conform to the CBIO API on their master 
interface, therefore they are not CBIO modules.  They are users of CBIO 
devices and always reside above CBIO devices in the logical stack.


TYPES OF CBIO DEVICES 

A "CBIO to CBIO device" uses the CBIO API for both its master and its 
subordinate interface.  Typically, some type of module specific I/O 
processing occurs during the interface between the master and subordinate
layers.  The libraries dpartCbio and dcacheCbio are examples of CBIO to 
CBIO devices.   CBIO to CBIO device layers are stackable.   Care should be 
taken to assemble the stack properly.  Refer to each modules reference 
manual entry for recommendations about the optimum stacking order. 

A "CBIO API device driver" is a device driver which provides the CBIO
API as the interface between the hardware and its upper layer.  The 
ramDiskCbio.c RAM DISK driver is an example of a simple CBIO API 
device driver.

A "basic BLK_DEV to CBIO wrapper device" wraps a subordinate BLK_DEV 
layer with a CBIO API compatible layer.  The wrapper is provided via 
the cbioWrapBlkDev() function.

The logical layers of a typical system using a CBIO RAM DISK appear:

.CS
         +--------------------+
         | Application module | 
         +--------------------+ <-- read(), write(), ioctl()
                   |
         +--------------------+
         | VxWorks I/O System | 
         +--------------------+ <-- IOS layer iosRead,Write,ioctl
                   |                (iosDrvInstall rtns from dosFsLib)
      +--------------- -----------+
      | File System (DOSFS/RAWFS) | 
      +---------------------------+ <-- CBIO API (cbioBlkRW, cbioIoctl, etc.)
                   |
+----------------------------------------------+
| CBIO API device driver module (ramDiskCbio.c)|
+----------------------------------------------+
                   |
              +----------+
              | Hardware |
              +----------+
.CE


The logical layers of a typical system with a fixed disk
using CBIO partitioning layer and a CBIO caching layer appears:

.CS
            +--------------------+
            | Application module |
            +--------------------+ <-- read(), write(), ioctl()
                      |
            +-------------------+
            | VxWorks IO System |
            +-------------------+ <-- IOS layer Read,Write, ioctl
                      |               (iosDrvInstall rtns from dosFsLib)
         +---------------------------+
         | File System (DOSFS/RAWFS) |
         +---------------------------+ <-- CBIO API RTNS (cbioLib.h)
                      |
      +---------------------------------+
      | CBIO to CBIO device (dpartCbio) |
      +---------------------------------+ <-- CBIO API RTNS
                      |
     +----------------------------------+
     | CBIO to CBIO device (dcacheCbio) |
     +----------------------------------+ <-- CBIO API RTNS 
                      |
  +------------------------------------------------+
  | basic CBIO to BLK_DEV wrapper device (cbioLib) |
  +------------------------------------------------+ <-- BLK_DEV (blkIo.h)
                      |
+-------------------------------------------------------+
| BLK_DEV API device driver. scsiLib, ataDrv, fdDrv,etc | 
+-------------------------------------------------------+
                      |
          +-------------------------+
          | Storage Device Hardware |
          +-------------------------+
.CE



PUBLIC CBIO API

The CBIO API provides user access to CBIO devices.  Users of CBIO 
devices are typically either file systems or other CBIO devices.  

The CBIO API is exposed via cbioLib.h.  Users of CBIO modules include 
the cbioLib.h header file.  The libraries dosFsLib, dosFsFat, dosVDirLib, 
dosDirOldLib, usrFdiskPartLib, and rawFsLib all use the CBIO API to access 
CBIO modules beneath them.    


The following functions make up the public CBIO API:
.IP
cbioLibInit() - Library initialization routine 
.IP
cbioBlkRW() - Transfer blocks (sectors) from/to a memory buffer
.IP
cbioBytesRW() - Transfer bytes from/to a memory buffer
.IP
cbioBlkCopy() - Copy directly from block to block (sector to sector)
.IP
cbioIoctl() - Perform I/O control operations on the CBIO device
.IP
cbioModeGet() - Get the CBIO device mode (O_RDONLY, O_WRONLY, or O_RDWR)
.IP
cbioModeSet() - Set the CBIO device mode (O_RDONLY, O_WRONLY, or O_RDWR)
.IP
cbioRdyChgdGet() - Determine the CBIO device ready status state
.IP
cbioRdyChgdSet() - Force a change in the CBIO device ready status state 
.IP
cbioLock() - Obtain exclusive ownership of the CBIO device 
.IP
cbioUnlock() - Release exclusive ownership of the CBIO device 
.IP
cbioParamsGet() - Fill a CBIO_PARAMS structure with data from the CBIO device
.IP
cbioDevVerify() - Verify valid CBIO device 
.IP
cbioWrapBlkDev() - Create CBIO wrapper atop a BLK_DEV 
.IP
cbioShow() - Display information about a CBIO device
.LP

These CBIO API functions (except cbioLibInit()) are passed a CBIO_DEV_ID 
handle in the first argument.   This handle (obtained from the subordinate 
CBIO modules device creation routine) is used by the routine to verify the
the CBIO device is valid and then to perform the requested operation
on the specific CBIO device.

When the CBIO_DEV_ID passed to the CBIO API routine is not a valid CBIO 
handle, ERROR will be returned with the errno set to 
S_cbioLib_INVALID_CBIO_DEV_ID (cbioLib.h).   

Refer to the individual manual entries for each function for a complete 
description.



THE BASIC CBIO TO BLK_DEV WRAPPER MODULE

The basic CBIO to BLK_DEV wrapper is a minimized disk cache using 
simplified algorithms.   It is used to convert a legacy BLK_DEV 
device into as CBIO device.  It may be used standalone with solid 
state disks which do not have mechanical seek and rotational latency 
delays, such flash cards.  It may also be used in conjunction with
the dpartCbio and dcacheCbio libraries.  The DOS file system 
dosFsDevCreate routine will call cbioWrapBlkDev() internally, so the file 
system may be installed directly on top of a block driver BLK_DEV
or it can be used with cache and partitioning support. 

The function cbioWrapBlkDev() is used to create the CBIO wrapper atop
a BLK_DEV device.

The functions dcacheDevCreate and dpartDevCreate also both interally use 
cbioDevVerify() and cbioWrapBlkDev() to either stack the new CBIO device 
atop a validated CBIO device or to create a basic CBIO to BLK_DEV wrapper 
as needed.  The user typically never needs to manually invoke the 
cbioWrapBlkDev() or cbioDevVerify() functions.

Please note that the basic CBIO BLK_DEV wrapper is inappropriate 
for rotational media without the disk caching layer.  The services 
provided by the dcacheCbio module are more appropriate for use on 
rotational disk devices and will yeild superior performance
when used.

SEE ALSO
.I "VxWorks Programmers Guide: I/O System"

INCLUDE FILES
cbioLib.h

INTERNAL

CBIO module internal implementation details

This section describes information concerning CBIO modules themselves.
Users of CBIO modules use only the public CBIO API.  It is not recommend 
that the information presented in this section be used to circumvent 
the public CBIO API.  All the private data structures presented herein 
are private and are indeed subject to change without notice.

Refer to the source file src/usr/ramDiskCbio.c for an implementation 
example of a CBIO module.  


The CBIO_DEV structure (PRIVATE)

The common fundamental component of every CBIO device is its CBIO_DEV 
structure.   The CBIO_DEV_ID is a pointer to a CBIO_DEV structure.  
The CBIO_DEV is defined in the WRS private cbioLibP.h header file.
Every instance of a CBIO device has its own CBIO_DEV structure.
Every CBIO module has code for creating a CBIO device.  This code
returns a pointer to the CBIO_DEV structure, ie a CBIO_DEV_ID handle.

A CBIO_DEV structure contains the following members:

VxWorks objects

.CS
    HANDLE	cbioHandle;
.CE

This is used for handle verification of the CBIO device.  
See also handleLibP.h.


.CS
    SEMAPHORE	cbioMutex;		
.CE

This is the semaphore provided by semMLib which will be used 
provide mutual exclusion for the CBIO device.  


Functions

.CS
    CBIO_FUNCS * pFuncs;
.CE

This member of the CBIO_DEV structure is a pointer to a 
CBIO_FUNCS structure.  The CBIO_FUNCS structure is also
a private data structure.   The CBIO_FUNCS structure
contains the following members:


.CS
    STATUS	(* cbioDevBlkRW)
.CE

Address of the block (sector) read and write routine for the CBIO device.


.CS
    STATUS	(* cbioDevBytesRW)	
.CE

Address of the byte oriented read and write routine for the CBIO device.


.CS
    STATUS	(* cbioDevBlkCopy)	
.CE

Address of the block to block copy routine for the CBIO device.


.CS
    STATUS	(* cbioDevIoctl)		
.CE

Address of the ioctl() routine for the CBIO device.

One CBIO_FUNCS structure is typically statically allocated
for each CBIO module.  Refer to ramDiskCbio.c for an example.


Attributes

.CS
    char *	cbioDesc;	
.CE

This member of the CBIO_DEV structure is a pointer to a 
printable descriptive character string for the CBIO device.


.CS
    short	cbioMode;		
.CE

Indicates the access permissions to the CBIO device. 
Valid modes are O_RDONLY, O_WRONLY, and O_RDWR.


.CS
    BOOL	readyChanged;
.CE

TRUE if the device ready status has changed, and the device 
needs to be remounted (disk insertion).  FALSE if the device 
ready status has not changed.


.CS
    CBIO_PARAMS     cbioParams;
.CE

The next CBIO_DEV member is a CBIO_PARAMS (cbioLib.h) structure.   
This contains physical information about the device.  The members 
of the CBIO_PARAMS structure are:

.CS
    BOOL	cbioParams.cbioRemovable;		
.CE

TRUE if the device is removable, FALSE if the device is not removable.


.CS
    block_t	cbioParams.nBlocks;		
.CE

Total number of blocks on this CBIO device


.CS
    size_t	bytesPerBlk;	
.CE

The number of bytes in a block.  512 is a typical value.


.CS
    size_t	blockOffset;		
.CE

The number of blocks offset from block zero on the subordinate device. 


.CS
    short	cbioParams.blocksPerTrack;	
.CE

The number of blocks per track on this device.


.CS
    short	nHeads;		
.CE

The number of heads on this device.


.CS
    short	retryCnt;	
.CE

A counter indicating the total number of retries that
have occurred since creating this device.


.CS
    block_t	cbioParams.lastErrBlk;	
.CE

The block number of the last error that occurred.


.CS
    int		lastErrno;		
.CE

The error code of the last encountered error.

Implementation defined CBIO_DEV attributes

.CS
    caddr_t	cbioMemBase;	
    size_t	cbioMemSize;		
.CE

These represent the base address and size of any memory 
pool supplied to the CBIO module.

.CS
    u_long	cbioPriv0;		/@ Implementation defined @/
    u_long	cbioPriv1;		/@ Implementation defined @/
    u_long	cbioPriv2;		/@ Implementation defined @/
    u_long	cbioPriv3;		/@ Implementation defined @/
    u_long	cbioPriv4;		/@ Implementation defined @/
    u_long	cbioPriv5;		/@ Implementation defined @/
    u_long	cbioPriv6;		/@ Implementation defined @/
    u_long	cbioPriv7;		/@ Implementation defined @/
    CBIO_DEV_EXTRA * pDc ;		/@ Implementation defined structure @/
.CE

These fields are implementation defined.


Some developers may wish to create their own modules using the CBIO API.  
The guidelines herein should be followed regardless of CBIO module type
being designed.  The source file ramDiskCbio.c may be used as an 
implementation example.

CBIO modules have a CBIO device creation routine.   The routine 
creates a CBIO_DEV structure and initializes all its fields.  
Un-used function members are initialized to NULL.  CBIO modules 
do not perform an iosDevAdd() call, and are akin to block device 
drivers in that respect.  The creation routine returns a CBIO 
handle of type CBIO_DEV_ID.

CBIO modules have a block transfer routine. This routine transfers 
between a user buffer and the lower layer (hardware, subordinate 
CBIO, or BLK_DEV).  It is optimized for block transfers.  This 
routine returns OK or ERROR and may otherwise set errno.

This routine is declared:

.CS
STATUS	xxCbioBlkRW		/@ Read/Write blocks @/
    (
    CBIO_DEV_ID     dev,
    block_t         startBlock,
    block_t         numBlocks,
    addr_t          buffer,
    CBIO_RW         rw,
    cookie_t *      pCookie
    );
.CE

.IP
dev - the CBIO handle of the device being accessed (from creation routine)
.IP
startBlock - the starting block of the transfer operation
.IP
numBlocks - the total number of blocks to transfer
.IP
buffer - address of the memory buffer used for the transfer
.IP
rw - indicates the direction of transfer up or down (READ/WRITE)
.IP
*pCookie - pointer to cookie used by upper layer such as dosFsLib(),
it should be preserved.
.LP


CBIO modules have a byte transfer routine. This routine transfers 
between a user buffer and the lower layer (hardware, subordinate CBIO,
or BLK_DEV).  It is optimized for byte transfers.  This routine returns
OK or ERROR and may otherwise set errno.

This routine is declared:

.CS
LOCAL STATUS xxCbioBytesRW        /@ Read/Write bytes @/
    (
    CBIO_DEV_ID 	dev,
    block_t		startBlock,
    off_t		offset,
    addr_t		buffer,
    size_t		nBytes,
    CBIO_RW             rw,
    cookie_t *          pCookie
    );
.CE

.IP
dev - the CBIO handle of the device being accessed (from creation routine)
.IP
startBlock - the starting block of the transfer operation
.IP
offset - offset in bytes from the beginning of the starting block
.IP
buffer - address of the memory buffer used for the transfer
.IP
nBytes - number of bytes to transfer
.IP
rw - indicates the direction of transfer up or down (READ/WRITE)
.IP
*pCookie - pointer to cookie used by upper layer such as dosFsLib(),
it should be preserved.
.LP


CBIO modules have a block to block copy routine. This makes
copies of one or more blocks on the lower layer (hardware, 
subordinate CBIO, or BLK_DEV).   It is optimized for block
copies on the subordinate layer.  This routine returns
OK or ERROR and may otherwise set errno.

This routine is declared:

.CS
LOCAL STATUS xxCbioBlkCopy  /@ Copy sectors @/
    (
    CBIO_DEV_ID        dev,
    block_t         srcBlock,
    block_t         dstBlock,
    block_t         numBlocks
    );
.CE

.IP
dev - the CBIO handle of the device being accessed (from creation routine)
.IP
srcBlock - source start block of the copy
.IP
dstBlock - destination start block of the copy
.IP
num_block - number of blocks to copy
.LP


CBIO modules have an ioctl() routine. This performs the requested
ioctl() operation.

CBIO modules can expect the following ioctl() codes from cbioLib.h:

.IP
CBIO_RESET - reset the CBIO device.   When the third argument to the 
CBIO_RESET ioctl is NULL, the code verifies that the disk is inserted 
and is ready, after getting it to a known state.   When the 3rd argument 
is a non-zero, it is assumed to be a BLK_DEV pointer and CBIO_RESET will 
install a new subordinate block device.    This work is performed at the 
BLK_DEV to CBIO layer, and all layers shall account for it.  A CBIO_RESET i
indicates a possible change in device geometry, and the CBIO_PARAMS members 
will be reinitialized after a CBIO_RESET.
.IP
CBIO_STATUS_CHK - check device status of CBIO device and lower layer
.IP
CBIO_DEVICE_LOCK - Prevent disk removal 
.IP
CBIO_DEVICE_UNLOCK - Allow disk removal
.IP
CBIO_DEVICE_EJECT - Unmount and eject device
.IP
CBIO_CACHE_FLUSH - Flush any dirty cached data
.IP
CBIO_CACHE_INVAL - Flush & Invalidate all cached data
.IP
CBIO_CACHE_NEWBLK - Allocate scratch block
.LP

The CBIO module may also implement other codes.  CBIO modules pass all 
ioctl() requests to the lower layer also passing the argument before 
performing the requested I/O operation.  This rule ensures changes in 
device geometry (if any occur) etc. are reflected in the upper layer.  
This routine returns OK or ERROR and may otherwise set errno.

This routine is declared:

.CS
LOCAL STATUS xxCbioIoctl        /@ Misc control operations @/
    (
    CBIO_DEV_ID         dev,
    UINT32              command,
    addr_t              arg
    );
.CE

.IP
dev - the CBIO handle of the device being accessed (from creation routine)
.IP
command - ioctl() command being issued
.IP
arg - specific to the particular ioctl() function requested or un-used.
.LP


Need to add WV instrumentation.
Need to beef up docs
Need to add cbioDevDelete routine.
*/

/* includes */
#include "vxWorks.h"
#include "private/dosFsVerP.h"
#include "stdlib.h"
#include "stdio.h"
#include "semLib.h"
#include "ioLib.h"
#include "errno.h"
#include "string.h"
#include "tickLib.h"
#include "sysLib.h"
#include "taskLib.h"
#include "logLib.h"
#include "blkIo.h"
#include "assert.h"
#include "dosFsLib.h"
#include "private/cbioLibP.h"

/* defines */
#undef DEBUG 		/* define to debug cbioLib */

#ifdef	DEBUG
#undef	NDEBUG
BOOL			cbioDebug = FALSE ;
#else
#define	NDEBUG
#endif	/* DEBUG */

/* implementation defined fields for Wrapper use */
#define	cbioBlkShift	cbioPriv0
#define	cbioDirtyMask	cbioPriv1
#define	cbioBlkNo	cbioPriv2

/* Globals & Statics */

LOCAL BOOL		cbioInstalled ;
LOCAL CBIO_DEV_ID	cbioRecentDev = NULL ;

#ifndef _WRS_DOSFS2_VXWORKS_AE
OBJ_CLASS		cbioClass ;
CLASS_ID		cbioClassId = &cbioClass ;
#endif /* _WRS_DOSFS2_VXWORKS_AE */

int 	cbioMutexSemOptions = 	
		(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE);

/* forward declarations */

STATUS cbioShow (CBIO_DEV_ID dev);
int cbioRdyChk (CBIO_DEV_ID dev);

LOCAL STATUS cbioWrapOk ();

LOCAL STATUS blkWrapBytesRW( CBIO_DEV_ID dev, block_t startBlock,
	off_t offset, addr_t buffer, size_t nBytes,
	CBIO_RW rw, cookie_t *pCookie);

LOCAL STATUS blkWrapBlkRW( CBIO_DEV_ID dev, block_t startBlock,
	block_t numBlocks, addr_t buffer, CBIO_RW rw, cookie_t * pCookie);

LOCAL STATUS blkWrapBlkRWbuf( CBIO_DEV_ID dev, block_t startBlock,
	block_t numBlocks, addr_t buffer, CBIO_RW rw, cookie_t * pCookie);

LOCAL STATUS blkWrapBlkCopy( CBIO_DEV_ID dev, block_t srcBlock,
	block_t dstBlock, block_t numBlocks);

LOCAL STATUS blkWrapIoctl( CBIO_DEV_ID dev, UINT32  command, addr_t arg);

LOCAL int shiftCalc (u_long mask);


/* CBIO_FUNCS, one per CBIO module */

LOCAL CBIO_FUNCS cbioFuncs = {(FUNCPTR) blkWrapBlkRW,
			      (FUNCPTR) blkWrapBytesRW,
			      (FUNCPTR) blkWrapBlkCopy,
			      (FUNCPTR) blkWrapIoctl};

#ifdef	__unused__
LOCAL void blkWrapDiskSizeAdjust ( CBIO_DEV_ID dev);
#endif	/*__unused__*/

/*******************************************************************************
*
* cbioLibInit - Initialize CBIO Library
*
* This function initializes the CBIO library, and will be called
* when the first CBIO device is created, hence it does not need to
* be called during system initialization.  It can be called mulitple
* times, but will do nothing after the first call.
*
* RETURNS: OK or ERROR 
*
*/
STATUS cbioLibInit( void )
    {
    STATUS stat;

    if( cbioInstalled )
	return OK;

#ifdef _WRS_DOSFS2_VXWORKS_AE

    /* Setup handle show routine. */

    stat = handleShowConnect( handleTypeCbioHdl, (FUNCPTR) cbioShow );

#else /* else, using VxWorks 5.x */

    /* Allocate object space for CBIO_DEV and CBIO_FUNCS */

    stat = classInit( cbioClassId,
                sizeof(CBIO_DEV),
                OFFSET( CBIO_DEV,objCore),
                (FUNCPTR) cbioDevCreate,        /* Create Routine */
                (FUNCPTR) NULL,         /* Init routine */
                (FUNCPTR) NULL                  /* Destroy routine */
                ) ;

    classShowConnect( cbioClassId, (FUNCPTR) cbioShow );

#endif /* _WRS_DOSFS2_VXWORKS_AE */

    if (ERROR == stat)
	return (ERROR);

    if( stat == OK )
	cbioInstalled = TRUE ;

    return( stat );
    }



/*******************************************************************************
*
* cbioBlkRW - transfer blocks to or from memory
* 
* This routine verifies the CBIO device is valid and if so calls the devices 
* block transfer routine.  The CBIO device performs block transfers 
* between the device and memory.  
*
* If the CBIO_DEV_ID passed to this routine is not a valid CBIO handle,
* ERROR will be returned with errno set to S_cbioLib_INVALID_CBIO_DEV_ID
*
* RETURNS OK if sucessful or ERROR if the handle is invalid, or if the
* CBIO device routine returns ERROR.
*
*/

STATUS cbioBlkRW		
    (
    CBIO_DEV_ID		dev, 		/* CBIO handle */
    block_t		startBlock,	/* starting block of transfer */
    block_t		numBlocks, 	/* number of blocks to transfer */
    addr_t		buffer,		/* address of the memory buffer */
    CBIO_RW		rw,		/* direction of transfer R/W */
    cookie_t *	 	pCookie		/* pointer to cookie */
    )
    {
    if( OK != cbioDevVerify (dev))
	{
	return ERROR;
	}

    return (dev->pFuncs->cbioDevBlkRW (dev,startBlock,
                                     numBlocks,buffer, 
                                     rw, pCookie));
    }

/*******************************************************************************
*
* cbioBytesRW - transfer bytes to or from memory
* 
* This routine verifies the CBIO device is valid and if so calls the devices
* byte tranfer routine which transfers between a user buffer and the lower 
* layer (hardware, subordinate CBIO, or BLK_DEV).  It is optimized for byte 
* transfers.  
* 
* If the CBIO_DEV_ID passed to this routine is not a valid CBIO handle,
* ERROR will be returned with errno set to S_cbioLib_INVALID_CBIO_DEV_ID
*
* RETURNS OK if sucessful or ERROR if the handle is invalid, or if the
* CBIO device routine returns ERROR.
*
*/
STATUS cbioBytesRW	
    (
    CBIO_DEV_ID 	dev,		/* CBIO handle */
    block_t		startBlock,	/* starting block of the transfer */
    off_t		offset,		/* offset into block in bytes */
    addr_t		buffer,		/* address of data buffer */
    size_t		nBytes,		/* number of bytes to transfer */
    CBIO_RW		rw,		/* direction of transfer R/W */
    cookie_t *		pCookie		/* pointer to cookie */
    )
    {
    if( OK != cbioDevVerify (dev))
	{
	return ERROR;
	}

    return (dev->pFuncs->cbioDevBytesRW(dev, startBlock,
				      offset, buffer,
			  	      nBytes, rw, pCookie));
    }


/*******************************************************************************
*
* cbioBlkCopy - block to block (sector to sector) tranfer routine
*
* This routine verifies the CBIO device is valid and if so calls the devices
* block to block tranfer routine which makes copies of one or more blocks on 
* the lower layer (hardware, subordinate CBIO, or BLK_DEV).   It is optimized 
* for block to block copies on the subordinate layer.  
* 
* If the CBIO_DEV_ID passed to this routine is not a valid CBIO handle,
* ERROR will be returned with errno set to S_cbioLib_INVALID_CBIO_DEV_ID
*
* RETURNS OK if sucessful or ERROR if the handle is invalid, or if the
* CBIO device routine returns ERROR.
*
*/
STATUS cbioBlkCopy
    (
    CBIO_DEV_ID 	dev,		/* CBIO handle */
    block_t		srcBlock,	/* source start block */
    block_t		dstBlock, 	/* destination start block */
    block_t		numBlocks	/* number of blocks to copy */
    )
    {
    if( OK != cbioDevVerify (dev))
	{
	return ERROR;
	}

    return (dev->pFuncs->cbioDevBlkCopy(dev, srcBlock,
 				      dstBlock, numBlocks));
    }


/*******************************************************************************
*
* cbioIoctl - perform ioctl operation on device
*
* This routine verifies the CBIO device is valid and if so calls the devices
* I/O control operation routine.
* 
* CBIO modules expect the following ioctl() codes:
* 
* .IP
* CBIO_RESET - reset the CBIO device.   When the third argument to the ioctl
* call accompaning CBIO_RESET is NULL, the code verifies that the disk is 
* inserted and is ready, after getting it to a known state.   When the 3rd 
* argument is a non-zero, it is assumed to be a BLK_DEV pointer and 
* CBIO_RESET will install a new subordinate block device.    This work
* is performed at the BLK_DEV to CBIO layer, and all layers shall account
* for it.  A CBIO_RESET indicates a possible change in device geometry, 
* and the CBIO_PARAMS members will be reinitialized after a CBIO_RESET.
* .IP
* CBIO_STATUS_CHK - check device status of CBIO device and lower layer
* .IP
* CBIO_DEVICE_LOCK - Prevent disk removal 
* .IP
* CBIO_DEVICE_UNLOCK - Allow disk removal
* .IP
* CBIO_DEVICE_EJECT - Unmount and eject device
* .IP
* CBIO_CACHE_FLUSH - Flush any dirty cached data
* .IP
* CBIO_CACHE_INVAL - Flush & Invalidate all cached data
* .IP
* CBIO_CACHE_NEWBLK - Allocate scratch block
* .LP
*
* If the CBIO_DEV_ID passed to this routine is not a valid CBIO handle,
* ERROR will be returned with errno set to S_cbioLib_INVALID_CBIO_DEV_ID
*
* RETURNS OK if sucessful or ERROR if the handle is invalid, or if the
* CBIO device routine returns ERROR.
*/

STATUS cbioIoctl
    (
    CBIO_DEV_ID		dev,		/* CBIO handle */
    int			command,	/* ioctl command to be issued */
    addr_t		arg		/* arg - specific to ioctl */
    )
    {
    if( OK != cbioDevVerify (dev))
	{
	return ERROR;
	}

    return (dev->pFuncs->cbioDevIoctl(dev, command, arg));
    }


/*******************************************************************************
*
* cbioModeGet - return the mode setting for CBIO device
*
* If the CBIO_DEV_ID passed to this routine is not a valid CBIO handle,
* ERROR will be returned with errno set to S_cbioLib_INVALID_CBIO_DEV_ID
* This routine is not protected by a semaphore.
*
* This routine confirms if the current layer is a CBIO to BLKDEV wrapper 
* or a CBIO to CBIO layer. Depending on the current layer it either 
* returns the mode from BLK_DEV or calls cbioModeGet() recursively.
*
* RETURNS  O_RDONLY, O_WRONLY, or O_RDWR or ERROR 
*/
int cbioModeGet
    (
    CBIO_DEV_ID dev	/* CBIO handle */
    )
    {
    BLK_DEV *pBd;

    if( OK != cbioDevVerify (dev))
	{
	return ERROR;
	}

    /*
     * SPR#67729: isDriver is TRUE for a CBIO to BLKDEV layer and ramDiskCbio.
     * In case of ramDiskCbio the blkSubDev is set to NULL since it is a CBIO 
     * layer. There should be some reference to the sub devices, either 
     * blkSubDev or cbioSubDev. 
     */

    if((dev->blkSubDev != NULL) || (dev->cbioSubDev != NULL))       
    	{                      
	if( dev->isDriver == TRUE) /* Boolean identifying the current layer */
	    {
            if(dev->blkSubDev == NULL) 
	        {
		return(CBIO_MODE (dev)); /* For ramDiskCbio */
	        }
	    else
		{
	        pBd = dev->blkSubDev;
	        return (pBd->bd_mode);
		}
	    }    
	else
	    return ( cbioModeGet(dev->cbioSubDev)); /* For CBIO to CBIO */	
	}
    
    return (ERROR);
    }

/*******************************************************************************
*
* cbioModeSet - set mode for CBIO device
* 
* Valid modes are O_RDONLY, O_WRONLY, or O_RDWR.
*
* If the CBIO_DEV_ID passed to this routine is not a valid CBIO handle,
* ERROR will be returned with errno set to S_cbioLib_INVALID_CBIO_DEV_ID
* This routine is not protected by a semaphore.
* 
* This routine confirms if the current layer is a CBIO to BLKDEV wrapper 
* or a CBIO to CBIO layer. Depending on the current layer it either 
* sets the mode of the BLK_DEV or calls cbioModeSet() recursively.
*
* RETURNS  OK or ERROR if mode is not set.
*
*/
STATUS cbioModeSet
    (
    CBIO_DEV_ID dev, 	/* CBIO handle */
    int mode		/* O_RDONLY, O_WRONLY, or O_RDWR */
    )
    {
    BLK_DEV *pBd;

    if( OK != cbioDevVerify (dev))
	{
	return ERROR;
	}

    /*
     * SPR#67729: isDriver is TRUE for a CBIO to BLKDEV layer and ramDiskCbio.
     * In case of ramDiskCbio the blkSubDev is set to NULL since it is a CBIO
     * layer. There should be some reference to the sub devices, either 
     * blkSubDev or cbioSubDev.
     */

    if((dev->blkSubDev != NULL) || (dev->cbioSubDev != NULL))
	{
        if(dev->isDriver == TRUE)  /* Boolean identifying the current layer */ 
	    {
	    if(dev->blkSubDev == NULL)
	        {
		CBIO_MODE(dev) = mode; /* For ramDiskCbio */
	        return (OK);
	        }
	    else
		{
	        pBd = dev->blkSubDev;
	        pBd->bd_mode = mode;
                CBIO_MODE (dev) = mode;
	        return (OK);
		}
	    }
	else			      
            return (cbioModeSet (dev->cbioSubDev, mode)); /* For CBIO to CBIO */
	}

    return (ERROR);
    }

/*******************************************************************************
*
* cbioRdyChgdGet - determine ready status of CBIO device
*
* For example 
* .CS
*    switch (cbioRdyChgdGet (cbioDeviceId)) 
*        {
*        case TRUE:
*            printf ("Disk changed.\n");
*            break;
*        case FALSE:
*            printf ("Disk has not changed.\n");
*            break;
*        case ERROR:
*            printf ("Not a valid CBIO device.\n");
*            break;
*        default:
*        break;
*        }
* .CE
*
* If the CBIO_DEV_ID passed to this routine is not a valid CBIO handle,
* ERROR will be returned with errno set to S_cbioLib_INVALID_CBIO_DEV_ID
* This routine is not protected by a semaphore.
*
* This routine will check down to the driver layer to see if any lower
* layer has its ready changed bit set to TRUE.  If so, this routine returns
* TRUE.  If no lower layer has its ready changed bit set to TRUE, this layer
* returns FALSE.
*
* RETURNS  TRUE if device ready status has changed, else FALSE if 
* the ready status has not changed, else ERROR if the CBIO_DEV_ID is invalid.
*
*/
int cbioRdyChgdGet
    (
    CBIO_DEV_ID dev		/* CBIO handle */
    )
    {
    if( OK != cbioDevVerify (dev))
	{
	return (ERROR);
	}

    /* check the current layers (dev) ready changed setting */

    if (TRUE == CBIO_READYCHANGED(dev))
        {
        /* this CBIO device's ready changed bit is set, return TRUE */

        return (TRUE);
        }

    /*
     * isDriver is TRUE for a CBIO to BLKDEV layer and ramDiskCbio.
     * In case of ramDiskCbio the blkSubDev is set to NULL since it is a CBIO
     * layer. There should be some reference to the sub devices, either
     * blkSubDev or cbioSubDev.
     */

    if((dev->blkSubDev != NULL) || (dev->cbioSubDev != NULL))
        {
        if(TRUE == dev->isDriver) /* Boolean identifying the current layer */
            {
            if(NULL == dev->blkSubDev)
                {
		/* 
		 * This is a CBIO driver (no subordinate CBIO present).
		 * Above, we have already checked for the 'dev' ready 
		 * changed bit being TRUE, so we may return FALSE.
		 */

                return (FALSE); /* For ramDiskCbio */
                }
            else
                {
		/* 
		 * This is a CBIO to BLK_DEV layer, so there is a BLK_DEV 
		 * device below this CBIO device.  Return TRUE or FALSE 
		 * depending upon the BLK_DEV's ready changed bit setting.
		 */

                return (((BLK_DEV *)(dev->blkSubDev))->bd_readyChanged);
                        
                }
            }
        else
            {
            /* 
	     * This is a CBIO to CBIO layer,  and it doesn't have
	     * its ready changed bit set, however a layer below us
	     * may have its bit set, so we recurse down a layer. 
	     */

            return (cbioRdyChgdGet(dev->cbioSubDev)); 
            }
        }

    /* should never get here */
#ifdef DEBUG
        logMsg("cbioRdyChgdGet: bad device 0x%08lx\n",
		(long unsigned int)dev,0,0,0,0,0);
#endif

    return (ERROR);
    }

/*******************************************************************************
*
* cbioRdyChgdSet - force a change in ready status of CBIO device
*
* Pass TRUE in status to force READY status change.
*
* If the CBIO_DEV_ID passed to this routine is not a valid CBIO handle,
* ERROR will be returned with errno set to S_cbioLib_INVALID_CBIO_DEV_ID
* If status is not passed as TRUE or FALSE, ERROR is returned.
* This routine is not protected by a semaphore.
*
* This routine sets readyChanged bit of passed CBIO_DEV.
*
* RETURNS  OK or ERROR if the device is invalid or status is not TRUE or FALSE.
*
*/
STATUS cbioRdyChgdSet
    (
    CBIO_DEV_ID dev,		/* CBIO handle */
    BOOL status			/* TRUE/FALSE */
    )
    {
    if((OK != cbioDevVerify (dev)) || (TRUE != status && FALSE != status))
	{
	return ERROR;
	}

    CBIO_READYCHANGED(dev) = status; /* For ramDiskCbio */

    return (OK);
    }

/*******************************************************************************
*
* cbioLock - obtain CBIO device semaphore.
*
* If the CBIO_DEV_ID passed to this routine is not a valid CBIO handle,
* ERROR will be returned with errno set to S_cbioLib_INVALID_CBIO_DEV_ID
*
* RETURNS: OK or ERROR if the CBIO handle is invalid or semTake fails.
*/
STATUS cbioLock
    (
    CBIO_DEV_ID	dev,		/* CBIO handle */
    int		timeout		/* timeout in ticks */
    )
    {
    if( OK != cbioDevVerify (dev))
	{
	return ERROR;
	}

    return (semTake(dev->cbioMutex, timeout));
    } 
    
/*******************************************************************************
*
* cbioUnlock - release CBIO device semaphore.
*
* If the CBIO_DEV_ID passed to this routine is not a valid CBIO handle,
* ERROR will be returned with errno set to S_cbioLib_INVALID_CBIO_DEV_ID
*
* RETURNS: OK or ERROR if the CBIO handle is invalid or the semGive fails.
*/
STATUS cbioUnlock
    (
    CBIO_DEV_ID	dev 		/* CBIO handle */
    )
    {

    if( OK != cbioDevVerify (dev))
	{
	return ERROR;
	}

    return (semGive(dev->cbioMutex));
    }


/*******************************************************************************
*
* cbioParamsGet - fill in CBIO_PARAMS structure with CBIO device parameters
*
* If the CBIO_DEV_ID passed to this routine is not a valid CBIO handle,
* ERROR will be returned with errno set to S_cbioLib_INVALID_CBIO_DEV_ID
*
* RETURNS: OK or ERROR if the CBIO handle is invalid.
*/
STATUS cbioParamsGet
    (
    CBIO_DEV_ID	dev,		/* CBIO handle */
    CBIO_PARAMS * pCbioParams	/* pointer to CBIO_PARAMS */
    )
    {
    if( OK != cbioDevVerify (dev))
	{
	return ERROR;
	}

    *pCbioParams = dev->cbioParams;

    return (OK);
    }

/*******************************************************************************
*
* cbioRdyChk - print ready changed setting of CBIO layer and its subordinates
*
* NOMANUAL
*/
int cbioRdyChk
    (
    CBIO_DEV_ID dev             /* CBIO handle */
    )
    {
    if( OK != cbioDevVerify (dev))
        {
        printf ("\tCBIO_DEV_ID 0x%08lx is invalid per cbioDevVerify.\n", 
                (unsigned long) dev);

        return (ERROR);
        }

    /* check the current layers (dev) readyChanged bit setting */

    printf ("\tCBIO_DEV_ID 0x%08lx ready changed is %d.\n", 
                (unsigned long) dev, CBIO_READYCHANGED (dev));
    /*
     * isDriver is TRUE for a CBIO to BLKDEV layer and ramDiskCbio.
     * In case of ramDiskCbio the blkSubDev is set to NULL since it is a CBIO
     * layer. There should be some reference to the sub devices, either
     * blkSubDev or cbioSubDev.
     */

    if((dev->blkSubDev != NULL) || (dev->cbioSubDev != NULL))
        {
        if(TRUE == dev->isDriver) /* Boolean identifying the current layer */
            {
            if(NULL == dev->blkSubDev)
                {
                /*
                 * This is a CBIO driver (no subordinate CBIO present).
                 * Above, we have already displayed the 'dev' ready
                 * changed bit,  so we may return.
                 */

                return (OK); /* For ramDiskCbio */
                }
            else
                {
                /*
                 * This is a CBIO to BLK_DEV layer, so there is a BLK_DEV
                 * device below this CBIO device.  Return TRUE or FALSE,
                 * depending upon the BLK_DEV's ready changed bit setting.
                 */
                printf ("\tBLK_DEV ptr 0x%08lx ready changed is %d.\n", 
                            (unsigned long) dev->blkSubDev,  
                            dev->blkSubDev->bd_readyChanged );

                return (OK);
                }
            }
        else
            {
            /*
             * This is a CBIO to CBIO layer,  and it doesn't have
             * its ready changed bit set, however a layer below us
             * may have its bit set, so we recurse down a layer.
             */

            return (cbioRdyChk (dev->cbioSubDev));
            }
        }

    /* should never get here */
#ifdef DEBUG
        logMsg("cbioRdyChk: bad device 0x%08lx\n",
                (long unsigned int)dev,0,0,0,0,0);
#endif

    return (ERROR);
    }

/*******************************************************************************
*
* cbioShow - print information about a CBIO device
*
* This function will display on standard output all information which
* is generic for all CBIO devices.
* See the CBIO modules particular device show routines for displaying
* implementation-specific information.
*
* It takes two arguments:
*
* A CBIO_DEV_ID which is the CBIO handle to display or NULL for 
* the most recent device.
*
* RETURNS OK or ERROR if no valid CBIO_DEV is found.
*
* SEE ALSO: dcacheShow(), dpartShow()
*
*/
STATUS cbioShow
    (
    CBIO_DEV_ID dev		/* CBIO handle */
    )
    {
    unsigned long size, factor = 1 ;
    char * units ;

    if( dev == NULL )
	dev = cbioRecentDev;

    if( dev == NULL )
	{
	errno = S_cbioLib_INVALID_CBIO_DEV_ID;
	return ERROR;
	}

    if( OK != cbioDevVerify (dev))
	{
	return ERROR;
	}

    if( dev->cbioParams.bytesPerBlk == 0 )
	{
	errno = EINVAL;
	return ERROR;
	}

    printf("Cached Block I/O Device, handle=%#lx\n", (u_long)dev);

    printf("\tDescription: %s\n", dev->cbioDesc);

    /* calculate disk size, while avoiding 64-bit arithmetic */

    if( dev->cbioParams.bytesPerBlk < 1024 )
	{
	factor = 1024 / dev->cbioParams.bytesPerBlk ;
	size = dev->cbioParams.nBlocks / factor ;
	units = "Kbytes" ;
	}
    else /* if( dev->cbioParams.bytesPerBlk >= 1024 ) */
	{
	factor = dev->cbioParams.bytesPerBlk / 1024 ;
	size = (dev->cbioParams.nBlocks / 1024) * factor ;
	units = "Mbytes" ;
	}

    if(( size > 10000 ) && (0 == strcmp (units,"Kbytes")))
	{
	units = "Mbytes" ;
	size /= 1024 ;
	}

    if(( size > 10000 ) && (0 == strcmp(units, "Mbytes")))
	{
        units = "Gbytes" ;
	size /= 1024 ;
	}

    printf("\tDisk size %ld %s, RAM Size %d bytes\n",
	size, units, (int) dev->cbioMemSize );

    printf("\tBlock size %d, heads %d, blocks/track %d, # of blocks %ld\n",
	(int) dev->cbioParams.bytesPerBlk, 
        dev->cbioParams.nHeads, dev->cbioParams.blocksPerTrack,
	(block_t) dev->cbioParams.nBlocks );

    printf("\tpartition offset %ld blocks, type %s, Media changed %s\n",
	(block_t) dev->cbioParams.blockOffset, 
	(dev->cbioParams.cbioRemovable) ? "Removable" : "Fixed",
	((TRUE == CBIO_READYCHANGED(dev)) ? "Yes" : "No" ));

    if( dev->cbioParams.lastErrno != 0)
	printf("\tLast error errno=%x, block=%ld\n",
	    dev->cbioParams.lastErrno, dev->cbioParams.lastErrBlk );

    printf ("\n   Current ready changed bit"
            " for this layer and all subordinates:\n");

    cbioRdyChk (dev);

    /* TBD: embedded objects */
    return (OK);
    }

/*******************************************************************************
*
* cbioDevVerify - verify CBIO_DEV_ID 
*
* The purpose of this function is to determine if the device complies with the 
* CBIO interface.  It can be used to verify a CBIO handle before it is passed 
* to dosFsLib, rawFsLib, usrFdiskPartLib, or other CBIO modules which expect a 
* valid CBIO interface.  
*
* The device handle provided to this function, <device> is verified to be a 
* CBIO device.  If <device> is not a CBIO device ERROR is returned with errno 
* set to S_cbioLib_INVALID_CBIO_DEV_ID
*
* The dcacheCbio and dpartCbio CBIO modules (and dosFsLib) use this function 
* internally, and therefore this function need not be otherwise invoked when 
* using compliant CBIO modules.
*
* RETURNS: OK or ERROR if not a CBIO device, if passed a NULL address, or 
* if the check could cause an unaligned access. 
*
* SEE ALSO: dosFsLib, dcacheCbio, dpartCbio
*/
STATUS cbioDevVerify
    (
    CBIO_DEV_ID device		/* CBIO_DEV_ID to be verified */
    )
    {
    /* SPR#71720, avoid NULL access */ 

    if (NULL == device)
	{
        errno = S_cbioLib_INVALID_CBIO_DEV_ID;
	return(ERROR);
	}

    /* SPR#71720, avoid causing unaligned pointer access. */

    if (FALSE == (_WRS_ALIGN_CHECK(device,CBIO_DEV)))
	{
        errno = S_cbioLib_INVALID_CBIO_DEV_ID;
	return(ERROR);
	}

#ifdef _WRS_DOSFS2_VXWORKS_AE
    /* if the input is a valid CBIO device, return it */
    if( OK == HANDLE_VERIFY( &device->cbioHandle, handleTypeCbioHdl ))
	{
	return( OK );
	}
#else
    if(OK == OBJ_VERIFY( device, cbioClassId ) )
	{
	return (OK);
	}
#endif /* _WRS_DOSFS2_VXWORKS_AE */
    else 
	{
#ifdef DEBUG
        logMsg("cbioDevVerify: device 0x%08lx is not a CBIO pointer\n",
		(long unsigned int)device,0,0,0,0,0);
#endif
        errno = S_cbioLib_INVALID_CBIO_DEV_ID;
	return (ERROR);
	}
    }

/*******************************************************************************
*
* cbioWrapBlkDev - create CBIO wrapper atop a BLK_DEV device
*
* The purpose of this function is to make a blkIo (BLK_DEV) device comply 
* with the CBIO interface via a wrapper.  
*
* The device handle provided to this function, <device> is verified to 
* be a blkIo device.  A lean CBIO to BLK_DEV wrapper is then created for 
* a valid blkIo device.  The returned CBIO_DEV_ID device handle may be 
* used with dosFsDevCreate(), dcacheDevCreate(), and any other routine 
* expecting a valid CBIO_DEV_ID handle.
*
* To verify a blkIo pointer we see that all mandatory functions
* are not NULL.
*
* Note that if a valid CBIO_DEV_ID is passed to this function, it will 
* simply be returned without modification.
*
* The dosFsLib, dcacheCbio, and dpartCbio CBIO modules use this function 
* internally, and therefore this function need not be otherwise invoked 
* when using those CBIO modules.
*
* RETURNS: a CBIO device pointer, or NULL if not a blkIo device
*
* SEE ALSO: dosFsLib, dcacheCbio, dpartCbio
*
*/
CBIO_DEV_ID cbioWrapBlkDev
    (
    BLK_DEV * pDevice	/* BLK_DEV * device pointer */
    )
    {
    CBIO_DEV * pDev = NULL ;
    BLK_DEV * pBd = pDevice ;
    u_char shift ;

    cbioLibInit();	/* just in case */

    if (NULL == pDevice)
	{
	goto bad_blkIo ;
	}

    /* simply return any valid CBIO_DEV_ID as is */

    if (OK == cbioDevVerify((CBIO_DEV_ID)pDevice))
	{
	return ((CBIO_DEV_ID) pDevice);
	}

    /* blkIo has no clear mechanism for verification, improvise (HELP)*/

    if( (pBd->bd_blkRd == NULL) || (pBd->bd_blkWrt == NULL) ||
	(pBd->bd_ioctl == NULL) ) 
	goto bad_blkIo ;

    shift = shiftCalc( pBd->bd_bytesPerBlk );

    /* we place a dummy function if statusChk is not provided */

    if( pBd->bd_statusChk == NULL )
	pBd->bd_statusChk = cbioWrapOk ;

    /* create our CBIO handle, no cache memory (yet) */

    pDev = (CBIO_DEV_ID) cbioDevCreate ( NULL, 0 );

    if( pDev == NULL )
	return( NULL );

    pDev->pDc				= pBd ;
    pDev->cbioDesc			= "CBIO to BLK_DEV Wrapper" ;
    pDev->cbioParams.cbioRemovable	= pBd->bd_removable ;
    pDev->readyChanged			= pBd->bd_readyChanged ;
    pDev->cbioParams.nBlocks		= pBd->bd_nBlocks ;
    pDev->cbioParams.bytesPerBlk	= pBd->bd_bytesPerBlk ;
    pDev->cbioParams.blocksPerTrack	= pBd->bd_blksPerTrack ;
    pDev->cbioParams.nHeads		= pBd->bd_nHeads ;
    pDev->cbioParams.blockOffset	= 0;
    pDev->cbioMode			= pBd->bd_mode ;
    pDev->cbioParams.lastErrBlk		= NONE ;
    pDev->cbioParams.lastErrno		= 0 ;
    pDev->cbioBlkNo			= NONE;
    pDev->cbioDirtyMask			= 0;
    pDev->cbioBlkShift			= shift ;
    pDev->pFuncs			= &cbioFuncs;

    /* SPR#67729: Fill in the members subDev and isDriver appropriately */

    pDev->blkSubDev                     = pBd; /* Pointer to lower BlkDev */
    pDev->cbioSubDev                    = NULL; /* Pointer to lower BlkDev */
    pDev->isDriver                      = TRUE;/* ==TRUE since this is a  */
                                              /* CBIO to BLKDEV layer */
    return (pDev) ;

bad_blkIo:

    logMsg("cbioWrapBlkDev: BLK_DEV pointer 0x%08lx appears invalid.\n", 
           (unsigned long) pDevice,0,0,0,0,0);

    return (NULL) ;
    }


/*******************************************************************************
*
* cbioDevCreate - Initialize a CBIO device (Generic)
*
* This routine will create an empty CBIO_DEV structure and
* return a handle to that structure (CBIO_DEV_ID).   
* 
* This routine is intended to be used by CBIO modules only.
* See cbioLibP.h
*
* RETURNS CBIO_DEV_ID or NULL if ERROR.
*/

CBIO_DEV_ID cbioDevCreate
    (
    caddr_t  	ramAddr, 	/* where it is in memory (0 = KHEAP_ALLOC) */
    size_t	ramSize		/* pool size */
    )
    {
    CBIO_DEV *	pDev = NULL ;
    caddr_t	pBase = NULL ;

    if( !cbioInstalled )
	if( cbioLibInit() == ERROR )
	     goto error;

    if (ramSize != 0)
	{
	if( ramAddr == NULL )
	    pBase = KHEAP_ALLOC(ramSize);
	else
	    pBase = ramAddr ;

	if( pBase == NULL )
	    goto error ;
	}

    /* allocate and initialize the device control structure */
#ifdef _WRS_DOSFS2_VXWORKS_AE
    pDev = (CBIO_DEV *) KHEAP_ALLOC((sizeof (CBIO_DEV)));
#else
    pDev = (CBIO_DEV *) objAlloc(cbioClassId);
#endif /* _WRS_DOSFS2_VXWORKS_AE */

    if( pDev == NULL )
	goto error;

    /* init Common fields */
    pDev->cbioMutex = semMCreate (cbioMutexSemOptions);

    if( NULL == pDev->cbioMutex )
	{
	goto error ;
	}

    pDev->readyChanged			= FALSE ;
    pDev->cbioParams.lastErrBlk		= NONE ;
    pDev->cbioParams.lastErrno		= 0 ;
    pDev->cbioMemBase			= pBase ;
    pDev->cbioMemSize			= ramSize ;

    /* pointer to method functions needs to be filled later */
    pDev->pFuncs = NULL ;

    /* init the handle */
#ifdef _WRS_DOSFS2_VXWORKS_AE
    handleInit (&pDev->cbioHandle, handleTypeCbioHdl);
#else
    /* make this object belong to its class */
    objCoreInit (&pDev->objCore, cbioClassId);
#endif /* _WRS_DOSFS2_VXWORKS_AE */

    cbioRecentDev = pDev ;

    /* return device handle */
    return( (CBIO_DEV_ID) pDev );

error:
    if( pBase != NULL )
	KHEAP_FREE(pBase);

    if( pDev != NULL )
	KHEAP_FREE((char *)pDev);

    return ( NULL );
    }


/*******************************************************************************
*
* shiftCalc - calculate how many shift bits
*
* How many shifts <N> are needed such that <mask> == 1 << <N>
* This is very useful for replacing multiplication with shifts,
* where it is known a priori that the multiplier is 2^k.
*
* RETURNS: Number of shifts.
*/

LOCAL int shiftCalc
    (
    u_long mask
    )
    {
    int i;

    for (i=0; i<32; i++)
	{
	if (mask & 1)
	    break ;
	mask = mask >> 1 ;
	}
    return( i );
    }

/*******************************************************************************
*
* cbioWrapOk - dummy function returning OK
*
*/
LOCAL STATUS cbioWrapOk ()
    {
    return OK;
    }

/*******************************************************************************
*
* cbioBlkWrapBufCreate - create the local block buffer
*
*/
LOCAL STATUS cbioBlkWrapBufCreate ( CBIO_DEV_ID pDev )
    {
    if( pDev->cbioMemBase != NULL &&
	pDev->cbioMemSize != pDev->cbioParams.bytesPerBlk )
	{
	/* block size may have changed */
	KHEAP_FREE(pDev->cbioMemBase);
	pDev->cbioMemBase = 0;
	pDev->cbioMemSize = 0;
	}

    if( pDev->cbioMemBase == NULL && pDev->cbioMemSize == 0 )
	{
	pDev->cbioMemSize = pDev->cbioParams.bytesPerBlk ;
	pDev->cbioMemBase = KHEAP_ALLOC(pDev->cbioMemSize);
	if( pDev->cbioMemBase == NULL )
	    {
	    return ERROR;
	    }

	/* Overload the block I/O function, better own the mutex here */

	pDev->pFuncs->cbioDevBlkRW = blkWrapBlkRWbuf ;

	/* empty block */

	pDev->cbioBlkNo = (u_long)NONE ;
	pDev->cbioDirtyMask = 0;

	return (OK);
	}
    else 
	{
	errno = EINVAL;
	return (ERROR);
	}
    }

/*******************************************************************************
*
* blkWrapBlkRW - Read/Write blocks
*
* Wrapper block Read/Write function calls the blkIo functions
* directly, It does not deal with the tiny cache, and is used
* only when there is no Tiny cache for by byte-wise operations.
* When a tiny cache is installed, this function is overloaded
* with the blkWrapBlkRWbuf function, which deals with Tiny cache
* coherency and call this function subsequently.
*
* This routine transfers between a user buffer and the lower layer 
* BLK_DEV. It is optimized for block transfers.  
*
* RETURNS OK or ERROR and may otherwise set errno.
*/
LOCAL STATUS blkWrapBlkRW
    (
    CBIO_DEV_ID		dev,
    block_t		startBlock,
    block_t		numBlocks,
    addr_t		buffer,
    CBIO_RW		rw,
    cookie_t *		pCookie
    )
    {
    CBIO_DEV *	pDev = (void *) dev ;
    BLK_DEV *	pBd = pDev->blkSubDev ;
    STATUS stat = ERROR;
    int retryCount = 0 ;

retryLoop:

    if (TRUE == pBd->bd_readyChanged || TRUE == CBIO_READYCHANGED(dev))
	{
	errno = S_ioLib_DISK_NOT_PRESENT ;
	return (ERROR);
	}

    if( startBlock > pDev->cbioParams.nBlocks ||
	(startBlock+numBlocks) > pDev->cbioParams.nBlocks )
	{
	errno = EINVAL;
	return (ERROR);
	}

    switch( rw )
	{
	case CBIO_READ:
	    stat = pBd->bd_blkRd( pBd, startBlock, numBlocks, buffer);
	    break;
	case CBIO_WRITE:
	    stat = pBd->bd_blkWrt( pBd, startBlock, numBlocks, buffer);
	    break;
	default:
	    errno = S_ioLib_UNKNOWN_REQUEST;
	    return (ERROR);
	}

    /* record error if any */

    if( stat == ERROR )
	{
	pDev->cbioParams.lastErrBlk = startBlock;
	pDev->cbioParams.lastErrno = errno ;

	if( errno == S_ioLib_DISK_NOT_PRESENT )
             {
	     CBIO_READYCHANGED (dev) = TRUE;
             }

	/* some block drivers dont do retires relying on dosFs1 for that */

	if( (blkWrapIoctl( pDev, CBIO_STATUS_CHK, 0) == OK) &&
	    ( retryCount++ < pBd->bd_retry ))
	    {
	    int tick = tickGet();

	    /* if device was not obviously replaced, try to reset it */

	    if( !(pBd->bd_readyChanged || CBIO_READYCHANGED(dev)) )
		{
		if( pBd->bd_reset != NULL )
		    pBd->bd_reset( pBd );
		do
		    {
		    if( blkWrapIoctl( pDev, CBIO_STATUS_CHK, 0) == OK)
			goto retryLoop;
		    taskDelay(5);
		    } while( tickGet() < (UINT32) tick+sysClkRateGet() ) ;
		}
	    }
	}

    return stat;
    }

/*******************************************************************************
*
* blkWrapBlkRWbuf - wrapper block I/O for coherency with tiny cache
*
* This routine transfers between a user buffer and the lower layer BLK_DEV
* It is optimized for block transfers.  
*
* RETURNS OK or ERROR and may otherwise set errno.
*/
LOCAL STATUS blkWrapBlkRWbuf
    (
    CBIO_DEV_ID     dev,
    block_t         startBlock,
    block_t         numBlocks,
    addr_t          buffer,
    CBIO_RW    	    rw,
    cookie_t *      pCookie
    )
    {
    FAST CBIO_DEV * pDev = dev ;
    STATUS stat = OK ;

    if( semTake( pDev->cbioMutex, WAIT_FOREVER) == ERROR )
	return ERROR;

    /* verify that there is a block buffer, if not, allocate one */
    if( pDev->cbioMemBase == NULL)
	{
	if( cbioBlkWrapBufCreate( pDev) == ERROR )
	    {
	    semGive(pDev->cbioMutex);
	    return ERROR;
	    }
	}
    /* see if Tiny Cache contains a valid block */
    if( pDev->cbioBlkNo != (u_long) NONE )
	{
	/* see if the range touches the cached block */
	if( (pDev->cbioBlkNo >= startBlock) &&
	    (pDev->cbioBlkNo < startBlock+numBlocks) )
	    {
	    /* flush the current contents of the block buffer if dirty */
	    if( pDev->cbioDirtyMask != 0 )
		{
		block_t cachedBlock = pDev->cbioBlkNo ;
		pDev->cbioBlkNo = NONE ;
		pDev->cbioDirtyMask = 0;
		stat = blkWrapBlkRW(  pDev, cachedBlock, 1,
			    pDev->cbioMemBase, CBIO_WRITE, NULL);
		}
	    else
		{
		/* else just forget about it */
		pDev->cbioBlkNo = NONE ;
		}
	    }
	}

    if( stat == ERROR )
        {
        semGive(pDev->cbioMutex); /* added semGive to fix SPR#76103 */
        return (ERROR);
        }                  

    stat = blkWrapBlkRW (dev, startBlock, numBlocks, buffer, rw, pCookie) ;

    semGive(pDev->cbioMutex);
    return stat ;
    }

/*******************************************************************************
*
* blkWrapBytesRW - Read/Write bytes
*
* In order to implement the byte-wise read/write operation, a tiny
* cache is implemented, which is used to store block data on which
* byte operations are performed.
*
* The tiny cache is a single disk block sized buffer at this time
* which should suffice for solid-state (e.g. Flash) disks to be used
* without the real disk cache, meaning dosFsLib directly on top of
* this wrapper.
*
* This routine transfers between a user buffer and the lower layer BLK_DEV
* It is optimized for byte transfers.  
*
* RETURNS OK or ERROR and may otherwise set errno.
*/
LOCAL STATUS blkWrapBytesRW
    (
    CBIO_DEV_ID 	dev,
    block_t		startBlock,
    off_t		offset,
    addr_t		buffer,
    size_t		nBytes,
    CBIO_RW		rw,
    cookie_t *		pCookie
    )
    {
    CBIO_DEV * 	pDev = dev ;
    BLK_DEV *	pBd = pDev->blkSubDev;
    STATUS	stat = OK ;
    caddr_t 	pStart;

    if( pBd->bd_readyChanged || CBIO_READYCHANGED (dev))
	{
	cbioRdyChgdSet(dev, TRUE);
	errno = S_ioLib_DISK_NOT_PRESENT ;
	return ERROR;
	}

    if( startBlock >= pDev->cbioParams.nBlocks )
	{
	errno = EINVAL;
	return ERROR;
	}

    /* verify that all bytes are within one block range */
    if (((offset + nBytes) > pDev->cbioParams.bytesPerBlk ) ||
	(offset <0) || (nBytes <=0))
	{
	errno = EINVAL;
	return ERROR;
	}

    if( semTake( pDev->cbioMutex, WAIT_FOREVER) == ERROR )
	return ERROR;

    /* verify that there is a block buffer, if not, allocate one */
    if( pDev->cbioMemBase == NULL)
	{
	if( cbioBlkWrapBufCreate( pDev) == ERROR )
	    {
	    semGive(pDev->cbioMutex);
	    return ERROR;
	    }
	}
    /* flush the current contents of the block buffer if needed */
    if( pDev->cbioBlkNo != (u_long)NONE &&
	 pDev->cbioBlkNo != startBlock &&
	 pDev->cbioDirtyMask != 0 )
	{
	block_t cachedBlock = pDev->cbioBlkNo ;
	pDev->cbioBlkNo = NONE ;
	pDev->cbioDirtyMask = 0;
	stat = blkWrapBlkRW( pDev, cachedBlock, 1,
			    pDev->cbioMemBase, CBIO_WRITE, NULL);
	if( stat == ERROR )
	    {
	    semGive(pDev->cbioMutex);
	    return ERROR;
	    }
	}

    /* get the requested block into cache */
    if( startBlock != pDev->cbioBlkNo && pDev->cbioDirtyMask == 0 )
	{
	stat = blkWrapBlkRW( pDev, startBlock, 1,
			    pDev->cbioMemBase, CBIO_READ, NULL);
	if( stat == ERROR )
	    {
	    semGive(pDev->cbioMutex);
	    return ERROR;
	    }
	pDev->cbioBlkNo = startBlock ;
	pDev->cbioDirtyMask = 0;
	}

    assert( startBlock == pDev->cbioBlkNo );

    /* calculate actual memory address of data */
    pStart = pDev->cbioMemBase + offset ;

#ifdef	DEBUG
    if(cbioDebug > 1)
	logMsg("blkWrapBytesRW: blk %d + %d # %d bytes -> addr %x, %x bytes\n",
	    startBlock, offset, nBytes, (int) pStart, nBytes, 0);
#endif

    switch( rw )
	{
	case CBIO_READ:
	    bcopy( pStart, buffer, nBytes );
	    break;
	case CBIO_WRITE:
	    bcopy( buffer, pStart,  nBytes );
	    pDev->cbioDirtyMask = 1;
	    break;
	}

    semGive(pDev->cbioMutex);

    return (OK);
    }

/*******************************************************************************
*
* blkWrapBlkCopy - Copy sectors 
*
* This routine makes copies of one or more blocks on the lower layer BLK_DEV.
* It is optimized for block copies on the subordinate layer.  
* 
* RETURNS OK or ERROR and may otherwise set errno.
*/
LOCAL STATUS blkWrapBlkCopy
    (
    CBIO_DEV_ID 	dev,
    block_t		srcBlock,
    block_t		dstBlock,
    block_t		numBlocks
    )
    {
    CBIO_DEV	*pDev = (void *) dev ;
    BLK_DEV *	pBd = pDev->blkSubDev;
    STATUS	stat = OK;

    if( pBd->bd_readyChanged || CBIO_READYCHANGED(dev))
	{
	CBIO_READYCHANGED(dev) = TRUE;
	errno = S_ioLib_DISK_NOT_PRESENT ;
	return (ERROR);
	}

    if( (srcBlock) > pDev->cbioParams.nBlocks ||
	(dstBlock) > pDev->cbioParams.nBlocks )
	{
	errno = EINVAL;
	return ERROR;
	}

    if( (srcBlock+numBlocks) > pDev->cbioParams.nBlocks ||
	(dstBlock+numBlocks) > pDev->cbioParams.nBlocks )
	{
	errno = EINVAL;
	return ERROR;
	}

    if( semTake( pDev->cbioMutex, WAIT_FOREVER) == ERROR )
	return ERROR;

    /* verify that there is a block buffer, if not, allocate one */

    if( pDev->cbioMemBase == NULL)
	{
	if( cbioBlkWrapBufCreate( pDev) == ERROR )
	    {
	    semGive(pDev->cbioMutex);
	    return ERROR;
	    }
	}

    /* flush the current contents of the block buffer if needed */

    if( pDev->cbioBlkNo != (u_long)NONE && 
         pDev->cbioDirtyMask != (u_long)0 )
	{
	block_t cachedBlock = pDev->cbioBlkNo ;

	stat = blkWrapBlkRW( pDev, cachedBlock, 1,
			    pDev->cbioMemBase, CBIO_WRITE, NULL);

	if( stat == ERROR )
	    {
	    semGive(pDev->cbioMutex);
	    return ERROR;
	    }
	}

    pDev->cbioBlkNo = NONE ;	/* invalidate buffer contents */
    pDev->cbioDirtyMask = 0;

    /* because tiny cache is one block size, copy blocks one at a time */
    for(; numBlocks > 0; numBlocks -- )
	{
	stat = blkWrapBlkRW( pDev, srcBlock, 1,
			    pDev->cbioMemBase, CBIO_READ, NULL);

	if( stat == ERROR)
	    break;

	stat = blkWrapBlkRW( pDev, dstBlock, 1,
			    pDev->cbioMemBase, CBIO_WRITE, NULL);

	if( stat == ERROR)
	    break;

	srcBlock ++;
	dstBlock ++;
	}

    semGive(pDev->cbioMutex);

    return stat;
    }

/*******************************************************************************
*
* blkWrapIoctl - Misc control operations 
*
* This performs the requested ioctl() operation.
* 
* CBIO modules can expect the following ioctl() codes from cbioLib.h:
* .IP
* CBIO_RESET - reset the CBIO device.   When the third argument to the ioctl
* call accompaning CBIO_RESET is NULL, the code verifies that the disk is 
* inserted and is ready, after getting it to a known state.   When the 3rd 
* argument is a non-zero, it is assumed to be a BLK_DEV pointer and 
* CBIO_RESET will install a new subordinate block device.    This work
* is performed at the BLK_DEV to CBIO layer, and all layers shall account
* for it.  A CBIO_RESET indicates a possible change in device geometry, 
* and the CBIO_PARAMS members will be reinitialized after a CBIO_RESET.
* .IP
* CBIO_STATUS_CHK - check device status of CBIO device and lower layer
* .IP
* CBIO_DEVICE_LOCK - Prevent disk removal 
* .IP
* CBIO_DEVICE_UNLOCK - Allow disk removal
* .IP
* CBIO_DEVICE_EJECT - Unmount and eject device
* .IP
* CBIO_CACHE_FLUSH - Flush any dirty cached data
* .IP
* CBIO_CACHE_INVAL - Flush & Invalidate all cached data
* .IP
* CBIO_CACHE_NEWBLK - Allocate scratch block
* .LP
*
* RETURNS OK or ERROR and may otherwise set errno.
*/
LOCAL STATUS blkWrapIoctl
    (
    CBIO_DEV_ID	dev,
    UINT32	command,
    addr_t	arg
    )
    {
    FAST CBIO_DEV_ID 	pDev = dev ;
    FAST BLK_DEV *	pBd = pDev->blkSubDev ;
    STATUS		stat = OK ;

    if( semTake( pDev->cbioMutex, WAIT_FOREVER) == ERROR )
	return ERROR;

    /*
     * CBIO_RESET may be optionally used to 
     * install a new subordinate block device
     * if 3rd argument is non-zero, it is expected to be
     * a new BLK_DEV pointer.
     */

    if( command == (int)CBIO_RESET  && arg != NULL )
	{
	BLK_DEV * pBd = (void *) arg ;
	int tmp;

	/*
	 * here we must verify BLK_DEV in much the same way as in
	 * cbioWrapBlkDev()
	 */

	stat = ERROR ;
	errno = EINVAL ;

	/* if the input is already a valid CBIO device, ignore it */
	if (OK == cbioDevVerify ((CBIO_DEV_ID)pBd))
	    {
	    goto ioctl_exit ; 
	    }

	/* blkIo has no mechanism for verification, improvise */
	else if( (pBd->bd_blkRd == NULL) || (pBd->bd_blkWrt == NULL) ||
		(pBd->bd_ioctl == NULL) )
	    goto ioctl_exit ;
	else
	    {

	    /* check that bytesPerBlk is 2^n */
	    tmp = shiftCalc( pBd->bd_bytesPerBlk );

	    if( pBd->bd_bytesPerBlk != (ULONG)( 1 << tmp ) )
		goto ioctl_exit ;

	    /* 
	     * for simplicity, we place a dummy function
	     * if statusChk is not provided
	     */

	    if( pBd->bd_statusChk == NULL )
		pBd->bd_statusChk = cbioWrapOk ;
	    stat = OK ;
	    errno = 0;
	    }

	/* this means that RESET will be later issued and reset all fields */

	CBIO_READYCHANGED(dev) = TRUE;

	dev->blkSubDev = (void *) pBd ;
	dev->pDc = (void *) pBd ;

	semGive(pDev->cbioMutex);
	return (stat);
	}

    /* End of CBIO_RESET for BLK_DEV replacement handling */

    /*
     * CBIO_RESET - 3rd arg is NULL:
     * verify that the disk is inserted and is ready,
     * after getting it to a known state for good measure
     * and reset readyChanged if indeed the disk is ready to roll.
     */

    if( command == (int)CBIO_RESET )
	{
	stat = OK; 
        errno = OK;

	/* if the Block's got a reset function, call it */

	if( pBd->bd_reset != NULL )
            {
            stat = pBd->bd_reset( pBd );
            }
	/* 
	 * for simplicity, we place a dummy function
	 * if statusChk is not provided
	 */

	if( pBd->bd_statusChk == NULL )
	    pBd->bd_statusChk = cbioWrapOk ;

	/*
	 * the drive's got a status check function,
	 * use it to find out if all is cool and dandy, and
	 * determine current state from status check result
	 */

	stat = pBd->bd_statusChk( pBd );

        CBIO_READYCHANGED(dev) = (ERROR == stat) ? TRUE : FALSE;

	/* since the block size may have changed, we must re-init */

	if( FALSE == CBIO_READYCHANGED (dev) )
	    {
	    pBd->bd_readyChanged = FALSE ;	/* HELP - FDC hack */
	    pDev->cbioParams.nBlocks		= pBd->bd_nBlocks ;
	    pDev->cbioParams.bytesPerBlk	= pBd->bd_bytesPerBlk ;
	    pDev->cbioParams.blocksPerTrack	= pBd->bd_blksPerTrack ;
	    pDev->cbioParams.nHeads		= pBd->bd_nHeads ;
	    pDev->cbioParams.blockOffset	= 0 ;
	    pDev->cbioMode			= pBd->bd_mode ;

	    if( pDev->cbioMemBase != NULL &&
		pDev->cbioMemSize != pBd->bd_bytesPerBlk )
		{
		KHEAP_FREE(pDev->cbioMemBase);
		pDev->cbioMemBase = NULL ;
		pDev->cbioMemSize = 0;
		pDev->pFuncs->cbioDevBlkRW = blkWrapBlkRW ;
		}

#ifdef	__unused__
	    blkWrapDiskSizeAdjust( pDev) ;
#endif	/*__unused__*/

	    pDev->cbioParams.lastErrBlk = NONE ;
	    pDev->cbioParams.lastErrno	 = 0 ;
	    }

	if( stat == ERROR && errno == OK )
	    errno = S_ioLib_DISK_NOT_PRESENT ;
	 
	semGive(pDev->cbioMutex);
	return (stat);
	}

    /* all other commands, except RESET, wont work if disk is not there */

    if( pBd->bd_readyChanged || CBIO_READYCHANGED(dev) )
	{
	CBIO_READYCHANGED(dev) = TRUE;
	errno = S_ioLib_DISK_NOT_PRESENT;
	semGive(pDev->cbioMutex);
	return ERROR;
	}

    switch ( command )
	{
	case CBIO_STATUS_CHK : 
	    stat = pBd->bd_statusChk( pBd );

            /* 
             * SPR#68387: readyChanged bit of the BLK_DEV 
             * should be checked to verify the change 
             * in the media. If there is a change in the 
             * media then update the cache layer's data structures.
             */

            if( pBd->bd_readyChanged || CBIO_READYCHANGED(dev) )
                {
	        pDev->cbioParams.nBlocks  = pBd->bd_nBlocks ;
       		pDev->cbioParams.bytesPerBlk  = pBd->bd_bytesPerBlk ;
	        pDev->cbioParams.blocksPerTrack = pBd->bd_blksPerTrack ;
	        pDev->cbioParams.nHeads  = pBd->bd_nHeads ;
	        pDev->cbioParams.blockOffset  = 0 ;
	        pDev->cbioMode           = pBd->bd_mode ;
                pDev->cbioParams.cbioRemovable  = pBd->bd_removable ;

		if( pDev->cbioMemBase != NULL && 
			pDev->cbioMemSize != pBd->bd_bytesPerBlk )
		      {
		      KHEAP_FREE ( pDev->cbioMemBase );
		      pDev->cbioMemBase = NULL ;
		      pDev->cbioMemSize = 0;
		      pDev->pFuncs->cbioDevBlkRW = blkWrapBlkRW ;
		      }
                }  
	    break;

	case CBIO_CACHE_FLUSH :
	case CBIO_CACHE_INVAL :
	case CBIO_CACHE_NEWBLK:
	    if( pDev->cbioMemBase != NULL &&
		pDev->cbioBlkNo != (u_long)NONE && 
                pDev->cbioDirtyMask != (u_long)0 )
		{
		block_t cachedBlock = pDev->cbioBlkNo ;
		stat = blkWrapBlkRW( pDev, cachedBlock, 1,
			    pDev->cbioMemBase, CBIO_WRITE, NULL);
		}

	    if( pDev->cbioMemBase != (u_long)NULL && 
                command == (int)CBIO_CACHE_NEWBLK )
		{
		bzero( pDev->cbioMemBase, pDev->cbioMemSize);
		pDev->cbioBlkNo = (block_t) arg ;
		pDev->cbioDirtyMask = 1;
		}
	    else
		{
		pDev->cbioBlkNo = NONE ;
		pDev->cbioDirtyMask = 0;
		}
	    break;

	default:
	    stat = pBd->bd_ioctl( pBd, command, arg );
	    break ;

	case CBIO_RESET : /* dealt with above, for switch completeness */
	case CBIO_DEVICE_LOCK :	/* unimplemented commands */
	case CBIO_DEVICE_UNLOCK :
	case CBIO_DEVICE_EJECT :
	    errno = S_ioLib_UNKNOWN_REQUEST;
	    stat = ERROR;
	    break;
	}
    ioctl_exit:
    semGive(pDev->cbioMutex);
    return stat;
    }

#ifdef	__unused__
/*******************************************************************************
*
* blkWrapDiskSizeAdjust - adjust true disk size
*
* Some block drivers report incorrect disk capacity in bd_nBlocks field.
* This function will experimentally discover the true size of the disk,
* by reading blocks until an error is returned by the driver.
* In order to overcome driver's sanity checks, the bd_nBlocks will
* be adjusted each time a call is made to read blocks beyond the
* previously reported capacity.
* There is a time limit on these attempts, so the final result may
* be still inaccurate if the capacity reported by the driver
* is significantly different from the actual disk capacity.
*
* RETURNS: N/A
*/
LOCAL void blkWrapDiskSizeAdjust
    (
    CBIO_DEV_ID 	dev 	/* CBIO device handle */
    )
    {
    FAST CBIO_DEV *	pDev = (void *) dev ;
    FAST BLK_DEV *	pBd = pDev->blkSubDev;
    STATUS		stat = OK ;
    int timeout = tickGet() + 2 * sysClkRateGet();	/* 2 sec timeout */
    int orig_nBlocks, secPerCyl ;
    caddr_t tmpBuf ;

    orig_nBlocks = pBd->bd_nBlocks ;
    secPerCyl = pBd->bd_blksPerTrack * pBd->bd_nHeads ;
    tmpBuf = KHEAP_ALLOC(pBd->bd_bytesPerBlk);
    if (NULL == tmpBuf)
	{
	}

    /* increment capacity by one cylinder and test readability */
    while( (tickGet()<=timeout) && (stat == OK ) )
	{
	/* read last block in current capacity */
	stat = pBd->bd_blkRd( pBd, pBd->bd_nBlocks-1, 1, tmpBuf );
	/* if OK, try to read end of next cyl, if failed, go back and break */
	if( stat == OK )
	    pBd->bd_nBlocks += secPerCyl;
	else
	    pBd->bd_nBlocks -= secPerCyl;
	}

    /* refresh timeout */
    timeout = tickGet() + (2 * sysClkRateGet());	/* 2 sec timeout */
    stat = OK ;

    /* increment capacity by one sector and test readability */
    while( (tickGet() <= timeout) && (stat == OK ) )
	{
	/* read last block in current capacity */
	stat = pBd->bd_blkRd( pBd, pBd->bd_nBlocks-1, 1, tmpBuf );
	if( stat == OK )
	    pBd->bd_nBlocks ++ ;
	else
	    pBd->bd_nBlocks -- ;
	}

#ifdef	DEBUG
	if(cbioDebug > 0 && tickGet()> timeout )
	    logMsg("blkWrapDiskSizeAdjust: timeout %d exceeded\n",
		timeout,0,0,0,0,0);
#endif

    /* if all goes well, the last read operation should have succeeded */
    if( (stat = pBd->bd_blkRd( pBd, pBd->bd_nBlocks-1, 1, tmpBuf )) == OK)
	{
#ifdef	DEBUG
	if(cbioDebug > 0)
	    logMsg("blkWrapDiskSizeAdjust: capacity increased by %d blocks\n",
		pBd->bd_nBlocks - orig_nBlocks, 0,0,0,0,0);
#endif
	/* make it official */
	pDev->cbioParams.nBlocks = pBd->bd_nBlocks ;
	}
    else
	{
	/* something is weird, put it back where it was */
#ifdef	DEBUG
	if(cbioDebug > 0)
	    logMsg("blkWrapDiskSizeAdjust: failed to adjust\n",
		0,0,0,0,0,0);
#endif
	pDev->cbioParams.nBlocks = pBd->bd_nBlocks = orig_nBlocks ;
	}

    KHEAP_FREE(tmpBuf);
    }

#endif	/*__unused__*/

/* End of File */

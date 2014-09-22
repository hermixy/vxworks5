/* rawFsLib.c - raw block device file system library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03j,30apr02,jkf  SPR#75255, corrected unneeded api change to rawFsDevInit()
                 SPR#72300, 4GB drive failed with rawFsOpen()
                 SPR#25017, corrected FIODISKFORMAT docs to reflect reality.
03i,09nov01,jkf  SPR#71633, dont set errno when DevCreate is called w/BLK_DEV
03h,20sep01,jkf  SPR#69031, common code for both AE & 5.x.
03g,31jul00,jkf  removal of RT-11 (rt11) file system, SPR#32821/31223
03f,29feb00,jkf  T3 changes, cleanup.
03e,31aug99,jkf  changes for new CBIO API.
03d,31jul99,jkf  T2 merge, tidiness & spelling, jdi's 01p SPR#(SPR 25663).
03c,23jul98,vld	 fixed SPR#8309 : rawFsDevInit() calls rawFsInit()
		 with default parameters;
		 fixed SPR#21875/9418 : don't validate seeked position for
		 volume end overgoing.
03b,23jul98,vld	 discontinued actual support for different
		 volume states except RAW_VD_MOUNTED and 
		 RAW_VD_READY_CHANGED;
03a,23jul98,vld	 driver interface changed to support CBIO_DEV;
		 fixed SPR#22606 : CBIO on-byte base access functions
		 are used instead of per file descriptor buffers
02b,23jul98,vld	 added support for 64-bit ioctl requests;
                 32-bit restricted old ioctl functions improved to check
   	         and to return ERROR for 32-bit overloading results;
		 added new routine rawFsIsValHuge().
02a,23jul98,vld	 - added support for 64-bit arithmetic to serve huge volumes:
		 type of rawFdCurPtr, rawFdNewPtr and rawFdEndPtr
                 fields of RAW_FILE_DESC changed to fsize_t;
    	    	 - new type defined for block number (block_t);
                 - "empty buffer" value constant
		 defined as BUF_EMPTY (block_t)(-1)
01p,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01l,02oct92,srh  added ioctl(FIOGETFL) to return file's open mode
01k,22jul92,kdl  changed from 4.0.2-style semaphores to mutexes.
01j,18jul92,smb  Changed errno.h to errnoLib.h.
01i,04jul92,jcf  scalable/ANSI/cleanup effort.
01h,26may92,rrr  the tree shuffle
01g,16dec91,gae  added includes for ANSI.
01f,19nov91,rrr  shut up some ansi warnings.
01e,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY and ...
		  -changed VOID to void
		  -changed copyright notice
01d,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by kdl.
01c,21feb91,jaa	 documentation cleanup.
01b,08oct90,kdl  lint.
01a,02oct90,kdl  written
*/

/*
This library provides basic services for disk devices that do not
use a standard file or directory structure.  The disk volume is treated
much like a large file.  Portions of it may be read, written, or the
current position within the disk may be changed.  However, there
is no high-level organization of the disk into files or directories.

USING THIS LIBRARY
The various routines provided by the VxWorks raw "file system" (rawFs) may
be separated into three broad groups: general initialization,
device initialization, and file system operation.

The rawFsInit() routine is the principal initialization function;
it need only be called once, regardless of how many rawFs devices
will be used.

A separate rawFs routine is used for device initialization.  For
each rawFs device, rawFsDevInit() must be called to install the device.

Several routines are provided to inform the file system of
changes in the system environment.  The rawFsModeChange() routine may be
used to modify the readability or writability of a particular device.
The rawFsReadyChange() routine is used to inform the file system that a
disk may have been swapped and that the next disk operation should first
remount the disk.  The rawFsVolUnmount() routine informs the
file system that a particular device should be synchronized and unmounted,
generally in preparation for a disk change.

INITIALIZATION
Before any other routines in rawFsLib can be used, rawFsInit() 
must be called to initialize the library.  This call specifies the
maximum number of raw device file descriptors that can be open
simultaneously and allocates memory for that many raw file descriptors.
Any attempt to open more raw device file descriptors than the specified
maximum will result in errors from open() or creat().

During the rawFsInit() call, the raw device library is installed as a driver
in the I/O system driver table.  The driver number associated with it is
then placed in a global variable, `rawFsDrvNum'.

This initialization is enabled when the configuration macro INCLUDE_RAWFS
is defined; rawFsInit() is then called from the root task, usrRoot(), in
usrConfig.c.

DEFINING A RAW DEVICE
To use this library for a particular device, the device structure
used by the device driver must contain, as the very first item, a CBIO
device description structure (CBIO_DEV) or block device description
structure (BLK_DEV) .  This must be initialized
before calling rawFsDevInit().

The rawFsDevInit() routine is used to associate a device with the rawFsLib
functions.  The <pVolName> parameter expected by rawFsDevInit() is a pointer to
a name string, to be used to identify the device.  This will serve as
the pathname for I/O operations which operate on the device.  This
name will appear in the I/O system device table, which may be displayed
using iosDevShow().

The syntax of the rawFsDevInit()
routine is as follows:
.CS
    rawFsDevInit
	(
	char     *pVolName, /@ name to be used for volume - iosDevAdd  @/
	BLK_DEV  *pDevice   /@ a pointer to BLK_DEV device or a CBIO_DEV_ID @/
	)
.CE

Unlike the VxWorks DOS file system, raw volumes do not
require an \%FIODISKINIT ioctl() function to initialize volume structures.
(Such an ioctl() call can be made for a raw volume, but it has no effect.)
As a result, there is no "make file system" routine for raw volumes
(for comparison, see the manual entry for rawFsMkfs()).

When rawFsLib receives a request from the I/O system, after rawFsDevInit()
has been called, it calls the appropriate device driver routines
to access the device.

MULTIPLE LOGICAL DEVICES
The block number passed to the block read and write routines is an absolute
number, starting from block 0 at the beginning of the device.  If desired,
the driver may add an offset from the beginning of the physical device
before the start of the logical device.  This would normally be done by
keeping an offset parameter in the driver's device-specific structure,
and adding the proper number of blocks to the block number passed to the read
and write routines.  See the ramDrv manual entry for an example.

UNMOUNTING VOLUMES (CHANGING DISKS)
A disk should be unmounted before it is removed.  When unmounted,
any modified data that has not been written to the disk will be written
out.  A disk may be unmounted by either calling rawFsVolUnmount() directly
or calling ioctl() with a FIODISKCHANGE function code.

There may be open file descriptors to a raw device volume when it is
unmounted.  If this is the case, those file descriptors will be marked
as obsolete.  Any attempts to use them for further I/O operations will
return an S_rawFsLib_FD_OBSOLETE error.  To free such file descriptors, use the
close() call, as usual.  This will successfully free the descriptor,
but will still return S_rawFsLib_FD_OBSOLETE.

SYNCHRONIZING VOLUMES
A disk should be "synchronized" before it is unmounted.  To synchronize a
disk means to write out all buffered data (the write buffers associated
with open file descriptors), so that the disk is updated.  It may
or may not be necessary to explicitly synchronize a disk, depending on
how (or if) the driver issues the rawFsVolUnmount() call.

When rawFsVolUnmount() is called, an attempt will be made to synchronize the
device before unmounting.  However, if the rawFsVolUnmount() call is made by
a driver in response to a disk being removed, it is obviously too late to
synchronize.  Therefore, a separate ioctl() call specifying the FIOSYNC
function should be made before the disk is removed.  (This could be done in
response to an operator command.)

If the disk will still be present and writable when rawFsVolUnmount() is
called, it is not necessary to first synchronize the disk.  In
all other circumstances, failure to synchronize the volume before
unmounting may result in lost data.

IOCTL FUNCTIONS
The VxWorks raw block device file system supports the following ioctl()
functions.  The functions listed are defined in the header ioLib.h.

.iP "FIODISKFORMAT" 16 3
No file system is initialized on the disk by this request.
This ioctl is passed directly down to the driver-provided function:
.CS
    fd = open ("DEV1:", O_WRONLY);
    status = ioctl (fd, FIODISKFORMAT, 0);
.CE
.iP "FIODISKINIT"
Initializes a raw file system on the disk volume.
Since there are no file system structures, this functions performs no action.
It is provided only for compatibility with other VxWorks file systems.
.iP "FIODISKCHANGE"
Announces a media change.  It performs the same function as rawFsReadyChange().
This function may be called from interrupt level:
.CS
    status = ioctl (fd, FIODISKCHANGE, 0);
.CE
.iP "FIOUNMOUNT"
Unmounts a disk volume.  It performs the same function as rawFsVolUnmount().
This function must not be called from interrupt level:
.CS
    status = ioctl (fd, FIOUNMOUNT, 0);
.CE
.iP "FIOGETNAME"
Gets the file name of the file descriptor and copies it to the buffer 
<nameBuf>:
.CS
    status = ioctl (fd, FIOGETNAME, &nameBuf);
.CE
.iP "FIOSEEK"
Sets the current byte offset on the disk to the position specified
by <newOffset>:
.CS
    status = ioctl (fd, FIOSEEK, newOffset);
.CE
.iP "FIOWHERE"
Returns the current byte position from the start of the device
for the specified file descriptor.
This is the byte offset of the next byte to be read or written.
It takes no additional argument:
.CS
    position = ioctl (fd, FIOWHERE, 0);
.CE
.iP "FIOFLUSH"
Writes all modified file descriptor buffers to the physical device.
.CS
    status = ioctl (fd, FIOFLUSH, 0);
.CE
.iP "FIOSYNC"
Performs the same function as FIOFLUSH.

.iP "FIONREAD"
Copies to <unreadCount> the number of bytes from the current file position
to the end of the device:
.CS
    status = ioctl (fd, FIONREAD, &unreadCount);
.CE
.LP

INCLUDE FILES: rawFsLib.h

SEE ALSO: ioLib, iosLib, rawFsLib, ramDrv,
.pG "I/O System, Local File Systems"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "private/dosFsVerP.h"
#include "ctype.h"
#include "ioLib.h"
#include "lstLib.h"
#include "stdlib.h"
#include "string.h"
#include "semLib.h"
#include "errnoLib.h"
#include "assert.h"
#include "cbioLib.h"
#include "rawFsLib.h"


#define SIZE64

#ifdef SIZE64
typedef long long       fsize_t;
#define RAWFS_IS_VAL_HUGE( val )  ( ( (fsize_t)(val) >> 32 ) != 0 )
#else
typedef size_t          fsize_t;
#define RAWFS_IS_VAL_HUGE( val )  FALSE
#endif /* SIZE64 */

/* Raw device file descriptor */

typedef struct		/* RAW_FILE_DESC */
    {
    NODE	rawFdNode;	/* linked list node info */
    SEM_ID	rawFdSemId;	/* semaphore for this file descriptor */
    int		rawFdStatus;	/* (OK | NOT_IN_USE) */
    RAW_VOL_DESC * pRawFdVd;	/* ptr to volume descriptor */
    int		rawFdMode;	/* mode: O_RDONLY, O_WRONLY, O_RDWR */
    fsize_t	rawFdNewPtr;	/* file byte ptr for new read/writes */
    fsize_t	rawFdEndPtr;	/* file byte ptr to end of file */
    cookie_t	rawFdCookie;	/* CBIO cookie of recently accessed sector */
    } RAW_FILE_DESC;


/* File descriptor status values */

#define RAWFD_AVAILABLE		-1	/* file descriptor available */
#define RAWFD_IN_USE		0	/* file descriptor in-use */
#define RAWFD_OBSOLETE		1	/* file descriptor obsolete */
#define BUF_EMPTY	(block_t)(-1)	/* buffer not initialized */

/* type definitions */

/* GLOBALS */

int 	rawFsDrvNum   = ERROR;	   /* I/O system driver number for rawFsLib */

					/* default mutex options */
int	rawFsVolMutexOptions    = (SEM_Q_PRIORITY | SEM_DELETE_SAFE);
int	rawFsFdListMutexOptions = (SEM_Q_PRIORITY | SEM_DELETE_SAFE);
int	rawFsFdMutexOptions     = (SEM_Q_PRIORITY | SEM_DELETE_SAFE);



/* LOCALS */

LOCAL LIST	rawFsFdActiveList;	/* linked list of in-use Fd's */
LOCAL LIST	rawFsFdFreeList;	/* linked list of avail. Fd's */
LOCAL SEM_ID 	rawFsFdListSemId;	/* file descr list semaphore  */
LOCAL int 	rawFsMaxFiles;		/* max files open at once     */


/* forward static functions */

static STATUS	rawFsBlkRdWr (RAW_VOL_DESC *pVd, block_t startBlk,
			      int numBlks, FAST char *pBuf, 
                              CBIO_RW rw);
static STATUS	rawFsBtRdWr (RAW_VOL_DESC *pVd, block_t blk,
			     off_t offset, char * pBuf, size_t nBytes,
			     CBIO_RW rw, cookie_t * pCookie);
static STATUS	rawFsClose (RAW_FILE_DESC *pRawFd);
static STATUS	rawFsFdFlush (RAW_FILE_DESC *pRawFd);
static void	rawFsFdFree (RAW_FILE_DESC *pRawFd);
static RAW_FILE_DESC *rawFsFdGet (void);
static STATUS	rawFsFlush (RAW_VOL_DESC *pVd);
static STATUS	rawFsIoctl (RAW_FILE_DESC *pRawFd, int function, int arg);
static RAW_FILE_DESC *rawFsOpen (RAW_VOL_DESC *pVd, char *name, int flags);
static int	rawFsRead (RAW_FILE_DESC *pRawFd, char *pBuf, int maxBytes);
static STATUS	rawFsReset (RAW_VOL_DESC *pVd);
static STATUS	rawFsSeek (RAW_FILE_DESC *pRawFd, fsize_t position);
static STATUS	rawFsVolCheck (RAW_VOL_DESC *pVd);
static STATUS	rawFsVolFlush (RAW_VOL_DESC *pVd);
static STATUS	rawFsVolMount (RAW_VOL_DESC *pVd);
static fsize_t	rawFsWhere (RAW_FILE_DESC *pRawFd);
static int	rawFsWrite (RAW_FILE_DESC *pRawFd, char *pBuf, int maxBytes);


/*******************************************************************************
*
* rawFsIsValHuge -  - check if value is greater, than 4GB (max 32 bit).
*
* * RETURNS: TRUE if is greater, else return FALSE.
*/
LOCAL BOOL rawFsIsValHuge
    (
    fsize_t     val
    )
    {
    return RAWFS_IS_VAL_HUGE( val );
    } /* rawFsIsValHuge() */

/*******************************************************************************
*
* rawFsBlkRdWr - read/write block(s) from/to a raw volume
*
* This routine reads/writes the specified block from/to the
* specified volume.
*
* RETURNS:
* OK, or
* ERROR, if read/write error.
*/

LOCAL STATUS rawFsBlkRdWr
    (
    FAST RAW_VOL_DESC   *pVd,         	/* pointer to volume descriptor */
    block_t             startBlk,       /* starting block for read */
    int                 numBlks,        /* how many blocks to read */
    FAST char           *pBuf,          /* buffer to receive block */
    CBIO_RW		rw		/* read / write */
    )
    {
    return (cbioBlkRW (pVd->rawVdCbio, startBlk, numBlks, pBuf, rw , NULL));
    }

/*******************************************************************************
*
* rawFsBtRdWr - read/write some bytes from/to block.
*
* This routine reads/writes the specified number of bytes
* from/to specified block starting at specified offset.
*
* RETURNS:
* OK, or
* ERROR, if volume access error.
*/

LOCAL STATUS rawFsBtRdWr
    (
    FAST RAW_VOL_DESC   *pVd,         /* pointer to volume descriptor */
    block_t		blkNum,       /* block number */
    off_t		offset,		/* offset in block */
    char		*pBuf,          /* data buffer */
    size_t		numBytes,        /* how many bytes to process */
    CBIO_RW		rw,		/* read / write */
    cookie_t        	*pCookie
    )
    {
    return ( cbioBytesRW (
    		pVd->rawVdCbio, blkNum, offset, pBuf, numBytes, rw, pCookie));
    }

/*******************************************************************************
*
* rawFsClose - close raw volume
*
* This routine closes the specified raw volume file descriptor.  Any
* buffered data for the descriptor is written to the physical device.
*
* If the file descriptor has been marked as obsolete (meaning the
* volume was unmounted while the descriptor was open), the descriptor
* is freed, but an error is returned.
*
* RETURNS: OK, or ERROR.
*/

LOCAL STATUS rawFsClose
    (
    FAST RAW_FILE_DESC  *pRawFd         /* file descriptor pointer */
    )
    {
    FAST STATUS	      status;
    FAST RAW_VOL_DESC *pVd = pRawFd->pRawFdVd;
					/* pointer to volume descriptor */


    /* Take control of file descriptor */

    semTake (pRawFd->rawFdSemId, WAIT_FOREVER); /* get ownership of fd */



    /* If file descriptor is obsolete, free it but return error */

    if (pRawFd->rawFdStatus == RAWFD_OBSOLETE)
	{
    	rawFsFdFree (pRawFd);			/* put fd on free list */
	semGive (pRawFd->rawFdSemId);		/* release fd */

	errnoSet (S_rawFsLib_FD_OBSOLETE);
	return (ERROR);				/* volume was remounted */
	}



    semTake (pVd->rawVdSemId, WAIT_FOREVER); /* get ownership of vol */

    /* Write out file descriptor buffer, if necessary */

    status = rawFsFdFlush (pRawFd);


    semGive (pVd->rawVdSemId);		/* release volume */

    rawFsFdFree (pRawFd);			/* put fd on free list */
    semGive (pRawFd->rawFdSemId);		/* release fd */
    return (status);
    }

/*******************************************************************************
*
* rawFsDevInit - associate a block device with raw volume functions
*
* This routine takes a block device created by a device driver and
* defines it as a raw file system volume.  As a result, when high-level 
* I/O operations, such as open() and write(), are performed on the
* device, the calls will be routed through rawFsLib.
*
* This routine associates <pVolName> with a device and installs it in
* the VxWorks I/O System's device table.  The driver number used when
* the device is added to the table is that which was assigned to the
* raw library during rawFsInit().  (The driver number is kept in
* the global variable `rawFsDrvNum'.)
*
* The pDevice is a CBIO_DEV_ID or BLK_DEV ptr and contains configuration
* data describing the device and the addresses of routines which
* will be called to access device. These routines
* will not be called until they are required by subsequent I/O 
* operations.
*
* INTERNAL
* The semaphore in the device is given, so the device is available for
* use immediately.
*
* RETURNS: A pointer to the volume descriptor (RAW_VOL_DESC),
* or NULL if there is an error.
*/

RAW_VOL_DESC *rawFsDevInit
    (
    char      * pVolName,       /* volume name to be used with iosDevAdd */
    BLK_DEV   * pDevice         /* a pointer to a BLK_DEV or a CBIO_DEV_ID */
    )
    {
    FAST RAW_VOL_DESC	*pVd;		/* pointer to new volume descriptor */
    CBIO_DEV_ID	cbio = NULL;

    /* init RAW FS, if not yet */
    
    if (rawFsInit (0) == ERROR)
	return (NULL);
    
    if (ERROR == cbioDevVerify( (CBIO_DEV_ID) pDevice ))
        {
        /* attempt to handle BLK_DEV subDev */
        cbio = cbioWrapBlkDev ( pDevice );

        if( NULL != cbio )
            {
            /* SPR#71633, clear the errno set in cbioDevVerify() */
            errno = 0;
            }
        }
    else
	{
	cbio = (CBIO_DEV_ID) pDevice;
	}

    if ( NULL == cbio )
	{
	return (NULL);
	}

    /* Allocate a raw volume descriptor for device */

    pVd = (RAW_VOL_DESC *) KHEAP_ALLOC (sizeof (RAW_VOL_DESC));

    if (NULL == pVd) 
	{
	return (NULL);			/* no memory */
	}
    else
	{
        bzero ((char *)pVd, sizeof (RAW_VOL_DESC));
	}


    /* Add device to system device table */

    if (iosDevAdd ((DEV_HDR *) pVd, pVolName, rawFsDrvNum) != OK)
	{
	KHEAP_FREE ((char *) pVd);
	return (NULL);				/* can't add device */
	}


    /* Initialize volume descriptor */

    pVd->rawVdCbio 	= cbio;
    pVd->rawVdStatus	= ERROR;	/* have not been mounted yet */
    pVd->rawVdState	= RAW_VD_READY_CHANGED;


    /* Create volume locking semaphore (initially available) */

    pVd->rawVdSemId = semMCreate (rawFsVolMutexOptions);
    if (pVd->rawVdSemId == NULL)
	{
	KHEAP_FREE ((char *) pVd);
	return (NULL);		/* could not create semaphore */
	}

    return (pVd);
    }

/*******************************************************************************
*
* rawFsFdFlush - initiate CBIO buffers flushing.
*
* RETURNS: CBIO ioctl return code.
*/

LOCAL STATUS rawFsFdFlush
    (
    FAST RAW_FILE_DESC  *pRawFd         /* pointer to file descriptor */
    )
    {
    /* device flush returns with ERROR, if device is unavailable */
    
    return (cbioIoctl (
    		pRawFd->pRawFdVd->rawVdCbio, CBIO_CACHE_FLUSH, 
    		(void*)(-1) ));
    }

/*******************************************************************************
*
* rawFsFdFree - free a file descriptor
*
* This routine removes a raw device file descriptor from the active
* Fd list and places it on the free Fd list.
*
*/

LOCAL void rawFsFdFree
    (
    FAST RAW_FILE_DESC  *pRawFd      /* pointer to file descriptor to free */
    )
    {

    semTake (rawFsFdListSemId, WAIT_FOREVER);	/* take control of Fd lists */

    pRawFd->rawFdStatus = RAWFD_AVAILABLE;

    lstDelete (&rawFsFdActiveList, &pRawFd->rawFdNode);
					/* remove Fd from active list */

    lstAdd (&rawFsFdFreeList, &pRawFd->rawFdNode);
						/* add Fd to free list */

    semGive (rawFsFdListSemId);			/* release Fd lists */
    }

/*******************************************************************************
*
* rawFsFdGet - get an available file descriptor
*
* This routine obtains a free raw volume file descriptor.
*
* RETURNS: Pointer to file descriptor, or NULL if none available.
*/

LOCAL RAW_FILE_DESC *rawFsFdGet (void)
    {
    FAST RAW_FILE_DESC	*pRawFd;	/* ptr to newly acquired file descr */


    semTake (rawFsFdListSemId, WAIT_FOREVER);	/* take control of Fd lists */

    pRawFd = (RAW_FILE_DESC *) lstGet (&rawFsFdFreeList);
						/* get a free Fd */

    if (pRawFd != NULL)
	{
	pRawFd->rawFdStatus = RAWFD_IN_USE;	/* mark Fd as in-use */
	lstAdd (&rawFsFdActiveList, (NODE *) pRawFd);
						/* add to active list */
	}
    else
    	errnoSet (S_rawFsLib_NO_FREE_FILE_DESCRIPTORS);
						/* max files already open */

    semGive (rawFsFdListSemId);			/* release Fd lists */

    return (pRawFd);
    }

/*******************************************************************************
*
* rawFsFlush - write all modified volume structures to disk
*
* This routine will write all buffered data for a volume to the physical
* device.  It is called by issuing an ioctl() call with the FIOFLUSH or
* FIOSYNC function codes.  It may be used periodically to provide enhanced
* data integrity, or to prepare a volume for removal from the device.
*
* RETURNS: OK, or ERROR if volume could not be sync'd.
*
*/

LOCAL STATUS rawFsFlush
    (
    FAST RAW_VOL_DESC   *pVd          /* pointer to volume descriptor */
    )
    {
    FAST STATUS		status;		/* return status */


    /* Check that volume is available */

    semTake (pVd->rawVdSemId, WAIT_FOREVER);	/* get ownership of volume */

    if (pVd->rawVdState != RAW_VD_MOUNTED)
        {
        semGive (pVd->rawVdSemId);		/* release volume */
        return (ERROR);				/* cannot access volume */
        }

    /* Flush volume to disk */

    status = rawFsVolFlush (pVd);


    semGive (pVd->rawVdSemId);		/* release volume */

    return (status);
    }

/*******************************************************************************
*
* rawFsInit - prepare to use the raw volume library
*
* This routine initializes the raw volume library.  It must be called exactly
* once, before any other routine in the library.  The argument specifies the
* number of file descriptors that may be open at once.  This routine allocates
* and sets up the necessary memory structures and initializes semaphores.
*
* This routine also installs raw volume library routines in the VxWorks I/O
* system driver table.  The driver number assigned to rawFsLib is placed in
* the global variable `rawFsDrvNum'.  This number will later be associated
* with system file descriptors opened to rawFs devices.
*
* To enable this initialization, define INCLUDE_RAWFS in configAll.h;
* rawFsInit() will then be called from the root task, usrRoot(), in usrConfig.c.
*
* RETURNS: OK or ERROR.
*/

STATUS rawFsInit
    (
    int  maxFiles  /* max no. of simultaneously open files */
    )
    {
    FAST RAW_FILE_DESC	*pRawFd;/* pointer to created file descriptor */
    FAST int		ix;		/* index var */

    if (rawFsDrvNum != ERROR)
    	return (OK);	/* initiation already done */
    
    maxFiles = (maxFiles <= 0)? RAWFS_DEF_MAX_FILES : maxFiles;
    
    /* Install rawFsLib routines in I/O system driver table
     *   Note that there is no delete routine, and that the
     *   rawFsOpen routine is also used as the create function.
     */

    rawFsDrvNum = iosDrvInstall ((FUNCPTR) rawFsOpen, (FUNCPTR) NULL,
			         (FUNCPTR) rawFsOpen, rawFsClose,
			         rawFsRead, rawFsWrite, rawFsIoctl);
    if (rawFsDrvNum == ERROR)
	return (ERROR);				/* can't install as driver */


    /* Create semaphore for locking access to file descriptor list */

    rawFsFdListSemId = semMCreate (rawFsFdListMutexOptions);
    if (rawFsFdListSemId == NULL)
	return (ERROR);				/* can't create semaphore */

    semTake (rawFsFdListSemId, WAIT_FOREVER);	/* take control of fd list */


    /* Allocate memory for required number of file descriptors */

    pRawFd = (RAW_FILE_DESC *) 
			KHEAP_ALLOC ((UINT) maxFiles * sizeof (RAW_FILE_DESC));

    if (NULL == pRawFd)
	{
	return (ERROR);				/* no memory */
	}
    else
	{
	bzero ((char *)pRawFd, ((UINT) maxFiles * sizeof (RAW_FILE_DESC)));
	}

    /* Create list containing required number of file descriptors */

    rawFsMaxFiles = maxFiles;

    for (ix = 0; ix < rawFsMaxFiles; ix++)
	{
	pRawFd->rawFdStatus = RAWFD_AVAILABLE;

	/* Create semaphore for this fd (initially available) */

    	pRawFd->rawFdSemId = semMCreate (rawFsFdMutexOptions);
    	if (pRawFd->rawFdSemId == NULL)
	    return ((int) NULL);		/* could not create semaphore */

	/* Add file descriptor to free list */

	lstAdd (&rawFsFdFreeList, &pRawFd->rawFdNode);

	pRawFd++;				/* next Fd */
	}

    semGive (rawFsFdListSemId);			/* release Fd lists */
    return (OK);
    }

/*******************************************************************************
*
* rawFsIoctl - perform a device-specific control function
*
* This routine performs the following ioctl() functions:
*
* .CS
*    FIODISKINIT:   Initialize the disk volume.  This routine does not
*                   format the disk, that must be done by the driver.
*    FIOSEEK:       Sets the file's current byte position to
*                   the position specified by arg.
*    FIOWHERE:      Returns the current byte position in the file.
*    FIOSYNC:	    Write all modified file descriptor buffers to device.
*    FIOFLUSH:      Same as FIOSYNC.
*    FIONREAD:      Return in arg the number of unread bytes.
*    FIODISKCHANGE: Indicate media change.  See rawFsReadyChange().
*    FIOUNMOUNT:    Unmount disk volume.  See rawFsVolUnmount().
*    FIOGETFL:      Return in arg the file's open mode.
* .CE
*
* If the ioctl (<function>) is not one of the above, the device driver
* ioctl() routine is called to perform the function.
*
* If an ioctl() call fails, the task status (see errnoGet()) indicates
* the nature of the error.
*
* RETURNS: OK, or ERROR if function failed or unknown function, or
* current byte pointer for FIOWHERE.
*/

LOCAL STATUS rawFsIoctl
    (
    FAST RAW_FILE_DESC  *pRawFd,       /* file descriptor of file to control */
    int                 function,       /* function code */
    int                 arg             /* some argument */
    )
    {
    FAST int		retValue = ERROR;	/* return value */
    fsize_t	buf64 = 0;



    semTake (pRawFd->rawFdSemId, WAIT_FOREVER); /* take control of fd */


    /* Check that file descriptor is current */

    if (pRawFd->rawFdStatus == RAWFD_OBSOLETE)
	{
	semGive (pRawFd->rawFdSemId);		/* release fd */
	errnoSet (S_rawFsLib_FD_OBSOLETE);
	return (ERROR);
	}


    /* Perform requested function */

    switch (function)
	{
	case FIODISKINIT:
	    retValue = OK;
	    break;

	case FIOSEEK:
            {
            buf64 = (fsize_t)arg;

	    retValue = rawFsSeek (pRawFd, buf64);
	    break;
    	    }

    	case FIOSEEK64:
            {
            retValue = rawFsSeek (pRawFd, *(fsize_t *)arg);
            break;
            } 
 
	case FIOWHERE:
            {
	    buf64 = rawFsWhere (pRawFd);
            if( rawFsIsValHuge( buf64 ) )
                {
                errnoSet( S_rawFsLib_32BIT_OVERFLOW );
    	    	retValue = ERROR;
                }
    	    else
                retValue = (u_int)buf64;

	    break;
            }

        case FIOWHERE64:        /* position within 64 bit boundary */
            {
            retValue = ERROR;
            if( (void *)arg == NULL )
                {
                errnoSet( S_rawFsLib_INVALID_PARAMETER );
                }
            else
                {
                *(fsize_t *)arg = rawFsWhere (pRawFd);
                retValue = OK;
                }
            break;
            }
 
	case FIOFLUSH:
	case FIOSYNC:
	    retValue = rawFsFlush (pRawFd->pRawFdVd);
	    break;

	case FIONREAD:
            {
            buf64 = pRawFd->rawFdEndPtr - (rawFsWhere (pRawFd));
    	    
            retValue = ERROR;
            if( (void *)arg == NULL )
                {
                errnoSet( S_rawFsLib_INVALID_PARAMETER );
                }
            else if( rawFsIsValHuge( buf64 ) )
                {
                errnoSet( S_rawFsLib_32BIT_OVERFLOW );
                *(u_int *)arg = (-1);
                }
            else
                {
                *(u_long *)arg = buf64;
    	    	retValue = OK;
    	    	}
	    break;
            }

        case FIONREAD64:
            {
            retValue = ERROR;
            if( (void *)arg == NULL )
                {
                errnoSet( S_rawFsLib_INVALID_PARAMETER );
                }
            else
                {
                *(fsize_t *)arg = pRawFd->rawFdEndPtr -
                                  (rawFsWhere (pRawFd));
                retValue = OK;
                }
    	    break;
            }

	case FIODISKCHANGE:
	    rawFsReadyChange (pRawFd->pRawFdVd);
	    retValue = OK;
	    break;

	case FIOUNMOUNT:
	    retValue = rawFsVolUnmount (pRawFd->pRawFdVd);
	    break;

	case FIOGETFL:
	    *((int *) arg) = pRawFd->rawFdMode;
	    retValue = OK;
	    break;

	default:
	    /* Call device driver function */

	    retValue = (cbioIoctl
		       (pRawFd->pRawFdVd->rawVdCbio, function, 
		       (void *)arg));
	}

    semGive (pRawFd->rawFdSemId);		/* release fd */
    return (retValue);				/* return obtained result */
    }

/*******************************************************************************
*
* rawFsModeChange - modify the mode of a raw device volume
*
* This routine sets the device's mode to <newMode> by setting
* the mode field in
* the device structure.  This routine should be called whenever the read
* and write capabilities are determined, usually after a ready change.
*
* The driver's device initialization routine should initially set the mode
* to O_RDWR (i.e., both O_RDONLY and O_WRONLY).
*
* RETURNS: N/A
*
* SEE ALSO: rawFsReadyChange()
*/

void rawFsModeChange
    (
    RAW_VOL_DESC  *pVd,       /* pointer to volume descriptor */
    int           newMode       /* O_RDONLY/O_WRONLY/O_RDWR (both) */
    )
    {
    cbioModeSet (pVd->rawVdCbio, newMode);
    }

/*******************************************************************************
*
* rawFsOpen - open a raw volume
*
* This routine opens the volume with the specified mode
* (O_RDONLY/O_WRONLY/O_RDWR/CREATE/TRUNC).
*
* The specified <name> must be a null string, indicating that the
* caller specified only the device name during open().
*
* If the device driver supplies a status-check routine, it will be
* called before the volume state is examined.
*
* RETURNS: Pointer to raw volume file descriptor, or ERROR.
*/

LOCAL RAW_FILE_DESC *rawFsOpen
    (
    FAST RAW_VOL_DESC   *pVd,         /* pointer to volume descriptor */
    char                *name,          /* "file" name - should be empty str */
    int                 flags           /* open flags */
    )
    {
    FAST RAW_FILE_DESC	*pRawFd;	/* file descriptor pointer */
    FAST CBIO_DEV_ID	rawVdCbio = pVd->rawVdCbio;
					/* pointer to block device info */
    CBIO_PARAMS cbioParams;

    /* Get CBIO device parameters */

    if (ERROR == cbioParamsGet (rawVdCbio, &cbioParams))
	{
	return ((RAW_FILE_DESC *) ERROR);
	}

    /* Check for open of other than raw volume (non-null filename) */

    if (name [0] != EOS)
	{
	errnoSet (S_rawFsLib_ILLEGAL_NAME);
	return ((RAW_FILE_DESC *) ERROR);	/* cannot specify filename */
	}

    /* Clear possible O_CREAT and O_TRUNC flag bits */

    flags &= ~(O_CREAT | O_TRUNC);


    /* Get a free file descriptor */

    if ((pRawFd = rawFsFdGet()) == NULL)
	return ((RAW_FILE_DESC *) ERROR);	/* max fd's already open */

    semTake (pRawFd->rawFdSemId, WAIT_FOREVER); /* take control of fd */


    /* Get ownership of volume */

    semTake (pVd->rawVdSemId, WAIT_FOREVER);	/* take volume */


    /* Check that volume is available */

    if (rawFsVolCheck (pVd) != OK)
	{
	semGive (pVd->rawVdSemId);
	rawFsFdFree (pRawFd);
	semGive (pRawFd->rawFdSemId);
	errnoSet (S_rawFsLib_VOLUME_NOT_AVAILABLE);
	return ((RAW_FILE_DESC *) ERROR);	/* cannot access volume */
	}

    /* Check for read-only volume opened for write/update */

    if ((cbioModeGet(pVd->rawVdCbio) == O_RDONLY)  &&
	(flags == O_WRONLY || flags == O_RDWR))
	{
	semGive (pVd->rawVdSemId);
	rawFsFdFree (pRawFd);
	semGive (pRawFd->rawFdSemId);
	errnoSet (S_ioLib_WRITE_PROTECTED);
	return ((RAW_FILE_DESC *) ERROR);	/* volume not writable */
	}

    semGive (pVd->rawVdSemId);		/* release volume */

    pRawFd->rawFdMode	   = flags;
    pRawFd->pRawFdVd	   = pVd;
    pRawFd->rawFdNewPtr   = 0;
    pRawFd->rawFdEndPtr   = (fsize_t) ((fsize_t)cbioParams.nBlocks * 
                                       (fsize_t)cbioParams.bytesPerBlk);

    semGive (pRawFd->rawFdSemId);		/* release fd */
    return (pRawFd);
    }

/*******************************************************************************
*
* rawFdRdWr - read from/write to a raw volume
*
* See rawFdRead()/rawFdWrite()
*
* RETURNS: Number of bytes actually processed, or 0 if end of file, or ERROR.
*/

LOCAL int rawFdRdWr
    (
    FAST RAW_FILE_DESC  *pRawFd,	/* file descriptor pointer */
    char                *pBuf,		/* addr of data buffer */
    int                 maxBytes,/* maximum bytes to read/write into buffer */
    CBIO_RW		rw	/* read / write */
    )
    {
    int			offset;	/* position in file I/O buffer */
    FAST int		bytesLeft; 
    			/* count of remaining bytes to process */
    FAST int		nBytes;	/* byte count from individual process */
    FAST block_t	blkNum;		/* starting block for read */
    FAST int		numBlks;	/* number of blocks to read */
    FAST RAW_VOL_DESC	*pVd = pRawFd->pRawFdVd;
					/* ptr to volume descriptor */
    FAST CBIO_DEV_ID	rawVdCbio = pRawFd->pRawFdVd->rawVdCbio;
					/* ptr to block device info */
    CBIO_PARAMS cbioParams;

    /* Get CBIO device parameters */

    if (ERROR == cbioParamsGet (rawVdCbio, &cbioParams))
	{
	return (ERROR);
	}
    
    assert (rw == CBIO_WRITE || rw == CBIO_READ);
    
    semTake (pRawFd->rawFdSemId, WAIT_FOREVER); /* take control of fd */

    /* Check that file descriptor is current */

    if (pRawFd->rawFdStatus == RAWFD_OBSOLETE)
	{
	semGive (pRawFd->rawFdSemId);
	errnoSet (S_rawFsLib_FD_OBSOLETE);
	return (ERROR);
	}

    /* Check that device was opened for writing, if write in progress */

    if (rw == CBIO_WRITE && pRawFd->rawFdMode == O_RDONLY)
	{
	semGive (pRawFd->rawFdSemId);
        errnoSet (S_rawFsLib_READ_ONLY);
	return (ERROR);
	}

    /* Check for valid maxBytes */

    if (maxBytes <= 0)
	{
	semGive (pRawFd->rawFdSemId);
	errnoSet (S_rawFsLib_INVALID_NUMBER_OF_BYTES);
	return (ERROR);
	}

    semTake (pVd->rawVdSemId, WAIT_FOREVER);	/* take control of volume */

    /* Do successive operation until requested byte count or EOF */

    bytesLeft = maxBytes;	/* init number remaining */
    blkNum = pRawFd->rawFdNewPtr / cbioParams.bytesPerBlk;
    				/* init current block number */
    for (; bytesLeft > 0 && blkNum < cbioParams.nBlocks;
    	   bytesLeft -= nBytes, pBuf += nBytes,
           pRawFd->rawFdNewPtr += nBytes,
           blkNum += numBlks)
	{
	nBytes = 0;		/* init individual read count */

    	/* Do direct whole-block read if possible */

    	if (((ULONG)bytesLeft >= cbioParams.bytesPerBlk) &&
	    (pRawFd->rawFdNewPtr %cbioParams.bytesPerBlk == 0))
	    {
	    /* Calculate number of blocks to access */

	    numBlks = bytesLeft / cbioParams.bytesPerBlk;
    	    nBytes = numBlks * cbioParams.bytesPerBlk;

	    /* Adjust if read would extend past end of device */

	    numBlks = min ((ULONG)numBlks, (cbioParams.nBlocks - blkNum));

	    /* execute operation on blocks */

	    if (rawFsBlkRdWr (pVd, blkNum, numBlks, pBuf, rw) != OK)
	    	{				/* read error */
		if (bytesLeft == maxBytes)	/* if nothing read yet */
		    {
	    	    semGive (pVd->rawVdSemId);
	    	    semGive (pRawFd->rawFdSemId);
		    return (ERROR);	/*  indicate error */
		    }
		else
	    	    break;		/* return number read OK */
	    	}
	    
	    pRawFd->rawFdCookie = (cookie_t) NULL;	/* invalidate cookie */
	    continue;	/* goto partial blocks access */
	    }
    	else		/* if not whole-block transfer*/
	    {
    	    /* partial blocks access */
    	    
    	    numBlks = 1;
    	    offset = pRawFd->rawFdNewPtr % cbioParams.bytesPerBlk;
    	    nBytes = min ((ULONG)bytesLeft,
    	    		  cbioParams.bytesPerBlk - offset);

    	    if ( rawFsBtRdWr (pVd, blkNum, offset, pBuf, nBytes,
    	    		      rw, & pRawFd->rawFdCookie) != OK)
		{	/* probably nothing done */
		if (bytesLeft == maxBytes)
		    {	/* if nothing read yet */
	    	    semGive (pVd->rawVdSemId);
	    	    semGive (pRawFd->rawFdSemId);
		    return (ERROR);
		    }
		else
	    	    break;	/* return number read OK */
		}
	    }
	}  /* end for */


    semGive (pVd->rawVdSemId);	/* release volume */
    semGive (pRawFd->rawFdSemId);	/* release fd */

    return (maxBytes - bytesLeft);	/* number of bytes processed */
    }

/*******************************************************************************
*
* rawFsReadyChange - notify rawFsLib of a change in ready status
*
* This routine sets the volume descriptor state to RAW_VD_READY_CHANGED.
* It should be called whenever a driver senses that a device has come on-line
* or gone off-line, (e.g., a disk has been inserted or removed).
*
* After this routine has been called, the next attempt to use the volume
* will result in an attempted remount.
*
* RETURNS: N/A
*/

void rawFsReadyChange
    (
    RAW_VOL_DESC *pVd         /* pointer to volume descriptor */
    )
    {
    pVd->rawVdState = RAW_VD_READY_CHANGED;
    }

/*******************************************************************************
*
* rawFsReset - reset a volume
*
* This routine calls the specified volume's reset routine, if any.
* If the reset fails, rawVdState is set to (RAW_VD_CANT_RESET)
* to indicate that disk won't reset.
*
* RETURNS: OK, or ERROR if driver returned error.
*/

LOCAL STATUS rawFsReset
    (
    FAST RAW_VOL_DESC   *pVd          /* pointer to volume descriptor */
    )
    {
    if ( cbioIoctl (pVd->rawVdCbio, CBIO_RESET, NULL) != OK)
	{
	pVd->rawVdState = RAW_VD_CANT_RESET;
	return (ERROR);
	}
    
    pVd->rawVdState = RAW_VD_RESET;
    return (OK);
    }

/*******************************************************************************
*
* rawFsSeek - change file descriptor's current character position
*
* This routine sets the specified file descriptor's current character
* position to the specified position.  This only changes the pointer, it
* doesn't affect the hardware at all.
*
* Attempts to set the character pointer to a negative offset (i.e. before
* the start of the disk) or to a point beyond the end of the device
* will return ERROR.
*
* RETURNS: OK, or ERROR if invalid character offset.
*/

LOCAL STATUS rawFsSeek
    (
    FAST RAW_FILE_DESC  *pRawFd,        /* file descriptor pointer */
    fsize_t             position      /* desired character position in file */
    )
    {
    /*
     * Check for valid new offset. Accept any positive offset.
     * volume end overgoing is checked within read/write
     */
    if (position < 0)
	{
	errnoSet (S_rawFsLib_BAD_SEEK);
	return (ERROR);
	}

    /* Update new file byte pointer */

    pRawFd->rawFdNewPtr = position;

    return (OK);
    }

/*******************************************************************************
*
* rawFsVolCheck - verify that volume descriptor is current
*
* This routine is called at the beginning of most operations on
* the device.  The status field in the volume descriptor is examined,
* and the appropriate action is taken.  In particular, the disk is
* mounted if it is currently unmounted or a ready-change has occurred.
*
* If the disk is already mounted or is successfully mounted as a
* result of this routine calling rawFsVolMount, this routine returns
* OK.  If the disk cannot be mounted, ERROR is returned.
*
* RETURNS: OK, or ERROR.
*/

LOCAL STATUS rawFsVolCheck
    (
    FAST RAW_VOL_DESC   *pVd          /* pointer to volume descriptor   */
    )
    {
    FAST int		status;		/* return status value */


    status = OK;				/* init return value */

    /* Check if device driver announced ready-change */

    if ( cbioIoctl (
    		pVd->rawVdCbio, CBIO_STATUS_CHK, NULL) != OK ||
    	TRUE == cbioRdyChgdGet (pVd->rawVdCbio))
	{
	pVd->rawVdState = RAW_VD_READY_CHANGED;
	}

    /* Check volume status */

    switch (pVd->rawVdState)
	{
	case RAW_VD_MOUNTED:
	    break;				/* ready to go as is */

	case RAW_VD_CANT_RESET:
	    /* This state means we tried to reset and failed.
	     * try again
	     */

	case RAW_VD_CANT_MOUNT:
	    /* This state means we tried to mount and failed.
	     * try again
	     */

	case RAW_VD_RESET:
	    /* Volume reset but not mounted; mount */

	case RAW_VD_READY_CHANGED:
	    /* Ready change occurred; remount volume */

	    if (rawFsVolMount (pVd) != OK)
		{
		pVd->rawVdState = RAW_VD_CANT_MOUNT; /* can't mount */
		status = ERROR;		 	/* don't try again */
		break;
		}

	    pVd->rawVdState = RAW_VD_MOUNTED;	/* note vol mounted */
	    break;
	}

    return (status);				/* return status value */
    }

/*******************************************************************************
*
* rawFsVolFlush - flush raw volume to disk
*
* This routine guarantees that any changes to the volume are actually
* written to disk.  Since there are no data structures kept on disk,
* the only possible changes are the file descriptor read/write buffers.
*
* INTERNAL
* Possession of the volume descriptor's semaphore must have been secured
* before this routine is called.
*
* RETURNS: OK, or ERROR.
*/

LOCAL STATUS rawFsVolFlush
    (
    FAST RAW_VOL_DESC   *pVd          /* pointer to volume descriptor */
    )
    {
    FAST RAW_FILE_DESC	*pRawFd;	/* pointer to file descriptor */

    if (pVd->rawVdState != RAW_VD_MOUNTED)
    	{
    	return (ERROR);
    	}
    
    /* Flush all modified file descriptor I/O buffers to disk */

    semTake (rawFsFdListSemId, WAIT_FOREVER);	/* take control of Fd lists */

    pRawFd = (RAW_FILE_DESC *) lstFirst (&rawFsFdActiveList);
						/* get first in-use Fd */

    while (pRawFd != NULL)
	{
	if (pRawFd->pRawFdVd == pVd)	/* if correct volume */
	    {
	    if (rawFsFdFlush (pRawFd) != OK)	/* write Fd buffer to disk */
	    	return (ERROR);			/* error flushing buffer */
	    }

	pRawFd = (RAW_FILE_DESC *) lstNext (&pRawFd->rawFdNode);
						/* get next in-use Fd */
	}

    semGive (rawFsFdListSemId);			/* release Fd lists */


    return (OK);
    }

/*******************************************************************************
*
* rawFsVolMount - prepare to use raw volume
*
* This routine prepares the library to use the raw volume on the device
* specified.
*
* This routine should be called every time a disk is changed (i.e. a floppy
* is swapped, or whatever), or before every open and create, if the driver
* can't tell when disks are changed.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS rawFsVolMount
    (
    FAST RAW_VOL_DESC   *pVd	/* pointer to volume descriptor */
    )
    {
    semTake (pVd->rawVdSemId, WAIT_FOREVER);	/* get ownership of volume */
    
    /* unmount volume prior mounting */
    
    if (pVd->rawVdStatus == OK)
    	{
    	rawFsVolUnmount (pVd);
    	}
    
    /* update new volume parameters */
    
    if (rawFsReset (pVd) == OK)
    	{
    	pVd->rawVdStatus = OK;
    	pVd->rawVdState = RAW_VD_MOUNTED;
    	}
    
    semGive (pVd->rawVdSemId);		/* release volume */
    
    return (pVd->rawVdStatus);
    }

/*******************************************************************************
*
* rawFsVolUnmount - disable a raw device volume
*
* This routine is called when I/O operations on a volume are to be
* discontinued.  This is commonly done before changing removable disks.
* All buffered data for the volume is written to the device (if possible),
* any open file descriptors are marked as obsolete, and the volume is
* marked as not mounted.
*
* Because this routine will flush data from memory to the physical
* device, it should not be used in situations where the disk-change is
* not recognized until after a new disk has been inserted.  In these
* circumstances, use the ready-change mechanism.  (See the manual entry for 
* rawFsReadyChange().)
*
* This routine may also be called by issuing an ioctl() call using the
* \%FIOUNMOUNT function code.
*
* RETURNS: OK, or ERROR if the routine cannot access the volume.
*
* SEE ALSO: rawFsReadyChange()
*/

STATUS rawFsVolUnmount
    (
    FAST RAW_VOL_DESC   *pVd          /* pointer to volume descriptor */
    )
    {
    FAST RAW_FILE_DESC	*pRawFd;	/* pointer to file descriptor */


    /* Check that volume is available */

    semTake (pVd->rawVdSemId, WAIT_FOREVER);	/* get ownership of volume */
    
    /* nothing to be done, if volume was not mounted since last unmount */
    
    if (pVd->rawVdStatus != OK)
        {
        semGive (pVd->rawVdSemId);	/* release volume */
        return (OK);			/* cannot access volume */
        }

    /* do not check device state before flush, let flush just fail */
    
    /* Flush volume structures, if possible */

    (void) rawFsVolFlush (pVd);	/* flush volume */

    /* Mark any open file descriptors as obsolete */

    semTake (rawFsFdListSemId, WAIT_FOREVER);	/* take control of Fd lists */

    pRawFd = (RAW_FILE_DESC *) lstFirst (&rawFsFdActiveList);
					/* get first in-use Fd */

    while (pRawFd != NULL)
	{
	if (pRawFd->pRawFdVd == pVd)
	    {
	    pRawFd->rawFdStatus = RAWFD_OBSOLETE;
	    }

	pRawFd = (RAW_FILE_DESC *) lstNext (&pRawFd->rawFdNode);
						/* get next in-use Fd */
	}

    semGive (rawFsFdListSemId);			/* release Fd lists */


    /* Mark volume as no longer mounted */

    rawFsReadyChange (pVd);
    pVd->rawVdStatus = ERROR;


    semGive (pVd->rawVdSemId);		/* release volume */

    return (OK);
    }

/*******************************************************************************
*
* rawFsWhere - return current character position on raw device
*
* This routine tells you where the character pointer is for a raw device.
* This character position applies only to the specified file descriptor;
* other file descriptor to the save raw device may have different character
* pointer values.
*
* RETURNS: Current character position of file descriptor.
*/

LOCAL fsize_t rawFsWhere
    (
    FAST RAW_FILE_DESC *pRawFd  /* file descriptor pointer */
    )
    {

    return (pRawFd->rawFdNewPtr);
    }

/*******************************************************************************
*
* rawFsRead - read from a raw volume
*
* This routine reads from the volume specified by the file descriptor
* (returned by rawFsOpen()) into the specified buffer.
* <maxbytes> bytes will be read, if there is that much data in the file.
*
* RETURNS: Number of bytes actually read, or 0 if end of file, or ERROR.
*/

LOCAL int rawFsRead
    (
    FAST RAW_FILE_DESC  *pRawFd,	/* file descriptor pointer */
    char                *pBuf,		/* addr of input buffer */
    int                 maxBytes	/* maximum bytes to read into buffer */
    )
    {
    return rawFdRdWr (pRawFd, pBuf, maxBytes, CBIO_READ);
    }
    
/*******************************************************************************
*
* rawFsWrite - write to a raw volume
*
* This routine writes to the raw volume specified by the file descriptor
* from the specified buffer.  If the block containing the disk locations
* to be written is already in the file descriptor's read/write buffer,
* the buffer won't be flushed.  If another in-memory block is needed,
* any block already in memory will be flushed.
*
* This routine calls the device driver block-writing routine directly
* to write (possibly multiple) whole blocks.
*
* RETURNS: Number of bytes written (error if != maxBytes), or
* ERROR if maxBytes < 0, or no more space for the file,
* or can't write block.
*/

LOCAL int rawFsWrite
    (
    FAST RAW_FILE_DESC  *pRawFd,        /* file descriptor pointer */
    char                *pBuf,          /* data to be written */
    int                 maxBytes        /* number of bytes to write */
    )
    {
    return rawFdRdWr (pRawFd, pBuf, maxBytes, CBIO_WRITE);
    }

/* tapeFsLib.c - tape sequential device file system library */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,21feb99,jdi  doc: listed errnos.
01e,06mar97,dds  SPR 8120: fixed tapeFsRead to return the actual number
                 of bytes read, 0 if end of file, or ERROR.
01d,29oct96,dgp  doc: editing for newly published SCSI libraries
01c,08may96,jds  more doc changes
01b,01may96,jds  doc changes
01a,17jul95,jds  written
*/

/*
This library provides basic services for tape devices that do not
use a standard file or directory structure on tape.  The tape volume is treated
much like a large file. The tape may either be read or written. However, there
is no high-level organization of the tape into files or directories, 
which must be provided by a higher-level layer.

USING THIS LIBRARY
The various routines provided by the VxWorks tape file system, or tapeFs, can
be categorized into three broad groupings: general initialization,
device initialization, and file system operation.

The tapeFsInit() routine is the principal general initialization function;
it needs to be called only once, regardless of how many tapeFs devices
are used.

To initialize devices, tapeFsDevInit() must be called for each tapeFs device.

Use of this library typically occurs through standard use of the I/O system
routines open(), close(), read(), write() and ioctl().  Besides these 
standard I/O system operations, several routines are provided to inform the 
file system of changes in the system environment.  
The tapeFsVolUnmount() routine informs the file system that a particular 
device should be unmounted; any synchronization should be done prior to 
invocation of this routine, in preparation for a tape volume change. 
The tapeFsReadyChange() routine is used to inform the file system that a
tape may have been swapped and that the next tape operation should first
remount the tape. Information about a ready-change is also obtained from the 
driver using the SEQ_DEV device structure. Note that tapeFsVolUnmount() and
tapeFsReadyChange() should be called only after a file has been closed.


INITIALIZATION OF THE FILE SYSTEM
Before any other routines in tapeFsLib can be used, tapeFsInit() 
must be called to initialize the library. This implementation
of the tape file system assumes only one file descriptor per volume. However,
this constraint can be changed in case a future implementation demands
multiple file descriptors per volume.

During the tapeFsInit() call, the tape device library is installed as a driver
in the I/O system driver table.  The driver number associated with it is
then placed in a global variable, `tapeFsDrvNum'.

To enable this initialization, define INCLUDE_TAPEFS in the BSP, or simply 
start using the tape file system with a call to tapeFsDevInit() and 
tapeFsInit() will be called automatically if it has not been called before. 

DEFINING A TAPE DEVICE
To use this library for a particular device, the device structure
used by the device driver must contain, as the very first item, a
sequential device description structure (SEQ_DEV).  The SEQ_DEV must be 
initialized before calling tapeFsDevInit(). The driver places in the SEQ_DEV 
structure the addresses of routines that it must supply:  one that reads one
or more blocks, one that writes one or more blocks, one that performs
I/O control (ioctl()) on the device, one that writes file marks on a tape, 
one that rewinds the tape volume, one that reserves a tape device for use,
one that releases a tape device after use, one that mounts/unmounts a volume,
one that spaces forward or backwards by blocks or file marks, one that erases
the tape, one that resets the tape device, and one that checks 
the status of the device.
The SEQ_DEV structure also contains fields that describe the physical 
configuration of the device.  For more information about defining sequential
devices, see the 
.I "VxWorks Programmer's Guide: I/O System."

INITIALIZATION OF THE DEVICE 
The tapeFsDevInit() routine is used to associate a device with the tapeFsLib
functions.  The `volName' parameter expected by tapeFsDevInit() is a pointer to
a name string which identifies the device.  This string serves as
the pathname for I/O operations which operate on the device and  
appears in the I/O system device table, which can be displayed
using iosDevShow().

The `pSeqDev' parameter expected by tapeFsDevInit() is a pointer
to the SEQ_DEV structure describing the device and containing the
addresses of the required driver functions.  

The `pTapeConfig' parameter is a pointer to a TAPE_CONFIG structure that 
contains information specifying how the tape device should be configured. The 
configuration items are fixed/variable block size, rewind/no-rewind device,
and number of file marks to be written. For more information about the 
TAPE_CONFIG structure, look at the header file tapeFsLib.h. 

The syntax of the tapeFsDevInit() routine is as follows:
.CS
    tapeFsDevInit
	(
	char *        volName,     /@ name to be used for volume   @/
	SEQ_DEV *     pSeqDev,     /@ pointer to device descriptor @/
	TAPE_CONFIG * pTapeConfig  /@ pointer to tape config info  @/
	)
.CE

When tapeFsLib receives a request from the I/O system, after tapeFsDevInit()
has been called, it calls the device driver routines (whose addresses were
passed in the SEQ_DEV structure) to access the device.

OPENING AND CLOSING A FILE
A tape volume is opened by calling the I/O system routine open(). A file can
be opened only with the O_RDONLY or O_WRONLY flags. The O_RDWR mode is not 
used by this library. A call to
open() initializes the file descriptor buffer and state information, reserves
the tape device, rewinds the tape device if it was configured as a rewind
device, and mounts a volume. Once a tape volume has been opened, that tape
device is reserved, disallowing any other system from accessing that device 
until the tape volume is closed. Also, the single file descriptor is marked
"in use" until the file is closed, making sure that a file descriptor is
not opened multiple times.

A tape device is closed by calling the I/O system routine close(). Upon a 
close() request, any unwritten
buffers are flushed, the device is rewound (if it is a rewind device), and, 
finally, the device is released.

UNMOUNTING VOLUMES (CHANGING TAPES)
A tape volume should be unmounted before it is removed.  When unmounting a 
volume, make sure that any open file is closed first.
A tape may be unmounted by calling tapeFsVolUnmount() directly.

If a file is open, it is not correct to change the medium and continue with 
the same file descriptor still open.
Since tapeFs assumes only one file descriptor per device, to reuse 
that device, the file must be closed and opened later for the new tape volume.

Before tapeFsVolUnmount() is called, the device should be synchronized by 
invoking the ioctl() FIOSYNC or FIOFLUSH. It is the responsibility
of the higher-level layer to synchronize the tape file system before 
unmounting. Failure to synchronize the volume before unmounting may result
in loss of data.


IOCTL FUNCTIONS
The VxWorks tape sequential device file system supports the following ioctl()
functions.  The functions listed are defined in the header files ioLib.h and 
tapeFsLib.h.

.iP "FIOFLUSH" 16 3
Writes all modified file descriptor buffers to the physical device.
.CS
    status = ioctl (fd, FIOFLUSH, 0);
.CE
.iP "FIOSYNC"
Performs the same function as FIOFLUSH.
.iP "FIOBLKSIZEGET"
Returns the value of the block size set on the physical device. This value
is compared against the 'sd_blkSize' value set in the SEQ_DEV device structure.
.iP "FIOBLKSIZESET"
Sets a specified block size value on the physical device and also updates the
value in the SEQ_DEV and TAPE_VOL_DESC structures, unless the supplied value
is zero, in which case the device structures are updated but the device 
is not set to zero. This is because zero implies variable block operations,
therefore the device block size is ignored.
.iP "MTIOCTOP"
Allows use of the standard UNIX MTIO `ioctl' operations
by means of the MTOP structure. The MTOP structure appears as follows:
.CS
typedef struct mtop
    {
    short       mt_op;                  /@ operation @/
    int         mt_count;               /@ number of operations @/
    } MTOP;
.CE

Use these ioctl() operations as follows:
.CS
    MTOP mtop;

    mtop.mt_op    = MTWEOF;
    mtop.mt_count = 1;
    status = ioctl (fd, MTIOCTOP, (int) &mtop);
.CE

.LP
The permissable values for 'mt_op' are:

.iP "MTWEOF"
Writes an end-of-file record to tape. An end-of-file record is a file mark.

.iP "MTFSF"
Forward space over a file mark and position the tape head in the gap between
the file mark just skipped and the next data block.
Any buffered data is flushed out to the tape if the tape is in write
mode.

.iP "MTBSF"
Backward space over a file mark and position the tape head in the gap 
preceeding the file mark, that is, right before the file mark.
Any buffered data is flushed out to the tape if the tape is in write
mode.

.iP "MTFSR"
Forward space over a data block and position the tape head in the gap between
the block just skipped and the next block.
Any buffered data is flushed out to the tape if the tape is in write
mode.

.iP "MTBSR"
Backward space over a data block and position the tape head right before the
block just skipped.
Any buffered data is flushed out to the tape if the tape is in write
mode.

.iP "MTREW"
Rewind the tape to the beginning of the medium.
Any buffered data is flushed out to the tape if the tape is in write
mode.

.iP "MTOFFL"
Rewind and unload the tape.
Any buffered data is flushed out to the tape if the tape is in write
mode.

.iP "MTNOP"
No operation, but check the status of the device, thus setting the appropriate
SEQ_DEV fields.

.iP "MTRETEN"
Retension the tape. This command usually sets tape tension and can be
used in either read or write mode.
Any buffered data is flushed out to tape if the tape is in write
mode.

.iP "MTERASE"
Erase the entire tape and rewind it.

.iP "MTEOM"
Position the tape at the end of the medium and unload the tape.
Any buffered data is flushed out to the tape if the tape is in write
mode.


INCLUDE FILES: tapeFsLib.h

SEE ALSO: ioLib, iosLib,
.pG "I/O System, Local File Systems"
*/

#include "vxWorks.h"
#include "ctype.h"
#include "ioLib.h"
#include "lstLib.h"
#include "stdlib.h"
#include "string.h"
#include "semLib.h"
#include "errnoLib.h"
#include "logLib.h"
#include "tapeFsLib.h"


/* GLOBALS */
int 	tapeDebug = FALSE;
#define TAPE_DEBUG_MSG if (tapeDebug) \
		            logMsg

int 	tapeFsDrvNum   = NONE;	   /* I/O system driver number for tapeFsLib */

					/* default mutex options */
int	tapeFsVolMutexOptions    = (SEM_Q_PRIORITY | SEM_DELETE_SAFE);


/* forward static functions */

LOCAL int	       tapeFsDevRd (TAPE_VOL_DESC * pTapeVol, UINT numBlks, 
						char * pBuf, BOOL fixed);
LOCAL STATUS	       tapeFsBlkWrt (TAPE_VOL_DESC * pTapeVol, int numBlks, 
						char * pBuf, BOOL fixed);
LOCAL STATUS	       tapeFsClose (TAPE_FILE_DESC * pTapeFd);
LOCAL STATUS	       tapeFsFdFlush (TAPE_FILE_DESC * pTapeFd);
LOCAL STATUS	       tapeFsIoctl (TAPE_FILE_DESC * pTapeFd, int function, 
								int arg);
LOCAL int	       tapeFsRead (TAPE_FILE_DESC * pTapeFd, char * pBuf, 
								UINT maxBytes);
LOCAL int 	       tapeFsPartWrt (TAPE_FILE_DESC * pTapeFd, char * pBuf, 
								int nBytes);
LOCAL STATUS	       tapeFsVolCheck (TAPE_VOL_DESC * pTapeVol);
LOCAL STATUS	       tapeFsVolMount (TAPE_VOL_DESC * pTapeVol);
LOCAL int	       tapeFsWrite (TAPE_FILE_DESC * pTapeFd, char * pBuf,
								int maxBytes);
LOCAL int 	       tapeFsPartRd (TAPE_FILE_DESC * pTapeFd, char * pBuf, 
								UINT nBytes);
LOCAL STATUS 	       mtOpHandle (FAST TAPE_FILE_DESC * pTapeFd, int op, 
								int numOps);
LOCAL TAPE_FILE_DESC * tapeFsOpen (TAPE_VOL_DESC * pTapeVol, char * name, 
							int flags, int mode);


/*******************************************************************************
*
* tapeFsDevRd - read blocks or bytes from a tape volume
*
* This routine reads the specified block or bytes from the specified volume.
*
* RETURNS: Number of bytes or blocks read, 0 if EOF, or error
*/

LOCAL int tapeFsDevRd
    (
    FAST TAPE_VOL_DESC * pTapeVol,      /* pointer to volume descriptor      */
    UINT                 numDataUnits,  /* how many (blocks | bytes) to read */
    FAST char *          pBuf,          /* buffer to receive block           */
    BOOL		 fixed	        /* FIXED_BLK or VARIABLE_BLK read    */
    )
    {
    /* Pointer to sequential device struct */
    
    SEQ_DEV	*pSeqDev = pTapeVol->tapevd_pSeqDev;
    
    if (pSeqDev->sd_seqRd == NULL)
	{
	errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
	return (ERROR);
	}
    
    /* Try to read the data once */

    return ((* pSeqDev->sd_seqRd) (pSeqDev, numDataUnits, pBuf, fixed));
    }

/*******************************************************************************
*
* tapeFsBlkWrt - write block(s) to a tape volume
*
* This routine writes the specified blocks to the specified volume.
*
* RETURNS:
* OK, or
* ERROR, if write error. 
*/

LOCAL STATUS tapeFsBlkWrt
    (
    FAST TAPE_VOL_DESC * pTapeVol,      /* pointer to volume descriptor       */
    int                  numDataUnits,  /* how many (blocks | bytes) to write */
    char *               pBuf,          /* pointer to buffer to write to tape */
    BOOL	         fixed		/* FIXED_BLK or VARIABLE_BLK write    */
    )
    {
    /* Pointer to sequential device struct */

    FAST SEQ_DEV	*pSeqDev = pTapeVol->tapevd_pSeqDev;

    if (pSeqDev->sd_seqWrt == NULL)
	{
	errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
	return (ERROR);
	}

    /* Try to write the data once */

    if (((* pSeqDev->sd_seqWrt) (pSeqDev, numDataUnits, pBuf, fixed)) != OK)
        return (ERROR);			

    return (OK);
    }

/*******************************************************************************
*
* tapeFsClose - close tape volume
*
* This routine closes the specified tape volume file descriptor.  Any
* buffered data for the descriptor is written to the physical device.
*
* If the file descriptor has been marked as obsolete (meaning the
* volume was unmounted while the descriptor was open), the descriptor
* is freed, but an error is returned.
*
* RETURNS: OK, or ERROR.
*/

LOCAL STATUS tapeFsClose
    (
    FAST TAPE_FILE_DESC  *pTapeFd         /* file descriptor pointer */
    )
    {
    FAST STATUS	         status;

    /* Pointer to volume descriptor */

    FAST TAPE_VOL_DESC * pTapeVol = pTapeFd->tapefd_pTapeVol;

    /* Pointer to sequential device */

    FAST SEQ_DEV *       pSeqDev  = pTapeVol->tapevd_pSeqDev;


    semTake (pTapeVol->tapevd_semId, WAIT_FOREVER); /* get ownership of vol */

    /* Write out file descriptor buffer, if WRONLY mode and fixed block */

    if ((pTapeFd->tapefd_mode == O_WRONLY) && (pTapeVol->tapevd_blkSize > 0))
	{
        if (tapeFsFdFlush (pTapeFd) != OK)
	    goto closeFailed;
	}

    /* Rewind if neccessary */

    if (pTapeVol->tapevd_rewind)
	{
	if (pSeqDev->sd_rewind == NULL)
	    goto closeFailed;

	if ((*(pSeqDev->sd_rewind)) (pSeqDev) != OK)
	    goto closeFailed;
	}

    /* Release the device */

    if (pSeqDev->sd_release == NULL)
	goto closeFailed;

    status = (*(pSeqDev->sd_release)) (pSeqDev);
    

    pTapeFd->tapefd_bufSize = 0;		/* disable buffer        */
    pTapeFd->tapefd_inUse   = FALSE;		/* fd not in use anymore */
    semGive (pTapeVol->tapevd_semId);		/* release volume        */
    free (pTapeFd->tapefd_buffer);		/* free file I/O buffer  */

    return (status);

closeFailed:
    semGive (pTapeVol->tapevd_semId);		/* release volume */
    return (ERROR);
    }

/*******************************************************************************
*
* tapeFsDevInit - associate a sequential device with tape volume functions
*
* This routine takes a sequential device created by a device driver and
* defines it as a tape file system volume.  As a result, when high-level 
* I/O operations, such as open() and write(), are performed on the
* device, the calls will be routed through tapeFsLib.
*
* This routine associates `volName' with a device and installs it in
* the VxWorks I/O system-device table.  The driver number used when
* the device is added to the table is that which was assigned to the
* tape library during tapeFsInit().  (The driver number is kept in
* the global variable `tapeFsDrvNum'.)
*
* The SEQ_DEV structure specified by `pSeqDev' contains configuration
* data describing the device and the addresses of the routines which
* are called to read blocks, write blocks, write file marks, reset the 
* device, check device status, perform other I/O control functions (ioctl()),
* reserve and release devices, load and unload devices, and rewind devices.
* These routines are not called until they are required by subsequent I/O
* operations.  The TAPE_CONFIG structure is used to define configuration
* parameters for the TAPE_VOL_DESC.  The configuration parameters are defined
* and described in tapeFsLib.h.
*
* INTERNAL
* The semaphore in the device is given, so the device is available for
* use immediately.
*
* RETURNS: A pointer to the volume descriptor (TAPE_VOL_DESC),
* or NULL if there is an error.
*
* ERRNO: S_tapeFsLib_NO_SEQ_DEV, S_tapeFsLib_ILLEGAL_TAPE_CONFIG_PARM
*/

TAPE_VOL_DESC *tapeFsDevInit
    (
    char *             volName,    	/* volume name                       */
    FAST SEQ_DEV *     pSeqDev,    	/* pointer to sequential device info */
    FAST TAPE_CONFIG * pTapeConfig	/* pointer to tape config info       */
    )
    {
    FAST TAPE_VOL_DESC * pTapeVol;	/* pointer to new volume descriptor */


    /* Initialise the tape file system if it has not been initialised before */

    if (tapeFsDrvNum == NONE)
	 tapeFsInit ();

    /* Return error if no SEQ_DEV */

    if (pSeqDev == NULL)
	{
	errno = S_tapeFsLib_NO_SEQ_DEV;
	TAPE_DEBUG_MSG ("tapeFsDevInit: pSeqDev null\n",0,0,0,0,0,0);
	return (NULL);
	}

    /* Check configuration parameters */

    if ((pTapeConfig->blkSize < 0) /* XXX || blkSize > some MAX*/)
	{
	errno = S_tapeFsLib_ILLEGAL_TAPE_CONFIG_PARM;
	TAPE_DEBUG_MSG ("tapeFsDevInit: blkSize < 0\n",0,0,0,0,0,0);
	return (NULL);
	}

    if (pTapeConfig->numFileMarks < 0)
	{
	errno = S_tapeFsLib_ILLEGAL_TAPE_CONFIG_PARM; 
	TAPE_DEBUG_MSG ("tapeFsDevInit: numFileMarks < 0\n",0,0,0,0,0,0);
	return (NULL);
	}

    if ((pTapeConfig->rewind != TRUE) && (pTapeConfig->rewind != FALSE))
	{
	errno = S_tapeFsLib_ILLEGAL_TAPE_CONFIG_PARM;
	TAPE_DEBUG_MSG ("tapeFsDevInit: rewind flag wrong\n",0,0,0,0,0,0);
	return (NULL);
	}

    /* Allocate a tape volume descriptor for device */

    if ((pTapeVol = (TAPE_VOL_DESC *) calloc 
					(sizeof (TAPE_VOL_DESC), 1)) == NULL)
	return (NULL);				/* no memory */

    TAPE_DEBUG_MSG ("tapeFsDevInit: pTapeVol is %x\n",(int)pTapeVol,0,0,0,0,0);

    /* Create volume locking semaphore (initially available) */

    pTapeVol->tapevd_semId = semMCreate (tapeFsVolMutexOptions);
    if (pTapeVol->tapevd_semId == NULL)
	{
	free ((char *) pTapeVol);
	return (NULL);				/* could not create semaphore */
	}

    /* Allocate a File Descriptor (FD) for this volume */

    pTapeVol->tapevd_pTapeFd = (TAPE_FILE_DESC *) calloc 
						  (sizeof (TAPE_FILE_DESC), 1);
    if (pTapeVol->tapevd_pTapeFd == NULL) 
	{
	free ((char *) pTapeVol);
	return (NULL);				/* could not allocate FD */
	}

    /* Add device to system device table */

    if (iosDevAdd ((DEV_HDR *) pTapeVol, volName, tapeFsDrvNum) != OK)
	{
	TAPE_DEBUG_MSG ("tapeFsDevInit: iosDevAdd error\n",0,0,0,0,0,0);
	free ((char *) pTapeVol->tapevd_pTapeFd);
	free ((char *) pTapeVol);
	return (NULL);				/* can't add device */
	}

    /* Initialize volume descriptor */

    pTapeVol->tapevd_pSeqDev      = pSeqDev;
    pTapeVol->tapevd_status	  = OK;
    pTapeVol->tapevd_state	  = TAPE_VD_READY_CHANGED;
    pTapeVol->tapevd_rewind	  = pTapeConfig->rewind;
    pTapeVol->tapevd_blkSize	  = pTapeConfig->blkSize;
    pTapeVol->tapevd_numFileMarks = pTapeConfig->numFileMarks;
    pTapeVol->tapevd_density      = pTapeConfig->density;
    pSeqDev->sd_blkSize           = pTapeConfig->blkSize;

    /* File descriptor initialization */

    pTapeVol->tapevd_pTapeFd->tapefd_inUse = FALSE;

    return (pTapeVol);
    }

/*******************************************************************************
*
* tapeFsFdFlush - flush tape volume file descriptor I/O buffer to tape
*
* This routine causes the I/O buffer of a tape volume file descriptor
* to be written out to the physical device. It is assumed that the volume
* is of fixed block type.
*
* Note: This function should only be called for file descriptors opened with
*       the O_WRONLY mode
*
* RETURNS: OK, or ERROR if something could not be written out.
*/

LOCAL STATUS tapeFsFdFlush
    (
    FAST TAPE_FILE_DESC  *pTapeFd         /* pointer to file descriptor */
    )
    {
				
    FAST int 	   index   = pTapeFd->tapefd_bufIndex;  /* buffer index */
    /* ptr to sequential device */
    FAST SEQ_DEV * pSeqDev = pTapeFd->tapefd_pTapeVol->tapevd_pSeqDev;
    /* seq device block Size */
    FAST int	   blkSize = pSeqDev->sd_blkSize;


    if (pTapeFd->tapefd_bufIndex != 0)
	{
	/* Pad partial blocks with zeros */

	bzero (pTapeFd->tapefd_buffer + index, blkSize - index); 

	/* Write out the current (dirty) block */

	if (tapeFsBlkWrt (pTapeFd->tapefd_pTapeVol, 1,
		       pTapeFd->tapefd_buffer, TRUE) != OK)
	    return (ERROR);			/* write error */

	/* Reset the buffer index so as to invalidate the buffer */

	pTapeFd->tapefd_bufIndex = 0;
	}

    return (OK);
    }

/*******************************************************************************
*
* tapeFsInit - initialize the tape volume library
*
* This routine initializes the tape volume library.  It must be called exactly
* once, before any other routine in the library. Only one file descriptor
* per volume is assumed. 
*
* This routine also installs tape volume library routines in the VxWorks I/O
* system driver table.  The driver number assigned to tapeFsLib is placed in
* the global variable `tapeFsDrvNum'.  This number is later associated
* with system file descriptors opened to tapeFs devices.
*
* To enable this initialization, simply call the routine tapeFsDevInit(), which
* automatically calls tapeFsInit() in order to initialize the tape file system.
*
* RETURNS: OK or ERROR.
*/

STATUS tapeFsInit ()
    {

    /*
     * Install tapeFsLib routines in I/O system driver table
     * Note: there is no delete routine, and that the
     *       tapeFsOpen routine is also used as the create function.
     */

    tapeFsDrvNum = iosDrvInstall ((FUNCPTR) tapeFsOpen, (FUNCPTR) NULL,
			         (FUNCPTR) tapeFsOpen, tapeFsClose,
			         tapeFsRead, tapeFsWrite, tapeFsIoctl);
    if (tapeFsDrvNum == ERROR)
	return (ERROR);				/* can't install as driver */

    return (OK);
    }

/*******************************************************************************
*
* tapeFsIoctl - perform a device-specific control function
*
* This routine performs the following ioctl() functions:
*
* .CS
*    MTIOCTOP 	     - Perform all UNIX MTIO operations. Use the MTIO structure
*		       to pass the correct operation and operation count.
*    FIOSYNC 	     - Write all modified file descriptor buffers to device.
*    FIOFLUSH        - Same as FIOSYNC.
*    FIOBLKSIZESET   - Set the device block size (0 implies variable blocks)
*    FIOBLKSIZEGET   - Get the device block size and compare with tapeFs value
* .CE
*
* If the ioctl (`function') is not one of the above, the device driver
* ioctl() routine is called to perform the function.
*
* If an ioctl() call fails, the task status (see errnoGet()) indicates
* the nature of the error.
*
* RETURNS: OK, or ERROR if function failed or unknown function, or
* current byte pointer for FIOWHERE.
*/

LOCAL STATUS tapeFsIoctl
    (
    FAST TAPE_FILE_DESC * pTapeFd,      /* file descriptor of file to control */
    int                   function,     /* function code                      */
    int                   arg           /* some argument                      */
    )
    {
    FAST STATUS		status;		/* return status value 		      */
    FAST TAPE_VOL_DESC *pTapeVol = pTapeFd->tapefd_pTapeVol;
    FAST SEQ_DEV *    	pSeqDev  = pTapeVol->tapevd_pSeqDev;
    FAST MTOP *		pMtOp    = (MTOP *) arg;   /* ptr to operation struct */


    switch (function)
	{
	case MTIOCTOP:

	    /* Handle MTIO MTIOCTOP operations */

	    status = mtOpHandle (pTapeFd, pMtOp->mt_op, pMtOp->mt_count);
            break;

        
        case FIOFLUSH:
        case FIOSYNC:

            /* Check if the file mode is correct */

            if (pTapeFd->tapefd_mode == O_RDONLY)
                {
                errno = EROFS;		/* posix errno */
                status = ERROR;
                break;
                }

            /* Flush any buffered data */

            status = tapeFsFdFlush (pTapeFd);  /* errno set by lower layer */

            break;

        case FIOBLKSIZESET:
            
	    if (pSeqDev->sd_ioctl == NULL)
                {
                errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
		status = ERROR;
                break;
		}
	    
	    if (arg != 0)		/* Variable block size */
		{
	        /* Call driver's ioctl routine with function and arg */

	        status = (*pSeqDev->sd_ioctl) (pSeqDev, function, arg);
		}
            else
		status = OK;

	    if (status == ERROR)
		break;			/* do not modify any device structs */

	    /* Set the block Size in the tapeFs device structures */

            pTapeVol->tapevd_blkSize = arg;
	    pSeqDev->sd_blkSize      = arg;

            break;

        case FIOBLKSIZEGET:

            if (pSeqDev->sd_ioctl == NULL)
                {
                errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
                status = ERROR;
                break;
                }
   
            /* Perform an FIOBLKSIZEGET ioctl on the driver */

            status = (*pSeqDev->sd_ioctl) (pSeqDev, function, arg);

            /*
             * If an ERROR was not returned, then the value returned is a 
             * valid block size, and this value should be compared with
             * that set in the device and volume structures.
             */

            if ((status != ERROR) && (pSeqDev->sd_blkSize != 0))
                {
                if  ( (pSeqDev->sd_blkSize != status) || 
                      (pTapeVol->tapevd_blkSize != status)
                    )
                    {
                    errno = S_tapeFsLib_BLOCK_SIZE_MISMATCH;
                    status = ERROR;
                    break;
                    }
                }

            break;

	default:

	    if (pSeqDev->sd_ioctl == NULL)
                {
                errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
		status = ERROR;
                break;
		}
	    
	    /* Call driver's ioctl routine with function and arg */

	    status = (*pSeqDev->sd_ioctl) (pSeqDev, function, arg);
            break;

	} /* switch */

        return (status);

    }

/*******************************************************************************
*
* mtOpHandle - Handles MTIOCTOP type ioctl operations
*
* This routine performs the following ioctl() functions:
*
*	MTWEOF		- write an end-of-file record 
*	MTFSF		- forward space over file mark 
*	MTBSF		- backward space over file mark 
*	MTFSR		- forward space to inter-record gap 
*	MTBSR		- backward space to inter-record gap 
*	MTREW		- rewind 
*	MTOFFL		- rewind and put the drive offline
*	MTNOP		- no operation, sets status only
*	MTRETEN		- retension the tape (use for cartridge tape only)
*	MTERASE		- erase the entire tape
*	MTEOM		- position to end of media
*	MTNBSF		- backward space file to BOF
*
* If the ioctl (`function') is not one of the above, the device driver
* ioctl() routine is called to perform the function.
*
* If an ioctl() call fails, the task status (see errnoGet()) indicates
* the nature of the error.
*
* RETURNS: OK, or ERROR if function failed or unknown function, or
* current byte pointer for FIOWHERE.
*/
LOCAL STATUS mtOpHandle
    (
    FAST TAPE_FILE_DESC * pTapeFd,    /* file descriptor of file to control */
    int                   op,         /* function code                      */
    int                   numOps      /* some argument                      */
    )
    {
    FAST TAPE_VOL_DESC *pTapeVol = pTapeFd->tapefd_pTapeVol;
    FAST SEQ_DEV *    	pSeqDev  = pTapeVol->tapevd_pSeqDev;

    /* Perform requested function */

    switch (op)
	{
	
	/* Write an end-of-file record, which means: write a file mark */

	case MTWEOF:

	    /* Check if the file mode is correct */

	    if (pTapeFd->tapefd_mode == O_RDONLY)
		{
                errno = EROFS;		/* posix errno */
		return (ERROR);
		}

	    /* Flush any buffered data */

	    if (tapeFsFdFlush (pTapeFd) == ERROR)
		{
		return (ERROR);
		}
		
	    if (pSeqDev->sd_seqWrtFileMarks == NULL)
		{
                errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
		return (ERROR);
		}

            /* Write "numOps" number of short file marks */

            if ((*pSeqDev->sd_seqWrtFileMarks) (pSeqDev, numOps, TRUE)
								      == ERROR)
                {
		return (ERROR);
		}
	    
	    pTapeFd->tapefd_bufIndex = 0;
	    return (OK);


        /* Forward space over a file mark */

	case MTFSF:     

	    if (pTapeFd->tapefd_mode != O_RDONLY)
		{
	        /* Flush any buffered data */

	        if (tapeFsFdFlush (pTapeFd) == ERROR)
	    	    {
		    return (ERROR);
		    }
		}

	    if (pSeqDev->sd_space == NULL)
		{
                errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
		return (ERROR);
		}

            /* Space forward "numOps" file marks */

	    if ((*pSeqDev->sd_space) (pSeqDev, numOps, SPACE_CODE_FILEMARK)
								      == ERROR)
                {
		return (ERROR);
		}

	    pTapeFd->tapefd_bufIndex = 0;
	    return (OK);
	    

        /* Backward space over a file mark */

	case MTBSF:
	    if (pTapeFd->tapefd_mode != O_RDONLY)
		{
	        /* Flush any buffered data */

	        if (tapeFsFdFlush (pTapeFd) == ERROR)
	    	    {
		    return (ERROR);
		    }
		}

	    if (pSeqDev->sd_space == NULL)
		{
	        errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
		return (ERROR);
		}

            /* Space backward "numOps" file marks */

	    if ((*pSeqDev->sd_space) (pSeqDev, -numOps, SPACE_CODE_FILEMARK)
								      == ERROR)
                {
		return (ERROR);
		}

	    pTapeFd->tapefd_bufIndex = 0;	    
	    return (OK);
	    

	/* Forward space to inter-record gap (data blocks) */

	case MTFSR:
	    if (pTapeFd->tapefd_mode != O_RDONLY)
		{
	        /* Flush any buffered data */

	        if (tapeFsFdFlush (pTapeFd) == ERROR)
	    	    {
		    return (ERROR);
		    }
		}

	    if (pSeqDev->sd_space == NULL)
		{
		errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
		return (ERROR);
		}

            /* Space forward "numOps" data blocks */

	    if ((*pSeqDev->sd_space) (pSeqDev, numOps, SPACE_CODE_DATABLK)
								      == ERROR)
                {
		return (ERROR);
		}
	    
	    pTapeFd->tapefd_bufIndex = 0;
	    return (OK);
	    

	/* Backward space to inter-record gap (data blocks) */

        case MTBSR:
	    if (pTapeFd->tapefd_mode != O_RDONLY)
		{
	        /* Flush any buffered data */

	        if (tapeFsFdFlush (pTapeFd) == ERROR)
	    	    {
		    return (ERROR);
		    }
		}

	    if (pSeqDev->sd_space == NULL)
		{
		errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
		return (ERROR);
		}

            /* Space backward "numOps" data blocks */

	    if ((*pSeqDev->sd_space) (pSeqDev, -numOps, SPACE_CODE_DATABLK)
								      == ERROR)
                {
		return (ERROR);
		}

	    pTapeFd->tapefd_bufIndex = 0;	    
	    return (OK);
	    

	/* Rewind the tape device */

	case MTREW:
	    if (pTapeFd->tapefd_mode != O_RDONLY)
		{
	        /* Flush any buffered data */

	        if (tapeFsFdFlush (pTapeFd) == ERROR)
	    	    {
		    return (ERROR);
		    }
		}

	    if (pSeqDev->sd_rewind == NULL)
		{
		errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
		return (ERROR);
		}

            /* Rewind the tape device */

	    if ((*pSeqDev->sd_rewind) (pSeqDev) == ERROR)
                {
		return (ERROR);
		}

	    pTapeFd->tapefd_bufIndex = 0;	    
	    return (OK);
	    

        /* Rewind and take the device offline */

	case MTOFFL:
	    if (pTapeFd->tapefd_mode != O_RDONLY)
		{
	        /* Flush any buffered data */

	        if (tapeFsFdFlush (pTapeFd) == ERROR)
	    	    {
		    return (ERROR);
		    }
		}

	    if ((pSeqDev->sd_rewind == NULL) || (pSeqDev->sd_load == NULL))
		{
		errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
		return (ERROR);
		}

            /* Rewind the tape device */

	    if ((*pSeqDev->sd_rewind) (pSeqDev) == ERROR)
                {
		return (ERROR);
		}

	    pTapeFd->tapefd_bufIndex = 0;	    

	    /* Take it offline by unloading the device */

	    if ((*pSeqDev->sd_load) (pSeqDev, UNLOAD, FALSE, FALSE) == ERROR)
                {
		return (ERROR);
		}
	    
	    return (OK);


        /* No operation on device, simply sets the status */

	case MTNOP:
	    if (pSeqDev->sd_statusChk == NULL)
		{
		errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
		return (ERROR);
		}

            /* Check device status (driver sets the status in SEQ_DEV) */

	    if ((*pSeqDev->sd_statusChk) (pSeqDev) == ERROR)
                {
		return (ERROR);
		}
	    
	    return (OK);
	    

	/* Retension the tape */

	case MTRETEN:
	    if (pTapeFd->tapefd_mode != O_RDONLY)
		{
	        /* Flush any buffered data */

	        if (tapeFsFdFlush (pTapeFd) == ERROR)
	    	    {
		    return (ERROR);
		    }
		}

	    if (pSeqDev->sd_load == NULL)
                {
		errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
		return (ERROR);
		}
	    
	    /* Take it offline by unloading the device */

	    if ((*pSeqDev->sd_load) (pSeqDev, UNLOAD, RETEN, FALSE) == ERROR)
                {
		return (ERROR);
		}
	    
	    pTapeFd->tapefd_bufIndex = 0;
	    return (OK);


        /* Erase the entire tape and rewind */

	case MTERASE:
	    if (pSeqDev->sd_erase == NULL)
                {
		errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
		return (ERROR);
		}
	    
	    /* Take it offline by unloading the device */

	    if ((*pSeqDev->sd_erase) (pSeqDev, LONG) == ERROR)
                {
		return (ERROR);
		}

	    pTapeFd->tapefd_bufIndex = 0;	    
	    return (OK);


        /* Position the tape at the end-of-medium before unloading */

	case MTEOM:
	    if (pTapeFd->tapefd_mode != O_RDONLY)
		{
	        /* Flush any buffered data */

	        if (tapeFsFdFlush (pTapeFd) == ERROR)
	    	    {
		    return (ERROR);
		    }
		}

	    if (pSeqDev->sd_load == NULL)
                {
		errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
		return (ERROR);
		}
	    
	    /* Take it offline by unloading the device */

	    if ((*pSeqDev->sd_load) (pSeqDev, UNLOAD, FALSE, EOT) == ERROR)
                {
		return (ERROR);
		}

	    pTapeFd->tapefd_bufIndex = 0;	    
	    return (OK);


        /* Backward space file to beginning of file */

        /* XXX Not sure how this is different that MTBSF 1 */

        case MTNBSF:

	    return (ERROR);

        /* Any other operation is an error */

        default:

	    return (ERROR);

        } /* switch */

    }

/*******************************************************************************
*
* tapeFsOpen - open a tape volume
*
* This routine opens the volume with the specified mode
* (O_RDONLY or O_WRONLY).
*
* The specified `name' must be a null string, indicating that the
* caller specified only the device name during open(). This is because
* there is no file structure on the tape device.
*
* If the device driver supplies a status-check routine, it will be
* called before the volume state is examined.
*
* RETURNS: Pointer to tape volume file descriptor, or ERROR.
*/

LOCAL TAPE_FILE_DESC *tapeFsOpen
    (
    FAST TAPE_VOL_DESC * pTapeVol,      /* pointer to volume descriptor      */
    FAST char *          name,     	/* "file" name - should be empty str */
    FAST int             flags,         /* open flags                        */
    FAST int		 mode		/* not used                          */
    )
    {
                                        /* file descriptor pointer */
    FAST TAPE_FILE_DESC	*pTapeFd = pTapeVol->tapevd_pTapeFd;	
					/* pointer to block device info */
    FAST SEQ_DEV	*pSeqDev = pTapeVol->tapevd_pSeqDev;
    UINT16          unused;


    /* Check for open of other than tape volume (non-null filename) */

    if (name [0] != EOS)
	{
	errno = S_tapeFsLib_ILLEGAL_FILE_SYSTEM_NAME; 
	return ((TAPE_FILE_DESC *) ERROR);	/* cannot specify filename */
	}

    /* check the open flags for read only or write only */

    if ((flags != O_RDONLY) && (flags != O_WRONLY))
	{
	errno = S_tapeFsLib_ILLEGAL_FLAGS; 
	TAPE_DEBUG_MSG ("tapeFsOpen: Wrong Tape Mode\n", 0,0,0,0,0,0);
	return ((TAPE_FILE_DESC *) ERROR);	/* cannot specify filename */
	}

    /* Get ownership of volume. If taken, return ERROR */

    if (semTake (pTapeVol->tapevd_semId, NO_WAIT) == ERROR)	
	{
	errno = EBUSY;			 	/* posix errno */
	TAPE_DEBUG_MSG ("tapeFsOpen: Could not get Semaphore\n", 0,0,0,0,0,0);
        return ((TAPE_FILE_DESC *) ERROR);
	}

    /* Check if file descriptor is in use */

    if (pTapeFd->tapefd_inUse == TRUE)
	{
        errno = S_tapeFsLib_FILE_DESCRIPTOR_BUSY;
	semGive (pTapeVol->tapevd_semId);	/* release volume         */
	return ((TAPE_FILE_DESC *) ERROR);	/* file descriptor in use */
	}

    /* Call driver check-status routine, if any */

    if (pSeqDev->sd_statusChk != NULL)
	{
	if ((* pSeqDev->sd_statusChk) (pSeqDev) != OK)
	    {
	    semGive (pTapeVol->tapevd_semId);
	    TAPE_DEBUG_MSG ("tapeFsOpen: statusChk failed\n", 0,0,0,0,0, 0);
	    return ((TAPE_FILE_DESC *) ERROR);	/* driver returned error */
	    }
	}


    /* Check for read-only volume opened for write/update */

    if ((pSeqDev->sd_mode == O_RDONLY)  && (flags == O_WRONLY ))
	{
	errno = S_ioLib_WRITE_PROTECTED;
	semGive (pTapeVol->tapevd_semId);
	TAPE_DEBUG_MSG ("tapeFsOpen: write protected. sd_mode is %x\n", 
				    pSeqDev->sd_mode,0,0,0,0, 0);
	return ((TAPE_FILE_DESC *) ERROR);	/* volume not writable */
	}

    /* fill in the rest of the tape volume fields */

    pTapeFd->tapefd_mode	   = flags;
    pTapeFd->tapefd_pTapeVol	   = pTapeVol;

    /* Check that volume is available */

    if (tapeFsVolCheck (pTapeVol) != OK)
	{
	semGive (pTapeVol->tapevd_semId);
	errno = S_tapeFsLib_VOLUME_NOT_AVAILABLE;
	TAPE_DEBUG_MSG ("tapeFsOpen: volume not available\n", 0,0,0,0,0, 0);
	return ((TAPE_FILE_DESC *) ERROR);	/* cannot access volume */
	}

    /* 
     * Reserve the tape device for exclusive use unitl the file is closed.
     * When the file is closed, the device should be released. Reserving the
     * device disallows any other host from accessing the tape device.
     */

    if ((*pSeqDev->sd_reserve) (pSeqDev) != OK)
	{
	semGive (pTapeVol->tapevd_semId);
	TAPE_DEBUG_MSG ("tapeFsOpen: reserve unit failed\n", 0,0,0,0,0, 0);
	return ((TAPE_FILE_DESC *) ERROR);  /* driver returned error */
	}

    /* mount a tape device if not already mounted */
    
    if (pTapeVol->tapevd_state != TAPE_VD_MOUNTED)
	{
	if ((*pSeqDev->sd_load) (pSeqDev, LOAD, FALSE, FALSE) != OK)
	    {
	    semGive (pTapeVol->tapevd_semId);
	    TAPE_DEBUG_MSG ("tapeFsOpen: mount failed\n", 0,0,0,0,0, 0);
	    return ((TAPE_FILE_DESC *) ERROR);  /* driver returned error */
	    }
        }

    /* rewind if it is a rewind device */

    if (pTapeVol->tapevd_rewind)
	{
	if ((*pSeqDev->sd_rewind) (pSeqDev) != OK)
	    {
	    semGive (pTapeVol->tapevd_semId);
	    TAPE_DEBUG_MSG ("tapeFsOpen: rewind failed\n", 0,0,0,0,0, 0);
	    return ((TAPE_FILE_DESC *) ERROR);  /* driver returned error */
	    }
	}

    /* Read block limits */

    if (pSeqDev->sd_readBlkLim != NULL)
	{
	if ((*pSeqDev->sd_readBlkLim) (pSeqDev, &pSeqDev->sd_maxVarBlockLimit,
				       &unused) != OK)
	    {
	    semGive (pTapeVol->tapevd_semId);
	    TAPE_DEBUG_MSG ("tapeFsOpen: readBlkLim failed\n", 0,0,0,0,0, 0);
	    return ((TAPE_FILE_DESC *) ERROR);  /* driver returned error */
	    }
	}


    /* Allocate a 1 block buffer for short reads/writes if fixed block */

    if (pSeqDev->sd_blkSize != 0)
	{
        pTapeFd->tapefd_buffer = (char *) calloc 
					((unsigned) pSeqDev->sd_blkSize, 1);
        if (pTapeFd->tapefd_buffer == NULL)
    	    {
            semGive (pTapeVol->tapevd_semId);		/* release volume */
	    TAPE_DEBUG_MSG ("tapeFsOpen: calloc failed\n", 0,0,0,0,0, 0);
	    return ((TAPE_FILE_DESC *) ERROR);	/* mem alloc error */
	    }
        pTapeFd->tapefd_bufSize = pSeqDev->sd_blkSize;
        }

    pTapeFd->tapefd_inUse = TRUE;		/* fd in use      */
    semGive (pTapeVol->tapevd_semId);		/* release volume */

    return (pTapeFd);
    }

/*******************************************************************************
*
* tapeFsRead - read from a tape volume
*
* This routine reads from the volume specified by the file descriptor
* (returned by tapeFsOpen()) into the specified buffer.
* `maxbytes' bytes will be read, if there is that much data in the file.
*
* RETURNS: Number of bytes actually read, 0 if end of file, or ERROR.
*/

LOCAL int tapeFsRead
    (
    FAST TAPE_FILE_DESC * pTapeFd,      /* file descriptor pointer          */
    char                * pBuf,         /* addr of input buffer             */
    UINT                  maxBytes      /* count of data units to read      */
    )
    {
    int		bytesLeft;	/* count of remaining bytes to read */
    int		dataCount;	/* count of remaining bytes to read */
    int		bytesRead;	/* count of bytes read              */
    int		blocksRead;	/* count of bytes read              */
    int		nBytes;		/* byte count from individual read  */
    int		numBytes;	/* byte count from individual read  */
    int		numBlks;	/* number of blocks to read         */
    
    /* pointer to vol descriptor        */
    TAPE_VOL_DESC	*pTapeVol = pTapeFd->tapefd_pTapeVol;
    
    /* pointer to sequential device     */
    SEQ_DEV	*pSeqDev = pTapeFd->tapefd_pTapeVol->tapevd_pSeqDev;
    
    /* take control of volume */
    semTake (pTapeVol->tapevd_semId, WAIT_FOREVER); 
    
    TAPE_DEBUG_MSG ("tapeFsRead: Device block size is: %d\n",
		    pSeqDev->sd_blkSize, 0,0,0,0,0);
    
    /* Check for valid length of read */
    
    if (maxBytes <= 0)
	{
	semGive (pTapeVol->tapevd_semId);
	errno =  S_tapeFsLib_INVALID_NUMBER_OF_BYTES;
	return (ERROR);
	}
    
    /* Check that device was opened for reading */
    
    if (pTapeFd->tapefd_mode == O_WRONLY)
        {
    	semGive (pTapeVol->tapevd_semId);
   	/* errnoSet (S_tapeFsLib_READ_ONLY); */
    	return (ERROR);
    	}
    
    /* Do successive reads until requested byte count or EOF */
    
    bytesLeft  = maxBytes;	/* init number bytes remaining     */
    dataCount  = maxBytes;
    
    /* Variable block read */
    
    if (pSeqDev->sd_blkSize == VARIABLE_BLOCK_SIZE)
	{
	while (dataCount > 0)
	    {
	    /* Read a variable block upto a maximum size */
	    
	    nBytes = (bytesLeft > pSeqDev->sd_maxVarBlockLimit) ?
		     pSeqDev->sd_maxVarBlockLimit  : bytesLeft;
	    
            if ((bytesRead = tapeFsDevRd (pTapeVol, nBytes, pBuf, 
					  VARIABLE_BLK)) == ERROR)
		break; 			/* do nothing, may not be an error */
	    
            dataCount -= nBytes;
            bytesLeft -= bytesRead;
	    pBuf      += bytesRead;
	    }
	
        semGive (pTapeVol->tapevd_semId);           /* release volume */
        return (maxBytes - bytesLeft);              /* number of bytes read */
    	}
    
    /* Fixed block read */
    
    while (dataCount > 0)
	{
	nBytes = 0;	/* init individual read count */
	
    	/* 
	 * Do direct whole-block read, if the data is more than a blocks worth
	 * and the FD buffer is empty.
	 */
	
    	if ((dataCount >= pSeqDev->sd_blkSize) &&
	    (pTapeFd->tapefd_bufIndex == 0))
	    {
            /* calculate number of blocks to read */
	    
	    numBlks = dataCount / pSeqDev->sd_blkSize;
	    
	    /* Read as many blocks as possible */
	    
	    if ((blocksRead = tapeFsDevRd (pTapeVol, numBlks, pBuf, FIXED_BLK))
		== ERROR)
    	    	break;	
	    
	    numBytes = numBlks * pSeqDev->sd_blkSize;
	    
	    /* calculate the number of bytes read */
	    
	    nBytes = blocksRead * pSeqDev->sd_blkSize;
	    }
	
        /* Handle partially read blocks (buffered) */
	
    	else	/* if not whole-block transfer */
	    {
	    if (pTapeFd->tapefd_bufIndex != 0)
	        numBytes =  pSeqDev->sd_blkSize - pTapeFd->tapefd_bufIndex;
	    else 
	        numBytes = dataCount;
	    
	    if ((nBytes = tapeFsPartRd (pTapeFd, pBuf, dataCount)) == ERROR)
		break;
	    }
	
	/* Adjust count remaining, buffer pointer, and file pointer */
	
	dataCount -= numBytes;
	bytesLeft -= nBytes;
	pBuf      += nBytes;
	}
    
    semGive (pTapeVol->tapevd_semId);		/* release volume */
    return (maxBytes - bytesLeft);  		/* number of bytes read */
    }

/*******************************************************************************
*
* tapeFsPartRd - read a partial tape block
*
* This routine reads from a tape volume specified by the file descriptor,
* to the specified buffer. When a partial block needs to be read, the entire
* block is first read into the file descriptor buffer and then the desired
* portion is copied into the specified buffer. If the number of bytes to be
* read is more than that in the buffer then only the number of bytes left
* in the buffer are read.
*
* This routine calls the device driver block-reading routine directly
* to read (possibly) a whole block.
*
* RETURNS: Number of bytes read (error if != maxBytes), or
* ERROR if maxBytes < 0, or can't read block.
*/

LOCAL int tapeFsPartRd
    (
    TAPE_FILE_DESC * pTapeFd,   /* pointer to a tape file descriptor      */
    char *           pBuf,      /* pointer to data buffer                 */
    UINT             nBytes     /* number of partial bytes to read        */
    )
    {
    int         status = 0;
    int 	blkSize = pTapeFd->tapefd_pTapeVol->tapevd_pSeqDev->sd_blkSize;
    TAPE_VOL_DESC * pTapeVol = pTapeFd->tapefd_pTapeVol;
    
    
    if (nBytes <= 0) 	/* This condition should never be true */
	return (ERROR);
    
    /* If there is no data in the buffer, then read a block into the buffer */

    /* Assumption: bufIndex will be 0 only if there is no data in the buffer */
    
    if (pTapeFd->tapefd_bufIndex == 0)
	{
	if ((status = tapeFsDevRd (pTapeVol, 1, pTapeFd->tapefd_buffer, 
				   FIXED_BLK)) == ERROR)
	    return (ERROR);
	
	nBytes *= status;
        }
    
    /*
     * If there are less bytes to be read than the remainder of the buffer
     * then simply read these bytes into the specified buffer (pBuf)
     */

    if (nBytes <= (blkSize - pTapeFd->tapefd_bufIndex))
        {
        bcopy (pTapeFd->tapefd_buffer+pTapeFd->tapefd_bufIndex, pBuf, nBytes);
	pTapeFd->tapefd_bufIndex += nBytes;
	
	if (blkSize == pTapeFd->tapefd_bufIndex)
	    pTapeFd->tapefd_bufIndex = 0;	/* reset bufIndex */
	}
    
    /*
     * If there are more bytes to be read than in the buffer, read the 
     * remainder of the bytes into the specified buffer (pBuf)
     */
    
    else
	{
        nBytes = blkSize - pTapeFd->tapefd_bufIndex;
	bcopy (pTapeFd->tapefd_buffer+pTapeFd->tapefd_bufIndex, pBuf, nBytes);
	pTapeFd->tapefd_bufIndex = 0;
	}

    /*
     * Return the number of bytes actually read into the 
     * specified buffer (pBuf)
     */
    
    return (nBytes);
    }

/*******************************************************************************
*
* tapeFsReadyChange - notify tapeFsLib of a change in ready status
*
* This routine sets the volume descriptor state to TAPE_VD_READY_CHANGED.
* It should be called whenever a driver senses that a device has come on-line
* or gone off-line (for example, that a tape has been inserted or removed).
*
* After this routine has been called, the next attempt to use the volume
* results in an attempted remount.
*
* RETURNS: OK if the read change status is set, or ERROR if the file descriptor
* is in use.
*
* ERRNO: S_tapeFsLib_FILE_DESCRIPTOR_BUSY
*/

STATUS tapeFsReadyChange
    (
    TAPE_VOL_DESC *pTapeVol         /* pointer to volume descriptor */
    )
    {
    FAST TAPE_FILE_DESC	*pTapeFd = pTapeVol->tapevd_pTapeFd;	

    semTake (pTapeVol->tapevd_semId, WAIT_FOREVER); /* take control of volume */

    if (pTapeFd->tapefd_inUse)
	{
        errno = S_tapeFsLib_FILE_DESCRIPTOR_BUSY;
        semGive (pTapeVol->tapevd_semId);		/* release volume */
	return (ERROR);
	}

    pTapeVol->tapevd_state = TAPE_VD_READY_CHANGED;

    semGive (pTapeVol->tapevd_semId);		/* release volume */

    return (OK);
    }

/*******************************************************************************
*
* tapeFsVolCheck - verify that volume descriptor is current
*
* This routine is called at the beginning of most operations on
* the device.  The status field in the volume descriptor is examined,
* and the appropriate action is taken.  In particular, the tape is
* mounted if it is currently unmounted or a ready-change has occurred.
*
* If the tape is already mounted or is successfully mounted as a
* result of this routine calling tapeFsVolMount, this routine returns
* OK.  If the tape cannot be mounted, ERROR is returned.
*
* RETURNS: OK, or ERROR.
*/

LOCAL STATUS tapeFsVolCheck
    (
    FAST TAPE_VOL_DESC   *pTapeVol          /* pointer to volume descriptor   */
    )
    {
    FAST int		status;		/* return status value      */
					/* ptr to sequential device */
    FAST SEQ_DEV *	pSeqDev = pTapeVol->tapevd_pSeqDev;


    status = OK;				/* init return value */

    /* Check if device driver announced ready-change */

    if (pSeqDev->sd_readyChanged)
	{
	pTapeVol->tapevd_state   = TAPE_VD_READY_CHANGED;
	pSeqDev->sd_readyChanged = FALSE;
	}


    /* Check volume status */

    switch (pTapeVol->tapevd_state)
	{
	case TAPE_VD_MOUNTED:
	    /* Tape is already mounted. Do nothing */

	    break;

        case TAPE_VD_UNMOUNTED:
            /*
	     * This case should cause an error because a volume has been 
	     * unmounted and a ready-change did not occur since then.
             */

	case TAPE_VD_READY_CHANGED:
	    /* Ready change occurred; try to mount volume */

	    if (tapeFsVolMount (pTapeVol) != OK)
		{
		TAPE_DEBUG_MSG ("tapeFsVolCheck: tapeFsVolMount failed\n",
								0,0,0,0,0,0);
		status = ERROR;			/* can't mount */
		break;
		}

	    pTapeVol->tapevd_state = TAPE_VD_MOUNTED;	/* volume mounted */

	    break;				/* ready to go as is */
	}

    return (status);				/* return status value */
    }

/*******************************************************************************
*
* tapeFsVolMount - prepare to use tape volume
*
* This routine prepares the library to use the tape volume on the device
* specified.
*
* This routine should be called every time a tape is changed (i.e. a tape
* is swapped, or whatever), or before every open and create, if the driver
* can't tell when tape are changed.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS tapeFsVolMount
    (
    FAST TAPE_VOL_DESC   *pTapeVol          /* pointer to volume descriptor */
    )
    {
					/* ptr to sequential device */
    FAST SEQ_DEV *	pSeqDev = pTapeVol->tapevd_pSeqDev;

   /* XXX */
    pTapeVol->tapevd_status = OK;

    if (pSeqDev->sd_load != NULL)
	{
        if ((*pSeqDev->sd_load) (pSeqDev, TRUE, FALSE, FALSE) != OK)
	    {
            TAPE_DEBUG_MSG ("tapeFsVolMount: pSeqDev->sd_load failed\n",
								0,0,0,0,0,0);
    	    return (ERROR);
	    }
        }
    else
	{
	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* tapeFsVolUnmount - disable a tape device volume
*
* This routine is called when I/O operations on a volume are to be
* discontinued.  This is commonly done before changing removable tape.
* All buffered data for the volume is written to the device (if possible),
* any open file descriptors are marked obsolete, and the volume is
* marked not mounted.
*
* Because this routine flushes data from memory to the physical
* device, it should not be used in situations where the tape-change is
* not recognized until after a new tape has been inserted.  In these
* circumstances, use the ready-change mechanism.  (See the manual entry for 
* tapeFsReadyChange().)
*
* This routine may also be called by issuing an ioctl() call using the
* FIOUNMOUNT function code.
*
* RETURNS: OK, or ERROR if the routine cannot access the volume.
*
* ERRNO:
* S_tapeFsLib_VOLUME_NOT_AVAILABLE,
* S_tapeFsLib_FILE_DESCRIPTOR_BUSY,
* S_tapeFsLib_SERVICE_NOT_AVAILABLE
*
* SEE ALSO: tapeFsReadyChange()
*/

STATUS tapeFsVolUnmount
    (
    FAST TAPE_VOL_DESC   *pTapeVol      /* pointer to volume descriptor */
    )
    {
					/* pointer to file descriptor   */
    FAST TAPE_FILE_DESC	*pTapeFd = pTapeVol->tapevd_pTapeFd;
					/* pointer to seq device        */
    FAST SEQ_DEV *	 pSeqDev = pTapeVol->tapevd_pSeqDev;
    FAST BOOL		 failed  = FALSE;


    /* Check that volume is available */

    semTake (pTapeVol->tapevd_semId, WAIT_FOREVER);/* get ownership of volume */

    if (pTapeVol->tapevd_state == TAPE_VD_UNMOUNTED)
        {
        errno = S_tapeFsLib_VOLUME_NOT_AVAILABLE;
        semGive (pTapeVol->tapevd_semId);		/* release volume */
        return (ERROR);				/* cannot access volume */
        }

    /* Check if the file descriptor is being used. It should be available */

    if (pTapeFd->tapefd_inUse == TRUE)
        {
        errno = S_tapeFsLib_FILE_DESCRIPTOR_BUSY;
        semGive (pTapeVol->tapevd_semId);		/* release volume */
        return (ERROR);				/* file descriptor busy */
        }

    /* Unload the volume */

    if (pSeqDev->sd_load != NULL)
	{
	if ((*pSeqDev->sd_load) (pSeqDev, FALSE, FALSE, FALSE) != OK)
	    failed = TRUE;
        }
    else
        {
        errno = S_tapeFsLib_SERVICE_NOT_AVAILABLE;
	failed = TRUE;
        }

    semGive (pTapeVol->tapevd_semId);		/* release volume */

    if (failed)
	{
        return (ERROR);				/* cannot access volume */
	}

    return (OK);
    }


/*******************************************************************************
*
* tapeFsWrite - write to a tape volume
*
* This routine writes to the tape volume specified by the file descriptor
* from the specified buffer.  If the block containing the tape locations
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

LOCAL int tapeFsWrite
    (
    FAST TAPE_FILE_DESC *pTapeFd,       /* file descriptor pointer  */
    char                *pBuf,          /* data to be written       */
    int                 maxBytes        /* number of bytes to write */
    )
    {
    FAST int		nBytes;		/* byte count for individual write */
    FAST int		numBlks;	/* number of blocks to write */
					/* pointer to vol descriptor */
    FAST TAPE_VOL_DESC	*pTapeVol = pTapeFd->tapefd_pTapeVol;
					/* pointer to sequential device info */
    FAST SEQ_DEV	*pSeqDev = pTapeFd->tapefd_pTapeVol->tapevd_pSeqDev;
    FAST int		bytesLeft;	/* remaining bytes to write */



    semTake (pTapeVol->tapevd_semId, WAIT_FOREVER); /* take control of volume */

    TAPE_DEBUG_MSG ("tapeFsWrite: Device block size is: %d\n",
					pSeqDev->sd_blkSize, 0,0,0,0,0);
    /* Check that device was opened for writing */

    if (pTapeFd->tapefd_mode == O_RDONLY)
	{
	semGive (pTapeVol->tapevd_semId);
        errno = EROFS;			/* posix errno */
	TAPE_DEBUG_MSG ("tapeFsWrite: read only volume\n",0,0,0,0,0,0);
	return (ERROR);
	}

    /* Check for valid length of write */

    if (maxBytes < 0)
	{
	semGive (pTapeVol->tapevd_semId);
	/* errnoSet (S_tapeFsLib_INVALID_NUMBER_OF_BYTES); */
	TAPE_DEBUG_MSG ("tapeFsWrite: maxBytes < 0\n",0,0,0,0,0,0);
	return (ERROR);
	}

    /* Device blockSize and FD buffer size should be identical */

    if (pSeqDev->sd_blkSize != pTapeFd->tapefd_bufSize)
	{
	semGive (pTapeVol->tapevd_semId);
	errno = S_tapeFsLib_INVALID_BLOCK_SIZE;
	TAPE_DEBUG_MSG ("tapeFsWrite: invalid blkSize\n",0,0,0,0,0,0);
	return (ERROR);
	}


    /* Write into successive blocks until all of caller's buffer written */


    bytesLeft = maxBytes;


    /* Variable block write */

    if (pSeqDev->sd_blkSize == VARIABLE_BLOCK_SIZE)
	{
	while (bytesLeft > 0)
	    {
	    /* Read a variable block upto a maximum size */

	    nBytes = (bytesLeft > pSeqDev->sd_maxVarBlockLimit) ?
				  pSeqDev->sd_maxVarBlockLimit  : bytesLeft;

            if (tapeFsBlkWrt (pTapeVol, nBytes, pBuf, VARIABLE_BLK) != OK)
		goto writeFailed;

            bytesLeft -= nBytes;
	    pBuf      += nBytes;
	    }

        semGive (pTapeVol->tapevd_semId);           /* release volume */
        return (maxBytes - bytesLeft);
        }


    /* Fixed block write */

    while (bytesLeft > 0)
	{
	nBytes = 0;				/* init individual write count*/

	/* Do direct whole-block write if possible */

	if ((bytesLeft >= pSeqDev->sd_blkSize) &&
	    				       (pTapeFd->tapefd_bufIndex == 0))
	    {
	    /* Calculate starting block and number to write */

	    numBlks = bytesLeft / pSeqDev->sd_blkSize;

	    /* Write as many blocks as possible */

	    if (tapeFsBlkWrt (pTapeVol, numBlks, pBuf, FIXED_BLK) != OK)
		goto writeFailed;

	    nBytes = numBlks * pSeqDev->sd_blkSize;
	    }

  	/* Handle partially written blocks with buffering */

	else
	    {
            if ((nBytes = tapeFsPartWrt (pTapeFd, pBuf, bytesLeft)) == ERROR)
		break;
	    }
	pBuf += nBytes;
	bytesLeft -= nBytes;
	}

    semGive (pTapeVol->tapevd_semId);		/* release volume */
    return (maxBytes - bytesLeft);		/* return actual copy count */

writeFailed:
    semGive (pTapeVol->tapevd_semId);
    TAPE_DEBUG_MSG ("tapeFsWrite: write failed \n",0,0,0,0,0,0);
    return (ERROR);

    }

/*******************************************************************************
*
* tapeFsPartWrt - write a partial tape block 
*
* This routine writes to the tape volume specified by the file descriptor
* from the specified buffer.  The partial block data is written into a 
* buffer, until that buffer is full and at that point the full buffer is
* written to tape.
*
* This routine calls the device driver block-writing routine directly
* to write (possibly) a whole block.
*
* RETURNS: Number of bytes written (error if != maxBytes), or
* ERROR if maxBytes < 0, or no more space for the file,
* or can't write block.
*/
LOCAL int tapeFsPartWrt 
    (
    TAPE_FILE_DESC * pTapeFd,	/* pointer to a tape file descriptor      */
    char *	     pBuf,      /* pointer to data buffer                 */
    int		     nBytes	/* number of partial block bytes to write */
    )
    {
    int blkSize = pTapeFd->tapefd_pTapeVol->tapevd_pSeqDev->sd_blkSize;
    TAPE_VOL_DESC * pTapeVol = pTapeFd->tapefd_pTapeVol;

    /*
     * If there are less bytes to be written than the remainder of the
     * buffered block, write those bytes to the buffer.
     */

    if (nBytes <= (blkSize - pTapeFd->tapefd_bufIndex))
        {
        bcopy (pBuf, pTapeFd->tapefd_buffer + pTapeFd->tapefd_bufIndex, nBytes);
	pTapeFd->tapefd_bufIndex += nBytes;

        /*
	 * If the entire block is filled, write out the block and reset the
	 * bufIndex.
	 */

	if (pTapeFd->tapefd_bufIndex == blkSize)
	    {
	    if (tapeFsBlkWrt (pTapeVol, 1, pTapeFd->tapefd_buffer, FIXED_BLK)
									!= OK)
		return (ERROR);

	    pTapeFd->tapefd_bufIndex = 0;
	    }
        }

    /*
     * If there are more bytes to be written than the remainder of the 
     * buffered block, write until block is full and then write that full block
     * out to tape.
     */

    else
	{
	nBytes = blkSize - pTapeFd->tapefd_bufIndex;
	bcopy (pBuf, pTapeFd->tapefd_buffer + pTapeFd->tapefd_bufIndex, nBytes);
	if (tapeFsBlkWrt (pTapeVol, 1, pTapeFd->tapefd_buffer, FIXED_BLK) != OK)
	    return (ERROR);
        pTapeFd->tapefd_bufIndex = 0;
	}

    /* Return the number of bytes actually written to buffer or tape */

    return (nBytes);
    }
        


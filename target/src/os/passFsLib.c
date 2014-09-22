/* passFsLib.c - pass-through (to UNIX) file system library (VxSim) */

/* Copyright 1992-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01r,06dec01,hbh  Fixed FIONREAD (SPR #22476 & #23615).
01q,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01p,17mar98,jmb  Merge my patch from HPSIM, 28oct97, fix bug in FIONREAD ioctl
                 so that number of bytes remaining includes number of bytes
                 in cache. (SPR 9628)
01o,11jul97,dgp  doc: ad (VxSim) to title line
01n,09jan97,pr   moved simsolaris macros and prototype in simLib.h 
		 moved solarisCvtFlags function in simLib.c
01n,23nov96,dvs  during merge fixed bug in ARCHCVTFLAGS for sunos,hppa 
01m,12jul95,ism  added support for simsolaris
01l,28mar95,kdl  changed to use POSIX date format during fstat().
01k,31oct94,kdl  made conditional for SIMSPARCSUNOS and SIMHPPA.
01j,24oct93,gae  documentation tweaks.
01i,18aug93,gae  modified unix_stat for hp.
01h,30jul93,gae  more doc touchups.
01g,05jun93,gae  fixed doc about number of passFs's allowed.
01f,23jan93,gae  removed u_printf; ANSIfied.
01e,31aug92,gae  changed mkdir to use 755 per smeg.
		 displayed unknown ioctl function code; ignored FIOSETOPTIONS.
01d,10aug92,gae  set drvnum to usual 0 initially.
01c,29jul92,gae  cleanup.
01b,01jun92,gae  minor WRS revision.
01a,04feb92,smeg written.
*/

/*
DESCRIPTION
This module is only used with VxSim simulated versions of VxWorks.

This library provides services for file-oriented device drivers to use
the UNIX file standard.  This module takes care of all the buffering,
directory maintenance, and file system details that are necessary.
In general, the routines in this library are not to be called directly 
by users, but rather by the VxWorks I/O System.

INITIALIZING PASSFSLIB
Before any other routines in passFsLib can be used, the routine passFsInit()
must be called to initialize this library.
The passFsDevInit() routine associates a device name with the passFsLib
functions.  The parameter expected by passFsDevInit() is a pointer to
a name string, to be used to identify the volume/device.  This will be part of
the pathname for I/O operations which operate on the device.  This
name will appear in the I/O system device table, which may be displayed
using the iosDevShow() routine.

As an example:
.CS
    passFsInit (1);
    passFsDevInit ("host:");
.CE

After the passFsDevInit() call has been made, when passFsLib receives a request 
from the I/O system, it calls the UNIX I/O system to service
the request.  Only one volume may be created.

READING DIRECTORY ENTRIES
Directories on a passFs volume may be searched using the opendir(),
readdir(), rewinddir(), and closedir() routines.  These calls allow the
names of files and sub-directories to be determined.

To obtain more detailed information about a specific file, use the fstat()
or stat() function.  Along with standard file information, the structure
used by these routines also returns the file attribute byte from a passFs
directory entry.

FILE DATE AND TIME
UNIX file date and time are passed though to VxWorks.

INCLUDE FILES: passFsLib.h

SEE ALSO
ioLib, iosLib, dirLib, ramDrv
*/


#include "vxWorks.h"
#include "ctype.h"
#include "dirent.h"
#include "iosLib.h"
#include "lstLib.h"
#include "semLib.h"
#include "stat.h"
#include "string.h"
#include "stdlib.h"
#include "passFsLib.h"

/* ONLY USED FOR SIMULATOR */

#if (CPU_FAMILY==SIMSPARCSUNOS || CPU_FAMILY==SIMHPPA || CPU_FAMILY==SIMSPARCSOLARIS)

#include "simLib.h"
#include "u_Lib.h"

extern int printErr ();

/* globals */

int passFsUnixFilePerm = 0666;
int passFsUnixDirPerm  = 0775;

/* locals */

LOCAL int passFsDrvNum;		/* driver number assigned to passFsLib */

#define CACHE_SIZE 8192		/* size of read-ahead buffer */

/* Volume descriptor */

typedef struct          /* VOL_DESC */
    {
    DEV_HDR passDevHdr;	/* tracks the device: only one passFs device */
    char passDevName [MAX_FILENAME_LENGTH];
    } VOL_DESC;

typedef struct
    {
    VOL_DESC *passVdptr;
    UNIX_DIR *passDir;
    char passName[MAX_FILENAME_LENGTH]; /* so we can do a UNIX stat() later */
    int unixFd;
    char *readCache;
    int cacheOffset;
    int cacheBytes;
    } PASS_FILE_DESC;

VOL_DESC *passVolume;	/* only 1 permitted */

LOCAL PASS_FILE_DESC *passFsDirCreate ();
LOCAL STATUS	     passFsDirMake ();
LOCAL STATUS	     passFsIoctl ();
LOCAL PASS_FILE_DESC *passFsOpen ();
LOCAL int	     passFsRead ();
LOCAL int	     passFsWrite ();


/*******************************************************************************
*
* passFsClose - close a passFs file
*
* This routine closes the specified passFs file.
*
* The remainder of the file I/O buffer beyond the end of file is cleared out,
* the buffer is flushed, and the directory is updated if necessary.
*
* RETURNS: OK, or ERROR if directory couldn't be flushed or entry couldn't 
* be found.
*/

LOCAL STATUS passFsClose
    (
    PASS_FILE_DESC *pPassFd	/* passthough file descriptor */
    )

    {
    int status = OK;

    if ((int)pPassFd != -1)
    	status = u_close (pPassFd->unixFd);

    if (pPassFd->passDir != (UNIX_DIR *)0)
    	status = u_closedir (pPassFd->passDir);

    if (pPassFd->readCache != (char *)0)
	free (pPassFd->readCache);

    free (pPassFd);

    return (status);
    }

/*******************************************************************************
*
* passFsCreate - create a UNIX file
*
* RETURNS: passthrough file descriptor, or ERROR if error in create.
*/

LOCAL PASS_FILE_DESC *passFsCreate
    (
    VOL_DESC	*vdptr,		/* pointer to volume descriptor	*/
    char	*fullName,	/* passFs path/filename string */
    int		flags		/* file flags (READ/WRITE/UPDATE) */
    )

    {
    PASS_FILE_DESC *pPassFd;

    if ((pPassFd = (PASS_FILE_DESC *) calloc (1, sizeof (PASS_FILE_DESC)))
						== (PASS_FILE_DESC *)0)
	{
	return ((PASS_FILE_DESC *)ERROR);
	}
		
    pPassFd->unixFd = u_open (fullName, L_CREAT | ARCHCVTFLAGS(flags),
	passFsUnixFilePerm);
    if (pPassFd->unixFd == -1)
	{
	free (pPassFd);
	return ((PASS_FILE_DESC *) ERROR);
	}

    strcpy (pPassFd->passName, fullName);
    pPassFd->passDir     = (UNIX_DIR *)0;
    pPassFd->readCache   = (char *)0;
    pPassFd->cacheBytes  = 0;
    pPassFd->cacheOffset = 0;
    pPassFd->passVdptr   = vdptr;

    return (pPassFd);
    }

/*******************************************************************************
*
* passFsDelete - delete a passFs file
*
* This routine deletes the file <name> from the specified passFs volume.
*
* RETURNS: OK, or ERROR if the file not found
*/

LOCAL STATUS passFsDelete
    (
    VOL_DESC	*vdptr,		/* pointer to volume descriptor	*/
    char	*name		/* passFs path/filename */
    )
    {
    /* XXX everything should relative to vdptr */

    if (u_unlink (name) == -1)
	{
	/* try rmdir if unlink failed */
	if (u_rmdir (name) == -1)
	    return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* passFsDevInit - associate a device with passFs file system functions
*
* This routine associates the name <devName> with the file system and installs 
* it in the I/O System's device table.  The driver number used when
* the device is added to the table is that which was assigned to the
* passFs library during passFsInit().
*
* RETURNS: A pointer to the volume descriptor, or NULL if there is an error.
*/

void *passFsDevInit
    (
    char *devName	/* device name */
    )
    {
    /* Add device to system device table */

    if (iosDevAdd (&passVolume->passDevHdr, devName, passFsDrvNum) != OK)
	{
	return (NULL);				/* can't add device */
	}

    strcpy (passVolume->passDevName, devName);

    return ((void*)passVolume);
    }

/*******************************************************************************
*
* passFsDirCreate - create a passFs directory
*
* This routine creates a new sub-directory on a passFs volume and
* returns a open file descriptor for it.  It calls passFsDirMake to
* actually create the directory; the remainder of this routine simply
* sets up the file descriptor.
*
* RETURNS: Pointer to passFs file descriptor (PASS_FILE_DESC),
* or ERROR if error.
*/

LOCAL PASS_FILE_DESC *passFsDirCreate
    (
    VOL_DESC	*vdptr,	/* pointer to volume descriptor	*/
    char *name          /* directory name */
    )
    {
    PASS_FILE_DESC *pPassFd;

    if (passFsDirMake (vdptr, name, TRUE) == ERROR)
	return ((PASS_FILE_DESC *)ERROR);

    if ((pPassFd = (PASS_FILE_DESC *) calloc (1, sizeof (PASS_FILE_DESC)))
						== (PASS_FILE_DESC *)0)
	{
	return ((PASS_FILE_DESC *)ERROR);
	}

    strcpy (pPassFd->passName, name);
    pPassFd->unixFd      = -1;
    pPassFd->passDir     = (UNIX_DIR *)u_opendir (name);
    pPassFd->readCache   = (char *)0;
    pPassFd->cacheBytes  = 0;
    pPassFd->cacheOffset = 0;
    pPassFd->passVdptr   = vdptr;

    if (pPassFd->passDir == (UNIX_DIR *)0)
	{
	free (pPassFd);
	return ((PASS_FILE_DESC *)ERROR);
	}

    return (pPassFd);
    }
/*******************************************************************************
*
* passFsDirMake - make a passFs directory
*
* This routine creates ("makes") a new passFs directory. 
*
* Since this routine may be called as a result of either an open() call
* or an ioctl() call, the level of filename expansion will vary.  A
* boolean flag set by the calling routine (passFsOpen or passFsIoctl) indicates
* whether the name has already been expanded (and the device name stripped).
*
* RETURNS: status indicating success
*/

LOCAL STATUS passFsDirMake
    (
    VOL_DESC	*vdptr,		/* pointer to volume descriptor	*/
    char	*name,		/* directory name */
    BOOL	nameExpanded	/* TRUE if name was already expanded */
    )
    {
    DEV_HDR		*pCurDev;	/* ptr to current dev hdr (ignored) */
    char		fullName [MAX_FILENAME_LENGTH];	

    /* Get full expanded dir name (no device name) unless already done */

    if (nameExpanded == FALSE)
	{
    	if (ioFullFileNameGet (name, &pCurDev, fullName) != OK)
	    return (ERROR);			/* cannot expand filename */
	}
    else
	{
	(void) strcpy (fullName, name);
	}

    if (u_mkdir (fullName, passFsUnixDirPerm) == -1)
	return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* passFsDirRead - read directory and return next filename
*
* This routine is called via an ioctl() call with the FIOREADDIR function
* code.  The "dirent" field in the passed directory descriptor (DIR) is
* filled with the null-terminated name of the next file in the directory.
*
* RETURNS: OK, or ERROR.
*/

LOCAL STATUS passFsDirRead
    (
    PASS_FILE_DESC  *pPassFd,	/* ptr to passFs file descriptor */
    DIR		  *pDir		/* ptr to directory descriptor */
    )
    {
    struct unix_dirent *dirent;

    if (pPassFd->passDir == (UNIX_DIR *)0)
    	pPassFd->passDir = (UNIX_DIR *)u_opendir (pPassFd->passName);

    if (pPassFd->passDir == (UNIX_DIR *)0)
	return (ERROR);

    dirent = (struct unix_dirent *) u_readdir (pPassFd->passDir);

    if (dirent == (struct unix_dirent *)0)
	return (ERROR);

    strcpy (pDir->dd_dirent.d_name, dirent->d_name);

    return (OK);
    }

/*******************************************************************************
*
* passFsFileStatGet - get file status (directory entry data)
*
* This routine is called via an ioctl() call, using the FIOFSTATGET
* function code.  The passed stat structure is filled, using data
* obtained from the directory entry which describes the file.
*
* RETURNS: ERROR or OK
*/

LOCAL STATUS passFsFileStatGet
    (
    PASS_FILE_DESC	*pPassFd,	/* pointer to file descriptor */
    struct stat	*pStat		/* structure to fill with data */
    )

    {
    struct unix_stat unixbuf;

    if (u_stat (pPassFd->passName, (char*)&unixbuf) == -1)
	return (ERROR);

    bzero ((char *) pStat, sizeof (struct stat)); /* zero out stat struct */

    /* Fill stat structure */

    pStat->st_dev     = (ULONG) passVolume;
						/* device ID = DEV_HDR addr */
    pStat->st_ino     = 0;			/* no file serial number */
    pStat->st_nlink   = 1;			/* always only one link */
    pStat->st_uid     = 0;			/* no user ID */
    pStat->st_gid     = 0;			/* no group ID */
    pStat->st_rdev    = 0;			/* no special device ID */
    pStat->st_size    = unixbuf.st_size;	/* file size, in bytes */
    pStat->st_atime   = 0;			/* no last-access time */
    pStat->st_mtime   = 0;			/* no last-modified time */
    pStat->st_ctime   = 0;			/* no last-change time */
    pStat->st_attrib  = 0;			/* file attribute byte */
    
    pStat->st_mtime = unixbuf.st_mtime;		/* file modified time */


    /* Set file type in mode field, mark whole volume (raw mode) as directory */

    pStat->st_mode = 0;				/* clear field */

    if (unixbuf.st_mode & 0x4000)		/* if directory */
	{
	pStat->st_mode |= S_IFDIR;		/*  set bits in mode field */
	}
    else if (unixbuf.st_mode & 0x8000)		/* if reg file */
	{
	pStat->st_mode |= S_IFREG;		/*  it is a regular file */
	}

    pStat->st_mode |= unixbuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);

    return (OK);
    }
		
/*******************************************************************************
*
* passFsInit - prepare to use the passFs library
*
* This routine initializes the passFs library.  It must be called exactly once,
* before any other routines in the library.  The argument specifies the number
* of passFs devices that may be open at once.  This routine installs
* passFsLib as a driver in the I/O system driver table, allocates and sets 
* up the necessary memory structures, and initializes semaphores.
*
* Normally this routine is called from the root task, usrRoot(),
* in usrConfig().  This initialization is enabled when the
* configuration macro INCLUDE_PASSFS is defined.
*
* NOTE
* Maximum number of pass-through file systems is 1.
*
* RETURNS: OK, or ERROR.
*/

STATUS passFsInit
    (
    int nPassfs	/* number of pass-through file systems */
    )

    {
    if (passFsDrvNum != 0)
	return (ERROR);			/* can't call more than once */

    /* Install passFsLib routines in I/O system driver table */

    passFsDrvNum = iosDrvInstall ((FUNCPTR) passFsCreate, passFsDelete, 
			         (FUNCPTR) passFsOpen, passFsClose, 
			         passFsRead, passFsWrite, passFsIoctl);

    if (passFsDrvNum == ERROR)
	return (ERROR);				/* can't install as driver */

    if (nPassfs < 0 || nPassfs > 1)
	return (ERROR);				/* can't install as driver */

    if ((passVolume = calloc (nPassfs, sizeof (VOL_DESC))) == NULL)
	return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* passFsIoctl - do device specific control function
*
* This routine performs the following ioctl() functions:
*
* .CS
*    FIODISKINIT:   Initialize the disk volume.  This routine does not
*                   format the disk, that must be done by the driver.
*    FIORENAME:     Rename the file to the string pointed to by arg.
*    FIOSEEK:       Sets the file's current byte position to
*                   the position specified by arg.
*    FIOWHERE:      Returns the current byte position in the file.
*    FIOFLUSH:      Flush file output buffer.
*                   Guarantees that any output that has been requested
*                   is actually written to the device.
*    FIONREAD:      Return in arg the number of unread bytes.
*    FIODISKCHANGE: Indicate media change.  See passFsReadyChange().
*    FIOUNMOUNT:    Unmount disk volume.  See passFsVolUnmount().
*    FIONFREE:	    Return amount of free space on volume.
*    FIOMKDIR:	    Create a new directory.
*    FIORMDIR:	    Remove a directory.
*    FIOLABELGET:   Get volume label (located in root directory).
*    FIOLABELSET:   Set volume label.
*    FIOATTRIBSET:  Set file attribute.
*    FIOCONTIG:	    Allocate contiguous disk space for file or directory.
*    FIOSYNC:	    Write all modified files and data structures to device.
*    FIOREADDIR:    Read next directory entry, uses DIR directory descriptor.
*    FIOFSTATGET:   Get file status info (directory enty data).
* .CE
*
* Any other ioctl function codes are passed to the block device driver
* for handling.
*
* If an ioctl fails, the task's status (see errnoGet()) indicates
* the nature of the error.
*
* RETURNS: OK, or ERROR if function failed or driver returned error, or
*   current byte pointer for FIOWHERE.
*/

LOCAL STATUS passFsIoctl
    (
    PASS_FILE_DESC	*pPassFd,	/* file descriptor of file to control */
    int			function,	/* function code */
    int			arg		/* some argument */
    )

    {
    int	retValue = OK;
    int whereCur;	/* current byte position in file, could include cache */
    int where;		/* exact current byte position in file */
    int end;
    
    switch (function)
	{
	case FIODISKINIT:
	    break;

	case FIORENAME:
	    if (u_rename (pPassFd->passName, (char *)arg) == -1)
		retValue = ERROR;
	    break;

	case FIOSEEK:
	    /* invalidate any read cache */
	    pPassFd->cacheBytes = 0;
    	    pPassFd->cacheOffset = 0;

	    /* do a lseek with offset arg0 from beginning of file */
	    if (u_lseek (pPassFd->unixFd, (int)arg, 0) == -1)
		retValue = ERROR;
	    break;

	case FIOWHERE:
	    /* do a lseek with offset 0 from current to get current offset */
	    retValue = u_lseek (pPassFd->unixFd, 0, 1/*SEEK_CUR*/);

	    /* Adjust since we cache ahead, so LSEEK returns too far */
	    if (retValue > 0 && pPassFd->cacheBytes != 0)
		retValue -= (pPassFd->cacheBytes - pPassFd->cacheOffset);
	    break;

	case FIOSYNC:
	case FIOFLUSH:
	    break;

	case FIONREAD:
	    /* 
	     * compute the number of bytes between the current location and
	     * the end of this file.
	     * We uses this method rather than a call to :
	     * u_ioctl (pPassFd->unixFd, U_FIONREAD, 0);
	     * because FIONREAD is not officially supported on Solaris and it
	     * does not work on Solaris 2.5. (SPR #22476 & 23615).
	     */
	    
	    whereCur = u_lseek (pPassFd->unixFd, 0, 1/*SEEK_CUR*/);

	    if (whereCur == ERROR)
	    	retValue = ERROR;
	    else
	    	{
		/* Adjust since we cache ahead, so LSEEK returns too far */

		if (pPassFd->cacheBytes != 0)
		    where = whereCur - 
		    	(pPassFd->cacheBytes - pPassFd->cacheOffset);
		else
		   where = whereCur;
		
	    	end = u_lseek (pPassFd->unixFd, 0, 2/*SEEK_END*/);

		if (end	== ERROR)
		    retValue = ERROR;	
		else
		    {
		    /* go back to the current location */

		    if (u_lseek (pPassFd->unixFd, whereCur, 0/*SEEK_SET*/) 
		    	== ERROR)
			retValue = ERROR;
	 	    else	
	    	        *((int *) arg) = end - where;
		    }	
	    	}

	    break;

	case FIODISKCHANGE:
	    break;

	case FIOUNMOUNT:
	    break;

	case FIONFREE:
	    /* this is hard to do on a UNIX file system - lie */
	    *((int *) arg) = 0x1000000;
	    break;

	case FIOMKDIR:
	    if (passFsDirMake (pPassFd->passVdptr, (char *) arg, FALSE) 
	    		== ERROR)
		retValue = ERROR;
	    break;

	case FIORMDIR:
	    if (u_rmdir (pPassFd->passName) == -1)
		retValue = ERROR;
	    break;

	case FIOLABELGET:
	    strcpy ((char *)arg, passVolume->passDevName);
	    break;

	case FIOLABELSET:
	    break;

	case FIOATTRIBSET:
	    break;

	case FIOCONTIG:
	    break;

	case FIOREADDIR:
	    retValue = passFsDirRead (pPassFd, (DIR *) arg);
	    break;

	case FIOFSTATGET:
	    retValue = passFsFileStatGet (pPassFd, (struct stat *) arg);
	    break;

	case FIOSETOPTIONS:
	    /* XXX usually trying to set OPT_TERMINAL -- just ignore */
	    break;

	default:
	    printErr ("passFsLib: unknown ioctl = %#x\n", function);
	    retValue = ERROR;
	    break;
	}

    return (retValue);
    }

/*******************************************************************************
*
* passFsOpen - open a file on a passFs volume
*
* This routine opens the file <name> with the specified mode 
* (READ/WRITE/UPDATE/CREATE/TRUNC).  The directory structure is 
* searched, and if the file is found a passFs file descriptor 
* is initialized for it.
*
* RETURNS: A pointer to a passFs file descriptor, or ERROR 
*   if the volume is not available 
*   or there are no available passFs file descriptors 
*   or there is no such file.
*/

LOCAL PASS_FILE_DESC *passFsOpen
    (
    VOL_DESC	*vdptr,		/* pointer to volume descriptor	*/
    char	*name,		/* passFs full path/filename */
    int		flags,		/* file open flags */
    int		mode		/* file open permissions (mode) */
    )
    {
    PASS_FILE_DESC	*pPassFd;	/* file descriptor pointer */

    /* Check if creating dir */

    if ((flags & O_CREAT)  &&  (mode & FSTAT_DIR))
        {
	return (passFsDirCreate (vdptr, name));
	}


    /* Get a free file descriptor */

    if ((pPassFd = (PASS_FILE_DESC *) calloc (1, sizeof (PASS_FILE_DESC)))
								== NULL)
	{
	return ((PASS_FILE_DESC *) ERROR);
	}

    if ((pPassFd->unixFd = u_open (name, ARCHCVTFLAGS(flags), mode)) == -1)
	{
	free (pPassFd);
	return ((PASS_FILE_DESC *) ERROR);
	}

    strcpy (pPassFd->passName, name);
    pPassFd->passDir     = (UNIX_DIR *)0;
    pPassFd->readCache   = (char *)0;
    pPassFd->cacheBytes  = 0;
    pPassFd->cacheOffset = 0;
    pPassFd->passVdptr   = vdptr;

    return (pPassFd);
    }

/*******************************************************************************
*
* passFsRead - read from a passFs file
*
* This routine reads from the file specified by the file descriptor
* (returned by passFsOpen()) into the specified buffer.
* <maxbytes> bytes will be read, if there is that much data in the file
* and the file I/O buffer is large enough.
*
* RETURNS: Number of bytes actually read, or 0 if end of file, or
*    ERROR if <maxbytes> is <= 0 or unable to get next cluster.
*/

LOCAL int passFsRead
    (
    PASS_FILE_DESC	*pPassFd,	/* file descriptor pointer */
    char		*pBuf,		/* addr of input buffer	*/
    int			maxBytes	/* maximum bytes to read into buffer */
    )

    {
    int ret;
    int bytesRead = 0;
    int readableCacheBytes;
    char *cachePtr;

    /* attempt to service the read from cache first */
    if (pPassFd->cacheBytes)
	{
        readableCacheBytes = pPassFd->cacheBytes - pPassFd->cacheOffset;
	cachePtr = pPassFd->readCache + pPassFd->cacheOffset;

	/* if we have more cached data than asked for */
	if (readableCacheBytes > maxBytes) {

	   readableCacheBytes = maxBytes;
	   pPassFd->cacheOffset += readableCacheBytes;

	} else {

	   /* invalidate cache since its all going to be read */
	   pPassFd->cacheBytes = 0;
	   pPassFd->cacheOffset = 0;
	}

	bcopy (cachePtr, pBuf, readableCacheBytes);

	/* if we are done, bail now */
	if (readableCacheBytes == maxBytes)
		return (maxBytes);

	maxBytes -= readableCacheBytes;
	bytesRead += readableCacheBytes;
	pBuf += readableCacheBytes;
	}

    /* initialize read cache if first read */
    if (pPassFd->readCache == (char *)0)
	{
	if ((pPassFd->readCache = (char*)calloc (1, CACHE_SIZE)) == (char *)0)
	    return (ERROR);
	}

    /* ok, here we know the cache is allocated and invalidated */

    /* caching can't help if they want a huge chunk */
    if (maxBytes >= CACHE_SIZE)
	{
    	if ((ret = u_read (pPassFd->unixFd, pBuf, maxBytes)) == -1)
	    return (bytesRead ? bytesRead : ERROR);

	bytesRead += ret;

	}
    else
	{
	ret = u_read (pPassFd->unixFd,pPassFd->readCache, CACHE_SIZE);
        if ( ret == -1)
	    return (bytesRead ? bytesRead : ERROR);

	pPassFd->cacheBytes = ret;

	/* If there wasn't as much as asked for */
	if (ret < maxBytes)
	    maxBytes = ret;

	bcopy (pPassFd->readCache, pBuf, maxBytes);
	bytesRead += maxBytes;
	pPassFd->cacheOffset = maxBytes;
	}

    return (bytesRead);
    }

/*******************************************************************************
*
* passFsWrite - write to a passFs file
*
* This routine writes to the file specified by the file descriptor
* (returned by passFsOpen()) from the specified buffer. 
*
* RETURNS: Number of bytes written (error if != nBytes), or ERROR if 
*    nBytes < 0, or no more space for the file, or can't write cluster.
*/

LOCAL int passFsWrite 
    (
    PASS_FILE_DESC	*pPassFd,	/* file descriptor pointer */
    char		*pBuf,		/* data to be written */
    int			maxBytes	/* number of bytes to write */
    )

    {
    int nBytes;

    /* invalidate read cache */
    pPassFd->cacheBytes = 0;
    pPassFd->cacheOffset = 0;

    if ((nBytes = u_write (pPassFd->unixFd, pBuf, maxBytes)) == -1)
	return (ERROR);

    return (nBytes);
    }


#endif /* (CPU_FAMILY==SIMSPARCSUNOS || CPU_FAMILY==SIMHPPA || CPU_FAMILY==SIMSPARCSOLARIS) */

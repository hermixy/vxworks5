/* ntPassFsLib.c - pass-through (to Windows NT) file system library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01k,21may02,jmp  added last-access time and last-change time fields
		 initialization to ntPassFsFileStatGet() (SPR #77386).
01j,06dec01,hbh  Implemented FIONREAD (SPR #71102).
01i,21nov01,jmp  added intLock()/intUnlock() around NT system call.
01h,30aug01,hbh  Completed SPR 27918 fix : O_EXCL flag did not work.
		 Fixed SPR 68557 to be able to "ls" root directories.
01g,08dec99,pai  corrected ntPassFsOpen() such that open() returns ERROR
                 if called without O_CREAT and the specified file does not
                 exist.  Introduced local constant INVALID_HANDLE_VALUE to
                 represent failed return from win_FindFirstFile().
01f,08dec99,pai  fixed NTPERMFLAGS to be an octal constant, as it was
                 intended to be, instead of a decimal constant.
01e,08dec99,pai  added explicit type casts to clean up compiler warnings
                 and clarify win_Lib function return types.
01d,08dec99,pai  moved HOST_BINARY to ntPassFsLib.h.
01c,08dec99,pai  fixed SPR 27918 by adding ARCHCVTFLAGS macro and
                 Win32 file control flag values to ntPassFsLib.h.
01b,05aug98,cym  changed HOST_BINARY for GNU to 0x8000 after changing 
		 to the mingwin toolchain from cygwin beta 19.
01a,19mar98,cym  written based on UNIX passFsLib.
*/

/*
DESCRIPTION
This module is only used with VxSim simulated versions of VxWorks.

This library provides services for file-oriented device drivers to use
the Windows NT file standard.  In general, the routines in this library are 
not to be called directly by users, but rather by the VxWorks I/O System.

INITIALIZING PASSFSLIB
Before any other routines in ntPassFsLib can be used, the routine ntPassFsInit()
must be called to initialize this library.
The ntPassFsDevInit() routine associates a device name with the ntPassFsLib
functions.  The parameter expected by ntPassFsDevInit() is a pointer to
a name string, to be used to identify the volume/device.  This will be part of
the pathname for I/O operations which operate on the device.  This
name will appear in the I/O system device table, which may be displayed
using the iosDevShow() routine.

As an example:
.CS
    ntPassFsInit (1);
    ntPassFsDevInit ("host:");
.CE

After the ntPassFsDevInit() call has been made, when ntPassFsLib receives a request 
from the I/O system, it calls the Windows NT I/O system to service
the request.  Only one volume may be created.

READING DIRECTORY ENTRIES
Directories on a ntPassFs volume may be searched using the opendir(),
readdir(), rewinddir(), and closedir() routines.  These calls allow the
names of files and sub-directories to be determined.

To obtain more detailed information about a specific file, use the fstat()
or stat() function.  Along with standard file information, the structure
used by these routines also returns the file attribute byte from a ntPassFs
directory entry.

FILE DATE AND TIME
Windows NT file date and time are passed through to VxWorks.

INCLUDE FILES: ntPassFsLib.h

SEE ALSO
ioLib, iosLib, dirLib, ramDrv
*/

/* includes */

#include "vxWorks.h"
#include "ctype.h"
#include "dirent.h"
#include "iosLib.h"
#include "lstLib.h"
#include "semLib.h"
#include "stat.h"
#include "string.h"
#include "stdlib.h"
#include "intLib.h"

#include "stdio.h"

/* ONLY USED FOR SIMULATOR */

#if CPU==SIMNT

#include "ntPassFsLib.h"
#include "win_Lib.h"

#undef READ
#undef WRITE

#ifdef __GNUC__
#define _OPEN OPEN
#define _CLOSE CLOSE
#define _READ READ
#define _WRITE WRITE
#define _LSEEK LSEEK
#define _FINDFIRST FINDFIRST
#define _FINDCLOSE FINDCLOSE
#define _FINDNEXT FINDNEXT
#define _STAT STAT
#define _MKDIR MKDIR
#define _RMDIR RMDIR
#define _UNLINK UNLINK
#define _SETMODE SETMODE
int FMODE;
#define _FMODE FMODE
#else
IMPORT int _FMODE;
#endif /* __GNUC__ */

IMPORT int _OPEN();
IMPORT int _WRITE();
IMPORT int _READ();
IMPORT int _CLOSE();
IMPORT int _LSEEK();
IMPORT int _SETMODE();
IMPORT int _UNLINK();
IMPORT int _RMDIR();
IMPORT int _MKDIR();
IMPORT int _STAT();
IMPORT int _FINDNEXT();
IMPORT int _FINDCLOSE();
IMPORT int _FINDFIRST();
IMPORT int RENAME();
IMPORT int printErr ();
IMPORT int sprintf();


#define INVALID_HANDLE_VALUE  ((WIN_HANDLE) -1)


/* 
 * Windows NT file macros - Note that these macros are octal
 *
 * NTPERMFLAGS is the logical bitwise OR of permissions flags defined
 * for Win32 in sys/stat.h:
 *
 * #define _S_IREAD    0000400  /@ read permission, owner @/
 * #define _S_IWRITE   0000200  /@ write permission, owner @/
 *
 * That is, we put (R/W) permissions on files made via creat().
 */
 
#define NTPERMFLAGS  0000600
#define WIN_IFDIR    0040000 
#define WIN_IFREG    0100000


/* locals */

LOCAL int ntPassFsDrvNum = 0;   /* driver number assigned to ntPassFsLib */

/* Volume descriptor */

typedef struct               /* VOL_DESC */
    {
    DEV_HDR ntPassDevHdr;    /* tracks the device: only one ntPassFs device */
    char ntPassDevName [MAX_FILENAME_LENGTH];
    } VOL_DESC;

typedef struct
    {
    VOL_DESC *ntPassVdptr;
    char path[MAX_FILENAME_LENGTH];
    int fd;			/* -1 = Directory */
    int hFind;			/* only used by directory */
    } PASS_FILE_DESC;

VOL_DESC *ntPassVolume;	/* only 1 permitted */

LOCAL PASS_FILE_DESC *ntPassFsDirCreate ();
LOCAL STATUS	     ntPassFsDirMake ();
LOCAL STATUS	     ntPassFsIoctl ();
LOCAL PASS_FILE_DESC *ntPassFsOpen ();
LOCAL int	     ntPassFsRead ();
LOCAL int	     ntPassFsWrite ();

/**************************************************************************
*
* ntPassFsDirRead - read directory and return next filename
*
* This routine is called via an ioctl() call with the FIOREADDIR function
* code.  The "dirent" field in the ntPassed directory descriptor (DIR) is
* filled with the null-terminated name of the next file in the directory.
*
* RETURNS: OK, or ERROR.
*/

LOCAL STATUS ntPassFsDirRead
    (
    PASS_FILE_DESC *	pPassFd, /* ptr to ntPassFs file descriptor */
    DIR	*		pDir	 /* ptr to directory descriptor */
    )
    {
    WIN_FIND_DATA findData;
    int lvl;

    /* 
     * On windows, we have to do a find of "*.*" to get the files in a 
     * directory 
     */

    if(pPassFd->hFind == -1)
	{
        char searchString [MAX_FILENAME_LENGTH+4];	
	sprintf (searchString, "%s\\*.*", pPassFd->path);

	lvl = intLock ();	/* lock windows system call */

	(WIN_HANDLE)(pPassFd->hFind) = win_FindFirstFile (searchString, 
							  &findData);
	intUnlock (lvl);	/* unlock windows system call */
	}
    else
        {
	lvl = intLock ();	/* lock windows system call */

	if( !win_FindNextFile ((HANDLE) pPassFd->hFind, &findData))
	    {
  	    win_FindClose ((HANDLE)(pPassFd->hFind));

	    intUnlock (lvl);	/* unlock windows system call */

	    pPassFd->hFind = -1; /* Next time, start the find over */
            return (ERROR);
	    }

	intUnlock (lvl);	/* unlock windows system call */
        }

    strcpy (pDir->dd_dirent.d_name, findData.name);

    return (OK);
    }

/***************************************************************************
*
* ntPassFsDirCreate - create a ntPassFs directory
*
* This routine creates a new sub-directory on a ntPassFs volume and
* returns a open file descriptor for it.  It calls ntPassFsDirMake to
* actually create the directory; the remainder of this routine simply
* sets up the file descriptor.
*
* RETURNS: Pointer to ntPassFs file descriptor (PASS_FILE_DESC),
* or ERROR if error.
*/

LOCAL PASS_FILE_DESC * ntPassFsDirCreate
    (
    VOL_DESC *	vdptr,	/* pointer to volume descriptor	*/
    char *	name	/* directory name */
    )
    {
    PASS_FILE_DESC * pPassFd;

    if (ntPassFsDirMake (vdptr, name, TRUE) == ERROR)
	return ((PASS_FILE_DESC *)ERROR);

    if ((pPassFd = (PASS_FILE_DESC *) calloc (1, sizeof (PASS_FILE_DESC)))
					      == (PASS_FILE_DESC *)0)
	{
	return ((PASS_FILE_DESC *)ERROR);
	}

    strcpy (pPassFd->path, name);
    pPassFd->fd = -1; 
    pPassFd->hFind = -1;
    pPassFd->ntPassVdptr   = vdptr;

    return (pPassFd);
    }

/***************************************************************************
*
* ntPassFsFileStatGet - get file status (directory entry data)
*
* This routine is called via an ioctl() call, using the FIOFSTATGET
* function code.  The pStat stat structure is filled, using data
* obtained from Windows.
*
* RETURNS: ERROR or OK
*/

LOCAL STATUS ntPassFsFileStatGet
    (
    PASS_FILE_DESC *	pPassFd,	/* pointer to file descriptor */
    struct stat	   *	pStat		/* structure to fill with data */
    )

    {
    struct win_stat	ntbuf;
    char 		newPath[4];	
    char * 		path = pPassFd->path;
    int lvl;
    int retValue = OK;
    
    /* 
     * SPR 68557 : if the path is the root path (e.g c:), then the '\'  
     * character should be added at the end of the path string otherwise,
     * windows stat API returns error.
     */
     
    if ((strlen (pPassFd->path) == 2) && (pPassFd->path[1] == ':'))
    	{
	sprintf (newPath,"%s\\",pPassFd->path);
	path = newPath;
	}

    lvl = intLock ();		/* lock windows system call */

    retValue = _STAT (path, &ntbuf);

    intUnlock (lvl);		/* unlock windows system call */

    if (retValue == -1)
        {
	return (ERROR);
        }

    bzero ((char *) pStat, sizeof (struct stat)); /* zero out stat struct */

    /* Fill stat structure */

    pStat->st_dev     = (ULONG) ntPassVolume;
						/* device ID = DEV_HDR addr */
    pStat->st_ino     = 0;			/* no file serial number */
    pStat->st_nlink   = 1;			/* always only one link */
    pStat->st_uid     = 0;			/* no user ID */
    pStat->st_gid     = 0;			/* no group ID */
    pStat->st_rdev    = 0;			/* no special device ID */
    pStat->st_size    = ntbuf.st_size;		/* file size, in bytes */
    pStat->st_atime   = ntbuf.st_atime;		/* last-access time */
    pStat->st_mtime   = ntbuf.st_mtime;		/* last-modified time */
    pStat->st_ctime   = ntbuf.st_ctime;		/* last-change time */
    pStat->st_attrib  = 0;			/* file attribute byte */

    /* Set file type in mode field,mark whole volume (raw mode) as directory */

    pStat->st_mode = 0;				/* clear field */

    if (ntbuf.st_mode & WIN_IFDIR)		/* if directory */
	{
	pStat->st_mode |= S_IFDIR;		/*  set bits in mode field */
	}
    else if (ntbuf.st_mode & WIN_IFREG)		/* if reg file */
	{
	pStat->st_mode |= S_IFREG;		/*  it is a regular file */
	}

    pStat->st_mode |= ntbuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);

    return (OK);
    }
		
/***************************************************************************
*
* ntPassFsDirMake - make a ntPassFs directory
*
* This routine creates ("makes") a new ntPassFs directory. 
*
* Since this routine may be called as a result of either an open() call
* or an ioctl() call, the level of filename expansion will vary.  A
* boolean flag set by the calling routine (ntPassFsOpen or ntPassFsIoctl) 
* indicates whether the name has already been expanded (and the device name 
* stripped).
*
* RETURNS: status indicating success
*/

LOCAL STATUS ntPassFsDirMake
    (
    VOL_DESC *	vdptr,		/* pointer to volume descriptor	*/
    char     *	name,		/* directory name */
    BOOL	nameExpanded	/* TRUE if name was already expanded */
    )
    {
    DEV_HDR *	pCurDev;	/* ptr to current dev hdr (ignored) */
    char	fullName [MAX_FILENAME_LENGTH];
    int		lvl;
    int		retValue;

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

    lvl = intLock ();		/* lock windows system call */

    retValue = _MKDIR (fullName);

    intUnlock (lvl);		/* unlock windows system call */

    if (retValue == -1)
        {
	return (ERROR);
        }

    return (OK);
    }

/***************************************************************************
*
* ntPassFsClose - close a ntPassFs file
*
* This routine closes the specified ntPassFs file.
*
* The remainder of the file I/O buffer beyond the end of file is cleared out,
* the buffer is flushed, and the directory is updated if necessary.
*
* RETURNS: OK, or ERROR if directory couldn't be flushed or entry couldn't 
* be found.
*/

LOCAL STATUS ntPassFsClose
    (
    PASS_FILE_DESC * pPassFd	/* ntPassthough file descriptor */
    )
    {
    int status = OK;
    int lvl;
    
    if (pPassFd->fd == -1)
        {
	/* Clean up after directory by closing any open finds */
       
	if (pPassFd->hFind != -1)
  	    {
	    lvl = intLock();	/* lock windows system call */
	    
	    if (win_FindClose ((HANDLE)(pPassFd->hFind)) == -1) 
	    	status = ERROR;

	    intUnlock (lvl);	/* unlock windows system call */
	    }
        }
    else
        {
	lvl = intLock();	/* lock windows system call */
	
	_CLOSE (pPassFd->fd);

	intUnlock (lvl);	/* unlock windows system call */
	}

    free (pPassFd);

    return (status);
    }

/***************************************************************************
*
* ntPassFsCreate - create a Windows NT file
*
* RETURNS: ntPassthrough file descriptor, or ERROR if error in create.
*/

LOCAL PASS_FILE_DESC * ntPassFsCreate
    (
    VOL_DESC *	vdptr,		/* pointer to volume descriptor	*/
    char *	fullName,	/* ntPassFs path/filename string */
    int		flags,		/* file flags (READ/WRITE/UPDATE) */
    int 	mode
    )
    {
    PASS_FILE_DESC * pPassFd;
    int lvl;

    if ((flags & O_CREAT) && (mode & FSTAT_DIR))
	return (ntPassFsDirCreate (vdptr, fullName));

    if ((pPassFd = (PASS_FILE_DESC *) calloc (1, sizeof (PASS_FILE_DESC)))
						== (PASS_FILE_DESC *)0)
	{
	return ((PASS_FILE_DESC *)ERROR);
	}

    lvl = intLock ();		/* lock windows system call */

    /* SPR 27918 - Convert Flags to Windows */

    pPassFd->fd = _OPEN (fullName, ARCHCVTFLAGS(flags) | L_CREAT | HOST_BINARY,
                  	 NTPERMFLAGS);

    intUnlock (lvl);		/* unlock windows system call */

    if (pPassFd->fd == -1)
	{
	free (pPassFd);
	return ((PASS_FILE_DESC *) ERROR);
	}

    pPassFd->ntPassVdptr = vdptr;

    return (pPassFd);
    }

/***************************************************************************
*
* ntPassFsDelete - delete a ntPassFs file
*
* This routine deletes the file <name> from the specified ntPassFs volume.
*
* RETURNS: OK, or ERROR if the file not found
*/

LOCAL STATUS ntPassFsDelete
    (
    VOL_DESC *	vdptr,		/* pointer to volume descriptor	*/
    char *	name		/* ntPassFs path/filename */
    )
    {
    int retValue = OK;
    int lvl = intLock ();	/* lock windows system call */

    if (_UNLINK (name) == -1)
	{
	/* try rmdir if unlink failed */
	if (_RMDIR (name) == -1)
	    retValue = ERROR;
	}

    intUnlock (lvl);		/* unlock windows system call */
    return (retValue);
    }

/***************************************************************************
*
* ntPassFsDevInit - associate a device with ntPassFs file system functions
*
* This routine associates the name <devName> with the file system and installs 
* it in the I/O System's device table.  The driver number used when
* the device is added to the table is that which was assigned to the
* ntPassFs library during ntPassFsInit().
*
* RETURNS: A pointer to the volume descriptor, or NULL if there is an error.
*/

void *ntPassFsDevInit
    (
    char * devName	/* device name */
    )
    {
    /* Add device to system device table */

    if (iosDevAdd (&ntPassVolume->ntPassDevHdr, devName, ntPassFsDrvNum) != OK)
	{
	return (NULL);				/* can't add device */
	}

    strcpy (ntPassVolume->ntPassDevName, devName);

    return ((void*)ntPassVolume);
    }

/***************************************************************************
*
* ntPassFsInit - prepare to use the ntPassFs library
*
* This routine initializes the ntPassFs library. It must be called exactly 
* once, before any other routines in the library. The argument specifies the 
* number of ntPassFs devices that may be open at once. This routine installs
* ntPassFsLib as a driver in the I/O system driver table, allocates and sets 
* up the necessary memory structures, and initializes semaphores.
*
* Normally this routine is called from the root task, usrRoot(),
* in usrConfig().  To enable this initialization, define INCLUDE_PASSFS
* in configAll.h.
*
* NOTE
* Maximum number of ntPass-through file systems is 1.
*
* RETURNS: OK, or ERROR.
*/

STATUS ntPassFsInit
    (
    int nPassfs	/* number of ntPass-through file systems */
    )
    {
    if (ntPassFsDrvNum != 0)
	return (ERROR);			/* can't call more than once */

    /* Install ntPassFsLib routines in I/O system driver table */

    ntPassFsDrvNum = iosDrvInstall ((FUNCPTR) ntPassFsCreate, ntPassFsDelete, 
			           (FUNCPTR) ntPassFsOpen, ntPassFsClose, 
			           ntPassFsRead, ntPassFsWrite, ntPassFsIoctl);

    if (ntPassFsDrvNum == ERROR)
	return (ERROR);				/* can't install as driver */

    if (nPassfs < 0 || nPassfs > 1)
	return (ERROR);				/* can't install as driver */

    if ((ntPassVolume = calloc (nPassfs, sizeof (VOL_DESC))) == NULL)
	return (ERROR);

    return (OK);
    }

/***************************************************************************
*
* ntPassFsIoctl - do device specific control function
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
*    FIODISKCHANGE: Indicate media change.  See ntPassFsReadyChange().
*    FIOUNMOUNT:    Unmount disk volume.  See ntPassFsVolUnmount().
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
* Any other ioctl function codes are ntPassed to the block device driver
* for handling.
*
* If an ioctl fails, the task's status (see errnoGet()) indicates
* the nature of the error.
*
* RETURNS: OK, or ERROR if function failed or driver returned error, or
*   current byte pointer for FIOWHERE.
*/

LOCAL STATUS ntPassFsIoctl
    (
    PASS_FILE_DESC *	pPassFd,  /* file descriptor of file to control */
    int			function, /* function code */
    int			arg	  /* some argument */
    )

    {
    int	retValue = OK;
    int lvl;
    int where;
    int end;

    switch (function)
	{
	case FIODISKINIT:
	    break;

	case FIORENAME:
	    lvl = intLock ();		/* lock windows system call */

	    if (RENAME (pPassFd->path, (char *)arg) == -1)
		retValue = ERROR;

	    intUnlock (lvl);		/* unlock windows system call */
	    break;

	case FIOSEEK:
	    /* do a lseek with offset arg0 from beginning of file */

	    lvl = intLock ();		/* lock windows system call */
	    
	    if (_LSEEK (pPassFd->fd, (int)arg, 0) == -1)
		retValue = ERROR;

	    intUnlock (lvl);		/* unlock windows system call */
	    break;

	case FIOWHERE:
	    /* do a lseek with offset 0 from current to get current offset */
	    
	    lvl = intLock ();		/* lock windows system call */

	    retValue = _LSEEK (pPassFd->fd, 0, 1/*SEEK_CUR*/);

	    intUnlock (lvl);		/* unlock windows system call */
	    break;

	case FIOSYNC:
	case FIOFLUSH:
	    break;

	case FIONREAD:
	    /* 
	     * compute the number of bytes between the current location and
	     * the end of this file.
	     */
	    	    
	    lvl = intLock ();
	    
	    if ((where = _LSEEK (pPassFd->fd, 0, 1/*SEEK_CUR*/)) == ERROR)
	    	{
		intUnlock (lvl);
	    	retValue = ERROR;
		}
	    else
	    	{
	    	if ((end =_LSEEK (pPassFd->fd, 0, 2/*SEEK_END*/)) == ERROR)
		    {
		    intUnlock (lvl);
	    	    retValue = ERROR;
		    }
	        else
		    {
		    /* get back to current location */

		    if (_LSEEK (pPassFd->fd, where, 0/*SEEK_SET*/) == ERROR)
		    	{
	    	    	intUnlock (lvl);
			retValue = ERROR;
		    	}
		    else
		    	{
	    	    	intUnlock (lvl);
	    	   	*((int *) arg) = end - where;
			}
		    }
		}    
	    break;

	case FIODISKCHANGE:
	    break;

	case FIOUNMOUNT:
	    break;

	case FIONFREE:
	    /* this is hard to do on a Windows NT file system - lie */
	    
	    *((int *) arg) = 0x1000000;
	    break;

	case FIOMKDIR:
	    if (ntPassFsDirMake (pPassFd->ntPassVdptr, (char *) arg, FALSE) 
	    	!= ERROR)
		retValue = OK;
	    break;

	case FIORMDIR:
	    lvl = intLock ();		/* lock windows system call */

	    if (_RMDIR (pPassFd->path) == -1)
		retValue = ERROR;
	    intUnlock (lvl);		/* unlock windows system call */

	    break;

	case FIOLABELGET:
	    strcpy ((char *)arg, ntPassVolume->ntPassDevName);
	    break;

	case FIOLABELSET:
	    break;

	case FIOATTRIBSET:
	    break;

	case FIOCONTIG:
	    break;

	case FIOREADDIR:
	    retValue = ntPassFsDirRead (pPassFd, (DIR *) arg);
	    break;

	case FIOFSTATGET:
	    retValue = ntPassFsFileStatGet (pPassFd, (struct stat *) arg);
	    break;

	case FIOSETOPTIONS:
	    /* XXX usually trying to set OPT_TERMINAL -- just ignore */
	    break;

	default:
	    printErr ("ntPassFsLib: unknown ioctl = %#x\n", function);
	    retValue = ERROR;
	    break;
	}

    return (retValue);
    }

/***************************************************************************
*
* ntPassFsOpen - open a file on a ntPassFs volume
*
* This routine opens the file <name> with the specified mode 
* (READ/WRITE/UPDATE/CREATE/TRUNC).  The directory structure is 
* searched, and if the file is found a ntPassFs file descriptor 
* is initialized for it.
*
* RETURNS: A pointer to a ntPassFs file descriptor, or ERROR 
*   if the volume is not available 
*   or there are no available ntPassFs file descriptors 
*   or there is no such file.
*/

LOCAL PASS_FILE_DESC * ntPassFsOpen
    (
    VOL_DESC * vdptr,       /* pointer to volume descriptor	*/
    char *     name,        /* ntPassFs full path/filename */
    int        flags,       /* file open flags */
    int        mode         /* file open permissions (mode) */
    )
    {
    PASS_FILE_DESC * pPassFd;  /* file descriptor pointer */
    int              winFd;
    int lvl;
    
    /* assume empty names mean current directory */

    if (name[0]=='\0')
        {
        name = ".";
        }

    /* Check if creating dir */

    if ((mode & FSTAT_DIR ) && (flags & O_CREAT))
        {
	return (ntPassFsDirCreate (vdptr, name));
        }

    lvl = intLock ();		/* lock windows system call */

    /* SPR 27918 - convert flags to windows */

    winFd = _OPEN (name, ARCHCVTFLAGS(flags) | HOST_BINARY, NTPERMFLAGS);

    intUnlock (lvl);		/* unlock windows system call */

    if (winFd == -1)
        {
        WIN_FIND_DATA findData;
        WIN_HANDLE    dummyfind;
        
        /* 
         * check to see if the file exists 
         * if it exists, assume it's a directory since open failed
         * winFd will be -1 still, indicating a directory.
	 * Without following fix for SPR 68557, O_EXCL flag did not work.
	 * In fact, in case an open is made with O_EXCL flag when the file 
	 * already exists, then the above call to OPEN returns ERROR. So this 
	 * part of code is executed but it is not a directory. If
	 * win_FindFirstFile() is called with the exact name, the API will 
	 * return OK because the file exists but the initial call to open() 
	 * should have return ERROR. The fix for SPR 68557 fixes the O_EXCL
	 * issue because the name of the file to open/create is completed
	 * with "\*.*".
	 * SPR 68557 : If it is the root directory, name (which is like "C:") 
	 * should be modified to follow Windows recommendations : To examine 
	 * files in a root directory, use something like "C:\*".
	 * For the other directories, Windows documentation specifies : 
	 * An attempt to open a search with a trailing backslash will 
	 * always fail.
	 * So we add "\*.*" characters at the end of name string whatever is
	 * the directory (root or not).
	 */

        char searchString[MAX_FILENAME_LENGTH+4];	
	sprintf (searchString,"%s\\*.*",name);
	
	lvl = intLock ();	/* lock windows system call */

	dummyfind = win_FindFirstFile (searchString, &findData);

	intUnlock (lvl);	/* unlock windows system call */

        if (dummyfind == INVALID_HANDLE_VALUE)
            {
	    return ((PASS_FILE_DESC *) ERROR);
            }
        
	lvl = intLock ();	/* lock windows system call */

        win_FindClose  (dummyfind);

	intUnlock (lvl);	/* unlock windows system call */
        }

    /* Get a free file descriptor */

    if ((pPassFd = (PASS_FILE_DESC *) calloc (1, sizeof (PASS_FILE_DESC)))
         == NULL)
        {
	return ((PASS_FILE_DESC *) ERROR);
        }

    pPassFd->fd          = winFd;
    pPassFd->hFind       = -1;
    pPassFd->ntPassVdptr = vdptr;
    strcpy (pPassFd->path, name);

    return (pPassFd);
    }

/***************************************************************************
*
* ntPassFsRead - read from a ntPassFs file
*
* This routine reads from the file specified by the file descriptor
* (returned by ntPassFsOpen()) into the specified buffer.
* <maxbytes> bytes will be read, if there is that much data in the file
* and the file I/O buffer is large enough.
*
* RETURNS: Number of bytes actually read, or 0 if end of file, or
*    ERROR if <maxbytes> is <= 0 or unable to get next cluster.
*/

LOCAL int ntPassFsRead
    (
    PASS_FILE_DESC *	pPassFd,	/* file descriptor pointer */
    char *		pBuf,		/* addr of input buffer	*/
    int			maxBytes	/* maximum bytes to read into buffer */
    )
    {
    int lvl;
    int retValue;

    /* Make sure they aren't trying to read a directory*/
    
    if (pPassFd->fd == -1 ) 
        {
        return (ERROR);
        }

    lvl = intLock();		/* lock windows system call */

    retValue = _READ (pPassFd->fd, pBuf, maxBytes);

    intUnlock (lvl);		/* unlock windows system call */

    return (retValue);
    }

/***************************************************************************
*
* ntPassFsWrite - write to a ntPassFs file
*
* This routine writes to the file specified by the file descriptor
* (returned by ntPassFsOpen()) from the specified buffer. 
*
* RETURNS: Number of bytes written (error if != nBytes), or ERROR if 
*    nBytes < 0, or no more space for the file, or can't write cluster.
*/

LOCAL int ntPassFsWrite 
    (
    PASS_FILE_DESC * 	pPassFd,	/* file descriptor pointer */
    char *		pBuf,		/* data to be written */
    int			maxBytes	/* number of bytes to write */
    )
    {
    int lvl;
    int retValue;

    /* Make sure they aren't trying to write a directory */
    
    if (pPassFd->fd == ERROR) 
    	return(ERROR);

    lvl = intLock();		/* lock windows system call */

    retValue = _WRITE (pPassFd->fd, pBuf, maxBytes);

    intUnlock (lvl);		/* unlock windows system call */

    return (retValue);
    }

#endif /* CPU==SIMNT */

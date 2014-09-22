/* nfsDrv.c - Network File System (NFS) I/O driver */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03l,07may02,kbw  man page edits
03k,06nov01,vvv  made max. path length configurable (SPR #63551)
03j,15oct01,rae  merge from truestack ver 03s, base 03h (SPR #9357 etc.)
03i,07feb01,spm  merged from version 03o of tor3_x branch (base 03h):
                 fixed group validation in nfsMountAll (SPR #3524),
                 added permission checks to nfsOpen() routine (SPR #21991)
03h,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
03g,15dec98,cjtc added helper routine nfsDrvNumGet() to return the nfs
		 driver number in the io system. This is needed by the
		 WindView host to ensure that the upload is going via
		 the upload mechanism requested
03f,20nov97,gnn  fix for spr#5436, need to support FIOFSTATFSGET
03e,26aug97,spm  removed compiler warnings (SPR #7866)
03f,19aug97,cth  changed nfsCacheSize back to before 03e (SPR 7834) fixed by
		 fixing SPR 8460.
03e,15apr97,cth  added simsol specific fix for spr#7834 (for sirocco release), 
           +dvs  at some pt this probably needs to be better integrated.
03d,06aug96,sgv  fix for spr#6468
02s,13feb95,jdi  doc tweaks.
02r,12jan95,rhp  doc: add FIOSYNC to list of NFS ioctl functions. 
		   Also, jdi doc tweaks to nfsDevListGet() and nfsDevInfoGet().
02q,01dec93,jag  added the routines nfsDevListGet(), and nfsDevInfoGet().  
		 These routines provide support for the SNMP demo.
02q,08sep94,jmm  changed NFS client to use new RPC genned xdr routines
02p,28jul93,jmm  added permission checking to nfsOpen () (spr 2046)
02o,01feb93,jdi  documentation cleanup for 5.1; added third param to
		 ioctl() examples.
02n,13nov92,dnw  added include of semLibP.h
02m,21oct92,jdi  removed mangen SECTION designation.
02l,02oct92,srh  added ioctl(FIOGETFL) to return file's open mode
02k,16sep92,jcf  added include of errnoLib.h.
02j,04sep92,jmm  changed nfsUnmount to make sure device names match exactly
                   (spr 1564)
02i,19jul92,smb  added include fcntl.h.
02h,26may92,rrr  the tree shuffle
02g,19Mar92,jmm  changed nfsSeek to allow seeks beyond EOF
                 added #include of pathLib.h
02f,20jan92,jmm  changed nfsOpen to not truncate path names after
                 looking up symlinks
02e,16dec91,gae  added includes plus fixes for ANSI.
02d,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY and ...
		  -changed VOID to void
		  -changed copyright notice
02c,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
02b,21feb91,jaa	 documentation cleanup.
02a,08oct90,dnw  changed nfsMountAll verbose flag to quiet flag.
01z,08oct90,hjb  de-linted.
01y,02oct90,hjb  nfsWrite() changed to avoid reading data into cache to
		   keep cache up-to-date when size of data being written
		   is bigger than the nfs cache size.
01x,10aug90,dnw  added verbose arg to nfsMountAll().
01w,26jun90,jcf  changed semaphore types.
		 removed super semaphores.
01v,29may90,dnw  fixed nfsMount to free allocated memory if iosDevAdd returns
		   ERROR
01u,10may90,dnw  added nfsMountAll
		 change nfsClose() to not call nfsClientClose() (unless
		   nfsAutoClose is set TRUE)
		 made SSEMAPHORE include embedded SEM_BINARY which is inited
		   instead of a SEM_ID which is created (to reduce malloc).
		 changed NFS_MAXPATHLEN to (PATH_MAX + 1)
01t,01may90,llk  added FIOREADDIR and FIOFSTATGET ioctl calls.
		 deleted FIODIRENTRY ioctl call.
01s,01apr90,llk  changed nfsRead() to always return the number of bytes
		   requested by caller (up to the bytes left in the file).
		 changed name nfsFlushCache() to nfsCacheFlush().
		 added "DEFICIENCIES" documentation (and probably some
		   deficiencies).
		 made string arrays shorter than defined in the NFS spec.
		 fixed bug which didn't update file ptr correctly if cache
		   size was 0.
		 coalesced malloc of cache and fd into one malloc.
01r,14mar90,jdi  documentation cleanup.
01q,09aug89,dab  made nfsOpen() check for O_TRUNC flag.
                 made nfsCreate() truncate file if file already exists.
01p,12jul89,llk  lint.
01o,24mar89,dab  caching disabled when nfsCacheSize is zero;
           +gae  formerly a bug when nfsCacheSize was zero.
01n,23mar89,dab  added FIOSYNC option to nfsIoctl().
01m,21nov88,jcf  lint.
01l,19oct88,llk  documentation.
01k,28sep88,llk  documentation.
01j,24aug88,gae  documentation.  Made nfsMount() check hostname, and
		 allow default local name.
01i,07jul88,jcf  lint.
01h,30jun88,llk  changed to handle links.
		 added nfsDevShow().
01g,22jun88,dnw  changed taskIdGet() to taskIdSelf().
01f,16jun88,llk  changed to more v4 names.
01e,04jun88,llk  added rm, mkdir, rmdir.
		 changed how nfsOpen retrieves directory and file names.
01d,30may88,dnw  changed to v4 names.
01c,04may88,jcf  changed ssemaphores to be consistent with new semLib.
01b,22apr88,gae  fixed drvnum reference in nfsUnmount().
01a,19apr88,llk  written.
*/

/*
This driver provides facilities for accessing files transparently over the
network via NFS (Network File System).  By creating a network device
with nfsMount(), files on a remote NFS system (such as a UNIX system) can
be handled as if they were local.

USER-CALLABLE ROUTINES
The nfsDrv() routine initializes the driver.  The nfsMount() and nfsUnmount()
routines mount and unmount file systems.  The nfsMountAll() routine mounts
all file systems exported by a specified host.

INITIALIZATION
Before using the network driver, it must be initialized by calling nfsDrv().
This routine must be called before any reads, writes, or other NFS calls.
This is done automatically when INCLUDE_NFS is defined..

CREATING NFS DEVICES
In order to access a remote file system, an NFS device must be created by
calling nfsMount().  For example, to create the device `/myd0/' for the file
system `/d0/' on the host `wrs', call:
.CS
    nfsMount ("wrs", "/d0/", "/myd0/");
.CE
The file `/d0/dog' on the host `wrs' can now be accessed as `/myd0/dog'.

If the third parameter to nfsMount() is NULL, VxWorks creates a device with
the same name as the file system.  For example, the call:
.CS
    nfsMount ("wrs", "/d0/", NULL);
.CE
or from the shell:
.CS
    nfsMount "wrs", "/d0/"
.CE
creates the device `/d0/'.  The file `/d0/dog' is accessed by the same name,
`/d0/dog'.

Before mounting a file system, the host must already have been created
with hostAdd().  The routine nfsDevShow() displays the mounted NFS devices.

IOCTL FUNCTIONS
The NFS driver responds to the following ioctl() functions:
.iP "FIOGETNAME" 18 3
Gets the file name of <fd> and copies it to the buffer referenced 
by <nameBuf>:
.CS
    status = ioctl (fd, FIOGETNAME, &nameBuf);
.CE
.iP "FIONREAD"
Copies to <nBytesUnread> the number of bytes remaining in the file
specified by <fd>:
.CS
    status = ioctl (fd, FIONREAD, &nBytesUnread);
.CE
.iP "FIOSEEK"
Sets the current byte offset in the file to the position specified by
<newOffset>.  If the seek goes beyond the end-of-file, the file grows.
The end-of-file pointer gets moved to the new position, and the new space
is filled with zeros:
.CS
    status = ioctl (fd, FIOSEEK, newOffset);
.CE
.iP "FIOSYNC"
Flush data to the remote NFS file.  It takes no additional argument:
.CS
    status = ioctl (fd, FIOSYNC, 0);
.CE
.iP "FIOWHERE"
Returns the current byte position in the file.  This is the byte offset of
the next byte to be read or written.  It takes no additional argument:
.CS
    position = ioctl (fd, FIOWHERE, 0);
.CE
.iP "FIOREADDIR"
Reads the next directory entry.  The argument <dirStruct> is a pointer to
a directory descriptor of type DIR.  Normally, the readdir() routine is used to
read a directory, rather than using the FIOREADDIR function directly.  See
the manual entry for dirLib:
.CS
    DIR dirStruct;
    fd = open ("directory", O_RDONLY);
    status = ioctl (fd, FIOREADDIR, &dirStruct);
.CE
.iP "FIOFSTATGET"
Gets file status information (directory entry data).  The argument
<statStruct> is a pointer to a stat structure that is filled with data
describing the specified file.  Normally, the stat() or fstat() routine is
used to obtain file information, rather than using the FIOFSTATGET
function directly.  See the manual entry for dirLib:
.CS
    struct stat statStruct;
    fd = open ("file", O_RDONLY);
    status = ioctl (fd, FIOFSTATGET, &statStruct);
.CE

.iP "FIOFSTATFSGET"
Gets the file system parameters for and open file descriptor.  The argument
<statfsStruct> is a pointer to a statfs structure that is filled with data
describing the underlying filesystem.  Normally, the stat() or fstat() routine
is used to obtain file information, rather than using the FIOFSTATGET function
directly.  See the manual entry for dirLib:

.CS
    statfs statfsStruct;
    fd = open ("directory", O_RDONLY);
    status = ioctl (fd, FIOFSTATFSGET, &statfsStruct);
.CE

DEFICIENCIES
There is only one client handle/cache per task.
Performance is poor if a task is accessing two or more NFS files.

Changing <nfsCacheSize> after a file is open could cause adverse effects.
However, changing it before opening any NFS file descriptors should not
pose a problem.

INCLUDE FILES: nfsDrv.h, ioLib.h, dirent.h

SEE ALSO: dirLib, nfsLib, hostAdd(), ioctl(),
*/

#include "vxWorks.h"
#include "iosLib.h"
#include "ioLib.h"
#include "memLib.h"
#include "stdlib.h"
#include "semLib.h"
#include "string.h"
#include "lstLib.h"
#include "rpc/rpc.h"
#include "xdr_nfs.h"
#include "xdr_nfsserv.h"
#include "hostLib.h"
#include "sys/stat.h"
#include "dirent.h"
#include "errnoLib.h"
#include "errno.h"
#include "stdio.h"
#include "nfsLib.h"
#include "nfsDrv.h"
#include "pathLib.h"
#include "fcntl.h"
#include "memPartLib.h"
#include "private/semLibP.h"

IMPORT unsigned int nfsMaxMsgLen;	/* max. length of NFs read/write buf */
IMPORT int          nfsMaxPath;         /* max. length of file path */

/* XXX FD_MODE is defined in here and in netDrv */

#define FD_MODE		3	/* mode mask for opened file */
				/* O_RDONLY, O_WRONLY or O_RDWR */
#ifdef __GNUC__
# ifndef alloca
#  define alloca __builtin_alloca
# endif
#endif

typedef struct			/* NFS_DEV - nfs device structure */
    {
    DEV_HDR	devHdr;			  /* nfs device header */
    char	host[MAXHOSTNAMELEN];	  /* host for this device */
    nfs_fh	fileHandle;		  /* handle for mounted file system */

    /*
     * File system mounted on this dev - though it appears as a single 
     * character, it is really a character array - the memory for the array
     * is allocated when an NFS_DEV struct is allocated. This change was 
     * required for configurable file path length support.
     */

    char	fileSystem[1];            
    } NFS_DEV;

/* nfs file descriptor */

typedef struct			/* NFS_FD - NFS file descriptor */
    {
    NFS_DEV	*nfsDev;		/* pointer to this file's NFS device */
    nfs_fh	 fileHandle;		/* file's file handle */
    nfs_fh	 dirHandle;		/* file's directory file handle */
    fattr	 fileAttr;		/* file's attributes */
    unsigned int mode;			/* (O_RDONLY, O_WRONLY, O_RDWR) */
    unsigned int fileCurByte;		/* number of current byte in file */
    SEM_ID	 nfsFdSem;		/* accessing semaphore */
    BOOL	 cacheValid;		/* TRUE: cache valid and remaining */
					/* fields in structure are valid. */
					/* FALSE: cache is empty or */
					/* cacheCurByte not the same as the */
					/* current byte in the file. */
    char	*cacheBuf;		/* pointer to file's read cache */
    char 	*cacheCurByte;		/* pointer to current byte in cache */
    unsigned int cacheBytesLeft;	/* number of bytes left in cache */
					/* between cacheCurByte and the end */
					/* of valid cache material.  NOTE: */
					/* This number may be smaller than */
					/* the amount of room left in for */
					/* the cache writing. */
    BOOL	 cacheDirty;		/* TRUE: cache is dirty, not flushed */
    } NFS_FD;

/* globals */

int mutexOptionsNfsDrv = SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE;

unsigned int nfsCacheSize;		/* size of an nfs cache */
BOOL	 nfsAutoClose;			/* TRUE: close client after file close*/

/* locals */

LOCAL int nfsDrvNum;			/* this particular driver number */

/* forward declarations */

LOCAL int nfsCacheFlush ();
LOCAL STATUS nfsClose ();
LOCAL int nfsCreate ();
LOCAL int nfsDelete ();
LOCAL int nfsIoctl ();
LOCAL int nfsOpen ();
LOCAL int nfsRead ();
LOCAL int nfsSeek  ();
LOCAL int nfsWrite ();
LOCAL int nfsChkFilePerms ();

/*******************************************************************************
*
* nfsDrv - install the NFS driver
*
* This routine initializes and installs the NFS driver.  It must be 
* called before any reads, writes, or other NFS calls.
* This is done automatically when INCLUDE_NFS is defined.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  This restriction does not apply under non-AE 
* versions of VxWorks.  
*
* RETURNS:
* OK, or ERROR if there is no room for the driver.
*/

STATUS nfsDrv (void)
    {
    if (nfsDrvNum == 0)
	{
	nfsCacheSize = nfsMaxMsgLen;

	nfsDrvNum = iosDrvInstall (nfsCreate, nfsDelete, nfsOpen, nfsClose,
				   nfsRead, nfsWrite, nfsIoctl);
	}

    return (nfsDrvNum == ERROR ? ERROR : OK);
    }
/*******************************************************************************
*
* nfsDrvNumGet - return the IO system driver number for the nfs driver
*
* This routine returns the nfs driver number allocated by iosDrvInstall during
* the nfs driver initialization. If the nfs driver has yet to be initialized,
* or if initialization failed, nfsDrvNumGet will return ERROR.
*
* RETURNS: the nfs driver number or ERROR
*/

int nfsDrvNumGet (void)
    {
    return (nfsDrvNum == 0 ? ERROR : nfsDrvNum);
    }

/*******************************************************************************
*
* nfsMount - mount an NFS file system
*
* This routine mounts a remote file system.  It creates a
* local device <localName> for a remote file system on a specified host.
* The host must have already been added to the local host table with
* hostAdd().  If <localName> is NULL, the local name will be the same as
* the remote name.
*
* RETURNS: OK, or ERROR if the driver is not installed,
* <host> is invalid, or memory is insufficient.
*
* SEE ALSO: nfsUnmount(), hostAdd()
*/

STATUS nfsMount
    (
    char *host,         /* name of remote host */
    char *fileSystem,   /* name of remote directory to mount */
    char *localName     /* local device name for remote dir */
                        /* (NULL = use fileSystem name) */
    )
    {
    FAST NFS_DEV *pNfsDev;
    nfs_fh fileHandle;

    if (nfsDrvNum < 1)
	{
	errnoSet (S_ioLib_NO_DRIVER);
	return (ERROR);
	}

    if (strlen (fileSystem) >= nfsMaxPath)
	{
	errnoSet (S_nfsLib_NFSERR_NAMETOOLONG);
	return (ERROR);
	}

    if ((pNfsDev = (NFS_DEV *) KHEAP_ALLOC(sizeof (NFS_DEV) + nfsMaxPath - 1)) 
	   == NULL)
	return (ERROR);

    /* perform an nfs mount of the directory */

    if (nfsDirMount (host, fileSystem, &fileHandle) != OK)
	{
	KHEAP_FREE((char *) pNfsDev);
	return (ERROR);
	}

    /* fill in the nfs device descripter */

    (void) strcpy (pNfsDev->host, host);
    (void) strcpy (pNfsDev->fileSystem, fileSystem);
    bcopy ((char *) &fileHandle, (char *) &pNfsDev->fileHandle, sizeof(nfs_fh));

    /*
     * If no device name was specified, use the name of the file system
     * being mounted.  For instance, if the file system is called "/d0",
     * the corresponding nfs device will be named "/d0".
     */

    if (localName == NULL)
	localName = fileSystem;		/* name the same as remote fs */

    /* add device to I/O system */

    if (iosDevAdd ((DEV_HDR *) pNfsDev, localName, nfsDrvNum) != OK)
	{
	KHEAP_FREE((char *) pNfsDev);
	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* nfsMountAll - mount all file systems exported by a specified host
*
* This routine mounts the file systems exported by the host <pHostName>
* which are accessible by <pClientName>. A <pClientName> entry of NULL
* will only mount file systems that are accessible by any client. 
* The nfsMount() routine is called to mount each file system. It creates
* a local device for each mount that has the same name as the remote file
* system.
*
* If the <quietFlag> setting is FALSE, each file system is printed on
* standard output after it is mounted successfully.
*
* RETURNS: OK, or ERROR if any mount fails.
*
* SEE ALSO: nfsMount()
*/

STATUS nfsMountAll
    (
    char *pHostName, 	/* name of remote host */
    char *pClientName, 	/* name of a client specified in access list, if any */
    BOOL quietFlag      /* FALSE = print name of each mounted file system */
    )
    {
    exports	nfsExportBody;
    exports 	pExport;
    groups 	pGroup;
    BOOL	accessFlag;
    STATUS	status = OK;

    if (nfsExportRead (pHostName, &nfsExportBody) != OK)
        return (ERROR);

    if (nfsExportBody)
        {
        /* Attempt to mount each file system in export list */

        pExport = nfsExportBody;
        while (pExport != NULL)
            {
            accessFlag = TRUE;      /* Allow mounting if not restricted. */

            pGroup = pExport->ex_groups;

            if (pGroup != NULL)
                {
                accessFlag = FALSE;    /* Limit mount to matching clients. */

                if (pClientName != NULL)
                    {
                    /* Check for match with client name. */

                    while (!accessFlag && pGroup != NULL)
                        {
                        if (strcmp (pGroup->gr_name, pClientName))
                            pGroup = pGroup->gr_next;
                        else
                            accessFlag = TRUE;
                        }
                    }
                }

            if (accessFlag)
                {
                if (nfsMount (pHostName, pExport->ex_dir, (char *) NULL) != OK)
                    status = ERROR;
                else
                    {
                    if (!quietFlag)
                        printf ("%s\n", pExport->ex_dir);
                    }
                }

            pExport = pExport->ex_next;
            }
	}

    nfsExportFree (&nfsExportBody);

    return (status);
    }

/*******************************************************************************
*
* nfsDevShow - display the mounted NFS devices
*
* This routine displays the device names and their associated NFS file systems.
*
* EXAMPLE:
* .CS
*     -> nfsDevShow
*     device name          file system
*     -----------          -----------
*     /yuba1/              yuba:/yuba1
*     /wrs1/               wrs:/wrs1
* .CE
*
* RETURNS: N/A
*/

void nfsDevShow (void)
    {
    char    *fileSysName;
    DEV_HDR *pDev0 = NULL;
    DEV_HDR *pDev1;

    if ((fileSysName = (char *) alloca (nfsMaxPath)) == NULL)
	{
	printf ("Memory allocation error\n");
	return;
	}

    printf ("%-20.20s %-50.50s\n", "device name", "file system");
    printf ("%-20.20s %-50.50s\n", "-----------", "-----------");

    /* get entries from the I/O system's device list */

    while ((pDev1 = iosNextDevGet (pDev0)) != NULL)
	{
	if (pDev1->drvNum == nfsDrvNum)
	    {
	    /* found an nfs device, print information */

	    (void) strcpy (fileSysName, ((NFS_DEV *) pDev1)->host);
	    (void) strcat (fileSysName, ":");
	    (void) strcat (fileSysName, ((NFS_DEV *) pDev1)->fileSystem);
	    printf ("%-20.20s %-50.50s\n", pDev1->name, fileSysName);
	    }
	pDev0 = pDev1;
	}
    }
/*******************************************************************************
*
* nfsUnmount - unmount an NFS device
*
* This routine unmounts file systems that were previously mounted via NFS.
*
* RETURNS: OK, or ERROR if <localName> is not an NFS device or cannot
* be mounted.
* 
* SEE ALSO: nfsMount()
*/

STATUS nfsUnmount
    (
    char *localName     /* local of nfs device */
    )
    {
    FAST NFS_DEV *pNfsDev;
    char *dummy;

    /* find the device in the I/O system */

#ifdef _WRS_VXWORKS_5_X
    if ((pNfsDev = (NFS_DEV *) iosDevFind (localName, 
                                           (char **)&dummy)) == NULL)
#else
    if ((pNfsDev = (NFS_DEV *) iosDevFind (localName, 
                                           (const char **)&dummy)) == NULL)
#endif /* _WRS_VXWORKS_5_X */
        
	{
	return (ERROR);
	}

    /* make sure device is an nfs driver and the names match exactly */

    if ((pNfsDev->devHdr.drvNum != nfsDrvNum) ||
	(strcmp (pNfsDev->devHdr.name, localName) != 0))
	{
	errnoSet (S_nfsDrv_NOT_AN_NFS_DEVICE);
	return (ERROR);
	}

    /* perform an nfs unmount of the directory */

    if (nfsDirUnmount (pNfsDev->host, pNfsDev->fileSystem) == ERROR)
	return (ERROR);

    /* delete the device from the I/O system */

    iosDevDelete ((DEV_HDR *) pNfsDev);
    KHEAP_FREE((char *) pNfsDev);
    return (OK);
    }

/*******************************************************************************
*
* nfsDevListGet - create list of all the NFS devices in the system
*
* This routine fills the array <nfsDevlist> up to <listSize>, with handles to
* NFS devices currently in the system.
*
* RETURNS: The number of entries filled in the <nfsDevList> array.
* 
* SEE ALSO: nfsDevInfoGet()
*/
int nfsDevListGet
    (
    unsigned long nfsDevList[],	/* NFS dev list of handles */
    int           listSize	/* number of elements available in the list */
    )
    {
    int       numMounted;
    DEV_HDR * pDev0 = NULL;
    DEV_HDR * pDev1;


    /*  Collect information of all the NFS currently mounted.  */

    numMounted = 0;

    while ((numMounted < listSize) && ((pDev1 = iosNextDevGet (pDev0)) != NULL))
        {
        if (pDev1->drvNum == nfsDrvNum)
            {
            /* found an nfs device, save pointer to the device */

            nfsDevList [numMounted] = (unsigned long) pDev1;
            numMounted++;
            }
        pDev0 = pDev1;
        }

    return (numMounted);
    }


/*******************************************************************************
*
* nfsDevInfoGet - read configuration information from the requested NFS device
*
* This routine accesses the NFS device specified in the parameter <nfsDevHandle>
* and fills in the structure pointed to by <pnfsInfo>. The calling function 
* should allocate memory for <pnfsInfo> and for the two character buffers,
* 'remFileSys' and 'locFileSys', that are part of <pnfsInfo>. These buffers 
* should have a size of 'nfsMaxPath'.
*
* RETURNS: OK if <pnfsInfo> information is valid, otherwise ERROR.
* 
* SEE ALSO: nfsDevListGet()
*/
STATUS nfsDevInfoGet
    (
    unsigned long   nfsDevHandle,	/* NFS device handle */
    NFS_DEV_INFO  * pnfsInfo		/* ptr to struct to hold config info */
    )
    {
    NFS_DEV *pnfsDev;
    DEV_HDR *pDev0;
    DEV_HDR *pDev1;

    if (pnfsInfo == NULL)
	return (ERROR);

    /* Intialize pointer variables */

    pnfsDev = NULL;
    pDev0   = NULL;

    /* Verify that the device is still in the list of devices */

    while ((pDev1 = iosNextDevGet (pDev0)) != NULL)
        {
        if (pDev1 == (DEV_HDR *) nfsDevHandle)
	    {
	    pnfsDev = (NFS_DEV *) pDev1;
	    break;				/* Found Device */
	    }

        pDev0 = pDev1;
        }


     if (pnfsDev != NULL)
	{
	strcpy (pnfsInfo->hostName,   pnfsDev->host);
	strcpy (pnfsInfo->remFileSys, pnfsDev->fileSystem);
	strcpy (pnfsInfo->locFileSys, pnfsDev->devHdr.name);

	return (OK);
	}

    return (ERROR);
    }



/* routines supplied to the I/O system */

/*******************************************************************************
*
* nfsCreate - create a remote NFS file
*
* Returns an open nfs file descriptor.
*
* Used for creating files only, not directories.
* Called only by the I/O system.
*
* To give the file a particular mode (UNIX chmod() style), use nfsOpen().
*
* RETURNS: Pointer to NFS file descriptor, or ERROR.
*/

LOCAL int nfsCreate
    (
    NFS_DEV *pNfsDev,   /* pointer to nfs device */
    char *fileName,     /* nfs file name (relative to mount point) */
    int mode            /* mode (O_RDONLY, O_WRONLY, or O_RDWR) */
    )
    {
    /* file descriptor mode legality is checked for in nfsOpen */

    /* don't allow null filenames */

    if (fileName [0] == EOS)
	{
	errnoSet (S_ioLib_NO_FILENAME);
	return (ERROR);
	}

    /* open the file being created,
       give the file default UNIX file permissions */

    return (nfsOpen (pNfsDev, fileName , O_CREAT | O_TRUNC | mode,
                     DEFAULT_FILE_PERM));
    }
/*******************************************************************************
*
* nfsDelete - delete a remote file
*
* Deletes a file on a remote system.
* Called only by the I/O system.
*
* RETURNS: OK or ERROR.
*/

LOCAL int nfsDelete
    (
    NFS_DEV *pNfsDev,   /* pointer to nfs device */
    char *fileName      /* remote file name */
    )
    {

    /* don't allow null filenames */

    if (fileName [0] == EOS)
	{
	errnoSet (S_ioLib_NO_FILENAME);
	return (ERROR);
	}

    return (nfsFileRemove (pNfsDev->host, &pNfsDev->fileHandle, fileName));
    }
/*******************************************************************************
*
* nfsChkFilePerms - check the NFS file permissions with a given permission.
*
* This routine compares the NFS file permissions with a given permission.
*
* This routine is basically designed for nfsOpen() to verify
* the target file's permission prior to deleting it due to O_TRUNC.
*
* The parameter "perm" will take 4(read), 2(write), 1(execute), or
* combinations of them.
*
* OK means the file has valid permission whichever group is.
* FOLLOW_LINK means this path name contains link.  ERROR means
* the file doesn't have matched permissions.
* 
* Called only by the I/O system.
*
* RETURNS: OK, FOLLOW_LINK, ERROR
*/

LOCAL int nfsChkFilePerms
    (
    NFS_DEV *	pNfsDev,		/* pointer to nfs device */
    char *	fullName,		/* full file or directory name */
    int		perm			/* 4:read 2:write 1:execute */
    )
    {
    diropres	dirResult;		/* directory info returned via nfs */
    nfs_fh	dirHandle;		/* file's directory file handle */
    int		retVal;			/* return from nfsLookUpByName */

    /* NFS permissions */
 
    int     nfsPerms;			/* permissions of the opened file */
    char    machineName [AUTH_UNIX_FIELD_LEN];
    int     uid;		        /* user ID */
    int     gid;		        /* group ID */
    int     nGids;		        /* number of groups */
    int     gids [MAX_GRPS];	        /* array of group IDs */
    BOOL    sameUid = FALSE;	        /* set to TRUE if users match */
    BOOL    sameGid = FALSE;	        /* set to TRUE if groups match */

    bzero ((char *) &dirResult, sizeof (dirResult));
    bzero ((char *) &dirHandle, sizeof (dirHandle));
    bzero ((char *) &gids, sizeof (int) * MAX_GRPS);


    if ((retVal = nfsLookUpByName (pNfsDev->host, fullName, 
				   &pNfsDev->fileHandle, 
				   &dirResult, &dirHandle)) != OK)
	return (retVal);	/* ERROR/FOLLOW_LINK. errno has S_nfsLib prefix */ 

    nfsPerms = dirResult.diropres_u.diropres.attributes.mode;
    nfsAuthUnixGet (machineName, &uid, &gid, &nGids, gids);
 
    sameUid = (uid == dirResult.diropres_u.diropres.attributes.uid
               ? TRUE : FALSE);
 
    if (gid == dirResult.diropres_u.diropres.attributes.gid)
	sameGid = TRUE;
    else while (nGids)
	if (gids [--nGids] == dirResult.diropres_u.diropres.attributes.gid)
            {
            sameGid = TRUE;
            break;
	    }

    /* Check "other" permissions */
    
    if (!sameGid && !sameUid &&	((nfsPerms & perm) == perm))
        goto permission_ok;

    /* check group permissions */
    
    else if (sameGid && ((nfsPerms & (perm << 3)) == (perm << 3)))
        goto permission_ok;

    /* check user permissions */
 
    else if (sameUid && ((nfsPerms & (perm << 6)) == (perm << 6)))
        goto permission_ok;

    else
	{
	errno = S_nfsDrv_PERMISSION_DENIED;
	return (ERROR);		/* caller should set EACCES to errno */
	}

permission_ok:
    return (OK);
    }
/*******************************************************************************
*
* nfsOpen - open an NFS file
*
* This routine opens the remote file.
* Called only by the I/O system.
*
* RETURNS: pointer to open network file descriptor || FOLLOW_LINK || ERROR
*/

LOCAL int nfsOpen
    (
    NFS_DEV * pNfsDev,		/* pointer to nfs device */
    char *    fileName,		/* remote file or directory name to open */
    int       flags,		/* O_RDONLY, O_WRONLY, or O_RDWR and O_CREAT */
    int       mode		/* UNIX chmod style */
    )
    {
    int status = OK;
    char *fullName;	                /* full file or directory name */
    FAST NFS_FD * nfsFd;
    diropres dirResult;			/* directory info returned via nfs */
    int	retVal;
    nfs_fh dirHandle;			/* file's directory file handle */
    int openMode = flags & FD_MODE;	/* mode to open file with */
    					/* (O_RDONLY, O_WRONLY, or O_RDWR */
    /* NFS permissions */
    int     filePermissions;	       	/* permissions of the opened file */
    int	    requestPerms = 0;		/* requested file permissions */
    char    machineName [AUTH_UNIX_FIELD_LEN];
    int     uid;		        /* user ID */
    int     gid;		        /* group ID */
    int     nGids;		        /* number of groups */
    int     gids [MAX_GRPS];	        /* array of group IDs */
    BOOL    correctPerms = FALSE;       /* set to TRUE if perms match */
    BOOL    sameUid = FALSE;	        /* set to TRUE if users match */
    BOOL    sameGid = FALSE;	        /* set to TRUE if groups match */
    
    if ((openMode != O_RDONLY) && (openMode != O_WRONLY) && 
        (openMode != O_RDWR))
	{
	errnoSet (S_nfsDrv_BAD_FLAG_MODE);
	return (ERROR);
	}

    if (strlen (fileName) >= nfsMaxPath)
	{
	errnoSet (S_nfsLib_NFSERR_NAMETOOLONG);
	return (ERROR);
	}

    if ((fullName = (char *) alloca (nfsMaxPath)) == NULL)
	return (ERROR);

    if (fileName [0] == EOS)
	{
	if (flags & O_CREAT)
	    {
	    errnoSet (S_nfsDrv_CREATE_NO_FILE_NAME);
	    return (ERROR);
	    }
	(void) strcpy (fullName, ".");
	}
    else
	(void) strcpy (fullName, fileName);

    /* truncate file - check the file has write-permission */

    if (flags & O_TRUNC)
	{
	int err;
	
#define READ_PERMISSION 2

        if ((err = nfsChkFilePerms (pNfsDev, fullName,
				    READ_PERMISSION)) == FOLLOW_LINK)
	    return (FOLLOW_LINK);

	else if (err == ERROR && errno == S_nfsDrv_PERMISSION_DENIED)
	    return (ERROR);	/* i.e. don't return if NFSERR_NOENT */

        else if (err == OK)
            {

	    /* the file has write-permission, go for deletion */

	    retVal = nfsDelete (pNfsDev, fullName);
	    if ((retVal == ERROR) && !(flags & O_CREAT))
		return (ERROR);
	    else
		{
		if (retVal == FOLLOW_LINK)
		    {
		    (void) strcpy (fileName, fullName);
		    return (FOLLOW_LINK);
		    }
		}
	    }
	} /* O_TRUNC */


    if ((flags & O_CREAT) || (flags & O_TRUNC)) /* create file or directory */
        {
        retVal = nfsLookUpByName (pNfsDev->host, fullName, 
                                  &pNfsDev->fileHandle, 
                                  &dirResult, &dirHandle);
        if (retVal == FOLLOW_LINK)
            {
            (void) strcpy (fileName, fullName);
            return (FOLLOW_LINK);
            }

        status = nfsThingCreate (pNfsDev->host, fullName, &pNfsDev->fileHandle,
                                 &dirResult, &dirHandle, (u_int)mode);
        }
    else                        /* open existing file or directory */
        {
        status = nfsLookUpByName (pNfsDev->host, fullName, 
                                  &pNfsDev->fileHandle,
                                  &dirResult, &dirHandle);
        }
    if (status == ERROR)
	return (ERROR);

    /* Check file permissions */

    filePermissions = dirResult.diropres_u.diropres.attributes.mode;

    switch (openMode)
        {
	case O_RDONLY: requestPerms = 4; break;
	case O_WRONLY: requestPerms = 2; break;
	case O_RDWR:   requestPerms = 6; break;
	}

    /* Check if uid and gid match file uid and gid */
    
    nfsAuthUnixGet (machineName, &uid, &gid, &nGids, gids);

    sameUid = (uid == dirResult.diropres_u.diropres.attributes.uid
	       ? TRUE : FALSE);
    
    if (gid == dirResult.diropres_u.diropres.attributes.gid)
        sameGid = TRUE;
    else while (nGids)
        if (gids [--nGids] == dirResult.diropres_u.diropres.attributes.gid)
	    {
	    sameGid = TRUE;
	    break;
	    }

    /* Check "other" permissions */
    
    if (!sameGid && !sameUid &&
	((filePermissions & requestPerms) == requestPerms))
        correctPerms = TRUE;

    /* check group permissions */
    
    else if (sameGid &&
	     ((filePermissions & (requestPerms << 3)) == (requestPerms << 3)))
        correctPerms = TRUE;

    /* check user permissions */

    else if (sameUid &&
	     ((filePermissions & (requestPerms << 6)) == (requestPerms << 6)))
        correctPerms = TRUE;

    else
	{
	errno = S_nfsDrv_PERMISSION_DENIED;
	return (ERROR);
	}
    
    /*
     * If the name returned by nfsLookUpByName doesn't start with a
     * slash, then nfsLookUpByName has changed the name to the link, but the
     * name no longer includes the name of the correct device.  If you pass
     * this to iosDevFind, it returns a path on the default device, which is
     * wrong (unless, of course, you get lucky and you are in fact looking
     * for something on the default device).  So, need to prepend the name of
     * the NFS device if fullName doesn't include it already.
     */

    if (status == FOLLOW_LINK)
        {
	if (fullName [0] != '/')
	    {
	    pathCat (pNfsDev->devHdr.name, fullName, fileName);
	    }
	else
	    {
	    (void) strcpy (fileName, fullName);
	    }
	}

    if (status != OK)
	return (status);

    if ((nfsFd = (NFS_FD *) KHEAP_ALLOC(sizeof (NFS_FD) + nfsCacheSize)) == NULL)
	return (ERROR);

    /* fill in file descriptor with newly retrieved information */

    bcopy ((char *) &dirResult.diropres_u.diropres.file,
	   (char *) &nfsFd->fileHandle, sizeof (nfs_fh));
    bcopy ((char *) &dirHandle,
	   (char *) &nfsFd->dirHandle, sizeof (nfs_fh));
    bcopy ((char *) &dirResult.diropres_u.diropres.attributes,
	   (char *) &nfsFd->fileAttr, sizeof (fattr));

    nfsFd->fileCurByte	= 0;
    nfsFd->mode		= openMode;
    nfsFd->cacheValid	= FALSE;

    nfsFd->cacheBuf	= (char *) ((u_int) nfsFd + sizeof (NFS_FD));

    nfsFd->cacheCurByte = nfsFd->cacheBuf;
    nfsFd->cacheDirty	= FALSE;
    nfsFd->cacheBytesLeft = 0;

    nfsFd->nfsDev	= pNfsDev;

    nfsFd->nfsFdSem = semMCreate (mutexOptionsNfsDrv);
    if (nfsFd->nfsFdSem == NULL)
        {
        free ( (char *)nfsFd);
        return (ERROR);
        }

    return ((int) nfsFd);
    }
/*******************************************************************************
*
* nfsClose - close a remote file
*
* Called only by the I/O system.
*
* RETURNS: OK or ERROR if file failed to flush
*/

LOCAL STATUS nfsClose
    (
    FAST NFS_FD *nfsFd          /* nfs file descriptor */
    )
    {
    int status = OK;

    semTake (nfsFd->nfsFdSem, WAIT_FOREVER);

    if (nfsFd->cacheDirty)
	status = nfsCacheFlush (nfsFd);

    semDelete (nfsFd->nfsFdSem);	/* terminate nfs fd semaphore */
    KHEAP_FREE((char *) nfsFd);

    /* close client if "auto close" is selected */

    if (nfsAutoClose)
	nfsClientClose ();

    return (status == ERROR ? ERROR : OK);
    }
/*******************************************************************************
*
* nfsRead - read bytes from remote file
*
* nfsRead reads the specified number of bytes from the open NFS
* file and puts them into a buffer.  Bytes are read starting
* from the point marked by the file pointer.  The file pointer is then
* updated to point immediately after the last character that was read.
*
* A cache is used for keeping nfs network reads and writes down to a minimum.
*
* Called only by the I/O system.
*
* SIDE EFFECTS: moves file pointer
*
* RETURNS: number of bytes read or ERROR.
*/

LOCAL int nfsRead
    (
    FAST NFS_FD *nfsFd, /* pointer to open network file descriptor */
    char *buf,          /* pointer to buffer to receive bytes */
    FAST int maxBytes   /* max number of bytes to read into buffer */
    )
    {
    int nRead;			/* number of bytes read from cache or net */
    int nCacheRead;		/* number of bytes read into cache */
    int readCount;		/* cum. no. of bytes read into user's buf */
    STATUS status = OK;

    /* check for valid maxBytes */

    if (maxBytes < 0)
	{
	errnoSet (S_nfsDrv_INVALID_NUMBER_OF_BYTES);
	return (ERROR);
	}

    if (maxBytes == 0)
	return (0);

    /* if file opened for O_WRONLY, don't read */

    if (nfsFd->mode == O_WRONLY)
	{
	errnoSet (S_nfsDrv_WRITE_ONLY_CANNOT_READ);
	return (ERROR);
	}

    readCount = 0;

    semTake (nfsFd->nfsFdSem, WAIT_FOREVER);

    if (nfsCacheSize == 0)
	{
	/* no caching */

	readCount = nfsFileRead (nfsFd->nfsDev->host, &nfsFd->fileHandle,
			         nfsFd->fileCurByte, (unsigned) maxBytes, buf,
			         &nfsFd->fileAttr);

	if (readCount < 0)
	    {
	    semGive (nfsFd->nfsFdSem);
	    return (ERROR);
	    }

	status = nfsSeek (nfsFd, (int) nfsFd->fileCurByte + readCount);
	}
    else
	{
	/* read from cache */

	while ((readCount < maxBytes) && (status != ERROR))
	    {
	    /* keep reading until all bytes requested are read,
	     * or an error occurs,
	     * or the end of file is hit
	     */

	    if (!nfsFd->cacheValid)
		{
		/* if the cache isn't valid,
		 * freshen it by reading across the net.
		 */

		nCacheRead = nfsFileRead (nfsFd->nfsDev->host,
					  &nfsFd->fileHandle,
					  nfsFd->fileCurByte, nfsCacheSize,
					  nfsFd->cacheBuf, &nfsFd->fileAttr);
		if (nCacheRead < 0)
		    {
		    semGive (nfsFd->nfsFdSem);
		    return (ERROR);
		    }

		nfsFd->cacheCurByte = nfsFd->cacheBuf;	/* update cache ptrs */
		nfsFd->cacheBytesLeft = nCacheRead;

		nfsFd->cacheValid = TRUE;		/* cache is valid */
		}

	    /* read as many bytes as possible from what's left in the cache,
	     * up to the amount requested
	     */

	    if (nfsFd->cacheBytesLeft > 0)
		{
		if (maxBytes - readCount < nfsFd->cacheBytesLeft)
		    nRead = maxBytes - readCount;
		else
		    nRead = nfsFd->cacheBytesLeft;

		/* copy bytes into user's buffer */

		bcopy (nfsFd->cacheCurByte, (char *) ((int) buf + readCount),
		       nRead);

		/* update file pointer */

		status = nfsSeek (nfsFd, (int) nfsFd->fileCurByte + nRead);

		/* how many bytes have we read so far? */

		readCount += nRead;
		}
	    else
		break;		/* we've hit the end of the file */
	    } /* while */
	} /* else */

    semGive (nfsFd->nfsFdSem);

    return (status == ERROR ? ERROR : readCount);
    }
/*******************************************************************************
*
* nfsWrite - write bytes to remote file
*
* nfsWrite copies the specified number of bytes from the buffer
* to the nfs file.  Bytes are written starting at the current file pointer.
* The file pointer is updated to point immediately after the last
* character that was written.
*
* A cache is used for keeping nfs network reads and writes down to a minimum.
* If all goes well, the entire buffer is written to the cache and/or actual
* file.
*
* Called only by the I/O system.
*
* SIDE EFFECTS: moves file pointer
*
* RETURNS:
*    Number of bytes written (error if != nBytes)
*    ERROR if nBytes < 0, or nfs write is not successful
*/

LOCAL int nfsWrite
    (
    FAST NFS_FD *nfsFd, /* open file descriptor */
    char *buf,          /* buffer being written from */
    FAST int nBytes     /* number of bytes to write to file */
    )
    {
    FAST unsigned int nWrite;	/* number of bytes to write */
    FAST unsigned int newPos;	/* new position of file pointer */
    unsigned int cacheMargin;	/* margin left in cache for writing */
    int		nCacheRead;	/* number of bytes read into cache */
    FAST char	*pBuf = buf;	/* ptr to spot in user's buffer */
    FAST int	writeCount = 0;	/* number of bytes written so far */

    /* check for valid number of bytes */

    if (nBytes < 0)
	{
	errnoSet (S_nfsDrv_INVALID_NUMBER_OF_BYTES);
	return (ERROR);
	}

    /* if file opened for O_RDONLY, don't write */

    if (nfsFd->mode == O_RDONLY)
	{
	errnoSet (S_nfsDrv_READ_ONLY_CANNOT_WRITE);
	return (ERROR);
	}

    semTake (nfsFd->nfsFdSem, WAIT_FOREVER);

    /* do the write, until entire buffer has been written out */

    if (nfsCacheSize == 0)
	{
	writeCount = nfsFileWrite (nfsFd->nfsDev->host, &nfsFd->fileHandle,
			       nfsFd->fileCurByte, (unsigned) nBytes, buf,
			       &nfsFd->fileAttr);

	if (writeCount == ERROR)
	    {
	    semGive (nfsFd->nfsFdSem);
	    return (ERROR);
	    }

	/* where will the new file pointer be? */

	newPos = nfsFd->fileCurByte + writeCount;

	/* update the file pointer.  */

	if (nfsSeek (nfsFd, (int) newPos) == ERROR)
	    {
	    semGive (nfsFd->nfsFdSem);
	    return (ERROR);
	    }
	}
    else
	{
	while (writeCount < nBytes)
	    {
	    /* fill cache before writing to it, keep cache in sync w/file */

	    if (!nfsFd->cacheValid)
		{
		if ((nBytes - writeCount) < nfsCacheSize)
		    {
		    nCacheRead = nfsFileRead (nfsFd->nfsDev->host,
					      &nfsFd->fileHandle,
					      nfsFd->fileCurByte,
					      nfsCacheSize, nfsFd->cacheBuf,
					      &nfsFd->fileAttr);
		    if (nCacheRead < 0)
			{
			semGive (nfsFd->nfsFdSem);
			return (ERROR);
			}

		    nfsFd->cacheBytesLeft = nCacheRead;
		    }
		else
		    {
		    nfsFd->cacheBytesLeft = nfsCacheSize;
		    }

		/* update cache pointers */

		nfsFd->cacheCurByte = nfsFd->cacheBuf;
		nfsFd->cacheDirty = FALSE;
		nfsFd->cacheValid = TRUE;
		}
	    /* margin allowable for writing */

	    cacheMargin = (unsigned) nfsFd->cacheBuf + nfsCacheSize
			  - (unsigned) nfsFd->cacheCurByte;

	    if (nBytes - writeCount < cacheMargin)
		nWrite = nBytes - writeCount;
	    else
		nWrite = cacheMargin;

	    /* copy the bytes into the cache */

	    bcopy (pBuf, nfsFd->cacheCurByte, (int) nWrite);

	    if (nWrite > nfsFd->cacheBytesLeft)
		nfsFd->cacheBytesLeft = nWrite;

	    /* cache has been written in, it is soiled */

	    nfsFd->cacheDirty = TRUE;

	    /* what's the new position of the file pointer? */

	    newPos = nfsFd->fileCurByte + nWrite;

	    /* if the new file pointer reaches past the end of the file,
	     * the file has grown, update the size of the file
	     */

	    if (newPos > nfsFd->fileAttr.size)
		nfsFd->fileAttr.size = newPos;

	    /* update the file pointer.
	     * any cache flushes will occur during the seek.
	     */

	    if (nfsSeek (nfsFd, (int) newPos) == ERROR)
		{
		semGive (nfsFd->nfsFdSem);
		return (ERROR);
		}

	    writeCount += nWrite;
	    pBuf += nWrite;
	    } /* while entire buffer has not been written */
	} /* else */

    semGive (nfsFd->nfsFdSem);

    return (writeCount);
    }
/*******************************************************************************
*
* nfsIoctl - do device specific control function
*
* Called only by the I/O system.
*
* RETURNS: whatever the called function returns
*/

LOCAL int nfsIoctl
    (
    FAST NFS_FD *nfsFd, /* open nfs file descriptor */
    FAST int function,  /* function code */
    FAST int arg        /* argument for function */
    )
    {
    FAST struct stat *pStat;
    int status = OK;

    switch (function)
	{
	case FIOSYNC:
	    semTake (nfsFd->nfsFdSem, WAIT_FOREVER);
	    if (nfsFd->cacheDirty)
		status = nfsCacheFlush (nfsFd);
	    semGive (nfsFd->nfsFdSem);
	    break;

	case FIOSEEK:
	    status = nfsSeek (nfsFd, arg);
	    break;

	case FIOWHERE:
    	    status = nfsFd->fileCurByte;
	    break;

	case FIONREAD:
    	    *((int *) arg) = nfsFd->fileAttr.size - nfsFd->fileCurByte;
	    break;

	case FIOREADDIR:
	    /* read a directory entry */

	    status = nfsDirReadOne (nfsFd->nfsDev->host, &nfsFd->fileHandle,
				    (DIR *) arg);
	    break;

	case FIOFSTATGET:
	    /* get status of a file */

	    pStat = (struct stat *) arg;

	    /* get file attributes returned in pStat */

	    nfsFileAttrGet (&nfsFd->fileAttr, pStat);

	    pStat->st_dev         = (ULONG) nfsFd->nfsDev;

	    break;

        case FIOFSTATFSGET:
            status = nfsFsAttrGet(nfsFd->nfsDev->host, &nfsFd->fileHandle,
                                  (struct statfs *)arg);
            break;

	case FIOGETFL:
	    *((int *) arg) = nfsFd->mode;
	    break;
	    
	default:
	    errnoSet (S_ioLib_UNKNOWN_REQUEST);
	    status = ERROR;
	    break;
	}

    return (status);
    }

/*******************************************************************************
*
* nfsSeek - change file's current character position
*
* This routine moves the file pointer by the offset to a new position.
* If the new position moves the file pointer out of the range of what's
* in the cache and the cache is dirty (i.e. the cache has been written
* in), then the cache is written out to the nfs file and the cache is
* marked as invalid.  If the new position is beyond EOF, then an empty
* area is created that will read as zeros.
*
* Called only by the I/O system.
*
* RETURNS: OK | ERROR
*/

LOCAL int nfsSeek
    (
    FAST NFS_FD *nfsFd,
    FAST int newPos
    )
    {
    FAST int interval;

    if (newPos < 0)
	return (ERROR);

    if (newPos == nfsFd->fileCurByte)
	return (OK);

    semTake (nfsFd->nfsFdSem, WAIT_FOREVER);

    if (nfsFd->cacheValid)	/* if cache is valid, update cache pointer */
	{
	/* how far will file pointer be moved? */

	interval = newPos - nfsFd->fileCurByte;

	/* if the new pointer precedes what's in the cache,
	 * OR if the new ptr points past what's in the cache,
	 * OR if the new ptr points past the end of the valid cache bytes,
	 * THEN the cache is no longer valid.
	 *
	 * NOTE:  The cache is still valid if the new pointer points
	 * IMMEDIATELY after the valid bytes in the cache, but still
	 * hasn't reached the end of the cache buffer.  The cache can
	 * still be written to and should not be flushed until it is full
	 * or the file is closed (a similar technique is used in some states
	 * during drought conditions).
	 */

	if (((interval < 0) && (nfsFd->cacheCurByte + interval <
				nfsFd->cacheBuf))
	    || ((interval > 0) && (interval > nfsFd->cacheBytesLeft))
	    || ((nfsFd->cacheBuf + nfsCacheSize) <= (nfsFd->cacheCurByte +
						    interval)))
	    {
	    /* if the new cache pointer goes out of range of the cache,
	     * mark the cache as invalid, and flush if necessary.
	     */

	    if (nfsFd->cacheDirty)
		if (nfsCacheFlush (nfsFd) == ERROR)
		    {
		    semGive (nfsFd->nfsFdSem);
		    return (ERROR);
		    }
	    nfsFd->cacheValid = FALSE;
	    nfsFd->cacheBytesLeft = 0;
	    nfsFd->cacheCurByte = nfsFd->cacheBuf;
	    nfsFd->cacheDirty = FALSE;
	    }
	else
	    {
	    /* if the new position stays within range of the cache,
	     * update the cache pointers.
	     */

	    nfsFd->cacheCurByte += interval;
	    nfsFd->cacheBytesLeft -= interval;
	    }
	}

    nfsFd->fileCurByte = newPos;

    semGive (nfsFd->nfsFdSem);

    return (OK);
    }
/*******************************************************************************
*
* nfsCacheFlush - flush the cache to the remote NFS file
*
* Called only by the I/O system.
*
* RETURNS: number of bytes written or ERROR
*/

LOCAL int nfsCacheFlush
    (
    FAST NFS_FD *nfsFd
    )
    {
    int nWrite;		/* how many bytes are written with nfs? */
    unsigned int offset;/* offset in the file where cache gets written */
    unsigned int count;	/* number of bytes to write with nfs */
    unsigned int dist;	/* distance between beginning of cache and current file
			 * pointer (number of bytes) */

    if (!nfsFd->cacheValid)
	{
	errnoSet (S_nfsDrv_FATAL_ERR_FLUSH_INVALID_CACHE);
	return (ERROR);
	}

    dist   = nfsFd->cacheCurByte - nfsFd->cacheBuf;
    offset = nfsFd->fileCurByte - dist;
    count  = dist + nfsFd->cacheBytesLeft;

    nWrite = nfsFileWrite (nfsFd->nfsDev->host, &nfsFd->fileHandle,
			   offset, count, nfsFd->cacheBuf, &nfsFd->fileAttr);
    return (nWrite);
    }

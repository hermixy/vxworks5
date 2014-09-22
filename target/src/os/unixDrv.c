/* unixDrv.c - UNIX-file disk driver (VxSim for Solaris and VxSim for HP) */

/* Copyright 1984-1999 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/* 
modification history
--------------------
01j,16mar99,elg  mention unixDrv is for Solaris and HP simulators (SPR 7719).
01i,11jul97,dgp  doc: ad (VxSim) to title line
01h,09jan97,pr    added ARCHCVTFLAGS for simsolaris
01g,30oct95,ism   added support for simsolaris
01f,31oct94,kdl   made conditional for simulator only.
01e,18aug93,gae   modified unix_stat for hp.
01d,30jul93,gae   doc touchup.
01c,23jan93,gae   ANSIfied.
01b,20jul92,gae   minor revision.
01a,01apr91,smeg  derived from ramDrv.
*/

/*
DESCRIPTION
This driver emulates a VxWorks disk driver, but actually uses the UNIX
file system to store the data.  The VxWorks disk appears under
UNIX as a single file.
The UNIX file name, and the size of the disk, may be specified during the 
unixDiskDevCreate() call.

USER-CALLABLE ROUTINES
Most of the routines in this driver are accessible only through the I/O
system. The routine unixDrv() must be called to initialize the driver and
the unixDiskDevCreate() routine is used to create devices.

CREATING UNIX DISKS
Before a UNIX disk can be used, it must be created.  This is done
with the unixDiskDevCreate() call.  The format of this call is:

.CS
    BLK_DEV *unixDiskDevCreate
	(
	char	*unixFile,	/@ name of the UNIX file to use		@/
	int	bytesPerBlk,	/@ number of bytes per block		@/
	int	blksPerTrack,	/@ number of blocks per track		@/
	int	nBlocks		/@ number of blocks on this device	@/
	)
.CE

The UNIX file must be pre-allocated separately.  This
can be done using the UNIX mkfile(8) command.  Note that you have to
create an appropriately sized file.  For example, to create a UNIX
file system that is used as a common floppy dosFs file system, you
would issue the comand:
.CS
    mkfile 1440k /tmp/floppy.dos
.CE
This will create space for a 1.44 Meg DOS floppy (1474560 bytes,
or 2880 512-byte blocks).

The <bytesPerBlk> parameter specifies the size of each logical block
on the disk.  If <bytesPerBlk> is zero, 512 is the default.

The <blksPerTrack> parameter specifies the number of blocks on each
logical track of the UNIX disk.  If <blksPerTrack> is zero, the count of
blocks per track will be set to <nBlocks> (i.e., the disk will be defined 
as having only one track).  UNIX disk devices typically are specified
with only one track.

The <nBlocks> parameter specifies the size of the disk, in blocks.
If <nBlocks> is zero the size of the UNIX file specified, divided by
the number of bytes per block, is used.

The formatting parameters (<bytesPerBlk>, <blksPerTrack>, and <nBlocks>)
are critical only if the UNIX disk already contains the contents
of a disk created elsewhere.  In that case, the formatting 
parameters must be identical to those used when the image was created.  
Otherwise, they may be any convenient number.

Once the device has been created it still does not have a name or
file system associated with it.  This must be done by using the file 
system's device initialization routine (e.g., dosFsDevInit()).  The
dosFs and rt11Fs file systems also provide make-file-system routines
(dosFsMkfs() and rt11FsMkfs()), which may be used to associate a name
and file system with the block device and initialize that file system
on the device using default configuration parameters.

The unixDiskDevCreate() call returns a pointer to a block device
structure (BLK_DEV).  
This structure contains fields that describe the physical properties of a 
disk device and specify the addresses of routines within the UNIX disk driver.
The BLK_DEV structure address must be passed to the desired file system 
(dosFs, rt11Fs, or rawFs) during the file system's device 
initialization or make-file-system routine.  Only then is a name and file 
system associated with the device, making it available for use.

As an example, to create a 200KB disk, 512-byte blocks, and only one track,
the proper call would be:
.CS
    BLK_DEV *pBlkDev;

    pBlkDev = unixDiskDevCreate ("/tmp/filesys1",  512,  400,  400,  0);
.CE
This will attach the UNIX file /tmp/filesys1 as a block device.

A convenience routine, unixDiskInit(), is provided to do the
unixDiskDevCreate() followed by either a dosFsMkFs() or dosFsDevInit(),
whichever is appropriate.  

The format of this call is:

.CS
    BLK_DEV *unixDiskInit
	(
	char * unixFile,  /@ name of the UNIX file to use @/
	char * volName,   /@ name of the dosFs volume to use @/
	int    nBytes     /@ number of bytes in dosFs volume @/
	)
.CE

This call will create the UNIX disk if required.

IOCTL
Only the FIODISKFORMAT request is supported; all other ioctl requests
return an error, and set the task's errno to S_ioLib_UNKNOWN_REQUEST.


SEE ALSO:
dosFsDevInit(), dosFsMkfs(), rt11FsDevInit(), rt11FsMkfs(), rawFsDevInit(),
.pG "I/O System, Local File Systems"

LINTLIBRARY
*/

#include "vxWorks.h"
#include "blkIo.h"
#include "ioLib.h"
#include "iosLib.h"
#include "memLib.h"
#include "errno.h"
/* #include "stdio.h" */
#include "dosFsLib.h"
#include "stdlib.h"


/* USED ONLY WITH SIMULATOR */

#if (CPU_FAMILY==SIMSPARCSUNOS || CPU_FAMILY==SIMHPPA || CPU_FAMILY==SIMSPARCSOLARIS)

#include "simLib.h"
#include "u_Lib.h"

extern int printErr ();

int unixRWFlag = O_RDWR;
int unixRWMode = 0;

int unixCRFlag = O_RDWR | O_CREAT; /* 514 */
int unixCRMode = 0644;

#define DEFAULT_SEC_SIZE   512
#define DEFAULT_DISK_SIZE  (1024*1024)


typedef struct		/* UNIXDISK_DEV - UNIX disk device descriptor */
    {
    BLK_DEV	unixDiskBlkDev; /* generic block device structure */
    int		unixFd;	 /* UNIX file descriptor */
    } UNIXDISK_DEV;


/* forward declarations */

LOCAL STATUS unixDiskIoctl ();
LOCAL STATUS unixDiskBlkRd ();
LOCAL STATUS unixDiskBlkWrt ();


/*******************************************************************************
*
* unixDrv - install UNIX disk driver
*
* Used in usrConfig.c to cause the UNIX disk driver to be linked in
* when building VxWorks.  Otherwise, it is not necessary to call this
* routine before using the UNIX disk driver.
*
* This routine is only applicable to VxSim for Solaris and VxSim for HP.
*
* RETURNS: OK (always).
*/

STATUS unixDrv (void)
    {
    return (OK);
    }
/*******************************************************************************
*
* unixDiskDevCreate - create a UNIX disk device
*
* This routine creates a UNIX disk device.
*
* The <unixFile> parameter specifies the name of the UNIX file to use
* for the disk device.
* 
* The <bytesPerBlk> parameter specifies the size of each logical block
* on the disk.  If <bytesPerBlk> is zero, 512 is the default.
* 
* The <blksPerTrack> parameter specifies the number of blocks on each
* logical track of the disk.  If <blksPerTrack> is zero, the count of
* blocks per track is set to <nBlocks> (i.e., the disk is defined 
* as having only one track).
* 
* The <nBlocks> parameter specifies the size of the disk, in blocks.
* If <nBlocks> is zero, a default size is used.  The default is calculated
* as the size of the UNIX disk divided by the number of bytes per block.
* 
* This routine is only applicable to VxSim for Solaris and VxSim for HP.
*
* RETURNS: A pointer to block device (BLK_DEV) structure,
* or NULL, if unable to open the UNIX disk.
*/

BLK_DEV *unixDiskDevCreate
    (
    char	*unixFile,	/* name of the UNIX file */
    int		bytesPerBlk,	/* number of bytes per block */
    int		blksPerTrack,	/* number of blocks per track */
    int		nBlocks		/* number of blocks on this device */
    )

    {
    FAST UNIXDISK_DEV	*pUnixDiskDev;	/* ptr to created UNIXDISK_DEV struct */
    FAST BLK_DEV	*pBlkDev;    /* ptr to BLK_DEV struct in UNIXDISK_DEV */
    int fd;

    /* Set up defaults for any values not specified */

    if (bytesPerBlk == 0)
	bytesPerBlk = DEFAULT_SEC_SIZE;

    if ((fd = u_open (unixFile, ARCHCVTFLAGS(unixRWFlag), unixRWMode)) < 0)
	{
	printErr ("unixDiskDevCreate: Could not open %s read/write\n", unixFile);
	return (NULL);
	}

    if (nBlocks == 0)
	{
	struct unix_stat buf;

	if (u_fstat (fd, (char*)&buf) < 0)
	    {
	    printErr ("unixDiskDevCreate: Could not stat %s\n", unixFile);
	    u_close (fd);
	    return (NULL);
	    }
	nBlocks = (int) buf.st_size / bytesPerBlk;
	}

    if (blksPerTrack == 0)
	blksPerTrack = nBlocks;

    /* Allocate a UNIXDISK_DEV structure for device */

    pUnixDiskDev = (UNIXDISK_DEV *) calloc (1, sizeof (UNIXDISK_DEV));
    if (pUnixDiskDev == NULL)
	{
	u_close (fd);
	return (NULL);				/* no memory */
	}


    /* Initialize BLK_DEV structure (in UNIXDISK_DEV) */

    pBlkDev = &pUnixDiskDev->unixDiskBlkDev;

    pBlkDev->bd_nBlocks      = nBlocks;		/* number of blocks */
    pBlkDev->bd_bytesPerBlk  = bytesPerBlk;	/* bytes per block */
    pBlkDev->bd_blksPerTrack = blksPerTrack;	/* blocks per track */

    pBlkDev->bd_nHeads       = 1;		/* one "head" */
    pBlkDev->bd_removable    = FALSE;		/* not removable */
    pBlkDev->bd_retry	     = 1;		/* retry count */
    pBlkDev->bd_mode	     = UPDATE;		/* initial mode for device */
    pBlkDev->bd_readyChanged = TRUE;		/* new ready status */

    pBlkDev->bd_blkRd	     = unixDiskBlkRd;	/* read block function */
    pBlkDev->bd_blkWrt	     = unixDiskBlkWrt;	/* write block function */
    pBlkDev->bd_ioctl	     = unixDiskIoctl;	/* ioctl function */
    pBlkDev->bd_reset	     = NULL;		/* no reset function */
    pBlkDev->bd_statusChk    = NULL;		/* no check-status function */


    /* Initialize remainder of device struct */
    pUnixDiskDev->unixFd = fd;

    return (&pUnixDiskDev->unixDiskBlkDev);
    }

/*******************************************************************************
*
* unixDiskIoctl - do device specific control function
*
* This routine is called when the file system cannot handle an ioctl
* function.
*
* The FIODISKFORMAT function always returns OK, since a UNIX file does
* not require formatting.  All other requests return ERROR.
*
* RETURNS:  OK or ERROR.
*/

LOCAL STATUS unixDiskIoctl
    (
    UNIXDISK_DEV *pUnixDiskDev,		/* device structure pointer */
    int		function,		/* function code */
    int		arg			/* some argument */
    )

    {
    FAST int	status;			/* returned status value */

    switch (function)
	{
	case FIODISKFORMAT:
	    status = OK;
	    break;

	default:
	    status = ERROR;
	    errno = S_ioLib_UNKNOWN_REQUEST;
	    break;
	}

    return (status);
    }

/*******************************************************************************
*
* unixDiskBlkRd - read one or more blocks from a unix file disk volume
*
* This routine reads one or more blocks from the specified volume,
* starting with the specified block number.  The byte offset is
* calculated and the UNIX disk data is copied to the specified buffer.
*
* RETURNS: OK for success and ERROR for bad block.
*/

LOCAL STATUS unixDiskBlkRd
    (
    UNIXDISK_DEV	*pUnixDiskDev,	/* pointer to device desriptor */
    int			startBlk,	/* starting block number to read */
    int			numBlks,	/* number of blocks to read */
    char		*pBuffer	/* pointer to buffer to receive data */
    )

    {
    long offset;
    int numBytes, ret;
    FAST int		bytesPerBlk;	/* number of bytes per block */

    bytesPerBlk = pUnixDiskDev->unixDiskBlkDev.bd_bytesPerBlk;

    /* Add in the block offset */

    offset = startBlk*bytesPerBlk;

    if (u_lseek (pUnixDiskDev->unixFd, offset, 0) != offset)
	{
	printErr ("unixDiskBlkRd: lseek to offset %d failed\n", offset);
	return (ERROR);
	}

    numBytes = bytesPerBlk * numBlks;

    /* Read the block(s) */

    if ((ret = u_read (pUnixDiskDev->unixFd, pBuffer, numBytes)) != numBytes)
	{
	printErr ("unixDiskBlkRd: failed to read %d bytes\n", numBytes);
	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* unixDiskBlkWrt - write one or more blocks to a unix disk volume
*
* This routine writes one or more blocks to the specified volume,
* starting with the specified block number.  The byte offset is
* calculated and the buffer data is copied to the unix disk.
*
* RETURNS: OK for success and ERROR for bad block.
*/

LOCAL STATUS unixDiskBlkWrt
    (
    UNIXDISK_DEV	*pUnixDiskDev,	/* pointer to device desriptor */
    int			startBlk,	/* starting block number to write */
    int			numBlks,	/* number of blocks to write */
    char		*pBuffer	/* pointer to buffer of data to write */
    )

    {
    long offset;
    int numBytes, ret;
    FAST int bytesPerBlk;	/* number of bytes per block */

    bytesPerBlk = pUnixDiskDev->unixDiskBlkDev.bd_bytesPerBlk;

    /* Add in the block offset */

    offset = startBlk * bytesPerBlk;

    if (u_lseek (pUnixDiskDev->unixFd, offset, 0) != offset)
	{
	printErr ("unixDiskBlkWrt: lseek to offset %d failed\n", offset);
	return (ERROR);
	}

    numBytes = bytesPerBlk * numBlks;

    if ((ret = u_write (pUnixDiskDev->unixFd, pBuffer, numBytes)) != numBytes)
	{
	printErr ("unixDiskBlkWrt: failed to write %d bytes\n", numBytes);
	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* unixDiskInit - initialize a dosFs disk on top of UNIX
*
* This routine provides some convenience for a user wanting to create
* a UNIX disk-based dosFs file system under VxWorks.  The user only
* specifes the UNIX file to use, the dosFs volume name, and the
* size of the volume in bytes, if the UNIX file needs to be created.
*
* This routine is only applicable to VxSim for Solaris and VxSim for HP.
*
* RETURNS: N/A
*/

void unixDiskInit
    (
    char *unixFile,	/* UNIX file name */
    char *volName,	/* dosFs name */
    int diskSize	/* number of bytes */
    )

    {
    int ix;
    int fd;
    int numEightKBlocks;
    int numHalfKBlocks;
    int create = 0;
    int stat;
    struct unix_stat buf;
    char *emptySpace;
    BLK_DEV *pBlkDev;

    if (u_stat (unixFile, (char*)&buf) < 0)
	{
	printErr ("unixDiskInit: %s not found. Creating...", unixFile);

	if ((fd = u_open (unixFile, ARCHCVTFLAGS(unixCRFlag), unixCRMode)) < 0)
	    {
	    printErr ("permission denied\n");
	    return;
	    }

        if (diskSize == 0)
	    diskSize = DEFAULT_DISK_SIZE;

#define	BLOCKSIZE	8192

	emptySpace = calloc (1, BLOCKSIZE);

	numEightKBlocks = diskSize / BLOCKSIZE;
	numHalfKBlocks = (diskSize % BLOCKSIZE) / DEFAULT_SEC_SIZE;

	for (ix = 0; ix < numEightKBlocks; ix++)
	    if (u_write (fd, emptySpace, BLOCKSIZE) < 0)
		{
		printErr ("Disk Write failed (disk full?)\n");
		free (emptySpace);
		return;
		}

	for (ix = 0; ix < numHalfKBlocks; ix++)
	    if (u_write (fd, emptySpace, DEFAULT_SEC_SIZE) < 0)
		{
		printErr ("Disk Write failed (disk full?)\n");
		free (emptySpace);
		return;
		}

	printErr ("done\n");

	u_close (fd);

	free (emptySpace);

	create = 1;
	}

    pBlkDev = unixDiskDevCreate (unixFile, DEFAULT_SEC_SIZE, 0, 0);

    if (pBlkDev != NULL)
	{
	if (create)
	    stat = (dosFsMkfs (volName, pBlkDev) == NULL) ? -1 : 0;
	else
	    stat = (dosFsDevInit (volName, pBlkDev, 0) == NULL) ? -1 : 0;

	if (stat == -1)
	    {
	    printErr ("unixDiskInit: could not init dosFs file system on %s\n",
								unixFile);
	    }
	else
	    {
	    printErr ("unixDiskInit: dosFs file system %s as device %s\n",
				create ? "init'd" : "attached", volName);
	    }
	}
    else
	printErr ("unixDiskInit: failed\n");
    }


#endif /* (CPU_FAMILY==SIMSPARCSUNOS || CPU_FAMILY==SIMHPPA) || CPU_FAMILY==SIMSPARCSOLARIS */

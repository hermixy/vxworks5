/* netDrv.c - network remote file I/O driver */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
05j,10jun02,elr  Correct unresolved reference to netDrvFileDoesNotExist()
05i,06jun02,elr  Documentation corrections (SPRs 78010, 78020)
05h,07may02,kbw  man page edits
05g,31mar02,jkf  SPR#74251, increase path length
05f,14mar02,elr  added more debugging facilities via netDrvDebugLevelSet()
                 split if statements for more precise debugging
                 fixed compilation warnings concerning checking of print 
                  specifiers
                 fixed netFileExists with new configlette for flexibility
                 fixed netOpen so that O_CREAT now works (SPR 8283)
                 Made all error messages optional (SPR 71496)
05e,05nov01,vvv  fixed compilation warnings
05d,15oct01,rae  merge from truestack ver 05k, base 05b (compile warnings, etc.)
05c,21jun00,rsh  upgrade to dosFs 2.0
05c,28seo99,jkf  fixed SPR#28927, netLsByName() causes exception.
05b,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
05a,12mar99,p_m  Fixed SPR 8295 by adding documentation on FIOFSTATGET.
04z,24mar95,jag  netLs ftpXfer call was change to send just "NLST" when the
		 remote directory is "." (SPR #4160)
04x,18jan95,rhp  doc: explain cannot always distinguish directories over netDrv
04w,11aug94,dzb  initialize variables.
04v,22apr93,caf  ansification: added cast to netIoctl().
04u,30sep93,jmm  netLsByName now frees the NET_FD it uses
04t,30aug93,jag  Changed netCreate to delete the end slash from a directory
		 name (SPR #2492)
04s,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
04r,23jul93,jmm  added netLsByName() (spr 2305)
04q,08feb93,jmm  added doc for FIOFSTATGET ioctl call
04p,08feb93,jmm  added support for FIOFSTATGET ioctl call (needed for windX)
                 SPR #1856
04o,01feb93,jdi  documentation cleanup for 5.1; added third param to ioctl()
		 examples.
04n,13nov92,dnw  added include of semLibP.h
04m,21oct92,jdi  removed mangen SECTION designation.
04l,02oct92,srh  added ioctl(FIOGETFL) to return file's open mode
04k,19jul92,smb  added include fcntl.h.
04j,18jul92,smb  Changed errno.h to errnoLib.h.
04i,26may92,rrr  the tree shuffle
04h,08may92,jmm  Fixed ansi prototype warnings
04g,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY and ...
		  -changed VOID to void
		  -changed copyright notice
04f,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
04e,20feb91,jaa	 documentation cleanup.
04d,01oct90,jcf  changed network fd semaphore to not be inversion safe.
04c,18sep90,kdl  removed forward declaration of netWhere().
04b,30jul90,dnw  changed pathLastName() to pathLastNamePtr().
		 added forward declaration of void routines.
04a,26jun90,jcf  changed network fd semaphore to mutex.
		 removed super semaphores.
03z,20jun90,jcf  changed binary semaphore interface.
03y,11may90,yao  added missing modification history (03x) for the last checkin.
03x,09may90,yao  typecasted malloc to (char *).
03w,01may90,llk  changed call to pathCat().  Now, pathCat() checks if a name
		   is too long, and returns ERROR if it is.  03v,17apr90,jcf  changed semaphores to binary for efficiency.  03u,25aug89,dab  fixed coding convention violations.  03t,23aug89,dab  lint.
03s,11jul89,dab  made netOpen() check for O_CREAT flag and create a file that
		 doesn't already exist.  made netLs() check for ftp protocol.
		 added netFileExists().  added netDelete ().  closed all data
		 sockets before calling ftpReplyGet(). QUIT ftp command now
		 sent before closing a ftp control socket. made netOpen()
		 check for O_TRUNC flag.
03r,06jul89,llk  included pathLib.h.
		 changed call to pathLastName().
03q,04apr89,gae  allowed open() to succeed for NFS directories so that ls()
	   +dab  would work correctly.  Added NET_LS_STRING variable.
03p,07jul88,jcf  lint.
03o,22jun88,dnw  name tweaks.
		 coalesced 3 malloc's into 1 in netCreate().
03n,16jun88,llk  added netLs.
03m,04jun88,llk  changed how netCreate retrieves directory and file name.
		 moved pathCat to pathLib.
03l,30may88,dnw  changed to v4 names.
03k,04may88,jcf  changed semaphores to be compatible with new semLib.
03j,21apr88,gae  fixed documentation about fseek().
03i,17mar88,gae  fixed netPut() bug.
		 fixed potential ssemGive() & releaseNetFd() ordering bug.
		 fixed bug in netCreate() - file not created if nothing written.
03h,28jan88,jcf  made kernel independent.
03g,18nov87,ecs  documentation.
03f,06nov87,ecs  documentation.
03e,16oct87,gae  fixed bug in netClose() - return value indeterminate.
		 changed "cd/cat" to use pathCat to construct UNIX filename.
		 Added pathCat().  Semaphored fd usage.
		 Added "super semaphores" to allow successive semTake's on
		 the same semaphore by the same task (needed for recursion).
03d,25mar87,jlf  documentation
	    dnw  added missing parameter on ftpGetReply calls.
		 prepended FTP_ to ftp reply codes.
03c,12feb86,dnw  fixed bug in netPut.  This time for sure.
03b,14jan87,dnw  fixed bug in netPut.
03a,01dec86,dnw  added ftp protocol option.
		 changed to not get include files from default directories.
02h,08nov86,llk  documentation.
02g,07oct86,gae	 made netCreate() use ioGetDefDevTail() instead of global
		 'currentDirectory' to conclude the remote directory name.
		 Use remGetCurId() to get 'currentIdentity'...included remLib.h
02f,05sep86,jlf  made netIoctrl return nbytes for FIONREAD in arg.
		 minor documentation.
02e,29jul86,llk  cleaned up netGet.
02d,27jul86,llk  receives standard error messages from Unix when giving
		 a remote command (remRcmd).  Forwards it on to user.
		 Added getNetStatus.
02c,02jul86,jlf  documentation
02b,21jun86,rdc  fixed bug in netCreate - wasn't allocating sufficient space
		 for strings.
02a,21may86,llk	 got rid of ram disk.  Implemented network file desciptors.
		 Wrote all routines except netDrv and netDevCreate.
01c,03mar86,jlf  changed ioctrl calls to ioctl.
		 changed netIoctrl to netIoctl.
01b,28jan86,jlf  documentation, made netIoctrl be LOCAL.
01a,14sep85,rdc  written.
*/

/*
This driver provides facilities for accessing files transparently over the
network via FTP or RSH.  By creating a network device with netDevCreate(),
files on a remote UNIX machine may be accessed as if they were local.

When a remote file is opened, the entire file is copied over the network
to a local buffer.  When a remote file is created, an empty local buffer
is opened.  Any reads, writes, or ioctl() calls are performed on the local
copy of the file.  If the file was opened with the flags O_WRONLY or O_RDWR
and modified, the local copy is sent back over the network to the UNIX
machine when the file is closed.

Note that this copying of the entire file back and forth can make netDrv
devices awkward to use.  A preferable mechanism is NFS as provided by
nfsDrv.

USER-CALLABLE ROUTINES
Most of the routines in this driver are accessible only through the I/O
system.  However, two routines must be called directly: netDrv() to
initialize the driver and netDevCreate() to create devices.

FILE OPERATIONS
This driver supports the creation, deletion, opening, reading,
writing, and appending of files.  The renaming of files is not
supported.

INITIALIZATION
Before using the driver, it must be initialized by calling the routine
netDrv().  This routine should be called only once, before any reads,
writes, netDevCreate(), or netDevCreate2() calls.  Initialization is
performed automatically when INCLUDE_NET_DRV is defined.

CREATING NETWORK DEVICES
To access files on a remote host, a network device must be created by
calling netDevCreate() or netDevCreate2().  The arguments to netDevCreate()
are the name of the device, the name of the host the device will access,
and the remote file access protocol to be used -- RSH or FTP.
The arguments to netDevCreate2() are ones described above and a size of
buffer used in the network device as a fourth argument.
By convention, a network device name is the remote machine name followed
by a colon ":".  For example, for a UNIX host on the network "wrs", files
can be accessed by creating a device called "wrs:".  For more information,
see the manual entry for netDevCreate() and netDevCreate2().

IOCTL FUNCTIONS
The network driver responds to the following ioctl() functions:
.iP "FIOGETNAME" 18 3
Gets the file name of the file descriptor <fd> and copies it to the buffer
specified by <nameBuf>:
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
The end-of-file pointer changes to the new position, and the new space is
filled with zeroes:
.CS
    status = ioctl (fd, FIOSEEK, newOffset);
.CE
.iP "FIOWHERE"
Returns the current byte position in the file.
This is the byte offset of the next byte to be read or written.
It takes no additional argument:
.CS
    position = ioctl (fd, FIOWHERE, 0);
.CE
.iP "FIOFSTATGET"
Gets file status information.  The argument <statStruct> is a pointer
to a stat structure that is filled with data describing the specified
file.  Normally, the stat() or fstat() routine is used to obtain file
information, rather than using the FIOFSTATGET function directly.
netDrv only fills in three fields of the stat structure: st_dev,
st_mode, and st_size. st_mode is always filled with S_IFREG.
.CS
    struct stat statStruct;
    fd = open ("file", O_RDONLY);
    status = ioctl (fd, FIOFSTATGET, &statStruct);
.CE

LIMITATIONS
The netDrv implementation strategy implies that directories cannot
always be distinguished from plain files.  Thus, opendir() does not
work for directories mounted on netDrv devices, and ll() does not flag
subdirectories with the label "DIR" in listings from netDrv devices.

When the access method is FTP, operations can only be done on files that
the FTP server allows to download. In particular it is not possible to
'stat' a directory, doing so will result in "<dirname>: not a plain file"
error.

INCLUDE FILES: netDrv.h

SEE ALSO: remLib, netLib, sockLib, hostAdd()

*/

#include "vxWorks.h"
#include "sys/socket.h"
#include "iosLib.h"
#include "ioLib.h"
#include "logLib.h"
#include "memLib.h"
#include "memPartLib.h"
#include "semLib.h"
#include "string.h"
#include "remLib.h"
#include "fcntl.h"
#include "ftpLib.h"
#include "lstLib.h"
#include "netDrv.h"
#include "errnoLib.h"
#include "unistd.h"
#include "stdio.h"
#include "pathLib.h"
#include "fioLib.h"
#include "stdlib.h"
#include "private/semLibP.h"
#include "private/iosLibP.h"
#include "private/funcBindP.h"
#include "sys/stat.h"

#define	FD_DIRTY	4	/* mask for the dirty bit */
				/* in the options field of NET_FD, bit [2] */
#define DATASIZE	512	/* bytes per block of data */


/* remote file access protocols */

#define PROTO_RSH	0	/* UNIX RSH protocol */
#define PROTO_FTP	1	/* ARPA FTP protocol */

#define	RSHD	514		/* rshd service */

#define NETDRV_DEBUG(string, param1, param2, param3, param4, param5, param6)\
    {\
    if ((_func_logMsg != NULL) && (netDrvDebugErrors ))\
        (* _func_logMsg) (string, param1, param2, param3, param4, param5, \
        param6);\
    }


STATUS netDrvFileDoesNotExist (char *filename, char *response);

FUNCPTR	_func_fileDoesNotExist	 = netDrvFileDoesNotExist; /* Use default configlette */

typedef struct			/* NET_DEV */
    {
    DEV_HDR	devHdr;				/* device header */
    char	host[MAX_FILENAME_LENGTH];	/* host for this device	*/
    int		protocol;			/* remote file access protocol*/
    UINT	bufSize;			/* size of buffer */
    int		fdMode;				/* FD_MODE of this device */
    } NET_DEV;

/* A network file descriptor consists of a NET_FD header with a linked list
 * of DATABLOCKs maintained by lstLib.  The file pointer (filePtrByte) and
 * end of file pointer (endOfFile) contain absolute byte numbers starting
 * at 0.  For instance, the first byte in the first block is #0.  The first
 * byte in the second block is #512 for 512 byte blocks.  The first byte in
 * the third block is #1024, etc.  Therefore, a byte number modulo 512 gives
 * the index of the byte in databuf.
 */

typedef struct			/* DATA_BLOCK */
    {
    NODE	link;			/* link to adjacent BLOCKs in file */
    int		used;			/* bytes used in databuf (offset) */
    char	databuf[DATASIZE];	/* store data here */
    } DATABLOCK;

/* network file descriptor */

typedef struct			/* NET_FD */
    {
    LIST	 dataLst;		/* list of data blocks */
    NET_DEV *	 pNetDev;		/* file descriptors network device */
    DATABLOCK *	 filePtrBlock;		/* pointer to current datablock */
    int		 filePtrByte;		/* number of current byte */
    int		 endOfFile;		/* byte number after last file byte */
    int		 options;		/* contains mode and dirty bit */
    char *	 remDirName;   		/* directory name on remote machine */
    char *	 remFileName;  		/* file name on remote machine */
    SEM_ID	 netFdSem;		/* mutex semaphore */
    char *	 buf;			/* read buffer */
    int		 bufBytes;		/* available bytes in the buffer */
    int		 bufOffset;		/* current offset in the buffer */
    int		 dataSock;		/* data socket */
    int		 ctrlSock;		/* control socket */
    int		 status [8];		/* status counter */
    } NET_FD;

/* globals */

#define NET_LS_STRING "/bin/ls -a %s"

int mutexOptionsNetDrv = SEM_Q_PRIORITY | SEM_DELETE_SAFE;

/* locals */

LOCAL BOOL netDrvDebugStats = FALSE; 
LOCAL BOOL netDrvDebugErrors = FALSE; 

LOCAL int netDrvNum;		/* this particular driver number */


/* forward static functions */

static int netCreate (NET_DEV *pNetDev, char *name, int mode);
static int netCreate2 (NET_FD *pNetFd, char *name, int mode);
static STATUS netDelete (NET_DEV *pNetDev, char *name);
static int netOpen (NET_DEV *pNetDev, char *name, int mode, int perm);
static STATUS netClose (NET_FD *pNetFd);
static STATUS netLs (NET_FD *pNetFd);
static STATUS netFileExists (NET_DEV *pNetDev, char *name);
static void getNetStatus (char *buf);
static int netRead (NET_FD *pNetFd, char *buf, int maxBytes);
static int netWrite (NET_FD *pNetFd, char *buffer, int nBytes);
static int netIoctl (NET_FD *pNetFd, int function, int arg);
static int netFdCreate (NET_DEV *pNetDev, char *name, int mode);
static void netFdRelease (NET_FD *pNetFd);
static STATUS netGet (NET_FD *pNetFd);
static STATUS netPut (NET_FD *pNetFd);
static int netSeek (NET_FD *pNetFd, int newPos);
static STATUS moveEndOfFile (NET_FD *pNetFd, int newPos);
static STATUS netSockOpen (NET_FD *pNetFd);
static STATUS netSockClose (NET_FD *pNetFd);
extern STATUS netDevCreate2 (char *devName, char *host, int protocol, 
			     UINT bufSize);


/* forward declarations */

IMPORT void lstInternalFree (LIST *pList, VOIDFUNCPTR freeFunc);

/*******************************************************************************
*
* netDrv - install the network remote file driver
*
* This routine initializes and installs the network driver.
* It must be called before other network remote file functions are performed.
* It is called automatically when INCLUDE_NET_DRV is defined.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  This restriction does not apply under non-AE 
* versions of VxWorks.  
*
* RETURNS: OK or ERROR.
*/

STATUS netDrv (void)
    {
    if (netDrvNum > 0)
        return (OK); /* driver already installed */

    netDrvNum = iosDrvInstall (netCreate, netDelete, netOpen, netClose,
                                netRead, netWrite, netIoctl);

    return (netDrvNum == ERROR ? ERROR : OK);
    }
/*******************************************************************************
*
* netDevCreate - create a remote file device
*
* This routine creates a remote file device.  Normally, a network device is
* created for each remote machine whose files are to be accessed.  By
* convention, a network device name is the remote machine name followed by a
* colon ":".  For example, for a UNIX host on the network whose name is
* "wrs", files can be accessed by creating a device called "wrs:".  Files
* can be accessed via RSH as follows:
*
* .CS
*     netDevCreate ("wrs:", "wrs", rsh);
* .CE
*
* The file /usr/dog on the UNIX system "wrs" can now be accessed as
* "wrs:/usr/dog" via RSH.
*
* Before creating a device, the host must have already been created with
* hostAdd().
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: hostAdd()
*/

STATUS netDevCreate
    (
    char *devName,      /* name of device to create */
    char *host,         /* host this device will talk to */
    int protocol        /* remote file access protocol 0 = RSH, 1 = FTP */
    )
    {
    return (netDevCreate2 (devName, host, protocol, 0));
    }

/*******************************************************************************
*
* netDevCreate2 - create a remote file device with fixed buffer size
*
* This routine creates a remote file device, just like netDevCreate(),  but it allows
* very large files to be accessed without loading the entire file to memory.
* The fourth parameter <bufSize> specifies the amount of memory. If the <bufSize> is 
* zero, then the behavior is exactly the same as netDevCreate().  If <bufSize>
* is not zero, the following restrictions apply:
* 
* \ml
* \m - O_RDONLY, O_WRONLY open modes are supported, but not O_RDWR open mode.
* \m - seek is supported in O_RDONLY open mode, but not in O_WRONLY open mode.
* \m - backward seek will be slow if it is beyond the buffer.
* \me
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: netDevCreate()
*/

STATUS netDevCreate2
    (
    char *devName,      /* name of device to create */
    char *host,         /* host this device will talk to */
    int protocol,       /* remote file access protocol 0 = RSH, 1 = FTP */
    UINT bufSize        /* size of buffer in NET_FD */
    )
    {
    FAST NET_DEV *pNetDev;

    if (netDrvNum < 1)
        {
        errno = S_ioLib_NO_DRIVER;
        return (ERROR);
        }

    pNetDev = (NET_DEV *) KHEAP_ALLOC (sizeof (NET_DEV));

    if (pNetDev == NULL)
        return (ERROR);

    (void) strcpy (pNetDev->host, host);
    pNetDev->protocol = protocol;
    pNetDev->bufSize = bufSize;     /* if it is not zero, we use the buf */
    if (bufSize == 0)
        pNetDev->fdMode = 3;        /* O_RDONLY, O_WRONLY or O_RDWR */
    else
        pNetDev->fdMode = 1;        /* O_RDONLY, O_WRONLY */

    if (iosDevAdd ((DEV_HDR *) pNetDev, devName, netDrvNum) != OK)
        {
        KHEAP_FREE ((char *) pNetDev); 
        return ERROR; 
        }
  
    return OK;
    }

/* routines supplied to I/O system */

/*******************************************************************************
*
* netCreate - create a remote file
*
* Returns an open network file descriptor.
*
* Called only by the I/O system.
*
* RETURNS: Pointer to open network file descriptor or ERROR.
*/

LOCAL int netCreate
    (
    NET_DEV *pNetDev,
    char *name,         /* remote file name */
    int mode            /* ignored, always set to O_WRONLY */
    )
    {
    FAST NET_FD *pNetFd;

    pNetFd = (NET_FD *) netFdCreate (pNetDev, name, mode);
    if (pNetFd == (NET_FD *) ERROR)
        {
        return (ERROR);
        }

    if (netCreate2 (pNetFd, name, mode) == ERROR)
        {
        NETDRV_DEBUG ("netCreate:  error calling netCreate2()  errno:0x%08x\n", errno,1,2,3,4,5);
        return ERROR;
        }

    return ((int) pNetFd);
    }

/*******************************************************************************
*
* netCreate2 - create a remote file with an existing net file descriptor
*
* Returns an open network file descriptor.
*
* Called only by netCreate and netOpen
*
* RETURNS: Pointer to open network file descriptor or ERROR.
*/

LOCAL int netCreate2
    (
    NET_FD *pNetFd,
    char *name,         /* remote file name */
    int mode            /* ignored, always set to O_WRONLY */
    )
    {

    if (pNetFd == (NET_FD *) ERROR)
        {
        NETDRV_DEBUG ("netCreate2:  pNetFd == NULL errno:0x%08x\n", errno,1,2,3,4,5);
        return (ERROR);
        }

    if (pNetFd->pNetDev->bufSize > 0)
        {
        char command[MAX_FILENAME_LENGTH];
        char buffer [MAX_FILENAME_LENGTH];
        char usr [MAX_IDENTITY_LEN];
        char passwd [MAX_IDENTITY_LEN];

        /* open the remote file */

        remCurIdGet (usr, passwd);

        if (pNetFd->pNetDev->protocol == PROTO_FTP)
            {
            if (ftpXfer (pNetFd->pNetDev->host, usr, passwd, "",
                "STOR %s", pNetFd->remDirName, pNetFd->remFileName,
                &pNetFd->ctrlSock, &pNetFd->dataSock) == ERROR)
                {
                NETDRV_DEBUG ("netCreate2:  error calling ftpXfer()  errno:0x%08x\n", \
                              errno,1,2,3,4,5);
                return (ERROR);
                }
            }
        else
            {
            if (pathCat (pNetFd->remDirName, pNetFd->remFileName, buffer) ==
                    ERROR)
                {
                NETDRV_DEBUG ("netCreate2:  error calling pathCat()  errno:0x%08x\n", \
                              errno,1,2,3,4,5);
                return (ERROR);
                }

            sprintf (command, "/bin/cat > %s", buffer);

            pNetFd->dataSock = rcmd (pNetFd->pNetDev->host, RSHD, usr,
                                     usr, command, &pNetFd->ctrlSock);
    
            if (pNetFd->dataSock == ERROR)
                {
                NETDRV_DEBUG ("netCreate2:  error calling rcmd()  errno:0x%08x\n", \
                              errno,1,2,3,4,5);
                return (ERROR);
                }
            }
        }

    return ((int) pNetFd);
    }

/*******************************************************************************
*
* netDelete - delete a file from a remote machine via the network.
*
* The remote shell daemon on the machine 'host' is used to delete
* the given file.  The remote userId should have been set previously
* by a call to iam().
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS netDelete
    (
    NET_DEV *pNetDev,
    char *name          /* remote file name */
    )
    {
    int dataSock;
    int ctrlSock;
    char buf [DATASIZE];    /* make buf the same size as NET_FD's databuf */
    FAST int nBytes;
    char command[MAX_FILENAME_LENGTH];
    STATUS status = OK;
    char usr [MAX_IDENTITY_LEN];
    char passwd [MAX_IDENTITY_LEN];
    char *pDirName;
    char *pFileName;

    pDirName  = (char *) KHEAP_ALLOC ((unsigned) (strlen (name) + 1));
    pFileName = (char *) KHEAP_ALLOC ((unsigned) (strlen (name) + 1));

    remCurIdGet (usr, passwd);


    /* get directory name and file name */

    pathSplit (name, pDirName, pFileName);

    if (pNetDev->protocol == PROTO_FTP)
        {
    
        if (ftpXfer (pNetDev->host, usr, passwd, "",
            "DELE %s", pDirName, pFileName,
            &ctrlSock, (int *) NULL) == ERROR)
            {
            NETDRV_DEBUG ("netDelete:  error calling ftpXfer()  errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            KHEAP_FREE (pDirName);
            KHEAP_FREE (pFileName);
            return (ERROR);
            }
        }
    else
        {
        sprintf (command, "/bin/rm %s", name);

        dataSock = rcmd (pNetDev->host, RSHD, usr, usr, command, &ctrlSock);

        if (dataSock == ERROR)
            {
            NETDRV_DEBUG ("netDelete:  error calling rcmd()  errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            KHEAP_FREE (pDirName);
            KHEAP_FREE (pFileName);
            return (ERROR);
            }

        close (dataSock);

        /* check control socket for error */

        if ((nBytes = fioRead (ctrlSock, buf, sizeof (buf) - 1)) > 0)
            {
            NETDRV_DEBUG ("netDelete:  error fioread()  errno:0x%08x\n", \
                          errno,1,2,3,4,5);

            /* print error message on standard error fd */

            buf [nBytes] = EOS; /* insure string termination */

            NETDRV_DEBUG ("%s:%s errno:0x%08x", pNetDev->host, buf, errno,3,4,5);

            /* set the task's status according to the UNIX error */
            getNetStatus (buf);

            status = ERROR;
            }
        }

    if (pNetDev->protocol == PROTO_FTP &&
        ftpCommand (ctrlSock, "QUIT",0,0,0,0,0,0) != FTP_COMPLETE)
        {
        NETDRV_DEBUG ( \
            "netDelete:  error calling ftpCommand(\"QUIT\")  errno:0x%08x\n",  \
            errno,1,2,3,4,5);
        status = ERROR;
        }

    close (ctrlSock);

    KHEAP_FREE (pDirName);
    KHEAP_FREE (pFileName);

    return (status);
    }

/*******************************************************************************
*
* netOpen - open a remote file
*
* netOpen loads the remote file from the host into a network file descriptor.
* If the file does not exist and the O_CREAT flag is not specifed, or the file
* is not readable on the UNIX machine, then the UNIX error message is printed
* on the standard error file descriptor and ERROR is returned.
*
* Called only by the I/O system.
*
* RETURNS: Pointer to open network file descriptor, or ERROR.
*/

LOCAL int netOpen
    (
    FAST NET_DEV *pNetDev,    /* pointer to network device */
    char         *name,       /* remote file name to open */
    int           mode,       /* O_RDONLY, O_WRONLY or O_RDWR, and O_CREAT */
    int           perm        /* UNIX permissions, includes directory flag */
    )
    {
    FAST NET_FD *pNetFd;
    STATUS status;

    NETDRV_DEBUG ("**** netOpen: name:%s mode:0x%x perm:0x%x Entered ***\n", \
                  1,2,3,4,5,6);

    pNetFd = (NET_FD *) netFdCreate (pNetDev, name, mode);

    if (pNetFd == (NET_FD *) ERROR)
        {
        NETDRV_DEBUG ("netOpen:  error calling netFdCreate()  errno:0x%08x\n",  \
                      errno,1,2,3,4,5);
        return (ERROR);
        }

    /* Leave file at 0 length. */
    if ((mode & O_TRUNC))
        {
        return ((int) pNetFd); 
        }

    if ((mode & O_CREAT))
        {
        NETDRV_DEBUG ("**** netOpen: calling netFileExists ***\n", 1,2,3,4,5,6);
        }
    else
        {
        NETDRV_DEBUG ("**** O_CREATE NOT USED!!! ***\n", 1,2,3,4,5,6);
        }

    if (mode & O_CREAT) 
        {
        if (netFileExists (pNetDev, name) == ERROR)
            {
            NETDRV_DEBUG ("netOpen: calling netCreate2 \n", 1,2,3,4,5,6);
            errno = 0; /* reset errno after netFileNetExists() */
            if (netCreate2 (pNetFd, name, mode) == ERROR)
                {
                NETDRV_DEBUG ("**** netOpen: netCreate2 return error ***\n", 1,2,3,4,5,6);
                return (ERROR);
                }

            NETDRV_DEBUG ("netOpen: returning file descriptor pointer 0x%08x after creating file\n", pNetFd,1,2,3,4,5);
            return ((int) pNetFd); 
            }
        else
            {
            NETDRV_DEBUG ("netOpen: netFileExists - returned OK - file exists\n", \
                          1,2,3,4,5,6);
            }

        NETDRV_DEBUG ("netOpen: CREAT called, but file already exists\n", \
                      pNetFd,1,2,3,4,5);
        }

    semTake (pNetFd->netFdSem, WAIT_FOREVER);

    /* netFdCreate sets a O_RDONLY mode to O_RDWR and sets dirty bit,
     * set it back to original mode. */

    pNetFd->options = mode & pNetFd->pNetDev->fdMode;

    if (pNetFd->pNetDev->bufSize == 0)
        {
        NETDRV_DEBUG ("netOpen:  calling netGet() \n", 0,1,2,3,4,5);
        if (netGet (pNetFd) != OK)
            {
            /* not able to get file from host */

            semGive (pNetFd->netFdSem);
            netFdRelease (pNetFd);
            NETDRV_DEBUG ("netOpen:  netGet() return !OK - errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            return (ERROR);
            }

        /* netGet might have moved the file pointer, so reset to beginning */

        status = netSeek (pNetFd, 0);
        if (status == ERROR)
            {
            NETDRV_DEBUG ("netOpen:  error calling netSeek()  errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            semGive (pNetFd->netFdSem);
            netFdRelease (pNetFd);
            return (ERROR);
            }
        }
    else
        {
        if ((status = netSockOpen (pNetFd)) != OK) /* open the connection */
            {
            NETDRV_DEBUG ("netOpen:  error calling netSockOpen()  errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            /* not able to open file from host */

            semGive (pNetFd->netFdSem);
            netFdRelease (pNetFd);

            return (ERROR);
            }

        pNetFd->filePtrByte = 0;
        pNetFd->bufBytes    = 0;
        pNetFd->bufOffset   = 0;
        }

    semGive (pNetFd->netFdSem);

    return ((int) pNetFd);
    }

/*******************************************************************************
*
* netClose - close a remote file.  Copy it to the remote machine.
*
* If the file is O_WRONLY or O_RDWR mode and it has been written to (it's
* dirty bit is set), then the file is copied over to UNIX.
*
* Called only by the I/O system.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS netClose
    (
    FAST NET_FD *pNetFd
    )
    {
    STATUS status = OK;

   NETDRV_DEBUG ("netClose: entered\n",0,0,0,0,0,0);

    semTake (pNetFd->netFdSem, WAIT_FOREVER);

    if (pNetFd->pNetDev->bufSize == 0)
        {
        /* if file has dirty bit set, it's been changed. Copy it to the host */

        if (pNetFd->options & FD_DIRTY)
            status = netPut (pNetFd);
        if (status == ERROR)
            {
            NETDRV_DEBUG ("netClose: error calling netPut to flush file errno=0x%08x\n",\
                          errno,2,3,4,5,6);
            }
        }
    else
        {
        status = netSockClose (pNetFd); /* close the connection */
        if (status != OK)
            {
            NETDRV_DEBUG ("netClose: error calling netSockClose errno=0x%08x\n",  \
                          errno,2,3,4,5,6);
            }
        }

    semGive (pNetFd->netFdSem);

    /* print out the statistic if the debug flag is on */

    if ((netDrvDebugStats) && (pNetFd->pNetDev->bufSize > 0))
        {
        NETDRV_DEBUG ("read[hit=%d mis=%d] fseek[hit=%d mis=%d] bseek[hit=%d mis=%d]\n",\
                      pNetFd->status[0], pNetFd->status[1], pNetFd->status[2],\
                      pNetFd->status[3], pNetFd->status[4], pNetFd->status[5]);
        }

    netFdRelease (pNetFd);

    return (status);
    }

/*******************************************************************************
*
* netLs - list a remote directory.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS netLs
    (
    FAST NET_FD *pNetFd
    )
    {
    int dataSock;
    int ctrlSock;
    char buf [DATASIZE]; /* make buf the same size as NET_FD's databuf */
    FAST int nBytes = -1;
    char command[MAX_FILENAME_LENGTH];
    char buffer [MAX_FILENAME_LENGTH];
    STATUS status = OK;
    char usr [MAX_IDENTITY_LEN];
    char passwd [MAX_IDENTITY_LEN];

    remCurIdGet (usr, passwd);

    if (pNetFd->pNetDev->protocol == PROTO_FTP)
        {
        if (pNetFd->remFileName[0] == '.')
            {
            if (ftpXfer (pNetFd->pNetDev->host, usr, passwd, "", "NLST",
                         pNetFd->remDirName, pNetFd->remFileName,
                         &ctrlSock, &dataSock) == ERROR)
                {
                    NETDRV_DEBUG ("netLs:  error calling ftpXfer(\"NLST\")  errno:0x%08x\n", errno,1,2,3,4,5);
                return (ERROR);
                }
            }
        else
           {
           if (ftpXfer (pNetFd->pNetDev->host, usr, passwd, "", "NLST %s",
                        pNetFd->remDirName, pNetFd->remFileName,
                        &ctrlSock, &dataSock) == ERROR)
               {
               NETDRV_DEBUG ("netLs:  error calling ftpXfer(\"NLST\")  errno:0x%08x\n", errno,1,2,3,4,5);
               return (ERROR);
               }
           }
        }
    else
        {
        if (pathCat (pNetFd->remDirName, pNetFd->remFileName, buffer) == ERROR)
            {
            NETDRV_DEBUG ("netLs:  error calling pathCat\(\"%s\", \"%s\")  errno:0x%08x\n", 
            pNetFd->remDirName, pNetFd->remFileName,errno,1,2,3);
            return (ERROR);
            }

        sprintf (command, NET_LS_STRING, buffer);

        dataSock = rcmd (pNetFd->pNetDev->host, RSHD, usr,
                          usr, command, &ctrlSock);

        if (dataSock == ERROR)
            {
            NETDRV_DEBUG ("netLs:  error calling rcmd() errno:0x%08x\n", errno,1,2,3,4,5);
            return (ERROR);
            }
        }

    /* read bytes from socket and write them
     * to standard out one block at a time
     */
    while ((status == OK) &&
           ((nBytes = read (dataSock, buf, sizeof (buf))) > 0))
        {
        if (write (STD_OUT, buf, nBytes) != nBytes)
            {
                NETDRV_DEBUG ("netLs:  error calling write()  errno:0x%08x\n", \
                              errno,1,2,3,4,5);
                status = ERROR;
             }
        }

    if (nBytes < 0) /* recv error */
        {
        NETDRV_DEBUG ("netLs:  error calling read()  errno:0x%08x\n", errno,1,2,3,4,5);
        status = ERROR;
        }

    close (dataSock);

    if (pNetFd->pNetDev->protocol == PROTO_FTP)
       {
       if (ftpReplyGet (ctrlSock, TRUE) != FTP_COMPLETE)
           {
           NETDRV_DEBUG ("netLs:  error calling ftpReplyGet()  errno:0x%08x\n", \
                         errno,1,2,3,4,5);
           status = ERROR;
           }
       }
    else
       {
       /* check control socket for error */

       if ((nBytes = fioRead (ctrlSock, buf, sizeof (buf) - 1)) > 0)
           {
           /* print error message */

           buf [nBytes] = EOS; /* insure string termination */

           NETDRV_DEBUG ("netLs: %s:%s errno:0x%08x error calling fioread() \n",  
                         pNetFd->pNetDev->host, buf, errno,1,2,3);

           /* set the task's status according to the UNIX error */
           getNetStatus (buf);

           status = ERROR;
           }
       }

    if (pNetFd->pNetDev->protocol == PROTO_FTP &&
       ftpCommand (ctrlSock, "QUIT",0,0,0,0,0,0) != FTP_COMPLETE)
       {
           NETDRV_DEBUG ("netLs:  error calling ftpCommand()  errno:0x%08x\n", \
                         errno,1,2,3,4,5);
           status = ERROR;
       }

    close (ctrlSock);

    return (status);
    }

/******************************************************************************
*
* netLsByName - list a remote directory by name
*
* RETURNS: OK if successful, otherwise ERROR.  If the device is not a netDrv
* device, errno is set to S_netDrv_UNKNOWN_REQUEST.
*
* ERRNO
*   S_netDrv_UNKNOWN_REQUEST
*
* NOMANUAL
*/

STATUS netLsByName
    (
    char * dirName		/* name of the directory */
    )
    {
    DEV_HDR * pDevHdr;		/* device header */
    char      fullFileName [MAX_FILENAME_LENGTH];
    NET_FD *  pNetFd;
    STATUS    netLsReturn;

    if (ioFullFileNameGet (dirName, &pDevHdr, fullFileName) == ERROR)
        {
        NETDRV_DEBUG ("netLsByName:  error calling ioFullFileNameGet()  errno:0x%08x\n",\
                      errno,1,2,3,4,5);
        return (ERROR);
        }

    /* make sure the device is a netDrv device */

    if (drvTable [pDevHdr->drvNum].de_open != netOpen)
        {
        NETDRV_DEBUG ("netLsByName:  error UNKNOWN_REQUEST errno:0x%08x\n", \
                      errno,1,2,3,4,5);
        errno = S_netDrv_UNKNOWN_REQUEST;
        return (ERROR);
        }

    /* create a NET_FD to pass to netLs */

    pNetFd = (NET_FD *) netFdCreate ((NET_DEV *) pDevHdr, fullFileName, 0);

    if (pNetFd == (NET_FD *)ERROR)
        {
        NETDRV_DEBUG ("netFdCreate:  error callinf netFdCreate errno:0x%08x\n", \
                      errno,1,2,3,4,5);
        return (ERROR);             /* SPR #28927 */
        }

    netLsReturn = netLs (pNetFd);

    netFdRelease (pNetFd);

    return (netLsReturn);
    }

/*******************************************************************************
*
* netFileExists - check for the existence of a file.
*
* RETURNS: OK if the file exists, or ERROR.
*/

LOCAL STATUS netFileExists
    (
    NET_DEV *pNetDev,
    char *name          /* remote file name */
    )
    {
    int dataSock;
    int ctrlSock;
    char buf [DATASIZE];        /* make buf the same size as NET_FD's databuf */
    char command[MAX_FILENAME_LENGTH];
    STATUS status = OK;
    char usr [MAX_IDENTITY_LEN];
    char passwd [MAX_IDENTITY_LEN];
    char *pDirName  = (char *) KHEAP_ALLOC ((unsigned) (strlen (name) + 1));
    char *pFileName = (char *) KHEAP_ALLOC ((unsigned) (strlen (name) + 1));

    /* don't allow null filenames */

    if (name[0] == EOS)
        {
        errno = S_ioLib_NO_FILENAME;
        NETDRV_DEBUG ("netFileExists:  error S_ioLib_NO_FILENAME errno:0x%08x\n", \
                      errno,1,2,3,4,5);
        return (ERROR);
        }

    /* get directory and filename */

    pathSplit (name, pDirName, pFileName);

    remCurIdGet (usr, passwd);

    if (pNetDev->protocol == PROTO_FTP)
        {
        if (ftpXfer (pNetDev->host, usr, passwd, "",
                     "NLST %s", pDirName, pFileName,
                     &ctrlSock, &dataSock) == ERROR)
            {
            NETDRV_DEBUG ("netFileExists:  error calling ftpXfer(). errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            KHEAP_FREE (pDirName);
            KHEAP_FREE (pFileName);
            return (ERROR);
            }
        }
    else
        {
        sprintf (command, NET_LS_STRING, name);

        dataSock = rcmd (pNetDev->host, RSHD, usr, usr, command, &ctrlSock);

        if (dataSock == ERROR)
            {
            NETDRV_DEBUG ("netFileExists:  error calling rcmd(). errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            KHEAP_FREE (pDirName);
            KHEAP_FREE (pFileName);
            return (ERROR);
            }
        }

    if (pNetDev->protocol == PROTO_FTP)
        {
        /* check dataSock for ls error message */

        if (read (dataSock, buf, sizeof (buf)) <= 0)
            {
            NETDRV_DEBUG ("netFileExists:  error calling read() on data socket for ls. errno:0x%08x\n", errno,1,2,3,4,5);
            status = ERROR;
            }

        NETDRV_DEBUG ("netFileExists:  full message:%s\n", buf, 1,2,3,4,5);

        NETDRV_DEBUG ("netFileExists:  message:%s\n", buf+ strlen (pFileName) + 1,
                       1,2,3,4,5);

        if ((_func_fileDoesNotExist != NULL))
            {
            if ((* _func_fileDoesNotExist) (pFileName, buf) == OK)
                {
                NETDRV_DEBUG ("netFileExists:  match error on ls \n", \
                              errno,1,2,3,4,5);
                status = ERROR;
                }
            else
                {
                NETDRV_DEBUG ("netFileExists:  file does not exist\n", errno,1,2,3,4,5);
                }
            }

        if (close (dataSock) == ERROR)
            {
            status = ERROR;
            NETDRV_DEBUG ("netFileExists:  error calling close() on dataSock. errno:0x%08x\n", errno,1,2,3,4,5);
            }

        if (ftpReplyGet (ctrlSock, FALSE) != FTP_COMPLETE)
            {
            NETDRV_DEBUG ("netFileExists: error calling ftpReplyGet(). errno:0x%08x\n",\
                          errno,1,2,3,4,5);
            status = ERROR;
            }
        }
    else
        {
        /* check control socket for error */

        if (read (ctrlSock, buf, sizeof (buf)) > 0)
            {
            /* set the task's status according to the UNIX error */
   
            getNetStatus (buf);

            NETDRV_DEBUG ("netFileExists:  error calling read() on control socket. errno:0x%08x\n", errno,1,2,3,4,5);
            status = ERROR;
            }
        close (dataSock);
        }

    if (pNetDev->protocol == PROTO_FTP &&
        ftpCommand (ctrlSock, "QUIT",0,0,0,0,0,0) != FTP_COMPLETE)
        {
        NETDRV_DEBUG ("netFileExists:  error calling ftpCommand(\"QUIT\"). errno:0x%08x\n", errno,1,2,3,4,5);
        status = ERROR;
        }

    close (ctrlSock);

    KHEAP_FREE (pDirName);
    KHEAP_FREE (pFileName);

    NETDRV_DEBUG ("netFileExists:  returning (%d)\n", status,1,2,3,4,5);
    return (status);
    }

/*******************************************************************************
*
* netGet - downLoad a file from a remote machine via the network.
*
* The remote shell daemon on the machine 'host' is used to download
* the given file to the specified previously opened network file descriptor.
* The remote userId should have been set previously by a call to iam().
* If the file does not exist, the error message from the UNIX 'host'
* is printed to the VxWorks standard error file descriptor and ERROR
* is returned.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS netGet
    (
    FAST NET_FD *pNetFd
    )
    {
    int dataSock;
    int ctrlSock;
    char buf [DATASIZE];	/* make buf the same size as NET_FD's databuf */
    FAST int nBytes = -1;
    char command[MAX_FILENAME_LENGTH];
    char buffer [MAX_FILENAME_LENGTH];
    int saveOptions;
    STATUS status = OK;
    char usr [MAX_IDENTITY_LEN];
    char passwd [MAX_IDENTITY_LEN];
    char *errMsg = "cat: read error: Is a directory";
    int errMsgLen = strlen (errMsg);

    remCurIdGet (usr, passwd);

    if (pNetFd->pNetDev->protocol == PROTO_FTP)
        {
        if (ftpXfer (pNetFd->pNetDev->host, usr, passwd, "",
                     "RETR %s", pNetFd->remDirName, pNetFd->remFileName,
                     &ctrlSock, &dataSock) == ERROR)
            {
            NETDRV_DEBUG ("netGet:  error calling ftpXfer(). errno:0x%08x\n", errno,1,2,3,4,5);
            return (ERROR);
            }
        }
    else
        {
        if (pathCat (pNetFd->remDirName, pNetFd->remFileName, buffer) == ERROR)
           {
           NETDRV_DEBUG ("netGet:  error calling pathCat(). errno:0x%08x\n", errno,1,2,3,4,5);
           return (ERROR);
           } 

        sprintf (command, "/bin/cat < %s", buffer);

        dataSock = rcmd (pNetFd->pNetDev->host, RSHD, usr,
                         usr, command, &ctrlSock);

        if (dataSock == ERROR)
            {
            NETDRV_DEBUG ("netGet:  error calling rcmd(). errno:0x%08x\n", errno,1,2,3,4,5);
            return (ERROR);
            }
        }

    /* Set file pointer to beginning of file */

    if (netSeek (pNetFd, 0) == ERROR)
        {
        if (pNetFd->pNetDev->protocol == PROTO_FTP &&
            ftpCommand (ctrlSock, "QUIT",0,0,0,0,0,0) != FTP_COMPLETE)
            {
            NETDRV_DEBUG ("netGet:  error calling ftpCommand(\"QUIT\"). errno:0x%08x\n",\
                          errno,1,2,3,4,5);
            status = ERROR;
            }

        close (dataSock);
        close (ctrlSock);
        return (ERROR);
        }

    /* set mode to write so that file can be written to,
    *  save original options so they can be restored later
    */

    saveOptions = pNetFd->options;
    pNetFd->options = O_WRONLY & pNetFd->pNetDev->fdMode;

    /* read bytes from socket and write them
     * out to file descriptor one block at a time
     */

    while ((status == OK) &&
           ((nBytes = read (dataSock, buf, sizeof (buf))) > 0))
        {
        if (netWrite (pNetFd, buf, nBytes) != nBytes)
            {
            NETDRV_DEBUG ("netGet:  error calling netWrite(). errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            status = ERROR;
            }
        }

    if (nBytes < 0)   /* recv error */
            status = ERROR;

    close (dataSock);

    if (pNetFd->pNetDev->protocol == PROTO_FTP)
        {
        if (ftpReplyGet (ctrlSock, FALSE) != FTP_COMPLETE)
            {
            NETDRV_DEBUG ("netGet:  error calling ftpReplyGet(). errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            status = ERROR;
            }
        }
    else
        {
        /* check control socket for error */

        if ((nBytes = fioRead (ctrlSock, buf, sizeof (buf) - 1)) > 0)
            {

            /* print error message on standard error fd */

            buf [nBytes] = EOS;	/* insure string termination */

            /* check error message indicating cat of NFS mounted directory */

            if (strncmp (buf, errMsg, errMsgLen) != 0)
                {
                NETDRV_DEBUG ("netGet: %s:%s  . errno:0x%08x\n",  
                                 pNetFd->pNetDev->host, buf, errno,1,2,3);
                /* set the task's status according to the UNIX error */
                getNetStatus (buf);

                status = ERROR;
                }
            }
    }

    if (pNetFd->pNetDev->protocol == PROTO_FTP &&
        ftpCommand (ctrlSock, "QUIT",0,0,0,0,0,0) != FTP_COMPLETE)
        {
        NETDRV_DEBUG ("netGet:  error calling ftpCommand(\"QUIT\"). errno:0x%08x\n", \
                      errno,1,2,3,4,5);
        status = ERROR;
        }

    close (ctrlSock);

    pNetFd->options = saveOptions;	/* restore original options */

    return (status);
    }

/*******************************************************************************
*
* netPut - upload a file to a remote machine via the network.
*
* The remote shell daemon on the machine 'host' is used to upload
* from the open network file descriptor to a remote file.
* The remote userId should have been set previously be a call
* to iam().  If an error occurs, the UNIX error is output to the
* VxWorks standard error fd.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS netPut
    (
    FAST NET_FD *pNetFd
    )
    {
    int dataSock;
    int ctrlSock;
    char buf [DATASIZE];	/* make buf the same size as NET_FD's databuf */
    FAST int nBytes = -1;
    char command[MAX_FILENAME_LENGTH];
    char buffer[MAX_FILENAME_LENGTH];
    int saveOptions;
    STATUS status = OK;
    char usr [MAX_IDENTITY_LEN];
    char passwd [MAX_IDENTITY_LEN];

    remCurIdGet (usr, passwd);

    if (pNetFd->pNetDev->protocol == PROTO_FTP)
        {
        if (ftpXfer (pNetFd->pNetDev->host, usr, passwd, "",
                     "STOR %s", pNetFd->remDirName, pNetFd->remFileName,
                      &ctrlSock, &dataSock) == ERROR)
            {
            NETDRV_DEBUG ("netPut:  error calling ftpXfer()  errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            return (ERROR);
            }
        }
    else
        {
        if (pathCat (pNetFd->remDirName, pNetFd->remFileName, buffer) == ERROR)
            {
            NETDRV_DEBUG ("netPut:  error calling pathCat(). errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            return (ERROR);
            }

        sprintf (command, "/bin/cat > %s", buffer);

        dataSock = rcmd (pNetFd->pNetDev->host, RSHD, usr,
                         usr, command, &ctrlSock);

        if (dataSock == ERROR)
            {
            NETDRV_DEBUG ("netPut:  error calling rcmd(). errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            return (ERROR);
            }
        }

    /* Set file pointer to beginning of file */

    if (netSeek (pNetFd, 0) == ERROR)
        {
        if (pNetFd->pNetDev->protocol == PROTO_FTP &&
            ftpCommand (ctrlSock, "QUIT",0,0,0,0,0,0) != FTP_COMPLETE)
            {
            NETDRV_DEBUG ("netPut: error calling ftpCommand(\"QUIT\").  errno:0x%08x\n",\
                          errno,1,2,3,4,5);
            status = ERROR;
            }

        close (dataSock);
        close (ctrlSock);
        return (ERROR);
        }

    /* set mode to write so that file can be written to,
     * save original options so they can be restored later
     */

        saveOptions = pNetFd->options;
        pNetFd->options = O_RDONLY & pNetFd->pNetDev->fdMode;

    /* Read the data from one DATABLOCK into buffer.
     *  Continue until file pointer reaches the end of file.
     */

    while (status == OK && (nBytes = netRead (pNetFd, buf, sizeof (buf))) > 0)
        {
        if (write (dataSock, buf, nBytes) != nBytes)
            {
            NETDRV_DEBUG ("netPut:  error calling write(). errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            status = ERROR;
            }
        }

    if (nBytes < 0)		/* netRead error */
        {
        NETDRV_DEBUG ("netPut:  error calling netRead(). errno:0x%08x\n", 
                      errno,1,2,3,4,5);
        status = ERROR;
        }

    if (close (dataSock) == ERROR)
        {
        NETDRV_DEBUG ("netPut:  error calling close(). errno:0x%08x\n", errno,1,2,3,4,5);
        status = ERROR;
        }

    if (pNetFd->pNetDev->protocol == PROTO_FTP)
        {
        if (ftpReplyGet (ctrlSock, FALSE) != FTP_COMPLETE)
            {
            NETDRV_DEBUG ("netPut:  error calling ftpReplyGet(). errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            status = ERROR;
            }
        }
    else
        {
        /* check control socket for error */

        if ((nBytes = fioRead (ctrlSock, buf, sizeof (buf) - 1)) > 0)
            {
            /* print error message */

            buf [nBytes] = EOS;	/* insure string termination */
            NETDRV_DEBUG ("netPut: %s:%s  . errno:0x%08x\n",  
                          pNetFd->pNetDev->host, buf, errno,1,2,3);
            /* set the task's status according to the UNIX error */
            getNetStatus (buf);

            NETDRV_DEBUG ("netPut:  error calling fioRead(). errno:0x%08x\n", \
                          errno,1,2,3,4,5);
            status = ERROR;
            }
        }

    if (pNetFd->pNetDev->protocol == PROTO_FTP &&
        ftpCommand (ctrlSock, "QUIT",0,0,0,0,0,0) != FTP_COMPLETE)
        {
        NETDRV_DEBUG ("netPut:  error calling ftpCommand(). errno:0x%08x\n", \
                      errno,1,2,3,4,5);
        status = ERROR;
        }

    close (ctrlSock);

    pNetFd->options = saveOptions;	/* restore original options */

    return (status);
    }

/****************************************************************************
*
* getNetStatus - set task status according to network error
*
* Compares string in buf with some known UNIX errors that can occur when
* copying files over the network.  Sets task status accordingly.
*/

LOCAL void getNetStatus
    (
    char *buf           /* buffer containing string with UNIX error */
    )
    {
    FAST char *pErr;

    pErr = (char *) index (buf, ':');

    if (pErr == NULL)
        errno = S_netDrv_UNIX_FILE_ERROR;

    else if (strcmp (pErr, ": Permission denied\n") == 0)
        errno = S_netDrv_PERMISSION_DENIED;

    else if (strcmp (pErr, ": No such file or directory\n") == 0)
        errno = S_netDrv_NO_SUCH_FILE_OR_DIR;

    else if (strcmp (pErr, ": Is a directory\n") == 0)
        errno = S_netDrv_IS_A_DIRECTORY;

    else
        errno = S_netDrv_UNIX_FILE_ERROR;
    }
/*******************************************************************************
*
* netRead - read bytes from remote file
*
* netRead reads up to the specified number of bytes from the open network
* file descriptor and puts them into a buffer.  Bytes are read starting
* from the point marked by the file pointer.  The file pointer is then
* updated to point immediately after the last character that was read.
*
* Called only by the I/O system.
*
* SIDE EFFECTS: moves file pointer
*
* RETURNS: Number of bytes read, or ERROR.
*/

LOCAL int netRead
    (
    FAST NET_FD *pNetFd,        /* pointer to open network file descriptor */
    char *buf,                  /* pointer to buffer to receive bytes   */
    FAST int maxBytes           /* max number of bytes to read into buffer */
    )
    {
    STATUS status = OK;
    FAST int byteCount = 0;	/* number of bytes read so far */

    /* check for valid maxbytes */

    if (maxBytes <= 0)
        {
        errno = S_netDrv_INVALID_NUMBER_OF_BYTES;
        return (ERROR);
        }

    /* if file opened for O_WRONLY, don't read */
    if ((pNetFd->options & pNetFd->pNetDev->fdMode) == O_WRONLY)
        {
        errno = S_netDrv_WRITE_ONLY_CANNOT_READ;
        return (ERROR);
        }

    semTake (pNetFd->netFdSem, WAIT_FOREVER);

    if (pNetFd->pNetDev->bufSize == 0)
        {
        FAST DATABLOCK *dptr;	/* points to current datablock */
        FAST int cindex;	/* character index datablock's databuf */
        FAST char *cptr;	/* points to character being read in the file */
        FAST char *bptr;	/* points to current position in buffer */

        /* if file pointer is at end of file, can't read any characters */

        if (pNetFd->filePtrByte == pNetFd->endOfFile)
            {
            semGive (pNetFd->netFdSem);
            return (0);
            }

        cindex = pNetFd->filePtrByte % DATASIZE;   /* char index in datablock */
        dptr = pNetFd->filePtrBlock;
        cptr = &dptr->databuf [cindex];	/* point to current char in datablock */
        bptr = buf;			/* point to beginning of read buffer */

        /* read until maxBytes characters have been read */

        while (byteCount < maxBytes)
            {
            if ((maxBytes - byteCount) < (dptr->used - cindex))
                {
                /* stop reading when maxBytes characters have been read.
                *  This is the last block to read from in order to finish
                *  filling up the buffer.
                */
                bcopy (cptr, bptr, maxBytes - byteCount);
                byteCount = maxBytes;
                }
            else
                {
                /* copy the rest of the datablock into buffer */

                bcopy (cptr, bptr, dptr->used - cindex);
                byteCount = byteCount + dptr->used - cindex;

                if (dptr == (DATABLOCK *) lstLast (&pNetFd->dataLst))
                    {
                    /* this is the last block in the file. Seek to the end. */

                    status = netSeek (pNetFd, pNetFd->endOfFile);
                    semGive (pNetFd->netFdSem);
                    return (status == ERROR ? ERROR : byteCount);
                    }
                else	/* get next block */
                    {
                    dptr = (DATABLOCK *) lstNext ((NODE *) dptr);
                    cindex = 0;
                    cptr = dptr->databuf;	/* point to 1st char in block */
                    bptr = buf + byteCount;	/* move buffer pointer */
                    }
                }
            }   /* end of while */

        status = netSeek (pNetFd, pNetFd->filePtrByte + byteCount);
        }
    else
        {
        FAST int nBytes = 0;
        FAST int bytes = 0;
        FAST int bufOffset = 0;

        /* if it is in the cache, read it from the cache */

        if (pNetFd->bufOffset < pNetFd->bufBytes)
            {
            if ((pNetFd->bufOffset + maxBytes) <= (pNetFd->bufBytes))
                {
                bcopy (pNetFd->buf + pNetFd->bufOffset, buf, maxBytes);
                pNetFd->bufOffset   += maxBytes; /* move the cache pointer */
                pNetFd->filePtrByte += maxBytes; /* move the file pointer */
                pNetFd->status[0]++;		 /* increment read[hit] */
                semGive (pNetFd->netFdSem);
                return (maxBytes);
                }
            else
                {
                bytes = pNetFd->bufBytes - pNetFd->bufOffset;
                bcopy (pNetFd->buf + pNetFd->bufOffset, buf, bytes);
                pNetFd->bufOffset   += bytes;	/* move the cache pointer */
                pNetFd->filePtrByte += bytes;	/* move the file pointer */
                bufOffset = bytes;		/* set the buf offset */
                }
            }

        while ((nBytes = maxBytes - bufOffset) > 0)
            {
            /* read bytes from socket into the cache */

            pNetFd->status[1]++;		/* increment read[mis] */
            byteCount = 0;
            while ((bytes = pNetFd->pNetDev->bufSize - byteCount) > 0)
                {
                bytes = read (pNetFd->dataSock, pNetFd->buf + byteCount, bytes);
                if (bytes <= 0)
                    break;
                else
                    byteCount += bytes;
                }
            pNetFd->bufOffset = 0;		/* set the cache pointer */
            pNetFd->bufBytes  = byteCount;	/* set the cache size */

            /* read bytes from the cache */

            if (bytes < 0)		/* recv error */
                {
                status = ERROR;
                break;
                }

            if (pNetFd->bufBytes == 0)	/* the cache is empty, i.e. EOF */
                break;
            else			/* the cache is not empty */
                {
                bytes = (pNetFd->bufBytes < nBytes) ? pNetFd->bufBytes : nBytes;
                bcopy (pNetFd->buf, buf + bufOffset, bytes);
                pNetFd->bufOffset   += bytes;	/* move the cache pointer */
                pNetFd->filePtrByte += bytes;	/* move the file pointer */
                bufOffset           += bytes;	/* move the buf offset */
                }
            }
        byteCount = bufOffset;			/* set the byte count */
        }
    semGive (pNetFd->netFdSem);

    return (status == ERROR ? ERROR : byteCount);
    }
/*******************************************************************************
*
* netWrite - write bytes to remote file
*
* netWrite copies up to the specified number of bytes from the buffer
* to the open network file descriptor.  Bytes are written starting
* at the spot pointed to by the file's block and byte pointers.
* The file pointer is updated to point immediately after the last
* character that was written.
*
* Called only by the I/O system.
*
* SIDE EFFECTS: moves file pointer
*
* RETURNS:
* Number of bytes written (error if != nbytes), or
* ERROR if nBytes < 0, or no more space can be allocated.
*/

LOCAL int netWrite
    (
    FAST NET_FD *pNetFd,        /* open file descriptor */
    char *buf,                  /* buffer being written from */
    FAST int nBytes             /* number of bytes to write to file */
    )
    {
    STATUS status = OK;
    FAST int byteCount = 0;	/* number of written read so far */

    /* check for valid number of bytes */

    if (nBytes < 0)
        {
        errno = S_netDrv_INVALID_NUMBER_OF_BYTES;
        return (ERROR);
        }

    /* if file opened for O_RDONLY, don't write */
    if ((pNetFd->options & pNetFd->pNetDev->fdMode) == O_RDONLY)
        {
        errno = S_netDrv_READ_ONLY_CANNOT_WRITE;
        return (ERROR);
        }

    semTake (pNetFd->netFdSem, WAIT_FOREVER);

    if (pNetFd->pNetDev->bufSize == 0)
        {
        FAST DATABLOCK *dptr;	/* points to current datablock */
        FAST int cindex;	/* character index datablock's databuf */
        FAST char *cptr;	/* points to char being overwritten in file */
        FAST char *bptr;	/* points to current position in buffer */

        cindex = pNetFd->filePtrByte % DATASIZE;
        dptr   = pNetFd->filePtrBlock;
        cptr   = &dptr->databuf [cindex];
        bptr   = buf;

        while (byteCount < nBytes)
            {
            if ((nBytes - byteCount) < (DATASIZE - cindex))
                {
                /* almost done writing nBytes. This is the last block */

                bcopy (bptr, cptr, nBytes - byteCount);
                byteCount = nBytes;

                /* if we wrote past end of file, adjust end of file pointer */

            if ((pNetFd->filePtrByte + byteCount > pNetFd->endOfFile) &&
                moveEndOfFile (pNetFd, pNetFd->filePtrByte + byteCount)
                == ERROR)
                {
                semGive (pNetFd->netFdSem);
                return (ERROR);
                }
           }
           else		/* not last block to write to */
               {
               bcopy (bptr, cptr, DATASIZE - cindex);
               byteCount = byteCount + DATASIZE - cindex;

                /* if we wrote past end of file, adjust end of file pointer.
                *  If necessary, moveEndOfFile will attach a new datablock
                *  to the end of the data chain.
                */

                if ((pNetFd->filePtrByte + byteCount > pNetFd->endOfFile) &&
                    moveEndOfFile (pNetFd, pNetFd->filePtrByte + byteCount)
                    == ERROR)
                    {
                    semGive (pNetFd->netFdSem);
                    return (ERROR);
                    }

                /* point to first character in next datablock */

                dptr   = (DATABLOCK *) lstNext ((NODE *) dptr);
                cindex = 0;
                cptr   = dptr->databuf;

                /* adjust buffer pointer */

                bptr = buf + byteCount;
                }
            } /* end of while loop */

        pNetFd->options |= FD_DIRTY;

        status = netSeek (pNetFd, pNetFd->filePtrByte + byteCount);
        }
    else
        {
        /* write the data to socket */

        if ((byteCount = write (pNetFd->dataSock, buf, nBytes)) != nBytes)
            status = ERROR;

        pNetFd->filePtrByte += byteCount;
        }

    semGive (pNetFd->netFdSem);

    return (status == ERROR ? ERROR : byteCount);
    }
/*******************************************************************************
*
* netIoctl - do device specific control function
*
* Called only by the I/O system.
*
* RETURNS: Whatever the called function returns.
*/

LOCAL int netIoctl
    (
    FAST NET_FD *pNetFd,        /* open network file descriptor */
    FAST int function,          /* function code */
    FAST int arg                /* argument for function */
    )
    {
    struct stat *pStat;         /* pointer to struct for stat() call */

    switch (function)
	{
	case FIOSEEK:
	    if (pNetFd->pNetDev->bufSize == 0)
                return (netSeek (pNetFd, arg));
	    {
	    int status = OK;
            FAST int bytes = 0;
            FAST int byteCount = 0;

	    if ((arg < 0) ||
		(pNetFd->options & O_WRONLY))	/* no seek in WR_ONLY mode */
		{
		errno = S_netDrv_BAD_SEEK;
		return (ERROR);
		}

	    if (pNetFd->filePtrByte == arg)
	        return (OK);

	    /* forward seek */

	    if (pNetFd->filePtrByte < arg)
		{
	        if ((arg - pNetFd->filePtrByte) <
		    (pNetFd->bufBytes - pNetFd->bufOffset))
		    {
	            /* forward seek in the cache. move the file pointer */

		    pNetFd->status[2]++;	/* increment fseek[hit] */
		    pNetFd->bufOffset  += (arg - pNetFd->filePtrByte);
	            pNetFd->filePtrByte = arg;
		    return (OK);
		    }
	        else
		    {
	            /* forward seek beyond the cache. read the file */

		    pNetFd->status[3]++;	/* increment fseek[mis] */
                    byteCount = pNetFd->filePtrByte +
			        (pNetFd->bufBytes - pNetFd->bufOffset);
		    }
		}

	    /* backward seek */

	    if (pNetFd->filePtrByte > arg)
		{
	        if ((pNetFd->filePtrByte - arg) < pNetFd->bufOffset)
		    {
	            /* backward seek in the cache. move the file pointer */

		    pNetFd->status[4]++;	/* increment bseek[hit] */
		    pNetFd->bufOffset  -= (pNetFd->filePtrByte - arg);
	            pNetFd->filePtrByte = arg;
		    return (OK);
		    }
	        else
		    {
	            /* backward seek beyond the cache. re-open the file */

            	    if ((netSockClose (pNetFd) == ERROR) ||	/* close it */
            	        (netSockOpen (pNetFd) == ERROR))	/* reopen it */
			    return (ERROR);
		    pNetFd->status[5]++;	/* increment bseek[mis] */
		    byteCount = 0;
		    }
		}

            /* read until it reaches the new file pointer */

            while ((bytes = arg - byteCount) > 0)
	        {
	        bytes = (bytes > pNetFd->pNetDev->bufSize) ?
			pNetFd->pNetDev->bufSize : bytes;
                bytes = read (pNetFd->dataSock, pNetFd->buf, bytes);
	        if (bytes <= 0)
		    {
		    pNetFd->filePtrByte = byteCount;
		    return (ERROR);
	            break;
		    }
	        else
	            byteCount += bytes;
	        }

	    /* set the file pointer and flush the cache */

	    pNetFd->filePtrByte = arg;
	    pNetFd->bufOffset = 0;
	    pNetFd->bufBytes  = 0;

	    return (status);
	    }

	case FIOWHERE:
    	    return (pNetFd->filePtrByte);

	case FIONREAD:
	    if (pNetFd->pNetDev->bufSize > 0)
		return (ERROR);
	    else
		{
    	        *((int *) arg) = pNetFd->endOfFile - pNetFd->filePtrByte;
	        return (OK);
	        }

	case FIODIRENTRY:
	    /* this is a kludge for 'ls'.  Do the listing, then return
	       ERROR so that 'ls' doesn't try listing an rt-11 device */

	    netLs (pNetFd);
	    return (ERROR);

	case FIOGETFL:
	    *((int *) arg) = pNetFd->options & pNetFd->pNetDev->fdMode;
	    return (OK);

        case FIOFSTATGET:
            /* get status of a file */

            pStat = (struct stat *) arg;

            /* get file attributes returned in pStat */

            /* zero out the pStat structure */
            bzero ((char *) pStat, sizeof (*pStat));

            /* fill in the elements of the stat struct that we have */

            pStat->st_dev = (ULONG) pNetFd->pNetDev;
            pStat->st_mode = S_IFREG;
            pStat->st_size = pNetFd->endOfFile;

            return (OK);

            break;

	default:
	    errno = S_netDrv_UNKNOWN_REQUEST;
	    return (ERROR);
	}
    }

/*******************************************************************************
*
* netSeek - change file's current character position
*
* This routine moves the file pointer by the offset to a new
* position.  If the new position is past the end of file, fill the
* unused space with 0's.  The end of file pointer gets moved to the
* position immediately following the last '0' written.
* If the resulting file pointer would be negative, ERROR is returned.
*
* Called only by the I/O system.
*
* INTERNAL
* There is deliberate co-recursion between netSeek and netWrite.
*
* RETURNS: OK or ERROR.
*/

LOCAL int netSeek
    (
    FAST NET_FD *pNetFd,
    FAST int newPos
    )
    {
    FAST DATABLOCK *saveFilePtrBlock;
    FAST int saveFilePtrByte;
    int endOfFile;
    int newBlock;
    int curBlock;
    char *buf;
    DATABLOCK *dptr;
    int nbytes;

    if (newPos < 0)
        {
        errno = S_netDrv_BAD_SEEK;
        return (ERROR);
        }

    saveFilePtrBlock = pNetFd->filePtrBlock;
    saveFilePtrByte  = pNetFd->filePtrByte;
    endOfFile        = pNetFd->endOfFile;
    newBlock         = newPos / DATASIZE;
    curBlock         = pNetFd->filePtrByte / DATASIZE;

    /* if new position is past end of file */

    if (newPos > endOfFile)
	{
	/* put 0's at the end of the file */

	if ((buf = (char *) KHEAP_ALLOC ((unsigned) (newPos - endOfFile))) == NULL)
	    {
	    return (ERROR);
	    }

	bzero (buf, newPos - endOfFile);

	/* move file pointer to end of file before writing 0's */
	pNetFd->filePtrBlock = (DATABLOCK *) lstLast (&pNetFd->dataLst);
	pNetFd->filePtrByte = endOfFile;

	/* netWrite will update the file pointer and end of file pointer */

	nbytes = netWrite (pNetFd, buf, (newPos - endOfFile));
	if (nbytes != (newPos - endOfFile))
	    {
	    if (nbytes != ERROR)
	    	errno = S_netDrv_SEEK_PAST_EOF_ERROR;

	    pNetFd->filePtrBlock = saveFilePtrBlock;
	    pNetFd->filePtrByte = saveFilePtrByte;
	    KHEAP_FREE (buf);
	    return (ERROR);
	    }

	KHEAP_FREE (buf);
	}
    else	/* else, new position is within current file size */
	{
    	/* point to new block */
	/* system error if we go out of range of the datablocks */

	dptr = (DATABLOCK *) lstNStep ((NODE *) pNetFd->filePtrBlock,
				       newBlock - curBlock);
	if (dptr == NULL)
	    {
	    errno = S_netDrv_SEEK_FATAL_ERROR;
	    return (ERROR);
	    }

	pNetFd->filePtrBlock = dptr;

	/* point to new byte */

    	pNetFd->filePtrByte = newPos;
	}

    return (OK);
    }
/*******************************************************************************
*
* moveEndOfFile - moves end of file pointer to new position
*
* Adds a new datablock to the end of the datablock list if necessary.
* Assumes that end of file moves at most to the first position of
* the next datablock.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS moveEndOfFile
    (
    FAST NET_FD *pNetFd,
    FAST int newPos
    )
    {
    FAST DATABLOCK *dptr;	/* pointer to new datablock */

    /*
    *  As soon as system error handling is implemented, a message should
    *  be put here.
    *
    *  If the new position is before the current end of file,
    *  OR more than one datablock ahead,
    *  OR further away than the first byte of the next datablock,
    *  this is a system error ....looks awful, I know
    */

    if ((newPos <= pNetFd->endOfFile)				||
	((newPos - pNetFd->endOfFile) > DATASIZE)		||
	((newPos % DATASIZE < pNetFd->endOfFile % DATASIZE)	&&
	(newPos % DATASIZE != 0)))
	{
	errno = S_netDrv_BAD_EOF_POSITION;
	return (ERROR);
	}

    /* if new position is in datablock after end of file,
    *  add another datablock to the file
    */

    if ((newPos / DATASIZE) > (pNetFd->endOfFile / DATASIZE))
	{
	/* current datablock is full.
	*  New position is in the block after the current
	*  end of file.
	*/

	((DATABLOCK *) lstLast (&pNetFd->dataLst))->used = DATASIZE;

	if ((dptr = (DATABLOCK *) KHEAP_ALLOC (sizeof (DATABLOCK))) == NULL)
	    return (ERROR);

    	dptr->used = 0;
	lstAdd (&pNetFd->dataLst, (NODE *) dptr);
	}
    else
	((DATABLOCK *) lstLast (&pNetFd->dataLst))->used = newPos % DATASIZE;

    pNetFd->endOfFile = newPos;
    return (OK);
    }

/*******************************************************************************
*
* netFdCreate - create a network file descriptor
*
* Returns an open network file descriptor.
* Files are created with O_WRONLY or O_RDWR modes.
* A O_RDONLY mode defaults to O_RDWR.
*
* RETURNS: Pointer to open network file descriptor or ERROR.
*/

LOCAL int netFdCreate
    (
    NET_DEV *pNetDev,
    char *name,         /* remote file name */
    int mode            /* ignored, always set to O_WRONLY */
    )
    {
    FAST NET_FD *pNetFd;
    DATABLOCK *pData;
    char *pFileName;
    FAST int fileNameLen;
    FAST int dirNameLen;
    char *ptr;

    /* don't allow null filenames */

    if (name[0] == EOS)
	{
	errno = S_ioLib_NO_FILENAME;
	return (ERROR);
	}

    /* get directory and filename pointers and lengths */

    pFileName   = pathLastNamePtr (name);
    fileNameLen = strlen (pFileName);
    dirNameLen  = pFileName - name;

    /* Some machines don't like to have a directory name terminated by "/" or
     * "\", unless is the root directory.  This code clips the extra slash.
     */

    if ((dirNameLen > 1) &&
	(name[dirNameLen-1] == '/' || name[dirNameLen-1] == '\\'))
	dirNameLen--;

    /* allocate and initialize net file descriptor;
     * leave room for directory and file name and both EOSs
     *
     * Make sure the file descriptor structure is allocated from the Kernel
     * heap since this structure includes a Mutex which is a Wind object and
     * as for all Wind object should be allocated from an location visible
     * by Kernel tasks.
     */

    if ((ptr = (char *) KHEAP_ALLOC ((unsigned) (sizeof (NET_FD) + fileNameLen +
				   dirNameLen + 2))) == NULL)
	return (ERROR);

    pNetFd = (NET_FD *) ptr;

    pNetFd->pNetDev      = pNetDev;
    pNetFd->filePtrByte  = 0;
    pNetFd->endOfFile    = 0;	/* XXX should be determined when O_RDWR! */
    pNetFd->options      = pNetFd->pNetDev->fdMode & ((mode == O_WRONLY) ? O_WRONLY : O_RDWR);
    pNetFd->options     |= FD_DIRTY;	/* set dirty */

    /* set remote directory name and file name */

    pNetFd->remFileName = ptr + sizeof (NET_FD);
    (void) strcpy (pNetFd->remFileName, pFileName);

    pNetFd->remDirName = pNetFd->remFileName + fileNameLen + 1;
    (void) strncpy (pNetFd->remDirName, name, dirNameLen);
    pNetFd->remDirName [dirNameLen] = EOS;

    /* attach first empty datablock */

    pData = (DATABLOCK *) KHEAP_ALLOC(sizeof (DATABLOCK));
    pData->used = 0;

    pNetFd->filePtrBlock = pData;

    lstInit (&pNetFd->dataLst);
    lstAdd (&pNetFd->dataLst, (NODE *) pData);

    bzero ((char *)pNetFd->status, sizeof (pNetFd->status));
    if ((pNetFd->pNetDev->bufSize > 0) &&
	((pNetFd->buf = KHEAP_ALLOC(pNetFd->pNetDev->bufSize)) == NULL))
	return (ERROR);

    pNetFd->netFdSem = semMCreate (mutexOptionsNetDrv);

    return ((int) pNetFd);
    }

/*******************************************************************************
*
* netFdRelease - free up NetFd
*/

LOCAL void netFdRelease
    (
    FAST NET_FD *pNetFd
    )
    {
#ifdef _WRS_VXWORKS_5_X
    lstFree (&pNetFd->dataLst);	/* free up data list */
#else
    lstInternalFree (&pNetFd->dataLst, kHeapFree);	/* free up data list */
#endif /* _WRS_VXWORKS_5_X */
    
    if (pNetFd->pNetDev->bufSize > 0)		/* free the read buffer */
	KHEAP_FREE(pNetFd->buf);
    semDelete (pNetFd->netFdSem);		/* terminate netFdSem */
    KHEAP_FREE ((char *) pNetFd);		/* deallocate network fd */
    }

/*******************************************************************************
*
* netSockOpen - open the socket connection
*/

LOCAL STATUS netSockOpen
    (
    NET_FD *pNetFd
    )
    {
    char command[MAX_FILENAME_LENGTH];
    char buffer [MAX_FILENAME_LENGTH];
    char usr [MAX_IDENTITY_LEN];
    char passwd [MAX_IDENTITY_LEN];
    char * p;

    /* open the remote file */

    remCurIdGet (usr, passwd);

    if (pNetFd->pNetDev->protocol == PROTO_FTP)
	{
	p = ((pNetFd->options & pNetFd->pNetDev->fdMode) == O_RDONLY) ?
	    "RETR %s" : "STOR %s";

	if (ftpXfer (pNetFd->pNetDev->host, usr, passwd, "",
		     p, pNetFd->remDirName, pNetFd->remFileName,
		     &pNetFd->ctrlSock, &pNetFd->dataSock) == ERROR)
	    {
	    NETDRV_DEBUG ("netSockOpen:  error calling ftpXfer()  errno:0x%08x\n", errno,1,2,3,4,5);
	    return (ERROR);
	    }
	}
    else
	{
	if (pathCat (pNetFd->remDirName, pNetFd->remFileName, buffer) == ERROR)
	    return (ERROR);

	if ((pNetFd->options & pNetFd->pNetDev->fdMode) == O_RDONLY) 
		sprintf (command, "/bin/cat < %s", buffer);
	else
		sprintf (command, "/bin/cat > %s", buffer);


	pNetFd->dataSock = rcmd (pNetFd->pNetDev->host, RSHD, usr,
			    usr, command, &pNetFd->ctrlSock);

	if (pNetFd->dataSock == ERROR)
	    return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* netSockClose - close the socket connection
*/

LOCAL STATUS netSockClose
    (
    NET_FD *pNetFd
    )
    {
    int status = OK;
    int nBytes;
    char errBuf [DATASIZE];
    char *errMsg0 = "cat: read error: Is a directory";
    char *errMsg1 = "cat: write error: Broken pipe";
    int errMsgLen0 = strlen (errMsg0);
    int errMsgLen1 = strlen (errMsg1);

    /* complete the FTP transfer before closing the dataSock */

    if ((pNetFd->pNetDev->bufSize > 0) &&
        (pNetFd->pNetDev->protocol == PROTO_FTP))
           {
           char buf [1024];
           while ((nBytes = read (pNetFd->dataSock, buf, sizeof (buf))) > 0)
            ;
           }

    status = close (pNetFd->dataSock);
    
    if (status == ERROR )
        {
        NETDRV_DEBUG ("netSockClose: error closing fd:%d errno=0x%08x\n", pNetFd->dataSock, errno,3,4,5,6);
        }

    if (pNetFd->pNetDev->protocol == PROTO_FTP)
        {
        status = ftpReplyGet (pNetFd->ctrlSock, FALSE);

        /* Check for specific status types */
        if (status != FTP_COMPLETE)
            {
            NETDRV_DEBUG ("netSockClose: error during ftpReplyGet ctrlSock:%d errno=0x%08x return value:%d\n",
                            pNetFd->ctrlSock, errno,status,4,5,6);
            status = ERROR;
            }
        }
    else
        {
        /* check control socket for error */

        if ((nBytes = fioRead (pNetFd->ctrlSock, errBuf, sizeof (errBuf) - 1))
            > 0)
            {
            /* print error message on standard error fd */

            errBuf [nBytes] = EOS;	/* insure string termination */

            /* check error message indicating cat of NFS mounted directory */

            if ((((pNetFd->options & pNetFd->pNetDev->fdMode) == O_RDONLY) &&
                 (strncmp (errBuf, errMsg0, errMsgLen0) != 0) &&
                 (strncmp (errBuf, errMsg1, errMsgLen1) != 0)) ||
                ((pNetFd->options & pNetFd->pNetDev->fdMode) == O_WRONLY))
                {
                NETDRV_DEBUG ("netSockClose: %s:%s   errno:0x%08x\n",  
                pNetFd->pNetDev->host, errBuf, errno,1,2,3);

                /* set the task's status according to the UNIX error */
                getNetStatus (errBuf);

                status = ERROR;
                }
            }
        else
            {
            NETDRV_DEBUG ("netSockClose: error during fioread ctrlSock:%d errno=0x%08x return value:%d\n", 
            pNetFd->ctrlSock, errno,status,4,5,6);
            }
        }

    if (pNetFd->pNetDev->protocol == PROTO_FTP &&
        ftpCommand (pNetFd->ctrlSock, "QUIT",0,0,0,0,0,0) != FTP_COMPLETE)
        {
        NETDRV_DEBUG ("netSockClose: error during ftpCommand (QUIT) ctrlSock:%d errno=0x%08x return value:%d\n", 
        pNetFd->ctrlSock, errno,status,4,5,6);
        status = ERROR;
        }

        status = close (pNetFd->ctrlSock);
        if (status == ERROR )
            {
            NETDRV_DEBUG ("netSockClose: error closing ctrl sock fd:%d errno=0x%08x\n", 
            pNetFd->ctrlSock, errno,3,4,5,6);
            }

        return (status);
    }
/*******************************************************************************
*
* netDrvDebugLevelSet - set the debug level of the netDrv library routines
*
* This routine enables the debugging of calls to the net driver.  The argument
* NETLIB_DEBUG_ERRORS will display only error messages to the console.  The 
* argument NETLIB_DEBUG_ALL will display warnings and errors to the console.
*
* RETURNS : OK, or ERROR if the debug level is invalid
*/

STATUS netDrvDebugLevelSet
    (
    UINT32 debugLevel /* NETDRV_DEBUG_OFF, NETDRV_DEBUG_ERRORS, NETDRV_DEBUG_ALL */
    )
    {
    switch (debugLevel)
        {
        case (NETLIB_DEBUG_OFF):    /* No debugging messages */ 
            netDrvDebugErrors = debugLevel;
            return (OK);
            break;

        case (NETLIB_DEBUG_ERRORS): /* Display all errors and warnings  */
            netDrvDebugErrors = debugLevel;
            return (OK);
            break;

        case (NETLIB_DEBUG_ALL):    /* Display all errors, warning and statistics */
            netDrvDebugErrors = debugLevel;
            netDrvDebugStats = 1;
            return (OK);
            break;

        default:
            errno = S_netDrv_ILLEGAL_VALUE;
            return (ERROR);
        }
    }
/*******************************************************************************
*
* netDrvFileDoesNotExist - Applette to test if a file does not exist
*
* This routine is used to parse the text of a remote FTP server stating a file does
*    not exist.   This routine is only used during an open() with the O_CREAT
*    option specified.  An 'NLST' command is issued and the response is compared
*    against various text strings.
*
* RETURNS : OK if the file does not exist, or ERROR if the file exists
*
* NOMANUAL
*/

STATUS netDrvFileDoesNotExist 
    (
    char *filename,
    char *response
    )
    {

/* XXX this function needs to move to usrNetDrv.c after veloce freeze */

#define SUNOS_ERRMSG "not found"
#define SOLARIS_ERRMSG " No such file"

    /* SunOs 1.x */
    if (strncmp (response + strlen (filename) + 1, 
        SUNOS_ERRMSG, 
        strlen (SUNOS_ERRMSG)
        ) == 0)
        {
        NETDRV_DEBUG ("fileDoesNotExist:matches SunOs 1.x\n", errno,1,2,3,4,5);
        return (OK);
        }

    /* Solaris (SunOs 5.7) */
    if (strncmp (response + strlen (filename) + 1, 
        SOLARIS_ERRMSG, 
        strlen (SOLARIS_ERRMSG)
        ) == 0)
        {
        NETDRV_DEBUG ("fileDoesNotExist:matches Solaris\n", errno,1,2,3,4,5);
        return (OK);
        }

        NETDRV_DEBUG ("fileDoesNotExist:NO MATCH - file must exist\n", errno,1,2,3,4,5);

        return ERROR;
    }
/*******************************************************************************
*
* netDrvFileDoesNotExistInstall - install an applette to test if the file exists
*
* Install a function which will test if a given file exists.
*
* The <pAppletteRtn> argument provides a routine using the following interface:
* \cs
* STATUS appletteRoutine
*     (
*     char *filename, /@ file name queried @/
*     char *response  /@ server response string @/
*     )
* \ce
* 
* The netDrv routine calls the applette routine during an open with O_CREAT
* specified. The system will perform an NLST command and then use the applette 
* routine to parse the response.  The routine compensates for server response
* implementaion variations.   The applette should return 'OK' if the file is 
* not found and 'ERROR'  if the file is found.
*
* RETURNS : OK, installation successful; ERROR, installation error.
*
* SEE ALSO : open()
*
*/

STATUS netDrvFileDoesNotExistInstall  
    (
    FUNCPTR pAppletteRtn /* Function that returns TRUE or FALSE */
    )
    {

    /* At least prevent this from being a disaster */
    if (pAppletteRtn == NULL)
        return (ERROR); 

    _func_fileDoesNotExist    = pAppletteRtn;

    return (OK); 
    }




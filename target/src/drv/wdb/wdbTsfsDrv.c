/* wdbTsfsDrv.c - virtual generic file I/O driver for the WDB agent */

/* Copyright 1996-2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01s,09oct01,jhw  Fixed memory leak in tsfsOpen(). SPR 67981.
01r,03mar99,dgp  update to reference project facility, make tsfsOpen() NOMANUAL
01q,28jan99,jmp  added initialization of semGiveAddr in WDB_TSFS_GENERIC_INFO
                 structure, for TSFS_BOOT facility (SPR# 24466).
01p,23nov98,cth  added explicit setting of WDB_TSFS_O_TEXT open mode (SPR 23467)
01o,31aug98,dgp  final editing for WV 2.0 FCS, corrections to tgtsvr stuff
01n,28aug98,dgp  FCS man page edit, add tgtsvr info
01m,25aug98,cth  added errno mapping for ECONNREFUSED
01l,10aug98,cth  FIORENAME now handles prepended device names, updated docs
01k,27may98,cth  modified docs, removed get snd/rcv buf ioctls
01j,05may98,dgp  clean up man pages for WV 2.0 beta release
01i,16apr98,cth  removed debug print statements
01h,16apr98,cth  mapped errnos from tsfs portable definitions to vxworks'
      		 added errno reset in open, updated docs
01g,03apr98,cjtc i960 port for WV20
01f,19mar98,cth  added ioctl support for sockets
01e,25feb98,cth  added O_APPEND, _EXCL modes, doc changes, added FIORENAME
01d,30jan98,cth  added directory creation/deletion, added man page
01c,18aug97,cth  added FIOSNDURGB ioctl command
01b,18nov96,dbt  replaced the field delete in WDB_TSFS_INFO struct for C++
                 compatibility, with field remove.
01a,28aug96,c_s  written, based on wdbVioDrv.c by ms.
*/

/*
DESCRIPTION

This library provides a virtual file I/O driver for use with the WDB
agent.  I/O is performed on this virtual I/O device exactly as it would be
on any device referencing a VxWorks file system.  File operations, such as
read() and write(), move data over a virtual I/O channel created between the
WDB agent and the Tornado target server.  The operations are then executed 
on the host file system.  Because file operations are actually performed on 
the host file system by the target server, the file system presented by this 
virtual I/O device is known as the target-server file system, or TSFS.

The driver is installed with wdbTsfsDrv(), creating a device typically
called '/tgtsvr'.  See the manual page for wdbTsfsDrv() for more information
about using this function.  The initialization is done automatically,
enabling access to TSFS, when INCLUDE_WDB_TSFS is defined.
The target server also must have TSFS enabled in order to use TSFS.  See the
.I WindView User's Guide: Data Upload
and the target server documentation.

TSFS SOCKETS
TSFS provides all of the functionality of other VxWorks file systems.
For details, see the 
.pG "I/O System and Local File Systems."
In addition to normal
files, however, TSFS also provides basic access to TCP sockets.  This
includes opening the client side of a TCP socket, reading, writing, and
closing the socket.  Basic setsockopt() commands are also supported.

To open a TCP socket using TSFS, use a filename of the form:
.tS
    TCP:<server_name> | <server_ip>:<port_number>
.tE

To open and connect a TCP socket to a server socket located on a server 
named 'mongoose', listening on port 2010, use the following:
.CS
    fd = open ("/tgtsvr/TCP:mongoose:2010", 0, 0)
.CE
The open flags and permission arguments to the open call are ignored when
opening a socket through TSFS.  If the server 'mongoose' has an IP
number of '144.12.44.12', you can use the following equivalent form of the 
command:
.CS
    fd = open ("/tgtsvr/TCP:144.12.44.12:2010", 0, 0)
.CE

DIRECTORIES
All directory functions, such as mkdir(), rmdir(), opendir(), readdir(),
closedir(), and rewinddir() are supported by TSFS, regardless of
whether the target server providing TSFS is being run on a UNIX or 
Windows host.

While it is possible to open and close directories using open() and close(),
it is not possible to read from a directory using read(). Instead, readdir() 
must be used.  It is also not possible to write to an open directory, and 
opening a directory for anything other than read-only results in an 
error, with 'errno' set to 'EISDIR'.  Calling read() on a directory 
returns 'ERROR' with 'errno' set to 'EISDIR'.

OPEN FLAGS
When the target server that is providing the TSFS is running on a Windows
host, the default file-translation mode is binary translation.  If text
translation is required, then WDB_TSFS_O_TEXT can be included in the mode
argument to open().  For example:
.CS
    fd = open ("/tgtsvr/foo", O_CREAT | O_RDWR | WDB_TSFS_O_TEXT, 0777)
.CE

If the target server providing TSFS services is running on a UNIX host, 
WDB_TSFS_O_TEXT is ignored.

TGTSVR
For general information on the target server, see the reference
entry for 'tgtsvr'. In order to use this library, the target server must
support and be configured with the following options:

.iP "-R <root>" 20
Specify the root of the host's file system that is visible to target processes
using TSFS.  This flag is required to use TSFS.  Files under this root are by
default read only.  To allow read/write access, specify -RW.
.iP "-RW"
Allow read and write access to host files by target processes using
TSFS.  When this option is specified, access to the
target server is restricted as if `-L' were also specified.

IOCTL SUPPORT
TSFS supports the following ioctl() functions for controlling files
and sockets. Details about each function can be found in the documentation 
listed below.

.iP 'FIOSEEK'
.iP 'FIOWHERE'
.iP 'FIOMKDIR'
Create a directory.  The path, in this case '/tgtsvr/tmp', must be an
absolute path prefixed with the device name. To create the directory '/tmp' 
on the root of the TSFS file system use the following:
.CS
    status = ioctl (fd, FIOMKDIR, "/tgtsvr/tmp")
.CE
.iP 'FIORMDIR'
Remove a directory.  The path, in this case '/tgtsvr/foo', must be an 
absolute path prefixed with the device name.  To remove the directory '/foo' 
from the root of the TSFS file system, use the following:
.CS
    status = ioctl (fd, FIORMDIR, "/tgtsvr/foo")
.CE
.iP 'FIORENAME'
Rename the file or directory represented by 'fd' to the name in the string
pointed to by 'arg'.  The path indicated by 'arg' may be prefixed with the
device name or not.  Using this ioctl() function with the path '/foo/goo'
produces the same outcome as the path '/tgtsvr/foo/goo'.  The path is not
modified to account for the current working directory, and therefore must
be an absolute path.
.CS
    char *arg = "/tgtsvr/foo/goo"; 
    status = ioctl (fd, FIORENAME, arg);
.CE
.iP 'FIOREADDIR'
.iP 'FIONREAD'
Return the number of bytes ready to read on a TSFS socket file
descriptor.
.iP 'FIOFSTATGET'
.iP 'FIOGETFL'
.LP
The following ioctl() functions can be used only on socket file
descriptors.  Using these functions with ioctl() provides similar behavior
to the setsockopt() and getsockopt() functions usually used with socket
descriptors.  Each command's name is derived from a
getsockopt()/setsockopt() command and works in exactly the same way as the
respective getsockopt()/setsockopt() command.  The functions setsockopt()
and getsockopt() can not be used with TSFS socket file descriptors.

For example, to enable recording of debugging information on the TSFS socket
file descriptor, call:
.CS
    int arg = 1;
    status = ioctl (fd, SO_SETDEBUG, arg);
.CE
To determine whether recording of debugging information for the TSFS-socket
file descritptor is enabled or disabled, call:
.CS
    int arg;
    status = ioctl (fd, SO_GETDEBUG, & arg);
.CE
After the call to ioctl(), 'arg' contains the state of the debugging attribute.

The ioctl() functions supported for TSFS sockets are:

.iP SO_SETDEBUG
Equivalent to setsockopt() with the SO_DEBUG command.
.iP SO_GETDEBUG
Equivalent to getsockopt() with the SO_DEBUG command.
.iP SO_SETSNDBUF
This command changes the size of the send buffer of the host socket.  The 
configuration of the WDB channel between the host and target also affects the
number of bytes that can be written to the TSFS file descriptor in a single
attempt.
.iP SO_SETRCVBUF
This command changes the size of the receive buffer of the host socket.  The 
configuration of the WDB channel between the host and target also affects the
number of bytes that can be read from the TSFS file descriptor in a single
attempt.
.iP SO_SETDONTROUTE
Equivalent to setsockopt() with the SO_DONTROUTE command.
.iP SO_GETDONTROUTE
Equivalent to getsockopt() with the SO_DONTROUTE command.
.iP SO_SETOOBINLINE
Equivalent to setsockopt() with the SO_OOBINLINE command.
.iP SO_GETOOBINLINE
Equivalent to getsockopt() with the SO_OOBINLINE command.
.iP SO_SNDURGB
The SO_SNDURGB command sends one out-of-band byte (pointed to by 'arg') 
through the socket.

ERROR CODES
The routines in this library return the VxWorks error codes that 
most closely match the errnos generated by the corresponding host function.
If an error is encountered that is due to a WDB failure, a WDB error
is returned instead of the standard VxWorks 'errno'.  If an 'errno' generated
on the host has no reasonable VxWorks counterpart, the host 'errno' is
passed to the target calling routine unchanged.

SEE ALSO:
.I "Tornado User's Guide,"
.I "VxWorks Programmer's Guide: I/O System, Local File Systems"

INTERNAL
opendir(), closedir(), readdir() and rewinddir() are supported when
the tgtsvr is running on a Unix host.  All of these, with the exception of
rewinddir() are supported on Win32.  Win32 provides directory listings with
an iterator model (findfirst(), findnext(), findclose()).  There is no
way to back up this iterator, like rewinddir does.

There are times when the host must return an error code that is not 
defined on the target.  These error codes are defined in wdb.h.  Each
time the errno is set in this function, it is first mapped from the
portable error codes defined in wdb.h to a vxWorks' error code.  Common,
usually Posix, errnos are passed through this mapping unchanged.  If
an errno generated on the host is not understood by the mapping routines,
then it is passed through unchanged.  This may give the user useful 
information about what has happened when vxWorks doesn't have an 
equivalent errno.  Many of the errnos mapped are the Win32 socket and
networking errnos, which have vxWorks counterparts, but are very different
from unix and vxWorks definitions.

The ioctl command FIORENAME does not work ideally.  It should modify the path
it is handed to account for the current working directory.  Doing this however
requires that this wdb-virtual-io driver call into ioLib, something that has
been avoided so far.  This creates a problem with rename() that is not seen
in dosFsLib.  The rename function, in ioLib, does not patch up the path name
with the current working directory, letting the ioctl in dosFsLib do it
instead.  When the current working directory is '/tgtsvr/foo/goo', and it 
contains the file 'file1', and the tsfsRoot is '/folk/blimpy', calling 
'rename ("file1", "file-renamed") results in /folk/blimpy/foo/goo/file1
being renamed to /folk/blimpy/file-renamed.  Notice the /foo/goo working
directory was ignored.  By the way, passFsLib has the same problem.
*/

#include "vxWorks.h"
#include "ioLib.h"
#include "iosLib.h"
#include "tyLib.h"
#include "stdlib.h"
#include "stdio.h"
#include "intLib.h"
#include "string.h"
#include "sys/stat.h"
#include "dirent.h"

#include "wdb/wdb.h"
#include "wdb/wdbLibP.h"
#include "wdb/wdbEvtLib.h"
#include "wdb/wdbVioLib.h"
#include "wdb/wdbLibP.h"



#if (defined (CPU_FAMILY) && (CPU_FAMILY==I960) && (defined __GNUC__))
#pragma align 1                 /* tell gcc960 not to optimize alignments */
#endif  /* CPU_FAMILY==I960 */

typedef struct
    {
    DEV_HDR		devHdr;			/* device header */
    } TSFS_DEV;

typedef struct
    {
    int			channel;		/* tsfs channel number */
    int			openmode;		/* used for FIOGETFL */
    SEMAPHORE		tsfsSem;		/* mutex sem for access */
    SEMAPHORE		waitSem;		/* used to wait for tgtsvr */
    WDB_EVT_NODE	evtNode;		/* agent event node */
    WDB_TSFS_INFO	info;			/* event contents */
    struct
	{
	TGT_INT_T	value;			/* value to return to caller */
	TGT_INT_T	errNo;			/* errno to return to caller */
	TGT_INT_T	extra1;			/* for ioctl extra results */
	TGT_INT_T	extra2;			/* for ioctl extra results */
	}		result;
    } TSFS_CHANNEL_DESC;

#if (defined (CPU_FAMILY) && (CPU_FAMILY==I960) && (defined __GNUC__))
#pragma align 0                 /* turn off alignment requirement */
#endif  /* CPU_FAMILY==I960 */

static int		tsfsDrvNum;		/* drvnum for this driver */
static int		tsfsChanNum;		/* next channel number */
static TSFS_DEV		tsfsDev;		/* virtual I/O device */



/* globals */

int mutexOptionsTsfsDrv   = SEM_Q_PRIORITY | SEM_DELETE_SAFE;
int binaryOptionsTsfsDrv  = SEM_Q_FIFO;
int wdbTsfsDefaultDirPerm = DEFAULT_DIR_PERM;	/* created directory perms */


/* forward declarations */

static int 		tsfsCreate   (TSFS_DEV *pDev, char *name, int mode);
static int     		tsfsOpen     (TSFS_DEV *pDev, char * name, int mode, 
				      int perm);
static int     		tsfsDelete   (TSFS_DEV *pDev, char * name);
static STATUS  		tsfsClose    (TSFS_CHANNEL_DESC *pChannelDesc);
static int 		tsfsRead     (TSFS_CHANNEL_DESC *pNetFd, char *buf,
				      int maxBytes);
static int 		tsfsWrite    (TSFS_CHANNEL_DESC *pNetFd, char *buf,
				      int maxBytes);
static STATUS		tsfsIoctl    (TSFS_CHANNEL_DESC *pChannelDesc,
				      int request, int arg);
static int		tsfsErrnoMap (int hostErrno);
static void		tsfsEventGet (void * pChannelDesc,
				      WDB_EVT_DATA *pEvtData);
static void		tsfsEventDeq (void * pChannelDesc);


/*******************************************************************************
*
* wdbTsfsDrv - initialize the TSFS device driver for a WDB agent
*
* This routine initializes the VxWorks virtual I/O "2" driver and creates
* a TSFS device of the specified name.
*
* This routine should be called exactly once, before any reads, writes, or
* opens.  Normally, it is called by usrRoot() in usrConfig.c,
* and the device name created is '/tgtsvr'.
*
* After this routine has been called, individual virtual I/O channels
* can be opened by appending the host file name to the virtual I/O
* device name.  For example, to get a file descriptor for the host
* file '/etc/passwd', call open() as follows:
* .CS
*     fd = open ("/tgtsvr/etc/passwd", O_RDWR, 0)
* .CE
*
* RETURNS: OK, or ERROR if the driver can not be installed.
*/

STATUS wdbTsfsDrv
    (
    char *		name		/* root name in i/o system */
    )
    {
    /* check if driver already installed */

    if (tsfsDrvNum > 0)
	return (OK);

    tsfsDrvNum = iosDrvInstall (tsfsCreate, tsfsDelete, tsfsOpen,
				tsfsClose, tsfsRead, tsfsWrite, 
				tsfsIoctl);

    if (tsfsDrvNum <= 0)
        return (ERROR);

    /* Add the device to the I/O systems device list */

    return (iosDevAdd (&tsfsDev.devHdr, name, tsfsDrvNum));
    }

/*******************************************************************************
*
* tsfsCreate - open a virtual I/O channel
*
* RETURNS: tsfsDev handle
*/

static int tsfsCreate
    (
    TSFS_DEV *		pDev,		/* I/O system device entry */
    char *		name,		/* name of file to open */
    int			mode		/* VxWorks open mode */
    )

    {
    return tsfsOpen (pDev, name, mode | O_CREAT | O_TRUNC, 0666);
    }

/*******************************************************************************
*
* tsfsOpen - open a virtual I/O channel
*
* This routine opens a virtual I/O channel and returns a device handle
* for the channel.  It does this by creating a WDB event containing the
* information necessary for the tgtsvr to perform the requested open
* operation.  This routine waits, after the event is sent to the tgtsvr,
* until the tgtsvr stores the results of the request back to target memory.
* These results are handed back to the caller.
*
* This routine is also used to create files and directories.  After the file
* or directory is created, it is opened and a valid device handle is handed
* back to the caller.
*
* Note, when creating a directory, the mode argument (O_RDWR, O_RDONLY...)
* is not necessary, except to indicate O_CREAT.  It is illegal in most file
* systems, including all of VxWorks' file systems, to open a directory in
* any mode other than O_RDONLY.  Because the directory is created, as well
* as opened in this routine, it is unreasonable to expect that the mode be
* O_RDONLY in order to succeed.  After all, the user wants to create the
* directory, not open it.  This is confused by our implementation of mkdir,
* where open is used to do the creation.  To resolve the problem here, the
* mode is coerced to O_RDONLY when a directory is being created.  Opening a
* directory (not creating it too) will rightfully fail if the mode is
* anything other than O_RDONLY.
*
* NOMANUAL
*
* RETURNS: tsfsDev handle
*/

static int tsfsOpen
    (
    TSFS_DEV *		pDev,		/* I/O system device entry */
    char *		name,		/* name of file to open */
    int			mode,		/* VxWorks open mode */
    int			perm		/* permission bits */
    )
    {
    TSFS_CHANNEL_DESC *	pChannel;	/* channel handle */
    int			pmode = 0;	/* portable open mode */
    int			pperm;		/* portable open permission-word */

    /* create a channel descriptor */

    pChannel = (TSFS_CHANNEL_DESC *) malloc (sizeof (TSFS_CHANNEL_DESC));
    if (pChannel == NULL)
	{
	errno = tsfsErrnoMap (ENOMEM);
	return ERROR;
	}

    /* initialize the channel descriptor */

    pChannel->channel = tsfsChanNum++;
    semMInit (&pChannel->tsfsSem, mutexOptionsTsfsDrv);
    semBInit (&pChannel->waitSem, binaryOptionsTsfsDrv, SEM_EMPTY);

    /* initialize our event node */

    wdbEventNodeInit (&pChannel->evtNode, tsfsEventGet, tsfsEventDeq,
		      pChannel);

    /* Now format up an event. */

    pChannel->info.info.channel = pChannel->channel;
    pChannel->info.info.opcode = WDB_TSFS_OPEN;
    pChannel->info.info.semId = (TGT_ADDR_T) &pChannel->waitSem;
    pChannel->info.info.pResult = (TGT_ADDR_T) &pChannel->result.value;
    pChannel->info.info.pErrno = (TGT_ADDR_T) &pChannel->result.errNo;
    pChannel->info.info.semGiveAddr = (TGT_ADDR_T) &semGive;
    pChannel->info.extra.open.filename = (TGT_ADDR_T) name;
    pChannel->info.extra.open.fnameLen = strlen (name);
    pChannel->openmode = mode;

    /* Map between VxWorks modes and portable TSFS modes. */

    if (mode & O_CREAT)
	{
	pmode |= WDB_TSFS_O_CREAT;
	mode &= ~O_CREAT;
	}

    if (mode & O_TRUNC)
	{
	pmode |= WDB_TSFS_O_TRUNC;
	mode &= ~O_TRUNC;
	}

    if (mode & O_APPEND)
	{
	pmode |= WDB_TSFS_O_APPEND;
	mode &= ~O_APPEND;
	}

    if (mode & O_EXCL)
	{
	pmode |= WDB_TSFS_O_EXCL;
	mode &= ~O_EXCL;
	}

    if (mode & WDB_TSFS_O_TEXT)
	{
	pmode |= WDB_TSFS_O_TEXT;
	mode &= ~WDB_TSFS_O_TEXT;
	}

    switch (mode)
	{
	case O_RDONLY:		pmode |= WDB_TSFS_O_RDONLY; break;
	case O_RDWR:		pmode |= WDB_TSFS_O_RDWR;   break;
	case O_WRONLY:		pmode |= WDB_TSFS_O_WRONLY; break;
	}

    pChannel->info.extra.open.mode = pmode;

    /* 
     * If we are creating a file, regular or directory, we let the host side
     * know what type by using the permission word.  Here we map between
     * VxWorks' FSTAT_DIR/REG and the portable TSFS file types.  See the
     * comments above about coercing the mode to O_RDONLY when creating
     * a directory.  This coersion is done in the tgtsvr rather than here.
     */

    pperm = perm;

    if (pmode & WDB_TSFS_O_CREAT)
	{
	if (perm & FSTAT_DIR)
	    {
	    pperm |= WDB_TSFS_S_IFDIR;
	    pperm &= ~FSTAT_DIR;
	    }
        else
	    {
	    pperm |= WDB_TSFS_S_IFREG;
	    pperm &= ~FSTAT_REG;
            }
        }

    pChannel->info.extra.open.perm = pperm;

    /* Post the event. */

    wdbEventPost (&pChannel->evtNode);

    /* Wait for the tgtsvr to get back to us with the result. */

    semTake (&pChannel->waitSem, WAIT_FOREVER);

    if (pChannel->result.errNo == OK)
	{
        errno = 0;
	return (int) pChannel;
	}

    errno = tsfsErrnoMap (pChannel->result.errNo);
    free (pChannel);
    return ERROR;
    }

/*******************************************************************************
*
* tsfsDelete - delete a file via the target server
*
* RETURNS: status
* NOMANUAL
*/

static int tsfsDelete
    (
    TSFS_DEV *		pDev,		/* I/O system device entry */
    char *		name		/* name of file to destroy */
    )
    {
    int			result;
    TSFS_CHANNEL_DESC *	pChannel;	/* channel handle */

    /* 
     * for delete, we aren't given a channel to begin with.  We'll have
     * to create one just for this operation. 
     */

    pChannel = (TSFS_CHANNEL_DESC *) malloc (sizeof (TSFS_CHANNEL_DESC));

    if (pChannel == NULL)
	{
	errno = tsfsErrnoMap (ENOMEM);
	return ERROR;
	}

    /*
     * For non-file requests (i.e., ones like delete, where we don't 
     * have an open file handle before we attempt the request), we 
     * set the channel number to -1.
     */

    pChannel->channel = -1;
    semBInit (&pChannel->waitSem, binaryOptionsTsfsDrv, SEM_EMPTY);

    /* initialize our event node */

    wdbEventNodeInit (&pChannel->evtNode, tsfsEventGet, tsfsEventDeq,
		      pChannel);

    /* Now format up an event. */

    pChannel->info.info.channel = pChannel->channel;
    pChannel->info.info.opcode = WDB_TSFS_DELETE;
    pChannel->info.info.semId = (TGT_ADDR_T) &pChannel->waitSem;
    pChannel->info.info.pResult = (TGT_ADDR_T) &pChannel->result.value;
    pChannel->info.info.pErrno = (TGT_ADDR_T) &pChannel->result.errNo;
    pChannel->info.info.semGiveAddr = (TGT_ADDR_T) &semGive;
    pChannel->info.extra.remove.filename = (TGT_ADDR_T) name;
    pChannel->info.extra.remove.fnameLen = strlen (name);

    /* Post the event. */

    wdbEventPost (&pChannel->evtNode);

    /* Wait for the tgtsvr to get back to us with the result. */

    semTake (&pChannel->waitSem, WAIT_FOREVER);

    result = pChannel->result.value;
    errno = tsfsErrnoMap (pChannel->result.errNo);

    /* discard the temporary channel */

    semTerminate (&pChannel->waitSem);
    free (pChannel);

    return result;
    }

/******************************************************************************
*
* tsfsClose - close a TSFS file
*
* RETURNS: status
* NOMANUAL
*/ 

static STATUS tsfsClose
    (
    TSFS_CHANNEL_DESC *	pChannel	/* channel handle */
    )
    {
    int			result;

    semTake (&pChannel->tsfsSem, WAIT_FOREVER);

    /* prepare the event for transmission */

    pChannel->info.info.channel = pChannel->channel;
    pChannel->info.info.opcode = WDB_TSFS_CLOSE;
    pChannel->info.info.semId = (TGT_ADDR_T) &pChannel->waitSem;
    pChannel->info.info.pResult = (TGT_ADDR_T) &pChannel->result.value;
    pChannel->info.info.pErrno = (TGT_ADDR_T) &pChannel->result.errNo;
    pChannel->info.info.semGiveAddr = (TGT_ADDR_T) &semGive;

    /* Post the event. */

    wdbEventPost (&pChannel->evtNode);

    /* Wait for the tgtsvr to get back to us with the result. */

    semTake (&pChannel->waitSem, WAIT_FOREVER);

    if (pChannel->result.errNo != OK)
	errno = tsfsErrnoMap (pChannel->result.errNo);

    /* XXX - reexamine this for reentrancy issues */

    result = pChannel->result.value;
    semGive (&pChannel->tsfsSem);

    semTerminate (&pChannel->tsfsSem);
    semTerminate (&pChannel->waitSem);
    
    free (pChannel);
    return result;
    }

/*******************************************************************************
*
* tsfsRead - read from a TSFS file
*
* RETURNS: the number of bytes read if successful, or ERROR with errno set 
*          to indicate the error.
* NOMANUAL
*/

static int tsfsRead
    (
    TSFS_CHANNEL_DESC *	pChannel,	/* channel handle */
    char *		buf,		/* buffer to receive data */
    int			maxBytes	/* max bytes to transfer */
    )

    {
    int			result;

    semTake (&pChannel->tsfsSem, WAIT_FOREVER);

    /* prepare the event for transmission */

    pChannel->info.info.channel = pChannel->channel;
    pChannel->info.info.opcode = WDB_TSFS_READ;
    pChannel->info.info.semId = (TGT_ADDR_T) &pChannel->waitSem;
    pChannel->info.info.pResult = (TGT_ADDR_T) &pChannel->result.value;
    pChannel->info.info.pErrno = (TGT_ADDR_T) &pChannel->result.errNo;
    pChannel->info.info.semGiveAddr = (TGT_ADDR_T) &semGive;
    pChannel->info.extra.rw.buffer = (TGT_ADDR_T) buf;
    pChannel->info.extra.rw.nBytes = maxBytes;

    /* Post the event. */

    wdbEventPost (&pChannel->evtNode);

    /* Wait for the tgtsvr to get back to us with the result. */

    semTake (&pChannel->waitSem, WAIT_FOREVER);

    if (pChannel->result.errNo != OK)
	errno = tsfsErrnoMap (pChannel->result.errNo);

    result = pChannel->result.value;
    semGive (&pChannel->tsfsSem);
    return result;
    }

/*******************************************************************************
* tsfsWrite - write to a TSFS file
*
* RETURNS:  the number of bytes written if successful, or ERROR with errno 
*           set to indicate the error.
* NOMANUAL
*/

static int tsfsWrite
    (
    TSFS_CHANNEL_DESC *	pChannel,	/* channel handle */
    char *		buf,		/* buffer of data to be sent */
    int			maxBytes	/* max bytes to transfer */
    )

    {
    int			result;

    semTake (&pChannel->tsfsSem, WAIT_FOREVER);

    /* prepare the event for transmission */

    pChannel->info.info.channel = pChannel->channel;
    pChannel->info.info.opcode = WDB_TSFS_WRITE;
    pChannel->info.info.semId = (TGT_ADDR_T) &pChannel->waitSem;
    pChannel->info.info.pResult = (TGT_ADDR_T) &pChannel->result.value;
    pChannel->info.info.pErrno = (TGT_ADDR_T) &pChannel->result.errNo;
    pChannel->info.info.semGiveAddr = (TGT_ADDR_T) &semGive;
    pChannel->info.extra.rw.buffer = (TGT_ADDR_T) buf;
    pChannel->info.extra.rw.nBytes = maxBytes;

    /* Post the event. */

    wdbEventPost (&pChannel->evtNode);

    /* Wait for the tgtsvr to get back to us with the result. */

    semTake (&pChannel->waitSem, WAIT_FOREVER);

    if (pChannel->result.errNo != OK)
	errno = tsfsErrnoMap (pChannel->result.errNo);

    result = pChannel->result.value;
    semGive (&pChannel->tsfsSem);
    return result;
    }

/*******************************************************************************
* tsfsIoctl - special device control
*
* RETURNS: status
* NOMANUAL
*/

static STATUS tsfsIoctl
    (
    TSFS_CHANNEL_DESC *	pChannel,	/* device to control */
    int 		request,	/* request code */
    int 		arg		/* some argument */
    )

    {
    int			result;
    DIR *		pDir = (DIR *) arg;	/* For FIOREADDIR Only */
    int			pNewChannel;		/* for FIOMKDIR */
    char *		pShortPath;		/* for same */

    semTake (&pChannel->tsfsSem, WAIT_FOREVER);

    /* map between VxWorks ioctl codes and supported TSFS codes */

    switch (request)
	{
	case FIOSEEK:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_FIOSEEK;
	    break;

        case FIOWHERE:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_FIOWHERE;
	    break;

	case FIONREAD:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_FIONREAD;
	    break;
	    
	case FIOFSTATGET:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_FIOFSTATGET;
	    break;

        case SO_SNDURGB:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_SO_SNDURGB;
	    break;

        case SO_SETDEBUG:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_SO_SETDEBUG;
	    break;

        case SO_GETDEBUG:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_SO_GETDEBUG;
	    break;

        case SO_SETSNDBUF:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_SO_SETSNDBUF;
	    break;

        case SO_SETRCVBUF:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_SO_SETRCVBUF;
	    break;

        case SO_SETDONTROUTE:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_SO_SETDONTROUTE;
	    break;

        case SO_GETDONTROUTE:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_SO_GETDONTROUTE;
	    break;

        case SO_SETOOBINLINE:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_SO_SETOOBINLINE;
	    break;

        case SO_GETOOBINLINE:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_SO_GETOOBINLINE;
	    break;

	case FIOREADDIR:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_FIOREADDIR;

	    /* 
	     * For FIOREADDIR we want to provide the address of the 
	     * struct dirent * and the dd_cookie. 
 	     */
	    
	    pChannel->result.extra1 = pDir->dd_cookie;
	    pChannel->result.extra2 = (TGT_INT_T) &pDir->dd_dirent;
	    break;

        case FIORENAME:
	    pChannel->info.extra.ioctl.request = WDB_TSFS_IOCTL_FIORENAME;

	    /* 
	     * If the new name starts with the device name, strip the
	     * device name first.  If it doesn't just continue.
	     */

            if ((pShortPath = strstr ((char *) arg, 
				      tsfsDev.devHdr.name)) != NULL)
		{
		/* 
		 * If the device name is in the path, but not the first 
		 * element of the path, use the whole path.  Otherwise
		 * remove the device name.  For example, /tgtsvr/foo/goo
		 * should yield /foo/goo, but /foo/tgtsvr/goo should 
		 * remain /foo/tgtsvr/goo (/tgtsvr is the dev name).
		 */

                if (pShortPath != (char *) arg)
		    pShortPath = (char *) arg;
                else
                    pShortPath = (char *) arg + strlen (tsfsDev.devHdr.name);

		}
            else
		{
                pShortPath = (char *) arg;
		}

	    /* 
	     * We put the length of the new name in extra1, and the address
	     * of the new name in extra2.
	     */

	    pChannel->result.extra1 = strlen (pShortPath);
	    pChannel->result.extra2 = (TGT_INT_T) pShortPath;
	    break;

	case FIOMKDIR:

	    /*
	     * We handle this by using our own tsfsOpen to do the creation.
	     * The device name must prefix the path or we generate an error.
	     * Start by stripping off the device name.
	     */
             
            if ((pShortPath = strstr ((char *) arg, 
				      tsfsDev.devHdr.name)) == NULL)
		{
	        errno = tsfsErrnoMap (WDB_ERR_INVALID_PARAMS);
	        semGive (&pChannel->tsfsSem);
		return (ERROR);
		}

            /* The device must be the first thing in the path. */

            if (pShortPath != (char *) arg)
		{
	        errno = tsfsErrnoMap (WDB_ERR_INVALID_PARAMS);
	        semGive (&pChannel->tsfsSem);
		return (ERROR);
		}

            pShortPath = (char *) arg + strlen (tsfsDev.devHdr.name);

            /* 
	     * Now we have the proper path, use our own tsfsOpen to create 
	     * the directory.  The tsfsDev arg to tsfsOpen isn't used by 
	     * tsfsOpen.
             */

	    if ((pNewChannel = 
		 tsfsOpen (& tsfsDev, pShortPath, O_CREAT | O_RDONLY, 
			   FSTAT_DIR | wdbTsfsDefaultDirPerm)) == ERROR)
                {
	        semGive (&pChannel->tsfsSem);
	        return ERROR;
		}

            /* Close the new channel and return. */

	    if (tsfsClose ((TSFS_CHANNEL_DESC *) pNewChannel) != OK)
		{
	        semGive (&pChannel->tsfsSem);
	        return ERROR;
		}

            semGive (&pChannel->tsfsSem);
	    return (OK);

	case FIORMDIR:
	    {
	    struct stat 	statBuf;

	    statBuf.st_mode = 0;

	    /*
	     * We handle this using our own tsfsDelete to remove the 
	     * directory.  The device name must prefix the path or we 
	     * generate an error.  Start by stripping off the device name.
	     */
             
            if ((pShortPath = strstr ((char *) arg, 
				      tsfsDev.devHdr.name)) == NULL)
		{
	        errno = tsfsErrnoMap (WDB_ERR_INVALID_PARAMS);
	        semGive (&pChannel->tsfsSem);
		return (ERROR);
		}

            /* The device must be the first thing in the path. */

            if (pShortPath != (char *) arg)
		{
	        errno = tsfsErrnoMap (WDB_ERR_INVALID_PARAMS);
	        semGive (&pChannel->tsfsSem);
		return (ERROR);
		}

            pShortPath = (char *) arg + strlen (tsfsDev.devHdr.name);

            /* 
	     * Now we have the proper path, but we have to ensure the file
	     * is a directory before we can remove it.  Use our own tsfsIoctl
	     * with FIOSTATGET to find out if it is.  We open the path, test
	     * if it points to a directory, then close it.  Then we delete it.
	     */

	    pNewChannel = tsfsOpen (& tsfsDev, pShortPath, O_RDONLY, 
			            wdbTsfsDefaultDirPerm);

            tsfsIoctl ((TSFS_CHANNEL_DESC *) pNewChannel, FIOFSTATGET, 
		       (int) &statBuf);
	    tsfsClose ((TSFS_CHANNEL_DESC *) pNewChannel);

            if (! (statBuf.st_mode & S_IFDIR))
		{
		errno = tsfsErrnoMap (ENOTDIR);
	        semGive (&pChannel->tsfsSem);
		return (ERROR);
		}
	     
	    /* 
	     * We are sure the file is a directory, so we use tsfsDelete to
	     * remove the directory.  The tsfsDev arg to tsfsDelete isn't 
	     * used by tsfsDelete.  Try to return the most useful errno
	     * by making sure errno records only the errors in tsfsDelete.
             */
           
	    errno = 0;
            if (tsfsDelete (& tsfsDev, pShortPath) != OK)
		{
	        semGive (&pChannel->tsfsSem);
		return (ERROR);
		}

	    semGive (&pChannel->tsfsSem);
	    return (OK);
            }

	case FIOGETFL:

	    /* this one is handled locally. */

	    *((int *) arg) = pChannel->openmode;
	    semGive (&pChannel->tsfsSem);
	    return (OK);
	    
	/* don't pass unknown ioctl's up to target server. */

	default:
	    errno = tsfsErrnoMap (WDB_ERR_INVALID_PARAMS);
	    semGive (&pChannel->tsfsSem);
	    return ERROR;
	}

    /* prepare the event for transmission */

    pChannel->info.info.channel = pChannel->channel;
    pChannel->info.info.opcode = WDB_TSFS_IOCTL;
    pChannel->info.info.semId = (TGT_ADDR_T) &pChannel->waitSem;
    pChannel->info.info.pResult = (TGT_ADDR_T) &pChannel->result.value;
    pChannel->info.info.pErrno = (TGT_ADDR_T) &pChannel->result.errNo;
    pChannel->info.info.semGiveAddr = (TGT_ADDR_T) &semGive;

    pChannel->info.extra.ioctl.arg = arg;
    pChannel->info.extra.ioctl.pExtra1 = (TGT_ADDR_T) &pChannel->result.extra1;
    pChannel->info.extra.ioctl.pExtra2 = (TGT_ADDR_T) &pChannel->result.extra2;

    /* Post the event. */

    wdbEventPost (&pChannel->evtNode);

    /* Wait for the tgtsvr to get back to us with the result. */

    semTake (&pChannel->waitSem, WAIT_FOREVER);

    if (pChannel->result.errNo != OK)
	errno = tsfsErrnoMap (pChannel->result.errNo);

    /* 
     * Some ioctl() requests require us to fill in data pointed to by arg.
     * If so we take care of that here.
     */

    switch (request)
	{
	case FIOFSTATGET:
	    {
	    struct stat *	pStat = (struct stat *) arg;

	    memset (pStat, 0, sizeof (struct stat));
	    pStat->st_dev = (int) &tsfsDev;
	    pStat->st_size = pChannel->result.extra1;

	    if (pChannel->result.extra2 & WDB_TSFS_S_IFREG)
		pStat->st_mode |= S_IFREG;
	    if (pChannel->result.extra2 & WDB_TSFS_S_IFDIR)
		pStat->st_mode |= S_IFDIR;
	    break;
	    }

	case FIONREAD:
	    {
	    * ((int *) arg) = pChannel->result.extra1;
	    break;
	    }

	case FIOREADDIR:
	    {
	    pDir->dd_cookie = pChannel->result.extra1;
	    break;
	    }

	case SO_GETDEBUG:
	case SO_GETDONTROUTE:
	case SO_GETOOBINLINE:
	    {
	    * ((int *) arg) = pChannel->result.extra1;
	    break;
	    }


	}

    result = pChannel->result.value;
    semGive (&pChannel->tsfsSem);
    return result;
    }

/******************************************************************************
*
* tsfsErrnoMap - map errnos returned from host to vxWorks' errnos
*
* This routine maps from the portable errno values defined in wdb.h for
* the TSFS, to errnos defined on the target.  All POSIX errnos are passed
* through this mapping without change.  WDB-specific errnos that have been
* returned from the host are also not mapped.
*
* INTERNAL
* Errnos that are not defined as portable.  This includes POSIX errnos, but
* is may include more if some get through the cracks.
*
* RETURNS:  vxWorks' errno, or zero (0) if passed zero (0) 
*         
* NOMANUAL
*/

static int tsfsErrnoMap
    (
    int hostErrno		/* non-vxWorks errno returned from host */
    )
    {
    int wverr;

    /*
     * By default pass on the error because it may be a POSIX errno.  It may
     * also be zero, so it shouldn't get modified.  Unknown errnos will pass
     * right through.  There should be no default case in the switch below.
     */

    wverr = hostErrno;

    switch (hostErrno)
	{
	case  WDB_TSFS_ERRNO_ENOTEMPTY:       	wverr = ENOTEMPTY;	break;
	case  WDB_TSFS_ERRNO_EDEADLK:         	wverr = EDEADLK;	break;
	case  WDB_TSFS_ERRNO_ENOLCK:          	wverr = ENOLCK;		break;
	case  WDB_TSFS_ERRNO_EMSGSIZE:        	wverr = EMSGSIZE;	break;
	case  WDB_TSFS_ERRNO_EOPNOTSUPP:      	wverr = EOPNOTSUPP;	break;
	case  WDB_TSFS_ERRNO_EADDRNOTAVAIL:   	wverr = EADDRNOTAVAIL;	break;
	case  WDB_TSFS_ERRNO_ENOTSOCK:        	wverr = ENOTSOCK;	break;
	case  WDB_TSFS_ERRNO_ENETRESET:       	wverr = ENETRESET;	break;
	case  WDB_TSFS_ERRNO_ECONNABORTED:    	wverr = ECONNABORTED;	break;
	case  WDB_TSFS_ERRNO_ECONNRESET:      	wverr = ECONNRESET;	break;
	case  WDB_TSFS_ERRNO_ECONNREFUSED:     	wverr = ECONNREFUSED;	break;
	case  WDB_TSFS_ERRNO_ENOBUFS:         	wverr = ENOBUFS;	break;
	case  WDB_TSFS_ERRNO_ENOTCONN:        	wverr = ENOTCONN;	break;
	case  WDB_TSFS_ERRNO_ESHUTDOWN:       	wverr = ESHUTDOWN;	break;
	case  WDB_TSFS_ERRNO_ETIMEDOUT:       	wverr = ETIMEDOUT;	break;
	case  WDB_TSFS_ERRNO_EINPROGRESS:     	wverr = EINPROGRESS;	break;
	case  WDB_TSFS_ERRNO_EWOULDBLOCK:      	wverr = EWOULDBLOCK;	break;
	case  WDB_TSFS_ERRNO_ENOSR:           	wverr = ENOSR;		break;
	case  WDB_TSFS_ERRNO_ELOOP:           	wverr = ELOOP;		break;
	case  WDB_TSFS_ERRNO_ENAMETOOLONG:    	wverr = ENAMETOOLONG;	break;
	case  WDB_TSFS_ERRNO_EBADMSG:         	wverr = EBADMSG;	break;
	}

    return (wverr);
    }

/******************************************************************************
*
* tsfsEventGet - fill in WDB event parameters at transmission time
* 
* RETURNS: N/A
* NOMANUAL
*/

static void tsfsEventGet
    (
    void *		arg,
    WDB_EVT_DATA * 	pEvtData
    )
    {
    TSFS_CHANNEL_DESC * pChannel = (TSFS_CHANNEL_DESC *) arg;
    int			size;

    size = sizeof (WDB_TSFS_GENERIC_INFO);

    switch (pChannel->info.info.opcode)
	{
	case WDB_TSFS_OPEN:
	    size += sizeof (WDB_TSFS_OPEN_INFO);
	    break;

	case WDB_TSFS_DELETE:
	    size += sizeof (WDB_TSFS_DELETE_INFO);
	    break;

	case WDB_TSFS_WRITE:
	case WDB_TSFS_READ:
	    size += sizeof (WDB_TSFS_RW_INFO);
	    break;

	case WDB_TSFS_IOCTL:
	    size += sizeof (WDB_TSFS_IOCTL_INFO);
	    break;

	case WDB_TSFS_CLOSE:
	default:
	    break;
	};

    memcpy (&pEvtData->eventInfo, &pChannel->info, size);

    pEvtData->evtType = WDB_EVT_TSFS_OP;
    pEvtData->eventInfo.evtInfo.numInts = size / sizeof (TGT_INT_T) - 1;
    }

/******************************************************************************
*
* tsfsEventDeq - free allocated event data after transmission
*
* NOMANUAL
*/

static void tsfsEventDeq
    (
    void *		arg
    )

    {
    /* The tsfs event has no allocated data or other need for cleanup. */

    return;
    }




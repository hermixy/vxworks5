/* rt11FsLib.c - RT-11 media-compatible file system library */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
06n,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
06m,29mar95,kdl  removed obsolete date fields from stat structure.
06l,14sep93,kdl  fixed ignoring of driver error during rt11FsWrite (SPR #2516).
06k,03sep93,kdl  fixed use of standard skew & interleave (rtFmt) (SPR #959).
06j,25aug93,jmm  rt11FsFileStatGet() now sets atime, mtime, and ctime fields
06i,24jun93,kdl  fixed loss of fd if driver status check returns error during 
		 open() (SPR #2278).
06h,02feb93,jdi  documentation tweaks.
06g,02feb93,jdi  documentation cleanup for 5.1; added 3rd param to ioctl() examples.
06f,02oct92,srh  added ioctl(FIOGETFL) to return file's open mode
06e,07sep92,smb  added blksize and blocks to the stat structure initialisation.
06d,22jul92,kdl  changed from 4.0.2-style semaphores to mutexes.
06c,18jul92,smb  Changed errno.h to errnoLib.h.
06b,26may92,rrr  the tree shuffle
06a,13dec91,gae  ANSI cleanup.
05z,19nov91,rrr  shut up some ansi warnings.
05y,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY and ...
		  -changed VOID to void
		  -changed copyright notice
05x,26jun91,wmd  modified rt11FsFindEntry() to use field comparisons instead of bcmp().
05w,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by kdl.
05v,22feb91,jaa	 documentation cleanup.
05u,08oct90,kdl  added forward declaration for rt11FsOpen().
05t,08oct90,kdl  made rt11FsOpen, rt11FsWrite, etc. routines LOCAL (again).
05s,10aug90,kdl  added forward declarations for functions returning void.
05r,09aug90,kdl  changed name from rt11Lib to rt11FsLib.
05q,19jul90,dnw  mangen fix
05p,15jul90,dnw  moved rt11Ls back to usrLib as oldLs since it is applicable
		   to the netDrv as well
05o,12jul90,kdl  added BLK_DEV statusChk routine hook.
05n,26jun90,jcf  changed semTake () calling sequeuece for 5.0.
05m,07may90,kdl  made rt11Read read all requested bytes, added bd_readyChanged
		 and bd_mode in BLK_DEV, added POSIX-style directory listing,
		 moved old "ls" routine from usrLib and renamed it "rt11Ls",
		 made rt11Create and rt11FindEntry ignore leading slashes
		 in filename.
            jcc	 rt11Squeeze now does multi-sector reads and writes.
05l,23mar90,kdl	 lint, changed BLK_DEV field names.
05k,22mar90,kdl	 made rt11Read, rt11Write, etc. global again (temporary).
05j,16mar90,kdl	 added support for multi-filesystem device drivers (BLK_DEV),
		 added direct multi-block transfer to/from user's buffer.
05i,09oct89,dab  fixed stripping of O_CREAT and O_TRUNC from file mode in
		   rt11Open().
05h,04aug89,dab  fixed bug introduced in 05g that didn't free extra fd.
05g,21jun89,dab  made rt11Open() check for O_CREAT and O_TRUNC modes.
05f,07jul88,jcf  fixed mallocs to match new declaration.
05e,05jun88,dnw  changed from rtLib to rt11Lib.
05d,30may88,dnw  changed to v4 names.
05c,04may88,jcf  changed semaphores and sem calls for new semLib.
05b,09nov87,ecs  removed intemperate printErr from rtCreate.
05a,03nov87,ecs  documentation.
04z,20oct87,gae  made rtRename() cancel device portion of name, if present.
		 made rtDirEntry() return name for DES_TENTATIVE.
		 fixed bug in create() by flushing directory;
		 made create fail if create already active on device.
		 made rtSqueeze local.
04y,14sep87,dpg  moved squeeze to usrLib.c. changed FIOSQUEEZE call in rtIoctl
		 to use a fd rather than some aesoteric device pointer (i.e.
		 keep it looking the same).
04x,10sep87,dpg  deleted rtMaxRoom & maxRoom - we don't really need them now
		 renamed rtPack & pack to rtSqueeze & squeeze
04w,26aug87,dpg  added rtMaxRoom to find largest piece of free space on
		 a disk device, and a FIOMAXROOM to ioctl at it; together
		 with maxRoom - a user callable routine
04v,26jun87,dpg  added rtPack to compress free space on an RT-11 device into
		 one big block so that hard disks don't get too fragmented.
		 also added ioctl entry to provide uniform interface to
		 rtPack & pack (user callable routine)
04u,05may87,gae  fixed "the other" fatal bug introduced in 04t in rtRead().
		 changed constant N_RETRIES to rtMaxRetries to allow variation.
04u,28apr87,gae  fixed fatal bug introduced in 04t in rtVolMount().
04t,26mar87,gae  Made rtVolMount() fail on bad (unlikely) directory.
		 Got rid of rtVolFlush(), then renamed rtVFlush() to
		   rtVolFlush().
04s,07mar87,gae  Made the following LOCAL: rtGetEntry(), rtAbsSector(),
		   rtDirEntry(), rtRename(), rtFlush(), rtWhere(), rtSeek(),
		   rtVolFlush(), and rtVolInit().  Fixed documentation.
		 Made rtDevInit() return ERROR if out of memory.
04r,14feb87,dnw  Changed to accept null filename as special name for "raw disk"
		   instead of "-", now that ioLib protects against accidental
		   null filenames.
04q,03feb87,llk  Modified directory segments to handle any number of file
		   entries.  Directories are 1 segment long, with at least
		   2 blocks per segment.  2 block segments are still RT-11
		   compatible.  rtDevInit has a new parameter, the maximum
		   number of file entries that will go on the device.
		   See documentation below under RT-11 COMPATIBILITY.
		 Added FIODISKCHANGE to ioctl function codes.  Used for
		   manually issuing a ready change.
		 Fixed rtDirEntry so that it checks if the entry number passed
		   to it is less than the maximum possible entry number.
04p,14jan87,llk  changed to handle file size up to RT_MAX_BLOCKS_PER_FILE.
		 Changed rtVolInit to initialize disk with as many maximum
		 sized files as needed to cover all the disk space.
		 Changed rtCoalesce to coalesce files together iff the new
		 empty file would have less than RT_MAX_BLOCKS_PER_FILE blocks.
...deleted pre 87 history - see RCS
*/

/*
This library provides services for file-oriented device drivers which use
the RT-11 file standard.  This module takes care of all the necessary 
buffering, directory maintenance, and RT-11-specific details.

USING THIS LIBRARY
The various routines provided by the VxWorks RT-11 file system (rt11Fs) may
be separated into three broad groups:  general initialization,
device initialization, and file system operation.

The rt11FsInit() routine is the principal initialization function;
it need only be called once, regardless of how many rt11Fs devices
will be used.

Other rt11Fs routines are used for device initialization.  For
each rt11Fs device, either rt11FsDevInit() or rt11FsMkfs() must be called
to install the device and define its configuration.

Several functions are provided to inform the file system of
changes in the system environment.  The rt11FsDateSet() routine is used
to set the date.  The rt11FsModeChange() routine is used to
modify the readability or writability of a particular device.  The
rt11FsReadyChange() routine is used to inform the file system that a
disk may have been swapped, and that the next disk operation should first
remount the disk.

INITIALIZING RT11FSLIB
Before any other routines in rt11FsLib can be used, rt11FsInit() must 
be called to initialize this library.  This call specifies 
the maximum number of rt11Fs files that can be open simultaneously 
and allocates memory for that many rt11Fs file descriptors.
Attempts to open more files than the specified maximum will result
in errors from open() or creat().

This initialization is enabled when the configuration macro
INCLUDE_RT11FS is defined.

DEFINING AN RT-11 DEVICE
To use this library for a particular device, the device structure must
contain, as the very first item, a BLK_DEV structure.  This must be
initialized before calling rt11FsDevInit().  In the BLK_DEV structure, the
driver includes the addresses of five routines which it must supply:  one
that reads one or more sectors, one that writes one or more sectors, one
that performs I/O control on the device (using ioctl()), one that checks the
status of the device, and one that resets the device.  This structure also
specifies various physical aspects of the device (e.g., number of sectors,
sectors per track, whether the media is removable).  For more information about
defining block devices, see the
.I "VxWorks Programmer's Guide: I/O System."

The device is associated with the rt11Fs file system by the
rt11FsDevInit() call.  The arguments to rt11FsDevInit() include the name
to be used for the rt11Fs volume, a pointer to the BLK_DEV structure,
whether the device uses RT-11 standard skew and interleave, and the
maximum number of files that can be contained in the device directory.

Thereafter, when the file system receives a request from the I/O system, it
simply calls the provided routines in the device driver to fulfill the
request.

RTFMT
The RT-11 standard defines a peculiar software interleave and
track-to-track skew as part of the format.  The <rtFmt> parameter passed
to rt11FsDevInit() should be TRUE if this formatting is desired.  This
should be the case if strict RT-11 compatibility is desired, or if files
must be transferred between the development and target machines using the
VxWorks-supplied RT-11 tools.  Software interleave and skew will
automatically be dealt with by rt11FsLib.

When <rtFmt> has been passed as TRUE and the maximum number of files is
specified RT_FILES_FOR_2_BLOCK_SEG, the driver does not need to do
anything else to maintain RT-11 compatibility (except to add the track 
offset as described above).

Note that if the number of files specified is different than
RT_FILES_FOR_2_BLOCK_SEG under either a VxWorks system or an RT-11 system,
compatibility is lost because VxWorks allocates a contiguous directory,
whereas RT-11 systems create chained directories.

MULTIPLE LOGICAL DEVICES AND RT-11 COMPATIBILITY
The sector number passed to the sector read and write routines is an
absolute number, starting from sector 0 at the beginning of the device.
If desired, the driver may add an offset from the beginning of the
physical device before the start of the logical device.  This would
normally be done by keeping an offset parameter in the device-specific
structure of the driver, and adding the proper number of sectors to the
sector number passed to the read and write routines.

The RT-11 standard defines the disk to start on track 1.  Track 0 is set
aside for boot information.  Therefore, in order to retain true
compatibility with RT-11 systems, a one-track offset (i.e., the number of
sectors in one track) needs to be added to the sector numbers passed to
the sector read and write routines, and the device size needs to be
declared as one track smaller than it actually is.  This must be done by
the driver using rt11FsLib; the library does not add such an offset
automatically.

In the VxWorks RT-11 implementation, the directory is a fixed size, able
to contain at least as many files as specified in the call to
rt11FsDevInit().  If the maximum number of files is specified to be
RT_FILES_FOR_2_BLOCK_SEG, strict RT-11 compatibility is maintained,
because this is the initial allocation in the RT-11 standard.

RT-11 FILE NAMES
File names in the RT-11 file system use six characters, followed
by a period (.), followed by an optional three-character extension.

DIRECTORY ENTRIES
An ioctl() call with the FIODIRENTRY function returns information about a
particular directory entry.  A pointer to a REQ_DIR_ENTRY structure is
passed as the parameter.  The field `entryNum' in the REQ_DIR_ENTRY
structure must be set to the desired entry number.  The name of the file,
its size (in bytes), and its creation date are returned in the structure.
If the specified entry is empty (i.e., if it represents an unallocated
section of the disk), the name will be an empty string, the size will be
the size of the available disk section, and the date will be meaningless.
Typically, the entries are accessed sequentially, starting with `entryNum' = 0,
until the terminating entry is reached, indicated by a return code of ERROR.

DIRECTORIES IN MEMORY
A copy of the directory for each volume is kept in memory (in the RT_VOL_DESC
structure).  This speeds up directory accesses, but requires that rt11FsLib
be notified when disks are changed (i.e., floppies are swapped).  If the
driver can find this out (by interrogating controller status or by
receiving an interrupt), the driver simply calls rt11FsReadyChange() when a
disk is inserted or removed.  The library rt11FsLib will automatically try
to remount the device next time it needs it.

If the driver does not have access to the information that disk volumes
have been changed, the <changeNoWarn> parameter should be set to TRUE
when the device is defined using rt11FsDevInit().  This will cause the
disk to be automatically remounted before each open(), creat(), delete(),
and directory listing.

The routine rt11FsReadyChange() can also be called by user tasks, by
issuing an ioctl() call with FIODISKCHANGE as the function code.

ACCESSING THE RAW DISK
As a special case in open() and creat() calls, rt11FsLib recognizes a NULL
file name to indicate access to the entire "raw" disk, as opposed to a
file on the disk.  Access in raw mode is useful for a disk that has no
file system.  For example, to initialize a new file system on the disk,
use an ioctl() call with FIODISKINIT.  To read the directory of a disk for
which no file names are known, open the raw disk and use an ioctl() call
with the function FIODIRENTRY.

HINTS
The RT-11 file system is much simpler than the more common UNIX or MS-DOS
file systems.  The advantage of RT-11 is its speed; file access is made in
at most one seek because all files are contiguous.  Some of the most common
errors for users with a UNIX background are:
.iP "" 4
Only a single create at a time may be active per device.
.iP
File size is set by the first create and close sequence;
use lseek() to ensure a specific file size;
there is no append function to expand a file.
.iP
Files are strictly block oriented; unused portions
of a block are filled with NULLs -- there is no
end-of-file marker other than the last block.
.LP

IOCTL FUNCTIONS
The rt11Fs file system supports the following ioctl() functions.
The functions listed are defined in the header ioLib.h.  Unless stated
otherwise, the file descriptor used for these functions can be any file
descriptor open to a file or to the volume itself.

.iP "FIODISKFORMAT" 16 3
Formats the entire disk with appropriate hardware track and sector marks.
No file system is initialized on the disk by this request.
Note that this is a driver-provided function:
.CS
    fd = open ("DEV1:", O_WRONLY);
    status = ioctl (fd, FIODISKFORMAT, 0);
.CE
.iP "FIODISKINIT"
Initializes an rt11Fs file system on the disk volume.
This routine does not format the disk; formatting must be done by the driver.
The file descriptor should be obtained by opening the entire volume in raw mode:
.CS
    fd = open ("DEV1:", O_WRONLY);
    status = ioctl (fd, FIODISKINIT, 0);
.CE
.iP "FIODISKCHANGE"
Announces a media change.  It performs the same function as rt11FsReadyChange().
This function may be called from interrupt level:
.CS
    status = ioctl (fd, FIODISKCHANGE, 0);
.CE
.iP "FIOGETNAME"
Gets the file name of the file descriptor and copies it to the buffer <nameBuf>:
.CS
    status = ioctl (fd, FIOGETNAME, &nameBuf);
.CE
.iP "FIORENAME"
Renames the file to the string <newname>:
.CS
    status = ioctl (fd, FIORENAME, "newname");
.CE
.iP "FIONREAD"
Copies to <unreadCount> the number of unread bytes in the file:
.CS
    status = ioctl (fd, FIONREAD, &unreadCount);
.CE
.iP "FIOFLUSH"
Flushes the file output buffer.  It guarantees that any output that has been
requested is actually written to the device.
.CS
    status = ioctl (fd, FIOFLUSH, 0);
.CE
.iP "FIOSEEK"
Sets the current byte offset in the file to the position specified by
<newOffset>:
.CS
    status = ioctl (fd, FIOSEEK, newOffset);
.CE
.iP "FIOWHERE"
Returns the current byte position in the file.  This is the byte offset of
the next byte to be read or written.  It takes no additional argument:
.CS
    position = ioctl (fd, FIOWHERE, 0);
.CE
.iP "FIOSQUEEZE"
Coalesces fragmented free space on an rt11Fs volume:
.CS
    status = ioctl (fd, FIOSQUEEZE, 0);
.CE
.iP "FIODIRENTRY"
Copies information about the specified directory entries to a
\%REQ_DIR_ENTRY structure that is defined in ioLib.h.  The argument <req>
is a pointer to a \%REQ_DIR_ENTRY structure.  On entry, the structure
contains the number of the directory entry for which information is
requested.  On return, the structure contains the information on the
requested entry.  For example, after the following:
.CS
    REQ_DIR_ENTRY req;
    req.entryNum = 0;
    status = ioctl (fd, FIODIRENTRY, &req);
.CE
the request structure contains the name, size, and creation date of the
file in the first entry (0) of the directory.
.iP "FIOREADDIR"
Reads the next directory entry.  The argument <dirStruct> is a DIR
directory descriptor.  Normally, readdir() is used to read a
directory, rather than using the FIOREADDIR function directly.  See dirLib.
.CS
    DIR dirStruct;
    fd = open ("directory", O_RDONLY);
    status = ioctl (fd, FIOREADDIR, &dirStruct);
.CE
.iP "FIOFSTATGET"
Gets file status information (directory entry data).  The argument
<statStruct> is a pointer to a stat structure that is filled with data
describing the specified file.  Normally, the stat() or fstat() routine
is used to obtain file information, rather than using the FIOFSTATGET
function directly.  See dirLib.
.CS
    struct stat statStruct;
    fd = open ("file", O_RDONLY);
    status = ioctl (fd, FIOFSTATGET, &statStruct);
.CE
.LP
Any other ioctl() function codes are passed to the block device driver
for handling.

INCLUDE FILES: rt11FsLib.h

SEE ALSO: ioLib, iosLib, ramDrv,
.pG "I/O System, Local File Systems"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "ctype.h"
#include "ioLib.h"
#include "semLib.h"
#include "rt11FsLib.h"
#include "blkIo.h"
#include "stdlib.h"
#include "string.h"
#include "dirent.h"
#include "errnoLib.h"
#include "time.h"
#include "sys/stat.h"

#define RT_NAME_LEN	11

#ifndef	WAIT_FOREVER
#define	WAIT_FOREVER    NULL
#endif


/* GLOBALS */

int	rt11FsDrvNum = ERROR;	/* I/O system driver number for RT-11 */

int	rt11FsVolMutexOptions = (SEM_Q_PRIORITY | SEM_DELETE_SAFE);
					/* default mutex options */


/* LOCALS */

LOCAL char rad50 [] = " abcdefghijklmnopqrstuvwxyz$.\3770123456789";

LOCAL RT_FILE_DESC *rt11FsFd;	/* pointer to list of file descriptors */
LOCAL int rt11FsMaxFiles;	/* max RT-11 files that can be open at once */
LOCAL SEM_ID rt11FsFdSemId;	/* interlock file descriptor list access */
LOCAL int rt11FsDay = 0;	/* day of month, default is 0 */
LOCAL int rt11FsMonth = 0;	/* month of year, default is 0 */
LOCAL int rt11FsYear = 72;	/* year, default is '72 (stored as 0) */


/* forward static functions */

static STATUS rt11FsClose (RT_FILE_DESC *pFd);
static void rt11FsCoalesce (RT_VOL_DESC *vdptr, int entryNum);
static RT_FILE_DESC *rt11FsCreate (RT_VOL_DESC *vdptr, char *name, int mode);
static int rt11FsDate (int year, int month, int day);
static STATUS rt11FsDelete (RT_VOL_DESC *vdptr, char *name);
static STATUS rt11FsDirEntry (RT_VOL_DESC *vdptr, REQ_DIR_ENTRY *rdeptr);
static STATUS rt11FsDirRead (RT_FILE_DESC *pFd, DIR *pDir);
static STATUS rt11FsFileStatGet (RT_FILE_DESC *pFd, struct stat *pStat);
static int rt11FsFindEntry (RT_VOL_DESC *vdptr, char *name, int *pstart);
static STATUS rt11FsFlush (RT_FILE_DESC *pFd);
static void rt11FsFreeFd (RT_FILE_DESC *pFd);
static RT_FILE_DESC *rt11FsGetFd (void);
static void rt11FsGetEntry (RT_VOL_DESC *vdptr, int entryNum, RT_DIR_ENTRY
		*pEntry);
static void rt11FsInsEntry (RT_VOL_DESC *vdptr, int entryNum, RT_DIR_ENTRY
		*pEntry);
static STATUS rt11FsIoctl (RT_FILE_DESC *pFd, int function, int arg);
static void rt11FsNameR50 (char *string, RT_NAME *pName);
static int rt11FsR50out (char *string);
static void rt11FsNameString (RT_NAME name, char *string);
static void rt11FsR50in (unsigned int r50, char *string);
static RT_FILE_DESC *rt11FsOpen (RT_VOL_DESC *vdptr, char *name, int mode);
static void rt11FsPutEntry (RT_VOL_DESC *vdptr, int entryNum, RT_DIR_ENTRY
		*pEntry);
static int rt11FsRead (RT_FILE_DESC *pFd, char *pBuf, int maxBytes);
static STATUS rt11FsRename (RT_FILE_DESC *pFd, char *newName);
static STATUS rt11FsSeek (RT_FILE_DESC *pFd, int position);
static STATUS rt11FsVolFlush (RT_VOL_DESC *vdptr);
static STATUS rt11FsVolInit (RT_VOL_DESC *vdptr);
static STATUS rt11FsVolMount (RT_VOL_DESC *vdptr);
static int rt11FsWhere (RT_FILE_DESC *pFd);
static int rt11FsWrite (RT_FILE_DESC *pFd, char *pBuf, int nbytes);
static int rt11FsNewBlock (RT_FILE_DESC *pFd);
static STATUS rt11FsRdBlock (RT_VOL_DESC *vdptr, int blockNum, int numBlocks,
		char *pBuf);
static STATUS rt11FsWrtBlock (RT_VOL_DESC *vdptr, int blockNum, int
		numBlocks, char *pBuf);
static int rt11FsAbsSector (ULONG secPerTrack, int sector);
static STATUS rt11FsCheckVol (RT_VOL_DESC *vdptr, BOOL doMount);
static STATUS rt11FsReset (RT_VOL_DESC *vdptr);
static STATUS rt11FsVolMode (RT_VOL_DESC *vdptr);
static STATUS rt11FsSqueeze (RT_VOL_DESC *vdptr);


/*******************************************************************************
*
* rt11FsClose - close an RT-11 file
*
* This routine closes the specified RT-11 file.
* The end of the buffer beyond the end of file is cleared out,
* the buffer is flushed, and the directory is updated if necessary.
*
* RETURNS:
* OK, or
* ERROR if directory couldn't be flushed or
* entry couldn't be found.
*/

LOCAL STATUS rt11FsClose
    (
    FAST RT_FILE_DESC *pFd      /* file descriptor pointer */
    )
    {
    FAST RT_DIR_ENTRY *pEntry = &pFd->rfd_dir_entry;
    FAST int remaining_blocks;
    FAST int nblocks;
    int buf_index;
    STATUS status;
    char name[RT_NAME_LEN];	/* file name gets unpacked into here */
    int entryNum;		/* file's entry number in directory */
    int start;			/* for receiving rt11FsFindEntry side-effect */

    /* if current buffer has been written to and contains end of file,
     *   clear out end of buffer that lies beyond end of file. */

    if (pFd->rfd_modified && ((pFd->rfd_curptr / RT_BYTES_PER_BLOCK) ==
			      (pFd->rfd_endptr / RT_BYTES_PER_BLOCK)))
	{
	buf_index = pFd->rfd_endptr - pFd->rfd_curptr;

	bzero (&pFd->rfd_buffer[buf_index], RT_BYTES_PER_BLOCK - buf_index);
	}

    status = rt11FsFlush (pFd);

    /* if file is new, update directory */

    if (pEntry->de_status == DES_TENTATIVE)
	{
	/* update directory entry to be permanent with actual size */

	nblocks = ((pFd->rfd_endptr - 1) / RT_BYTES_PER_BLOCK) + 1;
	remaining_blocks = pEntry->de_nblocks - nblocks;

	pEntry->de_status  = DES_PERMANENT;
	pEntry->de_nblocks = nblocks;

	semTake (pFd->rfd_vdptr->vd_semId, WAIT_FOREVER);

	/* unpack name and find entry number */

	rt11FsNameString (pFd->rfd_dir_entry.de_name, name);
	entryNum = rt11FsFindEntry (pFd->rfd_vdptr, name, &start);

	if (entryNum == ERROR)
	    status = ERROR;
	else
	    {
	    rt11FsPutEntry (pFd->rfd_vdptr, entryNum, pEntry);

	    /* if unused blocks are left over, insert EMPTY entry in directory*/

	    if (remaining_blocks != 0)
		{
		pEntry->de_status  = DES_EMPTY;
		pEntry->de_nblocks = remaining_blocks;

		rt11FsInsEntry (pFd->rfd_vdptr, entryNum + 1, pEntry);
		rt11FsCoalesce (pFd->rfd_vdptr, entryNum + 1);
		}

	    /* make sure directory is written out */

	    if (rt11FsVolFlush (pFd->rfd_vdptr) != OK)
		status = ERROR;
	    }

	semGive (pFd->rfd_vdptr->vd_semId);		/* release volume */
	}

    rt11FsFreeFd (pFd);				/* mark fd not in use */
    return (status);
    }

/*******************************************************************************
*
* rt11FsCoalesce - merge empty directory entries
*
* This routine merges a directory entry with its empty neighbors, if any.
* It stops when there are no more empty neighbors, or when adding
* another empty file would cause it to have more than RT_MAX_BLOCKS_PER_FILE
* blocks in an empty file.
* The directory will be updated.
*
* Possession of the volume descriptor's semaphore must have been secured
* before this routine is called.
*/

LOCAL void rt11FsCoalesce
    (
    RT_VOL_DESC *vdptr,     /* pointer to volume descriptor */
    int entryNum            /* number of empty entry to coalesce */
    )
    {
    RT_DIR_ENTRY entry;	    /* 1st of 1-3 empty directory entries */
    RT_DIR_ENTRY nentry;    /* next entry */
    int n = 0;		    /* number of entries to merge into first */

    rt11FsGetEntry (vdptr, --entryNum, &entry);

    if ((entryNum < 0) || (entry.de_status != DES_EMPTY))
	rt11FsGetEntry (vdptr, ++entryNum, &entry);

    rt11FsGetEntry (vdptr, entryNum + 1, &nentry);	/* get next entry */

    /* start coalescing -- don't coalesce 2 files if the new file would
       have more than RT_MAX_BLOCKS_PER_FILE */

    while (nentry.de_status == DES_EMPTY)
	{
    	if ((entry.de_nblocks + nentry.de_nblocks) > RT_MAX_BLOCKS_PER_FILE)
	    {
	    /* These two files can't be merged. The new file would be too big */

	    if (n > 0)		/* don't change entry ptr w.o. coalescing */
		break;
	    else
		{
		/* Can't coalesce current empty entry.  Point to next one */

		entryNum++;
		entry = nentry;
		}
	    }
	else
	    {
	    entry.de_nblocks += nentry.de_nblocks;	/* seize empty blocks */
	    ++n;
	    }
	rt11FsGetEntry (vdptr, entryNum + 1 + n, &nentry);/* get next entry */
	}

    if (n > 0)			/* any entries coalesced? */
	{
	rt11FsPutEntry (vdptr, entryNum, &entry);	/* put empty entry */
	rt11FsPutEntry (vdptr, ++entryNum, &nentry);	/* put nonempty entry */

	/* move rest of entries up by n places */

	while (nentry.de_status != DES_END)
	    {
	    rt11FsGetEntry (vdptr, ++entryNum + n, &nentry);
	    rt11FsPutEntry (vdptr, entryNum, &nentry);
	    }
	}
    }

/*******************************************************************************
*
* rt11FsCreate - create an RT-11 file
*
* This routine creates the file <name> with the specified <mode>.
* If the file already exists, it is first deleted then recreated.
* The largest empty space on the device is allocated to the new file.
* Excess space will be recovered when the file is closed.
* An RT-11 file descriptor is initialized for the file.
*
* A file <name> of zero length (i.e., "" is used to open an entire "raw" disk).
* In this case, no attempt is made to access the disk's directory, so that
* even un-initialized disks may be accessed.
*
* RETURNS
* Pointer to RT-11 file descriptor, or
* ERROR if error in create.
*/

LOCAL RT_FILE_DESC *rt11FsCreate
    (
    RT_VOL_DESC *vdptr, /* pointer to volume descriptor */
    char *name,         /* RT-11 string (ffffff.ttt) */
    int mode            /* file mode (O_RDONLY/O_WRONLY/O_RDWR) */
    )
    {
    RT_DIR_ENTRY entry;
    int start;
    FAST int e_num;
    RT_DIR_ENTRY max_entry;
    int max_start = 0;		/* used only if max_e_num != NONE */
    int max_e_num;		/* maximum entry number */
    FAST int nEntries;		/* number of entries that will fit in segment */
    FAST RT_FILE_DESC *pFd;	/* file descriptor pointer */


    /* Call driver check-status routine, if any */

    if (vdptr->vd_pBlkDev->bd_statusChk != NULL)
	{
	if ((* vdptr->vd_pBlkDev->bd_statusChk) (vdptr->vd_pBlkDev) != OK)
	    {
	    return ((RT_FILE_DESC *) ERROR);
	    }
	}

    /* Set up for re-mount if no disk change notification */

    if (vdptr->vd_changeNoWarn == TRUE)
	rt11FsReadyChange (vdptr);


    pFd = rt11FsGetFd (); 			/* get file descriptor */
    if (pFd == NULL)
	return ((RT_FILE_DESC *) ERROR);	/* no free file descriptors */

    nEntries = (vdptr->vd_nSegBlocks * RT_BYTES_PER_BLOCK
		- (sizeof (RT_DIR_SEG) - sizeof (RT_DIR_ENTRY)))
		/ sizeof (RT_DIR_ENTRY);

    /* check for create of raw device (null filename) */

    if (name [0] == EOS)
	{
	/* check that volume is available */

	semTake (vdptr->vd_semId, WAIT_FOREVER);

	if (rt11FsCheckVol (vdptr, FALSE) != OK)
	    {
	    semGive (vdptr->vd_semId);
	    rt11FsFreeFd (pFd);
	    errnoSet (S_rt11FsLib_VOLUME_NOT_AVAILABLE);
	    return ((RT_FILE_DESC *) ERROR);
	    }

	/* disk must be writable to create file */

	if (rt11FsVolMode (vdptr) == O_RDONLY)
	    {
	    semGive (vdptr->vd_semId);
	    rt11FsFreeFd (pFd);
	    errnoSet (S_ioLib_WRITE_PROTECTED);
	    return ((RT_FILE_DESC *) ERROR);
	    }

	semGive (vdptr->vd_semId);

	/* null name is special indicator for "raw" disk;
	 *   fabricate a bogus directory entry covering entire volume */

	pFd->rfd_dir_entry.de_status  = DES_BOGUS;
	pFd->rfd_dir_entry.de_date    = rt11FsDate (rt11FsYear, rt11FsMonth,
						    rt11FsDay);
	pFd->rfd_dir_entry.de_nblocks = vdptr->vd_nblocks;
	rt11FsNameR50 ("device.raw", &pFd->rfd_dir_entry.de_name);

	pFd->rfd_start	= 0;
	pFd->rfd_endptr	= vdptr->vd_nblocks * RT_BYTES_PER_BLOCK;
	}
    else
	{
    	/* Ignore any leading "/" or "\"'s in file name */

    	while ((*name == '/') || (*name == '\\'))
	    name++;

	/* first delete any existing file with the specified name */

	(void) rt11FsDelete (vdptr, name);

	/* check that volume is available */

	semTake (vdptr->vd_semId, WAIT_FOREVER);

	if ((rt11FsCheckVol (vdptr, TRUE) != OK) || (vdptr->vd_status != OK))
	    {
	    semGive (vdptr->vd_semId);			/* release volume */
	    rt11FsFreeFd (pFd);
	    errnoSet (S_rt11FsLib_VOLUME_NOT_AVAILABLE);
	    return ((RT_FILE_DESC *) ERROR);
	    }

	/* disk must be writable to create file */

	if (rt11FsVolMode (vdptr) == O_RDONLY)
	    {
	    semGive (vdptr->vd_semId);			/* release volume */
	    rt11FsFreeFd (pFd);
	    errnoSet (S_ioLib_WRITE_PROTECTED);
	    return ((RT_FILE_DESC *) ERROR);
	    }

	/* search entire directory for largest empty entry.
	 * keep track of start block by accumulating entry lengths.
	 */

	start = vdptr->vd_dir_seg->ds_start;
	max_e_num = NONE;

	for (e_num = 0; e_num < nEntries; e_num++)
	    {
	    rt11FsGetEntry (vdptr, e_num, &entry);

	    /* if this is the end of the directory, then quit. */

	    if (entry.de_status == DES_END)
		break;

	    /* check if create already in progress */

	    if (entry.de_status == DES_TENTATIVE)
		{
		semGive (vdptr->vd_semId);	/* release volume */
		rt11FsFreeFd (pFd);
		errnoSet (S_rt11FsLib_NO_MORE_FILES_ALLOWED_ON_DISK);
		return ((RT_FILE_DESC *) ERROR);
		}

	    /* check if this is largest empty file so far */

	    if ((entry.de_status == DES_EMPTY) &&
		((max_e_num == NONE) ||
		 (entry.de_nblocks > max_entry.de_nblocks)))
		{
		max_e_num = e_num;
		max_entry = entry;
		max_start = start;
		}

	    /* add file length to get start of next file */

	    start += entry.de_nblocks;
	    }

	/* check for running out of room for file entries
	 * (the last entry is a terminating entry) */

	if (e_num >= nEntries - 1)
	    {
	    semGive (vdptr->vd_semId);			/* release volume */
	    rt11FsFreeFd (pFd);
	    errnoSet (S_rt11FsLib_NO_MORE_FILES_ALLOWED_ON_DISK);
	    return ((RT_FILE_DESC *) ERROR);
	    }

	/* check for no empty holes found */

	if (max_e_num == NONE)
	    {
	    semGive (vdptr->vd_semId);			/* release volume */
	    rt11FsFreeFd (pFd);
	    errnoSet (S_rt11FsLib_DISK_FULL);
	    return ((RT_FILE_DESC *) ERROR);
	    }

	/* found an empty hole; initialize entry */

	max_entry.de_status = DES_TENTATIVE;
	max_entry.de_date   = rt11FsDate (rt11FsYear, rt11FsMonth, rt11FsDay);
	rt11FsNameR50 (name, &max_entry.de_name);

	rt11FsPutEntry (vdptr, max_e_num, &max_entry);

	(void) rt11FsVolFlush (vdptr);

	semGive (vdptr->vd_semId);			/* release volume */

	pFd->rfd_dir_entry = max_entry;
	pFd->rfd_start	   = max_start;
	pFd->rfd_endptr	   = 0;
	}

    /* initialize rest of file descriptor */

    pFd->rfd_mode	= mode;
    pFd->rfd_vdptr	= vdptr;
    pFd->rfd_curptr	= NONE;
    pFd->rfd_newptr	= 0;
    pFd->rfd_modified	= FALSE;

    return (pFd);
    }

/*******************************************************************************
*
* rt11FsDate - generate RT-11 encoded date
*
* This routine encodes the specified date into RT-1 format.
*
* RETURNS: Encoded date.
*/

LOCAL int rt11FsDate
    (
    int year,           /* 72, or 72...03 */
    int month,          /* 0, or 1...12 */
    int day             /* 0, or 1...31 */
    )
    {
    if ((year -= 72) < 0)
	year += 100;

    return ((month << 10) | (day << 5) | (year & 0x1f));
    }

/*******************************************************************************
*
* rt11FsDelete - delete RT-11 file
*
* This routine deletes the file <name> from the specified RT-11 volume.
*
* RETURNS:
* OK, or
* ERROR if file not found or volume not available.
*
*/

LOCAL STATUS rt11FsDelete
    (
    RT_VOL_DESC *vdptr, /* pointer to volume descriptor */
    char *name          /* RT-11 filename (ffffff.ttt) */
    )
    {
    int entryNum;
    int start;
    RT_DIR_ENTRY entry;


    /* Set up for re-mount if no disk change notification */

    if (vdptr->vd_changeNoWarn == TRUE)
	rt11FsReadyChange (vdptr);


    semTake (vdptr->vd_semId, WAIT_FOREVER);

    /* check that volume is available */

    if ((rt11FsCheckVol (vdptr, TRUE) != OK) || (vdptr->vd_status != OK))
	{
	semGive (vdptr->vd_semId);
	errnoSet (S_rt11FsLib_VOLUME_NOT_AVAILABLE);
	return (ERROR);
	}

    if (rt11FsVolMode (vdptr) == O_RDONLY)
	{
	semGive (vdptr->vd_semId);
	errnoSet (S_ioLib_WRITE_PROTECTED);
	return (ERROR);
	}

    /* search for entry with specified name */

    if ((entryNum = rt11FsFindEntry (vdptr, name, &start)) == ERROR)
	{
	semGive (vdptr->vd_semId);		/* release volume */
	return (ERROR);
	}

    rt11FsGetEntry (vdptr, entryNum, &entry);
    entry.de_status = DES_EMPTY;
    entry.de_date = 0;
    rt11FsPutEntry (vdptr, entryNum, &entry);

    rt11FsCoalesce (vdptr, entryNum);		/* merge with empty neighbors */

    /* make sure directory is written out */

    if (rt11FsVolFlush (vdptr) != OK)
	{
	semGive (vdptr->vd_semId);		/* release volume */
	return (ERROR);
	}

    semGive (vdptr->vd_semId);			/* release volume */

    return (OK);
    }

/*******************************************************************************
*
* rt11FsDevInit - initialize the rt11Fs device descriptor
*
* This routine initializes the device descriptor.  The <pBlkDev> parameter is
* a pointer to an already-created BLK_DEV device structure.  This structure
* contains definitions for various aspects of the physical device format,
* as well as pointers to the sector read, sector write, ioctl(), status check,
* and reset functions for the device.
*
* The <rt11Fmt> parameter is TRUE if the device is to be accessed using
* standard RT-11 skew and interleave.
*
* The device directory will consist of one segment able to contain at
* least as many files as specified by <nEntries>.
* If <nEntries> is equal to RT_FILES_FOR_2_BLOCK_SEG, strict RT-11
* compatibility is maintained.
*
* The <changeNoWarn> parameter is TRUE if the disk may be changed without
* announcing the change via rt11FsReadyChange().  Setting <changeNoWarn> to
* TRUE causes the disk to be regularly remounted, in case it has been
* changed.  This results in a significant performance penalty.
*
* NOTE
* An ERROR is returned if <rt11Fmt> is TRUE and the `bd_blksPerTrack'
* (sectors per track) field in the BLK_DEV structure is odd.
* This is because an odd number of sectors per track is incompatible with the
* RT-11 interleaving algorithm.
*
* INTERNAL
* The semaphore in the device is given, so the device is available for
* use immediately.
*
* RETURNS:
* A pointer to the volume descriptor (RT_VOL_DESC), or
* NULL if invalid device parameters were specified,
* or the routine runs out of memory.
*/

RT_VOL_DESC *rt11FsDevInit
    (
    char            *devName,       /* device name */
    FAST BLK_DEV    *pBlkDev,       /* pointer to block device info */
    BOOL            rt11Fmt,        /* TRUE if RT-11 skew & interleave */
    FAST int        nEntries,       /* no. of dir entries incl term entry */
    BOOL            changeNoWarn    /* TRUE if no disk change warning */
    )
    {
    FAST RT_VOL_DESC	*vdptr;		/* pointer to volume descriptor */
    FAST int		segmentLen;	/* segment length in bytes */
    FAST int		i;


    /* Return error if no BLK_DEV */

    if (pBlkDev == NULL)
	{
	errnoSet (S_rt11FsLib_NO_BLOCK_DEVICE);
	return (NULL);
	}

    /* Don't allow odd number of sectors/track if RT-11 interleave specified */

    if (rt11Fmt && (pBlkDev->bd_blksPerTrack & 1))
	{
	errnoSet (S_rt11FsLib_INVALID_DEVICE_PARAMETERS);
	return (NULL);
	}

    /* Allocate an RT-11 volume descriptor for device */

    if ((vdptr = (RT_VOL_DESC *) calloc (sizeof (RT_VOL_DESC), 1)) == NULL)
	return (NULL);				/* no memory */


    /* Add device to system device table */

    if (iosDevAdd ((DEV_HDR *) vdptr, devName, rt11FsDrvNum) != OK)
	{
	free ((char *) vdptr);
	return (NULL);				/* can't add device */
	}

    /* Initialize volume descriptor */

    vdptr->vd_pBlkDev		= pBlkDev;

    vdptr->vd_rtFmt		= rt11Fmt;

    vdptr->vd_status		= OK;
    vdptr->vd_state		= RT_VD_READY_CHANGED;

    vdptr->vd_secBlock		= RT_BYTES_PER_BLOCK /
				  pBlkDev->bd_bytesPerBlk;
    vdptr->vd_nblocks		= pBlkDev->bd_nBlocks / vdptr->vd_secBlock;

    /* Set flag for disk change without warning (removable disks only) */

    if (pBlkDev->bd_removable)			/* if removable device */
    	vdptr->vd_changeNoWarn = changeNoWarn;	/* get user's flag */
    else
    	vdptr->vd_changeNoWarn = FALSE;		/* always FALSE for fixed disk*/

    /* how many bytes of space are needed for the segment? */

    segmentLen = sizeof (RT_DIR_SEG) + (nEntries - 1) * sizeof (RT_DIR_ENTRY);

    /* Allocate blocks for the segment.
     * The smallest segment is 2 blocks long */

    if  ((i = 1 + (segmentLen / RT_BYTES_PER_BLOCK)) < 2)
	vdptr->vd_nSegBlocks = 2;
    else
        vdptr->vd_nSegBlocks = i;

    /* allocate segment blocks */

    vdptr->vd_dir_seg = (RT_DIR_SEG *)
			malloc ((unsigned)
				(vdptr->vd_nSegBlocks * RT_BYTES_PER_BLOCK));
    if (vdptr->vd_dir_seg == NULL)
	return (NULL);

    vdptr->vd_semId = semMCreate (rt11FsVolMutexOptions);
    if (vdptr->vd_semId == NULL)
	return (NULL);				/* could not create semaphore */

    return (vdptr);
    }

/*******************************************************************************
*
* rt11FsDirEntry - get info from a directory entry
*
* This routine returns information about a particular directory entry into
* the REQ_DIR_ENTRY structure whose pointer is passed as a parameter.
* The information put there is the name of the file, its size (in bytes),
* and its creation date.  If the specified entry is an empty (keeps
* track of empty space on the disk), the name will be an empty string,
* the size will be correct, and the date will be meaningless.
*
* Before this routine is called, the field `entryNum' must set in
* the REQ_DIR_ENTRY structure pointed to by rdeptr.  That is the entry
* whose information will be returned.  Typically, the entries are accessed
* sequentially starting with 0 until the terminating entry is reached
* (indicated by a return code of ERROR).
*
* RETURNS:
* OK, or
* ERROR if no such entry.
*/

LOCAL STATUS rt11FsDirEntry
    (
    RT_VOL_DESC *vdptr,         /* pointer to volume descriptor */
    REQ_DIR_ENTRY *rdeptr       /* ptr to structure into which to put info */
    )
    {
    RT_DIR_ENTRY *deptr;	/* pointer to directory entry in question */
    FAST int maxEntries;	/* max number of entries allowed in directory */


    /* Set up for re-mount if no disk change notification */

    if (vdptr->vd_changeNoWarn == TRUE)
	rt11FsReadyChange (vdptr);


    semTake (vdptr->vd_semId, WAIT_FOREVER);

    if (rt11FsCheckVol (vdptr, TRUE) != OK)
	{
	semGive (vdptr->vd_semId);
	errnoSet (S_rt11FsLib_VOLUME_NOT_AVAILABLE);
	return (ERROR);
	}

    /* what is the maximum number of entries allowed in this directory? */

    maxEntries = (vdptr->vd_nSegBlocks * RT_BYTES_PER_BLOCK
		- (sizeof (RT_DIR_SEG) - sizeof (RT_DIR_ENTRY)))
		/ sizeof (RT_DIR_ENTRY);

    if (rdeptr->entryNum >= maxEntries)
	{
	semGive (vdptr->vd_semId);
	errnoSet (S_rt11FsLib_ENTRY_NUMBER_TOO_BIG);
	return (ERROR);
	}

    deptr = &(vdptr->vd_dir_seg->ds_entries[rdeptr->entryNum]);

    switch (deptr->de_status)
	{
	case DES_TENTATIVE:	/* - should indicate tentative somehow? */
	case DES_PERMANENT:
	    rt11FsNameString (deptr->de_name, rdeptr->name);
	    break;

	case DES_EMPTY:
	    rdeptr->name[0] = EOS;			/* empty, no name */
	    break;

	default:
	    semGive (vdptr->vd_semId);			/* release volume */
	    errnoSet (S_rt11FsLib_FILE_NOT_FOUND);
	    return (ERROR);				/* no such entry */
	}

    rdeptr->nChars = deptr->de_nblocks * RT_BYTES_PER_BLOCK;
    rdeptr->day = (deptr->de_date >> 5) & 0x001f;
    rdeptr->month = (deptr->de_date >> 10) & 0x000f;
    rdeptr->year = (deptr->de_date & 0x001f) + 1972;

    semGive (vdptr->vd_semId);			/* release volume */

    return (OK);
    }

/*******************************************************************************
*
* rt11FsDirRead - read directory and return next file name
*
* This routine support POSIX directory searches.  The directory is read,
* and the name of the next file is returned in a "dirent" structure.
* This routine is called via an ioctl() call with a function code of
* FIOREADDIR.
*
* RETURNS: OK, or ERROR if end of dir (errno = OK) or real error (errno set).
*/

LOCAL STATUS rt11FsDirRead
    (
    FAST RT_FILE_DESC   *pFd,           /* ptr to RT-11 file descriptor */
    FAST DIR            *pDir           /* ptr to directory descriptor */
    )
    {

    FAST int		entryNum;	/* position within dir */
    FAST int		maxEntries;	/* number of entries in dir */
    FAST char		*pChar;		/* ptr to char in filename string */
    int			nameLen;	/* length of filename string */
    FAST RT_DIR_ENTRY	*pEntry;	/* ptr to directory entry in memory */
    FAST RT_VOL_DESC	*vdptr = pFd->rfd_vdptr;
					/* ptr to volume descriptor */


    semTake (vdptr->vd_semId, WAIT_FOREVER);

    if (rt11FsCheckVol (vdptr, TRUE) != OK)
	{
	semGive (vdptr->vd_semId);
	errnoSet (S_rt11FsLib_VOLUME_NOT_AVAILABLE);
	return (ERROR);
	}

    /* Check if cookie (entry number) is past end of directory */

    entryNum = pDir->dd_cookie;			/* get marker from DIR */

    maxEntries = (vdptr->vd_nSegBlocks * RT_BYTES_PER_BLOCK
		  - (sizeof (RT_DIR_SEG) - sizeof (RT_DIR_ENTRY)))
		 / sizeof (RT_DIR_ENTRY);


    /* Read an entry, keep going if empty */

    do
	{
    	if (entryNum >= maxEntries)
	    {
	    semGive (vdptr->vd_semId);
	    return (ERROR);			/* end of directory */
	    }

    	pEntry = &(vdptr->vd_dir_seg->ds_entries[entryNum]);
						/* find entry within dir */

	if (pEntry->de_status == DES_END)
	    {
	    semGive (vdptr->vd_semId);
	    return (ERROR);			/* end of directory */
	    }

	entryNum++;

	} while (pEntry->de_status == DES_EMPTY);


    /* Copy name to dirent struct */

   rt11FsNameString (pEntry->de_name, pDir->dd_dirent.d_name);


   /* Cancel trailing blanks or "." */

   nameLen = strlen (pDir->dd_dirent.d_name);

   if (nameLen > 0)
	{
	pChar = (pDir->dd_dirent.d_name + nameLen - 1);

	while ((*pChar == ' ') || (*pChar == '.'))
	    {
	    *pChar = EOS;
	    pChar--;
	    }
	}


    pDir->dd_cookie = entryNum;			/* copy updated entry number */

    semGive (vdptr->vd_semId);
    return (OK);
    }

/*******************************************************************************
*
* rt11FsFileStatGet - get file status info from directory entry
*
* This routine is called via an ioctl() call, using the FIOFSTATGET
* function code.  The passed stat structure is filled, using data
* obtained from the directory entry which describes the file.
*
* RETURNS: OK (always).
*/

LOCAL STATUS rt11FsFileStatGet
    (
    FAST RT_FILE_DESC   *pFd,           /* pointer to file descriptor */
    FAST struct stat    *pStat          /* structure to fill with data */
    )
    {
    struct tm 	        fileDate;

    bzero ((char *) pStat, sizeof (struct stat));
					/* zero out stat struct */

    /* Fill stat structure */

    pStat->st_dev     = (ULONG) pFd->rfd_vdptr;	/* device ID = DEV_HDR addr */
    pStat->st_ino     = 0;			/* no file serial number */
    pStat->st_nlink   = 1;			/* always only one link */
    pStat->st_uid     = 0;			/* no user ID */
    pStat->st_gid     = 0;			/* no group ID */
    pStat->st_rdev    = 0;			/* no special device ID */

    pStat->st_size    = (pFd->rfd_dir_entry.de_nblocks * RT_BYTES_PER_BLOCK);
						/* file size, in bytes */
    pStat->st_blksize = 0;
    pStat->st_blocks  = 0;
    pStat->st_attrib  = 0;			/* no file attribute byte */

    /*
     * Fill in the timestamp fields
     */

    bzero ((char *) &fileDate, sizeof (fileDate));

    fileDate.tm_mday = (pFd->rfd_dir_entry.de_date >> 5) & 0x001f;

    /* Need to adjust if day == 0 ; ANSI days run 1-31, zero isn't valid */

    if (fileDate.tm_mday == 0)
        fileDate.tm_mday = 1;

    fileDate.tm_mon  = (pFd->rfd_dir_entry.de_date >> 10) & 0x000f;

    /* RT11 uses 1972 as the starting date, ANSI uses 1900; adjust by 72 yrs */
    
    fileDate.tm_year = (pFd->rfd_dir_entry.de_date & 0x001f) + 72;

    /* Set access, change, and modify times all to the value for de_date */
    
    pStat->st_atime = pStat->st_mtime = pStat->st_ctime = mktime (&fileDate);

    /* Set mode field (if whole volume opened, mark as directory) */

    if (pFd->rfd_dir_entry.de_status == DES_BOGUS)
    	pStat->st_mode = S_IFDIR;			/* raw vol = dir */
    else
    	pStat->st_mode = S_IFREG;			/* regular file */


    pStat->st_mode |= (S_IRWXU | S_IRWXG | S_IRWXO);
						/* default privileges are
						 * read-write-execute for all
						 */

    return (OK);
    }

/*******************************************************************************
*
* rt11FsFindEntry - find a directory entry by name on an RT-11 volume
*
* This routines searches for the directory entry of the specified file
* <name> from the specified volume, if it exists.
*
* Possession of the volume descriptor semaphore must have been secured
* before this routine is called.
*
* RETURNS:
* Entry number if found, or
* ERROR if file not found.
*/

LOCAL int rt11FsFindEntry
    (
    FAST RT_VOL_DESC *vdptr,    /* pointer to volume descriptor */
    char *name,                 /* RT-11 filename (ffffff.ttt) */
    int *pstart                 /* pointer where to return start block number */
    )
    {
    RT_DIR_ENTRY entry;
    RT_NAME name_r50;
    FAST int i;


    /* Ignore any leading "/" or "\"'s in file name */

    while ((*name == '/') || (*name == '\\'))
	name++;

    /* search entire directory for active entry matching specified name.
     * keep track of start block by accumulating entry lengths.
     */

    rt11FsNameR50 (name, &name_r50);
    *pstart = vdptr->vd_dir_seg->ds_start;

    for (i = 0; ; i++)
	{
	rt11FsGetEntry (vdptr, i, &entry);

	/* if this is the end of the directory, then file not found. */

	if (entry.de_status == DES_END)
	    {
	    errnoSet (S_rt11FsLib_FILE_NOT_FOUND);
	    return (ERROR);
	    }


	/* check if this is the desired file */

        if (((entry.de_status == DES_PERMANENT)             ||
	     (entry.de_status == DES_TENTATIVE))            &&
	     (entry.de_name.nm_name1 == name_r50.nm_name1)  &&
	     (entry.de_name.nm_name2 == name_r50.nm_name2)  &&
	     (entry.de_name.nm_type == name_r50.nm_type))
	    {
	    return (i);
	    }

	/* entry doesn't match;
	 * add file length to get start of next file; bump to next entry */

	*pstart += entry.de_nblocks;
	}
    }

/*******************************************************************************
*
* rt11FsFlush - flush RT-11 file output buffer
*
* This routine guarantees that any output that has been requested
* is actually written to the disk.  In particular, writes that have
* been buffered are written to the disk.
*
* RETURNS:
* OK, or ERROR if unable to write.
*/

LOCAL STATUS rt11FsFlush
    (
    FAST RT_FILE_DESC *pFd      /* pointer to file descriptor */
    )
    {
    int block;

    if (pFd->rfd_modified)
	{
	/* write out the current (dirty) block and reset 'modified' indicator */

	block = pFd->rfd_start + (pFd->rfd_curptr / RT_BYTES_PER_BLOCK);

	if (rt11FsWrtBlock (pFd->rfd_vdptr, block, 1, pFd->rfd_buffer) != OK)
	    return (ERROR);

	pFd->rfd_modified = FALSE;
	}

    return (OK);
    }

/*******************************************************************************
*
* rt11FsFreeFd - free a file descriptor
*/

LOCAL void rt11FsFreeFd
    (
    RT_FILE_DESC *pFd                   /* pointer to file descriptor to free */
    )
    {
    semTake (rt11FsFdSemId, WAIT_FOREVER);
    pFd->rfd_status = NOT_IN_USE;
    semGive (rt11FsFdSemId);
    }

/*******************************************************************************
*
* rt11FsGetFd - get an available file descriptor
*
* RETURNS: pointer to file descriptor, or NULL if none available.
*/

LOCAL RT_FILE_DESC *rt11FsGetFd (void)
    {
    FAST RT_FILE_DESC *pRtFd;
    FAST int i;

    semTake (rt11FsFdSemId, WAIT_FOREVER);

    for (i = 0, pRtFd = &rt11FsFd [0]; i < rt11FsMaxFiles; i++, pRtFd++)
	{
	if (pRtFd->rfd_status == NOT_IN_USE)
	    {
	    pRtFd->rfd_status = OK;
	    semGive (rt11FsFdSemId);
	    return (pRtFd);
	    }
	}

    semGive (rt11FsFdSemId);
    errnoSet (S_rt11FsLib_NO_FREE_FILE_DESCRIPTORS);

    return (NULL);
    }

/*******************************************************************************
*
* rt11FsGetEntry - get a directory entry from an RT-11 volume
*
* This routines gets the specified directory entry from the specified
* volume.
*
* Possession of the volume descriptor semaphore must have been secured
* before this routine is called.
*
* RETURNS: a copy of the requested entry.
*/

LOCAL void rt11FsGetEntry
    (
    RT_VOL_DESC *vdptr,    /* pointer to volume descriptor */
    int entryNum,          /* number of entry to get (first entry = 0) */
    RT_DIR_ENTRY *pEntry   /* pointer where to return directory entry */
    )
    {
    bcopy ((char *) &(vdptr->vd_dir_seg->ds_entries[entryNum]),
	   (char *) pEntry, sizeof (RT_DIR_ENTRY));
    }

/*******************************************************************************
*
* rt11FsInit - prepare to use the rt11Fs library
*
* This routine initializes the rt11Fs library.  It must be called exactly
* once, before any other routine in the library.  The <maxFiles> parameter
* specifies the number of rt11Fs files that may be open at once.  This
* routine initializes the necessary memory structures and semaphores.
*
* This routine is called automatically from the root task, usrRoot(),
* in usrConfig.c when the configuration macro INCLUDE_RT11FS is defined.
*
* RETURNS:
* OK, or ERROR if memory is insufficient.
*/

STATUS rt11FsInit
    (
    int maxFiles        /* max no. of simultaneously */
			/* open rt11Fs files */
    )
    {
    int i;


    /* Install rt11FsLib routines in I/O system driver table */

    rt11FsDrvNum = iosDrvInstall ((FUNCPTR) rt11FsCreate, rt11FsDelete,
				  (FUNCPTR) rt11FsOpen, rt11FsClose,
				  rt11FsRead, rt11FsWrite, rt11FsIoctl);
    if (rt11FsDrvNum == ERROR)
	return (ERROR);				/* can't install as driver */


    /* initialize interlocking of file descriptor list access */

    rt11FsFdSemId = semCreate ();
    semGive (rt11FsFdSemId);

    rt11FsMaxFiles = maxFiles;
    rt11FsFd = (RT_FILE_DESC *) malloc ((unsigned)
				      (rt11FsMaxFiles * sizeof (RT_FILE_DESC)));

    if (rt11FsFd == (RT_FILE_DESC *) NULL)
	return (ERROR);

    for (i = 0; i < rt11FsMaxFiles; i++)
	rt11FsFreeFd (&rt11FsFd[i]);

    return (OK);
    }

/*******************************************************************************
*
* rt11FsInsEntry - insert a new directory entry on an RT-11 volume
*
* This routines inserts the specified directory entry into the specified
* volume at the specified entry index.
*
* Possession of the volume descriptor semaphore must have been secured
* before this routine is called.
*
* NOTE: Currently this routine does not handle the case of attempting
* to add an entry to a full directory.
*/

LOCAL void rt11FsInsEntry
    (
    RT_VOL_DESC *vdptr,   /* pointer to volume descriptor */
    int entryNum,         /* number of entry to insert (first entry = 0) */
    RT_DIR_ENTRY *pEntry  /* pointer to entry to insert */
    )
    {
    RT_DIR_ENTRY previous_entry;
    RT_DIR_ENTRY entry;

    /* replace current entry at entryNum with new */

    rt11FsGetEntry (vdptr, entryNum, &entry);
    rt11FsPutEntry (vdptr, entryNum, pEntry);


    /* replace each entry with the previous */

    while (entry.de_status != DES_END)
	{
	previous_entry = entry;
	entryNum++;

	rt11FsGetEntry (vdptr, entryNum, &entry);
	rt11FsPutEntry (vdptr, entryNum, &previous_entry);
	}


    /* put end-marker entry in next slot */

    rt11FsPutEntry (vdptr, entryNum + 1, &entry);
    }

/*******************************************************************************
*
* rt11FsIoctl - do device specific control function
*
* This routine performs the following ioctl() functions:
*
* .CS
*    FIODISKINIT:   Initialize the disk volume.  This routine does not
*                   format the disk, this must be done by the driver.
*    FIODIRENTRY:   Returns pointer to the next dir entry, into
*                   the REQ_DIR_ENTRY pointed to by arg.
*    FIORENAME:     Rename the file to the string pointed to by arg.
*    FIOSEEK:       Sets the file's current byte position to
*                   the position specified by arg.
*    FIOWHERE:      Returns the current byte position in the file.
*    FIOFLUSH:      Flush file output buffer.
*                   Guarantees that any output that has been requested
*                   is actually written to the device.
*    FIONREAD:      Return in arg the number of unread bytes.
*    FIODISKCHANGE: Indicate media change.  See rt11FsReadyChange().
*    FIOSQUEEZE:    Reclaim fragmented free space on RT-11 volume
*    FIOREADDIR:    Read one entry from directory and return file name.
*    FIOFSTATGET:   Get file status info from directory entry.
*    FIOGETFL:      Return in arg the file's open mode.
* .CE
*
* Any ioctl() functions which cannot be handled by rt11FsLib are passed to
* the device driver.
*
* If an ioctl() fails, the task status (see errnoGet()) indicates
* the nature of the error.
*
* RETURNS:
* OK, or
* ERROR if function failed or unknown function, or
* current byte pointer for FIOWHERE.
*
*/

LOCAL STATUS rt11FsIoctl
    (
    RT_FILE_DESC *pFd,  /* file descriptor of file to control */
    int function,       /* function code */
    int arg             /* some argument */
    )
    {
    int where;

    switch (function)
	{
	case FIODISKINIT:
	    return (rt11FsVolInit (pFd->rfd_vdptr));

	case FIODIRENTRY:
	    return (rt11FsDirEntry (pFd->rfd_vdptr, (REQ_DIR_ENTRY *)arg));

	case FIORENAME:
	    return (rt11FsRename (pFd, (char *)arg));

	case FIOSEEK:
	    return (rt11FsSeek (pFd, arg));

	case FIOWHERE:
	    return (rt11FsWhere (pFd));

	case FIOFLUSH:
	    return (rt11FsFlush (pFd));

	case FIONREAD:
	    if ((where = rt11FsWhere (pFd)) == ERROR)
		return (ERROR);
	    *((int *) arg) = pFd->rfd_endptr - where;
	    return (OK);

	case FIODISKCHANGE:
	    rt11FsReadyChange (pFd->rfd_vdptr);
	    return (OK);

	case FIOSQUEEZE:
	    return (rt11FsSqueeze (pFd->rfd_vdptr));

	case FIOREADDIR:
	    return (rt11FsDirRead (pFd, (DIR *) arg));

	case FIOFSTATGET:
	    return (rt11FsFileStatGet (pFd, (struct stat *) arg));

	case FIOGETFL:
	    *((int *) arg) = pFd->rfd_mode;
	    return (OK);

	default:
	    /* Call device driver function */

	    return ((* pFd->rfd_vdptr->vd_pBlkDev->bd_ioctl)
		    (pFd->rfd_vdptr->vd_pBlkDev, function, arg));
	}
    }

/*******************************************************************************
*
* rt11FsMkfs - initialize a device and create an rt11Fs file system
*
* This routine provides a quick method of creating an rt11Fs file system on
* a device.  It is used instead of the two-step procedure of calling
* rt11FsDevInit() followed by an ioctl() call with an FIODISKINIT function
* code.
*
* This routine provides defaults for the rt11Fs parameters expected by
* rt11FsDevInit().  The directory size is set to RT_FILES_FOR_2_BLOCK_SEG
* (defined in rt11FsLib.h).  No standard disk format is assumed; this
* allows the use of rt11Fs on block devices with an odd number of sectors per
* track.  The <changeNoWarn> parameter is defined as FALSE, 
* indicating that the disk will not be replaced without rt11FsReadyChange() 
* being called first.
*
* If different values are needed for any of these parameters, the routine
* rt11FsDevInit() must be used instead of this routine, followed by a
* request for disk initialization using the ioctl() function FIODISKINIT.
*
* RETURNS: A pointer to an rt11Fs volume descriptor (RT_VOL_DESC),
* or NULL if there is an error.
*
* SEE ALSO: rt11FsDevInit()
*/

RT_VOL_DESC *rt11FsMkfs
    (
    char                *volName,       /* volume name to use */
    BLK_DEV             *pBlkDev        /* pointer to block device struct */
    )
    {
    FAST RT_VOL_DESC	*vdptr;		/* pointer to RT-11 volume descriptor */


    vdptr = rt11FsDevInit (volName, 			/* name */
			 pBlkDev, 			/* device to use */
			 FALSE, 			/* no std RT11 format */
			 RT_FILES_FOR_2_BLOCK_SEG,	/* dir size */
			 FALSE);			/* no changeNoWarn */
    if (vdptr == NULL)
	return (NULL);


    if (rt11FsVolInit (vdptr) != OK)
	return (NULL);

    return (vdptr);
    }


/*******************************************************************************
*
* rt11FsNameR50 - convert ASCII string to radix-50 RT-11 name
*
* This routine converts the specified ASCII string to an RT-11 name.
* The <string> must be of the form "ffffff.ttt" and be null terminated.
*
* If characters are in <string> that don't exist in the radix-50 world,
* undefined things will happen.
*/

LOCAL void rt11FsNameR50
    (
    char *string,       /* pointer to ASCII string to convert */
    RT_NAME *pName      /* pointer where to return RT-11 radix-50 name struct */
    )
    {
    char tstring [RT_NAME_LEN];
    FAST char *dot;

    /* get local copy of string and make sure it's terminated */

    (void) strncpy (tstring, string, RT_NAME_LEN - 1);
    tstring [RT_NAME_LEN - 1] = EOS;


    /* find the dot, if any; convert extension; and replace dot w/EOS */

    dot = index (tstring, '.');

    if (dot == 0)
	pName->nm_type = 0;
    else
	{
	*dot = EOS;
	pName->nm_type = rt11FsR50out (dot + 1);
	}

    /* convert name part 1 then part 2 if more */

    pName->nm_name1 = rt11FsR50out (tstring);

    if (strlen (tstring) <= 3)
	pName->nm_name2 = 0;
    else
	pName->nm_name2 = rt11FsR50out (tstring + 3);
    }

/*******************************************************************************
*
* rt11FsR50out - convert up to 3 ASCII characters to radix-50
*
* RETURNS: radix-50 equivalent of first 3 chars of <string>.
*/

LOCAL int rt11FsR50out
    (
    FAST char *string   /* string to convert */
    )
    {
    unsigned int r50 = 0;
    int r;
    int i;
    char ch;

    for (i = 0; i < 3; i++)
	{
	if (*string == EOS)
	    r = 0;
	else
	    {
	    ch = *string;

	    if (isupper (ch))
		ch = tolower (ch);

	    r = (char *) index (rad50, ch) - rad50;
	    string++;
	    }

	r50 = (r50 * 050) + r;
	}

    return (r50);
    }

/*******************************************************************************
*
* rt11FsNameString - convert radix-50 RT-11 name to ASCII string
*
* This routine converts the specified RT-11 <name> into an ASCII string
* of the form "ffffff.ttt" and null terminated.
*/

LOCAL void rt11FsNameString
    (
    RT_NAME name,       /* RT-11 radix-50 name structure */
    FAST char *string   /* pointer to receive ASCII string */
    )
    {
    FAST char *pStr;	/* index into string */

    rt11FsR50in (name.nm_name1, string);
    rt11FsR50in (name.nm_name2, string + 3);

    for (pStr = string; (pStr < (string + 6)) && (*pStr != ' '); ++pStr)
	;		/* prepare to overwrite trailing blanks */

    *pStr++ = '.';

    rt11FsR50in (name.nm_type, pStr);

    *(pStr + 3) = EOS;
    }

/*******************************************************************************
*
* rt11FsR50in - convert radix-50 to 3 ASCII characters
*/

LOCAL void rt11FsR50in
    (
    unsigned int r50,   /* radix-50 number to convert */
    char *string        /* where to put resulting string */
    )
    {
    FAST int i;

    for (i = 2; i >= 0; i--)
	{
	string [i] = rad50 [r50 % 050];

	r50 /= 050;
	}
    }

/*******************************************************************************
*
* rt11FsOpen - open a file on an RT-11 volume
*
* This routine opens the file <name> with the specified <mode>.
* The volume directory is searched, and if the file is found
* an RT-11 file descriptor is initialized for it.
*
* The directory currently in memory for the volume is used unless there
* has been a ready change (rt11FsReadyChange()) or this is the very first
* open.  If that is the case, the directory will be read from the disk
* automatically.
*
* A null file <name> is treated specially, to open an entire disk.
* In this case, no attempt is made to access the disk directory, so that
* even un-initialized disks may be accessed.
*
* RETURNS:
* A pointer to the RT-11 file descriptor, or
* ERROR if the volume not available or
* no RT-11 file descriptors are available or
* there is no such file.
*/

LOCAL RT_FILE_DESC *rt11FsOpen
    (
    RT_VOL_DESC *vdptr, /* pointer to volume descriptor */
    char *name,         /* RT-11 filename (ffffff.ttt) */
    int mode            /* file mode (O_RDONLY/O_WRONLY/O_RDWR) */
    )
    {
    int start;
    int entryNum;
    FAST RT_FILE_DESC *pFd;	/* file descriptor pointer */



    /* Call driver check-status routine, if any */

    if (vdptr->vd_pBlkDev->bd_statusChk != NULL)
	{
	if ((* vdptr->vd_pBlkDev->bd_statusChk) (vdptr->vd_pBlkDev) != OK)
	    {
	    return ((RT_FILE_DESC *) ERROR);
	    }
	}


    /* Set up for re-mount if no disk change notification */

    if (vdptr->vd_changeNoWarn == TRUE)
	rt11FsReadyChange (vdptr);


    /* get a free file descriptor */

    pFd = rt11FsGetFd ();	/* file descriptor pointer */
    if (pFd == NULL)
	return ((RT_FILE_DESC *) ERROR);

    /* check for O_CREAT and O_TRUNC mode */

    if ((mode & O_CREAT) || (mode & O_TRUNC))
	{
	/*XXX O_CREAT and O_TRUNC are masked off because several rt11FsLib
	 * routines use rt11FsVolMode() to check the mode of a file by
	 * arithmetic comparison instead of by boolean comparison of
	 * specific bits.
	 */

	mode &= ~O_CREAT;

	/* create the file if it does not exist or O_TRUNC is set */

	if ((mode & O_TRUNC) ||
	    (rt11FsFindEntry (vdptr, name, &start) == ERROR))
	    {
	    mode &= ~O_TRUNC;
	    rt11FsFreeFd (pFd);
	    return (rt11FsCreate (vdptr, name, mode));
	    }
	}

    /* check for open of raw device (null filename) */

    if (name [0] == EOS)
	{
	/* check that volume is available */

	semTake (vdptr->vd_semId, WAIT_FOREVER);

	if (rt11FsCheckVol (vdptr, FALSE) != OK)
	    {
	    semGive (vdptr->vd_semId);
	    rt11FsFreeFd (pFd);
	    errnoSet (S_rt11FsLib_VOLUME_NOT_AVAILABLE);
	    return ((RT_FILE_DESC *) ERROR);
	    }

	if (rt11FsVolMode (vdptr) < mode)
	    {
	    semGive (vdptr->vd_semId);
	    rt11FsFreeFd (pFd);
	    errnoSet (S_ioLib_WRITE_PROTECTED);
	    return ((RT_FILE_DESC *) ERROR);
	    }

	semGive (vdptr->vd_semId);

	/* null name is special indicator for "raw" disk;
	 * fabricate a bogus directory entry covering entire volume */

	start = 0;

	pFd->rfd_dir_entry.de_status  = DES_BOGUS;
	pFd->rfd_dir_entry.de_date    = rt11FsDate (rt11FsYear, rt11FsMonth,
						    rt11FsDay);
	pFd->rfd_dir_entry.de_nblocks = vdptr->vd_nblocks;
	rt11FsNameR50 ("device.raw", &pFd->rfd_dir_entry.de_name);
	}
    else
	{
	/* check that volume is available */

	semTake (vdptr->vd_semId, WAIT_FOREVER);
	if ((rt11FsCheckVol (vdptr, TRUE) != OK) || (vdptr->vd_status != OK))
	    {
	    semGive (vdptr->vd_semId);
	    rt11FsFreeFd (pFd);
	    errnoSet (S_rt11FsLib_VOLUME_NOT_AVAILABLE);
	    return ((RT_FILE_DESC *) ERROR);
	    }

	if (rt11FsVolMode (vdptr) < mode)
	    {
	    semGive (vdptr->vd_semId);
	    rt11FsFreeFd (pFd);
	    errnoSet (S_ioLib_WRITE_PROTECTED);
	    return ((RT_FILE_DESC *) ERROR);
	    }

	/* find specified file name in directory */

	if ((entryNum = rt11FsFindEntry (vdptr, name, &start)) == ERROR)
	    {
	    semGive (vdptr->vd_semId);			/* release volume */
	    rt11FsFreeFd (pFd);
	    return ((RT_FILE_DESC *) ERROR);
	    }

	rt11FsGetEntry (vdptr, entryNum, &pFd->rfd_dir_entry);
	semGive (vdptr->vd_semId);			/* release volume */
	}

    /* initialize file descriptor */

    pFd->rfd_mode	= mode;
    pFd->rfd_vdptr	= vdptr;
    pFd->rfd_start	= start;
    pFd->rfd_curptr	= NONE;
    pFd->rfd_newptr	= 0;
    pFd->rfd_endptr	= pFd->rfd_dir_entry.de_nblocks * RT_BYTES_PER_BLOCK;
    pFd->rfd_modified	= FALSE;

    return (pFd);
    }

/*******************************************************************************
*
* rt11FsPutEntry - put directory entry of RT-11 file
*
* This routines puts the specified directory entry of the specified
* volume.  The first entry is #0.
*
* Posession of the volume descriptor's semaphore must have been secured
* before this routine is called.
*/

LOCAL void rt11FsPutEntry
    (
    RT_VOL_DESC         *vdptr,         /* pointer to volume descriptor */
    int                 entryNum,       /* # of entry to get (first entry = 0)*/
    RT_DIR_ENTRY        *pEntry         /* pointer to entry to put */
    )
    {

    vdptr->vd_dir_seg->ds_entries [entryNum] = *pEntry;

    }

/*******************************************************************************
*
* rt11FsRead - read from an RT-11 file
*
* This routine reads from the file specified by the file descriptor
* (returned by rt11FsOpen ()) into the specified <buffer>.
* Up to <maxbytes> bytes will be read, if there is that much data in the file.
*
* RETURNS:
* Number of bytes read, or
* 0 if end of file, or
* ERROR if maxbytes <= 0 or unable to get next block.
*/

LOCAL int rt11FsRead
    (
    FAST RT_FILE_DESC   *pFd,           /* file descriptor pointer */
    char                *pBuf,          /* buffer to receive data */
    FAST int            maxBytes        /* maximum bytes to read */
    )
    {
    FAST int		bufIndex;	/* offset to current position in buf */
    int			startBlk;	/* starting block for read */
    int			numBlks;	/* number of blocks to read */
    int			curBlk;		/* number of block currently in buf */
    STATUS		status;		/* number of blocks to read */
    FAST int		nBytes;		/* number of bytes read */
    FAST int		bytesLeft;	/* remaining bytes to read */



    /* check for valid maxBytes */

    if (maxBytes <= 0)
	{
	errnoSet (S_rt11FsLib_INVALID_NUMBER_OF_BYTES);
	return (ERROR);
	}

    /* Do successive reads until requested byte count or EOF */

    bytesLeft = maxBytes;			/* init number remaining */

    while (bytesLeft > 0)
	{
	nBytes = 0;				/* init individual read count */

    	/* Do direct whole-block read if possible
    	 *   Transfer must be >= 1 block, must start on a block
    	 *   boundary, and must not extend past end-of-file.
    	 */

    	if ((bytesLeft >= RT_BYTES_PER_BLOCK) &&
	    (pFd->rfd_newptr % RT_BYTES_PER_BLOCK == 0) &&
	    (pFd->rfd_newptr + bytesLeft <= (pFd->rfd_dir_entry.de_nblocks *
					    RT_BYTES_PER_BLOCK)))
	    {
	    /* Determine max number of blocks to read */

    	    if (pFd->rfd_vdptr->vd_rtFmt)	/* if using std skew/intleave */
		numBlks = 1;			/*  blocks are not contiguous */
	    else
	    	numBlks = bytesLeft / RT_BYTES_PER_BLOCK;


	    /* Determine starting block for read */

	    startBlk = (pFd->rfd_newptr / RT_BYTES_PER_BLOCK) + pFd->rfd_start;


	    /* Read block(s) directly into caller's buffer */

	    if (rt11FsRdBlock (pFd->rfd_vdptr, startBlk, numBlks, pBuf) != OK)
		{				/* read error */
		if (bytesLeft == maxBytes)	/* if nothing read yet */
		    return (ERROR);		/*  indicate error */
		else
	    	    break;			/* return number read OK */
		}


	    /* If file descriptor buffer contains modified data (O_RDWR mode)
	     *  and the read included bufferred block, insert buffer contents
	     *  into user's buffer at appropriate offset.
	     */

	    if (pFd->rfd_modified)
		{
	    	curBlk = (pFd->rfd_curptr / RT_BYTES_PER_BLOCK) +
			 pFd->rfd_start;

	    	if ((curBlk >= startBlk)  && (curBlk < (startBlk + numBlks)))
		    {
    	    	    bcopy (pFd->rfd_buffer,
		           pBuf + ((curBlk - startBlk) * RT_BYTES_PER_BLOCK),
		           RT_BYTES_PER_BLOCK);
		    }
		}

	    nBytes = numBlks * RT_BYTES_PER_BLOCK;
	    }
        else
	    {
    	    /* Get new pointer in buffer */

    	    if ((status = rt11FsNewBlock (pFd)) <= 0)
		{				/* probably end of file */
		if (bytesLeft == maxBytes)	/* if nothing read yet */
		    return (status);		/*  indicate error */
		else
	    	    break;			/* return number read OK */
		}


    	    /* Fill all of caller's buffer or rest of Fd block buffer,
	     * whichever is less.  Since files are always an even multiple
	     * of the block length, a separate check for EOF isn't needed.
	     */

    	    bufIndex = pFd->rfd_newptr % RT_BYTES_PER_BLOCK;

    	    nBytes = min (bytesLeft, RT_BYTES_PER_BLOCK - bufIndex);

    	    bcopy (&pFd->rfd_buffer [bufIndex], pBuf, nBytes);
	    }

	/* Adjust count remaining, buffer pointer, and file pointer */

	bytesLeft -= nBytes;
	pBuf      += nBytes;
    	pFd->rfd_newptr += nBytes;

	}  /* end while */

    return (maxBytes - bytesLeft);		/* number of bytes read */
    }

/*******************************************************************************
*
* rt11FsRename - change name of RT-11 file
*
* This routine changes the name of the specified stream to the specified
* <newName>.
*
* RETURNS:
* OK, or
* ERROR if newName already in use, or
* unable to write out new directory info.
*/

LOCAL STATUS rt11FsRename
    (
    RT_FILE_DESC *pFd,          /* pointer to file descriptor */
    char *newName               /* change name of file to this */
    )
    {
    char oldName [RT_NAME_LEN];	/* current file name gets unpacked into here */
    char *tail;			/* tail of newName */
    int entryNum;		/* gets index into directory of oldname */
    int start;			/* for receiving rt11FsFindEntry side-effect */
    RT_DIR_ENTRY entry;		/* directory entry of oldname goes here */

    /* ensure newName doesn't include device name */

    if (iosDevFind (newName, &tail) != NULL)
	(void) strcpy (newName, tail);

    semTake (pFd->rfd_vdptr->vd_semId, WAIT_FOREVER);

    /* be sure new name isn't already in use */

    if (rt11FsFindEntry (pFd->rfd_vdptr, newName, &start) != ERROR)
	{
	semGive (pFd->rfd_vdptr->vd_semId);	/* release volume */
	errnoSet (S_rt11FsLib_FILE_ALREADY_EXISTS);
	return (ERROR);				/* entry already exists */
	}

    /* unpack name and find entry number */

    rt11FsNameString (pFd->rfd_dir_entry.de_name, oldName);
    if ((entryNum = rt11FsFindEntry (pFd->rfd_vdptr, oldName, &start)) == ERROR)
	{
	semGive (pFd->rfd_vdptr->vd_semId);		/* release volume */
	return (ERROR);					/* entry not found */
	}

    rt11FsGetEntry (pFd->rfd_vdptr, entryNum, &entry);
    rt11FsNameR50 (newName, &entry.de_name);		/* put in new name */
    rt11FsNameR50 (newName, &pFd->rfd_dir_entry.de_name);
							/* rename fd also */
    rt11FsPutEntry (pFd->rfd_vdptr, entryNum, &entry);

    /* make sure directory is written out */

    if (rt11FsVolFlush (pFd->rfd_vdptr) != OK)
	{
	semGive (pFd->rfd_vdptr->vd_semId);		/* release volume */
	return (ERROR);					/* entry not found */
	}

    semGive (pFd->rfd_vdptr->vd_semId);		/* release volume */
    return (OK);
    }

/*******************************************************************************
*
* rt11FsSeek - change file's current character position
*
* This routine sets the specified file's current character position to
* the specified <position>.  This only changes the pointer; it does not affect
* the hardware.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS rt11FsSeek
    (
    FAST RT_FILE_DESC *pFd,     /* file descriptor pointer */
    int position                /* desired character position in file */
    )
    {
    if (pFd->rfd_status == NOT_IN_USE)
	{
	errnoSet (S_rt11FsLib_FILE_NOT_FOUND);
	return (ERROR);
	}

    if (position > (pFd->rfd_dir_entry.de_nblocks * RT_BYTES_PER_BLOCK))
	{
	errnoSet (S_rt11FsLib_BEYOND_FILE_LIMIT);
	return (ERROR);
	}

    /* update new file byte pointer; also end pointer if file just grew */

    pFd->rfd_newptr = position;

    if (pFd->rfd_endptr < position)
	pFd->rfd_endptr = position;

    return (OK);
    }

/*******************************************************************************
*
* rt11FsDateSet - set the rt11Fs file system date
*
* This routine sets the date for the rt11Fs file system, which remains in
* effect until changed.  All files created are assigned this creation date.
*
* To set a blank date, invoke the command:
* .CS
*     rt11FsDateSet (72, 0, 0);    /@ a date outside RT-11's epoch @/
* .CE
*
* NOTE: No automatic incrementing of the date is performed; each new date must
* be set with a call to this routine.
*
* RETURNS: N/A
*/

void rt11FsDateSet
    (
    int year,           /* year (72...03 (RT-11's days are numbered)) */
    int month,          /* month (0, or 1...12) */
    int day             /* day (0, or 1...31) */
    )
    {
    rt11FsYear  = year % 100;
    rt11FsMonth = month;
    rt11FsDay   = day;
    }

/*******************************************************************************
*
* rt11FsVolFlush - flush RT-11 volume directory
*
* This routine guarantees that any changes to the volume directory entry
* are actually written to disk.
*
* Posession of the volume descriptor's semaphore must have been secured
* before this routine is called.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS rt11FsVolFlush
    (
    FAST RT_VOL_DESC    *vdptr          /* pointer to volume descriptor */
    )
    {
    FAST int		nSegBytes = vdptr->vd_nSegBlocks * RT_BYTES_PER_BLOCK;
    FAST char 		*psegTop = (char *) vdptr->vd_dir_seg;
					/* top of directory segment */

    vdptr->vd_status = OK;

    swab (psegTop, psegTop, nSegBytes);	/* swap bytes in entire segment */

    /* flush all blocks */

    if (rt11FsWrtBlock (vdptr, RT_DIR_BLOCK, vdptr->vd_nSegBlocks,
			psegTop) != OK)
	{
	vdptr->vd_status = ERROR;
	}

    swab (psegTop, psegTop, nSegBytes);	/* swap bytes in segment back */

    return (vdptr->vd_status);
    }

/*******************************************************************************
*
* rt11FsVolInit - initialize an RT-11 volume
*
* This routine initializes a disk volume to be an RT-11 disk.  The volume
* structures in memory are initialized, the volume's in-memory directory
* is initialized, and then the empty directory is flushed to the disk
* itself, by calling rt11FsVolFlush.
*
* The volume descriptor itself should have already been initialized with
* rt11FsDevInit().
*
* This routine does not format the disk; that must have already been done.
*
* RETURNS:
* OK, or
* ERROR if can't flush to device.
*/

LOCAL STATUS rt11FsVolInit
    (
    RT_VOL_DESC *vdptr          /* pointer to volume descriptor */
    )
    {
    STATUS status;
    FAST int freeBlocks;	/* number of free file blocks left */
    FAST RT_DIR_SEG *pseg = vdptr->vd_dir_seg;
    FAST int i = 0;

    semTake (vdptr->vd_semId, WAIT_FOREVER);

    /* initialize directory segment */

    pseg->ds_nsegs	= 1;
    pseg->ds_next_seg	= 0;
    pseg->ds_last_seg	= 1;
    pseg->ds_extra	= 0;

    /* first data block starts after the directory segment */

    pseg->ds_start	= RT_DIR_BLOCK + vdptr->vd_nSegBlocks;

    freeBlocks = vdptr->vd_nblocks - pseg->ds_start;

    /* make first directory entries be EMPTY covering entire remaining volume */

    /* first, make all files of size RT_MAX_BLOCKS_PER_FILE */

    while (freeBlocks > RT_MAX_BLOCKS_PER_FILE)
	{
	pseg->ds_entries[i].de_status = DES_EMPTY;
	pseg->ds_entries[i++].de_nblocks = RT_MAX_BLOCKS_PER_FILE;
	freeBlocks -= RT_MAX_BLOCKS_PER_FILE;
	}

    /* make last empty directory entry */

    if (freeBlocks > 0)
	{
	pseg->ds_entries[i].de_status = DES_EMPTY;
	pseg->ds_entries[i++].de_nblocks = freeBlocks;
	}

    /* make terminating entry */

    pseg->ds_entries[i].de_status = DES_END;

    status = rt11FsVolFlush (vdptr);		/* flush dir to disk */

    if (status == OK)
	rt11FsReadyChange (vdptr);		/* set up for remount */
    else
	vdptr->vd_status = ERROR;		/* could not flush */


    semGive (vdptr->vd_semId);			/* release volume */
    return (status);
    }

/*******************************************************************************
*
* rt11FsVolMount - prepare to use RT-11 volume
*
* This routine prepares the library to use the RT-11 volume on the device
* specified.  The volume directory segment is read from the disk.
* This routine should be called every time a disk is changed (i.e., a floppy
* is swapped, or whatever), or before every open and create, if the driver
* can't tell when disks are changed.
*
* Eventually this routine could be taught to read the home
* block to get more initialization info.
*
* Note:  If an error occurs while reading, it's possible that the
* bytes in the directory segment will be swapped.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS rt11FsVolMount
    (
    FAST RT_VOL_DESC    *vdptr          /* pointer to volume descriptor */
    )
    {
    FAST char		*pseg = (char *) vdptr->vd_dir_seg;
					/* pointer to directory segment */

    vdptr->vd_status = ERROR;

    /* read in all blocks of directory segment */

    if (rt11FsRdBlock (vdptr, RT_DIR_BLOCK, vdptr->vd_nSegBlocks, pseg) != OK)
	return (vdptr->vd_status);

    pseg = (char *) vdptr->vd_dir_seg;

    swab (pseg, pseg, vdptr->vd_nSegBlocks * RT_BYTES_PER_BLOCK);

    /* check reasonableness of directory segment */

    if (vdptr->vd_dir_seg->ds_nsegs    == 1	&&
	vdptr->vd_dir_seg->ds_next_seg == 0	&&
	vdptr->vd_dir_seg->ds_last_seg == 1	&&
	vdptr->vd_dir_seg->ds_extra    == 0)
	{
	vdptr->vd_status = OK;
	}

    return (vdptr->vd_status);
    }

/*******************************************************************************
*
* rt11FsWhere - tell where file is (current character position)
*
* This routine tells you where the file pointer is for a file.
*
* RETURNS:
* Current character position of file, or
* ERROR if invalid file descriptor.
*/

LOCAL int rt11FsWhere
    (
    FAST RT_FILE_DESC *pFd      /* file descriptor pointer */
    )
    {
    if (pFd->rfd_status == NOT_IN_USE)
	{
	errnoSet (S_rt11FsLib_FILE_NOT_FOUND);
	return (ERROR);
	}
    else
	return (pFd->rfd_newptr);
    }

/*******************************************************************************
*
* rt11FsWrite - write to an RT-11 file
*
* This routine writes to the file specified by the file descriptor
* (returned by rt11FsOpen() or rt11FsCreate()) from the specified <buffer>.
* If the block containing the disk locations to be written is already
* in memory the block won't be flushed.  If another in-memory block is
* needed, any block already in memory will be flushed.
*
* RETURNS:
* Number of bytes written (error if != nbytes), or
* ERROR if nbytes < 0, or no more space for the file,
* or can't write block.
*
*/

LOCAL int rt11FsWrite
    (
    FAST RT_FILE_DESC   *pFd,                   /* file descriptor pointer */
    char                *pBuf,                  /* data to be written */
    int                 nbytes                  /* number of bytes to write */
    )
    {
    FAST int		bytesCopied;
    FAST int		bufIndex;
    int			startBlk;		/* starting blk for write */
    FAST int		numBlks;		/* number of blocks to write */
    int			curBlk;			/* # of blk currently in buf */
    FAST int		bytesLeft = nbytes;	/* remaining bytes to write */

    /* check for valid nbytes */

    if (nbytes < 0)
	{
	errnoSet (S_rt11FsLib_INVALID_NUMBER_OF_BYTES);
	return (ERROR);
	}

    /* Write into successive blocks until all of caller's buffer written */

    while (bytesLeft > 0)
	{
    	/* Do direct whole-block write if possible
    	 *   Transfer must be >= 1 block, must start on a block
	 *   boundary, and must not extend past end-of-file.
    	 */

    	if ((bytesLeft >= RT_BYTES_PER_BLOCK) &&
	    (pFd->rfd_newptr % RT_BYTES_PER_BLOCK == 0) &&
	    (pFd->rfd_newptr + bytesLeft <= (pFd->rfd_dir_entry.de_nblocks *
					     RT_BYTES_PER_BLOCK)))
	    {
	    /* Determine max number of blocks to write */

    	    if (pFd->rfd_vdptr->vd_rtFmt)	/* if using std skew/intleave */
		numBlks = 1;			/*  blocks are not contiguous */
	    else
	    	numBlks = bytesLeft / RT_BYTES_PER_BLOCK;


	    /* Determine starting block for write */

	    startBlk = (pFd->rfd_newptr / RT_BYTES_PER_BLOCK) + pFd->rfd_start;


	    /* Write block(s) directly from caller's buffer */

	    if (rt11FsWrtBlock (pFd->rfd_vdptr, startBlk, numBlks, pBuf) != OK)
		return (ERROR);

	    /* If write included bufferred block, invalidate buffer */

	    curBlk = (pFd->rfd_curptr / RT_BYTES_PER_BLOCK) + pFd->rfd_start;

	    if ((curBlk >= startBlk)  && (curBlk < (startBlk + numBlks)))
		{
    		pFd->rfd_curptr	= NONE;
    		pFd->rfd_modified = FALSE;
		}

	    bytesCopied = numBlks * RT_BYTES_PER_BLOCK;
	    }
	else
	    {
	    /* get new pointer in buffer */

	    if (rt11FsNewBlock (pFd) <= 0)
	    	return (ERROR);

	    /* copy length = rest of caller's buffer or rest of fd buffer,
	     *   whichever is less.
	     */

	    bufIndex = pFd->rfd_newptr % RT_BYTES_PER_BLOCK;

	    bytesCopied = min (bytesLeft, RT_BYTES_PER_BLOCK - bufIndex);

	    bcopy (pBuf, &pFd->rfd_buffer [bufIndex], bytesCopied);

	    pFd->rfd_modified = TRUE;
	    }

	/* update pointers and bytes remaining */

	pFd->rfd_newptr += bytesCopied;

	if (pFd->rfd_endptr < pFd->rfd_newptr)
	    pFd->rfd_endptr = pFd->rfd_newptr;

	pBuf += bytesCopied;
	bytesLeft -= bytesCopied;
	}

    return (nbytes);
    }

/*******************************************************************************
*
* rt11FsNewBlock - make file descriptor block buffer contain new pointer
*
* This routine does whatever is necessary to make the block buffer
* in the specified file descriptor contain the byte addressed by
* the current pointer in the descriptor.  In particular, if on entry
* the buffer already contains the desired byte, then no action is taken.
* Otherwise if the buffer is modified (contains data written by user
* but not yet on disk) then the block is written.  Then the correct
* block is read if the mode is O_RDONLY or O_RDWR.
*
* RETURNS:
* number of bytes in buffer if successful, or
* 0 if end of file, or
* ERROR if unable to read/write block.
*/

LOCAL int rt11FsNewBlock
    (
    FAST RT_FILE_DESC *pFd      /* file descriptor pointer */
    )
    {
    FAST int new_block;
    int cur_block;

    /* calculate block num of desired new block and of current block if any */

    new_block = pFd->rfd_newptr / RT_BYTES_PER_BLOCK;

    cur_block = (pFd->rfd_curptr == NONE) ?
    			NONE : (pFd->rfd_curptr / RT_BYTES_PER_BLOCK);


    /* check for new block already in current buffer */

    if (new_block == cur_block)
	return (RT_BYTES_PER_BLOCK);


    /* check for end of file */

    if (new_block >= pFd->rfd_dir_entry.de_nblocks)
	return (0);


    /* flush current block, read in new block, update current pointer */

    if (rt11FsFlush (pFd) != OK)
	return (ERROR);

    if (pFd->rfd_mode != O_WRONLY)
	{
	if (rt11FsRdBlock (pFd->rfd_vdptr, new_block + pFd->rfd_start, 1,
		         pFd->rfd_buffer) != OK)
	    return (ERROR);
	}

    pFd->rfd_curptr = new_block * RT_BYTES_PER_BLOCK;

    return (RT_BYTES_PER_BLOCK);
    }

/*******************************************************************************
*
* rt11FsRdBlock - read a block from an RT-11 volume
*
* This routine reads the specified block from the specified volume.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS rt11FsRdBlock
    (
    FAST RT_VOL_DESC    *vdptr,         /* pointer to volume descriptor */
    int                 blockNum,       /* starting block for read */
    int                 numBlocks,      /* number of blocks to read */
    char                *pBuf           /* buffer to receive block(s) */
    )
    {
    FAST int		physSector;
    FAST int		secNum = blockNum * vdptr->vd_secBlock;
    FAST BLK_DEV	*pBlkDev = vdptr->vd_pBlkDev;
					/* pointer to block device struct */


    if (vdptr->vd_rtFmt)
	physSector = rt11FsAbsSector (vdptr->vd_pBlkDev->bd_blksPerTrack,
				    secNum);
    else
	physSector = secNum;


    /* Read each sector in block(s) */

    vdptr->vd_retry = 0;

    while (((* pBlkDev->bd_blkRd) (pBlkDev, physSector,
				   (numBlocks * vdptr->vd_secBlock),
				   pBuf)) != OK)
	{
	/* read error: reset volume and retry, up to device's retry limit */

	if (rt11FsReset (vdptr) != OK)
	    return (ERROR);		/* drive won't reset! */

        if (++(vdptr->vd_retry) > pBlkDev->bd_retry)
	    return (ERROR);
	}


    return (OK);
    }

/*******************************************************************************
*
* rt11FsWrtBlock - write a block to an RT-11 volume
*
* This routine writes the specified block to the specified volume.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS rt11FsWrtBlock
    (
    FAST RT_VOL_DESC    *vdptr,         /* pointer to volume descriptor */
    int                 blockNum,       /* starting block number for write */
    int                 numBlocks,      /* number of blocks to write */
    char                *pBuf           /* buffer containing data to write */
    )
    {
    FAST int		physSector;	/* physcial sector number */
    FAST int		secNum;		/* logical sector number */
    FAST BLK_DEV	*pBlkDev = vdptr->vd_pBlkDev;
					/* pointer to block device struct */


    /* Get sector number corresponding to starting block */

    secNum = blockNum * vdptr->vd_secBlock;

    if (vdptr->vd_rtFmt)
	physSector = rt11FsAbsSector (vdptr->vd_pBlkDev->bd_blksPerTrack,
				    secNum);
    else
	physSector = secNum;


    /* Write each sector in block(s) */

    vdptr->vd_retry = 0;

    while (((* pBlkDev->bd_blkWrt) (pBlkDev, physSector,
				    (numBlocks * vdptr->vd_secBlock),
				    pBuf)) != OK)
	{
	/* write error: reset volume and retry, up to device's retry limit */

	if (rt11FsReset (vdptr) != OK)
	    return (ERROR);		/* drive won't reset! */

        if (++(vdptr->vd_retry) > pBlkDev->bd_retry)
	    return (ERROR);		/* retry limit reached */
	}


    return (OK);
    }

/*******************************************************************************
*
* rt11FsAbsSector - determine absolute sector of logical sector
*
* This routine calculates the absolute sector number from the beginning of a
* medium using the RT-11 2:1 software interleave and 6 sector
* track-to-track skew.
*
* RETURNS: Absolute sector number of specified logical sector number.
*/

LOCAL int rt11FsAbsSector
    (
    FAST ULONG  secPerTrack,    /* sectors per track */
    FAST int    sector          /* logical sector number
                                 * to calculate */
    )
    {
    int		physical_sector;
    FAST int	track = sector / secPerTrack;
    FAST int	trk_sector = sector % secPerTrack;

    /* convert to actual offset by interleave and skew factors */

    physical_sector = ((trk_sector * 2) + (track * 6)) % secPerTrack;

    if (trk_sector >= (secPerTrack / 2))
	physical_sector++;


    /* calculate absolute sector from start of disk */

    return (track * secPerTrack + physical_sector);
    }

/*******************************************************************************
*
* rt11FsCheckVol - verify that volume descriptor is current
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS rt11FsCheckVol
    (
    FAST RT_VOL_DESC *vdptr,    /* pointer to device descriptor */
    BOOL doMount                /* mount the device, if necessary */
    )
    {

    /* Check if device driver announced ready-change */

    if (vdptr->vd_pBlkDev->bd_readyChanged)
	{
	vdptr->vd_state = RT_VD_READY_CHANGED;
	vdptr->vd_pBlkDev->bd_readyChanged = FALSE;
	}


    /* Check volume state */

    switch (vdptr->vd_state)
	{
	case RT_VD_CANT_RESET:
	    /* this state means we tried to reset and failed so we
	     * don't try again until a ready change occurs */

	    return (ERROR);	/* can't reset */

	case RT_VD_CANT_MOUNT:
	    /* this state means we tried to mount and failed so we
	     * don't try again until a ready change occurs */

	    if (doMount)
		return (ERROR);	/* can't mount */

	    break;		/* caller didn't want a mount anyway */


	case RT_VD_READY_CHANGED:
	    /* ready change occurred; try to reset volume */

	    if (rt11FsReset (vdptr) != OK)
		return (ERROR);				/* can't reset */

	    vdptr->vd_state = RT_VD_RESET;		/* note volume reset */

	    /* fall-through to try to mount */

	case RT_VD_RESET:
	    /* volume reset but not mounted; if caller requested, try mount */

	    if (doMount)
		{
		if (rt11FsVolMount (vdptr) != OK)
		    {
		    vdptr->vd_state = RT_VD_CANT_MOUNT;	/* don't try again */
		    return (ERROR);			/* can't mount */
		    }

		vdptr->vd_state = RT_VD_MOUNTED;	/* note vol mounted */
		}

	    break;


	case RT_VD_MOUNTED:
	    break;
	}

    return (OK);	/* caller got what he wanted */
    }

/*******************************************************************************
*
* rt11FsReset - reset a volume
*
* This routine calls the specified volume's reset routine.
* If the reset fails, vd_state is set to (RT_VD_CANT_RESET)
* to indicate that disk won't reset.
*/

LOCAL STATUS rt11FsReset
    (
    RT_VOL_DESC         *vdptr          /* pointer to device descriptor */
    )
    {
    FAST BLK_DEV	*pBlkDev = vdptr->vd_pBlkDev;
					/* pointer to block device struct */


    if (pBlkDev->bd_reset != NULL)
	{
	if ((* pBlkDev->bd_reset) (pBlkDev) != OK)
	    {
	    vdptr->vd_state = RT_VD_CANT_RESET;
	    return (ERROR);
	    }
	}

    return (OK);
    }

/*******************************************************************************
*
* rt11FsVolMode - inquire the current volume mode (O_RDONLY/O_WRONLY/O_RDWR)
*/

LOCAL STATUS rt11FsVolMode
    (
    FAST RT_VOL_DESC *vdptr     /* pointer to device descriptor */
    )
    {
    return (vdptr->vd_pBlkDev->bd_mode);
    }

/*******************************************************************************
*
* rt11FsReadyChange - notify rt11Fs of a change in ready status
*
* This routine sets the volume descriptor state to RT_VD_READY_CHANGED.
* It should be called whenever a driver senses that a device has come on-line
* or gone off-line (e.g., a disk has been inserted or removed).
*
* RETURNS: N/A
*/

void rt11FsReadyChange
    (
    RT_VOL_DESC *vdptr  /* pointer to device descriptor */
    )
    {
    vdptr->vd_state = RT_VD_READY_CHANGED;
    }

/*******************************************************************************
*
* rt11FsModeChange - modify the mode of an rt11Fs volume
*
* This routine sets the volume descriptor mode to <newMode>.
* It should be called whenever the read and write capabilities are determined,
* usually after a ready change.  See the manual entry for rt11FsReadyChange().
*
* The rt11FsDevInit() routine initially sets the mode to O_RDWR,
* (e.g., both O_RDONLY and O_WRONLY).
*
* RETURNS: N/A
*
* SEE ALSO: rt11FsDevInit(), rt11FsReadyChange()
*/

void rt11FsModeChange
    (
    RT_VOL_DESC *vdptr, /* pointer to volume descriptor */
    int newMode         /* O_RDONLY, O_WRONLY, or O_RDWR (both) */
    )
    {
    vdptr->vd_pBlkDev->bd_mode = newMode;
    }

/*******************************************************************************
*
* rt11FsSqueeze - reclaim fragmented free space on RT-11 volume
*
* This routine moves data around on an RT-11 volume so that all the
* little bits of free space left on the device are joined together.
*
* CAVEAT:
* There MUST NOT be any files open on the device when this procedure
* is called - if there are then their condition after the call will
* be unknown - almost certainly damaged, and writing to a file may
* destroy the entire disk.
*/

LOCAL STATUS rt11FsSqueeze
    (
    FAST RT_VOL_DESC *vdptr     /* pointer to volume descriptor */
    )
    {
#define	NUM_BLOCKS	16

    FAST int rdptr;		/* number of next directory entry to read */
    FAST int wdptr;		/* number of next directory entry to write */
    FAST int rbptr;		/* number of next block to read */
    FAST int wbptr;		/* number of next block to write */
    int nblk;			/* number of blocks in file */
    int numToRead;		/* number of blocks to transfer in this pass */
    int mtcount;		/* number of empty blocks on device */
    RT_DIR_ENTRY dirEntry;	/* directory entry for current file */
    FAST char *buffer = (char *) malloc (RT_BYTES_PER_BLOCK * NUM_BLOCKS);

    if (buffer == NULL)
	return (ERROR);		/* oops - couldn't do it */

    semTake (vdptr->vd_semId, WAIT_FOREVER);

    rdptr = wdptr = 0;		/* start at first directory entry */
    rbptr = wbptr = vdptr->vd_dir_seg->ds_start;
				/* blocks start at first one after directory */

    if ((rt11FsCheckVol (vdptr, TRUE) != OK) || (vdptr->vd_status != OK))
	{
	semGive (vdptr->vd_semId);
	errnoSet (S_rt11FsLib_VOLUME_NOT_AVAILABLE);
	free (buffer);
	return (ERROR);
	}

    if (rt11FsVolMode (vdptr) == O_RDONLY)	/* check it's not write protected */
	{
	semGive (vdptr->vd_semId);
	errnoSet (S_ioLib_WRITE_PROTECTED);
	free (buffer);
	return (ERROR);
	}

    mtcount = 0;			/* initially no empty blocks */

    /* now loop through the entire directory till we hit the end */

    while (rt11FsGetEntry (vdptr, rdptr++, &dirEntry),
	   dirEntry.de_status != DES_END)
	{
	if (dirEntry.de_status == DES_PERMANENT)
	    {				/* we got an active file */
	    rt11FsPutEntry (vdptr, wdptr++, &dirEntry);
					/* write it back */
	    nblk = dirEntry.de_nblocks;

	    while (nblk)		/* loop till all blocks transferred */
		{
		numToRead = min (nblk, NUM_BLOCKS);
		nblk -= numToRead;	/* read full buffer or file size */

		if (rbptr != wbptr)
		    {
		    /* only move if it's at a different place */

		    /* read another buffers worth */

		    (void) rt11FsRdBlock (vdptr, rbptr, numToRead, buffer);

		    /* write it back */

		    (void) rt11FsWrtBlock (vdptr, wbptr, numToRead, buffer);
		    }

		rbptr += numToRead;
		wbptr += numToRead;
		}
	    }
	else
	    {
	    /* not active: just update block read pointer & empty block count */

	    rbptr += dirEntry.de_nblocks;
	    mtcount += dirEntry.de_nblocks;
	    }
	}

    dirEntry.de_status = DES_EMPTY;
    dirEntry.de_nblocks = mtcount;		/* set count of empty blocks */
    rt11FsPutEntry (vdptr, wdptr++, &dirEntry);	/* write DES_EMPTY entry */

    dirEntry.de_status = DES_END;
    rt11FsPutEntry(vdptr, wdptr, &dirEntry);	/* write DES_END to terminate */
    (void) rt11FsVolFlush(vdptr);			/* flush volume */

    semGive (vdptr->vd_semId);			/* and release it */
    free (buffer);				/* and release the memory */

    return (OK);
    }

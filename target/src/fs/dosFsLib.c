/* dosFsLib.c - MS-DOS media-compatible file system library */ 

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02q,02may02,jkf  Corrects SPR#76501, 72603. Avoids 65085 and 33221.
                 and a performance improvement for FIOSYNC.
02p,30apr02,jkf  SPR#62786, rename should preserve time and date fields
02o,30apr02,jkf  SPR#76510, dosFsVolDescGet() should return NULL instead of
                 the default device when the underlying iosDevFind() does. 
02n,15Jan02,chn  SPR#29751, removed path case sensitivity during creat phase 
                 of rename. Possible NFS interaction, see comments at change.
02m,18dec01,chn  SPR#71105, correct file size in dosFsContigAlloc & comment
02l,12dec01,jkf  SPR#72133, add FIOMOVE and fixing diab build warnings.
02k,10dec01,jkf  SPR#24332, dont delete existing files or dirs in dosFsRename
02j,10dec01,jkf  SPR#72039, various fixes from Mr. T. Johnson.
02i,09dec01,jkf  SPR#71637, fix for SPR#68387 caused ready changed bugs.
02h,30nov01,jkf  SPR#68203, updating last access date on open-read-close
                 operation causes unwanted writes, increasing flash wear.
02g,30nov01,jkf  SPR#33343, media unformatted or missing err better reported.
                 SPR#69074, statfs to invalid media caused continuous errors.
02f,15nov01,jkf  SPR#71720, avoid unaligned pointer access.
                 clean up multiple errno's when mounting unformatted volume.
02e,13nov01,jkf  correct typo in checkin comment.
02d,13nov01,jkf  improve dosFsBootSecGet() error message, add a comment about 
                 nBlocks reporting total blocks, not last addressable block.
02c,10nov01,jkf  SPR#67890, chkdsk writes to RDONLY device
02b,09nov01,jkf  SPR#71633, dont set errno when DevCreate is called w/BLK_DEV
                 SPR#32178, made dosFsVolDescGet public, cleaned up man page. 
                 SPR#33684, dosFsShow reporting incorrect verbosity setting.
02a,23oct01,rip  SPR#65085: tasks in FIOSYNC interlocking (dup of #33221 cf 01v)
                 SPR#30464: FIONCONTIG64 not shifting using 64bit math
                 SPR#30540: FillGap assumes step over sector on FIOSEEK > EOF
01z,21aug01,jkf  SPR#69031, common code for both AE & 5.x.
01y,26jul01,jkf  T2 SPR#65009, T3 SPR#65271.  dosFsLibInit returns OK or ERROR.
01x,14mar01,jkf  SPR#34704,FAT12/FAT16 determination, SPR#62415 sig location.
01w,19sep00,csh  Manual Merge From Alameda Branch To Sunnyvale Branch
01v,08sep00,nrj  fixed SPR#33221, to avoid dead-lock because of multiple tasks
		 doing FIOSYNC on opened files
		 fixed SPR#33702, 33684, The autocheck verbosity is now 
		 initialized properly in volume descriptor.
01u,29feb00,jkf  cleaning warning.

01t,29feb00,jkf  T3 changes.
01s,28sep99,jkf  fixed SPR#28554, now return error on write to full disk.
01r,31aug99,jkf  changes for new CBIO API.  Changed FIOTIMESET to allow
                 utime to follow Solaris 2.6 behavior.  If arg is NULL, 
                 the current time is used, added docs. SPR#28924
01q,06aug99,jkf  delete existing file when dosFsOpen uses O_CREAT/O_TRUNC
                 do not overflow read buffer on truncated files, SPR#28309
01p,31jul99,jkf  Dont allow larger than 4GB file on anything but
                 VXLONGNAMES directory entry files. SPR#27532.
01o,31jul99,jkf  Added support for FSTYPE (0x36) in bootsec, SPR#28273
01n,31jul99,jkf  T2 merge, tidiness & spelling.
01m,03dec98,vld  fixed SPR #23692: added FIOTRUNC64 ioctl code;
                 rd/wr time measurement excluded
01l,22nov98,vld  included  features required by NFS protocol:
		 - added support for FIOFSTATFSGET ioctl code;
		 - added support for creating files "with holes";
		 - added dosFsSeekDir() routine and controlling
		   of dd_cookie field within FIOREADDIR
01k,28sep98,vld  gnu extensions dropped from DBG_MSG/ERR_MSG macros
01j,24sep98,vld  added support for FIOTIMESET ioctl code
01i,16sep98,vld  created separate routine dosFsChkDsk() to solve
hen-and-egg problem during volume mounting and
                 external call for disk check operation.
01h,16sep98,vld  added support for read only devices
01j,11sep98,vld  added support for non CBIO ptr argument in dosFsDevCreate().
01i,26aug98,vld  ignore mode = S_IFDIR except with O_CREAT (SPR#22227)
01h,30jul98,wlf  partial doc cleanup
01g,27jul98,vld  fixed FIOWHERE64  return
01f,08jul98,vld  print64Lib.h moved to h/private directory. 
01e,08jul98,vld  dosFsContigAlloc() (FIOCONTIG effected) changed
                 not to zero allocated data and leave file size as 0
01d,08jul98,vld  fixed bug in dosFsContigAlloc() 
                 added counting sectors per file count for CONTIG_MAX case.
01c,30jun98,lrn  renamed dosFsInit to dosFsLibInit
01b,28jun98,vld  tested, checked in, ready for EAR
01a,18jan98,vld  written, preliminary
*/

/*
INTERNAL: MS-DOS is a registered trademark of Microsoft Corporation.

DESCRIPTION
This library implements the MS-DOS compatible file system.
This is a multi-module library, which depends on sub-modules to
perform certain parts of the file system functionality.
A number of different file system format variations are supported.

USING THIS LIBRARY
The various routines provided by the VxWorks DOS file system (dosFs) may be
separated into three broad groups: general initialization, device
initialization, and file system operation.

The dosFsLibInit() routine is the principal initialization function; it should
be called once during system initialization, regardless of how many dosFs
devices are to be used.

Another dosFs routine is used for device initialization. 
For each dosFs device, dosFsDevCreate() must be called to install the
device in VxWorks device list.
In the case where partitioned disks are used, dosFsDevCreate() must be
called for each partition that is anticipated, thereby it is associated
with a logical device name, so it can be later accessed via the I/O
system.

In case of a removable disk, dosFsDevCreate() must be called during
system initialization time, even if a cartridge or diskette may be
absent from the drive at boot time. dosFsDevCreate() will only
associate the device with a logical device name. Device access will be
done only when the logical device is first accessed by the application.

More detailed information on all of these routines is provided below.

INITIALIZING DOSFSLIB
To enable this file system in a particular VxWorks configuration,
a library initialization routine must be called for each sub-module of
the file system, as well as for the underlying disk cache, partition
manager and drivers.
This is usually done at system initialization time, within the 
.I usrRoot
task context.

Following is the list of initialization routines that need to be
called:
.IP dosFsLibInit
(mandatory) initialize the principle dosFs module. Must be called first.
.IP dosFsFatInit
(mandatory) initialize the File Allocation Table handler, which supports
12-bit, 16-bit and 32-bit FATs.
.IP dosVDirLibInit
(choice) install the variable size directory handler
supporting Windows-compatible Long File Names (VFAT) Directory
Handler.
.IP dosDirOldLibInit
(choice) install the fixed size  directory handler
which supports old-fashioned 8.3
MS-DOS file names, and Wind River Systems proprietary long file names
(VXLONG).
.IP dosFsFmtLibInit
(optional) install the volume formatting module.
.IP dosChkLibInit
(optional) install the file system consistency checking module.
.LP
The two Directory handlers which are marked
.I choice
are installed in accordance with the system requirements, either one
of these modules could be installed or both, in which case the VFAT will
take precedence for MS-DOS compatible volumes.

Also, at least one
.I CBIO
module must be initialized on a per-device basis prior to calling
dosFsDevCreate().
See the related documentation for more details and examples.

DEFINING A DOSFS DEVICE
The dosFsDevCreate() routine associates a device with the dosFsLib
functions.  It expects three parameters:
.IP "(1)" 4
A pointer to a name string, to be used to identify the device - logical
device name.
This will be part of the pathname for I/O operations which operate on the
device.  This name will appear in the I/O system device table, which may be
displayed using the iosDevShow() routine.
.IP "(2)"
CBIO_DEV_ID - a pointer to the CBIO_DEV structure which provides interface
to particular disk, via a disk cache, or a partition manager or a
combination of a number of
.I CBIO
modules which are stacked on top of each other to form one of many
configurations possible.
.IP "(3)"
A maximum number of files can be simultaneously opened on a particular device.
.IP "(4)"
Because volume integrity check utility can be automatically
invoked every time a device is mounted,
this parameter indicates whether the consistency check needs to be
performed automatically on a given device, and on what level of
verbosity is required.
In any event, the consistency check may be invoked at a later time
e.g. by calling chkdsk().
See description for FIOCHKDSK ioctl command for more information.
.LP
For example:
.CS
    dosFsDevCreate
	(
	"/sd0",		/@ name to be used for volume   @/
	pCbio,		/@ pointer to device descriptor @/
	10,		/@ max no. of simultaneously open files @/
	DOS_CHK_REPAIR | DOS_CHK_VERB_1
			/@ check volume during mounting and repair @/
			/@ errors, and display volume statistics @/
	)
.CE

Once dosFsDevCreate() has been called, the device can be accessed
using 
.I ioLib
generic I/O routines: open(), read(), write(), close(),
ioctl(), remove(). Also, the user-level utility functions may be used to
access the device at a higher level (See usrFsLib reference page for
more details).

DEVICE AND PATH NAMES
On true MS-DOS machines, disk device names are typically of the form "A:",
that is, a single letter designator followed by a colon.  Such names may be
used with the VxWorks dosFs file system.  However, it is possible (and
desirable) to use longer, more mnemonic device names, such as "DOS1:",
or "/floppy0". 
The name is specified during the dosFsDevCreate() call.

The pathnames used to specify dosFs files and directories may use either
forward slashes ("/") or backslashes ("\e") as separators.  These may be
freely mixed.  The choice of forward slashes or backslashes has absolutely
no effect on the directory data written to the disk.  (Note, however, that
forward slashes are not allowed within VxWorks dosFs filenames, although
they are normally legal for pure MS-DOS implementations.)

For the sake of consistency however use of forward slashes ("/") is
recommended at all times.

The leading slash of a dosFs pathname following the device name is
optional.  For example, both "DOS1:newfile.new" and "DOS1:/newfile.new"
refer to the same file.

USING EXTENDED DIRECTORY STRUCTURE
This library supports DOS4.0 standard file names which fit the restrictions
of eight upper-case characters optionally followed by a three-character
extension,
as well as Windows style VFAT standard long file names
that are stored mixed cased on disk, but are case insensitive when
searched and matched (e.g. during open() call).
The VFAT long file name is stored in a variable number of consecutive
directory entries.
Both standards restrict file size to 4 GB (32 bit value).

To provide additional flexibility, this implementation of the
DOS file system provides proprietary ling file name format (VXLONGNAMES),
which uses a simpler directory structure: the directory entry is
of fixed size.  When this option is
used, file names may consist of any sequence of up to 40 ASCII
characters.  No case conversion is performed, 
and file name match is case-sensitive.
With this directory format the
file maximum size is expanded to 1 Terabyte (40 bit value).

.RS 4 4
NOTE:  Because special directory entries are used on the disk, disks 
which use the extended names are 
.I not
compatible with other implementation of the
MS-DOS systems, and cannot be read on MS-DOS or Windows machines.
.RE

To enable the extended file names, set the DOS_OPT_VXLONGNAMES flag 
when calling dosFsVolFormat().

READING DIRECTORY ENTRIES
Directories on VxWorks dosFs volumes may be searched using the opendir(),
readdir(), rewinddir(), and closedir() routines.  These calls allow the
names of files and subdirectories to be determined.

To obtain more detailed information about a specific file, use the fstat()
or stat() routine.  Along with standard file information, the structure
used by these routines also returns the file attribute byte from a dosFs
directory entry.

For more information, see the manual entry for dirLib.

FILE DATE AND TIME
Directory entries on dosFs volumes contain creation, last modification
time and date, and the last access date for each file or subdirectory.
Directory last modification time and date fields are set only when 
a new entry is created, but not when any directory entries are deleted.
The last access date field indicates the date of the last read or write.  
The last access date field is an optional field, per Microsoft.  By 
default, file open-read-close operations do not update the last access 
date field.  This default avoids media writes (writing out the date field)
during read only operations.   In order to enable the updating of the 
optional last access date field for open-read-close operations, you must 
call dosFsLastAccessDateEnable(), passing it the volumes DOS_VOLUME_DESC_ID 
and TRUE.

The dosFs file system uses the ANSI time() function, that returns
system clock value to obtain date and time.  It is recommended that the
target system should set the system time during system initialization
time from a network server or from an embedded Calendar / Clock
hardware component, so that all files on the file system would be
associated with a correct date and time.

The file system consistency checker (see below) sets system clock to
value following the latest date-time field stored on the disk, if it
discovers, that function time() returns a date earlier then Jan 1,
1998, meaning that the target system does not have a source of valid
date and time to synchronize with.

See also the reference manual entry for ansiTime.

FILE ATTRIBUTES
Directory entries on dosFs volumes contain an attribute byte consisting
of bit-flags which specify various characteristics of the entry.  The
attributes which are identified are:  read-only file, hidden file,
system file, volume label, directory, and archive.  The VxWorks symbols
for these attribute bit-flags are:

.IP DOS_ATTR_RDONLY
File is write-protected, can not be modified or deleted.
.IP DOS_ATTR_HIDDEN
this attribute is not used by VxWorks.
.IP DOS_ATTR_SYSTEM
this attribute is not used by VxWorks.
.IP DOS_ATTR_VOL_LABEL
directory entry describes a volume label,
this attribute can not be set or used directly, see ioctl() command
FIOLABELGET and FIOLABELSET below for volume label manipulation.
.IP DOS_ATTR_DIRECTORY
directory entry is a subdirectory,
this attribute can not be set directly.
.IP DOS_ATTR_ARCHIVE
this attribute is not used by VxWorks.

.LP
All the flags in the attribute byte, except the directory and volume label
flags, may be set or cleared using the ioctl() FIOATTRIBSET function.  This
function is called after opening the specific file whose attributes are to
be changed.  The attribute byte value specified in the FIOATTRIBSET call is
copied directly.  To preserve existing flag settings, the current attributes
should first be determined via fstat(), and the appropriate
flag(s) changed using bitwise AND or OR operations.  For example, to make
a file read-only, while leaving other attributes intact:

.CS
    struct stat fileStat;

    fd = open ("file", O_RDONLY, 0);     /@ open file          @/
    fstat (fd, &fileStat);               /@ get file status    @/

    ioctl (fd, FIOATTRIBSET, (fileStat.st_attrib | DOS_ATTR_RDONLY));
                                         /@ set read-only flag @/
    close (fd);                          /@ close file         @/
.CE
.LP
See also the reference manual entry for attrib() and xattrib() for
user-level utility routines which control the attributes of files or
file hierarchy.

CONTIGOUS FILE SUPPORT
The VxWorks dosFs file system provides efficient files storage:
space will be allocated in groups of clusters (also termed 
.I extents
) so that a file will be composed of relatively large contiguous units.
This  nearly contiguous allocation technique is designed to
effectively eliminate the effects of disk space fragmentation,
keeping throughput very close to the maximum of which the hardware is
capable of.

However dosFs provides mechanism to allocate truly contiguous files,
meaning files which are made up of a consecutive series of disk sectors.
This support includes both the ability to allocate contiguous space to a file
and optimized access to such a file when it is used.
Usually this will somewhat improve performance when compared to
Nearly Contiguous allocation, at the price of disk space fragmentation.

To allocate a contiguous area to a file, the file is first created in the
normal fashion, using open() or creat().  The file descriptor returned
during the creation of the file is then used to make an ioctl() call,
specifying the FIOCONTIG or FIOCONTIG64 function.
The last parameter to the FIOCONTIG function is the size of the requested
contiguous area in bytes, If the FIOCONTIG64 is used, the last parameter
is pointer to 64-bit integer variable, which contains the required file size.
It is also possible to request that the largest contiguous free area on
the disk be obtained.  In this case, the size value CONTIG_MAX (-1) 
is used instead of an actual size.  These ioctl() codes
are not supported for directories.
The volume is searched for a contiguous area of free space, which 
is assigned to the file. If a segment of contiguous free space
large enough for the request was not found,
ERROR is returned, with <errno> set to  S_dosFsLib_NO_CONTIG_SPACE.

When contiguous space is allocated to a file, the file remains empty,
while the newly allocated space has not been initialized.
The data should be then written to the file, and eventually, when
all data has been written, the file is closed.
When file is closed, its space is truncated to reflect the amount
of data actually written to the file.
This file may then be again opened and used for further 
I/O operations read() or write(), 
but it can not be guaranteed that appended data will be contiguous
to the initially written data segment.

For example, the following will create a file and allocate 85 Mbytes of 
contiguous space:
.CS
    fd = creat ("file", O_RDWR, 0);             /@ open file             @/
    status = ioctl (fd, FIOCONTIG, 85*0x100000);/@ get contiguous area   @/
    if (status != OK)
       ...                                      /@ do error handling     @/
    close (fd);                                 /@ close file            @/
.CE

In contrast, the following example will create a file and allocate the
largest contiguous area on the disk to it:

.CS
    fd = creat ("file", O_RDWR, 0);             /@ open file             @/
    status = ioctl (fd, FIOCONTIG, CONTIG_MAX); /@ get contiguous area   @/
    if (status != OK)
       ...                                      /@ do error handling     @/
    close (fd);                                 /@ close file            @/
.CE

.IP NOTE
the FIOCONTIG operation should take place right after the file has been
created, before any data is written to the file.
Directories may not be allocated a contiguous disk area.
.LP
To determine the actual amount of contiguous space obtained when CONTIG_MAX
is specified as the size, use fstat() to examine the number of blocks
and block size for the file.

When any file is opened, it may be checked for contiguity.
Use the extended flag DOS_O_CONTIG_CHK when calling open() to access an
existing file which may have been allocated contiguous space.
If a file is detected as contiguous, all subsequent operations on the
file will not require access to the File Allocation Table, thus
eliminating any disk Seek operations.
The down side however is that if this option is used, open() will take
an amount of time which is linearly proportional of the file size.

CHANGING, UNMOUNTING, AND SYNCHRONIZING DISKS
Buffering of disk data in RAM, synchronization of these
buffers with the disk and detection of removable disk replacement are
all handled by the disk cache. See reference manual on dcacheCbio
for more details.

If a disk is physically removed, the disk cache will cause dosFsLib to
.I unmount
the volume, which will mark all currently open file descriptors as
.I obsolete.

If a new disk is inserted, it will be automatically
.I mounted
on the next call to open() or creat().

IOCTL FUNCTIONS
The dosFs file system supports the following ioctl() functions.  The
functions listed are defined in the header ioLib.h.  Unless stated
otherwise, the file descriptor used for these functions may be any file
descriptor which is opened to a file or directory on the volume or to 
the volume itself.
There are some ioctl() commands, that expect a 32-bit integer result
(FIONFREE, FIOWHERE, etc.).
However, disks and files with are grater than 4GB are supported.
In order to solve this problem, new ioctl() functions have been added
to support 64-bit integer results.
They have the same name as basic functions, but with suffix 
.I 64,
namely: FIONFREE64, FIOWHERE64 and so on. These commands
expect a pointer to a 64-bit integer, i.e.:
.CS
long long *arg ;
.CE
as the 3rd argument to the ioctl() function.
If a value which is requested with a 32-bit ioctl() command is
too large to be represented in the 32-bit variable, ioctl() will return
ERROR, and <errno> will be set to S_dosFsLib_32BIT_OVERFLOW.

.iP "FIODISKINIT"
Reinitializes a DOS file system on the disk volume.
This function calls dosFsVolFormat() to format the volume,
so dosFsFmtLib must be installed for this to work.
Third argument of ioctl() is passed as argument <opt> to
dosFsVolFormat() routine.
This routine does not perform a low level format,
the physical media is expected to be already formatted.
If DOS file system device has not been created yet for a particular device,
only direct call to dosFsVolFormat() can be used.
.CS
    fd = open ("DEV1:", O_WRONLY);
    status = ioctl (fd, FIODISKINIT, DOS_OPT_BLANK);
.CE

.iP "FIODISKCHANGE"
Announces a media change. No buffers flushing is performed.
This function may be called from interrupt level:
.CS
    status = ioctl (fd, FIODISKCHANGE, 0);
.CE

.iP "FIOUNMOUNT"
Unmounts a disk volume.  It performs the same function as dosFsVolUnmount().
This function must not be called from interrupt level:
.CS
    status = ioctl (fd, FIOUNMOUNT, 0);
.CE

.iP "FIOGETNAME"
Gets the file name of the file descriptor and copies it to the buffer <nameBuf>.
Note that <nameBuf> must be large enough to contain the largest possible
path name, which requires at least 256 bytes.
.CS
    status = ioctl (fd, FIOGETNAME, &nameBuf );
.CE

.iP "FIORENAME"
Renames the file or directory to the string <newname>:
.CS
    fd = open( "oldname", O_RDONLY, 0 );
    status = ioctl (fd, FIORENAME, "newname");
.CE

.iP "FIOMOVE"
Moves the file or directory to the string <newname>:
.CS
    fd = open( "oldname", O_RDONLY, 0 );
    status = ioctl (fd, FIOMOVE, "newname");
.CE

.iP "FIOSEEK"
Sets the current byte offset in the file to the position specified by
<newOffset>. This function supports offsets in 32-bit value range.
Use FIOSEEK64 for larger position values:
.CS
    status = ioctl (fd, FIOSEEK, newOffset);
.CE

.iP "FIOSEEK64"
Sets the current byte offset in the file to the position specified by
<newOffset>. This function supports offsets in 64-bit value range:
.CS
    long long	newOffset;
    status = ioctl (fd, FIOSEEK64, (int) & newOffset);
.CE

.iP "FIOWHERE"
Returns the current byte position in the file.  This is the
byte offset of
the next byte to be read or written.  This function returns a 32-bit value.
It takes no additional argument:
.CS
    position = ioctl (fd, FIOWHERE, 0);
.CE

.iP "FIOWHERE64"
Returns the current byte position in the file.  This is the
byte offset of
the next byte to be read or written.  This function returns a 64-bit
value in <position>:
.CS
    long long	position;
    status = ioctl (fd, FIOWHERE64, (int) & position);
.CE

.iP "FIOFLUSH"
Flushes disk cache buffers.  It guarantees that any output that has
been requested is actually written to the device:
.CS
    status = ioctl (fd, FIOFLUSH, 0);
.CE

.iP "FIOSYNC"
Updates the FAT copy for the passed file descriptor, then      
flushes and invalidates the CBIO cache buffers for the file    
descriptor's volume.  FIOSYNC ensures that any outstanding     
output requests for the passed file descriptor are written     
to the device and a subsequent I/O operation will fetch data   
directly from the physical medium.  To safely sync a volume    
for shutdown, all open file descriptor's should at the least   
be FIOSYNC'd by the application.  Better, all open FD's should 
be closed by the application and the volume should be unmounted
via FIOUNMOUNT.
.CS
    status = ioctl (fd, FIOSYNC, 0);
.CE

.iP "FIOTRUNC"
Truncates the specified file's length to <newLength> bytes.  Any disk
clusters which had been allocated to the file but are now unused are
deallocated, and the directory entry for the file is updated to reflect
the new length.  Only regular files may be truncated; attempts to use
FIOTRUNC on directories will return an error.
FIOTRUNC may only be used to make files shorter; attempting to specify
a <newLength> larger than the current size of the file produces an
error (setting errno to S_dosFsLib_INVALID_NUMBER_OF_BYTES).
.CS
    status = ioctl (fd, FIOTRUNC, newLength);
.CE

.iP "FIOTRUNC64"
Similar to FIOTRUNC, but can be used for files lager, than 4GB.
.CS
    long long newLength = .....;
    status = ioctl (fd, FIOTRUNC, (int) & newLength);
.CE  

.iP "FIONREAD"
Copies to <unreadCount> the number of unread bytes in the file:
.CS
    unsigned long unreadCount;
    status = ioctl (fd, FIONREAD, &unreadCount);
.CE
.iP "FIONREAD64"
Copies to <unreadCount> the number of unread bytes in the file.
This function returns a 64-bit integer value:
.CS
    long long unreadCount;
    status = ioctl (fd, FIONREAD64, &unreadCount);
.CE
.iP "FIONFREE"
Copies to <freeCount> the amount of free space, in bytes, on the volume:
.CS
   unsigned long freeCount;
   status = ioctl (fd, FIONFREE, &freeCount);
.CE
.iP "FIONFREE64"
Copies to <freeCount> the amount of free space, in bytes, on the volume.
This function can return value in 64-bit range:
.CS
   long long freeCount;
   status = ioctl (fd, FIONFREE64, &freeCount);
.CE
.iP "FIOMKDIR"
Creates a new directory with the name specified as <dirName>:
.CS
    status = ioctl (fd, FIOMKDIR, "dirName");
.CE
.iP "FIORMDIR"
Removes the directory whose name is specified as <dirName>:
.CS
    status = ioctl (fd, FIORMDIR, "dirName");
.CE
.iP "FIOLABELGET"
Gets the volume label (located in root directory) and copies the string to
<labelBuffer>. If the label contains DOS_VOL_LABEL_LEN significant
characters, resulting string  is not NULL terminated:
.CS
    char	labelBuffer [DOS_VOL_LABEL_LEN];
    status = ioctl (fd, FIOLABELGET, (int)labelBuffer);
.CE
.iP "FIOLABELSET"
Sets the volume label to the string specified as <newLabel>.  The string may
consist of up to eleven ASCII characters:
.CS
    status = ioctl (fd, FIOLABELSET, (int)"newLabel");
.CE
.iP "FIOATTRIBSET"
Sets the file attribute byte in the DOS directory entry to the new value
<newAttrib>.  The file descriptor refers to the file whose entry is to be 
modified:
.CS
    status = ioctl (fd, FIOATTRIBSET, newAttrib);
.CE
.iP "FIOCONTIG"
Allocates contiguous disk space for a file or directory.  The number of
bytes of requested space is specified in <bytesRequested>.  In general,
contiguous space should be allocated immediately after the file is
created:
.CS
    status = ioctl (fd, FIOCONTIG, bytesRequested);
.CE
.iP "FIOCONTIG64"
Allocates contiguous disk space for a file or directory.  The number of
bytes of requested space is specified in <bytesRequested>.  In general,
contiguous space should be allocated immediately after the file is
created. This function accepts a 64-bit value:
.CS
    long long bytesRequested;
    status = ioctl (fd, FIOCONTIG64, &bytesRequested);
.CE
.iP "FIONCONTIG"
Copies to <maxContigBytes> the size of the largest contiguous free space, 
in bytes, on the volume:
.CS
    status = ioctl (fd, FIONCONTIG, &maxContigBytes);
.CE
.iP "FIONCONTIG64"
Copies to <maxContigBytes> the size of the largest contiguous free space,
in bytes, on the volume. This function returns a 64-bit value:
.CS
    long long maxContigBytes;
    status = ioctl (fd, FIONCONTIG64, &maxContigBytes);
.CE
.iP "FIOREADDIR"
Reads the next directory entry.  The argument <dirStruct> is a DIR
directory descriptor.  Normally, the readdir() routine is used to read a
directory, rather than using the FIOREADDIR function directly.  See dirLib.
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
function directly.  See dirLib.
.CS
    struct stat statStruct;
    fd = open ("file", O_RDONLY);
    status = ioctl (fd, FIOFSTATGET, (int)&statStruct);
.CE
.iP "FIOTIMESET"
Update time on a file.   <arg> shall be a pointer to a utimbuf structure, 
see utime.h.  If <arg> is value NULL, the current system time is used for
both actime and modtime members.  If <arg> is not NULL then the utimbuf 
structure members actime and modtime are used as passed.  If actime is 
zero value, the file access time is not updated (the operation is ignored).  
If modtime is zero, the file modification time is not updated (the operation 
is ignored).   
See also utime()
..CS
    struct utimbuf newTimeBuf;;
    newTimeBuf.modtime = newTimeBuf.actime = fileNewTime;
    fd = open ("file", O_RDONLY);
    status = ioctl (fd, FIOTIMESET, (int)&newTimeBuf);
.CE
.iP "FIOCHKDSK"
This function invokes the integral consistency checking.
During the test, the file system will be blocked from application code
access, and will emit messages describing any inconsistencies found on
the disk, as well as some statistics, depending on the verbosity
level in the <flags> argument.
Depending on the repair permission value in <flags> argument,
the inconsistencies will be repaired, and changes written to disk
or only reported.
Argument <flags> should be composed of bitwise or-ed
verbosity level value and repair permission value.
Possible repair levels are:
.RS
.iP "DOS_CHK_ONLY (1)"
Only report errors, do not modify disk.
.iP "DOS_CHK_REPAIR (2)"
Repair any errors found.
.LP
Possible verbosity levels are:
.iP "DOS_CHK_VERB_SILENT (0xff00)"
Do not emit any messages, except errors encountered.
.iP "DOS_CHK_VERB_1 (0x0100)"
Display some volume statistics when done testing, as well
.iP "DOS_CHK_VERB_2 (0x0200)"
In addition to the above option, display path of every file, while it
is being checked. This option may significantly slow down the test
process.
.IP "NOTE"
In environments with reduced RAM size check disk uses reserved
FAT copy as temporary buffer, it can cause respectively long
time of execution on a slow CPU architectures..
.LP
.RE
See also the reference manual usrFsLib for the chkdsk() user level
utility which may be used to invoke the FIOCHKDSK ioctl().
The volume root directory should be opened, and the resulting file
descriptor should be used:
.CS
    int fd = open (device_name, O_RDONLY, 0);
    status = ioctl (fd, FIOCHKDSK, DOS_CHK_REPAIR | DOS_CHK_VERB_1);
    close (fd);
.CE
.LP
Any other ioctl() function codes are passed to the underlying
.I CBIO
modules for handling.

INCLUDE FILES: dosFsLib.h

SEE ALSO:
ioLib, iosLib, dirLib, usrFsLib, dcacheCbio, dpartCbio, dosFsFmtLib,
dosChkLib
.I "Microsoft MS-DOS Programmer's Reference"
(Microsoft Press),
.I "Advanced MS-DOS Programming"
(Ray Duncan, Microsoft Press),
.I "VxWorks Programmer's Guide: I/O System, Local File Systems"

INTERNAL:
Note:  To represent a backslash in documentation use "\e", not "\\".
The double backslash sometimes works, but results may be unpredictable.
*/

/* includes */

#include "vxWorks.h"
#include "private/dosFsVerP.h"
#include "stat.h"
#include "time.h"
#include "dirent.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "taskLib.h"
#include "tickLib.h"
#include "semLib.h"
#include "logLib.h"
#include "errnoLib.h"
#include "memLib.h"
#include "utime.h"
#include "blkIo.h"

#include "private/print64Lib.h"
#include "private/dosFsLibP.h"
#include "private/dosDirLibP.h"


/* defines */

#if FALSE
#   undef FAT_ALLOC_ONE
#   define FAT_ALLOC_ONE        (FAT_ALLOC | 8)
#endif /* FALSE */

/* macros */

#undef DBG_MSG
#undef ERR_MSG
#undef NDEBUG

#ifdef DEBUG
#   undef LOCAL
#   define LOCAL
#   undef ERR_SET_SELF
#   define ERR_SET_SELF
#   define DBG_MSG( lvl, fmt, arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8 )	\
	{ if( (lvl) <= dosFsDebug )					\
	    printErr( "%s : %d : " fmt,				\
	               __FILE__, __LINE__, arg1,arg2,	\
		       arg3,arg4,arg5,arg6,arg7,arg8 ); }

#   define ERR_MSG( lvl, fmt, a1,a2,a3,a4,a5,a6 )		\
        { logMsg( __FILE__ " : " fmt, (int)(a1), (int)(a2),	\
		  (int)(a3), (int)(a4), (int)(a5), (int)(a6) ); }
#else	/* NO DEBUG */

#   define NDEBUG
#   define DBG_MSG(lvl,fmt,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8) 	{}

#   define ERR_MSG( lvl, fmt, a1,a2,a3,a4,a5,a6 )		\
	{ if( (lvl) <= dosFsDebug ) 				\
            logMsg( __FILE__ " : " fmt, (int)(a1), (int)(a2),	\
		  (int)(a3), (int)(a4), (int)(a5), (int)(a6) ); }

#endif /* DEBUG */

#include "assert.h"

#ifdef ERR_SET_SELF
#   define errnoSet( err ) errnoSetOut( __FILE__, __LINE__, #err, (err) )
#endif /* ERR_SET_SELF */

/* typedefs */

/* globals */

int	dosFsDrvNum = ERROR; /* dosFs number in vxWorks driver table */
u_int	dosFsDebug = 1;

/* handlers lists */

DOS_HDLR_DESC    dosFatHdlrsList[ DOS_MAX_HDLRS ] = {{0}};
DOS_HDLR_DESC    dosDirHdlrsList[ DOS_MAX_HDLRS ] = {{0}};

STATUS (*dosFsChkRtn)( DOS_FILE_DESC_ID pFd ) = NULL;
					/* check disk routine */
STATUS (*dosFsVolFormatRtn)( void * pDev, int opt,
                             FUNCPTR pPromptFunc ) = NULL;
                          		/* volume format routine */

/* locals */

LOCAL STATUS dosFsRead( FAST DOS_FILE_DESC_ID pFd, char * pBuf,
                        int maxBytes );

LOCAL STATUS dosFsIoctl ( FAST DOS_FILE_DESC_ID pFd, int function, int arg );

LOCAL DOS_FILE_DESC_ID dosFsOpen ( FAST DOS_VOLUME_DESC * pVolDesc,
                                  char * pPath, int flags, int mode );

LOCAL STATUS dosFsDelete (DOS_VOLUME_DESC_ID pVolDesc, char * pPath);

LOCAL STATUS dosFsClose  (DOS_FILE_DESC_ID pFd);

LOCAL STATUS dosFsRename (DOS_FILE_DESC_ID    pFdOld, char * pNewName,
                          BOOL        allowOverwrite);

#ifdef ERR_SET_SELF
/*******************************************************************************
* errnoSetOut - put error message
*
* This routine is called instead of errnoSet during debugging.
*/
static VOID errnoSetOut(char * pFile, int line, const char * pStr, int errCode )
    {
    if( errCode != OK && strcmp( str, "errnoBuf" ) != 0 )
        printf( " %s : line %d : %s = %p, task %p\n",
                pFile, line, pStr, (void *)errCode,
                (void *)taskIdSelf() );
    errno = errCode;
    }
#endif  /* ERR_SET_SELF */


/*******************************************************************************
*
* dosSetVolCaseSens - set case sensitivity of volume
*
* Pass TRUE to setup a case sensitive volume.  
* Pass FALSE to setup a case insensitive volume.  
* Note this affects rename lookups only.
*
* RETURNS: TRUE if pVolDesc pointed to a DOS volume.
*/
STATUS dosSetVolCaseSens 
    ( 
    DOS_VOLUME_DESC_ID pVolDesc, 
    BOOL sensitivity 
    )
    {
    BOOL isValid; /* Validity flag */

    /* If the parameter is aligned OK */
    if ( TRUE == (isValid = _WRS_ALIGN_CHECK(pVolDesc, DOS_VOLUME_DESC)))
        {
        /* Aligned parameter, but is it a DOS volume? */
        if ( TRUE == (isValid = (pVolDesc->magic == DOS_FS_MAGIC )) )
            {
            pVolDesc->volIsCaseSens = sensitivity;
            }
        }

    /* Didn't work? Broken parameter then */
    if (FALSE == isValid)
        {
        /* Not a DOS volume or not aligned so that's an invalid parameter */
        errnoSet( S_dosFsLib_INVALID_PARAMETER );
        }

    /* If it was valid it got set. */
    return (isValid);
    }


/*******************************************************************************
*
* dosFsIsValHuge - check if value is grater, than 4GB (max 32 bit).
*
* RETURNS: TRUE if is grater, else return FALSE.
*/
LOCAL BOOL dosFsIsValHuge
    (
    fsize_t	val
    )
    {
    return DOS_IS_VAL_HUGE( val );
    } /* dosFsIsValHuge() */

/*******************************************************************************
*
* dosFsVolDescGet - convert a device name into a DOS volume descriptor pointer.
* 
* This routine validates <pDevNameOrPVolDesc> to be a DOS volume
* descriptor pointer else a path to a DOS device. This routine 
* uses the standard iosLib function iosDevFind() to obtain a pointer 
* to the device descriptor. If device is eligible, <ppTail> is 
* filled with the pointer to the first character following
* the device name.  Note that ppTail is passed to iosDevFind().
* <ppTail> may be passed as NULL, in which case it is ignored.
*
* RETURNS: A DOS_VOLUME_DESC_ID or NULL if not a DOSFS device.
*
* ERRNO:
* S_dosFsLib_INVALID_PARAMETER
*/

DOS_VOLUME_DESC_ID dosFsVolDescGet
    (
    void *      pDevNameOrPVolDesc, /* device name or pointer to dos vol desc */
    u_char **   ppTail      /* return ptr for name, used in iosDevFind */    
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc;	/* pointer to volume descriptor */
    char *      pDevName = (pDevNameOrPVolDesc == NULL) ?
    			  "" : pDevNameOrPVolDesc;
    u_char *    pNameTail;

    if( ppTail == NULL )
    	ppTail = &pNameTail;
    
    *ppTail = NULL;
    
    pVolDesc = pDevNameOrPVolDesc;
    
    /* SPR#71720 NULL is presumed to be an invalid value */
    
    if (NULL == pVolDesc)
        {
        errnoSet( S_dosFsLib_INVALID_PARAMETER );
	return (NULL);
        }

    /* SPR#71720 ensure alignment and then check the magic cookie */

    if (TRUE == _WRS_ALIGN_CHECK(pVolDesc, DOS_VOLUME_DESC))
        {
        if (pVolDesc->magic == DOS_FS_MAGIC )
            {
    	    return (pVolDesc);
            }
        }

    /* attempt to extract volume descriptor ptr from device name */
    
#ifdef _WRS_DOSFS2_VXWORKS_AE
    pVolDesc =
      (DOS_VOLUME_DESC_ID) iosDevFind (pDevName, (const char **)ppTail);
#else
    pVolDesc =
      (DOS_VOLUME_DESC_ID) iosDevFind (pDevName, (char **) ppTail);
#endif /* _WRS_DOSFS2_VXWORKS_AE */

    /* 
     * SPR#76510, if iosDevFind() returned default device,
     * then the tail (ppTail) will point to the front of
     * the passed name and that is considered a lookup failure.
     */

    if ((NULL == pVolDesc) || ((char *) *ppTail == pDevName))
        {
        errnoSet( S_dosFsLib_INVALID_PARAMETER );
	return (NULL);
        }

    /* SPR#71720 ensure alignment and then check the magic cookie */

    if (TRUE == _WRS_ALIGN_CHECK(pVolDesc, DOS_VOLUME_DESC))
        {
        if (pVolDesc->magic == DOS_FS_MAGIC )
            {
    	    return (pVolDesc);
            }
        }
    
    errnoSet( S_dosFsLib_INVALID_PARAMETER );
 
    return (NULL);
    } /* dosFsVolDescGet() */

/*******************************************************************************
*
* dosFsFSemTake - take file semaphore.
*
* RETURNS: STATUS as result of semTake.
*/
LOCAL STATUS dosFsFSemTake
    (
    DOS_FILE_DESC_ID	pFd,
    int			timeout
    )
    {
    STATUS retStat;

    assert( pFd - pFd->pVolDesc->pFdList < pFd->pVolDesc->maxFiles );
    assert( pFd->pFileHdl - pFd->pVolDesc->pFhdlList <
            pFd->pVolDesc->maxFiles );

    retStat =  semTake( *(pFd->pVolDesc->pFsemList + 
    		        (pFd->pFileHdl - pFd->pVolDesc->pFhdlList)),
                        timeout);
    assert( retStat == OK );
    return retStat;
    } /* dosFsFSemTake() */
    
/*******************************************************************************
*
* dosFsFSemGive - release file semaphore.
*
* RETURNS: STATUS as result of semGive.
*/
LOCAL STATUS dosFsFSemGive
    (
    DOS_FILE_DESC_ID	pFd
    )
    {
    STATUS retStat;
 
    assert( pFd - pFd->pVolDesc->pFdList < pFd->pVolDesc->maxFiles );
    assert( pFd->pFileHdl - pFd->pVolDesc->pFhdlList <
            pFd->pVolDesc->maxFiles );
 
    retStat =  semGive( *(pFd->pVolDesc->pFsemList + 
    		       (pFd->pFileHdl - pFd->pVolDesc->pFhdlList)));
    assert( retStat == OK );
    return retStat;
    } /* dosFsFSemGive() */
    
/*******************************************************************************
*
* dosFsVolUnmount - unmount a dosFs volume
*
* This routine is called when I/O operations on a volume are to be
* discontinued.  This is the preferred action prior to changing a
* removable disk.
*
* All buffered data for the volume is written to the device
* (if possible, with no error returned if data cannot be written),
* any open file descriptors are marked as obsolete,
* and the volume is marked as not currently mounted.
*
* When a subsequent open() operation is initiated on the device,
* new volume will be mounted automatically.
*
* Once file descriptors have been marked as obsolete, any attempt to
* use them for file operations will return an error.  (An obsolete file
* descriptor may be freed by using close().  The call to close() will
* return an error, but the descriptor will in fact be freed).
*
* This routine may also be invoked by calling ioctl() with the
* FIOUNMOUNT function code.
*
* This routine must not be called from interrupt level.
*
* RETURNS: OK, or ERROR if the volume was not mounted.
*
* NOMANUAL
*/
STATUS dosFsVolUnmount
    (
    void *	pDevNameOrPVolDesc	/* device name or ptr to */
    					/* volume descriptor */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc;	/* pointer to volume descriptor */
    int i;
    
    /* get volume descriptor */
    
    pVolDesc = dosFsVolDescGet( pDevNameOrPVolDesc, NULL );

    if( pVolDesc == NULL )
    	return ERROR;
    
    if( ! pVolDesc->mounted )
    	return ERROR;
    
    /* acquire semaphore */
    
    if( semTake( pVolDesc->devSem, WAIT_FOREVER ) == ERROR )
    	return ERROR;
    
    /* mark all opened file descriptors as obsolete */
    /* also synchronize the FAT and do not hold the */
    /* file semaphore, in certain operations like   */
    /* rename-file the file semaphore is locked     */
    /* before devSem. Trying to hold file semaphore */
    /* , after holding devSem, will cause dead-lock */
 
    for( i = 0; i < pVolDesc->maxFiles; i ++ )
    	{
   	if( pVolDesc->pFdList[ i ].busy )
	    {
	    /* Synchronize the FAT */
	    pVolDesc->pFatDesc->flush(&pVolDesc->pFdList[i]);
    	    pVolDesc->pFdList[ i ].pFileHdl->obsolet = 1;
	    }

    	}
    pVolDesc->nBusyFd = 0;
    
    /* flush buffers */
    
    cbioIoctl( pVolDesc->pCbio, CBIO_CACHE_FLUSH, (void*)(-1) );

    cbioIoctl( pVolDesc->pCbio, CBIO_CACHE_INVAL, 0 );
    
    pVolDesc->mounted = FALSE;	/* volume unmounted */
    
    semGive( pVolDesc->devSem );
    return OK;
    } /* dosFsVolUnmount() */
    
/*******************************************************************************
*
* dosFsChkDsk - make volume integrity checking.
*
* This library does not makes integrity check process itself, but
* instead uses routine provided by dosChkLib. 
* This routine prepares parameters and invokes checking routine
* via preinitialized function pointer. If dosChkLib does not configured
* into vxWorks, this routine returns ERROR.
*
* Ownership on device should be taken by an upper level routine.
*
* RETURNS: STATUS as returned by volume checking routine or
*  ERROR, if such routine does not installed.
*
* ERRNO:
* S_dosFsLib_UNSUPPORTED.
*/
STATUS dosFsChkDsk
    (
    FAST DOS_FILE_DESC_ID	pFd, /* file descriptor of root dir */
    u_int	params		     /* check level and verbosity */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    CBIO_DEV_ID	cbio =  pFd->pVolDesc->pCbio;
    STATUS	retVal = ERROR;
    
    if( dosFsChkRtn == NULL )
        {
    	errnoSet( S_dosFsLib_UNSUPPORTED );
	ERR_MSG(1,"Check disk utility not installed\n", 0,0,0,0,0,0 );
    	return ERROR;
        }

    /* prepare check disk parameters */
    
    if( (params & 0xff) == 0 )
    	params |= DOS_CHK_ONLY;
    
    /* prevent attempts to write on read only volume */
    
    cbioIoctl(cbio, CBIO_STATUS_CHK, 0 );

    if( cbioModeGet(cbio) == O_RDONLY )
        {
	/* avoid writing to O_RDONLY devices */
        params = (params & ~0xff) | DOS_CHK_ONLY;
        }

    pVolDesc->chkLevel = min( params & 0xff, DOS_CHK_REPAIR );

    pVolDesc->chkVerbLevel = 
    	    ( (params & 0xff00) == DOS_CHK_VERB_0 )? 0 : params >> 8;
    
    /* run disk check */
    
    retVal = dosFsChkRtn( pFd );
    
    if(params == ((params & ~0xff) | DOS_CHK_ONLY))
        {
        cbioIoctl( pVolDesc->pCbio, CBIO_CACHE_FLUSH, (void *)(-1) );
        cbioIoctl( pVolDesc->pCbio, CBIO_CACHE_INVAL, 0 );
        }
    	    
    pVolDesc->chkLevel = 0;
    pVolDesc->chkVerbLevel = 0;
    	    
    /* reset dcache driver to recount boot block checksum */

    if( TRUE == cbioRdyChgdGet (pVolDesc->pCbio) )
    	{
    	retVal = ERROR; /* volume was changed during check disk */
    	}
    else	
    	{
     	cbioIoctl( pVolDesc->pCbio, CBIO_RESET, NULL );
    	}
    	    	
    return retVal;
    } /* dosFsChkDsk() */

/*******************************************************************************
*
* dosFsBadBootMsg - print error message while boot sector parsing.
*
* RETURNS: N/A.
*/
LOCAL void dosFsBadBootMsg
    (
    u_int	dbgLevel, 	/* message level */
    u_int	offset,		/* offset of invalid field */
    u_int	val,		/* invalid value */
    char *	pExtraMsg,	/* extra message */
    u_int	line		/* line number of error detecting code */
    )
    {
#ifdef DEBUG
    printErr( "%s : %u. Malformed boot sector. Offset %u, value %u. %s\n",
    	      __FILE__, __LINE__, offset, val,
	      ( (pExtraMsg == NULL)? " " : pExtraMsg ) );
#else /* not DEBUG */
    ERR_MSG( dbgLevel, "Malformed boot sector. Offset %u, value %u. %s\n",
	     offset, val, ( (pExtraMsg == NULL)? " " : pExtraMsg ), 0,0,0 ); 
#endif /* DEBUG */
    }
    
/*******************************************************************************
*
* dosFsBootSecGet - extract boot sector parameters.
*
* This routine reads boot sector from the disk and extracts
* current volume parameters from of it.
*
* This routine also performs sanity check of those volume parameters,
* that are mutually dependent or alternative.
* 
* This routine also determines the FAT type: FAT32, FAT12, or FAT16.
*
* If read or damaged boot sector is encountered, this routine
* searches for backup copy of boot sector and accepts volume
* volume configuration from this copy.,
*
* RETURNS: OK or ERROR if disk read error or inconsistent
* boot sector data or failure to obtain cbio parameters.
*
* ERRNO:
* S_dosFsLib_UNKNOWN_VOLUME_FORMAT
*/
LOCAL STATUS dosFsBootSecGet
    (
    DOS_VOLUME_DESC_ID	pVolDesc	/* pointer to volume descriptor */
    )
    {
    CBIO_PARAMS cbioParams;

    u_int	work;
    u_char	bootSec[ 512 ] = { 0 }; /* buffer for boot sector data */
    u_char	pass = 0;
    u_char      tmpType [ DOS_BOOT_FSTYPE_LEN + 1] = { 0 };
    pVolDesc->bootSecNum = DOS_BOOT_SEC_NUM;  /* zero, per FAT standard */

boot_get:    

    /* reset the CBIO device in order to synchronize the boot sector */
    if( cbioIoctl(pVolDesc->pCbio, CBIO_RESET, 0 ) == ERROR )
    	{
    	return ERROR;
    	}

    /* request the underlying CBIO device parameters */

    if (ERROR == cbioParamsGet (pVolDesc->pCbio, &cbioParams))
	{
	return (ERROR);
	}

    /* ensure bytesPerBlk is non-zero */

    if (0 == cbioParams.bytesPerBlk)
	{
    	ERR_MSG( 1, "cbioParams.bytesPerBlk cannot be zero.\n", 
		0,0,0,0,0,0 );

    	if( TRUE == cbioRdyChgdGet (pVolDesc->pCbio) )
            {
	    return (ERROR);
            }

    	goto error;
	}

    /* read relevant boot sector data into bootSec[] */

    work = min( cbioParams.bytesPerBlk, (int) sizeof(bootSec) );

    if( cbioBytesRW(pVolDesc->pCbio, pVolDesc->bootSecNum, 0 /* offset */,
    	    	     (addr_t)bootSec, work, CBIO_READ, NULL ) == ERROR ) 
    	{
    	ERR_MSG( 1, "ERROR reading the device boot sector\n", 0,0,0,0,0,0 );
    	ERR_MSG( 1, "media not formatted or not present\n", 0,0,0,0,0,0 );

    	if( TRUE == cbioRdyChgdGet (pVolDesc->pCbio) )
            {
	    return (ERROR);
            }

    	goto error;
    	}

    /* check for both acceptable Intel 80x86 `jmp' opcodes */

    if( bootSec[ DOS_BOOT_JMP ] != 0xe9 && bootSec[ DOS_BOOT_JMP ] != 0xeb )
    	{
    	dosFsBadBootMsg( 1, DOS_BOOT_JMP, (u_int)bootSec[ DOS_BOOT_JMP ],
			 NULL , __LINE__);
        goto error;
        }

    /* 
     * Many FAT documents mistakenly state that the 0x55aa signature word
     * "occupies the last two bytes of the boot sector".  That is only true 
     * when the bytes-per-sector value is 512 bytes.  Microsoft defines the 
     * signature at offsets 510 and 511 (zero-based) regardless of the sector 
     * size.  It is acceptable, however to have the signature at the end of the 
     * sector.  We shall accept either location. SPR#62415.
     */

    /* read the ending 2 bytes of the sector to check signature */

    if( cbioBytesRW(pVolDesc->pCbio, pVolDesc->bootSecNum,
            cbioParams.bytesPerBlk - 2 /* off */,
            (addr_t)&work, 2, CBIO_READ, NULL ) == ERROR ) 
        {
    	ERR_MSG( 1, "ERROR reading boot sector\n", 0,0,0,0,0,0 );

    	if( TRUE == cbioRdyChgdGet (pVolDesc->pCbio) )
            {
	    return (ERROR);
            }

    	goto error;
    	}

    work = DISK_TO_VX_16( &work );

    if (0xaa55 != work) /* why have a back door? "&& (work != 0xface)") */
        {
	/* 
	 * We did not find the signature word at the end of the sector,
	 * so we will check at the 510/511 offset per the Microsoft FAT spec. 
	 * If we cannot find it there, then assume the boot sector is bad.
	 */

        if ((bootSec[510] != 0x55) || (bootSec[511] != 0xaa))
	    {
	    dosFsBadBootMsg( 1, cbioParams.bytesPerBlk - 2,
			     work, "At the end of boot sector", __LINE__ );
            goto error;
	    }
        }

    /* 
     * Start filling out and verifying the volume descriptor fields 
     * using the data from the boot parameter block.  Evaluate the validity
     * of the data, so that dosFsVolIsFat12() may be safely called.
     */

    /* evaluate bytes per sector */
    
    work = DISK_TO_VX_16( bootSec + DOS_BOOT_BYTES_PER_SEC );

    pVolDesc->bytesPerSec = work;

    if( work == 0 )
    	{
    	dosFsBadBootMsg( 1, DOS_BOOT_BYTES_PER_SEC, work, NULL, __LINE__ );
    	ERR_MSG(1, "bytesPerSec = 0\n", 0,0,0,0,0,0 );
    	goto error;
    	}

    if( work != cbioParams.bytesPerBlk )
    	{
        dosFsBadBootMsg( 1, DOS_BOOT_BYTES_PER_SEC, work, NULL, __LINE__ );
    	ERR_MSG(1, "cbioParams.bytesPerBlk %u != bytes-per-sec %u\n", 
    		   cbioParams.bytesPerBlk, work, 0,0,0,0 );
    	goto error;
    	}

    /*
     * bytes-per-sector must be a power-of-two per Microsoft FAT specification 
     * and at least 32 bytes, so a shift is used instead of multiplication in 
     * operations with this value.
     */

    pVolDesc->secSizeShift = 0;

    for( work = 5; work < 16; work ++ )
    	{
    	if( (1 << work) == pVolDesc->bytesPerSec )
    	    {
    	    pVolDesc->secSizeShift = work;
    	    break;
    	    }
    	}

    if( pVolDesc->secSizeShift == 0 )
    	{
        dosFsBadBootMsg( 1, DOS_BOOT_BYTES_PER_SEC, 
			 pVolDesc->bytesPerSec, NULL, __LINE__ );
    	goto error;
    	}
    	
    /* evaluate the total number of sectors on this volume */
    
    work = DISK_TO_VX_16( bootSec + DOS_BOOT_NSECTORS );

    /* 
     * When the volume has at least 0x10000 sectors, the 16 bit field 
     * DOS_BOOT_NSECTORS is zero, and the alternate 32bit field 
     * DOS_BOOT_LONG_NSECTORS is used to determine the number of 
     * sectors on the volume.
     */

    if( work == 0 )	/* it is a large disk */
    	{
    	work = DISK_TO_VX_32( bootSec + DOS_BOOT_LONG_NSECTORS );
    	if( work == 0 )
    	    {
    	    dosFsBadBootMsg( 1, DOS_BOOT_LONG_NSECTORS, work,
                             NULL, __LINE__ );
    	    goto error;
    	    }
    	}

    pVolDesc->totalSec = work;
    
     /* number of sectors can be greater than cbioParams.nBlocks */
     
    if( work != cbioParams.nBlocks )
    	{
	/* 
	 * XXX - An error here may indicate a problem with representing
	 * a partition size correctly in the underlying CBIO layer.
         * 
         * Also, an off by one error may mean a driver bug.
         * cbioParams.nBlocks is the number of blocks on the 
         * CBIO device. Using the "last addressable LBA value"
         * in nBlocks can produce an off by one error, this is 
         * considered a driver problem.  bd_nBlocks shall be the 
         * number of blocks (1-xxx) not the last addressable block.  
         * Rather the driver should set nBlocks to the 
         * (last addressable block + 1).  DOSFS1 did not make this 
         * check.  DOSFS2 does make this check to avoid overrun.
	 */

    	if( work < cbioParams.nBlocks )
    	    {
    	    ERR_MSG(10, 
		"WARNING: num-sectors %u < cbioParams.nBlocks %u\n",
    		    work, cbioParams.nBlocks, 0,0,0,0 );
    	    }
    	else
    	    {
    	    dosFsBadBootMsg( 1, DOS_BOOT_LONG_NSECTORS, 0,
                             NULL, __LINE__ );
    	    ERR_MSG(1, "num-sectors %u > cbioParams.nBlocks %u\n", 
    		    work, cbioParams.nBlocks, 0,0,0,0 );
    	    goto error;
    	    }
    	}
    
    /* evaluate the number of sectors per cluster */
    
    pVolDesc->secPerClust = bootSec[ DOS_BOOT_SEC_PER_CLUST ];

    if( pVolDesc->secPerClust == 0 )
    	{
    	dosFsBadBootMsg( 1, DOS_BOOT_SEC_PER_CLUST, 0,
                         NULL, __LINE__ );
    	goto error;
    	}
    
    /* evaluate the number of FAT copies */
    
    pVolDesc->nFats = bootSec[ DOS_BOOT_NFATS ];

    if( pVolDesc->nFats == 0 )
    	{
    	dosFsBadBootMsg( 1, DOS_BOOT_NFATS, 0, NULL, __LINE__ );
    	goto error;
    	}
    	
    /* get the number of hidden sectors */
    
    pVolDesc->nHiddenSecs = DISK_TO_VX_16( bootSec + DOS_BOOT_NHIDDEN_SECS);
    
    /* evaluate the number of reserved sectors */
    
    pVolDesc->nReservedSecs = DISK_TO_VX_16( bootSec + DOS_BOOT_NRESRVD_SECS);

    if( pVolDesc->nReservedSecs == 0 )
    	{
    	dosFsBadBootMsg( 1, DOS_BOOT_NRESRVD_SECS, 0,
                         NULL, __LINE__ );
    	goto error;
    	}

    /* evaluate the number of sectors alloted to FAT table */

    pVolDesc->secPerFat = DISK_TO_VX_16( bootSec + DOS_BOOT_SEC_PER_FAT );

    /* 
     * Now determine the volumes FAT type.  FAT12, FAT16, and FAT32.
     * NOTE: The secPerFat field is zero on FAT32 DOSFS volumes. 
     * This is how we determine if FAT32 will be used when mounting 
     * this volume.  If secPerFat is zero, it must be FAT32.
     * Else, we need to pick between FAT12 and FAT16.
     */

    if( pVolDesc->secPerFat != 0 ) /* then using either FAT12 or FAT16 */
    	{
	/* 
	 * The maximum number of 16 bit FAT entries is 65536. 
	 * Anything greater is invalid.  Check here.
	 */

   	if( pVolDesc->secPerFat >  (ULONG)0x10000*2 / pVolDesc->bytesPerSec)
    	    {
    	    dosFsBadBootMsg( 1, DOS_BOOT_SEC_PER_FAT,
			     pVolDesc->secPerFat, NULL, __LINE__ );
    	    ERR_MSG(1, "secPerFat 12/16 = %u, while BPS = %u\n",
    	    		pVolDesc->secPerFat,pVolDesc->bytesPerSec,0,0,0,0 );
    	    goto error;
    	    }
    	
	/* 
	 * Now we must decide if our volume is using FAT12 or FAT16.
	 * If we choose the wrong FAT type, volume mounting will fail, 
	 * and/or data corruption on the volume will occur when its exercised.
	 * See also: SPR#34704.
	 * We will also check the MS FSTYPE field (offset 0x36 in the 
	 * boot sector) when determining the FAT type.  If either of the 
	 * Microsoft defined strings exist, then we honor the boot sectors 
	 * wisdom.  This presumes that the formatter of the volume knew what
	 * they were doing when writing out these strings.  This may not
	 * be the case, but its seems the most compatible approach.
	 * The FSTYPE string field is also intentionaly being honored, so 
	 * that either FAT type can be forced in the field.  In the event 
	 * of a bad mount occuring in the field, a hack of writing the correct 
	 * string to the BPB FSTYPE field would force the mount to the desired 
	 * type.  Many DOS implementations do not set these strings and that 
	 * is just fine.  Copy FSTYPE string to tmpType.
	 */

	bcopy ( (char *) bootSec + DOS_BOOT_FSTYPE_ID,
                (char *) tmpType, DOS_BOOT_FSTYPE_LEN);


	/* 
	 * Now calculate the FAT type (FAT12 vs. FAT16) per a formula 
	 * We warn the user when the FSTYPE string (if present) doesn't match 
	 * the calculation.
	 */
        work = dosFsVolIsFat12(bootSec);

        if (ERROR == work)
            {
    	    ERR_MSG(1, "dosFsVolIsFat12 returned ERROR\n", 0,0,0,0,0,0 );
    	    goto error;
            }

    	if (TRUE == work) /* then calculated FAT12 */
    	    {
	    /* 
	     * Check the FSTYPE field in the BPB to ensure the string
	     * value matches our calculation.  If not, the we assume
	     * the formatter knew what they wanted, and we honor
	     * the string value. We look for "FAT12   " or "FAT16   ".
	     */

	    if ((strcmp ((char *)tmpType, DOS_BOOT_FSTYPE_FAT16)) == 0)
	        {
    	        pVolDesc->fatType = FAT16;
	        printf("WARNING: FAT16 indicated by BPB FSTYPE string, "
		       "cluster calculation was FAT12. Honoring string.\n");
	        }
	    else
		{
    	        pVolDesc->fatType = FAT12;
	    	}
    	    }
    	else /* we calculated FAT 16 */
    	    {
	    /* 
	     * Check the FSTYPE field in the BPB to ensure the string
	     * value matches our calculation.  If not, the we assume
	     * the formatter knew what they wanted, and we honor
	     * the string value. We look for "FAT12   " or "FAT16   ".
	     */
	    if ((strcmp ((char *)tmpType, DOS_BOOT_FSTYPE_FAT12)) == 0)
	        {
    	        pVolDesc->fatType = FAT12;
	        printf("WARNING: FAT12 indicated by BPB FSTYPE string, "
		       "cluster calculation was FAT16. Honoring string.\n");
	        }
	    else
		{
    	        pVolDesc->fatType = FAT16;
	    	}
    	    }

    	/* volume Id and label */
    	
    	pVolDesc->volIdOff = DOS_BOOT_VOL_ID;
    	pVolDesc->volLabOff = DOS_BOOT_VOL_LABEL;
    	}
    else	/* Use FAT32 because (pVolDesc->secPerFat == 0) */
    	{
    	pVolDesc->fatType = FAT32;
    	
    	/* sectors per fat copy */
    	
    	pVolDesc->secPerFat = DISK_TO_VX_32( bootSec +
    					     DOS32_BOOT_SEC_PER_FAT );

    	if( pVolDesc->secPerFat == 0 )
    	    {
    	    dosFsBadBootMsg( 1, DOS32_BOOT_SEC_PER_FAT, 0,
                             "(FAT32)", __LINE__ );
    	    goto error;
    	    }
    	
    	/* volume Id and label */
    	
    	pVolDesc->volIdOff = DOS32_BOOT_VOL_ID;
    	pVolDesc->volLabOff = DOS32_BOOT_VOL_LABEL;
    	}
    
    /*
     * count sector number of data area start cluster.
     * This value can be corrected later by directory handler, if
     * root directory is not stored as regular directory
     * in clusters (FAT32), but instead resides contiguously
     * ahead first data cluster (FAT12/FAT16)
     */
    pVolDesc->dataStartSec = pVolDesc->nReservedSecs +
    			     pVolDesc->secPerFat * pVolDesc->nFats;
    
    /* volume Id and label */
    	
    pVolDesc->volId = DISK_TO_VX_32( bootSec + pVolDesc->volIdOff );

    bcopy( (char *)bootSec + pVolDesc->volLabOff,
    	   (char *)pVolDesc->bootVolLab, DOS_VOL_LABEL_LEN );

    *(pVolDesc->bootVolLab + DOS_VOL_LABEL_LEN) = EOS;
    	     
    /* restore base version of boot sector */

    if( pVolDesc->bootSecNum != DOS_BOOT_SEC_NUM )
    	{
    	ERR_MSG( 1, "Try to reclaim original copy of boot sector\n",
		0,0,0,0,0,0 );
    	if( (pass == 0) &&
            (cbioBlkCopy(pVolDesc->pCbio, pVolDesc->bootSecNum,
			DOS_BOOT_SEC_NUM, 1 ) == OK) &&
    	    (cbioIoctl(pVolDesc->pCbio, 
		       CBIO_CACHE_FLUSH, (void *)(-1) ) == OK))
    	    {
    	    /* remount again */

    	    pass ++;

    	    pVolDesc->bootSecNum = DOS_BOOT_SEC_NUM;

    	    goto boot_get;
    	    }
   	}
    	
    /* currently it is enough for starting */
    
    return (OK);
    
error:	/* some data is inconsistent */

    pVolDesc->bootSecNum ++;

    if( pVolDesc->bootSecNum < (u_int)DOS_BOOT_SEC_NUM +
			       cbioParams.blocksPerTrack )
    	{
    	/* try to find other boot block copy on next sector */
	
    	if( pVolDesc->bootSecNum == DOS_BOOT_SEC_NUM + 1 )
    	    {
    	    ERR_MSG( 1, "Problem finding volume data, trying to "
                    "use the next block as boot block.\n",
    	    	     0,0,0,0,0,0 );

            /* 
             * SPR#69074 - only look at the next sector, this avoids
             * tons of error messages when mounting unformatted device
             * when we used to look at the remaining sectors in the track.
             */

    	    goto boot_get;
    	    }

    	ERR_MSG( 1, "Ensure this device is formatted and partitions are "
                    "properly handled.\n",
    	    	     0,0,0,0,0,0 );
    	}

    if( errnoGet() == OK )
    	errnoSet( S_dosFsLib_UNKNOWN_VOLUME_FORMAT );
    return ERROR;
    } /* dosFsBootSecGet() */

/*******************************************************************************
*
* dosFsVolIsFat12 - determine if a MSDOS volume is FAT12 or FAT16
*
* This routine is the container for the logic which determines if a 
* dosFs volume is using FAT12 or FAT16.  Two methods are implemented. 
* Both methods use information from the volumes boot parameter block
* fields found in the boot sector.
*
* The first FAT determination method follows the recommendations outlined
* in the Microsoft document:
*
* "Hardware White Paper
*  Designing Hardware for Microsoft» Operating Systems
*  FAT: General Overview of On-Disk Format
*  Version 1.02, May 5, 1999
*  Microsoft Corporation"
*
* This method is used in the hopes that greater compatability with
* MSDOS formatted media will be achieved.  The Microsoft recommended 
* method for FAT type determination between FAT12 and FAT16 is done 
* via the count of clusters on the volume.
*
* The Microsoft recommended approach is as follows:
*
* 1.) Determine the count of sectors occupied by the root directory
*     entries for this volume, rounding up:
*
* rootDirSecs = ((rootEntCount * dirEntSz) + (bytesPerSec-1)) / bytesPerSec;
*
* Where dirEntSz is 32 for MSDOS 8.3, and 64 for VXLONGNAMES.
*
*
* 2.) Determine the count of sectors occupied by the volumes data region:
*
* dataRgnSecs = totalSecs - (reservedSecs + (nFats * fatSecs) + rootDirSecs);
*
*
* 3.) determine the count of clusters, rounding down:
*
* countOfClusts = dataSecs / secsPerClust; /@ Note: this rounds down. @/
* 
* Note: countOfClusts represents the count of data clusters, starting at two.  
*
* 4.) determine the FAT types based on the count of clusters on the volume,
* 
*     if (countOfClusts < 4085) /@ Microsoft recommends using "less than" @/
*         {
*         /@ Volume is FAT12 @/
*         }
*     else
*         {
*         /@ Volume is FAT16 @/
*         }
*
*
* An alternate method is used when mounting a known VxWorks DOSFS-1.0 volume. 
* This method is used for greater backward compatability with VxWorks 
* DOSFS-1.0 volumes.  See also: SPR#34704.  The VxWorks dosFs1 method 
* deviates from the Microsoft currently recommened method.
*
* This is the VxWorks DOSFS1 method per dosFsVolDescFill(), dosFsLib.c,
* dosFs 1.0, revision history: "03l,16mar99,dgp".  Using the identical 
* method  here will help ensure backward compatablitity when mounting 
* volumes formatted by the VxWorks dosFs1.0 code. 
*
* The VxWorks DOSFS 1.0 approach is as follows:
*
* 1.) Get starting sector of the root directory:
*
* rootSec = reservedSecs + (nFats * secsPerFat);
*
*
* 2.) Get the size of the root dir in bytes:
*
* rootBytes = (nRootEnts * dirEntSz):
*
* Where dirEntSz is 32 for MSDOS 8.3, and 64 for VXLONGNAMES.
*
*
* 3.) Get the starting sector of the data area:
*
* dataSec = rootSec + ((rootBytes + bytesPerSec-1) / bytesPerSec);
*
*
* 4.) Get the number of "FAT entries":
*
* countOfClusts = 
*        ( ( (totalSecs - dataSecs) / secsPerClust) + DOS_MIN_CLUST);
*
*
* 5.) Choose the FAT type based on the count of clusters, note DOSFS1 
* uses less than or equal here.
*
* if (countOfClusts <= 4085) /@ VxDosFs1 uses less than or equal to. @/
*     {
*     /@ use FAT12 @/
*     }
* else
*     {
*     /@ use FAT16 @/
*     }
*
* By mimicking the dosFs 1.0 approach, we should be able to mount
* all dosFs 1.0 volumes correctly.  By using the microsoft recommened
* approach in all other cases, we should be as compatable as possible
* with Microsoft OS's.
*
* The volumes Boot Parameter Block fields MUST be validated for sanity
* before this routine is called.
*
* pBootBuf is not verified, DO NOT pass this routine a NULL pointer.
* This routine is also used by dosFsFmtLib.c
*
* RETURNS: TRUE if the FAT type is FAT12, FALSE if the FAT type is FAT16,
* or ERROR if the data is invalid.
*
* NOMANUAL
*/

int dosFsVolIsFat12
    (
    u_char * pBootBuf		/* boot parameter block buffer */
    )
    {
    u_int  dirEntSz; 		/* directory entry size in bytes */
    u_int  nRootEnts;		/* number of root directory entries */
    u_int  bytesPerSec;		/* number of bytes per sector */
    u_int  rootDirSecs;		/* count of sectors used for root directory */
    u_int  reservedSecs;	/* count of reserved sectors */
    u_int  totalSecs;		/* count of total sectors on volume */
    u_int  dataRgnSecs;		/* count of data region sectors */
    u_int  secsPerClust;	/* count of sectors per cluster */
    u_int  nFats;		/* number of FAT tables */
    u_int  secsPerFat;		/* number of sectors used for a FAT table */
    u_int  countOfClusts;	/* count of clusters on this volume */
    u_int  work;		/* work int */
    BOOL   vxDOSFS1Vol;         /* tracks when using VxWorks dosFs 1.0 */
    char   *dosFs1SysId = "VXDOS4.0";    /* VxDOSFS 1.0 using 8.3 */
    char   *dosFs1ExtSysId = "VXEXT1.0"; /* VxDOSFS 1.0 using VxLongnames */

    /* NULL is presumed to be an invalid value */

    if (NULL == pBootBuf)
        {
        return (ERROR);
        }

    /* initialize all variables used to determine FAT type */

    /* determine the total number of sectors on this volume */
    
    work = DISK_TO_VX_16(pBootBuf + DOS_BOOT_NSECTORS);

    /* 
     * When the volume has at least 0x10000 sectors, the 16 bit field 
     * DOS_BOOT_NSECTORS is zero, and the alternate 32bit field 
     * DOS_BOOT_LONG_NSECTORS is used to determine the number of 
     * sectors on the volume.
     */

    if( 0 == work)	/* disk has 0x10000 or more sectors */
    	{
    	work = DISK_TO_VX_32( pBootBuf + DOS_BOOT_LONG_NSECTORS );
    	}

    totalSecs = work; /* total sectors on volume */

    /* cannot have a device with zero sectors */

    if (0 == totalSecs)
        {
        return (ERROR);
        }

    /* 
     * Determine if we are mounting a VxWorks DOSFS 1.0 file system.
     * This is done because VxWorks DOSFS 1.0 uses a different 
     * FAT mounting formula than is recommended by Microsoft today.
     * We must account for this, or errors will occur in the FAT type 
     * calculation, causing us to fail to correctly mount VxWorks DOSFS 1.0 
     * volumes on some 12/16 boundary cases.   
     * We check for one of two possible VxWorks DOSFS 1.0 signature strings.
     * VxWorks DOSFS 1.0 will have "VXDOS1.0" for 8.3 filenames.
     * VxWorks DOSFS 1.0 will have "VXEXT1.0" for VxLongnames.
     */

    if( (bcmp( pBootBuf + DOS_BOOT_SYS_ID, dosFs1SysId,
    	      strlen( dosFs1SysId ) ) == 0)     ||
        (bcmp( pBootBuf + DOS_BOOT_SYS_ID, dosFs1ExtSysId,
    	      strlen( dosFs1ExtSysId ) ) == 0) )
	{
        vxDOSFS1Vol = TRUE;  /* using Kents VxLongs */
	}
    else /* presume not VxLongnames */
	{
        vxDOSFS1Vol = FALSE; /* standard 8.3 */
	}

    /* 
     * Determine the directory entry size to use.  Check for the "VXEXT" 
     * string, if present then use longnames dirent size, else use 
     * the standard 8.3 dirent size.  We only check the first 5 chars
     * so we cant get both versions, "VXEXT1.0" and "VXEXT1.1".
     * Yes, the FAT type mounting differs if using 1.1, this is intentional.
     */

    if( bcmp( pBootBuf + DOS_BOOT_SYS_ID, DOS_VX_LONG_NAMES_SYS_ID,
    	      strlen( DOS_VX_LONG_NAMES_SYS_ID ) ) == 0 )
	{
        dirEntSz = DOS_VX_DIRENT_LEN;	/* 64 bytes using Kents VxLongs */
	}
    else
	{
        dirEntSz = DOS_DIRENT_STD_LEN; /* 32 bytes using standard 8.3 */
	}

    /* Determine the number of bytes per sector on this volume */

    bytesPerSec = DISK_TO_VX_16(pBootBuf + DOS_BOOT_BYTES_PER_SEC);

    /* cannot have zero bytes per sector */

    if (0 == bytesPerSec)
	{
        return (ERROR);
	}

    /* Determine the number of root directory entries on this volume */

    nRootEnts = DISK_TO_VX_16(pBootBuf + DOS_BOOT_MAX_ROOT_ENTS); 

    /* cannot have zero root directory entries */

    if (0 == nRootEnts)
	{
        return (ERROR);
	}

    /* Determine the number of sectors per cluster */
    
    secsPerClust = pBootBuf [DOS_BOOT_SEC_PER_CLUST];

    /* cannot have zero sectors per cluster */

    if (0 == secsPerClust)
	{
        return (ERROR);
	}

    /* determine the number of FAT copies */
    
    nFats = pBootBuf [DOS_BOOT_NFATS];

    /* cannot have zero number of FAT copies */

    if (0 == nFats)
	{
        return (ERROR);
	}

    /* determine the number of reserved sectors */
    
    reservedSecs = DISK_TO_VX_16(pBootBuf + DOS_BOOT_NRESRVD_SECS);

    /* determine the number of sectors alloted to FAT table */

    secsPerFat = DISK_TO_VX_16(pBootBuf + DOS_BOOT_SEC_PER_FAT);

    /* cannot have zero sectors per FAT */

    if (0 == secsPerFat)
	{
        return (ERROR);
	}

    /* All needed fields have been stored, now determine the FAT type */

    if (FALSE == vxDOSFS1Vol) 
	{
	/* 
	 * Were not mounting a VxWorks DOSFS 1.0 volume.
	 * We will use the formula recommended by Microsoft.
	 */

	/* Determine the sectors used by root directory, this rounds up */

        rootDirSecs = ((nRootEnts * dirEntSz)+(bytesPerSec-1)) / bytesPerSec;

        /* determine the total count of sectors in volumes data region */

        dataRgnSecs = 
	    totalSecs - (reservedSecs + (nFats * secsPerFat) + rootDirSecs);

        /* determine the count of clusters, this rounds down. */

        countOfClusts = dataRgnSecs / secsPerClust; 

        /* determine FAT type based on the count of clusters */

        if (countOfClusts < (u_int) DOS_FAT_12BIT_MAX)
	    {
	    return (TRUE);  /* FAT12 */
	    }
        else 
	    {
	    return (FALSE); /* FAT16 */
	    }
	}
    else   /* (TRUE == vxDOSFS1Vol) */
	{
	/* 
	 * Mounting VxWorks DOSFS 1.0 volume, use VxDOSFS 1.0 method.
	 * This VxWorks DOSFS1 method is per dosFsVolDescFill(), dosFsLib.c
	 * dosFs 1.0 version "03l,16mar99,dgp".
	 *
	 * Note that some variables below are used a bit differently than 
	 * in the Microsoft method above.
	 */

	/* Determine the starting sector of the root dir  */

        rootDirSecs = reservedSecs + (nFats * secsPerFat);

        /* Determine the size of the root dir in bytes */

        work = (nRootEnts * dirEntSz);

        /* Determine the starting sector of the data area */

        dataRgnSecs = rootDirSecs + ((work + bytesPerSec-1) / bytesPerSec);

        /* Determine the number of FAT entries */

        countOfClusts = 
		 (((totalSecs - dataRgnSecs)/ secsPerClust)+DOS_MIN_CLUST);

	/* 
	 * Choose the FAT type based on countOfClusts, note VxDOSFS1 
	 * uses less than or equal here.
	 */

        if (countOfClusts <= (u_int) DOS_FAT_12BIT_MAX)  
	    {
	    return (TRUE);  /* FAT12 */
	    }
        else
	    {
	    return (FALSE);  /* FAT16 */
	    }
	}
    } /* dosFsVolIsFat12 */

/*******************************************************************************
*
* dosFsVolMount - prepare to use dosFs volume
*
* This routine prepares the library to use the dosFs volume on the 
* device specified.  The first sector, known as the boot sector,
* is read from the disk.  The required information in the boot sector
* is copied to the volume descriptor for this device.
* Some other fields in the volume descriptor
* are set using values calculated from the boot sector information.
*
* The appropriate File Allocation Table (FAT) handler and directory
* handler  are chosen from handlers list in accordance with
* particular volume format version and user's preferences.
*
* This routine is automatically called via first open() if device not
* mounted and every time after a disk is changed.
* 
* RETURNS: OK or ERROR.
*
* ERRNO:
* S_dosFsLib_INVALID_PARAMETER
*
*/
LOCAL STATUS dosFsVolMount
    (
    DOS_VOLUME_DESC_ID	pVolDesc	/* pointer to volume descriptor */
    )
    {
    DOS_FILE_DESC_ID	pFd = (void *)ERROR;
    u_int	errnoBuf = errnoGet();
    int	i;
    
    /* check volume descriptor */
    
    if( (pVolDesc == NULL) || pVolDesc->magic != DOS_FS_MAGIC )
    	{
    	errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	return ERROR;
    	}
        
    /* prevent mount process from reentrant call */
    
    if( semTake( pVolDesc->devSem, WAIT_FOREVER ) == ERROR )
    	return ERROR;
    
    /*
     * before we can mount new volume
     * we have to unmount previous one
     */

    if( TRUE == pVolDesc->mounted )
    	{
    	if( FALSE == cbioRdyChgdGet (pVolDesc->pCbio) )
    	    goto ret;
    	dosFsVolUnmount( pVolDesc );
    	}
    
    pVolDesc->mounted = FALSE;
    
    /* update and check base volume information from boot sector */
    errnoSet( OK );


    if( dosFsBootSecGet( pVolDesc ) == ERROR )
    	{
    	if( errnoGet() == OK )
    	    errnoSet( S_dosFsLib_UNKNOWN_VOLUME_FORMAT );
    	goto ret;
    	}
    /*
     * init DIRECTORY handler ahead FAT, because
     * directory handler finally sets "data start sector" field,
     * that depends for FAT12/FAT16 root directory size in sectors
     */
    
    /* dir handler mount */
    
    for( i = 0; i < (int)NELEMENTS( dosDirHdlrsList ); i ++ )
    	{
    	if( ( dosDirHdlrsList[ i ].mountRtn != NULL ) &&
    	    dosDirHdlrsList[ i ].mountRtn(
			pVolDesc, dosDirHdlrsList[ i ].arg ) == OK )
    	    {
    	    break;
    	    }
    	}

    if( i == NELEMENTS( dosDirHdlrsList ) )
   	goto ret;

    /* FAT handler mount */

    for( i = 0; i < (int)NELEMENTS( dosFatHdlrsList ); i ++ )
    	{
    	if( ( dosFatHdlrsList[ i ].mountRtn != NULL ) &&
    	    dosFatHdlrsList[ i ].mountRtn(
			pVolDesc, dosFatHdlrsList[ i ].arg ) == OK )
    	    {
    	    break;
    	    }
    	}

    if( i == NELEMENTS( dosFatHdlrsList ) )
   	goto ret;
    
    errnoSet( errnoBuf );
    
    pVolDesc->mounted = TRUE;
    
    /* execute device integrity check (if not started yet) */
    
    /* 
     * It may seem, that following call to open() while device
     * semaphore is taken can cause deadlock because during
     * open() a file handle semaphore possibly is taken.
     * But don't worry. First, file semaphore is taken only
     * when open() has been called with O_TRUNC or DOS_O_CONTIG_CHK
     * flags, second, all opened file handles have been
     * marked obsolete already and so no one of them will be
     * actually shared.
     */
    if(  dosFsChkRtn != NULL && pVolDesc->autoChk != 0 &&
         pVolDesc->chkLevel == 0 )
    	{
    	pFd = dosFsOpen( pVolDesc, "", 0, 0 );
    	if( pFd != (void *) ERROR )
    	    {
    	    if( dosFsChkDsk( pFd, pVolDesc->autoChk |
    	                          (pVolDesc->autoChkVerb << 8) ) == OK ||
    	    	pVolDesc->autoChk == DOS_CHK_ONLY )
    	    	{
    	    	goto ret;
    	    	}
    	    
    	    pVolDesc->mounted = FALSE;
    	    }
    	}
    
ret:
    if( pFd != (void *)ERROR )
    	{
    	pFd->pFileHdl->obsolet = 0;	/* avoid errno set */
    	dosFsClose( pFd );
    	}
    
    semGive( pVolDesc->devSem );
    
    if( pVolDesc->mounted )
    	return OK;
    return ERROR;
    } /* dosFsVolMount() */

/***************************************************************************
*
* dosFsFdFree - free a file descriptor
*
* This routine marks a file descriptor as free and decreases
* reference count of a referenced file handle.
*
* RETURNS: N/A.
*/
LOCAL void dosFsFdFree
    (
    DOS_FILE_DESC_ID	pFd
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;

    assert( pFd != NULL );
    
    DBG_MSG( 600, "pFd = %p\n", pFd ,0,0,0,0,0,0,0);
    
    semTake( pFd->pVolDesc->devSem, WAIT_FOREVER );
    
    assert( pFd->pFileHdl->nRef != 0 );
    pFd->pFileHdl->nRef --;
    pFd->busy = 0;
    semGive( pVolDesc->devSem );
    } /* dosFsFdFree() */
    
/***************************************************************************
*
* dosFsFdGet - get an available file descriptor
*
* This routine obtains a free dosFs file descriptor.
*
* RETURNS: Pointer to file descriptor, or NULL, if none available.
*
* ERRNO:
* S_dosFsLib_NO_FREE_FILE_DESCRIPTORS
*
*/

LOCAL DOS_FILE_DESC_ID dosFsFdGet
    (
    DOS_VOLUME_DESC_ID	pVolDesc
    )
    {
    FAST DOS_FILE_DESC_ID	pFd = pVolDesc->pFdList;
    FAST DOS_FILE_DESC_ID	pFdFree = NULL;
    FAST DOS_FILE_HDL_ID	pFileHdl = pVolDesc->pFhdlList;
    FAST DOS_FILE_HDL_ID	pFileHdlFree = NULL;
    
    if( semTake( pVolDesc->devSem, WAIT_FOREVER ) == ERROR )
    	return NULL;
    
    /* allocate file descriptor */
    
    for( pFd = pVolDesc->pFdList;
         pFd < pVolDesc->pFdList + pVolDesc->maxFiles; pFd++ )
    	{
    	if( ! pFd->busy )
    	    {
    	    pFdFree = pFd;
    	    break;
    	    }
    	}
    if( pFdFree == NULL )
    	{
    	errnoSet( S_dosFsLib_NO_FREE_FILE_DESCRIPTORS );
    	pFd = NULL;
    	goto ret;
    	}

    DBG_MSG( 600, "pFdFree = %p\n", pFdFree ,0,0,0,0,0,0,0);
    
    bzero( (char *)pFdFree, sizeof( *pFdFree ) );
    pFdFree->pVolDesc = pVolDesc;
    pFdFree->busy = TRUE;
    pFdFree->pVolDesc->nBusyFd ++;
    
    /* allocate file handle */
    
    for( pFileHdl = pVolDesc->pFhdlList;
         pFileHdl < pVolDesc->pFhdlList + pVolDesc->maxFiles;
         pFileHdl++ )
    	{
    	if( pFileHdl->nRef == 0 )
    	    {
    	    pFileHdlFree = pFileHdl;
    	    break;
    	    }
    	}
    
    assert( pFileHdlFree != NULL );
    
    bzero( (char *)pFileHdlFree, sizeof( *pFileHdlFree ) );
    
    pFileHdlFree->nRef = 1;
    pFdFree->pFileHdl = pFileHdlFree;
    
    DBG_MSG( 600, "pFileHdlFree = %p\n", pFileHdlFree,0,0,0,0,0,0,0 );
    
ret:
    semGive( pVolDesc->devSem );
    return pFdFree;
    } /* dosFsFdGet() */

/***************************************************************************
*
* dosFsHdlDeref - unify file descriptors of the same file.
*
* All file descriptors, that are opened for one file
* have to share the same file handle in order to
* prevent confusion when file is changed and accessed through
* several file descriptors simultaneously.
*
* This routine lookups throw list of file handles and
* references <pFd> to the file handle, that already describes the
* same file, if such exists. File handle, that has been used
* by <pFd> is freed.
*
* RETURNS: N/A.
*/
LOCAL void dosFsHdlDeref
    (
    DOS_FILE_DESC_ID	pFd
    )
    {
    FAST DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    FAST DOS_DIR_HDL_ID		pDirHdlFd = & pFd->pFileHdl->dirHdl;
    					/* dir handle ptr */
    FAST DOS_FILE_HDL_ID	pFhdlLCur = pVolDesc->pFhdlList;
    					/* loop file handle */
    FAST DOS_DIR_HDL_ID		pDirHdlCur = NULL;
    					/* dir handle of the */
    					/* loop file handle */
    FAST int	i;	/* loop counter */
    
    semTake( pVolDesc->devSem, WAIT_FOREVER );
    
    /* loop by file handles list */
    
    for( i = 0; i < pVolDesc->maxFiles; i++, pFhdlLCur++ )
    	{
    	if( pFhdlLCur->nRef == 0 || pFhdlLCur == pFd->pFileHdl ||
    	    pFhdlLCur->deleted || pFhdlLCur->obsolet )
    	    {
    	    continue;
    	    }
    	
    	/* compare directory handles */
    	
    	pDirHdlCur = & pFhdlLCur->dirHdl;
    	
    	if( pDirHdlCur->sector == pDirHdlFd->sector &&
    	    pDirHdlCur->offset == pDirHdlFd->offset )
    	    {
    	    /* the same directory entry */
    	    
    	    assert( pDirHdlCur->parDirStartCluster == 
    	            pDirHdlFd->parDirStartCluster );
    	    DBG_MSG( 600, " use %p instead of %p\n",
    	    	     pFhdlLCur, pFd->pFileHdl,0,0,0,0,0,0 );
    	    
    	    /* free file handle in <pFd> */
    	    
    	    assert( pFd->pFileHdl->nRef == 1 );
    	    bzero( (char *)pFd->pFileHdl, sizeof( *pFd->pFileHdl ) );
    	    
    	    /* deference <pFd> */
    	    
    	    pFd->pFileHdl = pFhdlLCur;
    	    pFhdlLCur->nRef ++;
    	    break;
    	    }
    	}
    semGive( pVolDesc->devSem );
    } /* dosFsHdlDeref() */
    
/***************************************************************************
*
* dosFsSeek - change file's current character position
*
* This routine sets the specified file's current character position to
* the specified position.  This only changes the pointer, doesn't affect
* the hardware at all.
*
* If the new offset pasts the end-of-file (EOF), attempts to read data
* at this location will fail (return 0 bytes).
*
* For a write if the seek is done past EOF, then use dosFsFillGap
* to fill the remaining space in the file.
*
* RETURNS: OK, or ERROR if invalid file position.
*
* ERRNO:
* S_dosFsLib_NOT_FILE
*
*/
 
LOCAL STATUS dosFsSeek
    (
    DOS_FILE_DESC_ID	pFd,	/* file descriptor pointer */
    fsize_t		newPos	/* ptr to desired character */
    				/* position in file */
    )
    {
    fsize_t	sizeBuf = pFd->pFileHdl->size; /* backup directory size */
    u_int	nSec;		/* sector offset of new position */
    				/* from seek start sector */
    u_int	startSec;	/* cluster number to count clusters from */
    STATUS	retStat = ERROR;
    
    DBG_MSG( 500, "pFd = %p: newPos = %lu, "
    		  "current pos = %lu, size = %lu\n",
    		pFd, newPos, pFd->pos, pFd->pFileHdl->size,0,0,0,0 );

    pFd->seekOutPos = 0;

     if( newPos == pFd->pos )	/* nothing to do ? */
    	return OK;
        
    /*  there is no field storing actual directory length */
    
    if( pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY )
    	{
    	pFd->pFileHdl->size = DOS_MAX_FSIZE;
    	}
    
    /* in case of seek passed EOF, only store the new position */
    
    if( newPos > pFd->pFileHdl->size )
    	{
    	pFd->seekOutPos = newPos;
    	return OK;
    	}
    /*
     * to very simplify all process, let us pose at start
     * of current cluster
     */
    if( pFd->curSec > 0 )	/* file is not just open */
    	{
    	/* 
    	 * now provide special support to the extreme case,
    	 * when contiguous block have been passed and position
    	 * stopped directly following it. In this case let return
    	 * one cluster back. It may be important, when
    	 * seek is called from close() in order to stand to
    	 * the last file position.
    	 */
    	if( pFd->nSec == 0 )	/* contiguous block exhausted */
    	    {
    	    pFd->nSec	 = 1;
    	    pFd->curSec	-= 1;
    	    pFd->pos	-= pFd->nSec <<
    	    		   pFd->pVolDesc->secSizeShift;
    	    }
    	else	/* goto start of the current sector */
    	    {
    	    pFd->pos	-= OFFSET_IN_SEC( pFd->pVolDesc, pFd->pos );
    	    }
    	} /* if( pFd->curSec > 0 ) */
    /*
     * count number of sectors to move and
     * check for seek within current contiguous block
     */
    if( newPos < pFd->pos || pFd->curSec == 0 )	/* backward */
    	{
    	/* number of sector from file start to move to */
    	
    	nSec = NSECTORS( pFd->pVolDesc,
                        min( newPos, pFd->pFileHdl->size - 1 ) );
    	
    	/* begin seeking from file start */
    	
    	startSec = FH_FILE_START;
    	DBG_MSG(550, "SEEK_SET : startClust = %lu\n", 
    		pFd->pFileHdl->startClust ,0,0,0,0,0,0,0);
    	}
    else 	/* forward */
    	{
    	/*
    	 * count number of sectors from current position.
    	 * If newPos == size, temporary move into (size-1), that is
         * last valid byte in file.
         */
    	nSec = min( newPos, pFd->pFileHdl->size - 1 ) - pFd->pos;
    	nSec = NSECTORS( pFd->pVolDesc, nSec );
    	if( nSec < pFd->nSec )	/* within current block ? */
    	    {
    	    pFd->nSec	-= nSec;
    	    pFd->curSec	+= nSec;
	    DBG_MSG(500, "within current cluster group\n", 0,0,0,0,0,0,0,0 );
    	    goto retOK;
    	    }
    	
    	/* begin seeking from current sector */
    	
    	startSec = pFd->curSec;
    	DBG_MSG(550, "SEEK_CUR : startSec = %lu\n", startSec,0,0,0,0,0,0,0 );
    	}
    
    /* go ! */
    
    if( pFd->pVolDesc->pFatDesc->seek( pFd, startSec, nSec ) == ERROR )
    	{
    	goto ret;
    	}

retOK:
    pFd->pos = newPos;
    retStat = OK;
    
    /*
     * special case, when the seek is done into the file last position,
     * and this position is also first position in sector.
     * Remember, that if newPos == size, we actually have moved
     * into (size-1)
     */
    if( newPos == pFd->pFileHdl->size &&
        OFFSET_IN_SEC( pFd->pVolDesc, newPos ) == 0 )
    	{
    	pFd->nSec --;
    	pFd->curSec ++;
    	}

ret:
    pFd->pFileHdl->size = sizeBuf;
    return retStat;
    } /* dosFsSeek() */

/***************************************************************************
*
* dosFsSeekDir - set current offset in directory.
*
* This routine sets current offset in directory.  It takes special
* care of contiguous root.
*
* File semaphore should be taken prior calling this routine.
*
* RETURNS: OK or ERROR, if seek is out of directory chain.
*/
LOCAL STATUS dosFsSeekDir
    (
    DOS_FILE_DESC_ID	pFd,	/* pointer to file descriptor */
    DIR *	pDir	/* seek for dd_cookie position */
    )
    {
    fsize_t	newOffset = DD_COOKIE_TO_POS( pDir );

    if( pFd->pos == newOffset ) /* at the place */
    	return OK;

    /* special process for contiguous root */

    if( IS_ROOT( pFd ) && pFd->pVolDesc->pDirDesc->rootNSec > 0 )
    	{
    	/* check for seek out of root */
    	
    	if( newOffset >= ( pFd->pVolDesc->pDirDesc->rootNSec <<
			   pFd->pVolDesc->secSizeShift ) )
    	    {
    	    errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	    return ERROR;
    	    }
    	
    	pFd->pos = newOffset;
    	newOffset = NSECTORS( pFd->pVolDesc, newOffset );
    	pFd->curSec = pFd->pVolDesc->pDirDesc->rootStartSec + 
    	    	      newOffset;
    	pFd->nSec = pFd->pVolDesc->pDirDesc->rootNSec - newOffset;
    	
    	return OK;
    	}
    	
    /* regular directory */

    return dosFsSeek( pFd, (fsize_t)newOffset );
    } /* dosFsSeekDir() */

/***************************************************************************
*
* dosFsIsDirEmpty - check if directory is empty.
*
* This routine checks if directory is not a root directory and
* whether it contains entries unless "." and "..".
*
* RETURNS: OK if directory is empty and not root, else ERROR.
*
* ERRNO:
* S_dosFsLib_DIR_NOT_EMPTY
*
*/
LOCAL STATUS dosFsIsDirEmpty
    (
    DOS_FILE_DESC_ID	pFd        /* pointer to file descriptor */
    )
    {
    DOS_DIR_DESC_ID	pDirDesc = pFd->pVolDesc->pDirDesc;
    DIR		dir;	/* use readDir calls */
    DOS_FILE_DESC	workFd = * pFd;	/* working file descriptor */
    
    assert( pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY );
    
    if( IS_ROOT( pFd ) )
    	goto ret;	/* root directory */
    
    dir.dd_cookie = 0;	/* rewind dir */
    
    /* pass "." and ".." */
    
    if( pDirDesc->readDir( &workFd, &dir, NULL ) == ERROR ||
        pDirDesc->readDir( &workFd, &dir, NULL ) == ERROR )
    	return ERROR;	/* illegal directory */
    
    /* try to get one more entry */
    
    if( pDirDesc->readDir( &workFd, &dir, NULL ) == ERROR )
    	return OK;	/* no more entries */
    
    DBG_MSG( 500, "name = %s\n", dir.dd_dirent.d_name,0,0,0,0,0,0,0 );
    
ret:    
    errnoSet( S_dosFsLib_DIR_NOT_EMPTY );
    return ERROR;
    } /* dosFsIsDirEmpty() */
    
/***************************************************************************
*
* dosFsTrunc - truncate a file.
* 
* This routine is called via an ioctl(), using the FIOTRUNC 
* function and from dosFsOpen(), when called with O_TRUNC flag.
* It causes the file specified by the file descriptor to be truncated
* to the specified <newLength>.  The directory entry for the file is
* updated to reflect the new length, but no clusters which
* were allocated to the file and became now unused are freed.
* These clusters are actually freed, when last
* file descriptor is currently opened for the file be closed.
*
* RETURNS: OK or ERROR, if pFd is opened for directory, or new size
* greater than the current size, or a disk access error occurs.
*
* ERRNO:
* S_dosFsLib_NOT_FILE
* S_dosFsLib_INVALID_NUMBER_OF_BYTES
* S_dosFsLib_READ_ONLY
*
*/

LOCAL STATUS dosFsTrunc
    (
    DOS_FILE_DESC_ID	pFd,        /* pointer to file descriptor */
    fsize_t	newLength       /* requested new file length */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    DOS_FILE_HDL_ID	pFileHdl = pFd->pFileHdl;
    
    assert( pVolDesc->magic == DOS_FS_MAGIC );
    assert( pFd - pFd->pVolDesc->pFdList < pFd->pVolDesc->maxFiles );
    assert( pFd->pFileHdl - pFd->pVolDesc->pFhdlList <
            pFd->pVolDesc->maxFiles );
    
    /* check for directory truncation */
    
    if( pFileHdl->attrib & DOS_ATTR_DIRECTORY )
    	{
    	errnoSet( S_dosFsLib_NOT_FILE );
    	return ERROR;
    	}
    
    if( pFileHdl->size == newLength )	/* nothing to do */
    	return OK;
    
    /* Check that new length is not greater than old one */
    
    if( pFileHdl->size < newLength )
    	{
    	DBG_MSG( 100,  "pFileHdl->size = %u < newLength = %u\n",
    	    	    (u_int)pFileHdl->size, (u_int)newLength,0,0,0,0,0,0 );
    	errnoSet( S_dosFsLib_INVALID_NUMBER_OF_BYTES );
    	return ERROR;
    	}
    
    /* Check that file is not read only */
 
    if ( pFd->openMode == O_RDONLY )
    	{
    	errnoSet (S_dosFsLib_READ_ONLY);
    	return ERROR;
    	}
    
    DBG_MSG( 500, "pFd:pFileHdl = %p:%p, newLength = %lu, size = %lu\n",
    		pFd, pFileHdl, newLength, pFileHdl->size,0,0,0,0 );
    
    /* update file size in the directory entry */
    
    pFileHdl->size = newLength;

    if( OK != pVolDesc->pDirDesc->updateEntry(pFd, 
                                              DH_TIME_MODIFY | DH_TIME_ACCESS, 
					      0))
    	{
    	DBG_MSG( 0, "Error updating directory entry, pFileHdl = %p\n",
    			pFileHdl,0,0,0,0,0,0,0 );
    	return ERROR;
    	}
    
    pFd->pFileHdl->changed = 1;
    
    return OK;
    } /* dosFsTrunc() */

/***************************************************************************
*
* dosFsOpen - open a file on a dosFs volume
*
* This routine opens the file <name> with the specified mode
* (O_RDONLY/O_WRONLY/O_RDWR/CREATE/TRUNC).  The directory structure is
* searched, and if the file is found a dosFs file descriptor
* is initialized for it.
* Extended flags are provided by DOS FS for more efficiency:
* .IP
* DOS_O_CONTIG_CHK - to check file for contiguity. 
* .IP
* DOS_O_CASENS - force the file name lookup in case insensitive manner,
* (if directory format provides such opportunity)
* .LP
*
* If this is the very first open for the volume,
* configuration data will be read from the disk automatically
* (via dosFsVolMount()).
*
* If a file is opened with O_CREAT | O_TRUNC, and the file 
* already exists, it is first deleted then (re-)created.
*
* RETURNS: A pointer to a dosFs file descriptor, or ERROR
* if the volume is not available,
* or there are no available dosFs file descriptors,
* or there is no such file and O_CREAT was not specified,
* or file can not be opened with such permissions.
*
* ERRNO:
* S_dosFsLib_INVALID_PARAMETER
* S_dosFsLib_READ_ONLY
* S_dosFsLib_FILE_NOT_FOUND
*
*/
 
LOCAL DOS_FILE_DESC_ID dosFsOpen
    (
    DOS_VOLUME_DESC_ID	pVolDesc, /* pointer to volume descriptor */
    char *	pPath,	/* dosFs full path/filename */
    int		flags,	/* file open flags */
    int		mode	/* file open permissions (mode) */
    )
    {
    DOS_FILE_DESC_ID	pFd = NULL;
    u_int	errnoBuf;
    BOOL	devSemOwned = FALSE;	/* device semaphore privated */
    BOOL	fileSemOwned = FALSE;	/* file semaphore privated */
    BOOL	error = TRUE;		/* result condition */

    
    if( (pVolDesc == NULL) || pVolDesc->magic != DOS_FS_MAGIC )
    	{
    	errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	return (void *)ERROR;
    	}

    if( pPath == NULL || strlen( pPath ) > PATH_MAX )
    	{
    	errnoSet( S_dosFsLib_ILLEGAL_PATH );
    	return (void *)ERROR;;
    	}
    
    /* prevent device from removing directory during a path search */
    
    if( semTake( pVolDesc->devSem, WAIT_FOREVER ) == ERROR )
    	goto ret;

    devSemOwned = TRUE;
    
    /* Call driver check-status routine, if any and mount the volume */
    
    if( (FALSE == pVolDesc->mounted) ||
        ( cbioIoctl(pVolDesc->pCbio, CBIO_STATUS_CHK, 0 ) == ERROR ) ||
        (TRUE == cbioRdyChgdGet (pVolDesc->pCbio)))
    	{
    	if( dosFsVolMount( pVolDesc ) == ERROR )
    	    goto ret;
    	}
    
    /* only regular file and directory creation are supported */
    
    if( ! S_ISDIR( mode ) )
    	mode = S_IFREG;
    DBG_MSG( 500, "path = %s, flags = %s%s%s%s mode = %d\n",
    		pPath,
    		( ((flags&0xff) == 0)? "O_RDONLY" : (
    		   ((flags&0xff) == 1)? "O_WRONLY" : (
    		    ((flags&0xff) == 2)? "O_RDWR" : "UNKN ?" ))),
    		( (flags&O_CREAT)? " | O_CREAT" : " " ),
    		( (flags&O_TRUNC)? " | O_TRUNC" : " " ),
    		( (flags&DOS_O_CONTIG_CHK)? " | DOS_O_CONTIG_CHK":" " ),
    		mode,0,0 );
    
    /*
     * check, that no file is being created, truncated
     * or opened for WRITE on read only  device.
     * It should be done after particular volume mounting
     */

    if( cbioModeGet(pVolDesc->pCbio) == O_RDONLY )
        {
    	if( (flags & O_CREAT) || (flags & O_TRUNC) ||
            (flags & 0x3) != 0 )
    	    {
            errnoSet( S_ioLib_WRITE_PROTECTED );
            goto ret;
            }
    	}


    /* 
     * try to delete the file if creating and 
     * truncating one with same name 
     */

    if ( (flags & O_CREAT) && (! S_ISDIR(mode)) && (flags & O_TRUNC) )
	{
	errnoBuf = errnoGet();
	/* attempt to delete old file */
	if (dosFsDelete (pVolDesc, pPath) == ERROR)
	    {
	    if (errnoGet() ==  S_dosFsLib_FILE_NOT_FOUND) /* ignore */
		{
		/* 
		 * dont care if file not found and next read will
		 * likely be from the cache MRU so not so slow.
		 */
		errnoSet( errnoBuf );
		}
	    else /* real error */
		{
		error = TRUE;
		goto ret;
	        }
	    }
	}

    /* allocate file descriptor and file handle */
    
    pFd = dosFsFdGet( pVolDesc );

    if( pFd == NULL )
    	goto ret;
    
     /* search for the file */
     
    errnoBuf = errnoGet();
    errnoSet( OK );
    if( pVolDesc->pDirDesc->pathLkup(
    		pFd, pPath,
                (flags & DOS_O_CASENS) | (flags & 0xf00) | mode ) == ERROR )
    	{
    	if( errnoGet() == OK )
    	    errnoSet( S_dosFsLib_FILE_NOT_FOUND );

    	goto ret;
    	}

    errnoSet( errnoBuf );
    
    /* share file handles in case of simultaneous file opening */
    
    dosFsHdlDeref( pFd );
    
    /* check open attributes */
    
    if( (pFd->pFileHdl->attrib & DOS_ATTR_RDONLY) &&
    	(flags & 0x0f) != O_RDONLY )
    	{
    	errnoSet( S_dosFsLib_READ_ONLY );
    	goto ret;
    	}
    
    /* set open mode */
    
    pFd->openMode = flags & 0x0f;

    /*
     * release device semaphore, because
     * file semaphores have to be taken ahead others.
     */
    semGive( pVolDesc->devSem );
    devSemOwned = FALSE;
        
    /*
     * it may be directory is being opened instead of existing file.
     * Preserve the file, and return EEXIST error.
     * fixed SPR #22227: ignore mode if O_CREAT was not requested.
     */

    if( (pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY) == 0 &&
    	(flags & O_CREAT) != 0 && S_ISDIR( mode ) )
    	{
    	errnoSet( S_dosFsLib_FILE_EXISTS );
    	error = TRUE;
        goto ret;
    	}

    /* service additional requests */
    
    if( flags & (O_TRUNC | DOS_O_CONTIG_CHK)  )
    	{
    	/* take control on file */
    	
    	dosFsFSemTake( pFd, WAIT_FOREVER );
    	fileSemOwned = TRUE;
    	
    	if( ! pFd->busy )
    	    {
    	    assert( pFd->busy );
    	    errnoSet (S_dosFsLib_INVALID_PARAMETER);
    	    goto ret;
    	    }
    	    	 
    	if( pFd->pFileHdl->deleted || pFd->pFileHdl->obsolet )
    	    {
    	    errnoSet( S_dosFsLib_FILE_NOT_FOUND );
    	    error = TRUE;
    	    goto ret;
    	    }
    	
    	if( flags & O_TRUNC )
    	    {
    	    u_int	openMode = pFd->openMode;
    	    STATUS	stat;
    	    
    	    /*
    	     * when file is newly created flag O_RDONLY may be supplied.
    	     * Because "file create" is implemented
    	     * via open( ... | O_TRUNC ... ), it can fail in
    	     * operation below in dosFsTrunc(). TO prevent the error
    	     * open flag is temporarily set to O_RDWR mode.
    	     */
    	    pFd->openMode  = O_RDWR; /* prevent error on rdonly open */
    	    stat = dosFsTrunc( pFd, 0 );
    	    pFd->openMode = openMode;
    	    
    	    if( stat == ERROR )
    	    	goto ret;
    	    } /* TRUNC */
    	
    	/* check for contiguity */
    	
    	if( flags & DOS_O_CONTIG_CHK )
    	    pVolDesc->pFatDesc->contigChk( pFd );    
    	
    	}
    
    error = FALSE;	/* file opened */

ret:    
    /* release the file and device */
    
    if( devSemOwned )
    	semGive( pVolDesc->devSem );
    if( fileSemOwned )
    	dosFsFSemGive( pFd );

    /* check state */
    
    if( error )	/* error opening file */
    	{
    	if( pFd != NULL )
    	    dosFsFdFree( pFd );	 /* free file descriptor */
    	return (void *)ERROR;
    	}
    
    return pFd;
    } /* dosFsOpen() */


/*******************************************************************************
*
* dosFsCreate - create a dosFs file
*
* This routine creates a file with the specified name and opens it
* with specified flags.
* If the file already exists, it is truncated to zero length, but not
* actually created.
* A dosFs file descriptor is initialized for the file.
*
* RETURNS: Pointer to dosFs file descriptor,
*  or ERROR if error in create.
*
*/
LOCAL DOS_FILE_DESC_ID	dosFsCreate
    (
    DOS_VOLUME_DESC_ID	pVolDesc, /* pointer to volume descriptor */
    char                *pName,	/* dosFs path/filename string */
    int                 flags	/* flags (O_RDONLY/O_WRONLY/O_RDWR) */
    )
    {
    DOS_FILE_DESC_ID	pFd;
    
    /*
     * create file via dosFsOpen().
     *
     * Do not make additional tests on device state, as it
     * is done in dosFsOpen()
     */
    
    pFd = dosFsOpen( pVolDesc, pName, flags | O_CREAT | O_TRUNC, S_IFREG );

    if( pFd == (void *)ERROR )
    	return pFd;	/* error creat file */
    
    /* set new create date */
    
    dosFsFSemTake( pFd, WAIT_FOREVER );

    if( ! pFd->busy || pFd->pFileHdl->deleted || 
    	pFd->pFileHdl->obsolet )
    	{
    	assert( FALSE );
    	errnoSet( S_dosFsLib_FILE_NOT_FOUND );
    	pFd = (void *)ERROR;
    	goto ret;
    	}

    pVolDesc->pDirDesc->updateEntry(pFd, (DH_TIME_CREAT  | 
                                          DH_TIME_ACCESS | 
                                          DH_TIME_MODIFY), 
                                    time( NULL ) );

ret:

    dosFsFSemGive( pFd );
    
    return pFd;
    } /* dosFsCreate() */

/*******************************************************************************
*
* dosFsClose - close a dosFs file
*
* This routine closes the specified dosFs file. If file contains
* excess clusters beyond EOF they are freed, when last
* file descriptor is being closed for that file.
*
* RETURNS: OK, or ERROR if directory couldn't be flushed
* or entry couldn't be found.
*
* ERRNO:
* S_dosFsLib_INVALID_PARAMETER
* S_dosFsLib_DELETED
* S_dosFsLib_FD_OBSOLETE
*/

LOCAL STATUS dosFsClose
    (
    DOS_FILE_DESC_ID	pFd	/* file descriptor pointer */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc;
    DOS_FILE_HDL_ID	pFileHdl = pFd->pFileHdl;
    STATUS	retStat = ERROR;
    BOOL	flushCache = FALSE;	/* blk I/O cache flushed */
    fsize_t	work = (fsize_t) (0);
    
    if( (pFd == NULL || pFd == (void *)ERROR  || ! pFd->busy) ||
        pFd->pVolDesc->magic != DOS_FS_MAGIC )
    	{
    	assert( FALSE );
    	errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	return ERROR;
    	}

    assert( pFd - pFd->pVolDesc->pFdList < pFd->pVolDesc->maxFiles );
    assert( pFd->pFileHdl - pFd->pVolDesc->pFhdlList <
            pFd->pVolDesc->maxFiles );

    pVolDesc = pFd->pVolDesc;
    
    /* Take control of file descriptor */
    
    if( dosFsFSemTake( pFd, WAIT_FOREVER ) == ERROR )
    	return ERROR;
    
    /*
     * If file was deleted or obsolete, free descriptor
     * but return error
     */
    if( pFileHdl->obsolet )
    	{
    	errnoSet( S_dosFsLib_FD_OBSOLETE );
    	retStat = ERROR;
    	goto ret;
    	}
    
    pVolDesc->nBusyFd--;	/* close normal file descriptor */
    
    if( pFileHdl->deleted )
    	{
	/*
	 * I directly assign <errno>, instead of using errnoSet, 
	 * because together with ERR_SET_SELF debug flag it
	 * becomes too much verbose
	 */
    	errno = S_dosFsLib_DELETED;
    	flushCache = TRUE;
    	goto ret;
    	}
    
    /* nothing to do for directory */
    
    if( pFileHdl->attrib & DOS_ATTR_DIRECTORY )
    	{
    	retStat = OK;
    	goto ret;
    	}

    /* 
     * SPR#68203, avoid writing last access date field 
     * on open-read-close.  
     */

    if (FALSE == pVolDesc->updateLastAccessDate)
        {
        pFd->accessed = 0;
        }

    /* update file's directory entry */
    
    if (pFd->accessed || pFd->changed )
        {
        u_int	timeFlag;

        timeFlag = (pFd->accessed) ? DH_TIME_ACCESS : 
                                     (DH_TIME_ACCESS | DH_TIME_MODIFY);

        if( cbioModeGet(pVolDesc->pCbio) != O_RDONLY)
    	    {
    	    pVolDesc->pDirDesc->updateEntry( pFd, timeFlag, time( NULL ) );
    	    pFd->accessed = 0;
            }
        else
	    {
            errnoSet( S_ioLib_WRITE_PROTECTED );
    	    goto ret;
            }
    	}
    	
    /*
     * flush buffers and deallocate unused clusters beyond EOF,
     * if last file descriptor is being closed for the file
     */

    if( pFd->changed || ( pFileHdl->changed && pFileHdl->nRef == 1 ) )
    	{
    	if( pFileHdl->nRef == 1 )	/* last fd for the file */
    	    {
    	    
    	    /* deallocate unused clusters */
    	
    	    /* 1) seek into the last position */
    	    work = 1; 
    	    work = ( pFileHdl->size == 0 )? 0 : pFileHdl->size - 1;
    	    dosFsSeek( pFd, work );
    	    
    	    /* 2) return clusters to free state */
    	    
    	    pVolDesc->pFatDesc->truncate(
    	    			pFd, pFd->curSec, FH_EXCLUDE );
    	    
    	    pFileHdl->changed = 0;
    	    }
    	
    	flushCache = TRUE;
    	pFd->changed = 0;
    	}
    
    retStat = OK;
    
ret:
    /* synchronize FAT */
    
    if( TRUE == flushCache )
    	pVolDesc->pFatDesc->flush( pFd );

    /* release file handle */
    
    dosFsFSemGive( pFd );
    
    /* free file descriptor */
    
    dosFsFdFree( pFd );
    
    /* force flush cache, when last opened file was closed */
    
    if( pVolDesc->nBusyFd == 0 || flushCache )
    	{
    	cbioIoctl( pVolDesc->pCbio, CBIO_CACHE_FLUSH, 0 );
    	flushCache = FALSE;
    	}
    
    return (retStat);
    } /* dosFsClose() */

/*******************************************************************************
*
* dosFsDeleteByFd - delete a file, described by file descriptor.
*
* This routine deletes the file the <pFd> was opened for it.
*
* RETURNS: OK, or ERROR if the file not found, is read only,
* or is not empty directory, if
* the volume is not available, or there are problems writing out.
*
* ERRNO:
* S_dosFsLib_DIR_NOT_EMPTY
* S_dosFsLib_FD_OBSOLETE
*/
LOCAL STATUS dosFsDeleteByFd
    (
    DOS_FILE_DESC_ID	pFd
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    STATUS	retStat = ERROR;
    BOOL	devSemOwned = FALSE;	/* device semaphore privated */
    BOOL	fileSemOwned = FALSE;	/* file semaphore privated */
    
    /* take control on the file */
    
    if( dosFsFSemTake( pFd, WAIT_FOREVER ) == ERROR )
    	return ERROR;
    fileSemOwned = TRUE;
    
    /* check the file state */
    
    if( ! pFd->busy )
    	{
    	assert( pFd->busy );
    	errnoSet (S_dosFsLib_INVALID_PARAMETER);
    	goto ret;
    	}
    if( pFd->pFileHdl->obsolet )
    	{
    	errnoSet (S_dosFsLib_FD_OBSOLETE);
    	goto ret;
    	}
    if( pFd->pFileHdl->deleted )
    	{
    	retStat = OK;
    	goto ret;
    	}
    
    /* take control on the volume */
    
    if( semTake( pVolDesc->devSem, WAIT_FOREVER ) == ERROR )
    	goto ret;
    devSemOwned = TRUE;
    
    /* check for directory */
    
    if( pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY )
    	{
    	/* only empty directory can be deleted */
    	
    	if( dosFsIsDirEmpty( pFd ) == ERROR )
    	    {
    	    goto ret;
    	    }
    	}
    
    /* mark entry as removed */
    
    if( pVolDesc->pDirDesc->updateEntry( pFd, DH_DELETE, 0 ) == ERROR )
    	goto ret;
    
    /* invalidate file handle */
    
    pFd->pFileHdl->deleted = 1;
    
    /* remove FAT chain */
    
    if( pFd->pFileHdl->startClust != 0 )
    	{
    	if( pVolDesc->pFatDesc->truncate(
    	    		pFd, FH_FILE_START, FH_INCLUDE ) == ERROR )
    	    goto ret;
    	}
    
    retStat = OK;
    
ret:
    if( devSemOwned )
    	semGive( pVolDesc->devSem );
    if( fileSemOwned )
    	dosFsFSemGive( pFd );
    
    return retStat;
    } /* dosFsDeleteByFd() */

/*******************************************************************************
*
* dosFsDelete - delete a dosFs file/directory
*
* This routine deletes the file <path> from the specified dosFs volume.
*
* RETURNS: OK, or ERROR if the file not found, is read only,
* or is not empty directory, if
* the volume is not available, or there are problems writing out.
*
* ERRNO:
*/

LOCAL STATUS dosFsDelete
    (
    DOS_VOLUME_DESC_ID	pVolDesc,	/* ptr to volume descriptor */
    char *		pPath           /* dosFs path/filename */
    )
    {
    DOS_FILE_DESC_ID	pFd;
    u_int	errnoBuf;	/* errno backup */
    STATUS	retStat = ERROR;
    
    /* open the file */
    
    pFd = dosFsOpen( pVolDesc, pPath, O_WRONLY, 0 );
    if( pFd == (void *)ERROR )
    	return ERROR;
    
    retStat = dosFsDeleteByFd( pFd );	/* actually delete */
    /*
     * because 'deleted' flag is set in file handle
     * dosFsClose() will set errno to S_dosFsLib_DELETED, 
     * that is not right.
     */
    errnoBuf = errnoGet();
    dosFsClose( pFd );
    errnoSet( errnoBuf );
    
    return retStat;
    } /* dosFsDelete() */

/*******************************************************************************
*
* dosFsFillGap -  fill gap from current EOF to current position.
*
* seek operation can set current position beyond EOF.
* This routine fills lacked gap. File size and current position
* are set to point immediate after the gap.
*
* RETURNS: OK or ERROR if write or cluster alloc failed.
*/
LOCAL STATUS dosFsFillGap
    (
    FAST DOS_FILE_DESC_ID	pFd,	/* file descriptor ptr */
    u_int	fatFlags	/* clusters allocation policy */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    					/* volume descriptor ptr */
    DOS_FILE_HDL_ID	pFileHdl = pFd->pFileHdl; /* file handle */
    FAST CBIO_DEV_ID	pCbio = pFd->pVolDesc->pCbio;
    fsize_t	seekOutPos = pFd->seekOutPos;

    assert( seekOutPos != 0 );

    /* seek to the current last file position */
    
    if( dosFsSeek( pFd, pFileHdl->size ) == ERROR )
    	return ERROR;

    /* pass current sector */

    /* 
     * SPR#30540, not every seek crosses a sector boundary: was (wrong):
     *
     * /@ pass current sector @/
     *
     *     if( pFileHdl->size > 0 &&
     *            OFFSET_IN_SEC( pVolDesc, pFd->pos ) != 0 )
     *
     * Why not?  The above check registers true whenever the file has a size 
     * greater than 0, and also when the offset of the current pos of the
     * file pointer into the section in question is NOT 0, ie anywhere
     * besides byte 0 of the section.
     *
     * This means that just about every call to dosFsFillGap will execute
     * the code between the braces.  Even code that shouldn't:
     *
     * -------------------------------------------------------------
     *            0        1               2
     * ||||||||||||               s
     * -------------------------------------------------------------
     *            ^ EOF pos       ^ seekOutPos
     *
     * if the sector demarcation is at two, then the new seekOutPos that is
     * being moved to does not cross the sector boundary.  If it is at
     * one, then it does.  Only need to execute the code in the braces if
     * the sector boundary falls between the old pos and the new seekOutpos
     * (case 1).  Also, when dealing with case 2, remember that at some point
     * in the past the block of space after EOF and up to the end of the
     * sector was 'filled' when the sector was acquired.
     
     * If the absolute distance between current EOF and seekOutPos is less
     * than the absolute distance between the current EOF and the sector
     * boundary, then the sector indicators need not be adjusted.  In this
     * case, since the sector has already been zeroed when it was originally
     * seeked to, the other half of this routine doesn't need running
     * either, aside from moving pFd->pos and pFileHdl->size.
     */

    if( ( seekOutPos - pFd->pos ) <
        ( pVolDesc->bytesPerSec - OFFSET_IN_SEC( pVolDesc, pFd->pos ) ))
        {
        /* condition met, nothing to do update some internal values */
        /* the sectore has already been zeroed out. */
    	pFileHdl->size = pFd->pos = seekOutPos;
        return OK;
        }
    else 
        {
        /* conditions indicate we've jumped the sector boundary */
    	pFd->nSec --;
    	pFd->curSec ++;
    	pFd->pos += min( seekOutPos - pFd->pos,
                        	pVolDesc->bytesPerSec -
                         	OFFSET_IN_SEC( pVolDesc, pFd->pos ) );
    	pFileHdl->size = pFd->pos;
        }
 
    /* fill entire sectors */

    while( pFd->pos < seekOutPos )
    	{
    	/* get next contiguous block, if current already finished */
    	
    	if( pFd->nSec == 0 )
    	    {
    	    if( pVolDesc->pFatDesc->getNext( pFd, fatFlags ) == ERROR )
    	    	return ERROR;
    	    }
    	
    	/* fill sector */

     	if(cbioIoctl(pCbio, CBIO_CACHE_NEWBLK, (void *)pFd->curSec ) == ERROR)
            {
            return (ERROR);
            }

    	/* correct position */
    	
    	pFd->pos += min( seekOutPos - pFd->pos, pVolDesc->bytesPerSec );
    	pFileHdl->size = pFd->pos;
    	
    	if( OFFSET_IN_SEC( pVolDesc, pFd->pos ) == 0 )
    	    {
    	    pFd->curSec ++;
    	    pFd->nSec --;
    	    }
    	} /* while */

    return OK;
    } /* dosFsFillGap() */

/*******************************************************************************
*
* dosFsFileRW - read/write a file.
*
* This function is called from both dosFsRead() and dosFsWrite().
*
* This routine reads/writes the requested number of bytes
* from/to a file.
*
* RETURNS: number of bytes successfully processed, 0 on EOF,
* or ERROR if file can not be accessed by required way.
*
* ERRNO:
* S_dosFsLib_INVALID_PARAMETER
* S_dosFsLib_VOLUME_NOT_AVAILABLE
* S_dosFsLib_NOT_FILE
* S_dosFsLib_FD_OBSOLETE
* S_dosFsLib_DELETED
* S_dosFsLib_DISK_FULL
* S_dosFsLib_WRITE_ONLY
* S_dosFsLib_READ_ONLY
* S_dosFsLib_ACCESS_BEYOND_EOF
*
*/

LOCAL int dosFsFileRW
    (
    FAST DOS_FILE_DESC_ID	pFd,	/* file descriptor ptr */
    char *	pBuf,		/* addr of destination/source buffer */
    int		maxBytes,	/* maximum bytes to read/write buffer */
    u_int	operation	/* _FREAD/FWRITE */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    					/* volume descriptor ptr */
    DOS_DIR_PDESCR_ID	pDirDesc;
    DOS_FILE_HDL_ID	pFileHdl = pFd->pFileHdl; /* file handle */
    FAST CBIO_DEV_ID	pCbio = pFd->pVolDesc->pCbio;
    					/* cached I/O descriptor ptr */
    FAST size_t		remainBytes = maxBytes;	/* work counter */
    FAST size_t	numRW = 0;	/* number of bytes/sectors to accept */
    FAST off_t	work;		/* short work buffer */
    u_int	fatFlags = 0;	/* flags to fat getNext call */
    BOOL	error = TRUE;	/* result flag */

    if( (pFd == NULL || pFd == (void *)ERROR) ||
        pFd->pVolDesc->magic != DOS_FS_MAGIC )
    	{
    	errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	return ERROR;
    	}
    
    assert( pFd - pFd->pVolDesc->pFdList < pFd->pVolDesc->maxFiles );
    assert( pFd->pFileHdl - pFd->pVolDesc->pFhdlList <
            pFd->pVolDesc->maxFiles );

    /* check volume state */
    
    if( (FALSE == pFd->pVolDesc->mounted) ||
        (TRUE == cbioRdyChgdGet (pFd->pVolDesc->pCbio)))
    	{
    	errnoSet( S_dosFsLib_FD_OBSOLETE );
    	return ERROR;
    	}
    
    assert( operation == _FREAD || operation == _FWRITE );
    	
    /* private control on file */
    
    if( dosFsFSemTake( pFd, WAIT_FOREVER ) == ERROR )
    	return ERROR;
    
    /* DOS does not access directories as a regular data files */
    
    if( pFileHdl->attrib & DOS_ATTR_DIRECTORY )
    	{
        dosFsFSemGive (pFd);
    	errnoSet (S_dosFsLib_NOT_FILE);
    	return ERROR;
    	}
    
    if( maxBytes == 0 )	/* nothing to be done */
	{
        dosFsFSemGive (pFd);
    	return (0);
	}
    
    /* check file state */
    
    if( ! pFd->busy )
    	{
    	assert( pFd->busy );
    	errnoSet (S_dosFsLib_INVALID_PARAMETER);
    	goto ret;
    	}
    if( pFileHdl->obsolet )
    	{
    	errnoSet (S_dosFsLib_FD_OBSOLETE);
    	goto ret;
    	}
    else if( pFileHdl->deleted )
    	{
    	errnoSet (S_dosFsLib_DELETED);
    	goto ret;
    	}

    /* we support seek for position passed EOF */
    
    if( pFd->seekOutPos != 0 && pFd->seekOutPos <= pFileHdl->size )
    	{
    	if( dosFsSeek( pFd, pFd->seekOutPos ) == ERROR )
    	    goto ret;
    	}

    /* init parameters */
    
    if( operation == _FREAD )	/* read data */
    	{
    	if( pFd->openMode == O_WRONLY )	/* write only */
    	    {
    	    errnoSet (S_dosFsLib_WRITE_ONLY);
    	    goto ret;
    	    }

	/* seek passed current EOF not allowed on READ */

    	if((pFd->seekOutPos != 0) || (pFd->pos > pFileHdl->size))
    	    {
    	    errnoSet( S_dosFsLib_ACCESS_BEYOND_EOF );
            error = FALSE;  /* return 0, not ERROR */
            goto ret;
            }

    	if( pFd->pos == pFileHdl->size ) /* EOF already reached */
    	    {
    	    DBG_MSG( 100, "EOF on read: Pos = %lu, size = %lu\n",
    	    		pFd->pos, pFileHdl->size,0,0,0,0,0,0 );
    	    error = FALSE;
    	    goto ret;
    	    }

    	operation = CBIO_READ;
    	fatFlags = FAT_NOT_ALLOC;
    	pFd->accessed = 1;

    	/* read data up to file size */

    	maxBytes = min( maxBytes, pFileHdl->size - pFd->pos );
    	}
    else	/* _FWRITE - write data */
    	{
    	if( pFd->openMode == O_RDONLY )	/* read only */
    	    {
    	    errnoSet (S_dosFsLib_READ_ONLY);
    	    goto ret;
    	    }

        /*
	 * Dont allow larger than 4GB file on anything but
	 * VXLONGNAMES directory entry files. SPR#27532.
	 */

	pDirDesc = (void *)pVolDesc->pDirDesc;

	if (pDirDesc->nameStyle != VXLONG) /* >4GB ok on VxLong */
	    {
#ifdef SIZE64
	    /* 
	     * Using DOS_IS_VAL_HUGE is faster but works only when SIZE64 
	     * is defined in dosFsLibP.h 
	     */
	    if ((DOS_IS_VAL_HUGE(pFd->pos + maxBytes)) == TRUE)
#else 
	    if ((0xffffffff - pFd->pos) < maxBytes)
#endif
		{
	    	DBG_MSG(1,"Cannot write more than 4GB without VXLONGNAMES.\n",
			0,0,0,0,0,0,0,0 );

		/* attempt write if big enough, else return 0. */
    		maxBytes = min( maxBytes, 
			       ((fsize_t)(0xffffffff) - (fsize_t)(pFd->pos)));

		if (maxBytes < 0x2)
		    {
	    	    errnoSet( S_dosFsLib_ACCESS_BEYOND_EOF );
 	            error = FALSE;  /* return 0, not ERROR */
  	            goto ret;
		    }
		}
	    }

    	
    	/* choose allocation policy */
    	
    	if( maxBytes <=
    	    (pVolDesc->secPerClust << pVolDesc->secSizeShift) )
    	    {
    	    fatFlags = FAT_ALLOC_ONE;	/* one-cluster allocation */
    	    }
    	else
    	    {
    	    fatFlags = FAT_ALLOC;	/* group allocation */
    	    }
    	DBG_MSG( 400, "Allocate policy = %p\n", (void *)fatFlags,
		 0,0,0,0,0,0,0 );
    	    	
    	/*
    	 * seek operation can set current position beyond EOF.
    	 * Fill the lacked gap.
     	*/
    	if( pFd->seekOutPos  != 0 )
    	    {
     	    if( dosFsFillGap( pFd, fatFlags ) == ERROR )
            	goto ret; 
    	    }

    	operation = CBIO_WRITE;
    	
    	pFileHdl->changed = 1;
    	pFd->changed = 1;
    	pFd->accessed = 0;	/* base upon pFd->changed */
    	
    	error = FALSE;	/* return value other, than maxBytes, */
    			/* indicates an error itself */
    	}
    	
    /* get next contiguous block, if current already finished */
    
    work = pFileHdl->startClust;	/* backup for future */

    if( pFd->nSec == 0 )
        {
        if( pVolDesc->pFatDesc->getNext( pFd, fatFlags ) == ERROR )
	    {
	    /* 
             * Return ERROR on condition of full disk per POSIX 
             * make sure we actually fill the disk though.
             */
            if (errnoGet() == S_dosFsLib_DISK_FULL)
		{
	    	error = TRUE; 
		}
    	    goto ret;
	    }
        }

    assert( pFd->nSec != 0 );
    /*
     * it may be first data in the file.
     * In this case file start cluster have been just allocated and
     * "start cluster" field in file's directory entry is 0 yet
     */
    if( work == 0 )
    	{
    	DBG_MSG( 400, "pFileHdl = %p : first cluster in chain = %u\n",
    			pFileHdl, pFileHdl->startClust,0,0,0,0,0,0 );
    	
    	if( pVolDesc->pDirDesc->updateEntry( pFd, 0, 0 ) == ERROR )
    	    {
    	    pFileHdl->obsolet = 1;
    	    pFd->pVolDesc->nBusyFd--;
    	    goto ret;
    	    }
    	}
    assert( pFileHdl->startClust != 0 );

    /* init bytes down-counter */

    remainBytes = maxBytes;
    
    /* for the beginning, process remain part of current sector */
    	
    work = OFFSET_IN_SEC( pVolDesc, pFd->pos ); /* offset in sector */

    if( work != 0 )
    	{
    	numRW = min( remainBytes, (size_t)pVolDesc->bytesPerSec - work );
    	
    	DBG_MSG( 600, "rest bytes in sec; op=%s off=%u numRW=%u\n",
    		 ( (operation==CBIO_READ)? "_FREAD":"_FWRITE" ),
    		 work, numRW,0,0,0,0,0 );
    	
    	if( cbioBytesRW(pCbio, pFd->curSec, work, pBuf,
    	    		numRW, operation,
    	    		&pFd->cbioCookie ) == ERROR )
    	    {
    	    goto ret;
    	    }
    	
    	remainBytes	-= numRW;
    	pBuf		+= numRW;
    	pFd->pos	+= numRW;
    	
    	/* it may be current sector exhausted */
    	    
    	if( OFFSET_IN_SEC( pVolDesc, pFd->pos ) == 0 )
    	    {
    	    pFd->cbioCookie = (cookie_t) NULL;
    	    pFd->curSec ++;
    	    pFd->nSec --;
    	    }
    	} /* if( offset != 0 ) */
    	
    /* main loop: read entire sectors */
    
    while( remainBytes >= pVolDesc->bytesPerSec )
    	{
    	/* get next contiguous block, if current already finished */
    	
    	if( pFd->nSec == 0 )
    	    {
    	    if( pVolDesc->pFatDesc->getNext( pFd, fatFlags ) == ERROR )
    	    	goto ret;
    	    }
    	
    	/* number of sectors to R/W */
    	
    	numRW = min( NSECTORS( pVolDesc, remainBytes ), pFd->nSec );
    	assert( numRW > 0 );
    	
    	/* R/W data */
    	
    	DBG_MSG( 600, "entire sectors; op=%s numRW=%u\n",
    		 ( (operation==_FREAD)? "_FREAD":"_FWRITE" ), numRW,
		 0,0,0,0,0,0 );
    		 
    	if( cbioBlkRW( pCbio, pFd->curSec, numRW,
    	    	       pBuf, operation, NULL ) == ERROR )
    	    {
    	    goto ret;
    	    }
    	
    	/* correct position */
    	
    	work = numRW << pVolDesc->secSizeShift;
    	
    	remainBytes	-= work;
    	pBuf		+= work;
    	pFd->pos	+= work;
    	pFd->curSec	+= numRW;
    	pFd->nSec	-= numRW;
    	} /* while ... */

    /* now process remain part of data, that is shorter, than sector */
    
    assert( remainBytes < pVolDesc->bytesPerSec );
    numRW = remainBytes;
    if( numRW > 0 )
    	{
    	DBG_MSG( 600, "part of sec; op=%s numRW=%u\n",
    		 ( (operation==CBIO_READ)? "_FREAD":"_FWRITE" ), numRW,
		 0,0,0,0,0,0 );

        /* get next contiguous block, if current already finished */
        
        if( pFd->nSec == 0 )
            {
            if( pVolDesc->pFatDesc->getNext( pFd, fatFlags ) == ERROR )
                goto ret;
            }

    	/* read/write data */
    	
    	/*
    	 * prevent cbio from reading a very new sector into
    	 * cache, because it does not contain yet any data
    	 */
    	if( pFd->pos >= pFileHdl->size )	/* append file */
    	    {
    	    assert( operation == CBIO_WRITE );
    	    if( cbioIoctl( pCbio, CBIO_CACHE_NEWBLK,
    	    		   (void *)pFd->curSec ) == ERROR )
    	    	{
    	    	goto ret;
    	    	}
    	    }

    	if( cbioBytesRW( pCbio, pFd->curSec, 0, pBuf,
    	    		 numRW, operation,
    	    		 &pFd->cbioCookie ) == ERROR )
    	    {
    	    goto ret;
    	    }

    	remainBytes	-= numRW;
    	pBuf		+= numRW;
    	pFd->pos	+= numRW;
    	}
    	    
    error = FALSE;	/* all right */
     	
ret:

    /* update file descriptor */
    
    if( operation == CBIO_WRITE && (size_t)remainBytes < (size_t)maxBytes )
    	{
        if( pFd->pos > pFileHdl->size )
            pFileHdl->size = pFd->pos;
        
        pVolDesc->pDirDesc->updateEntry(
        	pFd, DH_TIME_ACCESS | DH_TIME_MODIFY, 0 );
    	pFd->changed = 0;
        }

    dosFsFSemGive( pFd );
    
    if( error )
    	return (ERROR);

    DBG_MSG( 600, "Result = %u\n", maxBytes - remainBytes,0,0,0,0,0,0,0 );

    return (maxBytes - remainBytes);
    } /* dosFsFileRW() */
    
/*******************************************************************************
*
* dosFsRead - read a file.
*
* This routine reads the requested number of bytes from a file.
*
* RETURNS: number of bytes successfully read, or 0 if EOF,
* or ERROR if file is unreadable.
*
*/

LOCAL int dosFsRead
    (
    FAST DOS_FILE_DESC_ID	pFd,	/* file descriptor pointer */
    char *	pBuf,		/* addr of destination buffer */
    int		maxBytes	/* maximum bytes to read into buffer */
    )
    {
    if ( cbioModeGet(pFd->pVolDesc->pCbio) == O_WRONLY )
        {
        ERR_MSG(1,"No reading from write only device.\n",0,0,0,0,0,0 );
	return (ERROR);
	}

    return dosFsFileRW( pFd, pBuf, maxBytes, _FREAD );
    } /* dosFsRead() */
    
/*******************************************************************************
*
* dosFsWrite - read a file.
*
* This routine writes the requested number of bytes to a file.
*
* RETURNS: number of bytes successfully written,
* or ERROR if file is unwritable.
*
*/

LOCAL int dosFsWrite
    (
    FAST DOS_FILE_DESC_ID	pFd,	/* file descriptor pointer */
    char *	pBuf,		/* addr of src buffer */
    int		maxBytes	/* maximum bytes to read into buffer */
    )
    {
    if ( cbioModeGet(pFd->pVolDesc->pCbio) == O_RDONLY )
        {
        errnoSet( S_ioLib_WRITE_PROTECTED );
	return (ERROR);
	}

    return dosFsFileRW( pFd, pBuf, maxBytes, _FWRITE );
    } /* dosFsWrite() */

/*******************************************************************************
*
* dosFsStatGet - get file status (directory entry data)
*
* This routine is called via an ioctl() call, using the FIOFSTATGET
* function code.  The passed stat structure is filled, using data
* obtained from the directory entry which describes the file.
*
* RETURNS: OK or ERROR if disk access error.
*
*/

LOCAL STATUS dosFsStatGet
    (
    DOS_FILE_DESC  *pFd,	/* pointer to file descriptor */
    struct stat    *pStat	/* structure to fill with data */
    )
    {
    DOS_FILE_HDL_ID	pFileHdl = pFd->pFileHdl;
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    
    bzero ((char *) pStat, sizeof (struct stat));
    
    /* Fill stat structure */

    /* 
     * Not supported fields:
     * pStat->st_ino
     * pStat->st_uid
     * pStat->st_gid
     * pStat->st_rdev
     */
    
    pStat->st_dev     = (u_long) pVolDesc; /* device ID = DEV_HDR addr */

    pStat->st_nlink   = 1;		/* always only one link */

    /* st_size is 32-bit field only */

    pStat->st_size    = pFileHdl->size;	/* file size, in bytes */

    if( dosFsIsValHuge( pFileHdl->size ) )
    	{
    	errnoSet( S_dosFsLib_32BIT_OVERFLOW );
	pStat->st_size = (size_t)(-1); /* HELP 64 bit??? */
    	}

    /* block size = cluster size shifted */
    pStat->st_blksize = pVolDesc->secPerClust << pVolDesc->secSizeShift;
    
    if (0 == pStat->st_blksize)
        {
	/* avoid dividing by zero */
	return (ERROR);
        }

    pStat->st_blocks  = ( pStat->st_size + (pStat->st_blksize-1)) / 
    			pStat->st_blksize;	/* no. of blocks */

    pStat->st_attrib  = pFileHdl->attrib;	/* file attribute byte */
    
    /* Set file type in mode field */
    
    pStat->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;

    if( ( pFileHdl->attrib & DOS_ATTR_RDONLY ) != 0 )
    	pStat->st_mode &= ~( S_IWUSR | S_IWGRP | S_IWOTH );
    
    if ( pFileHdl->attrib & DOS_ATTR_DIRECTORY )
	{
	pStat->st_mode |= S_IFDIR;	/* set bits in mode field */

	pStat->st_size = pStat->st_blksize; /* make it look like dir */
					    /* uses one block */
	}
    else  /* if not a volume label */
	{
	assert( ! (pFileHdl->attrib & DOS_ATTR_VOL_LABEL) );

	pStat->st_mode |= S_IFREG;	/*  it is a regular file */
	}
    
    /* Fill in modified date and time */
    
    if( pVolDesc->pDirDesc->dateGet( pFd, pStat ) == ERROR )
    	return ERROR;
    
    return (OK);
    } /* dosFsStatGet() */

/******************************************************************************
*
* dosFsFSStatGet - get statistics about entire file system
*
* Used by fstatfs() and statfs() to get information about a file
* system.  Called through dosFsIoctl() with FIOFSTATFSGET code.
*
* RETURNS:  OK.
*/

LOCAL STATUS dosFsFSStatGet
    (
    DOS_FILE_DESC_ID	pFd,        /* pointer to file descriptor */
    struct statfs *	pStat          /* structure to fill with data */
    )
    {
    pStat->f_type   = 0;

    pStat->f_bsize  = pFd->pVolDesc->secPerClust <<
		      pFd->pVolDesc->secSizeShift;

    pStat->f_blocks = pFd->pVolDesc->nFatEnts;

    if (0 != pFd->pVolDesc->secPerClust)
        {
        pStat->f_bfree  = ( pFd->pVolDesc->pFatDesc->nFree( pFd ) >>
 		            pFd->pVolDesc->secSizeShift ) / 
		            pFd->pVolDesc->secPerClust;
        }

    pStat->f_bavail = pStat->f_bfree;

    pStat->f_files  = -1;

    pStat->f_ffree  = -1;

    pStat->f_fsid.val[0] = (long) pFd->pVolDesc->volId;

    pStat->f_fsid.val[1] = 0;

    return (OK);
    }

/*******************************************************************************
*
* dosFsRename - change name of dosFs file
*
* This routine changes the name of the specified file to the specified
* new name.
*
* RETURNS: OK, or ERROR if pNewName already in use, 
* or unable to write out new directory info.
*
* ERRNO:
* S_ioLib_NO_FILENAME
* S_dosFsLib_NOT_SAME_VOLUME
* S_ioLib_WRITE_PROTECTED
* S_dosFsLib_VOLUME_NOT_AVAILABLE
* S_dosFsLib_NOT_FILE
*/

LOCAL STATUS dosFsRename
    (
    DOS_FILE_DESC_ID	pFdOld,	/* pointer to file descriptor   */
    char *	pNewName,       /* change name of file to this  */
    BOOL        allowOverwrite  /* allow dest. file to be overwritten */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc;
    				/* ptr to volume descriptor */
    DOS_FILE_HDL_ID	pFileHdlOld = pFdOld->pFileHdl;
    				/* old file's file handle ptr */
    DOS_FILE_DESC_ID	pFdNew = NULL;
    				/* new file descriptor */
    DOS_FILE_HDL_ID	pFileHdlNew = NULL;
    				/* new file's file handle ptr */
    u_int	errnoBuf;	/* errno backup */
    struct stat fStat;	        /* structure to fill with data */
    DOS_DIR_HDL	oldDirHdl;	/* backup of directory handle */
    STATUS	retStat = ERROR;   /* function return status */
    u_char	newPath [PATH_MAX + 1];


    /* Get volume descriptor and real pathname */

    if ( (pNewName == NULL) || (pNewName[0] == EOS) )
	{
	errnoSet ( S_ioLib_NO_FILENAME );
	return (ERROR);
	}

    /* get full path to the new file */

    if (ioFullFileNameGet (pNewName, (DEV_HDR **) &pVolDesc, newPath) != OK)
      {
      return (ERROR);
      }

    /* Old and new devices must be the same */

    if ( pVolDesc != pFdOld->pVolDesc )
    	{
    	errnoSet( S_dosFsLib_NOT_SAME_VOLUME );
	return (ERROR);
	}
    
    /* Disk must be writable to modify directory */

    if ( cbioModeGet(pVolDesc->pCbio) == O_RDONLY )
        {
        errnoSet( S_ioLib_WRITE_PROTECTED );
	return (ERROR);
	}

    /* 
     * check to see if the new file path to rename towards already
     * exists. We never rename a file to an existing filename.
     * however in FIOMOVE case if allowOverwrite is TRUE it is ok.
     */

    if (FALSE == allowOverwrite)
        {
        pFdNew = dosFsOpen( pVolDesc, 
                            (char *) newPath,
                            (O_RDONLY | 
                             (pVolDesc->volIsCaseSens ? DOS_O_CASENS : 0)),
                            0 );

        if (ERROR != (int) pFdNew)
            {
            dosFsClose(pFdNew);
    	    errnoSet( S_dosFsLib_FILE_EXISTS );
            return (ERROR);
            }
        else
            {
    	    errnoSet( OK );
            }
        }
    
    /* get creation time etc. of file being renamed */    

    if( pVolDesc->pDirDesc->dateGet( pFdOld, &fStat ) == ERROR )
        {
        return ERROR;
        }

    /*
     * create the file to rename to, but
     * prevent from file self-name renaming.
     */

    FOREVER
    	{

	/* 
         * SPR 29751 - Made the case sensitivity depend upon a switch in 
         * the volume descriptor. Initially set to FALSE but can be set 
         * with an ioctl
         */

	/* Create the new name entry */
    	pFdNew = dosFsOpen( pVolDesc, 
                            (char *)newPath, 
                            (O_CREAT | O_WRONLY | 
                             (pVolDesc->volIsCaseSens ? DOS_O_CASENS : 0)),
                            0 );
    
    	if( pFdNew == (void *)ERROR )
	    {
	    return (ERROR);
	    }

    	/* get ownership of new file */

    	if( dosFsFSemTake( pFdNew, WAIT_FOREVER ) == ERROR )
            {
            dosFsClose (pFdNew);
	    return (ERROR);
            }

    	/* get ownership of volume */

    	semTake( pVolDesc->devSem, WAIT_FOREVER );

        /* do not allow a rename to original name */

    	if( pFdNew->pFileHdl == pFileHdlOld )
	    {
    	    semGive ( pVolDesc->devSem );
            dosFsFSemGive (pFdNew);
    	    dosFsClose( pFdNew );
    	    errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	    return (ERROR);
    	    }
    	
    	pFileHdlNew = pFdNew->pFileHdl;
    
    	/* check again new file state ( after semTake() ) */
    	
    	if( ! pVolDesc->mounted || pFileHdlNew->obsolet )
    	    {
    	    /* volume have been unmounted */
    	    
    	    errnoSet( S_dosFsLib_VOLUME_NOT_AVAILABLE );

    	    goto ret;
    	    }
    	
    	if( ! pFileHdlNew->deleted )
    	    break;
	 
        /* somebody had time to delete the file. Create again */
	     
    	semGive( pVolDesc->devSem );

    	dosFsFSemGive( pFdNew );
    	} /* FOREVER */
    
    /* protect an existing directory from being a destination */
    
    if( pFileHdlNew->attrib & DOS_ATTR_DIRECTORY )
    	{
    	errnoSet( S_dosFsLib_NOT_FILE );
    	goto ret;
    	}
    
    /* prevent access to the new file via its share file handle */
    
    pFileHdlNew->deleted = 1;
    
    /* repoint new directory chain onto the renamed file chain */
    
    oldDirHdl = pFileHdlOld->dirHdl; /* backup dir handle */

    pFileHdlOld->dirHdl = pFileHdlNew->dirHdl;


    /* restore original dirent times */

    if( pVolDesc->pDirDesc->updateEntry( pFdOld, DH_TIME_CREAT, 
    				         fStat.st_ctime ) == ERROR )
    	{
    	pFileHdlOld->dirHdl = oldDirHdl;
    	goto ret;
    	}
    
    if( pVolDesc->pDirDesc->updateEntry( pFdOld, DH_TIME_MODIFY, 
    				         fStat.st_mtime ) == ERROR )
    	{
    	pFileHdlOld->dirHdl = oldDirHdl;
    	goto ret;
    	}
    
    if( pVolDesc->pDirDesc->updateEntry( pFdOld, DH_TIME_ACCESS, 
    				         fStat.st_atime ) == ERROR )
    	{
    	pFileHdlOld->dirHdl = oldDirHdl;
    	goto ret;
    	}
    
    /* mark old directory entry as removed */
    
    pFileHdlOld->dirHdl = oldDirHdl;

    pVolDesc->pDirDesc->updateEntry( pFdOld, DH_DELETE, 0 );

    pFileHdlOld->dirHdl = pFileHdlNew->dirHdl;
    
    /* remove destination fat chain, if it was not a new file */
    
    if( pFileHdlNew->startClust != 0 )
    	{
    	pVolDesc->pFatDesc->truncate( pFdNew, FH_FILE_START,
    				      FH_INCLUDE );
    	}

    retStat = OK;
    
ret:

    /* release device and file semaphores */
    
    semGive( pVolDesc->devSem );

    dosFsFSemGive( pFdNew );
    
    /*
     * because 'deleted' flag in file handle of the new file
     * descriptor is set, dosFsClose() will set appropriate
     * errno, we ignore the error.
     */
    errnoBuf = errnoGet();

    dosFsClose( pFdNew );

    errnoSet( errnoBuf );

    /*
     * force flush cache in order to store deleted entry descriptor
     * before rename finishes (this is important for the check disk)
     */

    if( retStat == OK )
    	dosFsIoctl( pFdOld, FIOFLUSH, (-1) );
        
    return (retStat);
    } /* dosFsRename() */

/*******************************************************************************
*
* dosFsContigAlloc - allocate contiguous space for file
*
* This routine attempts to allocate a contiguous area on the disk for
* a file.  The 
* available area which is large enough is allocated.
*
* RETURNS: OK, or ERROR if no contiguous area large enough.
*
* ERRNO:
* S_dosFsLib_INVALID_PARAMETER
* S_dosFsLib_INVALID_NUMBER_OF_BYTES
*/
LOCAL STATUS dosFsContigAlloc
    (
    DOS_FILE_DESC_ID	pFd,	/* pointer to file descriptor */
    fsize_t *           pNBytes	/* requested size of contiguous area */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = pFd->pVolDesc;
    DOS_FILE_DESC_ID	pLoopFd = pFd->pVolDesc->pFdList;
    UINT32	startClust = pFd->pFileHdl->startClust;
    				/* backup current start cluster */
    UINT32	startClustNew;	/* backup file new start cluster */
    u_int	nSec;		/* clusters to alloc */
    int	i;

    /* Check for zero bytes requested */
    if (pNBytes == NULL)
    	{
    	errnoSet (S_dosFsLib_INVALID_PARAMETER);
    	return ERROR;
    	}

    if (*pNBytes == 0)
    	{
    	errnoSet (S_dosFsLib_INVALID_NUMBER_OF_BYTES);
    	return ERROR;
    	}
    
    /* only regular file can be contiguous preallocated */

    if( pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY )
    	{
    	errnoSet( S_dosFsLib_NOT_FILE );
    	return ERROR;
    	}
    
    /* file must be writable */
    if( pFd->openMode == O_RDONLY )
    	{
    	errnoSet (S_dosFsLib_READ_ONLY);
    	return ERROR;
    	}


    /* 
     * Find a suitable area on the disk 
     */

    /* First evaluate space in sectors */
    if (*pNBytes == (fsize_t)CONTIG_MAX) /* if max contig area req'd */
    	{
   	nSec = CONTIG_MAX;
   	}
    else	
    	{
    	nSec = ( *pNBytes + pVolDesc->bytesPerSec - 1 ) >>
             	 pVolDesc->secSizeShift;
    	}

    /* Try to allocate the required sectors */
    if( pVolDesc->pFatDesc->contigAlloc( pFd, nSec ) == ERROR )
      {
   	return ERROR;
      }

    /* SPR#71105: Got enough sectors so adjust the file size to new value */

    /* 
     * So now we have allocated enough sectors for the 
     * requested size, but was it a specific size or CONTIG_MAX?
     * If the latter then we have to work out how much space
     * we actually allocated!
     */
    if (*pNBytes == (fsize_t)CONTIG_MAX) /* if max contig area req'd */
    	{
	/* We got CONTIG_MAX, how much was that?? */

	/* How many clusters? */
	int clusters = pFd->pFileHdl->contigEndPlus1 
                       - pFd->pFileHdl->startClust;
	/* size = (clusters*sectors/cluster)*sector_size */
	pFd->pFileHdl->size = (
                                ( clusters  * pVolDesc->secPerClust ) 
                                << (pVolDesc->secSizeShift)
                              );
   	}
    else 
      /* OK, we got what we asked for so use that as the size */
    	{
	pFd->pFileHdl->size = *pNBytes;
    	}


    /* Update the endtry to reflect the changes */
    if( pVolDesc->pDirDesc->updateEntry( pFd, 0, 0 ) == ERROR )
      {
    	return ERROR;
      }

    /*
     * when the file is recreated, the old file chain
     * is preserved until last file descriptor is
     * closed for the file.
     * In this case all file descriptors must be synchronized
     * and old chain be deallocated.
     */

    if( startClust != 0 )
    	{
    	startClustNew = pFd->pFileHdl->startClust;
    	
    	/* deallocate old chain */
    	
    	pFd->pFileHdl->startClust = startClust;

    	pVolDesc->pFatDesc->truncate( pFd, FH_FILE_START, FH_INCLUDE );

    	pFd->pFileHdl->startClust = startClustNew;
    	
    	/*
    	 *  synchronize file descriptors: cause next access to
         * begin with absolute seek
    	 */
    	for( pLoopFd = pFd->pVolDesc->pFdList, i = 1;
             i < pFd->pFileHdl->nRef; pLoopFd ++ )
            {
            assert( pLoopFd - pFd->pVolDesc->pFdList < 
                    pVolDesc->maxFiles );

            if( pLoopFd->busy && pLoopFd != pFd &&
            	pLoopFd->pFileHdl == pFd->pFileHdl )
            	{
    	    	i ++;
            	pLoopFd->seekOutPos = pLoopFd->pos;
    	    	pLoopFd->pos = 0;
    	    	pLoopFd->nSec = 0;
    	    	pLoopFd->curSec = 0;
            	}
            }
        }

    /* seek to file start */
    if( pVolDesc->pFatDesc->seek( pFd, FH_FILE_START, 0 ) == ERROR )
      {
    	return ERROR;
      }

    return OK;
    } /* dosFsContigAlloc() */
            
/*******************************************************************************
*
* dosFsMakeDir - create a directory.
*
* This routine creates directory.
*
* RETURNS: OK or ERROR if such directory can not be created.
*/
LOCAL STATUS dosFsMakeDir
    (
    DOS_VOLUME_DESC_ID	pVolDesc, /* pointer to volume descriptor */
    u_char *	name	/* directory name */
    )
    {
    DOS_FILE_DESC_ID	pFd;
    u_char      path [PATH_MAX + 1];

    /* get full path to the new file (use pFd as temp buffer) */

    if (ioFullFileNameGet (name, (DEV_HDR **) &pFd, path) != OK)
        return (ERROR);

    /* Old and new devices must be the same */

    if ( (void *)pFd != (void *)pVolDesc )
        {
    	errnoSet( S_dosFsLib_NOT_SAME_VOLUME );
    	return (ERROR);
    	}

    /* create directory */

    pFd = dosFsOpen( pVolDesc, (char *)path, O_CREAT, FSTAT_DIR );

    if( pFd != (void *) ERROR )
    	{
        dosFsClose( pFd );
    	return OK;
        }

    return ERROR;
    } /* dosFsMakeDir() */

/*******************************************************************************
*
* dosFsIoctl - do device specific control function
*
* This routine performs the following ioctl functions.
*
* Any ioctl function codes, that are not supported by this routine
* are passed to the underlying CBIO module for handling.
*
* There are some ioctl() functions, that suppose to receive as
* result a 32-bit numeric value (FIONFREE, FIOWHERE and so on),
* however disks and files with size grater, than 4GB are supported.
* In order to solve this contradiction new ioctl() functions are
* provided. They have the same name as basic functions, but with
* suffix '64': FIONFREE64, FIOWHERE64 and so on. These functions
* gets pointer to 'long long' as an argument. Also FIOWHERE64
* returns value via argument, but not as ioctl()returned value.
* If an ioctl fails, the task's status (see errnoGet()) indicates
* the nature of the error.
*
* RETURNS: OK or current position in file for FIOWHERE,
* or ERROR if function failed or driver returned error, or if
* function supposes 32 bit result value, but
* actual result overloads this restriction.
*
* ERRNO:
* S_dosFsLib_INVALID_PARAMETER
* S_dosFsLib_VOLUME_NOT_AVAILABLE
* S_dosFsLib_FD_OBSOLETE
* S_dosFsLib_DELETED
* S_dosFsLib_32BIT_OVERFLOW
*/
 
LOCAL STATUS dosFsIoctl
    (
    FAST DOS_FILE_DESC_ID	pFd,	/* fd of file to control */       
    int                 function,       /* function code */
    int                 arg             /* some argument */
    )  
    {
    DOS_VOLUME_DESC_ID	pVolDesc;
    void *	pBuf;		/* work ptr */
    fsize_t	buf64 = 0;	/* 64-bit work buffer */
    BOOL	devSemOwned = FALSE, cbioOwned = FALSE;
    STATUS	retVal = ERROR;
    
    if( (pFd == NULL || pFd == (void *)ERROR) ||
        pFd->pVolDesc->magic != DOS_FS_MAGIC )
    	{
    	errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	return ERROR;
    	}

    assert( pFd - pFd->pVolDesc->pFdList < pFd->pVolDesc->maxFiles );
    assert( pFd->pFileHdl - pFd->pVolDesc->pFhdlList <
            pFd->pVolDesc->maxFiles );

    pVolDesc = pFd->pVolDesc;

    /*
     * FIODISKCHANGE might be called from interrupt, -
     * do not take semaphore
     */
    if( function == FIODISKCHANGE )
   	{
        return (cbioRdyChgdSet (pVolDesc->pCbio, TRUE));
        }
    
    /*
     * following ioctl codes cause volume data modification,
     * Do not rely on underlying functions to check if volume is 
     * write protected, simply check here and 'nip it in the bud'.
     */
    switch (function)
    	{
    	case FIOCONTIG:
    	case FIOCONTIG64:
    	case FIOFLUSH:
    	case FIOSYNC:
    	case FIOMKDIR:
    	case FIORMDIR:
        case FIORENAME:
        case FIOMOVE:
        case FIOTRUNC:
        case FIOTRUNC64:
    	case FIODISKINIT:
        case FIOLABELSET:
        case FIOATTRIBSET:
        case FIOSQUEEZE:
    	case FIOTIMESET:
    	    if( cbioModeGet (pVolDesc->pCbio) == O_RDONLY )
            	{
            	errnoSet( S_ioLib_WRITE_PROTECTED );
            	return (ERROR);
            	}
	}

    /* take control on file */
    
    if( dosFsFSemTake( pFd, WAIT_FOREVER ) == ERROR )
    	return ERROR;
    
    /* check file status */
    
    if( ! pFd->busy )
    	{
    	assert( pFd->busy );
    	
    	errnoSet (S_dosFsLib_INVALID_PARAMETER);
    	goto ioctlRet;
    	}

    if( (FALSE == pVolDesc->mounted) ||
        (TRUE == cbioRdyChgdGet (pVolDesc->pCbio)))
    	{
    	errnoSet( S_dosFsLib_FD_OBSOLETE );
    	goto ioctlRet;
    	}

    if( pFd->pFileHdl->obsolet )
    	{
    	errnoSet (S_dosFsLib_FD_OBSOLETE);
    	goto ioctlRet;
    	}

    if( pFd->pFileHdl->deleted )
    	{
    	errnoSet (S_dosFsLib_DELETED);
    	goto ioctlRet;
    	}

    /* Perform requested function */
    
    DBG_MSG( 600, "Function %u\n", function,0,0,0,0,0,0,0 );
    switch (function)
    	{
    	case FIODISKINIT:
    	    {
    	    if( dosFsVolFormatRtn == NULL )
    	    	{
    	    	errnoSet( S_dosFsLib_UNSUPPORTED );
    	    	retVal = ERROR;
    	    	}
    	    else
    	    	{
    	        retVal = dosFsVolFormatRtn( pVolDesc->pCbio,
    	                                    arg, NULL );
    	    	}
    	    goto ioctlRet;
    	    } /* FIODISKINIT */
    	
        case FIORENAME:
            {
            retVal = dosFsRename( pFd, (char *)arg, FALSE );
            goto ioctlRet;
            } /* FIORENAME */
 
        case FIOMOVE:
            {
            retVal = dosFsRename( pFd, (char *)arg, TRUE );
            goto ioctlRet;
            } /* FIORENAME */
 
        case FIOSEEK:
            {
	    buf64 = (fsize_t)arg;
    	    retVal = ERROR;

    	    if( pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY )
    	    	{ /* seek for directories is not supported */
    	    	errnoSet( S_dosFsLib_NOT_FILE );
    	    	}
            else if( arg < 0 )
    	    	{ /* traditional seek is restricted with positive int */
    	    	errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	    	}
            else
    	    	{
            	retVal = dosFsSeek (pFd, buf64 );
    	    	}
            goto ioctlRet;
            } /* FIOSEEK */
 
        case FIOSEEK64:	/* seek within 64-bit position */
            {
    	    retVal = ERROR;

    	    if( pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY )
    	    	{ /* seek for directories is not supported */
    	    	errnoSet( S_dosFsLib_NOT_FILE );
    	    	}
            else if( arg == (int) NULL )
    	    	{
    	    	errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	    	}
    	    else
    	    	{
            	retVal = dosFsSeek (pFd, *(fsize_t *)arg);
    	    	}

            goto ioctlRet;
            } /* FIOSEEK64 */
 
        case FIOWHERE:	/* position within 32 bit boundary */
            {
    	    buf64 = ( pFd->seekOutPos != 0 )? pFd->seekOutPos :
					      pFd->pos;
	    if( dosFsIsValHuge( buf64 ) )
    	        {
    	    	errnoSet( S_dosFsLib_32BIT_OVERFLOW );
   	        retVal = ERROR;
    	        }
    	    else
                retVal = buf64; 

            goto ioctlRet;
            } /* FIOWHERE */
 
        case FIOWHERE64:	/* position within 64 bit boundary */
            {
            retVal = ERROR;
            if( (void *)arg == NULL )
            	{
            	errnoSet( S_dosFsLib_INVALID_PARAMETER );
            	}
            else
            	{
            	*(fsize_t *)arg = ( pFd->seekOutPos != 0 )?
					pFd->seekOutPos : pFd->pos; 
            	retVal = OK;
            	}
            goto ioctlRet;
            } /* FIOWHERE64 */
        
        case FIOSYNC:
            {
            /* 
             * synchronize FAT copies which only happens if FAT 
             * syncing is enabled in the fat handler.
             */

            retVal = pVolDesc->pFatDesc->flush( pFd );

            /* flush and invalidate I/O cache */

            retVal = cbioIoctl(pVolDesc->pCbio, CBIO_CACHE_FLUSH,(void *)(-1));

            retVal = cbioIoctl(pVolDesc->pCbio, CBIO_CACHE_INVAL, 0 );

            goto ioctlRet;
            } /* FIOSYNC */
 
        case FIOFLUSH:
            {
            retVal = OK;
            
            /* store directory entry */
            
            if( pFd->accessed || pFd->changed )
                {
    	    	pBuf = (void *)((pFd->accessed) ? DH_TIME_ACCESS :
		                        (DH_TIME_ACCESS | DH_TIME_MODIFY)) ;

    	    	retVal = pVolDesc->pDirDesc->updateEntry(
    	    	             	       pFd, (u_int)pBuf, 0 );
    	    	}

            if( retVal == OK )	
                {
    	    	retVal = cbioIoctl(pVolDesc->pCbio,
                                       CBIO_CACHE_FLUSH, (void *)(-1) );
    	    	}

            goto ioctlRet;
            } /* FIOFLUSH */
           
        case FIONREAD:
            {
            buf64 = ( pFd->seekOutPos != 0 ) ? pFd->seekOutPos : pFd->pos; 

    	    buf64 = ( buf64 < pFd->pFileHdl->size ) ?
				 pFd->pFileHdl->size - buf64 : 0;
    	    
            retVal = ERROR;

            if( (void *)arg == NULL )
            	{
            	errnoSet( S_dosFsLib_INVALID_PARAMETER );
            	}
    	    else if( dosFsIsValHuge( buf64 ) )
    	    	{
    	    	errnoSet( S_dosFsLib_32BIT_OVERFLOW );
    	    	*(u_int *)arg = (-1);
    	    	}
            else
            	{
            	*(u_long *)arg = buf64;
            	retVal = OK;
            	}

            goto ioctlRet;
            } /* FIONREAD */

        case FIONREAD64:
            {
            retVal = ERROR;

            if( (void *)arg == NULL )
            	{
            	errnoSet( S_dosFsLib_INVALID_PARAMETER );
            	}
            else
            	{
            	buf64 = ( pFd->seekOutPos != 0 )? pFd->seekOutPos :
                                              	  pFd->pos; 
            	*(fsize_t *)arg = ( buf64 < pFd->pFileHdl->size )?
                                 	pFd->pFileHdl->size - buf64 : 0;
            	retVal = OK;
            	}

            goto ioctlRet;
            } /* FIONREAD64 */

        case FIOUNMOUNT:	/* unmount the volume */
            {
            retVal = dosFsVolUnmount( pVolDesc );
            goto ioctlRet;
            } /* FIOUNMOUNT */

        case FIONFREE:
            {
            buf64 = pVolDesc->pFatDesc->nFree( pFd );
    	    
            retVal = ERROR;

            if( (void *)arg == NULL )
            	{
            	errnoSet( S_dosFsLib_INVALID_PARAMETER );
            	}
    	    else if( dosFsIsValHuge( buf64 ) )
    	    	{
    	    	*(u_int *)arg = (-1);
    	    	errnoSet( S_dosFsLib_32BIT_OVERFLOW );
    	    	}
    	    else
            	{
            	*(u_int *)arg = (u_int)buf64;
            	retVal = OK;
            	}
            goto ioctlRet;
            } /* FIONFREE */
        
        case FIONFREE64:
            {
            if( (void *)arg == NULL )
            	{
            	errnoSet( S_dosFsLib_INVALID_PARAMETER );
            	retVal = ERROR;
            	goto ioctlRet;
            	}

            *(fsize_t *)arg = pVolDesc->pFatDesc->nFree( pFd );

            retVal = OK;

            goto ioctlRet;
            } /* FIONFREE64 */
        
        case FIOMKDIR:	/* creat new directory */
            {
    	    retVal = dosFsMakeDir( pFd->pVolDesc, (u_char *)arg );
            goto ioctlRet;
            } /* FIOMKDIR */

        case FIORMDIR:	/* remove directory */
            {
            if( pVolDesc !=
                dosFsVolDescGet( (void *) arg, (void *)&pBuf ) &&
                pBuf != (void *) arg )
	    	{
    	    	errnoSet( S_dosFsLib_NOT_SAME_VOLUME );
    	    	retVal = ERROR;
    	    	}
    	    else
    	    	{
            	retVal = dosFsDelete( pVolDesc, pBuf );
    	    	}
            
            goto ioctlRet;
            } /* FIORMDIR */

        case FIOLABELGET:
            {
            retVal = pVolDesc->pDirDesc->volLabel(
            		pVolDesc, (u_char *)arg, FIOLABELGET );
            goto ioctlRet;
            } /* FIOLABELGET */

        case FIOLABELSET:
            {
            retVal = ERROR;
            if( dosFsFSemTake( pFd, WAIT_FOREVER ) == ERROR )
            	goto ioctlRet;
            retVal = pVolDesc->pDirDesc->volLabel(
            		pVolDesc, (u_char *)arg, FIOLABELSET );
            dosFsFSemGive( pFd );
            goto ioctlRet;
            } /* FIOLABELSET */

        case FIOATTRIBSET:
            {
            /* 
             * ensure sane values before we shove it into the directory 
             * We never modify 'entry type' bits (directory or label) so
             * DOS_ATTR_DIRECTORY and DOS_ATTR_VOL_LABEL are masked.
             */

            arg &= ( DOS_ATTR_RDONLY | DOS_ATTR_HIDDEN | 
                     DOS_ATTR_SYSTEM | DOS_ATTR_ARCHIVE );

	    /* preserve entry type bits if they existed already */

            arg |= ((pFd->pFileHdl->attrib) & 
                    (DOS_ATTR_DIRECTORY | DOS_ATTR_VOL_LABEL)); 

            /* ready to set the new attribute */
            
            pFd->pFileHdl->attrib = arg;

            pVolDesc->pDirDesc->updateEntry(pFd, 
			                    DH_TIME_ACCESS | DH_TIME_MODIFY, 
					    0 );
            retVal = OK;

            goto ioctlRet;
            } /* FIOATTRIBSET */

        case FIOCONTIG:
            {
            buf64 = (fsize_t)arg;
            retVal = dosFsContigAlloc( pFd, &buf64 );
            goto ioctlRet;
            } /* FIOCONTIG */
        
        case FIOCONTIG64:
            {
            retVal = dosFsContigAlloc( pFd, (fsize_t *)arg );
            goto ioctlRet;
            } /* FIOCONTIG64 */

        case FIONCONTIG:
            {
            buf64 = pVolDesc->pFatDesc->maxContig( pFd ) <<
                                pVolDesc->secSizeShift;
    	    
            retVal = ERROR;

    	    if( dosFsIsValHuge( buf64 ) )
    	    	{
    	    	errnoSet( S_dosFsLib_32BIT_OVERFLOW );
    	    	}
            else if( (void *)arg == NULL )
            	{
            	errnoSet( S_dosFsLib_INVALID_PARAMETER );
            	}
    	    else
            	{
            	*(u_int *)arg = (u_int)buf64;
            	retVal = OK;
            	}
            goto ioctlRet;
            } /* FIONCONTIG */
        
        case FIONCONTIG64:
            {
            if( (void *)arg == NULL )
            	{
            	errnoSet( S_dosFsLib_INVALID_PARAMETER );
            	retVal = ERROR;
            	goto ioctlRet;
            	}
            /* 
             * SPR#30464 added (fsize_t) cast to pVolDesc->...  to ensure
             * use of 64bit math.
             * precedence is the (pFd) followed by the -> followed by the 
             * cast followed by the shift 
             */

            *(fsize_t *)arg = (fsize_t) pVolDesc->pFatDesc->maxContig( pFd ) <<
   		   	      pVolDesc->secSizeShift;

   	    retVal = OK;
            goto ioctlRet;
            } /* FIONCONTIG64 */
        
        case FIOREADDIR:
            {
            retVal = ERROR;
            if( (void *)arg == NULL )
            	{
            	errnoSet( S_dosFsLib_INVALID_PARAMETER );
            	}
    	    else if( (pFd->pFileHdl->attrib & DOS_ATTR_DIRECTORY) == 0 )
    	    	{
    	    	errnoSet( S_dosFsLib_NOT_DIRECTORY );
    	    	}
    	    else if( ((DIR *) arg)->dd_cookie == ERROR )
	    	{
    	    	/* any error already occurred while recent readdir */
    	    	}
    	    /*
    	     * may be application did seek by
	     * direct setting dd_cookie field
    	     */
            else if( (pFd->pos  == DD_COOKIE_TO_POS( (DIR *) arg )) ||
    	    	     dosFsSeekDir( pFd, (DIR *) arg ) == OK )
            	{
            	retVal = pVolDesc->pDirDesc->readDir(
            				pFd, (DIR *) arg, NULL);
            	}
            goto ioctlRet;
            } /* FIOREADDIR */

        case FIOFSTATGET:
            {
            retVal = dosFsStatGet (pFd, (struct stat *) arg);
            goto ioctlRet;
            } /* FIOFSTATGET */

	case FIOFSTATFSGET:
            {
	    retVal = dosFsFSStatGet (pFd, (struct statfs *) arg);
	    goto ioctlRet;
            } /* FIOFSTATFSGET */

        case FIOTRUNC:
            {
            retVal = dosFsTrunc (pFd, arg);
            goto ioctlRet;
            } /* FIOTRUNC */

        case FIOTRUNC64:
            {
            retVal = ERROR;
            if( (void *)arg == NULL )
            	{
            	errnoSet( S_dosFsLib_INVALID_PARAMETER );
            	}
    	    else
    	    	{
            	retVal = dosFsTrunc (pFd, *((fsize_t *)arg));
    	    	}
            goto ioctlRet;
            } /* FIOTRUNC64 */
        case FIOSQUEEZE:
            {	/* TBD: we can think about directory squeezing */
            retVal = OK;
            goto ioctlRet;
            } /* FIOSQUEEZE */

    	case FIOCHKDSK:
    	    {
    	    retVal = ERROR;

    	    if( dosFsChkRtn == NULL )
                {
                errnoSet( S_dosFsLib_UNSUPPORTED );
                ERR_MSG(1,"Check disk utility not installed\n", 0,0,0,0,0,0 );
                goto ioctlRet;
                }

    	    if( ! IS_ROOT( pFd ) )
    	    	{
    	    	errnoSet( S_dosFsLib_INVALID_PARAMETER );
    	    	goto ioctlRet;
    	    	}
    	    
    	    /* take ownership on device */
    	    
    	    if( dosFsFSemTake( pFd, WAIT_FOREVER ) == ERROR )
    	    	goto ioctlRet;

    	    devSemOwned = TRUE;
    	    
    	    if(ERROR == cbioLock (pVolDesc->pCbio, WAIT_FOREVER ))
    	    	{
    	    	goto FIOCHKDSK_ret;
    	    	}

    	    cbioOwned = TRUE;
    	    
    	    /*
    	     * it may be device had been unmounted before
    	     * devSem was taken
    	     */
    	    if( pFd->pFileHdl->obsolet )
    	    	{
    	    	errnoSet( S_dosFsLib_FD_OBSOLETE );
    	    	goto FIOCHKDSK_ret;
    	    	}
    	    
    	    /*
    	     * now force device remounting, but prevent
    	     * recursive check disk call
    	     */

    	    pVolDesc->chkLevel = 1;

    	    dosFsVolUnmount( pVolDesc );

    	    retVal = dosFsVolMount( pVolDesc );

    	    pVolDesc->chkLevel = 0;
    	    
    	    if( retVal == ERROR )
    	    	goto FIOCHKDSK_ret;
    	    
    	    pFd->pFileHdl->obsolet = 0;
    	    	
    	    /* run disk check */
    	    
    	    retVal = dosFsChkDsk( pFd, arg );

FIOCHKDSK_ret:            

    	    if( cbioOwned )
    	        cbioUnlock (pVolDesc->pCbio);

    	    if( devSemOwned )
    	        dosFsFSemGive( pFd );
    	    
    	    goto ioctlRet;
    	    } /* FIOCHKDSK */

        case FIOTIMESET:        /* later, surely */
    	    {
    	    retVal = ERROR;

    	    if( (void *)arg == NULL )
    	    	{
                /* 
                 * This will update file to current date 
                 * and time in either dosDirOldLib.c or 
                 * dosVDirLib.c if the FIOTIMESET argument is NULL.
                 * This make utime() behave as it does on Solaris 2.6
                 * SPR#28924.
                 */
 	    	retVal = pVolDesc->pDirDesc->updateEntry(
				pFd, DH_TIME_MODIFY | DH_TIME_ACCESS, 0);
    	        goto ioctlRet;
    	    	}

	    /* avoid setting any time on zero value */

	    if (0 != ((struct utimbuf *) arg)->modtime)
		{
    	        retVal = pVolDesc->pDirDesc->updateEntry(
				pFd, DH_TIME_MODIFY,
                                ((struct utimbuf *) arg)->modtime);
		}

	    /* avoid setting any time on zero value */

            if (0 != ((struct utimbuf *) arg)->actime)
		{
                retVal = pVolDesc->pDirDesc->updateEntry(
				pFd, DH_TIME_ACCESS, 
				((struct utimbuf *) arg)->actime);
		}
    	    goto ioctlRet;
    	    }
    	    
        case FIOINODETONAME:	/* probably never again */
        case FIOGETFL:		/* later, also need flock */
		/* not supported */
        default:
            {
            /* Call device driver function */
            retVal = cbioIoctl( pVolDesc->pCbio, function, (void *)arg );
            goto ioctlRet;
            }
        } /* switch */
    
ioctlRet:
    dosFsFSemGive( pFd );
    return (retVal);
    } /* dosFsIoctl() */

/*******************************************************************************
*
* dosFsLastAccessDateEnable - enable last access date updating for this volume
*
* This function enables or disables updating of the last access date directory
* entry field on open-read-close operations for the given dosFs volume.  The 
* last access date file indicates the last date that a file has been read or 
* written.  When the optional last access date field update is enabled, read 
* operations on a file will cause a write to the media.   
* 
* RETURNS: OK or ERROR if the volume is invalid or enable is not TRUE or FALSE.
*
*/
STATUS dosFsLastAccessDateEnable
    (
    DOS_VOLUME_DESC_ID dosVolDescId, /* dosfs volume ID to alter */
    BOOL enable      /* TRUE = enable update, FALSE = disable update */
    )
    {
    /* ensure this is a valid DOS_VOLUME_DESC_ID */

    if (TRUE == _WRS_ALIGN_CHECK(dosVolDescId, DOS_VOLUME_DESC))
        {
        if (( DOS_FS_MAGIC != dosVolDescId->magic )  || 
            ((TRUE != enable) && (FALSE != enable)))
            {
            return (ERROR);
            }
        }
    else 
        {
        /* alignment failed */
        return (ERROR);
        }

    /* set the last access field update boolean */

    dosVolDescId->updateLastAccessDate = enable;

    return (OK);
    } /* dosFsEnableLastAccessDate */

/*******************************************************************************
*
* dosFsLibInit - prepare to use the dosFs library
*
* This routine initializes the dosFs library.
* This routine installs dosFsLib as a 
* driver in the I/O system driver table, and allocates and sets up
* the necessary structures.
* The driver number assigned to dosFsLib is placed
* in the global variable <dosFsDrvNum>.
*
* RETURNS: OK or ERROR, if driver can not be installed.
*
*/
STATUS dosFsLibInit
    (
    int	ignored
    )
    {
    if( dosFsDrvNum != ERROR )
    	return OK;
    
    dosFsDrvNum = iosDrvInstall (
    				  (FUNCPTR) dosFsCreate,
    				  (FUNCPTR) dosFsDelete,
			          (FUNCPTR) dosFsOpen,
			          (FUNCPTR) dosFsClose,
			          (FUNCPTR) dosFsRead,
			          (FUNCPTR) dosFsWrite,
			          (FUNCPTR) dosFsIoctl
			        );

    return (dosFsDrvNum == ERROR ? ERROR : OK); /* SPR#65271,65009 */
    } /* dosFsLibInit() */

/*******************************************************************************
*
* dosFsDevCreate - create file system device.
*
* This routine associates a CBIO device with a logical I/O device name
* and prepare it to perform file system functions.
* It takes a CBIO_DEV_ID device handle, typically created by
* dcacheDevCreate(), and defines it as a dosFs volume.  As a result, when
* high-level I/O operations (e.g., open(), write()) are performed on
* the device, the calls will be routed through dosFsLib.  The <pCbio>
* parameter is the handle of the underlying cache or block
* device.
*
* The argument <maxFiles> specifies the number of files
* that can be opened at once on the device.
*
* The volume structure integrity can be automatically checked
* during volume mounting. Parameter <autoChkLevel>
* defines checking level (DOS_CHK_ONLY or DOS_CHK_REPAIR),
* that can be bitwise or-ed with check verbosity level value
* (DOS_CHK_VERB_SILENT, DOS_CHK_VERB_1 or DOS_CHK_VERB_2).
* If value of <autoChkLevel> is 0, this  means default level, that is
* DOS_CHK_REPAIR | DOS_CHK_VERB_1. To prevent check disk
* autocall set <autoChkLevel> to NONE.
*
* Note that actual disk accesses are deferred to the time
* when open() or creat() are first called. That is also when
* the automatic disk checking will take place.
* Therefore this function will succeed in cases where a removable
* disk is not present in the drive.
*
* RETURNS: OK, or ERROR if the device name is already in use or 
* insufficient memory.
*/

STATUS dosFsDevCreate
    (
    char *	pDevName,	/* device name */
    CBIO_DEV_ID	cbio,		/* CBIO or cast blkIo device */
    u_int	maxFiles,	/* max no. of simultaneously open files */
    u_int	autoChkLevel	/* automate volume integrity */
    				/* check level via mounting */
    				/* 0 - default: */
    				/* DOS_CHK_REPAIR | DOS_CHK_VERB_1 */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = NULL; /* volume descriptor ptr */
    SEM_ID tmpSem = NULL;
    CBIO_DEV_ID	subDev = cbio;
    
    /* install dos file system as a driver */
    
    if( dosFsDrvNum == ERROR )
    	if( dosFsLibInit( 0 ) == ERROR )
    	    return ERROR;
    
    /*
     * validate device driver and create CBIO device
     * on top of any other block device driver
     */

    if (ERROR == cbioDevVerify( cbio ))
	{
        subDev = cbioWrapBlkDev((BLK_DEV *)cbio);

        if( NULL != subDev )                                      
            {
            /* SPR#71633, clear the errno set in cbioDevVerify() */
            errno = 0;
            }
	}
    else
	{
	subDev = cbio;
	}


    if( subDev == NULL )
    	{
    	ERR_MSG( 1, "NULL CBIO_DEV_ID or BLK_DEV ptr\n", 0,0,0,0,0,0 );
    	return ERROR;
    	}

    /* allocate device descriptor */
    
    pVolDesc = (DOS_VOLUME_DESC_ID) KHEAP_ALLOC((sizeof( *pVolDesc )));

    if( pVolDesc == NULL )
    	return ERROR;

    bzero ((char *) pVolDesc, sizeof( *pVolDesc ) );
    
    /* disk access path */
    
    pVolDesc->pCbio = subDev;
    
    /* init semaphores */
    
    pVolDesc->devSem = semMCreate ( SEM_Q_PRIORITY | SEM_DELETE_SAFE );

    if (NULL == pVolDesc->devSem)
	{
	KHEAP_FREE ((char *)pVolDesc);
	return (ERROR);
	}

    pVolDesc->shortSem = semMCreate ( SEM_Q_PRIORITY | SEM_DELETE_SAFE );

    if (NULL == pVolDesc->devSem)
	{
	semDelete (pVolDesc->devSem);
	KHEAP_FREE ((char *)pVolDesc);
	return (ERROR);
	}
    
    /* init file descriptors and handles list */
    
    maxFiles = ( maxFiles == 0 )? DOS_NFILES_DEFAULT : maxFiles;
    maxFiles += 2;

    pVolDesc->pFdList = 
		KHEAP_ALLOC((maxFiles * (sizeof(*pVolDesc->pFdList))));

    if (NULL != pVolDesc->pFdList)
	bzero ((char *)pVolDesc->pFdList, 
			(maxFiles * (sizeof(*pVolDesc->pFdList))));

    pVolDesc->pFhdlList = 
		KHEAP_ALLOC((maxFiles * (sizeof(*pVolDesc->pFhdlList))));

    if (NULL != pVolDesc->pFhdlList)
	bzero ((char *)pVolDesc->pFhdlList,
			(maxFiles * (sizeof(*pVolDesc->pFhdlList))));

    pVolDesc->pFsemList = 
		KHEAP_ALLOC((maxFiles * (sizeof(SEM_ID))));

    if (NULL != pVolDesc->pFsemList)
	bzero ((char *)pVolDesc->pFsemList, (maxFiles * (sizeof(SEM_ID))));

    if( pVolDesc->pFdList == NULL || pVolDesc->pFhdlList == NULL ||
    	pVolDesc->pFsemList == NULL )
    	{
    	return ERROR;
    	}

    pVolDesc->maxFiles = maxFiles;

    /* init file semaphores */
    
    for( maxFiles = 0; maxFiles < pVolDesc->maxFiles; maxFiles++ )
    	{
	tmpSem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE);

	if (NULL == tmpSem)
    	    {
    	    return ERROR;
    	    }

	*(SEM_ID *) (pVolDesc->pFsemList + maxFiles) = tmpSem;

	tmpSem = NULL;
    	}
    
    /* device descriptor have been initiated. Install the device */
    
    pVolDesc->magic = DOS_FS_MAGIC;

    pVolDesc->mounted = FALSE;

    pVolDesc->updateLastAccessDate = FALSE; /* SPR#68203 */

    /* Initially directory paths are not case sensitive */
    pVolDesc->volIsCaseSens = FALSE;
    
    /* check disk autocall level */
    
    if( autoChkLevel != (u_int)NONE )
    	{
    	pVolDesc->autoChk = ( (autoChkLevel & 0xff) == DOS_CHK_REPAIR ) ?
    				DOS_CHK_REPAIR : DOS_CHK_ONLY;
    	
	/* extract the auto chk verbosity, verify and assign */
	switch (autoChkLevel & 0xff00)
	   {
	   case DOS_CHK_VERB_0:
	   case DOS_CHK_VERB_1:
	   case DOS_CHK_VERB_2:
	      pVolDesc->autoChkVerb = (autoChkLevel >> 8);
	      break;
	   default:
	      /* Force out of range verbosity to maximum level */
	      pVolDesc->autoChkVerb = DOS_CHK_VERB_2 >> 8 ;
	      break;
	   } /* switch */
    	}

    if( iosDevAdd( (void *)pVolDesc, pDevName, dosFsDrvNum ) == ERROR )
    	{
    	pVolDesc->magic = NONE;
    	return (ERROR);
    	}

    return (OK);
    } /* dosFsDevCreate() */

/*******************************************************************************
*
* dosFsShow - display dosFs volume configuration data.
*
* This routine obtains the dosFs volume configuration for the named
* device, formats the data, and displays it on the standard output.
*
* If no device name is specified, the current default device
* is described.
*
* RETURNS: OK or ERROR, if no valid device specified.
*
*/

STATUS dosFsShow
    (
    void *	pDevName,	/* name of device */
    u_int	level		/* detail level */
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc;	/* dos volume descriptor ptr */
    DOS_FILE_DESC_ID	pFd;	/* loop pointer */
    DOS_FILE_HDL_ID	pFileHdl; /* --"--"-- */
    int	nFdInUse, nFlOpen, nDeleted, nObsolet;	/* counters */
    char	volLab[ DOS_VOL_LABEL_LEN + 1 ] = { EOS };

    pVolDesc = dosFsVolDescGet( pDevName, NULL );

    if( pVolDesc == NULL )
    	{
    	printf(" Not A DOSFS Device\n");
    	return ERROR;
    	}
    
    /* try to mount the volume, if not yet */

    pFd = dosFsOpen( pVolDesc, "/", 0, 0 );

    if( pFd != (DOS_FILE_DESC_ID)(ERROR) )
    	dosFsClose( pFd );

    /* general device description */
    
    printf( "\nvolume descriptor ptr (pVolDesc):	%p\n"
    	    "cache block I/O descriptor ptr (cbio):	%p\n",
    	    pVolDesc,
    	    pVolDesc->pCbio );
    
    printf( "auto disk check on mount:		");
    if( pVolDesc->autoChk == 0 )
    	printf( "NOT ENABLED\n" );
    else
    	{
    	printf( "%s | %s\n",
                (pVolDesc->autoChk == DOS_CHK_ONLY) ? "DOS_CHK_ONLY" :
                				      "DOS_CHK_REPAIR",
                ( ((pVolDesc->autoChkVerb << 8) == DOS_CHK_VERB_0) ?
                	"DOS_CHK_VERB_SILENT" :
                  ( ((pVolDesc->autoChkVerb << 8) == DOS_CHK_VERB_1) ?
                  	"DOS_CHK_VERB_1" : "DOS_CHK_VERB_2" ) ) );
        }
    	    
    /* currently open file descriptors data base */
    
    nFdInUse = nFlOpen = nDeleted = nObsolet = 0;
    for( pFd = pVolDesc->pFdList, pFileHdl = pVolDesc->pFhdlList;
         pFd < pVolDesc->pFdList + pVolDesc->maxFiles;
         pFd++, pFileHdl++ )
    	{
    	if( pFd->busy )
    	    nFdInUse ++;
    	if( pFileHdl->nRef > 0 )
    	    {
    	    nFlOpen ++;
    	    if( pFileHdl->obsolet )
    	    	nObsolet++;
    	    else if( pFileHdl->deleted )
    	    	nDeleted ++;
    	    }
    	}
    
    printf( "max # of simultaneously open files:	%u\n",
    	    pVolDesc->maxFiles );
    printf ("file descriptors in use:		%d\n", nFdInUse ); 
    printf ("# of different files in use:		%d\n", nFlOpen ); 
    printf ("# of descriptors for deleted files:	%d\n", nDeleted );
    printf ("# of  obsolete descriptors:		%d\n\n", nObsolet );

    /* boot sector data */
    
    if( ! pVolDesc->mounted )
    	{
    	printf( "can't mount volume\n\n" );
	goto passedBoot;
    	}
    
    printf("current volume configuration:\n" );
    
    /* volume label */
    
    pFd = dosFsFdGet( pVolDesc );
    if( pFd != NULL )
    	{
    	dosFsIoctl( pFd, FIOLABELGET, (int)volLab );
    	dosFsFdFree( pFd );
    	}
    printf(" - volume label:	%s ; (in boot sector:	%s)\n",
    	   (( *volLab == EOS )? "NO LABEL" : volLab),
    	   pVolDesc->bootVolLab );
    
    printf(" - volume Id:		%p\n",
    		(void *)pVolDesc->volId );
    
    print64Fine( " - total number of sectors:	" ,
    		pVolDesc->totalSec, "\n", 10 );
    print64Mult(  " - bytes per sector:		",
    		pVolDesc->bytesPerSec, "\n", 10 );
    print64Mult(  " - # of sectors per cluster:	",
    		pVolDesc->secPerClust, "\n", 10 );
    print64Mult(  " - # of reserved sectors:	",
    		pVolDesc->nReservedSecs, "\n", 10 );

    printf(" - FAT entry size:		%s\n",
    	   ( (pVolDesc->fatType == FAT12)? "FAT12" :
    	     ( (pVolDesc->fatType == FAT16)? "FAT16" : "FAT32" ) ) );
    
    print64Mult(  " - # of sectors per FAT copy:	",
    		pVolDesc->secPerFat, "\n", 10 );
    printf(" - # of FAT table copies:	%u\n",
    		pVolDesc->nFats );
    print64Mult(  " - # of hidden sectors:		",
    		pVolDesc->nHiddenSecs, "\n", 10 );
    print64Mult(  " - first cluster is in sector #	",
    		pVolDesc->dataStartSec, "\n", 10 );
    
    printf(" - Update last access date for open-read-close = %s\n",
    		(FALSE == pVolDesc->updateLastAccessDate) ? "FALSE" : "TRUE");

passedBoot:

    /* handlers specific data */
    
    if( pVolDesc->pDirDesc != NULL && pVolDesc->pDirDesc->show != NULL )
    	pVolDesc->pDirDesc->show( pVolDesc );
    
    printf("\n");
    	
    if( pVolDesc->pFatDesc != NULL && pVolDesc->pFatDesc->show != NULL )
    	pVolDesc->pFatDesc->show( pVolDesc );
    
    return OK;
    } /* dosFsShow() */

/*******************************************************************************
*
* dosFsHdlrInstall - install handler.
*
* This library does not directly access directory structure,
* nor FAT, rather it uses particular handlers to serve such accesses.
* This function is intended for use by the dosFsLib sub-modules only.
*
* This routine installs a handler into DOS FS handlers list.
* There are two such lists: FAT Handlers List (<dosFatHdlrsList>)
* and Directory Handlers List (<dosDirHdlrsList>).
* Each handler must provide its unique Id (see dosFsLibP.h) and
* pointer to appropriate list to install it to. All lists
* are sorted by Id-s in ascending order. Every handler is tried
* to be mounted on each new volume in accordance to their order
* in list, until succeeded. So preferable handlers, that supports
* the same type of volumes must have less Id values.
*
* RETURNS: STATUS.
*
* NOMANUAL
*
*/
STATUS dosFsHdlrInstall
    (
    DOS_HDLR_DESC_ID	hdlrsList,	/* appropriate list */
    DOS_HDLR_DESC_ID	hdlr		/* ptr on handler descriptor */
    )
    {
    int i;
    STATUS	retStat = ERROR;
    DOS_HDLR_DESC	hdlrBuf1, hdlrBuf2;
    
    if( hdlr == NULL )
    	{
    	ERR_MSG( 1, "\a NULL handler descriptor ptr \a\n", 0,0,0,0,0,0 );
    	return ERROR;
    	}
    if( hdlrsList != dosFatHdlrsList && hdlrsList != dosDirHdlrsList )
    	{
    	ERR_MSG( 1, "\a Unknown handler list \a\n", 0,0,0,0,0,0 );
    	return ERROR;
    	}
    
    hdlrBuf1 = *hdlr;
    
    for( i = 0; i < DOS_MAX_HDLRS; i++ )
        {
        if(( hdlrsList[ i ] .id == 0) ||
           ( hdlrsList[ i ] .id > hdlrBuf1.id ))
            {
            retStat = OK;
            hdlrBuf2 = hdlrsList[ i ];
            hdlrsList[ i ] = hdlrBuf1;
            hdlrBuf1 = hdlrBuf2;
            }
        }
    
    if( retStat == ERROR )
    	{
    	ERR_MSG( 1, "\a Handler not installed \a\n", 0,0,0,0,0,0 );
    	}
    return retStat;
    } /* dosFsHdlrInstall() */
    

/* following code until End Of File is unused except for unitesting */
#ifdef	__unitest__

#ifdef DEBUG

STATUS dosSecPut
    (
    void * pDev,
    u_int sec,
    u_int off,
    u_int nBytes,
    char * buf
    )
    {
    DOS_VOLUME_DESC_ID	pVolDesc = dosFsVolDescGet( dev, NULL );
    CBIO_DEV_ID	pCbio;
    CBIO_PARAMS cbioParams;

    if( pVolDesc == NULL )
    	{
    	printf("Not device => cbio \n");
    	pCbio = pDev;
    	}
    else
    	pCbio = pVolDesc->pCbio;
    
    if( buf == NULL )
    	{
    	printf("NULL buffer\n" );
    	return ERROR;
    	}

    /* Get CBIO device parameters */

    if (ERROR == cbioParamsGet (pVolDesc->pCbio, &cbioParams))
	{
	return (ERROR);
	}
    
    if( sec >= cbioParams.nBlocks )
    	{
    	printf("sec = %u >= nBlocks = %u\n",
    		sec, (u_int)cbioParams.nBlocks );
    	return ERROR;
    	}
    
    if( off + nBytes > cbioParams.bytesPerBlk )
    	{
    	printf("off + nBytes = %u > bytesPerBlk = %u\n",
    		off + nBytes, cbioParams.bytesPerBlk );
    	return ERROR;
    	}
    
    if( cbioBytesRW( pCbio, sec, off, buf, nBytes,
    		     CBIO_WRITE, NULL )== ERROR )
    	{
    	printf("Error write sector, errno = %p\n", (void *)errno );
    	return ERROR;
    	}
    
    return OK;
    } /* dosSecPut() */
    
STATUS dosSecGet
    (
    void * pDev,
    u_int sec,
    u_int off,
    u_int nBytes,
    char * buf
    )
    {
    IMPORT d();
    DOS_VOLUME_DESC_ID	pVolDesc = dosFsVolDescGet( pDev, NULL );
    CBIO_DEV *	pCbio;
    CBIO_PARAMS cbioParams;
    static void * pBuf = NULL;
    
    if( pVolDesc == NULL )
    	{
    	printf("Not device => cbio \n");
    	pCbio = pDev;
    	}
    else
    	pCbio = pVolDesc->pCbio;
    
    if( buf == NULL )
    	{
    	if( pBuf == NULL )
    	    {
    	    pBuf = KHEAP_ALIGNED_ALLOC(16, cbioParams.bytesPerBlk); 
    	    if( pBuf == NULL )
    	    	return ERROR;
    	    }
    	buf = pBuf;
    	}

    /* Get CBIO device parameters */

    if (ERROR == cbioParamsGet (pVolDesc->pCbio, &cbioParams))
	{
	return (ERROR);
	}
    
    if( sec >= cbioParams.nBlocks )
    	{
    	printf("sec = %u >= nBlocks = %u\n",
    		sec, (u_int)cbioParams.nBlocks );
    	return ERROR;
    	}
    
    if( off + nBytes > cbioParams.bytesPerBlk )
    	{
    	printf("off + nBytes = %u > bytesPerBlk = %u\n",
    		off + nBytes, cbioParams.bytesPerBlk );
    	return ERROR;
    	}
    
    if( cbioBytesRW( pCbio, sec, off, buf, nBytes,
    		     CBIO_READ, NULL )== ERROR )
    	{
    	printf("Error read sector, errno = %p\n", (void *)errno );
    	return ERROR;
    	}
    
    if( buf == pBuf )
    	d( buf, nBytes, 1 );
    
    return OK;
    } /* dosSecGet() */
    
#ifdef SIZE64 
int test64( int val )
    {
    fsize_t     val64 = val;
 
    print64Fine( "val = ", val64, "\n", 16 );
    val64 = (val64 << 32) | val;
    print64Fine( "val<<32 | val = ", val64, "\n", 16 );
    val64 >>= 32;
    print64Fine( "val >> 32= ", val64, "\n", 16 );
    return val;
    }
#endif /* SIZE64 */

#endif /* DEBUG */
#endif	/*__unitest__*/

/* End of File */


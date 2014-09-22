/* dirLib.c - directory handling library (POSIX) */

/* Copyright 1984-1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01p,02nov01,cyr  doc: fix SPR 35650 fstatfs
01o,19jun96,dgp  doc: added note to stat() (SPR #6560)
01n,18jan95,rhp  doc: explain opendir() does not work over netDrv
01m,19apr94,jmm  fixed closedir() so it doesn't attempt invalid free ()
                 added statfs(), fstatfs(), and utime()
01m,18oct94,tmk  made closedir() check close() status before freeing (SPR#2744)
01l,05mar93,jdi  doc tweak.
01k,23nov92,jdi  documentation cleanup.
01j,18jul92,smb  Changed errno.h to errnoLib.h.
01i,26may92,rrr  the tree shuffle
01h,10dec91,gae  added includes for ANSI.
01g,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY and ...
		  -changed VOID to void
		  -changed copyright notice
01f,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by kdl.
01e,11feb91,jaa	 documentation.
01d,01oct90,dnw  changed to return ENOTDIR instead of S_dirLib_NOT_DIRECTORY
		 removed references to deleted dirLib.h
01c,09aug90,kdl  mangen fixes.
01b,07may90,kdl  lint.
01a,04may90,kdl  written.
	   +llk
	   +dnw
*/

/*
DESCRIPTION
This library provides POSIX-defined routines for opening, reading, and
closing directories on a file system.  It also provides routines to obtain
more detailed information on a file or directory.

SEARCHING DIRECTORIES
Basic directory operations, including opendir(), readdir(), rewinddir(),
and closedir(), determine the names of files and subdirectories in a
directory.

A directory is opened for reading using opendir(), specifying the name of
the directory to be opened.  The opendir() call returns a pointer to a
directory descriptor, which identifies a directory stream.  The stream is
initially positioned at the first entry in the directory.

Once a directory stream is opened, readdir() is used to obtain individual
entries from it.  Each call to readdir() returns one directory entry, in
sequence from the start of the directory.  The readdir() routine returns a
pointer to a `dirent' structure, which contains the name of the file (or
subdirectory) in the `d_name' field.

The rewinddir() routine resets the directory stream to the start of the
directory.  After rewinddir() has been called, the next readdir() will cause
the current directory state to be read in, just as if a new opendir() had
occurred.  The first entry in the directory will be returned by the first
readdir().

The directory stream is closed by calling closedir().

GETTING FILE INFORMATION
The directory stream operations described above provide a mechanism to
determine the names of the entries in a directory, but they do not provide
any other information about those entries.  More detailed information is
provided by stat() and fstat().

The stat() and fstat() routines are essentially the same, except for how
the file is specified.  The stat() routine takes the name of the file as
an input parameter, while fstat() takes a file descriptor number as
returned by open() or creat().  Both routines place the information from a
directory entry in a `stat' structure whose address is passed as an input
parameter.  This structure is defined in the include file stat.h.  The
fields in the structure include the file size, modification date/time,
whether it is a directory or regular file, and various other values.

The `st_mode' field contains the file type; several macro functions are
provided to test the type easily.  These macros operate on the `st_mode'
field and evaluate to TRUE or FALSE depending on whether the file is a
specific type.  The macro names are:
.RS 4
.iP S_ISREG 15
test if the file is a regular file
.iP S_ISDIR
test if the file is a directory
.iP S_ISCHR
test if the file is a character special file
.iP S_ISBLK
test if the file is a block special file
.iP S_ISFIFO
test if the file is a FIFO special file
.RE
.LP
Only the regular file and directory types are used for VxWorks local
file systems.  However, the other file types may appear when getting
file status from a remote file system (using NFS).

As an example, the S_ISDIR macro tests whether a particular entry describes
a directory.  It is used as follows:
.CS
    char          *filename;
    struct stat   fileStat;

    stat (filename, &fileStat);

    if (S_ISDIR (fileStat.st_mode))
	printf ("%s is a directory.\en", filename);
    else
	printf ("%s is not a directory.\en", filename);
.CE

See the ls() routine in usrLib for an illustration of how to combine
the directory stream operations with the stat() routine.

INCLUDE FILES: dirent.h, stat.h
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "dirent.h"
#include "ioLib.h"
#include "memLib.h"
#include "sys/stat.h"
#include "errnoLib.h"
#include "fcntl.h"
#include "stdlib.h"
#include "unistd.h"
#include "utime.h"

/*******************************************************************************
*
* opendir - open a directory for searching (POSIX)
*
* This routine opens the directory named by <dirName> and allocates a
* directory descriptor (DIR) for it.  A pointer to the DIR structure is
* returned.  The return of a NULL pointer indicates an error.
*
* After the directory is opened, readdir() is used to extract individual
* directory entries.  Finally, closedir() is used to close the directory.
*
* WARNING: For remote file systems mounted over netDrv, opendir() fails,
* because the netDrv implementation strategy does not provide a way to 
* distinguish directories from plain files.  To permit use of opendir() 
* on remote files, use NFS rather than netDrv.
*
* RETURNS: A pointer to a directory descriptor, or NULL if there is an error.
*
* SEE ALSO:
* closedir(), readdir(), rewinddir(), ls()
*/

DIR *opendir
    (
    char        *dirName                /* name of directory to open */
    )
    {
    FAST int	fd;			/* file descriptor for open directory */
    FAST DIR	*pDir;			/* ptr to allocated dir descriptor */
    struct stat	fileStat;		/* structure for file stat */

    if ((fd = open (dirName, O_RDONLY, 0)) == ERROR)
	return (NULL);				/* can't open */

    /* Check that it really is a directory */

    if (fstat (fd, &fileStat) != OK)
	{
	(void) close (fd);
	return (NULL);				/* can't stat */
	}

    if (S_ISDIR (fileStat.st_mode) != TRUE)
	{
	errnoSet (ENOTDIR);
	(void) close (fd);
	return (NULL);				/* not a dir */
	}

    /* Allocate directory descriptor */

    if ((pDir = (DIR *) calloc (sizeof (DIR), 1)) == NULL)
	{
	(void) close (fd);
	return (NULL);				/* no memory */
	}

    pDir->dd_fd     = fd;			/* put file descriptor in DIR */
    pDir->dd_cookie = 0;			/* start at beginning of dir */

    return (pDir);
    }

/*******************************************************************************
*
* readdir - read one entry from a directory (POSIX)
*
* This routine obtains directory entry data for the next file from an
* open directory.  The <pDir> parameter is the pointer to a directory
* descriptor (DIR) which was returned by a previous opendir().
*
* This routine returns a pointer to a `dirent' structure which contains
* the name of the next file.  Empty directory entries and MS-DOS volume
* label entries are not reported.  The name of the file (or subdirectory)
* described by the directory entry is returned in the `d_name' field
* of the `dirent' structure.  The name is a single null-terminated string.
*
* The returned `dirent' pointer will be NULL, if it is at the end of the
* directory or if an error occurred.  Because there are two conditions which
* might cause NULL to be returned, the task's error number (`errno') must be
* used to determine if there was an actual error.  Before calling readdir(),
* set `errno' to OK.  If a NULL pointer is returned, check the new
* value of `errno'.  If `errno' is still OK, the end of the directory was
* reached; if not, `errno' contains the error code for an actual error which
* occurred.
*
* RETURNS: A pointer to a `dirent' structure,
* or NULL if there is an end-of-directory marker or error.
*
* SEE ALSO:
* opendir(), closedir(), rewinddir(), ls()
*/

struct dirent *readdir
    (
    DIR         *pDir                   /* pointer to directory descriptor */
    )
    {
    if (ioctl (pDir->dd_fd, FIOREADDIR, (int)pDir) != OK)
	return (NULL);				/* can't ioctl */

    return (&pDir->dd_dirent);			/* return ptr to dirent */
    }

/*******************************************************************************
*
* rewinddir - reset position to the start of a directory (POSIX)
*
* This routine resets the position pointer in a directory descriptor (DIR).
* The <pDir> parameter is the directory descriptor pointer that was returned
* by opendir().
*
* As a result, the next readdir() will cause the current directory data to be
* read in again, as if an opendir() had just been performed.  Any changes
* in the directory that have occurred since the initial opendir() will now
* be visible.  The first entry in the directory will be returned by the
* next readdir().
*
* RETURNS: N/A
*
* SEE ALSO:
* opendir(), readdir(), closedir()
*/

void rewinddir
    (
    DIR         *pDir                   /* pointer to directory descriptor */
    )
    {
    pDir->dd_cookie = 0;			/* reset filesys-specific ptr */
    }

/*******************************************************************************
*
* closedir - close a directory (POSIX)
*
* This routine closes a directory which was previously opened using
* opendir().  The <pDir> parameter is the directory descriptor pointer
* that was returned by opendir().
*
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* opendir(), readdir(), rewinddir()
*/

STATUS closedir
    (
    DIR         *pDir                   /* pointer to directory descriptor */
    )
    {
    FAST STATUS	status;			/* return status */

    if ((status = close (pDir->dd_fd)) != ERROR)
        free ((char *) pDir);

    return (status);
    }

/*******************************************************************************
*
* fstat - get file status information (POSIX)
*
* This routine obtains various characteristics of a file (or directory).
* The file must already have been opened using open() or creat().
* The <fd> parameter is the file descriptor returned by open() or creat().
*
* The <pStat> parameter is a pointer to a `stat' structure (defined
* in stat.h).  This structure must be allocated before fstat() is called.
*
* Upon return, the fields in the `stat' structure are updated to
* reflect the characteristics of the file.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* stat(), ls()
*/

STATUS fstat
    (
    int         fd,                     /* file descriptor for file to check */
    struct stat *pStat                  /* pointer to stat structure */
    )
    {
    return (ioctl (fd, FIOFSTATGET, (int)pStat));
    }

/*******************************************************************************
*
* stat - get file status information using a pathname (POSIX)
*
* This routine obtains various characteristics of a file (or directory).
* This routine is equivalent to fstat(), except that the <name> of the file
* is specified, rather than an open file descriptor.
*
* The <pStat> parameter is a pointer to a `stat' structure (defined
* in stat.h).  This structure must have already been allocated before
* this routine is called.
*
* NOTE: When used with netDrv devices (FTP or RSH), stat() returns the size
* of the file and always sets the mode to regular; stat() does not distinguish
* between files, directories, links, etc.
*
* Upon return, the fields in the `stat' structure are updated to
* reflect the characteristics of the file.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* fstat(), ls()
*/

STATUS stat
    (
    char        *name,                  /* name of file to check */
    struct stat *pStat                  /* pointer to stat structure */
    )
    {
    FAST int	fd;			/* file descriptor */
    FAST STATUS	status;			/* return status */

    if ((fd = open (name, O_RDONLY, 0)) == ERROR)
	return (ERROR);				/* can't open file */

    status = fstat (fd, pStat);

    status |= close (fd);

    return (status);
    }

/*******************************************************************************
*
* fstatfs - get file status information (POSIX)
*
* This routine obtains various characteristics of a file system.
* A file in the file system must already have been opened using open() or creat().
* The <fd> parameter is the file descriptor returned by open() or creat().
*
* The <pStat> parameter is a pointer to a `statfs' structure (defined
* in stat.h).  This structure must be allocated before fstat() is called.
*
* Upon return, the fields in the `statfs' structure are updated to
* reflect the characteristics of the file.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* statfs(), ls()
*/

STATUS fstatfs
    (
    int           fd,                     /* file descriptor for file to check */
    struct statfs *pStat                  /* pointer to statfs structure */
    )
    {
    return (ioctl (fd, FIOFSTATFSGET, (int)pStat));
    }

/*******************************************************************************
*
* statfs - get file status information using a pathname (POSIX)
*
* This routine obtains various characteristics of a file system.
* This routine is equivalent to fstatfs(), except that the <name> of the file
* is specified, rather than an open file descriptor.
*
* The <pStat> parameter is a pointer to a `statfs' structure (defined
* in stat.h).  This structure must have already been allocated before
* this routine is called.
*
* Upon return, the fields in the `statfs' structure are updated to
* reflect the characteristics of the file.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* fstatfs(), ls()
*/

STATUS statfs
    (
    char          *name,                  /* name of file to check */
    struct statfs *pStat                  /* pointer to statfs structure */
    )
    {
    FAST int	fd;			/* file descriptor */
    FAST STATUS	status;			/* return status */

    if ((fd = open (name, O_RDONLY, 0)) == ERROR)
	return (ERROR);				/* can't open file */

    status = fstatfs (fd, pStat);

    status |= close (fd);

    return (status);
    }

/*******************************************************************************
*
* utime - update time on a file
*
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* stat(), fstat(), ls()
*/

int utime
    (
    char *           file,
    struct utimbuf * newTimes
    )
    {
    int fd;
    int retVal;

    if ((fd = open (file, O_RDONLY, 0666)) == ERROR)
        return (ERROR);
    else
	{
        retVal = ioctl (fd, FIOTIMESET, (int) newTimes);
	close (fd);
	return (retVal);
	}
    }

/* ioLib.c - I/O interface library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
04x,02Aug00,jgn  merge DOT-4 pthreads code +
		 add validation of null pointer for name as well as null string
04w,03oct01,dcb  Fix SPR 20033 side effect --- handle new return code from
                 iosFdSet.  Combine open() and creat() into one function.
04v,01mar99,yp   SPR 21655 - if iosDevFind() fails open() dosn't free fd
04u,21feb99,jdi  doc: listed errnos.
04t,03sep97,dgp  doc: SPR 9023 - correct reference to FIOGETNAME in ioctl()
04s,10jul97,dgp  doc: fix SPR 6117, rename() not supported on all devices
04r,20oct95,jdi  doc: addition for rename() (SPR 4167); added bit values
		    for open() and creat() flags (SPR 4276).
04q,10feb95,rhp  added names of stdio fd macros to ioLib man page
04p,26jul94,dvs  added doc for read/write if no driver routine (SPR #2019/2020).
04o,30sep93,jmm  resubmitted change originally in scd 10148
04n,08feb93,smb  changed int to size_t for read and write.
04m,21jan93,jdi  documentation cleanup for 5.1.
04l,29jul92,smb  added isatty for use by stdio library.
04k,22jul92,kdl  Replaced delete() with remove().
04j,18jul92,smb  Changed errno.h to errnoLib.h.
04i,04jul92,jcf  scalable/ANSI/cleanup effort.
04h,26may92,rrr  the tree shuffle
04g,28feb92,wmd  added bzero() call in ioFullFileNameGet() to init to 0's
		 fullPathName[] buffer before it is passed to subordinates.
04f,05dec91,rrr  made lseek take posix arguments (SEEK_)
04e,25nov91,llk  ansi stuff.  Modified remove() and rename().
		 included stdio.h.
04d,25nov91,rrr  cleanup of some ansi warings.
04c,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed READ, WRITE and UPDATE to O_RDONLY O_WRONLY and ...
		  -changed VOID to void
		  -changed copyright notice
04b,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
04a,08feb91,jaa	 documentation cleanup.
03z,15oct90,dnw  made ioFullFileNameGet() be NOMANUAL
03y,05oct90,dnw  made readv, writev, ioDefDevGet, ioDefDirGet be NOMANUAL
		 made ioFullFileNameGet() use pathCat()
		 added chdir(), getcwd(), rename() for POSIX compliance
		 added remove() for ANSI compliance
		 added ioDefPathCat() (NOMANUAL)
03x,06jun90,rdc  added readv and writev.
03w,25may90,dnw  changed creat and open to NOT malloc filename before calling
		   iosFdSet.  This is now left up to iosFdSet.
03v,11may90,yao	 added missing modification history (03u) for the last checkin.
03u,09may90,yao	 typecasted malloc to (char *).
03t,01may90,llk  changed call to pathCat().  Now, pathCat() checks if a name
		   is too long, and returns ERROR if it is.
03s,14apr90,jcf  removed tcb extension dependencies.
03r,01apr90,llk  changed ioFullFileNameGet() and ioDefPathSet() so that only
		   filenames of MAX_FILENAME_LENGTH characters or less will
		   be accepted.
03q,14mar90,jdi  documentation cleanup.
03p,12dec89,dab  documentation.
03o,15nov89,dab  changed variable name mode to flag in creat().  documentation.
            rdc  added unlink, getwd for unix compatibility.
03n,02may89,dab  documentation touchup in lseek().
03m,23mar89,dab  made L_XTND option work in lseek(); fixed L_INCR bug in
		 lseek(); documentation touchup in creat().
03l,23nov88,llk  fixed bug in handling multiple links.
	    dnw  changed ioTaskStd{Set,Get} to take taskId arg.
03k,08oct88,gae  made global standard fd array local.
03j,07sep88,gae  documentation.
03i,29aug88,gae  documentation.
03h,15aug88,jcf  lint.
03g,15jul88,llk  changed to allocate an fd name before calling iosFdSet or
		   iosFdNew.
03f,30jun88,llk  changed open and create so that they handle links.
03e,04jun88,llk  uses a default path instead of a default device.
		 changed ioDefDev to ioDefPath.
		 added ioDefPathPrompt, ioDefPathShow, ioDefPathSet,
		    ioDefPathGet, ioDefDirGet.
		 rewrote ioDefDevGet.
		 removed ioSetDefDev.
		 added ioFullFileNameGet.
03d,30may88,dnw  changed to v4 names.
03c,28may88,dnw  deleted obsolete create().
03b,27may88,llk  see change 03e above.
03a,30mar88,gae  made I/O system, i.e. iosLib, responsible for low-level
		   creat,open,read,write,close calls to drivers as
		   fdTable & drvTable are now hidden.
                 added parameters to open() for UNIX compatibility.
		 made L_INCR option work in lseek().
		 added io{G,S}et{Global,Task}Std().
02u,19apr88,llk  added a parameter to open.  Inspired by NFS and aspirations
		   toward UNIX compatibility.
02t,31dec87,jlf  made creat and open follow links.
02s,05nov87,jlf  documentation
02r,30sep87,gae  removed usage of fdTable by using new iosFdSet().
		 added FIOGETNAME to ioctl.
02q,28aug87,dnw  changed creat() and open() to only treat ERROR (-1) as error
		   indication from driver routines instead of < 0 to allow
		   driver values (i.e. structure pointers) to be in memory
		   with high bit set.
02p,28apr87,gae  close() now returns status; added UNIX compatible creat().
02o,24mar87,jlf  documentation
02n,25feb87,ecs  minor documentation.
		 added include of strLib.h.
02m,14feb87,dnw  changed create(), open(), delete() to return ERROR if null
		   filename is specified.  However, filename "." is turned
		   into null filename before being passed to driver routine.
02l,20dec86,dnw  fixed ioGetDefDevTail() to require complete match of dev name.
		 changed to not get include files from default directories.
02k,07oct86,gae	 added ioGetDefDevTail().
02j,04sep86,jlf  minor documentation
02i,01jul86,jlf  minor documentation
02h,03apr86,llk	 fixed syntax error in lseek.
02g,03mar86,jlf  changed ioctrl calls to ioctl.
		 added lseek.
...deleted pre 86 history - see RCS
*/

/*
DESCRIPTION
This library contains the interface to the basic I/O system.
It includes:
.iP "" 4
Interfaces to the seven basic driver-provided functions:
creat(), remove(), open(), close(), read(), write(), and ioctl().
.iP
Interfaces to several file system functions, including
rename() and lseek().
.iP
Routines to set and get the current working directory.
.iP
Routines to assign task and global standard file descriptors.

FILE DESCRIPTORS
At the basic I/O level, files are referred to by a file descriptor.
A file descriptor is a small integer returned by a call to open() or creat().  
The other basic I/O calls take a file descriptor as a parameter to specify 
the intended file.

Three file descriptors are reserved and have special meanings:

    0 (`STD_IN')  - standard input
    1 (`STD_OUT') - standard output
    2 (`STD_ERR') - standard error output

VxWorks allows two levels of redirection.  First, there is a global
assignment of the three standard file descriptors.  By default, new tasks use
this global assignment.  The global assignment of the three standard
file descriptors is controlled by the routines ioGlobalStdSet() and 
ioGlobalStdGet().

Second, individual tasks may override the global assignment of these
file descriptors with their own assignments that apply only to that task.  
The assignment of task-specific standard file descriptors is controlled 
by the routines ioTaskStdSet() and ioTaskStdGet().  

INCLUDE FILES: ioLib.h

SEE ALSO: iosLib, ansiStdio,
.pG "I/O System"
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "sys/types.h"
#include "memLib.h"
#include "ioLib.h"
#include "iosLib.h"
#include "string.h"
#include "taskLib.h"
#include "net/uio.h"
#include "errnoLib.h"
#include "stdio.h"
#include "fcntl.h"
#include "unistd.h"
#include "pathLib.h"
#include "string.h"
#define __PTHREAD_SRC
#include "pthread.h"

IMPORT char ioDefPath [MAX_FILENAME_LENGTH];	/* current default i/o prefix */

#define	STD_VALID(fd)	(((fd) >= 0) && ((fd) < 3))

/* globals */

BOOL ioMaxLinkLevels = 20;	/* maximum number of symlinks to traverse */

/* forward declarations */
LOCAL int ioCreateOrOpen (const char *name, int flags, int mode, BOOL create);

/* locals */

LOCAL int ioStdFd [3];		/* global standard input/output/error */


/*******************************************************************************
*
* creat - create a file
*
* This routine creates a file called <name> and opens it with a specified
* <flag>.  This routine determines on which device to create the file; 
* it then calls the create routine of the device driver to do most of the work.
* Therefore, much of what transpires is device/driver-dependent.
*
* The parameter <flag> is set to O_RDONLY (0), O_WRONLY (1), or O_RDWR (2)
* for the duration of time the file is open.  To create NFS files with a
* UNIX chmod-type file mode, call open() with the file mode specified in
* the third argument.
*
* INTERNAL
* A driver's create routine will return FOLLOW_LINK if any part of the file name
* contains a link (in the directory path).  In this case, it will also
* have changed the name of the file being opened to incorporate the name of
* the link.  The new file name is then repeatedly resubmitted to the driver's
* open routine until all links are resolved.
*
* NOTE
* For more information about situations when there are no file descriptors
* available, see the manual entry for iosInit().
*
* RETURNS:
* A file descriptor number, or ERROR if a filename is not specified, the
* device does not exist, no file descriptors are available, or the driver
* returns ERROR.
*
* SEE ALSO: open()
*
*/

int creat
    (
    const char *name,   /* name of the file to create    */
    int        flag   	/* O_RDONLY, O_WRONLY, or O_RDWR */
    )
    {
    return ioCreateOrOpen (name, flag, 0, TRUE);
    }


/*******************************************************************************
*
* open - open a file
*
* This routine opens a file for reading, writing, or updating, and returns
* a file descriptor for that file.  The arguments to open() are the filename 
* and the type of access:
*
* .TS
* tab(|);
* 8l l l.
* O_RDONLY  (0)    |(or READ)   |- open for reading only.
* O_WRONLY  (1)    |(or WRITE)  |- open for writing only.
* O_RDWR  (2)      |(or UPDATE) |- open for reading and writing.
* O_CREAT  (0x0200)|            |- create a file.
* .TE
*
* In general, open() can only open pre-existing devices and files.  However,
* for NFS network devices only, files can also be created with open() by
* performing a logical OR operation with O_CREAT and the <flags> argument.
* In this case, the file is created with a UNIX chmod-style file mode, as
* indicated with <mode>.  For example:
* .CS
*     fd = open ("/usr/myFile", O_CREAT | O_RDWR, 0644);
* .CE
* Only the NFS driver uses the <mode> argument.
*
* INTERNAL
* A driver's open routine will return FOLLOW_LINK if any part of the file name
* contains a link (directory path or file name).  In this case, it will also
* have changed the name of the file being opened to incorporate the name of
* the link.  The new file name is then repeatedly resubmitted to the driver's
* open routine until all links are resolved.
*
* NOTE
* For more information about situations when there are no file descriptors
* available, see the manual entry for iosInit().
*
* RETURNS:
* A file descriptor number, or ERROR if a file name is not specified, the
* device does not exist, no file descriptors are available, or the driver
* returns ERROR.
*
* ERRNO: ELOOP
*
* SEE ALSO: creat()
*
* VARARGS2
*
*/

int open
    (
    const char *name,   /* name of the file to open                  */
    int flags,          /* O_RDONLY, O_WRONLY, O_RDWR, or O_CREAT    */
    int mode            /* mode of file to create (UNIX chmod style) */
    )
    {
    return ioCreateOrOpen (name, flags, mode, FALSE);
    }


/*******************************************************************************
*
* ioCreateOrOpen creat() or open() a file
*
* This is the worker function for creat() and for open().  See their
* descriptions.
*
* NOMANUAL
*/

LOCAL int ioCreateOrOpen
    (
    const char *name,   /* name of the file to create    */
    int        flags,   /* O_RDONLY, O_WRONLY, O_RDWR or O_CREAT (open call) */
    int        mode,    /* mode of ile to create (if open call) */
    BOOL       create   /* set to TRUE if this is a creat() call */
    )
    {
    DEV_HDR *pDevHdr1;
    DEV_HDR *pDevHdr2 = NULL;
    int     value;
    int     fd = (int)ERROR;
    char    fullFileName [MAX_FILENAME_LENGTH];
    int     error = 0;

    char *pPartFileName;
    int savtype;

    /* don't allow null filename (for user protection) but change
     * "." into what null filename would be if we allowed it */

    if ((name == NULL) || (name[0] == EOS))
	{
	errnoSet (S_ioLib_NO_FILENAME);
	error = 1;
	goto handleError;
	}
    
    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);
        }

    if (strcmp (name, ".") == 0)
	{
	/* point to EOS (turn it into null filename) */
	++name;
	}

    
    if (ioFullFileNameGet (name, &pDevHdr1, fullFileName) == ERROR)
	{
	error = 1;
	goto handleError;
	}

    
    if ((fd = iosFdNew (pDevHdr1, (char *)NULL, 0)) == ERROR)
	{
	error = 1;
	goto handleError;
	}

    
    if (create == TRUE)
	{
	value = iosCreate (pDevHdr1, fullFileName, flags);
	}
    else
	{
	value = iosOpen (pDevHdr1, fullFileName, flags, mode);
	}
    
    if (value == ERROR)
	{
	error = 2;
	goto handleError;
	}

    
    /* In case the path does not follow the while loop, get the pDevHdr1
       recorded in pDevHdr2 for error handling. */
    pDevHdr2 = pDevHdr1;

    
    while (value == FOLLOW_LINK)
	{
	int linkCount = 1;
	
	/* if a file name contains a link, the driver's create routine changed
	 * fullFileName to incorporate the link's name. Try to create the file
	 * that the driver's create routine handed back.
	 */

	if (linkCount++ > ioMaxLinkLevels)
	    {
	    errno = ELOOP;
	    error = 2;
	    goto handleError;
	    }

	if ((pDevHdr2 = iosDevFind (fullFileName, &pPartFileName)) == NULL)
	    {
	    error = 2;
	    goto handleError;
	    }

	if (fullFileName != pPartFileName)
	    {
	    /* link file name starts with a vxWorks device name,
	     * possibly a different device from the current one.
	     */

	    strncpy (fullFileName, pPartFileName, MAX_FILENAME_LENGTH);

	    }
	else
	    {
	    /* link file name does not start with a vxWorks device name.
	     * create the file on the current device.
	     */

	    pDevHdr2 = pDevHdr1;
	    }

	
	if (create == TRUE)
	    {
	    value = iosCreate (pDevHdr2, fullFileName, flags);
	    }
	else
	    {
	    value = iosOpen (pDevHdr2, fullFileName, flags, mode);
	    }
	
	if (value == ERROR)
	    {
	    error = 2;
	    goto handleError;
	    }

	
	} /* while */


    
    if ((iosFdSet (fd, pDevHdr1, CHAR_FROM_CONST(name), value)) == ERROR)
	{
	error = 3;   
	goto handleError;
	}

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(savtype, NULL);
        }

    return (fd);
    
handleError:

    if (_func_pthread_setcanceltype != NULL)
	{
	_func_pthread_setcanceltype(savtype, NULL);
	}

    if (error > 2)
	{
	iosDelete (pDevHdr2, fullFileName);
	}
    if (error > 1)
	{
	iosFdFree (fd);
	}
    if (error > 0)
	{
	fd = (int)ERROR;
	}
    
    return (fd);
    }
/*******************************************************************************
*
* unlink - delete a file (POSIX)
*
* This routine deletes a specified file.  It performs the same function
* as remove() and is provided for POSIX compatibility.
*
* RETURNS:
* OK if there is no delete routine for the device or the driver returns OK;
* ERROR if there is no such device or the driver returns ERROR.
*
* SEE ALSO: remove()
*/

STATUS unlink
    (
    char *name          /* name of the file to remove */
    )
    {
    return (remove (name));
    }
/********************************************************************************
* remove - remove a file (ANSI)
*
* This routine deletes a specified file.  It calls the driver for the
* particular device on which the file is located to do the work.
*
* RETURNS:
* OK if there is no delete routine for the device or the driver returns OK;
* ERROR if there is no such device or the driver returns ERROR.
*
* SEE ALSO:
* .I "American National Standard for Information Systems -"
* .I "Programming Language - C, ANSI X3.159-1989: Input/Output (stdio.h),"
*/
 
STATUS remove
    (
    const char *name          /* name of the file to remove */
    )
    {
    DEV_HDR *pDevHdr;
    char fullFileName [MAX_FILENAME_LENGTH];

    /* don't allow null filename (for user protection) */
 
    if ((name == NULL) || (name[0] == EOS))
        {
        errnoSet (S_ioLib_NO_FILENAME);
        return (ERROR);
        }
 
    if (ioFullFileNameGet (name, &pDevHdr, fullFileName) == ERROR)
        return (ERROR);
 
    return (iosDelete (pDevHdr, fullFileName));
    }
/*******************************************************************************
*
* close - close a file
*
* This routine closes the specified file and frees the file descriptor.
* It calls the device driver to do the work.
*
* RETURNS:
* The status of the driver close routine, or ERROR if the file descriptor 
* is invalid.
*/

STATUS close
    (
    int fd              /* file descriptor to close */
    )
    {
    int ret, savtype;

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);
        }

    ret = iosClose (fd);

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(savtype, NULL);
        }

    return (ret);
    }

/*******************************************************************************
*
* rename - change the name of a file
*
* This routine changes the name of a file from <oldfile> to <newfile>.
*
* NOTE: Only certain devices support rename().  To confirm that your device
* supports it, consult the respective xxDrv or xxFs listings to verify that
* ioctl FIORENAME exists.  For example, dosFs and rt11Fs support rename(),
* but netDrv and nfsDrv do not.
*
* RETURNS: OK, or ERROR if the file could not be opened or renamed.
*/

int rename
    (
    const char *oldname,	/* name of file to rename         */
    const char *newname		/* name with which to rename file */
    )
    {
    int fd;
    int status;

    if ((oldname == NULL) || (newname == NULL) || (newname[0] == EOS))
	{
	errnoSet (ENOENT);
	return (ERROR);
	}

    /* try to open file */

    if ((fd = open ((char *) oldname, O_RDONLY, 0)) < OK)
	return (ERROR);

    /* rename it */

    status = ioctl (fd, FIORENAME, (int) newname);

    close (fd);
    return (status);
    }
/*******************************************************************************
*
* read - read bytes from a file or device
*
* This routine reads a number of bytes (less than or equal to <maxbytes>)
* from a specified file descriptor and places them in <buffer>.  It calls
* the device driver to do the work.
*
* RETURNS:
* The number of bytes read (between 1 and <maxbytes>, 0 if end of file), or
* ERROR if the file descriptor does not exist, the driver does not have
* a read routines, or the driver returns ERROR. If the driver does not 
* have a read routine, errno is set to ENOTSUP.
*/

int read
    (
    int fd,             /* file descriptor from which to read   */
    char *buffer,       /* pointer to buffer to receive bytes   */
    size_t maxbytes     /* max no. of bytes to read into buffer */
    )
    {
    int ret, savtype;

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);
        }

    ret = iosRead (fd, buffer, (int) maxbytes);

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(savtype, NULL);
        }

    return (ret);
    }
/*******************************************************************************
*
* write - write bytes to a file
*
* This routine writes <nbytes> bytes from <buffer> to a specified file
* descriptor <fd>.  It calls the device driver to do the work.
*
* RETURNS:
* The number of bytes written (if not equal to <nbytes>, an error has
* occurred), or ERROR if the file descriptor does not exist, the driver
* does not have a write routine, or the driver returns ERROR. If the driver
* does not have a write routine, errno is set to ENOTSUP.
*/

int write
    (
    int fd,             /* file descriptor on which to write     */
    char *buffer,       /* buffer containing bytes to be written */
    size_t nbytes       /* number of bytes to write              */
    )
    {
    int ret, savtype;

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &savtype);
        }

    ret = iosWrite (fd, buffer, (int) nbytes);

    if (_func_pthread_setcanceltype != NULL)
        {
        _func_pthread_setcanceltype(savtype, NULL);
        }

    return (ret);
    }
/*******************************************************************************
*
* ioctl - perform an I/O control function
*
* This routine performs an I/O control function on a device.  The control
* functions used by VxWorks device drivers are defined in the header file
* ioLib.h.  Most requests are passed on to the driver for handling.
* Since the availability of ioctl() functions is driver-specific, these
* functions are discussed separately in tyLib, pipeDrv, nfsDrv, dosFsLib,
* rt11FsLib, and rawFsLib.
*
* The following example renames the file or directory to the string "newname":
*
* .CS
*     ioctl (fd, FIORENAME, "newname");
* .CE
*
* Note that the function FIOGETNAME is handled by the I/O interface level and
* is not passed on to the device driver itself.  Thus this function code value
* should not be used by customer-written drivers.
*
* RETURNS:
* The return value of the driver, or ERROR if the file descriptor does 
* not exist.
*
* SEE ALSO: tyLib, pipeDrv, nfsDrv, dosFsLib, rt11FsLib, rawFsLib,
* .pG "I/O System, Local File Systems"
*
* VARARGS2
*/

int ioctl
    (
    int fd,             /* file descriptor    */
    int function,       /* function code      */
    int arg             /* arbitrary argument */
    )
    {
    return (iosIoctl (fd, function, arg));
    }
/*******************************************************************************
*
* lseek - set a file read/write pointer
*
* This routine sets the file read/write pointer of file <fd> 
* to <offset>.
* The argument <whence>, which affects the file position pointer,
* has three values:
* 
* .TS
* tab(|);
* 8l l.
* SEEK_SET  (0) |- set to <offset>
* SEEK_CUR  (1) |- set to current position plus <offset>
* SEEK_END  (2) |- set to the size of the file plus <offset>
* .TE
* 
* This routine calls ioctl() with functions FIOWHERE, FIONREAD, and FIOSEEK.
*
* RETURNS:
* The new offset from the beginning of the file, or ERROR.
*
* ARGSUSED
*/

int lseek
    (
    int fd,             /* file descriptor            */
    long offset,        /* new byte offset to seek to */
    int whence          /* relative file position     */
    )
    {
    int where;
    long nBytes;

    switch (whence)
	{
	case SEEK_SET:
	    return ((ioctl (fd, FIOSEEK, offset) == OK) ? offset : ERROR);

	case SEEK_CUR:
	    if ((where = ioctl (fd, FIOWHERE, 0 /*XXX*/)) == ERROR)
		return (ERROR);

	    offset += where;
	    return ((ioctl (fd, FIOSEEK, offset) == OK) ? offset : ERROR);

	case SEEK_END:
	    if ((where = ioctl (fd, FIOWHERE, 0 /*XXX*/)) == ERROR)
		return (ERROR);
	    if (ioctl (fd, FIONREAD, (int) (&nBytes)) == ERROR)
		return (ERROR);

	    offset += where + nBytes;
	    return ((ioctl (fd, FIOSEEK, offset) == OK) ? offset : ERROR);

	default:
	    return (ERROR);
	}
    }
/*******************************************************************************
*
* readv - read data from a device into scattered buffers
*
* readv takes as a parameter a list of buffers (iov) into which it should
* place data read from the specified file descriptor (fd).  iov is a pointer
* to an array of length iovcnt of (struct iovec) structures, which describe
* the location and length of each individual buffer.  readv will always
* completely fill a buffer before moving on to the next buffer in the list.
*
* RETURNS:
* Number of bytes read (0 if end of file), or
* ERROR if the file descriptor does not exist or the driver returns ERROR.
*
* NOMANUAL
*/

int readv
    (
    int fd,             /* file descriptor                   */
    struct iovec *iov,  /* buffer list                       */
    int iovcnt          /* number of elements in buffer list */
    )
    {
    int i;
    FAST char *pBuf;
    FAST int bytesToRead;
    FAST int totalBytesRead = 0;
    FAST int bytesRead;

    for (i = 0; i < iovcnt; i++)
        {
        pBuf = (char *) iov[i].iov_base;
        bytesToRead = iov[i].iov_len;
        while (bytesToRead > 0)
            {
            if ((bytesRead = iosRead (fd, pBuf, bytesToRead)) == ERROR)
                {
                if (totalBytesRead > 0)
                    return (totalBytesRead);
                else
                    return (ERROR);
                }

	    if (bytesRead == 0)
		return (totalBytesRead);

            totalBytesRead += bytesRead;
            bytesToRead -= bytesRead;
            pBuf += bytesRead;
            }
        }
    return (totalBytesRead);
    }

/*******************************************************************************
*
* writev - write data from scattered buffers to a device
*
* writev takes as a parameter a list of buffers (iov) from which it should
* write data to the specified file descriptor (fd).  iov is a pointer
* to an array of length iovcnt of (struct iovec) structures, which describe
* the location and length of each individual buffer.  writev will always
* completely empty a buffer before moving on to the next buffer in the list.
*
* RETURNS:
* Number of bytes written, or
* ERROR if the file descriptor does not exist or the driver returns ERROR.
*
* NOMANUAL
*/

int writev
    (
    int fd,
    register struct iovec *iov,
    int iovcnt
    )
    {
    int i;
    register char *pData;
    register int bytesToWrite;
    register int totalBytesWritten = 0;
    register int bytesWritten;

    for (i = 0; i < iovcnt; i++)
        {
        pData = (char *) iov[i].iov_base;
        bytesToWrite = iov[i].iov_len;
        while (bytesToWrite > 0)
            {
            if ((bytesWritten = iosWrite (fd, pData, bytesToWrite)) == ERROR)
                {
                if (totalBytesWritten > 0)
                    return (totalBytesWritten);
                else
                    return (ERROR);
                }
            totalBytesWritten += bytesWritten;
            bytesToWrite -= bytesWritten;
            pData += bytesWritten;
            }
        }
    return (totalBytesWritten);
    }



/*******************************************************************************
*
* ioFullFileNameGet - resolve path name to device and full file name
*
* This routine resolves the specified path name into a device and full
* filename on that device.  If the specified path name is not a full,
* absolute path name, then it is concatenated to the current default
* path to derive a full path name.  This full path name is separated
* into the device specification and the remaining filename on that device.
* A pointer to the device header is returned in <ppDevHdr> and the
* filename on that device is returned in <fullFileName>.
*
* RETURNS:
* OK, or
* ERROR if the full filename has more than MAX_FILENAME_LENGTH characters.
*
* NOMANUAL
*/

STATUS ioFullFileNameGet
    (
    char *	pathName,	/* path name */
    DEV_HDR **		ppDevHdr,	/* ptr to dev hdr to complete */
    char *		fullFileName	/* resulting complete filename */
    )
    {
    char *pTail;
    char fullPathName [MAX_FILENAME_LENGTH];

    bzero (fullPathName, MAX_FILENAME_LENGTH);  /* initialize buffer to 0 */

    /* resolve default path plus partial pathname to full path name */

    if (pathCat (ioDefPath, pathName, fullPathName) == ERROR)
	return (ERROR);

    /* remove device name from full path name */

    if ((*ppDevHdr = iosDevFind (fullPathName, &pTail)) == NULL)
	return (ERROR);

    strncpy (fullFileName, pTail, MAX_FILENAME_LENGTH);

    return (OK);
    }

/*******************************************************************************
*
* ioDefPathSet - set the current default path
*
* This routine sets the default I/O path.  All relative pathnames specified
* to the I/O system will be prepended with this pathname.  This pathname
* must be an absolute pathname, i.e., <name> must begin with an existing
* device name.
*
* RETURNS:
* OK, or ERROR if the first component of the pathname is not an existing
* device.
*
* SEE ALSO: ioDefPathGet(), chdir(), getcwd()
*/

STATUS ioDefPathSet
    (
    char *name          /* name of the new default device and path */
    )
    {
    char *pTail = name;

    if (iosDevFind (name, &pTail) == NULL)
	return (ERROR);

    if (pTail == name)
	{
	/* name does not begin with an existing device's name */

	errnoSet (S_ioLib_NO_DEVICE_NAME_IN_PATH);
	return (ERROR);
	}

    if (strlen (name) >= MAX_FILENAME_LENGTH)
	{
	errnoSet (S_ioLib_NAME_TOO_LONG);
	return (ERROR);
	}

    strcpy (ioDefPath, name);
    return (OK);
    }
/*******************************************************************************
*
* ioDefPathGet - get the current default path
*
* This routine copies the name of the current default path to <pathname>.
* The parameter <pathname> should be MAX_FILENAME_LENGTH characters long.
*
* RETURNS: N/A
*
* SEE ALSO: ioDefPathSet(), chdir(), getcwd()
*/

void ioDefPathGet
    (
    char *pathname              /* where to return the name */
    )
    {
    strcpy (pathname, ioDefPath);
    }

/*******************************************************************************
*
* chdir - set the current default path
*
* This routine sets the default I/O path.  All relative pathnames specified
* to the I/O system will be prepended with this pathname.  This pathname
* must be an absolute pathname, i.e., <name> must begin with an existing
* device name.
*
* RETURNS:
* OK, or ERROR if the first component of the pathname is not an existing device.
*
* SEE ALSO: ioDefPathSet(), ioDefPathGet(), getcwd()
*/

STATUS chdir
    (
    char *pathname      /* name of the new default path */
    )
    {
    return (ioDefPathSet (pathname));
    }
/*******************************************************************************
*
* getcwd - get the current default path (POSIX)
*
* This routine copies the name of the current default path to <buffer>.
* It provides the same functionality as ioDefPathGet() and
* is provided for POSIX compatibility.
*
* RETURNS:
* A pointer to the supplied buffer, or NULL if <size> is too small to hold
* the current default path.
*
* SEE ALSO: ioDefPathSet(), ioDefPathGet(), chdir()
*/

char *getcwd
    (
    char *buffer,       /* where to return the pathname */
    int  size           /* size in bytes of buffer      */
    )
    {
    if (size <= 0)
	{
	errnoSet (EINVAL);
	return (NULL);
	}

    if (size < strlen (ioDefPath) + 1)
	{
	errnoSet (ERANGE);
	return (NULL);
	}

    strcpy (buffer, ioDefPath);

    return (buffer);
    }

/*******************************************************************************
*
* getwd - get the current default path
*
* This routine copies the name of the current default path to <pathname>.
* It provides the same functionality as ioDefPathGet() and getcwd().
* It is provided for compatibility with some older UNIX systems.
*
* The parameter <pathname> should be MAX_FILENAME_LENGTH characters long.
*
* RETURNS: A pointer to the resulting path name.
*/

char *getwd
    (
    char *pathname      /* where to return the pathname */
    )
    {
    strcpy (pathname, ioDefPath);
    return (pathname);
    }

/*******************************************************************************
*
* ioDefPathCat - concatenate to current default path
*
* This routine changes the current default path to include the specified
* <name>.  If <name> is itself an absolute pathname beginning with
* a device name, then it becomes the new default path.  Otherwise <name>
* is appended to the current default path in accordance with the rules
* of concatenating path names.
*
* RETURNS:
* OK, or
* ERROR if a valid pathname cannot be derived from the specified name.
*
* NOMANUAL
*/

STATUS ioDefPathCat
    (
    char *name          /* path to be concatenated to current path */
    )
    {
    char newPath [MAX_FILENAME_LENGTH];
    char *pTail;

    /* interpret specified path in terms of current default path */

    if (pathCat (ioDefPath, name, newPath) == ERROR)
	return (ERROR);

    /* make sure that new path starts with a device name */

    iosDevFind (newPath, &pTail);
    if (pTail == newPath)
	{
	errnoSet (S_ioLib_NO_DEVICE_NAME_IN_PATH);
	return (ERROR);
	}

    pathCondense (newPath);	/* resolve ".."s, "."s, etc */

    strncpy (ioDefPath, newPath, MAX_FILENAME_LENGTH);

    return (OK);
    }

/*******************************************************************************
*
* ioDefDevGet - get current default device
*
* This routine copies the name of the device associated with the current
* default pathname to <devName>.
*
* NOTE: This routine was public in 4.0.2 but is obsolete.  It is made
* no-manual in 5.0 and should be removed in the next release.
*
* NOMANUAL
*/

void ioDefDevGet
    (
    char *devName               /* where to return the device name */
    )
    {
    DEV_HDR *pDevHdr;	/* pointer to device header */
    char *pTail;

    /* find the device of the default pathname */

    if ((pDevHdr = iosDevFind (ioDefPath, &pTail)) == NULL)
	{
	*devName = EOS;		/* no default device, return null device name */
	}
    else
	{
	strcpy (devName, pDevHdr->name);	/* return device name */
	}
    }
/*******************************************************************************
*
* ioDefDirGet - get current default directory
*
* This routine copies the current default directory to <dirName>.
* The current default directory is derived from the current default
* pathname minus the leading device name.
*
* <dirname> should be MAX_FILENAME_LENGTH characters long.
*
* NOTE: This routine was public in 4.0.2 but is obsolete.  It is made
* no-manual in 5.0 and should be removed in the next release.
*
* NOMANUAL
*/

void ioDefDirGet
    (
    char *dirName               /* where to return the directory name */
    )
    {
    char *pTail;

    /* find the directory name in the default path name */

    if (iosDevFind (ioDefPath, &pTail) == NULL)
	{
	*dirName = EOS;	/* no default device, return null directory name */
	}
    else
	{
	strcpy (dirName, pTail);
	}
    }

/*******************************************************************************
*
* ioGlobalStdSet - set the file descriptor for global standard input/output/error
*
* This routine changes the assignment of a specified global standard file
* descriptor <stdFd> (0, 1, or, 2) to the specified underlying file
* descriptor <newFd>.  <newFd> should be a file descriptor open to the
* desired device or file.  All tasks will use this new assignment when doing
* I/O to <stdFd>, unless they have specified a task-specific standard file
* descriptor (see ioTaskStdSet()).  If <stdFd> is not 0, 1, or 2, this
* routine has no effect.
*
* RETURNS: N/A
*
* SEE ALSO: ioGlobalStdGet(), ioTaskStdSet()
*/

void ioGlobalStdSet
    (
    int stdFd,  /* std input (0), output (1), or error (2) */
    int newFd   /* new underlying file descriptor          */
    )
    {
    if (STD_VALID (stdFd))
	ioStdFd [stdFd] = newFd;
    }
/*******************************************************************************
*
* ioGlobalStdGet - get the file descriptor for global standard input/output/error
*
* This routine returns the current underlying file descriptor for global 
* standard input, output, and error.
*
* RETURNS:
* The underlying global file descriptor, or ERROR if <stdFd> is not 0, 1, or 2.
*
* SEE ALSO: ioGlobalStdSet(), ioTaskStdGet()
*/

int ioGlobalStdGet
    (
    int stdFd   /* std input (0), output (1), or error (2) */
    )
    {
    return (STD_VALID (stdFd) ? ioStdFd [stdFd] : ERROR);
    }
/*******************************************************************************
*
* ioTaskStdSet - set the file descriptor for task standard input/output/error
*
* This routine changes the assignment of a specified task-specific standard
* file descriptor <stdFd> (0, 1, or, 2) to the specified underlying file
* descriptor<newFd>.  <newFd> should be a file descriptor open to the
* desired device or file.  The calling task will use this new assignment
* when doing I/O to <stdFd>, instead of the system-wide global assignment
* which is used by default.  If <stdFd> is not 0, 1, or 2, this routine has
* no effect.
*
* NOTE: This routine has no effect if it is called at interrupt level.
*
* RETURNS: N/A
*
* SEE ALSO: ioGlobalStdGet(), ioTaskStdGet()
*/

void ioTaskStdSet
    (
    int taskId, /* task whose std fd is to be set (0 = self) */
    int stdFd,  /* std input (0), output (1), or error (2)   */
    int newFd   /* new underlying file descriptor            */
    )
    {
    WIND_TCB *pTcb;

    if (STD_VALID (stdFd) && (pTcb = taskTcb (taskId)) != NULL)
	pTcb->taskStd [stdFd] = newFd;
    }
/*******************************************************************************
*
* ioTaskStdGet - get the file descriptor for task standard input/output/error
*
* This routine returns the current underlying file descriptor for task-specific
* standard input, output, and error.
*
* RETURNS:
* The underlying file descriptor, or ERROR if <stdFd> is not 0, 1, or 2, or
* the routine is called at interrupt level.
*
* SEE ALSO: ioGlobalStdGet(), ioTaskStdSet()
*/

int ioTaskStdGet
    (
    int taskId, /* ID of desired task (0 = self)           */
    int stdFd   /* std input (0), output (1), or error (2) */
    )
    {
    WIND_TCB *pTcb;
    int taskFd;

    if (STD_VALID (stdFd) && (pTcb = taskTcb (taskId)) != NULL)
	{
	taskFd = pTcb->taskStd [stdFd];
	if (STD_VALID (taskFd))
	    return (ioStdFd [taskFd]);
	else
	    return (taskFd);
	}

    return (ERROR);
    }

/********************************************************************************
* isatty - return whether the underlying driver is a tty device
*
* This routine simply invokes the ioctl() function FIOISATTY on the
* specified file descriptor.
*
* RETURNS: TRUE, or FALSE if the driver does not indicate a tty device.
*/

BOOL isatty
    (
    int fd      /* file descriptor to check */
    )
    {
    return (ioctl (fd, FIOISATTY, 0 /*XXX*/) == TRUE);
    }


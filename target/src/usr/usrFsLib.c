/* usrFsLib.c - file system user interface subroutine library */

/* Copyright 1984-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01r,16jan02,chn  SPR#24332, add check to avoid copy() to a directory name dest
01q,12dec01,jkf  SPR#72133, add FIOMOVE to dosFsLib, use it in mv()
01p,17oct01,jkf  SPR#74904, function cleanup.
01o,20sep01,jkf  SPR#69031, common code for both AE & 5.x.
01n,15mar01,jkf  SPR#33973, copy not preserving time/date.
01m,15mar01,jkf  SPR#33557, ls returning error on first invocation
01l,29aug00,jgn  add const to function signatures (SPR #30492) +
		 clean up coding standard violations
01k,17mar00,dgp  modify attrib and xattrib synopsis comments for syngen
                 processing
01j,16mar00,jkf  removed reference to dosFsVolFormat() from diskInit().
01i,26dec99,jkf  T3 KHEAP_ALLOC
01h,31jul99,jkf  T2 merge, tidiness & spelling.
01g,07dec98,lrn  fixed netDrv spurious error message (SPR#22554)
01f,23nov98,vld  print '/' after directory name in long output
01e,13jul98,lrn  fixed diskInit() to call dosFsVolFormat() if open() fails
01d,02jul98,lrn  corrected parameters for chkdsk()
01c,28jun98,lrn  added recursive xcopy, xdelete, added wildcards to
                 cp(), mv(), added attrib and xattrib(), added help
01b,23jun98,lrn  rewrite ls, add more utils
01a,19feb98,vld	 initial version, derived from usrLib.c
*/

/*
DESCRIPTION
This library provides user-level utilities for managing file systems.
These utilities may be used from Tornado Shell, the Target Shell or from
an application.

USAGE FROM TORNADO
Some of the functions in this library have counterparts of the same
names built into the Tornado Shell (aka Windsh). The built-in functions
perform similar functions on the Tornado host computer's I/O systems.
Hence if one of such functions needs to be executed in order to perform
any operation on the Target's I/O system, it must be preceded with an
'@' sign, e.g.:
.CE
-> @ls "/sd0"
.CE
will list the directory of a disk named "/sd0" on the target, wile
.CS
-> ls "/tmp"
.CE
will list the contents of the "/tmp" directory on the host.

The target I/O system and the Tornado Shell running on the host, each
have their own notion of current directory, which are not related,
hence
.CS
-> pwd
.CE
will display the Tornado Shell current directory on the host file
system, while
.CS
-> @pwd
.CE
will display the target's current directory on the target's console.

WILDCARDS
Some of the functions herein support wildcard characters in argument
strings where file or directory names are expected. The wildcards are
limited to "*" which matches zero or more characters and "?" which
matches any single characters. Files or directories with names beginning
with a "." are not normally matched with the "*" wildcard.

DIRECTORY LISTING
Directory listing is implemented in one function dirList(), which can be
accessed using one of these four front-end functions:

.iP ls()
produces a short list of files
.iP lsr()
is like ls() but ascends into subdirectories
.iP ll()
produces a detailed list of files, with file size, modification date
attributes etc.
.iP llr()
is like ll() but also ascends into subdirectories

All of the directory listing functions accept a name of a directory or a
single file to list, or a name which contain wildcards, which will
result in listing of all objects that match the wildcard string
provided.

SEE ALSO
ioLib, dosFsLib, netDrv, nfsLib,
.pG "Target Shell"
.pG "Tornado Users's Guide"
*/

/* includes */

#include <vxWorks.h>
#include <fioLib.h>
#include "ctype.h"
#include "stdio.h"
#include "ioLib.h"
#include "memLib.h"
#include "string.h"
#include "ftpLib.h"
#include "usrLib.h"
#include "dirent.h"
#include "sys/stat.h"
#include "errnoLib.h"
#include "iosLib.h"
#include "taskLib.h"
#include "logLib.h"
#include "netDrv.h"
#include "time.h"
#include "nfsLib.h"
#include "pathLib.h"
#include "private/dosFsVerP.h"
#include "dosFsLib.h"

#include "stat.h"
#include "utime.h"

/* types */

LOCAL int mvFile (const char *oldname, const char *newname);

/* defines */

/******************************************************************************
*
* usrPathCat - concatenate directory path to filename.
*
* This routine constructs a valid filename from a directory path
* and a filename.
* The resultant path name is put into <result>.
* No arcane rules are used to derive the concatenated result.
*
* RETURNS: N/A.
*/
LOCAL void usrPathCat
    (
    const char *	dirName,
    const char *	fileName,
    char *		result
    )
    {
    *result = EOS ;

    if(dirName[0] != EOS && dirName != NULL && strcmp(dirName,".") != 0)
	{
	strcpy( result, dirName );
	strcat( result,  "/" );
	}

    strcat( result, fileName );
    } /* usrPathCat() */

/*******************************************************************************
*
* cd - change the default directory
*
* .iP NOTE
* This is a target resident function, which manipulates the target I/O
* system. It must be preceded with the
* '@' letter if executed from the Tornado Shell (windsh), which has a
* built-in command of the same name that operates on the Host's I/O
* system.
*
* This command sets the default directory to <name>.  The default directory
* is a device name, optionally followed by a directory local to that
* device.
*
* To change to a different directory, specify one of the following:
* .iP "" 4
* an entire path name with a device name, possibly followed by a directory
* name.  The entire path name will be changed.
* .iP
* a directory name starting with a `~' or `/' or `$'.  The directory part
* of the path, immediately after the device name, will be replaced with the new
* directory name.
* .iP
* a directory name to be appended to the current default directory.
* The directory name will be appended to the current default directory.
* .LP
*
* An instance of ".." indicates one level up in the directory tree.
*
* Note that when accessing a remote file system via RSH or FTP, the
* VxWorks network device must already have been created using
* netDevCreate().
*
* WARNING
* The cd() command does very little checking that <name> represents a valid
* path.  If the path is invalid, cd() may return OK, but subsequent
* calls that depend on the default path will fail.
*
* EXAMPLES
* The following example changes the directory to device `/fd0/':
* .CS
*     -> cd "/fd0/"
* .CE
* This example changes the directory to device `wrs:' with the local
* directory `~leslie/target':
* .CS
*     -> cd "wrs:~leslie/target"
* .CE
* After the previous command, the following changes the directory to
* `wrs:~leslie/target/config':
* .CS
*     -> cd "config"
* .CE
* After the previous command, the following changes the directory to
* `wrs:~leslie/target/demo':
* .CS
*     -> cd "../demo"
* .CE
* After the previous command, the following changes the directory to
* `wrs:/etc'.
* .CS
*     -> cd "/etc"
* .CE
* Note that `~' can be used only on network devices (RSH or FTP).
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: pwd(),
* .pG "Target Shell"
*/

STATUS cd
    (
    const char *	name		/* new directory name */
    )
    {
    if (ioDefPathCat ((char *)name) != OK)
	{
	printf ("cd: error = %#x.\n", errno);
	return (ERROR);
	}

    return (OK);
    }

/******************************************************************************
*
* dirNameWildcard - check if file or dir name contains wildcards
*
* RETURNS: TRUE if "*" or "?" are contained in the <name> string
*/

LOCAL BOOL dirNameWildcard
    (
    const char *	name
    )
    {
    if (index(name, '*') != NULL ||
	index(name, '?') != NULL)
	return TRUE ;
    else
	return FALSE ;
    }


/******************************************************************************
*
* nameIsDir - check if name is a directory
*
* RETURNS: TRUE if the name is an existing directory
*/

LOCAL BOOL nameIsDir
    (
    const char *	name
    )
    {
    struct stat fStat ;

    if ((name == NULL) || (*name == EOS))
	return FALSE ;

    /* if it does not exist, it ain't a directory */

    if (stat ((char *) name, &fStat) == ERROR)
        {
	errno = OK ;
    	return FALSE;
        }

    if (S_ISDIR (fStat.st_mode))
        {
        return TRUE;
        }

    return FALSE ;
    }


/*******************************************************************************
*
* pwd - print the current default directory
*
* This command displays the current working device/directory.
*
* .iP NOTE
* This is a target resident function, which manipulates the target I/O
* system. It must be preceded with the
* '@' letter if executed from the Tornado Shell (windsh), which has a
* built-in command of the same name that operates on the Host's I/O
* system.
*
* RETURNS: N/A
*
* SEE ALSO: cd(),
* .pG "Target Shell,"
* windsh,
* .tG "Shell"
*/

void pwd (void)
    {
    char name [MAX_FILENAME_LENGTH];

    ioDefPathGet (name);
    printf ("%s\n", name);
    }


/*******************************************************************************
*
* mkdir - make a directory
*
* This command creates a new directory in a hierarchical file system.
* The <dirName> string specifies the name to be used for the
* new directory, and can be either a full or relative pathname.
*
* This call is supported by the VxWorks NFS and dosFs file systems.
*
* RETURNS: OK, or ERROR if the directory cannot be created.
*
* SEE ALSO: rmdir(),
* .pG "Target Shell"
*/

STATUS mkdir
    (
    const char *	dirName		/* directory name */
    )
    {
    int fd;

    if ((fd = open (dirName, O_RDWR | O_CREAT, FSTAT_DIR | DEFAULT_DIR_PERM))
	 == ERROR)
	{
	return (ERROR);
	}

    return (close (fd));
    }

/*******************************************************************************
*
* rmdir - remove a directory
*
* This command removes an existing directory from a hierarchical file
* system.  The <dirName> string specifies the name of the directory to
* be removed, and may be either a full or relative pathname.
*
* This call is supported by the VxWorks NFS and dosFs file systems.
*
* RETURNS: OK, or ERROR if the directory cannot be removed.
*
* SEE ALSO: mkdir(),
* .pG "Target Shell"
*/

STATUS rmdir
    (
    const char *	dirName		/* name of directory to remove */
    )
    {
    return (remove (dirName));
    }

/*******************************************************************************
*
* rm - remove a file
*
* This command is provided for UNIX similarity.  It simply calls remove().
*
* RETURNS: OK, or ERROR if the file cannot be removed.
*
* SEE ALSO: remove(),
* .pG "Target Shell"
*/

STATUS rm
    (
    const char *	fileName	/* name of file to remove */
    )
    {
    return (remove (fileName));
    }


/*******************************************************************************
*
* copyStreams - copy from/to specified streams
*
* This command copies from the stream identified by <inFd> to the stream
* identified by <outFd> until an end of file is reached in <inFd>.
* This command is used by copy().
*
* INTERNAL
* The size of an array buffer can have a dramatic impact on the throughput
* achieved by the copyStreams() routine.  The default is 1 Kbyte, but this
* can be increased as long as the calling task (usually the VxWorks shell)
* has ample stack space.  Alternately, copyStreams() can be modified to use a
* static buffer, as long as the routine is guaranteed not to be called in
* the context of more than one task simultaneously.
*
* RETURNS: OK, or ERROR if there is an error reading from <inFd> or writing
* to <outFd>.
*
* SEE ALSO: copy(),
* .pG "Target Shell"
*
* INTERNAL
* Note use of printErr since printf's would probably go to the output stream
* outFd!
*/

STATUS copyStreams
    (
    int inFd,		/* file descriptor of stream to copy from */
    int outFd 		/* file descriptor of stream to copy to */
    )
    {
    char * buffer;
    int totalBytes = 0;
    int nbytes;
    size_t	dSize;

    /* update file size */

    if (ioctl( inFd, FIONREAD, (int)&dSize ) == ERROR)
    	dSize = 1024;

    /* transferring buffer */

    dSize = min( 0x10000, dSize );
    buffer = KHEAP_ALLOC( dSize );
    if (buffer == NULL)
    	return ERROR;

    while ((nbytes = fioRead (inFd, buffer, dSize)) > 0)
	{
	if (write (outFd, buffer, nbytes) != nbytes)
	    {
	    printErr ("copy: error writing file. errno %p\n", (void *)errno);
     	    KHEAP_FREE( buffer );
	    return (ERROR);
	    }

	totalBytes += nbytes;
    	}

    KHEAP_FREE( buffer );

    if (nbytes < 0)
	{
	printErr ("copy: error reading file after copying %d bytes.\n",
		 totalBytes);
	return (ERROR);
	}

    printf("Copy OK: %u bytes copied\n", totalBytes );
    return (OK);
    }

/*******************************************************************************
*
* copy - copy <in> (or stdin) to <out> (or stdout)
*
* This command copies from the input file to the output file, until an
* end-of-file is reached.
*
* EXAMPLES:
* The following example displays the file `dog', found on the default file
* device:
* .CS
*     -> copy <dog
* .CE
* This example copies from the console to the file `dog', on device `/ct0/',
* until an EOF (default ^D) is typed:
* .CS
*     -> copy >/ct0/dog
* .CE
* This example copies the file `dog', found on the default file device, to
* device `/ct0/':
* .CS
*     -> copy <dog >/ct0/dog
* .CE
* This example makes a conventional copy from the file named `file1' to the file
* named `file2':
* .CS
*     -> copy "file1", "file2"
* .CE
* Remember that standard input and output are global; therefore, spawning
* the first three constructs will not work as expected.
*
* RETURNS:
* OK, or
* ERROR if <in> or <out> cannot be opened/created, or if there is an
* error copying from <in> to <out>.
*
* SEE ALSO: copyStreams(), tyEOFSet(), cp(), xcopy()
* .pG "Target Shell"
*/

STATUS copy
    (
    const char *in,	/* name of file to read  (if NULL assume stdin)  */
    const char *out 	/* name of file to write (if NULL assume stdout) */
    )
    {
    int inFd = ERROR;
    int outFd = ERROR;
    int status;
    struct stat    fileStat; /* structure to fill with data */
    struct utimbuf fileTime;

    /* open input file */

    inFd = in ? open (in, O_RDONLY, 0) : STD_IN;
    if (inFd == ERROR)
	{
	printErr ("Can't open input file \"%s\" errno = %p\n",
		  in, (void *)errno );
	return (ERROR);
	}
   

    /* Is this a directory or a file, can't write to a directory! */
    if (nameIsDir(out)) 
        {
        errnoSet(S_ioLib_CANT_OVERWRITE_DIR);
        close(inFd);
        return(ERROR);
        }

    /* creat output file */
    outFd = out ? creat (out, O_WRONLY) : STD_OUT;

    if (outFd  == ERROR)
	{
	printErr ("Can't write to \"%s\", errno %p\n", out, (void *)errno);

    	if (in)
    	    close( inFd );

	return (ERROR);
	}

    /* copy data */

    status = copyStreams (inFd, outFd);

    ioctl (inFd, FIOFSTATGET, (int) &fileStat);
    fileTime.actime = fileStat.st_atime;
    fileTime.modtime = fileStat.st_mtime;
    ioctl (outFd, FIOTIMESET, (int) &fileTime);

    if (in)
	close (inFd);

    /* close could cause buffers flushing */

    if (out && close (outFd) == ERROR)
	status = ERROR;

    return (status);
    }

/********************************************************************************
* chkdsk - perform consistency checking on a MS-DOS file system
*
* This function invokes the integral consistency checking built
* into the dosFsLib file system, via FIOCHKDSK ioctl.
* During the test, the file system will be blocked from application code
* access, and will emit messages describing any inconsistencies found on
* the disk, as well as some statistics, depending on the value of the
* <verbose> argument.
* Depending the value of <repairLevel>, the inconsistencies will be
* repaired, and changes written to disk.
*
* These are the values for <repairLevel>:
* .iP 0
* Same as DOS_CHK_ONLY (1)
* .IP "DOS_CHK_ONLY (1)"
* Only report errors, do not modify disk.
* .IP "DOS_CHK_REPAIR (2)"
* Repair any errors found.
*
* These are the values for <verbose>:
* .iP 0
* similar to DOS_CHK_VERB_1
* .iP "DOS_CHK_VERB_SILENT (0xff00)"
* Do not emit any messages, except errors encountered.
* .iP "DOS_CHK_VERB_1 (0x0100)"
* Display some volume statistics when done testing, as well
* as errors encountered during the test.
* .iP "DOS_CHK_VERB_2 (0x0200)"
* In addition to the above option, display path of every file, while it
* is being checked. This option may significantly slow down
* the test process.
*
* Note that the consistency check procedure will
* .I unmount
* the file system, meaning the all currently open file descriptors will
* be deemed unusable.
*
* RETURNS: OK or ERROR if device can not be checked or could not be repaired.
*
*/
STATUS chkdsk
    (
    const char *	pDevName,	/* device name */
    u_int		repairLevel,	/* how to fix errors */
    u_int		verbose		/* verbosity level */
    )
    {
    int fd = open (pDevName, O_RDONLY, 0);
    STATUS	retStat;

    if (fd == ERROR)
    	{
    	perror (pDevName);
    	return ERROR;
    	}

    repairLevel &= 0x0f;

    if ((verbose & 0xff00) != 0)
    	verbose	 &= 0xff00;
    else if ((verbose & 0xff) != 0)
    	verbose	 <<= 8;
    else	/* default is 1 */
    	verbose	 = 0x0100;
  
    retStat = ioctl (fd, FIOCHKDSK, repairLevel | verbose);
    close (fd);
    return retStat;
    } /* chkDsk() */


/*******************************************************************************
*
* dirListPattern - match file name pattern with an actual file name
*
* <pat> is a pattern consisting with '?' and '*' wildcard characters,
* which is matched against the <name> filename, and TRUE is returned
* if they match, or FALSE if do not match. Both arguments must be
* null terminated strings.
* The pattern matching is case insensitive.
*
* NOTE: we're using EOS as integral part of the pattern and name.
*/

LOCAL BOOL dirListPattern
    (
    char * pat,
    char * name
    )
    {
    FAST char * pPat;
    FAST char *	pNext;
    const char	anyOne	= '?';
    const char	anyMany	= '*';

    pPat = pat ;
    for (pNext = name ; * pNext != EOS ; pNext ++)
	{
	/* DEBUG-  logMsg("pattern %s, name %s\n", pPat, pNext, 0,0,0,0);*/
	if (*pPat == anyOne)
	    {
	    pPat ++ ;
	    }
	else if (*pPat == anyMany)
	    {
	    if (pNext[0] == '.')	/* '*' dont match . .. and .xxx */
		return FALSE ;
	    if (toupper(pPat[1]) == toupper(pNext[1]))
		pPat ++ ;
	    else if (toupper(pPat[1]) == toupper(pNext[0]))
		pPat += 2 ;
	    else
		continue ;
	    }
	else if (toupper(*pPat) != toupper(*pNext))
	    {
	    return FALSE ;
	    }
	else
	    {
	    pPat ++ ;
	    }
	}
    /* loop is done, let's see if there is anything left */
    if ((*pPat == EOS) || (pPat[0] == anyMany && pPat[1] == EOS))
	return TRUE ;
    else
	return FALSE ;
    }

/*******************************************************************************
*
* dirListEnt - list one file or directory
*
* List one particular file or directory entry.
*
* NOMANUAL
*/
LOCAL STATUS dirListEnt
    (
    int		fd,		/* file descriptor for output */
    char *	fileName,	/* file name */
    struct stat * fileStat,	/* stat() structure */
    char *	prefix,		/* prefix for short output */
    BOOL	doLong		/* do Long listing format */
    )
    {
    time_t		now;		/* current clock */
    struct tm		nowDate;	/* current date & time */
    struct tm		fileDate;	/* file date/time      (long listing) */
    const  char 	*monthNames[] = {"???", "Jan", "Feb", "Mar", "Apr",
				    	 "May", "Jun", "Jul", "Aug", "Sep",
				    	 "Oct", "Nov", "Dec"};
    char fType, modbits[10] ;
    char suffix = ' ';	/* print '/' after directory name */

    if(doLong)
	{
	/* Break down file modified time */
	time( &now );
	localtime_r (&now, &nowDate);
	localtime_r (&fileStat->st_mtime, &fileDate);

	if (fileStat->st_attrib & DOS_ATTR_RDONLY)
	    fileStat->st_mode &= ~(S_IWUSR|S_IWGRP|S_IWOTH);

	if (S_ISDIR (fileStat->st_mode))
    	    {
	    fType = 'd' ;
    	    suffix = '/';
    	    }
	else if (S_ISREG (fileStat->st_mode))
	    fType = '-' ;
	else
	    fType = '?' ;

	strcpy( modbits, "---------" );

	modbits[0] = (fileStat->st_mode & S_IRUSR)? 'r':'-';
	modbits[1] = (fileStat->st_mode & S_IWUSR)? 'w':'-';
	modbits[2] = (fileStat->st_mode & S_IXUSR)? 'x':'-';
	modbits[3] = (fileStat->st_mode & S_IRGRP)? 'r':'-';
	modbits[4] = (fileStat->st_mode & S_IWGRP)? 'w':'-';
	modbits[5] = (fileStat->st_mode & S_IXGRP)? 'x':'-';
	modbits[6] = (fileStat->st_mode & S_IROTH)? 'r':'-';
	modbits[7] = (fileStat->st_mode & S_IWOTH)? 'w':'-';
	modbits[8] = (fileStat->st_mode & S_IXOTH)? 'x':'-';

	if (fileStat->st_attrib & DOS_ATTR_ARCHIVE)
	    modbits[6] = 'A';
	if(fileStat->st_attrib & DOS_ATTR_SYSTEM)
	    modbits[7] = 'S';
	if(fileStat->st_attrib & DOS_ATTR_HIDDEN)
	    modbits[8] = 'H';

	modbits[9] = EOS ;

	/* print file mode */
	fdprintf(fd, "%c%9s", fType, modbits);

	/* fake links, user and group fields */
	fdprintf(fd, " %2d %-7d %-7d ", fileStat->st_nlink,
		fileStat->st_uid, fileStat->st_gid );

	/* print file size - XXX: add 64 bit file size support */
	fdprintf(fd, " %9lu ", fileStat->st_size );

	/* print date */
	if (fileDate.tm_year == nowDate.tm_year)
	    {
	    fdprintf(fd, "%s %2d %02d:%02d ",
		    monthNames [fileDate.tm_mon + 1],/* month */
		    fileDate.tm_mday,               /* day of month */
		    fileDate.tm_hour,		/* hour */
		    fileDate.tm_min		/* minute */
		    );
	    }
	else
	    {
	    fdprintf(fd, "%s %2d  %04d ",
		    monthNames [fileDate.tm_mon + 1],/* month */
		    fileDate.tm_mday,		/* day of month */
		    fileDate.tm_year+1900	/* year */
		    );
	    }

	} /* if doLong */
    else
	{ /* short listing */
	if (strcmp(prefix , ".") == 0 || prefix[0] == EOS)
	    /* dint print "." prefix */ ;
	else if (prefix != NULL &&  prefix [ strlen(prefix) -1 ] != '/')
	    fdprintf(fd, "%s/", prefix);
	else if (prefix != NULL)
	    fdprintf(fd, prefix);
	}

    /* last, print file name */
    if (fdprintf(fd, "%s%c\n", fileName, suffix ) == ERROR)
	return ERROR;

    return OK ;
    }

/*******************************************************************************
*
* dirList - list contents of a directory (multi-purpose)
*
* This command is similar to UNIX ls.  It lists the contents of a directory
* in one of two formats.  If <doLong> is FALSE, only the names of the files
* (or subdirectories) in the specified directory are displayed.  If <doLong>
* is TRUE, then the file name, size, date, and time are displayed.
* If <doTree> flag is TRUE, then each subdirectory encountered
* will be listed as well (i.e. the listing will be recursive).
*
* The <dirName> parameter specifies the directory to be listed.
* If <dirName> is omitted or NULL, the current working directory will be
* listed. <dirName> may contain wildcard characters to list some
* of the directory's contents.
*
* LIMITATIONS
* .iP -
* With dosFsLib file systems, MS-DOS volume label entries are not reported.
* .iP -
* Although an output format very similar to UNIX "ls" is employed,
* some information items have no particular meaning on some
* file systems.
* .iP -
* Some file systems which do not support the POSIX compliant dirLib()
* interface, can not support the <doLong> and <doTree> options.
*
* RETURNS:  OK or ERROR.
*
* SEE ALSO: dirLib, dosFsLib, ls(), ll(), lsr(), llr()
*/

STATUS dirList
    (
    int         fd,             /* file descriptor to write on */
    char *      dirName,        /* name of the directory to be listed */
    BOOL        doLong,         /* if TRUE, do long listing */
    BOOL	doTree		/* if TRUE, recurse into subdirs */
    )
    {
    FAST STATUS		status;		/* return status */
    FAST DIR		*pDir;		/* ptr to directory descriptor */
    FAST struct dirent	*pDirEnt;	/* ptr to dirent */
    struct stat		fileStat;	/* file status info    (long listing) */
    char		*pPattern;	/* wildcard pattern */
    char 		fileName [MAX_FILENAME_LENGTH];
					/* buffer for building file name */

    /* If no directory name specified, use "." */

    if (dirName == NULL)
	dirName = ".";

    /* try to do a netDrv listing first, hacky way to know its a local FS */
    if (_func_netLsByName != NULL)
	{
	if ((*_func_netLsByName) (dirName) == OK)
	    return (OK);
	else if (errno != S_netDrv_UNKNOWN_REQUEST)
	    return (ERROR);
	}

    pDir = NULL ; pPattern = NULL ;

    /* Open dir */

    pDir = opendir (dirName) ;

    /* could not opendir: last component may be a wildcard  pattern */
    if (pDir == NULL)
	{
	pPattern = rindex(dirName, '/');
	if ( pPattern != NULL && pPattern != dirName &&
		dirNameWildcard(pPattern))
	    {
	    /* dir and pattern e.g. dir1/a*.c */
	    *pPattern++ = EOS ;
	    pDir = opendir (dirName) ;
	    }
	else if (dirNameWildcard( dirName ))
	    {
	    /* just pattern e.g. *.c or abc.? */
	    pPattern = dirName ;
	    dirName = "." ;
	    pDir = opendir (dirName) ;
	    }
 	}

     /* count not opendir: no can do ! */
     if (pDir == NULL)
	goto _nodir ;

    status = OK;

    /* print directory name as header */
    if (dirName == ".")
	dirName = "" ;
    else if (doLong)
	fdprintf(fd,"\nListing Directory %s:\n", dirName);

    do
	{
	errno = OK;

    	pDirEnt = readdir (pDir);

	if (pDirEnt != NULL)
	    {
	    if (pPattern != NULL &&
		dirListPattern( pPattern, pDirEnt->d_name) == FALSE)
		continue ;

	    if (doLong)				/* if doing long listing */
		{
		/* Construct path/filename for stat */
		usrPathCat( dirName, pDirEnt->d_name, fileName );

		/* Get and print file status info */
		if (stat (fileName, &fileStat) != OK)
		    {
		    /* stat() error, mark the file questionable and continue */
		    bzero( (caddr_t) &fileStat, sizeof(fileStat));
		    }
		}

	    if (dirListEnt(fd, pDirEnt->d_name,
			&fileStat, dirName, doLong ) == ERROR)
		status = ERROR;
	    }
	else					/* readdir returned NULL */
	    {
	    if (errno != OK)			/* if real error, not dir end */
		{
		fdprintf (fd, "error reading dir %s, errno: %x\n",
			dirName, errno) ;
		status = ERROR;
		}
	    }

	} while (pDirEnt != NULL);		/* until end of dir */

    /* Close dir */
    status |= closedir (pDir);
    if (! doTree)
	return (status);

    /* do recursion into each subdir AFTER all files and subdir are listed */
    if (dirName[0] == EOS)
	pDir = opendir (".") ;
    else
	pDir = opendir (dirName) ;

    if (pDir == NULL)
	goto _nodir ;

    do
	{
	errno = OK;
    	pDirEnt = readdir (pDir);

	if (pDirEnt != NULL)
	    {
	    if (pPattern != NULL &&
		dirListPattern( pPattern, pDirEnt->d_name) == FALSE)
		continue ;
	   
	    /* Construct path/filename for stat */
	    usrPathCat( dirName, pDirEnt->d_name, fileName );

	    /* Get and print file status info */
	    if (stat (fileName, &fileStat) != OK)
		{
		/* stat() error, mark the file questionable and continue */
		bzero( (caddr_t) &fileStat, sizeof(fileStat));
		}
	    /* recurse into subdir, but not "." or ".." */
	    if (S_ISDIR (fileStat.st_mode) &&
		strcmp(pDirEnt->d_name,"." ) &&
		strcmp(pDirEnt->d_name,".." ))
		{
		status = dirList( fd, fileName, doLong, doTree );
		/* maybe count files ?? */
		}
	    }
	else					/* readdir returned NULL */
	    {
	    if (errno != OK)			/* if real error, not dir end */
		{
		fdprintf (fd, "error reading dir %s, errno: %x\n",
			dirName, errno) ;
		status = ERROR;
		}
	    }
	} while (pDirEnt != NULL);		/* until end of dir */

    /* Close dir */
    status |= closedir (pDir);
    return (status);

_nodir:
    fdprintf (fd, "Can't open \"%s\".\n", dirName);
    return (ERROR);
    }

/*******************************************************************************
*
* ls - generate a brief listing of a directory
*
* This function is simply a front-end for dirList(), intended for
* brevity and backward compatibility. It produces a list of files
* and directories, without details such as file size and date,
* and without recursion into subdirectories.
*
* <dirName> is a name of a directory or file, and may contain wildcards.
* <doLong> is provided for backward compatibility.
*
* .iP NOTE
* This is a target resident function, which manipulates the target I/O
* system. It must be preceded with the
* '@' letter if executed from the Tornado Shell (windsh), which has a
* built-in command of the same name that operates on the Host's I/O
* system.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: dirList()
*/

STATUS ls
    (
    char	*dirName,	/* name of dir to list */
    BOOL	doLong		/* switch on details */
    )
    {
    return (dirList (STD_OUT, dirName, doLong, FALSE));
    }

/******************************************************************************
*
* ll - generate a long listing of directory contents
*
* This command causes a long listing of a directory's contents to be
* displayed.  It is equivalent to:
* .CS
*     -> dirList 1, dirName, TRUE, FALSE
* .CE
*
* <dirName> is a name of a directory or file, and may contain wildcards.
*
* .iP "NOTE 1:"
* This is a target resident function, which manipulates the target I/O
* system. It must be preceded with the
* '@' letter if executed from the Tornado Shell (windsh), which has a
* built-in command of the same name that operates on the Host's I/O
* system.
*
* .iP "NOTE 2:"
* When used with netDrv devices (FTP or RSH), ll() does not give
* directory information.  It is equivalent to an ls() call with no
* long-listing option.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: dirList()
*/
STATUS ll
    (   
    char *dirName               /* name of directory to list */
    )
    {
    return (dirList (STD_OUT, dirName, TRUE, FALSE));
    }

/*******************************************************************************
*
* lsr - list the contents of a directory and any of its subdirectories
*
* This function is simply a front-end for dirList(), intended for
* brevity and backward compatibility. It produces a list of files
* and directories, without details such as file size and date,
* with recursion into subdirectories.
*
* <dirName> is a name of a directory or file, and may contain wildcards.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: dirList()
*/

STATUS lsr
    (
    char	*dirName	/* name of dir to list */
    )
    {
    return (dirList( STD_OUT, dirName, FALSE, TRUE));
    }

/******************************************************************************
*
* llr - do a long listing of directory and all its subdirectories contents
*
* This command causes a long listing of a directory's contents to be
* displayed.  It is equivalent to:
* .CS
*     -> dirList 1, dirName, TRUE, TRUE
* .CE
 *
* <dirName> is a name of a directory or file, and may contain wildcards.
*
* NOTE: When used with netDrv devices (FTP or RSH), ll() does not give
* directory information.  It is equivalent to an ls() call with no
* long-listing option.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: dirList()
*/
STATUS llr
    (   
    char *dirName               /* name of directory to list */
    )
    {
    return (dirList (STD_OUT, dirName, TRUE, TRUE));
    }

/********************************************************************************
* cp - copy file into other file/directory.
*
* This command copies from the input file to the output file.
* If destination name is directory, a source file is copied into
* this directory, using the last element of the source file name
* to be the name of the destination file.
*
* This function is very similar to copy(), except it is somewhat
* more similar to the UNIX "cp" program in its handling of the
* destination.
*
* <src> may contain a wildcard pattern, in which case all files
* matching the pattern will be copied to the directory specified in
* <dest>.
* This function does not copy directories, and is not recursive.
* To copy entire subdirectories recursively, use xcopy().
*
* EXAMPLES
* .CS
* -> cp( "/sd0/FILE1.DAT","/sd0/dir2/f001.dat")
* -> cp( "/sd0/dir1/file88","/sd0/dir2")
* -> cp( "/sd0/@.tmp","/sd0/junkdir")
* .CE
*
* RETURNS: OK or ERROR if destination is not a directory while <src> is
* a wildcard pattern, or if any of the files could not be copied.
*
* SEE ALSO; xcopy()
*
*/
STATUS cp
    (
    const char *src,   /* source file or wildcard pattern */
    const char *dest   /* destination file name or directory */
    )
    {
    FAST DIR *pDir;		/* ptr to directory descriptor */
    FAST struct dirent	*pDirEnt;	/* ptr to dirent */
    STATUS status = OK ;
    char * pattern = NULL ;
    char * dirName = NULL ;
    char  dir[MAX_FILENAME_LENGTH] ;
    char  fileName[MAX_FILENAME_LENGTH] ;
    char  destName[MAX_FILENAME_LENGTH] ;

    if (src == NULL)
    	{
	errno = EINVAL ;
        return ERROR;
    	}

    if (dest == NULL)
	dest = "." ;

    /* dest does not exist or regular file */
    if (!nameIsDir (dest))
	{
	if (dirNameWildcard (src))
	    {
	    printErr("mv: destination \"%s\" not a directory\n", dest );
	    errno = ENOTDIR ;
	    return ERROR;
	    }
	printf("copying file %s -> %s\n", src, dest );
        return copy( src, dest );
	}

    strncpy( dir, src, MAX_FILENAME_LENGTH-1 );

    pattern = rindex(dir, '/');
    if ( pattern != NULL && pattern != dir && dirNameWildcard(pattern))
	{
	/* dir and pattern e.g. dir1/a*.c */
	*pattern++ = EOS ;
	dirName = dir ;
	}
    else if (dirNameWildcard (dir))
	{
	/* just pattern e.g. *.c or abc.? */
	pattern = dir;
	dirName = "." ;
	}
    else
	{
	pattern = NULL ;
	dirName = dir ;
	}

    if (pattern == NULL)
	{
	printf("copying file %s -> %s\n", src, dest );
	return copy(src, dest ) ;
	}

    pDir = opendir (dirName) ;

    if (pDir == NULL)
	{
	perror(dirName);
	return ERROR;
	}

    errno = OK;

    do
	{
    	pDirEnt = readdir (pDir);
	if (pDirEnt != NULL)
	    {
	    if (pattern != NULL &&
		dirListPattern( pattern, pDirEnt->d_name) == FALSE)
		continue ;

	    if (strcmp(pDirEnt->d_name,"." )  == 0 &&
		strcmp(pDirEnt->d_name,".." ) == 0)
		continue ;

	    /* Construct path/filename for stat */
	    usrPathCat( dirName, pDirEnt->d_name, fileName );
	    usrPathCat( dest, pDirEnt->d_name, destName );

	    if (nameIsDir( fileName ))
		{
		printf("skipping directory %s\n", fileName );
		continue;
		}

	    printf("copying file %s -> %s\n", fileName, destName );
	    status |= copy( fileName, destName );
	    }
	} while (pDirEnt != NULL);		/* until end of dir */

    status |= closedir (pDir);
    return status ;
    }/* cp() */

/********************************************************************************
* mv - mv file into other directory.
*
* This function is similar to rename() but behaves somewhat more
* like the UNIX program "mv", it will overwrite files.
*
* This command moves the <src> file or directory into
* a file which name is passed in the <dest> argument, if <dest> is
* a regular file or does not exist.
* If <dest> name is a directory, the source object is moved into
* this directory as with the same name,
* if <dest> is NULL, the current directory is assumed as the destination
* directory.
* <src> may be a single file name or a path containing a wildcard
* pattern, in which case all files or directories matching the pattern
* will be moved to <dest> which must be a directory in this case.
*
* EXAMPLES
* .CS
* -> mv( "/sd0/dir1","/sd0/dir2")
* -> mv( "/sd0/@.tmp","/sd0/junkdir")
* -> mv( "/sd0/FILE1.DAT","/sd0/dir2/f001.dat")
* .CE
*
* RETURNS: OK or error if any of the files or directories could not be
* moved, or if <src> is a pattern but the destination is not a
* directory.
*/
STATUS mv
    (   
    const char *src,   /* source file name or wildcard */
    const char *dest   /* destination name or directory */
    )
    {
    FAST DIR *pDir;		/* ptr to directory descriptor */
    FAST struct dirent	*pDirEnt;	/* ptr to dirent */
    STATUS status = OK ;
    char * pattern = NULL ;
    char * dirName = NULL ;
    char  dir[MAX_FILENAME_LENGTH] ;
    char  fileName[MAX_FILENAME_LENGTH] ;
    char  destName[MAX_FILENAME_LENGTH] ;

    if (src == NULL)
    	{
	errno = EINVAL ;
        return ERROR;
    	}

    if (dest == NULL)
	dest = "." ;

    /* dest does not exist or regular file */

    if (nameIsDir( dest ) == nameIsDir( src ))
	{
	if (dirNameWildcard( src))
	    {
	    printErr("mv: destination \"%s\" not a directory\n", dest );
	    errno = ENOTDIR ;
	    return ERROR;
	    }

	printf("moving file %s -> %s\n", src, dest );

        return mvFile ( src, dest );
	}

    strncpy( dir, src, MAX_FILENAME_LENGTH-1 );

    pattern = rindex(dir, '/');

    if ( pattern != NULL && pattern != dir && dirNameWildcard(pattern))
	{
	/* dir and pattern e.g. dir1/a*.c */
	*pattern++ = EOS ;
	dirName = dir ;
	}
    else if (dirNameWildcard( dir))
	{
	/* just pattern e.g. *.c or abc.? */
	pattern = dir;
	dirName = "." ;
	}
    else
	{
	pattern = NULL ;
	dirName = dir ;
	}

    if (pattern == NULL)
	{
	printf("moving file %s -> %s\n", src, dest );
	return (mvFile (src, dest ));
	}

    pDir = opendir (dirName) ;

    if (pDir == NULL)
	{
	perror(dirName);
	return ERROR;
	}

    errno = OK;

    do
	{
    	pDirEnt = readdir (pDir);
	if (pDirEnt != NULL)
	    {
	    if (pattern != NULL &&
		dirListPattern( pattern, pDirEnt->d_name) == FALSE)
		continue ;

	    if (strcmp(pDirEnt->d_name,"." )  == 0 &&
		strcmp(pDirEnt->d_name,".." ) == 0)
		continue ;

	    /* Construct path/filename for stat */
	    usrPathCat( dirName, pDirEnt->d_name, fileName );
	    usrPathCat( dest, pDirEnt->d_name, destName );

	    printf("moving file %s -> %s\n", fileName, destName );
	    status |= mvFile( fileName, destName );
	    }
	} while (pDirEnt != NULL);		/* until end of dir */

    status |= closedir (pDir);
    return status ;
    }  /* mv() */


/*******************************************************************************
*
* mvFile - move a file from one place to another.  
*
* This routine does some of the work of the mv() function.
*
* RETURNS: OK, or ERROR if the file could not be opened or moved.
*/

LOCAL int mvFile
    (
    const char *oldname,        /* name of file to move */
    const char *newname         /* name with which to move file */
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

    if (ERROR == (fd = open ((char *) oldname, O_RDONLY, 0)))
        return (ERROR);

    /* move it */

    status = ioctl (fd, FIOMOVE, (int) newname);

    close (fd);
    return (status);
    }

/******************************************************************************
*
* xcopy - copy a hierarchy of files with wildcards
*
* <source> is a string containing a name of a directory, or a wildcard
* or both which will cause this function to make a recursive copy of all
* files residing in that directory and matching the wildcard pattern into
* the <dest> directory, preserving the file names and subdirectories.
*
* CAVEAT
* This function may call itself in accordance with the depth of the
* source directory, and occupies approximately 800 bytes per stack
* frame, meaning that to accommodate the maximum depth of subdirectories
* which is 20, at least 16 Kbytes of stack space should be available to
* avoid stack overflow.
*
* RETURNS: OK or ERROR if any operation has failed.
*
* SEE ALSO: tarLib, checkStack(), cp()
*/
STATUS xcopy
    (
    const char *source,	/* source directory or wildcard name */
    const char *dest		/* destination directory */
    )
    {
    FAST DIR *pDir;		/* ptr to directory descriptor */
    FAST struct dirent	*pDirEnt;	/* ptr to dirent */
    STATUS status = OK ;
    char * pattern = NULL ;
    char * dirName = NULL ;
    char  dir[MAX_FILENAME_LENGTH] ;
    char  fileName[MAX_FILENAME_LENGTH] ;
    char  destName[MAX_FILENAME_LENGTH] ;

    if (!nameIsDir( dest ))
	{
	printErr("xcopy: error: destination \"%s\" is not a directory\n",
		dest );
	errno = ENOTDIR ;
	return ERROR ;
	}

    strncpy( dir, source, MAX_FILENAME_LENGTH-1 );

    pattern = rindex(dir, '/');
    if ( pattern != NULL && pattern != dir && dirNameWildcard(pattern))
	{
	/* dir and pattern e.g. dir1/a*.c */
	*pattern++ = EOS ;
	dirName = dir ;
	}
    else if (dirNameWildcard( dir))
	{
	/* just pattern e.g. *.c or abc.? */
	pattern = dir;
	dirName = "." ;
	}
    else
	{
	pattern = NULL ;
	dirName = dir ;
	}

    if (!nameIsDir (dirName))
	{
	printf("copying file %s -> %s\n", source, dest );
	return copy(source, dest ) ;
	}

    pDir = opendir (dirName) ;

    if (pDir == NULL)
	{
	perror(dirName);
	return ERROR;
	}

    errno = OK;

    do
	{
    	pDirEnt = readdir (pDir);
	if (pDirEnt != NULL)
	    {
	    if (pattern != NULL &&
		dirListPattern( pattern, pDirEnt->d_name) == FALSE)
		continue ;

	    /* Construct path/filename for stat */
	    usrPathCat( dirName, pDirEnt->d_name, fileName );
	    usrPathCat( dest, pDirEnt->d_name, destName );


	    if (!nameIsDir( fileName ))
		{
		printf("copying file %s -> %s\n", fileName, destName );
		status |= copy( fileName, destName );
		}
	    else if (strcmp(pDirEnt->d_name,"." ) &&
		strcmp(pDirEnt->d_name,".." ))
		{
		printf("copying dir %s -> %s\n", fileName, destName );
		mkdir(destName);
		status |= xcopy( fileName, destName );
		}
	    }
	} while (pDirEnt != NULL);		/* until end of dir */

    status |= closedir (pDir);
    return status ;
    }


/******************************************************************************
*
* xdelete - delete a hierarchy of files with wildcards
*
* <source> is a string containing a name of a directory, or a wildcard
* or both which will cause this function to recursively remove all
* files and subdirectories residing in that directory
* and matching the wildcard pattern.
* When a directory is encountered, all its contents are removed,
* and then the directory itself is deleted.
*
* CAVEAT
* This function may call itself in accordance with the depth of the
* source directory, and occupies approximately 520 bytes per stack
* frame, meaning that to accommodate the maximum depth of subdirectories
* which is 20, at least 10 Kbytes of stack space should be available to
* avoid stack overflow.
*
* RETURNS: OK or ERROR if any operation has failed.
*
* SEE ALSO: checkStack(), cp(), copy(), xcopy(), tarLib
*/
STATUS xdelete
    (
    const char *source	/* source directory or wildcard name */
    )
    {
    FAST DIR *pDir;		/* ptr to directory descriptor */
    FAST struct dirent	*pDirEnt;	/* ptr to dirent */
    STATUS status = OK ;
    char * pattern = NULL ;
    char * dirName = NULL ;
    char  dir[MAX_FILENAME_LENGTH] ;
    char  fileName[MAX_FILENAME_LENGTH] ;

    strncpy( dir, source, MAX_FILENAME_LENGTH-1 );

    pattern = rindex(dir, '/');
    if ( pattern != NULL && pattern != dir && dirNameWildcard(pattern))
	{
	/* dir and pattern e.g. dir1/a*.c */
	*pattern++ = EOS ;
	dirName = dir ;
	}
    else if (dirNameWildcard( dir))
	{
	/* just pattern e.g. *.c or abc.? */
	pattern = dir;
	dirName = "." ;
	}
    else
	{
	pattern = NULL ;
	dirName = dir ;
	}

    if (! nameIsDir( dirName ))
	{
	printf("deleting file %s\n", source);
	return unlink((char *)source);
	}

    pDir = opendir (dirName) ;

    if (pDir == NULL)
	{
	perror(dirName);
	return ERROR;
	}

    errno = OK;

    do
	{
    	pDirEnt = readdir (pDir);
	if (pDirEnt != NULL)
	    {
	    if (pattern != NULL &&
		dirListPattern( pattern, pDirEnt->d_name) == FALSE)
		continue ;

	    /* Construct path/filename for stat */
	    usrPathCat( dirName, pDirEnt->d_name, fileName );

	    if (!nameIsDir( fileName ))
		{
		printf("deleting file %s\n", fileName);
		status |= unlink( fileName);
		}
	    else if (strcmp(pDirEnt->d_name,"." ) &&
		strcmp(pDirEnt->d_name,".." ))
		{
		printf("deleting directory %s \n", fileName);
		status |= xdelete( fileName);
		status |= rmdir(fileName);
		}
	    }
	} while (pDirEnt != NULL);		/* until end of dir */

    status |= closedir (pDir);
    return status ;
    }

/******************************************************************************
*
* attrib - modify MS-DOS file attributes on a file or directory
*
* This function provides means for the user to modify the attributes
* of a single file or directory. There are four attribute flags which
* may be modified: "Archive", "System", "Hidden" and "Read-only".
* Among these flags, only "Read-only" has a meaning in VxWorks,
* namely, read-only files can not be modified deleted or renamed.
*
* The <attr> argument string may contain must start with either "+" or
* "-", meaning the attribute flags which will follow should be either set
* or cleared. After "+" or "-" any of these four letter will signify their
* respective attribute flags - "A", "S", "H" and "R".
*
* For example, to write-protect a particular file and flag that it is a
* system file:
*
* .CS
* -> attrib( "bootrom.sys", "+RS")
* .CE
*
* RETURNS: OK, or ERROR if the file can not be opened.
*/

STATUS attrib
    (
    const char * fileName,	/* file or dir name on which to change flags */
    const char * attr		/* flag settings to change */
    )
    {
    BOOL set = TRUE ;
    STATUS status ;
    u_char bit = 0 ;
    struct stat fileStat;
    int fd ;

    if (attr == NULL)
	{
	errno = EINVAL ;
	return ERROR;
	}

    fd = open (fileName, O_RDONLY, 0);

    if (fd == ERROR)
	{
	perror(fileName);
	return ERROR ;
	}

    if (fstat (fd, &fileStat) == ERROR)          /* get file status    */
	{
	printErr("Can't get stat on %s\n", fileName );
	return ERROR;
	}

    for ( ; *attr != EOS ; attr ++)
	{
	switch( *attr)
	    {
	    case '+' :
		set = TRUE ;
		break ;
	    case '-' :
		set = FALSE ;
		break ;
	    case 'A' : case 'a' :
		bit = DOS_ATTR_ARCHIVE ;
		break ;
	    case 'S' : case 's' :
		bit = DOS_ATTR_SYSTEM ;
		break ;
	    case 'H' : case 'h' :
		bit = DOS_ATTR_HIDDEN ;
		break ;
	    case 'R' : case 'r' :
		bit = DOS_ATTR_RDONLY ;
		break ;
	    default:
		printErr("Illegal attribute flag \"%c\", ignored\n", *attr );
	    } /* end of switch */
	if (set)
	    fileStat.st_attrib |= bit ;
	else
	    fileStat.st_attrib &= ~bit ;
	}

    status = ioctl (fd, FIOATTRIBSET, fileStat.st_attrib);

    close(fd);

    return status ;
    }

/******************************************************************************
*
* xattrib - modify MS-DOS file attributes of many files
*
* This function is essentially the same as attrib(), but it accepts
* wildcards in <fileName>, and traverses subdirectories in order
* to modify attributes of entire file hierarchies.
*
* The <attr> argument string may contain must start with either "+" or
* "-", meaning the attribute flags which will follow should be either set
* or cleared. After "+" or "-" any of these four letter will signify their
* respective attribute flags - "A", "S", "H" and "R".
*
* EXAMPLE
* .CS
* -> xattrib( "/sd0/sysfiles", "+RS")	/@ write protect "sysfiles" @/
* -> xattrib( "/sd0/logfiles", "-R")	/@ unprotect logfiles before deletion @/
* -> xdelete( "/sd0/logfiles")
* .CE
*
* CAVEAT
* This function may call itself in accordance with the depth of the
* source directory, and occupies approximately 520 bytes per stack
* frame, meaning that to accommodate the maximum depth of subdirectories
* which is 20, at least 10 Kbytes of stack space should be available to
* avoid stack overflow.
*
* RETURNS: OK, or ERROR if the file can not be opened.
*/
STATUS xattrib
    (
    const char *source,	/* file or directory name on which to change flags */
    const char *attr	/* flag settings to change */
    )
    {
    FAST DIR *pDir;		/* ptr to directory descriptor */
    FAST struct dirent	*pDirEnt;	/* ptr to dirent */
    STATUS status = OK ;
    char * pattern = NULL ;
    char * dirName = NULL ;
    char  dir[MAX_FILENAME_LENGTH] ;
    char  fileName[MAX_FILENAME_LENGTH] ;

    strncpy (dir, source, MAX_FILENAME_LENGTH-1 );

    pattern = rindex(dir, '/');
    if ( pattern != NULL && pattern != dir && dirNameWildcard(pattern))
	{
	/* dir and pattern e.g. dir1/a*.c */
	*pattern++ = EOS ;
	dirName = dir ;
	}
    else if (dirNameWildcard( dir))
	{
	/* just pattern e.g. *.c or abc.? */
	pattern = dir;
	dirName = "." ;
	}
    else
	{
	pattern = NULL ;
	dirName = dir ;
	}

    if (!nameIsDir (dirName))
	{
	printf("changing attributes on %s\n", source);
	return attrib(source, attr);
	}

    pDir = opendir (dirName);

    if (pDir == NULL)
	{
	perror(dirName);
	return ERROR;
	}

    errno = OK;

    do
	{
    	pDirEnt = readdir (pDir);
	if (pDirEnt != NULL)
	    {
	    if (pattern != NULL &&
		dirListPattern( pattern, pDirEnt->d_name) == FALSE)
		continue ;

	    /* Construct path/filename for stat */
	    usrPathCat( dirName, pDirEnt->d_name, fileName );

	    if (!nameIsDir( fileName ))
		{
		printf("changing attributes on %s\n", fileName);
		status |= attrib( fileName, attr);
		}
	    else if (strcmp(pDirEnt->d_name,"." ) &&
		strcmp(pDirEnt->d_name,".." ))
		{
		printf("traversing directory %s to change attributes \n",
			fileName);
		status |= xattrib( fileName, attr);
		status |= attrib( fileName, attr);
		}
	    }
	} while (pDirEnt != NULL);		/* until end of dir */

    status |= closedir (pDir);
    return status ;
    }

/*******************************************************************************
*
* diskFormat - format a disk
*
* This command formats a disk and creates a file system on it.  The
* device must already have been created by the device driver and
* initialized for use with a particular file system, via dosFsDevInit().
*
* This command calls ioctl() to perform the FIODISKFORMAT function.
*
* EXAMPLE
* .CS
*     -> diskFormat "/fd0/"
* .CE
*
* RETURNS:
* OK, or ERROR if the device cannot be opened or formatted.
*
* SEE ALSO: dosFsLib
* .pG "Target Shell"
*/

STATUS diskFormat
    (
    const char *pDevName 	/* name of the device to initialize */
    )
    {
    FAST int fd;
    FAST const char *name;

    /* If no directory name specified, use current working directory (".") */

    name = (pDevName == NULL) ? "." : pDevName;

    /* Open the device, format it, and initialize it. */

    if ((fd = open (name, O_WRONLY, 0)) == ERROR)
	{
	printErr ("Couldn't open \"%s\".\n", name);
	return (ERROR);
	}

    printf ("\"%s\" formatting... ", name);
    if (ioctl (fd, FIODISKFORMAT, 0) != OK)
	{
	printErr ("Couldn't format \"%s\".\n", name);
	close (fd);
	return (ERROR);
	}

    printf ("\"%s\" formatted... ", name);

    if (ioctl (fd, FIODISKINIT, 0) < OK)
	{
	printErr ("Couldn't initialize file system on \"%s\".\n", name);
	close (fd);
	return (ERROR);
	}
    close (fd);
    printf ("\"%s\" initialized.\n", name);
    return (OK);
    }

/*******************************************************************************
*
* diskInit - initialize a file system on a block device
*
* This function is now obsolete, use of dosFsVolFormat() is recommended.
*
* This command creates a new, blank file system on a block device.  The
* device must already have been created by the device driver and
* initialized for use with a particular file system, via dosFsDevCreate().
*
* EXAMPLE
* .CS
*     -> diskInit "/fd0/"
* .CE
*
* Note that if the disk is unformatted, it can not be mounted,
* thus open() will return error, in which case use the dosFsVolFormat
* routine manually.
*
* This routine performs the FIODISKINIT ioctl operation.
*
* RETURNS:
* OK, or
* ERROR if the device cannot be opened or initialized.
*
* SEE ALSO: dosFsLib
* .pG "Target Shell"
*/

STATUS diskInit
    (
    const char *pDevName	/* name of the device to initialize */
    )
    {
    FAST int fd;
    char name[MAX_FILENAME_LENGTH+1];

    /* If no directory name specified, use current working directory (".") */

    if (pDevName == NULL)
	ioDefPathGet (name);
    else
	strncpy (name, pDevName, MAX_FILENAME_LENGTH);

    /* Open the device, format it, and initialize it. */

    fd = open (name, O_WRONLY, 0) ;

    if (ERROR == fd)
	{
	printErr ("Couldn't open file system on \"%s\".\n", name);
	printErr ("Perhaps a dosFsVolFormat on the file system is needed.\n");
	return (ERROR);
	}

    if (ioctl (fd, FIODISKINIT, 0) < OK)
	{
	printErr ("Couldn't initialize file system on \"%s\".\n", name);
	close (fd);
	return (ERROR);
	}

    printf ("\"%s\" initialized.\n", name);
    close (fd);
    return (OK);
    }

/*******************************************************************************
*
* ioHelp - print a synopsis of I/O utility functions
*
* This function prints out synopsis for the I/O and File System
* utility functions.
*
* RETURNS: N/A
*
* SEE ALSO:
* .pG "Target Shell"
*/

void ioHelp (void)
    {
    static char *help_msg [] = {
    "cd        \"path\"             Set current working path",
    "pwd                            Print working path",
    "ls        [\"wpat\"[,long]]    List contents of directory",
    "ll        [\"wpat\"]           List contents of directory - long format",
    "lsr       [\"wpat\"[,long]]    Recursive list of directory contents",
    "llr       [\"wpat\"]           Recursive detailed list of directory",
    "rename    \"old\",\"new\"      Change name of file",
    "copy      [\"in\"][,\"out\"]   Copy in file to out file (0 = std in/out)",
    "cp        \"wpat\",\"dst\"      Copy many files to another dir",
    "xcopy     \"wpat\",\"dst\"      Recursively copy files",
    "mv        \"wpat\",\"dst\"      Move files into another directory",
    "xdelete   \"wpat\"             Delete a file, wildcard list or tree",
    "attrib    \"path\",\"attr\"    Modify file attributes",
    "xattrib   \"wpat\",\"attr\"    Recursively modify file attributes",
    "chkdsk    \"device\", L, V     Consistency check of file system",
    "diskInit  \"device\"           Initialize file system on disk",
    "diskFormat \"device\"          Low level and file system disk format",
    "",
    "\"attr\" contains one or more of: \" + - A H S R\" characters",
    "\"wpat\" may be name of a file, directory or wildcard pattern",
    " in which case \"dst\" must be a directory name",
    "chkdsk() params: L=0, check only, L=2, check and fix, V=0x200 verbose",
    NULL
    };
    FAST int ix;
    char ch;

    printf ("\n");
    for (ix = 0; help_msg [ix] != NULL; ix++)
	{
	if ((ix+1) % 20 == 0)
	    {
	    printf ("\nType <CR> to continue, Q<CR> to stop: ");
	    fioRdString (STD_IN, &ch, 1);
	    if (ch == 'q' || ch == 'Q')
		break;
	    else
		printf ("\n");
	    }
	printf ("%s\n", help_msg [ix]);
	}
    printf ("\n");
    }
/* End of file */


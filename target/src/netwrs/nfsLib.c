/* nfsLib.c - Network File System (NFS) library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03k,10may02,kbw  making man page edits
03j,06nov01,vvv  made max path length configurable (SPR #63551)
03i,15oct01,rae  merge from truestack ver 03m, base 03h (cleanup)
03h,15mar99,c_c  Doc: updated nfsAuthUnixSet manual (SPR #20867).
03g,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
03f,20nov97,gnn  fix for spr#5436, need to support FIOFSTATFSGET
03e,30may97,sgv  fix for spr#8653, Check for NULL parameter in nfsExportShow
03d,06aug96,sgv  fix for spr#6468
03c,06aug96,sgv  fix for spr #4278. Eliminated the nfsLib closing
		 of the UDP socket(multiple close).
03b,06aug96,sgv  fix for spr#5895
03a,23feb95,caf  cleaned up pointer reference in nfsExportShow().
02g,09oct94,jag  nfsDirReadOne() was changed to handle EOF correctly.
02f,08sep94,jmm  Changed to use the new rpc genned versions of the xdr rtns
02e,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
02d,28jul93,jmm  moved AUTH_UNIX_FIELD_LEN and MAX_GRPS definitions to nfsLib.h
02c,03feb93,jdi  documentation cleanup for 5.1.
02b,16sep92,jcf  change typedef path to nfsPath.
02a,07sep92,smb  added blksize and blocks to the stat structure initialisation.
01z,19aug92,smb  Changed systime.h to sys/times.h.
01y,18jul92,smb  Changed errno.h to errnoLib.h.
01x,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01w,20jan92,jmm  changed nfsDirReadOne to also set errno to OK
01w,20jan92,jmm  changed nfsThingCreate to recognize symlinks (spr 1059)
02n,19dec91,rrr  put dirent.h before nfsLib.h so it would compile
02m,13dec91,gae  ANSI fixes.
02l,19nov91,rrr  shut up some ansi warnings.
02k,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
02j,26apr91,shl  removed NULL entry in help_msg of nfsHelp() (spr 987).
02i,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 switched nfsHelp() to first position; doc review by dnw.
02h,08mar91,elh  added nfsReXmit{Sec,USec} so re-xmit time not hardcoded.
02g,22feb91,lpf  made 'ls' command work on files mounted from HP. (SPR #0935)
02f,21feb91,jaa	 documentation.
02e,02oct90,hjb  added a call to htons() where needed.
02d,19jul90,dnw  mangen fix
02c,26jun90,jcf  changed semaphores to be mutex.
02b,29may90,dnw  fixed bug in nfsInit, introduced in 01x, of testing
		    taskRpcStatics->nfsClientCache before rpcTaskInit is called
		 made nfsInit be NOMANUAL
02a,10may90,dnw  fixed nfsDeleteHook to delete the owner task's nfs stuff rather
		   than the exception task's (del hooks are called by exception
		   task).
		 fixed nfsDeleteHook to free nfsClientCache
		 fixed nfsInit to add nfsDeleteHook only once
		 fixed bug in authorization usage: added authCount field to
		   verify that cached client authorization matches current
		   global setting
		 changed name of nfsAllUnmount() to nfsMntUnmountAll()
		 optimized by removing some unnecessary "freeres" calls
		 optimized by supplying buffers in xdr calls instead of having
		   them malloc'd, in readres (nfsFileRead) result msgs.
		 optimized by adding xdr_readdirresOne hack to nfsDirReadOne
		   which just decodes first entry and discards the rest.
		 changed references to taskModuleList to taskRpcStatics
		 added nfsExportRead and nfsExportFree for use by nfsMountAll
		   in nfsDrv
		 extracted client cache stuff from nfsClientCall() to new
		   function nfsClientCacheSetUp()
		 added forward declarations to fix lint about void
		 changed MAX_FILENAME_LENGTH to POSIX definitions PATH_MAX
		   and NAME_MAX where appropriate
		 cosmetic changes to print out of nfsExportShow and
		   nfsMountListPrint
		 removed nfsDirListPrint() (no longer used)
		 removed some unnecessary error printing
		 changed documentation about nfs's large stack requirements
		   (no longer a pig!)
01w,01may90,llk  deleted nfsLs().
		 added nfsDirReadOne() and nfsFileAttrGet().
		 changed to check return value of taskDeleteHookAdd().
		 changed calls to pathBuild() and pathCat() to examine return
		   values.
01v,14apr90,jcf  removed tcb extension dependencies.
01u,01apr90,llk  turned the `oldhost' field of moduleStatics into an array.
		 substituted smaller constants MAX_FILENAME_LENGTH for the
		   SUN specified NFS_MAXNAMLEN and NFS_MAXPATHLEN.
		 added safety checks for filename lengths.
		 deleted calls to pathArrayFree().
		 changed call to pathParse().
		 changed to clean out the CLIENT cache if authorization
		   parameters are changed by calling nfsClientCacheCleanUp().
		 changed rpcAuthUnix to nfsAuthUnix.  Made it LOCAL.
01t,07mar90,jdi  documentation cleanup.
01s,03dec89,llk  changed nfsClientClose() to check for non-null taskModuleList
		   before cleaning up the nfs cache.
01r,20oct89,hjb  changed "struct timeval" to nfsTimeval to avoid conflicts
		 with the definition of "struct timeval" in "utime.h".
01q,28aug89,dab  modified nfsFileRemove() to delete symbolic links.
01p,23jul89,gae  added taskDeleteHook for nfsCleanup.
01o,06jul89,llk  deleted nfsNameArrayFree().  Added call to pathArrayFree().
		 reworked calls to pathLib.  added panic() calls.  delinted.
01n,09jun89,llk  fixed bug in nfsLookUpByName() which showed up via rmdir().
01m,23may89,dnw  lint.
01l,26mar89,llk  added nfsClientCacheCleanUp and nfsNameArrayFree to get rid
		   of memory eaters.
01k,09nov88,llk  rearranged error checking of nfs structures returned by server.
01j,28sep88,llk  added nfsSockBufSize, nfsTimeoutSec, nfsTimeoutUSec.
		 added nfsIdSet().
01i,25aug88,gae  documentation.
01h,07jul88,jcf  lint.
01g,30jun88,llk  changed to handle links.
		 added nfsHelp().
		 more name changes.
01f,16jun88,llk  changed to more v4 names.
		 added LOCAL and NOMANUAL where appropriate.
		 moved pathSplit() to pathLib.c.
01e,04jun88,llk  rewrote nfsLs and nfsDirRead so they clean up after themselves.
		 incorporated pathLib.
		 added nfsAuthUnix{Set Get Prompt Show} for unix authentication.
01d,30may88,dnw  changed to v4 names.
01c,28may88,dnw  copied promptParam...() stuff here from bootLib so that
		   this lib doesn't depend on bootLib.
01b,26apr88,llk  bug fix.  Internet address returned from remGetHostByName
		   was compared to <= NULL.  Some large addresses can be
		   mistaken for negative numbers.
01a,19apr88,llk  written.
*/

/*
DESCRIPTION
This library provides the client side of services for NFS (Network File
System) devices.  Most routines in this library should not be called by
users, but rather by device drivers.  The driver is responsible for
keeping track of file pointers, mounted disks, and cached buffers.  This
library uses Remote Procedure Calls (RPC) to make the NFS calls.

VxWorks is delivered with NFS disabled.
To use this feature, include the following component:
INCLUDE_NFS

In the same file, NFS_USER_ID and NFS_GROUP_ID should be
defined to set the default user ID and group ID at system start-up.
For information about creating NFS devices, see the 
* .I "WindNet TCP/IP Network Programmer's Guide"

Normal use of NFS requires no more than 2000 bytes of stack. This 
requirement may change depending on how the maximum file name path length 
parameter, NFS_MAXPATH, is configured. As many as 4 character arrays of
length NFS_MAXPATH may be allocated off the stack during client operation.
Therefore any increase in the parameter can increase stack usage by a
factor of four times the deviation from default NFS_MAXPATH. For example,
a change from 255 to 1024 will increase peak stack usage by (1024 -255) * 4
which is 3076 bytes.


NFS USER IDENTIFICATION
NFS is built on top of RPC and uses a type of RPC authentication known as
AUTH_UNIX, which is passed on to the NFS server with every NFS request.
AUTH_UNIX is a structure that contains necessary information for NFS,
including the user ID number and a list of group IDs to which the user 
belongs.  On UNIX systems, a user ID is specified in the file 
\f3/etc/passwd\fP.  The list of groups to which a user belongs is 
specified in the file \f3/etc/group\fP.

To change the default authentication parameters, use nfsAuthUnixPrompt().
To change just the AUTH_UNIX ID, use nfsIdSet().  Usually, only the user
ID needs to be changed to indicate a new NFS user.

INCLUDE FILES: nfsLib.h

SEE ALSO
rpcLib, ioLib, nfsDrv

INTERNAL
Every routine starts with a call to nfsInit.  nfsInit initializes RPC
and sets up the appropriate RPC task variables used for NFS.  It
may be called more than once with the only ill effect being wasted time.

The constant nfsMaxPath is used instead of NFS_MAXPATHLEN (1024) or
PATH_MAX (255). nfsMaxPath is derived from the configuration parameter
NFS_MAXPATH. A short length like 255 helps conserve memory but restricts 
use of longer paths. The introduction of NFS_MAXPATH allows the user to
decide which is more important. The constant (NAME_MAX + 1) is used instead
of NFS_MAXNAMELEN (255) in order to conserve memory. All file/path names 
which have come to nfsLib via the I/O system, such as filenames being passed 
to nfsLookUpByName(), have already been screened for their path and name 
lengths being less than PATH_MAX and NAME_MAX.

xdr_nfs has been changed to use the shorter lengths as well.
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "limits.h"
#include "ctype.h"
#include "string.h"
#include "ioLib.h"
#include "taskLib.h"
#include "taskHookLib.h"
#include "rpc/rpc.h"
#include "stdlib.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "rpc/auth.h"
#include "rpc/rpcGbl.h"
#include "xdr_nfs.h"
#include "sys/stat.h"
#include "dirent.h"
#include "nfsLib.h"
#include "hostLib.h"
#include "sys/times.h"
#include "sockLib.h"
#include "pathLib.h"
#include "stdio.h"
#include "unistd.h"
#include "errnoLib.h"
#include "rpcLib.h"
#include "netLib.h"
#include "fioLib.h"
#include "memPartLib.h"

#define READDIR_MAXLEN		512	/* increased for NFSPROC_READDIR on */
					/* HP server */

#ifdef __GNUC__
# ifndef alloca
#  define alloca __builtin_alloca
# endif
#endif

/* IMPORTS */

IMPORT int   nfsMaxPath;               /* max. length of file path */

/* GLOBALS */

unsigned nfsMaxMsgLen = NFS_MAXDATA;	/* largest read or write buffer that
					 * can be transfered to/from the nfs
					 * server
					 */
u_int nfsTimeoutSec   = NFS_TIMEOUT_SEC;
u_int nfsTimeoutUSec  = NFS_TIMEOUT_USEC;

u_int nfsReXmitSec    = NFS_REXMIT_SEC;
u_int nfsReXmitUSec   = NFS_REXMIT_USEC;

int   nfsSockBufSize  = NFS_SOCKOPTVAL;

/* LOCALS */

/*
Static RPC information.
Set on initial NFS invocation and re-used during
subsequent calls on a per task basis.
*/

typedef struct moduleStatics
    {
    CLIENT *client;     /* pointer to client structure */
    int socket;         /* socket associated with this client */
    int oldprognum;     /* last program number used with this client */
    int oldversnum;     /* last version number used with this client */
    int authCount;	/* auth count this client was built with */
    int valid;          /* if TRUE, then this client information is valid */
    char oldhost [MAXHOSTNAMELEN];
    			/* last host name that was used with this client */
    } NFS_MODULE_STATICS;

typedef struct		/* NFS_AUTH_UNIX_TYPE - UNIX authentication structure */
    {
    char machname [AUTH_UNIX_FIELD_LEN];  /* host name where client is */
    int uid;			/* client's UNIX effective uid */
    int gid;			/* client's current group ID */
    int len;			/* element length of aup_gids */
    int aup_gids [MAX_GRPS];	/* array of groups user is in */
    } NFS_AUTH_UNIX_TYPE;

/* There is a single global nfs authorization, nfsAuthUnix.
 * Any time it is changed, nfsAuthCount is bumped.  This count is kept
 * in the nfsClientCache to make sure that the client was built with the
 * current authorization parameters.  In the future, it might be desirable
 * to let different tasks have different authorizations.
 */

LOCAL NFS_AUTH_UNIX_TYPE nfsAuthUnix;
LOCAL int  nfsAuthCount;

LOCAL BOOL nfsInstalled;

/* forward declarations */

LOCAL STATUS nfsClientCall (char *host, u_int prognum, u_int versnum, u_int procnum, xdrproc_t inproc, char *in, xdrproc_t outproc, char *out);
LOCAL STATUS nfsLinkGet (char * hostName, nfs_fh * pHandle, nfspath realPath);
LOCAL STATUS nfsInit (void);
LOCAL void nfsDeleteHook ();
LOCAL void nfsClientCacheCleanUp ();
LOCAL void nfsMountListPrint ();
LOCAL void nfsGroupsPrint ();
LOCAL void nfsExportPrint ();
LOCAL void nfsErrnoSet ();
LOCAL void printClear ();
LOCAL void promptParamString ();
LOCAL void promptParamNum ();

/*******************************************************************************
*
* nfsDirMount - mount an NFS directory
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

STATUS nfsDirMount
    (
    char *hostName,
    dirpath dirname,
    nfs_fh *pFileHandle         /* the directory's file handle */
    )
    {
    fhstatus fileHandleStatus;

    bzero ((char *) &fileHandleStatus, sizeof (fileHandleStatus));

    if (nfsClientCall (hostName, MOUNTPROG, MOUNTVERS, MOUNTPROC_MNT,
			xdr_dirpath, (char *) &dirname,
			xdr_fhstatus, (char *) &fileHandleStatus) == ERROR)
	{
	return (ERROR);
	}

    if (fileHandleStatus.fhs_status != 0)
	{
	netErrnoSet ((int) fileHandleStatus.fhs_status);	/* UNIX error */
	return (ERROR);
	}

    bcopy ((char *) &fileHandleStatus.fhstatus_u.fhs_fhandle,
	   (char *) pFileHandle, sizeof (nfs_fh));

    return (OK);
    }
/*******************************************************************************
*
* nfsDirUnmount - unmount an NFS directory
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

STATUS nfsDirUnmount
    (
    char *hostName,
    dirpath dirname
    )
    {
    return (nfsClientCall (hostName, MOUNTPROG, MOUNTVERS, MOUNTPROC_UMNT,
				xdr_dirpath, (char *) &dirname,
				xdr_void, (char *) NULL));
    }
/*******************************************************************************
*
* nfsMntUnmountAll - unmount all file systems on a particular host
*
* This routine removes all NFS file systems mounted on <hostName> from that
* host's client list.  It does NOT actually "unmount" the file systems.
*
* RETURNS: OK or ERROR.
*
* SEE ALSO: nfsMntDump()
*
* NOMANUAL
*/

STATUS nfsMntUnmountAll
    (
    char *hostName              /* host machine to unmount from */
    )
    {
    return (nfsClientCall (hostName, MOUNTPROG, MOUNTVERS, MOUNTPROC_UMNTALL,
				xdr_void, (char *) NULL,
				xdr_void, (char *) NULL));
    }
/*******************************************************************************
*
* nfsMntDump - display all NFS file systems mounted on a particular host
*
* This routine displays all the NFS file systems mounted on a specified host
* machine.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS nfsMntDump
    (
    char *hostName              /* host machine */
    )
    {
    mountlist mEntries;

    bzero ((char *) &mEntries, sizeof (mEntries));

    if (nfsClientCall (hostName, MOUNTPROG, MOUNTVERS, MOUNTPROC_DUMP,
				xdr_void, (char *) NULL,
				xdr_mountlist, (char *) &mEntries) == ERROR)
	{
	return (ERROR);
	}

    if (mEntries)
	nfsMountListPrint (mEntries);

    clntudp_freeres (taskRpcStatics->nfsClientCache->client, xdr_mountlist,
		     (caddr_t) &mEntries);
    return (OK);
    }
/*******************************************************************************
*
* nfsExportRead - get list of a host's exported file systems
*
* This routine gets the exported file systems of a specified host and the groups
* that are allowed to mount them.
*
* After calling this routine, the export list must be free'd by calling
* nfsExportFree.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*/

STATUS nfsExportRead
    (
    char *hostName,     /* host machine for which to show exports */
    exports *pExports
    )
    {
    bzero ((char *) pExports, sizeof (*pExports));

    return (nfsClientCall (hostName, MOUNTPROG, MOUNTVERS, MOUNTPROC_EXPORT,
			   xdr_void, (char *) NULL,
			   xdr_exports, (char *) pExports));
    }
/*******************************************************************************
*
* nfsExportFree - get list of a host's exported file systems
*
* This routine frees the list of exported file systems returned by
* nfsExportRead.  It must be called after calling nfsExportRead, if
* nfsExportRead returns OK.
*
* RETURNS: OK
*
* NOMANUAL
*/

STATUS nfsExportFree
    (
    exports *pExports
    )
    {
    clntudp_freeres (taskRpcStatics->nfsClientCache->client, xdr_exports,
		     (caddr_t) pExports);
    return (OK);
    }
/*******************************************************************************
*
* nfsHelp - display the NFS help menu
*
* This routine displays a summary of NFS facilities typically called from
* the shell:
* .CS
*     nfsHelp                       Print this list
*     netHelp                       Print general network help list
*     nfsMount "host","filesystem"[,"devname"]  Create device with
*                                     file system/directory from host
*     nfsUnmount "devname"          Remove an NFS device
*     nfsAuthUnixShow               Print current UNIX authentication
*     nfsAuthUnixPrompt             Prompt for UNIX authentication
*     nfsIdSet id		    Set user ID for UNIX authentication
*     nfsDevShow                    Print list of NFS devices
*     nfsExportShow "host"          Print a list of NFS file systems which
*                                     are exported on the specified host
*     mkdir "dirname"               Create directory
*     rm "file"                     Remove file
*
*     EXAMPLE:  -> hostAdd "wrs", "90.0.0.2"
*               -> nfsMount "wrs","/disk0/path/mydir","/mydir/"
*               -> cd "/mydir/"
*               -> nfsAuthUnixPrompt     /@ fill in user ID, etc.     @/
*               -> ls                    /@ list /disk0/path/mydir    @/
*               -> copy < foo            /@ copy foo to standard out  @/
*               -> ld < foo.o            /@ load object module foo.o  @/
*               -> nfsUnmount "/mydir/"  /@ remove NFS device /mydir/ @/
* .CE
*
* RETURNS: N/A
*/

void nfsHelp (void)
    {
    static char *help_msg [] =
{
"nfsHelp                       Print this list",
"netHelp                       Print general network help list",
"nfsMount \"host\",\"filesystem\"[,\"devname\"]  Create device with",
"                                file system/directory from host",
"nfsUnmount \"devname\"          Remove an NFS device",
"nfsAuthUnixShow               Print current UNIX authentication",
"nfsAuthUnixPrompt             Prompt for UNIX authentication",
"nfsIdSet id                   Set user ID for UNIX authentication",
"nfsDevShow                    Print list of NFS devices",
"nfsExportShow \"host\"          Print a list of NFS file systems which",
"                                are exported on the specified host",
"mkdir \"dirname\"               Create directory",
"rm \"file\"                     Remove file",
"",
"EXAMPLE:  -> hostAdd \"wrs\", \"90.0.0.2\"",
"          -> nfsMount \"wrs\",\"/disk0/path/mydir\",\"/mydir/\"",
"          -> cd \"/mydir/\"",
"          -> nfsAuthUnixPrompt     /* fill in user ID, etc. *",
"          -> ls                    /* list /disk0/path/mydir *",
"          -> copy < foo            /* copy foo to standard out *",
"          -> ld < foo.o            /* load object module foo.o *",
"          -> nfsUnmount \"/mydir/\"  /* remove NFS device /mydir/ *",
};
    FAST int ix;

    for (ix = 0; ix < NELEMENTS(help_msg); ix++)
	printf ("%s\n", help_msg [ix]);
    printf ("\n");
    }
/*******************************************************************************
*
* nfsExportShow - display the exported file systems of a remote host
*
* This routine displays the file systems of a specified host and the groups
* that are allowed to mount them.
*
* EXAMPLE
* .CS
*    -> nfsExportShow "wrs"
*    /d0               staff
*    /d1               staff eng
*    /d2               eng
*    /d3
*    value = 0 = 0x0
* .CE
*
* RETURNS: OK or ERROR.
*/

STATUS nfsExportShow
    (
    char *hostName      /* host machine to show exports for */
    )
    {
    exports nfsExports;

    if (hostName == NULL)
        {
        errnoSet (S_hostLib_INVALID_PARAMETER);
        return (ERROR);
        }

    if (nfsExportRead (hostName, &nfsExports) != OK)
	return (ERROR);

    if (nfsExports)
	nfsExportPrint (nfsExports);

    return (nfsExportFree (&nfsExports));
    }

/*******************************************************************************
*
* nfsLookUpByName - looks up a remote file
*
* This routine returns information on a given file.
* It takes the host name, mount handle, and file name, and returns the
* file handle of the file and the file's attributes in pDirOpRes, and
* the file handle of the directory that the file resides in in pDirHandle.
*
* If the file name (directory path or filename itself) contains a symbolic
* link, this routine changes the file name to incorporate the name of the link.
*
* RETURNS:  OK | ERROR | FOLLOW_LINK,
*	    if FOLLOW_LINK, then fileName contains the name of the link
*
* NOMANUAL
*/

int nfsLookUpByName
    (
    char        *hostName,
    char        *fileName,      /* name is changed if symbolic link */
    nfs_fh      *pMountHandle,
                                /* these args are returned to calling routine */
    FAST diropres *pDirOpRes,   /*    pointer to directory operation results */
    nfs_fh      *pDirHandle     /*    pointer to file's directory file handle */
    )
    {
    diropargs file;
    int nameCount = 0;
    char *nameArray  [MAX_DIRNAMES]; /* ptrs to individual dir/file names
				      * in path name */
    char *nameBuf;                   /* buffer for individual dir/file names */
    char *newLinkName;               /* new file name if file is symb link */
    char *linkName;                  /* name of link file */
    char *currentDir;                /* current dir for the look up's
				      * that have been done */


    if (nfsInit () != OK)
	return (ERROR);

    if ((nameBuf = (char *) alloca (nfsMaxPath)) == NULL)
	return (ERROR);
    if ((newLinkName = (char *) alloca (nfsMaxPath)) == NULL)
	return (ERROR);
    if ((linkName = (char *) alloca (nfsMaxPath)) == NULL)
	return (ERROR);
    if ((currentDir = (char *) alloca (nfsMaxPath)) == NULL)
	return (ERROR);

    bzero ((char *) &file, sizeof (file));

    /* parse file name, enter each directory name in the path into the array */

    pathParse (fileName, nameArray, nameBuf);

    bcopy ((char *) pMountHandle, (char *) &file.dir, sizeof (nfs_fh));
    bcopy ((char *) pMountHandle, (char *) pDirHandle, sizeof (nfs_fh));

    /* start traversing file name */

    while ((nameCount < MAX_DIRNAMES) && (nameArray [nameCount] != NULL))
	{
	file.name = nameArray [nameCount];

	if (nfsClientCall (hostName, NFS_PROGRAM, NFS_VERSION, NFSPROC_LOOKUP,
				    xdr_diropargs, (char *) &file,
				    xdr_diropres, (char *) pDirOpRes) == ERROR)

	    {
	    return (ERROR);
	    }

	if (pDirOpRes->status != NFS_OK)
	    {
	    nfsErrnoSet (pDirOpRes->status);
	    return (ERROR);
	    }

	if (pDirOpRes->diropres_u.diropres.attributes.type == NFDIR)
	    {
	    /* save directory information */

	    /* Store the directory handle of the second to last entry
	     * in the path name array.  This is the directory where the
	     * file in question (or directory in question) lives.
	     */

	    if ((nameCount < MAX_DIRNAMES - 1)
	        && (nameArray [nameCount + 1] != NULL))
	        {
		bcopy ((char *) &pDirOpRes->diropres_u.diropres.file,
		       (char *) pDirHandle, sizeof (nfs_fh));
	        }

	    /* copy directory handle for the next nfsClientCall */

	    bcopy ((char *) &pDirOpRes->diropres_u.diropres.file,
		   (char *) &file.dir, sizeof (nfs_fh));
	    }
	else if (pDirOpRes->diropres_u.diropres.attributes.type == NFLNK)
	    {
	    /* symbolic link, get real path name */

	    if (nfsLinkGet (hostName, &pDirOpRes->diropres_u.diropres.file,
			    linkName) == ERROR)
		{
		return (ERROR);
		}

	    /* Change fileName to include name of link.
	     * Concatenate the directory name, link name,
	     * and rest of the path name following the link.
	     */

	    *currentDir = EOS;

	    if ((pathBuild (nameArray, &(nameArray [nameCount]),
			    currentDir) == ERROR)
		|| (pathCat (currentDir, linkName, newLinkName) == ERROR)
		|| (pathBuild (&(nameArray [++nameCount]), (char **) NULL,
			       newLinkName) == ERROR))
		{
		return (ERROR);
		}

	    pathCondense (newLinkName);
	    (void) strcpy (fileName, newLinkName);

	    return (FOLLOW_LINK);
	    }

	nameCount++;	/* look up next directory */
	} /* while */

    return (OK);
    }
/*******************************************************************************
*
* nfsFileRemove - remove a file
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

STATUS nfsFileRemove
    (
    char *      hostName,
    nfs_fh *    pMountHandle,   /* file handle of mount device */
    char *      fullFileName
    )
    {
    u_int	nfsProc;
    diropres	dirResults;
    diropargs	where;
    nfsstat	status;
    int         retVal;
    char	fileName [NAME_MAX + 1];
    char *	dirName;
    nfs_fh	dirHandle;

    if (nfsInit () != OK)
	return (ERROR);

    if (strlen (fullFileName) >= nfsMaxPath)
	return (ERROR);

    if ((dirName = (char *) alloca (nfsMaxPath)) == NULL)
	return (ERROR);

    bzero ((char *) &dirResults, sizeof (dirResults));
    bzero ((char *) &status, sizeof (status));

    pathSplit (fullFileName, dirName, fileName);

    /*
     * get info on the file to be removed, such as the handle of its directory
     * and its file type (so we know whether to delete a file or directory)
     *
     * note:  some file servers will indeed delete a file with the
     *        NFSPROC_RMDIR command
     */

    retVal = nfsLookUpByName (hostName, fullFileName, pMountHandle,
			 &dirResults, &dirHandle);

    if (retVal == FOLLOW_LINK)
        {
        return (FOLLOW_LINK);
        }
    else
        if (retVal == ERROR)
            return (ERROR);


    bcopy ((char *) &dirHandle, (char *) &where.dir, sizeof (nfs_fh));

    where.name = fileName;

    switch (dirResults.diropres_u.diropres.attributes.type)
	{
	case NFREG:			/* remove regular file */
	case NFLNK:			/* remove symbolic link */
	    nfsProc = NFSPROC_REMOVE;
	    break;

	case NFDIR:			/* remove directory */
	    nfsProc = NFSPROC_RMDIR;
	    break;

	default:
	    errnoSet (S_nfsLib_NFS_INAPPLICABLE_FILE_TYPE);
	    return (ERROR);
	}

    if (nfsClientCall (hostName, NFS_PROGRAM, NFS_VERSION, nfsProc,
			    xdr_diropargs, (char *) &where,
			    xdr_nfsstat, (char *) &status) == ERROR)
	{
	return (ERROR);
	}

    if (status != NFS_OK)
	{
	nfsErrnoSet (status);
	return (ERROR);
	}

    return (OK);
    }
/*******************************************************************************
*
* nfsRename - rename a file
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

STATUS nfsRename
    (
    char *      hostName,
    nfs_fh *    pMountHandle,
    char *      oldName,
    nfs_fh *    pOldDirHandle,
    char *      newName
    )
    {
    diropres	dirResults;
    renameargs	renameArgs;
    nfsstat	status;
    char	newFileName [NAME_MAX + 1];
    char	oldFileName [NAME_MAX + 1];
    char *	newDirName;
    char *	oldDirName;
    nfs_fh	dummyDirHandle;

    if (nfsInit () != OK)
	return (ERROR);

    if ((strlen (oldName) >= nfsMaxPath) || (strlen (newName) >= nfsMaxPath))
	{
	errnoSet (S_ioLib_NAME_TOO_LONG);
	return (ERROR);
	}

    if ((newDirName = (char *) alloca (nfsMaxPath)) == NULL)
	return (ERROR);

    if ((oldDirName = (char *) alloca (nfsMaxPath)) == NULL)
	return (ERROR);

    bzero ((char *) &dirResults, sizeof (dirResults));
    bzero ((char *) &status, sizeof (status));

    pathSplit (oldName, oldDirName, oldFileName);
    pathSplit (newName, newDirName, newFileName);

    bcopy ((char *) pOldDirHandle, (char *) &renameArgs.from.dir,
	   sizeof (nfs_fh));
    renameArgs.from.name = oldFileName;

    if (newDirName [0] == EOS)
	{
	/* NULL directory name */

	bcopy ((char *) pMountHandle, (char *) &renameArgs.to.dir,
	       sizeof (nfs_fh));
	}
    else
	{
	if (nfsLookUpByName (hostName, newDirName, pMountHandle,
			     &dirResults, &dummyDirHandle) != OK)
	    return (ERROR);

	bcopy ((char *) &dirResults.diropres_u.diropres.file,
	       (char *) &renameArgs.to.dir, sizeof (nfs_fh));
	}

    renameArgs.to.name = newFileName;

    if (nfsClientCall (hostName, NFS_PROGRAM, NFS_VERSION, NFSPROC_RENAME,
			    xdr_renameargs, (char *) &renameArgs,
			    xdr_nfsstat, (char *) &status) == ERROR)
	{
	return (ERROR);
	}

    if (status != NFS_OK)
	{
	nfsErrnoSet (status);
	return (ERROR);
	}

    return (OK);
    }
/*******************************************************************************
*
* nfsThingCreate - creates file or directory
*
* RETURNS:  OK | ERROR | FOLLOW_LINK,
*	    if FOLLOW_LINK, then fullFileName contains the name of the link
*
* NOMANUAL
*/

int nfsThingCreate
    (
    char *      hostName,
    char *      fullFileName,   /* name of thing being created */
    nfs_fh *    pMountHandle,   /* mount device's file handle */
    diropres *  pDirOpRes,      /* ptr to returned direc. operation results */
    nfs_fh *    pDirHandle,     /* ptr to returned file's directory file hndl */
    u_int       mode            /* mode that file or dir is created with
                                 *   UNIX chmod style */
    )
    {
    FAST int	status;
    char *	newName;              /* new name if name contained a link */
    char *	dirName;              /* dir where file will be created */
    char	fileName[NAME_MAX + 1];	   /* name of file without path */
    char *	tmpName;	      /* temporary file name */

    FAST sattr *pSattr;			/* ptr to attributes */
    diropres	lookUpResult;
    createargs	createArgs;
    nfs_fh	dummyDirHandle;
    u_int	nfsProc;		/* which nfs procedure to call,
					 * depending on whether a file or
					 * directory is made */

    if (nfsInit () != OK)
	return (ERROR);

    if ((newName = (char *) alloca (nfsMaxPath)) == NULL)
	return (ERROR);

    if ((dirName = (char *) alloca (nfsMaxPath)) == NULL)
	return (ERROR);

    /* Extra 4 characters allocated for '/../' */

    if ((tmpName = (char *) alloca (nfsMaxPath + 1 + 3)) == NULL)
	return (ERROR);

    bzero ((char *) &createArgs, sizeof (createArgs));
    bzero ((char *) &lookUpResult, sizeof (lookUpResult));
    bzero ((char *) pDirOpRes, sizeof (diropres));

    pathSplit (fullFileName, dirName, fileName);

    status = nfsLookUpByName (hostName, dirName, pMountHandle, &lookUpResult,
			      &dummyDirHandle);

    if (status == FOLLOW_LINK)
	{
	if ((strlen (dirName) + strlen (fileName) + 1) >= nfsMaxPath)
	    {
	    errnoSet (S_ioLib_NAME_TOO_LONG);
	    status = ERROR;
	    }
	else
	    {
	    pathCat (dirName, fileName, newName);
	    (void) strcpy (fullFileName, newName);
	    }
	}

    if (status != OK)
	return (status);

    bcopy ((char *) &lookUpResult.diropres_u.diropres.file,
	   (char *) &createArgs.where.dir, sizeof (nfs_fh));

    bcopy ((char *) &lookUpResult.diropres_u.diropres.file,
	   (char *) pDirHandle, sizeof (nfs_fh));

    createArgs.where.name = fileName;

    pSattr = &createArgs.attributes;

    if (mode == 0)
    	/* create a file with default permissions */
	pSattr->mode = NFS_FSTAT_REG | DEFAULT_FILE_PERM;
    else
	pSattr->mode = mode;

    /* don't set these attributes */

    pSattr->uid = pSattr->gid = pSattr->size = -1;	/* user ID */
    pSattr->atime.seconds = pSattr->atime.useconds = -1;
    pSattr->mtime.seconds = pSattr->mtime.useconds = -1;

    if (mode & NFS_FSTAT_DIR)
	nfsProc = NFSPROC_MKDIR;
    else
        nfsProc = NFSPROC_CREATE;

    if (nfsClientCall (hostName, NFS_PROGRAM, NFS_VERSION, nfsProc,
			    xdr_createargs, (char *) &createArgs,
			    xdr_diropres, (char *) pDirOpRes) == ERROR)
	{
	return (ERROR);
	}

    /*
     * If the file created is really a symlink, then change the name and
     * return FOLLOW_LINK
     */

    if (pDirOpRes->diropres_u.diropres.attributes.type == NFLNK)
        {
	strcpy (tmpName, fullFileName);	/* Make a copy of fullFileName to
					 * use with nfsLinkGet */

	nfsLinkGet (hostName, &pDirOpRes->diropres_u.diropres.file, tmpName);

        /*
	 * Need to replace last element of fullFileName with return value from
         * nfsLinkGet.  Add /../newFileName to fullFileName, call
         * pathCondense().
	 */

	strcpy (fullFileName, tmpName);
	return (FOLLOW_LINK);
	}

    if (pDirOpRes->status != NFS_OK)
	{
	nfsErrnoSet (pDirOpRes->status);
	return (ERROR);
	}

    return (OK);
    }
/*******************************************************************************
*
* nfsFileWrite - write to a file
*
* Either all characters will be written, or there is an ERROR.
*
* RETURNS:  number of characters written | ERROR
*
* NOMANUAL
*/

int nfsFileWrite
    (
    char *      hostName,
    nfs_fh *    pHandle,        /* file handle of file being written to */
    unsigned    offset,         /* current byte offset in file */
    unsigned    count,          /* number of bytes to write */
    char *      data,           /* data to be written (up to NFS_MAXDATA) */
    fattr *     pNewAttr
    )
    {
    FAST unsigned nWrite = 0;	      /* bytes written in one nfs write */
    FAST unsigned nTotalWrite = 0;    /* bytes written in all writes together */
    writeargs	writeArgs;	      /* arguments to pass to nfs server */
    attrstat	writeResult;	      /* info returned from nfs server */

    if (nfsInit () != OK)
	return (ERROR);

    bzero ((char *) &writeArgs, sizeof (writeArgs));

    bcopy ((char *) pHandle, (char *) &writeArgs.file, sizeof (nfs_fh));

    /* set offset into file where write starts */

    writeArgs.offset = offset;

    while (nTotalWrite < count)
	{
	/* can only write up to NFS_MAXDATA bytes in one nfs write */

	if (count - nTotalWrite > nfsMaxMsgLen)
	    nWrite = nfsMaxMsgLen;
	else
	    nWrite = count - nTotalWrite;

	writeArgs.totalcount = nWrite;
	writeArgs.data.data_len = nWrite;
	writeArgs.data.data_val = data + nTotalWrite;
	writeArgs.offset = offset + nTotalWrite;

	bzero ((char *) &writeResult, sizeof (attrstat));

	if (nfsClientCall (hostName, NFS_PROGRAM, NFS_VERSION, NFSPROC_WRITE,
				xdr_writeargs, (char *) &writeArgs,
				xdr_attrstat, (char *) &writeResult) == ERROR)
	    {
	    return (ERROR);

	    /* XXX if some writes have been done and there was an error,
	     * nTotalWrite should be returned */
	    }

	if (writeResult.status != NFS_OK)
	    {
	    nfsErrnoSet (writeResult.status);
	    return (ERROR);
	    }

	nTotalWrite += nWrite;
	}

    bcopy ((char *) &writeResult.attrstat_u.attributes, (char *) pNewAttr,
	   sizeof (fattr));

    /* if nfs write returned ok, then all bytes were written */

    return (count);
    }
/*******************************************************************************
*
* nfsFileRead - read from a file
*
* Reads as many bytes as are requested if that many bytes are left before
* the end of file.
*
* RETURNS: number of characters read or ERROR
*
* NOMANUAL
*/

int nfsFileRead
    (
    char *      hostName,
    nfs_fh *    pHandle,        /* file handle of file being read */
    unsigned    offset,         /* start at offset bytes from beg. of file */
    unsigned    count,          /* number bytes to read up to */
    char *      buf,            /* buffer that data is read into */
    fattr *     pNewAttr        /* arguments returned from server */
    )
    {
    FAST unsigned nRead = 0;		/* number of bytes read in one read */
    FAST unsigned nTotalRead = 0;	/* bytes read in all reads together */
    readargs	readArgs;		/* arguments to pass to nfs server */
    readres	readReply;

    if (nfsInit () != OK)
	return (ERROR);

    bzero ((char *) &readArgs, sizeof (readArgs));

    bcopy ((char *) pHandle, (char *) &readArgs.file, sizeof (nfs_fh));

    while (nTotalRead < count)
	{
	if (count - nTotalRead > nfsMaxMsgLen)
	    readArgs.count = nfsMaxMsgLen;
	else
	    readArgs.count = count - nTotalRead;

	readArgs.offset = offset + nTotalRead;

	bzero ((char *) &readReply, sizeof (readReply));

	/* make xdr buffer point to caller's buffer */

	readReply.readres_u.reply.data.data_val = buf + nTotalRead;

	if (nfsClientCall (hostName, NFS_PROGRAM, NFS_VERSION, NFSPROC_READ,
				xdr_readargs, (char *) &readArgs,
				xdr_readres, (char *) &readReply) == ERROR)
	    {
	    return (ERROR);
	    }

	if (readReply.status != NFS_OK)
	    {
	    nfsErrnoSet (readReply.status);
	    return (ERROR);
	    }


	/* check for end of file (data_len == 0) */

	if ((nRead = readReply.readres_u.reply.data.data_len) == 0)
	    break;

	/* update attributes */

	bcopy ((char *) &(readReply.readres_u.reply.attributes),
	       (char *) pNewAttr, sizeof (fattr));

	nTotalRead += nRead;
	} /* while not all bytes requested have been read */

    return (nTotalRead);
    }
/*******************************************************************************
*
* nfsLinkGet - gets name of real file from a symbolic link
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS nfsLinkGet
    (
    char *      hostName,
    nfs_fh *    pHandle,        /* file handle of file being read */
    nfspath     realPath        /* the actual pathname gets returned here */
    )
    {
    readlinkres	pathStatus;

    if (nfsInit () != OK)
	return (ERROR);

    bzero ((char *) &pathStatus, sizeof (readlinkres));

    /* make xdr buffer point to caller's buffer */
    pathStatus.readlinkres_u.data = realPath;

    if (nfsClientCall (hostName, NFS_PROGRAM, NFS_VERSION, NFSPROC_READLINK,
			    xdr_nfs_fh, (char *) pHandle,
			    xdr_readlinkres, (char *) &pathStatus) == ERROR)
	{
	return (ERROR);
	}

    if (pathStatus.status != NFS_OK)
	{
	nfsErrnoSet (pathStatus.status);
	return (ERROR);
	}

    return (OK);
    }
#if 0
/*******************************************************************************
*
* nfsDirRead - read entries from a directory
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS nfsDirRead
    (
    char *      hostName,
    nfs_fh *    pDirHandle,
    nfscookie   cookie,         /* hand this cookie to the server.  The cookie
                                 * marks a place in the directory where entries
                                 * are to be read from */
    unsigned    count,          /* number of bytes that nfs may return */
    readdirres *pReadDirRes     /* return the results of directory read here */
    )
    {
    readdirargs readDirArgs;

    if (nfsInit () != OK)
	return (ERROR);

    bzero ((char *) &readDirArgs, sizeof (readdirargs));
    bcopy ((char *) pDirHandle, (char *) &readDirArgs.dir, sizeof (nfs_fh));
    bcopy (cookie, readDirArgs.cookie, sizeof (nfscookie));
    readDirArgs.count = count;

    bzero ((char *) pReadDirRes, sizeof (*pReadDirRes));

    if (nfsClientCall (hostName, NFS_PROGRAM, NFS_VERSION, NFSPROC_READDIR,
			    xdr_readdirargs, (char *) &readDirArgs,
			    xdr_readdirres, (char *) pReadDirRes) == ERROR)
	{
	return (ERROR);
	}

    if (pReadDirRes->status != NFS_OK)
	{
	nfsErrnoSet (pReadDirRes->status);
	return (ERROR);
	}

    /* normally, the readdirres structure would be freed up here, but
     * the calling routine needs its information, so the calling routine
     * must call clntudp_freeres after calling nfsDirRead
     */
    return (OK);
    }
/*******************************************************************************
*
* nfsDirResFree - free the rcp readdirres result structure
*
* RETURNS: TRUE if successful, FALSE if error
*/

LOCAL BOOL nfsDirResFree
    (
    readdirres *pReadDirRes
    )
    {
    return (clntudp_freeres (taskRpcStatics->nfsClientCache->client,
			     xdr_readdirres, (caddr_t) pReadDirRes));
    }
#endif
/*******************************************************************************
*
* nfsDirReadOne - read one directory entry
*
* Read one directory entry on the host from the directory represented by
* the file handle *pDirHandle.  Update the DIR structure that is passed
* in and pointed to by pDir.
*
* NOTE:
* There is actually no way to tell the server we want only one entry.
* We only want one entry, but we have to provide a buffer for the maximum
* length filename, where the buffer size is specified in the "count" param
* to nfsDirRead.  The server will return as many entries as will fit in
* this buffer.  We will return the first entry to the caller and throw
* away the rest.  A subsequent call with the cookie of the previous
* entry will acquire the next entry, even though it may have been
* sent in previous call.
*
* In the future, we could cache several entries at a time and just return
* them to the caller, one at a time.
*
* RETURNS: OK, or ERROR if dir read fails,
*   returns ERROR but errno unchanged if EOF
*
* NOMANUAL
*/

STATUS nfsDirReadOne
    (
    char *      hostName,       /* host name */
    nfs_fh *    pDirHandle,     /* file handle of directory */
    DIR *       pDir            /* ptr to DIR structure, return values here */
    )
    {
    readdirres   readRes;
    readdirargs  readDirArgs;

    bzero ((char *) &readRes, sizeof (readRes));
    
    if (nfsInit () != OK)
	return (ERROR);

    bzero ((char *) &readDirArgs, sizeof (readdirargs));
    bcopy ((char *) pDirHandle, (char *) &readDirArgs.dir, sizeof (nfs_fh));
    bcopy ((char *) &pDir->dd_cookie, readDirArgs.cookie, sizeof (nfscookie));

    /*
    readDirArgs.count = sizeof (struct readdirres) + sizeof (struct entry) +
			NAME_MAX;
    */

    /*
     For reasons yet unknown to me, the HP server expects the value of
     count to be AT LEAST 512 in order to successfully read directory files
     mounted from HP. Sun server does not have this limitation.
    */
    readDirArgs.count = READDIR_MAXLEN;

    if (nfsClientCall (hostName, NFS_PROGRAM, NFS_VERSION, NFSPROC_READDIR,
			xdr_readdirargs, (char *) &readDirArgs,
			xdr_readdirres, (char *) &readRes) == ERROR)
	{
	return (ERROR);
	}

    if (readRes.status != NFS_OK)
	{
	nfsErrnoSet (readRes.status);
	return (ERROR);
	}

    if (readRes.readdirres_u.reply.entries == NULL && 
	readRes.readdirres_u.reply.eof)
        {
	/* No entries were returned - must be end of file.
	 * (Note that xdr_readdirresOne hack discarded EOF flag.)
         * Return ERROR, but don't alter errno because there is no real error.
	 * (Thank you POSIX for this lovely, clear construct.)
	 */

	/*
	 * Need to set errno to OK, otherwise ls will print out an error
	 * message at the end of every directory listing.
	 */

	errnoSet (OK);
	return (ERROR);
	}


    strcpy (pDir->dd_dirent.d_name,
	    readRes.readdirres_u.reply.entries[0].name);
    
    /* update caller's cookie */

    bcopy (readRes.readdirres_u.reply.entries[0].cookie,
	   (char *) &pDir->dd_cookie, sizeof (nfscookie));

    /* free the memory used by RPC */

    clnt_freeres (taskRpcStatics->nfsClientCache->client, xdr_readdirres,
		  (caddr_t) &readRes);

    return (OK);
    }
/*******************************************************************************
*
* nfsFileAttrGet - get file attributes and put them in a stat structure
*
* NOMANUAL
*/

void nfsFileAttrGet
    (
    FAST fattr *        pFattr, /* ptr to nfs file attributes */
    FAST struct stat *  pStat   /* ptr to stat struct, return attrs here */
    )
    {
    /*
     * INTERNAL:  block information is kept in the fattr
     * structure, but we decided not to hold it in the
     * stat structure.  It could be added later.
     */

    pStat->st_ino         = (ULONG) pFattr->fileid;
    pStat->st_mode        = pFattr->mode;
    pStat->st_nlink       = pFattr->nlink;
    pStat->st_uid         = pFattr->uid;
    pStat->st_gid         = pFattr->gid;
    pStat->st_rdev        = 0;                          /* unused */
    pStat->st_size        = pFattr->size;
    pStat->st_atime       = pFattr->atime.seconds;
    pStat->st_mtime       = pFattr->mtime.seconds;
    pStat->st_ctime       = pFattr->ctime.seconds;
    pStat->st_blksize     = 0;
    pStat->st_blocks      = 0;
    }

/******************************************************************************
*
* nfsFsAttrGet - get file system attributes across NFS
*
* This routine does an FSTATFS across NFS.
*
* NOMANUAL
*/

STATUS nfsFsAttrGet
    (
    char* pHostName,                         /* host name */
    nfs_fh* pDirHandle,                      /* file handle of directory */
    struct statfs* pArg                      /* return value here */
    )
    {

    int status;
    statfsres retStat;

    status = nfsClientCall (pHostName,
                            NFS_PROGRAM, NFS_VERSION, NFSPROC_STATFS,
                            xdr_fhandle, (char *)pDirHandle,
                            xdr_statfsres, (char *)&retStat);
    
    if (retStat.status != NFS_OK)
        {
        nfsErrnoSet (retStat.status);
        return (ERROR);
        }

    pArg->f_type = 0;
    pArg->f_bsize = retStat.statfsres_u.reply.bsize;
    pArg->f_blocks = retStat.statfsres_u.reply.blocks;
    pArg->f_bfree = retStat.statfsres_u.reply.bfree;
    pArg->f_bavail = retStat.statfsres_u.reply.bavail;
    
    /* The following are undefined for NFS under vxWorks and
     * so are set to 0.
     */
    pArg->f_files = 0;    /* total file nodes in file system */
    pArg->f_ffree = 0;    /* free file nodes in fs */


    clnt_freeres (taskRpcStatics->nfsClientCache->client, xdr_statfsres,
                  (caddr_t)&retStat);

    return (OK);
    }

/*******************************************************************************
*
* nfsDeleteHook - cleanup NFS data structures
*/

LOCAL void nfsDeleteHook
    (
    WIND_TCB *pTcb
    )
    {
#ifdef  _WRS_VXWORKS_5_X
    FAST RPC_STATICS *pRPCModList = pTcb->pRPCModList;
#else
    FAST RPC_STATICS *pRPCModList = pTcb->pLcb->pRPCModList;
#endif /*  _WRS_VXWORKS_5_X */
    
    if (pRPCModList != NULL)
	{
	nfsClientCacheCleanUp (pRPCModList->nfsClientCache);
	if (pRPCModList->nfsClientCache != NULL)
	    {
	    KHEAP_FREE((char *) pRPCModList->nfsClientCache);
	    pRPCModList->nfsClientCache = NULL;
	    }
	}
    }
/*******************************************************************************
*
* nfsInit - initialize NFS
*
* nfsInit must be called by a task before it uses NFS.
* It is okay to call this routine more than once.
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

LOCAL STATUS nfsInit (void)
    {
    /* make sure rpc is initialized for this task */

    if (rpcTaskInit () == ERROR)
	return (ERROR);

    /* if first nfs use by this task, initialize nfs stuff for this task */

    if (taskRpcStatics->nfsClientCache == NULL)
	{
	/* if this the very first use of NFS in the system, add nfsDeleteHook */

	if (!nfsInstalled)
	    {
	    if (taskDeleteHookAdd ((FUNCPTR)nfsDeleteHook) == ERROR)
		return (ERROR);

	    nfsInstalled = TRUE;
	    }


	/* allocate nfs specific stuff for this task */

	taskRpcStatics->nfsClientCache =
	    (NFS_MODULE_STATICS *) KHEAP_ALLOC(sizeof (NFS_MODULE_STATICS));

	if (taskRpcStatics->nfsClientCache == NULL)
	    return (ERROR);
        bzero ((char *)taskRpcStatics->nfsClientCache,
            sizeof (NFS_MODULE_STATICS));
	}

    return (OK);
    }
/*******************************************************************************
*
* nfsClientCacheCleanUp - clean up the NFS client cache
*
* This routine is used to free up structures that are part of a valid
* NFS client cache.  The client cache is marked as invalid.
*/

LOCAL void nfsClientCacheCleanUp
    (
    FAST NFS_MODULE_STATICS *pClientCache       /* pointer to client cache */
    )
    {
    if ((pClientCache != NULL) && (pClientCache->valid))
	{
	pClientCache->valid = FALSE;

	/* close client socket */

	if (pClientCache->socket > 0)
	    {
#if 0
	    close (pClientCache->socket);
#endif
	    pClientCache->socket = NONE;
	    }

	pClientCache->oldhost[0] = EOS;


	/* destroy authorization and client */

	if (pClientCache->client != NULL)
	    {
	    if (pClientCache->client->cl_auth != NULL)
		{
		auth_destroy (pClientCache->client->cl_auth);
		pClientCache->client->cl_auth = NULL;
		}

	    clnt_destroy (pClientCache->client);
	    pClientCache->client = NULL;
	    }
	}
    }
/*******************************************************************************
*
* nfsClientCacheSetUp - set up the NFS client cache
*
* This routine is used to set up structures that are part of a valid
* NFS client cache.  If successful, the client cache is marked as valid.
*/

LOCAL STATUS nfsClientCacheSetUp
    (
    FAST NFS_MODULE_STATICS *ms,        /* pointer to client cache */
    char *      host,                   /* server's host name */
    u_int       prognum,                /* RPC program number */
    u_int       versnum                 /* RPC version number */
    )
    {
    int			hp;		/* host's inet address */
    struct sockaddr_in	server_addr;
    struct timeval	timeout;
    int			optval;

    /* check if cached client is appropriate */

    if (ms->valid && ms->oldprognum == prognum && ms->oldversnum == versnum
	&& strcmp (ms->oldhost, host) == 0 && ms->authCount == nfsAuthCount)
	{
	return (OK);	/* reuse old client */
    	}


    /* get rid of any old client and create new one */

    nfsClientCacheCleanUp (ms);

    if ((hp = hostGetByName (host)) == ERROR)
	return (ERROR);

    timeout.tv_sec  = nfsReXmitSec;
    timeout.tv_usec = nfsReXmitUSec;

    server_addr.sin_addr.s_addr	= hp;
    server_addr.sin_family	= AF_INET;
    server_addr.sin_port	= htons (0);

    ms->socket = RPC_ANYSOCK;

    if ((ms->client = clntudp_create(&server_addr, prognum, versnum,
				     timeout, &ms->socket)) == NULL)
	{
	server_addr.sin_port = htons (NFS_PORT);
	ms->client = clntudp_create (&server_addr, prognum, versnum,
				     timeout, &ms->socket);
	if (ms->client == NULL)
	    return (ERROR);
	}

    optval = nfsSockBufSize;
    if ((setsockopt (ms->socket, SOL_SOCKET, SO_SNDBUF, (char *)&optval,
		     sizeof (optval)) == ERROR)
	|| (setsockopt (ms->socket, SOL_SOCKET, SO_RCVBUF, (char *)&optval,
			sizeof (optval)) == ERROR))
	{
	nfsClientCacheCleanUp (ms);
	return (ERROR);
	}

    ms->client->cl_auth = authunix_create (nfsAuthUnix.machname,
					   nfsAuthUnix.uid, nfsAuthUnix.gid,
					   nfsAuthUnix.len,
					   nfsAuthUnix.aup_gids);
    if (ms->client->cl_auth == NULL)
	{
	nfsClientCacheCleanUp (ms);
	errnoSet (S_nfsLib_NFS_AUTH_UNIX_FAILED);
	return (ERROR);
	}

    ms->oldprognum = prognum;
    ms->oldversnum = versnum;
    ms->authCount  = nfsAuthCount;

    (void) strcpy (ms->oldhost, host);

    ms->valid = TRUE;

    return (OK);
    }
/*******************************************************************************
*
* nfsClientCall - make an NFS request to the server using RPC
*
* This is the routine which does the actual RPC call to the server.
* All NFS routines which communicate with the file server use this routine.
*
* RETURNS:  OK | ERROR
*/

LOCAL STATUS nfsClientCall
    (
    char *      host,           /* server's host name */
    u_int       prognum,        /* RPC program number */
    u_int       versnum,        /* RPC version number */
    u_int       procnum,        /* RPC procedure number */
    xdrproc_t   inproc,         /* xdr routine for args */
    char *      in,
    xdrproc_t   outproc,        /* xdr routine for results */
    char *      out
    )
    {
    nfstime		tottimeout;
    enum clnt_stat	clientStat;
    FAST NFS_MODULE_STATICS *ms;

    if (nfsInit () != OK)
	return (ERROR);
 
    ms = taskRpcStatics->nfsClientCache;

    /* get an appropriate client in the cache */

    if (nfsClientCacheSetUp (ms, host, prognum, versnum) != OK)
	return (ERROR);

    /* set time to allow results to come back */

    tottimeout.seconds  = nfsTimeoutSec;
    tottimeout.useconds = nfsTimeoutUSec;

    clientStat = clnt_call (ms->client, procnum, inproc, in, outproc, out,
			    tottimeout);

    if (clientStat != RPC_SUCCESS)
	{
	/* XXX this should be more gracefull */

	nfsClientCacheCleanUp (ms);
	rpcClntErrnoSet (clientStat);
	return (ERROR);
	}

    return (OK);
    }
/*******************************************************************************
*
* nfsClientClose - close the NFS client socket and associated structures
*
* NOMANUAL
*/

void nfsClientClose (void)
    {
    if (taskRpcStatics != NULL)
	nfsClientCacheCleanUp (taskRpcStatics->nfsClientCache);
    }
/*******************************************************************************
*
* nfsMountListPrint - prints a list of mount entries
*/

LOCAL void nfsMountListPrint
    (
    FAST mountlist pMountList
    )
    {
    while (pMountList)
	{
	printf ("%s:%s\n", pMountList->ml_hostname, pMountList->ml_directory);
	pMountList = pMountList->ml_next;
	}
    }
/*******************************************************************************
*
* nfsGroupsPrint - print a list of groups
*/

LOCAL void nfsGroupsPrint
    (
    FAST groups pGroup
    )
    {
    while (pGroup != NULL)
	{
	printf ("%s ", pGroup->gr_name);
	pGroup = pGroup->gr_next;
	}
    }
/*******************************************************************************
*
* nfsExportPrint - prints a list of exported file systems on a host
*/

LOCAL void nfsExportPrint
    (
    FAST exports pExport
    )
    {
    while (pExport != NULL)
	{
	printf ("%-25s ", pExport->ex_dir);
	nfsGroupsPrint (pExport->ex_groups);
	printf ("\n");
	pExport = pExport->ex_next;
	}
    }
/*******************************************************************************
*
* nfsErrnoSet - set NFS status
*
* nfsErrnoSet calls errnoSet with the given "nfs stat" or'd with the
* NFS status prefix.
*/

LOCAL void nfsErrnoSet
    (
    enum nfsstat status
    )
    {
    errnoSet (M_nfsStat | (int) status);
    }
/*******************************************************************************
*
* nfsAuthUnixPrompt - modify the NFS UNIX authentication parameters
*
* This routine allows
* UNIX authentication parameters to be changed from the shell.
* The user is prompted for each parameter, which can be changed
* by entering the new value next to the current one.
*
* EXAMPLE
* .CS
*    -> nfsAuthUnixPrompt
*    machine name:   yuba
*    user ID:        2001 128
*    group ID:       100
*    num of groups:  1 3
*    group #1:        100 100
*    group #2:        0 120
*    group #3:        0 200
*    value = 3 = 0x3
* .CE
*
* SEE ALSO: nfsAuthUnixShow(), nfsAuthUnixSet(), nfsAuthUnixGet(), nfsIdSet()
*/

void nfsAuthUnixPrompt (void)
    {
    char machname [AUTH_UNIX_FIELD_LEN];/* host name where client is */
    int uid;				/* client's UNIX effective uid */
    int gid;				/* client's current group ID */
    int len;				/* element length of aup_gids */
    int aup_gids [MAX_GRPS];		/* array of groups user is in */
    int ix;

    nfsAuthUnixGet (machname, &uid, &gid, &len, aup_gids);

    promptParamString ("machine name:  ", machname, sizeof (machname));
    promptParamNum ("user ID:       ", &uid, 8, "%d ");
    promptParamNum ("group ID:      ", &gid, 8, "%d ");
    promptParamNum ("num of groups: ", &len, 8, "%d ");
    for (ix = 0; ix < len; ix++)
	{
	printf ("group #%d:       ", ix + 1);
	promptParamNum ("", &aup_gids [ix], 8, "%d ");
	}
    nfsAuthUnixSet (machname, uid, gid, len, aup_gids);
    }
/*******************************************************************************
*
* nfsAuthUnixShow - display the NFS UNIX authentication parameters
*
* This routine displays the parameters set by nfsAuthUnixSet() or
* nfsAuthUnixPrompt().
*
* EXAMPLE:
* .CS
*    -> nfsAuthUnixShow
*    machine name = yuba
*    user ID      = 2001
*    group ID     = 100
*    group [0]    = 100
*    value = 1 = 0x1
* .CE
*
* RETURNS: N/A
*
* SEE ALSO: nfsAuthUnixPrompt(), nfsAuthUnixSet(), nfsAuthUnixGet(), nfsIdSet()
*/

void nfsAuthUnixShow (void)
    {
    char machname [AUTH_UNIX_FIELD_LEN]; /* host name where client is */
    int uid;				/* client's UNIX effective uid */
    int gid;				/* client's current group ID */
    int len;				/* element length of aup_gids */
    int aup_gids [MAX_GRPS];		/* array of groups user is in */
    int ix;

    nfsAuthUnixGet (machname, &uid, &gid, &len, aup_gids);

    printf ("machine name = %s\n", machname);
    printf ("user ID      = %d\n", uid);
    printf ("group ID     = %d\n", gid);
    for (ix = 0; ix < len; ix++)
	printf ("group [%d]    = %d\n", ix, aup_gids [ix]);
    }
/*******************************************************************************
*
* nfsAuthUnixSet - set the NFS UNIX authentication parameters
*
* This routine sets UNIX authentication parameters.
* It is initially called by usrNetInit().
* `machname' should be set with the name of the mounted system (i.e. the target
* name itself) to distinguish hosts from hosts on a NFS network.
*
* RETURNS: N/A
*
* SEE ALSO: nfsAuthUnixPrompt(), nfsAuthUnixShow(), nfsAuthUnixGet(), 
* nfsIdSet()
* 
* 
*
*/

void nfsAuthUnixSet
    (
    char *machname,     /* host machine        */
    int uid,            /* user ID             */
    int gid,            /* group ID            */
    int ngids,          /* number of group IDs */
    int *aup_gids       /* array of group IDs  */
    )
    {
    int ix;

    taskLock ();

    (void) strcpy (nfsAuthUnix.machname, machname);
    nfsAuthUnix.uid = uid;
    nfsAuthUnix.gid = gid;
    nfsAuthUnix.len = (ngids < MAX_GRPS ? ngids : MAX_GRPS);
    for (ix = 0; ix < ngids; ix++)
	nfsAuthUnix.aup_gids [ix] = aup_gids [ix];

    /* Cached client authentications are out of date now.
     * Bump auth count so clients will be rebuilt with new auth,
     * next time the client transport is used.
     */

    nfsAuthCount++;

    taskUnlock ();
    }
/*******************************************************************************
*
* nfsAuthUnixGet - get the NFS UNIX authentication parameters
*
* This routine gets the previously set UNIX authentication values.
*
* RETURNS: N/A
*
* SEE ALSO: nfsAuthUnixPrompt(), nfsAuthUnixShow(), nfsAuthUnixSet(), 
* nfsIdSet()
*/

void nfsAuthUnixGet
    (
    char *machname,     /* where to store host machine        */
    int *pUid,          /* where to store user ID             */
    int *pGid,          /* where to store group ID            */
    int *pNgids,        /* where to store number of group IDs */
    int *gids           /* where to store array of group IDs  */
    )
    {
    int ix;

    (void) strcpy (machname, nfsAuthUnix.machname);
    *pUid   = nfsAuthUnix.uid;
    *pGid   = nfsAuthUnix.gid;
    *pNgids = nfsAuthUnix.len;

    for (ix = 0; ix < nfsAuthUnix.len; ix++)
	gids [ix] = nfsAuthUnix.aup_gids [ix];
    }
/*******************************************************************************
*
* nfsIdSet - set the ID number of the NFS UNIX authentication parameters
*
* This routine sets only the UNIX authentication user ID number.
* For most NFS permission needs, only the user ID needs to be changed.
* Set <uid> to the user ID on the NFS server.
*
* RETURNS: N/A
*
* SEE ALSO: nfsAuthUnixPrompt(), nfsAuthUnixShow(), nfsAuthUnixSet(),
* nfsAuthUnixGet()
* 
*/

void nfsIdSet
    (
    int uid             /* user ID on host machine */
    )
    {
    taskLock ();

    nfsAuthUnix.uid = uid;

    /* Cached client authentications are out of date now.
     * Bump auth count so clients will be rebuilt with new auth,
     * next time the client transport is used.
     */

    nfsAuthCount++;

    taskUnlock ();
    }

/*******************************************************************************
*
* printClear - print string with '?' for unprintable characters
*/

LOCAL void printClear
    (
    FAST char *param
    )
    {
    FAST char ch;

    while ((ch = *(param++)) != EOS)
	printf ("%c", (isascii ((UINT)ch) && isprint ((UINT)ch)) ? ch : '?');
    }
/*******************************************************************************
*
* promptParamString - prompt the user for a string parameter
*
* - carriage return leaves the parameter unmodified;
* - "." clears the parameter (null string).
*/

LOCAL void promptParamString
    (
    char *msg,
    char *param,
    int fieldWidth
    )
    {
    int ix;
    char buf [100];

    FOREVER
	{
	printf ("%s ", msg);
	printClear (param);
	printf (" ");

	ix = fioRdString (STD_IN, buf, sizeof (buf));
	if (ix < fieldWidth)
	    break;
	printf ("too big - maximum field width = %d.\n", fieldWidth);
	}

    if (ix == 1)
	return;			/* just CR; leave field unchanged */

    if (buf[0] == '.')
	{
	param [0] = EOS;	/* just '.'; make empty field */
	return;
	}

    (void) strcpy (param, buf);	/* update parameter */
    }
/*******************************************************************************
*
* promptParamNum - prompt the user for a parameter
*
* - carriage return leaves the parameter unmodified;
* - "." clears the parameter (0).
*/

LOCAL void promptParamNum
    (
    char *msg,
    int *pParam,
    int fieldWidth,
    char *format
    )
    {
    int ix;
    char buf [100];

    FOREVER
	{
	(void) strcpy (buf, "%s ");
	(void) strcat (buf, format);

	printf (buf, msg, *pParam);

	ix = fioRdString (STD_IN, buf, sizeof (buf));
	if (ix < fieldWidth)
	    break;
	printf ("too big - maximum field width = %d.\n", fieldWidth);
	}

    if (ix == 1)
	return;			/* just CR; leave field unchanged */

    if (buf[0] == '.')
	{
	pParam = 0;		/* just '.'; make empty field */
	return;
	}

    (void) sscanf (buf, format, pParam);	/* scan field */
    }

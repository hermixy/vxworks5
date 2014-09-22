/* mountLib.c - Mount protocol library */

/* Copyright 1994 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01s,10may02,kbw  making man page edits
01r,07may02,kbw  making man page edits
01q,05nov01,vvv  made max. path length configurable (SPR #63551)
01p,05nov01,wap  Fix memory leak in mountproc_umnt_1 and mountproc_umntall_1
                 (SPR #63489)
01o,15oct01,rae  merge from truestack ver 01v, base 01m (SPRs 62705,
                 29301/29362, 32821/31223)
01n,04dec00,ijm  fixed file descriptor leak in nfsUnexport (SPR# 7531)
01n,21jun00,rsh  upgrade to dosFs 2.0
01l,05nov98,rjc  modifications for dosfs2.
01m,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01l,05feb99,dgp  document errno values
01k,26aug97,spm  removed compiler warnings (SPR #7866)
01j,12dec96,jag  fix spr 7601 on procedure mountproc_umnt_1, deleted unused
		 variable deleteThis.
01i,30aug95,rch  fix for spr 4333.
01h,27feb95,jdi  doc: changed dosFsMode to dosFsFileMode (doc only) (SPR 4085).
01g,11feb95,jdi  doc format repair.
01f,24jan95,rhp  update initialization info for exportable file systems,
                 and misc doc tweaks
01e,11may94,jmm  integrated Roland's doc changes
01d,25apr94,jmm  doc cleanup; changed mode to readOnly in NFS_EXPORT_ENTRY
01c,21apr94,jmm  fixed problem with mountproc_dump_1 overwritting strings
                 documentation cleanup, reordered routines
01b,20apr94,jmm  formatting cleanup, mountproc_dump_1 and mountproc_exports_1
                 fixed
01a,07mar94,jmm  written
*/

/*
DESCRIPTION
This library implements a mount server to support mounting VxWorks
file systems remotely.  The mount server is an implementation of
version 1 of the mount protocol as defined in RFC 1094.  It is closely
connected with version 2 of the Network File System Protocol
Specification, which in turn is implemented by the library nfsdLib.

\&NOTE: The only routines in this library that are normally called by
applications are nfsExport() and nfsUnexport().  The mount daemon is
normally initialized indirectly by nfsdInit().

The mount server is initialized by calling mountdInit().  Normally,
this is done by nfsdInit(), although it is possible to call
mountdInit() directly if the NFS server is not being initialized.
Defining INCLUDE_NFS_SERVER enables the call to nfsdInit() during
the boot process, which in turn calls mountdInit(), so there is
normally no need to call either routine manually.  mountdInit() spawns
one task, `tMountd', which registers as an RPC service with the
portmapper.

Currently, only the dosFsLib file system is supported.
File systems are exported with the nfsExport() call.

To export VxWorks file systems via NFS, you need facilities from both
this library and from nfsdLib.  To include both, add INCLUDE_NFS_SERVER
and rebuild VxWorks.
.SS Example
The following example illustrates how to export an
existing dosFs file system.

First, initialize the block device containing your file system.

Then assuming the dosFs system is called "/export" execute the following 
code on the target:

.CS
nfsExport ("/export", 0, FALSE, 0);           /@ make available remotely @/
.CE

This makes it available to all
clients to be mounted using the client's NFS mounting command.  (On
UNIX systems, mounting file systems normally requires root privileges.)

VxWorks does not normally provide authentication services for NFS
requests, and the DOS file system does not provide file permissions.
If you need to authenticate incoming requests, see the documentation
for nfsdInit() and mountdInit() for information about authorization
hooks.

The following requests are accepted from clients.  For details of
their use, see Appendix A of RFC 1094, "NFS: Network File System
Protocol Specification."

.TS
tab(|);
lf3 lf3
l n.
 Procedure Name     |   Procedure Number
_
 MOUNTPROC_NULL     |   0
 MOUNTPROC_MNT      |   1
 MOUNTPROC_DUMP     |   2
 MOUNTPROC_UMNT     |   3
 MOUNTPROC_UMNTALL  |   4
 MOUNTPROC_EXPORT   |   5
.TE

SEE ALSO: dosFsLib, nfsdLib, RFC 1094

INTERNAL: mountd is not reentrant, and there cannot be more than one
mountd task running at any one time.

The routines called by mountd are named mountproc_ROUTINENAME_1.  This
is the standard RPC naming convention used by code generated from
rpcgen.
*/

/* LINTLIBRARY */

#include "vxWorks.h"
#include "sys/stat.h"
#include "ioLib.h"
#include "taskLib.h"
#include "lstLib.h"
#include "mountLib.h"
#include "nfsdLib.h"
#include "pathLib.h"
#include "rpcLib.h"
#include "rpc/pmap_clnt.h"
#include "rpc/rpc.h"
#include "rpcLib.h"
#include "inetLib.h"
#include "semLib.h"
#include "hostLib.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "xdr_nfsserv.h"
#include "private/nfsHashP.h"
#include "memPartLib.h"

/* defines */
#ifndef KHEAP_REALLOC
#define KHEAP_REALLOC(pBuf,newsize) 	memPartRealloc(memPartIdKernel,pBuf,newsize)
#endif /* KHEAP_REALLOC */

#ifdef __GNUC__
# ifndef alloca
#  define alloca __builtin_alloca
# endif
#endif

/* IMPORTS */

IMPORT int nfsMaxPath;  /* max. length of file path */

/* DATA STRUCTURES */

typedef struct
    {
    char * next;           /* next and prev are used by lstLib */
    char * prev;
    char   clientName [MAXHOSTNAMELEN + 1]; /* name of the host mounting this
					     * directory */
    /*
     * Mounted directory. Though it appears a single character, it is a char
     * array. The array is allocated when a MOUNT_CLIENT struct is allocated.
     * This change was required for adding configurable path length support.
     */

    char   directory [1];        
    } MOUNT_CLIENT;

/* GLOBALS */

int     mountdNExports = MAX_EXPORTED_FILESYSTEMS; /* max. num. exported FS's */
int     mountdTaskId           = 0;	        /* Task ID of the mountd task */
int     mountdPriorityDefault  = MOUNTD_PRIORITY_DEFAULT; /* default priority */
int     mountdStackSizeDefault = MOUNTD_STACKSIZE_DEFAULT; /* default stack   */

/* LOCALS */

LOCAL FUNCPTR mountdAuthHook = NULL; /* Hook to run to authorize packets */

LOCAL LIST *  mountPoints = NULL; /* Linked list of exported mount points */
LOCAL SEM_ID  mountSem    = NULL; /* Sem controlling access to mountPoints */

LOCAL LIST *  nfsClients  = NULL; /* Linked list of clients mounting fs's */
LOCAL SEM_ID  clientsSem  = NULL; /* Sem controlling access to nfsClients */

LOCAL int     fsIdNumber  = 1;	/* ID number for exported file systems */


/* forward LOCAL functions */

LOCAL void mountdRequestProcess (struct svc_req * rqstp,
				 register SVCXPRT * transp);
LOCAL char * svc_reqToHostName (struct svc_req * rqstp, char * hostName);
LOCAL MOUNT_CLIENT * mountListFind (struct svc_req * rqstp, dirpath * path);

/******************************************************************************
*
* mountdInit - initialize the mount daemon
* 
* This routine spawns a mount daemon if one does not already
* exist.  Defaults for the <priority> and <stackSize> arguments are
* in the global variables `mountdPriorityDefault' and
* `mountdStackSizeDefault', and are initially set to
* MOUNTD_PRIORITY_DEFAULT and MOUNTD_STACKSIZE_DEFAULT respectively.
*
* Normally, no authorization checking is performed by either mountd or
* nfsd.  To add authorization checking, set <authHook> to point to a
* routine declared as follows:
*
* .CS
* nfsstat routine
*     (
*     int                progNum,       /@ RPC program number @/
*     int                versNum,	/@ RPC program version number @/
*     int                procNum,	/@ RPC procedure number @/
*     struct sockaddr_in clientAddr,    /@ address of the client @/
*     MOUNTD_ARGUMENT *  mountdArg   	/@ argument of the call @/
*     )
* .CE
* 
* The <authHook> callback must return OK if the request is authorized,
* and any defined NFS error code (usually NFSERR_ACCES) if not.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call mountdInit() from within the kernel 
* protection domain only, and the function referenced in the <authHook> 
* parameter must reside in the kernel protection domain.  
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK, or ERROR if the mount daemon could not be correctly
* initialized.
*/

STATUS mountdInit
    (
    int priority,		/* priority of the mount daemon            */
    int stackSize,		/* stack size of the mount daemon          */
    FUNCPTR authHook,		/* hook to run to authorize each request   */
    int nExports,		/* maximum number of exported file systems */
    int options			/* currently unused - set to 0             */
    )
    {
    if (0 != mountdTaskId)
	{
	printf ("Mount daemon already initialized\n");
	return (ERROR);
	}

    mountdAuthHook = authHook;

    if (nExports > 0)
        mountdNExports = nExports;

    if ((nfsClients = KHEAP_ALLOC (sizeof (*nfsClients))) == NULL)
        return (ERROR);

    if ((clientsSem = semMCreate (SEM_Q_PRIORITY)) == NULL)
	{
	KHEAP_FREE ((char *)nfsClients);
	return (ERROR);
	}

    lstInit (nfsClients);
    
    if ((mountPoints = KHEAP_ALLOC (sizeof (*mountPoints))) == NULL)
            return (ERROR);

    if ((mountSem = semMCreate (SEM_Q_PRIORITY)) == NULL)
        {
        KHEAP_FREE ((char *)mountPoints);
        return (ERROR);
        } 
    lstInit (mountPoints);

    if (priority == 0)
	priority = mountdPriorityDefault;

    if (stackSize == 0)
	stackSize = mountdStackSizeDefault;
    
    if ((mountdTaskId = taskSpawn ("tMountd", priority, 0, stackSize,
				(FUNCPTR) mountd, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
	== ERROR)
	{
	perror ("Could not spawn mountd task");
	return (ERROR);
	}

    return (OK);
    }

/******************************************************************************
*
* mountd - mount daemon process
*
* This routine processes incoming mount daemon requests.  It is
* normally used as the entry point of a task by mountdInit() and
* should not be called directly.  It will only return if svc_run()
* returns, which should only happen if select() returns an error.
* 
* This routine is global only so that its name will appear in the
* task display list.
* 
* Most of this routine was generated by rpcgen.
*
* RETURNS: N/A
* 
* NOMANUAL
*/

void mountd
    (
    void
    )
    {
    register SVCXPRT *transp;
    
    rpcTaskInit ();
    
    (void) pmap_unset(MOUNTPROG, MOUNTVERS);

    transp = svcudp_create(RPC_ANYSOCK);
    if (transp == NULL)
	{
	fprintf(stderr, "cannot create udp service.");
	exit(1);
	}
    
    if (!svc_register(transp, MOUNTPROG, MOUNTVERS, mountdRequestProcess,
		      IPPROTO_UDP))
	{
	fprintf(stderr, "unable to register (MOUNTPROG, MOUNTVERS, udp).");
	exit(1);
	}
    
    svc_run();

    /* Should never reach this point */
    
    fprintf(stderr, "svc_run returned");
    exit(1);
    }

/******************************************************************************
*
* mountdRequestProcess - process mount requests
* 
* This routine is registered from the mountd process via
* svc_register().  It parses the incoming request, decides which
* mountproc_*_1 routine to call, calls it, and sends the reply back
* to the client.
* 
* NOTE:  Most of this routine was generated by rpcgen.
* 
* RETURNS:  N/A.
* 
* NOMANUAL
*/

LOCAL void mountdRequestProcess
    (
    struct svc_req *   rqstp,
    register SVCXPRT * transp
    )
    {
    MOUNTD_ARGUMENT argument;
    char *          result = NULL;
    bool_t          (*xdr_argument)(), (*xdr_result)();
    char *          (*local)();
    nfsstat         authorized;	/* return val of user auth routine */
	
    switch (rqstp->rq_proc)
	{
	case MOUNTPROC_NULL:
	    xdr_argument = xdr_void;
	    xdr_result = xdr_void;
	    local = (char *(*)()) mountproc_null_1;
	    break;

	case MOUNTPROC_MNT:
	    xdr_argument = xdr_dirpath;
	    xdr_result = xdr_fhstatus;
	    local = (char *(*)()) mountproc_mnt_1;
	    break;

	case MOUNTPROC_DUMP:
	    xdr_argument = xdr_void;
	    xdr_result = xdr_mountlist;
	    local = (char *(*)()) mountproc_dump_1;
	    break;

	case MOUNTPROC_UMNT:
	    xdr_argument = xdr_dirpath;
	    xdr_result = xdr_void;
	    local = (char *(*)()) mountproc_umnt_1;
	    break;

	case MOUNTPROC_UMNTALL:
	    xdr_argument = xdr_void;
	    xdr_result = xdr_void;
	    local = (char *(*)()) mountproc_umntall_1;
	    break;

	case MOUNTPROC_EXPORT:
	    xdr_argument = xdr_void;
	    xdr_result = xdr_exports;
	    local = (char *(*)()) mountproc_export_1;
	    break;

	default:
	    svcerr_noproc(transp);
	    return;
	}
    bzero((char *)&argument, sizeof(argument));
    if (!svc_getargs(transp, xdr_argument, &argument)) 
	{
	svcerr_decode(transp);
	return;
	}


    /* Check authorization if hook exists */
    if (mountdAuthHook)
	authorized = (*mountdAuthHook) (MOUNTPROG, MOUNTVERS, rqstp->rq_proc,
					transp->xp_raddr, argument);
    else
	authorized = NFS_OK;

    if (authorized  != NFS_OK)
	{
	switch (rqstp->rq_proc)
	    {
	    /* there's no way to return a mount error for null, dump,
	     * umnt, umntall, or export.  Can only return an empty list.
	     */
	    case MOUNTPROC_NULL:
	    case MOUNTPROC_DUMP:
	    case MOUNTPROC_UMNT:
	    case MOUNTPROC_UMNTALL:
	    case MOUNTPROC_EXPORT:
		result = NULL;
		break;
		
	    /* Can return a real error for mnt */
	    case MOUNTPROC_MNT:
		result =(char *) &authorized;
		break;
	    }
	}
    else
	result = (char *) (*local)(&argument, rqstp);

    if (!svc_sendreply(transp, xdr_result, result)) 
	{
	svcerr_systemerr(transp);
	}
    if (!svc_freeargs(transp, xdr_argument, &argument)) 
	{
	fprintf(stderr, "unable to free arguments");
	exit(1);
	}
    return;
    }

/******************************************************************************
*
* mountproc_null_1 - do nothing
* 
* This procedure does no work.  It is made available in all RPC
* services to allow server response testing and timing.
* 
* NOMANUAL
*/

void * mountproc_null_1
    (
    void
    )
    {
    return (NULL);
    }

/******************************************************************************
*
* mountproc_mnt_1 -
* 
* If the reply "status" is 0, then the reply "directory" contains the
* file handle for the directory "dirname".  This file handle may be
* used in the NFS protocol.  This procedure also adds a new entry to
* the mount list for this client mounting "dirname".
* 
* NOMANUAL
*/

fhstatus * mountproc_mnt_1
    (
    dirpath *        path,
    struct svc_req * rqstp	/* Client information */
    )
    {
    static fhstatus    fstatus;	  /* Status information to return */
    NFS_FILE_HANDLE    fh;	  /* file handle of mount point */
    NFS_EXPORT_ENTRY * mntPoint = NULL;  /* export entry for exported FS */
    char *             reducedPath;      /* path after removing extra dots */
    char *             lastChar;  /* loop variable used to reduce path */
    MOUNT_CLIENT *     newClient; /* info about client mounting directory */

    if (strlen ((char *) path) >= nfsMaxPath)
	return (NULL);

    if ((reducedPath = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    /* Reduce any pathnames that look like "/foo/bar/.." */
    
    strcpy (reducedPath, *path);
    pathCondense (reducedPath);

    /* Any directory under a mount point is also eligible to be mounted.
     * If a requested directory isn't a mount point, see if any of its
     * parents are.
     */
     
    while (strlen (reducedPath) != 0)
	{
	mntPoint = nfsExportFindByName (reducedPath);
	if (mntPoint == NULL)
	    {
	    /* Get a pointer to the last char of reducedPath, and move
	     * to the left until you get to a slash.  Replace the slash
	     * with EOS, then see if that's a mount point.
	     */
	    
	    lastChar = reducedPath + strlen (reducedPath) - 1;
	    while ((lastChar > reducedPath) && (*--lastChar != '/'));
	    *lastChar = EOS;
	    }
	else
	    /* found the mount point */
	    break;
	}

    /* If we still didn't find a mount point, it isn't exported */
    
    if (mntPoint == NULL)
	{
	fstatus.fhs_status = ENOENT;
	return (&fstatus);
	}

    memset (&fh, 0, sizeof (fh));
    fh.volumeId = mntPoint->volumeId;

    if ((fh.inode = fh.fsId = nameToInode (fh.volumeId, *path)) == ERROR)
	{
	fstatus.fhs_status = nfsdErrToNfs (errno);
	return (&fstatus);
	}

    /* See if we have an entry already in the mount list for
     * this client/path pair. If so, then re-use it. Otherwise,
     * allocate a new entry.
     */

    if (mountListFind (rqstp, path) == NULL)
        {
        newClient = KHEAP_ALLOC((sizeof (*newClient) + nfsMaxPath - 1));

        if (NULL == newClient)
  	    {
	    fstatus.fhs_status = ENOMEM;
	    return (&fstatus);
	    }
        else
	    {
	    bzero ((char *)newClient, (sizeof (*newClient)));
	    }

        /* Set client name and directory fields of new client record */
        svc_reqToHostName (rqstp, newClient->clientName);
        strncpy (newClient->directory, *path, nfsMaxPath - 1);

        /* Add to the client list */

        semTake (clientsSem, WAIT_FOREVER);
        lstAdd (nfsClients, (NODE *) newClient);
        semGive (clientsSem);
        }

    /* Convert outgoing file handle to network byte order */
    nfsdFhHton (&fh);
    
    memcpy (&fstatus.fhstatus_u.fhs_fhandle, &fh, FHSIZE);
    fstatus.fhs_status = 0;
    return (&fstatus);
    }

/******************************************************************************
*
* mountproc_dump_1 -
* Returns the list of remote mounted file systems.  The "mountlist"
* contains one entry for each "hostname" and "directory" pair.
* 
* NOMANUAL
*/

mountlist * mountproc_dump_1
    (
    void
    )
    {
    static mountlist         mountList = NULL;
    int                      lstPos = 0;
    MOUNT_CLIENT *            mountEntry;
    MOUNT_CLIENT *            nextEntry;
    char *                   strTbl;
    int                      nClients;

    semTake (clientsSem, WAIT_FOREVER);

    nClients = lstCount (nfsClients);

    /* If there are no clients, free the semaphore and memory, return NULL */
    
    if (nClients == 0)
        {
	semGive (clientsSem);

	/* if mountList has been allocated, free it */
	if (mountList != NULL)
	    KHEAP_FREE ((char *)mountList);

        mountList = NULL;

	return ((mountlist *) &mountList);
	}
	
    if (mountList == NULL)
	{
	mountList = KHEAP_ALLOC (((nClients *
			(sizeof(mountbody) + nfsMaxPath + MAXHOSTNAMELEN+2))));
	if (NULL != mountList)
	    {
	    bzero ((char *)mountList,((nClients *
                        (sizeof(mountbody) + nfsMaxPath + MAXHOSTNAMELEN+2))));
	    }
	}
    else
	{
        mountList = KHEAP_REALLOC((char *)mountList,  (unsigned)
				((sizeof (mountbody) + nfsMaxPath +
					 MAXHOSTNAMELEN + 2) * nClients));
	}
    
    if (mountList == NULL)
        return ((mountlist *) &mountList);
    
    strTbl = (char *) &mountList[lstCount(nfsClients)];

    for (mountEntry =  (MOUNT_CLIENT *) lstFirst (nfsClients);
	 mountEntry != NULL;
	 mountEntry = (MOUNT_CLIENT *) lstNext ((NODE *) mountEntry))
	{
	nextEntry = (MOUNT_CLIENT *) lstNext ((NODE *) mountEntry);

	mountList [lstPos].ml_hostname = strTbl;
	strncpy (strTbl, mountEntry->clientName, MAXHOSTNAMELEN);
	strTbl [MAXHOSTNAMELEN] = EOS;
	strTbl += strlen (strTbl) + 1;
	strTbl = (char *) MEM_ROUND_UP (strTbl);

	mountList [lstPos].ml_directory = strTbl;
	strncpy (strTbl, mountEntry->directory, nfsMaxPath - 1);
	strTbl [nfsMaxPath - 1] = EOS;
	strTbl += strlen (strTbl) + 1;
	strTbl = (char *) MEM_ROUND_UP (strTbl);

	if (nextEntry != NULL)
	    mountList [lstPos].ml_next = &mountList [lstPos + 1];
	else
	    mountList [lstPos].ml_next = NULL;
	
	lstPos++;
	}
    
    semGive (clientsSem);
    return ((mountlist *) &mountList);
    }

/******************************************************************************
*
* mountproc_umnt_1 -
* 
* Removes the mount list entry for the input "dirpath".
* 
* NOMANUAL
*/

void * mountproc_umnt_1
    (
    dirpath *        path,
    struct svc_req * rqstp	/* Client information */
    )
    {
    MOUNT_CLIENT *   clientEntry;

    /* See if we have an entry for this client/path pair
     * in the mount list, and if so, remove it.
     */

    if ((clientEntry = mountListFind (rqstp, path)) != NULL)
        {
        semTake (clientsSem, WAIT_FOREVER);
        lstDelete (nfsClients, (NODE *) clientEntry);
        KHEAP_FREE (clientEntry);
        semGive (clientsSem);
        }

    return ((void *) TRUE);
    }

/******************************************************************************
*
* mountproc_umntall_1 -
* Removes all of the mount list entries for this client.
* 
* NOMANUAL
*/

void * mountproc_umntall_1
    (
    char *           nothing,	/* unused */
    struct svc_req * rqstp	/* Client information */
    )
    {
    MOUNT_CLIENT *   clientEntry;
    char             clientName [MAXHOSTNAMELEN + 1];
    MOUNT_CLIENT *   deleteThis;
    BOOL             completePass = FALSE;

    semTake (clientsSem, WAIT_FOREVER);
    while (!completePass)
	{
	deleteThis = NULL;
	for (clientEntry = (MOUNT_CLIENT *) lstFirst (nfsClients);
	     clientEntry != NULL;
	     clientEntry = (MOUNT_CLIENT *) lstNext ((NODE *) clientEntry))
	    {
	    svc_reqToHostName (rqstp, clientName);
	    if (strncmp(clientName, clientEntry->clientName, MAXHOSTNAMELEN)
		== 0)
		{
		/* Found it */
		deleteThis = clientEntry;
		break;
		}
	    }
	completePass = (clientEntry == NULL);
	if (deleteThis)
	    {
	    lstDelete (nfsClients, (NODE *) deleteThis);
            KHEAP_FREE (deleteThis); 
	    }
	}
    semGive (clientsSem);
    return ((void *) TRUE);
    }

/******************************************************************************
*
* mountproc_export_1 -
* Returns a variable number of export list entries.  Each entry
* contains a file system name and a list of groups that are allowed to
* import it.  The file system name is in "filesys", and the group name
* is in the list "groups".
* 
* NOMANUAL
*/

exports * mountproc_export_1
    (
    void
    )
    {
    static exportnode *      exportList = NULL;
    int                      lstPos = 0;
    NFS_EXPORT_ENTRY *       exportEntry;
    NFS_EXPORT_ENTRY *       nextEntry;
    static struct groupnode  aGroup;
    char *                   strTbl;

    /* If there are no exported file systems, return NULL 
     * The check for exportList removes any stale exportList that 
     * are present  
     */

    if(mountPoints == NULL || lstCount(mountPoints) == 0) 
      {
      if(exportList != NULL) 
        {
	KHEAP_FREE((char *)exportList);
	exportList = NULL;
        }
      return (&exportList);
    }

    if (exportList == NULL)
	{
	exportList = KHEAP_ALLOC((mountdNExports * 
			(sizeof (exportnode) + nfsMaxPath)));
	if (exportList == NULL)
	    {
	    return (&exportList);
	    }
	else
	    {
	    bzero ((char *)exportList,(mountdNExports *
                        (sizeof (exportnode) + nfsMaxPath)));
	    }
	}

    strTbl = (char *) &exportList[mountdNExports];
    
    /* Bogus group information */
    aGroup.gr_name = NULL;
    aGroup.gr_next = NULL;

    semTake (mountSem, WAIT_FOREVER);
    for (exportEntry =  (NFS_EXPORT_ENTRY *) lstFirst (mountPoints);
	 exportEntry != NULL;
	 exportEntry = (NFS_EXPORT_ENTRY *) lstNext ((NODE *) exportEntry))
	{
	nextEntry = (NFS_EXPORT_ENTRY *) lstNext ((NODE *) exportEntry);
	exportList [lstPos].ex_dir = strTbl;
	strncpy (strTbl, exportEntry->dirName, nfsMaxPath - 1);
	strTbl += strlen (strTbl);
	*strTbl++ = EOS;
	exportList [lstPos].ex_groups = NULL;
	if (nextEntry != NULL)
	    exportList [lstPos].ex_next = &exportList [lstPos + 1];
	else
	    exportList [lstPos].ex_next = NULL;
	lstPos++;
	}
    
    semGive (mountSem);
    return ((exports *) &exportList);
    }

/******************************************************************************
*
* nfsExport - specify a file system to be NFS exported
*
* This routine makes a file system available for mounting by a client.
* The client should be in the local host table (see hostAdd()),
* although this is not required.
* 
* The <id> parameter can either be set to a specific value, or to 0.
* If it is set to 0, an ID number is assigned sequentially.
* Every time a file system is exported, it must have the same ID
* number, or clients currently mounting the file system will not be
* able to access files.
*
* To display a list of exported file systems, use:
* .CS
*     -> nfsExportShow "localhost"
* .CE
* 
* RETURNS: OK, or ERROR if the file system could not be exported.
*
* SEE ALSO: nfsLib, nfsExportShow(), nfsUnexport()
*/

STATUS nfsExport
    (
    char * directory,	 /* Directory to export - FS must support NFS */
    int    id,   	 /* ID number for file system */
    BOOL   readOnly,     /* TRUE if file system is exported read-only */
    int    options	 /* Reserved for future use - set to 0 */
    )
    {
    NFS_EXPORT_ENTRY * exportFs; /* The file system to be exported */
    int		       exportDirFd; /* File descriptor for the exported dir */
    struct stat statInfo;	/* information from fstat() call */

    if (strlen (directory) >= nfsMaxPath)
	return (ERROR);

    /* Create the export point */
    
    if ((exportFs = KHEAP_ALLOC (sizeof (*exportFs) + nfsMaxPath)) == NULL)
        return (ERROR);

    if ((exportDirFd = open (directory, O_RDONLY, 0666)) == ERROR)
        {
	KHEAP_FREE ((char *)exportFs);
        return (ERROR);
	}

    /* Set up the NFS_EXPORT_ENTRY structure */
    
    strncpy (exportFs->dirName, directory, nfsMaxPath - 1);
    exportFs->dirFd = exportDirFd;

    /* Set the ID number for the exported system */
    
    if (id == 0)
        exportFs->volumeId = fsIdNumber++;
    else
        exportFs->volumeId = id;

    exportFs->readOnly = readOnly;

    /* Add it to the list */
    semTake (mountSem, WAIT_FOREVER);
    lstAdd (mountPoints, (NODE *)exportFs);
    semGive (mountSem);

    if (fstat (exportDirFd, &statInfo) != OK)
	{
	semTake (mountSem, WAIT_FOREVER);
	lstDelete (mountPoints, (NODE *)exportFs);
	semGive (mountSem);

        KHEAP_FREE ((char *) exportFs);
	close (exportDirFd);
        return (ERROR);
	}
    else
	{
        nfsHashTblSet (exportFs->dirName, exportFs->volumeId, statInfo.st_dev);
	}

    if ((exportFs->fsId = nameToInode (exportFs->volumeId, directory)) == ERROR)
	{
	semTake (mountSem, WAIT_FOREVER);
	lstDelete (mountPoints, (NODE *) exportFs);
	semGive (mountSem);

        nfsHashTblUnset (exportFs->dirName, exportFs->volumeId);  
	KHEAP_FREE ((char *)exportFs);
	close (exportDirFd);
        return (ERROR);
	}
    return (OK);
    }

/******************************************************************************
*
* nfsUnexport - remove a file system from the list of exported file systems
*
* This routine removes a file system from the list of file systems
* exported from the target.  Any client attempting to mount a file
* system that is not exported will receive an error (NFSERR_ACCESS).
* 
* RETURNS: OK, or ERROR if the file system could not be removed from
* the exports list.
* 
* ERRNO: ENOENT
*
* SEE ALSO: nfsLib, nfsExportShow(), nfsExport()
*/

STATUS nfsUnexport
    (
    char * dirName		/* Name of the directory to unexport */
    )
    {
    NFS_EXPORT_ENTRY * mntPoint; /* Mount point from mountPoints list */

    /* Make sure the directory name is mentioned */

    if ((dirName == NULL) || (strlen (dirName) >= nfsMaxPath))
	return(ERROR);
    
    /* Make sure there's a list of file systems */
    if (mountPoints == NULL)
        return (ERROR);

    /* Find it and take it off the list */
    if ((mntPoint = nfsExportFindByName (dirName)) != NULL)
        {
	semTake (mountSem, WAIT_FOREVER);
	lstDelete (mountPoints, (NODE *) mntPoint);
        nfsHashTblUnset (mntPoint->dirName, mntPoint->volumeId);
	semGive (mountSem);
	close(mntPoint->dirFd);
        KHEAP_FREE ((char *)mntPoint);
	return (OK);
	}
    else
	{
	/* Didn't find it, set errno and return ERROR */
	semGive (mountSem);
	errno = ENOENT;
	return (ERROR);
	}
    }

/******************************************************************************
*
* nfsExportFindByName - find a pointer to an exported file system
*
* Given a directory name, return a pointer to the corresponding
* NFS_EXPORT_ENTRY structure.
* 
* RETURNS:  OK, or NULL if the file system could not be found.
* 
* NOMANUAL 
*/

NFS_EXPORT_ENTRY * nfsExportFindByName
    (
    char * dirName		/* name of file system to find */
    )
    {
    NFS_EXPORT_ENTRY * mntPoint = NULL;	/* mount point from mountPoints list */
    char *             matchName;       /* fs name to find */

    /* Make sure the list exists */
    if (mountPoints == NULL)
        return (NULL);
    
    if ((matchName = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    /* Make sure matchName doesn't have ending slash */
    strcpy (matchName, dirName);
    if (matchName[strlen (matchName) - 1] == '/')
        matchName[strlen (matchName) - 1] = EOS;
    
    /* Search through the list */
    semTake (mountSem, WAIT_FOREVER);
    for (mntPoint = (NFS_EXPORT_ENTRY *) lstFirst (mountPoints);
	 mntPoint != NULL;
	 mntPoint = (NFS_EXPORT_ENTRY *) lstNext ((NODE *) mntPoint))
	{
	if (strcmp(mntPoint->dirName, matchName) == 0)
	    {
	    /* Found it, give the semaphore and return */
	    semGive (mountSem);
	    return (mntPoint);
	    }
	}

    /* Didn't find a match, return NULL */
    semGive (mountSem);
    return (NULL);
    }

/******************************************************************************
*
* nfsExportFindById - find a pointer to an exported file system
* 
* Given a directory ID number, return a pointer to the corresponding
* NFS_EXPORT_ENTRY structure.
* 
* RETURNS:  OK, or NULL if the file system could not be found.
* 
* NOMANUAL
*/

NFS_EXPORT_ENTRY * nfsExportFindById
    (
    int volumeId		/* volume ID number */
    )
    {
    NFS_EXPORT_ENTRY * mntPoint; /* mount point from mountPoints list */

    /* Make sure the list exists */
    if (mountPoints == NULL)
        return (NULL);
    
    /* Search through the list */
    semTake (mountSem, WAIT_FOREVER);
    for (mntPoint = (NFS_EXPORT_ENTRY *) lstFirst (mountPoints);
	 mntPoint != NULL;
	 mntPoint = (NFS_EXPORT_ENTRY *) lstNext ((NODE *) mntPoint))
	{
	if (volumeId == mntPoint->volumeId)
	    {
	    /* Found it, give the semaphore and return */
	    semGive (mountSem);
	    return (mntPoint);
	    }
	}

    /* Didn't find a match, return NULL */
    semGive (mountSem);
    return (NULL);
    }

/******************************************************************************
*
* nameToInode - file name to inode number
* 
* Given a file name, return the inode number associated with it.
* 
* RETURNS: the inode number for the file descriptor, or ERROR.
*
* NOMANUAL
*/

int nameToInode
    (
    int    mntId,               /* mount id */
    char * fileName		/* name of the file */
    )
    {
    int    fd;			/* File descriptor */
    int    inode;		/* inode of file */

    
    if ((fd = open (fileName, O_RDONLY, 0)) == ERROR)
        return (ERROR);
    else
	{
	inode = nfsNmLkupIns (mntId, fileName);
	}

    close (fd);
    return (inode);
    }

/******************************************************************************
*
* svc_reqToHostName - convert svc_req struct to host name
* 
* Given a pointer to a svc_req structure, fill in a string with the
* host name of the client associated with that svc_req structure.
* The calling routine is responsible for allocating space for the
* name string, which should be NAME_MAX characters long.
*
* RETURNS:  A pointer to the name of the client.
* 
* NOMANUAL
*/

LOCAL char * svc_reqToHostName
    (
    struct svc_req * rqstp,	/* Client information */
    char *           hostName	/* String to fill in with client name */
    )
    {
    /* Convert the callers address to a host name.  If hostGetByAddr()
     * returns ERROR, convert the address to a numeric string
     */
    if (hostGetByAddr (rqstp->rq_xprt->xp_raddr.sin_addr.s_addr, hostName)!= OK)
        inet_ntoa_b (rqstp->rq_xprt->xp_raddr.sin_addr, hostName);
    return (hostName);
    }

/******************************************************************************
*
* mountListFind - find an entry in the NFS mount list
*
* Given a pointer to a svc_req structure and a path name, search
* the client mount list to see if the client already has this
* particular path mounted.
*
* RETURNS:  A pointer to the mount list entry which we found,
*           or NULL if there was no match.
* 
* NOMANUAL
*/

LOCAL MOUNT_CLIENT * mountListFind
    (
    struct svc_req * rqstp,	/* Client information */
    dirpath *        path	/* path to search for */
    )
    {
    MOUNT_CLIENT *   clientEntry;
    char             clientName [MAXHOSTNAMELEN + 1];

    semTake (clientsSem, WAIT_FOREVER);
    for (clientEntry = (MOUNT_CLIENT *) lstFirst (nfsClients);
         clientEntry != NULL;
         clientEntry = (MOUNT_CLIENT *) lstNext ((NODE *) clientEntry))
        {
        svc_reqToHostName (rqstp, clientName);
        if ((strncmp(clientName, clientEntry->clientName, MAXHOSTNAMELEN) == 0)
            && strncmp (*path, clientEntry->directory, PATH_MAX) == 0)
            {
            semGive (clientsSem);
            return (clientEntry);
            }
        }
    semGive (clientsSem);
    return(NULL);
    }


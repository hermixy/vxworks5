/* nfsdLib.c - Network File System (NFS) server library */

/* Copyright 1994 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01s,07may02,kbw  man page edits
01r,06nov01,vvv  made max path length configurable (SPR #63551)
01q,15oct01,rae  merge from truestack ver 01r, base 01o (cleanup)
01p,21jun00,rsh  upgrade to dosFs 2.0
01o,05apr00,zl   fixed the fix in 01m (use retVal)
01m,15nov98,rjc  modifications for dosfs2 compatibility.
01o,03dec00,ijm  increased nfsdStackSize (SPR# 22650).  Corrected return
		 value if file exists (SPR# 31536)
01n,16mar99,spm  recovered orphaned code from tor1_0_1.sens1_1 (SPR #25770)
01m,06oct98,sgv  fixed nfsproc_create_2 to return NFSERR_EXIST when the file
                 exists
01m,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01l,11mar97,dvs  added test for udp in nfsd (SPR #8183).
01k,10feb97,dbt  close file descriptors before leaving in nfsproc_setattr_2 () 
		 routine before leaving (SPR #6615).
01j,05jul96,ms   fixed SPR 6579 - only NFSPROC_CREATE if file doen't exist.
01i,27feb95,jdi  doc: changed dosFsMode to dosFsFileMode (doc only) (SPR 4085).
01h,11feb95,jdi  doc format repair.
01g,24jan95,jdi  doc cleanup.
01f,08sep94,jmm  final changes
01e,11may94,jmm  integrated Roland's doc changes
01d,25apr94,jmm  added nfsdFsReadOnly(), all routines that write now call it 1st
                 added module documentation
01c,21apr94,jmm  reordered routines, documentation cleanup
01b,20apr94,jmm  formatting cleanup, nfsdFhNtoh and Hton calls fixed
01a,07mar94,jmm  written
*/

/*
DESCRIPTION
This library is an implementation of version 2 of the Network File
System Protocol Specification as defined in RFC 1094.  It is
closely connected with version 1 of the mount protocol, also defined
in RFC 1094 and implemented in turn by mountLib.

The NFS server is initialized by calling nfsdInit().  
This is done automatically at boot time if INCLUDE_NFS_SERVER is defined.

Currently, only the dosFsLib file system is supported.
File systems are exported with the nfsExport() call.

To create and export a file system, define INCLUDE_NFS_SERVER and rebuild
VxWorks.

To export VxWorks file systems via NFS, you need facilities from both
this library and from mountLib.  To include both, define
INCLUDE_NFS_SERVER and rebuild VxWorks.

Use the mountLib routine nfsExport() to export file systems.  For an
example, see the manual page for mountLib.

VxWorks does not normally provide authentication services for NFS
requests, and the DOS file system does not provide file permissions.
If you need to authenticate incoming requests, see the documentation
for nfsdInit() and mountdInit() for information about authorization
hooks.

The following requests are accepted from clients.  For details of
their use, see RFC 1094, "NFS: Network File System Protocol
Specification."

.TS
center,tab(|);
lf3 lf3
l n.
Procedure Name        |  Procedure Number
_
 NFSPROC_NULL         |  0
 NFSPROC_GETATTR      |  1
 NFSPROC_SETATTR      |  2
 NFSPROC_ROOT         |  3
 NFSPROC_LOOKUP       |  4
 NFSPROC_READLINK     |  5
 NFSPROC_READ         |  6
 NFSPROC_WRITE        |  8
 NFSPROC_CREATE       |  9
 NFSPROC_REMOVE       |  10
 NFSPROC_RENAME       |  11
 NFSPROC_LINK         |  12
 NFSPROC_SYMLINK      |  13
 NFSPROC_MKDIR        |  14
 NFSPROC_RMDIR        |  15
 NFSPROC_READDIR      |  16
 NFSPROC_STATFS       |  17
.TE

AUTHENTICATION AND PERMISSIONS
Currently, no authentication is done on NFS requests.  nfsdInit()
describes the authentication hooks that can be added should
authentication be necessary.

Note that the DOS file system does not provide information about ownership
or permissions on individual files.  Before initializing a dosFs file
system, three global variables--`dosFsUserId', `dosFsGroupId', and
`dosFsFileMode'--can be set to define the user ID, group ID, and permissions
byte for all files in all dosFs volumes initialized after setting these
variables.  To arrange for different dosFs volumes to use different user
and group ID numbers, reset these variables before each volume is
initialized.  See the manual entry for dosFsLib for more information.

TASKS
Several NFS tasks are created by nfsdInit().  They are:
.iP tMountd 11 3
The mount daemon, which handles all incoming mount requests.
This daemon is created by mountdInit(), which is automatically
called from nfsdInit().
.iP tNfsd
The NFS daemon, which queues all incoming NFS requests.
.iP tNfsdX
The NFS request handlers, which dequeues and processes all incoming
NFS requests.
.LP

Performance of the NFS file system can be improved by increasing the
number of servers specified in the nfsdInit() call, if there are
several different dosFs volumes exported from the same target system.
The spy() utility can be called to determine whether this is useful for
a particular configuration.

INTERNAL:
All of the nfsproc_*_2 routines follow the RPC naming convention
rather than the WRS naming convention.  They return a pointer to
malloced space, which is freed by nfsdRequestProcess().

*/

#include "vxWorks.h"
#include "dirent.h"
#include "dosFsLib.h"
#include "errno.h"
#include "fcntl.h"
#include "ioLib.h"
#include "limits.h"
#include "mountLib.h"
#include "msgQLib.h"
#include "netinet/in.h"
#include "nfsdLib.h"
#include "pathLib.h"
#include "rpcLib.h"
#include "rpc/pmap_clnt.h"
#include "rpc/rpc.h"
#include "semLib.h"
#include "sockLib.h"
#include "stdio.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/socket.h"
#include "sys/stat.h"
#include "sys/types.h"
#include "taskLib.h"
#include "tickLib.h"
#include "utime.h"
#include "xdr_nfs.h"
#include "iosLib.h"
#include "private/nfsHashP.h"
#include "memPartLib.h"

/* defines */
#define NAME_LEN               40
#if !defined(S_dosFsLib_FILE_EXISTS)
#define S_dosFsLib_FILE_EXISTS  (S_dosFsLib_FILE_ALREADY_EXISTS)
#endif
#define QLEN_PER_SRVR           10

#ifdef __GNUC__
# ifndef alloca
#  define alloca __builtin_alloca
# endif
#endif

/* DATA STRUCTURES */

/* svcudp_data is copied directly from svc_udp.c - DO NOT MODIFY */

struct svcudp_data {
	u_int   su_iosz;	/* byte size of send.recv buffer */
	u_long	su_xid;		/* transaction id */
	XDR	su_xdrs;	/* XDR handle */
	char	su_verfbody[MAX_AUTH_BYTES];	/* verifier body */
	char   *su_cache;	/* cached data, NULL if no cache - 4.0 */
};

typedef struct
    {
    FUNCPTR            routine;   /* nfsproc_*_2 routine to call */
    int                procNum;   /* NFS procedure number */
    NFSD_ARGUMENT *    argument;  /* argument to pass to nfsproc_*_2 */
    FUNCPTR            xdrArg;	  /* XDR function pointer to convert argument */
    FUNCPTR            xdrResult; /* XDR function pointer to convert result */
    struct sockaddr_in sockAddr;  /* Address of the client socket */
    int                xid;	  /* RPC XID of client request */
    int                socket;	  /* Socket to use to send reply */
    } NFS_Q_REQUEST;

/* IMPORTS */

IMPORT int nfsMaxPath;           /* Max. file path length */

/* GLOBALS */

int nfsdStackSize = 14000;	  /* Default stack size for NFS processes */
int nfsdNServers = 4;		  /* Default number of NFS servers */
int nfsdPriorityDefault = 55;	  /* Default priority of the NFS server */
int nfsdNFilesystemsDefault = 10; /* Default max. num. filesystems */

/* LOCALS */

LOCAL NFS_SERVER_STATUS nfsdServerStatus;    /* Status of the NFS server */
LOCAL FUNCPTR           nfsdAuthHook = NULL; /* Authentication hook */
MSG_Q_ID                nfsRequestQ;         /* Message Q for NFS requests */

/* forward LOCAL functions */

LOCAL void nfsdRequestEnqueue (struct svc_req * rqstp, SVCXPRT * transp);
LOCAL int nfsdFsReadOnly (NFS_FILE_HANDLE * fh);

/******************************************************************************
*
* nfsdInit - initialize the NFS server
* 
* This routine initializes the NFS server.  <nServers>  specifies the number of
* tasks to be spawned to handle NFS requests.  <priority> is the priority that
* those tasks will run at.  <authHook> is a pointer to an authorization
* routine.  <mountAuthHook> is a pointer to a similar routine, passed to
* mountdInit().  <options> is provided for future expansion.
* 
* Normally, no authorization is performed by either mountd or nfsd.
* If you want to add authorization, set <authHook> to a
* function pointer to a routine declared as follows:
* .CS
* nfsstat routine
*     (
*     int                progNum,	/@ RPC program number @/
*     int                versNum,	/@ RPC program version number @/
*     int                procNum,	/@ RPC procedure number @/
*     struct sockaddr_in clientAddr,    /@ address of the client @/
*     NFSD_ARGUMENT *    nfsdArg   	/@ argument of the call @/
*     )
* .CE
* 
* The <authHook> routine should return NFS_OK if the request is authorized,
* and NFSERR_ACCES if not.  (NFSERR_ACCES is not required; any legitimate
* NFS error code can be returned.)
* 
* See mountdInit() for documentation on <mountAuthHook>.  Note that
* <mountAuthHook> and <authHook> can point to the same routine.
* Simply use the <progNum>, <versNum>, and <procNum> fields to decide
* whether the request is an NFS request or a mountd request.
* 
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  In addition, all arguments to this function can  
* reference only that data which is valid in the kernel protection domain. 
* This restriction does not apply under non-AE versions of VxWorks.  
*
* RETURNS: OK, or ERROR if the NFS server cannot be started.
*
* SEE ALSO: nfsExport(), mountdInit()
*/

STATUS nfsdInit
    (
    int nServers,		/* the number of NFS servers to create */
    int nExportedFs,		/* maximum number of exported file systems */
    int priority,		/* the priority for the NFS servers */
    FUNCPTR authHook,		/* authentication hook */
    FUNCPTR mountAuthHook,	/* authentication hook for mount daemon */
    int options			/* currently unused */
    )
    {
    char serverName [50];	/* Synthetic name for NFS servers */
    int  mountTask;		/* taskId of the mountd task */
    int  queuingTask;		/* taskId of the task that queues NFS calls */

    /*
     * Scaling stack with change in NFS_MAXPATH since multiple arrays of size 
     * NFS_MAXPATH are allocated from stack. Difference is computed from 
     * default max. path of 255.
     */

    nfsdStackSize += 4 * (nfsMaxPath - 255);

    /* Set up call statistics */
    memset (&nfsdServerStatus, 0, sizeof (nfsdServerStatus));

    /* Set up authorization hook */
    nfsdAuthHook = authHook;
    
    /* If number and priority of servers isn't specified, set it to default */
    
    if (nServers == 0)
        nServers = nfsdNServers;

    if (priority == 0)
        priority = nfsdPriorityDefault;

    if (nExportedFs == 0)
        nExportedFs = nfsdNFilesystemsDefault;

    /* Create the request queue */
    if ((nfsRequestQ = msgQCreate (nServers * QLEN_PER_SRVR, sizeof (NFS_Q_REQUEST),
			      MSG_Q_FIFO)) == NULL)
	return (ERROR);

    /* Spawn the mount task */

    if ((mountTask = mountdInit (0, 0, mountAuthHook, 0, 0)) == ERROR)
	{
	msgQDelete (nfsRequestQ);
	return (ERROR);
	}

    /* spawn the queuing task */
    
    if ((queuingTask = taskSpawn ("tNfsd", priority, VX_FP_TASK, nfsdStackSize,
	 			  (FUNCPTR) nfsd, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
	== ERROR)
	{
	taskDelete (mountTask);
	msgQDelete (nfsRequestQ);
	return (ERROR);
	}

    /* spawn the call processing tasks */
    
    while (nServers-- > 0)
	{
	/* Create names of the form tNfsdX */
	sprintf (serverName, "tNfsd%d", nServers);
	
	if ((taskSpawn (serverName, priority + 5, VX_FP_TASK, nfsdStackSize,
			(FUNCPTR) nfsdRequestProcess, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0)) == ERROR)
	    {
	    taskDelete (mountTask);
	    taskDelete (queuingTask);

	    msgQDelete (nfsRequestQ);
	    return (ERROR);
	    }
	}

    return (OK);
    }

/******************************************************************************
*
* nfsdRequestEnqueue - queue up NFS requests from clients
*
* Called only via svc_run(), via nfsd().  Puts an incoming NFS request
* into the message queue for processing by nfsdRequestProcess().
*
* Some of this routine was generated by rpcgen.
* 
* RETURNS:  N/A
* 
* NOMANUAL
*/

LOCAL void nfsdRequestEnqueue
    (
    struct svc_req * rqstp,	/* Request */
    SVCXPRT *        transp	/* Transport */
    )
    {
    NFSD_ARGUMENT *     argument;     /* Client call argument */
    FUNCPTR             xdr_result;   /* XDR result conversion routine */
    FUNCPTR             xdr_argument; /* XDR argument conversion routine */
    char *              (*local)();   /* Local routine to call */
    NFS_Q_REQUEST       request;      /* Request to queue up */

    /* Allocate space for the argument */
    argument = KHEAP_ALLOC((NFS_MAXDATA + sizeof (argument) + nfsMaxPath));

    if (NULL != argument)
	{
	bzero((char *) argument,(NFS_MAXDATA + sizeof (argument) + nfsMaxPath));
	}

    /* Beginning of code generated by rpcgen */

    switch (rqstp->rq_proc) {
    case NFSPROC_NULL:
	    xdr_argument = xdr_void;
	    xdr_result = xdr_void;
	    local = (char *(*)()) nfsproc_null_2;
	    break;

    case NFSPROC_GETATTR:
	    xdr_argument = xdr_nfs_fh;
	    xdr_result = xdr_attrstat;
	    local = (char *(*)()) nfsproc_getattr_2;
	    break;

    case NFSPROC_SETATTR:
	    xdr_argument = xdr_sattrargs;
	    xdr_result = xdr_attrstat;
	    local = (char *(*)()) nfsproc_setattr_2;
	    break;

    case NFSPROC_ROOT:
	    xdr_argument = xdr_void;
	    xdr_result = xdr_void;
	    local = (char *(*)()) nfsproc_root_2;
	    break;

    case NFSPROC_LOOKUP:
	    xdr_argument = xdr_diropargs;
	    xdr_result = xdr_diropres;
	    local = (char *(*)()) nfsproc_lookup_2;
	    break;

    case NFSPROC_READLINK:
	    xdr_argument = xdr_nfs_fh;
	    xdr_result = xdr_readlinkres;
	    local = (char *(*)()) nfsproc_readlink_2;
	    break;

    case NFSPROC_READ:
	    xdr_argument = xdr_readargs;
	    xdr_result = xdr_readres;
	    local = (char *(*)()) nfsproc_read_2;
	    break;

    case NFSPROC_WRITECACHE:
	    xdr_argument = xdr_void;
	    xdr_result = xdr_void;
	    local = (char *(*)()) nfsproc_writecache_2;
	    break;

    case NFSPROC_WRITE:
	    xdr_argument = xdr_writeargs;
	    xdr_result = xdr_attrstat;
	    local = (char *(*)()) nfsproc_write_2;
	    break;

    case NFSPROC_CREATE:
	    xdr_argument = xdr_createargs;
	    xdr_result = xdr_diropres;
	    local = (char *(*)()) nfsproc_create_2;
	    break;

    case NFSPROC_REMOVE:
	    xdr_argument = xdr_diropargs;
	    xdr_result = xdr_nfsstat;
	    local = (char *(*)()) nfsproc_remove_2;
	    break;

    case NFSPROC_RENAME:
	    xdr_argument = xdr_renameargs;
	    xdr_result = xdr_nfsstat;
	    local = (char *(*)()) nfsproc_rename_2;
	    break;

    case NFSPROC_LINK:
	    xdr_argument = xdr_linkargs;
	    xdr_result = xdr_nfsstat;
	    local = (char *(*)()) nfsproc_link_2;
	    break;

    case NFSPROC_SYMLINK:
	    xdr_argument = xdr_symlinkargs;
	    xdr_result = xdr_nfsstat;
	    local = (char *(*)()) nfsproc_symlink_2;
	    break;

    case NFSPROC_MKDIR:
	    xdr_argument = xdr_createargs;
	    xdr_result = xdr_diropres;
	    local = (char *(*)()) nfsproc_mkdir_2;
	    break;

    case NFSPROC_RMDIR:
	    xdr_argument = xdr_diropargs;
	    xdr_result = xdr_nfsstat;
	    local = (char *(*)()) nfsproc_rmdir_2;
	    break;

    case NFSPROC_READDIR:
	    xdr_argument = xdr_readdirargs;
	    xdr_result = xdr_readdirres;
	    local = (char *(*)()) nfsproc_readdir_2;
	    break;

    case NFSPROC_STATFS:
	    xdr_argument = xdr_nfs_fh;
	    xdr_result = xdr_statfsres;
	    local = (char *(*)()) nfsproc_statfs_2;
	    break;

    default:
            KHEAP_FREE((char *)argument);
	    svcerr_noproc(transp);
	    return;
    }

    if (!svc_getargs(transp, xdr_argument, argument)) {
	svcerr_decode(transp);
	return;
    }
    
    /* end of code generated by rpcgen */
    
    /* Build the request and put it into the message queue */
    
    request.routine   = (FUNCPTR) local;
    request.procNum   = rqstp->rq_proc;
    request.argument  = argument;
    request.xdrArg    = xdr_argument;
    request.xdrResult = xdr_result;
    request.sockAddr  = transp->xp_raddr;

    /* only use xp_p2 for udp requests */
    if (transp->xp_p2 != NULL)	
	request.xid       = ((struct svcudp_data *) transp->xp_p2)->su_xid;


    request.socket    = transp->xp_sock;
    
    if (msgQSend (nfsRequestQ, (char *) &request, sizeof (request),
		  WAIT_FOREVER, MSG_PRI_NORMAL) != OK)
	{
	KHEAP_FREE((char *)argument);
	perror ("nfsd aborted");
	return;
	}

    }

/******************************************************************************
*
* nfsdRequestProcess - process client NFS requests
* 
* nfsdRequestProcess is started as a task by nfsdInit().  It sits in a loop
* reading nfsdRequestQ and responding to all requests that are put into this
* queue.
* 
* Multiple processes can be spawned with this entry point.  Normally,
* nfsdNServers are started when nfsdInit() is called.
* 
* RETURNS:  N/A
*
* NOMANUAL
*/

void nfsdRequestProcess
    (
    void
    )
    {
    SVCXPRT *     transp;	/* Dummy transport for RPC */
    int           sock;		/* Transmission socket for reply */
    NFS_Q_REQUEST request;	/* Client request */
    NFSD_ARGUMENT * argument; /* Argument from client request */
    char *        result;	/* Result of NFS call */
    FUNCPTR       xdr_argument;	/* XDR argument conversion routine */
    FUNCPTR       xdr_result;	/* XDR result conversion routine */
    FUNCPTR       local;	/* Local NFS routine to call */
    struct sockaddr_in * addr;	/* Address of client socket */
    int           xid;		/* XID from client request */
    nfsstat       authorized;	/* request is authorized if == NFS_OK*/
    
    /* Initialize task for VxWorks RPC work */
    
    rpcTaskInit ();

    transp = svcudp_create(-1);

    /* Work loop is repeated until catastrophic error encountered */
    
    FOREVER
	{
	/* Get the next message */
	
	if (msgQReceive (nfsRequestQ, (char *) &request, sizeof(request),
			 WAIT_FOREVER) == ERROR)
	    {
	    perror ("NFS server aborted abnormally");
	    return;
	    }

	/* Break down the incoming message */
	
	local        = request.routine;
	argument     = request.argument;
	xdr_argument = request.xdrArg;  
	xdr_result   = request.xdrResult;
	addr         = &request.sockAddr;
	xid          = request.xid;
	sock         = request.socket;

	/* Create a new RPC transport to reply.  Need to do this as
	 * next incoming client request will alter xid field of old
	 * transport, and replies won't match the correct request.
	 */

	transp->xp_sock = sock;
	((struct svcudp_data *) transp->xp_p2)->su_xid = xid;
	transp->xp_raddr = *addr;
	memset (&transp->xp_verf, 0, sizeof(transp->xp_verf));
	transp->xp_addrlen = sizeof (transp->xp_raddr);

	/* Authenticate the request, then call the correct NFS routine */

	if (nfsdAuthHook)
	    {
	    authorized = (*nfsdAuthHook) (NFS_PROGRAM, NFS_VERSION,
					  request.procNum, request.sockAddr,
					  argument);
	    }
	else
	    authorized = NFS_OK;

	if (authorized  != NFS_OK)
	    {
	    result = KHEAP_ALLOC(sizeof (nfsstat));

	    if (NULL != result)
		{
		bzero ((char *)result, sizeof (nfsstat));
		}

	    *((nfsstat *) result) = authorized;
	    }
	else
	    result = (char *) (*local)(argument);

	/* Send the result */
	
	if (result != NULL && !svc_sendreply(transp, xdr_result, result))
	    {
	    svcerr_systemerr(transp);
	    }

	/* Free any space used by RPC/XDR */
	
	if (!svc_freeargs(transp, xdr_argument, argument))
	    {
	    perror ("unable to free arguments for NFS server");
	    return;
	    }

	/* Free space allocated when request was queued up */
	
	KHEAP_FREE((char *)result);
	KHEAP_FREE((char *)argument);

	/* Unfortunately, there's no way to tell svc_destroy() to not close
	 * the socket.  So, set the socket to -1. svc_destroy() doesn't
	 * check the value of the close() anyway.  If errno is set to
	 * S_iosLib_INVALID_FILE_DESCRIPTOR due to the close (-1), reset it
	 * to OK.  (svc_destroy() returns NULL, so there's no way to check
	 * its return value.)
	 */
	
	}
    transp->xp_sock = -1;
    svc_destroy (transp);
    if (errno == S_iosLib_INVALID_FILE_DESCRIPTOR)
        errno = OK;
    }

/******************************************************************************
*
* nfsd - NFS daemon process
*
* Used as the entrypoint to a task spawned from nfsdInit().  This task
* sets up the NFS RPC service, and calls svc_run(), which should never return.
* nfsdRequestEnqueue() is called from the svc_run() call.
*
* This routine is not declared LOCAL only because the symbol for its entry
* point should be displayed by i().
* 
* NOTE:  Some of this routine was generated by rpcgen.
* 
* RETURNS:  Never returns unless an error is encountered.
* 
* SEE ALSO: nfsdRequestEnqueue().
* 
* NOMANUAL
*/

void nfsd
    (
    void
    )
    {
    register SVCXPRT * transp;	/* RPC transport */
    int                sock;	/* NFS socket */
    struct sockaddr_in addr;	/* Address of NFS (port 2049 */
    int                optVal = NFS_MAXDATA * 2 + sizeof (struct rpc_msg) * 2;
                                /* XXX - too much space is wasted here */
    
    /* Initialize the task to use VxWorks RPC code */
    
    rpcTaskInit();

    /* Create NFS socket and bind it to correct address */
    
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    /* Set up the socket address */
    
    addr.sin_family             = AF_INET;
    addr.sin_addr.s_addr        = INADDR_ANY;
    addr.sin_port               = htons (NFS_PORT);

    bzero (addr.sin_zero, sizeof (addr.sin_zero));
    if (bind (sock, (struct sockaddr *) &addr, sizeof (addr)) == ERROR)
	{
	perror ("Could not bind to NFS port address");
	return;
	}

    /* Unset any previous NFS services */
    pmap_unset(NFS_PROGRAM, NFS_VERSION);

    /* Set socket buffer large enough to accommodate max NFS message */
    if ((setsockopt (sock, SOL_SOCKET, SO_SNDBUF, (char *)&optVal,
		     sizeof (optVal)) == ERROR)
	|| (setsockopt (sock, SOL_SOCKET, SO_RCVBUF, (char *)&optVal,
			sizeof (optVal)) == ERROR))
        return;

    /* Create RPC transport */
    transp = svcudp_create(sock);
    if (transp == NULL)
	{
	fprintf(stderr, "cannot create udp service.");
	exit(1);
	}

    /* Register NFS */
    if (!svc_register(transp, NFS_PROGRAM, NFS_VERSION, nfsdRequestEnqueue,
		      IPPROTO_UDP))
	{
	perror ("unable to register (NFS_PROGRAM, NFS_VERSION, udp).\n");
	return;
	}

    /* Start the RPC service, and never come back */
    svc_run();

    /* Should never get to this point */
    
    perror ("NFS svc_run returned");
    }

/******************************************************************************
*
* nfsproc_null_2 - do nothing
*
* RETURNS: A pointer to malloced spaced that contains no information.
*
* NOMANUAL
*/

void * nfsproc_null_2
    (
    void
    )
    {
    nfsdServerStatus.nullCalls++;
    
    return ((void *) KHEAP_ALLOC(0));
    }

/******************************************************************************
*
* nfsproc_getattr_2 - get file attributes
*
* If the reply status is NFS_OK, then the reply attributes contains the
* attributes for the file given by the input fhandle.
* 
* RETURNS: A pointer to an attrstat struct.
*
* NOMANUAL
*/

attrstat * nfsproc_getattr_2
    (
    nfs_fh *   fh /* File handle to get attributes of */
    )
    {
    attrstat * retVal = KHEAP_ALLOC(sizeof (attrstat)); /* Struct to fill in */

    nfsdServerStatus.getattrCalls++;

    nfsdFhNtoh ((NFS_FILE_HANDLE *) fh);
    
    if (retVal == NULL)
        return (NULL);

    /* Get the attributes for the file */
    
    if (nfsdFattrGet ((NFS_FILE_HANDLE *) fh,
			 &retVal->attrstat_u.attributes) == ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}
    else
        retVal->status = NFS_OK;
    
    return (retVal);
    }

/******************************************************************************
*
* nfsproc_setattr_2 - set the attributes for a file
* 
* The "attr" argument contains fields which are either -1 or are
* the new value for the attributes of "file".  If the reply status is
* NFS_OK, then the reply attributes have the attributes of the file
* after the "SETATTR" operation has completed.
* 
* Notes:  The use of -1 to indicate an unused field in "attributes" is
* changed in the next version of the protocol.
*
* NOMANUAL
*/

attrstat * nfsproc_setattr_2
    (
    sattrargs *    attr		/* Attributes to change */
    )
    {
    attrstat *     retVal = KHEAP_ALLOC(sizeof (attrstat));/* Return info */
    char *         fileName;    /* Name of the file being changed */
    int 	   fd;		/* File descriptor for file being changed */
    struct utimbuf timeBuf;	/* New time settings for file */

    if (retVal == NULL)
        return (NULL);
     
    if ((fileName = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    nfsdServerStatus.setattrCalls++;

    nfsdFhNtoh ((NFS_FILE_HANDLE *) &attr->file);

    /* Make sure the file system is writeable */

    if (nfsdFsReadOnly((NFS_FILE_HANDLE *) &attr->file))
	{
	retVal->status = NFSERR_ACCES;
	return (retVal);
	}

    if (nfsdFhToName ((NFS_FILE_HANDLE *) &attr->file, fileName) == ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    if (attr->attributes.mode != -1)
        /* Currently don't have any way to set the mode */
        ;
        
    if (attr->attributes.uid != -1)
        /* Currently don't have any way to set the uid */
        ;
        
    if (attr->attributes.gid != -1)
        /* Currently don't have any way to set the gid */
        ;
        
    if (attr->attributes.size != -1)
	{
	/* Set the size of the file using FIOTRUNC ioctl, as VxWorks
	 * doesn't have ftruncate() */
	
        fd = open (fileName, O_RDWR, 0666);
	if (fd != ERROR)
	    {
	    if (ioctl (fd, FIOTRUNC, attr->attributes.size) == ERROR)
		{
		retVal->status = nfsdErrToNfs (errno);
		close (fd);
		return (retVal);
		}
	    else
	        close (fd);
	    }
	else
	    {
	    retVal->status = nfsdErrToNfs (errno);
	    close (fd);
	    return (retVal);
	    }
	}

    /* Set file time.
     *
     * As there's no way to set only one of these fields, if only one
     * is set, set the other one to the same value.
     */
    
    if (attr->attributes.atime.seconds != -1)
	timeBuf.modtime = timeBuf.actime = attr->attributes.atime.seconds;
    
    if (attr->attributes.mtime.seconds != -1)
	{
	timeBuf.modtime = attr->attributes.mtime.seconds;
	if (attr->attributes.atime.seconds == -1)
	    timeBuf.actime = timeBuf.modtime;
	}

    if ((attr->attributes.atime.seconds != -1) ||
	(attr->attributes.mtime.seconds != -1))
        if (utime (fileName, &timeBuf) == ERROR)
	    {
	    retVal->status = nfsdErrToNfs (errno);
	    return (retVal);
	    }
    
    /* Get the status information for the file to return */
    
    if (nfsdFattrGet ((NFS_FILE_HANDLE *) &attr->file,
			 &retVal->attrstat_u.attributes) != OK)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    retVal->status = NFS_OK;

    return (retVal);
    }

/******************************************************************************
*
* nfsproc_root_2 - obsolete
* 
* Obsolete.  This procedure is no longer used because finding the root
* file handle of a filesystem requires moving pathnames between client
* and server.  To do this right, we would have to define a network
* standard representation of pathnames.  Instead, the function of
* looking up the root file handle is done by the MNTPROC_MNT procedure.
* 
* RETURNS:  A pointer to no information.
*
* NOMANUAL
*/

void * nfsproc_root_2
    (
    void
    )
    {
    nfsdServerStatus.setattrCalls++;
    return ((void *) KHEAP_ALLOC(0));
    }

/******************************************************************************
*
* nfsproc_lookup_2 -
* 
* If the reply "status" is NFS_OK, then the reply "file" and reply
* "attributes" are the file handle and attributes for the file "name"
* in the directory given by "dir" in the argument.
* 
* RETURNS:  A pointer to a diropres struct, or NULL.
*
* NOMANUAL
*/

diropres * nfsproc_lookup_2
    (
    diropargs * dir		/* The direcory and file to look up */
    )
    {
    diropres *  retVal = KHEAP_ALLOC(sizeof (diropres)); /* return status */

    if (retVal == NULL)
        return (NULL);
    
    nfsdServerStatus.lookupCalls++;

    nfsdFhNtoh ((NFS_FILE_HANDLE *) &dir->dir);
    
    /* Build file handle for new file */

    if (nfsdFhCreate ((NFS_FILE_HANDLE *) &dir->dir, dir->name,
			 (NFS_FILE_HANDLE *) &retVal->diropres_u.diropres.file)
	== ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    /* Get the attributes for the newly created file handle */
    if (nfsdFattrGet ((NFS_FILE_HANDLE *) &retVal->diropres_u.diropres.file,
			 &retVal->diropres_u.diropres.attributes) == ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}
    retVal->status = NFS_OK;
    nfsdFhHton ((NFS_FILE_HANDLE *) &retVal->diropres_u.diropres.file);
    
    return (retVal);
    }

/******************************************************************************
*
* nfsproc_readlink_2 - read from a symbolic link
* 
* VxWorks does not support symbolic links.
* 
* RETURNS:  A pointer to an EOPNOTSUPP error.
*
* NOMANUAL
*/

readlinkres * nfsproc_readlink_2
    (
    nfs_fh * fh			/* File handle to read */
    )
    {
    readlinkres * retVal = KHEAP_ALLOC(sizeof (retVal->status));

    if (retVal == NULL)
        return (NULL);

    /* Increment statistics */
    nfsdServerStatus.readlinkCalls++;

    /* symbolic links not supported */
    retVal->status = EOPNOTSUPP;
    return (retVal);
    }

/******************************************************************************
*
* nfsproc_read_2 - read data from a file
* 
* Returns up to "count" bytes of "data" from the file given by "file",
* starting at "offset" bytes from the beginning of the file.  The first
* byte of the file is at offset zero.  The file attributes after the
* read takes place are returned in "attributes".
* 
* Notes:  The argument "totalcount" is unused, and is removed in the
* next protocol revision.
*
* NOMANUAL
*/

readres * nfsproc_read_2
    (
    readargs * readFile		/* File, offset, and count information */
    )
    {
    char *     fileName;        /* Name of the file being read */
    readres *  retVal = KHEAP_ALLOC(sizeof (readres) + readFile->count);
				/* Return information, including read data */
    char *     readData;	/* Address of read data in retVal */
    int        fd;		/* File descriptor for file being read */
    int	       nBytes;		/* Number of bytes actually read */

    if (retVal == NULL)
        return (NULL);

    if ((fileName = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    /* Increment statistics */
    nfsdServerStatus.readCalls++;

    /* set readData to point to the correct position in the return value */
    readData = (char *) retVal + sizeof (readres);
    
    nfsdFhNtoh ((NFS_FILE_HANDLE *) &readFile->file);
    
    if (nfsdFhToName ((NFS_FILE_HANDLE *) &readFile->file, fileName)
	== ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    if ((fd = open (fileName, O_RDONLY, 0)) == ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    if (lseek (fd, readFile->offset, SEEK_SET) == ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	close (fd);
	return (retVal);
	}

    if ((nBytes = read (fd, readData, readFile->count)) == ERROR)
    	{
	retVal->status = nfsdErrToNfs (errno);
	close (fd);
	return (retVal);
	}
    else
	{
        retVal->readres_u.reply.data.data_val = readData;
	retVal->readres_u.reply.data.data_len = nBytes;
	}

    if (close (fd) == ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    if (nfsdFattrGet ((NFS_FILE_HANDLE *) &readFile->file,
			 &retVal->readres_u.reply.attributes) == ERROR)
    	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    retVal->status = NFS_OK;

    return (retVal);
    }

/******************************************************************************
*
* nfsproc_writecache_2 - unused
*
* NOMANUAL
*/

void * nfsproc_writecache_2
    (
    void
    )
    {
    /* Increment statistics */
    nfsdServerStatus.writecacheCalls++;

    return ((void *) KHEAP_ALLOC(0));
    }

/******************************************************************************
*
* nfsproc_write_2 -
* 
* Writes "data" beginning "offset" bytes from the beginning of "file".
* The first byte of the file is at offset zero.  If the reply "status"
* is NFS_OK, then the reply "attributes" contains the attributes of the
* file after the write has completed.  The write operation is atomic.
* Data from this "WRITE" will not be mixed with data from another
* client's "WRITE".
* 
* Notes:  The arguments "beginoffset" and "totalcount" are ignored and
* are removed in the next protocol revision.
*
* NOMANUAL
*/

attrstat * nfsproc_write_2
    (
    writeargs * wrArgs		/* File, write data, and offset information */
    )
    {
    char *      fileName;       /* Name of the file to write */
    attrstat *  retVal;		/* Return value */
    int         fd;		/* File descriptor for file to write */
    int		nBytes;		/* Number of bytes actually written */
    int         writeCount;     /* Number of bytes to write */
    BOOL        writeError = FALSE;
    
    if ((retVal = KHEAP_ALLOC(sizeof (*retVal))) == NULL)
	return (NULL);
    
    if ((fileName = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    /* Increment statistics */
    nfsdServerStatus.writeCalls++;

    nfsdFhNtoh ((NFS_FILE_HANDLE *) &wrArgs->file);
    
    /* Make sure the file system is writeable */

    if (nfsdFsReadOnly((NFS_FILE_HANDLE *) &wrArgs->file))
	{
	retVal->status = NFSERR_ACCES;
	return (retVal);
	}

    if (nfsdFhToName ((NFS_FILE_HANDLE *) &wrArgs->file, fileName) == ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    if ((fd = open (fileName, O_WRONLY, 0)) == ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}
    
    if (lseek (fd, wrArgs->offset, SEEK_SET) != wrArgs->offset)
    	{
	retVal->status = nfsdErrToNfs (errno);
	close (fd);
	return (retVal);
	}

    errno = OK;
    writeCount = wrArgs->data.data_len;
    if ((nBytes = write (fd, wrArgs->data.data_val, writeCount)) != writeCount)
	{
	if (errno == OK)
	    {
	    /* Try this twice to get around a dosFs bug */
	    writeCount -= nBytes;
	    if (write (fd, wrArgs->data.data_val, writeCount) != writeCount)
		writeError = TRUE;
	    }
	else
	    writeError = TRUE;
	
	if (writeError)
	    {
	    retVal->status = nfsdErrToNfs (errno);
	    close (fd);
	    return (retVal);
	    }
	}
    
    if (close (fd) == ERROR)
    	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    if (nfsdFattrGet ((NFS_FILE_HANDLE *) &wrArgs->file,
			 &retVal->attrstat_u.attributes) == ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    retVal->status = NFS_OK;
    return (retVal);
    }

/******************************************************************************
*
* nfsproc_create_2 - create a file
* 
* The file "name" is created in the directory given by "dir".  The
* initial attributes of the new file are given by "attributes".  A
* reply "status" of NFS_OK indicates that the file was created, and
* reply "file" and reply "attributes" are its file handle and
* attributes.  Any other reply "status" means that the operation failed
* and no file was created.
* 
* Notes:  This routine should pass an exclusive create flag, meaning
* "create the file only if it is not already there".
* 
* NOMANUAL
*/

diropres * nfsproc_create_2
    (
    createargs * newFile  /* File to create */
    )
    {
    diropres *   retVal = KHEAP_ALLOC(sizeof (diropres)); /* Return value */
    char *       newName;       /* Name of file to create */
    int          fd;		/* File descriptor for file to create */

    if (retVal == NULL)
        return (NULL);
    
    if ((newName = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    /* Increment statistics */
    nfsdServerStatus.createCalls++;

    /* Create the name for the new file by getting the name of the directory,
     * followed by "/" and the name of the new file
     */
    
    nfsdFhNtoh ((NFS_FILE_HANDLE *) &newFile->where.dir);
    
    /* Make sure the file system is writeable */

    if (nfsdFsReadOnly((NFS_FILE_HANDLE *) &newFile->where.dir))
	{
	retVal->status = NFSERR_ACCES;
	return (retVal);
	}

    if (nfsdFhToName ((NFS_FILE_HANDLE *) &newFile->where.dir, newName)
	== ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    strcat (newName, "/");
    strcat (newName, newFile->where.name);

    /* Check if the file has been created already */

    if ((fd = open (newName, O_RDWR, 0)) != ERROR)
	{
	retVal->status = NFSERR_EXIST;
	close(fd);
	return (retVal);
	}

    /* Create the file */
    
    if ((fd = open (newName, O_RDWR | O_CREAT, 0)) == ERROR)
    	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}
    
    if (close (fd) == ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    /* Create its file handle */
    if (nfsdFhCreate ((NFS_FILE_HANDLE *) &newFile->where.dir,
			 newFile->where.name,
			 (NFS_FILE_HANDLE *) &retVal->diropres_u.diropres.file)
	== ERROR)
    	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    /* Get its attributes */
    if (nfsdFattrGet ((NFS_FILE_HANDLE *) &retVal->diropres_u.diropres.file,
		     &retVal->diropres_u.diropres.attributes) == ERROR)
    	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    retVal->status = NFS_OK;
    nfsdFhHton ((NFS_FILE_HANDLE *) &retVal->diropres_u.diropres.file);
    
    return (retVal);
    }

/******************************************************************************
*
* nfsproc_remove_2 - remove a file
* 
* The file "name" is removed from the directory given by "dir".  A
* reply of NFS_OK means the directory entry was removed.
* 
* Notes:  possibly non-idempotent operation.
* 
* NOMANUAL
*/

nfsstat * nfsproc_remove_2
    (
    diropargs * removeFile	/* File to be removed */
    )
    {
    nfsstat * retVal = KHEAP_ALLOC(sizeof (nfsstat)); /* Return value */
    char *    newName;          /* Name of the file to be removed */

    if (retVal == NULL)
        return NULL;

    if ((newName = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    /* Increment statistics */
    nfsdServerStatus.removeCalls++;

    /* Create the name for removed file by getting the name of the directory,
     * followed by "/" and the name of the new file
     */
    nfsdFhNtoh ((NFS_FILE_HANDLE *) &removeFile->dir);
    
    /* Make sure the file system is writeable */

    if (nfsdFsReadOnly((NFS_FILE_HANDLE *) &removeFile->dir))
	{
	*retVal = NFSERR_ACCES;
	return (retVal);
	}

    if (nfsdFhToName ((NFS_FILE_HANDLE *) &removeFile->dir, newName)
	== ERROR)
	{
	*retVal = nfsdErrToNfs (errno);
	return (retVal);
	}

    strcat (newName, "/");
    strcat (newName, removeFile->name);

    /* Do the removal */
    if (unlink (newName) == ERROR)
	*retVal = nfsdErrToNfs (errno);
    else
	*retVal = NFS_OK;

    return (retVal);
    }

/******************************************************************************
*
* nfsproc_rename_2 - rename a file
* 
* The existing file "from.name" in the directory given by "from.dir" is
* renamed to "to.name" in the directory given by "to.dir".  If the
* reply is NFS_OK, the file was renamed.  The RENAME operation is
* atomic on the server; it cannot be interrupted in the middle.
* 
* Notes:  possibly non-idempotent operation.
* 
* NOMANUAL
*/

nfsstat * nfsproc_rename_2
    (
    renameargs * names		/* New and old file names */
    )
    {
    nfsstat * retVal = KHEAP_ALLOC(sizeof (nfsstat)); /* Return value */
    char *    oldName;                                /* Old file name */
    char *    newName;                                /* New file name */

    /* Increment statistics */
    nfsdServerStatus.renameCalls++;

    if (retVal == NULL)
        return (NULL);
    
    if ((oldName = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    if ((newName = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    /* Create strings for the old and new file names */
    
    nfsdFhNtoh ((NFS_FILE_HANDLE *)  &names->from.dir);
    nfsdFhNtoh ((NFS_FILE_HANDLE *)  &names->to.dir);
    
    /* Make sure the file system is writeable */

    if (nfsdFsReadOnly((NFS_FILE_HANDLE *) &names->from.dir))
	{
	*retVal = NFSERR_ACCES;
	return (retVal);
	}

    if (nfsdFhToName ((NFS_FILE_HANDLE *) &names->from.dir, oldName)
	== ERROR)
	{
	*retVal = nfsdErrToNfs (errno);
	return (retVal);
	}

    strcat (oldName, "/");
    strcat (oldName, names->from.name);

    if (nfsdFhToName ((NFS_FILE_HANDLE *) &names->to.dir, newName)
	== ERROR)
	{
	*retVal = nfsdErrToNfs (errno);
	return (retVal);
	}

    strcat (newName, "/");
    strcat (newName, names->to.name);

    /* Do the rename() */
    if (rename (oldName, newName) == ERROR)
	{
	*retVal = nfsdErrToNfs (errno);
	return (retVal);
	}
    else
        *retVal = NFS_OK;
    
    return (retVal);
    }

/******************************************************************************
*
* nfsproc_link_2 - create a link - not supported by VxWorks
*
* NOMANUAL
*/

nfsstat * nfsproc_link_2
    (
    void
    )
    {
    nfsstat * notSupported = KHEAP_ALLOC(sizeof (nfsstat));
    *notSupported = EOPNOTSUPP;
    return ((nfsstat *) notSupported);
    }

/******************************************************************************
*
* nfsproc_symlink_2 - create a symbolic link - not supported by VxWorks
*
* NOMANUAL
*/

nfsstat * nfsproc_symlink_2
    (
    void
    )
    {
    nfsstat * notSupported = KHEAP_ALLOC(sizeof (nfsstat));
    /* Increment statistics */
    nfsdServerStatus.symlinkCalls++;

    *notSupported = EOPNOTSUPP;
    return ((nfsstat *) notSupported);
    }

/******************************************************************************
*
* nfsproc_mkdir_2 - create a directory
* 
* The new directory "where.name" is created in the directory given by
* "where.dir".  The initial attributes of the new directory are given
* by "attributes".  A reply "status" of NFS_OK indicates that the new
* directory was created, and reply "file" and reply "attributes" are
* its file handle and attributes.  Any other reply "status" means that
* the operation failed and no directory was created.
* 
* Notes:  possibly non-idempotent operation.
*
* NOMANUAL
*/

diropres * nfsproc_mkdir_2
    (
    createargs * crArgs
    )
    {
    diropres *      retVal = KHEAP_ALLOC(sizeof (diropres));
    char *          newName;

    if (retVal == NULL)
        return (NULL);
    
    if ((newName = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    /* Increment statistics */
    nfsdServerStatus.mkdirCalls++;

    retVal->status = NFS_OK;

    /* Create the string for the name of the directory to make */
    
    nfsdFhNtoh ((NFS_FILE_HANDLE *) &crArgs->where.dir);
    
    /* Make sure the file system is writeable */

    if (nfsdFsReadOnly((NFS_FILE_HANDLE *) &crArgs->where.dir))
	{
	retVal->status = NFSERR_ACCES;
	return (retVal);
	}

    if (nfsdFhToName ((NFS_FILE_HANDLE *) &crArgs->where.dir, newName)
	== ERROR)
	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    strcat (newName, "/");
    strcat (newName, crArgs->where.name);

    /* make the directory */
    if (mkdir (newName) == ERROR)
    	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    /* Create a file handle for the new directory */
    if (nfsdFhCreate ((NFS_FILE_HANDLE *) &crArgs->where.dir,
			 crArgs->where.name,
			 (NFS_FILE_HANDLE *) &retVal->diropres_u.diropres.file)
	== ERROR)
    	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    /* Get the file attributes for the new directory */
    if (nfsdFattrGet ((NFS_FILE_HANDLE *) &retVal->diropres_u.diropres.file,
		     &retVal->diropres_u.diropres.attributes) == ERROR)
    	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    nfsdFhHton ((NFS_FILE_HANDLE *) &retVal->diropres_u.diropres.file);
    return (retVal);
    }

/******************************************************************************
*
* nfsproc_rmdir_2 - remove a directory
* 
* The existing empty directory "name" in the directory given by "dir"
* is removed.  If the reply is NFS_OK, the directory was removed.
* 
* Notes:  possibly non-idempotent operation.
*
* NOMANUAL
*/

nfsstat * nfsproc_rmdir_2
    (
    diropargs * removeDir	/* Directory to remove */
    )
    {
    nfsstat * retVal = KHEAP_ALLOC(sizeof (nfsstat)); /* Return value */
    char *    newName;          /* Path name of directory to remove */

    if (retVal == NULL)
        return NULL;

    if ((newName = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    /* Increment statistics */
    nfsdServerStatus.rmdirCalls++;

    nfsdFhNtoh ((NFS_FILE_HANDLE *) &removeDir->dir);
    
    /* Make sure the file system is writeable */

    if (nfsdFsReadOnly((NFS_FILE_HANDLE *) &removeDir->dir))
	{
	*retVal = NFSERR_ACCES;
	return (retVal);
	}

    /* Create the name of the doomed directory */
    if (nfsdFhToName ((NFS_FILE_HANDLE *) &removeDir->dir, newName) == ERROR)
	{
	*retVal = nfsdErrToNfs (errno);
	return (retVal);
	}

    strcat (newName, "/");
    strcat (newName, removeDir->name);

    /* Remove it */
    if (rmdir (newName) == ERROR)
	*retVal = nfsdErrToNfs (errno);
    else
	*retVal = NFS_OK;

    return (retVal);
    }

/******************************************************************************
*
* nfsproc_readdir_2 - read from a directory
* 
* Returns a variable number of directory entries, with a total size of
* up to "count" bytes, from the directory given by "dir".  If the
* returned value of "status" is NFS_OK, then it is followed by a
* variable number of "entry"s.  Each "entry" contains a "fileid" which
* consists of a unique number to identify the file within a filesystem,
* the "name" of the file, and a "cookie" which is an opaque pointer to
* the next entry in the directory.  The cookie is used in the next
* READDIR call to get more entries starting at a given point in the
* directory.  The special cookie zero (all bits zero) can be used to
* get the entries starting at the beginning of the directory.  The
* "fileid" field should be the same number as the "fileid" in the the
* attributes of the file.  (See section "2.3.5. fattr" under "Basic
* Data Types".)  The "eof" flag has a value of TRUE if there are no
* more entries in the directory.
* 
* NOMANUAL
*/

readdirres * nfsproc_readdir_2
    (
    readdirargs * readDir	/* Directory to read */
    )
    {
    struct entry *  dirEntries;
    filename *      fileNames;
    int             fileNameSize;
    int             entrySpace;
    char *          currentName;
    char *          dirName;
    DIR	*	    theDir = NULL;
    struct dirent * localEntry;
    readdirres *    retVal;
    int		    dirCount = 0;
    char *          newFile;
    int             numEntries;
    int             entryCurrent;

    if ((dirName = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    if ((newFile = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    /* Increment statistics */
    nfsdServerStatus.readdirCalls++;

    /* Compute the amount of space needed for each directory entry */

    fileNameSize = MEM_ROUND_UP (NAME_LEN + 1);
    entrySpace = MEM_ROUND_UP (sizeof (readdirres)) +
	         MEM_ROUND_UP (sizeof (entry)) +
	         fileNameSize;
    
    /* Compute the number of possible entries to return */
    
    numEntries = readDir->count / entrySpace;

    /* Allocate space for the entries */
    
    if ((retVal = KHEAP_ALLOC(numEntries * entrySpace)) == NULL)
	{
        return (NULL);
	}
    else
	{
	bzero ((char *) retVal, numEntries * entrySpace);
	dirEntries = ((struct entry *) retVal) + 1;
	fileNames = (filename *) (dirEntries + numEntries);
	}

    nfsdFhNtoh ((NFS_FILE_HANDLE *) &readDir->dir);
    
    /* Create the directory name */
    if (nfsdFhToName ((NFS_FILE_HANDLE *) &readDir->dir, dirName) == ERROR)
    	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    /* Open the directory and set the cookie field to what was
     * passed in readDir
     */
    if ((theDir = opendir (dirName)) == NULL)
    	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}
    else
        theDir->dd_cookie = *((int *) &readDir->cookie);

    retVal->readdirres_u.reply.eof = FALSE;
    
    /* Read through all the files in the directory */
    for (entryCurrent = 0; entryCurrent < numEntries; entryCurrent++)
	{
	errno = OK;
	localEntry = readdir (theDir);
	if (localEntry == NULL)
	    {
	    if (errno != OK)
		{
		retVal->status = nfsdErrToNfs (errno);
		closedir (theDir);
		return (retVal);
		}
	    else
		{
		retVal->readdirres_u.reply.eof = TRUE;
		if (entryCurrent != 0)
		    dirEntries[entryCurrent - 1].nextentry = NULL;
		else
		    retVal->readdirres_u.reply.entries = NULL;
	        break;
		}
	    }

	strcpy (newFile, dirName);
	strcat (newFile, "/");
	strcat (newFile, localEntry->d_name);
	dirEntries[entryCurrent].fileid = 
	   nameToInode (((NFS_FILE_HANDLE *) &readDir->dir)->volumeId, newFile);
	if (dirEntries[entryCurrent].fileid == ERROR)
	    {
	    retVal->status = nfsdErrToNfs (errno);
	    closedir (theDir);
	    return (retVal);
	    }
	currentName = ((char *) fileNames) + entryCurrent * fileNameSize;
	strcpy(currentName, localEntry->d_name);
	dirEntries[entryCurrent].name = currentName;
	*((int *) dirEntries[entryCurrent].cookie) = theDir->dd_cookie;

	if (entryCurrent == numEntries - 1)
	    dirEntries[entryCurrent].nextentry = NULL;
	else
	    dirEntries[entryCurrent].nextentry = &dirEntries[entryCurrent + 1];
	    
	dirCount++;
	}

    retVal->status = NFS_OK;

    if (dirCount != 0)
	retVal->readdirres_u.reply.entries = dirEntries;
    else
	retVal->readdirres_u.reply.entries = NULL;

    closedir (theDir);

    return (retVal);
    }

/******************************************************************************
*
* nfsproc_statfs_2 - Get filesystem attributes
* 
* Return information about the filesystem.  The NFS equivalent of statfs().
*
* NOMANUAL
*/

statfsres * nfsproc_statfs_2
    (
    nfs_fh * fh
    )
    {
    char *        fileName;
    struct statfs statInfo;
    statfsres *   retVal = KHEAP_ALLOC(sizeof (statfsres));

    if (retVal == NULL)
        return (NULL);

    if ((fileName = (char *) alloca (nfsMaxPath)) == NULL)
	return (NULL);

    /* Increment statistics */
    nfsdServerStatus.statfsCalls++;

    nfsdFhNtoh ((NFS_FILE_HANDLE *) fh);

    if (nfsdFhToName ((NFS_FILE_HANDLE *) fh, fileName) == ERROR)
    	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    if (statfs (fileName, &statInfo) == ERROR)
    	{
	retVal->status = nfsdErrToNfs (errno);
	return (retVal);
	}

    retVal->status = NFS_OK;
    retVal->statfsres_u.reply.tsize  = 8 * 1024;
    retVal->statfsres_u.reply.bsize  = statInfo.f_bsize;
    retVal->statfsres_u.reply.blocks = statInfo.f_blocks;
    retVal->statfsres_u.reply.bfree  = statInfo.f_bfree;
    retVal->statfsres_u.reply.bavail = statInfo.f_bavail;
    return (retVal);
    }

/******************************************************************************
*
* nfsdStatusGet - get the status of the NFS server
* 
* This routine gets status information about the NFS server.
* 
* RETURNS: OK, or ERROR if the information cannot be obtained.
*/

STATUS nfsdStatusGet
    (
    NFS_SERVER_STATUS * serverStats /* pointer to status structure */
    )
    {
    memcpy (serverStats, &nfsdServerStatus, sizeof (nfsdServerStatus));
    return (OK);
    }

/******************************************************************************
*
* nfsdStatusShow - show the status of the NFS server
* 
* This routine shows status information about the NFS server.
* 
* RETURNS: OK, or ERROR if the information cannot be obtained.
*/

STATUS nfsdStatusShow
    (
    int options			/* unused */
    )
    {
    NFS_SERVER_STATUS serverStat;
    char*             outputFormat = "%15s	%8d\n";

    if (nfsdStatusGet (&serverStat) == ERROR)
        return (ERROR);

    printf ("%15s    %8s\n", "Service", "Number of Calls");
    printf ("%15s    %8s\n", "-------", "---------------");
    printf (outputFormat, "null", serverStat.nullCalls);
    printf (outputFormat, "getattr", serverStat.getattrCalls);
    printf (outputFormat, "setattr", serverStat.setattrCalls);
    printf (outputFormat, "root", serverStat.rootCalls);
    printf (outputFormat, "lookup", serverStat.lookupCalls);
    printf (outputFormat, "readlink", serverStat.readlinkCalls);
    printf (outputFormat, "read", serverStat.readCalls);
    printf (outputFormat, "writecache", serverStat.writecacheCalls);
    printf (outputFormat, "write", serverStat.writeCalls);
    printf (outputFormat, "create", serverStat.createCalls);
    printf (outputFormat, "remove", serverStat.removeCalls);
    printf (outputFormat, "rename", serverStat.renameCalls);
    printf (outputFormat, "link", serverStat.linkCalls);
    printf (outputFormat, "symlink", serverStat.symlinkCalls);
    printf (outputFormat, "mkdir", serverStat.mkdirCalls);
    printf (outputFormat, "rmdir", serverStat.rmdirCalls);
    printf (outputFormat, "readdir", serverStat.readdirCalls);
    printf (outputFormat, "statfs", serverStat.statfsCalls);

    return (OK);
    }


/******************************************************************************
*
* nfsdFhCreate - Create an NFS file handle
*
* Given a handle to the parent directory, and the name of a file inside that
* directory, create a new file handle that specifies the file.
* 
* RETURNS:  OK, or ERROR if a file handle for the specified file could not be
* created.
*
* NOMANUAL
*/

STATUS nfsdFhCreate
    (
    NFS_FILE_HANDLE * parentDir,
    char *            fileName,
    NFS_FILE_HANDLE * fh
    )
    {
    struct stat       str;
    char *            fullName;
    char              *p;       /* to remove slash from end of fileName */

    if ((fullName = (char *) alloca (nfsMaxPath)) == NULL)
	return (ERROR);

    memset (fh, 0, NFS_FHSIZE);
    
    if (nfsdFhToName (parentDir, fullName) == ERROR)
        return (ERROR);
    
    /* build fullname, but remove slash from the end of fileName */
    /* in order to lkup correspond name from nfsHash             */
 
    strcat (fullName, "/");
    strcat (fullName, fileName);
    pathCondense (fullName);
    for (p = strchr (fullName, EOS); p != fullName && *--p == '/'; )
         *p = EOS;
    if (stat (fullName, &str) == ERROR)
        return (ERROR);
    fh->volumeId = parentDir->volumeId;
    fh->fsId     = parentDir->fsId;
    fh->inode    = nfsNmLkupIns (parentDir->volumeId, fullName);
    if (fh->inode == ERROR)
	return (ERROR);
    return (OK);
    }

/******************************************************************************
*
* nfsdFhToName - Convert a file handle to a complete file name
* 
* Given a file handle, fill in the variable fileName with a string specifying
* that file.
* 
* RETURNS:  OK, or ERROR if the file name could not be created.
*
* NOMANUAL
*/

STATUS nfsdFhToName
    (
    NFS_FILE_HANDLE * fh,
    char *            fileName
    )
    {
    NFS_EXPORT_ENTRY *  fileSys;

    if ((fileSys = nfsExportFindById (fh->volumeId)) == NULL)
        return (ERROR);

    if (nfsFhLkup (fh, fileName) == ERROR)
        {
	errno = NFSERR_STALE;
        return (ERROR);
	}
    return (OK);
    }

/******************************************************************************
*
* nfsdFattrGet - Get the file attributes for a file specified by a file handle
* 
* Given a file handle, fill in the file attribute structure pointed to by
* fileAttributes.
* 
* RETURNS:  OK, or ERROR if the file attributes could not be obtained.
*
* NOMANUAL
*/

STATUS nfsdFattrGet
    (
    NFS_FILE_HANDLE * fh,
    struct fattr *    fileAttributes
    )
    {
    char *         fileName;
    struct stat    str;
    
    if ((fileName = (char *) alloca (nfsMaxPath)) == NULL)
	return (ERROR);

    if (nfsdFhToName (fh, fileName) == ERROR)
        return (ERROR);
    
    if (stat (fileName, &str) == ERROR)
        return (ERROR);

    /* set the type */

    switch (str.st_mode & S_IFMT)
	{
	case S_IFIFO:
	    fileAttributes->type = NFFIFO;
	    break;
	case S_IFCHR:
	    fileAttributes->type = NFCHR;
	    break;
	case S_IFDIR:
	    fileAttributes->type = NFDIR;
	    break;
	case S_IFBLK:
	    fileAttributes->type = NFBLK;
	    break;
	case S_IFREG:
	    fileAttributes->type = NFREG;
	    break;
	case S_IFLNK:
	    fileAttributes->type = NFLNK;
	    break;
	case S_IFSOCK:
	    fileAttributes->type = NFSOCK;
	    break;
	}

    fileAttributes->mode      = str.st_mode;
    fileAttributes->nlink     = str.st_nlink;
    fileAttributes->uid       = str.st_uid;
    fileAttributes->gid       = str.st_gid;
    fileAttributes->size      = str.st_size;
    fileAttributes->blocksize = str.st_blksize;
    fileAttributes->rdev      = str.st_rdev;
    fileAttributes->blocks    = str.st_blocks + 1;
    fileAttributes->fileid    = fh->inode;
    fileAttributes->fsid      = str.st_dev;
    fileAttributes->rdev      = str.st_rdev;

    fileAttributes->atime.seconds = str.st_atime;
    fileAttributes->mtime.seconds = str.st_mtime;
    fileAttributes->ctime.seconds = str.st_ctime;

    fileAttributes->atime.useconds = 0;
    fileAttributes->mtime.useconds = 0;
    fileAttributes->ctime.useconds = 0;

    return (OK);
    }

/******************************************************************************
*
* nfsdErrToNfs - Convert a VxWorks error number to an NFS error number
* 
* Given a VxWorks error number, find the equivalent NFS error number.
* If an exact match is not found, return NFSERR_IO.
* 
* RETURNS:  The NFS error number.
*
* NOMANUAL
*/

nfsstat nfsdErrToNfs
    (
    int theErr			/* The error number to convert  */
    )
    {
    switch (theErr)
	{
	case EPERM:
	case S_dosFsLib_READ_ONLY:
	case S_dosFsLib_WRITE_ONLY:
	    return (NFSERR_PERM);
	    break;
	    
	case ENOENT:
	case S_dosFsLib_FILE_NOT_FOUND:
	    return (NFSERR_NOENT);
	    break;

	case EIO:
	    return (NFSERR_IO);
	    break;
	    
	case ENXIO:
	case S_dosFsLib_VOLUME_NOT_AVAILABLE:
	    return (NFSERR_NXIO);
	    break;
	    
	case EACCES:
	    return (NFSERR_ACCES);
	    break;
	    
	case EEXIST:
	case S_dosFsLib_FILE_EXISTS:
	    return (NFSERR_EXIST);
	    break;

	case ENODEV:
	    return (NFSERR_NODEV);
	    break;
	    
	case ENOTDIR:
	case S_dosFsLib_NOT_DIRECTORY:
	    return (NFSERR_NOTDIR);
	    break;
	    
	case EISDIR:
	    return (NFSERR_ISDIR);
	    break;
	    
	case EFBIG:
	    return (NFSERR_FBIG);
	    break;
	    
	case ENOSPC:
	case S_dosFsLib_DISK_FULL:
	    return (NFSERR_NOSPC);
	    break;
	    
	case EROFS:
	    return (NFSERR_ROFS);
	    break;
	    
	case ENAMETOOLONG:
	case S_dosFsLib_ILLEGAL_NAME:
	    return (NFSERR_NAMETOOLONG);
	    break;
	    
	case S_dosFsLib_DIR_NOT_EMPTY:
	case ENOTEMPTY:
	    return (NFSERR_NOTEMPTY);
	    break;
	    
        default:
	    /* If the error number is less than 100, just return it.
	     * NFS client implementations should understand anything
	     * that looks like a UNIX error number.  If it's greater than
	     * 100, return NFSERR_IO.
	     */
	
	    if (theErr < 100)
		return theErr;
	    else
		return (NFSERR_IO);

	    break;
	}
    }


/******************************************************************************
*
* nfsdFhHton - convert a file handle from host to network byte order
*
* NOMANUAL
*/

void nfsdFhHton
    (
    NFS_FILE_HANDLE * fh
    )
    {
    fh->volumeId = htonl (fh->volumeId);
    fh->fsId = htonl (fh->fsId);
    fh->inode = htonl (fh->inode);
    }

/******************************************************************************
*
* nfsdFhNtoh - convert a file handle from host to network byte order
*
* NOMANUAL
*/

void nfsdFhNtoh
    (
    NFS_FILE_HANDLE * fh
    )
    {
    fh->volumeId = ntohl (fh->volumeId);
    fh->fsId = ntohl (fh->fsId);
    fh->inode = ntohl (fh->inode);
    }

/******************************************************************************
*
* nfsdFsReadOnly - get the mode of the NFS file system
* 
* This routine returns TRUE if the filesystem that contains the file
* pointed to by <fh> is exported read-only.
* 
* RETURNS: TRUE, or FALSE if the filesystem is exported read-write.
* 
* NOMANUAL
*/

LOCAL BOOL nfsdFsReadOnly
    (
    NFS_FILE_HANDLE * fh
    )
    {
    NFS_EXPORT_ENTRY *  fileSys;

    if ((fileSys = nfsExportFindById (fh->volumeId)) == NULL)
        return (ERROR);
    else
	return (fileSys->readOnly);
    }

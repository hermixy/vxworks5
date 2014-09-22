/* rpcLib.c - Remote Procedure Call (RPC) support library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02v,07may02,kbw  man page edits
02u,15apr02,wap  change portmapd task priority from 100 to 54
02t,15oct01,rae  merge from truestack ver 02y base 02r (AE/5.X)
02s,24may01,mil  Bump up portmapper task stack size to 10000.
02r,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
02q,16oct96,dgp  doc: modify rpcLib to show correct location of sprites demo
		      SPR #7221
02p,22oct95,jdi  doc: removed references to dbxtool/dbxTask (SPR 4378).
02o,11nov94,jag  Changed rpcTaskDeleteHook to fix SPR# 3269
02n,05may93,caf  tweaked for ansi.
02m,17jul94,ms   jacked up stack size for VxSim/HPPA
02l,23sep94,rhp  doc: clarify which RPC data structures auto-deleted (SPR3148)
02k,15sep94,rhp  documentation: delete obsolete ref to dbxLib (SPR#3446).
02j,02feb93,jdi  documentation tweak on configuration.
02i,20jan93,jdi  documentation cleanup for 5.1.
02h,19jul92,smb  removed abort to ANSI stdlib library.
02g,18jul92,smb  Changed errno.h to errnoLib.h.
02f,26may92,rrr  the tree shuffle
02e,13dec91,gae  ANSI cleanup.
02d,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -fixed #else and #endif
		  -changed VOID to void
		  -changed copyright notice
02c,13may91,jdi	 documentation tweak.
02b,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
02a,01mar91,elh  made references to include the rpc library routines.
01z,24mar91,del  made tPortmapd stack larger for i960ca (no change for 68k).
01y,11mar91,jaa	 documentation cleanup.
01x,02oct90,hjb  added code to clean up raw rpc and errBuf properly in
		   rpcTaskDeleteHook().
01w,10may90,dnw  removed calls to individual rpc module init and delete routines
		 changed to do init/delete of statics in rpcTaskInit and
		   rpcTaskDeleteHook
		 added rpcErrnoGet()
01v,19apr90,hjb  deleted references to bindresvportInit and associated module
		 statics.
01u,15apr90,jcf  changed portmapd name to tPortmapd
		 made module statics referenced via macro thru taskIdCurrent.
01t,07mar90,jdi  documentation cleanup.
01s,27oct89,hjb  added bindresvport handling to the rpcTaskInit () and
		 rpcTaskDeleteHook ().
01r,23jul89,gae  removed explicit NFS cleanup -- now done with taskDeleteHook.
01q,23apr89,gae  rpcDeleteHook now invokes each rpc facility's cleanup routine.
		 Removed documentation on callrpc's failings -- fixed.
		 changed rpcTaskInit to not bother passing the module list.
01p,08apr89,dnw  changed rpcInit() to call taskVarInit().
		 changed rpcInit() to remember if init has already been done.
01o,26mar89,llk  added a call to nfsClientCacheCleanUp in rpcTaskDeleteHook.
01n,30jun88,llk  moved rpcClntErrnoSet() here from nfsLib.
01m,22jun88,dnw  name tweaks.
01l,06jun88,dnw  changed taskSpawn/taskCreate args.
01k,30may88,dnw  changed to v4 names.
01j,13apr88,gae  lint, documentation, changed parm. to taskVar{Add,Del}().
		 fixed portmapd to run at priority 100.  Made rpcTaskExit()
		 local and made it become automatic as a task delete hook.
		 checked for valid nfsClientCache in rpcTaskExit().
01j,19apr88,llk  added nfsClientCache field to taskModuleList.
01i,30mar88,rdc  beefed up documentation.
01h,22feb88,jcf  made kernel independent.
01g,29dec87,rdc  rpcTaskInit now checks to see if it has already been called.
01f,13dec87,rdc  added rpcTaskExit.
01e,04dec87,rdc  removed perror (now in unixLib.c).
01d,05nov87,dnw  moved definition of taskModuleList here from rpcGbl.h.
01c,23oct87,dnw  changed rpcInit to rpcTaskInit.
		 added new rpcInit that initializes overall rpc facilities.
01b,14oct87,rdc	 added perror and abort.
01a,04oct87,rdc	 written.
*/

/*
This library supports Sun Microsystems' Remote Procedure Call (RPC)
facility.  RPC provides facilities for implementing distributed
client/server-based architectures.  The underlying communication mechanism
can be completely hidden, permitting applications to be written without
any reference to network sockets.  The package is structured such that
lower-level routines can optionally be accessed, allowing greater control
of the communication protocols.

For more information and a tutorial on RPC, see Sun Microsystems'
.I "Remote Procedure Call Programming Guide."
For an example of RPC usage, see `/target/unsupported/demo/sprites'.

The RPC facility is enabled when INCLUDE_RPC is defined.

VxWorks supports Network File System (NFS), which is built
on top of RPC.  If NFS is configured into the VxWorks system,
RPC is automatically included as well.

IMPLEMENTATION
A task must call rpcTaskInit() before making any calls to other
routines in the RPC library.  This routine creates task-specific data
structures required by RPC.  These task-specific data structures are
automatically deleted when the task exits.

Because each task has its own RPC context, RPC-related objects (such as
SVCXPRTs and CLIENTs) cannot be shared among tasks; objects created by one
task cannot be passed to another for use.  Such additional objects must
be explicitly deleted (for example, using task deletion hooks).

INCLUDE FILES: rpc.h

SEE ALSO: nfsLib, nfsDrv, Sun Microsystems'
.I "Remote Procedure Call Programming Guide"
*/

#include "vxWorks.h"
#include "rpc/rpctypes.h"
#include "rpc/rpcGbl.h"
#include "taskLib.h"
#include "taskHookLib.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"
#include "stdio.h"
#include "errnoLib.h"
#include "rpc/portmap.h"
#include "memPartLib.h"

int portmapdId;
int portmapdPriority  = 54;
int portmapdOptions   = VX_SUPERVISOR_MODE | VX_UNBREAKABLE;
#if       (CPU_FAMILY!=I960) && (CPU_FAMILY!=SIMHPPA)
int portmapdStackSize = 10000;
#else	/* CPU_FAMILY!=I960 */
int portmapdStackSize = 30000;
#endif	/* CPU_FAMILY!=I960 */


/* forward static functions */

static void rpcTaskDeleteHook (WIND_TCB *pTcb);


/*******************************************************************************
*
* rpcInit - initialize the RPC package
*
* This routine must be called before any task can use the RPC facility; 
* it spawns the portmap daemon.  It is called automatically if INCLUDE_RPC
* is defined.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  This restriction does not apply under non-AE 
* versions of VxWorks.  
*
* RETURNS: OK, or ERROR if the portmap daemon cannot be spawned.
*/

STATUS rpcInit (void)
    {
    static BOOL rpcInitialized = FALSE;		/* TRUE = rpc inited */

    if (!rpcInitialized)
	{
	/* spawn the portmap daemon */

	portmapdId = taskSpawn ("tPortmapd", portmapdPriority,
				portmapdOptions, portmapdStackSize,
				(FUNCPTR)portmapd, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0);

	if (portmapdId == ERROR)
	    return (ERROR);

	rpcInitialized = TRUE;
	}

    /*
     * the following is to drag in the rest of rpc
     */

    clnt_genericInclude ();
    clnt_rawInclude ();
    clnt_simpleInclude ();
    pmap_getmapsInclude ();
    svc_rawInclude ();
    svc_simpleInclude ();
    xdr_floatInclude ();

    return (OK);
    }
/*******************************************************************************
*
* rpcTaskInit - initialize a task's access to the RPC package
*
* This routine must be called by a task before it makes any calls to
* other routines in the RPC package.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  This restriction does not apply under non-AE 
* versions of VxWorks.  
*
* RETURNS: OK, or ERROR if there is insufficient memory or the routine is
* unable to add a task delete hook.
*/

STATUS rpcTaskInit (void)
    {
    static BOOL rpcInstalled = FALSE;
    FAST RPC_STATICS *pRpcStatics;

    if (!rpcInstalled)
	{
	if (taskDeleteHookAdd ((FUNCPTR)rpcTaskDeleteHook) == ERROR)
	    return (ERROR);
	rpcInstalled = TRUE;
	}

    /* if rpcTaskInit has already been called for this task,
     * the RpcStatics pointer will already be filled in
     */

   if (taskRpcStatics != NULL)
	return (OK);


    /* allocate the structure of rpc statics */

    pRpcStatics = (RPC_STATICS *) KHEAP_ALLOC(sizeof(RPC_STATICS));

    if (pRpcStatics == NULL)
	return (ERROR);

    bzero ((char *)pRpcStatics, sizeof(RPC_STATICS));
    taskRpcStatics = pRpcStatics;	/* set pointer in tcb */

    /* initialize clnt_simple statics */

    pRpcStatics->clnt_simple.socket = RPC_ANYSOCK;

    return (OK);
    }
/*******************************************************************************
*
* rpcTaskDeleteHook - deallocate RPC resources of exiting task
*
* This routine is the task delete hook for tasks using the RPC package.
* It is installed by rpcTaskInit.
*/

LOCAL void rpcTaskDeleteHook
    (
    WIND_TCB *pTcb      /* pointer to control block of exiting task */
    )
    {
#ifdef _WRS_VXWORKS_5_X
    FAST RPC_STATICS *pRpcStatics = pTcb->pRPCModList;
#else
    FAST RPC_STATICS *pRpcStatics = pTcb->pLcb->pRPCModList;
#endif /* _WRS_VXWORKS_5_X */
    
    FAST int sock;
    RPC_STATICS      *pDeleterStatics;

    if (pRpcStatics == NULL)
	return;				/* task didn't use RPC */

    pDeleterStatics = taskRpcStatics;	/* Save deleters RPC vars pointer */
    taskRpcStatics  = pRpcStatics;      /* Use victims RPC vars pointer */

    /* clnt_simple.c cleanup */

    if (pRpcStatics->clnt_simple.client != NULL)
	clnt_destroy (pRpcStatics->clnt_simple.client);
    if (pRpcStatics->clnt_simple.socket > 0)
	close (pRpcStatics->clnt_simple.socket);

    /* svc.c cleanup */

    /* close open sockets and allocated memory space for
     * this server transport since VxWorks doesn't close
     * the descriptors automatically when a task is terminated.
     */
    for (sock = 0; sock < FD_SETSIZE; sock++)
	{
	SVCXPRT *xprt = pRpcStatics->svc.xports [sock];

	/* is the socket registered? */
	if (xprt != NULL)
	    svc_destroy (xprt);
	}

    /* raw cleanup */

    if (pRpcStatics->clnt_raw != NULL)
        (*(pRpcStatics->clnt_rawExit)) ();
    if (pRpcStatics->svc_raw != NULL)
        (*(pRpcStatics->svc_rawExit)) ();

    /* error buffer cleanup */

    if (pRpcStatics->errBuf != NULL)
        KHEAP_FREE(pRpcStatics->errBuf);

    /* free the rpc statics */

    KHEAP_FREE((char *) pRpcStatics);
    taskRpcStatics = pDeleterStatics;  /* Restore deleters RPC vars pointer */
    }
/******************************************************************************
*
* rpcClntErrnoSet - set RPC client status
*
* rpcClntErrnoSet calls errnoSet with the given "rpc stat" or'd with the
* rpc status prefix.
*
* NOMANUAL
*/

void rpcClntErrnoSet
    (
    enum clnt_stat status
    )
    {
    errnoSet (M_rpcClntStat | (int) status);
    }
/******************************************************************************
*
* rpcErrnoGet - get RPC errno status
*
* This function returns the errno the way rpc code expects to see it, namely
* without the rpc status prefix in the upper word.
*
* NOMANUAL
*/

int rpcErrnoGet (void)
    {
    return (errnoGet () & 0xffff);
    }

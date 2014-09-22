/* netLib.c - network interface library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
03q,07may02,kbw  man page edits
03p,15oct01,rae  merge from truestack ver 04a, base 03o, (SPRs 69112, 32626 etc.)
03o,16mar99,spm  recovered orphaned code from tor1_0_1.sens1_1 (SPR #25770)
03n,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
03m,22sep98,n_s  fixed return value of netJobAdd.  spr # 8538.
03l,05oct97,vin  added multicasting hash tbl initialization mcastHashInit.
03k,26aug97,spm  removed compiler warnings (SPR #7866)
03j,15jul97,spm  corrected comments for schednetisr() routine
03i,01jul97,vin  added addDomain() and route_init(), made routing sockets
		 scalable, fixed warnings.
03h,17apr97,vin	 added mCastRouteCmdHook & mCastRouteFwdHook for scalability.
03g,13apr97,rjc  added ip filter hook.
03f,20jan96,vin  moved _func_remCurId[SG]et to usrNetwork.c for scalability,
		 multiplexed through _netIsrMask.
03d,24aug96,vin  modified schednetisr() for BSD44
03e,21jul95,dzb  removed call to sockInit().
03d,05may93,caf  tweaked for ansi.
03c,06sep93,jcf  added prototype of netTypeAdd.
03b,05sep93,jcf  initialized _func_remCurId[SG]et to decouple net from shell.
03a,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
02z,02feb93,jdi  documentation cleanup for 5.1.
02y,13nov92,dnw  added include of semLibP.h
02x,18jul92,smb  Changed errno.h to errnoLib.h.
02w,26may92,rrr  the tree shuffle
02v,31Mar92,elh  added call to netTypeInit
02u,16dec91,gae  added includes for ANSI.
02t,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed VOID to void
		  -changed copyright notice
02s,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
02r,23jan91,jaa	 documentation.
02q,05oct90,dnw  made schednetisr() be NOMANUAL.
02p,11jul90,hjb  made ipintrPending LOCAL.  added schednetisr().
02o,26jun90,jcf  added splSemInit (), and moved semMInit () to unixLib ()
02n,20jun90,jcf  changed binary semaphore interface
02m,20mar90,jcf  changed semaphores to binary for efficiency.
		 changed netTask name to tNetTask.
02l,18mar90,hjb  added code to eliminate costly netJobAdd's of ipintr ().
02k,22feb90,jdi  documentation cleanup.
02j,02dec88,ecs  bumped netTaskStackSize from 4000 to 10000 for SPARC.
02i,28nov88,gae  moved netHelp to usrLib.c to shrink bootroms.
02h,04nov88,dnw  changed JOB_RING_SIZE from 2000 to (85*sizeof(TODO_NODE))
		   so that ring overflow won't cause ring to be permanently
		   out of sync.
02g,18aug88,gae  documentation.  removed obsolete header includes.
02f,22jun88,dnw  name tweaks.
02e,06jun88,dnw  changed taskSpawn/taskCreate args.
02d,30may88,dnw  changed to v4 names.
02c,28may88,dnw  removed if{config,broadcast,netmask} to ifLib.
		 deleted obsolete netGetInetAddr.
		 removed mbufStat to uipc_mbuf.
		 moved setNetStatus here from sockLib.
		 changed call to fioStdIn to STD_IN.
02b,05mar88,jcf  changed semaphore calls for new semLib.
02a,22feb88,jcf  made kernel independent.
01v,16feb88,rdc  added ifnetmask.
01u,05jan88,rdc  added include of systm.h
01t,23nov87,ecs  lint.
01s,20nov87,jcf  added vxAddRebootRtn call in netStart.
	   +gae  spawned netd with 0 args to look nice.
01r,18nov87,ecs  documentation.
01q,17nov87,ecs  lint: added include of inetLib.h.
01p,11nov87,jlf  documentation
01o,08nov87,dnw  updated and expanded nethelp.
01n,01nov87,llk  removed addRoute(), addNetRoute(), deleteRoute().
		 moved inet_lnaof() to inetLib.c.
		 changed remInetAddr() calls to UNIX compatible inet_addr().
		 changed filbuf(0) calls to bzero().
		 added netGetInetAddr().
		 moved nethelp() here from remLib.c.
01m,02may87,dnw  removed unnecessary includes.
		 removed initialization of obsolete "hz".
		 moved call to ifinit() to netStart() from usrConfig.c.
01l,04apr87,jlf  documentation fix.
01k,03apr87,llk  documentation.
01j,02apr87,jlf  more delinting. grrrrrr.
01i,01apr87,jlf  more documentation.
		 delinted.
	    ecs  added include of strLib.h.
01h,26mar87,rdc  added addNetRoute().
01g,23mar87,jlf  documentation.
01f,27feb87,dnw  changed to spawn netd UNBREAKABLE.
01e,10dec86,dnw  reduced TO_DO_RING_SIZE from 10000 to 2000.
01d,19nov86,llk  made netd unbreakable.
01c,08nov86,dnw  changed netd to perform all actions at splnet().
		 changed to spawn netd at "netPriority" level,
		   and let spawn choose task id (saved in netdId).
		 fixed bug of trying to report error with interrupts
		   disabled.
01b,06nov86,rdc  ifconfig and the routing stuff had some unclosed sockets.
01a,28jul86,rdc  written.
*/

/*
DESCRIPTION
This library contains the network task that runs low-level network
interface routines in a task context.  The network task executes and
removes routines that were added to the job queue.  This facility is used
by network interfaces in order to have interrupt-level processing at
task level.

The routine netLibInit() initializes the network and spawns the network
task netTask().  This is done automatically when INCLUDE_NET_LIB is defined.

The routine netHelp() in usrLib displays a summary of the network facilities
available from the VxWorks shell.

INCLUDE FILES: netLib.h

SEE ALSO: routeLib, hostLib, netDrv, netHelp(),
*/

#include "vxWorks.h"
#include "rngLib.h"
#include "semLib.h"
#include "taskLib.h"
#include "errnoLib.h"
#include "rebootLib.h"
#include "sys/socket.h"
#include "sockLib.h"
#include "logLib.h"
#include "intLib.h"
#include "netLib.h"
#include "remLib.h"
#include "net/mbuf.h"
#include "net/if_subr.h"
#include "net/unixLib.h"
#include "net/domain.h"
#include "net/route.h"
#include "private/semLibP.h"
#include "private/funcBindP.h"
#include "netinet/if_ether.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "routeEnhLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsMcast.h"
#endif

IMPORT void ifresetImmediate (void);            /* in netinet/if.c */
 
#ifndef VIRTUAL_STACK
IMPORT struct domain inetdomain; 		/* in netinet/in_proto.c */
#endif

IMPORT void splSemInit (void); 			/* in netinet/unixLib.c */
IMPORT void mbinit (void); 			/* in netinet/uipc_mbuf.c */
IMPORT void mcastHashInit (void); 		/* initialize mcast hsh tbl */

typedef struct
    {
    FUNCPTR routine;	/* routine to be called */
    int param1;		/* arg to routine */
    int param2;
    int param3;
    int param4;
    int param5;
    } TODO_NODE;

#define JOB_RING_SIZE (85 * sizeof (TODO_NODE))

/* local variables */

LOCAL SEMAPHORE netTaskSem;		/* netTask work-to-do sync-semaphore */
LOCAL RING_ID   netJobRing;		/* ring buffer of net jobs to do */

/* global variables */

SEM_ID netTaskSemId = &netTaskSem;

int netTaskId;
int netTaskPriority  = 50;
int netTaskOptions   = VX_SUPERVISOR_MODE | VX_UNBREAKABLE;
int netTaskStackSize = 10000;
int netLibInitialized = FALSE;

#ifndef VIRTUAL_STACK
int _protoSwIndex    = 0; 	/* index for number of protocols initialized */

VOIDFUNCPTR 	_icmpErrorHook		= NULL;	/* icmp error Hook */
FUNCPTR 	_mCastRouteFwdHook	= NULL;	/* mcast forwarding hook */
#endif

/* various netinet wrs internal hooks added for scalability */

VOIDFUNCPTR 	_igmpJoinGrpHook  	= NULL;	/* igmp join Hook */
VOIDFUNCPTR 	_igmpLeaveGrpHook 	= NULL;	/* igmp leave Hook */
FUNCPTR     	_ipFilterHook		= NULL; /* ip filter/firewall Hook */
FUNCPTR  	_mCastRouteCmdHook	= NULL;	/* mcast route command hook */

VOIDFUNCPTR	rtMissMsgHook		= NULL; /* route miss message hook */
VOIDFUNCPTR	rtIfaceMsgHook		= NULL; /* netwk interface msg hook */
VOIDFUNCPTR	rtNewAddrMsgHook	= NULL; /* new address msg hook */


/******************************************************************************
*
* netLibGeneralInit - initialize the various network code
*
* This code use to be in netLibInit. With virtual stacks, we need these
* specific routines to be executed on a per virtual stack basis.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void netLibGeneralInit (void)
    {

    /*
     * Original startup code: uses contents of in_proto.c module 
     * for inetdomain setup. The virtual stack startup sequences
     * uses virtualStackInit() to setup the stack-specific
     * environments instead of that module.
     */
    splSemInit ();			/* initialize spl semaphore */

    mbinit ();				/* network buffer initialization */

#ifdef VIRTUAL_STACK
    arpLibInit();			/* initialize the VS part of arpLib */
#endif /* VIRTUAL_STACK */

    ifinit ();				/* generic interface initialization */

    addDomain (&inetdomain); 		/* add the internet domain */
    
    domaininit ();			/* intialize all domains */

#ifndef VIRTUAL_STACK
    route_init (); 			/* routing table intialization */
#else
    routeStorageCreate ();
#endif /* VIRTUAL_STACK */

#ifdef ROUTER_STACK
    routeIntInit ();                    /* routing enhancement initialization */
#endif /* ROUTER_STACK */
    
    netTypeInit ();			/* initialize netTypes */

    mcastHashInit ();	 		/* defined in in.c */

    }

/*******************************************************************************
*
* netLibInit - initialize the network package
*
* This creates the network task job
* queue, and spawns the network task netTask().  It should be called once to
* initialize the network.  This is done automatically when INCLUDE_NET_LIB
* is defined.
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  This restriction does not apply under non-AE 
* versions of VxWorks.  
*
* RETURNS: OK, or ERROR if network support cannot be initialized.
*
* SEE ALSO: usrConfig, netTask()
*/

STATUS netLibInit (void)
    {

    if (netLibInitialized)
#ifndef VIRTUAL_STACK
	return (netTaskId == ERROR ? ERROR : OK);
#else
	{
	/*
	 * This is so we can initialized other virtual stacks
	 * by just calling netLibInit instead of netLibGeneralInit.
	 */
	netLibGeneralInit ();
	return (OK);
	}
#endif /* VIRTUAL_STACK */

    netLibInitialized = TRUE;

    if ((netJobRing = rngCreate (JOB_RING_SIZE)) == (RING_ID) NULL)
	panic ("netLibInit: couldn't create job ring\n");

    if (rebootHookAdd ((FUNCPTR) ifresetImmediate) == ERROR)
	logMsg ("netLibInit: unable to add reset hook\n", 0, 0, 0, 0, 0, 0);
    semBInit (netTaskSemId, SEM_Q_PRIORITY, SEM_EMPTY);

    netLibGeneralInit ();	     /* General initialization of the network */

    netTaskId = taskSpawn ("tNetTask", netTaskPriority,
		           netTaskOptions, netTaskStackSize,
			   (FUNCPTR) netTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    return (netTaskId == ERROR ? ERROR : OK);
    }

/*******************************************************************************
*
* netTask - network task entry point
*
* This routine is the VxWorks network support task.  Most of the VxWorks
* network runs in this task's context.
*
* NOTE
* To prevent an application task from monopolizing the CPU if it is
* in an infinite loop or is never blocked, the priority of netTask()
* relative to an application may need to be adjusted.  Network communication
* may be lost if netTask() is "starved" of CPU time.  The default task
* priority of netTask() is 50.  Use taskPrioritySet() to change the priority
* of a task.
*
* This task is spawned by netLibInit().
*
* VXWORKS AE PROTECTION DOMAINS
* Under VxWorks AE, you can call this function from within the kernel 
* protection domain only.  This restriction does not apply under non-AE 
* versions of VxWorks.  
*
* RETURNS: N/A
*
* SEE ALSO: netLibInit()
*
* INTERNAL
* netTask() reads messages from a ring buffer which is filled by calling
* netJobAdd().
*/

void netTask (void)
    {
    TODO_NODE 	jobNode;

    FOREVER
	{
	/* wait for somebody to wake us up */

	semTake (netTaskSemId, WAIT_FOREVER);

	/* process requests in the toDo list */

	while (rngIsEmpty (netJobRing) == FALSE)
	    {
	    if (rngBufGet (netJobRing, (char *) &jobNode,
			   sizeof (jobNode)) != sizeof (jobNode))
		{
		panic ("netTask: netJobRing overflow!\n");
		}

	    (*(jobNode.routine)) (jobNode.param1, jobNode.param2,
				  jobNode.param3, jobNode.param4,
				  jobNode.param5);
	    }
	}
    }

/*******************************************************************************
*
* netJobAdd - add a routine to the network task job queue
*
* This function allows a routine and up to 5 parameters,
* to be added to the network job queue.
* Only network interfaces should use this function, usually
* to have their interrupt level processing done at task level.
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

STATUS netJobAdd
    (
    FUNCPTR routine,
    int param1,
    int param2,
    int param3,
    int param4,
    int param5
    )
    {
    FAST int oldlevel;
    TODO_NODE newNode;
    BOOL ok;

    newNode.routine = routine;
    newNode.param1 = param1;
    newNode.param2 = param2;
    newNode.param3 = param3;
    newNode.param4 = param4;
    newNode.param5 = param5;

    oldlevel = intLock ();
    ok = rngBufPut (netJobRing, (char *) &newNode, sizeof (newNode)) ==
							sizeof (newNode);
    intUnlock (oldlevel);

    if (!ok)
	{
	panic ("netJobAdd: ring buffer overflow!\n");
	return (ERROR);
	}

    /* wake up the network daemon to process the request */

    semGive (netTaskSemId);

    return (OK);
    }
/*******************************************************************************
*
* netErrnoSet - set network error status
*
* netErrnoSet calls errnoSet () with the given `status' or'd with the
* network status prefix.
*
* NOMANUAL
*/

void netErrnoSet
    (
    int status
    )
    {
    errnoSet (M_errno | status);
    }

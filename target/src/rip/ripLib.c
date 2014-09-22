/* ripLib.c - Routing Information Protocol (RIP) v1 and v2 library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.

 */

/*
modification history
--------------------
02c,22mar02,niq  Merged from Synth view, tor3_x.synth branch, ver 02x
02b,24jan02,niq  SPR 72415 - Added support for Route tags
                 SPR 62733 - Initialize parameters correctly in ripSplitPacket
02a,15oct01,rae  merge from truestack ver 02n, base 01x (SPRs 70188, 69983 etc.) 
01z,10nov00,spm  merged from version 02b of tor3_x branch (multiple SPR fixes)
01y,06nov00,rae  Fixed SPR #26462
01x,16mar99,spm  recovered orphaned code from tor1_0_1.sens1_1 (SPR #25770)
01w,05oct98,spm  made task parameters adjustable and optimized stack 
                 size (SPR #22422)
01v,11sep98,spm  general overhaul - moved expanded ripShutdown routine
                 from ripTimer.c (SPR #22352); altered ripBuildPacket to 
                 allow class-based masks for internal supernets (SPR #22350)
01u,01sep98,spm  changed ripBuildPacket to include correct netmask for
                 classless routing (SPR #22220 and #22065); added support
                 for default next hop routers (SPR #21940)
01t,26jun98,spm  corrected ripBuildPacket to test version against MIB values 
                 (allowing RIPv1 updates) and added subnet test to use valid
                 router in RIPv2 updates; moved semaphore creation before 
                 first use; changed RIP_MCAST_ADDR constant from string to
                 value; added ripClearInterfaces routine needed to comply
                 with ANVL RIP tests; removed compiler warnings
01s,14dec97,jdi  doc: cleanup.
01r,21oct97,kbw  made minor man page font fix
01q,06oct97,gnn  added sendHook routines and cleaned up warnings
01p,04aug97,kbw  fixed man page problems found in beta review
01o,02jun97,gnn  fixed SPR 8685 so that the timer task does not respawn.
01n,16may97,gnn  added code to implement leaking routes.
                 modified ripSplitPacket to handle stupid packets (ANVL).
                 renamed myHook to ripAuthHook.
01m,08may97,gnn  fixed an authentication bug.
01l,05may97,rjc  changed error return value to m2Lib stuff.
01k,28apr97,gnn  added some descriptive text.
01j,20apr97,kbw  fixed man page format, did spell check.
01h,18apr97,gnn  removed device specific code.
01g,17apr97,gnn  fixed errors pointed out by ANVL.
01f,14apr97,gnn  added authentication hook routines.
01e,07apr97,gnn  removed device specific code.
01d,07apr97,gnn  cleared up some of the more egregious warnings.
                 added MIB-II interfaces and options.
01c,12mar97,gnn  added multicast support.
                 added time variables.
01b,24feb97,gnn  added routines for version 2 support.
01a,26nov96,gnn  created from BSD4.4 routed main.c
*/

/*
DESCRIPTION
This library implements versions 1 and 2 of the Routing Information Protocol 
(RIP). The protocol is intended to operate as an interior gateway protocol
within a relatively small network with a longest path of 15 hops.

HIGH-LEVEL INTERFACE
The ripLibInit() routine links this library into the VxWorks image and begins
a RIP session. This happens automatically if INCLUDE_RIP is defined at the
time the image is built. Once started, RIP will maintain the network routing 
table until deactivated by a call to the ripShutdown() routine, which will 
remove all route entries and disable the RIP library routines. All RIP
requests and responses are handled as defined in the RFC specifications.
RFC 1058 defines the basic protocol operation and RFC 1723 details the
extensions that constitute version 2.

When acting as a supplier, outgoing route updates are filtered using simple
split horizon. Split horizon with poisoned reverse is not currently available.
Additional route entries may be excluded from the periodic update with the
ripSendHookAdd() routine. 

If a RIP session is terminated, the networking subsystem may not function 
correctly until RIP is restarted with a new call to ripLibInit() unless
routing information is provided by some other method.

CONFIGURATION INTERFACE
By default, a RIP session only uses the network interfaces created before it
started. The ripIfSearch() routine allows RIP to recognize any interfaces 
added to the system after that point. If the address or netmask of an
existing interface is changed during a RIP session, the ripIfReset() 
routine must be used to update the RIP configuration appropriately.
The current RIP implementation also automatically performs the border
gateway filtering required by the RFC specification. Those restrictions
provide correct operation in a mixed environment of RIP-1 and RIP-2 routers.
The ripFilterDisable() routine will remove those limitations, and can produce
more efficient routing for some topologies. However, you must not use that 
routine if any version 1 routers are present. The ripFilterEnable() routine 
will restore the default behavior. 

AUTHENTICATION INTERFACE
By default, authentication is disabled, but may be activated by an SNMP
agent on an interface-specific basis. While authentication is disabled,
any RIP-2 messages containing authentication entries are discarded. When
enabled, all RIP-2 messages without authentication entries are automatically
rejected. To fully support authentication, an authentication routine should
be specified with the ripAuthHookAdd() routine. The specified function
will be called to screen every RIP-1 message and all unverified RIP-2 
messages containing authentication entries. It may be removed with the 
ripAuthHookDelete() routine. All RIP-1 and unverified RIP-2 messages will 
be discarded while authentication is enabled unless a hook is present.

OPTIONAL INTERFACE
The ripLeakHookAdd() routine allows the use of an alternative routing
protocol that uses RIP as a transport mechanism. The specified function
can prevent the RIP session from creating any table entries from the
received messages. The ripLeakHookDelete() routine will restore the
default operation. 

DEBUGGING INTERFACE
As required by the RFC specification, the obsolete traceon and traceoff 
messages are not supported by this implementation. The ripRouteShow()
routine will display the contents of the internal RIP routing table.
Routines such as mRouteShow() to display the corresponding kernel routing 
table will also be available if INCLUDE_NET_SHOW is defined when the image 
is built. If additional information is required, the ripDebugLevelSet() 
routine will enable predefined debugging messages that will be sent to 
the standard output.

INCLUDE FILES: ripLib.h

SEE ALSO: RFC 1058, RFC 1723
*/

/*
 * Routing Table Management Daemon
 */
#include "vxWorks.h"
#include "rip/defs.h"
#include "m2Lib.h"
#include "sys/ioctl.h"
#include "sys/socket.h"
#include "inetLib.h"
#include "taskLib.h"
#include "tickLib.h"
#include "sockLib.h"
#include "sysLib.h"
#include "lstLib.h"
#include "routeEnhLib.h"

#include "net/if.h"

#include "errnoLib.h"
#include "errno.h"

#include "logLib.h"
#include "wdLib.h"
#include "semLib.h"
#include "ioLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsRip.h"
#endif /* VIRTUAL_STACK */

#ifdef RIP_MD5
#include "rip/md5.h"
#endif /* RIP_MD5 */

/* forward declarations. */

IMPORT void addrouteforif(register struct interface *ifp);
IMPORT void ripRouteMetricSet (struct rt_entry * pRtEntry);
void ripIfShow (void);

#define SOCKADDR_IN(s) (((struct sockaddr_in*)(s))->sin_addr.s_addr)

#define RIP_TASK_PRIORITY		101
#define RIP_TIMER_TASK_PRIORITY		100
#define RIP_TASK_STACK_SIZE		3750
#define RIP_TIMER_TASK_STACK_SIZE	3000
#define RIP_TASK_OPTIONS			0
#define RIP_TIMER_TASK_OPTIONS		0

#ifndef VIRTUAL_STACK
/* globals */

RIP 	ripState;
int	routedDebug = 0;
SEM_ID 	ripLockSem;
BOOL 	ripFilterFlag = TRUE;

LIST 	ripIfExcludeList;

/*
 * Settings for primary and timer tasks. For correct operation, the timer
 * task must run at a higher priority than the primary task. The stack
 * sizes are chosen based on the high-water mark measured on a Sparc target,
 * since the high use of registers in that architecture provides a likely
 * maximum. The actual values measured were 2524 bytes for the primary task 
 * and 1824 bytes for the timer task.
 *
 * Use a task priority between 100 and 255 as that is the default priority 
 * range for a user protection domain.
 */

int _ripTaskPriority = RIP_TASK_PRIORITY;
int _ripTaskOptions = RIP_TASK_OPTIONS;
int _ripTaskStackSize = RIP_TASK_STACK_SIZE;

int _ripTimerTaskPriority = RIP_TIMER_TASK_PRIORITY;
int _ripTimerTaskOptions = RIP_TIMER_TASK_OPTIONS;
int _ripTimerTaskStackSize = RIP_TIMER_TASK_STACK_SIZE;

/* locals */

LOCAL BOOL ripInitFlag = FALSE;

IMPORT struct interface *ripIfNet;
IMPORT struct interface **ifnext;

#endif /* VIRTUAL_STACK */

#ifdef ROUTER_STACK
LOCAL char rtmMessages [][16] = {
    "ILLEGAL",
    "RTM_ADD",
    "RTM_DELETE",
    "RTM_CHANGE",
    "RTM_GET",
    "RTM_LOSING",
    "RTM_REDIRECT",
    "RTM_MISS",
    "RTM_LOCK",
    "RTM_OLDADD",
    "RTM_OLDDEL",
    "RTM_RESOLVE",
    "RTM_NEWADDR",
    "RTM_DELADDR",
    "RTM_IFINFO",
    "RTM_ADDEXTRA",
    "RTM_DELEXTRA",
    "RTM_NEWCHANGE",
    "RTM_NEWGET",
    "RTM_GETALL",
    "RTM_NEWIPROUTE",
    "RTM_OLDIPROUTE"
    };
#endif /* ROUTER_STACK */

/* defines */

#define BUFSPACE 127*1024
#define ROUNDUP(a) \
        ((a) > 0 ? (1 + (((a) - 1) | (sizeof (long) - 1))) : sizeof (long))
#define ADVANCE(x, n) (x += ROUNDUP ((n)->sa_len))
 

/* forward declarations */
   
void rtdeleteall ();
IMPORT void ripTimer ();
int ripTask ();
void timevaladd (struct timeval *t1, struct timeval *t2);
void timevalsub (struct timeval *t1, struct timeval *t2);
void ripTimeSet (struct timeval *pTimer);
void process (int fd);
#ifdef ROUTER_STACK
LOCAL void ripRouteMsgProcess (void);
LOCAL BOOL ripRouteSame (struct rt_entry *pRtEntry, struct sockaddr *pDstAddr, 
                         struct sockaddr *pNetmask, struct sockaddr *pGateway);
LOCAL STATUS ripInterfaceUpFlagSet (u_short ifIndex, BOOL up);
LOCAL STATUS ripInterfaceDelete (struct interface *pIf);
#endif /* ROUTER_STACK */
void ripTimerArm (long timeout);
int getsocket (int domain, int type, struct sockaddr_in *sin);
void routedTableInit ();
void rtdefault ();
STATUS routedIfInit (BOOL resetFlag, long ifIndex);
void routedInput (struct sockaddr *from, register RIP_PKT *rip, int size);
void toall (int (*f)(), int rtstate, struct interface *skipif);
LOCAL STATUS ripRoutesDelete (struct interface *pIf, BOOL deleteAllRoutes);
LOCAL STATUS ripInterfaceIntDelete (struct interface *pIf, 
                                    struct interface *pPrevIf);
void _ripAddrsXtract (caddr_t cp, caddr_t cpLim, struct rt_addrinfo *pRtInfo);

/******************************************************************************
*
* ripLibInit - initialize the RIP routing library
*
* This routine creates and initializes the global data structures used by 
* the RIP routing library and starts a RIP session to maintain routing 
* tables for a host. You must call ripLibInit() before you can use any other 
* ripLib routines. A VxWorks image automatically invokes ripLibInit() 
* if INCLUDE_RIP was defined when the image was built.
*
* The resulting RIP session will monitor all network interfaces that are 
* currently available for messages from other RIP routers. If the <supplier>
* parameter is true, it will also respond to specific requests from other
* routers and transmit route updates over every known interface at the 
* interval specified by <supplyInterval>.
*
* Specifying a <gateway> setting of true establishes this router as a
* gateway to the wider Internet, capable of routing packets anywhere within 
* the local networks. The final <multicast> flag indicates whether the
* RIP messages are sent to the pre-defined multicast address of 224.0.0.9
* (which requires a <version> setting of 2) or to the broadcast address of 
* the interfaces.
*
* The <version> parameter determines the format used for outgoing RIP 
* messages, and also sets the initial settings of the MIB-II compatibility 
* switches in combination with the <multicast> flag. A <version> of 1 will 
* restrict all incoming traffic to that older message type. A <version> of 
* 2 will set the receive switch to accept either type unless <multicast> is 
* true, which limits reception to version 2 messages only. SNMP agents may 
* alter those settings on a per-interface basis once startup is complete.
*
* The remaining parameters set various system timers used to maintain the
* routing table. All of the values are expressed in seconds, and must be
* greater than or equal to 1. The <timerRate> determines how often
* the routing table is examined for changes and expired routes. The
* <supplyInterval> must be an exact multiple of that value. The
* <expire> parameter specifies the maximum time between updates before
* a route is invalidated and removed from the kernel table. Expired routes 
* are then deleted from the internal RIP routing table if no update has
* been received within the time set by the <garbage> parameter.
*
* The following configuration parameters determine the initial values for
* all these settings. The default timer values match the settings indicated
* in the RFC specification.
*
* \ts
* Parameter Name    | Default Value   | Configuration Parameter
* ------------------+-----------------+------------------------
* <supplier>        | 0 (FALSE)       | RIP_SUPPLIER
* <gateway>         | 0 (FALSE)       | RIP_GATEWAY
* <multicast>       | 0 (FALSE)       | RIP_MULTICAST
* <version>         | 1               | RIP_VERSION
* <timerRate>       | 1               | RIP_TIMER_RATE
* <supplyInterval>  | 30              | RIP_SUPPLY_INTERVAL
* <expire>          | 180             | RIP_EXPIRE_TIME
* <garbage>         | 300             | RIP_GARBAGE_TIME
* <authType>        | 1               | RIP_AUTH_TYPE
* \te
*
* INTERNAL
* This routine creates two tasks, 'tRip' and 'tRipTimer'.  The first is the 
* main loop of the routing task that monitors the routing port (520) for 
* updates and request messages. The second task uses a watchdog timer and
* signalling semaphore to update the internal RIP routing table. The 
* 'ripLockSem' blocking semaphore provides any necessary interlocking between 
* each of the tasks and the user routines that alter the RIP configuration.
* 
* RETURNS: OK; or ERROR, if configuration fails.
*
*/

STATUS ripLibInit
    (
    BOOL supplier,	    /* operate in silent mode? */
    BOOL gateway,	    /* act as gateway to the Internet? */
    BOOL multicast,	    /* use multicast or broadcast addresses? */
    int version,            /* 1 or 2: selects format of outgoing messages */
    int timerRate,          /* update frequency for internal routing table */
    int supplyInterval,     /* update frequency for neighboring routers */
    int expire,             /* maximum interval for renewing learned routes */
    int garbage,            /* elapsed time before deleting stale route */
    int authType            /* default authentication type to use */
    )
    {
    IMPORT STATUS m2RipInit ();

#ifdef VIRTUAL_STACK
    if ((vsTbl [myStackNum]->pRipGlobals != NULL) &&
        (ripInitFlag))
        return (OK);
#else
    if (ripInitFlag)
        return (OK);
#endif /* VIRTUAL_STACK */

    if (version < 1 || version > 2)
        return (ERROR);

    if ((version < 2) && (multicast == TRUE))
        return (ERROR);
    
    if (timerRate < 1)
        return (ERROR);

    if (supplyInterval < 1)
        return (ERROR);

    if (expire < 1)
        return (ERROR);

    if (garbage < 1)
        return (ERROR);

#ifdef VIRTUAL_STACK
    /* Allocate space for former global variables. */
    if (vsTbl[myStackNum]->pRipGlobals == NULL)
        {
        vsTbl[myStackNum]->pRipGlobals = malloc (sizeof (VS_RIP));
        if (vsTbl[myStackNum]->pRipGlobals == NULL)
            return (ERROR);
        }

    bzero ((char *)vsTbl [myStackNum]->pRipGlobals, sizeof (VS_RIP));
#else
    bzero ((char *)&ripState, sizeof (ripState));
#endif /* VIRTUAL_STACK */
 
    /* Set the global state. */

    ripState.version = version;
    ripState.multicast = multicast;
    ripState.timerRate = timerRate;
    ripState.supplyInterval = supplyInterval;
    ripState.expire = expire;
    ripState.garbage = garbage;
    ripState.pRouteHook = NULL;

    ripFilterFlag = TRUE; 	/* Enable border gateway filtering. */
    routedDebug = 0; 		/* Disable debugging messages by default. */

    /* Initialize the list of interfaces on which RIP should not be started */

    lstInit (&ripIfExcludeList);

#ifdef VIRTUAL_STACK
    /*
     * Assign (former) global variables previously initialized by the compiler.
     * Setting 0 is repeated for clarity - the vsLib.c setup zeroes all values.
     */

   _ripTaskPriority = RIP_TASK_PRIORITY; 		/* ripLib.c */
   _ripTaskOptions = RIP_TASK_OPTIONS; 			/* ripLib.c */
   _ripTaskStackSize = RIP_TASK_STACK_SIZE; 		/* ripLib.c */
   _ripTimerTaskPriority = RIP_TIMER_TASK_PRIORITY; 	/* ripLib.c */
   _ripTimerTaskOptions = RIP_TIMER_TASK_OPTIONS; 	/* ripLib.c */
   _ripTimerTaskStackSize = RIP_TIMER_TASK_STACK_SIZE; 	/* ripLib.c */

   ifnext = &ripIfNet;
#endif

    /*
     * This all has to do with RIP MIB II stuff.
     *
     * We set a global configuration that all interfaces will have
     * at startup time.  SNMP agents can then modify these values
     * on a per interface basis.
     */

    m2RipInit ();

    if (version == 1)
        ripState.ripConf.rip2IfConfSend = M2_rip2IfConfSend_ripVersion1;
    else if (multicast)
        ripState.ripConf.rip2IfConfSend = M2_rip2IfConfSend_ripVersion2;
    else
        ripState.ripConf.rip2IfConfSend = M2_rip2IfConfSend_rip1Compatible;
        
    if (version == 1)
        ripState.ripConf.rip2IfConfReceive = M2_rip2IfConfReceive_rip1;
    else if (multicast)
        ripState.ripConf.rip2IfConfReceive = M2_rip2IfConfReceive_rip2;
    else
        ripState.ripConf.rip2IfConfReceive = M2_rip2IfConfReceive_rip1OrRip2;

    ripState.ripConf.rip2IfConfAuthType = authType;

    ripState.ripConf.rip2IfConfStatus = M2_rip2IfConfStatus_valid;

    /* Create the monitor task to receive and process RIP messages. */

    ripState.ripTaskId = taskSpawn (RIP_TASK, _ripTaskPriority, 
                                    _ripTaskOptions, _ripTaskStackSize, 
                                    ripTask, supplier, gateway, multicast, 
#ifdef VIRTUAL_STACK
                                    myStackNum, 0, 0, 0, 0, 0, 0);
#else
                                    0, 0, 0, 0, 0, 0, 0);
#endif


    if (ripState.ripTaskId == ERROR)
        {
#ifdef VIRTUAL_STACK
        free (vsTbl[myStackNum]->pRipGlobals);
        vsTbl[myStackNum]->pRipGlobals = NULL;
#endif
        return (ERROR);
        }

    ripInitFlag = TRUE;

    return (OK);
    }


STATUS ripTask
    (
    BOOL supplier,
    BOOL gateway,
    BOOL multicast
#ifdef VIRTUAL_STACK
    ,
    int 	stackNum
#endif
    )
    {
    int n, nfd, on;
    struct timeval *pTimeout;
    struct timeval waittime;
    fd_set ibits;
    extern STATUS ripLeakHook ();

#ifdef VIRTUAL_STACK
    register RIP_PKT *query;

    /* Assign virtual stack number to access appropriate data structures. */

    if (virtualStackNumTaskIdSet (stackNum) == ERROR)
        {
        if (routedDebug)
            logMsg ("Unable to access stack data.\n", 0, 0, 0, 0, 0, 0);
        return (ERROR);
        }
    query = (RIP_PKT *)ripState.packet;

#else
    register RIP_PKT *query = (RIP_PKT *)ripState.packet;
#endif

    ripState.msg = (RIP_PKT *)ripState.packet;
    
    /* Fake out getservbyname. */
    ripState.port = htons (RIP_PORT);
    
    ripState.addr.sin_family = AF_INET;
    ripState.addr.sin_port = ripState.port;
    ripState.addr.sin_addr.s_addr = INADDR_ANY;
        
    ripState.s = getsocket (AF_INET, SOCK_DGRAM, &ripState.addr);
    if (ripState.s < 0)
        {
        if (routedDebug)
            logMsg ("Unable to get input/output socket.\n", 0, 0, 0, 0, 0, 0);
        return (ERROR);
        }

    /* 
     * Now open a routing socket so that we can receive routing messages
     * from the Routing system
     */
    
    ripState.routeSocket = socket (AF_ROUTE, SOCK_RAW, 0);
    if (ripState.routeSocket < 0)
        {
        if (routedDebug)
            logMsg ("Unable to get route socket.\n", 0, 0, 0, 0, 0, 0);
        close (ripState.s); 
        return (ERROR);
        }

    /* Set the non-block option on the routing socket */

    on = 1;
    if (ioctl (ripState.routeSocket, FIONBIO, (int) &on) == -1) 
        if (routedDebug) 
            logMsg ("error setting O_NONBLOCK option on route socket.\n", 
                    0, 0, 0, 0, 0, 0);

    /*
     * Turn off the loopback option on the socket as we do not want
     * routing messages for events generated a write on this socket.
     * Currently we only read from this socket, never write to it.
     * So this operation is a virtual no-op, but will come in handy
     * if we ever use routing sockets to add/delete/change routes.
     */

    on = 0;
    if (setsockopt (ripState.routeSocket, SOL_SOCKET, SO_USELOOPBACK, 
                    (char *)&on, sizeof (on)) == ERROR)
        if (routedDebug) 
            logMsg ("error resetting SO_USELOOPBACK option on route socket.\n", 
                    0, 0, 0, 0, 0, 0);

    ripState.supplier = supplier;
    ripState.gateway = gateway;

    /* VxWorks specific setup. */

    ripState.timerDog = wdCreate ();
    ripState.timerSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY);
    if (ripState.timerSem == NULL)
        {
        if (routedDebug)
            logMsg ("Error creating timer semaphore.\n", 0, 0, 0, 0, 0, 0);
        close (ripState.s); 
        close (ripState.routeSocket); 
        wdDelete (ripState.timerDog);
        return (ERROR);
        }

    ripLockSem = semBCreate (SEM_Q_FIFO, SEM_FULL);
    if (ripLockSem == NULL)
        {
        if (routedDebug)
            logMsg ("Error creating mutex semaphore.\n", 0, 0, 0, 0, 0, 0);
        close (ripState.s); 
        close (ripState.routeSocket); 
        wdDelete (ripState.timerDog);
        semDelete (ripState.timerSem);
        return (ERROR);
        }


    /*
     * Setup the hash tables for route entries and create entries to
     * access the directly connected networks through the current
     * interfaces. 
     */

    routedTableInit ();
    if (routedIfInit (TRUE, 0) == ERROR)
        {
        if (routedDebug)
            logMsg ("Error building interface list.\n", 0, 0, 0, 0, 0, 0);
        close (ripState.s); 
        close (ripState.routeSocket); 
        wdDelete (ripState.timerDog);
        semDelete (ripState.timerSem);
        semDelete (ripLockSem);
        return (ERROR);
        }

    /*
     * If configured as a gateway to the wider Internet, add an internal
     * entry to the RIP routing table so that any default routes received 
     * will be ignored.
     */

    if (ripState.gateway > 0)
        rtdefault ();

    if (ripState.supplier < 0)
        ripState.supplier = 0;

    /* 
     * Send a request message over all available interfaces
     * to retrieve the tables from neighboring RIP routers.
     */

    query->rip_cmd = RIPCMD_REQUEST;
    query->rip_vers = ripState.version;
    if (sizeof (query->rip_nets[0].rip_dst.sa_family) > 1)	/* XXX */
        query->rip_nets[0].rip_dst.sa_family = htons ( (u_short)AF_UNSPEC);
    else
        query->rip_nets[0].rip_dst.sa_family = AF_UNSPEC;
    query->rip_nets[0].rip_metric = htonl ( (u_long)HOPCNT_INFINITY);
    toall (sndmsg, 0, NULL);
    
    /* Start the watchdog used by the timer task to send periodic updates. */

    ripTimerArm (ripState.timerRate);

    /* Create the timer task. */

    ripState.ripTimerTaskId = taskSpawn (RIP_TIMER, _ripTimerTaskPriority, 
                                         _ripTimerTaskOptions, 
                                         _ripTimerTaskStackSize,
                                         (FUNCPTR)ripTimer,
#ifdef VIRTUAL_STACK
                                         stackNum, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#else
                                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#endif

    if (ripState.ripTimerTaskId == ERROR)
        {
        if (routedDebug)
            logMsg ("Error creating timer task. : errno=0x%x\n", errno, 0, 0, 0, 
                    0, 0);
        close (ripState.s); 
        close (ripState.routeSocket); 
        wdCancel (ripState.timerDog);
        wdDelete (ripState.timerDog);
        semDelete (ripState.timerSem);
        semDelete (ripLockSem);
        return (ERROR);
        }

    FD_ZERO (&ibits);
    nfd = max (ripState.s, ripState.routeSocket) + 1;	/* 1 + max (fd's) */
    for (;;)
        {
        FD_SET (ripState.s, &ibits);
        FD_SET (ripState.routeSocket, &ibits);

        if (ripState.needupdate)
            {
            /*
             * Changes to the routing table entries occurred within the
             * mandatory (random) quiet period following an earlier
             * triggered update. Set the selection interval for incoming 
             * messages to the remaining delay so that the pending 
             * (cumulative) triggered update will be sent on schedule.
             */

            pTimeout = &waittime;

            waittime = ripState.nextbcast;
            timevalsub (&waittime, &ripState.now);
            if (waittime.tv_sec < 0)
                {
                /* 
                 * The scheduled update time has passed. Just poll the
                 * interface before sending the triggered update.
                 */

                waittime.tv_sec = 0;
                waittime.tv_usec = 0;
                }
            if (routedDebug)
                logMsg ("Pending dynamic update scheduled in %d/%d sec/usec\n",
                        waittime.tv_sec, waittime.tv_usec, 0, 0, 0, 0);
            }
        else
            {
            /* 
             * No triggered updates are pending. Wait for 
             * messages indefinitely. 
             */

            pTimeout = (struct timeval *)NULL;
            }


        n = select (nfd, &ibits, 0, 0, pTimeout);
        if (n <= 0)
            {
            /*
             * No input received during specified interval: generate 
             * (delayed) triggered update if needed. Ignore all other 
             * errors (e.g. EINTR).
             */

            if (n < 0)
                {
                if (errno == EINTR)
                    continue;
                if (routedDebug)
                    logMsg ("Error %d (%x) from select call", 
                            n, errno, 0, 0, 0, 0);
                }

            /* Block timer task to prevent overlapping updates. */

            semTake (ripLockSem, WAIT_FOREVER);
            if (n == 0 && ripState.needupdate)
                {
                /* 
                 * The pending triggered update was not subsumed by a
                 * regular update during the selection interval. Send it
                 * and reset the update flag and timers.
                 */

                if (routedDebug)
                    logMsg ("send delayed dynamic update\n", 0, 0, 0, 0, 0, 0);

                ripTimeSet (&ripState.now);

                toall (supply, RTS_CHANGED, (struct interface *)NULL);

                ripState.lastbcast = ripState.now;
                ripState.needupdate = 0;
                ripState.nextbcast.tv_sec = 0;
                }
            semGive (ripLockSem);

            continue;
            }

        ripTimeSet (&ripState.now);

        /* 
         * Block the timer task to prevent overlapping updates or
         * route addition/deletion during 
         * processing of RIP and route messages.
         */

        semTake  (ripLockSem, WAIT_FOREVER);

        /* If RIP messages have arrived, process them */

        if (FD_ISSET (ripState.s, &ibits))
            process (ripState.s);

        semGive (ripLockSem);

#ifdef ROUTER_STACK
        /* If there are any routing messages, process them */

        if (FD_ISSET (ripState.routeSocket, &ibits))
            ripRouteMsgProcess ();
#endif /* ROUTER_STACK */
	}
    }

void timevaladd (t1, t2)
	struct timeval *t1, *t2;
{

	t1->tv_sec += t2->tv_sec;
	if ((t1->tv_usec += t2->tv_usec) > 100000) {
		t1->tv_sec++;
		t1->tv_usec -= 1000000;
	}
}

void timevalsub (t1, t2)
	struct timeval *t1, *t2;
{

	t1->tv_sec -= t2->tv_sec;
	if ((t1->tv_usec -= t2->tv_usec) < 0) {
		t1->tv_sec--;
		t1->tv_usec += 1000000;
	}
}

/******************************************************************************
*
* ripTimeSet - update a RIP timer
*
* This routine sets the various RIP timers used to track periodic updates
* to the elapsed time since startup. Because all events use relative
* times, the actual (calendar) time is not necessary.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void ripTimeSet
    (
    struct timeval * 	pTimer 		/* destination for timestamp */
    ) 
    {
    int clockrate;
    ULONG tickcount;

    tickcount = tickGet ();
    clockrate = sysClkRateGet ();

    pTimer->tv_sec = tickcount / clockrate;
    pTimer->tv_usec = (1000000 * tickcount % clockrate) / clockrate;

    return;
    }

void process (fd)
	int fd;
{
	struct sockaddr from;
	int fromlen, cc;
	union {
		char	buf[MAXPACKETSIZE+1];
		RIP_PKT rip;
	} inbuf;

        bzero ((char *)&from, sizeof (from));
	for (;;) {
		fromlen = sizeof (from);
		cc = recvfrom (fd, (char *)&inbuf, sizeof (inbuf), 0, 
                               &from, &fromlen);
		if (cc <= 0) {
			if (cc < 0 && errno != EWOULDBLOCK)
                            {
                            if (routedDebug)
				logMsg ("Error %d (%x) reading RIP message.\n",
                                        cc, errno, 0, 0, 0, 0);
                            }
			break;
		}
		if (fromlen != sizeof (struct sockaddr_in))
			break;
		routedInput (&from, &inbuf.rip, cc);
	}
}

/******************************************************************************
*
* _ripAddrsXtract - extract socket addresses
*
* This routine is a copy of the rt_xaddrs() routine in rtsock.c
* Copied here so that RIP can access it from a non-kernel domain
* it basicaly fills in the rti_info array with pointers to the
* sockaddr structures denoted by the rti_addrs field.
* The sockaddr structures themselves start at <cp> and end at <cpLim>.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void _ripAddrsXtract
    (
    caddr_t		cp,
    caddr_t		cpLim, 
    struct rt_addrinfo *	pRtInfo
    ) 
    {
    register struct sockaddr *	pSockAddr; 
    register int 			i;
 
    bzero ((char *)pRtInfo->rti_info, sizeof (pRtInfo->rti_info));
    for (i = 0; (i < RTAX_MAX) && (cp < cpLim); i++) 
        {
        if ((pRtInfo->rti_addrs & (1 << i)) == 0)
            continue;
        pRtInfo->rti_info[i] = pSockAddr = (struct sockaddr *)cp;
        if (pSockAddr->sa_len == 0)
            pRtInfo->rti_info[i] = NULL;
        ADVANCE (cp, pSockAddr);
        }
    }

/******************************************************************************
*
* ripAddrsXtract - extract socket address pointers from the route message
*
* This routine extracts the socket addresses from the route message in 
* <pRtInfo> and uses the other parameters to return pointers to the 
* extracted messages.
* \is
* \i <pRtInfo>
* Passes in a pointer to a route information message. 
* \i <pDstAddr>    
* Returns a pointer to the destination address.
* \i <pNetmask>    
* Returns a pointer to the netmask.
* \i <pGateway>    
* Returns a pointer to the gateway address.
* \i <pOldGateway>  
* Returns a pointer to the OLD gateway address if it exists.
* \ie
*              
* If the route message doesn't specify an address, the corresponding
* address pointer is set to NULL
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void ripAddrsXtract
    (
    ROUTE_INFO *	pRtInfo,	/* Route information message */
    struct sockaddr **	pDstAddr,	/* Where to store the Destination addr
                                           pointer */
    struct sockaddr **	pNetmask,	/* Where to store the netmask pointer*/
    struct sockaddr **	pGateway,	/* Where to store the Gateway addr
                                           pointer */
    struct sockaddr **	pOldGateway	/* Where to store the Old gateway addr
                                           (if any) pointer */
    ) 
    {
    struct rt_addrinfo		rtInfo; 

    /* First extract pointers to the addresses into the info structure */

    rtInfo.rti_addrs = pRtInfo->rtm.rtm_addrs;
    _ripAddrsXtract ((caddr_t)pRtInfo->addrs, 
                     (caddr_t)&pRtInfo->addrs[RTAX_MAX], 
                     &rtInfo);
 
    /* Now set the users's pointers to point to the addresses */

    *pDstAddr = INFO_DST (&rtInfo);
    *pNetmask = INFO_MASK (&rtInfo);
    *pGateway = INFO_GATE (&rtInfo);
    *pOldGateway = INFO_AUTHOR (&rtInfo);
    }


/******************************************************************************
*
* ripSockaddrPrint - print a sockaddr structure
*
* This routine is a copy of the db_print_sa() routine in if_ether.c
* Copied here so that RIP can access it from a non-kernel domain.
* It prints out the sockaddr structure pointed to by <pSockAddr>
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void ripSockaddrPrint
    (
    struct sockaddr *	pSockAddr	/* Sockaddr to be printed */
    )
    {
    int 		len;
    u_char *	p;

    if (pSockAddr == 0) 
        {
        printf ("[NULL]\n");
        return;
        }

    p = (u_char *)pSockAddr;
    len = pSockAddr->sa_len;
    printf ("[");
    while (len > 0) 
        {
        printf ("%d", *p);
        p++; len--;
        if (len) 
            printf (",");
        }
    printf ("]\n");
    }

#ifdef ROUTER_STACK

/***************************************************************************
*
* ripRouteMsgProcess - Process the routing socket messages
*
* This function retrieves the messages from the routing sockets 
* and processes them. It handles the following messages:
*
* RTM_IFINFO:
*       This message indicates if an interface has been brought up or down.
*       If the interface is brought up, this routine does the following:
*         For all interfaces that match the name, it marks the interface UP
*         and adds the interface route.
*         Then it calls the routedIfInit() call to pick up any addresses that
*         were added to the interface when the interface was down
*       If the interface is brought down, this routine does the following:
*         For all interfaces that match the name, it marks the interface DOWN.
*         Routes passing through the interface are deleted except the
*         routes we learned through the Routing socket messages. Those routes
*         will not be advertised though.
* RTM_NEWADDR:
*	Calls the routedIfInit() routine to pick up the new address. If the
*	interface is down, routedIfInit() doesn't do anything. The new address
*	is picked up when the interface comes back up.
* RTM_DELADDR:
*	Deletes all routes that pass though the interface whose address
*	matches the deleted address and then deletes the interface structure
*	Deleting the interface structure would cause the MIB settings for
*	the structure to be lost, but that is OK as the MIB settings do
*	not apply to an interface name, but to an interface addr. Thus if
*	the address goes away, it is reasonable to remove whatever settings
*	were associated with it.
* RTM_ADD:
* RTM_ADDEXTRA:
*	This message indicates that a route has been added. If the route 
*	is installed by us or the system (interface route) it is ignored.
*	All other routes are passed to the user route handler hook routine
*	if it is installed. If the handler is not installed the route is
*	ignored. If the handler returns a metric < HOPCNT_INFINITY, then we
*	install the route if none exists already. If a route already exists
*	for the same destination then we use the following logic to determine
*	if we need to replace the old route with the new one:
*	  If the old route is a RIP route, we replace it with the new one
*	  If the metric of the old route is greater than or equal to
*         the metric of the new route, then we replace the old route.
*	RTM_ADD signifies that it the route is the primary route and
*	RTM_ADDEXTRA signifies a duplicate route. We don't prefer
*	one over the other and let the user route hook do that.	
* RTM_NEWIPROUTE:
*	This message indicates that a formerly duplicate route has been
*	promoted to being a primary route because either the previously
*	primary route was deleted or demoted because of weight change.
*	We treat this just as we would treat the RTM_ADD command, except
*       that we mark the previous primary route as non-primary.
* RTM_DELETE:
* RTM_DELEXTRA:
*	This message indicates that the route has been deleted. Again if
*	the route has been installed by us or by the system, we ignore it,
*	else if the exact same route exists in our table we delete it.
*	RTM_DELETE signifies that the route is the primary route and
*	RTM_DELEXTRA signifies a duplicate route. 
* RTM_OLDIPROUTE:
*	This message indicates that a formerly primary route has been
*	demoted to being a duplicate route because either a new
*	primary route was added or because of weight change.
*	If we have this route in our cache we mark it non-primary.
*	
* RTM_CHANGE:
* RTM_NEWCHANGE:
*	This message indicates that either the route metric and or the
*	route gateway has been changed. We first check if we know about this
*	route. If we don't, we ignore it, else if it is only a metric change,
*	we record that in our copy of the route. If the gateway has changed,
*	we invoke the user route hook routine, if installed, to provide
*	us an alternate metric, else we use the metric in the route. Finally
*	we change the gateway in our copy of the route.
* RTM_REDIRECT:
*	This message indicates a gateway change caused by an ICMP redirect
*	message. This should happen only with static routes. 
*	We simply invoke the route Hook if it is installed to notify the
*	user of the redirect. If is the user's responsibility to deal
*	with the redirect message.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void ripRouteMsgProcess (void) 
    {
    int			cmd;
    int			flags;
    int			routeProto;
    int			metric;
    int			hookFlag;
    long			count; 
    u_short		index;
    struct rt_addrinfo	rtInfo; 
    struct sockaddr *	pDstAddr = NULL; 
    struct sockaddr *	pNetmask = NULL; 
    struct sockaddr *	pGateway = NULL;
    struct sockaddr *	pOldGateway;
    struct interface * 	pIf; 		
    struct rt_entry *	pRtEntry;
    BOOL		primaryRoute;
    union 
        {
        ROUTE_INFO	rtMsg;	/* Route message */
        struct if_msghdr ifMsg;	/* Interface message header */
        struct 
            {
            struct ifa_msghdr	ifa; 
            struct sockaddr	addrs [RTAX_MAX];
            } 		ifaMsg; /* Interface address message */
        } 		msg; 	/* Routing socket message */

    /*
     * Receive the messages from the routing socket. Return when there
     * are no more
     */

    FOREVER
        {
        count = recv (ripState.routeSocket, (char *)&msg, sizeof (msg), 0);
        if (count <= 0) 
            {
            if (count < 0 && errno != EWOULDBLOCK)
                if (routedDebug)
                    logMsg ("Error %d (%x) reading Route message.\n",
                            count, errno, 0, 0, 0, 0);
            return;
            }

        /* Skip if not the correct version of routing socket message */

        if (msg.rtMsg.rtm.rtm_version != RTM_VERSION) 
            {
            logMsg ("Bogus routing message version %d\n", 
                    msg.rtMsg.rtm.rtm_version, 0, 0, 0, 0, 0);
            continue;
            }

        cmd = msg.rtMsg.rtm.rtm_type;

        /*
         * Retrieve the interface index and validate parameters
         * based on the command value
         */

        switch (cmd)
            {
            case RTM_ADD:		/* Fall through */
            case RTM_DELETE:	/* Fall through */
            case RTM_CHANGE:	/* Fall through */
            case RTM_REDIRECT:	/* Fall through */
            case RTM_ADDEXTRA:	/* Fall through */
            case RTM_DELEXTRA:	/* Fall through */
            case RTM_NEWCHANGE:	/* Fall through */
            case RTM_NEWIPROUTE:	/* Fall through */
            case RTM_OLDIPROUTE:	

                index = msg.rtMsg.rtm.rtm_index;

                if (msg.rtMsg.rtm.rtm_flags & RTF_LLINFO) 
                    {
                    if (routedDebug)
                        logMsg ("ignore route message %s for ifindex %d ARP\n", 
                                (int)(rtmMessages [cmd]), index, 0, 0, 0, 0); 
                    continue;
                    }

                /* Now extract the sockaddr structures into the rtInfo array */

                rtInfo.rti_addrs = msg.rtMsg.rtm.rtm_addrs;
                _ripAddrsXtract ((caddr_t)msg.rtMsg.addrs, 
                                 (caddr_t)&msg.rtMsg.addrs[RTAX_MAX], 
                                 &rtInfo);

                /* Set the destination, netmask and gateway address pointers */

                pDstAddr = INFO_DST (&rtInfo);
                pGateway = INFO_GATE (&rtInfo);
                pNetmask = INFO_MASK (&rtInfo);

                /* So some sanity checking on the addresses */

                /* No destination address ? Reject it */

                if (pDstAddr == 0) 
                    {
                    if (routedDebug)
                        logMsg ("ignore route message %s for ifindex %d without"
                                "dst\n", (int)(rtmMessages [cmd]), 
                                index, 0, 0, 0, 0);
                    continue;
                    }

                /* Destination address doesn't belong to the Internet family */

                if (pDstAddr->sa_family != AF_INET) 
                    {
                    if (routedDebug)
                        logMsg ("ignore route message %s for ifindex %d for AF %d\n",
                                (int)(rtmMessages [cmd]), index, 
                                pDstAddr->sa_family, 0, 0, 0);
                    continue;
                    }

                /* Destination address is a multicast address ?  Reject it */

                if (IN_MULTICAST (ntohl (S_ADDR (pDstAddr)))) 
                    {
                    if (routedDebug)
                        logMsg ("ignore route message %s for ifindex %d multicast\n",
                                (int)(rtmMessages [cmd]), index, 0, 0, 0, 0);
                        continue;
                    }


                /* 
                 * If the gateway address is AF_LINK, this is an interface
                 * route. Ignore it.
                 */

                if (pGateway != NULL) 
                    {
                    if (pGateway->sa_family == AF_LINK) 
                        {
                        if (routedDebug)
                            {
                            logMsg ("Got interface address in %s for ifindex %d\n",
                                    (int)(rtmMessages [cmd]), index, 
                                    0, 0, 0, 0);

                            ripSockaddrPrint (pDstAddr);
                            ripSockaddrPrint (pNetmask);
                            ripSockaddrPrint (pGateway);
                            }

                        continue;
                        }
                    }

                /* 
                 * If the gateway flag is not set, if must be an 
                 * interface route. Ignore it.
                 */

                if ((msg.rtMsg.rtm.rtm_flags & RTF_GATEWAY) == 0) 
                    {
                    if (routedDebug)
                        {
                        logMsg ("got i/f route message %s for ifindex %d ARP\n", 
                                (int)(rtmMessages [cmd]), index, 0, 0, 0, 0); 

                        ripSockaddrPrint (pDstAddr);
                        ripSockaddrPrint (pNetmask);
                        ripSockaddrPrint (pGateway);
                        }

                    continue;
                    }

                break;

            case RTM_NEWADDR:	/* Fall through */
            case RTM_DELADDR:

                index = msg.ifaMsg.ifa.ifam_index;

                /* 
                 * Now extract the sockaddr structures into the
                 * rtInfo array 
                 */

                rtInfo.rti_addrs = msg.ifaMsg.ifa.ifam_addrs;
                _ripAddrsXtract ((caddr_t)msg.ifaMsg.addrs, 
                                 (caddr_t)&msg.ifaMsg.addrs[RTAX_MAX], 
                                 &rtInfo);
                break;

            default:
                index = msg.ifMsg.ifm_index; 
                break;
            }

        if (routedDebug)
            {
            if (cmd <= (sizeof (rtmMessages) / sizeof (rtmMessages[0]))) 
                logMsg ("\nripRouteMsgProcess:cmd = %s Interface index = %d\n", 
                        (int)(rtmMessages [cmd]), index, 0, 0, 0, 0);
            else
                logMsg ("\nripRouteMsgProcess:cmd = %d Interface index = %d\n", 
                        cmd, index, 0, 0, 0, 0);
            }

        /* Now process the actual commands */

        switch (cmd) 
            {
            case RTM_ADD:
            case RTM_ADDEXTRA:
            case RTM_NEWIPROUTE:

                if (msg.rtMsg.rtm.rtm_errno != 0) 
                    {
                    if (routedDebug)
                        logMsg ("ignore route message %s for index %d" 
                                " error = %d\n", (int)rtmMessages [cmd], 
                                index, msg.rtMsg.rtm.rtm_errno, 0, 0, 0);
                    break;
                    } 

                if (routedDebug) 
                    {
                    logMsg ("RIP: %s received for index %d\n", 
                            (int)rtmMessages [cmd], index, 0, 0, 0, 0);
                    ripSockaddrPrint (pDstAddr);
                    ripSockaddrPrint (pNetmask);
                    ripSockaddrPrint (pGateway);
                    }

                /* 
                 * If the route was installed by us or is an interface
                 * route, ignore it. We already know about it
                 */

                routeProto = RT_PROTO_GET (pDstAddr);
                if (routeProto == M2_ipRouteProto_rip ||
                    routeProto == M2_ipRouteProto_local)
                    break;

                primaryRoute = (cmd == RTM_ADDEXTRA) ? FALSE : TRUE;

                /*
                 * The destination sockaddr structure has the 
                 * protocol value set. Make it zero since 
                 * we compare the entire sockaddr structure at other
                 * places (input.c for example) that expect the
                 * standard fields to be zero.
                 * We set the gateway sockaddr fields to be zero
                 * too (just being paranoid, in case they aren't already zero)
                 * as rtlookup() expects it to be zero. Do the same
                 * for the TOS value too.
                 * We store the proto value anyway in the rtentry
                 * structure.
                 */

                RT_PROTO_SET (pGateway, 0);
                TOS_SET (pGateway, 0);
                RT_PROTO_SET (pDstAddr, 0);
                TOS_SET (pDstAddr, 0);

                /*
                 * If a new primary route is coming into being,
                 * we should mark our primary route, if any, as non-primary
                 * We don't need to do this for RTM_ADD as it will be preceded
                 * by a RTM_OLDIPROUTE if there was a pre existing primary
                 * route
                 */

                if (cmd == RTM_NEWIPROUTE)
                    {
                    /* Take the semaphore to lock the RIP timer task out */

                    semTake (ripLockSem, WAIT_FOREVER); 

                    pRtEntry = rtlookup (pDstAddr);
                    if (pRtEntry != NULL && 
                        ripRouteSame (pRtEntry, pDstAddr, pNetmask, NULL))
                        {
                        if (routedDebug)
                            logMsg ("RTM_NEWIPROUTE: demoting primary route\n", 
                                    0, 0, 0, 0, 0, 0); 
                        pRtEntry->rt_state &= ~RTS_PRIMARY;
                        }

                    /*
                     * If we have the exact same route in our table,
                     * then mark it as primary 
                     */

                    if (pRtEntry != NULL && 
                        ripRouteSame (pRtEntry, pDstAddr, pNetmask, pGateway))
                        {
                        if (routedDebug)
                            logMsg ("RTM_NEWIPROUTE: restoring primary route\n", 
                                    0, 0, 0, 0, 0, 0); 
                        pRtEntry->rt_state |= RTS_PRIMARY;
                        }
                    /* Now release the semaphore we took earlier */

                    semGive (ripLockSem);
                    }

addIt:
                /* 
                 * Now set the route hook flag to signify whether it
                 * is an add operation or a change operation.
                 * Note that we can get here from change processing too.
                 * Also we treat RTM_NEWIPROUTE as a change since it is
                 * not a new route that is being added to the system.
                 */

                hookFlag = (cmd == RTM_ADD || cmd == RTM_ADDEXTRA) ?
                    0 : RIP_ROUTE_CHANGE_RECD;

                /*
                 * Call the user hook routine if installed. If the routine
                 * returns a metric less than infinity, add the route to
                 * our table, else just ignore it. The user is responsible
                 * for setting the metric of the route. In case he forgets
                 * to assign a valid metric, assign a default metric of 1
                 */

                if (ripState.pRouteHook == NULL)
                    break; 

                if ((metric = (ripState.pRouteHook)
                     (&msg.rtMsg, routeProto, primaryRoute, hookFlag)) >= 
                    HOPCNT_INFINITY)
                    {
                    /* Take the semaphore to lock the RIP timer task out */

                    semTake (ripLockSem, WAIT_FOREVER); 

                    pRtEntry = rtlookup (pDstAddr);

                    /*
                     * If we have the exact same route in our table,
                     * then delete it as the user doesn't want it.
                     */

                    if (pRtEntry != NULL && 
                        ripRouteSame (pRtEntry, pDstAddr, pNetmask, pGateway))
                        {
                        if (routedDebug)
                            logMsg ("ripRouteMsgProcess:cmd = %s: deleting primary"
                                    "route\n", (int)(rtmMessages [cmd]),
                                    0, 0, 0, 0, 0); 
                        rtdelete (pRtEntry);
                        }

                    /* Now release the semaphore we took earlier */

                    semGive (ripLockSem);

                    break;
                    }

                if (metric < 0)
                    metric = 1;

                /* Take the semaphore to lock the RIP timer task out */

                semTake (ripLockSem, WAIT_FOREVER); 

                /* 
                 * Check if we already have an entry for the specified 
                 * destination. If we do and it is a RIP route 
                 * (RTS_OTHER not set) or if the route belongs to another
                 * protocol, but has a higher or same metric than this new
                 * route, delete the old route. We always give precedence
                 * to OTHER routes over RIP routes. In any case, we 
                 * do not allow an interface route to be replaced. Also
                 * if we have the same route and its metric is being changed
                 * we need to record that change. For simplicity, we just
                 * delete the old route and add the new one.
                 */

                pRtEntry = rtlookup (pDstAddr);
                if ((pRtEntry == NULL) ||
                    (!(pRtEntry->rt_state & RTS_INTERFACE) && 
                     (((pRtEntry->rt_state & RTS_OTHER) == 0) || 
                      (pRtEntry->rt_metric >= metric) || 
                      ripRouteSame (pRtEntry, pDstAddr, pNetmask, pGateway)))) 
                    {
                    if (pRtEntry)
                        rtdelete (pRtEntry);

                    flags = RTS_OTHER;

                    /* Record whether this route is the primary route or not */

                    if (primaryRoute)
                        flags |= RTS_PRIMARY;

                    rtadd (pDstAddr, pGateway, metric, flags,
                           pNetmask, routeProto, 0, 0, NULL);
                    
                    if (routedDebug)
                        logMsg ("%s: Added new route\n", (int)rtmMessages [cmd], 
                                0, 0, 0, 0, 0);
                    }

                /* Now release the semaphore we took earlier */

                semGive (ripLockSem);
                break; 

            case RTM_CHANGE: 	/* Primary route changing */
            case RTM_NEWCHANGE: 	/* Duplicate route changing */

                if (msg.rtMsg.rtm.rtm_errno != 0) 
                    {
                    if (routedDebug)
                        logMsg ("ignore route message %s for index %d" 
                                " error = %d\n", (int)rtmMessages [cmd], 
                                index, msg.rtMsg.rtm.rtm_errno, 0, 0, 0);
                    break;
                    } 

                /* 
                 * Get the address of the old gateway. This information
                 * is only available for the RTM_NEWCHANGE and not for
                 * RTM_CHANGE (for compatibility reasons)
                 */

                if (cmd == RTM_NEWCHANGE) 
                    {
                    pOldGateway = INFO_AUTHOR (&rtInfo);

                    /* 
                     * If Gateway didn't change, set old gateway to
                     * the previous gateway address, so that ripRouteSame
                     * can work properly.
                     */

                    if (pOldGateway == NULL)
                        pOldGateway = pGateway;

                    if (pOldGateway == NULL)
                        {
                        /*
                         * This is a route change for a duplicate route
                         * for which the gateway didn't change.
                         * Also, we don't have enough information
                         * to determine if we have this particular route
                         * in our cache. We need a gateway
                         * address to identify a duplicate route.
                         * Since we can't do that, simply exit.
                         */

                        if (routedDebug)
                            logMsg ("RTM_NEWCHANGE: gateway not specified. "
                                    "Doing nothing.\n", 0, 0, 0, 0, 0, 0);
                        break;
                        }
                    }
                else
                    pOldGateway = NULL;

                if (routedDebug) 
                    {
                    logMsg ("RIP: %s received\n", 
                            (int)rtmMessages [cmd],0,0,0,0,0);
                    ripSockaddrPrint (pDstAddr);
                    ripSockaddrPrint (pNetmask);
                    ripSockaddrPrint (pGateway);
                    logMsg ("RIP: %s received: old Gateway addr\n", 
                            (int)rtmMessages [cmd],0,0,0,0,0);
                    ripSockaddrPrint (pOldGateway);

                    logMsg ("RIP: new metric %d\n", 
                            msg.rtMsg.rtm.rtm_rmx.rmx_hopcount,0,0,0,0,0);
                    }

                /* 
                 * If the route was installed by us or was an interface
                 * route, ignore it. Nobody should be changing this route.
                 */

                routeProto = RT_PROTO_GET (pDstAddr);
                if (routeProto == M2_ipRouteProto_rip ||
                    routeProto == M2_ipRouteProto_local)
                    break;

                /*
                 * The destination sockaddr structure has the 
                 * protocol value set. Make it zero since 
                 * we compare the entire sockaddr structure at other
                 * places (input.c for example) that expect the
                 * standard fields to be zero.
                 * We set the gateway sockaddr fields to be zero
                 * too (just being paranoid, in case they aren't already zero)
                 * as rtlookup() expects it to be zero. Do the same
                 * for the TOS value too.
                 * We store the proto value anyway in the rtentry
                 * structure.
                 */

                if (pGateway)
                    {
                    RT_PROTO_SET (pGateway, 0);
                    TOS_SET (pGateway, 0);
                    }
                RT_PROTO_SET (pDstAddr, 0);
                TOS_SET (pDstAddr, 0);

                /* Take the semaphore to lock the RIP timer task out */

                semTake (ripLockSem, WAIT_FOREVER); 

                /* 
                 * Check if we have this route in our table. 
                 * If we have it, 
                 *   make sure it is the exact
                 *   same route:
                 *     same dst, gateway and netmask values
                 *     same routing protocol
                 *
                 *   If we know about this route, record the gateway change,
                 *   if any. If the hop count field was modified, then
                 *   that indicates that the RIP metric was stored in that
                 *   field; so we copy the metric from that field,
                 *   else we call the user hook for a new metric.
                 *   If they return a metric >= HOPCNT_INFINITY we delete the
                 *   route. else we update the metric.
                 *   Note that pOldGateway would be NULL if the primary route
                 *   is being changed. We account for that in the 
                 *   ripRouteSame routine by checking if the route we have
                 *   is the primary route
                 *
                 * If we don't have the route in our table, call the user
                 * hook to decide if they want us to propagate this route.
                 */

                pRtEntry = rtlookup (pDstAddr);
                if (pRtEntry && 
                    ((pRtEntry->rt_state & RTS_OTHER) != 0) && 
                    (pRtEntry->rt_proto == routeProto) &&
                    ripRouteSame (pRtEntry, pDstAddr, pNetmask, pOldGateway))
                    {

                    /* If the gateway has changed, record the change */

                    if (pGateway && !equal (&pRtEntry->rt_router, pGateway))
                        {

                        /* Now record the new gateway and its interface */

                        pRtEntry->rt_router = *pGateway; 
                        pRtEntry->rt_ifp = 
                            ripIfWithDstAddr (&pRtEntry->rt_router, NULL);
                        if (pRtEntry->rt_ifp == 0) 
                            pRtEntry->rt_ifp = 
                                ripIfWithNet (&pRtEntry->rt_router);

                        pRtEntry->rt_state |= RTS_CHANGED;
                        if (routedDebug)
                            logMsg ("%s: Changed gateway\n", 
                                    (int)rtmMessages [cmd], 0, 0, 0, 0, 0);
                        }

                    /* If the metric changed, record it. */

                    if ((msg.rtMsg.rtm.rtm_inits & RTV_HOPCOUNT) != 0) 
                        {
                        pRtEntry->rt_metric = 
                            msg.rtMsg.rtm.rtm_rmx.rmx_hopcount;

                        pRtEntry->rt_state |= RTS_CHANGED;
                        if (pRtEntry->rt_metric >= HOPCNT_INFINITY)
                            {
                            /* The user wants the route deleted. Do so */

                            rtdelete (pRtEntry);
                            }
                        }
                    else if (ripState.pRouteHook != NULL)
                        {
                        /* Allow the use to specify a new metric */

                        metric = (ripState.pRouteHook) 
                            (&msg.rtMsg, routeProto, (cmd == RTM_CHANGE), 
                             RIP_ROUTE_CHANGE_RECD);

                        /* If the user wants the route to be deleted, do so */

                        if (metric >= HOPCNT_INFINITY)
                            {
                            pRtEntry->rt_state |= RTS_CHANGED;
                            rtdelete (pRtEntry);
                            }
                        else if (metric > 0)
                            {
                            /* Record the metric change */

                            pRtEntry->rt_metric = metric;
                            pRtEntry->rt_state |= RTS_CHANGED;
                            }
                        }
                    }
                else
                    {
                    /* 
                     * This might be a route that was duplicate, but
                     * because it's weight changed for the better, it became
                     * primary. Or it just might be a route that we ere told to
                     * ignore earlier on, but since things have changed,
                     * Allow the user to decide if he wants this
                     * route to be propagated. 
                     */

                    if (routedDebug)
                        logMsg ("%s: Trying to add new route\n", 
                                (int)rtmMessages [cmd], 0, 0, 0, 0, 0);
                    semGive (ripLockSem);
                    primaryRoute = (cmd == RTM_CHANGE) ? TRUE : FALSE;
                    goto addIt;
                    }

                semGive (ripLockSem);

                break;

            case RTM_OLDIPROUTE:

                if (routedDebug) 
                    {
                    logMsg ("RIP: %s received for index %d\n", 
                            (int)rtmMessages [cmd], index, 0, 0, 0, 0);
                    ripSockaddrPrint (pDstAddr);
                    ripSockaddrPrint (pNetmask);
                    ripSockaddrPrint (pGateway);
                    }

                /* 
                 * If the route was installed by us or is an interface
                 * route, ignore it. We don't care about these routes.
                 */

                routeProto = RT_PROTO_GET (pDstAddr);
                if (routeProto == M2_ipRouteProto_rip ||
                    routeProto == M2_ipRouteProto_local)
                    break;

                /*
                 * The destination sockaddr structure has the 
                 * protocol value set. Make it zero since 
                 * we compare the entire sockaddr structure at other
                 * places (input.c for example) that expect the
                 * standard fields to be zero.
                 * We set the gateway sockaddr fields to be zero
                 * too (just being paranoid, in case they aren't already zero)
                 * as rtlookup() expects it to be zero. Do the same
                 * for the TOS value too.
                 * We store the proto value anyway in the rtentry
                 * structure.
                 */

                RT_PROTO_SET (pGateway, 0);
                TOS_SET (pGateway, 0);
                RT_PROTO_SET (pDstAddr, 0);
                TOS_SET (pDstAddr, 0);

                /*
                 * If the current primary route is being demoted
                 * we should mark our primary route, if any, as non-primary
                 */

                /* Take the semaphore to lock the RIP timer task out */

                semTake (ripLockSem, WAIT_FOREVER); 

                pRtEntry = rtlookup (pDstAddr);
                if (pRtEntry != NULL && 
                    ripRouteSame (pRtEntry, pDstAddr, pNetmask, pGateway))
                    {
                    if (routedDebug)
                        logMsg ("RTM_OLDIPROUTE: demoting primary route\n", 
                                0, 0, 0, 0, 0, 0);
                    pRtEntry->rt_state &= ~RTS_PRIMARY;
                    }

                /* Now release the semaphore we took earlier */

                semGive (ripLockSem);
                break;

            case RTM_REDIRECT:

                /* Get the address of the old gateway */

                pOldGateway = INFO_AUTHOR (&rtInfo);

                if (msg.rtMsg.rtm.rtm_errno != 0) 
                    {
                    if (routedDebug)
                        logMsg ("ignore route message RTM_REDIRECT for index %d" 
                                " error = %d\n", 
                                index, msg.rtMsg.rtm.rtm_errno, 0, 0, 0, 0);
                    break;
                    } 

                if (routedDebug) 
                    {
                    logMsg ("RIP: route redirect received for index %d\n", 
                            index, 0, 0, 0, 0, 0);
                    ripSockaddrPrint (pDstAddr);
                    ripSockaddrPrint (pNetmask);
                    ripSockaddrPrint (pGateway);
                    logMsg ("RIP: route redirect old gateway\n", 0,0,0,0,0,0);
                    ripSockaddrPrint (pOldGateway);
                    }

                routeProto = RT_PROTO_GET (pDstAddr);

                /*
                 * The destination sockaddr structure has the 
                 * protocol value set. Make it zero as the user may not expect
                 * to see it. Do the same with the gateway address
                 * even though it might already be zero. it doesn't hurt
                 * being paranoid.
                 */

                RT_PROTO_SET (pGateway, 0);
                TOS_SET (pGateway, 0);
                RT_PROTO_SET (pDstAddr, 0);
                TOS_SET (pDstAddr, 0);

                /* If the routeHook is installed call it */

                if (ripState.pRouteHook != NULL)
                    {
                    (ripState.pRouteHook)(&msg.rtMsg, routeProto, FALSE,
                                          RIP_REDIRECT_RECD);
                    }

                break; 

            case RTM_DELETE:
            case RTM_DELEXTRA:

                if (msg.rtMsg.rtm.rtm_errno != 0 && 
                    msg.rtMsg.rtm.rtm_errno != ESRCH) 
                    {
                    if (routedDebug)
                        logMsg ("ignore route message %s for ifindex"
                                " %d error = %d\n", (int)rtmMessages [cmd],
                                index, msg.rtMsg.rtm.rtm_errno, 0, 0, 0);
                    break;
                    } 

                if (routedDebug) 
                    {
                    logMsg ("route message %s for ifindex %d\n", 
                            (int)rtmMessages [cmd], index, 0, 0, 0, 0);
                    ripSockaddrPrint (pDstAddr);
                    ripSockaddrPrint (pNetmask);
                    ripSockaddrPrint (pGateway);
                    } 

                /* 
                 * If the route was installed by us or is an interface
                 * route, ignore it. We already know about it
                 */

                routeProto = RT_PROTO_GET (pDstAddr);
                if (routeProto == M2_ipRouteProto_rip ||
                    routeProto == M2_ipRouteProto_local)
                    break;

                /*
                 * The destination sockaddr structure has the 
                 * protocol value set. Make it zero since 
                 * we compare the entire sockaddr structure at other
                 * places (input.c for example) that expect the
                 * standard fields to be zero.
                 * We set the gateway sockaddr fields to be zero
                 * too (just being paranoid, in case they aren't already zero)
                 * as rtlookup() expects it to be zero. Do the same
                 * for the TOS value too.
                 * We store the proto value anyway in the rtentry
                 * structure.
                 */

                RT_PROTO_SET (pGateway, 0);
                TOS_SET (pGateway, 0);
                RT_PROTO_SET (pDstAddr, 0);
                TOS_SET (pDstAddr, 0);

                /* Take the semaphore to lock the RIP timer task out */

                semTake (ripLockSem, WAIT_FOREVER); 

                /* 
                 * Check if we have this route in our table. 
                 * If we do, delete it. Make sure it is the exact
                 * same route:
                 *   same dst, gateway and netmask values
                 *   same routing protocol
                 */

                pRtEntry = rtlookup (pDstAddr);
                if (pRtEntry && 
                    ((pRtEntry->rt_state & RTS_OTHER) != 0) &&
                    pRtEntry->rt_proto == routeProto &&
                    ripRouteSame (pRtEntry, pDstAddr, pNetmask, pGateway))
                    {
                    if (routedDebug)
                        logMsg ("%s: deleted route\n", (int)rtmMessages [cmd], 
                                0, 0, 0, 0, 0);
                    rtdelete (pRtEntry);
                    }

                /* Now release the semaphore we took earlier */

                semGive (ripLockSem);
                break; 

            case RTM_IFINFO:

                /* Take the semaphore to lock the RIP timer task out */

                semTake (ripLockSem, WAIT_FOREVER); 

                if ((msg.ifMsg.ifm_flags & IFF_UP) == IFF_UP)
                    {

                    /* 
                     * Now change the interface flag to UP for all interfaces
                     * that match the given name. If this is a new interface
                     * the routine below does nothing. The routine below also
                     * adds the interface route for all logical interfaces.
                     */ 
                    if (routedDebug) 
                        logMsg ("setting up interface for %d\n", 
                                index, 0, 0, 0, 0, 0);

                    ripInterfaceUpFlagSet (index, TRUE);

                    /* 
                     * Either this might be a new interface, or a new address
                     * might have been added to the interface while it was down
                     * in which case the routedIfInit call would have simply
                     * returned without getting the new address.
                     * Call routedIfInit now to get any new addresses.
                     * It is perfectly safe to call routedIfInit() 
                     * even if nothing
                     * has changed since routedIfInit() doesn't do anything
                     * in that case
                     */

                    if (routedDebug)
                        logMsg ("Calling routedIfInit for index %d.\n", 
                                index, 0, 0, 0, 0, 0);

                    routedIfInit (FALSE, index); 
                    }
                else
                    {

                    /* 
                     * Now change the interface flag to DOWN for all interfaces
                     * that match the given name. The routine below will also
                     * delete all routes that pass through the interface
                     * except the routes learnt through the Routing system
                     * callback.
                     */ 
                    if (routedDebug)
                        logMsg ("setting down interface for %d\n", 
                                index, 0, 0, 0, 0, 0); 

                    ripInterfaceUpFlagSet (index, FALSE);
                    }

                /* Now release the semaphore we took earlier */

                semGive (ripLockSem);

                break; 

            case RTM_NEWADDR:

                /* Take the semaphore to lock the RIP task out */

                semTake (ripLockSem, WAIT_FOREVER); 

                /* 
                 * A new address has been added to the interface.
                 * Call routedifInit() to pick it up. If the interface
                 * is down, routedIfInit() will simply return, but when
                 * the interface comes back up, the new address will be
                 * picked up.
                 */

                if (routedDebug)
                    {
                    logMsg ("RTM_NEWADDR: added interface address\n",
                            0, 0, 0, 0, 0, 0);
                    ripSockaddrPrint (INFO_IFA (&rtInfo)); 

                    logMsg ("Calling routedIfInit for index %d.\n", 
                            index, 0, 0, 0, 0, 0);
                    }

                routedIfInit (FALSE, index); 

                /* Now release the semaphore we took earlier */

                semGive (ripLockSem);

                break; 

            case RTM_DELADDR:

                /* Take the semaphore to lock the RIP task out */

                semTake (ripLockSem, WAIT_FOREVER); 

                /* 
                 * Now delete all routes that pass though the interface
                 * corresponding to the address being deleted.
                 * Note that we create a separate interface for each
                 * alias. So even if the system has one interface with
                 * two address, we treat them as two logical interfaces.
                 * After the routes have been deleted, delete the
                 * interface structure from our list.
                 */

                if (routedDebug)
                    {
                    logMsg ("RTM_DELADDR: deleted interface address\n",
                            0, 0, 0, 0, 0, 0);
                    ripSockaddrPrint (INFO_IFA (&rtInfo)); 
                    }

                if ((pIf = ripIfWithAddrAndIndex (INFO_IFA (&rtInfo), index))
                    != NULL)
                    {
                    /*
                     * Mark the interface down so that ifRouteAdd() will not
                     * select this interface. This status change is harmless
                     * as we are about to delete the interface structure.
                     */

                    pIf->int_flags &=  ~IFF_UP;

                    ripRoutesDelete (pIf, TRUE);
                    ripInterfaceDelete (pIf);
                    }

                /* Now release the semaphore we took earlier */

                semGive (ripLockSem);

                break; 

            default:
                if (routedDebug)
                    logMsg ("route unknown message %d for ifindex %d\n", 
                            cmd, index, 0, 0, 0, 0);
                break;
            } /* End switch (cmd) */
	} /* End FOREVER */
    }

#endif /* ROUTER_STACK */

int getsocket (domain, type, sin)
	int domain, type;
	struct sockaddr_in *sin;
{
	int sock, on = 1;

	if ((sock = socket (domain, type, 0)) < 0) {
                if (routedDebug)
		    logMsg ("Error creating socket.\n", 0, 0, 0, 0, 0, 0);
		return (-1);
	}
#ifdef SO_BROADCAST
	if (setsockopt (sock, SOL_SOCKET, SO_BROADCAST, (char *)&on, 
                        sizeof (on)) < 0) {
                if (routedDebug)
		    logMsg ("error setting SO_BROADCAST option", 
                            0, 0, 0, 0, 0, 0);
		close (sock);
		return (-1);
	}
#endif
#ifdef SO_RCVBUF
	for (on = BUFSPACE; ; on -= 1024) {
		if (setsockopt (sock, SOL_SOCKET, SO_RCVBUF, 
                                (char *)&on, sizeof (on)) == 0)
			break;
		if (on <= 8*1024) {
                        if (routedDebug)
			    logMsg ("unable to set SO_RCVBUF option", 
                                     0, 0, 0, 0, 0, 0);
			break;
		}
	}
	if (routedDebug)
		logMsg ("Receive buffer size %d.\n", on, 0, 0, 0, 0, 0);
#endif
	if (bind (sock, (struct sockaddr *)sin, sizeof (*sin)) < 0) {
                if (routedDebug)
		    logMsg ("error binding socket.\n", 0, 0, 0, 0, 0, 0);
		close (sock);
		return (-1);
	}
        on = 1;
	if (ioctl (sock, FIONBIO, (int) &on) == -1)
                if (routedDebug)
		    logMsg ("error setting O_NONBLOCK option.\n", 
                            0, 0, 0, 0, 0, 0);

	return (sock);
}

/******************************************************************************
*
* ripTimerArm - arm the timeout to do routing updates
*
* This routine starts (or resets) the watchdog timer to trigger periodic
* updates at the assigned interval.
*
* RETURNS: N/A
*
* NOMANUAL        
*/

void ripTimerArm
    (
    long timeout 	/* update interval in seconds */
    )
    {
    int ticks;

    ticks = timeout * sysClkRateGet ();

#ifdef _WRS_VXWORKS_5_X
    wdStart (ripState.timerDog, ticks, semGive, (int)ripState.timerSem);
#else
    /*
     * VxWorks AE applications outside the kernel domain are not
     * permitted to access the semGive() function directly. 
     */      

    wdSemStart (ripState.timerDog, ticks, ripState.timerSem);
#endif /* _WRS_VXWORKS_5_X */
    }


/******************************************************************************
*
* ripSplitPacket - split up a rip packet for version 2
*
* INTERNAL 
*
* The <orig> parameter accesses a single route entry within the payload
* of a RIP message. In order for the rtadd() routine to store the data in
* the expected format, the sin_port and sin_zero fields of the overlayed
* structure (which correspond to the route tag, subnet mask, and next hop
* values) must be cleared after that data is extracted.
*
* NOMANUAL
*/

void ripSplitPacket
    (
    struct interface* pIfp,
    struct sockaddr_in *src, 	/* Address of router which sent update. */
    struct sockaddr* orig,
    struct sockaddr* gateway,
    struct sockaddr* netmask,
    int*	pTag
    )
    {
    BOOL noGate = FALSE;
    BOOL noMask = FALSE;
    char zero[4];
    long tmpNetmask;

    bzero ( (char *)&zero, 4);

    /* Check to see if the packet has the proper info. */

    if (bcmp ((orig->sa_data + RIP2_MASK_OFFSET), (char *)&zero, 4) == 0)
        noMask = TRUE;

    if (bcmp ((orig->sa_data + RIP2_GATE_OFFSET), (char *)&zero, 4) == 0)
        noGate = TRUE;
    
    /* 
     * Clear the provided storage for packet data. The rtadd routine used
     * later requires zero values for all unused fields for correct 
     * operation of the protocol. Later queries will fail if this condition 
     * is not met. 
     */

    bzero ( (char *)gateway, sizeof (*gateway));
    bzero ( (char *)netmask, sizeof (*netmask));
    
    /* Duplicate the shared parameters from the current route entry. */

    gateway->sa_family = orig->sa_family;
    gateway->sa_len = orig->sa_len;
    
    netmask->sa_family = orig->sa_family;
    netmask->sa_len = orig->sa_len;

    /* Extract the data from the original packet. */

    if (noGate)
        {
        /* 
         * RFC 1723, Section 3.4: If a value of 0.0.0.0 is given in the next
         * hop field, use the originator of the RIP advertisement as the 
         * gateway.
         */

        bcopy ( (char *)&(src->sin_addr.s_addr), 
               (char *)gateway->sa_data + 2, 4);
        }
    else

        bcopy ( (char *)(orig->sa_data + RIP2_GATE_OFFSET), 
               (char *)gateway->sa_data + 2, 4);

    if (noMask)
        {
        bcopy ( (char *)&pIfp->int_subnetmask, 
               (char *)&tmpNetmask, 4);
        tmpNetmask = htonl (tmpNetmask);
        bcopy ( (char *)&tmpNetmask,
               (char *)netmask->sa_data + 2, 4);
        }
    else
        bcopy ( (char *)(orig->sa_data + RIP2_MASK_OFFSET), 
               (char *)netmask->sa_data + 2, 4);

    /* 
     * Clear the fields in the original data now that the values have been
     * copied. This step converts that structure into a format suitable
     * for use with the rtadd() function.
     */

    /* Erase the route tag (sin_port field). */

    *pTag = (int)(*((unsigned short *)(orig->sa_data)));
    bzero ( (char *)orig->sa_data, 2); 	

    /* Erase the adjacent subnet mask and next hop values (sin_zero field). */

    bzero ( (char *)(orig->sa_data + RIP2_MASK_OFFSET), 8);

    return;
    }

/******************************************************************************
*
* ripRouteToAddrs - convert a route entry into separate sockaddr_in structures
*
* This routine takes a pointer to an rt_entry and parses it out into
* three sockaddr_in structures.  pDsin contains the destination, pGsin contains
* the gateway and pNsin contains the subnetmask.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void ripRouteToAddrs
    (
    struct rt_entry* pRoute,    /* Route to convert. */
    struct sockaddr_in** ppDsin,  /* Destination sin. */
    struct sockaddr_in** ppGsin,  /* Gateway sin. */
    struct sockaddr_in** ppNsin  /* Netmask sin. */
    )
    {

    if (ppDsin != NULL)
        *ppDsin = (struct sockaddr_in *) &(pRoute->rt_dst);
    if (ppGsin != NULL)
        *ppGsin = (struct sockaddr_in *) &(pRoute->rt_router);
    if (ppNsin != NULL)
        *ppNsin = (struct sockaddr_in *) &(pRoute->rt_netmask);

    }

/******************************************************************************
*
* ripBuildPacket - this routine builds a packet from the route passed
*
* This routine takes the routing entry that we are passed and properly
* fills the fields in the packet. If RIP version 1 packets are requested,
* then the sections that must be zero are left alone, otherwise they are 
* properly filled. The version parameter uses the values for the MIB variable
* stored in ripState.ripConf.rip2IfConfSend instead of the actual version
* number. This routine is never called if that value is set to disable
* transmission.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void ripBuildPacket
    (
    RIP2PKT *pPacket,
    struct rt_entry* rt,
    struct interface * 	pIf,
    int version
    )
    {
    UINT32 	routenet = 0; 	/* Network number of route gateway */
    UINT32 	destnet = 0; 	/* Network number of route destination */
    struct sockaddr_in* dSin;
    struct sockaddr_in* gSin;
    struct sockaddr_in* nSin;

    if (version == M2_rip2IfConfSend_ripVersion1)
        {
        pPacket->tag = 0;
        ripRouteToAddrs (rt, &dSin, &gSin, &nSin);
        pPacket->dest = dSin->sin_addr.s_addr;
        pPacket->subnet = 0;
        pPacket->gateway = 0;
        pPacket->metric = htonl (rt->rt_metric);
        }
    else
        {
        /* 
         * Building version 2 packets requires special processing to 
         * support both RIP-1 and RIP-2 routers in all situations.
         */

        pPacket->tag = rt->rt_tag;
        ripRouteToAddrs (rt, &dSin, &gSin, &nSin);

        if (nSin->sin_addr.s_addr)
            {
            /* 
             * Don't apply the route's netmask if it is shorter than the 
             * mask of the outgoing interface. The interface's mask is used
             * for those destinations so that an unreachable gateway is not 
             * sent. The shorter mask could conceal the network mismatch,
             * (e.g. - 147.11.151.47/23 and 147.11.150.48/24 would match
             * in that case).
             */

            if ( (nSin->sin_addr.s_addr & htonl (pIf->int_subnetmask)) ==
                    pIf->int_subnetmask)
                routenet = gSin->sin_addr.s_addr & nSin->sin_addr.s_addr;
            else
                routenet = gSin->sin_addr.s_addr & pIf->int_subnetmask;
            }
        else
            routenet = gSin->sin_addr.s_addr & pIf->int_subnetmask;

        destnet = (*(UINT32 *)(&pIf->int_addr.sa_data[0] + 2) &
                                      pIf->int_subnetmask);

        /* No special processing is required for the advertised destination. */

        pPacket->dest = dSin->sin_addr.s_addr;

        /*
         * Internally generated entries that substitute for supernets
         * still contain the classless netmask so that the border gateway 
         * filtering can select the correct entry for both directly connected 
         * and "distant" destinations. That value must be replaced with
         * the "natural" (class-based) network mask before the update is
         * sent so that the route will be understood by RIP-2 routers. If 
         * border gateway filtering was disabled, none of the internally 
         * generated entries will be included in any update, so this
         * substitution is (correctly) ignored.
         */

        if ( (rt->rt_state & (RTS_INTERNAL | RTS_SUBNET)) == 
                (RTS_INTERNAL | RTS_SUBNET))
            {
            /*
             * For internal routes, the route entry's netmask is equal to 
             * rt->rt_ifp->int_netmask unless it is a substitute for a
             * supernet.
             */

            if (ntohl (nSin->sin_addr.s_addr) != rt->rt_ifp->int_netmask)
                {
                /* 
                 * Substitute the class-based value for the 
                 * supernet mask in the outgoing message.
                 */

                pPacket->subnet = htonl (rt->rt_ifp->int_netmask);
                }
            else
                {
                /* 
                 * For subnet replacements, use the existing 
                 * (class-based) value.
                 */

                pPacket->subnet = nSin->sin_addr.s_addr;
                }
            }
        else
            {
            /* 
             * Common case: use mask from actual route entry. This
             * case includes the internally generated default route
             * created when the gateway flag is set during initialization.
             */

            pPacket->subnet = nSin->sin_addr.s_addr;
            }

        /* 
         * For RIPv2 packets, the network number of the published gateway 
         * must match the network number of the outgoing interface. Unless
         * the host contains multiple interfaces with the same network
         * number, the value stored in the RIP routing table generally
         * contains an address on a different network. In that case,
         * use the interface IP address to include a reachable gateway in 
         * the route update.
         */

        if (routenet != destnet)
            pPacket->gateway = *(UINT32 *)(&pIf->int_addr.sa_data[0] + 2);
        else
            pPacket->gateway = gSin->sin_addr.s_addr;

        /* 
         * Unlike the gateway and netmask, the specified metric
         * can always be copied directly. 
         */

        pPacket->metric = htonl (rt->rt_metric);
        }
    }

/******************************************************************************
*
* ripRouteShow - display the internal routing table maintained by RIP
*
* This routine prints every entry in the local RIP routing table. The
* flags displayed below the destination, gateway, and netmask addresses
* indicate the current route status. Entries with the RTS_INTERFACE flag
* indicate locally generated routes to directly connected networks. 
* If RTS_SUBNET is set for an entry, it is subject to border
* gateway filtering (if enabled). When RTS_INTERNAL is also present, the 
* corresponding entry is an "artificial" route created to supply distant
* networks with legitimate destinations if border filtering excludes the
* actual entry. Those entries are not copied to the kernel routing table.
* The RTS_CHANGED flag marks entries added or modified in the last timer 
* interval that will be included in a triggered update. The RTS_OTHER 
* flag is set for routes learnt from other sources. The RTS_PRIMARY
* flag (set only if the RTS_OTHER flag is also set) indicates that the route
* is a primary route, visible to the IP forwarding process. The DOWN flag
* indicates that the interface through which the gateway is reachable is
* down.
*/

void ripRouteShow ()
    {
    int doinghost = 1;
    register struct rthash *rh;
    register struct rt_entry *rt;
    struct rthash *base = hosthash;
    struct sockaddr_in* dSin;
    struct sockaddr_in* gSin;
    struct sockaddr_in* nSin;
    char address[32];

    if (!ripInitFlag)
        return;

    /* Block all processing to guarantee a stable list of routes. */

    semTake (ripLockSem, WAIT_FOREVER);
 
    printf ("Destination      Gateway          Netmask          Metric Proto Timer "
            "Tag \n");
    again:
    for (rh = base; rh < &base[ROUTEHASHSIZ]; rh++)
        {
        rt = rh->rt_forw;
        for (; rt != (struct rt_entry *)rh; rt = rt->rt_forw)
            {
            ripRouteToAddrs (rt, &dSin, &gSin, &nSin);
            inet_ntoa_b (dSin->sin_addr, (char *)&address);
            printf ("%-17s", address);
            inet_ntoa_b (gSin->sin_addr, (char *)&address);
            printf ("%-17s", address);
            inet_ntoa_b (nSin->sin_addr, (char *)&address);
            printf ("%-17s", address);
            printf (" %-6d", rt->rt_metric);
            printf (" %-5d", rt->rt_proto);
            printf (" %-5d", rt->rt_timer);
            printf ("%-6x\n", (unsigned short)ntohs((unsigned short)(rt->rt_tag)));

            /* Now figure out all the state. */

            if (rt->rt_state & RTS_CHANGED)
                printf ("RTS_CHANGED ");
            if (rt->rt_state & RTS_EXTERNAL)
                printf ("RTS_EXTERNAL ");
            if (rt->rt_state & RTS_INTERNAL)
                printf ("RTS_INTERNAL ");
            if (rt->rt_state & RTS_PASSIVE)
                printf ("RTS_PASSIVE ");
            if (rt->rt_state & RTS_INTERFACE)
                printf ("RTS_INTERFACE ");
            if (rt->rt_state & RTS_REMOTE)
                printf ("RTS_REMOTE ");
            if (rt->rt_state & RTS_SUBNET)
                printf ("RTS_SUBNET ");
            if (rt->rt_state & RTS_OTHER)
                {
                printf ("RTS_OTHER ");
                if (rt->rt_state & RTS_PRIMARY)
                    printf ("RTS_PRIMARY ");
                }
            if (rt->rt_ifp && (rt->rt_ifp->int_flags & IFF_UP) == 0)
                printf ("DOWN ");
            printf ("\n");
            }
        }
    if (doinghost)
        {
        doinghost = 0;
        base = nethash;
        goto again;
        }

    semGive (ripLockSem);

    return;
    }

/******************************************************************************
*
* ripDbgRouteShow - display debug info about RIP's internal route table
*
* This routine prints every entry in the local RIP routing table. The
* flags displayed below the destination, gateway, and netmask addresses
* indicate the current route status. Entries with the RTS_INTERFACE flag
* indicate locally generated routes to directly connected networks. 
* If RTS_SUBNET is set for an entry, it is subject to border
* gateway filtering (if enabled). When RTS_INTERNAL is also present, the 
* corresponding entry is an "artificial" route created to supply distant
* networks with legitimate destinations if border filtering excludes the
* actual entry. Those entries are not copied to the kernel routing table.
* The RTS_CHANGED flag marks entries added or modified in the last timer 
* interval that will be included in a triggered update. The RTS_OTHER 
* flag is set for routes learnt from other sources. The RTS_PRIMARY
* flag (set only if the RTS_OTHER flag is also set) indicates that the route
* is a primary route, visible to the IP forwarding process. The DOWN flag
* indicates that the interface through which the gateway is reachable is
* down.
*
* NOMANUAL
*/

void ripDbgRouteShow ()
    {
    int doinghost = 1;
    register struct rthash *rh;
    register struct rt_entry *rt;
    struct rthash *base = hosthash;
    struct sockaddr_in* dSin;
    struct sockaddr_in* gSin;
    struct sockaddr_in* nSin;
    char address[32];

    if (!ripInitFlag)
        return;

    /* Block all processing to guarantee a stable list of routes. */

    printf ("Destination      Gateway          Netmask          Metric Proto Timer Tag "
            "  ref  sref  from              Interface\n");
    again:
    for (rh = base; rh < &base[ROUTEHASHSIZ]; rh++)
        {
        rt = rh->rt_forw;
        for (; rt != (struct rt_entry *)rh; rt = rt->rt_forw)
            {
            ripRouteToAddrs (rt, &dSin, &gSin, &nSin);
            inet_ntoa_b (dSin->sin_addr, (char *)&address);
            printf ("%-17s", address);
            inet_ntoa_b (gSin->sin_addr, (char *)&address);
            printf ("%-17s", address);
            inet_ntoa_b (nSin->sin_addr, (char *)&address);
            printf ("%-17s", address);
            printf (" %-6d", rt->rt_metric);
            printf (" %-5d", rt->rt_proto);
            printf (" %-5d", rt->rt_timer);
            printf ("%-6x", (unsigned short)ntohs((unsigned short)(rt->rt_tag)));
            printf (" %-4d", rt->rt_refcnt);
            printf (" %-5d ", rt->rt_subnets_cnt);
            inet_ntoa_b (*(struct in_addr *)&(rt->rt_orgrouter), 
                         (char *)&address); 
            printf ("%-17s", address);

            printf (" %s\n", rt->rt_ifp->int_name);

            /* Now figure out all the state. */

            if (rt->rt_state & RTS_CHANGED)
                printf ("RTS_CHANGED ");
            if (rt->rt_state & RTS_EXTERNAL)
                printf ("RTS_EXTERNAL ");
            if (rt->rt_state & RTS_INTERNAL)
                printf ("RTS_INTERNAL ");
            if (rt->rt_state & RTS_PASSIVE)
                printf ("RTS_PASSIVE ");
            if (rt->rt_state & RTS_INTERFACE)
                printf ("RTS_INTERFACE ");
            if (rt->rt_state & RTS_REMOTE)
                printf ("RTS_REMOTE ");
            if (rt->rt_state & RTS_SUBNET)
                printf ("RTS_SUBNET ");
            if (rt->rt_state & RTS_OTHER)
                {
                printf ("RTS_OTHER ");
                if (rt->rt_state & RTS_PRIMARY)
                    printf ("RTS_PRIMARY ");
                }
            if (rt->rt_ifp && (rt->rt_ifp->int_flags & IFF_UP) == 0)
                printf ("DOWN ");
            printf ("\n");
            }
        }
    if (doinghost)
        {
        printf ("-----------------------------------------------------------------\n");
        doinghost = 0;
        base = nethash;
        goto again;
        }

    return;
    }

/*****************************************************************************
*
* ripSetInterfaces - add all multicast interfaces to address group
*
* This routine sets all interfaces that are multicast capable into the
* multicast group address passed in as an argument.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void ripSetInterfaces
    (
    INT32 sock,
    UINT32 mcastAddr                          /* Address to join. */
    )
    {
    struct ip_mreq	ipMreq; /* Multicast structure for version 2 */
    struct interface * 	pIf;
    UINT32 		addr;

    for (pIf = ripIfNet; pIf; pIf = pIf->int_next)
        {
        addr = ((struct sockaddr_in *)&(pIf->int_addr))->sin_addr.s_addr; 

        ipMreq.imr_multiaddr.s_addr = htonl (mcastAddr);
        ipMreq.imr_interface.s_addr = addr;

        if (setsockopt (sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                        (char *)&ipMreq, sizeof (ipMreq)) < 0)
            { 
            if (routedDebug)
                logMsg ("setsockopt IP_ADD_MEMBERSHIP error:\n",
                        0, 0, 0, 0, 0, 0); 
            }
        }
    }

/*****************************************************************************
*
* ripClearInterfaces - remove all multicast interfaces from address group
*
* This routine removes all interfaces that are multicast capable from the
* multicast group address given by <mcastAddr>. It is called when changing
* the receive control switch from RIP-2 only mode with SNMP. Although it 
* seems acceptable to allow membership in the multicast group unless the 
* receive switch is set to RIP-1 only mode, ANVL test 16.1 would fail.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void ripClearInterfaces
    (
    INT32 sock,
    UINT32 mcastAddr                          /* Address to join. */
    )
    {
    struct ip_mreq	ipMreq; /* Multicast structure for version 2 */
    struct interface * 	pIf;
    UINT32 		addr;

    for (pIf = ripIfNet; pIf; pIf = pIf->int_next)
        {
        addr = ((struct sockaddr_in *)&(pIf->int_addr))->sin_addr.s_addr; 
        ipMreq.imr_multiaddr.s_addr = htonl (mcastAddr);
        ipMreq.imr_interface.s_addr = addr;

        if (setsockopt (sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                        (char *)&ipMreq, sizeof (ipMreq)) < 0)
            { 
            if (routedDebug)
                logMsg ("setsockopt IP_DROP_MEMBERSHIP error.\n",
                        0, 0, 0, 0, 0, 0); 
            }
        }
    }
/******************************************************************************
*
* _ripIfShow - display information about an interface 
*
* This routine prints information about an interface entry.
* The interface name, interface index, the UP/DOWN status and the 
* interface address and netmask are displayed.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void _ripIfShow 
    (
    struct interface * 	pIf
    )
    {
    char 		address[32];
    struct in_addr 	sin_addr;

    printf ("%-8s%-8d%-12s",
            pIf->int_name, pIf->int_index,
            (pIf->int_flags & IFF_UP) == IFF_UP ? "UP" : "DOWN");

    inet_ntoa_b (((struct sockaddr_in *)&(pIf->int_addr))->sin_addr, 
                 address);
    printf ("%-17s", address);

    sin_addr.s_addr = htonl (pIf->int_subnetmask);
    inet_ntoa_b (sin_addr, address);
    printf ("%-17s", address);

    inet_ntoa_b (((struct sockaddr_in *)&(pIf->int_broadaddr))->sin_addr, 
                 address);
    printf ("%-17s\n", address);

    return;
}


/******************************************************************************
*
* ripIfShow - display the internal interface table maintained by RIP
*
* This routine prints every entry in the local RIP interface table. 
* The interface name, interface index, the UP/DOWN status and the 
* interface address and netmask are displayed.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void ripIfShow (void)
    {
    struct interface * 	pIf;

    if (!ripInitFlag)
        return;

    /* Block all processing to guarantee a stable list of interfaces. */

    semTake (ripLockSem, WAIT_FOREVER);
 
    /*
     * Loop through the interface list printing out the 
     * interface name    index   UP/DOWN status	   I/f addr    I/f netmask
     */

    printf ("Name    Index   Up/Down     Address          Netmask          "
            "Broadcast\n");	
    for (pIf = ripIfNet; pIf; pIf = pIf->int_next)
        _ripIfShow (pIf);
                
    semGive (ripLockSem);

    return;
    }

/******************************************************************************
*
* ripAuthHookAdd - add an authentication hook to a RIP interface
*
* This routine installs a hook routine to validate incoming RIP messages
* for a registered interface given by <pIpAddr>. (Interfaces created or
* changed after a RIP session has started may be installed/updated with the
* ripIfSearch() and ripIfReset() routines). The hook is only called if
* an SNMP agent enables authentication for the corresponding interface.
* It uses the following prototype:
*
* \cs
*     STATUS ripAuthHookRtn (char *pKey, RIP_PKT *pRip);
* \ce
* The first argument contains the authentication key for the message
* stored in the rip2IfConfAuthKey MIB variable and the second argument 
* uses the RIP_PKT structure (defined in rip/ripLib.h) to access the message 
* body. The routine must return OK if the message is acceptable, or ERROR 
* otherwise. All RIP-2 messages sent to that routine already contain an 
* authentication entry, but have not been verified. (Any unauthenticated
* RIP-2 messages have already been discarded as required by the RFC 
* specification). RIP-1 messages may be accepted or rejected. RIP-2 messages
* requesting simple password authentication that match the key are
* accepted automatically before the hook is called. The remaining RIP-2
* messages either did not match that key or are using an unknown 
* authentication type. If any messages are rejected, the MIB-II counters are 
* updated appropriately outside of the hook routine.
*
* The current RIP implementation contains a sample authentication hook that
* you may add as follows:
*
* \cs
*     if (ripAuthHookAdd ("90.0.0.1", ripAuthHook) == ERROR)
*         logMsg ("Unable to add authorization hook.\n", 0, 0, 0, 0, 0, 0);
* \ce
*
* The sample routine supports only simple password authentication against
* the key included in the MIB variable. Since all such messages have already
* been accepted, all RIP-2 messages received by the routine are discarded.
* All RIP-1 messages are also discarded, so the hook actually has no
* effect. The body of that routine is:
*
* \cs
* STATUS ripAuthHook
*    (
*    char * 	pKey, 	/@ rip2IfConfAuthKey entry from MIB-II family @/
*    RIP_PKT * 	pRip 	/@ received RIP message @/
*    )
*    {
*    if (pRip->rip_vers == 1)
*        {
*        /@ 
*         @ The RFC specification recommends, but does not require, rejecting
*         @ version 1 packets when authentication is enabled.
*         @/
*
*        return (ERROR);
*        }
*
*    /@
*     @ The authentication type field in the RIP message corresponds to
*     @ the first two bytes of the sa_data field overlayed on that
*     @ message by the sockaddr structure contained within the RIP_PKT 
*     @ structure (see rip/ripLib.h).
*     @/
*
*    if ( (pRip->rip_nets[0].rip_dst.sa_data[0] != 0) ||
*        (pRip->rip_nets[0].rip_dst.sa_data[1] !=
*        M2_rip2IfConfAuthType_simplePassword))
*        {
*        /@ Unrecognized authentication type. @/
*
*        return (ERROR);
*        }
*
*    /@ 
*     @ Discard version 2 packets requesting simple password authentication
*     @ which did not match the MIB variable. 
*     @/
*
*    return (ERROR);
*    }
* \ce
*
* A comparison against a different key could be performed as follows:
*
* \cs
*  bzero ( (char *)&key, AUTHKEYLEN);    /@ AUTHKEYLEN from rip/m2RipLib.h @/
*
*   /@
*    @ The start of the authorization key corresponds to the third byte
*    @ of the sa_data field in the sockaddr structure overlayed on the
*    @ body of the RIP message by the RIP_PKT structure. It continues
*    @ for the final 14 bytes of that structure and the first two bytes
*    @ of the following rip_metric field.
*    @/
*
*  bcopy ( (char *)(pRip->rip_nets[0].rip_dst.sa_data + 2),
*         (char *)&key, AUTHKEYLEN);
*
*  if (bcmp ( (char *)key, privateKey, AUTHKEYLEN) != 0)
*      {
*      /@ Key does not match: reject message. @/
*
*      return (ERROR);
*      }
*  return (OK);
* \ce
*
* The ripAuthHookDelete() routine will remove the installed function. If
* authentication is still enabled for the interface, all incoming messages
* that do not use simple password authentication will be rejected until a
* routine is provided.
*
* RETURNS: OK, if hook added; or ERROR otherwise.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*/

STATUS ripAuthHookAdd
    (
    char* pIpAddr,	/* IP address in dotted decimal notation */
    FUNCPTR pAuthHook	/* routine to handle message authentication */
    )
    {
    struct interface* pIfp;
    struct sockaddr_in address;

    if (!ripInitFlag)
        return (ERROR);

    if (pIpAddr == NULL)
        {
        errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
        }

    address.sin_len = sizeof (address);
    address.sin_addr.s_addr = inet_addr (pIpAddr);
    address.sin_family = AF_INET;
    
    pIfp = ripIfLookup ((struct sockaddr *)&address);

    if (pIfp == NULL)
        {
        errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        return (ERROR);
        }

    /* Short critical section with input processing. */

    semTake (ripLockSem, WAIT_FOREVER);    
    pIfp->authHook = pAuthHook;
    semGive (ripLockSem);
 
    return (OK);
    }

/******************************************************************************
*
* ripAuthHookDelete - remove an authentication hook from a RIP interface
*
* This routine removes an assigned authentication hook from a registered
* interface indicated by <pIpAddr>. (Interfaces created or changed after 
* a RIP session has started may be installed/updated with the ripIfSearch() 
* and ripIfReset() routines). If authentication is still enabled for the 
* interface, RIP-2 messages using simple password authentication will be
* accepted if they match the key in the MIB variable, but all other incoming 
* messages will be rejected until a routine is provided.
*
* RETURNS: OK; or ERROR, if the interface could not be found.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*/

STATUS ripAuthHookDelete
    (
    char* pIpAddr	/* IP address in dotted decimal notation */
    )
    {
    struct interface* pIfp;
    struct sockaddr_in address;

    if (!ripInitFlag)
        return (ERROR);

    if (pIpAddr == NULL)
        {
        errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
        }
    address.sin_len = sizeof (address);
    address.sin_addr.s_addr = inet_addr (pIpAddr);
    address.sin_family = AF_INET;
    
    pIfp = ripIfLookup ( (struct sockaddr *)&address);

    if (pIfp == NULL)
        {
        errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        return (ERROR);
        }

    /* Short critical section with input processing. */
 
    semTake (ripLockSem, WAIT_FOREVER);    
    pIfp->authHook = NULL;
    semGive (ripLockSem);
 
    return (OK);
    }

/******************************************************************************
*
* ripAuthCheck - verify RIP messages if authentication is enabled
*
* This routine accepts all RIP-2 messages using simple password
* authentication that match the key in the MIB variable. It is
* called automatically when authentication is enabled. All other
* RIP-2 messages and any RIP-1 messages may be accepted by an
* authentication hook, or will be discarded if no hook is present.
*
* RETURNS: OK, if message is acceptable; or ERROR otherwise.
*
* NOMANUAL
*/

STATUS ripAuthCheck
    (
    char * 	pKey, 	/* rip2IfConfAuthKey entry from MIB-II family */
    RIP_PKT * 	pRip 	/* received RIP message */
    )
    {
    if (pRip->rip_vers == 1)
        {
        /* 
         * The RFC specification recommends, but does not require, rejecting
         * version 1 packets when authentication is enabled. Those packets
         * will be discarded unless accepted by the hook.
         */

        return (ERROR);
        }

    /*
     * The authentication type field in the RIP message corresponds to
     * the first two bytes of the sa_data field overlayed on that
     * message by the sockaddr structure contained within the RIP_PKT 
     * structure (see rip/ripLib.h).
     */

    if ((pRip->rip_nets[0].rip_dst.sa_data[0] != 0) ||
        (pRip->rip_nets[0].rip_dst.sa_data[1] !=
         M2_rip2IfConfAuthType_simplePassword))
        {
        /* 
         * Reject messages with an unrecognized authentication type. This
         * behavior can be overridden by the user's authentication hook.
         */

        return (ERROR);
        }

    /*
     * The start of the authorization key corresponds to the third byte
     * of the sa_data field in the sockaddr structure and continues for
     * AUTHKEYLEN bytes (defined in rip/m2RipLib.h).
     */

    if (bcmp ( (char *)(pRip->rip_nets[0].rip_dst.sa_data + 2), 
              pKey, AUTHKEYLEN) != 0)
        {
        return (ERROR);
        }

    /*
     * Accept version 2 packets requesting simple password 
     * authentication that matched the MIB variable.
     */

    return (OK);
    }

/******************************************************************************
*
* ripAuthHook - sample authentication hook
*
* This hook demonstrates one possible authentication mechanism. It rejects
* all RIP-2 messages that used simple password authentication since they
* did not match the key contained in the MIB variable. All other RIP-2
* messages are also rejected since no other authentication type is
* supported and all RIP-1 messages are also rejected, as recommended by
* the RFC specification. This behavior is the same as if no hook were 
* installed.
*
* RETURNS: OK, if message is acceptable; or ERROR otherwise.
*/

STATUS ripAuthHook
    (
    char *      pKey,   /* rip2IfConfAuthKey entry from MIB-II family */
    RIP_PKT *   pRip    /* received RIP message */
    )
    {
    if (pRip->rip_vers == 1)
        {
        /*
         * The RFC specification recommends, but does not require, rejecting
         * version 1 packets when authentication is enabled.
         */

        return (ERROR);
        }

    /*
     * The authentication type field in the RIP message corresponds to
     * the first two bytes of the sa_data field overlayed on that
     * message by the sockaddr structure contained within the RIP_PKT
     * structure (see rip/ripLib.h).
     */

    if ((pRip->rip_nets[0].rip_dst.sa_data[0] != 0) ||
        (pRip->rip_nets[0].rip_dst.sa_data[1] !=
         M2_rip2IfConfAuthType_simplePassword))
        {
        /* Unrecognized authentication type. */

        return (ERROR);
        }

    /*
     * Discard version 2 packets requesting simple password
     * authentication that did not match the MIB variable.
     */

    return (ERROR);
    }

/******************************************************************************
*
* ripLeakHookAdd - add a hook to bypass the RIP and kernel routing tables
*
* This routine installs a hook routine to support alternative routing
* protocols for the registered interface given by <pIpAddr>. (Interfaces 
* created or changed after a RIP session has started may be installed/updated
* with the ripIfSearch() and ripIfReset() routines). 
*
* The hook uses the following interface:
* \cs
*     STATUS ripLeakHookRtn (long dest, long gateway, long netmask)
* \ce
*
* The RIP session will not add the given route to any tables if the hook
* routine returns OK, but will create a route entry otherwise.
*
* The ripLeakHookDelete() will allow the RIP session to add new routes
* unconditionally.
*
* RETURNS: OK; or ERROR, if the interface could not be found.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*/

STATUS ripLeakHookAdd
    (
    char * 	pIpAddr,	/* IP address in dotted decimal notation */
    FUNCPTR 	pLeakHook	/* function pointer to hook */
    )
    {
    struct interface* pIfp;
    struct sockaddr_in address;

    if (!ripInitFlag)
        return (ERROR);

    if (pIpAddr == NULL)
        {
        errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
        }
    address.sin_len = sizeof (address);
    address.sin_addr.s_addr = inet_addr (pIpAddr);
    address.sin_family = AF_INET;
    
    pIfp = ripIfLookup ((struct sockaddr *)&address);

    if (pIfp == NULL)
        {
        errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        return (ERROR);
        }

    /* Short critical section with message and timer processing. */
 
    semTake (ripLockSem, WAIT_FOREVER);    
    pIfp->leakHook = pLeakHook;
    semGive (ripLockSem);

    return (OK);
    }

/******************************************************************************
*
* ripLeakHookDelete - remove a table bypass hook from a RIP interface
*
* This routine removes the assigned bypass hook from a registered interface
* indicated by <pIpAddr>. (Interfaces created or changed after a RIP
* session has started may be installed/updated with the ripIfSearch() 
* and ripIfReset() routines). The RIP session will return to the default 
* behavior and add entries to the internal RIP table and kernel routing 
* table unconditionally.
*
* RETURNS: OK; or ERROR, if the interface could not be found.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*/

STATUS ripLeakHookDelete
    (
    char* pIpAddr	/* IP address in dotted decimal notation */
    )
    {
    struct interface* pIfp;
    struct sockaddr_in address;

    if (!ripInitFlag)
        return (ERROR);

    if (pIpAddr == NULL)
        {
        errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
        }
    address.sin_len = sizeof (address);
    address.sin_addr.s_addr = inet_addr (pIpAddr);
    address.sin_family = AF_INET;
    
    pIfp = ripIfLookup ((struct sockaddr *)&address);

    if (pIfp == NULL)
        {
        errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        return (ERROR);
        }
   
    /* Short critical section with message and timer processing. */
 
    semTake (ripLockSem, WAIT_FOREVER);    
    pIfp->leakHook = NULL;
    semGive (ripLockSem);
 
    return (OK);
    }

/******************************************************************************
*
* ripLeakHook - sample leak hook for RIP routes
*
* This routine prevents any routes from being added to the internal RIP
* table or kernel routing table, and prints the information that was passed 
* to it.
*
* RETURNS: OK
*
* NOMANUAL
*/

STATUS ripLeakHook
    (
    long dest,
    long gate,
    long mask
    )
    {
    printf ("Destination %lx\tGateway %lx\tMask %lx\n", dest, gate, mask);

    return (OK);
    }

/******************************************************************************
*
* ripSendHookAdd - add an update filter to a RIP interface
*
* This routine installs a hook routine to screen individual route entries
* for inclusion in a periodic update. The routine is installed for the
* registered interface given by <pIpAddr>. (Interfaces created or
* changed after a RIP session has started may be installed/updated with 
* the ripIfSearch() and ripIfReset() routines).
*
* The hook uses the following prototype:
* \cs
*     BOOL ripSendHookRtn (struct rt_entry* pRt);
* \ce
*
* If the hook returns FALSE, the route is not included in the update.
* Otherwise, it is included if it meets the other restrictions, such
* as simple split horizon and border gateway filtering. The 
* ripSendHookDelete() routine removes this additional filter from the
* output processing.
*
* RETURNS: OK; or ERROR, if the interface could not be found.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*/

STATUS ripSendHookAdd
    (
    char* pIpAddr,	/* IP address in dotted decimal notation */
    BOOL (*ripSendHook) (struct rt_entry* pRt) /* Routine to use. */
    )
    {
    struct interface* pIfp;
    struct sockaddr_in address;

    if (!ripInitFlag)
        return (ERROR);

    if (pIpAddr == NULL)
        {
        errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
        }
    address.sin_len = sizeof (address);
    address.sin_addr.s_addr = inet_addr (pIpAddr);
    address.sin_family = AF_INET;
    
    pIfp = ripIfLookup ((struct sockaddr *)&address);

    if (pIfp == NULL)
        {
        errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        return (ERROR);
        }

    /* Short critical section with output processing. */
 
    semTake (ripLockSem, WAIT_FOREVER);    
    pIfp->sendHook = ripSendHook;
    semGive (ripLockSem);
 
    return (OK);
    }

/******************************************************************************
*
* ripSendHookDelete - remove an update filter from a RIP interface
*
* This routine removes the hook routine that allowed additional screening
* of route entries in periodic updates from the registered interface
* indicated by <pIpAddr>. (Interfaces created or changed after a RIP 
* session has started may be installed/updated with the ripIfSearch() 
* and ripIfReset() routines). The RIP session will return to the default 
* behavior and include any entries that meet the other restrictions (such 
* as simple split horizon).
*
* RETURNS: OK; or ERROR, if the interface could not be found.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*/

STATUS ripSendHookDelete
    (
    char* pIpAddr	/* IP address in dotted decimal notation */
    )
    {
    struct interface* pIfp;
    struct sockaddr_in address;

    if (!ripInitFlag)
        return (ERROR);

    if (pIpAddr == NULL)
        {
        errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
        }
    address.sin_len = sizeof (address);
    address.sin_addr.s_addr = inet_addr (pIpAddr);
    address.sin_family = AF_INET;
    
    pIfp = ripIfLookup ((struct sockaddr *)&address);

    if (pIfp == NULL)
        {
        errnoSet (S_m2Lib_ENTRY_NOT_FOUND);
        return (ERROR);
        }

    /* Short critical section with output processing. */

    semTake (ripLockSem, WAIT_FOREVER);
    pIfp->sendHook = NULL;
    semGive (ripLockSem);
 
    return (OK);
    }

/******************************************************************************
*
* ripSendHook - sample send hook for rip routes
*
* This routine displays all the routes that are being sent and returns OK
* so that they are included in the update if all other criteria are met.
*
* RETURNS: OK
*
* NOMANUAL
*/


BOOL ripSendHook
    (
    struct rt_entry* pRt
    )
    {
    char address[32];
    struct sockaddr_in* dSin;
    struct sockaddr_in* gSin;
    struct sockaddr_in* nSin;

    ripRouteToAddrs (pRt, &dSin, &gSin, &nSin);
    inet_ntoa_b (dSin->sin_addr, (char *)&address);
    printf ("Route to %s ", address);
    inet_ntoa_b (gSin->sin_addr, (char *)&address);
    printf ("with gateway %s ", address);
    inet_ntoa_b (nSin->sin_addr, (char *)&address);
    printf ("and netmask %s being sent.\n", address);
    return (OK);
    }

/******************************************************************************
*
* ripRouteHookAdd - add a hook to install static and non-RIP routes into RIP
*
* This routine installs a hook routine that you can use to give RIP the 
* ability to respond to route-add events generated by non-RIP agents. By   
* design, RIP is not interested in the routes generated by non-RIP agents. 
* If you do not install a route hook function, RIP continues this default 
* behavior.  However, if you want RIP to add these non-RIP routes to its 
* internal routing database and even propagate routes added by other 
* agents, you must use ripRouteHookAdd() to register a function of the form:
* 
* \ss
* STATUS <Your>RipRouteHookRtn 
*     ( 
*     struct ROUTE_INFO * pRouteInfo, 
*     int  protoId, 
*     BOOL primaryRoute,
*     int  flags 
*     ) 
* \se
* RIP invokes this function in response to the following events:
* 
* 	'1.'  A non-RIP non-system route was added to the routing table. 
* 	'2.'  A route change message arrived. 
* 	'3.'  An ICMP redirect message arrived. 
* 
* The returned function value of the route hook routine tells rip 
* how to respond to the event. In the first case, the returned 
* function value tells RIP whether to add or ignore the new route. 
* In the second case, the returned function tells RIP whether to 
* delete the specified route or change its metric. In the third 
* case, the event is of no direct importance for RIP, so RIP 
* ignores the returned value of the route hook function. 
* \is
* \i <pRouteInfo>
* This parameter passes in a pointer to a route information 
* structure that stores the routing message. You should not 
* access the contents of this structure directly. Instead, 
* use ripAddrsXtract() to extract the following information: 
*
* 	'-' destination address
* 	'-' netmask
* 	'-' gateway address
* 	'-' old gateway address (if available)
* 
* \i <protoId> 
* This parameter passes in the ID of the protocol that generated 
* the event. Valid protocol IDs are defined in m2Lib.h as follows:
* 
* 	'M2_ipRouteProto_other' (static routes) 
* 	'M2_ipRouteProto_local' 
* 	'M2_ipRouteProto_netmgmt'
* 	'M2_ipRouteProto_icmp'
* 	'M2_ipRouteProto_egp'
* 	'M2_ipRouteProto_ggp'
* 	'M2_ipRouteProto_hello'
* 	'M2_ipRouteProto_rip'
* 	'M2_ipRouteProto_is_is'
* 	'M2_ipRouteProto_es_is'
* 	'M2_ipRouteProto_ciscoIgrp'
* 	'M2_ipRouteProto_bbnSpfIgp'
* 	'M2_ipRouteProto_ospf'
* 	'M2_ipRouteProto_bgp'
* 
* \i <primaryRoute> 
* This parameter passes in a boolean value that indicates whether 
* the route is a primary route. 'TRUE' indicates a primary route. 
* 'FALSE' indicates a duplicate route. 
* \i <flags> 
* This parameter passes in a value that indicates which event occurred: 
* 
*  '0' (zero)
* 	This indicates a route added to the routing table by a non-RIP agent. 
* 
*  'RIP_ROUTE_CHANGE_RECD' 
* 	This indicates a route change message. 
* 
*  'RIP_REDIRECT_RECD' 
* 	This indicates and ICMP redirect message. 
* \ie 
* \sh A New Non-RIP Non-System Route was Added to the Routing Table 
* 
* In response to this event, RIP needs to be told whether to ignore 
* or add the route. RIP does this on the basis of the returned 
* function value of the route hook routine. In the case of route-add 
* event, RIP interprets the returned function value of the route hook 
* routine as the metric for the route.
*  
* If the metric is HOPCNT_INFINITY, RIP ignores the route. If the 
* metric is greater than zero but less than HOPCNT_INFINITY, RIP 
* considers the route for inclusion. If the route is new to RIP, RIP 
* adds the new route to its internal database, and then propagates 
* the route in its subsequent update messages. If RIP already stores 
* a route for that destination, RIP compares the metric of the new 
* route and the stored route. If the new route has a better (lower) 
* metric, RIP adds the new route. Otherwise, RIP ignores the new route. 
* 
* When generating its returned function value, your route hook 
* routine can use the creator of the event (<protoID>) as a factor 
* in the decision on whether to include the route. For example, 
* if you wanted the route hook to tell RIP to ignore all non-RIP 
* routes except static routes, your route hook would return 
* HOPCNT_INFINITY if the <protoID> were anything other than 
* 'M2_ipRouteProto_other'. Thus, your route hook routine is a 
* vehicle through which you can implement a policy for including 
* non-RIP routes in the RIP internal route data base. 
* 
* When designing your policy, you should keep in mind how RIP 
* prioritizes these non-RIP routes and when it deletes these 
* non-RIP routes. For example, non-RIP routes never time out. 
* They remain in the RIP database until one of the following 
* events occurs:
* 
* 	'1.'  An agent deletes the route from the system routing table.
* 	'2.'  An agent deletes the interface through which the route passes. 
* 	'3.'  A route change message for the route arrives. 
*  
* Also, these non-RIP routes take precedence over RIP routes to 
* the same destination. RIP ignores routes learned from RIP peers 
* if a route to the same destination was recommended by the hook 
* routine. This non-RIP route takes precedence over the RIP route 
* without regard of the route metric. However, if the route hook 
* routine adds multiple same-destination routes, the route with 
* the lowest metric takes precedence. If the route hook route 
* approves multiple same-metric same-destination routes, the 
* most recently added route is installed.
*  
* \sh A Route Change Notification Arrived
*  
* In response to this event, RIP needs to be told whether to 
* delete the route or change its metric. If the hook returns a 
* value greater than or equal to HOPCNT_INFINITY, RIP deletes 
* the route from its internal routing data base. If the hook 
* routine returns a valid metric (a value greater than zero 
* but less than HOPCNT_INFINITY), RIP reassigns the routes 
* metric to equal the returned value of the route hook routine. 
* If the returned value of the route hook route is invalid 
* (less than zero) RIP ignores the event. RIP also ignores 
* the event if the route specified in <pRouteInfo> is not one 
* stored in its internal data base. 
* 
* \sh An ICMP Redirect Message Arrived 
* 
* In response to this event, RIP never needs to make any changes 
* to its internal routing database. Thus, RIP ignores the returned 
* function value of the route hook routine called in response 
* to an ICMP redirect message. However, if the event is of 
* interest to your particular environment, and it makes sense 
* to catch the event in the context of the RIP task, you can 
* use the route hook routine to do so. 
* 
* Within your route hook routine, your can recognize an ICMP 
* event by checking whether the flags parameter value sets 
* the RIP_REDIRECT_RECD bit. The <primaryRoute> parameter passes 
* in a boolean value that indicates whether the route is 
* primary route. If the <primaryRoute> passes in 'FALSE', the 
* route hook routine need will most likely need to do nothing 
* more. If this parameter passes in 'TRUE', take whatever action 
* (if any) that you know to be appropriate to your particular 
* environment. 
* 
* RETURNS: OK; or ERROR, if RIP is not initialized.
*/

STATUS ripRouteHookAdd
    (
    FUNCPTR 	pRouteHook	/* function pointer to hook */
    )
    {
    if (!ripInitFlag)
        return (ERROR);

    /* Short critical section with message and timer processing. */
 
    semTake (ripLockSem, WAIT_FOREVER);    
    ripState.pRouteHook = pRouteHook;
    semGive (ripLockSem);

    return (OK);
    }

/****************************************************************************
*
* ripRouteHookDelete - remove the route hook
*
* This routine removes the route hook installed earlier by the 
* ripRouteHookAdd() routine. This will cause RIP to ignore any routes
* added to the system Routing database.
*
* RETURNS: OK; or ERROR, if RIP is not initialized.
*
*/

STATUS ripRouteHookDelete (void)
    {

    if (!ripInitFlag)
        return (ERROR);
   
    /* Short critical section with message and timer processing. */
 
    semTake (ripLockSem, WAIT_FOREVER);    
    ripState.pRouteHook = NULL;
    semGive (ripLockSem);
 
    return (OK);
    }

/****************************************************************************
*
* ripRouteHook - sample route hook for non-RIP and redirected routes
*
* This routine allows RIP to include static (and other protocol) routes in
* its database and propagate them to its peers. 
* <pRouteInfo> points to a routing socket message structure that describes
* the route details.
* <protoId> identifies that routing protocol that is installing this route.
* <primaryRoute> indicate if this is the route that will be used by the
* IP forwarding process.
* <flags> denote if this is a ICMP redirected route or a changed route or
* a new route.
*   flags == 0     			New route (non-RIP)
*   flags == RIP_REDIRECT_RECD		ICMP redirected route
*   flags == RIP_ROUTE_CHANGE_RECD	Changed route
* 
* This routine first extracts pointers to the route destination, netmask and
* gateway from the <pRouteInfo> parameter. In the case of redirects, the
* old gateway address is also extracted and then a log message is printed
* if configured to do so and then this routine exits.
*
* If it is a new route, this routine asks RIP to reject all routes except the
* primary routes. For static routes, it assigns a metric that is the same 
* as the interface index + the interface metric + protocol ID.
*
* RETURNS: A metric value (less than HOPCNT_INFINITY), if the route is to
* be accepted; or HOPCNT_INFINITY, if the route is to be ignored.
*
* NOMANUAL
*/

STATUS ripRouteHook
    (
    ROUTE_INFO *	pRouteInfo,	/* Route information */
    int			protoId,	/* Routing protocol ID */
    BOOL		primaryRoute,	/* Primary route ? */
    int			flags		/* Whether redirect */
    )
    {

    int 			ifIndex;
    int			metric;
    struct sockaddr *	pDstAddr, * pNetmask, * pGateway, * pOldGateway;

    /* Extract the address pointers from the route info structure */

    ripAddrsXtract (pRouteInfo, &pDstAddr, &pNetmask, &pGateway, &pOldGateway);

    /* Get the interface index */

    ifIndex = pRouteInfo->rtm.rtm_index;

    if (routedDebug)
        {
        logMsg ("\nripRouteHook: called for proto: %d, ifIndex=%d, Redirect = %s,"
                " Initial metric = %d Primary route=%s\n", protoId, ifIndex, 
                (int)((flags & RIP_REDIRECT_RECD) ? "Yes" : "No"),
                pRouteInfo->rtm.rtm_rmx.rmx_hopcount, 
                (int)(primaryRoute ? "TRUE" : "FALSE"), 0);
        ripSockaddrPrint (pDstAddr);
        ripSockaddrPrint (pNetmask);
        ripSockaddrPrint (pGateway);
        }

    /* If it is a redirect message, print the old gateway and exit */

    if (flags & RIP_REDIRECT_RECD)
        {
        if (routedDebug)
            {
            logMsg ("ripRouteHook: Redirect received from\n", 0, 0, 0, 0, 0, 0);
            ripSockaddrPrint (pOldGateway);
            }
        return (HOPCNT_INFINITY);
        }

    /* If it is a route change message, print the old gateway address */

    if (flags & RIP_ROUTE_CHANGE_RECD)
        {
        if (routedDebug)
            {
            logMsg ("ripRouteHook: route change: Old gateway:\n", 0, 0, 0, 0, 0, 0);
            ripSockaddrPrint (pOldGateway);
            }
        }

    /* 
     * If it is not a primary route return ERROR to let RIP ignore this
     * route
     */

    if (! primaryRoute)
        return (HOPCNT_INFINITY);

    /* 
     * Calculate metric. We give preference to static routes over other routes
     * This is an arbitrary metric that is the sum of protocol ID,
     * interface index and the initial route metric. Since the static route
     * has the lowest protocol ID it gets the lowest metric.
     */

    metric = protoId + ifIndex + pRouteInfo->rtm.rtm_rmx.rmx_hopcount;
    if (metric >= (HOPCNT_INFINITY - 1))
        metric = HOPCNT_INFINITY - 2;

    if (routedDebug) 
        logMsg ("ripRouteHook: returning metric %d\n", metric, 0, 0, 0, 0, 0);
    return (metric);
    }


/****************************************************************************
*
* ripIfSearch - add new interfaces to the internal list
*
* By default, a RIP session will not recognize any interfaces initialized
* after it has started. This routine schedules a search for additional
* interfaces that will occur during the next update of the internal routing
* table. Once completed, the session will accept and send RIP messages over
* the new interfaces.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* INTERNAL
* This routine just sets the flag tested by the watchdog timer handler
* to trigger a search. Mutual exclusion with that thread is not necessary
* in this case, since any change will correctly begin the search, which
* is not time sensitive. However, the actual search routine must lock out 
* all other processing.
*/

void ripIfSearch (void)
    {
    if (!ripInitFlag)
        return;

    ripState.lookforinterfaces = TRUE;
    }

/******************************************************************************
*
* ripIfReset - alter the RIP configuration after an interface changes
*
* This routine updates the interface list and routing tables to reflect
* address and/or netmask changes for the device indicated by <pIfName>. 
* To accommodate possible changes in the network number, all routes using 
* the named interface are removed from the routing tables, but will be
* added in the next route update if appropriate. None of the removed
* routes are poisoned, so it may take some time for the routing tables of
* all the RIP participants to stabilize if the network number has changed.
* This routine replaces the existing interface structure with a new one.
* Thus, any interface specific MIB2 changes that were made to the interface
* being reset will be lost
*
* RETURNS: OK, or ERROR if named interface not found or not added to list.
*
* ERRNO: N/A
*
* INTERNAL
* This routine uses the mutex semaphore to block all other RIP processing
* until the data structures are in a stable state. The internal RIP routing
* table and the kernel routing table will be updated after it returns. If
* the network number did not change, all the removed routes received from
* other RIP sessions are still valid and will be added by the next update.
*/

STATUS ripIfReset 
    (
    char * 	pIfName 	/* name of changed interface */
    )
    {

    long 		ifIndex;
    int 			slen; 
    int 			tlen;
    BOOL		found = FALSE;
    struct interface *	pIf; 	/* Target interface entry, if found. */
    struct interface *	pPrevIf;/* Previous interface entry. */
    struct interface *	pNextIf;/* Next interface entry. */

    STATUS result = OK;

    if (!ripInitFlag)
        return (ERROR);

    if (pIfName == NULL)
        return (ERROR);

    slen = strlen (pIfName);
    if (slen == 0)
        return (ERROR);

    /* Get the index of the Interface */

    ifIndex = ifNameToIfIndex (pIfName);
    if (ifIndex == 0)
        {
        /* 
         * Well, this means that the interface is gone. We just
         * remove its entries in our table
         */
        }

    /* 
     * Block access to all processing while updating the interface 
     * list and routing tables. 
     */

    semTake (ripLockSem, WAIT_FOREVER);

    pPrevIf = ripIfNet; 
    for (pIf = ripIfNet; pIf; pIf = pNextIf)
        {
        /* 
         * Save the next interface value now as we might delete the
         * interface below.
         */

        pNextIf = pIf->int_next;

        /* 
         * Ignore names of different lengths to prevent a false match 
         * between overlapping unit numbers such as "ln1" and "ln10".
         */

        tlen = strlen (pIf->int_name);
        if (tlen != slen)
            {
            pPrevIf = pIf;
            continue;
            }

        if (strcmp (pIfName, pIf->int_name) == 0)
            {
            found = TRUE;

            /*
             * Mark the interface down so that ifRouteAdd() will not
             * select this interface
             */

            pIf->int_flags &=  ~IFF_UP;

            /* 
             * Now delete all routes passing through this interface.
             * including the locally generated entries for accessing the
             * directly connected network
             */

            ripRoutesDelete (pIf, TRUE);

            /* Now delete the interface itself */

            ripInterfaceIntDelete (pIf, pPrevIf);
            }
        else 
            pPrevIf = pIf;
        }

    /* If interface was not found, return error */

    if (!found)
        {
        semGive (ripLockSem);
        return (ERROR);
        }

    /* 
     * Add the new entry from kernel's list. If ifIndex == 0, that means
     * the interface has been deleted. Don't bother calling routedIfInit
     * if that's the case.
     */

    if (ifIndex != 0) 
        result = routedIfInit (FALSE, ifIndex); 

    semGive (ripLockSem);

    return (result);
    }

/******************************************************************************
* 
* ripFilterEnable - activate strict border gateway filtering
*
* This routine configures an active RIP session to enforce the restrictions 
* necessary for RIP-1 and RIP-2 routers to operate correctly in the same 
* network as described in section 3.2 of RFC 1058 and section 3.3 of RFC 1723. 
* When enabled, routes to portions of a logical network (including host 
* routes) will be limited to routers within that network. Updates sent 
* outside that network will only include a single entry representing the 
* entire network. That entry will subsume all subnets and host-specific
* routes. If supernets are used, the entry will advertise the largest
* class-based portion of the supernet reachable through the connected
* interface.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* INTERNAL
* This routine allows the supply() routine in ./rip/output.c to call
* the family-specific sendroute routine that selects the RTS_INTERNAL 
* route entries for transmission outside of a network. The current
* implementation for AF_INET routes is the inet_sendroute routine in
* the ./rip/inet.c file.
*/

void ripFilterEnable (void)
    {
    if (!ripInitFlag)
        return;

    /* Wait for current processing to complete before changing behavior. */

    semTake (ripLockSem, WAIT_FOREVER);
    ripFilterFlag = TRUE;
    semGive (ripLockSem);

    return;
    }

/******************************************************************************
*
* ripFilterDisable - prevent strict border gateway filtering
*
* This routine configures an active RIP session to ignore the restrictions 
* necessary for RIP-1 and RIP-2 routers to operate correctly in the same 
* network. All border gateway filtering is ignored and all routes to
* subnets, supernets, and specific hosts will be sent over any available 
* interface. This operation is only correct if no RIP-1 routers are present 
* anywhere on the network. Results are unpredictable if that condition is 
* not met, but high rates of packet loss and widespread routing failures are 
* likely.
*
* The border gateway filtering rules are in force by default.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* INTERNAL
* This routine prevents the supply() routine in ./rip/output.c from executing
* the family-specific sendroute routine that limits the propagation
* of internal network information. The current implementation for AF_INET 
* routes is the inet_sendroute routine in the ./rip/inet.c file.
*/

void ripFilterDisable (void)
    {
    if (!ripInitFlag)
        return;

    semTake (ripLockSem, WAIT_FOREVER);
    ripFilterFlag = FALSE;
    semGive (ripLockSem);

    return;
    }

/******************************************************************************
*
* ripShutdown - terminate all RIP processing
*
* This routine "poisons" all routes in the current table by transmitting
* updates with an infinite metric for each entry over all available
* interfaces. It then halts all RIP processing and removes the associated
* tasks and data structures. When completed successfully, the RIP
* services are unavailable until restarted with the ripLibInit() routine.
*
* RETURNS: OK if shutdown completed, or ERROR otherwise.
*
* ERRNO: N/A
*
* INTERNAL
* The ripState global is not changed by this routine so that the system
* status can still be examined before restarting. That global is cleared
* by the initialization routine.
*/

STATUS ripShutdown (void)
    {
    register struct rthash *rh;
    register struct interface *ifp, *pTemp;
    register struct rt_entry *rt;
    struct rthash *base = hosthash;
    int doinghost = 1;
#ifdef RIP_MD5
    RIP_AUTH_KEY * pAuth;
#endif /* RIP_MD5 */

#ifndef VIRTUAL_STACK
    IMPORT RIP ripState;
#endif

    if (!ripInitFlag)
        return (OK);

    /* 
     * Obtain the lock semaphore to synchronize the shutdown with the
     * timer and message processing threads.
     */

    semTake (ripLockSem, WAIT_FOREVER);

    /* 
     * Remove the watchdog timer and RIP tasks to prevent further updates
     * and eliminate the mutex semaphore. The timer task must be removed
     * first to prevent an invalid reference to the watchdog.
     */

    taskDelete (ripState.ripTimerTaskId);
    wdCancel (ripState.timerDog);
    wdDelete (ripState.timerDog);
    taskDelete (ripState.ripTaskId);
    semDelete (ripLockSem);

    /* Set the route hook to NULL */

    ripState.pRouteHook = NULL;

    /* 
     * Poison all known routes to signal neighboring RIP 
     * routers of pending exit.
     */

    if (ripState.supplier)
        {
        again:
        for (rh = base; rh < &base[ROUTEHASHSIZ]; rh++)
            {
            rt = rh->rt_forw;
            for (; rt != (struct rt_entry *)rh; rt = rt->rt_forw)
                rt->rt_metric = HOPCNT_INFINITY;
            }
        if (doinghost)
            {
            doinghost = 0;
            base = nethash;
            goto again;
            }
        toall (supply, 0, (struct interface *)NULL);
	}

    /* Remove all entries from the kernel and RIP routing tables. */

    rtdeleteall ();

    /* Remove list of available interfaces. */

    ifp = ripIfNet;
    while (ifp != NULL)
        {
        pTemp = ifp;
        ifp = pTemp->int_next;

#ifdef RIP_MD5
        /* free the auth keys for this interface */
        pAuth = pTemp->pAuthKeys;
        while (pAuth != NULL)
            {
            pTemp->pAuthKeys = pAuth->pNext;
            free (pAuth);
            pAuth = pTemp->pAuthKeys;
            }
#endif /* RIP_MD5 */

        free (pTemp);
        }

    /* Free up the RIP interface exclusion list */

    lstFree (&ripIfExcludeList);

    /* Remove the remaining data structures and the input/output socket. */

    semDelete (ripState.timerSem);
    close (ripState.s); 
    close (ripState.routeSocket); 

    /* Set ripInitFlag to FALSE now that we have uninitialized RIP */

    ripInitFlag = FALSE;

    return (OK);
    }

/******************************************************************************
*
* ripDebugLevelSet - specify amount of debugging output
*
* This routine determines the amount of debugging information sent to
* standard output during the RIP session. Higher values of the <level>
* parameter result in increasingly verbose output. A <level> of zero
* restores the default behavior by disabling all debugging output.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void ripDebugLevelSet
    (
    int 	level 		/* verbosity level (0 - 3) */
    )
    {
    /* Disable debugging if a bogus value is given. */

    if (level < 0 || level > 3)
        level = 0;

    routedDebug = level;
    }


#ifdef RIP_MD5

/***************************************************************************
*
* ripAuthKeyShow - show current authentication configuration
*
* This routines shows the current configuration of the authentication keys
* for each interface.
*
* RETURNS: N/A
*/
void ripAuthKeyShow
    (
    UINT showKey        /* if non-zero then key values are shown */
    )
    {
    register RIP_AUTH_KEY * pAuthKey;
    register struct interface *ifp;
    time_t t;

    semTake (ripLockSem, WAIT_FOREVER);

    printf ("\n%-10s  %-9s %-10s  %-15s %-24s\n", 
            "Interface", "KeyID", "Protocol", "Expires", "Key");
    printf ("----------------------------------------------------------------" 
            "----------\n");

    t = time (NULL);

    for (ifp = ripIfNet; ifp; ifp = ifp->int_next)
        {
        for (pAuthKey = ifp->pAuthKeys; pAuthKey; pAuthKey = pAuthKey->pNext)
            {
            printf ("%-10s  ", ifp->int_name);
            printf ("%-9u ", pAuthKey->keyId);

            if (pAuthKey->authProto == RIP_AUTH_PROTO_MD5)
                printf ("%-10s  ", "MD5");
            else if (pAuthKey->authProto == RIP_AUTH_PROTO_SHA1)
                printf ("%-10s  ", "SHA-1");
            else
                printf ("%-10s  ", "unknown");

            if (pAuthKey->timeEnd > t)
                printf ("%-15lu ", (pAuthKey->timeEnd - t));
            else
                printf ("%-15s ", "expired");

            if (showKey)
                printf ("%-24s\n", pAuthKey->pKey);
            else
                printf ("<not shown>\n");
            }
        }

    printf ("\n");

    semGive (ripLockSem);
    }


/***************************************************************************
*
* ripAuthKeyAdd - add a new RIP authentication key
*
* This routine is used to add a new RIP authentication key for a specific
* interface.
*
* RETURNS: ERROR, if the interface does not exist, or the <keyId> already 
* exists, or if the protocol is not supported; OK, if key was entered.
*/
STATUS ripAuthKeyAdd
    (
    char *      pInterfaceName, /* interface to add a key */
    UINT16      keyId,          /* the keyId for this new key */
    char *      pKey,           /* the secret key */
    UINT        keyLen,         /* length of the secret key */
    UINT        authProto,      /* auth protocol to use (1 = MD5) */
    ULONG       timeValid       /* number of seconds until key expires */
    )
    {
    register struct interface *ifp;
    register RIP_AUTH_KEY * pAuthKey;
    RIP_AUTH_KEY * pNewAuthKey;

    if (authProto != RIP_AUTH_PROTO_MD5) return ERROR;

    semTake (ripLockSem, WAIT_FOREVER);

    for (ifp = ripIfNet; ifp; ifp = ifp->int_next)
        {
        if (strcmp (pInterfaceName, ifp->int_name) == 0)
            {
            for (pAuthKey = ifp->pAuthKeys; pAuthKey;
                 pAuthKey = pAuthKey->pNext)
                {
                if (pAuthKey->keyId == keyId)
                    {
                    semGive (ripLockSem);
                    return ERROR;
                    }
                }

            pNewAuthKey = malloc (sizeof (RIP_AUTH_KEY));
            if (pNewAuthKey == NULL)
                {
                semGive (ripLockSem);
                return ERROR;
                }

            memset (pNewAuthKey, 0, sizeof (RIP_AUTH_KEY));
            pNewAuthKey->keyId = keyId;
            pNewAuthKey->authProto = authProto;
            pNewAuthKey->timeStart = time (NULL);
            pNewAuthKey->timeEnd = (pNewAuthKey->timeStart + timeValid);
            memcpy (pNewAuthKey->pKey, pKey, keyLen);
            pNewAuthKey->ifp = ifp;
            pNewAuthKey->pNext = ifp->pAuthKeys;
            ifp->pAuthKeys = pNewAuthKey;

            semGive (ripLockSem);
            return OK;
            }
        }

    semGive (ripLockSem);

    return ERROR;
    }


/***************************************************************************
*
* ripAuthKeyDelete - delete an existing RIP authentication key
*
* This routine is used to delete a RIP authentication key for a specific
* interface.
*
* RETURNS: ERROR, if the interface does not exist, or the <keyId> does not 
* exist; OK, if key was deleted.
*/
STATUS ripAuthKeyDelete
    (
    char *      pInterfaceName, /* interface to delete a key from */
    UINT16      keyId           /* the keyId of the key to delete */
    )
    {
    register struct interface *ifp;
    register RIP_AUTH_KEY * pPrevAuthKey;
    register RIP_AUTH_KEY * pAuthKey;

    semTake (ripLockSem, WAIT_FOREVER);

    for (ifp = ripIfNet; ifp; ifp = ifp->int_next)
        {
        if (strcmp (pInterfaceName, ifp->int_name) == 0)
            {
            if (ifp->pAuthKeys == NULL)
                {
                semGive (ripLockSem);
                return ERROR;
                }

            pPrevAuthKey = ifp->pAuthKeys;
            pAuthKey = pPrevAuthKey->pNext;

            if (pPrevAuthKey->keyId == keyId)
                {
                ifp->pAuthKeys = pAuthKey;
                free (pPrevAuthKey);
                semGive (ripLockSem);
                return OK;
                }

            while (pAuthKey)
                {
                if (pAuthKey->keyId == keyId)
                    {
                    pPrevAuthKey->pNext = pAuthKey->pNext;
                    free (pAuthKey);
                    semGive (ripLockSem);
                    return OK;
                    }
                pPrevAuthKey = pAuthKey;
                pAuthKey = pAuthKey->pNext;
                }

            semGive (ripLockSem);
            return ERROR;
            }
        }

    semGive (ripLockSem);

    return ERROR;
    }


/***************************************************************************
*
* ripAuthKeyFind - find a RIP authentication key
*
* This routine is used to find a RIP authentication key based on a specified
* interface and keyId.  When a key is found, a pointer to the RIP_AUTH_KEY
* struct for the key is stored in pKey.
*
* RETURNS: ERROR, if the key is not found; OK if the key was found.
*/
STATUS ripAuthKeyFind
    (
    struct interface *  ifp,   /* interface to search for key */
    UINT16              keyId, /* the keyId of the key to search for */
    RIP_AUTH_KEY **     pKey   /* storage for the key data */
    )
    {
    register RIP_AUTH_KEY * pAuthKey;

    for (pAuthKey = ifp->pAuthKeys; pAuthKey; pAuthKey = pAuthKey->pNext)
        {
        if (pAuthKey->keyId == keyId)
            {
            *pKey = pAuthKey;
            return OK;
            }
        }

    return ERROR;
    }


/***************************************************************************
*
* ripAuthKeyFindFirst - find a RIP authentication key
*
* This routine is used to find a RIP authentication key based on a specified
* interface.  Because a <keyId> is not specified, this routine returns the 
* first non-expired key found for the interface.  When a key is found, a 
* pointer to the RIP_AUTH_KEY structure for the key is returned in <pKey>.
*
* RETURNS: ERROR, if a key is not found; OK, if a key was found.
*/
STATUS ripAuthKeyFindFirst
    (
    struct interface *  ifp,   /* interface to search for key */
    RIP_AUTH_KEY **     pKey   /* storage for the key data */
    )
    {
    register RIP_AUTH_KEY * pAuthKey;
    RIP_AUTH_KEY * pBackup;

    time_t t;

    /*
     * Save an available key for possible use if no valid entries exist,
     * regardless of the expiration time (RFC 2082, section 4.3).
     */

    pBackup = ifp->pAuthKeys;
    if (pBackup == NULL)
        {
        logMsg ("RIP: Error! No MD5 keys available!", 0, 0, 0, 0, 0, 0);
        return (ERROR);
        }

    /* check that the key has not yet expired */

    t = time (NULL);

    for (pAuthKey = ifp->pAuthKeys; pAuthKey; pAuthKey = pAuthKey->pNext)
        {
        if (pAuthKey->timeEnd > t)
            {
            *pKey = pAuthKey;
            return OK;
            }
        }

    /*
     * All keys expired! Send notification to network manager and treat
     * available key as having an infinite lifetime (RFC 2082, section 4.3).
     */

    logMsg ("RIP: Warning! Last authentication key expired. Using key ID %x.\n",
            pBackup->keyId, 0, 0, 0, 0, 0);

    *pKey = pBackup;
    return ERROR;
    }


/***************************************************************************
*
* ripAuthKeyInMD5 - authenticate an incoming RIP-2 message using MD5
*
* This routine is used to authenticate an incoming RIP-2 message using
* the MD5 digest protocol.  This authentication scheme is described in
* RFC 2082.
*
* RETURNS: ERROR, if could not authenticate; OK, if authenticated.
*/
STATUS ripAuthKeyInMD5
    (
    struct interface *  ifp,    /* interface message received on */
    RIP_PKT * 	        pRip,	/* received RIP message */
    UINT                size    /* length of the RIP message */
    )
    {
    RIP2_AUTH_PKT_HDR * pAuthHdr;
    RIP2_AUTH_PKT_TRL * pAuthTrl;
    RIP_AUTH_KEY * pAuthKey;
    UCHAR savedDigest [MD5_DIGEST_LEN];
    UCHAR newDigest [MD5_DIGEST_LEN];
    UINT numRipEntries;
    MD5_CTX_T context;
    time_t t;

    /* must be a RIP version 2 packet */
    if (pRip->rip_vers == 1)
        return ERROR;

    pAuthHdr = (RIP2_AUTH_PKT_HDR *)pRip->rip_nets;

    /* check basic auth header fields */
    if ((pAuthHdr->authIdent != RIP2_AUTH) ||
        (ntohs (pAuthHdr->authType) != M2_rip2IfConfAuthType_md5) ||
        (pAuthHdr->zero1 != 0) || (pAuthHdr->zero2 != 0))
        return ERROR;

    /* get the secret key for the given keyid for this interface */
    if (ripAuthKeyFind (ifp, pAuthHdr->keyId, &pAuthKey) == ERROR)
        return ERROR;

    /* check that the key has not yet expired */
    t = time (NULL);
    if (pAuthKey->timeEnd <= t)
        return ERROR;

    /* check that MD5 is being used for this key and the authDataLen is 16 */
    if ((pAuthKey->authProto != RIP_AUTH_PROTO_MD5) ||
        (pAuthHdr->authDataLen != MD5_DIGEST_LEN))
        return ERROR;

    /* make sure the trailing digest is the correct length */
    if ((size - (ntohs (pAuthHdr->packetLen) + 4)) != MD5_DIGEST_LEN)
        return ERROR;

    /* make sure all RIP entries are 20 bytes long */
    if (((ntohs (pAuthHdr->packetLen) - 24) % 20) != 0)
        return ERROR;

    /* get the number of RIP entries including the auth header */
    numRipEntries = ((ntohs (pAuthHdr->packetLen) - 24) / 20);

    /* get the auth trailer that includes the message digest */
    pAuthTrl = (RIP2_AUTH_PKT_TRL *)(pAuthHdr + 1 + numRipEntries);

    /* check basic auth trailer fields */
    if ((pAuthTrl->authIdent != RIP2_AUTH) ||
        ((ntohs (pAuthTrl->authTag) != 0x01) &&
         (ntohs (pAuthTrl->authTag) != M2_rip2IfConfAuthType_md5)))
        return ERROR;

    /* save the old digest */
    memcpy (savedDigest, pAuthTrl->authDigest, MD5_DIGEST_LEN);

    /* write the secret key over the digest in the packet */
    memcpy (pAuthTrl->authDigest, pAuthKey->pKey, MD5_DIGEST_LEN);

    /* compute the MD5 authentication digest */
    memset (&context, 0, sizeof (MD5_CTX_T));
    MD5Init (&context);
    MD5Update (&context, (UCHAR *)pRip, size);
    MD5Final (newDigest, &context);

    /* check if the message is authentic by comparing the digests */
    if (memcmp (savedDigest, newDigest, MD5_DIGEST_LEN) != 0)
        return ERROR;

    /*
     * check that the sequence number is greater than the last received
     * make sure this check is done after the message is authenticated
     */
    if ((ntohl (pAuthHdr->sequence) != 0) &&
        (ntohl (pAuthHdr->sequence) <= pAuthKey->lastRcvSequence))
        return ERROR;

    /* save the new recieved sequence number */
    pAuthKey->lastRcvSequence = ntohl (pAuthHdr->sequence);

    return OK; /* Gee wiz... */
    }

/***************************************************************************
*
* ripAuthKeyOut1MD5 - start MD5 authentication of an outgoing RIP-2 message
*
* This routine is used to start the authentication of an outgoing RIP-2
* message by adding the authentication header used for MD5 authentication.
* This authentication scheme is described in RFC 2082.  This function
* returns a pointer the authentication header and a pointer to the looked up 
* authentication key.
*
* RETURNS: ERROR, if a key could not be found; OK, if the header was added.
*/
STATUS ripAuthKeyOut1MD5
    (
    struct interface *   pIfp,      /* interface message being sent on */
    struct netinfo *     pNetinfo,  /* pointer to next RIP entry to fill in */
    RIP2_AUTH_PKT_HDR ** ppAuthHdr, /* stores the authentication header */
    RIP_AUTH_KEY **      ppAuthKey  /* stores the authentication key to use */
    )
    {
    /* get the first RIP entry for storing auth data */
    *ppAuthHdr = (RIP2_AUTH_PKT_HDR *)pNetinfo;
    memset (*ppAuthHdr, 0, sizeof (RIP2_AUTH_PKT_HDR));

    /* fill in the auth tag and type */
    (*ppAuthHdr)->authIdent = RIP2_AUTH;
    (*ppAuthHdr)->authType = htons (M2_rip2IfConfAuthType_md5);

    /* the packet len will be filled in later (in ripAuthKeyOut2MD5) */

    /* get a secret key for this interface */
    if (ripAuthKeyFindFirst (pIfp, ppAuthKey) == ERROR)
        return ERROR;

    /* fill in the keyId and authDataLen */
    (*ppAuthHdr)->keyId = (*ppAuthKey)->keyId;
    if ((*ppAuthKey)->authProto != RIP_AUTH_PROTO_MD5)
        return ERROR;
    (*ppAuthHdr)->authDataLen = MD5_DIGEST_LEN;

    /*
     * Fill in the sequence number.  If the last sent is 0 then
     * no RIP message has been sent out this interface.  So 0 must
     * be sent as the sequence number.
     */
    if ((*ppAuthKey)->lastSntSequence == 0)
        (*ppAuthHdr)->sequence = 0;
    else
        (*ppAuthHdr)->sequence = htonl ((*ppAuthKey)->lastSntSequence + 1);
    (*ppAuthKey)->lastSntSequence++;

    return OK;
    }

/***************************************************************************
*
* ripAuthKeyOut2MD5 - authenticate an outgoing RIP-2 message using MD5
*
* This routine is used to authenticate an outgoing RIP-2 message using
* the MD5 digest protocol.  This authentication scheme is described in
* RFC 2082.  This function modifies the size given in pSize to account
* for the extra auth trailer data.  The auth trailer is appended to the
* given RIP_PKT and the authentication digest is filled in.
*
* RETURNS: N/A
*/
void ripAuthKeyOut2MD5
    (
    RIP_PKT * 	        pRip,	  /* RIP message to authenticate */
    UINT *              pSize,    /* length of the RIP message */
    struct netinfo *    pNetinfo, /* pointer to next RIP entry to fill in */
    RIP2_AUTH_PKT_HDR * pAuthHdr, /* pointer to auth header in the message */
    RIP_AUTH_KEY *      pAuthKey  /* the auth key data to use */
    )
    {
    RIP2_AUTH_PKT_TRL * pAuthTrl;
    MD5_CTX_T context;
    UCHAR newDigest [MD5_DIGEST_LEN];

    /* get the last RIP entry for storing the auth digest */
    pAuthTrl = (RIP2_AUTH_PKT_TRL *)pNetinfo;
    memset (pAuthTrl, 0, sizeof (RIP2_AUTH_PKT_TRL));

    /* fill in the packet len located in the auth header */
    pAuthHdr->packetLen = htons ((UINT16)*pSize);

    /* increment the size to show the auth trailer */
    *pSize += sizeof (struct netinfo);

    /* fill in the auth tag and type */
    pAuthTrl->authIdent = RIP2_AUTH;
    pAuthTrl->authTag = htons (0x01);

    /* copy in the secret key into the auth trailer */
    memcpy (pAuthTrl->authDigest, pAuthKey->pKey, MD5_DIGEST_LEN);

    /* compute the digest and store it in the auth trailer */
    memset (&context, 0, sizeof (MD5_CTX_T));
    MD5Init (&context);
    MD5Update (&context, (UCHAR *)pRip, *pSize);
    MD5Final (newDigest, &context);
    memcpy (pAuthTrl->authDigest, newDigest, MD5_DIGEST_LEN);
    }

#endif /* RIP_MD5 */

#ifdef ROUTER_STACK
LOCAL BOOL ripRouteSame 
    (
    struct rt_entry *	pRtEntry,	/* RIP Route entry to compare */
    struct sockaddr *	pDstAddr,	/* Destination addr to compare with */
    struct sockaddr *	pNetmask,	/* Netmask to compare with */
    struct sockaddr *	pGateway	/* Gateway to compare with */
    )
    {
    struct sockaddr	netmask;
    struct sockaddr_in *	pNsin;

    if (routedDebug)
        {
        logMsg ("\n\nripRouteSame: route entry:\n", 0, 0, 0, 0, 0, 0);
        ripSockaddrPrint (&pRtEntry->rt_dst);
        ripSockaddrPrint (&pRtEntry->rt_netmask);
        ripSockaddrPrint (&pRtEntry->rt_router);
        logMsg ("ripRouteSame: to be compared entry:\n", 0, 0, 0, 0, 0, 0);
        ripSockaddrPrint (pDstAddr);
        ripSockaddrPrint (pNetmask);
        ripSockaddrPrint (pGateway);
        logMsg ("\n\n", 0, 0, 0, 0, 0, 0);
        }

    /* If destination values differ, return FALSE */

    if (!equal (pDstAddr, &pRtEntry->rt_dst))
        return (FALSE);

    /* 
     * If the gateway value is specified, then 
     * compare the gateway value we have in the route entry.
     * If the gateway value is not specified, then this is a primary route
     * that is being changed. Check that the route entry we have is also for
     * the primary route. If not, the routes don't match.
     */

    if (pGateway)
        {
        if (!equal (pGateway, &pRtEntry->rt_router))
            return (FALSE);
        }
    else
        {
        if (! (pRtEntry->rt_state & RTS_PRIMARY))
            return (FALSE);
        }

    /* 
     * We always store the netmask of a default route as all zeros.
     * So if it is a default route, we need only compare the destination
     * and gateway address fields
     */

    if (((struct sockaddr_in *)&(pRtEntry->rt_dst))->sin_addr.s_addr == 0)
        return (TRUE);

    /* 
     * If netmask is specified, check that they match. We
     * check only the address part as thge prefix part sometimes
     * differs
     */

    if (pNetmask)
        return (SOCKADDR_IN (pNetmask) == SOCKADDR_IN (&pRtEntry->rt_netmask));

    /* 
     * If no netmask was specified, we calculated the netmask to be that
     * of the interface. Check for that case
     */

    bzero ((char *)&netmask, sizeof (netmask));
    netmask.sa_family = AF_INET;
    netmask.sa_len = 16;
    pNsin = (struct sockaddr_in *)&netmask;
    pNsin->sin_addr.s_addr = htonl (pRtEntry->rt_ifp->int_subnetmask);
    return (equal (&netmask, &pRtEntry->rt_netmask));
    }

/***************************************************************************
*
* ripInterfaceUpFlagSet - set or reset the interface UP flag
*
* This function sets or resets the IFF_UP flag of the interface specified
* by <pIfName> according to the status described by <up>. If the interface
* is being set up, it also adds the interface routes the interface,
* else if the interface is being brought down, it deletes all routes 
* passing through the interface except the routes learned through the
* Routing system callback.
*
* RETURNS: OK or ERROR.
*
* NOMANUAL
*
* INTERNAL
* This routine loops through the entire interface table looking for
* interfaces with the specified name. A single IP interface with aliases
* is stored by RIP as multiple interface with the same name (but different
* addresses). Hence the looping is needed.
*/

LOCAL STATUS ripInterfaceUpFlagSet
    (
    u_short 	ifIndex,
    BOOL 	up
    )
    {
    int 			error = ERROR;
    struct interface * 	pIf; 		/* Target interface entry, if found. */

    if (ifIndex == 0) 
        return (ERROR);


    if (routedDebug) 
        logMsg ("ripInterfaceUpFlagSet: Setting %d %s\n", 
                ifIndex, (int)(up ? "UP" : "DOWN"), 0, 0, 0, 0);

    /* 
     * Now loop through our interface list and set the flag of all matching
     * interfaces. We might have more than one interface match a given
     * index if the interface is aliased. We treat each alias as a separate
     * interface.
     */

    for (pIf = ripIfNet; pIf; pIf = pIf->int_next)
        {
        if (ifIndex == pIf->int_index)
            {
            if (up && (pIf->int_flags & IFF_UP) == 0) 
                {
                pIf->int_flags |= IFF_UP;

                /*
                 * Now add a route to the local network. This will delete
                 * any non-interface routes that we might have learned
                 * for the network while we were down
                 */

                addrouteforif (pIf);

                }
            else if (!up && (pIf->int_flags & IFF_UP) == IFF_UP) 
                {
                pIf->int_flags &= ~IFF_UP;

                /* 
                 * Now delete all routes that pass through this interface.
                 * Don't delete the STATIC or non-RIP routes that we learned
                 * for this interface (through the Routing system callback
                 * as we will never ever re-learn them
                 */

                ripRoutesDelete (pIf, FALSE);
                }

            error = OK;
            }
        }
    return (error);
    }
#endif /* ROUTER_STACK */

/***************************************************************************
*
* ripRoutesDelete - delete routes passing through an interface
*
* This function deletes all routes passing though the interface specified
* by the <pIf> parameter. If the deleteAllRoutes parameter is set to FALSE,
* then only the routes that were added by us (the interface routes and routes
* learned from our peers) are deleted. The Static and other routing protocol
* routes  (like OSPF) learned through the routing syste, callback are not
* deleted.
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*/

LOCAL STATUS ripRoutesDelete
    (
    struct interface *	pIf,
    BOOL		deleteAllRoutes
    )
    {
    struct rt_entry *	rt;
    struct sockaddr *	dst;
    struct rthash * 	pHashList; 	/* Individual hash table entry. */
    struct rt_entry *	pRt; 		/* Individual route entry. */
    struct sockaddr_in 	net;
    int 			error = ERROR;

    if (!(pIf->int_flags & IFF_ROUTES_DELETED))
        {
        pIf->int_flags |= IFF_ROUTES_DELETED;

        /* 
         * If the interface is subnetted, then we must have added
         * (or incremented ref count on) the class based network route.
         * If we added the route, then that will be taken care of in
         * rt_delete(). If we just increased the refcnt, then we need to
         * decrease the refcnt
         */
        if (!(pIf->int_flags & IFF_POINTOPOINT) &&
            pIf->int_netmask != pIf->int_subnetmask)
            {

            /* look for route to logical network */
            memset(&net, 0, sizeof (net));
            net.sin_len = sizeof (net);
            net.sin_family = AF_INET;
            net.sin_addr.s_addr = htonl (pIf->int_net);
            dst = (struct sockaddr *)&net;
            rt = rtlookup(dst);
            if (rt && rt->rt_ifp != pIf)
                {
                /* 
                 * We didn't add the route, just incremented the ref count.
                 * Decrement the ref counts now
                 */
                rt->rt_refcnt--;
                rt->rt_subnets_cnt--;
                }
            }

        /*
         * Now check if we added the network (remote, for PPP links)
         * route. If we did, then no need to do anything as rtdelete will
         * handle the route deletion. If we didn't add the route, then 
         * we must've increased the ref count. We need to decrement the ref
         * count now that we will not be referencing it any more.
         */
        if (pIf->int_flags & IFF_POINTOPOINT)
            dst = &pIf->int_dstaddr;
        else
            {
            memset(&net, 0, sizeof (net));
            net.sin_len = sizeof (net);
            net.sin_family = AF_INET;
            net.sin_addr.s_addr = htonl (pIf->int_subnet);
            dst = (struct sockaddr *)&net;
            }

        if ((rt = rtlookup(dst)) == NULL) 
            rt = rtfind(dst);
        if (rt && rt->rt_ifp != pIf)
            rt->rt_refcnt--;

        /*
         * For a point to point link, we either added a route for the
         * local end of the link, or incremented the reference count 
         * on an existing  route. If there are no other references to that 
         * route, delete it, else just decrement the reference count
         * NOTE: He we don't check for the interface, as the interface is
         * the same (loopback) for all instances of this route
         */
        if (pIf->int_flags & IFF_POINTOPOINT)
            {
            dst = &pIf->int_addr;
            if ((rt = rtlookup(dst)) == NULL) 
                rt = rtfind(dst);
            if (rt)
                {
                if (rt->rt_refcnt == 0)
                    rtdelete (rt);
                else 
                    rt->rt_refcnt--;
                }
            }
        }

    /* 
     * Remove any routes associated with the changed interface from
     * both hash tables.
     */

    for (pHashList = hosthash; pHashList < &hosthash [ROUTEHASHSIZ]; 
         pHashList++)
        {
        pRt = pHashList->rt_forw;

        for (; pRt != (struct rt_entry *)pHashList; pRt = pRt->rt_forw)
             {
             if (pRt->rt_ifp != pIf)
                 continue;

             if (!deleteAllRoutes && ((pRt->rt_state & RTS_OTHER) != 0))
                 continue;

             pRt = pRt->rt_back;
             rtdelete (pRt->rt_forw);
             error = OK;
             }
        }

    for (pHashList = nethash; pHashList < &nethash [ROUTEHASHSIZ]; pHashList++)
        {
        pRt = pHashList->rt_forw;

        for (; pRt != (struct rt_entry *)pHashList; pRt = pRt->rt_forw)
             {
             if (pRt->rt_ifp != pIf)
                 continue;

             if (!deleteAllRoutes && ((pRt->rt_state & RTS_OTHER) != 0))
                 continue;

             pRt = pRt->rt_back;
             rtdelete (pRt->rt_forw);
             error = OK;
             }
        }

    return (error);
    }

#ifdef ROUTER_STACK
/***************************************************************************
*
* ripInterfaceDelete - delete the interface
*
* This function deletes the interface structure pointed to by <pIf>
* and frees the memory associated with it. This function is actually
* a wrapper for the ripInterfaceIntDelete() call that actually deletes
* the interface.
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*
*/

LOCAL STATUS ripInterfaceDelete
    (
    struct interface *	pIf
    )
    {
    struct interface *	pTmpIf;
    struct interface *	pPrevIf;

    /* Search for the interface in our list */

    pPrevIf = ripIfNet; 
    for (pTmpIf = ripIfNet; pTmpIf && pTmpIf != pIf; pTmpIf = pTmpIf->int_next)
        pPrevIf = pTmpIf;

    if (pTmpIf == NULL)
        return (ERROR);

    return (ripInterfaceIntDelete (pIf, pPrevIf));
    }
#endif /* ROUTER_STACK */

/***************************************************************************
*
* ripInterfaceIntDelete - delete the interface
*
* This function deletes the interface structure pointed to by <pIf>
* from the list of interfaces and frees the memory associated with
* the interface.
*
* RETURNS: OK or ERROR
*
* NOMANUAL
*
*/

LOCAL STATUS ripInterfaceIntDelete
    (
    struct interface *	pIf,
    struct interface *	pPrevIf
    )
    {
#ifdef RIP_MD5
    RIP_AUTH_KEY *	pAuth;
#endif /* RIP_MD5 */

    /* Reset head of the list if necessary. */

    if (pIf == ripIfNet)
        ripIfNet = pIf->int_next;
    else
        pPrevIf->int_next = pIf->int_next;

    /* 
     * If the matching interface is the last entry in the list, reset
     * the (global) tail pointer so the list will rebuild correctly.
     */

    if (pIf->int_next == NULL)
        {
        /* 
         * Set tail pointer to head of list if the matching interface 
         * is the only entry. Otherwise set it to the next field of
         * the new list tail (which currently points to NULL).
         */

        if (ripIfNet == NULL)   /* Only entry in list? */
            ifnext = &ripIfNet;
        else
            ifnext = &pPrevIf->int_next;
        }

#ifdef RIP_MD5
    /* free the auth keys for this interface */
    pAuth = pIf->pAuthKeys;
    while (pAuth != NULL)
        {
        pIf->pAuthKeys = pAuth->pNext;
        free (pAuth);
        pAuth = pIf->pAuthKeys;
        }
#endif /* RIP_MD5 */

    free (pIf); 

    return (OK);
    }

/***************************************************************************
*
* ripIfExcludeListAdd - Add an interface to the RIP exclusion list
*
* This function adds the interface specified by <ifName> to a list of
* interfaces on which RIP will not be started. This can be used to 
* prevent RIP from starting on an interface.
*
* RETURNS: OK if the interface was successfully added to the list;
*          ERROR otherwise.
*
* NOTE: This command must be issued prior to the interface being added
*       to the system, as RIP starts on an interface, unless it has been
*       excluded, as soon as an interface comes up.
*       If RIP was already running on the interface which is now desired
*       to be excluded from RIP, the ripIfReset command should be used 
*       after the ripIfExcludeListAdd command.
*/

STATUS ripIfExcludeListAdd
    (
    char * 	pIfName 	/* name of interface to be excluded */
    )
    {

    RT_IFLIST *		pIfList;
    int 			slen; 

    if (pIfName == NULL)
        return (ERROR);

    slen = strlen (pIfName);
    if (slen == 0 || slen >= IFNAMSIZ)
        return (ERROR);

    pIfList = (RT_IFLIST *)malloc (sizeof (RT_IFLIST));
    if (pIfList == NULL)
        return (ERROR);

    strcpy (pIfList->rtif_name, pIfName);

    /* 
     * Insert the interface in the list of interfaces on which RIP is not
     * to be started
     */

    lstAdd (&ripIfExcludeList, &(pIfList->node)); 

    return (OK);
    }

/***************************************************************************
*
* ripIfExcludeListDelete - Delete an interface from RIP exclusion list
*
* This function deletes the interface specified by <ifName> from the list of
* interfaces on which RIP will not be started. That is, RIP will start on the
* interface when it is added or comes up.
*
* RETURNS: OK if the interface was successfully removed from the list;
*          ERROR otherwise.
*
* NOTE: RIP will not automatically start on the interface. The ripIfSearch() 
*       call will need to be made after this call to cause RIP to start on
*       this interface.
*
*/

STATUS ripIfExcludeListDelete
    (
    char * 	pIfName 	/* name of un-excluded interface */
    )
    {

    RT_IFLIST *		pIfList;
    int 			slen; 

    if (pIfName == NULL)
        return (ERROR);

    slen = strlen (pIfName);
    if (slen == 0 || slen >= IFNAMSIZ)
        return (ERROR);

    for (pIfList = (RT_IFLIST *)lstFirst (&ripIfExcludeList);
         pIfList != NULL; pIfList = (RT_IFLIST *)lstNext (&pIfList->node))
        {
        if (slen == strlen (pIfList->rtif_name) && 
            strcmp (pIfName, pIfList->rtif_name) == 0) 
            {
            /* 
             * Delete the interface from the list of interfaces on 
             * which RIP is not to be started
             */

            lstDelete (&ripIfExcludeList, &pIfList->node);
            free ((char *)pIfList);
            return (OK);
            }
        }

    return (ERROR);
    }

/***************************************************************************
*
* ripIfExcludeListCheck - Check if an interface is on the RIP exclusion list
*
* This function checks if the interface specified by <ifName> is on the
* RIP exclusion list.
*
* RETURNS: OK if the interface exists on the list; ERROR otherwise.
*
* NOMANUAL
*/

STATUS ripIfExcludeListCheck
    (
    char * 	pIfName 	/* name of interface to check*/
    )
    {

    RT_IFLIST *		pIfList;
    int 			slen; 

    if (pIfName == NULL)
        return (ERROR);

    slen = strlen (pIfName);
    if (slen == 0 || slen >= IFNAMSIZ)
        return (ERROR);

    for (pIfList = (RT_IFLIST *)lstFirst (&ripIfExcludeList);
         pIfList != NULL; pIfList = (RT_IFLIST *)lstNext (&pIfList->node))
        {
        if (slen == strlen (pIfList->rtif_name) && 
            strcmp (pIfName, pIfList->rtif_name) == 0) 
            {
            return (OK);
            }
        }

    return (ERROR);
    }


/***************************************************************************
*
* ripIfExcludeListShow - Show the RIP interface exclusion list
*
* This function prints out the interfaces on which RIP will not be started.
*
* RETURNS: Nothing
*
*/

void ripIfExcludeListShow (void)
    {

    RT_IFLIST *		pIfList;

    printf ("List of interfaces on which RIP will not be started:\n");

    for (pIfList = (RT_IFLIST *)lstFirst (&ripIfExcludeList);
         pIfList != NULL; pIfList = (RT_IFLIST *)lstNext (&pIfList->node))
        printf ("%s\n", pIfList->rtif_name);

    printf ("\n");
    }

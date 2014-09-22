/* distLib - distributed objects initialization and control library (VxFusion option) */

/* Copyright 1999 - 2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01q,29oct01,jws  fix manual page entries (SPR 71239)
01p,04oct01,jws  final fixes for SPR 34727
01o,27sep01,p_r  Fixes for SPR#34727
01n,11jun99,drm  Updating documentation to reflect new piggy-backing default.
01m,07jun99,drm  Updating documentation for distCtl().
01l,24may99,drm  added vxfusion prefix to VxFusion related includes
01k,23feb99,wlf  doc edits
01j,18feb99,wlf  doc cleanup
01i,17feb99,drm  fixed spelling errors in man pages
01h,29oct98,drm  added code to initialize TBuf library after adapter init
01g,27oct98,drm  modified documentation
01f,11sep98,drm  added distDump() function (from distStatLib.c)
01e,09may98,ur   v1.00.6: removed 8 bit node id restriction
01d,20may98,ur   v1.00.5: changes according to Dave Mikulin's remarks
01c,08apr98,ur   added documentation to distCtl() (changes in distNodeCtl())
                 version update: "dicoo version 1.00.4"
01b,20jan98,ur   changed order in initializing layers, added network startup
01a,10jun97,ur   written.
*/

/*
DESCRIPTION
This library provides an initialization and control interface for VxFusion.

Use distInit() to initialize VxFusion on the current node.  In addition to 
performing local initialization, distInit() attempts to locate remote VxFusion
nodes on the network and download copies of the databases from one of the 
remote nodes.

Call distCtl() to set VxFusion run-time parameters using an ioctl()-like
syntax.

NOTE: In this release, the distInit() routine is called automatically with 
default parameters when a target boots using a VxWorks image with VxFusion 
installed.

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.

INCLUDE FILES: distLib.h
*/

#include "vxWorks.h"
#include "stdio.h"
#include "ctype.h"
#include "errnoLib.h"
#include "msgQLib.h"
#include "intLib.h" 
#include "logLib.h"
#include "private/kernelLibP.h"
#include "private/windLibP.h"
#include "vxfusion/msgQDistLib.h"
#include "vxfusion/msgQDistGrpLib.h"
#include "vxfusion/distLib.h"
#include "vxfusion/distNameLib.h"
#include "vxfusion/distStatLib.h"
#include "vxfusion/private/msgQDistGrpLibP.h"
#include "vxfusion/private/distIncoLibP.h"
#include "vxfusion/private/distNodeLibP.h"
#include "vxfusion/private/distNetLibP.h"
#include "vxfusion/private/distStatLibP.h"


LOCAL FUNCPTR    distLogHook = NULL;
LOCAL FUNCPTR    distPanicHook = NULL;

/***************************************************************************
*
* distLibInit - initialize distLib module (VxFusion option)
*
* This routine currently does nothing.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* 
* NOMANUAL
*/

void distLibInit (void)
    {
    }

/***************************************************************************
*
* distInit - initialize and bootstrap the current node (VxFusion option)
*
* This routine initializes VxFusion on the current node.  The routine begins by
* initializing the local databases and other internal services.  As part of this
* process, the current node is given the address specified by the <myNodeId>
* argument.  
*
* Secondly, this routine links a network driver to the stack by
* calling the interface adapter initialization routine specified by the
* <ifInitRtn> argument.  If the interface adapter initialization is successful,
* this routine then initializes the telegram buffer library which is needed
* for manipulating telegram buffers--the buffers that hold the packets
* sent between nodes.
*
* Thirdly, this routine attempts to determine what other VxFusion nodes are
* active on the network.  This is done by continually sending a `BOOTSTRAP'
* telegram, which indicates to other nodes that VxFusion is starting up on this
* node.  Nodes that receive a `BOOTSTRAP' telegram answer by sending an `XACK'
* telegram.  The `XACK' telegram contains information about the remote node.
* The sender of the first `XACK' received is the godfather for the current node.
* The purpose of the godfather is to update local databases.  If
* no `XACK' is received within the amount of time specified by the <waitNTicks>
* argument, it is assumed that this node is the first node to come up on the
* network.
*
* As soon as a godfather is located or it is assumed that a node sending an
* `XACK' is the first to do so on the network, the state of the node shifts
* from the <booting> state to the <network> state.  In the
* network state, all packets are sent using reliable communication
* channels; therefore all packets must be now acknowledged by the
* receiver(s).
* 
* If a godfather has been located, the current node asks it to
* update the local databases by sending an INCO_REQ packet.  The godfather 
* then begins updating the local databases.  When the godfather finishes 
* the update, it sends an INCO_DONE packet to the node being updated.
* 
* Once the database updates have completed, the node moves into the 
* <operational> state and broadcasts an INCO_UPNOW packet.
*
* The number of telegram buffers pre-allocated is equal to 2^<maxTBufsLog2>.
*
* Up to 2^<maxNodesLog2> nodes can be handled by the node database.
*
* The number of distributed message queues is limited to 2^<maxQueuesLog2>.
*
* Distributed message queue groups may not exceed 2^<maxGroupsLog2> groups.
*
* The distributed name database can work with up to 2^<maxNamesLog2> entries.
*
* EXAMPLE:
* \cs
* -> distInit (0x930b2610, distIfUdpInit, "ln0", 9, 5, 7, 6, 8, (4*sysClkRateGet())
* \ce
* This command sets the ID of the local node to 0x930b2610 (147.11.38.16). 
* The distIfUdpInit() routine is called to initialize the interface
* adapter (in this case, a UDP adapter). The UDP adapter requires a pointer to 
* the hardware interface name as configuration data (in this case, "ln0").  
* When starting up, 512 (2^9) telegram buffers are pre-allocated.
* The node database is configured to hold as many as 32 (2^5) nodes, including
* the current node.  128 (2^7) distributed message queues can be created on 
* the local node.  The local group database can hold up to 64 (2^6) groups, 
* while the name database is limited to 256 (2^8) entries.
*
* When the node bootstraps, it waits for 4 seconds (4*sysClkRateGet()) 
* to allow other nodes to respond.
*
* NOTE: This routine is called automatically with default parameters when a 
* target boots using a VxWorks image with VxFusion installed.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, or ERROR if the initialization fails.
*
* SEE ALSO: distLib
*/

STATUS distInit
    (
    DIST_NODE_ID  myNodeId,         /* node ID of this node              */
    FUNCPTR       ifInitRtn,        /* interface adapter init routine    */
    void *        pIfInitConf,      /* ptr to interface configuration    */
    int           maxTBufsLog2,     /* max number of telegram buffers    */
    int           maxNodesLog2,     /* max number of nodes in node db    */
    int           maxQueuesLog2,    /* max number of queues on this node */
    int           maxGroupsLog2,    /* max number of groups in db        */
    int           maxNamesLog2,     /* max bindings in name db           */
    int           waitNTicks        /* wait n ticks when bootstrapping   */
    )
    {
    FUNCPTR    startup;
#ifdef DIST_DIAGNOSTIC
    static char distCommObjectsVersionString[] = "VxFusion version 1.1";

    distLog ("distInit: %s\n", distCommObjectsVersionString);
#endif

    distNodeLocalSetId (myNodeId);

    distStatInit();

    /* INITIALIZE INTERFACE LAYER (layer 2) */

    if (((* ifInitRtn) (pIfInitConf, &startup)) == ERROR)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distInit: distIfInit() failed\n");
#endif
        return (ERROR);
        }

#ifdef DIST_DIAGNOSTIC
    distLog ("distInit: initialisation interface layer: done.\n");
#endif

    if (distTBufLibInit (1 << maxTBufsLog2) == ERROR)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distIfUdpInit()  - initialization of TBufs failed\n");
#endif
        return (ERROR);
        }

#ifdef DIST_DIAGNOSTIC
    distLog ("distInit: initialization of TBufs: done.\n");
#endif


    /* INITIALIZE NETWORK LAYER (layer 3) */

    distNetInit ();

    if (distNodeInit (maxNodesLog2) == ERROR)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distInit: distNodeInit() failed\n");
#endif
        return (ERROR);
        }

#ifdef DIST_DIAGNOSTIC
    distLog ("distInit: initialisation network layer: done.\n");
#endif

    /* START INTERFACE LAYER (layer 2) */

    if (((* startup) (pIfInitConf)) == ERROR)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distInit: distIfStartup() failed\n");
#endif
        return (ERROR);
        }

#ifdef DIST_DIAGNOSTIC
    distLog ("distInit: start interface layer: done.\n");
#endif

    /* INITIALIZE SERVICE LAYER (layer 4) */

    /* distributed message queue service  */

    if (msgQDistInit (maxQueuesLog2) == ERROR)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distInit: msgQDistInit() failed\n");
#endif
        return (ERROR);
        }

    /* group agreement protocol (GAP) service */

    if (distGapLibInit() == ERROR)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distInit: distGapLibInit() failed\n");
#endif
        return (ERROR);
        }

    /* distributed message queue group service */

    if (msgQDistGrpInit (maxGroupsLog2) == ERROR)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distInit: msgQDistGrpLibInit() failed\n");
#endif
        return (ERROR);
        }

    /* name database service */

    if (distNameInit (maxNamesLog2) == ERROR)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distInit: distNameInit() failed\n");
#endif
        return (ERROR);
        }

    /* incorporation service */

    if (distIncoInit() == ERROR)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distInit: distIncoInit() failed\n");
#endif
        return (ERROR);
        }

#ifdef DIST_DIAGNOSTIC
    distLog ("distInit: initialisation service layer: done.\n");
#endif

    /* GET INCORPORATED */

    /* Bootstrap and update databases of service layer (layer 4) */

#ifdef DIST_DIAGNOSTIC
    distLog ("distInit: starting incorporation phase.\n");
#endif
    if (distIncoStart(waitNTicks) == ERROR)
        {
#ifdef DIST_DIAGNOSTIC
        distLog ("distInit: distIncoStart() failed\n");
#endif
        return (ERROR);
        }

#ifdef DIST_DIAGNOSTIC
    distLog ("distInit: incorporation done--ok\n");
#endif

    return (OK);
    }

/***************************************************************************
*
* distCtl - perform a distributed objects control function (VxFusion option)
*
* This routine sets various parameters and hooks that control the system.  It
* uses a syntax similar to that of the ioctl() routine.
* It accepts the following functions:
* \is
* \i DIST_CTL_LOG_HOOK
* This function sets a routine to be called each time a log message is 
* produced. By default, the log hook writes the message to standard output.
* The prototype of the log() routine should look like this:
* \cs
*    void log (char *logMsg);
* \ce
* \i DIST_CTL_PANIC_HOOK
* This function sets a routine to be called when the system panics. 
* By default, the panic hook writes the panic message to standard output. 
* The panic() routine must not return.  The prototype of the panic() routine 
* should look like this:
* \cs
*    void panic (char *panicMsg);
* \ce
* \i DIST_CTL_RETRY_TIMEOUT
* This function sets the initial send retry timeout in clock ticks. If no 
* ACK is received within a timeout period, the packet is resent. The default
* value and granularity of DIST_CTL_RETRY_TIMEOUT is system dependent.
* \ts
* vxWorks Version | Default Value | Granularity
* -----------------| ---------------| -----------
* 5.4 and below | 1000ms | 500ms
* 5.5 and AE | 200ms | 100ms
* \te
* DIST_CTL_RETRY_TIMEOUT is designated in ticks, but rounded down to a multiple
* of the system's granularity.  The timeout period for the <n>th send is: 
* 
*   <n> * DIST_CTL_RETRY_TIMEOUT
* 
* \i DIST_CTL_MAX_RETRIES
* This function sets a limit for the number of retries when sending fails.
* The default value is system dependent, but is set to 5 for all current
* versions of vxWorks.
*
* \i DIST_CTL_NACK_SUPPORT
* This function enables or disables the sending of negative acknowledgments 
* (NACKs).  NACKs are used to request a resend of a single
* missing fragment from a packet. They are sent immediately after a fragment
* is found to be missing.  If <arg> is FALSE (0), the sending of negative
* acknowledgments is disabled.  If <arg> is TRUE (1), the sending of NACKs
* is enabled. By default, NACKs are enabled.
*
* \i DIST_CTL_PGGYBAK_UNICST_SUPPORT
* This function enables or disables unicast piggy-backing.  When unicast 
* piggy-backing is enabled, the system waits some time until it sends 
* an acknowledgment for a previously received packet. In the meantime, if a
* data packet is sent to a host already awaiting an acknowledgment, the
* acknowledgment is delivered (that is, piggy-backed) with the data packet.
* Enabling piggy-backing is useful for reducing the number of packets sent;
* however, it increases latency if no data packets are sent while the system 
* waits.  When unicast piggy-backing is disabled, an acknowledgment is 
* delivered immediately in its own packet.  This function turns piggy-backing 
* on and off for unicast communication only.  If <arg> is FALSE (0), unicast 
* piggy-backing is disabled.  If <arg> is TRUE (1), unicast piggy-backing is 
* enabled.  By default, piggy-backing is disabled for unicast communication.
*
* \i DIST_CTL_PGGYBAK_BRDCST_SUPPORT
* This function enables or disables broadcast piggy-backing.  When broadcast
* piggy-backing is enabled, the system waits some time until it 
* sends an acknowledgment for a previously received packet.  In the meantime,
* if a data packet is sent to a host already awaiting an acknowledgment, the
* acknowledgment is delivered (that is, piggy-backed)with the data packet.
* Enabling piggy-backing is useful for reducing the number of packets sent;
* however, it increases latency if no broadcast data packets are sent while the
* system waits.  When broadcast piggy-backing is disabled, an 
* acknowledgment is delivered immediately in its own packet.  This function 
* turns piggy-backing on and off for broadcast communication only.  If <arg> is 
* FALSE (0), broadcast piggy-backing is disabled.  If <arg> is TRUE (1), 
* broadcast piggy-backing is enabled.  By default, piggy-backing is disabled 
* for broadcast communication.
*
* \i DIST_CTL_OPERATIONAL_HOOK
* This function adds a routine to a list of routines to be called each time a 
* node shifts to the operational state.  A maximum of 8 routines can be added to
* the list.  The prototype of each operational() routine should look as 
* follows:
* \cs
*    void operational (DIST_NODE_ID nodeStateChanged);
* \ce
* \i DIST_CTL_CRASHED_HOOK
* This function adds a routine to a list of routines to be called each time a 
* node shifts to the crashed state.  A node shifts to the crashed state when 
* it does not acknowledge a message within the maximum number of retries.  
* The list can contain a maximum of 8 routines; however VxFusion supplies one 
* routine, leaving room for only 7 user-supplied routines.  The prototype of 
* each crashed() routine should look as follows:
* \cs
*    void crashed (DIST_NODE_ID nodeStateChanged);
* \ce
* \i DIST_CTL_GET_LOCAL_ID
* This function returns the local node ID.
*
* \i DIST_CTL_GET_LOCAL_STATE
* This function returns the state of the local node.
*
* \i DIST_CTL_SERVICE_HOOK
* This function sets a routine to be called each time a service fails, for a 
* service invoked by a remote node. The <argument> parameter is a pointer to
* a servError() routine.  The prototype of the servError() routine should
* look as follows:
* \cs
*    void servError (int servId, int status);
* \ce
* The system is aware of the following services:
* \cs
* DIST_ID_MSG_Q_SERV     (0)  /@ message queue service       @/
* DIST_ID_MSG_Q_GRP_SERV (1)  /@ message queue group service @/
* DIST_ID_DNDB_SERV      (2)  /@ distributed name database   @/
* DIST_ID_DGDB_SERV      (3)  /@ distributed group database  @/
* DIST_ID_INCO_SERV      (4)  /@ incorporation protocol      @/
* DIST_ID_GAP_SERV       (5)  /@ group agreement protocol    @/
* \ce
* \i DIST_CTL_SERVICE_CONF
* This function configures a specified service. The <argument> parameter is
* a pointer to a DIST_SERV_CONF structure which holds the service ID and its
* configuration to be set.  DIST_SERV_CONF is defined as follows:
* \cs
* typedef struct
*     {
*     int servId;     /@ ID of service to configure  @/
*     int taskPrio;   /@ priority of service task    @/
*     int netPrio;    /@ network priority of service @/
*     } DIST_SERV_CONF;
* \ce
* The system is aware of the following services:
* \cs
* DIST_ID_MSG_Q_SERV     (0)  /@ message queue service       @/
* DIST_ID_MSG_Q_GRP_SERV (1)  /@ message queue group service @/
* DIST_ID_DNDB_SERV      (2)  /@ distributed name database   @/
* DIST_ID_DGDB_SERV      (3)  /@ distributed group database  @/
* DIST_ID_INCO_SERV      (4)  /@ incorporation protocol      @/
* DIST_ID_GAP_SERV       (5)  /@ group agreement protocol    @/
* \ce
* If one of the configuration parameters is -1, it remains unchanged.
* The parameter <taskPrio> can range from 0 to 255; <netPrio> can range from 
* 0 to 7.
*
* A service's configuration can be changed at any time.
* \ie
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK or the value requested if <function> is known;
* ERROR if <function> is unknown or the argument is invalid.
*
* ERRNO
* \is
* \i S_distLib_UNKNOWN_REQUEST
* The control function is unknown.
* \ie
*
*/

int distCtl
    (
    int function,        /* function code      */
    int argument         /* arbitrary argument */
    )
    {
    switch (function & DIST_CTL_TYPE_MASK)
        {
        case DIST_CTL_TYPE_DIST:
            {
            switch (function)
                {
                case DIST_CTL_LOG_HOOK:
                    distLogHook = (FUNCPTR) argument;
                    return (OK);
                case DIST_CTL_PANIC_HOOK:
                    distPanicHook = (FUNCPTR) argument;
                    return (OK);
                default:
                    errnoSet (S_distLib_UNKNOWN_REQUEST);
                    return (ERROR);
                }
            }
        case DIST_CTL_TYPE_NODE:
            return (distNodeCtl (function, argument));
        case DIST_CTL_TYPE_NET:
            return (distNetCtl (function, argument));
        default:
            errnoSet (S_distLib_UNKNOWN_REQUEST);
            return (ERROR);
        }
    }


/***************************************************************************
*
* distPanic - halt VxFusion task or VxWorks (VxFusion option)
*
* This routine is called when an unrecoverable error is encountered.
* It will suspend the calling task.  If called from interrupt level,
* it will effectively stop VxWorks.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void distPanic
    (
    char * fmt,   /* output format spec */
    ...           /* varargs */
    )
    {
    va_list ap;
    char szPanic[160];
 
    va_start (ap, fmt);
    vsprintf ((char *) &szPanic, fmt, ap);
    va_end (ap);
 
    if (distPanicHook == NULL)
        {
        /* Check if context is root Task or interrupt or in kernel State */
        /* If it is then loop else Suspend calling task */
 
        if ((INT_CONTEXT ()) ||
            (kernelState != FALSE) ||
            ((int)taskIdCurrent == rootTaskId))
            {
            logMsg ("distPanic (system Halted): %s",(int)&szPanic,0,0,0,0,0);
            while (1);
            }
        else
            {
            logMsg ("distPanic: Suspending Task ID %ud: %s", taskIdSelf (),
                   (int)&szPanic,0,0,0,0);
            taskSuspend (0);
            }
        }
    else
        (* distPanicHook) ((char *) &szPanic);

    }

/***************************************************************************
*
* distLog - log a message (VxFusion option)
*
* This routine is used internally to log VxFusion messages.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void distLog
    (
    char * fmt,   /* output format spec */
    ...           /* varargs */
    )
    {
    va_list ap;
    char    szLog[160];

    va_start (ap, fmt);
    if (distLogHook == NULL)
    {
    printf ("log: ");
    vprintf (fmt, ap);
    }
    else
    {
    vsprintf (szLog, fmt, ap);
    (* distLogHook) (szLog);
    }
    va_end (ap);
    }


/***************************************************************************
*
* distDump - hexdump (VxFusion option)
*
* This routine prints a sequence of bytes as hex characters.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: N/A
* 
* NOMANUAL
*/

void distDump
    (
    char * buffer,   /* points to bytes to dump as hex */
    int    len       /* number of bytes to dump */
    )
    {
    int i;

    for (i=0; i<len; i++)
        {
        printf ("%02x ", (unsigned char) buffer[i]);
        if (! ((i + 1) % 16))
            printf ("\n");
        }
    if (len % 16)
        printf ("\n");
    }



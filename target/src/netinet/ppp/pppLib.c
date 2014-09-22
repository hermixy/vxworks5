/* pppLib.c - Point-to-Point Protocol library */

/* Copyright 1995 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
modification history
--------------------
01x,04dec00,adb  enabled PPP_HOOK_DISCONNECT in pppDelete
01w,23oct00,cn   Fix for spr 34068, cleanup of clients' addresses structures.
01y,19oct00,cn   Fix for spr 34657, cleanup of callout structures.
01v,17feb99,sgv  Fix for spr 24560, proper cleanup when ppp options file 
		 cannot be accessed
01u,31aug98,fle  doc : got rid of simple quotes to avoid bold troubles
01t,15aug97,sgv  fixed spr #9109. In cleanup routine, ppp_if[unit] 
		 is checked for NULL memory address before freeing it
01s,10dec96,vin  fixed problem with die(..) if OPT_SILENT or OPT_PASSIVE
		 is specified. Made changes to term(..) so that pppDelete()
		 would work even if silent or passive options are specified.
		 fixed SPR 7604.
01r,15nov96,vin  fixed problem with OPT_DRIVER_DEBUG, establish_ppp() 
		 should be called after the options are set. The only
		 dependency is that pppopen() should be called before
		 establish_ppp().
01q,19dec95,vin  doc tweaks.
01p,30nov95,vin  added hooks on a unit basis.
01o,11jul95,dzb  doc tweaks.
01n,22jun95,dzb  changed [un]timeout() to ppp_[un]timeout().
                 removed usehostname option.  Enhanced doc.
01m,12jun95,dzb  added MIBII counter for unknown protocol packets.
                 changed [dis]connect strings to global hook routines.
                 header file consolidation.  removed device_script().
		 removed perror() reference.
01l,08jun95,dzb  made LOG_NOTICE message printed even w/o debug option.
01k,16may95,dzb  put back in login option doc.
01j,09may95,dzb  added callout to pppSecretLibInit().
                 switched all lcp_close() calls to go through die().
01i,03may95,dzb  doc tweaks: removed login option, fixed require_chap_file,
                 added pap_passwd option.
01h,07mar95,dzb  changed cfree() to free() (ANSI).  Changed name to pppLib.c.
                 additional doc/formatting.
01g,13feb95,dab  fixed pppDelete() for multi unit support (SPR #4062).
01f,09feb95,dzb  fixed proto tbl input call for multi unit support (SPR #4062).
01e,07feb95,dab  changed syslog to MAINDEBUG for reg status messages.
            dzb  changed to look at (errno != EINTR) after sigsuspend().
01d,24jan95,dzb  ifdef VXW5_1 for timer_create() backwards capatibility.
                 renamed params for pppInit() and ppp_task().
01c,13jan95,dab  enhanced doc.
01b,22dec94,dzb  tree shuffle and warning cleanup.
01a,21dec94,dab  VxWorks port - first WRS version.
	   +dzb  added: path for ppp header files, WRS copyright.
*/

/*
DESCRIPTION
This library implements the VxWorks Point-to-Point Protocol (PPP)
facility.  PPP allows VxWorks to communicate with other machines by sending
encapsulated multi-protocol datagrams over a point-to-point serial link.
VxWorks may have up to 16 PPP interfaces active at any one time.  Each
individual interface (or "unit") operates independent of the state of
other PPP units.

USER-CALLABLE ROUTINES
PPP network interfaces are initialized using the pppInit() routine. This 
routine's parameters specify the unit number, the name of the serial 
interface (<tty>) device, Internet (IP) addresses for both ends of the link,
the interface baud rate, an optional pointer to a configuration options 
structure, and an optional pointer to a configuration options file.  
The pppDelete() routine deletes a specified PPP interface.

DATA ENCAPSULATION
PPP uses HDLC-like framing, in which five header and three trailer octets are
used to encapsulate each datagram.  In environments where bandwidth is at
a premium, the total encapsulation may be shortened to four octets with
the available address/control and protocol field compression options.

LINK CONTROL PROTOCOL
PPP incorporates a link-layer protocol called Link Control Protocol (LCP),
which is responsible for the link set up, configuration, and termination.
LCP provides for automatic negotiation of several link options, including
datagram encapsulation format, user authentication, and link monitoring
(LCP echo request/reply).

NETWORK CONTROL PROTOCOLS
PPP's Network Control Protocols (NCP) allow PPP to support 
different network protocols.  VxWorks supports only one NCP, the Internet
Protocol Control Protocol (IPCP), which allows the establishment and
configuration of IP over PPP links.  IPCP supports the negotiation of IP
addresses and TCP/IP header compression (commonly called "VJ" compression).

AUTHENTICATION
The VxWorks PPP implementation supports two separate user authentication
protocols: the Password Authentication Protocol (PAP) and the
Challenge-Handshake Authentication Protocol (CHAP).  While PAP only
authenticates at the time of link establishment, CHAP may be configured to
periodically require authentication throughout the life of the link.
Both protocols are independent of one another, and either may be configured
in through the PPP options structure or options file.

IMPLEMENTATION
Each VxWorks PPP interface is handled by two tasks: the daemon task
(tPPP<unit>) and the write task (tPPP<unit>Wrt).

The daemon task controls the various PPP control protocols (LCP, IPCP, CHAP,
and PAP).  Each PPP interface has its own daemon task that handles link
set up, negotiation of link options, link-layer user athentication, and
link termination.  The daemon task is not used for the actual sending and
receiving of IP datagrams.

The write task controls the transmit end of a PPP driver interface.
Each PPP interface has its own write task that handles the actual sending
of a packet by writing data to the <tty> device.  Whenever a packet is ready
to be sent out, the PPP driver activates this task by giving a semaphore.
The write task then completes the packet framing and writes
the packet data to the <tty> device.

The receive end of the PPP interface is implemented as a "hook" into
the <tty> device driver.  The <tty> driver's receive interrupt service routine
(ISR) calls the PPP driver's ISR every time a character is received on the
serial channel.  When the correct PPP framing character sequence is received,
the PPP ISR schedules the tNetTask task to call the PPP input routine.
The PPP input routine reads a whole PPP packet out of the <tty> ring
buffer and processes it according to PPP framing rules.
The packet is then queued either to the IP input queue or to the PPP daemon
task input queue.

INCLUDE FILES: pppLib.h

SEE ALSO: ifLib, tyLib, pppSecretLib, pppShow,
.pG "Network"
, 
.I "RFC-1332: The PPP Internet Protocol Control Protocol (IPCP)", 
.I "RFC-1334: PPP Authentication Protocols", 
.I "RFC-1548: The Point-to-Point Protocol (PPP)", 
.I "RFC-1549: PPP in HDLC Framing"

ACKNOWLEDGEMENT
This program is based on original work done by Paul Mackerras of
Australian National University, Brad Parker, Greg Christy, Drew D. Perkins,
Rick Adams, and Chris Torek.

INTERNAL

         pppInit               pppDelete 
            |                     |
            |                     |
            |                     |
         pppopen   pppwrite    pppclose   pppintr   pppread   pppioctl
           /\          |          |          |               
          /  \         |          |          |              
         /    \    pppoutput      |          |             
        /      \       |          |          |            
       /        \      |          |          |           
  pppalloc pppWrtTask <-      pppdealloc ppp_tty_read 
      |                                      |           
      |                                      |           
      |                                      |           
  pppattach                               pppinput
                                             |
                                             |
                                             |
                                          ppppktin
*/

#include "vxWorks.h"
#include "string.h"
#include "sigLib.h"
#include "sysLib.h"
#include "sockLib.h"
#include "ioLib.h"
#include "iosLib.h"
#include "excLib.h"
#include "time.h"
#include "timers.h"
#include "taskLib.h"
#include "taskVarLib.h"
#include "hostLib.h"
#include "sys/ioctl.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "stdio.h"
#include "signal.h"
#include "errno.h"
#include "fcntl.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/socket.h"
#include "sys/times.h"
#include "net/if.h"
#include "netLib.h"
#include "pppLib.h"

/* PPP parameter structure */
typedef struct ppp_parms
{
	char *task_name;
        char *devname;
        char *local_addr;
        char *remote_addr;
        char *filename;
        int unit;
        int baud;
        PPP_OPTIONS options;
} PPP_PARMS;

/* globals */

int ppp_unit;				/* PPP unit id */
PPP_TASK_VARS *ppp_if[NPPP];		/* per task PPP variables */
PPP_PARMS *ppp_parms[NPPP];		/* PPP parameters */
PPP_HOOK_RTNS * pppHookRtns [NPPP];	/* PPP table of hooks */

int ppp_task_priority		= 55;
int ppp_task_options		= VX_FP_TASK | VX_DEALLOC_STACK;
int ppp_task_stack_size		= 0x4000;
char *ppp_task_name		= "tPPP";

/* prototypes */

static int  baud_rate_of __ARGS((int));
static int  translate_speed __ARGS((int));
static int  set_up_tty __ARGS((int));
static void hup __ARGS((int));
static void intr __ARGS((int));
static void term __ARGS((int));
static void alrm __ARGS((int));
static void io __ARGS(());
static void incdebug __ARGS((int));
static void nodebug __ARGS((int));
static void cleanup __ARGS((int, int, caddr_t));

/*
 * PPP Data Link Layer "protocol" table.
 * One entry per supported protocol.
 */
static struct protent {
    u_short protocol;
    void (*init)();
    void (*input)();
    void (*protrej)();
    int  (*printpkt)();
    char *name;
} prottbl[] = {
    { LCP, lcp_init, lcp_input, lcp_protrej, lcp_printpkt, "LCP" },
    { IPCP, ipcp_init, ipcp_input, ipcp_protrej, ipcp_printpkt, "IPCP" },
    { UPAP, upap_init, upap_input, upap_protrej, upap_printpkt, "PAP" },
    { CHAP, ChapInit, ChapInput, ChapProtocolReject, ChapPrintPkt, "CHAP" },
};

#define N_PROTO         (sizeof(prottbl) / sizeof(prottbl[0]))

/*******************************************************************************
*
* pppInit - initialize a PPP network interface
*
* This routine initializes a Point-to-Point Protocol (PPP) network interface.
* The parameters to this routine specify the unit number (<unit>) of the
* PPP interface, the name of the serial interface (<tty>) device (<devname>),
* the IP addresses of the local and remote ends of the link, the interface
* baud rate, an optional configuration options structure pointer, and an
* optional configuration options file name.
*
* IP ADDRESSES:
* The <local_addr> and <remote_addr> parameters specify the IP addresses
* of the local and remote ends of the PPP link, respectively.
* If <local_addr> is NULL, the local IP address will be negotiated with
* the remote peer.  If the remote peer does not assign a local IP address,
* it will default to the address associated with the local target's machine
* name.  If <remote_addr> is NULL, the remote peer's IP address will obtained
* from the remote peer.  A routing table entry to the remote peer will be
* automatically added once the PPP link is established.
*
* CONFIGURATION OPTIONS STRUCTURE:
* The optional parameter <pOptions> specifies configuration options for
* the PPP link.  If NULL, this parameter is ignored, otherwise it
* is assumed to be a pointer to a PPP_OPTIONS options structure (defined
* in h/netinet/ppp/options.h).
*
* The "flags" member of the PPP_OPTIONS structure is a bit-mask, where the
* following bit-flags may be specified:
* .iP "OPT_NO_ALL"
* Do not request/allow any options.
* .iP "OPT_PASSIVE_MODE"
* Set passive mode.
* .iP "OPT_SILENT_MODE"
* Set silent mode.
* .iP "OPT_DEFAULTROUTE"
* Add default route.
* .iP "OPT_PROXYARP"
* Add proxy ARP entry.
* .iP "OPT_IPCP_ACCEPT_LOCAL"
* Accept peer's idea of the local IP address.
* .iP "OPT_IPCP_ACCEPT_REMOTE"
* Accept peer's idea of the remote IP address.
* .iP "OPT_NO_IP"
* Disable IP address negotiation.
* .iP "OPT_NO_ACC"
* Disable address/control compression.
* .iP "OPT_NO_PC"
* Disable protocol field compression.
* .iP "OPT_NO_VJ"
* Disable VJ (Van Jacobson) compression.
* .iP "OPT_NO_VJCCOMP"
* Disable VJ (Van Jacobson) connnection ID compression.
* .iP "OPT_NO_ASYNCMAP"
* Disable async map negotiation.
* .iP "OPT_NO_MN"
* Disable magic number negotiation.
* .iP "OPT_NO_MRU"
* Disable MRU (Maximum Receive Unit) negotiation.
* .iP "OPT_NO_PAP"
* Do not allow PAP authentication with peer.
* .iP "OPT_NO_CHAP"
* Do not allow CHAP authentication with peer.
* .iP "OPT_REQUIRE_PAP"
* Require PAP authentication with peer.
* .iP "OPT_REQUIRE_CHAP"
* Require CHAP authentication with peer.
* .iP "OPT_LOGIN"
* Use the login password database for PAP authentication of peer.
* .iP "OPT_DEBUG"
* Enable PPP daemon debug mode.
* .iP "OPT_DRIVER_DEBUG"
* Enable PPP driver debug mode.
* .LP
*
* The remaining members of the PPP_OPTIONS structure specify PPP configurations
* options that require string values.  These options are:
* .iP "char *asyncmap"
* Set the desired async map to the specified string.
* .iP "char *escape_chars"
* Set the chars to escape on transmission to the specified string.
* .iP "char *vj_max_slots"
* Set maximum number of VJ compression header slots to the specified string.
* .iP "char *netmask"
* Set netmask value for negotiation to the specified string.
* .iP "char *mru"
* Set MRU value for negotiation to the specified string.
* .iP "char *mtu"
* Set MTU (Maximum Transmission Unit) value for negotiation to the
* specified string.
* .iP "char *lcp_echo_failure"
* Set the maximum number of consecutive LCP echo failures to the specified
* string.
* .iP "char *lcp_echo_interval"
* Set the interval in seconds between LCP echo requests to the specified string.
* .iP "char *lcp_restart"
* Set the timeout in seconds for the LCP negotiation to the specified string.
* .iP "char *lcp_max_terminate"
* Set the maximum number of transmissions for LCP termination requests
* to the specified string.
* .iP "char *lcp_max_configure"
* Set the maximum number of transmissions for LCP configuration
* requests to the specified string.
* .iP "char *lcp_max_failure"
* Set the maximum number of LCP configuration NAKs to the specified string.
* .iP "char *ipcp_restart"
* Set the timeout in seconds for IPCP negotiation to the specified string.
* .iP "char *ipcp_max_terminate"
* Set the maximum number of transmissions for IPCP termination requests
* to the specified string.
* .iP "char *ipcp_max_configure"
* Set the maximum number of transmissions for IPCP configuration requests
* to the specified string.
* .iP "char *ipcp_max_failure"
* Set the maximum number of IPCP configuration NAKs to the specified string.
* .iP "char *local_auth_name"
* Set the local name for authentication to the specified string.
* .iP "char *remote_auth_name"
* Set the remote name for authentication to the specified string.
* .iP "char *pap_file"
* Get PAP secrets from the specified file.  This option is necessary
* if either peer requires PAP authentication.
* .iP "char *pap_user_name"
* Set the user name for PAP authentication with the peer to the
* specified string.
* .iP "char *pap_passwd"
* Set the password for PAP authentication with the peer to the specified string.
* .iP "char *pap_restart"
* Set the timeout in seconds for PAP negotiation to the specified string.
* .iP "char *pap_max_authreq"
* Set the maximum number of transmissions for PAP authentication
* requests to the specified string.
* .iP "char *chap_file"
* Get CHAP secrets from the specified file.  This option is necessary
* if either peer requires CHAP authentication.
* .iP "char *chap_restart"
* Set the timeout in seconds for CHAP negotiation to the specified string.
* .iP "char *chap_interval"
* Set the interval in seconds for CHAP rechallenge to the specified string.
* .iP "char *chap_max_challenge"
* Set the maximum number of transmissions for CHAP challenge to the
* specified string.
*
* CONFIGURATION OPTIONS FILE:
* The optional parameter <fOptions> specifies configuration options for
* the PPP link.  If NULL, this parameter is ignored, otherwise it
* is assumed to be the name of a configuration options file.  The format
* of the options file is one option per line; comment lines start with "#".
* The following options are recognized:
* .iP "no_all"
* Do not request/allow any options.
* .iP "passive_mode"
* Set passive mode.
* .iP "silent_mode"
* Set silent mode.
* .iP "defaultroute"
* Add default route.
* .iP "proxyarp"
* Add proxy ARP entry.
* .iP "ipcp_accept_local"
* Accept peer's idea of the local IP address.
* .iP "ipcp_accept_remote"
* Accept peer's idea of the remote IP address.
* .iP "no_ip"
* Disable IP address negotiation.
* .iP "no_acc"
* Disable address/control compression.
* .iP "no_pc"
* Disable protocol field compression.
* .iP "no_vj"
* Disable VJ (Van Jacobson) compression.
* .iP "no_vjccomp"
* Disable VJ (Van Jacobson) connnection ID compression.
* .iP "no_asyncmap"
* Disable async map negotiation.
* .iP "no_mn"
* Disable magic number negotiation.
* .iP "no_mru"
* Disable MRU (Maximum Receive Unit) negotiation.
* .iP "no_pap"
* Do not allow PAP authentication with peer.
* .iP "no_chap"
* Do not allow CHAP authentication with peer.
* .iP "require_pap"
* Require PAP authentication with peer.
* .iP "require_chap"
* Require CHAP authentication with peer.
* .iP "login"
* Use the login password database for PAP authentication of peer.
* .iP "debug"
* Enable PPP daemon debug mode.
* .iP "driver_debug"
* Enable PPP driver debug mode.
* .iP "asyncmap <value>"
* Set the desired async map to the specified value.
* .iP "escape_chars <value>"
* Set the chars to escape on transmission to the specified value.
* .iP "vj_max_slots <value>"
* Set maximum number of VJ compression header slots to the specified value.
* .iP "netmask <value>"
* Set netmask value for negotiation to the specified value.
* .iP "mru <value>"
* Set MRU value for negotiation to the specified value.
* .iP "mtu <value>"
* Set MTU value for negotiation to the specified value.
* .iP "lcp_echo_failure <value>"
* Set the maximum consecutive LCP echo failures to the specified value.
* .iP "lcp_echo_interval <value>"
* Set the interval in seconds between LCP echo requests to the specified value.
* .iP "lcp_restart <value>"
* Set the timeout in seconds for the LCP negotiation to the specified value.
* .iP "lcp_max_terminate <value>"
* Set the maximum number of transmissions for LCP termination requests
* to the specified value.
* .iP "lcp_max_configure <value>"
* Set the maximum number of transmissions for LCP configuration
* requests to the specified value.
* .iP "lcp_max_failure <value>"
* Set the maximum number of LCP configuration NAKs to the specified value.
* .iP "ipcp_restart <value>"
* Set the timeout in seconds for IPCP negotiation to the specified value.
* .iP "ipcp_max_terminate <value>"
* Set the maximum number of transmissions for IPCP termination requests
* to the specified value.
* .iP "ipcp_max_configure <value>"
* Set the maximum number of transmissions for IPCP configuration requests
* to the specified value.
* .iP "ipcp_max_failure <value>"
* Set the maximum number of IPCP configuration NAKs to the specified value.
* .iP "local_auth_name <name>"
* Set the local name for authentication to the specified name.
* .iP "remote_auth_name <name>"
* Set the remote name for authentication to the specified name.
* .iP "pap_file <file>"
* Get PAP secrets from the specified file.  This option is necessary
* if either peer requires PAP authentication.
* .iP "pap_user_name <name>"
* Set the user name for PAP authentication with the peer to the specified name.
* .iP "pap_passwd <password>
* Set the password for PAP authentication with the peer to the specified
* password.
* .iP "pap_restart <value>"
* Set the timeout in seconds for PAP negotiation to the specified value.
* .iP "pap_max_authreq <value>"
* Set the maximum number of transmissions for PAP authentication
* requests to the specified value.
* .iP "chap_file <file>"
* Get CHAP secrets from the specified file.  This option is necessary
* if either peer requires CHAP authentication.
* .iP "chap_restart <value>"
* Set the timeout in seconds for CHAP negotiation to the specified value.
* .iP "chap_interval <value>"
* Set the interval in seconds for CHAP rechallenge to the specified value.
* .iP "chap_max_challenge <value>"
* Set the maximum number of transmissions for CHAP challenge to the
* specified value.
*
* AUTHENTICATION:
* The VxWorks PPP implementation supports two separate user authentication
* protocols: the Password Authentication Protocol (PAP) and the
* Challenge-Handshake Authentication Protocol (CHAP).  If authentication is
* required by either peer, it must be satisfactorily completed before the
* PPP link becomes fully operational.  If authentication fails, the link
* will be automatically terminated.
*
* EXAMPLES:
* The following routine initializes a PPP interface that uses the
* target's second serial port (`/tyCo/1').  The local IP address is
* 90.0.0.1; the IP address of the remote peer is 90.0.0.10.  The baud
* rate is the default rate for the <tty> device.  VJ compression
* and authentication have been disabled, and LCP echo requests have been
* enabled.
*
* .CS
* PPP_OPTIONS pppOpt;	/@ PPP configuration options @/
*
* void routine ()
*     {
*     pppOpt.flags = OPT_PASSIVE_MODE | OPT_NO_PAP | OPT_NO_CHAP | OPT_NO_VJ;
*     pppOpt.lcp_echo_interval = "30";
*     pppOpt.lcp_echo_failure = "10";
*
*     pppInit (0, "/tyCo/1", "90.0.0.1", "90.0.0.10", 0, &pppOpt, NULL);
*     }
* .CE
*
* The following routine generates the same results as the previous example.
* The difference is that the configuration options are obtained from
* a file rather than a structure.
* 
* .CS
* pppFile = "phobos:/tmp/ppp_options";	/@ PPP configuration options file @/
*
* void routine ()
*     {
*     pppInit (0, "/tyCo/1", "90.0.0.1", "90.0.0.10", 0, NULL, pppFile);
*     }
* .CE
* where phobos:/tmp/ppp_options contains:
*
* .CS
*     passive
*     no_pap
*     no_chap
*     no_vj
*     lcp_echo_interval 30
*     lcp_echo_failure 10
* .CE
*
* RETURNS:
* OK, or ERROR if the PPP interface cannot be initialized because the
* daemon task cannot be spawned or memory is insufficient.
*
* SEE ALSO: pppShow, pppDelete(),
* .pG "Network"
*/

int pppInit
    (
    int unit,			/* PPP interface unit number to initialize */
    char *devname,		/* name of the <tty> device to be used */
    char *local_addr,		/* local IP address of the PPP interface */
    char *remote_addr,		/* remote peer IP address of the PPP link */
    int baud,			/* baud rate of <tty>; NULL = default */
    PPP_OPTIONS *pOptions,	/* PPP options structure pointer */
    char *fOptions		/* PPP options file name */
    )
    {
    int status;

    if (ppp_if[unit] != NULL) {
        fprintf(stderr, "PPP Unit %d already in use\r\n", unit);
        return(ERROR);
    }

    if ((ppp_parms[unit] = (PPP_PARMS *)calloc(1, sizeof(PPP_PARMS))) == NULL) {
	fprintf(stderr, "Unable to allocate memory for ppp parmeters\r\n");
	return(ERROR);
    }

    /* Save parameters in case they're local variables */
    ppp_parms[unit]->unit = unit;
    ppp_parms[unit]->baud = baud;

    if (devname)
        ppp_parms[unit]->devname = (char *)stringdup(devname);
    else
        ppp_parms[unit]->devname = (char *)stringdup("");

    if (local_addr)
        ppp_parms[unit]->local_addr = (char *)stringdup(local_addr);
    else
        ppp_parms[unit]->local_addr = (char *)stringdup("");

    if (remote_addr)
        ppp_parms[unit]->remote_addr = (char *)stringdup(remote_addr);
    else
        ppp_parms[unit]->remote_addr = (char *)stringdup("");

    if (fOptions)
        ppp_parms[unit]->filename = (char *)stringdup(fOptions);

    ppp_parms[unit]->task_name = (char *)malloc(strlen(ppp_task_name) + 2);
    sprintf(ppp_parms[unit]->task_name, "%s%x", ppp_task_name, unit);

    if (pOptions)
        bcopy((char *)pOptions, (char *)&ppp_parms[unit]->options,
	      sizeof(PPP_OPTIONS));

    status = taskSpawn(ppp_parms[unit]->task_name,
		       ppp_task_priority,
		       ppp_task_options,
                       ppp_task_stack_size, (FUNCPTR)ppp_task,
		       ppp_parms[unit]->unit,
		       (int)ppp_parms[unit]->devname,
		       (int)ppp_parms[unit]->local_addr,
		       (int)ppp_parms[unit]->remote_addr,
		       ppp_parms[unit]->baud,
		       (int)&ppp_parms[unit]->options,
		       (int)ppp_parms[unit]->filename,
		       0, 0, 0);

    if (status == ERROR)
        return(ERROR);
    else
        return(OK);
    }

/*******************************************************************************
*
* ppp_task - PPP daemon task
*
* This is the entry point for the PPP daemon task.
*
* NOMANUAL
*/

void ppp_task(unit, devname, local_addr, remote_addr, baud, pOptions, fOptions)
    int unit;
    char *devname;
    char *local_addr;
    char *remote_addr;
    int baud;
    PPP_OPTIONS *pOptions;
    char *fOptions;
{
    sigset_t mask;
    struct sigaction sa;
    int i;
    char user[MAXNAMELEN];

    if ((ppp_if[unit] = (PPP_TASK_VARS *)calloc(1, sizeof(PPP_TASK_VARS))) == NULL) {
	fprintf(stderr, "Unable to allocate memory for ppp task\r\n");
	return;
    }

    if (taskVarAdd(0, (int *)&ppp_unit) != OK) {
        fprintf(stderr, "Unable to add task variable\r\n");
	ppp_if[unit] = NULL;
        return;
    }

    /* Initialize ppp task variables */
    ppp_unit = ppp_if[unit]->ifunit = unit;
    ppp_if[unit]->debug = 0;
    ppp_if[unit]->unknownProto = 0;
    ppp_if[unit]->fd = ERROR;
    ppp_if[unit]->s = ERROR;
    ppp_if[unit]->options = pOptions;
    ppp_if[unit]->task_id = taskIdSelf();
    ppp_if[unit]->hungup = 0;
    strcpy(ppp_if[unit]->devname, devname);

#ifdef	VXW5_1
    ppp_if[unit]->timer_id = timer_create (CLOCK_REALTIME, NULL);
#else	/* VXW5_1 */
    timer_create (CLOCK_REALTIME, NULL, &ppp_if[unit]->timer_id);
#endif	/* VXW5_1 */

    if (ppp_if[unit]->timer_id < (timer_t)0) {
        fprintf(stderr, "timer_create failed\r\n");
	ppp_if[unit] = NULL;
        return;
    }

    if (timer_connect(ppp_if[unit]->timer_id, alrm, 0) < 0) {
        fprintf(stderr, "timer_connect failed\r\n");
	ppp_if[unit] = NULL;
        return;
    }

    if (gethostname(ppp_if[unit]->hostname, MAXNAMELEN) < 0 ) {
        fprintf(stderr, "couldn't get hostname\r\n");
	ppp_if[unit] = NULL;
        return;
    }
    ppp_if[unit]->hostname[MAXNAMELEN-1] = 0;

    /*
     * Open the serial device and set it up to be the ppp interface.
     */
    if (pppopen(unit, devname) == ERROR) {
        fprintf(stderr, "ppp%d: error openning the %s interface\r\n",
	    unit, devname);
	ppp_if[unit] = NULL;
        return;
    }

    /* initialize the authentication secrets table (vxWorks) */

    (void) pppSecretLibInit ();

    /*
     * Initialize to the standard option set, then parse, in order,
     * the system options file, the user's options file, and the command
     * line arguments.
     */
    for (i = 0; i < N_PROTO; i++)
        (*prottbl[i].init)(unit);

    if (!parse_args(unit, devname, local_addr, remote_addr, baud, ppp_if[unit]->options, fOptions))
        die(unit, 1);

    /* set up the serial device as a ppp interface */
    establish_ppp();

    check_auth_options();
    setipdefault();
    
    magic_init();

    remCurIdGet(user, NULL);
    syslog (LOG_NOTICE, "ppp %s.%d started by %s", VERSION, PATCHLEVEL, user);

    /* Get an internet socket for doing socket ioctl's on. */
    if ((ppp_if[unit]->s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        syslog(LOG_NOTICE, "socket error");
        die(unit, 1);
    }

    /*
     * Compute mask of all interesting signals and install signal handlers
     * for each.  Only one signal handler may be active at a time.  Therefore,
     * all other signals should be masked when any handler is executing.
     */
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGALRM);
    sigaddset(&mask, SIGIO);

#define SIGNAL(s, handler)      { \
        sa.sa_handler = (void (*)())handler; \
        if (sigaction(s, &sa, NULL) < 0) { \
            syslog(LOG_ERR, "sigaction(%d) error", s); \
            die(unit, 1); \
        } \
    }

    sa.sa_mask = mask;
    sa.sa_flags = 0;
    SIGNAL(SIGHUP, hup);		/* Hangup */
    SIGNAL(SIGINT, intr);		/* Interrupt */
    SIGNAL(SIGTERM, term);		/* Terminate */
    SIGNAL(SIGALRM, alrm);		/* Timeout */
    SIGNAL(SIGIO, io);			/* Input available */

    signal(SIGUSR1, incdebug);		/* Increment debug flag */
    signal(SIGUSR2, nodebug);		/* Reset debug flag */

    /*
     * Block SIGIOs and SIGPOLLs for now
     */
    sigemptyset(&mask);
    sigaddset(&mask, SIGIO);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    /* set line speed, flow control, etc. */
    set_up_tty(ppp_if[unit]->fd);

    /* run connection hook routine */

    if ((pppHookRtns[unit] != NULL) && 
	(pppHookRtns[unit]->connectHook != NULL))
	{
        if (((*pppHookRtns[unit]->connectHook) (unit, ppp_if[unit]->fd)) 
	    == ERROR)
	    {
            syslog(LOG_ERR, "connect hook failed");
            die(unit, 1);
	    }

        syslog(LOG_INFO, "Connected...");
	}

    MAINDEBUG((LOG_INFO, "Using interface ppp%d", unit));

    (void) sprintf(ppp_if[unit]->ifname, "ppp%d", unit);

    /*
     * Block all signals, start opening the connection, and  wait for
     * incoming signals (reply, timeout, etc.).
     */
    syslog (LOG_NOTICE, "Connect: %s <--> %s", ppp_if[unit]->ifname,
           ppp_if[unit]->devname);
    sigprocmask(SIG_BLOCK, &mask, NULL); /* Block signals now */
    lcp_lowerup(unit);		/* XXX Well, sort of... */
    lcp_open(unit);		/* Start protocol */
    sigemptyset(&mask);
    for (ppp_if[unit]->phase = PHASE_ESTABLISH, errno = 0;
	 ppp_if[unit]->phase != PHASE_DEAD;) {
	sigsuspend(&mask);		/* Wait for next signal */
	if (errno != EINTR)
	    {
            syslog(LOG_ERR, "ppp_task: sigsuspend error!");
	    break;
	    }

        errno = 0;			/* reset errno */
    }

    /* run disconnection hook routine */

    if ((pppHookRtns[unit] != NULL) && 
	(pppHookRtns[unit]->disconnectHook != NULL))
	{
        if (((*pppHookRtns[unit]->disconnectHook) (unit, ppp_if[unit]->fd))
	    == ERROR)
	    {
            syslog(LOG_WARNING, "disconnect hook failed");
            die(unit, 1);
	    }

        syslog(LOG_INFO, "Disconnected...");
	}

    die(unit, 1);
}
  
/*******************************************************************************
*
* pppDelete - delete a PPP network interface
*
* This routine deletes the Point-to-Point Protocol (PPP) network interface
* specified by the unit number <unit>.
*
* A Link Control Protocol (LCP) terminate request packet is sent to notify
* the peer of the impending PPP link shut-down.  The associated serial
* interface (<tty>) is then detached from the PPP driver, and the PPP interface
* is deleted from the list of network interfaces.  Finally, all resources
* associated with the PPP link are returned to the VxWorks system.
*
* RETURNS: N/A
*/

void pppDelete
    (
    int unit			/* PPP interface unit number to delete */
    )
    {
    struct wordlist *next;
    struct wordlist *wp;
 
    if  (ppp_if[unit] != NULL)
	    {
        if  ((pppHookRtns[unit] != NULL) &&
         (pppHookRtns[unit]->disconnectHook != NULL))
            {
            if  (ERROR ==
                 (*pppHookRtns[unit]->disconnectHook)(unit, ppp_if[unit]->fd))
                {
                syslog(LOG_WARNING, "disconnect hook failed");
                }
            }
    
	    /* check for any clients' addresses */

	    wp = ppp_if [unit]->addresses;
	    while (wp != NULL) 
	        {
	        next = wp->next;
	        free(wp);
	        wp = next;
	        }

	    netJobAdd ((FUNCPTR)kill, ppp_if[unit]->task_id, SIGTERM, 0, 0, 0);
	    }
    }

/*
 * Translate from bits/second to a speed_t.
 */
static int
translate_speed(bps)
    int bps;
{
    return bps;
}

/*
 * translate from a speed_t to bits/second.
 */
static int
baud_rate_of(speed)
    int speed;
{
    return speed;
}

/*
 * set_up_tty: Set up the serial port on `fd' for 8 bits, no parity,
 * at the requested speed, etc.  If `local' is true, set CLOCAL
 * regardless of whether the modem option was specified.
 */
static int
set_up_tty(fd)
    int fd;
{
    int speed;
    int state;

    /*
     * Put the tty in raw mode
     */
    if ((state = ioctl(fd, FIOGETOPTIONS, NULL)) == ERROR) {
        syslog(LOG_ERR, "ioctl(FIOGETOPTIONS) error");
        die(ppp_unit, 1);
    }

    if (!ppp_if[ppp_unit]->restore_term)
        ppp_if[ppp_unit]->ttystate = state;

    speed = translate_speed(ppp_if[ppp_unit]->inspeed);

    if (ioctl(fd, FIOOPTIONS, OPT_RAW) < 0) {
        syslog(LOG_ERR, "ioctl(FIOOPTIONS) error");
        die(ppp_unit, 1);
    }

    if (speed && ioctl(fd, FIOBAUDRATE, speed) == ERROR) {
        syslog(LOG_ERR, "ioctl(FIOBAUDRATE) error");
        die(ppp_unit, 1);
    }

    ppp_if[ppp_unit]->baud_rate = baud_rate_of(speed);
    ppp_if[ppp_unit]->restore_term = TRUE;
    return 0;
}
  
/*
 * die - like quit, except we can specify an exit status.
 */
void
die(unit, status)
    int unit;
    int status;
{

    fsm *f = &ppp_if[unit]->lcp_fsm;

    lcp_close(unit);		/* Close connection */

    if (f->flags & (OPT_PASSIVE | OPT_SILENT))
	return; 
    MAINDEBUG((LOG_INFO, " Exitting."));
    cleanup(unit, status, NULL);
    if (status)
        exit(status);
}

/*
 * cleanup - restore anything which needs to be restored before we exit
 */
static void
cleanup(unit, status, arg)
    int unit;
    int status;
    caddr_t arg;
{
    struct callout *freep = NULL;
    struct callout *list = NULL;

    if (unit < 0 || unit > NPPP)
        return;

    {
        disestablish_ppp();

	if (ppp_if[unit]->restore_term) {
            if (ioctl(ppp_if[unit]->fd, FIOSETOPTIONS, ppp_if[unit]->ttystate) == ERROR)
            syslog(LOG_ERR, "ioctl(FIOSETOPTIONS) error");
        }

        pppclose(unit);
        ppp_if[unit]->fd = ERROR;
    }

    if (ppp_if[unit]->s != ERROR)
        close(ppp_if[unit]->s);

    if (!status)
        if (ppp_if[unit]->task_id)
            taskDelete(ppp_if[unit]->task_id);

    timer_delete(ppp_if[unit]->timer_id);

    /* clean-up callout structures */

    list = ppp_if [ppp_unit]->callout;
    while (list)
	{
	freep = list;
	list = list->c_next;
	free ((char *) freep);
	freep = NULL;
	}

    free(ppp_parms[unit]->task_name);

    if (ppp_parms[unit]->devname)
        free(ppp_parms[unit]->devname);

    if (ppp_parms[unit]->local_addr)
        free(ppp_parms[unit]->local_addr);

    if (ppp_parms[unit]->remote_addr)
        free(ppp_parms[unit]->remote_addr);

    if (ppp_parms[unit]->filename)
        free(ppp_parms[unit]->filename);

    free((char *)ppp_parms[unit]);

    if (ppp_if[unit]->options->pap_file)
        free(ppp_if[unit]->options->pap_file);

    if (ppp_if[unit]->options->chap_file)
        free(ppp_if[unit]->options->chap_file);

    if ((char *)ppp_if[unit])
        free((char *)ppp_if[unit]);

    ppp_if[unit] = NULL;
}
  

/*
 * ppp_timeout - Schedule a timeout.
 *
 * Note that this timeout takes the number of seconds, NOT hz (as in
 * the kernel).
 */
void
ppp_timeout(func, arg, seconds)
    void (*func)();
    caddr_t arg;
    int seconds;
{
    struct itimerspec itv;
    struct callout *newp, **oldpp;
    
    MAINDEBUG((LOG_DEBUG, "Timeout %x:%x in %d seconds.", (int) func, (int) arg, seconds));
  
    /*
     * Allocate timeout.
     */
    if ((newp = (struct callout *) malloc(sizeof(struct callout))) == NULL) {
      syslog(LOG_ERR, "Out of memory in timeout()!");
      die(ppp_unit, 1);
    }
    newp->c_arg = arg;
    newp->c_func = func;
    
    /*
     * Find correct place to link it in and decrement its time by the
     * amount of time used by preceding timeouts.
     */
    for (oldpp = &(ppp_if[ppp_unit]->callout);
         *oldpp && (*oldpp)->c_time <= seconds;
         oldpp = &(*oldpp)->c_next)
        seconds -= (*oldpp)->c_time;
    newp->c_time = seconds;
    newp->c_next = *oldpp;
    if (*oldpp)
        (*oldpp)->c_time -= seconds;
    *oldpp = newp;
    
    /*
     * If this is now the first callout then we have to set a new
     * itimer.
     */
    if (ppp_if[ppp_unit]->callout == newp) {
        itv.it_interval.tv_sec = itv.it_interval.tv_nsec =
            itv.it_value.tv_nsec = 0;
        itv.it_value.tv_sec = ppp_if[ppp_unit]->callout->c_time;
        MAINDEBUG((LOG_DEBUG, "Setting itimer for %d seconds in timeout.",
  	          itv.it_value.tv_sec));
        if (timer_settime(ppp_if[ppp_unit]->timer_id, CLOCK_REALTIME, &itv, NULL) < 0) {
            syslog(LOG_ERR, "setitimer(ITIMER_REAL): error");
            die(ppp_unit, 1);
        }
        if (time(&(ppp_if[ppp_unit]->schedtime)) == ERROR) {
            syslog(LOG_ERR, "gettimeofday: error");
            die(ppp_unit, 1);
        }
    }
}
  

/*
 * ppp_untimeout - Unschedule a timeout.
 */
void
ppp_untimeout(func, arg)
    void (*func)();
    caddr_t arg;
{
    struct itimerspec itv;
    struct callout **copp, *freep;
    int reschedule = 0;
    
    MAINDEBUG((LOG_DEBUG, "Untimeout %x:%x.", (int) func, (int) arg));
    
    /*
     * If the first callout is unscheduled then we have to set a new
     * itimer.
     */
    if (ppp_if[ppp_unit]->callout &&
        ppp_if[ppp_unit]->callout->c_func == func &&
        ppp_if[ppp_unit]->callout->c_arg == arg)
        reschedule = 1;
    
    /*
     * Find first matching timeout.  Add its time to the next timeouts
     * time.
     */
    for (copp = &(ppp_if[ppp_unit]->callout); *copp; copp = &(*copp)->c_next)
        if ((*copp)->c_func == func &&
  	    (*copp)->c_arg == arg) {
            freep = *copp;
            *copp = freep->c_next;
            if (*copp)
  	    (*copp)->c_time += freep->c_time;
	    (void) free((char *) freep);
            break;
          }
    
    if (reschedule) {
        itv.it_interval.tv_sec = itv.it_interval.tv_nsec =
            itv.it_value.tv_nsec = 0;
        itv.it_value.tv_sec = ppp_if[ppp_unit]->callout ? ppp_if[ppp_unit]->callout->c_time : 0;
        MAINDEBUG((LOG_DEBUG, "untimeout: Setting itimer for %d seconds in untimeout.",
  	          itv.it_value.tv_sec));
        if (timer_settime(ppp_if[ppp_unit]->timer_id, CLOCK_REALTIME, &itv, NULL) < 0) {
            syslog(LOG_ERR, "setitimer(ITIMER_REAL): error");
            die(ppp_unit, 1);
        }
        if (time(&(ppp_if[ppp_unit]->schedtime)) == ERROR) {
            syslog(LOG_ERR, "gettimeofday: error");
            die(ppp_unit, 1);
        }
    }
}


/*
 * adjtimeout - Decrement the first timeout by the amount of time since
 * it was scheduled.
 */
void
adjtimeout()
{
    time_t tv;
    int timediff;
    
    if (ppp_if[ppp_unit]->callout == NULL)
        return;
    /*
     * Make sure that the clock hasn't been warped dramatically.
     * Account for recently expired, but blocked timer by adding
     * small fudge factor.
     */
    if (time(&tv) == ERROR) {
        syslog(LOG_ERR, "gettimeofday: error");
        die(ppp_unit, 1);
    }
    timediff = tv - ppp_if[ppp_unit]->schedtime;
    if (timediff < 0 ||
        timediff >= ppp_if[ppp_unit]->callout->c_time)
        return;
    
    ppp_if[ppp_unit]->callout->c_time -= timediff;	/* OK, Adjust time */
}
  
/*
 * hup - Catch SIGHUP signal.
 *
 * Indicates that the physical layer has been disconnected.
 */
static void
hup(sig)
    int sig;
{
    MAINDEBUG((LOG_INFO, "Hangup (SIGHUP)"));
    ppp_if[ppp_unit]->hungup = 1;	/* they hung up on us! */
    adjtimeout();               	/* Adjust timeouts */
    die(ppp_unit, 1);			/* Close connection */
}

/*
 * term - Catch SIGTERM signal.
 *
 * Indicates that we should initiate a graceful disconnect and exit.
 */
static void
term(sig)
    int sig;
{
    fsm *f = &ppp_if[ppp_unit]->lcp_fsm;

    /* even if OPT_SILENT | OPT_PASSIVE, force die(..) to delete */
    f->flags = 0;   
    MAINDEBUG((LOG_INFO, "Terminating link."));
    adjtimeout();               	/* Adjust timeouts */
    die(ppp_unit, 1);			/* Close connection */
}


/*
 * intr - Catch SIGINT signal (DEL/^C).
 *
 * Indicates that we should initiate a graceful disconnect and exit.
 */
static void
intr(sig)
    int sig;
{
    MAINDEBUG((LOG_INFO, "Interrupt received: terminating link"));
    adjtimeout();			/* Adjust timeouts */
    die(ppp_unit, 1);			/* Close connection */
}

/*
 * alrm - Catch SIGALRM signal.
 *
 * Indicates a timeout.
 */
static void
alrm(sig)
    int sig;
{
    struct itimerspec itv;
    struct callout *freep, *list, *last;
    
    MAINDEBUG((LOG_DEBUG, "Alarm"));
    
    if (ppp_if[ppp_unit]->callout == NULL)
        return;
    /*
     * Get the first scheduled timeout and any that were scheduled
     * for the same time as a list, and remove them all from callout
     * list.
     */
    list = last = ppp_if[ppp_unit]->callout;
    while (last->c_next != NULL && last->c_next->c_time == 0)
        last = last->c_next;
    ppp_if[ppp_unit]->callout = last->c_next;
    last->c_next = NULL;

    /*
     * Set a new itimer if there are more timeouts scheduled.
     */
    if (ppp_if[ppp_unit]->callout) {
        itv.it_interval.tv_sec = itv.it_interval.tv_nsec = 0;
        itv.it_value.tv_nsec = 0;
        itv.it_value.tv_sec = ppp_if[ppp_unit]->callout->c_time;
        MAINDEBUG((LOG_DEBUG, "Setting itimer for %d seconds in alrm.",
                   itv.it_value.tv_sec));
        if (timer_settime(ppp_if[ppp_unit]->timer_id, CLOCK_REALTIME, &itv, NULL) < 0) {
            syslog(LOG_ERR, "setitimer(ITIMER_REAL): error");
            die(ppp_unit, 1);
        }
        if (time(&ppp_if[ppp_unit]->schedtime) == ERROR) {
            syslog(LOG_ERR, "gettimeofday: error");
            die(ppp_unit, 1);
        }
    }

    /*
     * Now call all the timeout routines scheduled for this time.
     */
    while (list) {
        (*list->c_func)(list->c_arg);
        freep = list;
        list = list->c_next;
        (void) free((char *) freep);
    }
  }
  
/*
 * io - Catch SIGIO signal.
 *
 * Indicates that incoming data is available.
 */
static void
io()
{
    int len, i;
    u_char *p;
    u_short protocol;

    MAINDEBUG((LOG_DEBUG, "IO signal received"));
    adjtimeout();         /* Adjust timeouts */

    /* Yup, this is for real */
    for (;;) {                    /* Read all available packets */
        p = ppp_if[ppp_unit]->inpacket_buf;/* point to beggining of packet buffer */

        len = read_packet(ppp_if[ppp_unit]->inpacket_buf);
        if (len < 0)
            break;

        if (ppp_if[ppp_unit]->debug /*&& (debugflags & DBG_INPACKET)*/)
            log_packet(p, len, "rcvd ");

        if (len < DLLHEADERLEN) {
            MAINDEBUG((LOG_DEBUG, "io(): Received short packet."));
            break;
        }

        p += 2;                             /* Skip address and control */
        GETSHORT(protocol, p);
        len -= DLLHEADERLEN;

        /*
         * Toss all non-LCP packets unless LCP is OPEN.
         */
        if (protocol != LCP && ppp_if[ppp_unit]->lcp_fsm.state != OPENED) {
            MAINDEBUG((LOG_DEBUG, "io(): Received non-LCP packet and LCP not open."
));
            break;
        }

        /*
         * Upcall the proper protocol input routine.
         */
        for (i = 0; i < sizeof (prottbl) / sizeof (struct protent); i++)
            if (prottbl[i].protocol == protocol) {
                (*prottbl[i].input)(ppp_unit, p, len);
                break;
            }

        if (i == sizeof (prottbl) / sizeof (struct protent)) {
	    ppp_if[ppp_unit]->unknownProto++;
            syslog(LOG_WARNING, "input: Unknown protocol (%x) received!",
		   protocol);
            lcp_sprotrej(ppp_unit, p - DLLHEADERLEN,
		         len + DLLHEADERLEN);
        }
    }
}

/*
 * demuxprotrej - Demultiplex a Protocol-Reject.
 */
void
demuxprotrej
    (
    int unit,
    u_short protocol
    )
{
    int i;
    
    /*
     * Upcall the proper Protocol-Reject routine.
     */
    for (i = 0; i < sizeof (prottbl) / sizeof (struct protent); i++)
        if (prottbl[i].protocol == protocol) {
            (*prottbl[i].protrej)(unit);
            return;
        }
    syslog(LOG_WARNING,
	   "demuxprotrej: Unrecognized Protocol-Reject for protocol %d!",
	   protocol);
}

/*
 * incdebug - Catch SIGUSR1 signal.
 *
 * Increment debug flag.
 */
/*ARGSUSED*/
static void
incdebug(sig)
    int sig;
{
    syslog(LOG_INFO, "Debug turned ON, Level %d", ppp_if[ppp_unit]->debug);
    ppp_if[ppp_unit]->debug++;
}

/*
 * nodebug - Catch SIGUSR2 signal.
 *
 * Turn off debugging.
 */
/*ARGSUSED*/
static void
nodebug(sig)
    int sig;
{
    ppp_if[ppp_unit]->debug = 0;
}

#ifdef	notyet
/*
 * device_script - run a program to connect or disconnect the
 * serial device.
 */
int
device_script(program, in, out)
    char *program;
    int in, out;
{
    int pid;
    int status;
    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGHUP);
    sigprocmask(SIG_BLOCK, &mask, &mask);

    pid = fork();

    if (pid < 0) {
        syslog(LOG_ERR, "fork: %m");
        die(1);
    }

    if (pid == 0) {
        setreuid(getuid(), getuid());
        setregid(getgid(), getgid());
        sigprocmask(SIG_SETMASK, &mask, NULL);
        dup2(in, 0);
        dup2(out, 1);
        execl("/bin/sh", "sh", "-c", program, (char *)0);
        syslog(LOG_ERR, "could not exec /bin/sh: %m");
        _exit(99);
        /* NOTREACHED */
    }

    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR)
            continue;
        syslog(LOG_ERR, "waiting for (dis)connection process: %m");
        die(1);
    }
    sigprocmask(SIG_SETMASK, &mask, NULL);


    return (status == 0 ? 0 : -1);
}

/*
 * run-program - execute a program with given arguments,
 * but don't wait for it.
 * If the program can't be executed, logs an error unless
 * must_exist is 0 and the program file doesn't exist.
 */
int
run_program(prog, args, must_exist)
    char *prog;
    char **args;
    int must_exist;
{
    int pid;

    pid = fork();
    if (pid == -1) {
        syslog(LOG_ERR, "can't fork to run %s: %m", prog);
        return -1;
    }
    if (pid == 0) {
        execv(prog, args);
        if (must_exist || errno != ENOENT)
            syslog(LOG_WARNING, "can't execute %s: %m", prog);
        _exit(-1);
    }
    MAINDEBUG((LOG_DEBUG, "Script %s started; pid = %d", prog, pid));
    ++n_children;
    return 0;
}
#endif	/* notyet */

/*
 * log_packet - format a packet and log it.
 */

char line[256];                 /* line to be logged accumulated here */
char *linep;

void
log_packet(p, len, prefix)
    u_char *p;
    int len;
    char *prefix;
{
    strcpy(line, prefix);
    linep = line + strlen(line);
    format_packet(p, len, pr_log, NULL);
    if (linep != line)
        syslog(LOG_DEBUG, "%s", line);
}

/*
 * format_packet - make a readable representation of a packet,
 * calling `printer(arg, format, ...)' to output it.
 */
void
format_packet(p, len, printer, arg)
    u_char *p;
    int len;
    void (*printer) __ARGS((void *, char *, ...));
    void *arg;
{
    int i, n;
    u_short proto;
    u_char x;

    if (len >= DLLHEADERLEN && p[0] == ALLSTATIONS && p[1] == UI) {
        p += 2;
        GETSHORT(proto, p);
        len -= DLLHEADERLEN;
        for (i = 0; i < N_PROTO; ++i)
            if (proto == prottbl[i].protocol)
                break;
        if (i < N_PROTO) {
            printer(arg, "[%s", prottbl[i].name);
            n = (*prottbl[i].printpkt)(p, len, printer, arg);
            printer(arg, "]");
            p += n;
            len -= n;
        } else {
            printer(arg, "[proto=0x%x]", proto);
        }
    }

    for (; len > 0; --len) {
        GETCHAR(x, p);
        printer(arg, " %.2x", x);
    }
}

#ifdef __STDC__
#include <stdarg.h>

void
pr_log(void *arg, char *fmt, ...)
{
    int n;
    va_list pvar;
    char buf[256];

    va_start(pvar, fmt);
    vsprintf(buf, fmt, pvar);
    va_end(pvar);

    n = strlen(buf);
    if (linep + n + 1 > line + sizeof(line)) {
        syslog(LOG_DEBUG, "%s", line);
        linep = line;
    }
    strcpy(linep, buf);
    linep += n;
}

#else /* __STDC__ */
#include <varargs.h>

void
pr_log(arg, fmt, va_alist)
void *arg;
char *fmt;
va_dcl
{
    int n;
    va_list pvar;
    char buf[256];

    va_start(pvar);
    vsprintf(buf, fmt, pvar);
    va_end(pvar);

    n = strlen(buf);
    if (linep + n + 1 > line + sizeof(line)) {
        syslog(LOG_DEBUG, "%s", line);
        linep = line;
    }
    strcpy(linep, buf);
    linep += n;
}
#endif

/*
 * print_string - print a readable representation of a string using
 * printer.
 */
void
print_string(p, len, printer, arg)
    char *p;
    int len;
    void (*printer) __ARGS((void *, char *, ...));
    void *arg;
{
    int c;

    printer(arg, "\"");
    for (; len > 0; --len) {
        c = *p++;
        if (' ' <= c && c <= '~')
            printer(arg, "%c", c);
        else
            printer(arg, "\\%.3o", c);
    }
    printer(arg, "\"");
}

/*
 * novm - log an error message saying we ran out of memory, and die.
 */
void
novm(msg)
    char *msg;
{
    syslog(LOG_ERR, "Memory exhausted allocating %s", msg);
    die(ppp_unit, 1);
}

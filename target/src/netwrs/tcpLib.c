/* tcpLib.c - tcp protocol interface library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,20may02,vvv  moved tcpstates[] definition and pTcpStates initialization
                 from tcp_debug.c (SPR #62272)
01d,15oct01,rae  merge from truestack ver 01e, base 01c (TCP_RAND_FUNC)
01c,14nov00,ham  fixed unnecessary dependency against tcp_debug(SPR 62272).
01b,31jan97,vin  added new variable for connection timeout.
01a,20jan97,vin  written
*/

/*
DESCRIPTION
This library contains the interface to the tcp protocol. The TCP protocol
is configured in this library. 

The routine tcpLibInit() is responsible for configuring the tcp protocol
with various parameters.

INCLUDE FILES: netLib.h

.pG "Network"

NOMANUAL
*/

/* includes */
#include "vxWorks.h"
#include "netLib.h"
#include "net/protosw.h"
#include "net/domain.h"
#include "net/mbuf.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/in_pcb.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "netinet/tcp.h"
#include "netinet/tcp_fsm.h"
#include "netinet/tcp_seq.h"
#include "netinet/tcp_timer.h"
#include "netinet/tcp_var.h"
#include "netinet/tcpip.h"
#include "netinet/tcp_debug.h"
#include "memPartLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#else

/* externs */

IMPORT int	_protoSwIndex;
IMPORT struct 	protosw	inetsw [IP_PROTO_NUM_MAX]; 

IMPORT int	tcp_do_rfc1323;
IMPORT u_long	tcp_sendspace;
IMPORT u_long	tcp_recvspace;
IMPORT int	tcp_keepinit;
IMPORT int	tcprexmtthresh;
IMPORT int 	tcp_mssdflt;
IMPORT int 	tcp_rttdflt;
IMPORT int	tcp_keepidle;
IMPORT int	tcp_keepcnt;
#endif

/* globals */

char **         pTcpstates = NULL;

char *tcpstates[] = { "CLOSED",      "LISTEN",     "SYN_SENT",   "SYN_RCVD",
		      "ESTABLISHED", "CLOSE_WAIT", "FIN_WAIT_1", "CLOSING",
		      "LAST_ACK",    "FIN_WAIT_2", "TIME_WAIT" };
/* defines */

/* typedefs */


/* locals */

STATUS tcpLibInit 
    (
    TCP_CFG_PARAMS * 	tcpCfg		/* tcp configuration parameters */
    )
    {
#ifdef VIRTUAL_STACK
    VS_TCP * pGlobals;
#endif

    FAST struct protosw	* pProtoSwitch; 

    if (_protoSwIndex >= sizeof(inetsw)/sizeof(inetsw[0]))
	return (ERROR) ;

    pProtoSwitch = &inetsw [_protoSwIndex]; 

    if (pProtoSwitch->pr_domain != NULL)
	return (OK); 				/* already initialized */

#ifdef VIRTUAL_STACK

    /* Allocate space for former global variables. */

    pGlobals = KHEAP_ALLOC (sizeof (VS_TCP));
    if (pGlobals == NULL)
        return (ERROR);

    bzero ( (char *)pGlobals, sizeof (VS_TCP));

    vsTbl [myStackNum]->pTcpGlobals = pGlobals;
#endif

    pProtoSwitch->pr_type   	=  SOCK_STREAM;
    pProtoSwitch->pr_domain   	=  &inetdomain;
    pProtoSwitch->pr_protocol   =  IPPROTO_TCP;
    pProtoSwitch->pr_flags	=  PR_CONNREQUIRED | PR_WANTRCVD;
    pProtoSwitch->pr_input	=  tcp_input;
    pProtoSwitch->pr_output	=  0;
    pProtoSwitch->pr_ctlinput	=  tcp_ctlinput;
    pProtoSwitch->pr_ctloutput	=  tcp_ctloutput;
    pProtoSwitch->pr_usrreq	=  tcp_usrreq;
    pProtoSwitch->pr_init	=  tcp_init;
    pProtoSwitch->pr_fasttimo	=  tcp_fasttimo;
    pProtoSwitch->pr_slowtimo	=  tcp_slowtimo;
    pProtoSwitch->pr_drain	=  tcp_drain;
    pProtoSwitch->pr_sysctl	=  0;

    _protoSwIndex++; 

    /* initialize tcp configuration parameters */

    tcp_do_rfc1323 = (tcpCfg->tcpCfgFlags & TCP_DO_RFC1323) ? TRUE : FALSE; 
    tcp_sendspace  = tcpCfg->tcpSndSpace;
    tcp_recvspace  = tcpCfg->tcpRcvSpace;
    tcp_keepinit   = tcpCfg->tcpConnectTime;
    tcprexmtthresh = tcpCfg->tcpReTxThresh;
    tcp_mssdflt	   = tcpCfg->tcpMssDflt;
    tcp_rttdflt    = tcpCfg->tcpRttDflt;
    tcp_keepidle   = tcpCfg->tcpKeepIdle;
    tcp_keepcnt    = tcpCfg->tcpKeepCnt;

#ifdef VIRTUAL_STACK
    /*
     * Assign (former) global variables previously initialized by the compiler.
     * Setting 0 is repeated for clarity - the vsLib.c setup zeroes all values.
     */

   tcp_last_inpcb = NULL; 			/* tcp_input.c */
   tcp_do_rfc1323 = 1; 				/* tcp_subr.c */
   tcp_pcbhashsize = 128; 			/* tcp_subr.c (TCBHASHSIZE) */
   tcp_keepintvl = TCPTV_KEEPINTVL; 		/* tcp_timer.c */
   tcp_maxpersistidle = TCPTV_KEEP_IDLE; 	/* tcp_timer.c */
#endif

    tcpRandHookAdd (tcpCfg->pTcpRandFunc);

    pTcpstates = tcpstates;
    return (OK); 
    }

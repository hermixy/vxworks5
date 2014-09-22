/* udpLib.c - udp protocol interface library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01b,15oct01,rae  merge from truestack ver 01b, base 01a (VIRTUAL_STACK)
01a,20jan97,vin written
*/

/*
DESCRIPTION
This library contains the interface to the udp protocol. The UDP protocol
is configured in this library. 

The routine udpLibInit() is responsible for configuring the udp protocol
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
#include "netinet/udp.h"
#include "netinet/udp_var.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif

/* externs */

#ifndef VIRTUAL_STACK

IMPORT int	_protoSwIndex;
IMPORT struct protosw 	inetsw [IP_PROTO_NUM_MAX]; 

IMPORT u_long	udp_sendspace;		/* variable to control send space */
IMPORT u_long	udp_recvspace;		/* variable to control recv space */
IMPORT int	udpcksum;		/* variable to control send cksum */
IMPORT int	udpDoCkSumRcv;		/* vairiable to control rcv cksum */

#endif /* VIRTUAL_STACK */

/* globals */

/* defines */

/* typedefs */


/* locals */

STATUS udpLibInit 
    (
    UDP_CFG_PARAMS * 	udpCfg		/* udp config parameters */
    )
    {
    FAST struct protosw	* pProtoSwitch; 

    if (_protoSwIndex >= sizeof(inetsw)/sizeof(inetsw[0]))
	return (ERROR) ;

    pProtoSwitch = &inetsw [_protoSwIndex]; 

    if (pProtoSwitch->pr_domain != NULL)
	return (OK); 				/* already initialized */

    pProtoSwitch->pr_type   	=  SOCK_DGRAM;
    pProtoSwitch->pr_domain   	=  &inetdomain;
    pProtoSwitch->pr_protocol   =  IPPROTO_UDP;
    pProtoSwitch->pr_flags	=  PR_ATOMIC | PR_ADDR;
    pProtoSwitch->pr_input	=  udp_input;
    pProtoSwitch->pr_output	=  0;
    pProtoSwitch->pr_ctlinput	=  udp_ctlinput;
    pProtoSwitch->pr_ctloutput	=  ip_ctloutput;
    pProtoSwitch->pr_usrreq	=  udp_usrreq;
    pProtoSwitch->pr_init	=  udp_init;
    pProtoSwitch->pr_fasttimo	=  0;
    pProtoSwitch->pr_slowtimo	=  0;
    pProtoSwitch->pr_drain	=  0;
    pProtoSwitch->pr_sysctl	=  0;

    _protoSwIndex++; 

    /* initialize udp configuration parameters */

    udpcksum 	  = (udpCfg->udpCfgFlags & UDP_DO_CKSUM_SND) ? TRUE : FALSE; 
    udpDoCkSumRcv = (udpCfg->udpCfgFlags & UDP_DO_CKSUM_RCV) ? TRUE : FALSE;
    udp_sendspace = udpCfg->udpSndSpace;
    udp_recvspace = udpCfg->udpRcvSpace;

#ifdef VIRTUAL_STACK
    /*
     * Assign (former) global variables previously initialized by the compiler.
     * Setting 0 is repeated for clarity - the vsLib.c setup zeroes all values.
     */

    udp_in.sin_len = sizeof (struct sockaddr_in);
    udp_in.sin_family = AF_INET;
    udp_last_inpcb = NULL;
    udp_ttl = UDP_TTL;
    udp_pcbhashsize = UDBHASHSIZE;
#endif

    return (OK); 
    }

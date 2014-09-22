/* igmpLib.c - igmp protocol interface library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01b,15oct01,rae  merge from truestack ver 01c, base 01a (IGMPv2)
01a,20jan97,vin written
*/

/*
DESCRIPTION
This library contains the interface to the igmp protocol. The IGMP protocol
is configured in this library. 

The routine igmpLibInit() is responsible for configuring the IGMP protocol
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
#include "netinet/igmp_var.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif

/* externs */

#ifndef VIRTUAL_STACK
IMPORT int	_protoSwIndex;
IMPORT struct 	protosw inetsw [IP_PROTO_NUM_MAX]; 
#endif

/* globals */

/* defines */

/* typedefs */

/* locals */

STATUS igmpLibInit (void)
    {
    FAST struct protosw	* pProtoSwitch; 

    if (_protoSwIndex >= sizeof(inetsw)/sizeof(inetsw[0]))
	return (ERROR) ;

    pProtoSwitch = &inetsw [_protoSwIndex]; 

    if (pProtoSwitch->pr_domain != NULL)
	return (OK); 				/* already initialized */

    pProtoSwitch->pr_type   	=  SOCK_RAW;
    pProtoSwitch->pr_domain   	=  &inetdomain;
    pProtoSwitch->pr_protocol   =  IPPROTO_IGMP;
    pProtoSwitch->pr_flags	=  PR_ATOMIC | PR_ADDR;
    pProtoSwitch->pr_input	=  igmp_input;
    pProtoSwitch->pr_output	=  rip_output;
    pProtoSwitch->pr_ctlinput	=  0;
    pProtoSwitch->pr_ctloutput	=  rip_ctloutput;
    pProtoSwitch->pr_usrreq	=  rip_usrreq;
    pProtoSwitch->pr_init	=  igmp_init;
    pProtoSwitch->pr_fasttimo	=  igmp_fasttimo;
    pProtoSwitch->pr_slowtimo	=  igmp_slowtimo;
    pProtoSwitch->pr_drain	=  0;
    pProtoSwitch->pr_sysctl	=  0;

    _protoSwIndex++; 
    return (OK); 
    }

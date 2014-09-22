/* udpShow.c - UDP information display routines */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,10may02,kbw  making man page edits
01e,15oct01,rae  merge from truestack ver 01f, base 01d (VIRTUAL_STACK)
01d,14dec97,jdi  doc: cleanup
01c,04aug97,kbw  fixed man page problems found in beta review
01b,20apr97,kbw  fixed man page format, spell check.
01a,08mar97,vin  written.
*/

/*
DESCRIPTION
This library provides routines to show UDP related statistics.

Interpreting these statistics requires detailed knowledge of Internet
network protocols.  Information on these protocols can be found in
the following books:

.iP
.I "TCP/IP Illustrated Volume II, The Implementation,"
by Richard Stevens
.iP
.I "The Design and Implementation of the 4.4 BSD UNIX Operating System,"
by Leffler, McKusick, Karels and Quarterman
.LP
The udpShowInit() routine links the UDP show facility into the VxWorks
system.  This is performed automatically if INCLUDE_NET_SHOW and INCLUDE_UDP
are defined.

SEE ALSO: netLib, netShow
*/

#include "vxWorks.h"
#include "sys/types.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "netinet/udp.h"
#include "netinet/udp_var.h"
#include "errno.h"
#include "string.h"
#include "stdio.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif    /* VIRTUAL_STACK */

#define plural(num)	((num) > 1 ? "s": "")

/* externs */

#ifndef VIRTUAL_STACK 
IMPORT struct inpcbhead		udb;		/* defined in udp_usrreq.c */
IMPORT struct inpcbhead * 	_pUdpPcbHead;	/* defined in netShow.c */
IMPORT struct udpstat 		udpstat;	/* defined in udp_usrreq.c */
#endif    /* VIRTUAL_STACK */

/******************************************************************************
*
* udpShowInit - initialize UDP show routines
*
* This routine links the UDP show facility into the VxWorks system.
* These routines are included automatically if INCLUDE_NET_SHOW
* and INCLUDE_UDP are defined.
*
* RETURNS: N/A
*/

void udpShowInit (void)
    {
    _pUdpPcbHead = &udb; 	/* initialize the pcb for generic show rtns */
    }

/*****************************************************************************
*
* udpstatShow - display statistics for the UDP protocol
*
* This routine displays statistics for the UDP protocol.
*
* RETURNS: N/A
*/

void udpstatShow (void)
    {
    unsigned int udptotal = udpstat.udps_ipackets + udpstat.udps_opackets;

#ifdef VIRTUAL_STACK
    printf("UDP: (stack number %d)\n\t%u total packets\n", myStackNum,
           udptotal);
#else
    printf("UDP:\n\t%u total packets\n", udptotal);
#endif /* VIRTUAL_STACK */
    printf("\t%u input packets\n", (int)udpstat.udps_ipackets);
    printf("\t%u output packets\n", (int)udpstat.udps_opackets);
    printf("\t%u incomplete header%s\n",
	   (int)udpstat.udps_hdrops, plural(udpstat.udps_hdrops));
    printf("\t%u bad data length field%s\n",
	   (int)udpstat.udps_badlen, plural(udpstat.udps_badlen));
    printf("\t%u bad checksum%s\n",
	   (int)udpstat.udps_badsum, plural(udpstat.udps_badsum));
    printf("\t%u broadcasts received with no ports\n",
	   (int)udpstat.udps_noportbcast);
    printf("\t%u full socket%s\n",
	   (int)udpstat.udps_fullsock, plural(udpstat.udps_fullsock));
    printf("\t%u pcb cache lookup%s failed\n",
	   (int)udpstat.udpps_pcbcachemiss,
           plural(udpstat.udpps_pcbcachemiss));
    printf("\t%u pcb hash lookup%s failed\n",
	   (int)udpstat.udpps_pcbhashmiss, plural(udpstat.udpps_pcbhashmiss));
    }


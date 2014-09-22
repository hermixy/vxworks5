/* igmpShow.c - IGMP information display routines */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,10may02,kbw  making manpage edits
01e,15oct01,rae  merge from truestack ver 01f, base 01d 
01d,14dec97,kbw  made man page edits
01c,31jul97,kbw  fixed man page problems found in beta review
01b,20apr97,kbw  fixed man page format, spell check.
01a,08mar97,vin  written.
*/

/*
DESCRIPTION
This library provides routines to show IGMP related
statistics.

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
The igmpShowInit() routine links the IGMP show facility into the VxWorks
system.  This is performed automatically if INCLUDE_NET_SHOW and
INCLUDE_IGMP are defined.

SEE ALSO: netLib, netShow
*/

#include "vxWorks.h"
#include "sys/types.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "netinet/igmp_var.h"
#include "errno.h"
#include "string.h"
#include "stdio.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsIgmp.h"
#endif

#define plural(num)	((num) > 1 ? "s": "")

#ifndef VIRTUAL_STACK
extern struct igmpstat igmpstat;
#endif /* VIRTUAL_STACK */

/******************************************************************************
*
* igmpShowInit - initialize IGMP show routines
*
* This routine links the IGMP show facility into the VxWorks system.
* These routines are included automatically if INCLUDE_NET_SHOW
* and INCLUDE_IGMP are defined.
*
* RETURNS: N/A
*/

void igmpShowInit (void)
    {
    
    }

/*******************************************************************************
*
* igmpstatShow - display statistics for IGMP
*
* This routine displays statistics for the IGMP 
* (Internet Group Management Protocol) protocol.
*
* RETURNS: N/A
*/

void igmpstatShow (void)
    {

    printf ("IGMP:\n\t%u invalid queries received\n",
            (int)igmpstat.igps_rcv_badqueries);
    printf ("\t%u invalid reports received\n",
            (int)igmpstat.igps_rcv_badreports);
    printf("\t%u bad checksums received\n", (int)igmpstat.igps_rcv_badsum);
    printf("\t%u reports for local groups received\n",
           (int)igmpstat.igps_rcv_ourreports);
    printf("\t%u membership queries received\n",
           (int)igmpstat.igps_rcv_queries);
    printf("\t%u membership reports received\n",
           (int)igmpstat.igps_rcv_reports);
    printf("\t%u short packets received\n", (int)igmpstat.igps_rcv_tooshort);
    printf("\t%u total messages received\n", (int)igmpstat.igps_rcv_total);
    printf("\t%u membership reports sent\n", (int)igmpstat.igps_snd_reports);
    
    }

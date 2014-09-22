/* icmpShow.c - ICMP Information display routines */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,10may02,kbw  making man page edits
01e,15oct01,rae  merge from truestack ver 01g, base o1c (VIRTUAL_STACK)
01d,07feb01,spm  added internal documentation for parallel host shell routines
01c,31jul97,kbw  fixed man page problems found in beta review
01b,20apr97,kbw  fixed man page format, spell check.
01a,08mar97,vin  written.
*/

/*
DESCRIPTION
This library provides routines to show ICMP related
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
The icmpShowInit() routine links the ICMP show facility into the VxWorks
system.  This is performed automatically if INCLUDE_NET_SHOW is defined.


INTERNAL
The icmpstatShow routine in this module is actually preempted by a
parallel version built in to the Tornado shell source code in the
$WIND_BASE/host/src/windsh directory. Changes to this routine must
be duplicated within that shell to maintain consistent output. The
version in this module can still be accessed by prepending an "@" sign
to the routine name (i.e. @icmpstatShow).

SEE ALSO: netLib, netShow
*/

#include "vxWorks.h"
#include "sys/types.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "netinet/ip_var.h"
#include "netinet/ip_icmp.h"
#include "netinet/icmp_var.h"
#include "errno.h"
#include "string.h"
#include "stdio.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif /* VIRTUAL_STACK */

#define plural(num)	((num) > 1 ? "s": "")

/******************************************************************************
*
* icmpShowInit - initialize ICMP show routines
*
* This routine links the ICMP show facility into the VxWorks system.
* These routines are included automatically if INCLUDE_NET_SHOW
* and INCLUDE_ICMP are defined.
*
* RETURNS: N/A
*/

void icmpShowInit (void)
    {
    
    }

/*******************************************************************************
*
* icmpstatShow - display statistics for ICMP
*
* This routine displays statistics for the ICMP 
* (Internet Control Message Protocol) protocol.
*
* RETURNS: N/A
*
* INTERNAL
* When using the Tornado shell, this routine is only available if an
* "@" sign is prepended to the routine name. Otherwise, it is preempted
* by a built-in version.
*/

void icmpstatShow (void)
    {
    static char *icmpNames[] =
    {
	"echo reply",
	"#1",
	"#2",
	"destination unreachable",
	"source quench",
	"routing redirect",
	"#6",
	"#7",
	"echo",
	"#9",
	"#10",
	"time exceeded",
	"parameter problem",
	"time stamp",
	"time stamp reply",
	"information request",
	"information request reply",
	"address mask request",
	"address mask reply",
    };

    FAST int ix;
    FAST int first;

#ifdef VIRTUAL_STACK
    printf("ICMP: (stack number %d)\n\t%u call%s to icmp_error\n", myStackNum,
	   (int)_icmpstat.icps_error, plural(_icmpstat.icps_error));
#else
    printf("ICMP:\n\t%u call%s to icmp_error\n",
	   (int)icmpstat.icps_error, plural(icmpstat.icps_error));
#endif /* VIRTUAL_STACK */

    printf("\t%u error%s not generated because old message was icmp\n",
#ifdef VIRTUAL_STACK
	   (int)_icmpstat.icps_oldicmp, plural(_icmpstat.icps_oldicmp));
#else
	   (int)icmpstat.icps_oldicmp, plural(icmpstat.icps_oldicmp));
#endif /* VIRTUAL_STACK */

    for (first = 1, ix = 0; ix < ICMP_MAXTYPE + 1; ix++)
	{
#ifdef VIRTUAL_STACK
	if (_icmpstat.icps_outhist [ix] != 0)
#else
	if (icmpstat.icps_outhist [ix] != 0)
#endif /* VIRTUAL_STACK */
	    {
	    if (first)
		{
		printf("\tOutput histogram:\n");
		first = 0;
		}

	    printf("\t\t%s: %u\n", icmpNames [ix],
#ifdef VIRTUAL_STACK
		   (int)_icmpstat.icps_outhist [ix]);
#else
		   (int)icmpstat.icps_outhist [ix]);
#endif /* VIRTUAL_STACK */
	    }
	}

    printf("\t%u message%s with bad code fields\n",
#ifdef VIRTUAL_STACK
	   (int)_icmpstat.icps_badcode, plural(_icmpstat.icps_badcode));
#else
	   (int)icmpstat.icps_badcode, plural(icmpstat.icps_badcode));
#endif /* VIRTUAL_STACK */

    printf("\t%u message%s < minimum length\n",
#ifdef VIRTUAL_STACK
	   (int)_icmpstat.icps_tooshort, plural(_icmpstat.icps_tooshort));
#else
	   (int)icmpstat.icps_tooshort, plural(icmpstat.icps_tooshort));
#endif /* VIRTUAL_STACK */

    printf("\t%u bad checksum%s\n",
#ifdef VIRTUAL_STACK
	   (int)_icmpstat.icps_checksum, plural(_icmpstat.icps_checksum));
#else
	   (int)icmpstat.icps_checksum, plural(icmpstat.icps_checksum));
#endif /* VIRTUAL_STACK */

    printf("\t%u message%s with bad length\n",
#ifdef VIRTUAL_STACK
	   (int)_icmpstat.icps_badlen, plural(_icmpstat.icps_badlen));
#else
	   (int)icmpstat.icps_badlen, plural(icmpstat.icps_badlen));
#endif /* VIRTUAL_STACK */

    for (first = 1, ix = 0; ix < ICMP_MAXTYPE + 1; ix++)
	{
#ifdef VIRTUAL_STACK
	if (_icmpstat.icps_inhist [ix] != 0)
#else
	if (icmpstat.icps_inhist [ix] != 0)
#endif /* VIRTUAL_STACK */
	    {
	    if (first)
		{
		printf("\tInput histogram:\n");
		first = 0;
		}

	    printf("\t\t%s: %u\n", icmpNames [ix],
#ifdef VIRTUAL_STACK
		   (int)_icmpstat.icps_inhist [ix]);
#else
		   (int)icmpstat.icps_inhist [ix]);
#endif /* VIRTUAL_STACK */
	    }
	}

    printf("\t%u message response%s generated\n",
#ifdef VIRTUAL_STACK
	   (int)_icmpstat.icps_reflect, plural(_icmpstat.icps_reflect));
#else
	   (int)icmpstat.icps_reflect, plural(icmpstat.icps_reflect));
#endif /* VIRTUAL_STACK */
    }

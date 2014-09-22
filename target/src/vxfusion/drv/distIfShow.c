/* distIfShow.c - distributed objects interface adapter show routines (VxFusion option) */

/* Copyright 1999-2002 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,26oct01,jws  man page clean-up (SPR 71239)
01f,24may99,drm  added vxfusion prefix to VxFusion related includes
01e,23feb99,wlf  doc edits
01d,18feb99,wlf  doc cleanup
01c,11feb99,drm  changed discared to discarded re: SPR #24699
01b,28oct98,drm  documentation updates
01a,07oct97,ur   written.
*/

/*
DESCRIPTION
This library provides a show routine for displaying information about the 
installed interface adapter.

AVAILABILITY
This module is distributed as a component of the unbundled distributed
message queues option, VxFusion.

INCLUDE FILES: distIfLib.h

SEE ALSO: distStatLib
*/

#include "vxWorks.h"
#include "stdio.h"
#include "vxfusion/distIfLib.h"
#include "vxfusion/distStatLib.h"

/***************************************************************************
*
* distIfShowInit - initialize the interface show routine (VxFusion option)
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

void distIfShowInit (void)
    {
    }

/***************************************************************************
*
* distIfShow - display information about the installed interface adapter (VxFusion option)
*
* This routine displays information about the installed interface adapter.
* It displays the configuration parameters, as well as some statistical 
* data.
*      
* EXAMPLE:
* \cs
* -> distIfShow
* Interface Name                 : "UDP adapter"
* MTU                            : 1500
* Network Header Size            : 14
* SWP Buffer                     : 32
* Maximum Number of Fragments    : 10
* Maximum Length of Packet       : 14860 
* Broadcast Address              : 0x930b26ff
* 
* Telegrams received             : 23
* Telegrams received for sending : 62
* Incoming Telegrams discarded   : 0
* Outgoing Telegrams discarded   : 0
* \ce
*
* In this example, the installed interface adapter has the name "UDP 
* adapter."  The largest telegram that can be transmitted without
* fragmentation is 1500 bytes long. The network header requires fourteen
* (14) of those bytes; therefore the largest amount of user data that can be
* transmitted without fragmentation is 1486 bytes.  The sliding window
* protocol's buffer has 32 entries, which results in a window of size 16.
* The number of fragments that the packet can be broken into is limited by
* the size of the sequence field in the network header.  The example
* interface adapter can handle up to 10 fragments, which results in a
* maximum packet length of 14860 ((1500 - 14) * 128) bytes. The broadcast
* address of this driver is 0x930b26ff (147.11.38.255). The last four lines
* of output show statistical data.
*
* AVAILABILITY
* This routine is distributed as a component of the unbundled distributed
* message queues option, VxFusion.
*
* RETURNS: OK, or ERROR if there is no interface installed.
*/

STATUS distIfShow (void)
    {
    if (pDistIf == NULL)
        {
        printf ("no interface installed\n");
        return (ERROR);
        }

    printf ("Interface Name                 : \"%s\"\n", DIST_IF_NAME);
    printf ("MTU                            : %d\n", DIST_IF_MTU);
    printf ("Network Header Size            : %d\n", DIST_IF_HDR_SZ);
    printf ("SWP Buffer                     : %d\n", DIST_IF_RNG_BUF_SZ);
    printf ("Maximum Number of Fragments    : %d\n", DIST_IF_MAX_FRAGS);
    printf ("Maximum Length of Packet       : %d\n",
            DIST_IF_MAX_FRAGS * (DIST_IF_MTU - DIST_IF_HDR_SZ));
    printf ("Broadcast Address              : 0x%lx\n",
            (u_long) DIST_IF_BROADCAST_ADDR);

    printf ("\n");
    printf ("Telegrams received             : %ld\n", distStat.ifInReceived);
    printf ("Telegrams received for sending : %ld\n", distStat.ifOutReceived);
    printf ("Incoming Telegrams discarded   : %ld\n", distStat.ifInDiscarded);
    printf ("Outgoing Telegrams discarded   : %ld\n", distStat.ifOutDiscarded);

    return (OK);
    }


/* mCastRouteLib.c - multicast routing interface library */

/* Copyright 1984-1997 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01a,08apr97,vin written
*/

/*
DESCRIPTION
This library contains the interface to the multicast routing.

The routine mCastRouteLibInit() is responsible for configuring the
multicast routing interface.

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
#include "netinet/ip_mroute.h"

/* externs */
IMPORT FUNCPTR _mCastRouteCmdHook;	/* mcast router command Hook */

/* globals */

/* defines */

/* typedefs */

/* locals */

STATUS mCastRouteLibInit (void)
    {
    if (_mCastRouteCmdHook != NULL)		/* already configured */
        return (OK);

    _mCastRouteCmdHook = ip_mrouter_cmd;	/* mcast router command hook */
    
    return (OK); 
    }

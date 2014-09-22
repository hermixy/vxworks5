/* routeSockLib.c - routing socket interface library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01b,26jun01,jpf virtualized rtSockLibInitialized
01a,01jul97,vin written
*/

/*
DESCRIPTION
This library contains the interface to the routing sockets. The routing
socket interface is configured in this library. 

The routine routeSockLibInit() is responsible for configuring the routing
socket interface. 

INCLUDE FILES: netLib.h

.pG "Network"

NOMANUAL
*/

/* includes */

#include "vxWorks.h"
#include "netLib.h"
#include "net/protosw.h"
#include "net/domain.h"

/* externs */

IMPORT struct domain  routedomain;

/* globals */

/* defines */

/* typedefs */

/* locals */

#ifndef VIRTUAL_STACK
BOOL	rtSockLibInitialized	= FALSE;	/* boolean variable */
#else
#include "netinet/vsLib.h"
#endif /* VIRTUAL_STACK */

/*******************************************************************************
* routeSockLibInit - intialize the routing socket interface
*
* This function intializes the routing socket interface. It initializes
* the routing domain and then intializes the protocol switch. This function
* should be called only at network initialization time, after domaininit()
* and route_init(). There is only one protocol in the protocol switch to
* enable routing sockets.
*
* RETURNS: OK/ERROR
*
* NOMANUAL
*/

STATUS routeSockLibInit (void)
    {
    if (rtSockLibInitialized == TRUE)
        return (OK); 			/* already initialized */

    addDomain (&routedomain);		/* add routing domain */
    
    rtSockLibInitialized = TRUE;
    
    return (OK); 
    }

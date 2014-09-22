/* dhcpc.c - DHCP client runtime finite state machine definition */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01i,12oct01,rae  fixed modhist line
01h,24oct00,spm  fixed modification history after tor3_x merge
01g,23oct00,niq  merged from version 01g of tor3_x branch (base version 01f)
01f,26aug97,spm  major overhaul: reorganized code and changed user interface
                 to support multiple leases at runtime
01e,06aug97,spm  removed parameters linked list to reduce memory required
01d,18apr97,spm  added conditional include DHCPC_DEBUG for displayed output
01c,07apr97,spm  modified control function to return ERROR, changed docs 
01b,29jan97,spm  removed all functions except state machine definition 
01a,03oct96,spm  created by modifying WIDE project DHCP Implementation 
*/

/*
DESCRIPTION
This library contains the control function for the DHCP client finite
state machine when executing during runtime.

INCLUDE_FILES: dhcpcLib.h
*/

/*
 * WIDE Project DHCP Implementation
 * Copyright (c) 1995 Akihiro Tominaga
 * Copyright (c) 1995 WIDE Project
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided only with the following
 * conditions are satisfied:
 *
 * 1. Both the copyright notice and this permission notice appear in
 *    all copies of the software, derivative works or modified versions,
 *    and any portions thereof, and that both notices appear in
 *    supporting documentation.
 * 2. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by WIDE Project and
 *      its contributors.
 * 3. Neither the name of WIDE Project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPER ``AS IS'' AND WIDE
 * PROJECT DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE. ALSO, THERE
 * IS NO WARRANTY IMPLIED OR OTHERWISE, NOR IS SUPPORT PROVIDED.
 *
 * Feedback of the results generated from any improvements or
 * extensions made to this software would be much appreciated.
 * Any such feedback should be sent to:
 * 
 *  Akihiro Tominaga
 *  WIDE Project
 *  Keio University, Endo 5322, Kanagawa, Japan
 *  (E-mail: dhcp-dist@wide.ad.jp)
 *
 * WIDE project has the rights to redistribute these changes.
 */

/* includes */

#include "vxWorks.h"
#include "logLib.h"

#include "stdio.h"

#include "stdlib.h"
#include "netinet/in.h"
#include "netinet/udp.h"
#include "netinet/ip.h"
#include "netinet/if_ether.h"

#include "dhcp/dhcpcCommonLib.h"
#include "dhcp/dhcpcInternal.h"
#include "dhcp/dhcpcStateLib.h"

/*******************************************************************************
*
* dhcp_client_setup - initialize WIDE project data structures for client
*
* This routine initializes the WIDE project data structures used by all leases.
* It assigns functions to handle each of the states in the DHCP client's finite
* state machine, and invokes the initialize() routine to set up the structures
* used for network data transfer. It is called by the dhcpcLibInit() routine
* during the overall initialization of the DHCP client library.
*
* RETURNS: OK if setup completed, or ERROR otherwise.
*
* ERRNO: N/A
*
* NOMANUAL
*/

STATUS dhcp_client_setup 
    (
    int 	serverPort, 	/* port used by DHCP servers */
    int 	clientPort, 	/* port used by DHCP clients */
    int 	bufSize 	/* maximum size of any DHCP message */
    )
    {
    int retval = 0;

    /* Assign routines for processing client's finite state machine. */

    fsm [INIT] = init;
    fsm [WAIT_OFFER] = wait_offer;
    fsm [SELECTING] = selecting;
    fsm [REQUESTING] = requesting;
    fsm [BOUND] = bound;
    fsm [RENEWING] = renewing;
    fsm [REBINDING] = rebinding;
    fsm [INIT_REBOOT] = init_reboot;
    fsm [VERIFY] = verify;
    fsm [REBOOTING] = reboot_verify;
    fsm [VERIFYING] = reboot_verify;
    fsm [INFORMING] = inform;

    /* Create data used for message transfer by all leases. */

    if ( (retval = initialize (serverPort, clientPort, bufSize)) < 0)
        return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* dhcp_client - Exercise client finite state machine
*
* This routine triggers the finite state machine to obtain a set of
* configration parameters. It should only be called internally.
*
* The <pCookie> argument indicates the handle which accesses any
* parameters from the API. The <bindFlag> argument indicates whether
* the dhcpcBind() or dhcpcInformGet() routine starts the process. The
* first routine negotiates a lease with a server. The second only
* attempts to obtain additional parameters when an address is already
* available and does not create a lease.
* .IP
* The message exchanges will be performed synchronously or asynchronously,
* depending on the setting of the waitFlag parameter. If that flag is set,
* the routine executes synchronously by waiting until the process completes,
* then returning the results to the caller. This situation always occurs when
* renewing a lease obtained during system boot. The client library always
* applies any configuration parameters associated with that lease to the
* network interface used for transmission.
* .IP 
* When starting a lease negotiation, this routine adds a bind request to
* the event ring which is monitored by the client monitor task. A similar
* request starts the shorter message exchange to retrieve additional
* parameters. Once either process completes, any event hook registered for
* the corresponding handle will be invoked by the state machine to allow
* application-specific processing of any parameters obtained.
* .IP
* If automatic configuration was requested during the lease initialization, 
* the address settings of the transmission network interface will change 
* without warning throughout the lifetime of the lease. In particular, the 
* initial state of the client's finite state machine will reset that interface 
* to use a reserved IP address, preventing access from any other source.
* Notification after the settings are changed will only be available if an
* event hook is installed for the lease. 
*
* RETURNS: 0 if processing completed, or value of failed state on error.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int dhcp_client
    (
    void * 	pCookie, 	/* parameter handle from dhcpcInit() routine */
    BOOL 	bindFlag 	/* Perform lease negotiation or inform only? */
    )
    {
    LEASE_DATA * 	pLeaseData;
    BOOL 		syncFlag;
    STATUS 		result;
    int 		eventType;

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pCookie;

    /* Set up variables used on a lease-specific basis. */

         /* Reseed random number generator used for retransmission backoffs. */

    srand (generate_xid (&pLeaseData->ifData));  

    syncFlag = pLeaseData->waitFlag; /* Assign lease synchronization status. */

    if (bindFlag)
        {
        /* Full lease negotiation process. */

        if (pLeaseData->autoConfig || pLeaseData->leaseType == DHCP_AUTOMATIC)
            {
            /* 
             * Disable network interface until negotiation completes
             * if it used the address information provided by the lease. 
             */

            reset_if (&pLeaseData->ifData);
            }

        /*
         * The INIT_REBOOT state attempts to verify a current lease, if any.
         * Otherwise, it proceeds to the INIT state.
         */

        pLeaseData->prevState = pLeaseData->currState = INIT_REBOOT;

        eventType = DHCP_USER_BIND;
        }
    else
        {
        /*
         * Partial message exchange for known (externally assigned) address.
         * Send an INFORM message and wait for an acknowledgement containing
         * additional parameters.
         */

        pLeaseData->prevState = pLeaseData->currState = INFORMING;

        eventType = DHCP_USER_INFORM;
        }

    /* Add an event to the message queue to start the requested process. */

    result = dhcpcEventAdd (DHCP_USER_EVENT, eventType, pCookie, FALSE);
    if (result == ERROR)
        return (-1);

    /* For synchronous message exchanges, wait for the completion signal. */

    if (syncFlag)
        semTake (pLeaseData->leaseSem, WAIT_FOREVER);

    return (0);   /* Request successful. */
    }

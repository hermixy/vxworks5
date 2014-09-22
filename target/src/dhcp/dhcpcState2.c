/* dhcpcState2.c - DHCP client runtime state machine (lease maintenance) */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01t,25apr02,wap  correct timeout calculations for RENEWING & REBINDING states
                 (SPR #69069)
01s,23apr02,wap  use dhcpTime() instead of time() (SPR #68900)
01r,07mar02,wap  Remember to re-enable the BPF filter and set a watchdog
                 timeout when we enter the REBINDING state in bound()
                 (SPR #73243)
01q,06dec01,wap  Fix reboot_verify() so that it properly constructs retransmit
                 request (make_request() called with wrong type) (SPR #70938)
01p,12oct01,rae  merge from truestack (SPRs 67822, 27426, 30344)
01o,24oct00,spm  fixed modification history after tor3_x merge
01n,23oct00,niq  merged from version 01r of tor3_x branch (base version 01m)
01m,04dec97,spm  added code review modifications
01l,06oct97,spm  removed reference to deleted endDriver global
01k,03sep97,spm  added specified minimum timeouts to lease reacquisition
01j,02sep97,spm  removed excess IMPORT statement and extra event hook parameter
01i,26aug97,spm  major overhaul: reorganized code and changed user interface
                 to support multiple leases at runtime
01h,06aug97,spm  removed parameters linked list to reduce memory required
01g,15jul97,spm  replaced floating point to prevent ss5 exception (SPR #8738);
                 removed unneeded checkpoint messages
01f,10jun97,spm  isolated incoming messages in state machine from input hooks
01e,02jun97,spm  changed DHCP option tags to prevent name conflicts (SPR #8667)
                 and updated man pages
01d,06may97,spm  changed memory access to align IP header on four byte boundary
01c,18apr97,spm  added conditional include DHCPC_DEBUG for displayed output
01b,07apr97,spm  added code to use Host Requirements defaults and cleanup
                 memory on exit, rewrote documentation
01a,29jan97,spm  extracted from dhcpc.c to reduce object size
*/

/*
DESCRIPTION
This library contains a portion of the finite state machine for the WIDE 
project DHCP client, modified for vxWorks compatibility.

INTERNAL
This module contains the functions used during and after the BOUND state. 
It was created to reduce the size of the boot ROM image so that the DHCP client
can be used with targets like the MV147 which have limited ROM capacity. When 
executing at boot time, the DHCP client does not use any of the routines 
defined in this module.

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include "vxWorks.h"
#include "ioLib.h" 	/* ioctl declaration */
#include "rngLib.h"
#include "wdLib.h"
#include "time.h"
#include "inetLib.h"
#include "logLib.h"
#include "taskLib.h"
#include "sysLib.h"
#include "vxLib.h"
#include "netLib.h"

#include "sys/ioctl.h"
#include "net/bpf.h"

#include "dhcp/dhcpcCommonLib.h"
#include "dhcp/dhcpcStateLib.h"
#include "dhcp/dhcpcInternal.h"

/* defines */

/* Retransmission delay is timer value plus/minus one second (RFC 1541). */

#define	SLEEP_RANDOM(timer) ( (timer - 1) + (rand () % 2) )
#define REQUEST_RETRANS   4  /* Max number of retransmissions. (RFC 1541). */

/* globals */

IMPORT SEM_ID	dhcpcMutexSem;		/* Protects status indicator */
IMPORT struct dhcpLeaseData dhcpBootLease;	/* Boot lease time / address */

/*******************************************************************************
*
* use_parameter - reset the network according to the parameters provided
*
* If no event hook is present for the lease described by <pLeaseData>, this 
* routine sets the network address, broadcast address and subnet mask for 
* a network interface to the values chosen by the DHCP client and acknowledged 
* by the offering DHCP server. The configuration of the network interface is
* also changed automatically for any lease established during system startup,
* whether or not an event hook is present. This routine calls any installed 
* event notification hook to indicate that new lease parameters are available.
*
* RETURNS: 0 if setup completed, or 1 if error occurs.
*
* ERRNO: N/A
* 
* NOMANUAL
*/

int use_parameter
    (
    struct dhcp_param *	paramp, 	/* Current DHCP parameters */
    LEASE_DATA * 	pLeaseData 	/* lease-specific status information */
    )
    {
    struct in_addr addr;
    struct in_addr mask; 
    struct in_addr brdaddr;
    int status = 0;
    int length;
    char * 	bufp;
    void * 	pCookie;

    /*
     * For now, use the address of the lease-specific data structure as the
     * internal lease identifier. This could be replaced with a more 
     * sophisticated mapping if necessary.
     */

    pCookie = (void *)pLeaseData;

    if (pLeaseData->cacheHookRtn != NULL)
        {
#ifdef DHCPC_DEBUG
        logMsg ("Saving lease data.\n", 0, 0, 0, 0, 0, 0);
#endif

#if BSD<44
        length = ntohs (dhcpcMsgIn.udp->uh_ulen) +
                     (dhcpcMsgIn.ip->ip_v_hl & 0xff) * WORD;
#else
        length = ntohs (dhcpcMsgIn.udp->uh_ulen) +
                     dhcpcMsgIn.ip->ip_hl * WORD;
#endif
        bufp = (char *)dhcpcMsgIn.ip;
        (* pLeaseData->cacheHookRtn) (DHCP_CACHE_WRITE,
                                      &pLeaseData->dhcpcParam->lease_origin, 
                                      &length, bufp);
        }

    if (pLeaseData->autoConfig || pLeaseData->leaseType == DHCP_AUTOMATIC)
        {
        /* 
         * If automatic configuration was requested or the lease was 
         * established during system startup, the client library will 
         * reconfigure the transmit/receive interface to use the retrieved 
         * addressing information. Fetch the values for the interface address, 
         * subnet mask, and broadcast address, if available.
         */

         bzero ( (char *)&addr, sizeof (struct in_addr));
         bzero ( (char *)&mask, sizeof (struct in_addr));
         bzero ( (char *)&brdaddr, sizeof (struct in_addr));
         addr.s_addr = paramp->yiaddr.s_addr;

         /* Set subnet mask, if available. */

         if (paramp->subnet_mask != NULL) 
             mask.s_addr = paramp->subnet_mask->s_addr;
         else 
             mask.s_addr = 0;

         /* Set broadcast address, if available. */

         if (paramp->brdcast_addr != NULL)
             brdaddr.s_addr = paramp->brdcast_addr->s_addr;
         else 
             brdaddr.s_addr = 0;
         }

    /* 
     * Set the transmit/receive interface to use the new parameters.
     */

    if (pLeaseData->autoConfig || pLeaseData->leaseType == DHCP_AUTOMATIC)
        {
        /* Set new address info. Returns 1 if address unchanged.  */

        status = config_if (&pLeaseData->ifData, &addr, 
                            ( (mask.s_addr != 0) ? &mask : NULL),
                            ( (brdaddr.s_addr != 0) ? &brdaddr : NULL));
        if (status == 0)   
            set_route (paramp);
        else if (status == -1)    /* Error. */
            return (1);

        /* Send an ARP reply to update the ARP cache on other hosts. */

        arp_reply (&paramp->yiaddr, &pLeaseData->ifData);
        }

    /*
     * If an event notification hook is present, send an
     * indication that a new set of parameters is available.
     */

    if (pLeaseData->eventHookRtn != NULL)
        (* pLeaseData->eventHookRtn) (DHCPC_LEASE_NEW, pCookie);

    return (0);
    }

/*******************************************************************************
*
* release - relinquish a DHCP lease
*
* This routine sends a message to the DHCP server relinquishing the active
* lease contained in the <pEvent> event descriptor. If <shutdownFlag> is TRUE,
* the routine cleans up all lease-specific data structures because the user
* issued a dhcpcRelease() or dhcpcShutdown() call. Otherwise, the routine is
* handling a fatal error in the state machine and the lease-specific data is
* retained to allow retries of the dhcpcBind() or dhcpcInformGet() operation.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void release
    (
    LEASE_DATA * 	pLeaseData, 	/* lease-specific status information */
    BOOL 		shutdownFlag 	/* remove lease-specific data? */
    )
    {
    char 		errmsg[255];
    struct dhcp_reqspec tmp_reqspec;
    int			 boundstat;

    bzero ( (char *)&tmp_reqspec, sizeof (tmp_reqspec));
    bzero ( (char *)&errmsg, sizeof (errmsg));

    semTake (dhcpcMutexSem, WAIT_FOREVER);    /* Reset status indicator. */
    if (pLeaseData->leaseGood)
        {
        boundstat = 1;
        pLeaseData->leaseGood = FALSE;
        }
    else                     /* Not bound - don't send release message. */
        boundstat = 0;
    semGive (dhcpcMutexSem);

    if (boundstat)
        {
        switch (pLeaseData->currState) 
            {
            case BOUND:		/* fall-through */
            case RENEWING:	/* fall-through */
            case REBINDING:	/* fall-through */
            case VERIFY:	/* fall-through */
            case VERIFYING:	/* fall-through */
                if (pLeaseData->dhcpcParam != NULL) 
                    {
                    set_relinfo (&tmp_reqspec, pLeaseData, errmsg);
                    dhcp_release (&tmp_reqspec, &pLeaseData->ifData,
                                  pLeaseData->oldFlag);

                    /* 
                     * Disable the underlying network interface if
                     * it used the (soon-to-be) relinquished lease. 
                     */

                    if (pLeaseData->autoConfig || 
                            pLeaseData->leaseType == DHCP_AUTOMATIC)
                        down_if (&pLeaseData->ifData);
 
                    if (pLeaseData->cacheHookRtn != NULL)
                        {
                        (* pLeaseData->cacheHookRtn) (DHCP_CACHE_ERASE, NULL, 
                                                      NULL, NULL);
                        }
                    clean_param (pLeaseData->dhcpcParam);
                    free (pLeaseData->dhcpcParam);
                    pLeaseData->dhcpcParam = NULL;
                    }
                break;
            default:
                break;
            }
        }

    /* Remove lease information if appropriate. */

    if (shutdownFlag)
        dhcpcLeaseCleanup (pLeaseData);
        
    return;
    }

/*******************************************************************************
*
* alarm_bound - timeout during bound state
*
* This routine sends a timeout notification to the client monitor task when 
* a lease timer expires. It is called at interrupt level by a watchdog timer.
* The monitor task will eventually advance the lease from the BOUND state to
* the RENEWING or the REBINDING state, depending on which interval elapsed.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void alarm_bound
    (
    LEASE_DATA * 	pLeaseData 	/* lease-specific status information */
    )
    {
    /*
     * Ignore the timeout if a state transition occurred
     * before processing completed.
     */

    if (pLeaseData->currState != BOUND)
        return;

    /* Construct and send a timeout message to the lease monitor task. */

    dhcpcEventAdd (DHCP_AUTO_EVENT, DHCP_TIMEOUT, pLeaseData, TRUE);

    return;
    }

/*******************************************************************************
*
* retrans_renewing - signal when reception interval for renewal reply expires
*
* This routine sends a timeout notification to the client monitor task when 
* the interval for receiving a server reply to a renewal request expires. It
* is called at interrupt level by a watchdog timer. The monitor task will 
* eventually execute the RENEWING state to process the timeout event and
* retransmit the DHCP request message.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void retrans_renewing
    (
    LEASE_DATA * 	pLeaseData 	/* lease-specific status information */
    )
    {
    /*
     * Ignore the timeout if a state transition occurred
     * before processing completed.
     */

    if (pLeaseData->currState != RENEWING)
        return;

    /* Construct and send a timeout message to the lease monitor task. */

    dhcpcEventAdd (DHCP_AUTO_EVENT, DHCP_TIMEOUT, pLeaseData, TRUE);

    return;
    }

/*******************************************************************************
*
* retrans_rebinding - signal when reception interval for rebind offer expires
*
* This routine sends a timeout notification to the client monitor task when 
* the interval for receiving a lease offer from a different server expires. It
* is called at interrupt level by a watchdog timer. The monitor task will 
* eventually execute the REBINDING state to process the timeout event and
* retransmit the DHCP discover message.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void retrans_rebinding
    (
    LEASE_DATA * 	pLeaseData 	/* lease-specific status information */
    )
    {
    /*
     * Ignore the timeout if a state transition occurred
     * before processing completed.
     */

    if (pLeaseData->currState != REBINDING)
        return;

    /* Construct and send a timeout message to the lease monitor task. */

    dhcpcEventAdd (DHCP_AUTO_EVENT, DHCP_TIMEOUT, pLeaseData, TRUE);

    return;
    }

/*******************************************************************************
*
* retrans_reboot_verify - retransmission in rebooting state
*
* This routine signals the DHCP client monitor task when the timeout interval 
* for receiving a server reply expires for the lease indicated by the 
* <pLeaseData> parameter. The client monitor task will pass the event to the 
* reboot_verify() routine in the finite state machine.
* retrans_reboot_verify - signal when verification reception interval expires
*
* This routine sends a timeout notification to the client monitor task when 
* the interval for receiving a server reply to a reboot (or verify) request 
* expires. It is called at interrupt level by a watchdog timer. The monitor 
* task will eventually execute the REBOOTING or VERIFYING state to process 
* the timeout event and retransmit the appropriate DHCP request message.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void retrans_reboot_verify
    (
    LEASE_DATA * 	pLeaseData 	/* lease-specific status information */
    )
    {
    /*
     * Ignore the timeout if a state transition occurred
     * before processing completed.
     */

    if (pLeaseData->currState != REBOOTING &&
            pLeaseData->currState != VERIFYING)
        return;

    /* Construct and send a timeout message to the lease monitor task. */

    dhcpcEventAdd (DHCP_AUTO_EVENT, DHCP_TIMEOUT, pLeaseData, TRUE);

    return;
    }

/*******************************************************************************
*
* bound - Active state of client finite state machine
*
* This routine contains the fourth state of the client finite state machine.
* It accepts and services all user requests for BOOTP and infinite DHCP leases
* throughout their lifetimes. For finite DHCP leases, it handles all processing
* until one of the lease timers expires. At that point, processing continues 
* with the renewing() or rebinding() routines.
* .IP
* This routine is invoked by the event handler of the client monitor task,
* and should only be called internally. Any arriving DHCP messages contain
* stale offers or responses and are discarded. User requests generated
* by calls to the dhcpcRelease() and dhcpcVerify() routines are accepted.
*
* RETURNS: OK (processing complete), DHCPC_DONE (remove lease),
*          DHCPC_MORE (continue), or ERROR.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int bound
    (
    EVENT_DATA * 	pEvent 	/* pointer to event descriptor */
    )
    {
    int timeout;    /* Epoch when intermediate lease timers expire. */
    int limit;      /* Epoch when lease expires. */
   
    LEASE_DATA * 	pLeaseData;
    int status;
    int length;

    time_t curr_epoch = 0;
    struct sockaddr_in dest;
    struct ifnet * pIf;
    struct ifreq   ifr;

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pEvent->leaseId;

    /* 
     * Process events for BOOTP and infinite DHCP leases. Timeouts and
     * incoming messages should never occur for either type of lease. Also,
     * BOOTP leases require no special processing in response to user requests
     * for verification or shutdown.
     */
     
    if (pLeaseData->dhcpcParam->lease_duration == ~0)
        {
        /* Ignore timeouts and received DHCP messages. */

        if (pEvent->source != DHCP_AUTO_EVENT) 
            {
            /* Process user requests. */

            if (pEvent->type == DHCP_USER_RELEASE)    /* Relinquish lease. */
                {
                if (pLeaseData->leaseType != DHCP_BOOTP)
                    release (pLeaseData, TRUE);
                return (DHCPC_DONE);
                }
            if (pEvent->type == DHCP_USER_VERIFY)    /* Verify lease. */
                {
                if (pLeaseData->leaseType != DHCP_BOOTP)
                    {
                    /* Set the lease data to execute verify() routine. */

                    pLeaseData->prevState = BOUND;
                    pLeaseData->currState = VERIFY;

                    return (DHCPC_MORE);
                    }
                }
            }
        return (OK);
        }

    /* The rest of the routine handles processing for finite DHCP leases. */

    bzero (sbuf.buf, sbuf.size);

    if (dhcpTime (&curr_epoch) == -1) 
        {
#ifdef DHCPC_DEBUG
        logMsg ("time() error in bound()\n", 0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }

    if (pLeaseData->prevState != BOUND)
        {
        /* 
         * The interval between lease acknowledgement and the entry to the 
         * bound() routine depends on the workload of the client monitor task 
         * and is completely unpredictable. Check all lease timers, since they 
         * might have expired during that time.
         */

        wdCancel (pLeaseData->timer);

#ifdef DHCPC_DEBUG
        logMsg ("dhcpc: Entering BOUND state.\n", 0, 0, 0, 0, 0, 0);
#endif
        timeout = pLeaseData->dhcpcParam->lease_origin + 
                      pLeaseData->dhcpcParam->dhcp_t2;
        limit = pLeaseData->dhcpcParam->lease_origin + 
                    pLeaseData->dhcpcParam->lease_duration;

        if (curr_epoch >= limit || timeout <= curr_epoch)
            {
            /* Lease or second timer expired - create a timeout event. */

            pEvent->source = DHCP_AUTO_EVENT;
            pEvent->type = DHCP_TIMEOUT;
            }
        else
            {
            timeout =  pLeaseData->dhcpcParam->lease_origin + 
                           pLeaseData->dhcpcParam->dhcp_t1;
            if (timeout <= curr_epoch)
                {
                /* First timer expired - create a timeout event. */

                pEvent->source = DHCP_AUTO_EVENT;
                pEvent->type = DHCP_TIMEOUT;
                }
            else
                {
                /* No timers expired - set to time remaining with T1. */

                timeout = pLeaseData->dhcpcParam->lease_origin + 
                              pLeaseData->dhcpcParam->dhcp_t1 - curr_epoch;
                wdStart (pLeaseData->timer, sysClkRateGet() * timeout,
                         (FUNCPTR)alarm_bound, (int)pLeaseData);
                }
            }
        pLeaseData->prevState = BOUND;

#ifdef DHCPC_DEBUG
        logMsg ("dhcpc: Ready for user requests.\n", 0, 0, 0, 0, 0, 0); 
#endif
        }

    if (pEvent->type == DHCPC_STATE_BEGIN)   /* Initial processing finished. */
        return (OK);

    if (pEvent->source == DHCP_AUTO_EVENT)
        {
        /* If no timers have expired, ignore received DHCP messages. */

        if (pEvent->type == DHCP_MSG_ARRIVED)
            return (OK);
        else
            {
            /*
             * Initiate renewal or rebinding when lease timers expire.
             * If the client monitor task is overloaded, processing may
             * occur well after the event notification is received.
             * In the worst case, the lease could expire before any
             * processing is performed. The timers can also expire
             * during a requested lease verification. Excess timeout 
             * notifications which might be present from earlier states 
             * are ignored.
             */

            limit = pLeaseData->dhcpcParam->lease_origin + 
                        pLeaseData->dhcpcParam->lease_duration;
            if (curr_epoch >= limit)
                {
                /*
                 * Lease expired between notification and processing.
                 * Resume filtering with the BPF device (suspended before
                 * entry to this routine) before restarting.
                 */

                bzero ( (char *)&ifr, sizeof (struct ifreq));
                sprintf (ifr.ifr_name, "%s%d",
                         pLeaseData->ifData.iface->if_name,
                         pLeaseData->ifData.iface->if_unit);

                ioctl (pLeaseData->ifData.bpfDev, BIOCSTART, (int)&ifr);

                pLeaseData->prevState = REQUESTING;
                pLeaseData->currState = INIT;
                return (DHCPC_MORE);
                }

            timeout = pLeaseData->dhcpcParam->lease_origin +
                          pLeaseData->dhcpcParam->dhcp_t2;
            if (timeout <= curr_epoch && curr_epoch < limit)
                {
                /* Second timer expired: contact any server for parameters. */

#ifdef DHCPC_DEBUG
                logMsg ("Entering REBINDING state.\n", 0, 0, 0, 0, 0, 0);
#endif
                length = make_request (pLeaseData, REBINDING, TRUE);

                dhcpcMsgOut.udp->uh_sum = 0;
                dhcpcMsgOut.udp->uh_sum = udp_cksum (&spudph, 
                                                     (char *)dhcpcMsgOut.udp,
                                                     ntohs (spudph.ulen));

                bzero ( (char *)&dest, sizeof (struct sockaddr_in));

                dest.sin_len = sizeof (struct sockaddr_in);
                dest.sin_family = AF_INET;
                dest.sin_addr.s_addr = dhcpcMsgOut.ip->ip_dst.s_addr;

                pIf = pLeaseData->ifData.iface;

                /* Remember to re-enable the BPF filter here. */

                bzero ( (char *)&ifr, sizeof (struct ifreq));
                sprintf (ifr.ifr_name, "%s%d",
                         pLeaseData->ifData.iface->if_name,
                         pLeaseData->ifData.iface->if_unit);

                ioctl (pLeaseData->ifData.bpfDev, BIOCSTART, (int)&ifr);

                status = dhcpSend (pIf, &dest, sbuf.buf, length, TRUE);

#ifdef DHCPC_DEBUG
                if (status == ERROR)
                    logMsg ("Can't send DHCPREQUEST(REBINDING)\n", 
                            0, 0, 0, 0, 0, 0);
#endif
                pLeaseData->prevState = BOUND;
                pLeaseData->currState = REBINDING;

                /* Set a timeout in case we miss the reply. */

                timeout = limit - curr_epoch;
                timeout /= 2;
                if (timeout < 60) 
                    timeout = 60;

                wdStart (pLeaseData->timer, sysClkRateGet() * timeout, 
                         (FUNCPTR)retrans_rebinding, (int)pLeaseData);

                return (OK);
                }
            timeout = pLeaseData->dhcpcParam->lease_origin +
                          pLeaseData->dhcpcParam->dhcp_t1;
            limit = pLeaseData->dhcpcParam->lease_origin +
                        pLeaseData->dhcpcParam->dhcp_t2;
            if (timeout <= curr_epoch && curr_epoch < limit)
                {
                /*
                 * First timer expired: 
                 * attempt to renew lease with current server.
                 * Resume filtering with the BPF device (suspended while 
		 * bound).
		 */

                bzero ( (char *)&ifr, sizeof (struct ifreq));
                sprintf (ifr.ifr_name, "%s%d", pLeaseData->ifData.iface->if_name,
                         pLeaseData->ifData.iface->if_unit);
                
                ioctl (pLeaseData->ifData.bpfDev, BIOCSTART, (int)&ifr);
                
                
#ifdef DHCPC_DEBUG
                logMsg ("Entering RENEWING state.\n", 0, 0, 0, 0, 0, 0);
#endif
                length = make_request (pLeaseData, RENEWING, TRUE);
                
                /* Ignore transmission failures for handling by timeout. */

                status = send_unicast (&pLeaseData->dhcpcParam->server_id, 
                                       dhcpcMsgOut.dhcp, length);
#ifdef DHCPC_DEBUG
                if (status < 0)
                    logMsg ("Can't send DHCPREQUEST(RENEWING)\n.", 
                            0, 0, 0, 0, 0, 0);
#endif

                
                /* DHCP request sent. Set lease data to execute next state. */

                pLeaseData->prevState = BOUND;
                pLeaseData->currState = RENEWING;

                pLeaseData->initEpoch = curr_epoch;

                /*
                 * Set the retransmission timer to wait for one-half of the 
                 * remaining time before T2 expires, or 60 seconds if T2 
                 * expires in less than one minute.
                 */

                timeout = limit - curr_epoch;
                timeout /= 2;
                if (timeout < 60) 
                    timeout = 60;

                /* Set timer for retransmission of request message. */

                wdStart (pLeaseData->timer, sysClkRateGet() * timeout, 
                         (FUNCPTR)retrans_renewing, (int)pLeaseData);

                return (OK);
                }
            }    /* End of timeout processing. */
        }    /* End of automatic events. */
    else
        {
        /* Process user requests. */

        if (pEvent->type == DHCP_USER_RELEASE)  /* Relinquish lease. */
            {
            release (pLeaseData, TRUE);
            return (DHCPC_DONE);
            }
        if (pEvent->type == DHCP_USER_VERIFY)   /* Verify lease */
            {
            /* Set the lease data to execute verify() routine. */

            pLeaseData->prevState = BOUND;
            pLeaseData->currState = VERIFY;

            return (DHCPC_MORE);
            }
        }
    return (OK);
    }

/*******************************************************************************
*
* renewing - Specific lease renewal state of client finite state machine
*
* This routine contains the fifth state of the client finite state machine.
* During this state, a DHCP lease is active. The routine handles all 
* processing after the first lease timer has expired. If the server which 
* issued the corresponding lease acknowledges the request for renewal, 
* processing returns to the bound() routine. If the request is denied, the 
* negotiation process restarts with the init() routine. If a timeout occurs
* before the second lease timer has expired, the DHCP request message is 
* retransmitted. Otherwise, processing continues with the rebinding() routine.
* .IP
* This routine is invoked by the event handler of the client monitor task,
* and should only be called internally. Any arriving DHCP messages containing
* stale offers from earlier states are discarded. User requests generated
* by calls to the dhcpcRelease() and dhcpcVerify() routines are accepted.
*
* RETURNS: OK (processing complete), DHCPC_DONE (remove lease),
*          DHCPC_MORE (continue), or ERROR.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int renewing
    (
    EVENT_DATA * 	pEvent 	/* pointer to event descriptor */
    )
    {
    char *option = NULL;
    char errmsg[255];
    time_t curr_epoch = 0;
    int timeout = 0;
    int limit;
    int length;
    struct dhcp_param tmpparam;
    int msgtype;

#ifdef DHCPC_DEBUG
    char newAddr [INET_ADDR_LEN];
#endif

    LEASE_DATA * 	pLeaseData;
    char * 		pMsgData;

    struct sockaddr_in dest;
    struct ifnet * pIf;

    bzero (errmsg, sizeof (errmsg));

    if (dhcpTime (&curr_epoch) == -1) 
        {
#ifdef DHCPC_DEBUG
        logMsg ("time() error in renewing()\n", 0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pEvent->leaseId;

    if (pEvent->source == DHCP_AUTO_EVENT)
        {
        if (pEvent->type == DHCP_TIMEOUT)
            {
            /*
             * The interval between a timeout event and the entry to the 
             * renewing() routine depends on the workload of the client 
             * monitor task and is completely unpredictable. Either the 
             * entire lease or the second lease timer may have expired 
             * during that time. Also, the final (60 second) retransmission 
             * timeout will always end after timer T2 has expired. If T2
             * has expired but the lease has not, set the lease data to 
             * continue with the rebinding() routine. If the lease has
             * expired, return to the initial state. Otherwise (the most
             * likely case), retransmit the request message since T2 has
             * not expired.
             */

            limit = pLeaseData->dhcpcParam->lease_origin +
                        pLeaseData->dhcpcParam->lease_duration;
            if (curr_epoch >= limit)
                {
                /* Lease expired before processing started. */

                pLeaseData->prevState = RENEWING;
                pLeaseData->currState = INIT;
                return (DHCPC_MORE);
                }

            limit = pLeaseData->dhcpcParam->lease_origin +
                        pLeaseData->dhcpcParam->dhcp_t2;
            if (limit <= curr_epoch)
                {
                /* 
                 * Second timer expired: 
                 *     request configuration data from any server. 
                 */

                length = make_request (pLeaseData, REBINDING, TRUE);
                if (length < 0)
                    {
#ifdef DHCPC_DEBUG
                 logMsg ("Error making rebind request. Entering INIT state.\n",
                         0, 0, 0, 0, 0, 0);
#endif
                    pLeaseData->prevState = RENEWING;
                    pLeaseData->currState = INIT;
                    return (DHCPC_MORE);
                    }

                dhcpcMsgOut.udp->uh_sum = 0;
                dhcpcMsgOut.udp->uh_sum = udp_cksum(&spudph, 
                                                    (char *)dhcpcMsgOut.udp, 
                                                    ntohs (spudph.ulen));

                bzero ( (char *)&dest, sizeof (struct sockaddr_in));

                dest.sin_len = sizeof (struct sockaddr_in);
                dest.sin_family = AF_INET;
                dest.sin_addr.s_addr = dhcpcMsgOut.ip->ip_dst.s_addr;

                pIf = pLeaseData->ifData.iface;

                if (dhcpSend (pIf, &dest, sbuf.buf, length, TRUE) == ERROR)
                    {
#ifdef DHCPC_DEBUG
                    logMsg ("Can't send DHCPREQUEST(REBINDING)\n", 
                            0, 0, 0, 0, 0, 0);
#endif
                    pLeaseData->prevState = RENEWING;
                    pLeaseData->currState = INIT;
                    return (DHCPC_MORE);
                    }

                /* DHCP request sent. Set lease data to execute next state. */

                pLeaseData->prevState = RENEWING;
                pLeaseData->currState = REBINDING;

                pLeaseData->initEpoch = curr_epoch;

                /*
                 * Set the retransmission timer to wait for one-half of the
                 * remaining time before the lease expires, or 60 seconds if
                 * it expires in less than one minute.
                 */

                timeout = pLeaseData->dhcpcParam->lease_origin + 
                            pLeaseData->dhcpcParam->lease_duration - 
                                curr_epoch;
                timeout /= 2;
                if (timeout < 60)
                    timeout = 60;

                /* Set timer for retransmission of request message. */

                wdStart (pLeaseData->timer, sysClkRateGet() * timeout,
                         (FUNCPTR)retrans_rebinding, (int)pLeaseData);
                }
            else
                {
                /* Transmit lease renewal request to issuing server. */

                length = make_request (pLeaseData, RENEWING, FALSE);

                if (send_unicast (&pLeaseData->dhcpcParam->server_id, 
                                  dhcpcMsgOut.dhcp, length) 
                        < 0)
                    {
#ifdef DHCPC_DEBUG
                    logMsg ("Can't send DHCPREQUEST(RENEWING)\n", 
                            0, 0, 0, 0, 0, 0);
#endif
                    pLeaseData->prevState = RENEWING;
                    pLeaseData->currState = INIT;
                    return (DHCPC_MORE);
                    }

                pLeaseData->prevState = RENEWING;
                pLeaseData->currState = RENEWING;
                pLeaseData->initEpoch = curr_epoch;

                /*
                 * Set the retransmission timer to wait for one-half of the
                 * remaining time before T2 expires, or 60 seconds if T2
                 * expires in less than one minute.
                 */

                timeout = limit - curr_epoch;
                timeout /= 2;
                if (timeout < 60)
                    timeout = 60;

                /* Set timer for retransmission of request message. */

                wdStart (pLeaseData->timer, sysClkRateGet() * timeout,
                         (FUNCPTR)retrans_renewing, (int)pLeaseData);
                }
            }    /* End of timeout processing. */
        else
            {
            bzero ( (char *)&tmpparam, sizeof (tmpparam));

            /*
             * Process DHCP message stored in receive buffer by monitor task.
             * The 4-byte alignment of the IP header needed by Sun BSP's is
             * guaranteed by the Berkeley Packet Filter during input.
             */

            pMsgData = pLeaseData->msgBuffer;

            align_msg (&dhcpcMsgIn, pMsgData);

            /* Examine type of message. Accept DHCPACK or DHCPNAK replies. */

            option = (char *)pickup_opt (dhcpcMsgIn.dhcp, 
                                         DHCPLEN (dhcpcMsgIn.udp),
                                         _DHCP_MSGTYPE_TAG);
            if (option == NULL)
                {
                /*
                 * Message type not found -  ignore untyped DHCP messages, and
                 * any BOOTP replies.
                 */

                return (OK);
                }
            else
                {
                msgtype = *OPTBODY (option);
                if (msgtype == DHCPNAK)
                    {
#ifdef DHCPC_DEBUG
                    logMsg ("Got DHCPNAK in renewing()\n", 0, 0, 0, 0, 0, 0);

                    option = (char *)pickup_opt (dhcpcMsgIn.dhcp,
                                                 DHCPLEN (dhcpcMsgIn.udp),
                                                 _DHCP_ERRMSG_TAG);
                    if (option != NULL &&
                        nvttostr (OPTBODY (option), errmsg,
                                  (int)DHCPOPTLEN (option)) == 0)
                        logMsg ("DHCPNAK contains the error message \"%s\"\n",
                                 (int)errmsg, 0, 0, 0, 0, 0);
#endif
                    clean_param (pLeaseData->dhcpcParam);
                    free (pLeaseData->dhcpcParam);
                    pLeaseData->dhcpcParam = NULL;

                    pLeaseData->prevState = RENEWING;
                    pLeaseData->currState = INIT;
                    return (DHCPC_MORE);
                    }
                else if (msgtype == DHCPACK)
                    {
                    /* Fill in host requirements defaults. */

                    dhcpcDefaultsSet (&tmpparam);

                    if (dhcp_msgtoparam (dhcpcMsgIn.dhcp, 
                                         DHCPLEN (dhcpcMsgIn.udp),
                                         &tmpparam) == OK)
                        {
                        merge_param (pLeaseData->dhcpcParam, &tmpparam);
                        *(pLeaseData->dhcpcParam) = tmpparam;
                        pLeaseData->dhcpcParam->lease_origin =
                            pLeaseData->initEpoch;
#ifdef DHCPC_DEBUG
                        inet_ntoa_b (pLeaseData->dhcpcParam->yiaddr, newAddr);
                        logMsg ("Got DHCPACK (IP = %s, duration = %d secs)\n",
                                (int)newAddr,
                                pLeaseData->dhcpcParam->lease_duration,
                                0, 0, 0, 0);
#endif
                        /* 
                         * Send an ARP reply to update the ARP cache on other
                         * hosts if the assigned IP address was applied to
                         * the underlying network interface.
                         */

                        if (pLeaseData->autoConfig || 
                                pLeaseData->leaseType == DHCP_AUTOMATIC)
                            arp_reply (&pLeaseData->dhcpcParam->yiaddr, 
                                       &pLeaseData->ifData);

                        pLeaseData->prevState = RENEWING;
                        pLeaseData->currState = BOUND;
                        }
                    return (DHCPC_MORE);
                    }
                }    /* End of processing for typed DHCP message. */
            }    /* End of DHCP message processing. */
        }
    else
        {
        /* Process user requests. */

        if (pEvent->type == DHCP_USER_RELEASE)    /* Relinquish lease. */
            {
            release (pLeaseData, TRUE);
            return (DHCPC_DONE);
            }
        if (pEvent->type == DHCP_USER_VERIFY)    /* Verify lease. */
            {
            /* Set the lease data to execute verify() routine. */

            pLeaseData->prevState = RENEWING;
            pLeaseData->currState = VERIFY;

            return (DHCPC_MORE);
            }
        }
    return (OK);
    }


/*******************************************************************************
*
* rebinding - Promiscuous lease renewal state of client finite state machine
*
* This routine contains the sixth state of the client finite state machine.
* During this state, a DHCP lease is active. The routine handles all
* processing after the second lease timer has expired. It accepts
* lease acknowledgements from any DHCP server in response to an earlier
* subnet local broadcast of a lease request. If an acknowlegement is received,
* processing returns to the bound() routine. If a DHCP server explicitly 
* denies the request, the negotiation restarts with the init() routine.
* Otherwise, the broadcast is repeated periodically until the lease expires.
* .IP
* This routine is invoked by the event handler of the client monitor task,
* and should only be called internally. Any arriving DHCP messages containing
* stale offers from earlier states are discarded. User requests generated
* by calls to the dhcpcRelease() and dhcpcVerify() routines are accepted.
*
* RETURNS: OK (processing complete), DHCPC_DONE (remove lease),
*          DHCPC_MORE (continue), or ERROR.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int rebinding
    (
    EVENT_DATA * 	pEvent 	/* pointer to event descriptor */
    )
    {
    char *option = NULL;
    char errmsg[255];
    time_t curr_epoch = 0;
    int timeout;
    int limit;
    int msgtype;
    int status;
    struct dhcp_param tmpparam;
#ifdef DHCPC_DEBUG
    char newAddr [INET_ADDR_LEN];
#endif

    LEASE_DATA * 	pLeaseData;
    char * 		pMsgData;
    int 		length;

    struct sockaddr_in 	dest;
    struct ifnet * 	pIf;
    struct ifreq 	ifr;

    bzero (errmsg, sizeof (errmsg));
    bzero ( (char *)&tmpparam, sizeof (tmpparam));

    if (dhcpTime (&curr_epoch) == -1) 
        {
#ifdef DHCPC_DEBUG
        logMsg ("time() error in rebinding()\n", 0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pEvent->leaseId;

    if (pLeaseData->prevState == BOUND)
        {
        /* Resume filtering with the BPF device (suspended while bound). */

        bzero ( (char *)&ifr, sizeof (struct ifreq));
        sprintf (ifr.ifr_name, "%s%d", pLeaseData->ifData.iface->if_name,
                                       pLeaseData->ifData.iface->if_unit);

        ioctl (pLeaseData->ifData.bpfDev, BIOCSTART, (int)&ifr);
        }

    if (pEvent->source == DHCP_AUTO_EVENT)
        {
        if (pEvent->type == DHCP_TIMEOUT)
            {
            /*
             * The interval between a timeout event and the entry to the
             * rebinding() routine depends on the workload of the client
             * monitor task and is completely unpredictable. The lease
             * may have expired during that time, and the final retransmission 
             * timeout will always end after the lease has expired. In either 
             * case, set the lease data to restart the negotiation with the 
             * init() routine. Otherwise, repeat the broadcast of the request 
             * message.
             */

            limit = pLeaseData->dhcpcParam->lease_origin +
                        pLeaseData->dhcpcParam->lease_duration;
            if (limit <= curr_epoch)
                {
                /* Lease has expired: 
                 *    shut down network and return to initial state. 
                 */
#ifdef DHCPC_DEBUG
                logMsg ("Can't extend lease. Entering INIT state.\n", 
                        0, 0, 0, 0, 0, 0);
#endif
                pLeaseData->prevState = REBINDING;
                pLeaseData->currState = INIT;
                return (DHCPC_MORE);
                }

            /* Retransmit rebinding request. */

            length = make_request (pLeaseData, REBINDING, FALSE);
            if (length < 0)
                {
#ifdef DHCPC_DEBUG
                logMsg ("Error making rebind request. Entering INIT state.\n",
                        0, 0, 0, 0, 0, 0);
#endif
                pLeaseData->prevState = REBINDING;
                pLeaseData->currState = INIT;
                return (DHCPC_MORE);
                }
            dhcpcMsgOut.udp->uh_sum = 0;
            dhcpcMsgOut.udp->uh_sum = udp_cksum(&spudph,
                                                (char *)dhcpcMsgOut.udp,
                                                ntohs (spudph.ulen));

            bzero ( (char *)&dest, sizeof (struct sockaddr_in));

            dest.sin_len = sizeof (struct sockaddr_in);
            dest.sin_family = AF_INET;
            dest.sin_addr.s_addr = dhcpcMsgOut.ip->ip_dst.s_addr;

            pIf = pLeaseData->ifData.iface;

            if (dhcpSend (pIf, &dest, sbuf.buf, length, TRUE) == ERROR)
                {
#ifdef DHCPC_DEBUG
                logMsg ("Can't send DHCPREQUEST(REBINDING)\n",
                        0, 0, 0, 0, 0, 0);
#endif
                pLeaseData->prevState = REBINDING;
                pLeaseData->currState = INIT;
                return (DHCPC_MORE);
                }

            /* DHCP request sent. Set lease data to repeat current state. */

            pLeaseData->prevState = REBINDING;
            pLeaseData->currState = REBINDING;

            pLeaseData->initEpoch = curr_epoch;

           /*
            * Set the retransmission timer to wait for one-half of the
            * remaining time before the lease expires, or 60 seconds if it
            * expires in less than one minute.
            */

           timeout = pLeaseData->dhcpcParam->lease_origin +
                        pLeaseData->dhcpcParam->lease_duration -
                            curr_epoch;
            timeout /= 2;
            if (timeout < 60)
                timeout = 60;

            /* Set timer for retransmission of request message. */

            wdStart (pLeaseData->timer, sysClkRateGet() * timeout,
                     (FUNCPTR)retrans_rebinding, (int)pLeaseData);
            }    /* End of timeout processing. */
        else
            {
            bzero ( (char *)&tmpparam, sizeof (tmpparam));

            /*
             * Process DHCP message stored in receive buffer by monitor task.
             * The 4-byte alignment of the IP header needed by Sun BSP's is
             * guaranteed by the Berkeley Packet Filter during input.
             */

            pMsgData = pLeaseData->msgBuffer;

            align_msg (&dhcpcMsgIn, pMsgData);

            /* Examine type of message. Accept DHCPACK or DHCPNAK replies. */

            option = (char *)pickup_opt (dhcpcMsgIn.dhcp,
                                         DHCPLEN (dhcpcMsgIn.udp),
                                         _DHCP_MSGTYPE_TAG);
            if (option == NULL)
                {
                /*
                 * Message type not found -  ignore untyped DHCP messages, and
                 * any BOOTP replies.
                 */

                return (OK);
                }
            else
                {
                msgtype = *OPTBODY (option);
                if (msgtype == DHCPNAK)
                    {
#ifdef DHCPC_DEBUG
                    logMsg ("Got DHCPNAK in rebinding()\n", 0, 0, 0, 0, 0, 0);

                    option = (char *)pickup_opt (dhcpcMsgIn.dhcp,
                                                 DHCPLEN (dhcpcMsgIn.udp),
                                                 _DHCP_ERRMSG_TAG);
                    if (option != NULL &&
                        nvttostr (OPTBODY (option), errmsg,
                                  (int)DHCPOPTLEN (option)) == 0)
                        logMsg ("DHCPNAK contains the error message \"%s\"\n",
                                 (int)errmsg, 0, 0, 0, 0, 0);
#endif
                    clean_param (pLeaseData->dhcpcParam);
                    free (pLeaseData->dhcpcParam);
                    pLeaseData->dhcpcParam = NULL;

                    pLeaseData->prevState = REBINDING;
                    pLeaseData->currState = INIT;
                    return (DHCPC_MORE);
                    }
                else if (msgtype == DHCPACK)
                    {
                    /* Fill in host requirements defaults. */

                    dhcpcDefaultsSet (&tmpparam);

                    if (dhcp_msgtoparam (dhcpcMsgIn.dhcp,
                                         DHCPLEN (dhcpcMsgIn.udp),
                                         &tmpparam) == OK)
                        {
                        merge_param (pLeaseData->dhcpcParam, &tmpparam);
                        *(pLeaseData->dhcpcParam) = tmpparam;
                        pLeaseData->dhcpcParam->lease_origin =
                            pLeaseData->initEpoch;
#ifdef DHCPC_DEBUG
                        inet_ntoa_b (pLeaseData->dhcpcParam->yiaddr, newAddr);
                        logMsg ("Got DHCPACK (IP = %s, duration = %d secs)\n",
                                (int)newAddr,
                                pLeaseData->dhcpcParam->lease_duration,
                                0, 0, 0, 0);
#endif
                        status = use_parameter (pLeaseData->dhcpcParam,
                                                pLeaseData);
                        semTake (dhcpcMutexSem, WAIT_FOREVER);
                        if (status != 0)
                            {
#ifdef DHCPC_DEBUG
                         logMsg ("Error configuring network. Shutting down.\n",
                                  0, 0, 0, 0, 0, 0);
#endif
                            pLeaseData->leaseGood = FALSE;
                            }
                        else
                            {
                            pLeaseData->leaseGood = TRUE;
                            }
                        semGive (dhcpcMutexSem);

                        pLeaseData->prevState = REBINDING;
                        pLeaseData->currState = BOUND;
                        }
                    return (DHCPC_MORE);
                    }
                }    /* End of processing for typed DHCP message. */
            }    /* End of DHCP message processing. */
        }
    else
        {
        /* Process user requests. */

        if (pEvent->type == DHCP_USER_RELEASE)    /* Relinquish lease. */
            {
            release (pLeaseData, TRUE);
            return (DHCPC_DONE);
            }
        if (pEvent->type == DHCP_USER_VERIFY)    /* Verify lease. */
            {
            /* Set the lease data to execute verify() routine. */

            pLeaseData->prevState = REBINDING;
            pLeaseData->currState = VERIFY;

            return (DHCPC_MORE);
            }
        }
    return (OK);
    }

/*******************************************************************************
*
* init_reboot - Rebooting state of the client finite state machine
*
* This routine implements the "zeroth" state of the client finite state
* machine. It attempts to renew an active lease after the client has
* rebooted. This generally requires the presence of a cache, but the lease
* data may also be read from global variables to renew the lease obtained
* during system startup. If no lease data is found or renewal fails, processing
* continues with the INIT state.
* .IP
* This routine is invoked by the event handler of the client monitor task,
* and should only be called internally. Any user requests generated by
* incorrect calls or delayed responses to the dhcpcVerify() routine are 
* ignored.
*
* RETURNS: OK (processing complete), DHCPC_DONE (remove lease),
*          DHCPC_MORE (continue), or ERROR.
*
* ERRNO: N/A
*
* INTERNAL
*
* When calculating the values for the lease timers, floating-point calculations
* can't be used because some boards (notably the SPARC architectures) disable
* software floating point by default to speed up context switching. These
* boards abort with an exception when floating point operations are
* encountered. The error introduced by the integer approximations is not
* significant.
*
* NOMANUAL
*/

int init_reboot
    (
    EVENT_DATA * 	pEvent 	/* pointer to event descriptor */
    )
    {
    char *rbufp = NULL;
    STATUS result;
    int length = 0;
    unsigned long origin = 0;
    int tmp;
    int status;

    struct sockaddr_in dest;
    struct ifnet * 	pIf;

    LEASE_DATA * 	pLeaseData = NULL;

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pEvent->leaseId;

    /*
     * The DHCP_USER_RELEASE event occurs in response to the dhcpcRelease()
     * or dhcpcShutdown() call. Remove all data structures for this lease.
     */

    if (pEvent->type == DHCP_USER_RELEASE)
        {
        dhcpcLeaseCleanup (pLeaseData);
        return (DHCPC_DONE);
        }

    /* Ignore verify user events, which are meaningless here. */

    if (pEvent->source == DHCP_USER_EVENT && pEvent->type != DHCP_USER_BIND)
        return (OK);

    /* 
     * Safety check - ignore timeouts and message arrivals, which are
     * theoretically possible under extremely unlikely chains of events.
     */
      
    if (pEvent->source == DHCP_AUTO_EVENT)
        return (OK);

    /* If a cache is needed and unavailable, restart from the INIT state. */

    if (pLeaseData->cacheHookRtn == NULL &&
            pLeaseData->leaseType == DHCP_MANUAL)
        {
        pLeaseData->prevState = INIT_REBOOT;
        pLeaseData->currState = INIT;
        return (DHCPC_MORE);
        }

#ifdef DHCPC_DEBUG
    logMsg("dhcpc: Attempting to re-use parameters.\n", 0, 0, 0, 0, 0, 0);
#endif

    bzero (sbuf.buf, sbuf.size);

    /*
     * The client might have established a lease (during boot-time) with
     * an older DHCP server which ignores messages less than the minimum
     * length obtained with a fixed options field. Set this lease attempt
     * to pad the renewal request to the minimum size (for RFC 1541) to
     * avoid unnecessary retransmissions in that case.
     */

    pLeaseData->oldFlag = TRUE;
     
    if (pLeaseData->leaseType == DHCP_MANUAL)   
        {
        /*
         * Post-boot: read from cache into aligned scratch buffer
         * (used during other states to transmit DHCP messages).
         */

        length = sbuf.size;
        rbufp = sbuf.buf;
        result = (* pLeaseData->cacheHookRtn) (DHCP_CACHE_READ, &origin, 
                                               &length, rbufp);
        if (result != OK)     /* Can't re-use stored parameters. */
            {
            pLeaseData->prevState = INIT_REBOOT;
            pLeaseData->currState = INIT;
            return (DHCPC_MORE);
            }

        /*
         * Set the pointers to access the individual DHCP message components.
         */

        dhcpcMsgIn.ip = (struct ip *)rbufp;
#if BSD<44
    dhcpcMsgIn.udp = (struct udphdr *)&rbufp [(dhcpcMsgIn.ip->ip_v_hl & 0xf) * 
                                              WORD];
    dhcpcMsgIn.dhcp = (struct dhcp *) &rbufp [(dhcpcMsgIn.ip->ip_v_hl & 0xf) * 
                                              WORD + UDPHL];
#else
    dhcpcMsgIn.udp = (struct udphdr *)&rbufp [dhcpcMsgIn.ip->ip_hl * WORD];
    dhcpcMsgIn.dhcp = (struct dhcp *) &rbufp [dhcpcMsgIn.ip->ip_hl * WORD + 
                                              UDPHL];
#endif
        }

    if (pLeaseData->dhcpcParam == NULL)
        {
        pLeaseData->dhcpcParam = (struct dhcp_param *)calloc (1,
                                                   sizeof (struct dhcp_param));
        if (pLeaseData->dhcpcParam == NULL)
            {
#ifdef DHCPC_DEBUG
            logMsg ("calloc() error in init_reboot()\n", 0, 0, 0, 0, 0, 0);
#endif
            pLeaseData->prevState = INIT_REBOOT;
            pLeaseData->currState = INIT;
            return (DHCPC_MORE);
            }
        }

    if (pLeaseData->leaseType == DHCP_AUTOMATIC)
        {
        /* Get parameters previously read from bootline. */

        bcopy ( (char *)&dhcpcBootLease.yiaddr, 
               (char *)&pLeaseData->dhcpcParam->yiaddr,
               sizeof (struct in_addr));

        origin = dhcpcBootLease.lease_origin;
        pLeaseData->dhcpcParam->lease_duration = dhcpcBootLease.lease_duration;
        pLeaseData->dhcpcParam->dhcp_t1 =
            pLeaseData->dhcpcParam->lease_duration / 2;
        SETBIT (pLeaseData->dhcpcParam->got_option, _DHCP_T1_TAG);

        /* Set t2 to .875 of lease without using floating point. */

        tmp = (pLeaseData->dhcpcParam->lease_duration * 7) >> 3;
        pLeaseData->dhcpcParam->dhcp_t2 = (unsigned long)tmp;
        SETBIT (pLeaseData->dhcpcParam->got_option, _DHCP_T2_TAG);
        }
    else                             /* Get parameters from cached message. */
        {
        /* Host requirements defaults. */

        dhcpcDefaultsSet (pLeaseData->dhcpcParam);   

        if (dhcp_msgtoparam (dhcpcMsgIn.dhcp, DHCPLEN (dhcpcMsgIn.udp), 
                             pLeaseData->dhcpcParam) != OK)
            {
#ifdef DHCPC_DEBUG
            logMsg("dhcp_msgtoparam() error in init_reboot()\n",
                    0, 0, 0, 0, 0, 0);
#endif
            clean_param (pLeaseData->dhcpcParam);
            free (pLeaseData->dhcpcParam);
            pLeaseData->dhcpcParam = NULL;
            pLeaseData->prevState = INIT_REBOOT;
            pLeaseData->currState = INIT;
            return (DHCPC_MORE);
            }
        }
    pLeaseData->dhcpcParam->lease_origin = origin;

    /*
     * If the cache contained a BOOTP reply, no further processing is needed.
     * Set the lease type to prevent the inclusion of timing information in 
     * the bootline so that a later reboot will not attempt to renew the lease.
     */

    if (pLeaseData->leaseType == DHCP_MANUAL &&
            pLeaseData->dhcpcParam->msgtype == DHCP_BOOTP)
        {
        pLeaseData->leaseType = DHCP_BOOTP;
        pLeaseData->prevState = INIT_REBOOT;
        pLeaseData->currState = BOUND;

        status = use_parameter (pLeaseData->dhcpcParam, pLeaseData);

        semTake (dhcpcMutexSem, WAIT_FOREVER);
        if (status != 0)
            {
#ifdef DHCPC_DEBUG
            logMsg ("Error configuring network. Shutting down.\n",
                    0, 0, 0, 0, 0, 0);
#endif
            pLeaseData->leaseGood = FALSE;
            pLeaseData->prevState = INIT_REBOOT;
            pLeaseData->currState = INIT;
            }
        else
            {
            pLeaseData->leaseGood = TRUE;
            }
        semGive (dhcpcMutexSem);

        if (status != 0)
            return (DHCPC_MORE);

        return (OK);
        }

    /* The cache contained the record of a DHCP lease. Send the request. */

    length = make_request (pLeaseData, REBOOTING, TRUE);
    if (length < 0)
        {
#ifdef DHCPC_DEBUG
        logMsg("error constructing REBOOTING message. Entering INIT state.\n",
               0, 0, 0, 0, 0, 0);
#endif
        pLeaseData->prevState = INIT_REBOOT;
        pLeaseData->currState = INIT;
        return (DHCPC_MORE);
        }
    dhcpcMsgOut.udp->uh_sum = 0;
    dhcpcMsgOut.udp->uh_sum = udp_cksum (&spudph, (char *)dhcpcMsgOut.udp, 
                                         ntohs (spudph.ulen));

    bzero ( (char *)&dest, sizeof (struct sockaddr_in));

    dest.sin_len = sizeof (struct sockaddr_in);
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dhcpcMsgOut.ip->ip_dst.s_addr;

    pIf = pLeaseData->ifData.iface;

    if (dhcpSend (pIf, &dest, sbuf.buf, length, TRUE) == ERROR)
        {
#ifdef DHCPC_DEBUG
        logMsg ("Can't send DHCPREQUEST(REBOOTING)\n", 0, 0, 0, 0, 0, 0);
#endif
        pLeaseData->prevState = INIT_REBOOT;
        pLeaseData->currState = INIT;
        return (DHCPC_MORE);
        }

    /* Set lease data to execute next state and start retransmission timer. */

    pLeaseData->prevState = INIT_REBOOT;
    pLeaseData->currState = REBOOTING;

    pLeaseData->timeout = FIRSTTIMER;
    pLeaseData->numRetry = 0;

    wdStart (pLeaseData->timer, sysClkRateGet() *
                                SLEEP_RANDOM (pLeaseData->timeout),
             (FUNCPTR)retrans_reboot_verify, (int)pLeaseData);

    return (OK);
    }


/*******************************************************************************
*
* verify - Initial lease verification state of client finite state machine
*
* This routine begins the verification process by broadcasting a DHCP verify
* message. It is called in response to the user request generated by the
* dhcpcVerify() routine in the API.
*
* RETURNS: OK (processing complete), ERROR (failure), or DHCPC_MORE (continue).
*
* ERRNO: N/A
*
* NOMANUAL
*/

int verify
    (
    EVENT_DATA * 	pEvent 	/* pointer to event descriptor */
    )
    {
    int limit = 0;
    int length;

    time_t 	curr_epoch;
    LEASE_DATA * 	pLeaseData;

    struct sockaddr_in 	dest;
    struct ifnet * 	pIf;
    struct ifreq 	ifr;

    if (dhcpTime (&curr_epoch) == -1)
        {
#ifdef DHCPC_DEBUG
        logMsg ("time() error in verify()\n", 0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pEvent->leaseId;

    if (pLeaseData->prevState == BOUND)
        {
        /* Resume filtering with the BPF device (suspended while bound). */

        bzero ( (char *)&ifr, sizeof (struct ifreq));
        sprintf (ifr.ifr_name, "%s%d", pLeaseData->ifData.iface->if_name,
                                       pLeaseData->ifData.iface->if_unit);

        ioctl (pLeaseData->ifData.bpfDev, BIOCSTART, (int)&ifr);
        }

    /*
     * The interval between the user request and the entry to the verify() 
     * routine depends on the workload of the client monitor task and the
     * status of the lease timers. The lease may have expired before this
     * routine executes, which will cause the negotiation process to restart
     * with the init() routine.
     */

    limit = pLeaseData->dhcpcParam->lease_origin +
                pLeaseData->dhcpcParam->lease_duration;
    if (limit <= curr_epoch)
        {
        pLeaseData->prevState = VERIFY;
        pLeaseData->currState = INIT;
        return (DHCPC_MORE);
        }
 
    length = make_request (pLeaseData, VERIFYING, TRUE);
    if (length < 0)
        {
#ifdef DHCPC_DEBUG
        logMsg("Unable to construct verify message. Entering INIT state.\n",
                0, 0, 0, 0, 0, 0);
#endif
        pLeaseData->prevState = VERIFY;
        pLeaseData->currState = INIT;
        return (DHCPC_MORE);
        }
    dhcpcMsgOut.udp->uh_sum = 0;
    dhcpcMsgOut.udp->uh_sum = udp_cksum (&spudph, (char *)dhcpcMsgOut.udp, 
                                         ntohs (spudph.ulen));

    if (dhcpTime (&pLeaseData->initEpoch) == -1)
        {
#ifdef DHCPC_DEBUG
        logMsg ("time() error setting initEpoch\n", 0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }

    bzero ( (char *)&dest, sizeof (struct sockaddr_in));

    dest.sin_len = sizeof (struct sockaddr_in);
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dhcpcMsgOut.ip->ip_dst.s_addr;

    pIf = pLeaseData->ifData.iface;

    if (dhcpSend (pIf, &dest, sbuf.buf, length, TRUE) == ERROR)
        {
#ifdef DHCPC_DEBUG
        logMsg("Can't send DHCPREQUEST(VERIFY)\n", 0, 0, 0, 0, 0, 0);
#endif
        pLeaseData->prevState = VERIFY;
        pLeaseData->currState = INIT;
        return (DHCPC_MORE);
        }

    /* Set lease data to execute next state and start retransmission timer. */

    pLeaseData->prevState = VERIFY;
    pLeaseData->currState = VERIFYING;

    pLeaseData->timeout = FIRSTTIMER;
    pLeaseData->numRetry = 0;

    wdStart (pLeaseData->timer, sysClkRateGet() *
                                SLEEP_RANDOM (pLeaseData->timeout),
             (FUNCPTR)retrans_reboot_verify, (int)pLeaseData);

    return (OK);    /* Next state is REBOOT_VERIFY */
    }

/*******************************************************************************
*
* reboot_verify - Final lease verification state of client finite state machine
*
* This routine continues the verification process of the finite state machine
* by waiting for a server response until the current timeout value expires.
* If a timeout occurs, the DHCP message is retransmitted. This routine collects
* the replies for manual lease renewals and lease renewals resulting from a
* reboot of the client.
*
* RETURNS: OK (processing complete), DHCPC_DONE (remove lease),
*          DHCPC_MORE (continue), or ERROR.
*
* ERRNO: N/A
*
* NOMANUAL
*/

int reboot_verify
    (
    EVENT_DATA * 	pEvent 	/* pointer to event descriptor */
    )
    {
    int arpans = 0;
    time_t curr_epoch = 0;
    char *option = NULL;
    char errmsg[255];
    int timer = 0;
    int retry = 0;
    int msgtype;
    struct dhcp_param tmpparam;
    struct dhcp_reqspec tmp_reqspec;
    int status = 0;
#ifdef DHCPC_DEBUG
    char newAddr [INET_ADDR_LEN];
#endif

    LEASE_DATA * 	pLeaseData = NULL;
    char * 		pMsgData;
    int 		length;

    /*
     * Use the cookie to access the lease-specific data structures. For now,
     * just typecast the cookie. This translation could be replaced with a more
     * sophisticated lookup at some point.
     */

    pLeaseData = (LEASE_DATA *)pEvent->leaseId;

    /*
     * The DHCP_USER_RELEASE event occurs in response to the dhcpcRelease()
     * or dhcpcShutdown() call. Remove all data structures for this lease.
     */

    if (pEvent->type == DHCP_USER_RELEASE)
        {
        dhcpcLeaseCleanup (pLeaseData);
        return (DHCPC_DONE);
        }

    /* Ignore bind and verify user events, which are meaningless here. */

    if (pEvent->source == DHCP_USER_EVENT)
        return (OK);

    bzero (errmsg, sizeof (errmsg));
    bzero ( (char *)&tmpparam, sizeof (tmpparam));
    bzero( (char *)&tmp_reqspec, sizeof (tmp_reqspec));

    if (dhcpTime (&curr_epoch) == -1)
        {
#ifdef DHCPC_DEBUG
        logMsg ("time() error in reboot_verify()\n", 0, 0, 0, 0, 0, 0);
#endif
        return (ERROR);
        }

    if (pEvent->type == DHCP_TIMEOUT)
        {
        /*
         * The interval between a timeout event and the entry to the
         * reboot_verify() routine depends on the workload of the client
         * monitor task and is completely unpredictable. The lease
         * may have expired during that time. The lease may also expire
         * before the retransmission limit is reached. In either case, set 
         * the lease data to restart the negotiation with the init() routine.
         * If the retransmission limit is reached before the lease expires,
         * return to the BOUND state. Otherwise, retransmit the request 
         * message.
         */

        retry = pLeaseData->numRetry;
        timer = pLeaseData->timeout;

        retry++;
        if (retry > REQUEST_RETRANS)    /* Retransmission limit reached. */
            {
#ifdef DHCPC_DEBUG
            logMsg("No server response received. Giving up.\n", 
                   0, 0, 0, 0, 0, 0);
#endif
            if (pLeaseData->prevState == INIT_REBOOT)
                {
                pLeaseData->currState = INIT;
                } 
            else if (pLeaseData->dhcpcParam->lease_origin == 0 ||
                       pLeaseData->dhcpcParam->lease_origin + 
                         pLeaseData->dhcpcParam->lease_duration <= curr_epoch)
                {
#ifdef DHCPC_DEBUG
                logMsg ("The lease has expired. Entering INIT state.\n",
                        0, 0, 0, 0, 0, 0);
#endif
                if (pLeaseData->prevState == INIT_REBOOT)
                    pLeaseData->prevState = REBOOTING;
                else if (pLeaseData->prevState == VERIFY);
                    pLeaseData->prevState = VERIFYING;

                pLeaseData->currState = INIT;
                }
            else 
                {
#ifdef DHCPC_DEBUG
                logMsg ("Entering BOUND state.\n", 0, 0, 0, 0, 0, 0);
#endif
                /*
                 * Send an ARP reply to update the ARP cache on other hosts 
                 * if the assigned IP address was applied to the underlying 
                 * network interface.
                 */

                if (pLeaseData->autoConfig ||
                        pLeaseData->leaseType == DHCP_AUTOMATIC)
                    arp_reply (&pLeaseData->dhcpcParam->yiaddr,
                               &pLeaseData->ifData);

                pLeaseData->prevState = VERIFYING;
                pLeaseData->currState = BOUND;
                }
            return (DHCPC_MORE);
            }
        else
            {
            /* Try to retransmit appropriate DHCP message for current state. */

           length = make_request (pLeaseData, pLeaseData->currState, FALSE);
           if (length < 0)
               {
#ifdef DHCPC_DEBUG
    logMsg("Unable to construct reboot/verify message. Entering INIT state.\n",
           0, 0, 0, 0, 0, 0);
#endif
                if (pLeaseData->prevState == INIT_REBOOT)
                    pLeaseData->prevState = REBOOTING;
                else
                    pLeaseData->prevState = VERIFYING;

                pLeaseData->currState = INIT;
                return (DHCPC_MORE);
                }

            gen_retransmit (pLeaseData, length);
            }

        if (timer < MAXTIMER)
            {
            /* Double retransmission delay with each attempt. (RFC 1541). */

            timer *= 2;
            }

        /* Set retransmission timer to randomized exponential backoff. */
            
        wdStart (pLeaseData->timer, sysClkRateGet() * SLEEP_RANDOM (timer),
                 (FUNCPTR)retrans_reboot_verify, (int)pLeaseData);

        pLeaseData->timeout = timer;
        pLeaseData->numRetry = retry;
        }
    else
        {
        /*
         * Process DHCP message stored in receive buffer by monitor task.
         * The 4-byte alignment of the IP header needed by Sun BSP's is
         * guaranteed by the Berkeley Packet Filter during input.
         */

        pMsgData = pLeaseData->msgBuffer;

        align_msg (&dhcpcMsgIn, pMsgData);

        /* Examine type of message. Accept DHCPACK or DHCPNAK replies. */

        option = (char *)pickup_opt (dhcpcMsgIn.dhcp, DHCPLEN (dhcpcMsgIn.udp),
                                     _DHCP_MSGTYPE_TAG);
        if (option == NULL)
            {
            /*
             * Message type not found -  discard untyped DHCP messages, and
             * any BOOTP replies.
             */

            return (OK);
            }
        else
            {
            msgtype = *OPTBODY (option);

            if (msgtype == DHCPNAK)
                {
#ifdef DHCPC_DEBUG
                logMsg ("Got DHCPNAK in reboot_verify()\n", 0, 0, 0, 0, 0, 0);

                option = (char *)pickup_opt (dhcpcMsgIn.dhcp,
                                             DHCPLEN(dhcpcMsgIn.udp),
                                             _DHCP_ERRMSG_TAG);
                if (option != NULL &&
                    nvttostr (OPTBODY (option), errmsg,
                              (int)DHCPOPTLEN (option)) == 0)
                    logMsg ("DHCPNAK contains the error message \"%s\"\n",
                             (int)errmsg, 0, 0, 0, 0, 0);
#endif
                clean_param (pLeaseData->dhcpcParam);
                free (pLeaseData->dhcpcParam);
                pLeaseData->dhcpcParam = NULL;

                if (pLeaseData->prevState == INIT_REBOOT)
                    pLeaseData->prevState = REBOOTING;
                else if (pLeaseData->prevState == VERIFY);
                    pLeaseData->prevState = VERIFYING;

                pLeaseData->currState = INIT;
                return (DHCPC_MORE);
                }
            else if (msgtype == DHCPACK)
                {
                /* Fill in host requirements defaults. */

                dhcpcDefaultsSet (&tmpparam);

                if (dhcp_msgtoparam (dhcpcMsgIn.dhcp, DHCPLEN (dhcpcMsgIn.udp),
                                     &tmpparam) == OK)
                    {
                    if ( (arpans = arp_check (&tmpparam.yiaddr, 
                                                  &pLeaseData->ifData)) == OK)
                        {
                        merge_param (pLeaseData->dhcpcParam, &tmpparam);
                        *(pLeaseData->dhcpcParam) = tmpparam;
                        pLeaseData->dhcpcParam->lease_origin =
                            pLeaseData->initEpoch;
#ifdef DHCPC_DEBUG
                        inet_ntoa_b (pLeaseData->dhcpcParam->yiaddr, newAddr);
                        logMsg ("Got DHCPACK (IP = %s, duration = %d secs)\n",
                                (int)newAddr,
                                pLeaseData->dhcpcParam->lease_duration,
                                0, 0, 0, 0);
#endif
                        /*
                         * Send an ARP reply to update the ARP cache on other
                         * hosts if the assigned IP address was applied to
                         * the underlying network interface.
                         */

                        if (pLeaseData->autoConfig ||
                                pLeaseData->leaseType == DHCP_AUTOMATIC)
                            arp_reply (&pLeaseData->dhcpcParam->yiaddr,
                                       &pLeaseData->ifData);

                        pLeaseData->currState = BOUND;

                        status = 0;

                        /* Use retrieved configuration parameters. */

                        if (pLeaseData->prevState == INIT_REBOOT ||
                                pLeaseData->prevState == REBOOTING)
                            {
                            status = use_parameter (pLeaseData->dhcpcParam,
                                                    pLeaseData);
                            if (status != 0)
                                {
#ifdef DHCPC_DEBUG
                         logMsg ("Error configuring network. Shutting down.\n",
                                 0, 0, 0, 0, 0, 0);
#endif
                                release (pLeaseData, FALSE);

                                semTake (dhcpcMutexSem, WAIT_FOREVER);
                                pLeaseData->leaseGood = FALSE;
                                semGive (dhcpcMutexSem);
                                }
                            else
                                {
                                semTake (dhcpcMutexSem, WAIT_FOREVER);
                                pLeaseData->leaseGood = TRUE;
                                semGive (dhcpcMutexSem);
                                }
                            pLeaseData->prevState = REBOOTING;
                            }
                        else
                            {
                            /*
                             * Send an ARP reply to update the ARP cache on 
                             * other hosts if the assigned IP address was
                             * applied to the underlying network interface.
                             */

                            if (pLeaseData->autoConfig ||
                                    pLeaseData->leaseType == DHCP_AUTOMATIC)
                                arp_reply (&pLeaseData->dhcpcParam->yiaddr,
                                           &pLeaseData->ifData);

                            pLeaseData->prevState = VERIFYING;
                            }

                        if (status == 0)
                            return (DHCPC_MORE);
                        else
                            return (ERROR);
                        }
                    }
                }

            /* Unknown message type or invalid parameters - send DHCPDECLINE */

            set_declinfo (&tmp_reqspec, pLeaseData, errmsg, arpans);
            dhcp_decline (&tmp_reqspec, &pLeaseData->ifData);
            clean_param (pLeaseData->dhcpcParam);
            free (pLeaseData->dhcpcParam);
            pLeaseData->dhcpcParam = NULL;
#ifdef DHCPC_DEBUG
            logMsg ("Received unacceptable DHCPACK. Entering INIT state.\n",
                    0, 0, 0, 0, 0, 0);
#endif
            if (pLeaseData->prevState == INIT_REBOOT)
                pLeaseData->prevState = REBOOTING;
            else if (pLeaseData->prevState == VERIFY);
                pLeaseData->prevState = VERIFYING;

            pLeaseData->currState = INIT;
            return (DHCPC_MORE);
            }    /* End of processing for typed DHCP message. */
        }     /* End of DHCP message processing. */
    return (OK);
    }

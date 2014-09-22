/* ripTimer.c - routines to handle RIP periodic updates*/

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * @(#)timer.c	8.1 (Berkeley) 6/5/93
 */

/*
modification history
--------------------
01l,22mar02,niq  Merged from Synth view, tor3_x.synth branch, ver 01p
01k,24jan02,niq  SPR 72415 - Added support for Route tags
01j,15oct01,rae  merge from truestack ver 01k, base 01h (VIRTUAL_STACK etc.)
01i,10nov00,spm  merged from version 01i of tor3_x branch (SPR #29099 fix)
01h,11sep98,spm  moved expanded ripShutdown routine to ripLib.c (SPR #22352);
                 added mutual exclusion to prevent collisions with RIP 
                 message processing (SPR #22350)
01g,01sep98,spm  corrected error preventing expiration of routes stored
                 in hosthash table (SPR #22066)
01f,06oct97,gnn  fixed SPR 9211 and cleaned up warnings
01e,11aug97,gnn  name change.
01d,02jun97,gnn  fixed SPR 8685 so that the timer task does not respawn.
01c,12mar97,gnn  added multicast support.
                 added time variables.
01b,24feb97,gnn  fixed bugs in interface aging.
01a,26nov96,gnn  created from BSD4.4 routed
*/

/*
DESCRIPTION
*/

/*
 * Routing Table Management Daemon
 */
#include "vxWorks.h"
#include "rip/defs.h"

#include "wdLib.h"
#include "semLib.h"
#include "intLib.h"
#include "logLib.h"
#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsRip.h"
#endif /* VIRTUAL_STACK */

/* globals */

#ifndef VIRTUAL_STACK
IMPORT int 			routedDebug;
IMPORT RIP 			ripState;
#endif /* VIRTUAL_STACK */

/* forward declarations. */

#ifdef VIRTUAL_STACK
void ripTimer(int stackNum);
#else
void ripTimer(void);
#endif /* VIRTUAL_STACK */
IMPORT void ripTimerArm (long timeout);
IMPORT STATUS routedIfInit (BOOL resetFlag, long ifIndex);
IMPORT void ripTimeSet (struct timeval *pTimer);

/*
 * Timer routine.  Performs routing information supply
 * duties and manages timers on routing table entries.
 * Management of the RTS_CHANGED bit assumes that we broadcast
 * each time called.
 */

void ripTimer
    (
#ifdef VIRTUAL_STACK
    int stackNum
#else
    void
#endif
    )
    {
    register struct rthash *rh;
    register struct rt_entry *rt;
    struct rthash *base;
    int doinghost;
    int timetobroadcast;
    int diffTime;
    int deltaTime;
    int time;
    int timeLeft;

#ifdef VIRTUAL_STACK

    /* Assign virtual stack number to access appropriate data structures. */

    if (virtualStackNumTaskIdSet (stackNum) == ERROR)
        {
        if (routedDebug)
            logMsg ("ripTimerTask: unable to access stack data.\n",
                    0, 0, 0, 0, 0, 0);
        return;
        }
#endif /* VIRTUAL_STACK */

    FOREVER 
        {
        /* Wait for the semaphore given by the watchdog routine. */

        semTake(ripState.timerSem, WAIT_FOREVER);

        /* 
         * Block until any processing of incoming messages has completed.
         * This exclusion guarantees that the routing entries are stable
         * during updates and that the timer values shared with the
         * triggered update scheduling will be set atomically.
         */

        semTake (ripLockSem, WAIT_FOREVER);
        ripTimeSet (&ripState.now);

        /* Calculate the time difference from when we last checked */

        diffTime = (u_long)ripState.now.tv_sec - (u_long)ripState.then.tv_sec;

        /* If the timer has wrapped around, account for that */

        if (diffTime < 0)
            diffTime = -diffTime;

        /* 
         * Now round off the time to an integer multiple of timerRate.
         * Whatever time remains after rounding off, will be adjusted
         * when we restart the timer below
         */

        if (diffTime < ripState.timerRate)
            {
            deltaTime = 0;
            diffTime = ripState.timerRate;
            }
        else
            {
            deltaTime =  diffTime % ripState.timerRate;
            diffTime = (diffTime / ripState.timerRate) * ripState.timerRate;
            }

        /* Record the time we got above */

        ripState.then = ripState.now;

        /*
         * Now check if the time elapsed (diffTime) covers an update
         * point. If so we missed an update. We should send an update
         * right now. Since the calculation below sends out updates
         * only at multiples of the supply interval, we should
         * increment the time only to the extent that it becomes an
         * integer multiple of the supply interval. After deciding that we need
         * to send an update, we'll add the remaining time. Note, that
         * if so much time had elapsed such that 2 updates (supplies)
         * could have been sent, we still send out only one as in the RIP
         * protocol two or more immediate updates are equivalent to
         * a single update
         *
         *        |last update             | new update (scheduled)
         *        |                        |
         *        |                        |      actual update (delayed)
         *        |________________________|_____|________________
         *        0              |         30                     | 60
         *                       |<-------------->                |
         *                       |   diffTime                     | next update
         *                       |
         *                       fake time
         *
         */
        time = ripState.faketime % ripState.supplyInterval;
        if ((time + diffTime) > ripState.supplyInterval)
            {
            time = ripState.supplyInterval - time;
            timeLeft = diffTime - time;
            diffTime -= timeLeft;
            }
        else
            timeLeft = 0;
        ripState.faketime += diffTime;
        if (ripState.lookforinterfaces)
            {
            if (routedDebug)
                logMsg ("Calling routedIfInit.\n", 0, 0, 0, 0, 0, 0);
            routedIfInit (FALSE, 0);
            }
        timetobroadcast = ripState.supplier &&
            ((ripState.faketime % ripState.supplyInterval) == 0);
        ripState.faketime += timeLeft;

        /* Restart the timer. */

        ripTimerArm (ripState.timerRate - deltaTime);

        base = hosthash;
        doinghost = 1;

        again:

        for (rh = base; rh < &base[ROUTEHASHSIZ]; rh++)
            {
            rt = rh->rt_forw;
            for (; rt != (struct rt_entry *)rh; rt = rt->rt_forw)
                {
                /*
                 * We don't advance time on a routing entry for
                 * a passive gateway, or any interface.
                 */

                if (!(rt->rt_state & RTS_PASSIVE) &&
                    !(rt->rt_state & RTS_INTERFACE) &&
                    !(rt->rt_state & RTS_OTHER))
                    rt->rt_timer += ripState.timerRate;

                if (rt->rt_timer >= ripState.garbage)
                    {
                    rt = rt->rt_back;

                    rtdelete (rt->rt_forw);

                    continue;
                    }

                /* 
                 * If the timer expired, then we should delete the 
                 * route from the system routing database. The 
                 * check for inKernel allows us to delete valid routes
                 * that were advertised with a metric of 15 (and added
                 * to our table with a metric of 16). After deleting the
                 * route, rt->inKernel will become FALSE and rt->rt_metric
                 * will be set to HOPCNT_INFINITY (16) so that we would
                 * not call rtchange again.
                 */

                if (rt->rt_timer >= ripState.expire && 
                    (rt->inKernel || rt->rt_metric < HOPCNT_INFINITY))
                    rtchange(rt, &rt->rt_router, HOPCNT_INFINITY, NULL,
                             rt->rt_tag, 0, NULL);

		/* 
		 * Only reset the flag after a broadcast is done
		 */

                if (timetobroadcast)
		    rt->rt_state &= ~RTS_CHANGED;
                }
            }

        if (doinghost)
            {
            doinghost = 0;
            base = nethash;
            goto again;
            }

        /*
         * Send a periodic update if necessary and reset the timer
         * and flag settings to suppress any pending triggered updates.
         */

        if (timetobroadcast)
            {
            toall (supply, 0, (struct interface *)NULL);

            ripState.lastbcast = ripState.now;
            ripState.lastfullupdate = ripState.now;
            ripState.needupdate = 0;
            ripState.nextbcast.tv_sec = 0;
            }

        semGive (ripLockSem); 	/* End of critical section. */

        }
    }

/* uipc_dom.c - domain routines */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1982, 1986, 1993, 1995
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
 *
 *	@(#)uipc_domain.c	8.3 (Berkeley) 2/14/95
 */

/*
modification history
--------------------
01d,12oct01,rae  merge from truestack ver 01e, base 01c (SPR #69112 etc.)
01c,01jul97,vin  modified for making routing sockets scalable, removed
		 unnecessary max_datalen, added addDomain function.
01b,22nov96,vin  modified max_datalen variable to new cluster size.
01a,03mar96,vin  created from BSD4.4 stuff and integrated with 02n version
		 of uipc_dom.c. Did not change the name as it would cause
		 name conflict when releasing the 44stack as a component.
*/

#include "vxWorks.h"
#include "wdLib.h"
#include "errno.h"
#include "sys/socket.h"
#include "net/protosw.h"
#include "net/domain.h"
#include "net/mbuf.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#else
IMPORT void netJobAdd ();
#endif

IMPORT int  sysClkRateGet(); 

#ifndef VIRTUAL_STACK
struct domain *domains;		/* list of domain descriptors */

LOCAL WDOG_ID pfslowtimoWd;	/* watchdog timer for pfslowtimo routine */
LOCAL WDOG_ID pffasttimoWd;	/* watchdog timer for pffasttimo routine */

static void pfslowtimo (void);
static void pffasttimo (void); 
#else
static void pfslowtimo (int stackNum);
static void pffasttimo (int stackNum); 
#endif /* VIRTUAL_STACK */

int	max_linkhdr;		/* largest link-level header */
int	max_protohdr;		/* largest protocol header */
int	max_hdr;		/* largest link+protocol header */

/*******************************************************************************
* addDomain - add the domain to the domain list
*
* This function adds the domain passed to it into a global domain list
* This global domain list is used by various routines to get to the
* protocols. After adding all the domains then call domaininit ().
* This function is to be called only once per domain at the initialization
* time.
*
* NOTE: No checking is done if a domain is added multiple times. That would be
*       insane.
*
* RETURNS: OK/ERROR
*
* NOMANUAL
*/

int addDomain
    (
    struct domain * pDomain		/* pointer to the domain to add */
    )
    {

    if (pDomain == NULL)
        return (ERROR);

    /* add the domain to the global list */
    
    pDomain->dom_next = domains;	
    domains = pDomain;
    
    return (OK); 
    }

void domaininit(void)
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next) {
		if (dp->dom_init)
			(*dp->dom_init)();
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_init)
				(*pr->pr_init)();
	}

        if (max_linkhdr < 16)		/* XXX */
            max_linkhdr = 16;
	max_hdr = max_linkhdr + max_protohdr;

        pffasttimoWd = wdCreate ();
        pfslowtimoWd = wdCreate ();

#ifdef VIRTUAL_STACK
	pffasttimo (myStackNum);
	pfslowtimo (myStackNum);
#else
	pffasttimo ();
	pfslowtimo ();
#endif
}

struct protosw *
pffindtype(family, type)
	int family;
	int type;
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		if (dp->dom_family == family)
			goto found;
	return (0);
found:
	for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
		if (pr->pr_type && pr->pr_type == type)
			return (pr);
	return (0);
}

struct protosw *
pffindproto(family, protocol, type)
	int family, protocol, type;
{
	register struct domain *dp;
	register struct protosw *pr;
	struct protosw *maybe = 0;

	if (family == 0)
		return (0);
	for (dp = domains; dp; dp = dp->dom_next)
		if (dp->dom_family == family)
			goto found;
	return (0);
found:
	for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++) {
		if ((pr->pr_protocol == protocol) && (pr->pr_type == type))
			return (pr);

		if (type == SOCK_RAW && pr->pr_type == SOCK_RAW &&
		    pr->pr_protocol == 0 && maybe == (struct protosw *)0)
			maybe = pr;
	}
	return (maybe);
}

int
net_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	register struct domain *dp;
	register struct protosw *pr;
	int family, protocol;

	/*
	 * All sysctl names at this level are nonterminal;
	 * next two components are protocol family and protocol number,
	 * then at least one addition component.
	 */
	if (namelen < 3)
		return (EISDIR);		/* overloaded */
	family = name[0];
	protocol = name[1];

	if (family == 0)
		return (0);
	for (dp = domains; dp; dp = dp->dom_next)
		if (dp->dom_family == family)
			goto found;
	return (ENOPROTOOPT);
found:
	for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
		if (pr->pr_protocol == protocol && pr->pr_sysctl)
			return ((*pr->pr_sysctl)(name + 2, namelen - 2,
			    oldp, oldlenp, newp, newlen));
	return (ENOPROTOOPT);
}

void
pfctlinput(cmd, sa)
	int cmd;
	struct sockaddr *sa;
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_ctlinput)
				(*pr->pr_ctlinput)(cmd, sa, (caddr_t)0);
}

#ifdef VIRTUAL_STACK
static void pfslowtimoRestart
    (
    int stackNumber
    )
    {
    netJobAdd ((FUNCPTR)pfslowtimo, stackNumber, 2, 3, 4, 5);
    }

static void pfslowtimo
    (
    int stackNum
    )
#else
static void pfslowtimo (void)
#endif
    {
    register struct domain *dp;
    register struct protosw *pr;
    int s;

#ifdef VIRTUAL_STACK    
    virtualStackNumTaskIdSet(stackNum);
#endif

    s = splnet();

    for (dp = domains; dp; dp = dp->dom_next)
        for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
            if (pr->pr_slowtimo)
                (*pr->pr_slowtimo)();

    splx (s);

#ifdef VIRTUAL_STACK
    wdStart (pfslowtimoWd, sysClkRateGet()/2, (FUNCPTR) pfslowtimoRestart,
             (int) myStackNum);
#else
    wdStart (pfslowtimoWd, sysClkRateGet()/2, (FUNCPTR) netJobAdd,
             (int) pfslowtimo);
#endif
    }

#ifdef VIRTUAL_STACK
static void pffasttimoRestart
    (
    int stackNumber
    )
    {
    netJobAdd ((FUNCPTR)pffasttimo, stackNumber, 2, 3, 4, 5);
    }

static void pffasttimo
    (
    int stackNum
    )
#else
static void pffasttimo (void)
#endif
    {
    register struct domain *dp;
    register struct protosw *pr;

    int s;

#ifdef VIRTUAL_STACK
    virtualStackNumTaskIdSet(stackNum);
#endif

    s = splnet();

    for (dp = domains; dp; dp = dp->dom_next)
        for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
            if (pr->pr_fasttimo)
                (*pr->pr_fasttimo)();

    splx (s);

#ifdef VIRTUAL_STACK    
    wdStart (pffasttimoWd, sysClkRateGet()/5, (FUNCPTR) pffasttimoRestart, 
             (int) stackNum);
#else
    wdStart (pffasttimoWd, sysClkRateGet()/5, (FUNCPTR) netJobAdd, 
             (int) pffasttimo);
#endif
    }

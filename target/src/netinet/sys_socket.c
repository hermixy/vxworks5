/* sys_socket.c - socket ioctl routines */

/* Copyright 1984 - 2000 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *      @(#)sys_socket.c        7.3 (Berkeley) 6/29/88
 */

/*
modification history
--------------------
02l,12oct01,rae  merge from truestack (compilation warnings, WindView)
02k,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
02j,26may92,rrr  the tree shuffle
02i,02dec91,elh  changed error handling to return ERROR (on error)
		 with errno set with return value.
02h,04oct91,rrr  passed through the ansification filter
		  -changed includes to have absolute path from h/
		  -changed copyright notice
02g,19apr90,hjb  deleted param.h
02f,16mar90,rdc  added select stuff.
02e,16apr89,gae  updated to new 4.3BSD.
02d,14dec87,gae  added include of vxWorks.h.
02c,29apr87,dnw  removed unnecessary includes.
		 removed unimplemented FIO_ASYNC ioctl ops from soo_ioctl().
02b,03apr87,ecs  added header and copyright.
02a,02feb87,jlf  Removed CLEAN ifdefs.
*/

/*
DESCRIPTION
*/

#include "vxWorks.h"
#include "net/mbuf.h"
#include "net/protosw.h"
#include "sys/socket.h"
#include "net/socketvar.h"
#include "sys/ioctl.h"
#include "net/uio.h"
#include "selectLib.h"

#include "net/if.h"
#include "net/route.h"
#include "errno.h"

#include "ioLib.h"

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
#include "wvNetLib.h"
#endif
#endif

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET
    /* Set common fields of event identifiers for this module. */

LOCAL UCHAR wvNetModuleId = WV_NET_SYSSOCK_MODULE; /* Value for sys_socket.c */
LOCAL UCHAR wvNetLocalFilter = WV_NET_NONE;     /* Available event filter */

LOCAL ULONG wvNetEventId;       /* Event identifier: see wvNetLib.h */
#endif    /* INCLUDE_WVNET */
#endif

extern void sbselqueue ();
extern void sbseldequeue ();

int
soo_ioctl(so, cmd, data)
	struct socket *so;
	int cmd;
	register caddr_t data;
{
	int	status;

#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
        WV_NET_MARKER_2 (NET_AUX_EVENT, WV_NET_INFO, 23, 1,
                         WV_NETEVENT_SOCKIOCTL_START, so->so_fd, cmd)
#endif  /* INCLUDE_WVNET */
#endif

	switch (cmd) {

	case FIONBIO:
		if (*(int *)data)
			so->so_state |= SS_NBIO;
		else
			so->so_state &= ~SS_NBIO;
		return (0);

	case FIONREAD:
		*(int *)data = so->so_rcv.sb_cc;
		return (0);

	case SIOCSPGRP:
		so->so_pgrp = *(int *)data;
		return (0);

	case SIOCGPGRP:
		*(int *)data = so->so_pgrp;
		return (0);

	case SIOCATMARK:
		*(int *)data = (so->so_state&SS_RCVATMARK) != 0;
		return (0);

	case FIOSELECT:
		soo_select (so, (SEL_WAKEUP_NODE *) data);
		return (0);

	case FIOUNSELECT:
		soo_unselect (so, (SEL_WAKEUP_NODE *) data);
		return (0);

	}
	/*
	 * Interface/routing/protocol specific ioctls:
	 * interface and routing ioctls should have a
	 * different entry since a socket's unnecessary
	 */
#define	cmdbyte(x)	(((x) >> 8) & 0xff)
	if (cmdbyte(cmd) == 'i')
		status = ifioctl(so, cmd, data);
	else {
		if (cmdbyte(cmd) == 'r')
			status = rtioctl(cmd, data);
		else
			status = ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
	    			 (struct mbuf *)cmd, (struct mbuf *)data,
				 (struct mbuf *)0));
	}
	if (status != 0)
		{
		errno = status;
		return (ERROR);
		}
	else
		return (OK);
}

int
soo_select (so, wakeupNode)
    struct socket *so;
    SEL_WAKEUP_NODE *wakeupNode;
    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_INFO, 24, 2,
                     WV_NETEVENT_SOCKSELECT_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

    switch (selWakeupType (wakeupNode))
	{

	case SELREAD:
	    sbselqueue(so, &so->so_rcv, wakeupNode);
	    if (soreadable(so))
		{
		selWakeup (wakeupNode);
		return (1);
		}
	    break;

	case SELWRITE:
	    sbselqueue(so, &so->so_snd, wakeupNode);
	    if (sowriteable(so))
		{
		selWakeup (wakeupNode);
		return (1);
		}
	    break;

	default:
	    return (ERROR);

	}
    return (0);
    }

int
soo_unselect (so, wakeupNode)
    struct socket *so;
    SEL_WAKEUP_NODE *wakeupNode;
    {
#ifdef WV_INSTRUMENTATION
#ifdef INCLUDE_WVNET    /* WV_NET_INFO event */
    WV_NET_MARKER_1 (NET_AUX_EVENT, WV_NET_INFO, 25, 3,
                     WV_NETEVENT_SOCKUNSELECT_START, so->so_fd)
#endif  /* INCLUDE_WVNET */
#endif

    switch (selWakeupType (wakeupNode))
	{
	case SELREAD:
	    sbseldequeue(so, &so->so_rcv, wakeupNode);
	    break;

	case SELWRITE:
	    sbseldequeue(so, &so->so_snd, wakeupNode);
	    break;

	default:
	    return (ERROR);

	}
    return (0);
    }

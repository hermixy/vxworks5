/* pingLib.c - Packet InterNet Groper (PING) library */

/* Copyright 1994 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01r,22apr02,rae  Note that PING_OPT_DONTROUTE affects pinging localhost
                 (SPR #72917), other minor changes
01q,11mar02,rae  Print stats when task killed (SPR #73570)
01p,08jan02,rae  Don't print error messages when PING_OPT_SILENT (SPR #69537)
01o,15oct01,rae  merge from truestack ver 01q, base 01k (SPRs 67440,
                 30151, 66062, etc.)
01n,30oct00,gnn  Added PING_OPT_NOHOST flag to deal with SPR 22766
01m,08nov99,pul  T2 cumulative patch 2
01l,22sep99,cno  corrected ping error for pingRxPrint() (SPR22571)
01k,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01j,12mar99,p_m  Fixed SPR 8742 by documentating ping() configuration global
                 variables.
01i,05feb99,dgp  document errno values
01h,17mar98,jmb  merge jmb patch of 04apr97 from HPSIM: corrected
                 creation/deletion of task delete hook.
01g,30oct97,cth  changed stack size of tPingTxn from 3000 to 6000 (SPR 8222).
01f,26aug97,spm  removed compiler warnings (SPR #7866)
01e,30sep96,spm  corrected ping error for little-endian machines (SPR #4235)
01d,13mar95,dzb  changed to use free() instead of cfree() (SPR #4113)
01c,24jan95,jdi  doc tweaks
01b,10nov94,rhp  minor edits to man pages
01a,25oct94,dzb  written
*/

/*
DESCRIPTION
This library contains the ping() utility, which tests the reachability
of a remote host.

The routine ping() is typically called from the VxWorks shell to check the
network connection to another VxWorks target or to a UNIX host.  ping()
may also be used programmatically by applications that require such a test.
The remote host must be running TCP/IP networking code that responds to
ICMP echo request packets.  The ping() routine is re-entrant, thus may
be called by many tasks concurrently.

The routine pingLibInit() initializes the ping() utility and allocates
resources used by this library.  It is called automatically when
INCLUDE_PING is defined.
*/

/* includes */

#include "vxWorks.h"
#include "string.h"
#include "stdioLib.h"
#include "wdLib.h"
#include "netLib.h"
#include "sockLib.h"
#include "inetLib.h"
#include "semLib.h"
#include "taskLib.h"
#include "hostLib.h"
#include "ioLib.h"
#include "tickLib.h"
#include "taskHookLib.h"
#include "sysLib.h"
#include "vxLib.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"
#include "netinet/ip_icmp.h"
#include "netinet/icmp_var.h"
#include "pingLib.h"
#include "errnoLib.h"
#include "kernelLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif /* VIRTUAL_STACK */

/* defines */

#define pingError(pPS)	{ pPS->flags |= PING_OPT_SILENT; goto release; }

/* globals */

int	_pingTxLen = 64;			/* size of icmp echo packet */
int	_pingTxInterval = PING_INTERVAL;	/* packet interval in seconds */
int	_pingTxTmo = PING_TMO;			/* packet timeout in seconds */
extern	int errno;

/* locals */

LOCAL PING_STAT	*	pingHead = NULL;	/* ping list head */
LOCAL SEM_ID		pingSem = NULL;		/* mutex for list access */

/* static forward declarations */

LOCAL STATUS pingRxPrint (PING_STAT *pPS, int len, struct sockaddr_in *from,
                          ulong_t now);
LOCAL void pingFinish (WIND_TCB *);

/*******************************************************************************
*
* pingLibInit - initialize the ping() utility
*
* This routine allocates resources used by the ping() utility.
* It is called automatically when INCLUDE_PING is defined.
*
* RETURNS:
* OK
*/

STATUS pingLibInit (void)
    {
    if (pingSem == NULL)			/* already initialized ? */
	{
	if ((pingSem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE |
	    SEM_INVERSION_SAFE)) == NULL)
	    return (ERROR);
        }

    return (OK);
    }

/*******************************************************************************
*
* ping - test that a remote host is reachable
*
* This routine tests that a remote host is reachable by sending ICMP
* echo request packets, and waiting for replies.  It may called from
* the VxWorks shell as follows:
* .CS
*    -> ping "remoteSystem", 1, 0
* .CE
* where <remoteSystem> is either a host name that has been previously added
* to the remote host table by a call to hostAdd(), or an Internet address in
* dot notation (for example, "90.0.0.2").
*
* The second parameter, <numPackets>, specifies the number of ICMP packets
* to receive from the remote host.  If <numPackets> is 1, this routine waits
* for a single echo reply packet, and then prints a short message
* indicating whether the remote host is reachable.  For all other values
* of <numPackets>, timing and sequence information is printed as echoed
* packets are received.  If <numPackets> is 0, this routine runs continuously.
* 
* If no replies are received within a 5-second timeout period, the
* routine exits.  An ERROR status is returned if no echo replies
* are received from the remote host.
*
* The following flags may be given through the <options> parameter:
* .iP PING_OPT_SILENT
* Suppress output.  This option is useful for applications that 
* use ping() programmatically to examine the return status.
* .iP PING_OPT_DONTROUTE
* Do not route packets past the local network.  This also prevents pinging
* local addresses (i.e. the IP address of the host itself).  The 127.x.x.x 
* addresses will still work however.
* .iP PING_OPT_NOHOST
* Suppress host lookup.  This is useful when you have the DNS resolver
* but the DNS server is down and not returning host names.
* .iP PING_OPT_DEBUG
* Enables debug output.
* .RS 4 4
* \&NOTE: The following global variables can be set from the target shell
* or Windsh to configure the ping() parameters:
* .iP _pingTxLen 
* Size of the ICMP echo packet (default 64).
* .iP _pingTxInterval
* Packet interval in seconds (default 1 second).
* .iP _pingTxTmo
* Packet timeout in seconds (default 5 seconds).
*
*.RE
* 
* RETURNS:
* OK, or ERROR if the remote host is not reachable.
*
* ERRNO: EINVAL, S_pingLib_NOT_INITIALIZED, S_pingLib_TIMEOUT
*
*/

STATUS ping
    (
    char *		host,		/* host to ping */
    int			numPackets,	/* number of packets to receive */
    ulong_t		options		/* option flags */
    )
    {
    PING_STAT *		pPS;			/* current ping stat struct */
    struct sockaddr_in	to;			/* addr of Tx packet */
    struct sockaddr_in	from;			/* addr of Rx packet */
    int			fromlen = sizeof (from);/* size of Rx addr */
    int			ix = 1;			/* bytes read */
    int			txInterval;		/* packet interval in ticks */
    STATUS		status = ERROR;		/* return status */
    struct fd_set	readFd;
    struct timeval	pingTmo;
    ulong_t		now;
    int			sel;

#ifndef ALT_PING_PAR_SEMANTIC
    if (numPackets < 0)                         /* numPackets positive ? */
        {
        errno = EINVAL;
        return (ERROR);
        }
#else /* alternative parameter semantic for numPackets */
    if (numPackets < -1)		/* numPackets positive or -1 ? */
	{
	errno = EINVAL;
	return (ERROR);
	}
    if (numPackets == 0)
    	numPackets = 3;		/* don't do infinite by default */
    if (numPackets == -1)
    	numPackets = 0;		/* infinite */
#endif /* ALT_PING_PAR_SEMANTIC */

    /* allocate size for ping statistics/info structure */
    if ((pPS = (PING_STAT *) calloc (1, sizeof (PING_STAT))) == NULL)
	return (ERROR);

#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
    pPS->vsid = myStackNum;
#endif /* VIRTUAL_STACK */

    semTake (pingSem, WAIT_FOREVER);		/* get access to list */
    pPS->statNext = pingHead;			/* push session onto list */
    if (pingHead == NULL && !(options & PING_OPT_SILENT))
        if (taskDeleteHookAdd ((FUNCPTR) pingFinish) == ERROR)
	    {
	    free ((char *) pPS);
	    semGive (pingSem);			/* give up access to list */
            if (options & PING_OPT_DEBUG)
            	printf ("ping: unable to add task Delete hook\n");
	    return (ERROR);
	    }
    pingHead = pPS;
    semGive (pingSem);				/* give up access to list */

    pPS->tMin = 999999999;			/* init min rt time */
    pPS->numPacket = numPackets;		/* save num to send */
    pPS->flags = options;			/* save flags field */
    pPS->clkTick = sysClkRateGet ();		/* save sys clk rate */

    txInterval = _pingTxInterval * pPS->clkTick;/* init interval value */

    pingTmo.tv_sec = _pingTxTmo;
    pingTmo.tv_usec = 0;

    pPS->pBufIcmp = (struct icmp *) pPS->bufTx;	/* pointer to icmp header out */
    pPS->pBufTime = (ulong_t *) (pPS->bufTx + 8);/* pointer to time out */

    pPS->idRx = taskIdSelf ();			/* get own task Id  */

    /* initialize the socket address struct */

    to.sin_family = AF_INET;
    if ((to.sin_addr.s_addr = inet_addr (host)) == (u_long) ERROR)
	{
	if ((to.sin_addr.s_addr = hostGetByName (host)) == (u_long) ERROR)
	    {
            if (!(options & PING_OPT_SILENT))
	        printf ("ping: unknown host %s\n", host);

	    pingError (pPS);
	    }

	inet_ntoa_b (to.sin_addr, pPS->toInetName);
        }
    
    strcpy (pPS->toHostName, host);		   /* save host name */
    _pingTxLen = max (_pingTxLen, PING_MINPACKET); /* sanity check global */
    _pingTxLen = min (_pingTxLen, PING_MAXPACKET); /* sanity check global */
    pPS->dataLen = _pingTxLen - 8;		   /* compute size of data */

    /* open raw socket for ICMP communication */

    if ((pPS->pingFd = socket (AF_INET, SOCK_RAW, ICMP_PROTO)) < 0)
	pingError (pPS);

    if (options & PING_OPT_DONTROUTE)		/* disallow packet routing ? */
        if (setsockopt (pPS->pingFd, SOL_SOCKET, SO_DONTROUTE, (char *) &ix,
	    sizeof (ix)) == ERROR)
	    pingError (pPS);

    if (!(options & PING_OPT_SILENT) && pPS->numPacket != 1)
	{
        printf ("PING %s", pPS->toHostName);	/* print out dest info */
        if (pPS->toInetName[0])
            printf (" (%s)", pPS->toInetName);

        printf (": %d data bytes\n", pPS->dataLen);
	}
    
    pPS->pBufIcmp->icmp_type = ICMP_ECHO;	/* set up Tx buffer */
    pPS->pBufIcmp->icmp_code = 0;
    pPS->pBufIcmp->icmp_id = pPS->idRx & 0xffff;


    for (ix = 4; ix < pPS->dataLen; ix++)	/* skip 4 bytes for time */
        pPS->bufTx [8 + ix] = ix;


     /* receive echo reply packets from remote host */
    while (!pPS->numPacket || (pPS->numRx != pPS->numPacket))
	{

	*pPS->pBufTime = tickGet ();		/* load current tick count */
        pPS->pBufIcmp->icmp_seq = pPS->numTx++;	/* increment seq number */
        pPS->pBufIcmp->icmp_cksum = 0;
	pPS->pBufIcmp->icmp_cksum = checksum ((u_short *) pPS->pBufIcmp,
					      _pingTxLen);
        /* transmit ICMP packet */

	if ((ix = sendto (pPS->pingFd, (char *) pPS->pBufIcmp,
			  _pingTxLen, 0, (struct sockaddr *)&to,
			  sizeof (struct sockaddr))) != _pingTxLen)
	    if (pPS->flags & PING_OPT_DEBUG)
	        printf ("ping: wrote %s %d chars, ret=%d\n", pPS->toHostName,
		    _pingTxLen, ix);

        /* Update ICMP statistics for ECHO messages - this is not the best
	 * place to put it since this shifts responsibility of updating ECHO
	 * statistics to anyone writing an application that sends out ECHO
	 * messages. However, this seems to be the only place to put it.
         */ 
#ifdef VIRTUAL_STACK
        _icmpstat.icps_outhist [ICMP_ECHO]++;
#else
        icmpstat.icps_outhist [ICMP_ECHO]++;
#endif /* VIRTUAL_STACK */

check_fd_again:					/* Wait for ICMP reply */
	FD_ZERO(&readFd);
	FD_SET(pPS->pingFd, &readFd);

	sel = select (pPS->pingFd+1, &readFd, NULL, NULL, &pingTmo);
        if (sel == ERROR)
	    {
	    errno = errnoGet();
	    if (!(options & PING_OPT_SILENT))
                printf ("ping: ERROR\n");
	    break; /* goto release */
	    }
	else if (sel == 0)
	    {
	    if (!(options & PING_OPT_SILENT))
                printf ("ping: timeout\n");
            errno = S_pingLib_TIMEOUT;		/* timeout error */
	    break; /* goto release */
	    }

	if (!FD_ISSET(pPS->pingFd, &readFd))
            goto check_fd_again;

	/* the fd is ready - FD_ISSET isn't needed */	
	if ((ix = recvfrom (pPS->pingFd, (char *) pPS->bufRx, PING_MAXPACKET,
	    0, (struct sockaddr *) &from, &fromlen)) == ERROR)
            {
	    if (errno == EINTR)
	        goto check_fd_again;
	    break; /* goto release */
	    }
	now = tickGet();
	if (pingRxPrint (pPS, ix, &from, now) == ERROR)
	    goto check_fd_again;
	taskDelay (txInterval);
        }

    if (pPS->numRx > 0)
	status = OK;				/* host is reachable */

release:
    pingFinish (taskTcb (pPS->idRx));
    return (status);
    }

/*******************************************************************************
*
* pingRxPrint - print out information about a received packet
*
* This routine prints out information about a received ICMP echo reply
* packet.  First, the packet is checked for minimum length and
* correct message type and destination.
*
* RETURNS:
* N/A.
*/

LOCAL STATUS pingRxPrint
    (
    PING_STAT *		pPS,		/* ping stats structure */
    int			len,		/* Rx message length */
    struct sockaddr_in *from,		/* Rx message address */
    ulong_t		now
    )
    {
    struct ip *		ip = (struct ip *) pPS->bufRx;
    long *		lp = (long *) pPS->bufRx;
    struct icmp *	icp;
    int			ix;
    int			hlen;
    int			triptime;
    char		fromHostName [MAXHOSTNAMELEN + 1];
    char		fromInetName [INET_ADDR_LEN];
    
    /* convert address notation */

    inet_ntoa_b (from->sin_addr, fromInetName);

    /* Do we lookup hosts or not? */
    if (pPS->flags & PING_OPT_NOHOST)
        *fromHostName = EOS;
    else
        {
        if ((hostGetByAddr (from->sin_addr.s_addr, fromHostName)) == ERROR)
            *fromHostName = EOS;		/* hostname not found */
        }
    
    hlen = ip->ip_hl << 2;

    if (len < hlen + ICMP_MINLEN)		/* at least min length ? */
	{
	if (pPS->flags & PING_OPT_DEBUG)
	    printf ("packet too short (%d bytes) from %s\n", len,
		fromInetName);
	return (ERROR);
        }

    len -= hlen;				/* strip IP header */
    icp = (struct icmp *) (pPS->bufRx + hlen);

    if (icp->icmp_type != ICMP_ECHOREPLY)	/* right message ? */
	{
	if (pPS->flags & PING_OPT_DEBUG)	/* debug odd message */
	    {
	    if (*fromHostName != (char)NULL)
                printf ("%d bytes from %s (%s): ", len, fromHostName,
		    fromInetName);
	    else 
	        printf ("%d bytes from %s: ", len, fromInetName);
	    icp->icmp_type = min (icp->icmp_type, ICMP_TYPENUM); 
	    printf ("icmp_type=%d\n", icp->icmp_type);
	    for(ix = 0; ix < 12; ix++)
	        printf ("x%2.2lx: x%8.8lx\n", (ULONG)(ix * sizeof (long)),
                        (ULONG)*lp++);

	    printf ("icmp_code=%d\n", icp->icmp_code);
	    }

	return (ERROR);
        }

    /* check if the received reply is ours. */
    if (icp->icmp_id != (pPS->idRx & 0xffff))
	{
	return (ERROR);					/* wasn't our ECHO */
	}

    /* print out Rx packet stats */
    if (!(pPS->flags & PING_OPT_SILENT) && pPS->numPacket != 1)
	{
	if (*fromHostName != (char)NULL)
            printf ("%d bytes from %s (%s): ", len, fromHostName, fromInetName);
	else 
            printf ("%d bytes from %s: ", len, fromInetName);
        printf ("icmp_seq=%d. ", icp->icmp_seq);
        triptime = (now - *((ulong_t *) icp->icmp_data)) *
	    (1000 / pPS->clkTick);
        printf ("time=%d. ms\n", triptime);
        pPS->tSum += triptime;
        pPS->tMin = min (pPS->tMin, triptime);
        pPS->tMax = max (pPS->tMax, triptime);
	}

    pPS->numRx++;
    return (OK);
    }

/*******************************************************************************
*
* pingFinish - return all allocated resources and print out final statistics
*
* This routine returns all resources allocated for the ping session, and
* prints out a stats summary.
*
* The ping session is located in the session list (pingHead) by searching
* the ping stats structure for the receiver task ID.  This is necessary
* because this routine is passed a pointer to the task control block, and does
* not have ready access to the ping stats structure itself.  This accomodates
* the use of task delete hooks as a means of calling this routine.
*
* RETURNS:
* N/A.
*/

LOCAL void pingFinish
    (
    WIND_TCB *		pTcb		/* pointer to task control block */
    )
    {
    PING_STAT *		pPS;			/* ping stats structure */
    PING_STAT **	ppPrev;			/* pointer to prev statNext */

    semTake (pingSem, WAIT_FOREVER);		/* get list access */
    ppPrev = &pingHead;
    for (pPS = pingHead; (pPS != NULL) && (taskTcb (pPS->idRx) != pTcb);
        pPS = pPS->statNext)			/* find session in list */
        ppPrev = &pPS->statNext;

    if (pPS == NULL)				/* session found ? */
	{
        semGive (pingSem);			/* give up list access */
	return;
	}

    *ppPrev = pPS->statNext;			/* pop session off list */

    if (pingHead == NULL)
        (void) taskDeleteHookDelete ((FUNCPTR) pingFinish); 

    semGive (pingSem);				/* give up list access */

    /* return all allocated/created resources */

#ifdef VIRTUAL_STACK
    if (!(pPS->flags & PING_OPT_SILENT) &&
        virtualStackNumTaskIdSet(pPS->vsid) == ERROR)
        {
        printf("pingFinish: invalid virtual stack id\n");
        return;
        }
#endif /* VIRTUAL_STACK */

    if (pPS->pingFd)
        (void) close (pPS->pingFd);

    if (!(pPS->flags & PING_OPT_SILENT))	/* print final report ? */
	{
        if (pPS->numRx)				/* received at least one ? */
	    {
	    if (pPS->numPacket != 1)		/* full report */
		{
                printf ("----%s PING Statistics----\n", pPS->toHostName);
                printf ("%d packets transmitted, ", pPS->numTx);
                printf ("%d packets received, ", pPS->numRx);

                if (pPS->numTx)
                    printf ("%d%% packet loss", ((pPS->numTx - pPS->numRx) *
			100) / pPS->numTx);
                printf ("\n");

                if (pPS->numRx)
                    printf ("round-trip (ms)  min/avg/max = %d/%d/%d\n",
			pPS->tMin, pPS->tSum / pPS->numRx, pPS->tMax);
                }
            else				/* short report */
	        printf ("%s is alive\n", pPS->toHostName);
            }
        else
	    printf ("no answer from %s\n", pPS->toHostName);
	}

    free ((char *)pPS);		/* free stats memory space */
    }

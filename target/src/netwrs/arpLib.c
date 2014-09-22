/* arpLib.c - Address Resolution Protocol (ARP) table manipulation library */

/* Copyright 1984 - 2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02q,10may02,kbw  making man page edits
02p,24apr02,rae  Added note about arp flooding to arpResolve(), etc. (SPR #69412)
02o,07dec01,rae  merge from synth ver 02x (SPR #69889)
02n,15oct01,rae  merge from truestack ver 02w base 02j (SPRs 69405, 65783, etc.)
02m,07feb01,spm  added merge record for 30jan01 update from version 02l of
                 tor2_0_x branch (base 02j) and fixed modification history;
                 fixed code conventions and updated documentation; replaced
                 printed error message with errno value
02l,30jan01,ijm  merged SPR# 28602 fixes: proxy ARP services are obsolete 
02k,05feb99,dgp  document errno values
02j,16apr97,vin  changed SOCK_DGRAM to SOCK_RAW as udp can be scaled out.
02i,11aug93,jmm  Changed ioctl.h and socket.h to sys/ioctl.h and sys/socket.h
02h,21sep92,jdi  documentation cleanup. 
02g,14aug92,elh  documentation changes. 
02f,11jun92,elh  changed parameter to arpCmd.  Moved arpShow to netShow.
02e,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
02d,19apr92,elh  added spl pair around arpFlush.
02c,04apr92,elh  added arpFlush.
02b,03jan92,elh  ansi-fied.
02a,18nov91,elh  major overhaul.  Rewrote to comply with WRS standards and
		 coding conventions, added error handling and documentation.
01a,06jun91,jrb  written.
*/

/*
DESCRIPTION
This library provides direct access to the address translation table
maintained by the Address Resolution Protocol (ARP). Each entry in
the table maps an Internet Protocol (IP) address to a physical hardware 
address. This library supports only those entries that translate between IP 
and Ethernet addresses. It is linked into the VxWorks image if INCLUDE_ARP 
is defined at the time the image is built. The underlying ARP protocol, 
which creates and maintains the table, is included automatically as part 
of the IP component.

RELATED INTERFACES
The arpShow() routine (in the netShow library) displays the current 
contents of the ARP table.

A low -evel interface to the ARP table is available with the socket-specific 
SIOCSARP, SIOCDARP and SIOCGARP ioctl functions.

INTERNAL

The structure chart for this module looks like:

     arpAdd	arpDelete
	|   \ 	  /
	|    v   v
        |    arpCmd
	v
etherAsciiToEnet

INCLUDE FILES: arpLib.h

SEE ALSO: inetLib, routeLib, netShow
*/

/* includes */

#include "vxWorks.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "net/if.h"
#include "netinet/if_ether.h"
#include "net/if_arp.h"
#include "net/unixLib.h"
#include "errno.h"
#include "arpLib.h"
#include "inetLib.h"
#include "stdio.h"
#include "string.h"
#include "sockLib.h"
#include "ioLib.h"
#include "unistd.h"
#include "hostLib.h"
#include "netShow.h"
#include "taskLib.h"

#ifdef VIRTUAL_STACK

#ifdef _WRS_VXWORKS_5_X
#include "memPartLib.h"
#endif /* _WRS_VXWORKS_5_X */

#include "netinet/vsLib.h"
#include "netinet/vsArp.h"
extern void arptimer (int);
#endif

/* defines */

#define ENET_SIZE		6		/* Ethernet address size */

/* forward static functions */

LOCAL STATUS etherAsciiToEnet (char * asciiAddr, u_char * retEnet);

IMPORT void arptfree ();

#ifndef VIRTUAL_STACK
IMPORT struct llinfo_arp llinfo_arp;
#endif

/*******************************************************************************
*
* arpLibInit - ARP table manipulation library initialization
*
* This routine is called during system startup if INCLUDE_ARP_API is defined. 
* Normally, its only purpose is to automatically link this module into the
* runtime image. The virtual stack modifications extend this routine to
* initialize the required variables and start the appropriate timer.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* NOMANUAL
*/

void arpLibInit (void)
    {
#ifdef VIRTUAL_STACK

    /*
     * Make sure arpLibInit has not been called before. This can happen
     * when INCLUDE_ARP_API has been included in a build.
     */

    if (arptimerWd == NULL)
	{
        _llinfo_arp.la_next = &_llinfo_arp;
        _llinfo_arp.la_prev = &_llinfo_arp;
        arpRxmitTicks = -1;
        arptimerWd = wdCreate ();
        arptimer(myStackNum);
	}
#endif /* VIRTUAL_STACK */
    return;
    }

/*******************************************************************************
*
* arpAdd - create or modify an ARP table entry
*
* This routine assigns an Ethernet address to an IP address in the ARP table.
* The <pHost> parameter specifies the host by name or by Internet address 
* using standard dotted decimal notation. The <pEther> parameter provides the 
* Ethernet address as six hexadecimal bytes (between 0 and ff) separated by 
* colons. A new entry is created for the specified host if necessary.
* Otherwise, the existing entry is changed to use the given Ethernet address.
*
* The <flags> parameter combines any of the following options: 
* .iP "ATF_PERM  (0x04)"
* Create a permanent ARP entry which will not time out.
* .iP "ATF_PUBL  (0x08)"
* Publish this entry. The host will respond to ARP requests even if the
* <pHost> parameter does not match a local IP address. This setting provides 
* a limited form of proxy ARP.
* .iP "ATF_PROXY (0x10)"
* Use a "wildcard" hardware address. The proxy server uses this setting to
* support multiple proxy networks. The entry always supplies the hardware
* address of the sending interface.
* 
* EXAMPLE
* Create a permanent ARP table entry for the host named "myHost" with
* Ethernet address 0:80:f9:1:2:3:
* .CS
*     arpAdd ("myHost", "0:80:f9:1:2:3", 0x4);
* .CE
*
* Assuming "myHost" has the Internet address "90.0.0.3", the following call
* changes the Ethernet address to 0:80:f9:1:2:4. No additional flags are set 
* for that entry.
* .CS
*     arpAdd ("90.0.0.3", "0:80:f9:1:2:4", 0);
* .CE
*
* RETURNS: OK, or ERROR if unsuccessful.
*
* ERRNO:
*  S_arpLib_INVALID_ARGUMENT
*  S_arpLib_INVALID_HOST
*  S_arpLib_INVALID_ENET_ADDRESS
*  S_arpLib_INVALID_FLAG
*  or results of low-level ioctl call.
*/

STATUS arpAdd
    (
    char *		pHost,		/* host name or IP address */
    char *		pEther,		/* Ethernet address */
    int  		flags     	/* ARP flags */
    )
    {
    struct in_addr	hostAddr;	/* host address	*/
    u_char 		ea [ENET_SIZE];	/* Ethernet address */

#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
#endif /* VIRTUAL_STACK */

    if ((pHost == NULL) || (pEther == NULL)) 	/* validate parameters */
	{
	errno = S_arpLib_INVALID_ARGUMENT;
	return (ERROR);
	}
						/* convert address from ascii */
    if (((hostAddr.s_addr = inet_addr (pHost)) == ERROR) &&
	((hostAddr.s_addr = hostGetByName (pHost)) == ERROR))
	{
	errno = S_arpLib_INVALID_HOST;
	return (ERROR);
	}
						/* convert enet from ascii */
    if (etherAsciiToEnet (pEther, ea) != OK)
	return (ERROR);
						/* validate flags */
    if (flags & ~(ATF_PERM | ATF_PUBL | ATF_INCOMPLETE | ATF_PROXY))
	{
	errno = S_arpLib_INVALID_FLAG;
	return (ERROR);
	}

    if (arpCmd (SIOCSARP, &hostAddr, ea, &flags) == ERROR)
	return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* arpDelete - remove an ARP table entry
*
* This routine deletes an ARP table entry. The <pHost> parameter indicates
* the target entry using the host name or Internet address.
*
* EXAMPLE
* .CS
*    arpDelete ("91.0.0.3")
*    arpDelete ("myHost")
* .CE
*
* RETURNS: OK, or ERROR if unsuccessful.
*
* ERRNO
*  S_arpLib_INVALID_ARGUMENT
*  S_arpLib_INVALID_HOST
*/

STATUS arpDelete
    (
    char *		pHost			/* host name or IP address */
    )
    {
    struct in_addr 	hostAddr;		/* host address 	*/
    char		addrInAscii [ INET_ADDR_LEN ]; /* IP in ascii	*/

#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
#endif /* VIRTUAL_STACK */

    if (pHost == NULL)				/* validate argument */
	{
	errno = S_arpLib_INVALID_ARGUMENT;
	return (ERROR);
	}
						/* convert addr from ascii */
    if (((hostAddr.s_addr = inet_addr (pHost)) == ERROR) &&
	((hostAddr.s_addr = hostGetByName (pHost)) == ERROR))
	{
        errno = S_arpLib_INVALID_HOST;
	return (ERROR);
	}

    inet_ntoa_b (hostAddr, addrInAscii);	/* convert to printable fmt */

    if (arpCmd (SIOCDARP, &hostAddr, (u_char *) NULL, (int *) NULL) == ERROR)
	{
        errno = S_arpLib_INVALID_HOST;    /* No such entry in table. */
	return (ERROR);
	}

    return (OK);
    }

/*******************************************************************************
*
* arpCmd - issues an ARP command
*
* This routine generates the low-level ioctl call for an ARP command. Expected
* values for the <cmd> parameter are SIOCSARP, SIOCGARP, or SIOCDARP. 
* The <pIpAddr> parameter specifies the IP address of the host. The
* <pHwAddr> pointer provides the corresponding link-level address in
* binary form. That parameter is only used with SIOCSARP, and is limited
* to the 6-byte maximum required for Ethernet addresses. The <pFlags> 
* argument provides any flag settings for the entry.
*
* RETURNS: OK, or ERROR if unsuccessful.
*
* NOMANUAL
*
* INTERNAL
* This routine should be LOCAL, but it is also used by the proxy ARP
* library. That library should call the ioctl interface directly instead
* to avoid the overhead created by the additional routines in this
* module which it does not use.
*/

STATUS arpCmd
    (
    int                 cmd,			/* arp command		*/
    struct in_addr *    pIpAddr,		/* ip address		*/
    u_char *            pHwAddr,		/* hardware address	*/
    int *               pFlags			/* arp flags 		*/
    )

    {
    struct arpreq       arpRequest;		/* arp request struct	*/
    STATUS              status = ERROR;		/* return status 	*/
    int                 sock;			/* socket 		*/

    /* fill in arp request structure */

    bzero ((caddr_t) &arpRequest, sizeof (struct arpreq));
    arpRequest.arp_pa.sa_family = AF_INET;
    ((struct sockaddr_in *)
	&(arpRequest.arp_pa))->sin_addr.s_addr = pIpAddr->s_addr;
    arpRequest.arp_ha.sa_family = AF_UNSPEC;

    if (pHwAddr != NULL)
        bcopy ((caddr_t) pHwAddr ,(caddr_t) arpRequest.arp_ha.sa_data,
	       ENET_SIZE);

    if (pFlags != NULL)
    	arpRequest.arp_flags = *pFlags;

    if ((sock = socket (AF_INET, SOCK_RAW, 0)) != ERROR)
        {
        if ((status = ioctl (sock, cmd, (int) &arpRequest)) == OK)

	    {
    	    if (pHwAddr != NULL)
        	bcopy ((caddr_t) arpRequest.arp_ha.sa_data, (caddr_t) pHwAddr,
			ENET_SIZE);

	    if (pFlags != NULL)
    		*pFlags = arpRequest.arp_flags;
	    }
        close (sock);
        }

    return (status);
    }

/*******************************************************************************
*
* arpFlush - flush all entries in the system ARP table
*
* This routine flushes all non-permanent entries in the ARP cache.
*
* RETURNS: N/A
*/

void arpFlush (void)

    {
    register struct llinfo_arp *la;
    FAST struct rtentry * 	pRoute; 
    int				s;

#ifdef VIRTUAL_STACK
    virtualStackIdCheck();
    la = _llinfo_arp.la_next;
#else
    la = llinfo_arp.la_next;
#endif /* VIRTUAL_STACK */

    s = splnet ();

#ifdef VIRTUAL_STACK
    while (la != &_llinfo_arp) 
#else
    while (la != &llinfo_arp) 
#endif
	{
	pRoute = la->la_rt; 
	la = la->la_next;

	/* if entry permanent */
	if ((pRoute->rt_rmx.rmx_expire == 0) || (pRoute->rt_flags == 0))
	    continue;

	arptfree(la->la_prev); /* timer has expired; clear */
	}

    splx (s);
    }


/*******************************************************************************
*
* etherAsciiToEnet - convert Ethernet address
*
* This routine converts an Ethernet address in ascii format to its normal
* 48 bit format.  <asciiAddr> is the string Ethernet address which has the form
* "x:x:x:x:x:x" where x is a hexadecimal number between 0 and ff.  <retEnet> is
* where the Ethernet address gets returned.   This routine is similar to the
* UNIX call ether_aton.
*
* RETURNS: OK, or ERROR if unsuccessful.
*
* ERRNO: S_arpLib_INVALID_ENET_ADDRESS
*/

LOCAL STATUS etherAsciiToEnet
    (
    char *		asciiAddr,		/* enet addr in ascii	*/
    u_char *		retEnet			/* return enet addr	*/
    )

    {
    int			enet [ENET_SIZE];	/* Ethernet address     */
    int			ix;			/* index variable	*/

    if (sscanf (asciiAddr, "%x:%x:%x:%x:%x:%x", &enet [0],
		&enet [1], &enet [2], &enet [3], &enet [4],
		&enet [5]) != ENET_SIZE)
	{
	printf ("arp: invalid Ethernet address %s\n", asciiAddr);
	errno = S_arpLib_INVALID_ENET_ADDRESS;
	return (ERROR);
	}

    for (ix = 0; ix < ENET_SIZE; ix++)
	retEnet [ix] = (u_char) enet [ix];

   return (OK);
   }

/*******************************************************************************
*
* arpResolve - resolve a hardware address for a specified Internet address
*
* This routine uses the Address Resolution Protocol (ARP) and internal ARP
* cache to resolve the hardware address of a machine that owns the Internet
* address given in <targetAddr>.
*
* The hardware address is copied to <pHwAddr> as network byte order, if the
* resolution of <targetAddr> is successful.  <pHwAddr> must point to a buffer
* which is large enough to receive the address.
*
* NOTE: RFC 1122 prohibits sending more than one arp request per second.  Any
* numTicks value that would result in a shorter time than this is ignored.
*
* RETURNS:
* OK if the address is resolved successfully, or ERROR if <pHwAddr> is NULL,
* <targetAddr> is invalid, or address resolution is unsuccessful.
*
* ERRNO:
*  S_arpLib_INVALID_ARGUMENT
*  S_arpLib_INVALID_HOST
*
*/

STATUS arpResolve
    (
    char                *targetAddr,  /* name or Internet address of target */
    char                *pHwAddr,     /* where to return the H/W address */
    int                 numTries,     /* number of times to try ARPing (-1 means try
                                         forever) */
    int                 numTicks      /* number of ticks between ARPs */
    )
    {
    struct ifnet *	pIf = NULL;
    struct sockaddr_in  sockInetAddr;
    struct rtentry *	pRt;
    unsigned long	addr;
    int			retVal = 0;

    if (pHwAddr == NULL || numTries < -1 || numTries == 0)     /* user messed up */
	{
	errno = S_arpLib_INVALID_ARGUMENT;
        return (ERROR);
	}

    /* the 'targetAddr' can either be the hostname or the actual Internet
     * address.
     */

    if ((addr = (unsigned long) hostGetByName (targetAddr)) == ERROR &&
	(addr = inet_addr (targetAddr)) == ERROR)
	{
	errno = S_arpLib_INVALID_HOST;
	return (ERROR);
	}

    bzero ((caddr_t)&sockInetAddr, sizeof (sockInetAddr));
    sockInetAddr.sin_len = sizeof(struct sockaddr_in);
    sockInetAddr.sin_family = AF_INET;
    sockInetAddr.sin_addr.s_addr = addr; 

    /*
     * Get associated local interface's ifnet. This search also
     * clones an empty ARP entry from the interface route if one
     * does not already exist.
     */

    pRt = rtalloc1 ( (struct sockaddr *)&sockInetAddr, 1);
    if (pRt == NULL)
	{
	errno = S_arpLib_INVALID_HOST;
	return (ERROR);
	}

    pIf = pRt->rt_ifp;
    if (pIf == NULL)
	{
        rtfree (pRt);
	errno = S_arpLib_INVALID_HOST;
	return (ERROR);
	}

    /* return 0xffffffffffff for broadcast Internet address */

    if (in_broadcast (sockInetAddr.sin_addr, pIf))
	{
        bcopy ((char *) etherbroadcastaddr, pHwAddr,
	       sizeof (etherbroadcastaddr));

        rtfree (pRt);

	return (OK);
	}

    /* Try to resolve the Ethernet address by calling arpresolve() which
     * may send out ARP request messages out onto the Ethernet wire.
     */

    while ((numTries == -1 || numTries-- > 0) &&
	   (retVal = arpresolve ((struct arpcom *) pIf, 
				 (struct rtentry *)pRt, 
				 (struct mbuf *) NULL,
				 (struct sockaddr *)&sockInetAddr,
				 (UCHAR *)pHwAddr))
	   == 0)
    	if (numTries)           /* don't delay after last arp */
            taskDelay (numTicks);

    rtfree (pRt);

    if (retVal == 0)		/* unsuccessful resolution */
	{
	errno = S_arpLib_INVALID_HOST;
        return (ERROR);
	}

    return (OK);
    }


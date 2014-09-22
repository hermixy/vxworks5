/* comKernelSupport.c - COM library (VxDCOM) kernel level support functions */

/* Copyright (c) 2000 Wind River Systems, Inc. */

/*

modification history
--------------------
01h,03jul01,nel  set correct name of comSysSetComLocal.
01g,27jun01,dbs  fix for T2 build
01f,22jun01,nel  Add extra headers for WIND_TCB def.
01e,22jun01,dbs  move some includes into here
01d,21jun01,nel  Rename interface to conform to new naming standard in COM.
01c,10apr00,nel  Fix for Solaris and Linux Build.
01b,05arp00,nel  MAC address detection.
01a,22mar00,nel  created

*/

/*

DESCRIPTION

This library provides low level kernel support for VxDCOM which must run in
an application domain and therefore can't access kernel functions directly.
.NOMANUAL
*/

#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#include <stdlib.h>
#include <taskLib.h>

/**************************************************************************
*
* comSysAddressGet - returns the MAC address for the board.
*
* This function returns the MAC address for the board.
*
* RETURNS: 0 if MAC address retrieved, -1 otherwise.
* .NOMANUAL
*/

int comSysAddressGet
    (
    unsigned char * 	addr		/* will hold MAC address on exit */
    )
    {
    FAST struct ifnet       *ifp;

    for (ifp = ifnet;  ifp!= NULL;  ifp = ifp->if_next)
	{

    /* if the interface is not LOOPBACK or SLIP, return the link
     * level address.
     */
    if (!(ifp->if_flags & IFF_POINTOPOINT) &&
	!(ifp->if_flags & IFF_LOOPBACK))
	    {
	    memcpy(addr, ((struct arpcom *)ifp)->ac_enaddr, 6);
	    return 0;
	    }
	}	
    return -1;
    }

unsigned long comSysLocalGet ()
    {
    WIND_TCB*	pTcb = taskTcb (taskIdSelf ());
#if VXDCOM_PLATFORM_VXWORKS == 5
    /* VxWorks 5.x */
    return (unsigned long) pTcb->pComLocal;
#else
    /* VxWorks AE 1.x */
    return (unsigned long) pTcb->pLcb->pComLocal;
#endif
    }

void comSysLocalSet
    (
    unsigned long	nextTaskId	/* */
    )
    {
    WIND_TCB*	pTcb = taskTcb (taskIdSelf ());
#if VXDCOM_PLATFORM_VXWORKS == 5
    /* VxWorks 5.x */
    pTcb->pComLocal = (void*) nextTaskId;
#else
    /* VxWorks AE 1.x */
    pTcb->pLcb->pComLocal = (void*) nextTaskId;
#endif
    }

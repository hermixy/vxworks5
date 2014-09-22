/* etherMultiLib.c - a library to handle Ethernet multicast addresses */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01h,15oct01,rae  merge from truestack ver 01j, base o1g
01g,14dec97,jdi  doc: cleanup.
01f,10dec97,kbw  making minor man page fixes
01e,25oct97,kbw  making minor man page fixes
01d,26aug97,spm  removed compiler warnings (SPR #7866)
01c,23oct96,gnn  changed names to follow coding standards; removed some 
                 compiler warnings
01b,22oct96,gnn  changed printfs to logMsgs
01a,29mar96,gnn  taken from BSD 4.4 code
*/
 
/*
DESCRIPTION
This library manages a list of multicast addresses for network
drivers.  This abstracts the management of these drivers into a device
independant library.

To use this feature, include the following component:
INCLUDE_NETWRS_ETHERMULTILIB

INCLUDE FILES: string.h, errno.h, netinet/in.h, net/if.h, lstLib.h,
etherMultiLib.h
*/

/* includes */
#include "vxWorks.h"
#include "logLib.h"
#include "string.h"
#include "errno.h"
#include "netinet/in.h"
#include "net/if.h"
#include "lstLib.h"
#include "end.h"
#include "etherMultiLib.h"

#include "stdlib.h"
#include "memPartLib.h"

/* defints */

/* typedefs */

/* globals */
int etherMultiDebug = 0;

/* locals */

/* forward declarations */

/*****************************************************************************
*
* etherMultiAdd - add multicast address to a multicast address list
*
* This routine adds an Ethernet multicast address list for a given END.
* The address is a six-byte value pointed to by <pAddress>.
* 
* RETURNS: OK or ENETRESET.
*/

int etherMultiAdd
    (
    LIST *pList,    /* pointer to list of multicast addresses */
    char* pAddress  /* address you want to add to list */
    )
    {
	    
	
    ETHER_MULTI* pCurr;

    /*
     * Verify that we have valid Ethernet multicast addresses.
     */
    if ((pAddress[0] & 0x01) != 1) {
	if (etherMultiDebug)
	    logMsg("Invalid address!\n", 1, 2, 3, 4, 5, 6);
	return (EINVAL);
    }
    /*
     * See if the address range is already in the list.
     */
    for (pCurr = (ETHER_MULTI *)lstFirst(pList); pCurr != NULL && 
	    (bcmp(pCurr->addr, pAddress, 6) != 0); 
	    pCurr = (ETHER_MULTI *)lstNext(&pCurr->node)); 
    
    if (pCurr != NULL) {
	/*
	 * Found it; just increment the reference count.
	 */
	if (etherMultiDebug)
	    logMsg("Address already exists!\n", 1, 2, 3, 4, 5, 6);
	++pCurr->refcount;
	return (0);
    }

    /*
     * New address or range; malloc a new multicast record
     * and link it into the interface's multicast list.
     */

    pCurr = (ETHER_MULTI *) KHEAP_ALLOC(sizeof(ETHER_MULTI));
    if (pCurr == NULL) {
	if (etherMultiDebug)
	    logMsg("Cannot allocate memory!\n", 1, 2, 3, 4, 5, 6);
	return (ENOBUFS);
    }

    bcopy((char *)pAddress, (char *)pCurr->addr, 6);
    pCurr->refcount = 1;

    lstAdd(pList, &pCurr->node);
    if (etherMultiDebug)
	{
	logMsg("Added address is %x:%x:%x:%x:%x:%x\n",
	    pCurr->addr[0],
	    pCurr->addr[1],
	    pCurr->addr[2],
	    pCurr->addr[3],
	    pCurr->addr[4],
	    pCurr->addr[5]);
	}
    /*
     * Return ENETRESET to inform the driver that the list has changed
     * and its reception filter should be adjusted accordingly.
     */
    return (ENETRESET);
    }

/*****************************************************************************
*
* etherMultiDel - delete an Ethernet multicast address record
*
* This routine deletes an Ethernet multicast address from the list.
* The address is a six-byte value pointed to by <pAddress>.
* 
* RETURNS: OK or ENETRESET.
*/

int etherMultiDel
    (
    LIST *pList,    /* pointer to list of multicast addresses */
    char* pAddress  /* address you want to add to list */
    )
    {
    ETHER_MULTI* pCurr;
    /*
     * Look up the address in our list.
     */
    for (pCurr = (ETHER_MULTI *)lstFirst(pList); pCurr != NULL && 
	    (bcmp(pCurr->addr, pAddress, 6) != 0); 
	    pCurr = (ETHER_MULTI *)lstNext(&pCurr->node)); 
    
    if (pCurr == NULL) {
	    return (ENXIO);
    }
    if (--pCurr->refcount != 0) {
	    /*
	     * Still some claims to this record.
	     */
	    return (0);
    }
    /*
     * No remaining claims to this record; unlink and free it.
     */
    lstDelete(pList, &pCurr->node);
    KHEAP_FREE((char *)pCurr);

    /*
     * Return ENETRESET to inform the driver that the list has changed
     * and its reception filter should be adjusted accordingly.
     */
    return (ENETRESET);
    }
/*****************************************************************************
* 
* etherMultiGet - retrieve a table of multicast addresses from a driver
*
* This routine runs down the multicast address list stored in a driver
* and places all the entries it finds into the multicast table
* structure passed to it.
*
* RETURNS: OK or ERROR.
*/

int etherMultiGet
    (
    LIST* pList,        /* pointer to list of multicast addresses */
    MULTI_TABLE* pTable /* table into which to copy addresses */
    )
    {
	
    int count = 0;
    int len;
    ETHER_MULTI* pCurr;

    len = pTable->len;	/* Save the passed in table length. */
    pTable->len = 0;	

    pCurr = (ETHER_MULTI *)lstFirst(pList);

    while (pCurr != NULL && count < len)
	{
	if (etherMultiDebug)
	    {
	    logMsg("%x:%x:%x:%x:%x:%x\n", 
		pCurr->addr[0], 
		pCurr->addr[1], 
		pCurr->addr[2], 
		pCurr->addr[3],
		pCurr->addr[4],
		pCurr->addr[5]);
	    }
	bcopy(pCurr->addr, (char *)&pTable->pTable[count], 6);
	count+=6;
	pTable->len += 6;
	pCurr = (ETHER_MULTI *)lstNext(&pCurr->node);
	}

    return (OK);
    }


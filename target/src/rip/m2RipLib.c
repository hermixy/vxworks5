/* m2RipLib.c - VxWorks interface routines to RIP for SNMP Agent */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01n,15oct01,rae  merge from truestack ver 01o, base 01m (VIRTUAL_STACK)
01m,30jun98,spm  corrected authentication to allow change to shorter key
01l,26jun98,spm  changed RIP_MCAST_ADDR constant from string to value; altered
                 multicast group handling to comply with ANVL RIP tests
01k,25oct97,kbw  making minor man page fixes
01j,06oct97,gnn  cleaned up issues with IMPORT of ripState
01i,15may97,gnn  cleaned up some warnings; #ifdef'd out some test code
01h,08may97,gnn  fixed the authentication string code.
01g,30apr97,kbw  fiddled man page text
01f,28apr97,gnn  fixed some of the documentation
01e,24apr97,gnn  fixed an errno value
01d,20apr97,kbw  fixed man page format, spell check
01c,17apr97,gnn  modified interfaces and variable to follow standards.
01b,14apr97,gnn  added documentation for MIB-II stuff.
01a,01apr97,gnn  written.
*/
 
/*
DESCRIPTION
This library provides routines to initialize the group, access the
group global variables, read the table of network interfaces that RIP
knows about, and change the state of such an interface.  For a broader
description of MIB-II services, see the manual entry for m2Lib.

USING THIS LIBRARY
This library can be initialized and deleted by calling m2RipInit() and
m2RipDelete() respectively, if only the RIP group's services are needed.
If full MIB-II support is used, this group and all other groups can be
initialized and deleted by calling m2Init() and m2Delete().

The group global variables are accessed by calling
m2RipGlobalCountersGet() as follows:
.CS
    M2_RIP2_GLOBAL_GROUP   ripGlobal;

    if (m2RipGlobalCountersGet (&ripGlobal) == OK)
	/@ values in ripGlobal are valid @/
.CE
To retrieve the RIP group statistics for a particular interface you call the
m2RipIfStatEntryGet() routine a pointer to an M2_RIP2_IFSTAT_ENTRY structure 
that contains the address of the interface you are searching for.  For example:
.CS
    M2_RIP2_IFSTAT_ENTRY ripIfStat;
    
	ripIfStat.rip2IfStatAddress = inet_addr("90.0.0.3");
	if (m2RipIfStatEntryGet(M2_EXACT_VALUE, &ripIfStat) == OK)
	/@ values in ripIfState are valid @/

.CE
To retrieve the configuration statistics for a particular interface the
m2RipIfConfEntryGet() routine must be called with an IP address encoded in an
M2_RIP2_IFSTAT_ENTRY structure which is passed as the second argument.  For
example:
.CS
    M2_RIP2_IFCONF_ENTRY ripIfConf;
    
	ripIfConf.rip2IfConfAddress = inet_addr("90.0.0.3");
	if (m2RipIfConfEntryGet(M2_EXACT_VALUE, &ripIfConf) == OK)
	/@ values in ripIfConf are valid @/
.CE
To set the values of for an interface the m2RipIfConfEntrySet() routine must
be called with an IP address in dot notation encoded into an
M2_RIP2_IFSTAT_ENTRY structure, which is passed as the second argument.  For
example:
.CS
    M2_RIP2_IFCONF_ENTRY ripIfConf;

	ripIfConf.rip2IfConfAddress = inet_addr("90.0.0.3");

	/@ Set the authorization type. @/
	ripIfConf.rip2IfConfAuthType = M2_rip2IfConfAuthType_simplePassword;
	bzero(ripIfConf.rip2IfConfAuthKey, 16);
	bcopy("Simple Password ", ripIfConf.rip2IfConfAuthKey, 16);

	/@ We only accept version 1 packets. @/
	ripIfConf.rip2IfConfSend = M2_rip2IfConfSend_ripVersion1;

	/@ We only send version 1 packets. @/
	ripIfConf.rip2IfConfReceive = M2_rip2IfConfReceive_rip1;

	/@ Default routes have a metric of 2 @/
	ripIfConf.rip2IfConfDefaultMetric = 2;

	/@ If the interface is invalid it is turned off, we make it valid. @/
	ripIfConf.rip2IfConfStatus = M2_rip2IfConfStatus_valid;
	
	if (m2RipIfConfEntrySet(varsToSet, &ripIfConf) == OK)
	/@ Call succeded. @/
.CE

INCLUDE FILES: rip/m2RipLib.h rip/defs.h

SEE ALSO:
ripLib

*/

/* includes */
#include "vxWorks.h"
#include "rip/m2RipLib.h"
#include "rip/defs.h"
#include "m2Lib.h"
#include "errnoLib.h"
#include "errno.h"
#include "inetLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#include "netinet/vsRip.h"
#endif

/* defines */

#define	same(a1, a2) \
	(memcmp((a1)->sa_data, (a2)->sa_data, 14) == 0)

/* typedefs */

/* globals */

#ifndef VIRTUAL_STACK
extern	struct interface *ripIfNet;
#endif

/* locals */

/* forward declarations */

/******************************************************************************
*
* m2RipInit - initialize the RIP MIB support
*
* This routine sets up the RIP MIB and should be called before any 
* other m2RipLib routine.
*
* RETURNS: OK, always.
*
*/

STATUS m2RipInit (void)
    {
    return (OK);
    }

/******************************************************************************
*
* m2RipDelete - delete the RIP MIB support
*
* This routine should be called after all m2RipLib calls are completed.
*
* RETURNS: OK, always. 
*
*/

STATUS m2RipDelete (void)
    {
    return (OK);
    }

/******************************************************************************
*
* m2RipGlobalCountersGet - get MIB-II RIP-group global counters
*
* This routine fills in an M2_RIP2_GLOBAL_GROUP structure pointed to 
* by <pRipGlobal> with the values of the MIB-II RIP-group global counters.
*
* RETURNS: OK or ERROR. 
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*
* SEE ALSO:
* m2RipInit()
*/

STATUS m2RipGlobalCountersGet
    (
    M2_RIP2_GLOBAL_GROUP* pRipGlobal
    )
    {
#ifndef VIRTUAL_STACK
    extern RIP ripState;
#endif

    if (pRipGlobal == NULL)
        {
        errnoSet(S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
        }

    bcopy((char *)&ripState.ripGlobal, (char *)pRipGlobal,
          sizeof(M2_RIP2_GLOBAL_GROUP));

    return (OK);
    }

/******************************************************************************
*
* m2RipIfStatEntryGet - get MIB-II RIP-group interface entry
*
* This routine retrieves the interface statistics for the interface serving
* the subnet of the IP address contained in the M2_RIP2_IFSTAT_ENTRY
* structure.  <pRipIfStat> is a pointer to an M2_RIP2_IFSTAT_ENTRY structure
* which the routine will fill in upon successful completion.
*
* This routine either returns an exact match if <search> is M2_EXACT_VALUE,
* or the next value greater than or equal to the value supplied if the
* <search> is M2_NEXT_VALUE.
*
* RETURNS: OK, or ERROR if either <pRipIfStat> is invalid or an exact match
* failed.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
* 
* SEE ALSO:
* m2RipInit()
*/

STATUS m2RipIfStatEntryGet
    (
    int search,
    M2_RIP2_IFSTAT_ENTRY* pRipIfStat
    )
    {

    struct interface* pIfp;
    struct interface* pIfpSaved = NULL;
    struct sockaddr_in address;
    unsigned long ipAddrSaved = -1;
    unsigned long currIpAddr;
    
    address.sin_addr.s_addr = pRipIfStat->rip2IfStatAddress;
    address.sin_family = AF_INET;
        
    if (search == M2_EXACT_VALUE)
        {
        
        pIfpSaved = ripIfLookup((struct sockaddr *)&address);
        
        if (pIfpSaved == NULL)
            {
            errnoSet(S_m2Lib_ENTRY_NOT_FOUND);
            return (ERROR);
            }
        }
    else
        {
	for (pIfp = ripIfNet; pIfp; pIfp = pIfp->int_next)
            {
            currIpAddr = ntohl(((struct sockaddr_in *)&pIfp->int_addr)->sin_addr.s_addr);
            if ((currIpAddr >= ntohl(address.sin_addr.s_addr)) &&
                (currIpAddr < ipAddrSaved))
                {
                pIfpSaved = pIfp;
                ipAddrSaved = currIpAddr;
                }
            }

        if (pIfpSaved == NULL)
            {
            errnoSet(S_m2Lib_ENTRY_NOT_FOUND);
            return (ERROR);
            }
        }

    bcopy((char *)&pIfpSaved->ifStat, (char *)pRipIfStat,
          sizeof(M2_RIP2_IFSTAT_ENTRY));

    return (OK);

    }

/******************************************************************************
*
* m2RipIfConfEntryGet - get MIB-II RIP-group interface entry
*
* This routine retrieves the interface configuration for the interface serving
* the subnet of the IP address contained in the M2_RIP2_IFCONF_ENTRY structure
* passed to it.  <pRipIfConf> is a pointer to an M2_RIP2_IFCONF_ENTRY 
* structure which the routine will fill in upon successful completion.
*
* This routine either returns an exact match if <search> is M2_EXACT_VALUE,
* or the next value greater than or equal to the value supplied if the
* <search> is M2_NEXT_VALUE.
*
* RETURNS: OK, or ERROR if <pRipIfConf> was invalid or the interface was
* not found.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*
* SEE ALSO:
* m2RipInit()
*/

STATUS m2RipIfConfEntryGet
    (
    int search,
    M2_RIP2_IFCONF_ENTRY* pRipIfConf
    )
    {

    struct interface* pIfp;
    struct sockaddr_in address;
    struct interface* pIfpSaved = NULL;
    unsigned long ipAddrSaved = -1;
    unsigned long currIpAddr;
    
    address.sin_addr.s_addr = pRipIfConf->rip2IfConfAddress;
    address.sin_family = AF_INET;
    
    if (search == M2_EXACT_VALUE)
        {
        
        pIfpSaved = ripIfLookup((struct sockaddr *)&address);
        
        if (pIfpSaved == NULL)
            {
            errnoSet(S_m2Lib_ENTRY_NOT_FOUND);
            return (ERROR);
            }
        }
    else
        {
	for (pIfp = ripIfNet; pIfp; pIfp = pIfp->int_next)
            {
            currIpAddr = ntohl(((struct sockaddr_in *)&pIfp->int_addr)->sin_addr.s_addr);
            if ((currIpAddr >= ntohl(address.sin_addr.s_addr)) &&
                (currIpAddr < ipAddrSaved))
                {
                pIfpSaved = pIfp;
                ipAddrSaved = currIpAddr;
                }
            }

        if (pIfpSaved == NULL)
            {
            errnoSet(S_m2Lib_ENTRY_NOT_FOUND);
            return (ERROR);
            }
        }

    bcopy((char *)&pIfpSaved->ifConf, (char *)pRipIfConf,
          sizeof(M2_RIP2_IFCONF_ENTRY));

    return (OK);

    }

#if 0
void m2RipIfConfTest
    (
    char* pIpAddr
    )
    {

    M2_RIP2_IFCONF_ENTRY ripIfConf;
    
    ripIfConf.rip2IfConfAddress = inet_addr(pIpAddr);
    
    if (m2RipIfConfEntryGet(M2_EXACT_VALUE, &ripIfConf) != OK)
        return;

    printf("IP Address: %s\nAuthType: %ld\nAuthKey: %s\nSend: %ld\n",
           inet_ntoa(ripIfConf.rip2IfConfAddress),
           ripIfConf.rip2IfConfAuthType,
           ripIfConf.rip2IfConfAuthKey,
           ripIfConf.rip2IfConfSend);
    printf("Receive: %ld\nDefault Metric: %ld\nStatus: %ld\n",
           ripIfConf.rip2IfConfReceive,
           ripIfConf.rip2IfConfDefaultMetric,
           ripIfConf.rip2IfConfStatus);
    
    }

#endif

/******************************************************************************
*
* m2RipIfConfEntrySet - set MIB-II RIP-group interface entry
*
* This routine sets the interface configuration for the interface serving
* the subnet of the IP address contained in the M2_RIP2_IFCONF_ENTRY structure.
*
* <pRipIfConf> is a pointer to an M2_RIP2_IFCONF_ENTRY structure which the
* routine places into the system based on the <varToSet> value. 
*
* RETURNS: OK, or ERROR if <pRipIfConf> is invalid or the interface cannot
* be found.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_ENTRY_NOT_FOUND
*
* SEE ALSO:
* m2RipInit()
*/

STATUS m2RipIfConfEntrySet
    (
    unsigned int varToSet,
    M2_RIP2_IFCONF_ENTRY* pRipIfConf
    )
    {
    struct interface* pIfp;
    struct sockaddr_in address;

#ifndef VIRTUAL_STACK
    IMPORT RIP ripState;
#endif
    BOOL changeFlag = FALSE; 	/* Changing receive control switch? */

    address.sin_addr.s_addr = pRipIfConf->rip2IfConfAddress;
    address.sin_family = AF_INET;
    
    pIfp = ripIfLookup((struct sockaddr *)&address);
    
    if (pIfp == NULL)
        {
        errnoSet(S_m2Lib_ENTRY_NOT_FOUND);
        return (ERROR);
        }

    if (varToSet & M2_RIP2_IF_CONF_DOMAIN)
        {
        bcopy(pRipIfConf->rip2IfConfDomain, pIfp->ifConf.rip2IfConfDomain,
              2);
        }

    if (varToSet & M2_RIP2_IF_CONF_AUTH_TYPE)
        {
        pIfp->ifConf.rip2IfConfAuthType = pRipIfConf->rip2IfConfAuthType;
        }
    
    if (varToSet & M2_RIP2_IF_CONF_AUTH_KEY)
        {
        bzero (pIfp->ifConf.rip2IfConfAuthKey, AUTHKEYLEN);
        strncpy(pIfp->ifConf.rip2IfConfAuthKey, pRipIfConf->rip2IfConfAuthKey,
              AUTHKEYLEN);
        }

    if (varToSet & M2_RIP2_IF_CONF_SEND)
        {
        pIfp->ifConf.rip2IfConfSend = pRipIfConf->rip2IfConfSend;
        }

    if (varToSet & M2_RIP2_IF_CONF_RECEIVE)
        {
        if (pIfp->ifConf.rip2IfConfReceive != pRipIfConf->rip2IfConfReceive)
            changeFlag = TRUE;
        if (changeFlag)
            {
            /* Add to multicast group if changing to RIPv2 packets only. */

            if (pRipIfConf->rip2IfConfReceive == M2_rip2IfConfReceive_rip2)
                if (ripState.s != 0)
                    ripSetInterfaces(ripState.s, (UINT32)RIP_MCAST_ADDR);

            /*
             * ANVL 16.1 - remove from multicast group if changing to 
             *             receive any RIPv1 packets.
             */

            if (pIfp->ifConf.rip2IfConfReceive == M2_rip2IfConfReceive_rip2)
                if (ripState.s != 0)
                    ripClearInterfaces(ripState.s, (UINT32)RIP_MCAST_ADDR);

            pIfp->ifConf.rip2IfConfReceive = pRipIfConf->rip2IfConfReceive;
            }
        }

    if (varToSet & M2_RIP2_IF_CONF_DEFAULT_METRIC)
        {
        pIfp->ifConf.rip2IfConfDefaultMetric =
            pRipIfConf->rip2IfConfDefaultMetric;
        }
    
    if (varToSet & M2_RIP2_IF_CONF_STATUS)
        {
        pIfp->ifConf.rip2IfConfStatus = pRipIfConf->rip2IfConfStatus;
        }
    
    return (OK);

    }

#if 0 
void m2RipIfConfSetTest
    (
    char* pIpAddr,
    long authType,         /* Authentication type */
    char authKey[15],      /* Key */
    long send,             /* What version to send */
    long receive,          /* What version to listen to */
    long defaultMetric,    /* Default metric for default route */
    long status            /* Putting M2_rip2IfConfStatus_invalid */
                           /* here turns the interface off */
    )
    {

    M2_RIP2_IFCONF_ENTRY ripIfConf;
    
    ripIfConf.rip2IfConfAddress = inet_addr(pIpAddr);
    ripIfConf.rip2IfConfAuthType = authType;
    bzero(ripIfConf.rip2IfConfAuthKey, 16);
    strcpy(ripIfConf.rip2IfConfAuthKey, authKey);
    ripIfConf.rip2IfConfSend = send;
    ripIfConf.rip2IfConfReceive = receive;
    ripIfConf.rip2IfConfDefaultMetric = defaultMetric;
    ripIfConf.rip2IfConfStatus = status;
    
    if (m2RipIfConfEntrySet(0xff, &ripIfConf) != OK)
        return;

    if (m2RipIfConfEntryGet(M2_EXACT_VALUE, &ripIfConf) != OK)
        return;

    printf("IP Address: %s\nAuthType: %ld\nAuthKey: %s\nSend: %ld\n",
           inet_ntoa(ripIfConf.rip2IfConfAddress),
           ripIfConf.rip2IfConfAuthType,
           ripIfConf.rip2IfConfAuthKey,
           ripIfConf.rip2IfConfSend);
    printf ("Receive: %ld\nDefault Metric: %ld\nStatus: %ld\n",
            ripIfConf.rip2IfConfReceive,
            ripIfConf.rip2IfConfDefaultMetric,
            ripIfConf.rip2IfConfStatus);

    
    }
#endif

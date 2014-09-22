/* m2IcmpLib.c - MIB-II ICMP-group API for SNMP Agents */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01d,15oct01,rae  merge from truestack ver 01e, base 01c (VIRTUAL_STACK)
01c,25jan95,jdi  doc cleanup.
01b,11nov94,rhp  edited man pages
01a,08dec93,jag  written
*/
/*
DESCRIPTION
This library provides MIB-II services for the ICMP group.  It provides routines
to initialize the group, and to access the group scalar variables. For a
broader description of MIB-II services, see the manual entry for m2Lib.

To use this feature, include the following component:
INCLUDE_MIB2_ICMP

USING THIS LIBRARY
This library can be initialized and deleted by calling the routines
m2IcmpInit() and m2IcmpDelete() respectively, if only the ICMP group's services
are needed.  If full MIB-II support is used, this group and all other
groups can be initialized and deleted by calling m2Init() and
m2Delete().

The group scalar variables are accessed by calling
m2IcmpGroupInfoGet() as follows:
.CS
    M2_ICMP   icmpVars;

    if (m2IcmpGroupInfoGet (&icmpVars) == OK)
	/@ values in icmpVars are valid @/
.CE

INCLUDE FILES: m2Lib.h
 
SEE ALSO:
m2Lib, m2IfLib, m2IpLib, m2TcpLib, m2SysLib
*/

/* includes */
#include <vxWorks.h>
#include "m2Lib.h"
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include "errnoLib.h"
#include "semLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif /* VIRTUAL_STACK */

/* external declarations */

#ifndef VIRTUAL_STACK
extern struct icmpstat    icmpstat;   	/* ICMP statistics structure */
#endif /* VIRTUAL_STACK */

/*******************************************************************************
*
* m2IcmpInit - initialize MIB-II ICMP-group access
*
* This routine allocates the resources needed to allow access to the MIB-II
* ICMP-group variables.  This routine must be called before any ICMP variables
* can be accessed.
*
* RETURNS: OK, always.
*
* SEE ALSO: m2IcmpGroupInfoGet(), m2IcmpDelete()
*/
STATUS m2IcmpInit (void)
    {
    return (OK);
    }

/******************************************************************************
*
* m2IcmpGroupInfoGet - get the MIB-II ICMP-group global variables
*
* This routine fills in the ICMP structure at <pIcmpInfo> with the 
* MIB-II ICMP scalar variables.
*
* RETURNS: OK, or ERROR if the input parameter <pIcmpInfo> is invalid.
*
* ERRNO:
* S_m2Lib_INVALID_PARAMETER
*
* SEE ALSO: m2IcmpInit(), m2IcmpDelete()
*/
STATUS m2IcmpGroupInfoGet
    (
    M2_ICMP * pIcmpInfo          /* pointer to the ICMP group structure */
    )
    {
    int ix;
 
    /* Validate Pointer to ICMP structure */
 
    if (pIcmpInfo == NULL)
	{
	errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
	}
 
    pIcmpInfo->icmpInMsgs  = 0;
    pIcmpInfo->icmpOutMsgs = 0;
 
    /* Add the counts of all the packet types Sent and Received */
 
    for (ix = 0; ix < ICMP_MAXTYPE; ix++)
        {
#ifdef VIRTUAL_STACK
        pIcmpInfo->icmpInMsgs      += _icmpstat.icps_inhist [ix];
        pIcmpInfo->icmpOutMsgs     += _icmpstat.icps_outhist [ix];
#else
        pIcmpInfo->icmpInMsgs      += icmpstat.icps_inhist [ix];
        pIcmpInfo->icmpOutMsgs     += icmpstat.icps_outhist [ix];
#endif /* VIRTUAL_STACK */
        }
 
    /* Add to the Input count the ICMP packets received in error */
 
#ifdef VIRTUAL_STACK
    pIcmpInfo->icmpInMsgs          += _icmpstat.icps_badcode +
                                      _icmpstat.icps_tooshort +
                                      _icmpstat.icps_checksum +
                                      _icmpstat.icps_badlen;
 
    pIcmpInfo->icmpInErrors         = _icmpstat.icps_badcode +
                                      _icmpstat.icps_tooshort +
                                      _icmpstat.icps_checksum +
                                      _icmpstat.icps_badlen;
 
    pIcmpInfo->icmpInDestUnreachs   = _icmpstat.icps_inhist [ICMP_UNREACH];
    pIcmpInfo->icmpInTimeExcds      = _icmpstat.icps_inhist [ICMP_TIMXCEED];
    pIcmpInfo->icmpInParmProbs      = _icmpstat.icps_inhist [ICMP_PARAMPROB];
    pIcmpInfo->icmpInSrcQuenchs     = _icmpstat.icps_inhist [ICMP_SOURCEQUENCH];
    pIcmpInfo->icmpInRedirects      = _icmpstat.icps_inhist [ICMP_REDIRECT];
    pIcmpInfo->icmpInEchos          = _icmpstat.icps_inhist [ICMP_ECHO];
    pIcmpInfo->icmpInEchoReps       = _icmpstat.icps_inhist [ICMP_ECHOREPLY];
    pIcmpInfo->icmpInTimestamps     = _icmpstat.icps_inhist [ICMP_TSTAMP];
    pIcmpInfo->icmpInTimestampReps  = _icmpstat.icps_inhist [ICMP_TSTAMPREPLY];
    pIcmpInfo->icmpInAddrMasks      = _icmpstat.icps_inhist [ICMP_MASKREQ];
    pIcmpInfo->icmpInAddrMaskReps   = _icmpstat.icps_inhist [ICMP_MASKREPLY];
 
    pIcmpInfo->icmpOutErrors        = _icmpstat.icps_error;
    pIcmpInfo->icmpOutDestUnreachs  = _icmpstat.icps_outhist [ICMP_UNREACH];
    pIcmpInfo->icmpOutTimeExcds     = _icmpstat.icps_outhist [ICMP_TIMXCEED];
    pIcmpInfo->icmpOutParmProbs     = _icmpstat.icps_outhist [ICMP_PARAMPROB];
    pIcmpInfo->icmpOutSrcQuenchs    = _icmpstat.icps_outhist [ICMP_SOURCEQUENCH];
    pIcmpInfo->icmpOutRedirects     = _icmpstat.icps_outhist [ICMP_REDIRECT];
    pIcmpInfo->icmpOutEchos         = _icmpstat.icps_outhist [ICMP_ECHO];
    pIcmpInfo->icmpOutEchoReps      = _icmpstat.icps_outhist [ICMP_ECHOREPLY];
    pIcmpInfo->icmpOutTimestamps    = _icmpstat.icps_outhist [ICMP_TSTAMP];
    pIcmpInfo->icmpOutTimestampReps = _icmpstat.icps_outhist [ICMP_TSTAMPREPLY];
    pIcmpInfo->icmpOutAddrMasks     = _icmpstat.icps_outhist [ICMP_MASKREQ];
    pIcmpInfo->icmpOutAddrMaskReps  = _icmpstat.icps_outhist [ICMP_MASKREPLY];
#else
    pIcmpInfo->icmpInMsgs          += icmpstat.icps_badcode +
                                      icmpstat.icps_tooshort +
                                      icmpstat.icps_checksum +
                                      icmpstat.icps_badlen;
 
    pIcmpInfo->icmpInErrors         = icmpstat.icps_badcode +
                                      icmpstat.icps_tooshort +
                                      icmpstat.icps_checksum +
                                      icmpstat.icps_badlen;
 
    pIcmpInfo->icmpInDestUnreachs   = icmpstat.icps_inhist [ICMP_UNREACH];
    pIcmpInfo->icmpInTimeExcds      = icmpstat.icps_inhist [ICMP_TIMXCEED];
    pIcmpInfo->icmpInParmProbs      = icmpstat.icps_inhist [ICMP_PARAMPROB];
    pIcmpInfo->icmpInSrcQuenchs     = icmpstat.icps_inhist [ICMP_SOURCEQUENCH];
    pIcmpInfo->icmpInRedirects      = icmpstat.icps_inhist [ICMP_REDIRECT];
    pIcmpInfo->icmpInEchos          = icmpstat.icps_inhist [ICMP_ECHO];
    pIcmpInfo->icmpInEchoReps       = icmpstat.icps_inhist [ICMP_ECHOREPLY];
    pIcmpInfo->icmpInTimestamps     = icmpstat.icps_inhist [ICMP_TSTAMP];
    pIcmpInfo->icmpInTimestampReps  = icmpstat.icps_inhist [ICMP_TSTAMPREPLY];
    pIcmpInfo->icmpInAddrMasks      = icmpstat.icps_inhist [ICMP_MASKREQ];
    pIcmpInfo->icmpInAddrMaskReps   = icmpstat.icps_inhist [ICMP_MASKREPLY];
 
    pIcmpInfo->icmpOutErrors        = icmpstat.icps_error;
    pIcmpInfo->icmpOutDestUnreachs  = icmpstat.icps_outhist [ICMP_UNREACH];
    pIcmpInfo->icmpOutTimeExcds     = icmpstat.icps_outhist [ICMP_TIMXCEED];
    pIcmpInfo->icmpOutParmProbs     = icmpstat.icps_outhist [ICMP_PARAMPROB];
    pIcmpInfo->icmpOutSrcQuenchs    = icmpstat.icps_outhist [ICMP_SOURCEQUENCH];
    pIcmpInfo->icmpOutRedirects     = icmpstat.icps_outhist [ICMP_REDIRECT];
    pIcmpInfo->icmpOutEchos         = icmpstat.icps_outhist [ICMP_ECHO];
    pIcmpInfo->icmpOutEchoReps      = icmpstat.icps_outhist [ICMP_ECHOREPLY];
    pIcmpInfo->icmpOutTimestamps    = icmpstat.icps_outhist [ICMP_TSTAMP];
    pIcmpInfo->icmpOutTimestampReps = icmpstat.icps_outhist [ICMP_TSTAMPREPLY];
    pIcmpInfo->icmpOutAddrMasks     = icmpstat.icps_outhist [ICMP_MASKREQ];
    pIcmpInfo->icmpOutAddrMaskReps  = icmpstat.icps_outhist [ICMP_MASKREPLY];
#endif /* VIRTUAL_STACK */
 
    return (OK);
    }

/*******************************************************************************
*
* m2IcmpDelete - delete all resources used to access the ICMP group
*
* This routine frees all the resources allocated at the time the ICMP group was
* initialized.  The ICMP group should not be accessed after this routine has 
* been called.
*
* RETURNS: OK, always.
*
* SEE ALSO: m2IcmpInit(), m2IcmpGroupInfoGet()
*/
STATUS m2IcmpDelete (void)
    {
    return (OK);
    }


/* m2SysLib.c - MIB-II system-group API for SNMP agents */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01h,15oct01,rae  merge from truestack ver 01i, base 01g (VIRTUAL_STACK)
01g,30dec97,vin  fixed SPR 20090
01f,03apr96,rjc  set the m2SystemSem semaphore to NULL in m2SysDelete.
01d,25jan95,jdi  doc cleanup.
01c,11nov94,rhp  additional man-page corrections
01b,10nov94,rhp  edit man pages
01b,22feb94,elh  put in check for null parameters in m2SysInit. 
01a,08dec93,jag  written
*/

/*
DESCRIPTION
This library provides MIB-II services for the system group.  It provides
routines to initialize the group and to access the group scalar variables.
For a broader description of MIB-II services, see the manual entry for m2Lib.

To use this feature, include the following component:
INCLUDE_MIB2_SYSTEM

USING THIS LIBRARY
This library can be initialized and deleted by calling m2SysInit() and
m2SysDelete() respectively, if only the system group's services are
needed.  If full MIB-II support is used, this group and all other groups
can be initialized and deleted by calling m2Init() and m2Delete().

The system group provides the option to set the system variables at the
time m2Sysinit() is called.  The MIB-II variables `sysDescr' and `sysobjectId'
are read-only, and can be set only by the system-group initialization routine.
The variables `sysContact', `sysName' and `sysLocation' can be set through
m2SysGroupInfoSet() at any time.

The following is an example of system group initialization:
.CS
    M2_OBJECTID mySysObjectId = { 8, {1,3,6,1,4,1,731,1} };

    if (m2SysInit ("VxWorks MIB-II library ",
		   "support@wrs.com",
		   "1010 Atlantic Avenue Alameda, California 94501",
		   &mySysObjectId) == OK)
	/@ System group initialized successfully @/
.CE

The system group variables can be accessed as follows:
.CS
    M2_SYSTEM   sysVars;

    if (m2SysGroupInfoGet (&sysVars) == OK)
	/@ values in sysVars are valid @/
.CE

The system group variables can be set as follows:
.CS
    M2_SYSTEM    sysVars; 
    unsigned int varToSet;	/@ bit field of variables to set @/

    /@ Set the new system Name @/

    strcpy (m2SysVars.sysName, "New System Name");
    varToSet |= M2SYSNAME;

   /@ Set the new contact name @/

    strcpy (m2SysVars.sysContact, "New Contact");
    varToSet |= M2SYSCONTACT;

    if (m2SysGroupInfoGet (varToSet, &sysVars) == OK)
	/@ values in sysVars set @/

.CE

INCLUDE FILES: m2Lib.h
 
SEE ALSO:
m2Lib, m2IfLib, m2IpLib, m2IcmpLib, m2UdpLib, m2TcpLib
*/

/* includes */
#include <vxWorks.h>
#include "m2Lib.h"
#include "netLib.h"
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include "hostLib.h"
#include "string.h"
#include "semLib.h"
#include "tickLib.h"
#include "sysLib.h"
#include "errnoLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif /* VIRTUAL_STACK */

#ifndef VIRTUAL_STACK

/* externs */

IMPORT int  _ipCfgFlags; 

/* globals */

/* 
 * The system group is supported by the structure defined below.  All changes to
 * the system group are reflected in this structure.
 */

LOCAL M2_SYSTEM m2SystemVars;

/* This semaphore protects the m2SystemVars from multiple readers and writers */
 
LOCAL SEM_ID m2SystemSem;
LOCAL unsigned long startCentiSecs;	/* Hundred of Seconds at start */

#endif /* VIRTUAL_STACK */

/*
 * The zero object id is used throught out the MIB-II library to fill OID 
 * requests when an object ID is not provided by a group variable.
 */

LOCAL M2_OBJECTID sysZeroObjectId = { 2, {0,0} };

/******************************************************************************
*
* centiSecsGet - get hundreds of a second
*
* The number of hundreds of a second that have passed since this routine was
* first called.
*
* RETURNS: Hundreds of a second since the group was initialized.
*
* SEE ALSO: N/A
*/
LOCAL unsigned long centiSecsGet (void)
    {
    unsigned long currCentiSecs;
    unsigned long clkRate = sysClkRateGet ();

#ifdef VIRTUAL_STACK
    if (sysStartCentiSecs == 0)
	sysStartCentiSecs = (tickGet () * 100) / clkRate;
    currCentiSecs = (tickGet () * 100) / clkRate;
    return (currCentiSecs - sysStartCentiSecs);
#else    /* VIRTUAL_STACK */
    if (startCentiSecs == 0)
	startCentiSecs = (tickGet () * 100) / clkRate;

    currCentiSecs = (tickGet () * 100) / clkRate;

    return (currCentiSecs - startCentiSecs);
#endif   /* VIRTUAL_STACK */
    }

/******************************************************************************
*
* m2SysInit - initialize MIB-II system-group routines
*
* This routine allocates the resources needed to allow access to the
* system-group MIB-II variables.  This routine must be called before
* any system-group variables can be accessed.  The input parameters
* <pMib2SysDescr>, <pMib2SysContact>, <pMib2SysLocation>, and
* <pObjectId> are optional.  The parameters <pMib2SysDescr>,
* <pObjectId> are read only, as specified by MIB-II, and can be set
* only by this routine.
*
* RETURNS: OK, always.
*
* ERRNO:
* S_m2Lib_CANT_CREATE_SYS_SEM
*
* SEE ALSO: m2SysGroupInfoGet(), m2SysGroupInfoSet(), m2SysDelete() 
*/
STATUS m2SysInit
    (
    char *		pMib2SysDescr,	      /* pointer to MIB-2 sysDescr */
    char *		pMib2SysContact,      /* pointer to MIB-2 sysContact */
    char *		pMib2SysLocation,     /* pointer to MIB-2 sysLocation */
    M2_OBJECTID	*	pObjectId 	      /* pointer to MIB-2 ObjectId */
    )
    {
    /* Initialize System Group with defaults */

    if ((pMib2SysDescr != NULL) && (pMib2SysDescr [0] != '\0'))
	strcpy ((char *) m2SystemVars.sysDescr, pMib2SysDescr);

    if ((pMib2SysContact != NULL) && (pMib2SysContact [0] != '\0'))
	strcpy ((char *) m2SystemVars.sysContact, pMib2SysContact);

    if ((pMib2SysLocation != NULL) && (pMib2SysLocation [0] != '\0'))
	strcpy ((char *) m2SystemVars.sysLocation, pMib2SysLocation);
    
    /* Use targets host name as a the system name */

    gethostname ((char *) m2SystemVars.sysName, sizeof(m2SystemVars.sysName));

    if ((pObjectId != NULL) && (pObjectId->idLength > 0))
    	{ 
    	bcopy (((char *) (pObjectId->idArray)), 
	       ((char *) (m2SystemVars.sysObjectID.idArray)), 
	       pObjectId->idLength * sizeof(long));

	m2SystemVars.sysObjectID.idLength = pObjectId->idLength;
	}
    else
	{
	/* Initialize System group OID with the Zero OID */

    	bcopy (((char *) (sysZeroObjectId.idArray)), 
	       ((char *) (m2SystemVars.sysObjectID.idArray)), 
	       sysZeroObjectId.idLength * sizeof(long));

	m2SystemVars.sysObjectID.idLength = sysZeroObjectId.idLength;
	}

    /* Create semaphore */

    if (m2SystemSem == NULL)
        {
        m2SystemSem = semMCreate (SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                SEM_DELETE_SAFE);
        if (m2SystemSem == NULL)
            {
	    errnoSet (S_m2Lib_CANT_CREATE_SYS_SEM);
            return (ERROR);
            }
        }

    (void) centiSecsGet ();     /* Initialize group time reference */
 
    return (OK);
    }

/******************************************************************************
*
* m2SysGroupInfoGet -  get system-group MIB-II variables
*
* This routine fills in the structure at <pSysInfo> with the values of MIB-II
* system-group variables.
*
*
* RETURNS: OK, or ERROR if <pSysInfo> is not a valid pointer.
*
* ERRNO:
* S_m2Lib_INVALID_PARAMETER
*
* SEE ALSO: m2SysInit(), m2SysGroupInfoSet(), m2SysDelete()
*/
STATUS m2SysGroupInfoGet
    (
    M2_SYSTEM * pSysInfo	/* pointer to MIB-II system group structure */
    )
    {
 
    /* Validate Pointer to the requested System structure */
 
    if (pSysInfo == NULL)
	{
	errnoSet (S_m2Lib_INVALID_PARAMETER);
        return (ERROR);
	}
 
    pSysInfo->sysUpTime = centiSecsGet ();
 
    /* Take the System semaphore before reading the system variables */

    semTake (m2SystemSem, WAIT_FOREVER);
 
    strcpy ((char *) pSysInfo->sysDescr,    (char *) m2SystemVars.sysDescr);
    strcpy ((char *) pSysInfo->sysContact,  (char *) m2SystemVars.sysContact);
    strcpy ((char *) pSysInfo->sysName,     (char *) m2SystemVars.sysName);
    strcpy ((char *) pSysInfo->sysLocation, (char *) m2SystemVars.sysLocation);
 
    bcopy (((char *) m2SystemVars.sysObjectID.idArray), 
	   ((char *) pSysInfo->sysObjectID.idArray),
	   m2SystemVars.sysObjectID.idLength * sizeof (long));

    pSysInfo->sysObjectID.idLength = m2SystemVars.sysObjectID.idLength;
 
    semGive (m2SystemSem);
 
    /* Compute type of targets service based on the IP forwarding variables */

    if (_ipCfgFlags & IP_DO_FORWARDING)
        pSysInfo->sysServices |= (1 << (3 - 1));
    else
        pSysInfo->sysServices &= ~(1 << (3 - 1));
 
    return (OK);
    }


/******************************************************************************
*
* m2SysGroupInfoSet - set system-group MIB-II variables to new values
*
* This routine sets one or more variables in the system group as specified in
* the input structure at <pSysInfo> and the bit field parameter <varToSet>.
*
* RETURNS: 
* OK, or ERROR if <pSysInfo> is not a valid pointer, or <varToSet> has an 
* invalid bit field.
*
* ERRNO:
*  S_m2Lib_INVALID_PARAMETER
*  S_m2Lib_INVALID_VAR_TO_SET
*
* SEE ALSO: m2SysInit(), m2SysGroupInfoGet(), m2SysDelete()
*/
STATUS m2SysGroupInfoSet
    (
    unsigned int varToSet,		/* bit field of variables to set */
    M2_SYSTEM * pSysInfo		/* pointer to the system structure */
    )
    {
 
    /* Validate Pointer to System structure and bit field in varToSet */
 
    if (pSysInfo == NULL ||
        (varToSet & (M2SYSNAME | M2SYSCONTACT | M2SYSLOCATION)) == 0)
	{
	if (pSysInfo == NULL)
	    errnoSet (S_m2Lib_INVALID_PARAMETER);
	else
	    errnoSet (S_m2Lib_INVALID_VAR_TO_SET);

        return (ERROR);
	}
 
    /* Set requested variables */
 
    semTake (m2SystemSem, WAIT_FOREVER);
 
    if (varToSet & M2SYSNAME)
        strcpy ((char *) m2SystemVars.sysName, (char *) pSysInfo->sysName);
 
    if (varToSet & M2SYSCONTACT)
        strcpy ((char *) m2SystemVars.sysContact, (char *)pSysInfo->sysContact);
 
    if (varToSet & M2SYSLOCATION)
        strcpy ((char *) m2SystemVars.sysLocation, 
		(char *) pSysInfo->sysLocation);
 
    semGive (m2SystemSem);
 
    return (OK);
    }

/*******************************************************************************
*
* m2SysDelete - delete resources used to access the MIB-II system group
*
* This routine frees all the resources allocated at the time the group was
* initialized.  Do not access the system group after calling this routine.
*
* RETURNS: OK, always.
*
* SEE ALSO: m2SysInit(), m2SysGroupInfoGet(), m2SysGroupInfoSet().
*/
STATUS m2SysDelete (void)
    {
    if (m2SystemSem != NULL)
        {
        semDelete (m2SystemSem);
        m2SystemSem = NULL;
        }


    return (OK);
    }

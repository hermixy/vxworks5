/* hostLib.c - host table subroutine library */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02d,10may02,kbw  making man page edits
02c,15oct01,rae  merge from truestack ver 02g, base 02b (SPR #63610)
02b,15mar99,c_c  Doc: updated hostGetByName manual page (SPR #5184).
02a,14mar99,jdi  doc: removed refs to config.h and/or configAll.h (SPR 25663).
01z,20aug,97,jag added functions hostTblSearchByAddr(), hostTblSearchByName() 
		 to address (SPR #9175)
01x,19may97,spm  added checks for NULL pointers to user interface routines
01w,20apr97,kbw  fixed man page format problems, did a spell check
01v,07apr97,jag  added hooks for the resolver library.
01w,17nov97,dgp  doc: SPR 9410, hostGetByName() returns addr in network byte
			order
01v,29aug97,dgp  doc: SPR 9196 correct ERROR return for hostAdd()
01u,16feb94,caf  added check for NULL pointer in hostGetByName() (SPR #2920).
01t,02feb93,jdi  documentation cleanup for 5.1.
01s,13nov92,dnw  added include of semLibP.h
01r,27jul92,elh  Moved hostShow to netShow.c
01q,18jul92,smb  Changed errno.h to errnoLib.h.
01p,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01o,10dec91,gae  more cleanup of ANSI warnings.
01n,25oct91,rrr  cleanup of some ansi warnings.
01m,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01l,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01k,12feb91,jaa	 documentation.
01j,02oct90,hjb  defined HOSTNAME_LEN. added sethostname() and gethostname().
01i,15jul90,dnw  changed hostNameFill() to coerce malloc() to (char*)
01h,26jun90,jcf  changed hostList semaphore to type mutex.
01g,23may89,shl  changed addr parameter in hostGetByAddr() from char* to int.
01f,21apr89,shl  added hostGetByAddr().
		 added hostDelete ().
01e,18mar88,gae  documentation.
01d,07jul88,jcf  fixed malloc to match new declaration.
01c,04jun88,gae  changed S_remLib_* to S_hostLib_*.
01b,30may88,dnw  changed to v4 names.
01a,28may88,dnw  extracted from remLib.
*/

/*
DESCRIPTION
This library provides routines to store and access the network host database.
The host table contains information regarding the known hosts on the
local network.  The host table (displayed with hostShow()) contains
the Internet address, the official host name, and aliases.

By convention, network addresses are specified in dotted (".") decimal 
notation.  The library inetLib contains Internet address manipulation 
routines.  Host names and aliases may contain any printable character.

Before any of the routines in this module can be used, the library must be
initialized by hostTblInit().  This is done automatically if INCLUDE_HOST_TBL
is defined.

INCLUDE FILES: hostLib.h

SEE ALSO
inetLib
*/

#include "vxWorks.h"
#include "sys/types.h"
#include "netinet/in.h"
#include "stdlib.h"
#include "semLib.h"
#include "string.h"
#include "inetLib.h"
#include "lstLib.h"
#include "hostLib.h"
#include "errnoLib.h"
#include "stdio.h"
#include "private/semLibP.h"
#include "memPartLib.h"

#ifdef VIRTUAL_STACK
#include "netinet/vsLib.h"
#endif

#ifndef VIRTUAL_STACK

#define HOSTNAME_LEN 128

/* globals */

LIST hostList;
SEM_ID hostListSem = NULL;
BOOL hostInitFlag = FALSE;

#endif /* VIRTUAL_STACK */

/* Function pointers installed by resolvLib, when the resolver is included */
														/* Use DNS resolver for the name query */
FUNCPTR _presolvHostLibGetByName = NULL;  

				/* Use DNS resolver for the pointer query */
FUNCPTR _presolvHostLibGetByAddr = NULL;  

/* mutual exclusion options */

int mutexOptionsHostLib = SEM_Q_FIFO | SEM_DELETE_SAFE;

#ifndef VIRTUAL_STACK

/* locals */

LOCAL char targetName [HOSTNAME_LEN];	/* name for this target machine */

#endif /* VIRTUAL_STACK */

/* forward static functions */

static STATUS hostNameFill (HOSTNAME *pHostName, char *newHostName);


/*******************************************************************************
*
* hostTblInit - initialize the network host table
*
* This routine initializes the host list data structure used by routines
* throughout this module.  It should be called before any other routines in
* this module.  This is done automatically if INCLUDE_HOST_TBL is defined.
*
* RETURNS: N/A
*
* SEE ALSO: usrConfig
*/

void hostTblInit (void)
    {

    if (!hostInitFlag)
	{
	hostInitFlag = TRUE;
	lstInit (&hostList);
	hostListSem = semMCreate (mutexOptionsHostLib);
	}
    }

/*******************************************************************************
*
* hostAdd - add a host to the host table
*
* This routine adds a host name to the local host table.
* This must be called before sockets on the remote host are opened,
* or before files on the remote host are accessed via netDrv or nfsDrv.
*
* The host table has one entry per Internet address.
* More than one name may be used for an address.
* Additional host names are added as aliases.
*
* EXAMPLE
* .CS
*    -> hostAdd "wrs", "90.2"
*    -> hostShow
*    hostname         inet address       aliases
*    --------         ------------       -------
*    localhost        127.0.0.1
*    yuba             90.0.0.3
*    wrs              90.0.0.2
*    value = 12288 = 0x3000 = _bzero + 0x18
* .CE
* RETURNS:
* OK, or ERROR if the host table is full, the host name/inet address pair
* is already entered, the Internet address is invalid, or memory is 
* insufficient.
*
* SEE ALSO: netDrv, nfsDrv
*/

STATUS hostAdd
    (
    char *hostName,     /* host name */
    char *hostAddr      /* host addr in standard Internet format */
    )
    {
    HOSTNAME *pHostNamePrev = NULL;	/* pointer to previous host name entry */
    FAST HOSTNAME *pHostName;		/* pointer to host name entry */
    FAST HOSTENTRY *pHostEntry;
    struct in_addr netAddr;		/* network address */

    if (hostName == NULL || hostAddr == NULL)
        {
        errnoSet (S_hostLib_INVALID_PARAMETER);
        return (ERROR);
        }

    if ((netAddr.s_addr = inet_addr (hostAddr)) == ERROR)
	return (ERROR);

    if (semTake (hostListSem, WAIT_FOREVER) == ERROR)
	return (ERROR);

    for (pHostEntry = (HOSTENTRY *)lstFirst (&hostList);
	 pHostEntry != NULL;
	 pHostEntry = (HOSTENTRY *)lstNext (&pHostEntry->node))
	{
	if (pHostEntry->netAddr.s_addr == netAddr.s_addr)
	    {
	    /* host internet address already in table, add name as an alias */

	    pHostNamePrev = &pHostEntry->hostName;

	    for (pHostName = &pHostEntry->hostName;
		 pHostName != NULL;
		 pHostName = pHostName->link)
		{
		/* make sure name is not already used for this address */

		if (strcmp (pHostName->name, hostName) == 0)
		    {
		    semGive (hostListSem);
		    errnoSet (S_hostLib_HOST_ALREADY_ENTERED);
		    return (ERROR);
		    }

		pHostNamePrev = pHostName;
		}

	    if (pHostNamePrev == NULL)
		{
		/* XXX corrupted list! */
		return (ERROR);
		}

	    /* name not used for this address, add it as an alias */

	    if ((pHostNamePrev->link = (HOSTNAME *)
		 KHEAP_ALLOC(sizeof (HOSTNAME))) == NULL)
		{
		semGive (hostListSem);
		return (ERROR);
		}

	    bzero ((char *)pHostNamePrev->link, sizeof(HOSTNAME));

	    if (hostNameFill (pHostNamePrev->link, hostName) == ERROR)
		{
		semGive (hostListSem);
		return (ERROR);
		}

	    semGive (hostListSem);
	    return (OK);
	    }
	}

    /* host name and internet address not in host table, add new host */

    if ((pHostEntry = (HOSTENTRY *) KHEAP_ALLOC(sizeof (HOSTENTRY))) == NULL)
	{
	semGive (hostListSem);
	return (ERROR);
	}
	
    bzero ((char *)pHostEntry, sizeof(HOSTENTRY));

    if ((hostNameFill (&pHostEntry->hostName, hostName)) == ERROR)
	{
	semGive (hostListSem);
	return (ERROR);
	}

    
    pHostEntry->netAddr = netAddr;

    lstAdd (&hostList, &pHostEntry->node);

    semGive (hostListSem);
    return (OK);
    }

/*******************************************************************************
*
* hostDelete - delete a host from the host table
*
* This routine deletes a host name from the local host table.  If <name> is
* a host name, the host entry is deleted.  If <name> is a host name alias,
* the alias is deleted.
*
* RETURNS: OK, or ERROR if the parameters are invalid or the host is unknown.
*
* ERRNO: S_hostLib_INVALID_PARAMETER, S_hostLib_UNKNOWN_HOST
*/

STATUS hostDelete
    (
    char *name, /* host name or alias */
    char *addr  /* host addr in standard Internet format */
    )
    {
    HOSTNAME *pHostNamePrev;	/* pointer to previous host name entry */
    HOSTNAME *pHostNameNext;	/* pointer to next host name entry */
    FAST HOSTNAME *pHostName;
    FAST HOSTENTRY *pHostEntry;
    struct in_addr netAddr;

    if (name == NULL || addr == NULL)
        {
        errnoSet (S_hostLib_INVALID_PARAMETER);
        return (ERROR);
        }

    /* convert from string to int format */

    if ((netAddr.s_addr = inet_addr (addr)) == ERROR)
	return ERROR;

    semTake (hostListSem, WAIT_FOREVER);

    /* search inet address */

    for (pHostEntry = (HOSTENTRY *)lstFirst (&hostList);
	 pHostEntry != NULL;
	 pHostEntry = (HOSTENTRY *)lstNext (&pHostEntry->node))
        {

	if (pHostEntry->netAddr.s_addr != netAddr.s_addr)
	    continue;

	if (strcmp (pHostEntry->hostName.name, name) == 0)	/* given name is exact match */
	    {
	    FAST HOSTNAME * pAlias = pHostEntry->hostName.link;
	    FAST HOSTNAME * pNext = NULL;

	    /* free all associated alias(es) 1st if any, then free itself */

	    for ( ; pAlias != NULL; pAlias = pNext)
		{
		pNext = pAlias->link;
		KHEAP_FREE(pAlias->name);
		KHEAP_FREE((char *) pAlias);
		}

	    lstDelete (&hostList, &pHostEntry->node);
	    semGive (hostListSem);
	    KHEAP_FREE(pHostEntry->hostName.name);
	    KHEAP_FREE((char *) pHostEntry);
	    return (OK);
	    }
        else 	    /* given name is an alias */
	    {
	    for (pHostNamePrev = pHostName = &pHostEntry->hostName;
		 pHostName != NULL;
		 pHostNamePrev = pHostName, pHostName = pHostName->link)
		{
		pHostNameNext = pHostName->link;

		if (strcmp (pHostName->name, name) == 0)	/* found alias */
		    {
		    pHostNamePrev->link = pHostNameNext;
		    semGive (hostListSem);
		    KHEAP_FREE(pHostName->name);
		    KHEAP_FREE((char *) pHostName);
		    return (OK);
		    }
		}
	    }
	}

    errnoSet (S_hostLib_UNKNOWN_HOST);
    semGive (hostListSem);
    return (ERROR);
    }

/*******************************************************************************
*
* hostTblSearchByName - look up a host in the host table by its name
*
* This routine returns the Internet address of a host that has
* been added to the host table by hostAdd().
*
* RETURNS
* The Internet address (as an integer in network byte order), or ERROR if the 
* host is unknown.
*
* NOMANUAL
*
* INTERNAL
* This function is used by the resolver to search the static host table.
*/

int hostTblSearchByName
    (
    char *name          /* name of host */
    )
    {
    HOSTNAME  *pHostName;
    HOSTENTRY *pHostEntry;
    int retAddr = ERROR;

    semTake (hostListSem, WAIT_FOREVER);

    for (pHostEntry = (HOSTENTRY *)lstFirst (&hostList);
	 pHostEntry != NULL;
	 pHostEntry = (HOSTENTRY *)lstNext (&pHostEntry->node))
	{
	/* check official name */

	if (strcmp (pHostEntry->hostName.name, name) == 0)
	    {
	    retAddr = (int)pHostEntry->netAddr.s_addr;
	    break;
	    }

	/* check aliases */

	for (pHostName = pHostEntry->hostName.link;
	     pHostName != NULL;
	     pHostName = pHostName->link)
	    {
	    if (strcmp (pHostName->name, name) == 0)
		{
		retAddr = (int)pHostEntry->netAddr.s_addr;

		/* force termination of outer loop */
		pHostEntry = (HOSTENTRY *)lstLast (&hostList);
		break;
		}
	    }
	}

    semGive (hostListSem);

    return (retAddr);
    }


/*******************************************************************************
*
* hostGetByName - look up a host in the host table by its name
*
* This routine returns the Internet address of a host that has
* been added to the host table by hostAdd().  If the DNS resolver library
* resolvLib has been configured in the vxWorks image, a query for the host
* IP address is sent to the DNS server, if the name was not found in the local
* host table.
*
* RETURNS: The Internet address (as an integer), or ERROR if the host is
*          unknown.
*
* ERRNO: S_hostLib_INVALID_PARAMETER, S_hostLib_UNKNOWN_HOST
*/

int hostGetByName
    (
    char *name          /* name of host */
    )
    {
    int retAddr;

    if (name == (char *) NULL)
        {
        errnoSet (S_hostLib_INVALID_PARAMETER);
        return (ERROR);
        }

    /* Search the host table using the host name as the key */

    retAddr = hostTblSearchByName (name);

    if ((retAddr == ERROR) && (_presolvHostLibGetByName != NULL))
	{

	/*
	 * If host address was not found in the local table.  Try to get the
	 * IP from the DNS server, if and only if the resolver library has
	 * been linked with the Vxworks image.
	 */

	retAddr = (*_presolvHostLibGetByName) (name);
	}


    if (retAddr == ERROR)
	errnoSet (S_hostLib_UNKNOWN_HOST);

    return (retAddr);
    }

/*******************************************************************************
*
* hostTblSearchByAddr - look up a host in the host table by its Internet address
*
* This routine finds the host name by its Internet address and copies it to
* <name>.  The buffer <name> should be preallocated with (MAXHOSTNAMELEN + 1)
* bytes of memory and is NULL-terminated unless insufficient space is
* provided.
*
* WARNING
* This routine does not look for aliases.  Host names are limited to
* MAXHOSTNAMELEN (from hostLib.h) characters.
*
* RETURNS
* OK, or ERROR if the host is unknown.
*
* NOMANUAL
*
* INTERNAL
* This function is used by the resolver to search the static host table.
*/

STATUS hostTblSearchByAddr
    (
    int addr,           /* inet address of host */
    char *name          /* buffer to hold name */
    )
    {
    HOSTENTRY *pHostEntry;
    struct in_addr netAddr;
    STATUS status = ERROR;
    int n;

    netAddr.s_addr = addr;

    semTake (hostListSem, WAIT_FOREVER);

    /* search for internet address */
    for (pHostEntry = (HOSTENTRY *)lstFirst (&hostList);
	 pHostEntry != NULL;
	 pHostEntry = (HOSTENTRY *)lstNext (&pHostEntry->node))
	{
	if (pHostEntry->netAddr.s_addr == netAddr.s_addr)
	    {
	    n = strlen (pHostEntry->hostName.name);

	    strncpy (name, pHostEntry->hostName.name,
			min (n + 1 , MAXHOSTNAMELEN +1));
	    status = OK;
	    break;
	    }
  	}
    semGive (hostListSem);

    return (status);
    }

/*******************************************************************************
*
* hostGetByAddr - look up a host in the host table by its Internet address
*
* This routine finds the host name by its Internet address and copies it to
* <name>.  The buffer <name> should be preallocated with (MAXHOSTNAMELEN + 1)
* bytes of memory and is NULL-terminated unless insufficient space is
* provided.  If the DNS resolver library resolvLib has been configured in the
* vxWorks image, a query for the host name is sent to the DNS server, if the
* name was not found in the local host table.
*
* WARNING
* This routine does not look for aliases.  Host names are limited to
* MAXHOSTNAMELEN (from hostLib.h) characters.
*
* RETURNS
* OK, or ERROR if buffer is invalid or the host is unknown.
*
* SEE ALSO
* hostGetByName()
*/

STATUS hostGetByAddr
    (
    int addr,           /* inet address of host */
    char *name          /* buffer to hold name */
    )
    {
    STATUS status;

    if (name == NULL)
        {
        errnoSet (S_hostLib_INVALID_PARAMETER);
        return (ERROR);
        }

    /* Search the host table using the host address as the key */

    status = hostTblSearchByAddr (addr, name);

    if ((status != OK) && (_presolvHostLibGetByAddr != NULL))
	{

	/*
	 * If host name was not found in the local table.  Try to get the
	 * name from the DNS server, if and only if the resolver library has
	 * been linked with the Vxworks image.
	 */

	status = (*_presolvHostLibGetByAddr) (addr, name);
	}

    if (status != OK)
	errnoSet (S_hostLib_UNKNOWN_HOST);

    return (status);
    }

/*******************************************************************************
*
* hostNameFill - fill in host name
*
* RETURNS: OK or ERROR if out of memory
*/

LOCAL STATUS hostNameFill
    (
    FAST HOSTNAME *pHostName,
    char *newHostName
    )
    {
    FAST char *pName = (char *) KHEAP_ALLOC((unsigned) (strlen (newHostName) + 1));

    if (pName == NULL)
	return (ERROR);

    strcpy (pName, newHostName);
    pHostName->name = pName;
    pHostName->link = NULL;

    return (OK);
    }

/*******************************************************************************
*
* sethostname - set the symbolic name of this machine
*
* This routine sets the target machine's symbolic name, which can be used
* for identification.
*
* RETURNS: OK or ERROR.
*/

int sethostname
    (
    char *name,                 /* machine name */
    int  nameLen                /* length of name */
    )
    {
#ifdef VIRTUAL_STACK
    if (name != NULL && nameLen < sizeof (_targetName))
	{
        strcpy (_targetName, name);
#else
    if (name != NULL && nameLen < sizeof (targetName))
	{
        strcpy (targetName, name);
#endif
	return (0);
	}

    return (-1);
    }

/******************************************************************************
*
* gethostname - get the symbolic name of this machine
*
* This routine gets the target machine's symbolic name, which can be used
* for identification.
*
* RETURNS: OK or ERROR.
*/

int gethostname
    (
    char *name,  /* machine name */
    int   nameLen  /* length of name */
    )
    {
#ifdef VIRTUAL_STACK
    if (name != NULL && strlen (_targetName) < nameLen)
	{
        strcpy (name, _targetName);
#else
    if (name != NULL && strlen (targetName) < nameLen)
	{
        strcpy (name, targetName);
#endif
	return (0);
	}

    return (-1);
    }

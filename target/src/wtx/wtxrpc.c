/* wtxrpc.c - tornado rpc transport library */

/* Copyright 1984-1996 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
02w,28sep01,fle  SPR#28684 : added usePMap parameter to ExchangeCreate ()
                 routines
02v,15jun01,pch  Add new WTX_TS_INFO_GET_V2 service to handle WTX_RT_INFO
                 change (from BOOL32 hasFpp to UINT32 hasCoprocessor) without
                 breaking existing clients.
02u,19apr00,fle  SPR#28684 : Win32 port
02t,29mar00,fle  SPR#28684 : HPUX port
02s,24nov99,fle  SPR#28684 fix : multiple adapters in target server connection
02r,10may99,fle  made wtxRpcExchangeFree not use CLIENT anymore
02q,24mar99,fle  made WIN32 code also use auth_destroy mechanism (SPR#26002)
02p,07jan99,c_c  Let the target use the hard-coded Registry port number (SPR
                 #23681 and #23449).
02o,21oct98,c_c  replaced a getenv by a wpwrGetEnv to be sure that
                 WIND_REGISTRY is set.
02n,18aug98,pcn  Re-use xdr_WTX_MSG_EVTPT_LIST_2.
02m,11jun98,pcn  Added an input parameter at wtxEventListGet.
02l,26may98,pcn  Changed WTX_MSG_EVTPT_LIST_2 in WTX_MSG_EVTPT_LIST_GET.
02k,25mar98,dbt  added WTX_CONTEXT_STATUS_GET.
02j,24mar98,fle  Adapted wtxrpcKey to NULL transport handle mangement.
02i,24mar98,c_c  Get rid of portmapper.
02h,17mar98,jmb  merge HPSIM patch by jmb from 19aug97:
		 added socket option call to prevent target server sync task
		 from hanging when retrieving symbols from host. (SPR 8708)
02g,03mar98,fle  got rid of WTX_REGISTRY_PING service
		 + got rid of regex uses
02f,02mar98,pcn  WTX 2: Removed WTX_UN_REGIS_FOR_EVENT. Added
                 WTX_UNREGISTER_FOR_EVENT, WTX_CACHE_TEXT_UPDATE,
                 WTX_MEM_WIDTH_READ, WTX_MEM_WIDTH_WRITE, WTX_COMMAND_SEND,
                 WTX_OBJ_MODULE_CHECKSUM, WTX_EVENT_LIST_GET,
                 WTX_OBJ_MODULE_LOAD_2, WTX_EVENTPOINT_ADD_2,
                 WTX_EVENTPOINT_LIST_2, WTX_OBJ_MODULE_UNLOAD_2.
02e,06feb98,c_c  Undo the last remove.
02d,05feb98,c_c  Removed an extra include "regex.h".
02c,04feb98,lcs  relocate #ifdef to repair damages
02b,03feb98,lcs  remove static variable declarations
02a,03feb98,lcs  use regex.h for WIN32 & </usr/include/regex.h> for UNIX
01z,26jan98,fle  added WTX_REGISTRY_PING routine
01y,20nov97,fle  added WTX_MEM_DISASSEMBLE service
01x,25feb97,elp  added valid machname in authunix_create() call (SPR# 7959).
01w,30sep96,elp  put in share, adapted to be compiled on target side SPR# 6775.
01v,28jun96,c_s  tweaks for AIX.
01u,09dec95,wmd  used authunix_create() call to pass uid explicity for WIN32,
                 to fix SPR# 5609.
01t,27oct95,p_m  fixed SPR# 5196 by using the pid to generate the base
		 RPC program number (Unix only).
01s,26oct95,p_m  added WTX_AGENT_MODE_GET and WTX_DIRECT_CALL.
01r,24oct95,wmd  fixed so that processing delays caused by timeout in call
                 to hostbyname() is eliminated, spr #5084, 
01q,14jun95,s_w	 return WTX_ERR_EXCHANGE_NO_SERVER if client create fails.
01p,08jun95,c_s  added WTX_TARGET_ATTACH to rpc function list.
01o,01jun95,p_m  changed WTX_FUNC_CALL to use xdr_WTX_MSG_CONTEXT_DESC.
01n,30may95,p_m  completed WTX_MEM_SCAN and WTX_MEM_MOVE implementation.
01m,30may95,c_s  portablity tweak: use struct in_addr with inet_ntoa().
01l,30may95,c_s  added dotted-decimal IP address to key
01k,23may95,s_w  don't setup or destroy authentication for WIN32
01j,23may95,p_m  made missing name changes.
01i,22may95,s_w  update RPC server table for new WTX_MSG_OBJ_KILL in args
		 and add WTX_MSG_VIO_CHAN_GET/RELEASE messages.
01h,22may95,jcf  name revision.
01g,19may95,jcf	 changed over to WIND_REGISTRY.
01g,18may95,jcf	 changed over to TORNADO_REGISTRY.
01f,14may95,s_w	 add in exchange functions for use by C API (wtx.c). Tidy
		 up rouintes to use types from host.h and make all routines
		 use status and error codes rather than print error messages.
01e,23feb95,p_m  took care of WIN32 platform.
01d,14feb95,p_m  changed S_wtx_ to WTX_ERR_ .
01c,22jan95,c_s  fixed memory leak.
01b,20jan95.jcf  made more portable.
01a,24dec94,c_s  written.
*/

/* includes */

#ifdef HOST
#include "wpwrutil.h"
#include "wtxrpc.h"
#include "private/wtxexchp.h"
#include "wtxmsg.h"
#include "wtxxdr.h"

#if defined(RS6000_AIX4) || defined (RS6000_AIX3)
#undef malloc
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <arpa/inet.h>

#ifdef SUN4_SOLARIS2
#   include <sys/sockio.h>
#endif /* SUN4_SOLARIS2 */

#ifndef WIN32
#   include <net/if.h>
#endif /* ! WIN32 */

#include <netinet/in.h>
#include <unistd.h>

#else
#include "hostLib.h"
#include "arpa/inet.h"
#include "socket.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "errno.h"
#include "wtx.h"

#include "wtxrpc.h"
#include "private/wtxexchp.h"
#include "wtxmsg.h"
#include "wtxxdr.h"
#if (CPU==SIMHPPA)
#include "wdb/wdbLibP.h"
#include "sockLib.h"
#endif /* (CPU==SIMHPPA) */
#endif /* HOST */


/* typedefs */

typedef struct rpc_service
    {
    xdrproc_t	xdrIn;
    xdrproc_t	xdrOut;
    } RPC_SERVICE;

#ifndef HOST
/* globals */

UINT32			syncRegistry;	/* inet registry address */
struct opaque_auth 	authCred;	/* address of the authentication info */
#endif /* HOST */

#ifndef MCALL_MSG_SIZE
#define MCALL_MSG_SIZE	24		/* marshalled call message */
#endif /* MCALL_MSG_SIZE */

/* locals */

LOCAL RPC_SERVICE * (*rpcSvcTable)[] = NULL;
LOCAL UINT32	rpcSvcTableSize = 0;
LOCAL XDR	svrXdrs;			/* server XDR struct  */
LOCAL char	svrXdrsMCall[MCALL_MSG_SIZE];	/* marshalled callmsg */

/* This is the UDP retry timeout used when creating a UDP service connection */

LOCAL struct timeval rpcDefaultUdpTimeout = { 5, 0 };

/* forward declarations */

LOCAL WTX_ERROR_T rpcStatToError	/* convert RPC status to WTX err code */
    (
    enum clnt_stat	rpcStat		/* status from call */
    );

LOCAL STATUS rpcSvcTableInstall (void);	/* install initial service table */

LOCAL STATUS rpcSvcAdd			/* add new key in exchange svc table */
    (
    UINT32		svcNum,		/* service number */
    xdrproc_t		xdrIn,		/* XDR input procedure */
    xdrproc_t		xdrOut		/* XDR output procedure */
    );

LOCAL char * wtxRpcKeyFromRpcKeySplitGet	/* get rpc key from structure */
    (
    WTX_RPC_SPLIT_KEY *	pSplitKey	/* splitted key to create key from    */
    );

LOCAL int wtxRpcKeyIPListSizeGet	/* get the number of IP adapters      */
    (
    WTX_RPC_SPLIT_KEY *	pSplitKey	/* splitted RPC key desc              */
    );


/******************************************************************************
*
* wtxRpcSvcUnregister - perform svc_unregister using RPC key string
*
* This routine will unregister the specified RPC service with svc_unregister.
* This routine will not destroy the service.
*
* RETURNS: WTX_OK, or WTX_ERROR if invalid key.
*/

STATUS wtxRpcSvcUnregister
    (
    const char *	rpcKey	/* service key for service to unregister */
    )
    {
    WTX_RPC_SPLIT_KEY	splitKey;

    if (wtxRpcKeySplit (rpcKey, &splitKey) != WTX_OK)	/* split the key */
	return (WTX_ERROR);

    svc_unregister (splitKey.progNum, splitKey.version);

    wtxRpcKeySplitFree (&splitKey);
    return (WTX_OK);
    }

/******************************************************************************
*
* wtxRpcKey - construct an RPC key string given RPC parameters.
*
* An RPC key string is built from the given progNum, version, and protocol
* code.  The format of the string is
* 
*     rpc/<hostname>/<ip-addr>/<progNum>/<version>|<tcp|udp>/<port#>
*
* The hostname is found from the OS.
*
* RETURNS:
* The constructed RPC key string, or NULL if the input data does not represent
* a valid RPC server address or memory for the string could not be allocated.
* The storage for the string should be freed with free() when it is no longer
* needed.
*/

char *wtxRpcKey
    (
    UINT32	progNum,	/* rpc prog number, or NULL TBA */
    UINT32	version,	/* rpc version number */
    UINT32	protocol,	/* IPPROTO_TCP or IPPROTO_UDP */
    void	(* dispatch) 	/* rpc dispatch routine */
		(struct svc_req *, SVCXPRT *),
    BOOL	register2pmap,	/* Do we register to the local portmapper ? */
    SVCXPRT **	ppNewXprt	/* where to return transport */
    )
    {
#if (!defined SUN4_SOLARIS2)
    struct in_addr	ina;			/* inet address               */
#endif /* ! SUN4_SOLARIS2 */
    SVCXPRT *	newXprt = NULL;			/* transport we make          */
    UINT32 	rpcProgNum;			/* hunted rpc program num     */
    UINT32	svc_protocol;			/* protocol sent to the       */
						/* svc_register ()            */
    char * 	ipAddrStr = NULL;		/* string IP addr (a.b.c.d)   */
    char *	newKey = NULL;			/* final key (malloc'd)       */
    char	hostName [MAXHOSTNAMELEN];	/* our host name              */
    char	buf [256];			/* accumulates key string     */
    int		newKeyLen = 0;			/* new key string length      */

#ifdef HOST
    BOOL		firstAddress = TRUE;	/* first inet address ?       */
    int			inetLen = 0;		/* iet address string length  */
    #ifdef WIN32
    struct hostent *	pHostEnt;		/* contains our IP addr       */
    char **		addrList = NULL;	/* address list               */
    char *		inetAddr = NULL;	/* address from inet_ntoa()   */
    #else
    struct sockaddr_in	addr;			/* socket address definition  */
    struct ifconf	ifc;			/* if config result           */
    struct ifreq	ifReq;			/* current if config request  */
    struct ifreq *	ifr = NULL;		/* if config request result   */
    char		ifBuf [BUFSIZ];		/* if config container        */
    char		ipAddr [16];		/* current IP string          */
    int			addrLen = 0;		/* current address length     */
    int			len = 0;		/* adress counter for loop    */
    int			s = 0;			/* connection socket          */
    #endif /* WIN32 */
#endif /* HOST */

    /* sanity check progNum >= 0, version > 0, and IPPROTO_TCP or IPPROTO_UDP */

    if (((protocol != IPPROTO_TCP) && (protocol != IPPROTO_UDP)) ||
	((progNum == 0) && (dispatch == NULL)))
	goto error1;

    if (progNum == 0)					/* hunt for prog num */
	{
	if (protocol == IPPROTO_TCP)
	    newXprt = svctcp_create (RPC_ANYSOCK, 0, 0);
	else
	    newXprt = svcudp_create (RPC_ANYSOCK);

	if (! newXprt)					/* service kaput! */
	    goto error1;

	/*
	 * In order to limit the number of times a Target Server can
	 * get the RPC program number of a just killed (with kill -9)
	 * Target Server we introduce a little bit of random here (SPR# 5196).
	 */

#if defined(WIN32) || !defined(HOST)

	rpcProgNum = WTX_SVC_BASE;
#else /* WIN32 */

	rpcProgNum = WTX_SVC_BASE + ((int) getpid () & 0xff);
#endif /* WIN32 */

	/*
	 * If we don't want to register ourself to the local portmapper, set the
	 * protocol to 0 before calling svc_register ().
	 */

	svc_protocol = register2pmap?protocol:0;

	while (!svc_register (newXprt, ++rpcProgNum, version, dispatch,
			      svc_protocol))
	    if (rpcProgNum >= WTX_SVC_BASE + WTX_MAX_SERVICE_CNT)
		goto error1;

	if (ppNewXprt)					/* return transport? */
	    *ppNewXprt = newXprt;

	progNum = rpcProgNum;				/* store prog num */
	}

    /* Key format: "rpc/host/ip/port/prog/version/{tcp,udp}" */

    gethostname (hostName, sizeof (hostName));		/* where are we? */

#ifdef HOST

    #ifdef WIN32
    if ((pHostEnt = gethostbyname (hostName)))		/* know our IP addr? */
	{
	/* get the size of the cumulative adapters string */

	for (addrList = pHostEnt->h_addr_list ; *addrList != NULL ; addrList ++)
	    {
	    memcpy (&ina.s_addr, *addrList, sizeof (ina.s_addr));

	    if ( (inetAddr = inet_ntoa (ina)) != NULL)
		inetLen += strlen (inetAddr) + 1;
	    }

	/* allocate memory for the IP addresses string */

	if ( (ipAddrStr = (char *) calloc (inetLen, sizeof (char))) == NULL)
	    goto error1;

	/* now concat all the IP addresses in one string separeted with ',' */

	firstAddress = TRUE;

	for (addrList = pHostEnt->h_addr_list ; *addrList != NULL ; addrList ++)
	    {
	    memcpy (&ina.s_addr, *addrList, sizeof (ina.s_addr));

	    if (firstAddress)
		{
		strcpy (ipAddrStr, inet_ntoa (ina));
		firstAddress = FALSE;
		}
	    else
	        {
		sprintf (ipAddrStr, "%s,%s", ipAddrStr, inet_ntoa (ina));
		}
	    }
	}
    else
	if ( (ipAddrStr = strdup ("")) == NULL)
	    goto error1;

    #else /* WIN32 */

    /* init some variables */

    memset (&addr, 0, sizeof (struct sockaddr_in));
    memset (ifBuf, 0, sizeof (ifBuf));
    memset (&ifc, 0, sizeof (struct ifconf));
    memset (&ifReq, 0, sizeof (struct ifreq));

    /*
     * On UNIX hosts, the gethostbyname() routine may not return all the
     * available IP addresses for the local host. We use a part of the
     * RPC 4.0 get_myaddress.c code to retrieve the list of all IPs registered
     * for the local host
     */

    if ( (s = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
	goto error1;

    ifc.ifc_len = sizeof (ifBuf);
    ifc.ifc_buf = ifBuf;

    /* get the if config */

    if (ioctl (s, SIOCGIFCONF, (char *)&ifc) < 0)
	goto error2;

    ifr = ifc.ifc_req;

    /* get the list of all the IP addresses */

    for (len = ifc.ifc_len; len; len -= sizeof (ifReq))
	{
	/* first take to get the length of the ip addr list length */

	ifReq = *ifr;
	if (ioctl (s, SIOCGIFFLAGS, (char *)&ifReq) < 0)
	    goto error2;


	if ( (ifReq.ifr_flags & IFF_UP) &&
	     (ifr->ifr_addr.sa_family == AF_INET))
	    {
	    memcpy (&addr, &ifr->ifr_addr, sizeof (struct sockaddr_in));

#ifdef PARISC_HPUX10
	    strcpy (ipAddr, inet_ntoa (addr.sin_addr));
#else
	    sprintf (ipAddr, "%i.%i.%i.%i", addr.sin_addr.S_un.S_un_b.s_b1,
					    addr.sin_addr.S_un.S_un_b.s_b2,
					    addr.sin_addr.S_un.S_un_b.s_b3,
					    addr.sin_addr.S_un.S_un_b.s_b4);
#endif /* PARISC_HPUX10 */

	    addrLen = strlen (ipAddr);
	    if ( (addrLen >= 4) && (strncmp (ipAddr, "127.", 4) == 0))
		{
		/* this is the local host address 127.0.0.1, don't add it */
		}
	    else
		{
		inetLen += addrLen + 1;
		}
	    }

	ifr++;
	}

    close (s);

    /* do that again */

    if ( (s = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
	goto error1;

    ifc.ifc_len = sizeof (ifBuf);
    ifc.ifc_buf = ifBuf;

    /* get the if config */

    if (ioctl (s, SIOCGIFCONF, (char *)&ifc) < 0)
	goto error2;

    ifr = ifc.ifc_req;

    /* get the list of all the IP addresses */
    /* allocate the string for the IP addresses list */

    if ( (ipAddrStr = (char *) calloc (inetLen, sizeof (char))) == NULL)
	goto error2;

    for (len = ifc.ifc_len; len; len -= sizeof (ifReq))
	{
	/* first take to get the length of the ip addr list length */

	ifReq = *ifr;
	if (ioctl (s, SIOCGIFFLAGS, (char *)&ifReq) < 0)
	    goto error2;


	if ( (ifReq.ifr_flags & IFF_UP) &&
	     (ifr->ifr_addr.sa_family == AF_INET))
	    {
	    memcpy (&addr, &ifr->ifr_addr, sizeof (struct sockaddr_in));

#ifdef PARISC_HPUX10
	    strcpy (ipAddr, inet_ntoa (addr.sin_addr));
#else
	    sprintf (ipAddr, "%i.%i.%i.%i", addr.sin_addr.S_un.S_un_b.s_b1,
					    addr.sin_addr.S_un.S_un_b.s_b2,
					    addr.sin_addr.S_un.S_un_b.s_b3,
					    addr.sin_addr.S_un.S_un_b.s_b4);
#endif /* PARISC_HPUX10 */

	    addrLen = strlen (ipAddr);

	    if (firstAddress)
		{
		if ( (addrLen >= 4) && (strncmp (ipAddr, "127.", 4) == 0))
		    {
		    /* this is the local host address 127.0.01, do not add it */
		    }
		else
		    {
		    strcpy (ipAddrStr, ipAddr);
		    firstAddress = FALSE;
		    }
		}
	    else
	        {
		addrLen = strlen (ipAddr);
		if ( (addrLen >= 4) && (strncmp (ipAddr, "127.", 4) == 0))
		    {
		    /* this is the local host address 127.0.0.1, don't add it */
		    }
		else
		    {
		    sprintf (ipAddrStr, "%s,%s", ipAddrStr, ipAddr);
		    }
		}
	    }

	ifr++;
	}

     #endif /* WIN32 */
#else /* HOST */

    ina.s_addr = hostGetByName (hostName);
    strcpy (ipAddrStr, inet_ntoa (ina));
#endif /* HOST */

    /* calculate the new key length */

    newKeyLen = strlen (hostName) + strlen (ipAddrStr) + 10;
    sprintf (buf, "%d", progNum);
    newKeyLen += strlen (buf) + 1;
    sprintf (buf, "%d", version);
    newKeyLen += strlen (buf) + 1;

    /* If we cannot get the port used, don't write it. */

    if(ppNewXprt == NULL)
	{
	/* allocate room for the new key string */

	if ( (newKey = (char *) calloc (newKeyLen + 1, sizeof (char))) == NULL)
	    goto error2;

	sprintf (newKey, "rpc/%s/%s/%d/%d/%s", hostName, ipAddrStr,
		 progNum, version, (protocol == IPPROTO_TCP) ? "tcp" : "udp");
	}
    else
	{
	sprintf (buf, "%d", (*ppNewXprt)->xp_port);
	newKeyLen += strlen (buf) + 1;

	/* allocate room for the new key string */

	if ( (newKey = (char *) calloc (newKeyLen + 1, sizeof (char))) == NULL)
	    goto error2;

	sprintf (newKey, "rpc/%s/%s/%d/%d/%s/%d", hostName, ipAddrStr,
		 progNum, version, (protocol == IPPROTO_TCP) ? "tcp" : "udp",
		 (*ppNewXprt)->xp_port);
	}

    /* free the ip address string now */

error2:
    if (ipAddrStr != NULL)
	free (ipAddrStr);
#if (defined HOST) && (!defined WIN32)
    close (s); 
#endif /* ! WIN32 */
error1:
    return (newKey);					/* return key */
    }

/*******************************************************************************
*
* wtxRpcKeyIPListUpdate - updates IP address list of the given RPC key
*
* This routine uses the given <rpcKeyStr> RPC key string to update the
* IP address list with the given <ipNum>. It takes the IP address <ipNum> in the
* <rpcKeyStr> (if there are several), and set it first in the IP address list.
*
* This allows to put the given <ipNum> as the first address in the list
* of addresses to use to connect to the target server
*
* If <ipNum> is negative, or if the <rpcKeyStr> is NULL, the routine will exit,
* returning NULL.
*
* RETURNS: a string with updated RPC key, or NULL on error.
*
* ERRORS:
* \ibs
* \ibe
*
* SEE ALSO: wtxRpcKey(), wtxRpcKeySplit()
*/

char * wtxRpcKeyIPListUpdate
    (
    const char *	rpcKeyStr,	/* rpc Key string to update with IP   */
    int			ipNum		/* IP address num to set as default   */
    )
    {
    WTX_RPC_SPLIT_KEY	splitKey;		/* splitted key output struct */
    char *		newKey = NULL;		/* new updated RPC key string */
    char *		tmpIP = NULL;		/* temp address IP holder     */
    int			nAddr = 0;		/* address number in list     */

    /* first of all, check the imput parameters */

    if ((rpcKeyStr == NULL) || (ipNum < 0))
	goto error1;


    /* split the key and look for <withIP> in this key */

    memset (&splitKey, 0, sizeof (WTX_RPC_SPLIT_KEY));
    if (wtxRpcKeySplit (rpcKeyStr, &splitKey) != WTX_OK)
	goto error1;

    if ( ( (nAddr = wtxRpcKeyIPListSizeGet (&splitKey)) == 0) ||
	 (nAddr < ipNum))
	goto error2;

    tmpIP = splitKey.ipAddrList [0];
    splitKey.ipAddrList [0] = splitKey.ipAddrList [ipNum];
    splitKey.ipAddrList [ipNum] = tmpIP;

    /* we have an updated split key structure. Build new key string with it */

    newKey = wtxRpcKeyFromRpcKeySplitGet (&splitKey);

error2:
    wtxRpcKeySplitFree (&splitKey);
error1:
    return (newKey);
    }

/*******************************************************************************
*
* wtxRpcKeyIPListSizeGet - get the number of IP addresses of a splitted RPC key
*
* This routine returns the number of IP addresses from a splitted RPC key
* structure, where the WTX_RPC_SPLIT_KEY structure has the following
* description :
*
* EXPAND ../../../include/wtxrpc.h WTX_RPC_SPLIT_KEY
*
* The <ipAddrList> contains the list of several IP adapters for the given
* address
*
* RETURNS: the number of IP adapters or 0 if there was no adapters
*
* ERRROS: N/A
*
* SEE ALSO: wtxRpcKeySplit(), wtxRpcKey()
*
* NOMANUAL
*/

LOCAL int wtxRpcKeyIPListSizeGet
    (
    WTX_RPC_SPLIT_KEY *	pSplitKey	/* splitted RPC key desc              */
    )
    {
    int			nIpAddr = 0;	/* number of adapters for this IP     */

    if ( (pSplitKey == 0) || (pSplitKey->ipAddrList == NULL))
	goto error1;

    while (pSplitKey->ipAddrList [nIpAddr] != NULL)
	nIpAddr++;

error1:
    return (nIpAddr);
    }

/*******************************************************************************
*
* wtxRpcKeySplitFree - free the freeable members of an RPC key structure
*
* This routine frees the freeable members of an RPC splitted structure which
* definition is :
*
* EXPAND ../../../include/wtxrpc.h WTX_RPC_SPLIT_KEY
*
* The RPC key structure itself is not freed, and it is up to the user to know
* if he has to free it calling free() or not.
*
* RETURNS: WTX_OK
*
* ERRORS: N/A
*
* SEE ALSO: wtxRpcKeySplit(), wtxRpcKeyIPListUpdate()
*/

STATUS wtxRpcKeySplitFree
    (
    WTX_RPC_SPLIT_KEY *	pRpcSplitKey	/* RPC key to free members of         */
    )
    {
    int		addrNum = 0;		/* address number in address list     */

    /* first of all, check the input parameters */

    if ( (pRpcSplitKey == NULL) || (pRpcSplitKey->ipAddrList == NULL))
	goto error1;

    /* free all the IP addressed from the IP address list */

    while (pRpcSplitKey->ipAddrList [addrNum] != NULL)
	{
	free (pRpcSplitKey->ipAddrList [addrNum]);
	addrNum ++;
	}

    free (pRpcSplitKey->ipAddrList);
error1:
    return (WTX_OK);
    }

/******************************************************************************
*
* wtxRpcKeySplit - split an RPC key string into its component parts
*
* The given RPC key string is decoded and the various fields are placed
* into the WTX_RPC_SPLIT_KEY structure provided (by reference).
*
* RETURNS:
* WTX_OK, if the key was split successfully and pSplitKey is filled, or
* WTX_ERROR if the key could not be decoded.
*/

STATUS wtxRpcKeySplit
    (
    const char *	rpcKey,		/* rpc key string to split            */
    WTX_RPC_SPLIT_KEY *	pSplitKey	/* splitted key output structure      */
    )
    {
    STATUS	status = WTX_ERROR;	/* routine returned status            */
    char *	ipAddrList = NULL;	/* list of IP addressed for key       */
    char *	keyStart = NULL;	/* key start in string                */
    char *	ptr = NULL;		/* address list cursor                */
    char *	curIPAddr = NULL;	/* current IP address string          */
    char 	progNum[10];		/* program number in string format    */
    char	version[10];		/* transport version in string format */
    char	protocol[10];		/* tcp or udp                         */
    char	portNum[10] = "     0";	/* port number in string format       */
    char	keyPat[] = "rpc/%[^/]/%[.,0-9a-fA-F]/%[0-9a-fA-F]/%[0-9a-fA-F]/%[tcpudp]/%[0-9a-fA-F]";
    int		nPatternsToMatch = 6;	/* number of patterns to match        */
    int		nMatchedPatterns;	/* number of matched patterns         */
    int		nAddr = 1;		/* number of addresses in key         */

    /* check arguments */

    if ( (rpcKey == NULL) || (pSplitKey == NULL))
	goto error1;

    /*
     * allocate memory for the IP adresses list, default it to the length of the
     * RPC key ... the list of IP addresses should not be longer
     */

    if ( (ipAddrList = (char *) calloc (strlen (rpcKey) + 1, sizeof (char)))
	 == NULL)
	goto error1;

    /* type/host/ipaddr/prognum/vers/protocol/portnum */

    memset (pSplitKey, 0, sizeof (*pSplitKey));

    nMatchedPatterns = sscanf (rpcKey, keyPat, pSplitKey->host,
					       ipAddrList, progNum,
					       version, protocol, portNum);

    /*
     * The port number may not be present in the key. Lets' examine if we got
     * all the elements or not.
     */

    if (nMatchedPatterns == nPatternsToMatch)
	{
    	pSplitKey->port = (unsigned short) atoi (portNum);
	}
    else
	{
	if (nMatchedPatterns == (nPatternsToMatch-1))
	    {	/* port number not present. Use the portmapper */
	    pSplitKey->port = 0;	/* use portmapper */
	    }
	else	/* something wrong in the key! */
	    goto error2;
	}

    pSplitKey->progNum = atoi (progNum);
    pSplitKey->version = atoi (version);

    if (! strcmp (protocol, "udp"))
	pSplitKey->protocol = IPPROTO_UDP;
    else if (! strcmp (protocol, "tcp"))
	pSplitKey->protocol = IPPROTO_TCP;
    else
	goto error2;

    /* allocate and store the list of IP addresses */

    ptr = ipAddrList;
    nAddr = 1;

    while ( (ptr != NULL) && (*ptr != '\0'))
	{
	/* count the number of ',' in the address list */

	if ( (*ptr) == ',')
	    nAddr++;

	ptr++;
	}

    /* allocate <nAddr> + 1 strings */

    if ( (pSplitKey->ipAddrList = (char **) calloc (nAddr + 1, sizeof (char *)))
	 == NULL)
	goto error2;

    keyStart = ipAddrList;
    ptr = ipAddrList;
    nAddr = 0;

    while ((ptr != NULL) && (*ptr != '\0'))
	{
	if ( (*ptr) == ',')
	    {
	    /* there is an IP address list separator, copy this address */

	    if ( (curIPAddr = (char *) calloc ( (int) (ptr - keyStart) + 1,
						sizeof (char))) == NULL)
		goto error3;

	    strncpy (curIPAddr, keyStart, (int) (ptr - keyStart));

	    pSplitKey->ipAddrList [nAddr] = curIPAddr;

	    nAddr ++;
	    keyStart = ptr + 1;
	    }

	ptr ++;
	}

    /* we need to also copy the last address form the key */

    if ( (pSplitKey->ipAddrList [nAddr] = (char *)
	  calloc ( (int) (ptr - keyStart) + 1, sizeof (char))) == NULL)
	goto error3;

    strcpy (pSplitKey->ipAddrList [nAddr], keyStart);
    status = WTX_OK;

error3:
    if (status != WTX_OK)
	wtxRpcKeySplitFree (pSplitKey);
error2:
    if (ipAddrList != NULL)
	free (ipAddrList);
error1:
    return (status);
    }

/*******************************************************************************
*
* wtxRpcKeyFromRpcKeySplitGet - get an RPC key from a splitted RPC key structure
*
* This routines takes the input <pSplitKey> RPC splitted key structure, to
* build an RPC key string.
*
* RETURNS: The RPC key string or NULL on error
*
* ERRORS: N/A
*
* SEE ALSO: wtxRpcKey(), wtxRpcKeySplit(), wtxRpcKeyIPListUpdate()
* wtxRpcKeySplitFree()
*
* NOMANUAL
*/

LOCAL char * wtxRpcKeyFromRpcKeySplitGet
    (
    WTX_RPC_SPLIT_KEY *	pSplitKey	/* splitted key to create key from    */
    )
    {
    char *		ipList = NULL;	/* IP address list, ',' separated     */
    char *		rpcKey = NULL;	/* extracted RPC ey string            */
    char		buf [256];	/* formatting buffer                  */
    int			addrNum = 0;	/* address number in list             */
    int			keyLen = 0;	/* RPC key string length              */
    int			ipListLen = 0;	/* IP addresses list length           */

    /* first of all, check the routines' arguments */

    if ( (pSplitKey == NULL) || (pSplitKey->host == NULL) ||
	 (pSplitKey->ipAddrList == NULL))
	goto error1;

    /* calculate the RPC ey string length */

    memset (&buf, 0, 256);
    keyLen += strlen (pSplitKey->host) + 1;
    sprintf (buf, "%d", pSplitKey->progNum);
    keyLen += strlen (buf) + 1;
    sprintf (buf, "%d", pSplitKey->version);
    keyLen += strlen (buf) + 1;

    /* add the rpc/ + the (udp)|(rpc)/? */

    keyLen += 7;

    /* allocate and store the list of IP addresses */

    addrNum = 0;
    while (pSplitKey->ipAddrList [addrNum] != NULL)
	{
	ipListLen += strlen (pSplitKey->ipAddrList [addrNum]) + 1;
	addrNum ++;
	}

    keyLen += ipListLen;

    /* we have the key cumulated IP list length, allocate the string for it */

    if ( (ipList = (char *) calloc (ipListLen, sizeof (char))) == NULL)
	goto error1;

    addrNum = 0;
    while (pSplitKey->ipAddrList [addrNum] != NULL)
	{
	if (addrNum == 0)
	    strcpy (ipList, pSplitKey->ipAddrList [addrNum]);
	else
	    sprintf (ipList, "%s,%s", ipList, pSplitKey->ipAddrList [addrNum]);

	addrNum ++;
	}

    if (pSplitKey->port == 0)
	{
	/* allocate room for the new key string */

	if ( (rpcKey = (char *) calloc (keyLen + 1, sizeof (char))) == NULL)
	    goto error2;

	sprintf (rpcKey, "rpc/%s/%s/%d/%d/%s", pSplitKey->host,
		 ipList, pSplitKey->progNum, pSplitKey->version,
		 (pSplitKey->protocol == IPPROTO_TCP) ? "tcp" : "udp");
	}
    else
	{
	sprintf (buf, "%u", pSplitKey->port);
	keyLen += strlen (buf) + 1;

	/* allocate room for the new key string */

	if ( (rpcKey = (char *) calloc (keyLen + 1, sizeof (char))) == NULL)
	    goto error2;

	sprintf (rpcKey, "rpc/%s/%s/%d/%d/%s/%u", pSplitKey->host,
		 ipList, pSplitKey->progNum, pSplitKey->version,
		 (pSplitKey->protocol == IPPROTO_TCP) ? "tcp" : "udp",
		 pSplitKey->port);
	}

error2:
    if (ipList != NULL)
	free (ipList);
error1:
    return (rpcKey);
    }

/*******************************************************************************
*
* wtxRpcExchangeCreate - create a new exchange client using the RPC transport
*
* This routine takes a wpwr key string and creates a new exchange client
* 
* RETURNS: WTX_OK, or WTX_ERROR
*
* NOMANUAL
*/

STATUS wtxRpcExchangeCreate
    (
    WTX_XID		xid,		/* Exchange handle                    */
    const char *	key,		/* Key for server to connect to       */
    BOOL		usePMap		/* use port mapper to connect ?       */
    )
    {
    struct sockaddr_in	addr;			/* socket address             */
    WTX_RPC_SPLIT_KEY	split;			/* split key                  */
    CLIENT *		pClient = NULL;		/* rpc client handle          */
    int			rpcSock = 0;		/* rpc socket                 */

#ifdef HOST
    struct hostent *	pHostEnt = NULL;	/* host entry                 */
    char *              registryHost = NULL;	/* registry host name         */
#endif	/* HOST */

    if (key != NULL)
	{
	if (wtxRpcKeySplit (key, &split) != WTX_OK)
	    WTX_EXCHANGE_RETURN (xid, WTX_ERR_EXCHANGE_BAD_KEY, WTX_ERROR);
	}
    else 
	{
	/*
	 * NULL key => contact the registry service so fill in the split key
	 * with the Tornado registry particulars 
	 */

#ifdef HOST

	if ((registryHost = wpwrGetEnv ("WIND_REGISTRY")) == NULL)
	    registryHost = "127.0.0.1";			/* its probably local */
	strncpy (split.host, registryHost, sizeof (split.host));
	split.host [sizeof (split.host) - 1] = '\0';
#endif
	if (usePMap)
	    split.port = 0;
	else
	    split.port = REGISTRY_PORT;

	/*
	 * Set ipAddrList to NULL.  That will cause the address lookup logic
	 * below to use gethostbyname for us.
	 */

	split.ipAddrList = NULL;
	split.progNum	= WTX_SVC_BASE;
	split.version	= 1;
	split.protocol	= IPPROTO_TCP;
	}

    /* 
     * Get the internet address for the given host.  Parse hostname
     * as host name or alternatively in the `xx.xx.xx.xx' notation. 
     */

    memset ((void *) &addr, 0, sizeof (addr));

    /*
     * Get the IP address. First use the dotted decimal part of the
     * split key if that is comprehensible.  Otherwise use
     * gethostbyname to look up the address.  If that fails try to
     * parse the hostname itself as a dotted decimal.  Otherwise
     * complain. 
     */

    if ( (split.ipAddrList != NULL) &&
	 (int) (addr.sin_addr.s_addr = inet_addr (split.ipAddrList [0])) != -1)
	;

#ifdef HOST
#   ifdef WIN32

    else if ((addr.sin_addr.s_addr = inet_addr (split.host)) != INADDR_NONE)
	;
    else if ((pHostEnt = (struct hostent *)gethostbyname (split.host)) != NULL)
	addr.sin_addr.s_addr = * ((u_long *) pHostEnt->h_addr_list[0]);
    else
	{
	wtxRpcKeySplitFree (&split);
	WTX_EXCHANGE_RETURN (xid, WTX_ERR_EXCHANGE_NO_SERVER, WTX_ERROR);
	}

#   else /* WIN32 */

    else if ((pHostEnt = (struct hostent *)gethostbyname (split.host)) != NULL)
	addr.sin_addr.s_addr = * ((u_long *) pHostEnt->h_addr_list[0]);
    else if ( (int) (addr.sin_addr.s_addr = inet_addr (split.host)) == -1)
	{
	wtxRpcKeySplitFree (&split);
	WTX_EXCHANGE_RETURN (xid, WTX_ERR_EXCHANGE_NO_SERVER, WTX_ERROR);
	}

#   endif /* WIN32 */
#else

    else
	addr.sin_addr.s_addr = syncRegistry;
#endif /* HOST */

    addr.sin_family	= AF_INET;
    addr.sin_port = htons(split.port);	/* given by registry/portmapper */
    rpcSock = RPC_ANYSOCK;		/* any old socket */

    if (split.protocol == IPPROTO_TCP)
	pClient = clnttcp_create (&addr, split.progNum, split.version,
				  &rpcSock, 0, 0);
    else
	pClient = clntudp_create (&addr, split.progNum, split.version,
				  rpcDefaultUdpTimeout, &rpcSock);

    if (pClient == NULL)				/* transport failed? */
	{
	wtxRpcKeySplitFree (&split);
	WTX_EXCHANGE_RETURN (xid, WTX_ERR_EXCHANGE_NO_SERVER, WTX_ERROR);
	}

    /* Set up the authorization mechanism */

#ifdef HOST

#   ifndef WIN32

    pClient->cl_auth = authunix_create_default();
#   else

    pClient->cl_auth = authunix_create("WIN32", wpwrGetUid(), NULL, NULL, NULL);
#   endif /* WIN32 */

#else
    /* authentication */

    pClient->cl_auth = authunix_create ("", 0, 0, 0, NULL);

    /* set the real value computed on host and stored on target */

    pClient->cl_auth->ah_cred.oa_flavor = authCred.oa_flavor;
    pClient->cl_auth->ah_cred.oa_base = authCred.oa_base;
    pClient->cl_auth->ah_cred.oa_length = authCred.oa_length;
#endif  /* HOST */

    xid->transport = pClient;
#ifndef HOST
#if (CPU==SIMHPPA)
    /*
     *  Set RPC receive buffer for HPSIM when using SLIP.  This is needed for
     *  target symbol synchronization, since the target server can send
     *  messages larger than the target MTU when not communicating thru WDB.
     */
    {
    WDB_AGENT_INFO agentInfo;
    int blen;

    wdbInfoGet (&agentInfo);
    blen = agentInfo.mtu;
    if ( blen < 1000 )			/* Must be using SLIP */
	setsockopt (rpcSock, SOL_SOCKET, SO_RCVBUF, 
		    (void *) &blen, sizeof (blen));
    }
#endif /* CPU=SIMHPPA */
#endif /* ! HOST */

    wtxRpcKeySplitFree (&split);
    return WTX_OK;
    }

/*******************************************************************************
*
* wtxRpcExchangeControl - perform a miscellaneous exchange control function.
*
* This routine performs a miscellaneous control function on the exchange 
* handle <xid>. 
*
* RETURNS: WTX_OK, or WTX_ERROR.
*
* ERRORS: WTX_ERR_EXCHANGE_INVALID_HANDLE, WTX_ERR_EXCHANGE_INVALID_ARG
*
* NOMANUAL
*/

STATUS wtxRpcExchangeControl
    (
    WTX_XID	xid,		/* Exchange handle */
    UINT32	request,	/* Exchange control request number */
    void *	arg		/* Pointer to request argument */
    )

    {
    CLIENT * pClient;

    pClient = (CLIENT *) xid->transport;

    switch (request) 
	{
	case WTX_EXCHANGE_CTL_TIMEOUT_SET:
	    xid->timeout = *(UINT32 *) arg;
	    break;
	    
	case WTX_EXCHANGE_CTL_TIMEOUT_GET:
	    *(UINT32 *) arg = xid->timeout;
	    break;
	    
	default:
	    WTX_EXCHANGE_RETURN (xid, WTX_ERR_EXCHANGE_REQUEST_UNSUPPORTED, 
				 WTX_ERROR);
	}

    return WTX_OK;
    }


/*******************************************************************************
*
* wxRpcDelete - delete the exchange handle and any transport connection.
*
* This routine deletes the exchange mechanism specified by the exchange 
* handle <xid>. <xid> should not be used again in any further exchange
* calls as the underlying transport mechanism is closed also.
*
* RETURNS: WTX_OK, or WTX_ERROR
*
* NOMANUAL
*/

STATUS wtxRpcExchangeDelete
    (
    WTX_XID xid			/* Exchange handle */
    )
    {
    CLIENT *pClient;

    pClient = (CLIENT *) xid->transport;

    if (pClient == NULL)
	/* Nothing to do */
	return WTX_OK;

    if (pClient->cl_auth != NULL)
	auth_destroy (pClient->cl_auth);

    clnt_destroy (pClient);

    xid->transport = NULL;

    return (WTX_OK);
    }


/*******************************************************************************
*
* wtxRpcExchangeFree - free a exchange call result message
*
* This routine frees the memory allocated internally to the result message
* <pMsg> used in making the exchange service call <svcNum>.
*
* RETURNS: WTX_OK, or WTX_ERROR
*
* NOMANUAL
*/

WTX_ERROR_T wtxRpcExchangeFree
    (
    WTX_REQUEST	svcNum,		/* number of server service result to free */
    void *	pMsg		/* pointer to result message to free */
    )
    {
    if (svcNum > rpcSvcTableSize || (*rpcSvcTable)[svcNum] == NULL)
	return (WTX_ERR_EXCHANGE_REQUEST_UNSUPPORTED);

    (*rpcSvcTable)[svcNum]->xdrOut (&svrXdrs, pMsg);
 
    return (WTX_ERR_NONE);
    }



/*******************************************************************************
*
* wtxRpcExchange - do a WTX API call via the exchange mechanism
*
* This routine performs the exchange service call <svcNum> with in parameters
* pointed to by <pIn> and result (out) parameters pointed to by <pOut>. The
* status of the call is represented by the error code returned.  The result
* <pOut> may have memory allocated to it internally which can be freed by
* using wtxRpcExchangeFree().
*
* RETURNS: WTX_OK, or WTX_ERROR
*
* NOMANUAL
*/

STATUS wtxRpcExchange
    (
    WTX_XID	xid,		/* exchange handle */
    WTX_REQUEST	svcNum,		/* exchange service number */
    void *	pIn,		/* exchange service in args */
    void *	pOut		/* exchange service out args */
    )

    {
    enum clnt_stat	rpcStat; 
    WTX_ERROR_T		wtxStat;
    WTX_MSG_RESULT *	pMsgOut;
    CLIENT *		pClient;
    RPC_SERVICE	*	pService;
    struct timeval	timeout;


    pClient = (CLIENT *) xid->transport;
    if (pClient == NULL)
	WTX_EXCHANGE_RETURN (xid, WTX_ERR_EXCHANGE_NO_TRANSPORT, WTX_ERROR);

    /* Make sure the table has been initialized */

    if (rpcSvcTable == NULL)
	rpcSvcTableInstall ();

    if (svcNum > rpcSvcTableSize || (*rpcSvcTable)[svcNum] == NULL)
	WTX_EXCHANGE_RETURN (xid, WTX_ERR_EXCHANGE_REQUEST_UNSUPPORTED, 
			     WTX_ERROR);

    pService = (*rpcSvcTable)[svcNum];

    /* Construct a timeout struct on the fly for service request */

    timeout.tv_sec = xid->timeout / 1000;
    timeout.tv_usec = xid->timeout * 1000 - timeout.tv_sec * 1000000;

    rpcStat = clnt_call (pClient,
			 svcNum, 
			 pService->xdrIn, pIn,  
			 pService->xdrOut, pOut,  
			 timeout);

    wtxStat = rpcStatToError (rpcStat);

    if (wtxStat != WTX_ERR_NONE)
	{
	/* An RPC error occured. Free the result and return */
	clnt_freeres (pClient, pService->xdrOut, pOut);

	/* Handle case of RPC failing so badly we must disconnect */
	if (wtxStat == WTX_ERR_EXCHANGE_TRANSPORT_DISCONNECT)
	    wtxRpcExchangeDelete (xid);

	WTX_EXCHANGE_RETURN (xid, wtxStat, WTX_ERROR);
	}

    /* Sort cut for NULLPROC that doesn't return a proper WTX message result. */

    if (svcNum == NULLPROC)
	return WTX_OK;

    pMsgOut = (WTX_MSG_RESULT *) pOut;

    if (pMsgOut->wtxCore.errCode != OK)
	{
	UINT32 errCode;

	/* A service specific error occured. Free the result and return */

	errCode = pMsgOut->wtxCore.errCode;
	clnt_freeres (pClient, pService->xdrOut, pOut);
	WTX_EXCHANGE_RETURN (xid, errCode, WTX_ERROR);
	}

    return WTX_ERR_NONE;
    }


/*******************************************************************************
*
* rpcStatToError - convert an RPC status to a WTX error code.
* 
* This routine converts an RPC error status <callStat> to a WTX error code.
*
* RETURNS: the WTX error code or WTX_ERR_NONE if no error occured.
*
* NOMANUAL
*/

LOCAL WTX_ERROR_T rpcStatToError
    (
    enum clnt_stat	rpcStat		/* status from call */
    )

    {
    switch (rpcStat) 
	{
	case RPC_SUCCESS:
	    return WTX_ERR_NONE;	/* call succeeded */

	case RPC_CANTENCODEARGS:	/* can't encode arguments */
	case RPC_CANTDECODERES:		/* can't decode results */
	case RPC_CANTDECODEARGS:	/* decode arguments error */
	case RPC_SYSTEMERROR:		/* generic "other problem" */
	case RPC_UNKNOWNHOST:		/* unknown host name */
	case RPC_UNKNOWNPROTO:		/* unknown protocol -- 4.0 */
	case RPC_PMAPFAILURE:		/* the pmapper failed in its call */
	case RPC_PROGNOTREGISTERED:	/* remote program is not registered */
	case RPC_FAILED:
	    return WTX_ERR_EXCHANGE_TRANSPORT_ERROR;

	case RPC_PROCUNAVAIL:		/* procedure unavailable */
	    return WTX_ERR_EXCHANGE_REQUEST_UNSUPPORTED;

	case RPC_CANTSEND:		/* failure in sending call */
	case RPC_CANTRECV:		/* failure in receiving result */
	case RPC_VERSMISMATCH:		/* rpc versions not compatible */
	case RPC_AUTHERROR:		/* authentication error */
	case RPC_PROGUNAVAIL:		/* program not available */
	case RPC_PROGVERSMISMATCH:	/* program version mismatched */
	    return WTX_ERR_EXCHANGE_TRANSPORT_DISCONNECT;

	case RPC_TIMEDOUT:		/* call timed out */
	    return WTX_ERR_EXCHANGE_TIMEOUT;

	default:
	    return WTX_ERR_EXCHANGE;
	}
    
    }


/*******************************************************************************
*
* rpcSvcAdd - add a new service into the exchange service table
*
* This routine adds a new service with service number <svcNum> into the
* exchange service table.  The in and out XDR routines <xdrIn> and <xdrOut>
* are recorded in the table.
*
* RETURNS: OK, or ERROR.
*/

LOCAL STATUS rpcSvcAdd
    (
    UINT32	svcNum,
    xdrproc_t	xdrIn,
    xdrproc_t	xdrOut
    )

    {
    if (svcNum + 1 > rpcSvcTableSize)
	{
	RPC_SERVICE *(*newTable)[];

	/* Current table not big enough */

	newTable = calloc (svcNum + 1, sizeof (RPC_SERVICE *));
	if (newTable == NULL)
	    return ERROR;

	/* Copy and free the old one */

	if (rpcSvcTable != NULL)
	    {
	    memcpy (newTable, rpcSvcTable, 
		    rpcSvcTableSize * sizeof (RPC_SERVICE *));
	    free (rpcSvcTable);
	    }

	rpcSvcTable = newTable;
	rpcSvcTableSize = svcNum + 1;
	}
    
    (*rpcSvcTable)[svcNum] = malloc (sizeof (RPC_SERVICE));
    if  ((*rpcSvcTable)[svcNum] == NULL)
	/* No space for the new service description */
	return ERROR;

    (*rpcSvcTable)[svcNum]->xdrIn = xdrIn;
    (*rpcSvcTable)[svcNum]->xdrOut = xdrOut;

    return OK;
    }

    
/*******************************************************************************
*
* rpcSvcTableInstall - install initial table of service definitions
*
* This routine sets a the table that defines the XDR conversion functions
* for each available exchange service. No action is taken if this function
* is called more than once.
*
* RETURNS: OK, or ERROR
*/

LOCAL STATUS rpcSvcTableInstall (void)
    {
    if (rpcSvcTable != NULL)

	/* Already initialized */

	return (OK);

    /* Add the services to the table of available services */

    /* store the client XDRs */

    xdrmem_create (&svrXdrs, svrXdrsMCall, MCALL_MSG_SIZE, XDR_FREE);

    /* Ping or no-op service */

    rpcSvcAdd (NULLPROC, xdr_void, xdr_void);

    /* Registry services */

    rpcSvcAdd (WTX_INFO_Q_GET, 
	       xdr_WTX_MSG_WTXREGD_PATTERN, xdr_WTX_MSG_SVR_DESC_Q); 
    rpcSvcAdd (WTX_INFO_GET, xdr_WTX_MSG_PARAM, xdr_WTX_MSG_SVR_DESC); 
    rpcSvcAdd (WTX_REGISTER, xdr_WTX_MSG_SVR_DESC, xdr_WTX_MSG_SVR_DESC); 
    rpcSvcAdd (WTX_UNREGISTER, xdr_WTX_MSG_PARAM, xdr_WTX_MSG_RESULT); 

    /* Target server services */

    rpcSvcAdd (WTX_TOOL_ATTACH, xdr_WTX_MSG_TOOL_DESC, xdr_WTX_MSG_TOOL_DESC);
    rpcSvcAdd (WTX_TOOL_DETACH, xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_TS_INFO_GET_V1, xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_TS_INFO_V1);
    rpcSvcAdd (WTX_TS_INFO_GET_V2, xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_TS_INFO_V2);
    rpcSvcAdd (WTX_TS_LOCK, xdr_WTX_MSG_TS_LOCK, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_TS_UNLOCK, xdr_WTX_MSG_TS_UNLOCK, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_TARGET_RESET, xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_TARGET_ATTACH, xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_RESULT);

#if 0 /* ELP */

    rpcSvcAdd (WTX_TARGET_PING, xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_RESULT);
#endif

    rpcSvcAdd (WTX_CONTEXT_CREATE,
	       xdr_WTX_MSG_CONTEXT_DESC, xdr_WTX_MSG_CONTEXT);
    rpcSvcAdd (WTX_CONTEXT_KILL, xdr_WTX_MSG_CONTEXT, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_CONTEXT_SUSPEND, xdr_WTX_MSG_CONTEXT, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_CONTEXT_CONT, xdr_WTX_MSG_CONTEXT, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_CONTEXT_RESUME, xdr_WTX_MSG_CONTEXT, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_CONTEXT_STATUS_GET, xdr_WTX_MSG_CONTEXT, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_CONTEXT_STEP,
	       xdr_WTX_MSG_CONTEXT_STEP_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_EVENTPOINT_ADD, xdr_WTX_MSG_EVTPT_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_EVENTPOINT_DELETE, xdr_WTX_MSG_PARAM, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_EVENTPOINT_LIST, 
	       xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_EVTPT_LIST);
    rpcSvcAdd (WTX_MEM_CHECKSUM, xdr_WTX_MSG_MEM_SET_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_MEM_DISASSEMBLE, xdr_WTX_MSG_DASM_DESC,
               xdr_WTX_MSG_DASM_INST_LIST);
    rpcSvcAdd (WTX_MEM_READ, 
	       xdr_WTX_MSG_MEM_READ_DESC, xdr_WTX_MSG_MEM_COPY_DESC);
    rpcSvcAdd (WTX_MEM_WRITE, xdr_WTX_MSG_MEM_COPY_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_MEM_SET, xdr_WTX_MSG_MEM_SET_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_MEM_SCAN, xdr_WTX_MSG_MEM_SCAN_DESC,	xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_MEM_MOVE, xdr_WTX_MSG_MEM_MOVE_DESC,	xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_MEM_ALLOC, xdr_WTX_MSG_PARAM, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_MEM_REALLOC,	xdr_WTX_MSG_MEM_BLOCK_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_MEM_ALIGN, xdr_WTX_MSG_MEM_BLOCK_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_MEM_ADD_TO_POOL,
	       xdr_WTX_MSG_MEM_BLOCK_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_MEM_FREE, xdr_WTX_MSG_PARAM, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_MEM_INFO_GET, xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_MEM_INFO);
    rpcSvcAdd (WTX_REGS_GET, xdr_WTX_MSG_REG_READ, xdr_WTX_MSG_MEM_XFER_DESC);
    rpcSvcAdd (WTX_REGS_SET, xdr_WTX_MSG_REG_WRITE, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_OPEN, xdr_WTX_MSG_OPEN_DESC,	xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_VIO_READ, 
	       xdr_WTX_MSG_VIO_COPY_DESC, xdr_WTX_MSG_VIO_COPY_DESC);
    rpcSvcAdd (WTX_VIO_WRITE, xdr_WTX_MSG_VIO_COPY_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_CLOSE, xdr_WTX_MSG_PARAM, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_VIO_CTL, xdr_WTX_MSG_VIO_CTL_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_VIO_FILE_LIST, 
	       xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_VIO_FILE_LIST);
    rpcSvcAdd (WTX_OBJ_MODULE_LOAD, 
	       xdr_WTX_MSG_LD_M_FILE_DESC, xdr_WTX_MSG_LD_M_FILE_DESC);
    rpcSvcAdd (WTX_OBJ_MODULE_UNLOAD, xdr_WTX_MSG_PARAM, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_OBJ_MODULE_LIST, 
	       xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_MODULE_LIST);
    rpcSvcAdd (WTX_OBJ_MODULE_INFO_GET, 
	       xdr_WTX_MSG_MOD_NAME_OR_ID, xdr_WTX_MSG_MODULE_INFO);
    rpcSvcAdd (WTX_OBJ_MODULE_FIND, 
	       xdr_WTX_MSG_MOD_NAME_OR_ID, xdr_WTX_MSG_MOD_NAME_OR_ID);
    rpcSvcAdd (WTX_SYM_TBL_INFO_GET, 
	       xdr_WTX_MSG_PARAM, xdr_WTX_MSG_SYM_TBL_INFO);
    rpcSvcAdd (WTX_SYM_LIST_GET, 
	       xdr_WTX_MSG_SYM_MATCH_DESC, xdr_WTX_MSG_SYM_LIST);
    rpcSvcAdd (WTX_SYM_FIND, xdr_WTX_MSG_SYMBOL_DESC, xdr_WTX_MSG_SYMBOL_DESC);
    rpcSvcAdd (WTX_SYM_ADD, xdr_WTX_MSG_SYMBOL_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_SYM_REMOVE, xdr_WTX_MSG_SYMBOL_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_EVENT_GET, xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_EVENT_DESC);
    rpcSvcAdd (WTX_REGISTER_FOR_EVENT, 
	       xdr_WTX_MSG_EVENT_REG_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_EVENT_ADD, xdr_WTX_MSG_EVENT_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_GOPHER_EVAL, xdr_WTX_MSG_PARAM, xdr_WTX_MSG_GOPHER_TAPE);
    rpcSvcAdd (WTX_FUNC_CALL, xdr_WTX_MSG_CONTEXT_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_DIRECT_CALL, xdr_WTX_MSG_CONTEXT_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_SERVICE_ADD, xdr_WTX_MSG_SERVICE_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_AGENT_MODE_SET, xdr_WTX_MSG_PARAM, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_AGENT_MODE_GET, xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_CONSOLE_CREATE, 
	       xdr_WTX_MSG_CONSOLE_DESC, xdr_WTX_MSG_CONSOLE_DESC);
    rpcSvcAdd (WTX_CONSOLE_KILL, xdr_WTX_MSG_PARAM, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_OBJ_KILL, xdr_WTX_MSG_KILL_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_VIO_CHAN_GET, xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_VIO_CHAN_RELEASE, xdr_WTX_MSG_PARAM, xdr_WTX_MSG_RESULT);

    /* WTX 2: new services */

    rpcSvcAdd (WTX_UNREGISTER_FOR_EVENT,
               xdr_WTX_MSG_EVENT_REG_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_CACHE_TEXT_UPDATE,
               xdr_WTX_MSG_MEM_BLOCK_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_MEM_WIDTH_READ,
               xdr_WTX_MSG_MEM_WIDTH_READ_DESC, xdr_WTX_MSG_MEM_WIDTH_COPY_DESC);
    rpcSvcAdd (WTX_MEM_WIDTH_WRITE,
               xdr_WTX_MSG_MEM_WIDTH_COPY_DESC, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_COMMAND_SEND,
               xdr_WTX_MSG_PARAM, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_OBJ_MODULE_CHECKSUM,
               xdr_WTX_MSG_MOD_NAME_OR_ID, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_EVENT_LIST_GET,
               xdr_WTX_MSG_PARAM, xdr_WTX_MSG_EVENT_LIST);
    rpcSvcAdd (WTX_OBJ_MODULE_LOAD_2,
               xdr_WTX_MSG_FILE_LOAD_DESC, xdr_WTX_MSG_LD_M_FILE_DESC);
    rpcSvcAdd (WTX_EVENTPOINT_ADD_2,
               xdr_WTX_MSG_EVTPT_DESC_2, xdr_WTX_MSG_RESULT);
    rpcSvcAdd (WTX_EVENTPOINT_LIST_GET,
               xdr_WTX_MSG_TOOL_ID, xdr_WTX_MSG_EVTPT_LIST_2);
    rpcSvcAdd (WTX_OBJ_MODULE_UNLOAD_2, xdr_WTX_MSG_MOD_NAME_OR_ID,
               xdr_WTX_MSG_RESULT);

    return (OK);
    }

/* resolvLib.c - DNS resolver library */

/* Copyright 1984 - 2001 Wind River Systems, Inc */
#include <copyright_wrs.h>

/* 
modification history 
-------------------------
01k,15oct01,rae  merge from truestack ver 01l, base 01j (SPRs 67238, 28659)
01j,06oct98,jmp  moved doc to resolvLibDoc.c.
01i,14dec97,jdi  doc: cleanup.
01h,19aug97,jag  fixed man page problems in resolvInit() SPR#9173, fixed
		 SPR#9174, SPR#9175. Deleted getHostInfo(), getbyaddrWrapper()
01g,04aug97,kbw  fixed man page problems found in beta review
01f,19may97,spm  added checks for NULL pointers to user interface (SPR #8603)
01e,30apr97,kbw  fiddled man page text 
01d,01apr97,kbw  fixed man page text, changed parameter name to "length"
                 in resolvDNExpand 
01c,01apr97,jag  removed unused variable resolvDefaultDomainName.  Added
                 routines: resolvHostLibGetByAddr(), resolvHostLibGetByName().
                 Added necessary man pages.
01b,05feb97,jag  added debug function pointer in resolvInit. MAX_DOMAIN_NAME
01a,12aug96,rjc  written
*/


/*
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!      WARNING      !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

         DOCUMENTATION of this library is located in resolvLibDoc.c
      If you modify any documented routine, please update resolvLibDoc.c.

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/


/* includes */

#include <vxWorks.h>
#include <resolvLib.h>
#include <semLib.h>
#include <hostLib.h>
#include <stdlib.h>
#include <sockLib.h>
#include <netinet/in.h>
#include <inetLib.h>
#include <stdio.h>
#include <ctype.h>
#include <errnoLib.h>
#include <string.h>


/* defines */

#define  LOCAL_ENTRIES_TTL     60     /* Local entries have a ttl for 60 seconds */
#define  ALIGNMENT              4     /* for word alignment */
#define  MAX_DOMAIN_NAME      260     /* Max domain name including EOS marker */
#define  MAX_HOSTLIB_BUF      512     /* hostLib interface functions buffer */

/* macro to align ptr to buffer and decrement buffer size accordingly */

#define ALIGN_BUF_PTR(pBuf, bufLen, alignment)     \
     while ((UINT) (pBuf) % (alignment) != 0) \
        {    \
        ++ (pBuf);  \
        -- (bufLen);  \
        }

/* typedefs */

/* Globals */

/*
 * Resolver state default settings.  Structure moved here from res_init.c
 */

struct __res_state _res = 
	{
        RES_TIMEOUT,                    /* retransmition time interval */
        4,                              /* number of times to retransmit */
        RES_DEFAULT,                    /* options flags */
        1,                              /* number of name servers */
	};

/* This variable point to the optional debug routine */

FUNCPTR pdnsDebugFunc;

/* locals */

LOCAL RESOLV_PARAMS_S            resolvParams;        /* resolver settings */

LOCAL STATUS resolvHostLibGetByAddr (int addr, char * pHostName);
LOCAL int resolvHostLibGetByName (char * pHostName);

/* externs */

extern struct hostent *  _gethostbyname ();
extern struct hostent *  _gethostbyaddr ();

/* Ptrs defined in hostLib.c.  These ptrs are set by the resolver library */
extern FUNCPTR _presolvHostLibGetByName;
extern FUNCPTR _presolvHostLibGetByAddr;


/*******************************************************************************
*
* hostEntFormat - Formats a char array as a structure of type hostent
*
* This routine formarts a character array pointed to by <ppBuf> of the length 
* specified by <pBuflen>.  It creates a hostent structure in the buffer along
* with the `h_aliases' and `h_addr_list' vectors big enough for <numAliases> 
* aliases and <numAddrs> addresses respectively. On a successful return 
* <ppBuf> and <pBuflen> specify the remaining unused part of the buffer.
*
* NOMANUAL
*
* RETURNS: A pointer to a hostent structure on success, or NULL if the
* input buffer is too small.
*/

LOCAL struct hostent *  hostEntFormat
    (
    char **           ppBuf,      /* Pointer to character buffer pointer */
    int *             pBufLen,    /* Pointer to the buffer length */
    int               numAliases, /* Max # of host aliases expected */
    int               numAddrs    /* Max # of host addresses expected */
    )
    {
    char *            pTmp;
    struct hostent *  pHostEnt;

    pTmp =  *ppBuf;

    if (((numAliases + 1 + numAddrs + 1) * sizeof (char *) + 
         sizeof (struct hostent) + 3 + 3) > *pBufLen)
        {
        return (NULL);
        }

    ALIGN_BUF_PTR (pTmp, *pBufLen, ALIGNMENT);

    pHostEnt = (struct hostent *) pTmp;

    pTmp += sizeof (struct hostent);
    *pBufLen -= sizeof (struct hostent); 

    ALIGN_BUF_PTR (pTmp, *pBufLen, ALIGNMENT);

    pHostEnt->h_aliases = (char **) pTmp; 

    pTmp += (numAliases + 1)* sizeof (char **);
    *pBufLen -=  (numAliases + 1) * sizeof (char **); 

    pHostEnt->h_addr_list = (char **) pTmp;

    pTmp += (numAddrs + 1)  * sizeof (char **);
    *pBufLen -= (numAddrs + 1)  * sizeof (char **);

    *ppBuf = pTmp;

    return (pHostEnt);
    }

/*******************************************************************************
*
* resolvInit - initialize the resolver library 
*
* This function initializes the resolver.  <pNameServer> is a single IP address
* for a name server in dotted decimal notation.  <pDefaultDomainName> is the 
* default domain name to be appended to names without a dot.  The function 
* pointer <pdnsDebugRtn> is set to the resolver debug function.  Additional
* name servers can be configured using the function resolvParamsSet().
*
* RETURNS: OK or ERROR.
*
* SEE ALSO:
* resolvGetHostByName(), resolvGetHostByAddr(), resolvDNExpand(),
* resolvDNComp(), resolvSend(), resolvParamsSet(), resolvParamsGet(),
* resolvQuery()
*/

STATUS resolvInit
    (
    char *     pNameServer,	    /* pointer to Name server IP address */
    char *     pDefaultDomainName,  /* default domain name */
    FUNCPTR    pdnsDebugRtn         /* function ptr to debug routine */
    )
    {
    int nserv = 0;           /* number of nameserver records read from file */
    struct in_addr   ipAddr;

    /* Initialize global pointer to the DNS debug routine */

    pdnsDebugFunc = (FUNCPTR) NULL;

    if (pdnsDebugRtn != NULL)
	pdnsDebugFunc = pdnsDebugRtn;

    _res.nsaddr.sin_addr.s_addr = INADDR_ANY;
    _res.nsaddr.sin_family = AF_INET;
    _res.nsaddr.sin_port = htons(NAMESERVER_PORT);
    _res.ndots = 1;
    _res.pfcode = 0;
    strncpy (_res.lookups, "b", sizeof _res.lookups);
    
    if (inet_aton (pNameServer, &ipAddr) == OK) 
	{
	_res.nsaddr_list [nserv].sin_addr = ipAddr;
	_res.nsaddr_list [nserv].sin_family = AF_INET;
	_res.nsaddr_list [nserv].sin_port = htons(NAMESERVER_PORT);
	nserv++;
	}
    else
	{
	/* Illegal IP address for DNS server */

	return (ERROR);
	}

    _res.nscount = nserv;	/* Number of name servers */

    _res.dnsrch [0] = _res.defdname;
    _res.options |= RES_INIT | RES_DEFNAMES | RES_DEBUG;

    (void) strcpy (_res.defdname, pDefaultDomainName);

    /* The resolver is initialized to query only the DNS server */

    resolvParams.queryOrder = QUERY_DNS_ONLY;

    /* Install pointers used by hostLib to access the resolver library */
    _presolvHostLibGetByName = resolvHostLibGetByName;
    _presolvHostLibGetByAddr = resolvHostLibGetByAddr;

    return (OK);
    }

/*******************************************************************************
*
* resolvHostGetByName - queries the static host table for the host name
*
* Retrieve host information installed by hostAdd() and install it in the
* `hostent' structure referenced in <pHostEnt>.  This routine uses the 
* buffer referenced in <pBuf> to store hostname and network address 
* information.
*
* NOMANUAL
*
* RETURNS: A pointer to hostent, or NULL if the entry was not found.
*/

LOCAL struct hostent *  resolvHostGetByName 
    (
    char *             pHostName,  /* pointer to the name of the host */
    struct hostent *   pHostEnt,   /* ptr to hostent to hold the results */
    char *             pBuf,       /* buffer to be used by hostnt */
    int                bufLen      /* length of the buffer */
    )
    {

    ALIGN_BUF_PTR (pBuf, bufLen, ALIGNMENT);

    *(int*) pBuf =  hostTblSearchByName (pHostName);

    if (*(int*)pBuf != ERROR)
        {
        pHostEnt->h_addr_list [0] = pBuf;
        pHostEnt->h_addr_list [1] = NULL;
	pHostEnt->h_addrtype = AF_INET;
	pHostEnt->h_length = sizeof (int);
        pBuf += sizeof (int);
        pHostEnt->h_name = pBuf;
        strcpy (pBuf, pHostName);
        pHostEnt->h_ttl =  LOCAL_ENTRIES_TTL;
        return (pHostEnt);
        }
    return (NULL);
    }

/*******************************************************************************
*
* resolvHostGetByAddr - queries the static host table for the IP address
*
* Retrieve host information installed by hostAdd().  Copies the entry into
* the hostent specified by <pHostEnt>, using the buffer specified by <pBuf> and
* bufLen for storing hostname and network address info.
*
* NOMANUAL
*
* RETURNS: A pointer to hostent on sucess, or NULL.
*/

LOCAL struct hostent *  resolvHostGetByAddr 
    (
    const char *       pAddr,    /* pointer to IP address in network order */
    struct hostent *   pHostEnt, /* ptr to hostent to hold the results */
    char *             pBuf,     /* buffer used by hostent */
    int                bufLen    /* length of the buffer */
    )
    {
    ALIGN_BUF_PTR (pBuf, bufLen, ALIGNMENT);

    *(int*) pBuf = *(int*)pAddr;
    pHostEnt->h_addr_list [0] = pBuf;
    pHostEnt->h_addr_list [1] = NULL;
    pBuf += 4;

    if (hostTblSearchByAddr (*((int *)pAddr), pBuf) != OK)
        {
        return (NULL);
        }

    pHostEnt->h_name = pBuf;
    pHostEnt->h_aliases [0] = NULL;
    pHostEnt->h_ttl =  LOCAL_ENTRIES_TTL;
    return (pHostEnt);
    }

/*******************************************************************************
*
* resolvGetHostByName - query the DNS server for the IP address of a host
*
* This function returns a `hostent' structure. This structure is defined as
* follows: 
*
* .CS
*     struct   hostent 
*     {
*     char *   h_name;          /@ official name of host @/ 
*     char **  h_aliases;       /@ alias list @/
*     int      h_addrtype;      /@ address type @/
*     int      h_length;        /@ length of address @/ 
*     char **  h_addr_list;     /@ list of addresses from name server @/
*     unsigned int h_ttl;       /@ Time to Live in Seconds for this entry @/
*     }
* .CE
* The `h_aliases' and `h_addr_type' vectors are NULL-terminated.
*
* Specify the host you want to query in <pHostname>.  Use <pBuf> and <bufLen> 
* to specify the location and size of a buffer to receive the `hostent' 
* structure and its associated contents.  Host addresses are returned in 
* network byte order.  Given the information this routine retrieves, the 
* <pBuf> buffer should be 512 bytes or larger.
*
* RETURNS: A pointer to a `hostent' structure if the host is found, or 
* NULL if the parameters are invalid, the host is not found, or the 
* buffer is too small.
*
* ERRNO:
*  S_resolvLib_INVALID_PARAMETER
*  S_resolvLib_BUFFER_2_SMALL
*  S_resolvLib_TRY_AGAIN
*  S_resolvLib_HOST_NOT_FOUND
*  S_resolvLib_NO_DATA
*  S_resolvLib_NO_RECOVERY
* 
* SEE ALSO:
* resolvInit(), resolvGetHostByAddr(), resolvDNExpand(),
* resolvDNComp(), resolvSend(), resolvParamsSet(), resolvParamsGet(),
* resolvMkQuery(), resolvQuery()
*/
struct hostent *  resolvGetHostByName
    (
    char *     pHostName,  /* ptr to the name of  the host */
    char *     pHostBuf,   /* ptr to the buffer used by hostent structure */
    int        bufLen      /* length of the buffer */ 
    )
    {
    struct hostent *       pHostEnt;         /* Ptr to host entry */
  
    /* Validate input parameters */
    if (pHostName == NULL || pHostBuf == NULL)
        {
        errnoSet (S_resolvLib_INVALID_PARAMETER);
        return (NULL);
        }

    /* Format input buffer for use as a hostent structure */
    pHostEnt = hostEntFormat (&pHostBuf, &bufLen, MAXALIASES, MAXADDRS);

    if (pHostEnt == NULL)
        {
        errno = S_resolvLib_BUFFER_2_SMALL;
        return (NULL);
        }

    /* Check the hostLib static host table first ? */
    if (resolvParams.queryOrder == QUERY_LOCAL_FIRST)
        {
        if (resolvHostGetByName (pHostName, pHostEnt, pHostBuf, bufLen) != NULL)
            {
            return (pHostEnt);
            }
        }

    /* 
     * Ask the DNS Server to resolve the query.  In the case of queryOrder set
     * to QUERY_ DNS_ONLY, this is the only query done.
     */
    if (_gethostbyname (pHostName,  pHostEnt, pHostBuf, bufLen))
        {
        return (pHostEnt);      /* Got an answer from the DNS server */
        }

    /* We need to check the hostLib static host table next */
    if (resolvParams.queryOrder == QUERY_DNS_FIRST)
        {
        if (resolvHostGetByName (pHostName, pHostEnt, pHostBuf, bufLen) != NULL)
            {
            return (pHostEnt);
            }
        }
 
    return (NULL);  /* Host IP Address Not Found */
    }

/*******************************************************************************
*
* resolvGetHostByAddr - query the DNS server for the host name of an IP address
*
* This function returns a `hostent' structure, which is defined as follows:
*
* .CS
* struct   hostent 
*     {
*     char *   h_name;            /@ official name of host @/
*     char **  h_aliases;         /@ alias list @/
*     int      h_addrtype;        /@ address type @/
*     int      h_length;          /@ length of address @/
*     char **  h_addr_list;       /@ list of addresses from name server @/
*     unsigned int h_ttl;         /@ Time to Live in Seconds for this entry @/
*     }
* .CE
* The `h_aliases' and `h_addr_type' vectors are NULL-terminated.
*
* The <pinetAddr> parameter passes in the IP address (in network byte order)
* for the host whose name you want to discover.  The <pBuf> and <bufLen> 
* parameters specify the location and size (512 bytes or more) of the buffer 
* that is to receive the hostent structure.  resolvGetHostByAddr() returns 
* host addresses are returned in network byte order. 
*
* RETURNS: A pointer to a `hostent' structure if the host is found, or 
* NULL if the parameters are invalid, host is not found, or the buffer 
* is too small.
* 
* ERRNO:
*  S_resolvLib_INVALID_PARAMETER
*  S_resolvLib_BUFFER_2_SMALL
*  S_resolvLib_TRY_AGAIN
*  S_resolvLib_HOST_NOT_FOUND
*  S_resolvLib_NO_DATA
*  S_resolvLib_NO_RECOVERY
*
* SEE ALSO:
* resolvGetHostByName(), resolvInit(), resolvDNExpand(),
* resolvDNComp(), resolvSend(), resolvParamsSet(), resolvParamsGet(),
* resolvMkQuery(), resolvQuery()
*/

struct hostent *     resolvGetHostByAddr 
    (
    const char *       pInetAddr,
    char *             pHostBuf,
    int                bufLen
    )
    {
    struct hostent *       pHostEnt;         /* Ptr to host entry */

    if (pInetAddr == NULL || pHostBuf == NULL)
        {
        errnoSet (S_resolvLib_INVALID_PARAMETER);
        return (NULL);
        }

    /* Format input buffer for use as a hostent structure */
    pHostEnt = hostEntFormat (&pHostBuf, &bufLen, MAXALIASES, MAXADDRS);

    if (pHostEnt == NULL)
        {
        errno = S_resolvLib_BUFFER_2_SMALL;
        return (NULL);
        }

    /* Check the hostLib static host table first ? */
    if (resolvParams.queryOrder == QUERY_LOCAL_FIRST)
        {
        if (resolvHostGetByAddr (pInetAddr, pHostEnt, pHostBuf, bufLen) != NULL)
            {
            return (pHostEnt);
            }
        }

    /* 
     * Ask the DNS Server to resolve the query.  In the case of queryOrder set
     * to QUERY_ DNS_ONLY, this is the only query done.
     */
    if (_gethostbyaddr (pInetAddr, 4, AF_INET, pHostEnt, pHostBuf, bufLen))
        {
        return (pHostEnt);      /* Got an answer from the DNS server */
        }

    /* We need to check the hostLib static host table next */
    if (resolvParams.queryOrder == QUERY_DNS_FIRST)
        {
        if (resolvHostGetByAddr (pInetAddr, pHostEnt, pHostBuf, bufLen) != NULL)
            {
            return (pHostEnt);
            }
        }

    return (NULL);             /* Host Name Not found */
    }

/*******************************************************************************
*
* resolvParamsSet - set the parameters which control the resolver library
*
* This routine sets the resolver parameters.  <pResolvParams> passes in
* a pointer to a RESOLV_PARAMS_S structure, which is defined as follows: 
* .CS
*     typedef struct
*        {
*        char   queryOrder;
*        char   domainName [MAXDNAME];
*        char   nameServersAddr [MAXNS][MAXIPADDRLEN];
*        } RESOLV_PARAMS_S;
* .CE
* Use the members of this structure to specify the settings you want to 
* apply to the resolver.  It is important to remember that multiple tasks 
* can use the resolver library and that the settings specified in 
* this RESOLV_PARAMS_S structure affect all queries from all tasks.  In 
* addition, you should set resolver parameters at initialization and not 
* while queries could be in progress. Otherwise, the results of the query 
* are unpredictable.  
*
* Before calling resolvParamsSet(), you should first call resolvParamsGet() 
* to populate a RESOLV_PARAMS_S structure with the current settings.  Then
* you change the values of the members that interest you.    
*
* Valid values for the `queryOrder' member of RESOLV_PARAMS_S structure 
* are defined in resolvLib.h.  Set the `domainName' member to the domain to 
* which this resolver belongs.  Set the `nameServersAddr' member to the IP 
* addresses of the DNS server that the resolver can query.  You must specify 
* the IP addresses in standard dotted decimal notation.  This function tries 
* to validate the values in the `queryOrder' and `nameServerAddr' members.  
* This function does not try to validate the domain name.  
*
* RETURNS: OK if the parameters are valid, ERROR otherwise.
*
* SEE ALSO:
* resolvGetHostByName(), resolvGetHostByAddr(), resolvDNExpand(),
* resolvDNComp(), resolvSend(), resolvInit(), resolvParamsGet(),
* resolvMkQuery(), resolvQuery()
*/

STATUS resolvParamsSet 
    (
    RESOLV_PARAMS_S *  pResolvParams  /* ptr to resolver parameter struct */
    )
    {
    struct in_addr   ipAddr;
    int               index;

   /* Validate queryOrder parameters */

   if (pResolvParams->queryOrder != QUERY_LOCAL_FIRST && 
       pResolvParams->queryOrder != QUERY_DNS_FIRST &&
       pResolvParams->queryOrder != QUERY_DNS_ONLY)
       return (ERROR);


    /* Validate IP addresses */

    for (index = 0 ; (index < MAXNS) && 
	 (pResolvParams->nameServersAddr [index][0] != '\0'); index++)
	{
	if (inet_aton (pResolvParams->nameServersAddr [index], &ipAddr) != OK)
		return(ERROR);
	}

    /* Update queryOrder parameter */

    resolvParams.queryOrder = pResolvParams->queryOrder;

    /* Update the name server IP addresses in decimal dot notation */

    for (index = 0, _res.nscount = 0; (index < MAXNS) && 
	 (pResolvParams->nameServersAddr [index][0] != '\0'); index++)
	{
	
	(void) inet_aton (pResolvParams->nameServersAddr [index], &ipAddr);

	/* update the resolver server entry */

	_res.nsaddr_list [index].sin_addr = ipAddr;
	_res.nsaddr_list [index].sin_family = AF_INET;
	_res.nsaddr_list [index].sin_port = htons(NAMESERVER_PORT);
        _res.lookups [index] = 'b';
        _res.nscount++;  /* Number of DNS servers to query */
	}

    _res.lookups [index] = 'f';

    /* Update domain name; Assume a valid domain name */

    (void) strcpy (_res.defdname, pResolvParams->domainName);

    return (OK);
    }

/*******************************************************************************
*
* resolvParamsGet - get the parameters which control the resolver library
*
* This routine copies the resolver parameters to the RESOLV_PARAMS_S
* structure referenced in the <pResolvParms> parameter.  The RESOLV_PARAMS_S
* structure is defined in resolvLib.h as follows: 
* .CS
*     typedef struct
*        {
*        char   queryOrder;
*        char   domainName [MAXDNAME];
*        char   nameServersAddr [MAXNS][MAXIPADDRLEN];
*        } RESOLV_PARAMS_S;
* .CE
* Typically, you call this function just before calling resolvParamsSet().
* The resolvParamsGet() call populates the RESOLV_PARAMS_S structure. 
* You can then modify the default values just before calling 
* resolvParamsSet().  
*
* RETURNS: N/A
*
* SEE ALSO:
* resolvGetHostByName(), resolvGetHostByAddr(), resolvDNExpand(),
* resolvDNComp(), resolvSend(), resolvParamsSet(), resolvInit(),
* resolvMkQuery(), resolvQuery()
*/

void resolvParamsGet 
    (
    RESOLV_PARAMS_S *     pResolvParams  /* ptr to resolver parameter struct */
    )
    {
    int index;

    pResolvParams->queryOrder = resolvParams.queryOrder;

    (void) strcpy (pResolvParams->domainName, _res.defdname);
    
    /* Copy the name server IP addresses in decimal dot notation */ 
    for ( index = 0 ; index < MAXNS ; index++)
	{ 
	if (index < _res.nscount)
	    {
	    inet_ntoa_b (_res.nsaddr_list [index].sin_addr,
			 pResolvParams->nameServersAddr [index]);
	    }
	else
	    {
	    pResolvParams->nameServersAddr [index][0] = 0;
	    }
	}
    }

/*******************************************************************************
*
* resolvHostLibGetByName - query the DNS server in behalf of hostGetByName
*
* When the resolver library is installed, the routine hostGetByName() in the 
* hostLib library invokes this routine.  When the host name is not found by
* hostGetByName in the static host table.  This feature allows existing 
* applications to take advantage of the resolver without any changes.
*
* NOMANUAL
*
* ERRNO:
*  S_resolvLib_TRY_AGAIN
*  S_resolvLib_HOST_NOT_FOUND
*  S_resolvLib_NO_DATA
*  S_resolvLib_NO_RECOVERY
*
* RETURNS: IP address of host, or ERROR if the host was not found.
*/

LOCAL int resolvHostLibGetByName 
    (
    char * pHostName       /* Pointer to host name */
    )
    {
    char reqBuf [MAX_HOSTLIB_BUF];	       /* Holding buffer  */
    char *     pHostBuf;
    int        bufLen;
    struct hostent *       pHostEnt;           /* Ptr to host entry */

    /* Try to get the answer from the DNS Server */

    pHostBuf = reqBuf;
    bufLen   = sizeof(reqBuf);

    pHostEnt = hostEntFormat (& pHostBuf, & bufLen, MAXALIASES, MAXADDRS);

    pHostEnt = _gethostbyname (pHostName, pHostEnt, pHostBuf, bufLen);

    if (pHostEnt != NULL)
	{
	return (*(int *)(pHostEnt->h_addr_list [0])); /* Host IP address */
	}

    return (ERROR);  /* Host IP address not found ! */
    }

/*******************************************************************************
*
* resolvHostLibGetByAddr - query the DNS server in behalf of hostGetByAddr()
*
* When the resolver library is installed the routine hostGetByAddr() in the 
* hostLib library invokes this routine.  When the IP address is not found by
* hostGetByAddr() in the static host table.  This feature allows existing 
* applications to take advantage of the resolver without any changes.  The
* <addr> paramter specifies the IP address and <pHostName> points to the
* official name of the host when the query is successful.
*
* NOMANUAL
*
* ERRNO:
*  S_resolvLib_TRY_AGAIN
*  S_resolvLib_HOST_NOT_FOUND
*  S_resolvLib_NO_DATA
*  S_resolvLib_NO_RECOVERY
*
* RETURNS: OK, or ERROR if the host name was not found.
*/

LOCAL STATUS resolvHostLibGetByAddr
    (
    int   addr,			/* IP address of requested host name */
    char * pHostName		/* host name output by this routine */
    )
    {
    char reqBuf [MAX_HOSTLIB_BUF];	       /* Holding buffer  */
    char *     pHostBuf;
    int        bufLen;
    struct hostent *       pHostEnt;           /* Ptr to host entry */

    /* Try to get the answer from the DNS Server */

    pHostBuf = reqBuf;
    bufLen   = sizeof(reqBuf);

    pHostEnt = hostEntFormat (& pHostBuf, & bufLen, MAXALIASES, MAXADDRS);

    pHostEnt = _gethostbyaddr ((char *)&addr, 4, AF_INET, pHostEnt, 
			       pHostBuf, bufLen);

    if (pHostEnt != NULL)
	{
        strcpy (pHostName, pHostEnt->h_name);  /* Copy the host official name */
	return (OK);
	}

    return (ERROR);
    }

/*******************************************************************************
*
* resolvDNExpand - expand a DNS compressed name from a DNS packet
*
* This functions expands a compressed DNS name from a DNS packet.  The <msg>
* parameter points to that start of the DNS packet.  The <eomorig> parameter
* points to the last location of the DNS packet plus 1.  The <comp_dn> 
* parameter points to the compress domain name, and <exp_dn> parameter 
* expects a pointer to a buffer.  Upon function completion, this buffer 
* contains the expanded domain name.  Use the <length> parameter to pass in
* the size of the buffer referenced by the <exp_dn> parameter.  
*
* RETURNS: The length of the expanded domain name, or ERROR on failure.
*
* SEE ALSO:
* resolvGetHostByName(), resolvGetHostByAddr(), resolvInit(),
* resolvDNComp(), resolvSend(), resolvParamsSet(), resolvParamsGet(),
* resolvMkQuery(), resolvQuery()
*/

int resolvDNExpand 
    (
    const u_char * msg,     /* ptr to the start of the DNS packet */
    const u_char * eomorig, /* ptr to the last location +1 of the DNS packet */
    const u_char * comp_dn, /* ptr to the compressed domain name */
          u_char * exp_dn,  /* ptr to where the expanded DN is output */
          int      length   /* length of the buffer pointed by <expd_dn> */
    );

/*******************************************************************************
*
* resolvDNComp - compress a DNS name in a DNS packet
*
* This routine takes the expanded domain name referenced in the <exp_dn> 
* parameter, compresses it, and stores the compressed name in the location
* pointed to by the <comp_dn> parameter.  The <length> parameter passes in 
* the length of the buffer starting at <comp_dn>.  The <dnptrs> parameter 
* is a pointer to a list of pointers to previously compressed names.  The 
* <lastdnptr> parameter points to the last entry in the <dnptrs> array.
*
* RETURNS: The size of the compressed name, or ERROR.
*
* SEE ALSO:
* resolvGetHostByName(), resolvGetHostByAddr(), resolvDNExpand(),
* resolvInit(), resolvSend(), resolvParamsSet(), resolvParamsGet(),
* resolvMkQuery(), resolvQuery()
*/

int resolvDNComp 
    (
    const u_char *  exp_dn,   /* ptr to the expanded domain name */
          u_char *  comp_dn,  /* ptr to where to output the compressed name */  
          int       length,   /* length of the buffer pointed by <comp_dn> */
          u_char ** dnptrs,   /* ptr to a ptr list of compressed names */ 
          u_char ** lastdnptr /* ptr to the last entry pointed by <dnptrs> */
      );

/*******************************************************************************
*
* resolvQuery - construct a query, send it, wait for a response
*
* This routine constructs a query for the domain specified in the <name> 
* parameter.  The <class> parameter specifies the class of the query. 
* The <type> parameter specifies the type of query. The routine then sends
* the query to the DNS server.  When the server responds, the response is 
* validated and copied to the buffer you supplied in the <answer> parameter.
* Use the <anslen> parameter to pass in the size of the buffer referenced
* in <answer>.
*
* RETURNS: The length of the response or ERROR.
*
* ERRNO:
*  S_resolvLib_TRY_AGAIN
*  S_resolvLib_HOST_NOT_FOUND
*  S_resolvLib_NO_DATA
*  S_resolvLib_NO_RECOVERY
*
* SEE ALSO:
* resolvGetHostByName(), resolvGetHostByAddr(), resolvDNExpand(),
* resolvDNComp(), resolvInit(), resolvParamsSet(), resolvParamsGet(),
* resolvMkQuery()
*/

int resolvQuery 
    (
    char   *name,       /* domain name */
    int    class,       /* query class for IP is C_IN */
    int    type,        /* type is T_A, T_PTR, ... */
    u_char *answer,     /* buffer to put answer */
    int    anslen       /* length of answer buffer */
    );


/*******************************************************************************
*
* resolvMkQuery - create all types of DNS queries
*
* This routine uses the input parameters to create a domain name query.
* You can set the <op> parameter to QUERY or IQUERY.  Specify the domain 
* name in <dname>, the class in <class>, the query type in <type>.  Valid
* values for type include T_A, T_PTR, and so on.  Use <data> to add Resource 
* Record data to the query.  Use <datalen> to pass in the length of the 
* data buffer.  Set <newrr_in> to NULL.  This parameter is reserved for 
* future use.  The <buf> parameter expects a pointer to the output buffer 
* for the constructed query.  Use <buflen> to pass in the length of the 
* buffer referenced in <buf>.
*
* RETURNS: The length of the constructed query or ERROR.
*
* SEE ALSO:
* resolvGetHostByName(), resolvGetHostByAddr(), resolvDNExpand(),
* resolvDNComp(), resolvSend(), resolvParamsSet(), resolvParamsGet(),
* resolvInit(), resolvQuery()
*/

int resolvMkQuery 
    (
          int     op,	       /* set to desire query QUERY or IQUERY */
    const char *  dname,       /* domain name to be use in the query */
    int           class,       /* query class for IP is C_IN */
    int           type,        /* type is T_A, T_PTR, ... */
    const char *  data,        /* resource Record (RR) data */
    int           datalen,     /* length of the RR */
    const char *  newrr_in,    /* not used always set to NULL */
          char *  buf,         /* out of the constructed query */
          int     buflen       /* length of the buffer for the query */
    );

/*******************************************************************************
*
* resolvSend - send a pre-formatted query and return the answer
*
* This routine takes a pre-formatted DNS query and sends it to the domain
* server.  Use <buf> to pass in a pointer to the query.  Use <buflen> to 
* pass in the size of the buffer referenced in <buf>.  The <answer> parameter
* expects a pointer to a buffer into which this routine can write the 
* answer retrieved from the server.  Use <anslen> to pass in the size of
* the buffer you have provided in <anslen>.
*
* RETURNS: The length of the response or ERROR.
*
* ERRNO:
*  S_resolvLib_TRY_AGAIN
*  ECONNREFUSE
*  ETIMEDOU
*
* SEE ALSO:
* resolvGetHostByName(), resolvGetHostByAddr(), resolvDNExpand(),
* resolvDNComp(), resolvInit(), resolvParamsSet(), resolvParamsGet(),
* resolvMkQuery(), resolvQuery()
*/

int resolvSend 
    (
    const char * buf,		/* pre-formatted query */
          int    buflen,	/* length of query */
          char * answer,	/* buffer for answer */
          int    anslen		/* length of answer */
    );



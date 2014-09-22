/* resolvLib/resolvLibDoc.c - DNS resolver library */

/* Copyright 1998-2002 Wind River Systems, Inc */
#include <copyright_wrs.h>

/* 
modification history 
--------------------
01b,25apr02,vvv  updated doc for resolvGetHostByName (SPR #72988)
01a,06oct98,jmp  written from resolvLib.c
*/

/*
DESCRIPTION
This library provides the client-side services for DNS (Domain Name
Service) queries.  DNS queries come from applications that require
translation of IP addresses to host names and back.  If you include 
this library in VxWorks, it extends the services of the host library.  The
interface to this library is described in hostLib.  The hostLib interface 
uses resolver services to get IP and host names. In addition, the resolver
can query multiple DNS servers, if necessary, to add redundancy for queries.  

There are two interfaces available for the resolver library.  One is 
a high-level interface suitable for most applications.  The other is also a
low-level interface for more specialized applications, such as mail protocols.

USING THIS LIBRARY
By default, a VxWorks build does not include the resolver code.  In addition,
VxWorks is delivered with the resolver library disabled.  To include the
resolver library in the VxWorks image, edit config/all/configAll.h and 
include the definition:
.CS
    #define INCLUDE_DNS_RESOLVER
.CE
To enable the resolver services, you need to redefine only one DNS 
server IP address, changing it from a place-holder value to an actual value. 
Additional DNS server IP addresses can be configured using resolvParamsSet().
To do the initial configuration, edit configAll.h, and enter the correct IP 
address for your domain server in the definition:
.CS
    #define RESOLVER_DOMAIN_SERVER  "90.0.0.3"
.CE
If you do not provide a valid IP address, resolver initialization fails.  
You also need to configure the domain to which your resolver belongs.  To 
do this, edit configAll.h and enter the correct domain name for your 
organization in the definition:
.CS
    #define RESOLVER_DOMAIN  "wrs.com"
.CE
The last and most important step is to make sure that you have a route to 
the configured DNS server.  If your VxWorks image includes a routing protocol, 
such as RIP or OSPF, the routes are created for you automatically.  Otherwise,
you must use routeAdd() or mRouteAdd() to add the routes to the routing table.

The resolver library comes with a debug option.  To turn on debugging, 
edit configAll.h to include the define:
.CS
    #define INCLUDE_DNS_DEBUG
.CE
This include makes VxWorks print a log of the resolver queries to the console.
This feature assumes a single task.  Thus, if you are running multiple tasks,
your output to the console is a garble of messages from all the tasks.

The resolver library uses UDP to send queries to the DNS server and expects
the DNS server to handle recursion.  You can change the resolver parameters 
at any time after the library has been initialized with resolvInit().
However, it is strongly recommended that you change parameters only shortly 
after initialization, or when there are no other tasks accessing the resolver
library.  

Your procedure for changing any of the resolver parameter should start
with a call to resolvParamsGet() to retrieve the active parameters.  Then
you can change the query order (defaults to query DNS server only), the 
domain name, or add DNS server IP addresses.  After the parameters are
changed, call resolvParamsSet().  For the values you can use when accessing 
resolver library services, see the header files resolvLib.h, resolv/resolv.h, 
and resolv/nameser.h. 

INCLUDE FILES: resolvLib.h 
SEE ALSO
hostLib
*/

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
    ...
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
* The `h_aliases' and `h_addr_list' vectors are NULL-terminated. For a locally
* resolved entry `h_ttl' is always 60 (an externally resolved entry may also
* have a TTL of 60 depending on its age but it is usually much higher).
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
    ...
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
* The `h_aliases' and `h_addr_list' vectors are NULL-terminated. For a locally
* resolved entry `h_ttl' is always 60 (an externally resolved entry may also
* have a TTL of 60 depending on its age but it is usually much higher).
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
    ...
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
    ...
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
    ...
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
    ...
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
    ...
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
    )
    {
    ...
    }

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
    )
    {
    ...
    }

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
    )
    {
    ...
    }

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
    )
    {
    ...
    }

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
    )
    {
    ...
    }

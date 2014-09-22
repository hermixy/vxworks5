/* inetLib.c - Internet address manipulation routines */

/* Copyright 1984 - 2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specify the terms and conditions for redistribution.
 */

/*
modification history
--------------------
02k,10may02,kbw  making man page edits
02j,05nov01,vvv  fixed compilation warnings
02i,15oct01,rae  merge from truestack ver 02p, base 02h (SPRs 9424, 21740,
                 65809, 34949, 36026)
02h,13dec97,kbw  made man page edits
02g,31jul97,kbw  fixed man page problems found in beta review
02f,15apr97,kbw  fixing man page format and minor wording issues
02e,16dec96,jag  added function inet_aton, added stricter check for invalid
		 IP addresses to inet_addr. Cleaned up warnings.
02d,06sep96,vin  upgraded to BSD44. Added CLASSD network and host processing.
		 removed call to in_makeaddr_b, no longer necessary.
02d,07oct96,dgp  doc: inet_ntoa and inet_ntoa_b - update descriptions
02c,16may96,dgp  doc fixes, SPR 5991 & 5993.
02b,09jan96,gnn	 Added a define for MAX_PARTS for inet_addr.  Fixed
		 and array bounds checking error.  (SPR 5217)
02a,31oct95,jdi  doc: changed in.h to inet.h (SPR 5306).
01z,16oct95,jdi  doc: removed extra zeroes in some inet addresses (SPR 4869).
01y,16feb94,caf  added check for NULL pointer in inet_addr() (SPR #2920).
01x,20jan93,jdi  documentation cleanup for 5.1.
01w,18jul92,smb  Changed errno.h to errnoLib.h.
01v,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01u,10dec91,gae  added includes for ANSI.
01t,04oct91,rrr  passed through the ansification filter
                  -changed functions to ansi style
		  -changed includes to have absolute path from h/
		  -changed VOID to void
		  -changed copyright notice
01s,20may91,jdi	 documentation tweak.
01r,30apr91,jdi	 documentation tweaks.
01q,05apr91,jdi	 documentation -- removed header parens and x-ref numbers;
		 doc review by dnw.
01p,12feb91,jaa	 documentation.
01o,26jun90,hjb  moved Show routines to netShow.c.
01n,11may90,yao  added missing modification history (01m) for the last checkin.
01m,09may90,yao  typecasted malloc to (char *).
01l,11apr90,hjb  de-linted.
01k,08sep89,hjb  added tcpstatShow(), udpstatShow(), icmpstatShow(),
		 inetstatShow().
01j,29sep88,gae  documentation.
01i,18aug88,gae  documentation.
01h,30may88,dnw  changed to v4 names.
01g,18feb88,dnw  changed inet_netof_string() to be work with subnet masks
		   by changing to call in_{makeaddr_b,netof} instead of
		   inet_{...}.
		 lint.
01f,15dec87,gae  appeased lint; checked malloc's for NULL.
01e,17nov87,ecs  lint.
01d,16nov87,jlf  documentation.
01c,16nov87,llk  documentation.
		 changed to use more descriptive variable names.
01b,06nov87,dnw  cleanup.
01a,01nov87,llk  written.
		 modified routines for VxWorks style (setStatus(), etc.)
		 added inet_netof_string(), inet_makeaddr_b(), inet_ntoa_b().
		 changed inet_ntoa() and inet_makeaddr() so that they malloc
		   the structures that they return.
		 NOTE: inet_addr returns u_long as specified in SUN documenta-
		   tion, NOT in ISI documentation and arpa/inet.h header file.
*/

/*
DESCRIPTION
This library provides routines for manipulating Internet addresses,
including the UNIX BSD 4.3 'inet_' routines.  It includes routines for
converting between character addresses in Internet standard dotted decimal 
notation and integer addresses, routines for extracting the 
network and host portions out of an Internet address, and routines 
for constructing Internet addresses given the network and host address 
parts.

All Internet addresses are returned in network order (bytes ordered from
left to right).  All network numbers and local address parts are returned
as machine format integer values.

INTERNET ADDRESSES
Internet addresses are typically specified in dotted decimal notation or 
as a 4-byte number.  Values specified using the dotted decimal notation 
take one of the following forms:
.CS
	a.b.c.d
	a.b.c
	a.b
	a
.CE
If four parts are specified, each is interpreted as a byte of data and
assigned, from left to right, to the four bytes of an Internet address.
Note that when an Internet address is viewed as a 32-bit integer quantity
on any MC68000 family machine, the bytes referred to above appear as
"a.b.c.d" and are ordered from left to right.

If a three-part address is specified, the last part is interpreted as a
16-bit quantity and placed in the right-most two bytes of the network
address.  This makes the three-part address format convenient for
specifying Class B network addresses as "128.net.host".

If a two-part address is supplied, the last part is interpreted as a
24-bit quantity and placed in the right-most three bytes of the network
address.  This makes the two-part address format convenient for specifying
Class A network addresses as "net.host".

If only one part is given, the value is stored directly in the network
address without any byte rearrangement.

Although dotted decimal notation is the default, it is possible to use the 
dot notation with hexadecimal or octal numbers.  The base is indicated
using the same prefixes as are used in C.  That is, a leading 
0x or 0X indicates a hexadecimal number.  A leading 0 indicates an
octal number.  If there is no prefix, the number is interpreted as decimal.

To use this feature, include the following component:
INCLUDE_NETWRS_INETLIB

INCLUDE FILES:
inetLib.h, inet.h

SEE ALSO: UNIX BSD 4.3 manual entry for inet(3N)
*/

#include "vxWorks.h"
#include "sys/types.h"
#include "ctype.h"
#include "netinet/in.h"
#include "memLib.h"
#include "string.h"
#include "inetLib.h"
#include "stdlib.h"
#include "errnoLib.h"
#include "stdio.h"

/*******************************************************************************
*
* inet_addr - convert a dot notation Internet address to a long integer
*
* This routine interprets an Internet address.  All the network library
* routines call this routine to interpret entries in the data bases
* which are expected to be an address.  The value returned is in network order.
* Numbers will be interpreted as octal if preceded by a zero (e.g. "017.0.0.3"),
* as hexadecimal if preceded by 0x (e.g. "0x17.0.0.4"), and as decimal in all
* other cases.
*
* EXAMPLE
* The following example returns 0x5a000002:
* .CS
*     inet_addr ("90.0.0.2");
* .CE
*
* RETURNS: The Internet address, or ERROR.
*/

u_long inet_addr
    (
    register char *inetString    /* string inet address */
    )
    {
#define MAX_PARTS 4 	/* Maximum number of parts in an IP address. */

    register u_long val, base, n;
    register char c;
    u_long parts[MAX_PARTS], *pp = parts;

    /* check for NULL pointer */

    if (inetString == (char *) NULL)
	{
	(void) errnoSet (S_inetLib_ILLEGAL_INTERNET_ADDRESS);
	return (ERROR);
	}

again:

    /* Collect number up to ``.''.  Values are specified as for C:
     * 0x=hex, 0=octal, other=decimal. */

    val = 0; base = 10;
    if (*inetString == '0')
	{
	base = 8, inetString++;
        if (*inetString == 'x' || *inetString == 'X')
	    base = 16, inetString++;
        }
    while ((c = *inetString))
	{
	if (isdigit ((int) c))
	    {
	    val = (val * base) + (c - '0');
	    inetString++;
	    continue;
	    }
	if (base == 16 && isxdigit ((int) c))
	    {
	    val = (val << 4) + (c + 10 - (islower ((int) c) ? 'a' : 'A'));
		inetString++;
		continue;
	    }
	    break;
	} /* while */

    if (*inetString == '.')
	{
	/*
	 * Internet format:
	 *	a.b.c.d
	 *	a.b.c	(with c treated as 16-bits)
	 *	a.b	(with b treated as 24 bits)
	 * Check each value for greater than 0xff for each part of the IP addr.
	 */

	if ((pp >= parts + (MAX_PARTS - 1)) || val > 0xff)
	    {
	    (void) errnoSet (S_inetLib_ILLEGAL_INTERNET_ADDRESS);
	    return (ERROR);
	    }
	*pp++ = val, inetString++;
	goto again;
	}

    /* Check for trailing characters */

    if (*inetString && !isspace ((int) *inetString)) 
	{
	(void) errnoSet (S_inetLib_ILLEGAL_INTERNET_ADDRESS);
	return (ERROR);
	}
    *pp++ = val;

    /* Concoct the address according to the number of parts specified. */

    n = pp - parts;
    switch ((int) n)
	{
	case 1:				/* a -- 32 bits */
	    val = parts[0];
	    break;

	case 2:				/* a.b -- 8.24 bits */
	    if (val > 0xffffff)
	        {
		(void) errnoSet (S_inetLib_ILLEGAL_INTERNET_ADDRESS);
		return (ERROR);
		}

	    val = (parts[0] << 24) | parts[1];
	    break;

	case 3:				/* a.b.c -- 8.8.16 bits */
	    if (val > 0xffff)
	        {
		(void) errnoSet (S_inetLib_ILLEGAL_INTERNET_ADDRESS);
		return (ERROR);
		}

	    val = (parts[0] << 24) | (parts[1] << 16) | parts[2];
	    break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
	    if (val > 0xff)
	        {
		(void) errnoSet (S_inetLib_ILLEGAL_INTERNET_ADDRESS);
		return (ERROR);
		}

	    val = (parts[0] << 24) | (parts[1] << 16) |
		  (parts[2] << 8) | parts[3];
	    break;

	default:
	    (void) errnoSet (S_inetLib_ILLEGAL_INTERNET_ADDRESS);
	    return (ERROR);
	}

    return (htonl (val));
    }

#ifndef	STANDALONE_AGENT
/*******************************************************************************
*
* inet_lnaof - get the local address (host number) from the Internet address
*
* This routine returns the local network address portion of an Internet address.
* The routine handles class A, B, and C network number formats.
*
* EXAMPLE
* The following example returns 2:
* .CS
*     inet_lnaof (0x5a000002);
* .CE
*
* RETURNS: The local address portion of <inetAddress>.
*/

int inet_lnaof
    (
    int inetAddress   /* inet addr from which to extract local portion */
    )
    {
    register u_long i = ntohl ((u_long) inetAddress);

    if (IN_CLASSA (i))
	return ((i) &IN_CLASSA_HOST);
    else if (IN_CLASSB (i))
	return ((i) &IN_CLASSB_HOST);
    else if (IN_CLASSC (i))
	return ((i) &IN_CLASSC_HOST);
    else
	return ((i) &IN_CLASSD_HOST); 
    }
/*******************************************************************************
*
* inet_makeaddr_b - form an Internet address from network and host numbers
*
* This routine constructs the Internet address from the network number and
* local host address.  This routine is identical to the UNIX inet_makeaddr()
* routine except that you must provide a buffer for the resulting value.
*
* EXAMPLE
* The following copies the address 0x5a000002 to the location 
* pointed to by <pInetAddr>:
* .CS
*     inet_makeaddr_b (0x5a, 2, pInetAddr);
* .CE
*
* RETURNS: N/A
*/

void inet_makeaddr_b
    (
    int netAddr,                /* network part of the inet address */
    int hostAddr,               /* host part of the inet address */
    struct in_addr *pInetAddr   /* where to return the inet address */
    )
    {
    register u_long addr;

    if (netAddr < 128)
	addr = (netAddr << IN_CLASSA_NSHIFT) | (hostAddr & IN_CLASSA_HOST);
    else if (netAddr < 65536)
	addr = (netAddr << IN_CLASSB_NSHIFT) | (hostAddr & IN_CLASSB_HOST);
    else if (netAddr < 16777216)
	addr = (netAddr << IN_CLASSC_NSHIFT) | (hostAddr & IN_CLASSC_HOST);
    else
	addr = (netAddr << IN_CLASSD_NSHIFT) | (hostAddr & IN_CLASSD_HOST);

    pInetAddr->s_addr = htonl (addr);
    }
/*******************************************************************************
*
* inet_makeaddr - form an Internet address from network and host numbers
*
* This routine constructs the Internet address from the network number and
* local host address.
*
* WARNING
* This routine is supplied for UNIX compatibility only.  Each time this
* routine is called, four bytes are allocated from memory.  Use
* inet_makeaddr_b() instead.
*
* EXAMPLE
* The following example returns the address 0x5a000002 to the structure 
* `in_addr':
* .CS
*     inet_makeaddr (0x5a, 2);
* .CE
*
* RETURNS: The network address in an `in_addr' structure.
*
* SEE ALSO: inet_makeaddr_b()
*/

struct in_addr inet_makeaddr
    (
    int netAddr,    /* network part of the address */
    int hostAddr    /* host part of the address */
    )
    {
    struct in_addr *pAddr = (struct in_addr *) malloc (sizeof (struct in_addr));

    if (pAddr != NULL)
	inet_makeaddr_b (netAddr, hostAddr, pAddr);

    return (*pAddr);
    }
/*******************************************************************************
*
* inet_netof - return the network number from an Internet address
*
* This routine extracts the network portion of an Internet address.
*
* EXAMPLE
* The following example returns 0x5a:
* .CS
*     inet_netof (0x5a000002);
* .CE
*
* RETURNS: The network portion of <inetAddress>.
*/

int inet_netof
    (
    struct in_addr inetAddress  /* inet address */
    )
    {
    register u_long i = ntohl ((u_long) inetAddress.s_addr);

    if (IN_CLASSA (i))
	return (((i)&IN_CLASSA_NET) >> IN_CLASSA_NSHIFT);
    else if (IN_CLASSB (i))
	return (((i)&IN_CLASSB_NET) >> IN_CLASSB_NSHIFT);
    else if (IN_CLASSC (i))
	return (((i)&IN_CLASSC_NET) >> IN_CLASSC_NSHIFT);
    else
	return (((i)&IN_CLASSD_NET) >> IN_CLASSD_NSHIFT);
    }
/*******************************************************************************
*
* inet_netof_string - extract the network address in dot notation
*
* This routine extracts the network Internet address from a host Internet
* address (specified in dotted decimal notation).  The routine handles 
* class A, B, and C network addresses.  The buffer <netString> should 
* be INET_ADDR_LEN bytes long.
*
* NOTE
* This is the only routine in inetLib that handles subnet masks correctly.
*
* EXAMPLE
* The following example copies "90.0.0.0" to <netString>:
* .CS
*     inet_netof_string ("90.0.0.2", netString);
* .CE
*
* RETURNS: N/A
*/

void inet_netof_string
    (
    char *inetString,   /* inet addr to extract local portion from */
    char *netString     /* net inet address to return */
    )
    {
    struct in_addr inaddrHost;
    struct in_addr inaddrNet;

    /* convert string to u_long */

    inaddrHost.s_addr = inet_addr (inetString);

    inaddrNet.s_addr = htonl ((in_netof (inaddrHost))); 

    /* convert network portion to dot notation */

    inet_ntoa_b (inaddrNet, netString);
    }
/*******************************************************************************
*
* inet_network - convert an Internet network number from string to address
*
* This routine forms a network address from an ASCII string containing
* an Internet network number.
*
* EXAMPLE
* The following example returns 0x5a:
* .CS
*     inet_network ("90");
* .CE
*
* RETURNS: The Internet address for an ASCII string, or ERROR if invalid.
*/

u_long inet_network
    (
    register char *inetString           /* string version of inet addr */
    )
    {
    register u_long val, base, n;
    register char c;
    u_long parts[4], *pp = parts;
    register int i;

again:
    val = 0;
    base = 10;

    if (*inetString == '0')
	base = 8, inetString++;
    if (*inetString == 'x' || *inetString == 'X')
	base = 16, inetString++;

    while ((c = *inetString))
	{
	if (isdigit ((int) c))
	    {
	    val = (val * base) + (c - '0');
	    inetString++;
	    continue;
	    }
	if (base == 16 && isxdigit ((int) c))
	    {
	    val = (val << 4) + (c + 10 - (islower ((int) c) ? 'a' : 'A'));
	    inetString++;
	    continue;
	    }
	break;
	}

    if (*inetString == '.')
	{
	if (pp >= parts + 4)
	    {
	    (void) errnoSet (S_inetLib_ILLEGAL_NETWORK_NUMBER);
	    return (ERROR);
	    }
	*pp++ = val, inetString++;
	goto again;
	}

    if (*inetString && !isspace ((int) *inetString))
	{
	(void) errnoSet (S_inetLib_ILLEGAL_NETWORK_NUMBER);
	return (ERROR);
	}

    *pp++ = val;
    n = pp - parts;

    if (n > 4)
	{
	(void) errnoSet (S_inetLib_ILLEGAL_NETWORK_NUMBER);
	return (ERROR);
	}

    for (val = 0, i = 0; i < n; i++)
	{
	val <<= 8;
	val |= parts[i] & 0xff;
	}

    return (val);
    }
/*******************************************************************************
*
* inet_ntoa_b - convert an network address to dot notation, store it in a buffer
*
* This routine converts an Internet address in network format to dotted
* decimal notation.
*
* This routine is identical to the UNIX inet_ntoa() routine
* except that you must provide a buffer of size INET_ADDR_LEN.
*
* EXAMPLE
* The following example copies the string "90.0.0.2" to <pString>:
* .CS
*     struct in_addr iaddr;
*      ...
*     iaddr.s_addr = 0x5a000002;
*      ...
*     inet_ntoa_b (iaddr, pString);
* .CE
*
* RETURNS: N/A
*/

void inet_ntoa_b
    (
    struct in_addr inetAddress, /* inet address */
    char *pString               /* where to return ASCII string */
    )
    {
    register char *p = (char *)&inetAddress;

#define	UC(b)	(((int)b)&0xff)
    (void) sprintf (pString, "%d.%d.%d.%d",
		    UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));
    }
/*******************************************************************************
*
* inet_ntoa - convert a network address to dotted decimal notation
*
* This routine converts an Internet address in network format to dotted
* decimal notation.
*
* WARNING
* This routine is supplied for UNIX compatibility only.  Each time this
* routine is called, 18 bytes are allocated from memory.  Use inet_ntoa_b()
* instead.
*
* EXAMPLE
* The following example returns a pointer to the string "90.0.0.2":
* .CS
*     struct in_addr iaddr;
*      ...
*     iaddr.s_addr = 0x5a000002;
*      ...
*     inet_ntoa (iaddr);
* .CE
*
* RETURNS: A pointer to the string version of an Internet address.
*
* SEE ALSO: inet_ntoa_b()
*/

char *inet_ntoa
    (
    struct in_addr inetAddress    /* inet address */
    )
    {
    FAST char *buf = (char *) malloc (INET_ADDR_LEN);

    if (buf != NULL)
	inet_ntoa_b (inetAddress, buf);

    return (buf);
    }


/*******************************************************************************
*
* inet_aton - convert a network address from dot notation, store in a structure
*
* This routine interprets an Internet address.  All the network library
* routines call this routine to interpret entries in the data bases
* that are expected to be an address.  The value returned is stored in
* network byte order in the structure provided.
*
* EXAMPLE
* The following example returns 0x5a000002 in the 's_addr' member of the 
* structure pointed to by <pinetAddr>:
* .CS
*     inet_aton ("90.0.0.2", pinetAddr);
* .CE
*
* RETURNS: OK, or ERROR if address is invalid.
*/

STATUS inet_aton 
    (
    char           * pString,     /* string containing address, dot notation */
    struct in_addr * inetAddress  /* struct in which to store address */
    )
    {
    u_long rtnAddress;
    int oldError;

    /*
     * Since the conversion routine returns the equivalent of ERROR for
     * the string "255.255.255.255", this routine detects problems by
     * a change in the global error number value. Save the original
     * setting in case no error occurs. 
     */

    oldError = errno;
    errno = 0;

    rtnAddress = inet_addr (pString);

    if (errno != S_inetLib_ILLEGAL_INTERNET_ADDRESS)
	{
	inetAddress->s_addr = rtnAddress; /* Valid address, even 0xffffffff */
        errno = oldError;

	return (OK);
        }

    return (ERROR);
    }
#endif	/* STANDALONE_AGENT */

/* INETSockAddr */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,17dec01,nel  Add include symbol for diab build.
01i,16nov99,nel  Correct type usage in INETSockAddr :: setAddr
01h,19aug99,aim  added TraceCall header
01g,16jul99,dbs  fix address-equality
01f,15jul99,dbs  add equality op
01e,23jun99,aim  fix hostAddrGet on vxWorks
01d,15jun99,aim  changed hostAddrGet to return dotted ip number
01c,05jun99,aim  added clone method
01b,03jun99,dbs  fix for OS-specifics
01a,11may99,aim  created
*/

#include "INETSockAddr.h"
#include "TraceCall.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_INETSockAddr (void)
    {
    return 0;
    }

INETSockAddr::INETSockAddr ()
    {
    TRACE_CALL;
    (void) ::memset (addr_in(), 0, size ());
    sin_family      = PF_INET;
    sin_addr.s_addr = htonl(INADDR_ANY);
    sin_port        = 0;
    }
 
INETSockAddr::INETSockAddr (int port)
    {
    TRACE_CALL;
    (void) ::memset (addr_in(), 0, size ());
    sin_family      = PF_INET;
    sin_addr.s_addr = htonl(INADDR_ANY);
    sin_port        = htons (port);
    }

INETSockAddr::INETSockAddr (unsigned long address, int port)
    {
    TRACE_CALL;
    (void) ::memset (addr_in(), 0, size ());
    sin_family      = PF_INET;
    sin_addr.s_addr = htonl (address);
    sin_port        = htons (port);
    }

INETSockAddr::INETSockAddr (const char* hostname, int port)
    {
    TRACE_CALL;
    (void) ::memset (addr_in(), 0, size ());
    sin_port = htons (port);
    setAddr (hostname);		// sets sin_family
    }

INETSockAddr::~INETSockAddr ()
    {
    TRACE_CALL;
    }

int
INETSockAddr::setAddr (const char* address)
    {
    TRACE_CALL;

    sin_family = PF_UNSPEC;
    sin_addr.s_addr = ::inet_addr (const_cast<char *> (address) );

    if (sin_addr.s_addr == (unsigned long) -1) // inet_addr failure
	{
#ifndef VXDCOM_PLATFORM_VXWORKS
	hostent* hp = ::gethostbyname (address);

	if (hp != 0)
	    {
	    (void) ::memcpy (&sin_addr.s_addr, hp->h_addr, hp->h_length);
	    sin_family = hp->h_addrtype;
	    }
#else
	// Only PF_INET supported on VxWorks
	sin_addr.s_addr = ::hostGetByName (const_cast<char*> (address));
	sin_family = PF_INET;
#endif
	}
    else
	{
	sin_family = PF_INET;
	}

    return sin_family == PF_INET;
    }

int
INETSockAddr::size () const
    {
    TRACE_CALL;
    return (sizeof (sockaddr_in));
    }

sockaddr*
INETSockAddr::addr ()
    {
    TRACE_CALL;
    return reinterpret_cast<sockaddr*> (addr_in());
    }

INETSockAddr::operator sockaddr* ()
    {
    TRACE_CALL;
    return addr ();
    }

int
INETSockAddr::family () const
    {
    TRACE_CALL;
    return PF_INET;
    }

sockaddr_in*
INETSockAddr::addr_in ()
    {
    TRACE_CALL;
    return dynamic_cast<sockaddr_in*> (this);
    }

INETSockAddr::operator sockaddr_in* ()
    {
    TRACE_CALL;
    return dynamic_cast<sockaddr_in*> (this);
    }

const sockaddr_in*
INETSockAddr::addr_in () const
    {
    TRACE_CALL;
    return static_cast<const sockaddr_in*> (this);
    }

INETSockAddr::operator const sockaddr_in* () const
    {
    TRACE_CALL;
    return static_cast<const sockaddr_in*> (this);
    }

int
INETSockAddr::portGet () const
    {
    TRACE_CALL;
    return ntohs (sin_port);
    }

int
INETSockAddr::hostAddrGet (char* hostAddr, int len) const
    {
    TRACE_CALL;

    in_addr internet_addr = sin_addr;

    if (internet_addr.s_addr == INADDR_ANY)
	{
#ifndef VXDCOM_PLATFORM_VXWORKS
	if (::gethostname (hostAddr, len) < 0)
	    return -1;

	hostent* hp = 0;

	if ((hp = ::gethostbyname (hostAddr)) < 0)
	    return -1;

	(void) ::memcpy (&internet_addr.s_addr,
			 hp->h_addr,
			 sizeof (internet_addr.s_addr));
#else
	if (::gethostname (hostAddr, len) == ERROR)
	    return -1;

	int addr = ::hostGetByName (hostAddr);

	(void) ::memcpy (&internet_addr.s_addr,
			 &addr,
			 sizeof (internet_addr.s_addr));
#endif
	}

    char* dottedName = ::inet_ntoa (internet_addr);

    if (dottedName != 0)
	::strncpy (hostAddr, dottedName, len);	

    return dottedName != 0 ? 0 : -1;
    }

int
INETSockAddr::familyNameGet (char* buf, int len) const
    {
    TRACE_CALL;
    ::strncpy (buf, "INET", len);
    return 0;
    }

int
INETSockAddr::clone (sockaddr*& buf) const
    {
    TRACE_CALL;

    buf = 0;

    sockaddr_in* sin_addr = new sockaddr_in (*addr_in ());

    if (sin_addr)
	buf = reinterpret_cast<sockaddr*> (sin_addr);

    return buf ? 0 : -1;
    }

bool INETSockAddr::operator == (const INETSockAddr& other) const
    {
    return (memcmp (static_cast<const sockaddr_in*> (this),
		    static_cast<const sockaddr_in*> (&other),
		    size ()) == 0);
    }

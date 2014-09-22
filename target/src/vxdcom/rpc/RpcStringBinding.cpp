/* RpcStringBinding.cpp - A DCE/RPC endpoints symbolic address */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,03jan02,nel  Remove OLE2T.
01k,17dec01,nel  Add include symbol for diab build.
01j,13jul01,dbs  fix up includes
01i,13mar01,nel  SPR#35873 - add extra method to check to see if an address
                 string can be resolved.
01h,21jul99,aim  fix compiler warnings
01g,21jul99,aim  all ctors use initialiseFromString
01f,16jul99,aim  fix initialiseFromString
01e,15jul99,dbs  fix return-value check of gethostname
01d,15jul99,dbs  add isLocalhost() method
01c,08jul99,dbs  add equality operator
01b,07jul99,aim  added strdup; apparently it doesn't exist on the target??
01a,06jul99,aim  created
*/

#include <stdio.h>
#include "RpcStringBinding.h"
#include "private/comMisc.h"
#include "INETSockAddr.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_RpcStringBinding (void)
    {
    return 0;
    }

RpcStringBinding::RpcStringBinding ()
  : m_ipAddress (0),
    m_portNumber (-1),
    m_protocolSeq (0),
    m_strFormat (0),
    m_strFormatNoPortNumber (0)
    {
    }

RpcStringBinding::RpcStringBinding
    (
    DUALSTRINGARRAY*    pdsa,
    int			portNumber
    )
  : m_ipAddress (0),
    m_portNumber (portNumber),
    m_protocolSeq (0),
    m_strFormat (0),
    m_strFormatNoPortNumber (0)
    {
    // Check the DUALSTRINGARRAY entries, to find one which can be
    // 'resolved' into an IP address on this machine. Win2K sometimes
    // provides hostnames as well as the IP numbers that NT usually
    // sends, so we need to scan the array to find an IP number...
    OLECHAR* pStart = pdsa->aStringArray;
    OLECHAR* pCurr = pStart;
    
    while ((pCurr - pStart) < pdsa->wNumEntries)
        {
        // Make a string-binding out of the DSA entry...
	char * pStr = new char [comWideStrLen (pCurr + 1) + 1];
	comWideToAscii (pStr, pCurr + 1, comWideStrLen (pCurr + 1) + 1);
        initialiseFromString (pStr);
	delete []pStr;
        m_protocolSeq  = *pCurr;

        // See if it can be 'resolved' into a viable IP number...
        if (! resolve ())
            {
            S_ERR (LOG_DCOM, "Could not resolve " << ipAddress ());

            // Free up the allocated strings.
            delString (m_ipAddress);
            delString (m_strFormat);
            delString (m_strFormatNoPortNumber);

            // Could not make sense of the address, so move on through
            // the array to the next entry, or the end of the array...
            while (((pCurr - pStart) < pdsa->wNumEntries) && (*pCurr))
                ++pCurr;

            ++pCurr;
            }
        else
            {
            S_INFO (LOG_DCOM, "Resolved " << ipAddress ());

            // Take the first one that is viable...
            break;
            }
        }
    // at this point the string binding may have worked or not so the calling
    // routine must make a call to resolve to determine if a valid address was
    // bound since the constructor can't /init
    }

RpcStringBinding::RpcStringBinding
    (
    const char*		binding,
    PROTSEQ		protocolSeq
    )
  : m_ipAddress (0),
    m_portNumber (-1),
    m_protocolSeq (protocolSeq),
    m_strFormat (0),
    m_strFormatNoPortNumber (0)
    {
    initialiseFromString (binding);
    }

RpcStringBinding::RpcStringBinding
    (
    const OLECHAR*	binding,
    PROTSEQ		protocolSeq
    )
  : m_ipAddress (0),
    m_portNumber (-1),
    m_protocolSeq (protocolSeq),
    m_strFormat (0),
    m_strFormatNoPortNumber (0)
    {
    char * pStr = new char [comWideStrLen (binding) + 1];
    comWideToAscii (pStr, binding, comWideStrLen (binding) + 1);
    initialiseFromString (pStr);
    delete []pStr;
    }

RpcStringBinding::RpcStringBinding
    (
    const char*		binding,
    PROTSEQ		protocolSeq,
    int			portNumber
    )
  : m_ipAddress (0),
    m_portNumber (portNumber),
    m_protocolSeq (protocolSeq),
    m_strFormat (0),
    m_strFormatNoPortNumber (0)
    {
    initialiseFromString (binding);
    }

RpcStringBinding::RpcStringBinding
    (
    const OLECHAR*	binding, 
    PROTSEQ		protocolSeq,
    int			portNumber
    )
  : m_ipAddress (0),
    m_portNumber (portNumber),
    m_protocolSeq (protocolSeq),
    m_strFormat (0),
    m_strFormatNoPortNumber (0)
    {
    if (binding)
	{
	char * pStr = new char [comWideStrLen (binding) + 1];
	comWideToAscii (pStr, binding, comWideStrLen (binding) + 1);
	initialiseFromString (pStr);
	delete []pStr;
	}
    }

RpcStringBinding::RpcStringBinding
    (
    const SockAddr&	addr,
    PROTSEQ		protocolSeq
    )
  : m_ipAddress (0),
    m_portNumber (-1),
    m_protocolSeq (protocolSeq),
    m_strFormat (0),
    m_strFormatNoPortNumber (0)
    {
    char buf [MAXHOSTNAMELEN +1];

    if (addr.hostAddrGet (buf, MAXHOSTNAMELEN) == 0)
	{
	m_ipAddress = copyString (buf);
	m_portNumber = addr.portGet ();
	}
    }

RpcStringBinding::~RpcStringBinding ()
    {
    delString (m_ipAddress);
    delString (m_strFormat);
    delString (m_strFormatNoPortNumber);
    }

RpcStringBinding::RpcStringBinding (const RpcStringBinding& other)
  : m_ipAddress (0),
    m_portNumber (-1),
    m_protocolSeq (0),
    m_strFormat (0),
    m_strFormatNoPortNumber (0)
    {
    *this = other;
    }

RpcStringBinding& RpcStringBinding::operator=(const RpcStringBinding& rhs)
    {
    if (this != &rhs)
	{
	delString (m_ipAddress);
	delString (m_strFormat);
	delString (m_strFormatNoPortNumber);

	m_ipAddress = copyString (rhs.m_ipAddress);
	m_portNumber = rhs.m_portNumber;
	m_protocolSeq = rhs.m_protocolSeq;
	m_strFormat = copyString (rhs.m_strFormat);
	m_strFormatNoPortNumber = copyString (rhs.m_strFormatNoPortNumber);
	}

    return *this;
    }

int RpcStringBinding::initialiseFromString (const char* binding)
    {
    if (binding == 0)
	return -1;

    int len = ::strlen (binding);
    char* sb = ::strchr (binding, '[');

    if (sb)			// found opening square bracket
	{
	// we copy the whole binding and then set the square bracket
	// to "NULL".  Cheap and nasty but we are only going to
	// waste at most 7 bytes.

	m_ipAddress = copyString (binding);

	// copy over the opening "["

	m_ipAddress[sb - binding] = '\0';
	
	// convert the portnumber if there is any data left in binding.

	if ((1 + (sb - binding)) < len)
	    m_portNumber = ::atoi (++sb);
	else
	    return -1;
	}
    else
	{
	// no square bracket, just copy the address.
	m_ipAddress = copyString (binding);
	}

    return 0;
    }

const char* RpcStringBinding::ipAddress () const
    {
    return m_ipAddress;
    }

int RpcStringBinding::portNumber () const
    {
    return m_portNumber;
    }

PROTSEQ RpcStringBinding::protocolSequence () const
    {
    return m_protocolSeq;
    }

const char* RpcStringBinding::formatted (bool includePortNumber) const
    {
    char*& p = includePortNumber ? m_strFormat : m_strFormatNoPortNumber;

    if (p == 0 && m_ipAddress != 0)
	{
	char binding [MAXHOSTNAMELEN]; // XXX

	int n = 0;

	n = ::sprintf (binding, "%c%s", protocolSequence (), ipAddress ());

	if (n && includePortNumber)
	    ::sprintf (&binding[n], "[%d]", portNumber ());

	p = copyString (binding);
	}

    return p;
    }

int RpcStringBinding::format
    (
    BSTR*	bstr,
    bool	includePortNumber
    ) const
    {
    USES_CONVERSION;

    const char* f = formatted (includePortNumber);

    if (f != 0)
	{
	OLECHAR * wStr = new OLECHAR [strlen (f) + 1];
	comAsciiToWide (wStr, f, strlen (f) + 1);
	*bstr = ::SysAllocString (wStr);
	delete []wStr;
	}
    else
	*bstr = 0;

    return *bstr != 0 ? 0 : -1;
    }

bool RpcStringBinding::isLocalhost () const
    {
    INETSockAddr addr (m_ipAddress);

    char hostname [MAXHOSTNAMELEN +1];

    if (::gethostname (hostname, MAXHOSTNAMELEN) < 0)
	return false;

    INETSockAddr local (hostname);
    return (local == addr);
    }


//////////////////////////////////////////////////////////////////////////
//
// RpcStringBinding::resolve - does the IP address of this string
// binding resolve
//

bool RpcStringBinding::resolve () const
    {
    // check to see if a valid address is present.
    if (!m_ipAddress)
        return false;

    // Do a very TCP/IP-specific test to see if the ipAddress can be
    // resolved. This needs to be made more protocol-independent in
    // future... 

    // Firstly, check if its a dotted-IP number...
    if (inet_addr (m_ipAddress) == (unsigned long)ERROR)
        {
        // No, so try to resolve the hostname...
#ifndef VXDCOM_PLATFORM_VXWORKS
    hostent* hp = ::gethostbyname (m_ipAddress);
    if (hp == 0)
        return false;
#else
    if (::hostGetByName (const_cast<char*> (m_ipAddress)) == ERROR)
        return false;
#endif
        }

    // Address did resolve...
    return true;
    }

bool RpcStringBinding::operator== (const RpcStringBinding& sb) const
    {
    const char* s1 = formatted ();
    const char* s2 = sb.formatted ();
    if (s1 && s2 && (strcmp (s1, s2) == 0))
        return true;
    if ((s1 == 0) && (s2 == 0))
        return true;
    return false;    
    }

bool RpcStringBinding::operator< (const RpcStringBinding& sb) const
    {
    const char* s1 = formatted ();
    const char* s2 = sb.formatted ();
    if (s1 && s2)
        return (strcmp (s1, s2) < 0);
    
    if (s1 == 0)
        return false;
    
    return true;    
    }

bool RpcStringBinding::isValid () const
    {
    return m_ipAddress != 0 && m_portNumber != -1;
    }

ostream& operator<< (ostream& os, const RpcStringBinding& obj)
    {
    const char* s = obj.formatted ();

    if (s)
	os << s;

    return os;
    }

char* RpcStringBinding::copyString (const char* s1) const
    {
    char* s2 = 0;

    if (s1)
	{
	s2 = reinterpret_cast<char*> (::malloc (::strlen (s1) + 1));

	if (s2)
	    ::strcpy (s2, s1);
	}

    return s2;
    }

void RpcStringBinding::delString (char*& s1)
    {
    if (s1)
	::free (s1);
    
    s1 = 0;
    }

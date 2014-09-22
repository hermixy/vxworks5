/* SockEP */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01n,17dec01,nel  Add include symbol for diab build.
01m,13jul01,dbs  fix up includes
01l,01feb00,nel  Correct casting for Linux debug build
01k,19jan00,nel  Modifications for Linux debug build
01j,19aug99,aim  change assert to VXDCOM_ASSERT
01i,10aug99,aim  fix hostAddrGet and peerAddrGet on target
01h,30jul99,aim  add keepAliveSet method
01g,13jul99,aim  syslog api changes
01f,12jul99,aim  removed debug
01e,30jun99,aim  close only calls handleGet once
01d,28jun99,aim  close handle if optReuseAddrSet fails
01c,17jun99,aim  changed assert to assert
01b,04jun99,aim  added syslog messages
01a,10may99,aim  created
*/

#include "ReactorTypes.h"
#include "SockEP.h"
#include "TraceCall.h"
#include "private/comMisc.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_SockEP (void)
    {
    return 0;
    }

SockEP::SockEP ()
    {
    TRACE_CALL;
#ifdef VXDCOM_PLATFORM_WIN32
    COM_ASSERT (winsockInitialised);
#endif
    }

SockEP::~SockEP ()
    {
    TRACE_CALL;

    if (handleIsValid ())
	close ();
    }

SockEP::SockEP
    (
    int type,
    int protocol_family,
    int protocol,
    int reuseAddr
    )
    {
    TRACE_CALL;
    open (type, protocol_family, protocol, reuseAddr);
    }

int
SockEP::optGet
    (
    int	level,
    int	option,
    void* optval,
    int* optlen
    ) const
    {
    TRACE_CALL;

#ifdef VXDCOM_PLATFORM_LINUX
    return ::getsockopt (handleGet (),
			 level,
			 option,
			 (char *) optval, 
			 reinterpret_cast <unsigned int *>(optlen));
#else
    return ::getsockopt (handleGet (),
			 level,
			 option,
			 (char *) optval, 
			 optlen);
#endif
    }

int
SockEP::optSet
    (
    int		level,
    int		option,
    void*	optval,
    int		optlen
    ) const
    {
    TRACE_CALL;

    return ::setsockopt (handleGet (),
			 level,
			 option,
			 (char *) optval, 
			 optlen);
    }

int
SockEP::optReuseAddrSet ()
    {
    TRACE_CALL;
    int one = 1;
    return optSet (SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    }

int
SockEP::optKeepAliveSet ()
    {
    TRACE_CALL;
    int one = 1;
    return optSet (SOL_SOCKET, SO_KEEPALIVE, &one, sizeof one);
    }

int
SockEP::open
    (
    int type,
    int protocol_family, 
    int protocol,
    int reuseAddr,
    int keepalive
    )
    {
    TRACE_CALL;

    int fd = ::socket (protocol_family, type, protocol);

    if (fd != INVALID_REACTOR_HANDLE)
	{
	handleSet (fd);

	if (protocol_family != PF_UNIX && reuseAddr)
	    if (optReuseAddrSet () < 0)
		{
		close ();
		fd = -1;
		}

	if (keepalive)
	    if (optKeepAliveSet () < 0)
		{
		close ();
		fd = -1;
		}
	}

    return fd;
    }

int
SockEP::close ()
    {
    TRACE_CALL;

    int result = -1;
    REACTOR_HANDLE handle = handleGet ();

    if (handle != INVALID_REACTOR_HANDLE)
	{
#ifdef VXDCOM_PLATFORM_WIN32
	result = ::closesocket (handle);
#else
	// result = ::shutdown (handleGet (), 2);
	result = ::close (handle);
#endif
	}

    handleSet (INVALID_REACTOR_HANDLE);

    return result;
    }

int
SockEP::hostAddrGet (SockAddr& sa) const
    {
    TRACE_CALL;

    int len = sa.size ();
    sockaddr *addr = sa.addr ();

    if (handleInvalid ())
	return -1;

#if defined(VXDCOM_PLATFORM_VXWORKS)
    if (::getsockname (handleGet (), addr, &len) == ERROR)
	return -1;
#elif defined(VXDCOM_PLATFORM_SOLARIS)
    if (::getsockname (handleGet (), addr, &len) < 0)
	return -1;
#elif defined(VXDCOM_PLATFORM_LINUX)
    if (::getsockname (handleGet (),                                                                   addr, 
                       reinterpret_cast <socklen_t *> (&len)) < 0)
	return -1;
#else
    return -1;
#endif

    COM_ASSERT (len == sa.size());

    return 0;
    }

int
SockEP::peerAddrGet (SockAddr& sa) const
    {
    TRACE_CALL;

    int len = sa.size ();
    sockaddr *addr = sa.addr ();

    if (handleInvalid ())
	return -1;
	
#if defined(VXDCOM_PLATFORM_VXWORKS)
    if (::getpeername (handleGet (), addr, &len) == ERROR)
	return -1;
#elif defined(VXDCOM_PLATFORM_SOLARIS)
    if (::getpeername (handleGet (), addr, &len) < 0)
	return -1;
#elif defined(VXDCOM_PLATFORM_LINUX)
    if (::getpeername (handleGet (), 
                       addr, 
                       reinterpret_cast <socklen_t *> (&len)) < 0)
	return -1;
#else
    return -1;
#endif

    COM_ASSERT (len == sa.size());

    return 0;
    }

ostream& 
operator<< (ostream& os, const SockEP& s)
    {
    TRACE_CALL;
    os << "SockEP(" << s.handleGet () << ")";
    return os;
    }

#ifdef VXDCOM_PLATFORM_WIN32
//////////////////////////////////////////////////////////////////////////
//
// winsockInit -- Win32-specific function to initialise the TCP stack.
//

static int winsockInit ()
    {
    WORD wVersionRequested = MAKEWORD (1, 1);
    WSADATA wsaData; 
 
    int err = WSAStartup (wVersionRequested, &wsaData);

    if (err)
        {
        // Couldn't get useable WINSOCK version...
        return 0;
        }
    else
        {
        // Loaded WINSOCK - check for correct version...
        if (LOBYTE (wsaData.wVersion) != 1 ||
            HIBYTE (wsaData.wVersion) != 1)
            {
            // Not right version...
            WSACleanup (); 
            return 0;
            }
        }

    return 1;
    }

static int winsockInitialised = winsockInit ();

#endif // VXDCOM_PLATFORM_WIN32
 

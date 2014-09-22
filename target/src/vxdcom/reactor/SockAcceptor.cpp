/* SockAcceptor */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,17dec01,nel  Add include symbol for diab build.
01e,01feb00,nel  Correct casting for Linux debug build
01d,19aug99,aim  change assert to VXDCOM_ASSERT
01c,18aug99,aim  remove default parameter on accept
01b,05jun99,aim  removed hardcoded INETSockAddr assumptions
01a,11may99,aim  created
*/

#include "stdio.h"
#include "errno.h"
#include "SockAcceptor.h"
#include "TraceCall.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_SockAcceptor (void)
    {
    return 0;
    }

SockAcceptor::SockAcceptor ()
    {
    TRACE_CALL;
    }

SockAcceptor::~SockAcceptor ()
    {
    TRACE_CALL;
    }

SockAcceptor::SockAcceptor
    (
    const SockAddr&	sa,
    int			reuseAddr, 
    int			protocolFamily,
    int			backlog, 
    int			protocol
    )
    {
    TRACE_CALL;
    open (sa, reuseAddr, protocolFamily, backlog, protocol);
    }

int
SockAcceptor::open
    (
    const SockAddr&	sa,
    int			reuseAddr,
    int			protocolFamily, 
    int			backlog, 
    int			protocol
    )
    {
    TRACE_CALL;

    if (SockEP::open (SOCK_STREAM, protocolFamily, protocol, reuseAddr) != -1)
	{
	sockaddr* laddr = 0;

	if (sa.clone (laddr) != -1)
	    {
	    size_t size = sa.size ();

	    if (::bind (handleGet (), laddr, size) < 0)
		close ();
	    else if (::listen (handleGet (), backlog) < 0)
		close ();
	    }

	delete laddr;
	}
    
    return handleInvalid () ? -1 : 0;
    }

int
SockAcceptor::accept
    (
    SockStream&		newStream,
    SockAddr&		peerAddr
    )
    {
    TRACE_CALL;

    REACTOR_HANDLE newHandle = INVALID_REACTOR_HANDLE;

    int len = peerAddr.size ();
	
    do // blocking accept
        {
#ifdef VXDCOM_PLATFORM_LINUX
        newHandle = ::accept (handleGet (), 
                              peerAddr, 
                              reinterpret_cast <socklen_t *> (&len));
#else
        newHandle = ::accept (handleGet (), peerAddr, &len);
#endif
        }
    while (newHandle < 0 && errno == EINTR);

    if (newHandle != INVALID_REACTOR_HANDLE)
	newStream.handleSet (newHandle);

    return newHandle == INVALID_REACTOR_HANDLE ? -1 : 0;
    }

/* SockConnector */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,17dec01,nel  Add include symbol for diab build.
01a,05jun99,aim  created
*/

#include "SockConnector.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_SockConnector (void)
    {
    return 0;
    }

SockConnector::~SockConnector ()
    {
    TRACE_CALL;
    }

SockConnector::SockConnector ()
    {
    TRACE_CALL;
    }

SockConnector::SockConnector
    (
    SockStream& stream,
    const SockAddr& sockAddr,
    int reuseAddr,
    int protocolFamily,
    int protocol
    )
    {
    TRACE_CALL;
    connect (stream, sockAddr, reuseAddr, protocolFamily, protocol);
    }

int
SockConnector::connect
    (
    SockStream& stream,
    const SockAddr& sockAddr,
    int reuseAddr,
    int protocolFamily,
    int protocol
    )
    {
    TRACE_CALL;

    if (stream.handleInvalid ())
	{
	// open on a new socket descriptor

	if (stream.open (SOCK_STREAM,
			 protocolFamily,
			 protocol,
			 reuseAddr) < 0)
	    return -1;		// cannot open
	}

    sockaddr* remoteAddr = 0;

    if (sockAddr.clone (remoteAddr) != -1)
	{
	size_t size = sockAddr.size ();

	REACTOR_HANDLE handle = stream.handleGet ();

	if (::connect (handle, remoteAddr, size) < 0)
	    stream.close ();
#if 0
	if (::bind (handle, remoteAddr, size) < 0)
	    stream.close ();
	else if (::connect (handle, remoteAddr, size) < 0)
	    stream.close ();
#endif
	}

    delete remoteAddr;

    return stream.handleInvalid () ? -1 : 0;
    }

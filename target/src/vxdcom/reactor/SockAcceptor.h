/* SockAcceptor */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,19aug99,aim  made dtor virtual
01b,18aug99,aim  remove default parameter on accept
01a,11may99,aim  created
*/

#ifndef __INCSockAcceptor_h
#define __INCSockAcceptor_h

#include "ReactorTypes.h"
#include "SockEP.h"
#include "SockAddr.h"
#include "SockStream.h"
#include "iostream"

class SockAcceptor : public SockEP
    {
  public:
    virtual ~SockAcceptor ();

    SockAcceptor ();

    SockAcceptor
	(
	const SockAddr& sa,
	int		reuseAddr      = 0,
	int		protocolFamily = PF_INET,
	int		backlog        = 5,
	int		protocol       = 0
	);

    // Initiate a passive mode socket.

    int open
	(
	const SockAddr& sa,
	int		reuseAddr      = 0,
	int		protocolFamily = PF_INET,
	int		backlog        = 5,
	int		protocol       = 0
	);
    // Initiate a passive mode socket.

    int accept
	(
	SockStream&	newStream,
	SockAddr&	peerAddr
	);
    // Passive connection acceptance method.

    // Meta-type info

    typedef INETSockAddr PEER_ADDR;
    typedef SockStream PEER_STREAM;

    friend ostream& operator<< (ostream& os, const SockAcceptor&);
    };

#endif // __INCSockAcceptor_h

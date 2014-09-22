/* SockEP */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,11aug99,aim  keepalive no longer the default
01b,30jul99,aim  add keepAliveSet method
01a,10may99,aim  created
*/

#ifndef __INCSockEP_h_
#define __INCSockEP_h_

#include "ReactorHandle.h"
#include "SockAddr.h"
#include "iostream"

class SockEP : public ReactorHandle
    {
  public:

    virtual ~SockEP ();
      
    int optSet
	(
	int	level,
	int	option,
	void*	optval,
	int	optlen
	) const;
    // Wrapper around the setsockopt() system call.

    int optGet
	(
	int	level,
	int	option,
	void*	optval,
	int*	optlen
	) const;
    // Wrapper around the getsockopt() system call.

    int optReuseAddrSet ();
    int optKeepAliveSet ();

    int open
	(
	int type,
	int protocolFamily,
	int protocol,
	int reuseAddr,
	int keepAliveSet = 0
	);
    // Invokes the <socket> system call

    int close (void);
    // Close down the socket.

    int hostAddrGet (SockAddr& sa) const;
    int peerAddrGet (SockAddr& sa) const;

    friend ostream& operator<< (ostream& os, const SockEP&);

  protected:

    SockEP ();
    SockEP (const SockAddr& sa);

    SockEP
	(
	int type,
	int protocolFamily = PF_INET,
	int protocol = 0,
	int reuseAddr = 0
	);
    };

#endif // __INCSockEP_h

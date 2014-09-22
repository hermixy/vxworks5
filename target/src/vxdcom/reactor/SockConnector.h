/* SockConnector */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,05jun99,aim  created
*/

#ifndef __INCSockConnector_h
#define __INCSockConnector_h

#include "ReactorTypes.h"
#include "SockStream.h"
#include <iostream>

class SockConnector
    {
  public:
    virtual ~SockConnector ();

    SockConnector ();

    SockConnector (SockStream& stream,
		   const SockAddr& sockAddr,
		   int reuseAddr = 0,
		   int protocolFamily = PF_INET,
		   int protocol = 0);

    int connect (SockStream& stream,
		 const SockAddr& sockAddr,
		 int reuseAddr = 0,
		 int protocolFamily = PF_INET,
		 int protocol = 0);

    // Meta-type info for Connector template
    typedef INETSockAddr PEER_ADDR;
    typedef SockStream PEER_STREAM;

    friend ostream& operator<< (ostream& os, const SockConnector&);
    };

#endif // __INCSockConnector_h

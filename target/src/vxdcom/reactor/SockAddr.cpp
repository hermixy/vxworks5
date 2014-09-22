/* SockAddr */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,17dec01,nel  Add include symbol for diab build.
01d,18jun99,aim  removed family name from ostream operator
01c,15jun99,aim  make hostAddrGet() and family() reentrant
01b,05jun99,aim  added clone method
01a,10may99,aim  created
*/

#include <SockEP.h>
#include <SockAddr.h>

/* Include symbol for diab */
extern "C" int include_vxdcom_SockAddr (void)
    {
    return 0;
    }

SockAddr::SockAddr ()
    {
    TRACE_CALL;
    }

SockAddr::~SockAddr ()
    {
    TRACE_CALL;
    }

ostream& operator<< (ostream& os, const SockAddr& sa)
    {
    TRACE_CALL;

    const int len = MAXHOSTNAMELEN;
    char buf [MAXHOSTNAMELEN +1];

    int hostAddr = sa.hostAddrGet (buf, len);

    os << (hostAddr < 0 ? "<nil>" : buf)
       << ":"
       << sa.portGet ();

    return os;
    }

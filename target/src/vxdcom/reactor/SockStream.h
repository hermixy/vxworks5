/* SockStream */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,02oct01,nel  Add debug hooks for dcomShow.
01a,11may99,aim  created
*/

#ifndef __INCSockStream_h
#define __INCSockStream_h

#include <SockIO.h>
#include <INETSockAddr.h>
#include "private/DebugHooks.h"

class SockStream : public SockIO
    {
  public:

    virtual ~SockStream ();

    SockStream ();

    // Selectively close endpoints.
    int readerClose ();
    int writerClose ();

    int close ();
    // calls ::shutdown (n, 2) first.


    void processDebugOutput
        (
	void (*pHook)(const BYTE *, DWORD, const char *, int, int),
	BYTE * pBuf,
	DWORD length
	)
	{
	if (CHECKHOOK(pHook))
	    {
	    INETSockAddr peerAddr;
	    INETSockAddr hostAddr;
	    char hostName [32] = { 0, };

	    if (peerAddrGet (peerAddr) >= 0)
		{
		if (peerAddr.hostAddrGet (hostName, sizeof (hostName)) < 0)
		    {
		    memset (hostName, '\0', sizeof (hostName));
		    }
		}
	    hostAddrGet (hostAddr);
	    HOOK(pHook)((BYTE *)pBuf, 
		        (DWORD)length, 
			hostName, 
			hostAddr.portGet (),
			peerAddr.portGet ());
	    }
        }

    // Meta-type info
    typedef INETSockAddr PEER_ADDR;
    };

#endif // __INCSockStream_h


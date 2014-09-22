/* SvcHandler */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,19aug99,aim  changed assert to vxdcomAssert
01b,07jun99,aim  changed peer to stream
01a,12may99,aim  created
*/

#ifndef __INCSvcHandler_h
#define __INCSvcHandler_h

#include "ReactorTypes.h"
#include "EventHandler.h"
#include "TraceCall.h"

class Reactor;

template <class PEER_STREAM>
class SvcHandler : public EventHandler
    {
  public:
    virtual ~SvcHandler ();

    SvcHandler (Reactor*);
    //SvcHandler (Reactor* = Reactor::instance ());

    virtual int open (void* = 0);
    // Activate the client REACTOR_HANDLEr (called by Acceptor/Connector).

    virtual int close (u_long flags = 0);
    // Object termination hook.

    virtual int handleClose (REACTOR_HANDLE = INVALID_REACTOR_HANDLE,
			     REACTOR_EVENT_MASK =
			     EventHandler::ALL_EVENTS_MASK);
    // Perform termination activities on the SvcHandler.  The default
    // behavior is to close down the <peer> and to <destroy> this
    // object.  If you don't want this behavior make sure you override
    // this method.

    virtual REACTOR_HANDLE handleGet () const;
    // Get the underlying REACTOR_HANDLE associated with the <peer>.
    
    virtual REACTOR_HANDLE handleSet (REACTOR_HANDLE);
    // Set the underlying REACTOR_HANDLE associated with the <peer>.

    virtual void destroy ();
    // Call this to free up dynamically allocated <SvcHandler>s.

    void shutdown ();
    // Close down the descriptor and unregister from the Reactor

    PEER_STREAM& stream ();
    // Returns the underlying stream.  Used by <Acceptor::accept> and
    // <Connector::connect> factories.

  private:
    int m_closing;
    PEER_STREAM m_stream;
    };

#include <SvcHandler.tcc>

#endif // __INCSvcHandler_h

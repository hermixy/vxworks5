/* Acceptor */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,18aug99,aim  SockAcceptor API changes
01b,05jun99,aim  removed unnecessary headers
01a,12may99,aim  created
*/

#ifndef __INCAcceptor_h
#define __INCAcceptor_h

#include "ReactorTypes.h"
#include "EventHandler.h"

class Reactor;

template <class SVC_HANDLER, PEER_ACCEPTOR_1>
class Acceptor : public EventHandler
    {
  public:
    virtual ~Acceptor ();

    Acceptor (Reactor*);
    // XXX Acceptor (Reactor* = Reactor::instance ());
    // "Do-nothing" constructor.

    Acceptor (const PEER_ACCEPTOR_ADDR& addr,
	      Reactor* reactor,
	      // XXX Reactor* = Reactor::instance (),
	      int reuseAddr = 1);

    int open (const PEER_ACCEPTOR_ADDR& addr,
	      // XXX Reactor* = Reactor::instance (),
	      Reactor* reactor,
	      int reuseAddr = 1);

    virtual operator PEER_ACCEPTOR& () const;
    // Return the underlying PEER_ACCEPTOR object.

    virtual PEER_ACCEPTOR& acceptor () const;
    // Return the underlying PEER_ACCEPTOR object.

    virtual REACTOR_HANDLE handleGet () const;
    // Returns the listening acceptor's <REACTOR_HANDLE>.

    virtual int close ();
    // Close down the Acceptor

    // Demultiplexing hooks.

    virtual int handleClose (REACTOR_HANDLE = INVALID_REACTOR_HANDLE,
			     REACTOR_EVENT_MASK =
			     EventHandler::ALL_EVENTS_MASK);

    // Perform termination activities when <this> is removed from the
    // <reactor>.

    virtual int handleInput (REACTOR_HANDLE);
    // Accepts all pending connections from clients, and creates and
    // activates SVC_HANDLER.

    virtual int hostAddrGet (PEER_ACCEPTOR_ADDR&);

  protected:

    Acceptor ();

    // bridging methods
    virtual int newSvcHandler (SVC_HANDLER *&);
    virtual int acceptSvcHandler (SVC_HANDLER *);

  private:

    PEER_ACCEPTOR m_acceptor;
    };

#include "Acceptor.tcc"

#endif // __INCAcceptor_h

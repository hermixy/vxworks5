/* Connector - Encapsulates a Socket connection */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,05jun99,aim  created
*/

#ifndef __INCConnector_h
#define __INCConnector_h

#include "ReactorTypes.h"
#include "EventHandler.h"

class Reactor;

template <class SVC_HANDLER, PEER_CONNECTOR_1>
class Connector : public EventHandler
    {
  public:
    virtual ~Connector ();
    Connector (Reactor *r);
      
    //virtual int open (Reactor *r = Reactor::instance ());
    virtual int open (Reactor *r);

    // Connection establishment methods.
    virtual int connect
	(
	const PEER_CONNECTOR_ADDR& remoteAddr,
	SVC_HANDLER*& svcHandler = 0
	);

    virtual int close ();
    // Close down the Connector

  protected:
    Connector ();		// ensure Connector is an ABC.

    virtual int newSvcHandler (SVC_HANDLER*& svcHandler);
    // Bridge method for creating a SVC_HANDLER.  The default is to
    // create a new SVC_HANDLER only if SVC_HANDLER == 0.

    virtual int connectSvcHandler
	(
	const PEER_CONNECTOR_ADDR& remoteAddr,
	SVC_HANDLER*& svcHandler = 0
	);

    // Bridge method for connecting the <SvcHandler> to the
    // <remote_addr>.  The default behavior delegates to the
    // <PEER_CONNECTOR::connect>.

    virtual int activateSvcHandler (SVC_HANDLER* svcHandler);
    // Bridge method for activating a <svc_handler> with the appropriate
    // concurrency strategy.  The default behavior of this method is to
    // activate the SVC_HANDLER by calling its open() method (which
    // allows the SVC_HANDLER to define its own concurrency strategy).
    // However, subclasses can override this strategy to do more
    // sophisticated concurrency activations (such as creating the
    // SVC_HANDLER as an "active object" via multi-threading or
    // multi-processing).

    // = Demultiplexing hooks.
    virtual int handleClose
	(
	REACTOR_HANDLE = INVALID_REACTOR_HANDLE,
	REACTOR_EVENT_MASK = EventHandler::ALL_EVENTS_MASK
	);

  private:
    PEER_CONNECTOR m_connector;
    // This is the concrete connector factory (it keeps no state so the
    // <Connector> is reentrant).

    bool m_closing;

    // unsupported
    Connector (const Connector& other);
    Connector& operator= (const Connector& rhs);
    };

#include "Connector.tcc"	// template implementations

#endif // __INCConnector_h

/* Connector - Encapsulates a Socket connection */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,05jun99,aim  created
*/

#include "Connector.h"

#ifndef __INCConnector_tcc_
#define __INCConnector_tcc_

template <class SVC_HANDLER, PEER_CONNECTOR_1>
Connector<SVC_HANDLER, PEER_CONNECTOR_2>::Connector (Reactor* r)
  : m_connector (),
    m_closing (false)
    {
    TRACE_CALL;
    reactorSet (r);
    }

template <class SVC_HANDLER, PEER_CONNECTOR_1>
Connector<SVC_HANDLER, PEER_CONNECTOR_2>::~Connector ()
    {
    handleClose ();
    TRACE_CALL;
    }

template <class SVC_HANDLER, PEER_CONNECTOR_1> int
Connector<SVC_HANDLER, PEER_CONNECTOR_2>::handleClose
    (
    REACTOR_HANDLE,
    REACTOR_EVENT_MASK mask
    )
    {
    TRACE_CALL;
    m_closing = true;
    return 0;
    }

template <class SVC_HANDLER, PEER_CONNECTOR_1> int
Connector<SVC_HANDLER, PEER_CONNECTOR_2>::connectSvcHandler
    (
    const PEER_CONNECTOR_ADDR& remoteAddr,
    SVC_HANDLER*& svcHandler
    )
    {
    TRACE_CALL;

    if (newSvcHandler (svcHandler) < 0)
	return -1;

    if (m_connector.connect (svcHandler->stream (), remoteAddr) < 0)
	return -1;

    return activateSvcHandler (svcHandler);
    }

template <class SVC_HANDLER, PEER_CONNECTOR_1> int
Connector<SVC_HANDLER, PEER_CONNECTOR_2>::newSvcHandler
    (
    SVC_HANDLER*& svcHandler
    )
    {
    TRACE_CALL;

    if (svcHandler == 0)
	svcHandler = new SVC_HANDLER(reactorGet ());

    return svcHandler ? 0 : -1;
}

template <class SVC_HANDLER, PEER_CONNECTOR_1> int
Connector<SVC_HANDLER, PEER_CONNECTOR_2>::activateSvcHandler
    (
    SVC_HANDLER* svcHandler
    )
    {
    TRACE_CALL;

    if (svcHandler->open (this) < 0)
	svcHandler->close ();	// avoid fd leaks

    return svcHandler->stream().handleInvalid () ? -1 : 0;
    }

template <class SVC_HANDLER, PEER_CONNECTOR_1> int
Connector<SVC_HANDLER, PEER_CONNECTOR_2>::open (Reactor *r)
    {
    TRACE_CALL;
    reactorSet (r);
    m_closing = false;
    return 0;
    }

template <class SVC_HANDLER, PEER_CONNECTOR_1> int
Connector<SVC_HANDLER, PEER_CONNECTOR_2>::connect
    (
    const PEER_CONNECTOR_ADDR& remoteAddr,
    SVC_HANDLER*& svcHandler
    )
    {
    TRACE_CALL;
    return connectSvcHandler (remoteAddr, svcHandler);
    }

template <class SVC_HANDLER, PEER_CONNECTOR_1> int
Connector<SVC_HANDLER, PEER_CONNECTOR_2>::close ()
    {
    TRACE_CALL;
    return handleClose ();
    }

#endif __INCConnector_tcc_

/* SvcHandler - does stuff */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,13jul01,dbs  fix up includes
01c,19aug99,aim  changed assert to vxdcomAssert
01b,07jun99,aim  changed peer to stream
01a,12May99,aim  created
*/

#include "SvcHandler.h"
#include "private/comMisc.h"

template <class PEER_STREAM> void
SvcHandler<PEER_STREAM>::destroy ()
    {
    TRACE_CALL;

    if (m_closing == 0)
	{
	// Will call the destructor, which automatically calls
	// <shutdown>.  Note that if we are *not* allocated dynamically
	// then the destructor will call <shutdown> automatically when
	// it gets run during cleanup.
	delete this;
	}
    }

template <class PEER_STREAM>
SvcHandler<PEER_STREAM>::SvcHandler (Reactor *reactor)
  : m_closing (0),
    m_stream ()
    {
    TRACE_CALL;
    reactorSet (reactor);
    }

template <class PEER_STREAM> int
SvcHandler<PEER_STREAM>::open (void*)
    {
    TRACE_CALL;
    COM_ASSERT (reactorGet ());
    return reactorGet()->handlerAdd (this, EventHandler::READ_MASK);
    }

template <class PEER_STREAM> int
SvcHandler<PEER_STREAM>::close (u_long)
    {
    TRACE_CALL;
    return handleClose ();
    }

template <class PEER_STREAM> void
SvcHandler<PEER_STREAM>::shutdown ()
    {
    TRACE_CALL;

    COM_ASSERT (reactorGet ());

    reactorGet()->handlerRemove (this,
				 EventHandler::ALL_EVENTS_MASK |
				 EventHandler::DONT_CALL);

    m_stream.close();
    }

template <class PEER_STREAM> REACTOR_HANDLE
SvcHandler<PEER_STREAM>::handleGet () const
    {
    TRACE_CALL;
    return m_stream.handleGet ();
    }

template <class PEER_STREAM> REACTOR_HANDLE
SvcHandler<PEER_STREAM>::handleSet (REACTOR_HANDLE h)
    {
    TRACE_CALL;
    return m_stream.handleSet (h);
    }

template <class PEER_STREAM>
SvcHandler<PEER_STREAM>::~SvcHandler ()
    {
    TRACE_CALL;

    if (m_closing == 0)
	{
	m_closing = 1;
	shutdown ();
	}
    }

template <class PEER_STREAM> int
SvcHandler<PEER_STREAM>::handleClose (REACTOR_HANDLE, REACTOR_EVENT_MASK)
    {
    TRACE_CALL;
    destroy ();
    return 0;
    }

template <class PEER_STREAM> PEER_STREAM&
SvcHandler<PEER_STREAM>::stream ()
    {
    TRACE_CALL;
    return m_stream;
    }

/* EventHandler */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01e,17dec01,nel  Add include symbol for diab.
01d,13jul01,dbs  fix up includes
01c,19aug99,aim  change assert to VXDCOM_ASSERT
01b,08jul99,aim  added timers
01a,07may99,aim  created
*/

#include "EventHandler.h"
#include "TraceCall.h"
#include "private/comMisc.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_EventHandler (void)
    {
    return 0;
    }

EventHandler::EventHandler ()
  : m_reactor (0)
    {
    TRACE_CALL;
    }

EventHandler::~EventHandler () 
    {
    TRACE_CALL;
    }

REACTOR_HANDLE
EventHandler::handleGet () const
    {
    TRACE_CALL;
    return INVALID_REACTOR_HANDLE;
    }

REACTOR_HANDLE
EventHandler::handleSet (REACTOR_HANDLE)
    {
    TRACE_CALL;
    return INVALID_REACTOR_HANDLE;
    }

int
EventHandler::handleClose (REACTOR_HANDLE, REACTOR_EVENT_MASK)
    {
    TRACE_CALL;
    return INVALID_REACTOR_HANDLE;
    }

int
EventHandler::handleInput (REACTOR_HANDLE)
    {
    TRACE_CALL;
    return INVALID_REACTOR_HANDLE;
    }

int
EventHandler::handleOutput (REACTOR_HANDLE)
    {
    TRACE_CALL;
    return INVALID_REACTOR_HANDLE;
    }

int
EventHandler::handleException (REACTOR_HANDLE)
    {
    TRACE_CALL;
    return INVALID_REACTOR_HANDLE;
    }

int
EventHandler::handleTimeout
    (
    const TimeValue&
    )
    {
    TRACE_CALL;
    return INVALID_REACTOR_HANDLE;
    }

Reactor*
EventHandler::reactorSet (Reactor *reactor)
    {
    TRACE_CALL;
    COM_ASSERT (reactor);
    return m_reactor = reactor;
    }

Reactor*
EventHandler::reactorGet () const
    {
    TRACE_CALL;
    return m_reactor;
    }


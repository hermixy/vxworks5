/* ReactorHandle */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,17dec01,nel  Add include symbol for diab.
01b,28jun99,aim  added handleValid
01a,10may99,aim  created
*/

#include <ReactorHandle.h>

/* Include symbol for diab */
extern "C" int include_vxdcom_ReactorHandle (void)
    {
    return 0;
    }

ReactorHandle::ReactorHandle ()
  : m_reactorHandle (INVALID_REACTOR_HANDLE)
    {
    TRACE_CALL;
    }

ReactorHandle::~ReactorHandle ()
    {
    TRACE_CALL;
    m_reactorHandle = INVALID_REACTOR_HANDLE;
    }

REACTOR_HANDLE
ReactorHandle::handleGet () const
    {
    TRACE_CALL;
    return m_reactorHandle;
    }

REACTOR_HANDLE
ReactorHandle::handleSet (REACTOR_HANDLE handle)
    {
    TRACE_CALL;
    return m_reactorHandle = handle;
    }

bool
ReactorHandle::handleInvalid () const
    {
    TRACE_CALL;
    return m_reactorHandle == INVALID_REACTOR_HANDLE;
    }

bool
ReactorHandle::handleIsValid () const
    {
    TRACE_CALL;
    return m_reactorHandle != INVALID_REACTOR_HANDLE;
    }

ostream& 
operator<< (ostream& os, const ReactorHandle& h)
    {
    TRACE_CALL;
    os << h.handleGet ();
    return os;
    }

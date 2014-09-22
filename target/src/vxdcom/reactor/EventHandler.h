/* EventHandler - Base class for Reactor events */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,07may99,aim  created
*/

#ifndef __INCEventHandler_h
#define __INCEventHandler_h

#include "ReactorTypes.h"
#include "TimeValue.h"
#include "iostream"

class Reactor;

class EventHandler
    {
  public:

    enum {
	NULL_MASK       = 0,
	READ_MASK       = (1 << 0),
	ACCEPT_MASK     = (1 << 0),
	WRITE_MASK      = (1 << 1),
	CONNECT_MASK    = (1 << 3),
	EXCEPT_MASK     = (1 << 4),
	ALL_EVENTS_MASK = READ_MASK|WRITE_MASK|ACCEPT_MASK|CONNECT_MASK,
	DONT_CALL       = (1 << 16)
    };

    virtual ~EventHandler ();

    virtual REACTOR_HANDLE handleGet () const;
    // Get the I/O REACTOR_HANDLE.

    virtual REACTOR_HANDLE handleSet (REACTOR_HANDLE);
    // Set the I/O REACTOR_HANDLE.

    virtual int handleInput (REACTOR_HANDLE h);
    // Called when input events occur (e.g., connection or data).

    virtual int handleOutput (REACTOR_HANDLE h);
    // Called when output events are possible.  (e.g., flow control
    // abates).

    virtual int handleClose
	(
	REACTOR_HANDLE = INVALID_REACTOR_HANDLE,
	REACTOR_EVENT_MASK = ALL_EVENTS_MASK	
	);

    virtual int handleException (REACTOR_HANDLE);
    virtual int handleTimeout (const TimeValue& timeValue);

    // Called when timer expires.

    virtual Reactor* reactorSet (Reactor *reactor);
    virtual Reactor* reactorGet () const;

    friend ostream& operator<< (ostream& os, const EventHandler&);

  protected:

    EventHandler ();
    // Force EventHandler to be an abstract base class.

    Reactor* m_reactor;
    };
 
#endif // __INCEventHandler_h

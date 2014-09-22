/* Reactor.h - I/O Multiplexer */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01k,13jul01,dbs  fix up includes
01j,16nov99,nel  Mod to get round compiler differences between T2 and T3 in
                 timerAdd
01i,12aug99,aim  MT fixes
01h,09aug99,aim  change wakeup behaviour
01g,02aug99,aim  added wakeup event handler
01f,21jul99,aim  changed select parameters
01e,12jul99,aim  fix returned nHandles in select
01d,08jul99,aim  added timers
01c,07jul99,aim  added critical section protection
01b,29jun99,aim  added handler2handle map
01a,07may99,aim  created
*/

#ifndef __INCReactor_h
#define __INCReactor_h

#include "ReactorTypes.h"
#include "EventHandler.h"
#include "HandleSet.h"
#include "TimeValue.h"
#include "private/comMisc.h"
#include "private/comStl.h"
#include "SockStream.h"
#include <iostream>

class Reactor
    {
  public:

    class WakeupHandler : public EventHandler
	{
      public:
	WakeupHandler (Reactor*);

	virtual int handleInput (REACTOR_HANDLE h);
	virtual REACTOR_HANDLE handleSet (REACTOR_HANDLE);
	virtual REACTOR_HANDLE handleGet () const;
	virtual int handleTimeout (const TimeValue&);

	int reactorWakeup ();

      private:

	bool		m_wakeupPending;
	VxMutex		m_wakeupPendingLock;
	REACTOR_HANDLE	m_handles[2];
	};

    virtual ~Reactor ();
    Reactor ();

    void close ();

    virtual int run ();
    virtual int eventLoopRun ();
    virtual void eventLoopEnd ();
    virtual bool eventLoopDone ();

    virtual int handleEvents ();

    virtual int handlerAdd
	(
	EventHandler*,
	REACTOR_EVENT_MASK
	);

    virtual int handlerRemove
	(
	EventHandler*		eventHandler,
	REACTOR_EVENT_MASK	mask
	);

    virtual int handleFind
	(
	EventHandler*		eventHandler,
	REACTOR_HANDLE&		handle
	);

    virtual int handlerFind
	(
	REACTOR_HANDLE		handle,
	EventHandler*&		eventHandler
	);

    virtual void timerAdd
	(
	EventHandler*		eventHandler,
	const TimeValue&	timeValue
	);

    virtual void timerRemove
	(
	EventHandler*		eventHandler
	);

    const HandleSet& rdHandles () const;
    const HandleSet& wrHandles () const;
    const HandleSet& exHandles () const;

    static Reactor* instance ();

    friend ostream& operator<< (ostream& os, const Reactor&);

  private:

    typedef STL_MAP (REACTOR_HANDLE, EventHandler*) Handle2HandlerMap;
    typedef STL_MAP (REACTOR_HANDLE, EventHandler*)::iterator
	Handle2HandlerMapIter;

    typedef STL_MAP (EventHandler*, REACTOR_HANDLE) Handler2HandleMap;
    typedef STL_MAP (EventHandler*, REACTOR_HANDLE)::iterator
	Handler2HandleMapIter;

    // t1 is the request timeout value, t2 is the expiring value.
    typedef pair<TimeValue, TimeValue> TimerValuePair;

    typedef STL_MAP (EventHandler*, TimerValuePair) TimerMap;

    typedef int (EventHandler::*EventHandlerCallback) (REACTOR_HANDLE);

    int handlerBind
	(
	REACTOR_HANDLE		handle,
	REACTOR_EVENT_MASK	mask
	);

    int handlerUnbind
	(
	REACTOR_HANDLE		handle,
	REACTOR_EVENT_MASK	mask,
	EventHandler*		eventHandler
	);

    int dispatchFdEvents
	(
	HandleSet&		rdSet,
	HandleSet&		wrSet,
	HandleSet&		exSet
	);

    int dispatchFdEvents
	(
	HandleSet&		selectHandles,
	HandleSet&		reactorHandles,
	REACTOR_EVENT_MASK	mask,
	EventHandlerCallback	callback
	);

    int dispatchFdEvent
	(
	HandleSet&		reactorSet,
	REACTOR_HANDLE		handle,
	REACTOR_EVENT_MASK	mask,
	EventHandlerCallback	callback
	);

    int updateTimers
	(
	const TimeValue&	elapsedTime
	);

    int dispatchTimers ();

    int dispatchTimer
	(
	const TimeValue&	timeValue,
	EventHandler*		eventHandler
	);

    int nextTimerGet
	(
	TimeValue&		timeValue
	);

    int select
	(
	int			nHandles,
	HandleSet*		rdSet,
	HandleSet*		wrSet,
	HandleSet*		exSet,
	TimeValue*		timeout
	);

    int wakeup ();
    
    HandleSet			m_rdHandles;
    HandleSet			m_wrHandles;
    HandleSet			m_exHandles;
    bool			m_endEventLoop;
    WakeupHandler		m_wakeupHandler;
    Handle2HandlerMap		m_handle2handlerMap;
    Handler2HandleMap		m_handler2handleMap;
    VxMutex			m_mutex; // protect internal data
    TimerMap			m_timerMap;

    // unsupported
    Reactor (const Reactor &);
    Reactor& operator= (const Reactor &);
    };

#endif // __INCReactor_h

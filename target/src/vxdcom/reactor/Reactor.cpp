/* Reactor - IO Multiplexor */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01v,17dec01,nel  Add include symbol for diab build.
01u,10dec01,dbs  diab build
01t,20sep01,nel  Fix compilation for ARM.
01s,30jul01,dbs  fix assertion logic
01r,13jul01,dbs  fix up includes
01q,19jan00,nel  Modifications for Linux debug build
01p,21sep99,aim  changed pipeDevCreate(1024, 1024) => (1,1); this fixes the
                 MBX860
01o,19aug99,aim  change assert to VXDCOM_ASSERT
01n,13aug99,aim  added ARG_UNUSED to clear compiler warnings
01m,12aug99,aim  MT fixes
01l,09aug99,aim  change wakeup behaviour
01k,02aug99,aim  added wakeup event handler
01j,21jul99,aim  quantify tweaks
01i,13jul99,aim  clear errno before entering select
01h,12jul99,aim  fix returned nHandles in select
01g,08jul99,aim  added timers
01f,07jul99,aim  added critical section protection
01e,29jun99,aim  reset timeout on eventLoopReset
01d,07jun99,aim  remove dubious event loop termination
01c,04jun99,aim  removed debug
01b,03jun99,aim  remove abort
01a,07may99,aim  created
*/

#include "EventHandler.h"
#include "Reactor.h"
#include "Syslog.h"
#include "TraceCall.h"
#include "private/comMisc.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_Reactor (void)
    {
    return 0;
    }

#ifdef VXDCOM_PLATFORM_VXWORKS
#include "pipeDrv.h"
#include "selectLib.h"
#endif

Reactor::Reactor ()
  : m_rdHandles (),
    m_wrHandles (),
    m_exHandles (),
    m_endEventLoop (false),
    m_wakeupHandler (this),
    m_handle2handlerMap (),
    m_handler2handleMap (),
    m_mutex (),
    m_timerMap ()
    {
    TRACE_CALL;

    if (m_wakeupHandler.handleGet () != INVALID_REACTOR_HANDLE)
	{
	timerAdd (&m_wakeupHandler, TimeValue (60));
	handlerAdd (&m_wakeupHandler, EventHandler::READ_MASK);
	}
    else
	m_endEventLoop = true;

    COM_ASSERT (m_wakeupHandler.handleGet () > 0);
    }

Reactor::~Reactor ()
    {
    TRACE_CALL;

    Handle2HandlerMapIter iter (m_handle2handlerMap.begin ());

    while (iter != m_handle2handlerMap.end ())
	{
	REACTOR_HANDLE handle = (*iter).first;
	EventHandler* eventHandler = (*iter).second;

	// note: handleClose could remove an entry from the map we are
	// currently iterating over and the entry it will delete is the
	// current position of the iterator.  Therefore we must
	// advance the iter and then call handleClose.

	++iter;

	COM_ASSERT (eventHandler);

	if (eventHandler)
	    eventHandler->handleClose (handle);
	}
    }

int Reactor::run ()
    {
    TRACE_CALL;
    return eventLoopRun ();
    }

int Reactor::eventLoopRun ()
    {
    TRACE_CALL;

    int result = 0;

    while (!m_endEventLoop)
	{
	if ((result = handleEvents ()) < 0)
	    break;
	}

    return result;
    }

void Reactor::eventLoopEnd ()
    {
    TRACE_CALL;
    m_endEventLoop = true;
    }

bool Reactor::eventLoopDone ()
    {
    TRACE_CALL;
    return m_endEventLoop;
    }

void Reactor::close ()
    {
    TRACE_CALL;
    eventLoopEnd ();
    }

int Reactor::handlerAdd
    (
    EventHandler*	eventHandler,
    REACTOR_EVENT_MASK	eventMask
    )
    {
    TRACE_CALL;

    VxCritSec cs (m_mutex);
    
    REACTOR_HANDLE handle = eventHandler->handleGet ();

    COM_ASSERT (handle != INVALID_REACTOR_HANDLE);

    int result = handlerBind (handle, eventMask);

    if (result != -1)
	{
	m_handle2handlerMap[handle] = eventHandler;
	m_handler2handleMap[eventHandler] = handle;
	}

    wakeup ();
    
    return result;
    }

int Reactor::handlerRemove
    (
    EventHandler*	eventHandler,
    REACTOR_EVENT_MASK	eventMask
    )
    {
    TRACE_CALL;

    VxCritSec cs (m_mutex);
    
    REACTOR_HANDLE handle;

    int result = -1;

    if (handleFind (eventHandler, handle) == 0)
	{
	m_handle2handlerMap.erase (handle);
	m_handler2handleMap.erase (eventHandler);
	result = handlerUnbind (handle, eventMask, eventHandler);
	}

    wakeup ();

    return result;
    }

int Reactor::handleFind
    (
    EventHandler*	eventHandler,
    REACTOR_HANDLE&	handle
    )
    {
    VxCritSec cs (m_mutex);

    Handler2HandleMapIter iter;

    iter = m_handler2handleMap.find (eventHandler);

    if (iter == m_handler2handleMap.end ())
	return -1;

    handle = (*iter).second;

    return 0;
    }

int Reactor::handlerFind
    (
    REACTOR_HANDLE handle,
    EventHandler*& eventHandler
    )
    {
    VxCritSec cs (m_mutex);

    Handle2HandlerMapIter iter;

    iter = m_handle2handlerMap.find (handle);

    if (iter == m_handle2handlerMap.end ())
	return -1;

    eventHandler = (*iter).second;

    return 0;
    }

int Reactor::select
    (
    int		nHandles,
    HandleSet*	rdSet,
    HandleSet*	wrSet,
    HandleSet*  exSet,
    TimeValue*	timeout
    )
    {
    timeval* t = 0;

    if (timeout)
	t = *timeout;

    REACTOR_HANDLE_SET_TYPE* r = 
		rdSet ? *rdSet : (REACTOR_HANDLE_SET_TYPE *)NULL;
    REACTOR_HANDLE_SET_TYPE* w = 
		wrSet ? *wrSet : (REACTOR_HANDLE_SET_TYPE *)NULL;
    REACTOR_HANDLE_SET_TYPE* e = 
		exSet ? *exSet : (REACTOR_HANDLE_SET_TYPE *)NULL;

#ifdef VXDCOM_PLATFORM_VXWORKS
    errno = 0;
#endif
    
    return ::select (nHandles, r, w, e, t);
    }

int Reactor::handleEvents ()
    {
    TRACE_CALL;

    HandleSet rdSet (m_rdHandles);
    HandleSet wrSet (m_wrHandles);
    HandleSet exSet; // (m_exHandles);

    int haveTimer = 0;
    int maxHandle = max (rdSet.maxHandle (), wrSet.maxHandle ());

    TimeValue timeout;

    if (nextTimerGet (timeout) == 0)
	{
	haveTimer = 1;

	if (timeout < TimeValue::zero ())
	    timeout = TimeValue::zero (); // + TimeValue (0, 100);
	}

    TimeValue startTime = TimeValue::now ();

    int selectFds = select (maxHandle +1,
			    (rdSet.count () > 0) ? &rdSet : 0,
			    (wrSet.count () > 0) ? &wrSet : 0,
			    (exSet.count () > 0) ? &exSet : 0,
			    haveTimer ? &timeout : 0);

    if (m_endEventLoop)
	return 0;

    if (selectFds < 0)
	return -1;

    if (haveTimer)
	updateTimers ((TimeValue::now() - startTime));
    
    if (haveTimer)
	dispatchTimers ();

    if (selectFds > 0)
	{
	rdSet.sync (maxHandle +1);
	wrSet.sync (maxHandle +1);
	exSet.sync (maxHandle +1);
	dispatchFdEvents (rdSet, wrSet, exSet);
	}

    return 0;
    }

int Reactor::dispatchFdEvents
    (
    HandleSet&	rdSet,
    HandleSet&	wrSet,
    HandleSet&	exSet
    )
    {
    // exceptions not supported on vxWorks
    // dispatchFdEvents (exSet,
    //	               m_exHandles,
    //	               EventHandler::EXCEPT_MASK,
    //	               &EventHandler::handleException);

    if (rdSet.count () > 0)
	{
	dispatchFdEvents (rdSet,
			  m_rdHandles,
			  EventHandler::READ_MASK,
			  &EventHandler::handleInput);
	}

#if 0
    if (wrSet.count () > 0)
	{
	dispatchFdEvents (wrSet,
			  m_wrHandles,
			  EventHandler::WRITE_MASK,
			  &EventHandler::handleOutput);
	}
#endif

    return 0;
    }

int Reactor::dispatchFdEvents
    (
    HandleSet&			selectHandles,
    HandleSet&	       		reactorHandles,
    REACTOR_EVENT_MASK		mask,
    EventHandlerCallback	callback
    )
    {
    TRACE_CALL;

    REACTOR_HANDLE handle;

    HandleSetIterator hsIter (selectHandles);

    while ((handle = hsIter ()) != INVALID_REACTOR_HANDLE)
	dispatchFdEvent (reactorHandles, handle, mask, callback);

    return 0;
    }

int Reactor::dispatchFdEvent
    (
    HandleSet&			reactorHandles,
    REACTOR_HANDLE		handle,
    REACTOR_EVENT_MASK		eventMask,
    EventHandlerCallback	callback
    )
    {
    TRACE_CALL;

    int result = -1;
    EventHandler* eventHandler = 0;

    handlerFind (handle, eventHandler);

    if (eventHandler)
	{
	result = (eventHandler->*callback) (handle);

	if (result < 0)
	    handlerRemove (eventHandler, eventMask);
	else if (result > 0)
	    COM_ASSERT (0);
	// XXX reactorHandles.clr (handle);
	}
    else
	{
	S_DEBUG(0, "Missing eventHandler: " << handle << (*this));
        COM_ASSERT (eventHandler);
	}

    return result;
    }

int Reactor::handlerBind
    (
    REACTOR_HANDLE	handle,
    REACTOR_EVENT_MASK	eventMask
    )
    {
    TRACE_CALL;

    VxCritSec cs (m_mutex);

    if (handle == INVALID_REACTOR_HANDLE)
	return -1;

    if (eventMask & EventHandler::READ_MASK)
	m_rdHandles.set (handle);

    if (eventMask & EventHandler::ACCEPT_MASK)
	m_rdHandles.set (handle);

    if (eventMask & EventHandler::CONNECT_MASK)
	m_rdHandles.set (handle);

    if (eventMask & EventHandler::WRITE_MASK)
	m_wrHandles.set (handle);

    if (eventMask & EventHandler::EXCEPT_MASK)
	m_exHandles.set (handle);

    return 0;
    }

int Reactor::handlerUnbind
    (
    REACTOR_HANDLE	handle,
    REACTOR_EVENT_MASK	eventMask,
    EventHandler*	eventHandler
    )
    {
    TRACE_CALL;

    VxCritSec cs (m_mutex);

    if (handle == INVALID_REACTOR_HANDLE)
	return -1;

    if (eventMask & EventHandler::READ_MASK)
	m_rdHandles.clr (handle);

    if (eventMask & EventHandler::ACCEPT_MASK)
	m_rdHandles.clr (handle);

    if (eventMask & EventHandler::CONNECT_MASK)
	m_rdHandles.clr (handle);

    if (eventMask & EventHandler::WRITE_MASK)
	m_wrHandles.clr (handle);

    if (eventMask & EventHandler::EXCEPT_MASK)
	m_exHandles.clr (handle);

    if ((eventMask & EventHandler::DONT_CALL) == 0)
	eventHandler->handleClose (handle, eventMask);

    return 0;
    }

Reactor* Reactor::instance ()
    {
    static Reactor r;
    return &r;
    }

const HandleSet& Reactor::rdHandles () const
    {
    TRACE_CALL;
    return m_rdHandles;
    }

const HandleSet& Reactor::wrHandles () const
    {
    TRACE_CALL;
    return m_wrHandles;
    }

const HandleSet& Reactor::exHandles () const
    {
    TRACE_CALL;
    return m_exHandles;
    }

ostream& operator<< (ostream& os, const Reactor& r)
    {
    os << "read-mask ("
       << r.rdHandles ()
       << ") ";
#if 0
    os << "write-mask ("
       << r.wrHandles ()
       << ") ";

    os << "event-mask ("
       << r.exHandles ()
       << ")";
#endif

    return os;
    }

void Reactor::timerAdd
    (
    EventHandler*	eventHandler,
    const TimeValue&	timeValue
    )
    {
    TRACE_CALL;
    VxCritSec cs (m_mutex);
    m_timerMap [eventHandler] = TimerValuePair (timeValue, timeValue);
    wakeup ();
    }

void Reactor::timerRemove
    (
    EventHandler*	eventHandler
    )
    {
    TRACE_CALL;
    VxCritSec cs (m_mutex);
    m_timerMap.erase (eventHandler);
    }

int Reactor::nextTimerGet
    (
    TimeValue&	timeValue
    )
    {
    TRACE_CALL;

    VxCritSec cs (m_mutex);

    if (m_timerMap.size () == 0)
	return -1;
    
    TimerMap::const_iterator iter (m_timerMap.begin ());

    while (iter != m_timerMap.end ())
	{
	const TimerValuePair& tp = (*iter).second;

	if (iter == m_timerMap.begin ())
	    timeValue = tp.second;
	else
	    timeValue = min (timeValue, tp.second);

	++iter;
	}

    return 0;
    }

int Reactor::updateTimers (const TimeValue& elapsedTime)
    {
    TRACE_CALL;

    VxCritSec cs (m_mutex);

    TimerMap::iterator iter (m_timerMap.begin ());

    while (iter != m_timerMap.end ())
	{
	TimerValuePair& tp = (*iter).second;
	++iter;
	tp.second -= elapsedTime;
	}

    return 0;
    }

int Reactor::dispatchTimers ()
    {
    TRACE_CALL;

    VxCritSec cs (m_mutex);

    TimerMap::iterator iter (m_timerMap.begin ());

    while (iter != m_timerMap.end ())
	{
	EventHandler* eventHandler = (*iter).first;
	TimerValuePair& tp = (*iter).second;

	// note: dispatchTimer (could) remove an entry from the map we
	// are currently iterating over.  The entry it will delete is
	// the current position of the iterator.  Therefore we must
	// advance the iter and then call dispatchTimer.

	++iter;

	if (tp.second <= TimeValue::zero ())
	    {
	    dispatchTimer (tp.first, eventHandler);
	    tp.second = tp.first;	      // reset timer
	    }
	}

    return 0;
    }

int Reactor::dispatchTimer
    (
    const TimeValue&	timeValue,
    EventHandler*	eventHandler
    )
    {
    TRACE_CALL;
    
    COM_ASSERT (eventHandler);

    int result = eventHandler->handleTimeout (timeValue);

    if (result < 0)
	timerRemove (eventHandler);

    return result;
    }

int Reactor::wakeup ()
    {
    TRACE_CALL;
    return m_wakeupHandler.reactorWakeup ();
    }

Reactor::WakeupHandler::WakeupHandler (Reactor* reactor)
  : m_wakeupPending (false),
    m_wakeupPendingLock ()
    {
    TRACE_CALL;

    m_handles[0] = INVALID_REACTOR_HANDLE;
    m_handles[1] = INVALID_REACTOR_HANDLE;

    reactorSet (reactor);

#if defined (VXDCOM_PLATFORM_SOLARIS) || defined (VXDCOM_PLATFORM_LINUX)
    int result = ::pipe (m_handles);

    if (result != -1)
	{
	handleSet (m_handles[0]);
	}

#elif defined (VXDCOM_PLATFORM_VXWORKS)
    char* filename = "/pipe/vxdcom";

    if (::pipeDevCreate (filename, 1, 1) == OK)
	{
	m_handles[0] = ::open (filename, O_RDONLY, 0);
	m_handles[1] = ::open (filename, O_WRONLY, 0);

	if (m_handles[0] < 0 || m_handles[1] < 0)
	    {
	    // tidy up on any error.
	    
	    ::close (m_handles[0]);
	    ::close (m_handles[1]);

	    m_handles[0] = INVALID_REACTOR_HANDLE;
	    m_handles[1] = INVALID_REACTOR_HANDLE;
	    }
	}
    else
	{
	S_EMERG (LOG_REACTOR, "cannot open: " << filename);
	}
#endif
    }

REACTOR_HANDLE Reactor::WakeupHandler::handleGet () const
    {
    return m_handles[0];
    }

REACTOR_HANDLE Reactor::WakeupHandler::handleSet (REACTOR_HANDLE handle)
    {
    return m_handles[0] = handle;
    }
    
int Reactor::WakeupHandler::handleInput (REACTOR_HANDLE handle)
    {
    TRACE_CALL;
    
    VxCritSec cs (m_wakeupPendingLock);
    
    char buf [1];

    COM_ASSERT (m_wakeupPending);
    
    int n = ::read (m_handles[0], buf, 1);

    if (n != 1)
	{
	S_ERR (LOG_REACTOR | LOG_ERRNO,
	       "Reactor::WakeupHandler read failed");
	}

    // Mark reactorWakeup() that so that a new event may be posted.
        
    m_wakeupPending = false;

    // Always return 0 for this EventHandler.

    return 0;
    }

int Reactor::WakeupHandler::reactorWakeup ()
    {
    TRACE_CALL;

    VxCritSec cs (m_wakeupPendingLock);

    // Don't bombard the Reactor if there is an event outstanding.
    
    if (m_wakeupPending)
	return 0;

    m_wakeupPending = true;

    // Now, wakeup the Reactor

    int n = ::write (m_handles[1], "", 1); // write one null byte

    if (n != 1)
	{
	S_ERR (LOG_REACTOR | LOG_ERRNO,
	       "Reactor::Wakeup failed to write null byte");
	}

    // Always return 0 for this EventHandler.
    
    return 0;
    }

int Reactor::WakeupHandler::handleTimeout (const TimeValue&)
    {
    Reactor* reactor = reactorGet ();

    COM_ASSERT (reactor);

    S_DEBUG(LOG_REACTOR, (*reactor));

    return 0;
    }

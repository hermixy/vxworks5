/* ThreadPool */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01m,22apr02,nel  SPR#76056. Correct test on task creation to test for -1
                 rather than < 0.
01l,17dec01,nel  Add include symbol for diab.
01k,03aug01,dbs  remove usage of Thread class
01j,13jul01,dbs  fix up includes
01i,24feb00,dbs  fix thread-killing in dtor
01h,21sep99,aim  changed API for activate
01g,20sep99,aim  added Thread name parameter
01f,19aug99,aim  dtor now tries to clean up all threads
01e,19aug99,aim  change assert to VXDCOM_ASSERT
01d,13aug99,aim  added default threadPriority
01c,12aug99,aim  added queue length ctor parameter
01b,11aug99,aim  now blocks when all threads are busy
01a,29jul99,aim  created
*/

#include "ThreadPool.h"
#include "EventHandler.h"
#include "Syslog.h"
#include "TraceCall.h"
#include "private/comMisc.h"
#include "taskLib.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_ThreadPool (void)
    {
    return 0;
    }

ThreadPool::ThreadPool
    (
    size_t	thrStackSize,
    long	thrPriority
    )
  : m_minThreads (0),
    m_maxThreads (0),
    m_threadCount (0),
    m_threadCountLock (),
    m_thrStackSize (thrStackSize),
    m_thrPriority (thrPriority),
    m_thrScavenger (0),
    m_thrName (0)
    {
    TRACE_CALL;
    }

ThreadPool::~ThreadPool ()
    {
    TRACE_CALL;

    DELZERO (m_thrScavenger);

    threadNameDelete ();

    }

int ThreadPool::open
    (
    int		maxThreads,
    const char*	threadName
    )
    {
    TRACE_CALL;

    threadNameSet (threadName);

    for (int i = 0; i < maxThreads; ++i)
	{
	    if (threadAdd () < 0)
		return -1;
	    else {
	        ++m_minThreads;
		++m_maxThreads;
	    }
	}

    return 0;
    }

int ThreadPool::open
    (
    Reactor*	reactor,
    int		minThreads,
    int		maxThreads,
    const char*	threadName
    )
    {
    TRACE_CALL;

    if ((m_thrScavenger = createScavenger (reactor)) == 0)
	return -1;

    threadNameSet (threadName);

    for (int i = 0; i < minThreads; ++i)
	if (threadAdd () < 0) 
	    {
	    m_minThreads = 0;
	    m_maxThreads = 0;
	    return -1;
	    }

    m_minThreads = minThreads;
    m_maxThreads = maxThreads;

    return 0;
    }

int ThreadPool::close ()
    {
    TRACE_CALL;

    DELZERO (m_thrScavenger);

    removeAll ();

    // Post a NULL event to all possible threads...
    for (int i=0; i < threadCount (); ++i)
	threadRemove ();
    
    // ...and wait for threads to terminate...
    cout << "waiting for threads..." << endl;
    while (threadCount () > 0)
        {
        cout << "...sleeping..." << endl;
        ::taskDelay (1);
        }

    return 0;
    }


int ThreadPool::queueFullHandler ()
    {
    // If we have no scavenger we cannot dynamically add threads.

    if (m_thrScavenger == 0)
	return 0;

    if (threadCount () < maxThreads ())
	return threadAdd ();
    else
	return 0;
    }

void* ThreadPool::serviceHandler ()
    {
    TRACE_CALL;

    EventHandler* pEventHandler;

    while (1)
	{
        ::taskPrioritySet (::taskIdSelf (), m_thrPriority);
	
	pEventHandler = 0;

	// remove() will block until a job is inserted into the Q.

	if (remove (pEventHandler) < 0)
	    break;

	if (pEventHandler == 0)
	    break;

	REACTOR_HANDLE handle = pEventHandler->handleGet ();

	if (pEventHandler->handleInput (handle) < 0)
	    pEventHandler->handleClose (handle);
	}

    VxCritSec cs (m_threadCountLock);

    queueSizeSet (--m_threadCount);
    
    return 0;
    }

int ThreadPool::minThreads () const
    {
    TRACE_CALL;
    return m_minThreads;
    }

int ThreadPool::maxThreads () const
    {
    TRACE_CALL;
    return m_maxThreads;
    }

int ThreadPool::threadCount () const
    {
    TRACE_CALL;
    return m_threadCount;
    }

int ThreadPool::enqueue (EventHandler* pEventHandler)
    {
    TRACE_CALL;
    return add (pEventHandler);
    }

int ThreadPool::threadAdd ()
    {
    TRACE_CALL;

    VxCritSec cs (m_threadCountLock);

    // Create a new (named) thread.
    
    int result = activate (m_thrName, m_thrPriority, m_thrStackSize);
    
    if (result != -1)
	{
	queueSizeSet (++m_threadCount);
	return 0;
	}
    
    return -1;
    }

int ThreadPool::threadRemove ()
    {
    TRACE_CALL;
    return enqueue (0);
    }

int ThreadPool::threadReaper ()
    {
    TRACE_CALL;

    VxCritSec cs (m_threadCountLock);

    int unusedThreads = threadCount() - queueSize ();

    if ((threadCount () - unusedThreads) < minThreads ())
	unusedThreads -= minThreads ();

    if (queueIsFull () || unusedThreads <= 0)
	return 0;
    
    while (unusedThreads-- > 0)
	threadRemove ();

    return 0;
    }

ThreadPool::Scavenger* ThreadPool::createScavenger
    (
    Reactor*	reactor,
    int		scavengerTimeout
    )
    {
    if (reactor == 0)
	return 0;
    
    ThreadPool::Scavenger* s = new ThreadPool::Scavenger (reactor, this);

    if (s)
	reactor->timerAdd (s, scavengerTimeout);
    else
	DELZERO (s);

    return s;
    }
    
ThreadPool::Scavenger::Scavenger
    (
    Reactor*	reactor,
    ThreadPool*	threadPool
    )
  : m_threadPool (threadPool)
    {
    TRACE_CALL;
    }

ThreadPool::Scavenger::~Scavenger ()
    {
    TRACE_CALL;

    Reactor* reactor = reactorGet ();

    if (reactor)
	reactor->timerRemove (this);
    }

int ThreadPool::Scavenger::handleTimeout (const TimeValue&)
    {
    TRACE_CALL;
    COM_ASSERT (m_threadPool);
    return m_threadPool->threadReaper ();
    }

void ThreadPool::threadNameDelete ()
    {
    delete [] m_thrName;
    m_thrName = 0;
    }

void ThreadPool::threadNameSet (const char* threadName)
    {
    threadNameDelete ();
    xstrdup (m_thrName, threadName);
    }

const char* ThreadPool::xstrdup (char*& dst, const char* src)
    {
    if (src == 0)
	return 0;

    dst = new char [::strlen (src) +1 ];

    return dst ? ::strcpy (dst, src) : 0;
    }

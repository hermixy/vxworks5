/* ThreadPool */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,03aug01,dbs  remove usage of Thread class
01f,21sep99,aim  changed API for activate
01e,20sep99,aim  added Thread name parameter
01d,13aug99,aim  added default threadPriority
01c,12aug99,aim  added queue length ctor parameter
01b,11aug99,aim  now blocks when all threads are busy
01a,29jul99,aim  created
*/

#ifndef __INCThreadPool_h
#define __INCThreadPool_h

#include "TaskQueue.h"
#include "Reactor.h"
#include "TimeValue.h"

class EventHandler;

class ThreadPool : public TaskQueue<EventHandler*>
    {
  public:

    ThreadPool
	(
	size_t		thrStackSize = 0,
	long		thrPriority  = 150
	);

    virtual ~ThreadPool ();

    virtual int close ();
    
    virtual int open
	(
	int		maxThreads,
	const char*	threadName = 0
	);

    virtual int open
	(
	Reactor*	reactor,
	int		minThreads,
	int		maxThreads,
	const char*	threadName = 0
	);

    // from TaskQueue
    virtual void* serviceHandler ();
    virtual int queueFullHandler ();

    int minThreads () const;
    int maxThreads () const;
    int threadCount () const;
    
    int enqueue (EventHandler*);

    class Scavenger;
    friend class Scavenger;
    
  private:

    int		m_minThreads;
    int		m_maxThreads;
    int		m_threadCount;
    VxMutex	m_threadCountLock;
    size_t	m_thrStackSize;
    long	m_thrPriority;
    Scavenger*	m_thrScavenger;
    char*	m_thrName;
	
    int 	threadAdd ();
    int 	threadRemove ();
    int		threadReaper ();
    void	threadNameSet (const char* name);
    void	threadNameDelete ();
    const char*	xstrdup (char*& dst, const char* src);
    
    Scavenger*	createScavenger
	(
	Reactor*	reactor,
	int		scavengerTimeout = 5 // sec
	);

    ThreadPool (const ThreadPool& other);
    ThreadPool& operator= (const ThreadPool& rhs);
    };

class ThreadPool::Scavenger : public EventHandler
    {
  public:
    virtual ~Scavenger ();
    virtual int handleTimeout (const TimeValue&);
    
  private:
    friend class ThreadPool;
    Scavenger (Reactor*, ThreadPool*);
    ThreadPool*	m_threadPool;
    };

#endif // __INCThreadPool_h

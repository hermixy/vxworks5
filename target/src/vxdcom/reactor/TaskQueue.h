/* TaskQueue */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,13nov01,nel  Change DEQUE to LIST.
01i,03aug01,dbs  remove usage of Thread class
01h,13jul01,dbs  fix up condvar/mutex usage
01g,13jul01,dbs  fix up includes
01f,28feb00,dbs  fix q-not-full name, add removeAll() method
01e,21sep99,aim  changed API for activate
01d,20sep99,aim  added Thread name parameter
01c,13aug99,aim  added thread parameters
01b,12aug99,aim  added queue length ctor parameter
01a,29jul99,aim  created
*/

#ifndef __INCTaskQueue_h
#define __INCTaskQueue_h

#include "private/comMisc.h"
#include "private/comStl.h"

template <class T>
class TaskQueue
    {
  public:

    virtual ~TaskQueue ();

    TaskQueue (size_t maxLength = 0);

    virtual int activate
	(
	const char*	threadName = 0,
	long		priority = 150,
	size_t		stackSize = 0
	);
    
    virtual int open (void* = 0);
    virtual int close ();
    virtual int add (T);
    virtual int remove (T&);
    virtual void* serviceHandler ();
    virtual size_t queueSizeSet (size_t maxLength);
    virtual size_t queueSizeMax () const;
    virtual size_t queueSize () const;
    virtual bool queueIsFull ();
    virtual int queueFullHandler ();
    virtual int removeAll ();

  private:

    STL_LIST(T)		m_queue;
    VxMutex	        m_queueLock;
    VxCondVar		m_queueNotEmpty;
    VxCondVar		m_queueNotFull;
    size_t		m_queueMaxLength;
    
    static void* threadHandler (void*);

    TaskQueue (const TaskQueue& other);
    TaskQueue& operator= (const TaskQueue& rhs);
    };

#include "TaskQueue.tcc"

#endif // __INCTaskQueue_h



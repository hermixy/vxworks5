/* TaskQueue */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,13nov01,nel  Change DEQUE to LIST.
01k,12oct01,nel  Fix test harness build.
01j,10oct01,nel  Change VX_FP_STACK to VX_FP_TASK.
01i,10oct01,nel  SPR#70838. Ensure that all threads are started with
                 VX_FP_TASK to get round any FP/longlong issues with certain
                 BSPs and also usage of FP in DCOM servers.
01h,03aug01,dbs  remove usage of Thread class
01g,13jul01,dbs  fix condvar/mutex usage
01f,23feb00,dbs  revert previous 01f
01e,21sep99,aim  changed API for activate
01d,20sep99,aim  added Thread name parameter
01c,13aug99,aim  added thread parameters
01b,12aug99,aim  added queue length ctor parameter
01a,29jul99,aim  created
*/

#include "TaskQueue.h"
#include "TraceCall.h"
#include "taskLib.h"


template <class T> TaskQueue<T>::TaskQueue (size_t maxLength)
  : m_queue (),
    m_queueLock (),
    m_queueNotEmpty (),
    m_queueMaxLength (maxLength)
    {
    }

template <class T> TaskQueue<T>::~TaskQueue ()
    {
    }

template <class T>
int TaskQueue<T>::activate
    (
    const char* threadName,
    long	priority,
    size_t	stackSize
    )
    {
    return ::taskSpawn (const_cast<char*> (threadName),
                        priority,
                        VX_FP_TASK,
                        stackSize,
                        reinterpret_cast<FUNCPTR> (TaskQueue::threadHandler),
                        reinterpret_cast<int> (this),
                        0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

template <class T>
int TaskQueue<T>::open (void*)
    {
    return 0;
    }

template <class T>
int TaskQueue<T>::close ()
    {
    return 0;
    }

template <class T>
int TaskQueue<T>::add (T t)
    {
    VxCritSec cs (m_queueLock);

    m_queue.push_back (t);
    m_queueNotEmpty.signal ();

    if (queueIsFull ())
	{
	queueFullHandler ();
	
	while (queueIsFull ())
	    m_queueNotFull.wait (m_queueLock);
	}

    return 0;
    }

template <class T>
int TaskQueue<T>::remove (T& t)
    {
    VxCritSec cs (m_queueLock);

    // block until add() signals that there is something on the queue

    while (queueSize () == 0)
	m_queueNotEmpty.wait (m_queueLock);

    // remember first element
    t = m_queue.front ();

    // physically remove
    m_queue.pop_front ();

    m_queueNotFull.signal ();
    
    return 0;
    }

template <class T>
int TaskQueue<T>::removeAll ()
    {
    VxCritSec cs (m_queueLock);

    // physically remove all
    m_queue.erase (m_queue.begin (), m_queue.end ());

    m_queueNotFull.signal ();
    
    return 0;
    }

template <class T>
void* TaskQueue<T>::serviceHandler ()
    {
    return 0;
    }

template <class T>
void* TaskQueue<T>::threadHandler (void* arg)
    {
    TaskQueue<T>* self = static_cast<TaskQueue<T>*> (arg);
    void* result = self->serviceHandler ();
    return result;
    }

template <class T>
size_t TaskQueue<T>::queueSizeSet (size_t maxLength)
    {
    return m_queueMaxLength = maxLength;
    }

template <class T>
size_t TaskQueue<T>::queueSizeMax () const
    {
    return m_queueMaxLength;
    }

template <class T>
size_t TaskQueue<T>::queueSize () const
    {
    return m_queue.size ();
    }

template <class T>
int TaskQueue<T>::queueFullHandler ()
    {
    return 0;
    }

template <class T>
bool TaskQueue<T>::queueIsFull ()
    {
    return queueSize () > queueSizeMax ();
    }

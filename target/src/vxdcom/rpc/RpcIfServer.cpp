/* RpcIfServer */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,17dec01,nel  Add include symbol for diab.
01k,13jul01,dbs  fix up includes
01j,24feb00,dbs  call close() from dtor
01i,20sep99,aim  added ThreadName parameter
01h,19aug99,aim  change assert to VXDCOM_ASSERT
01g,13aug99,aim  added threadPriority to ThreadPool ctor
01f,12aug99,aim  ThreadPool API changes
01e,30jul99,aim  added thread pooling
01d,15jul99,aim  added serverAddress
01c,09jul99,dbs  use final filenames
01b,08jun99,aim  rework
01a,27may99,aim  created
*/

#include "RpcIfServer.h"
#include "RpcEventHandler.h"
#include "Reactor.h"
#include "RpcDispatcher.h"
#include "Syslog.h"
#include "TraceCall.h"
#include "private/comMisc.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_RpcIfServer (void)
    {
    return 0;
    }

RpcIfServer::RpcIfServer
    (
    Reactor*		reactor,
    RpcDispatcher*	dispatcher,
    concurrency_t	concurrency
    )
  : RpcServer (reactor),
    m_dispatcher (dispatcher),
    m_addressBinding (),
    m_concurrency (concurrency),
    m_threadPool (g_vxdcomDefaultStackSize, g_vxdcomThreadPoolPriority)
    {
    COM_ASSERT (m_dispatcher);
    TRACE_CALL;
    };

RpcIfServer::~RpcIfServer ()
    {
    TRACE_CALL;

    // Call our own close() in case someone has over-ridden close()
    // and doesn't defer to this class's close() as they should...
    RpcIfServer::close ();
    }

int RpcIfServer::open
    (
    const INETSockAddr&	addr,
    Reactor*		reactor,
    int			reuseAddr,
    int			minThreads,
    int			maxThreads
    )
    {
    if (m_concurrency == ThreadPooled)
	m_threadPool.open (reactor, minThreads, maxThreads, "tComTask");
    
    // delegate everything else
    return super::open (addr, reactor, reuseAddr);
    }

int RpcIfServer::close ()
    {
    if (m_concurrency == ThreadPooled)
	m_threadPool.close ();

    // delegate everything else
    return super::close ();
    }

RpcIfServer::concurrency_t RpcIfServer::concurrency () const
    {
    return m_concurrency;
    }

ThreadPool* RpcIfServer::threadPool ()
    {
    return &m_threadPool;
    }
    
const RpcStringBinding& RpcIfServer::addressBinding ()
    {
    if (!m_addressBinding.isValid ())
	{
	INETSockAddr addr;

	if (hostAddrGet (addr) != -1)
	    m_addressBinding = RpcStringBinding (addr);
	}

    return m_addressBinding;
    }

int RpcIfServer::rpcAddressFormat
    (
    BSTR*		bstr,
    bool		includePortNumber
    )
    {
    const RpcStringBinding& sb = addressBinding ();

    if (sb.isValid () && sb.format (bstr, includePortNumber) != -1)
	return 0;
    else
	return -1;
    }

int RpcIfServer::newSvcHandler
    (
    RpcEventHandler*&	eventHandler
    )
    {
    TRACE_CALL;
    eventHandler = new RpcEventHandler (reactorGet (), m_dispatcher);
    return eventHandler ? 0 : -1;
    }


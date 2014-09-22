/* RpcIfServer -- Rpc Interface Server (either DCE or ORPC) */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01j,19jul01,dbs  fix include path of vxdcomGlobals.h
01i,19jan00,nel  Modifications for Linux debug build
01h,20sep99,aim  added ThreadName parameter
01g,13aug99,aim  changed min/max thread params
01f,12aug99,aim  ThreadPool API changes
01e,30jul99,aim  added thread pooling
01d,15jul99,aim  added serverAddress
01c,09jul99,dbs  use final filenames
01b,08jun99,aim  rework
01a,27may99,aim  created
*/

#ifndef __INCRpcIfServer_h
#define __INCRpcIfServer_h

#include "ReactorTypes.h"
#include "Acceptor.h"
#include "SockAcceptor.h"
#include "INETSockAddr.h"
#include "RpcEventHandler.h"
#include "Reactor.h"
#include "RpcDispatcher.h"
#include "RpcStringBinding.h"
#include "ThreadPool.h"
#include "private/vxdcomGlobals.h"

#if (defined VXDCOM_PLATFORM_WIN32 || defined VXDCOM_PLATFORM_VXWORKS)
typedef Acceptor <RpcEventHandler, SockAcceptor, INETSockAddr> RpcServer;
#elif (defined VXDCOM_PLATFORM_SOLARIS || defined VXDCOM_PLATFORM_LINUX)
typedef Acceptor <RpcEventHandler, SockAcceptor> RpcServer;
#endif

class RpcIfServer : public RpcServer
    {
  public:

    typedef RpcServer super;

    enum concurrency_t {
	SingleThreaded = 1,
	ThreadPerConnection,
	ThreadPooled
    };
        
    RpcIfServer
	(
	Reactor*	reactor,
	RpcDispatcher*	dispatcher,
	concurrency_t	= SingleThreaded
	);

    virtual ~RpcIfServer ();

    virtual int open
	(
	const INETSockAddr&	addr,
	Reactor*		reactor,
	int			reuseAddr = 1,
	int			minThreads = g_vxdcomMinThreads,
	int			maxThreads = g_vxdcomMaxThreads
	);

    virtual int close ();

    concurrency_t concurrency () const;

    ThreadPool* threadPool ();
	
    const RpcStringBinding& addressBinding ();

    int rpcAddressFormat
	(
	BSTR*		result,
	bool		includePortNumber = true
	);

  protected:

    RpcIfServer ();
    // ensure RpcIfServer is an ABC.

    virtual int newSvcHandler (RpcEventHandler*&);
    // override Acceptor

  private:

    RpcDispatcher*	m_dispatcher;
    RpcStringBinding	m_addressBinding;
    concurrency_t	m_concurrency;
    ThreadPool		m_threadPool;

    // unsupported
    RpcIfServer (const RpcIfServer& other);
    RpcIfServer& operator= (const RpcIfServer& rhs);
    };

#endif // __INCRpcIfServer_h

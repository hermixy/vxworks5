/* Acceptor - Encapsulates the behaviour for a listening socket */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,18aug99,aim  SockAcceptor API changes
01g,12aug99,aim  fix handleInput
01f,10aug99,aim  fix SvcHandler leaks when accept fails
01e,13jul99,aim  syslog api changes
01d,07jul99,aim  prints info on new connection
01c,29jun99,aim  handleClose always calls m_acceptor.close
01b,05jun99,aim  removed unnecessary headers
01a,12may99,aim  created
*/

template <class SVC_HANDLER, PEER_ACCEPTOR_1>
Acceptor<SVC_HANDLER, PEER_ACCEPTOR_2>::operator PEER_ACCEPTOR& () const
    {
    return (PEER_ACCEPTOR &) m_acceptor;
    }

template <class SVC_HANDLER, PEER_ACCEPTOR_1> PEER_ACCEPTOR&
Acceptor<SVC_HANDLER, PEER_ACCEPTOR_2>::acceptor () const
    {
    return (PEER_ACCEPTOR &) m_acceptor;
    }

template <class SVC_HANDLER, PEER_ACCEPTOR_1> REACTOR_HANDLE
Acceptor<SVC_HANDLER, PEER_ACCEPTOR_2>::handleGet () const
    {
    return m_acceptor.handleGet ();
    }

template <class SVC_HANDLER, PEER_ACCEPTOR_1> int
Acceptor<SVC_HANDLER, PEER_ACCEPTOR_2>::open
    (
    const PEER_ACCEPTOR_ADDR& localAddr,
    Reactor* reactor,
    int reuseAddr
    )
    {
    int result = m_acceptor.open (localAddr, reuseAddr);

    if (result != -1)
	result = reactor->handlerAdd (this, EventHandler::ACCEPT_MASK);

    if (result != -1)
	reactorSet (reactor);

    return result;
    }

template <class SVC_HANDLER, PEER_ACCEPTOR_1>
Acceptor<SVC_HANDLER, PEER_ACCEPTOR_2>::Acceptor (Reactor* reactor)
    {
    reactorSet (reactor);
    }

template <class SVC_HANDLER, PEER_ACCEPTOR_1>
Acceptor<SVC_HANDLER, PEER_ACCEPTOR_2>::Acceptor
    (
    const PEER_ACCEPTOR_ADDR&	addr,
    Reactor*			reactor,
    int				reuseAddr
    )
    {
    reactorSet (reactor);
    open (addr, reactor, reuseAddr);
    }

template <class SVC_HANDLER, PEER_ACCEPTOR_1>
Acceptor<SVC_HANDLER, PEER_ACCEPTOR_2>::~Acceptor ()
    {
    handleClose ();
    }

template <class SVC_HANDLER, PEER_ACCEPTOR_1> int
    Acceptor<SVC_HANDLER, PEER_ACCEPTOR_2>::close ()
    {
    return handleClose ();
    }

template <class SVC_HANDLER, PEER_ACCEPTOR_1> int
Acceptor<SVC_HANDLER, PEER_ACCEPTOR_2>::handleClose
    (
    REACTOR_HANDLE		handle,
    REACTOR_EVENT_MASK		mask
    )
    {
    m_acceptor.close ();

    // Guard against multiple closes.
    if (reactorGet () != 0)
	{
	// We must pass the DONT_CALL flag here to avoid recursion.
	REACTOR_EVENT_MASK mask = EventHandler::ACCEPT_MASK |
				  EventHandler::DONT_CALL;

	reactorGet()->handlerRemove (this, mask);
	}

    return 0;
    }

template <class SVC_HANDLER, PEER_ACCEPTOR_1> int
Acceptor<SVC_HANDLER, PEER_ACCEPTOR_2>::newSvcHandler
    (
    SVC_HANDLER*& svcHandler
    )
    {
    if (svcHandler == 0)
	svcHandler = new SVC_HANDLER(reactorGet ());

    return svcHandler ? 0 : -1;
    }

template <class SVC_HANDLER, PEER_ACCEPTOR_1> int
Acceptor<SVC_HANDLER, PEER_ACCEPTOR_2>::acceptSvcHandler
    (
    SVC_HANDLER* svcHandler
    )
    {
    if (svcHandler == 0)
	return -1;
    
    PEER_ACCEPTOR_ADDR peerAddr;
    
    if (m_acceptor.accept (svcHandler->stream (), peerAddr) < 0)
	return -1;

    if (svcHandler->open (this) < 0)
	{
	svcHandler->close ();		// close fd
	return -1;
	}

    return 0;
    }

template <class SVC_HANDLER, PEER_ACCEPTOR_1> int
Acceptor<SVC_HANDLER, PEER_ACCEPTOR_2>::handleInput (REACTOR_HANDLE)
    {
    SVC_HANDLER* svcHandler = 0;

    if (newSvcHandler (svcHandler) < 0)
	return 0;		// always leave this Handler registered

    if (acceptSvcHandler (svcHandler) < 0)
	{
	S_ERR(LOG_REACTOR|LOG_ERRNO, "acceptSvcHandler failed: " << (*svcHandler));
	DELZERO (svcHandler);
	}
    else
	S_INFO(LOG_REACTOR, "connection: " << (*svcHandler));
    
    return 0;			// always leave this Handler registered
    }

template <class SVC_HANDLER, PEER_ACCEPTOR_1> int
Acceptor<SVC_HANDLER, PEER_ACCEPTOR_2>::hostAddrGet (PEER_ACCEPTOR_ADDR& sa)
    {
    return m_acceptor.hostAddrGet (sa);
    }

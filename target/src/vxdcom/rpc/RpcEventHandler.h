/* RpcEventHandler */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,03aug01,dbs  remove usage of Thread class
01k,26jun00,dbs  implement presentation context IDs
01j,30jul99,aim  added thread pooling
01i,09jul99,dbs  use new filenames
01h,07jul99,aim  added ostream operator<<
01g,28jun99,dbs  add channelId() method
01f,24jun99,dbs  add authn checking
01e,24jun99,dbs  move authn into new class
01d,18jun99,aim  set data rep on outgoing packets
01c,08jun99,aim  rework
01b,08jun99,aim  now uses NRpcPdu
01a,27may99,aim  created
*/

#ifndef __INCRpcEventHandler_h
#define __INCRpcEventHandler_h

#include "SvcHandler.h"
#include "SockStream.h"
#include "RpcPdu.h"
#include "ntlmssp.h"
#include <iostream>

class Reactor;
class RpcDispatcher;
class ThreadPool;
class RpcIfServer;

class RpcEventHandler : public SvcHandler<SockStream>
    {
  public:

    typedef SvcHandler<SockStream> super;

    virtual ~RpcEventHandler ();

    RpcEventHandler (Reactor* r);

    RpcEventHandler
	(
	Reactor*	reactor,
	RpcDispatcher*	dispatcher
	);

    virtual int handleInput (REACTOR_HANDLE);
    virtual int open (void* = 0);
    virtual int close (u_long flags = 0);

    int channelId () const;

    RpcIfServer* acceptorGet () const;
    RpcIfServer* acceptorSet (RpcIfServer*);

    const INETSockAddr& hostAddr ();
    const INETSockAddr& peerAddr ();

    friend ostream& operator<< (ostream&, const RpcEventHandler&);

  private:

    typedef STL_MAP(USHORT, IID) PresCtxMap;
    
    RpcPdu*		m_pdu;
    INETSockAddr	m_hostAddr;
    INETSockAddr	m_peerAddr;
    RpcDispatcher*	m_dispatcher;
    int         	m_creatorTaskId;
    RpcIfServer*	m_acceptor;
    PresCtxMap		m_presCtxMap;
    
    int reply (const RpcPdu& pdu, RpcPdu& replyPdu);
    int sendPdu (RpcPdu&);
    int process ();
    int dispatchPdu (const RpcPdu& pdu);
    int dispatchBind (const RpcPdu& pdu);
    int dispatchRequest (const RpcPdu& pdu);
    int dispatchAuth3 (const RpcPdu&);

    int concurrency ();
    ThreadPool* RpcEventHandler::threadPool ();

    // unsupported
    RpcEventHandler (const RpcEventHandler& other);
    RpcEventHandler& operator= (const RpcEventHandler& rhs);
    };

#endif // __INCRpcEventHandler_h

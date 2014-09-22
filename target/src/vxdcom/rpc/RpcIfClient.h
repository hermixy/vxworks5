/* RpcIfClient */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,10oct01,dbs  add AddKnownInterface() method to IOrpcClientChannel
01k,26jul01,dbs  implement IOrpcClientChannel interface
01j,13jul01,dbs  fix up includes
01i,29mar01,nel  SPR#35873. Add extra code to hold the client side context IDs
                 for bound interfaces and modify the bindToIID method to send
                 alter context for already bound interfaces rather than always
                 sending bind.
01h,20jun00,dbs  fix client BIND/ALTER_CONTEXT handling
01g,22jul99,dbs  enforce serialisation on method-calls
01f,19jul99,dbs  add client-side authn support
01e,09jul99,dbs  remove references to obsolete files
01d,06jul99,aim  new ctors
01c,02jul99,aim  added RpcInvocation functionality
01b,08jun99,aim  rework
01a,05jun99,aim  created
*/

#ifndef __INCRpcIfClient_h
#define __INCRpcIfClient_h

#include "private/comStl.h"
#include "SockStream.h"
#include "SockConnector.h"
#include "INETSockAddr.h"
#include "RpcPdu.h"
#include "RpcStringBinding.h"
#include "comLib.h"
#include "private/comMisc.h"

class RpcIfClient : public IOrpcClientChannel
    {
  public:
    virtual ~RpcIfClient ();

    RpcIfClient (const INETSockAddr& peerAddr);
    RpcIfClient (const RpcStringBinding& binding);

    int channelId () const;

    bool connected () const;
    bool interfaceIsBound (const IID& iid, USHORT& ctxId);

    // IOrpcClientChannel methods
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();
    STDMETHOD(QueryInterface) (REFIID riid, void** ppv);
    STDMETHOD(InvokeMethod) (REFIID,
                             const IPID*,
                             USHORT,
                             const MSHL_BUFFER*,
                             MSHL_BUFFER*);
    STDMETHOD(AddKnownInterface) (REFIID);
    STDMETHOD(AllocBuffer) (MSHL_BUFFER*, DWORD);
    STDMETHOD(FreeBuffer) (MSHL_BUFFER*);
    
  private:

    typedef STL_VECTOR(IID) InterfaceVec;
    typedef unsigned long CALL_ID;

    INETSockAddr	m_peerAddr;
    InterfaceVec	m_boundIIDs;
    IID			m_currentIID;
    SockStream		m_stream;
    SockConnector	m_connector;
    bool		m_connected;
    VxMutex		m_mutex;
    LONG                m_dwRefCount;
    
    static CALL_ID	s_callId;

    int connect
	(
	const INETSockAddr&	peerAddr, 
	HRESULT&		hresult
	);

    int connect ();

    int sendMethod
	(
	RpcPdu&		request,
	RpcPdu&		result,
	HRESULT&	hresult
	);

    SockStream& stream ();
    CALL_ID nextCallId ();
    int sendPdu (RpcPdu& pdu);
    int recvPdu (RpcPdu& pdu);
    int bindToIID (const IID& iid, const GUID*, HRESULT&, USHORT&);

    // unsupported
    RpcIfClient (const RpcIfClient& other);
    RpcIfClient& operator= (const RpcIfClient& rhs);
    };

#endif // __INCRpcIfClient_h

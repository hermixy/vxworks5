/* RpcProxyMsg.h - VxDCOM RpcProxyMsg class definition */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,26jul01,dbs  use IOrpcClientChannel interface
01e,05aug99,dbs  change to byte instead of char
01d,06jul99,aim  change from RpcBinding to RpcIfClient
01c,20may99,dbs  move NDR phase into streams
01b,17may99,dbs  fix DCE usage of class
01a,12may99,dbs  created

*/

#ifndef __INCRpcProxyMsg_h
#define __INCRpcProxyMsg_h

#include "dcomProxy.h"
#include "NdrStreams.h"
#include "orpc.h"

typedef CComPtr<IOrpcClientChannel> IOrpcClientChannelPtr;

class RpcProxyMsg
    {
  public:
    RpcProxyMsg (REFIID iid, RpcMode::Mode_t mode, ULONG opnum, void* pv);
    ~RpcProxyMsg ();

    NdrMarshalStream*	marshalStreamGet ();
    HRESULT		SendReceive ();
    NdrUnmarshalStream* unmarshalStreamGet ();

    void		interfaceInfoSet (REFIPID i, IOrpcClientChannel* c)
	{ m_ipid = i; m_pChannel = c; }
	
  private:
    RpcMode::Mode_t	m_mode;		// mode flag (OBJECT or DCE)
    NdrMarshalStream	m_mshlStrm;	// stream to marshal into
    NdrUnmarshalStream	m_unmshlStrm;	// stream to unmarshal from
    IUnknown*		m_punkItfPtr;	// used only for OBJECT mdoe
    IID			m_iid;		// IID
    IPID		m_ipid;		// IPID
    ULONG		m_opnum;	// method number
    byte*		m_pReply;	// received reply
    IOrpcClientChannelPtr m_pChannel;	// channel to remote server
    };

#endif


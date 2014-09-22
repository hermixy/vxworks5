/* RemoteSCM.h -- VxDCOM Remote SCM class */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01f,26jul01,dbs  use IOrpcClientChannel interface
01e,13jul01,dbs  fix up includes
01d,22jul99,dbs  add re-use of remote-SCM connection
01c,16jul99,dbs  convert map/set with long long key to use new macros
01b,09jul99,dbs  implement ping functionality in SCM now
01a,09jul99,dbs  created
*/

#ifndef __INCRemoteSCM_h
#define __INCRemoteSCM_h

#include "comObjLib.h"
#include "dcomLib.h"
#include "private/comStl.h"
#include "RemoteOxid.h"

typedef CComPtr<IOrpcClientChannel> IOrpcClientChannelPtr;

class RemoteSCM
    {
  public:
    
    RemoteSCM (const RpcStringBinding& sb);
    RemoteSCM ();
    ~RemoteSCM ();

    // ref-counting methods
    ULONG AddRef ();
    ULONG Release ();

    // accessors
    const RpcStringBinding& stringBinding () const
	{ return m_strb; }

    // method to return a connection to the remote SCM
    IOrpcClientChannel* connectionGet ();
    
    // method to look up an OXID binding
    bool oxidBindingLookup (OXID, SPRemoteOxid&) const;

    // method to update the table of OXID bindings
    void oxidBindingUpdate
	(
	OXID			oxid,
	REFIPID			ipidRemUnk,
	const RpcStringBinding&	sbRemoteOxid
	);

    // method to indicate passage of time in ping-handling
    void pingTick (size_t nSecs);

    // methods to add/delete OIDs from ping-sets
    void oidAdd (OID oid)
	{
	VxCritSec cs (m_mutex);
	m_oidsToAdd.push_back (oid);
	}

    void oidDel (OID oid)
	{
	VxCritSec cs (m_mutex);
	m_oidsToDel.push_back (oid);
	}
    
    
  private:
    // not implemented
    RemoteSCM (const RemoteSCM&);
    RemoteSCM& operator= (const RemoteSCM&);

    typedef STL_MAP_LL(SPRemoteOxid)	OXIDMAP;
    typedef STL_VECTOR(OID)		OIDSET;


    VxMutex		m_mutex;
    LONG		m_dwRefCount;
    OXIDMAP		m_remoteOxidTable;
    RpcStringBinding	m_strb;

    IOrpcClientChannelPtr  m_pChannel;
    
    SETID		m_setid;
    unsigned short	m_pingSeqNum;
    OIDSET		m_oidsToAdd;
    OIDSET		m_oidsToDel;
    long		m_tickCount;
    };

typedef CComPtr<RemoteSCM> SPRemoteSCM;

#endif


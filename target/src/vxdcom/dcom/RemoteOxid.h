/* RemoteOxid.h - COM/DCOM RemoteOxid class definition */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01u,26jul01,dbs  remove obsolete code
01t,13jul01,dbs  fix up includes
01s,15feb00,dbs  add dcomProxy.h to includes
01r,12jul99,dbs  add accessor to modify string-binding
01q,09jul99,dbs  implement ping functionality in SCM now
01p,08jul99,dbs  move address and other info into new class
01o,06jul99,aim  change from RpcBinding to RpcIfClient
01n,10jun99,dbs  remove inclusion of comNew.h
01m,08jun99,dbs  remove use of mtmap
01l,07jun99,dbs  change GuidMap to mtmap
01k,01jun99,dbs  make tick-count signed
01j,28may99,dbs  continue Ping implementation
01i,28may99,dbs  implement Ping functionality
01h,18may99,dbs  change to new marshaling architecture
01g,11may99,dbs  change name of ChannelBuffer.h to Remoting.h
01f,05may99,dbs  add update() method
01e,29apr99,dbs  fix -Wall warnings
01d,28apr99,dbs  make all classes allocate from same pool
01c,27apr99,dbs  add mem-pool to classes
01b,21apr99,dbs  add include for IRpcChannelBuffer
01a,20apr99,dbs  created during Grand Renaming

*/


#ifndef __INCRemoteOxid_h
#define __INCRemoteOxid_h

#include "private/comMisc.h"
#include "dcomProxy.h"
#include "OxidResolver.h"
#include "RpcStringBinding.h"
#include "RemUnknown.h"
#include "comObjLib.h"

///////////////////////////////////////////////////////////////////////////
//
// VxRemoteOxid -- records information about a remote Object
// Exporter. It must re-use a StdProxy's connection in order to
// process IRemUnknown methods, so each of those methods requires an
// RpcIfClient with which to make the request.
//

class VxRemoteOxid
    {
  public:
    
    VxRemoteOxid
	(
	OXID			oxid,
	REFIPID 		ipidRemUnk,
	const RpcStringBinding&	oxidAddr
	);
    
    virtual ~VxRemoteOxid ();

    // Accessors...
    OXID oxid () const
	{ return m_oxid; }

    IPID ipidRemUnknown () const
	{ return m_ipidRemUnk; }

    const RpcStringBinding& stringBinding () const
	{ return m_stringBinding; }

    // update members...
    void update (REFIPID ipid, const RpcStringBinding& sb)
	{ m_ipidRemUnk=ipid; m_stringBinding=sb; }
    
    // ref-counting methods
    ULONG AddRef ();
    ULONG Release ();

  private:

    VxMutex		m_mutex;
    LONG                m_dwRefCount;
    OXID		m_oxid;
    IPID		m_ipidRemUnk;
    RpcStringBinding	m_stringBinding;

    };

typedef CComPtr<VxRemoteOxid> SPRemoteOxid;

#endif



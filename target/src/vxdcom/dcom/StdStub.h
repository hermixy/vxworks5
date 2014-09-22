/* StdStub.h - COM/DCOM StdStub class definition */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01u,13jul01,dbs  fix up includes
01t,20sep00,nel  Add changes made in T2 since branch.
01s,27jul99,drm  Returning CLSID from interfaceInfoGet().
01r,29jun99,dbs  remove obsolete ctor
01q,28jun99,dbs  make Relase() return refs so deletion can be done by OX
01p,17jun99,aim  uses new SCM
01o,10jun99,dbs  remove inclusion of comNew.h
01n,08jun99,dbs  remove use of mtmap
01m,04jun99,dbs  change GuidMap to mtmap
01l,02jun99,dbs  inherit from RpcDispatchTable
01k,18may99,dbs  change to new marshaling architecture
01j,11may99,dbs  remove inclusion of ChannelBuffer.h
01i,11may99,dbs  simplify stub remoting architecture
01h,10may99,dbs  add method to determine if interface is supported
01g,28apr99,dbs  make all classes allocate from same pool
01f,26apr99,dbs  remove class-specific debug code
01e,26apr99,dbs  add mem-pool to StubInfo class
01d,26apr99,dbs  add show command
01c,23apr99,dbs  add mem-pool to class
01b,21apr99,dbs  add include for IRpcChannelBuffer
01a,20apr99,dbs  created during Grand Renaming

*/

#ifndef __INCStdStub_h
#define __INCStdStub_h

#include "dcomProxy.h"
#include "private/comMisc.h"
#include "private/comStl.h"
#include "RpcDispatchTable.h"

///////////////////////////////////////////////////////////////////////////
//
// VxStdStub - the standard-marshaling stub (a.k.a. stub manager) which
// represents the server-object for the purposes of remoting its 
// interfaces. It keeps a list of all the stublets (interface stubs)
// it maintains, one for each remoted interface on the object, keyed
// by their IPID, in the table m_stublets. It also maintains a
// parallel table, m_interfaces, which maps from IID to stublet - this
// increases lookup speed when determining if a particular interface
// is supported by the stub itself.
//
// The VxStdStub object itself maintains one reference to the server,
// and each stublet maintains one reference. Remote references are 
// counted per-IPID and only when the total remote ref-count reaches 
// zero is the VxStdStub object destroyed.
//
// Remote refs to the IUnknown of the server are kept in the
// 'm_stublets' table just like all other refs - this makes it easier
// to total up the outstanding refs, etc. However, it does mean the
// IUnknown entry requires special treatment, as IUnknown is not
// remoted, so sometimes (if the client has only asked for IUnknown)
// then there will be an entry in m_interfaces for IID_IUnknown,
// whereas normally there would be an entry for both IID_IUnknown and
// some other IID both pointing to the same stublet.
//

class VxStublet;

class VxStdStub : public RpcDispatchTable
    {
    typedef STL_MAP(GUID, VxStublet*) STUBLETMAP;
    
    IUnknown*		m_pUnkServer;	// server object
    STUBLETMAP		m_stublets;	// IPID -> Stublet
    STUBLETMAP		m_interfaces;	// IID -> Stublet
    OID			m_oid;		// object ID
    VxMutex		m_mutex;	// task safety
    

    VxStdStub& operator= (const VxStdStub&);
    VxStdStub::VxStdStub (const VxStdStub&);
    
  public:

    // ctor and dtor
    VxStdStub ();
    ~VxStdStub ();

    // RpcDispatchTable methods
    bool supportsInterface (REFIID);
    HRESULT interfaceInfoGet (REFIID, REFIPID, ULONG, IUnknown**, PFN_ORPC_STUB*, CLSID &);

    IUnknown* getIUnknown () const { return m_pUnkServer; }
    // adopt a particular server object...
    void adopt (IUnknown* punk, OID oid);
    
    // add an interface
    HRESULT interfaceAdd (REFIID iid, DWORD nRefs, IPID* pIpidNew);

    // add remote references per-IPID
    ULONG AddRef (ULONG n, REFIPID ipid);
    ULONG Release (ULONG n, REFIPID ipid);

#ifdef VXDCOM_DEBUG
    void printOn (ostream& os);
#endif
    };


#endif



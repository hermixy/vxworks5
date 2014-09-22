/* ObjectExporter.h - COM/DCOM ObjectExporter class definition */

/* Copyright (c) 1998 Wind River Systems, Inc. */

/*

modification history
--------------------
02a,03aug01,dbs  re-instate ref-counting methods
01z,30jul01,dbs  import later T2 changes
01y,19jul01,dbs  fix include-path of Remoting.h
01x,13jul01,dbs  fix up includes
01w,02mar01,nel  SPR#62130 - implement CoDisconnectObject.
01v,23feb00,dbs  add full ref-counting impl
01u,30jul99,aim  added thread pooling
01t,26jul99,dbs  move marshaling into exporter
01s,15jul99,aim  replaced RpcINETAddr
01r,09jul99,dbs  copy in replacement class from NObjectExporter files
01q,10jun99,dbs  replace explicit new/delete with macro
01p,02jun99,dbs  use export-table inheriting from RpcDispatchTable
01o,01jun99,dbs  add NOPING flag
01n,27may99,dbs  add get-class-object mode support
01m,27may99,dbs  implement IOXIDResolver functionality
01l,25may99,dbs  get OID and OXID values from SCM for uniqueness
01k,24may99,dbs  enhance SCM/ObjectExporter relationship
01j,18may99,dbs  remove old marshaling scheme
01i,11may99,dbs  change name of ChannelBuffer.h to Remoting.h
01h,07may99,dbs  add RPC-dispatcher functionality to SCM
01g,29apr99,dbs  fix -Wall warnings
01f,28apr99,dbs  make all classes allocate from same pool
01e,28apr99,dbs  must delete OTE in objectUnregister()
01d,27apr99,dbs  add mem-pool to classes
01c,22apr99,dbs  improve iteration mechanism
01b,21apr99,dbs  add include for IRpcChannelBuffer
01a,20apr99,dbs  created during Grand Renaming

*/


#ifndef __INCObjectExporter_h
#define __INCObjectExporter_h

#include "RemUnknown.h"			// IRemUnknown interface
#include "dcomLib.h"			// basic types, structs, etc
#include "MemoryStream.h"
#include "StdStub.h"
#include "RpcIfServer.h"
#include "private/comMisc.h"
#include "ObjectTable.h"


////////////////////////////////////////////////////////////////////////////
//
// ObjectExporter -- this class implements an object which 'exports'
// a number of COM-objects. In the current prototype design, there is
// only one object-exporter per system, and hence one OXID value. The
// object records all OIDs and IPIDs exported, and can verify them
// when an incoming ORPC is to be dispatched.
//
// Also, note that there is only ever one SCM task, and one
// IOXIDResolver interface, per machine. However, there may (in
// future) be multiple Object Exporters, perhaps one per Protection
// Domain in VxWorks 6.0 for example.
//
// The SCM creates object exporters in response to incoming activation
// requests, and once an exporter has been created, it stays around
// effectively forever, or until its containing application dies. In
// VxWorks 5.x there is no such thing as an 'application' really, so
// the one and only exporter stays around forever (i.e. until the
// system restarts).
//
// In future (e.g. VxWorks 6.x) there may well be applications, in the
// context of Protection Domains, and so an exporter may well be
// destroyed when a PD is destroyed. This will become the SCM's
// responsibility, and so it will always retain a reference to
// an exporter whilst the exporter remains in the SCM's tables.
//

class Reactor;

class ObjectExporter : public RpcIfServer, public IRemUnknown
    {
  public:

    virtual ~ObjectExporter ();
    ObjectExporter (Reactor*, OXID);

    HRESULT init ();

    // method to 'ping' one object (identified by its OID)...
    HRESULT oidPing (OID oid);

    // method to time-out all exported objects
    HRESULT tick (size_t nSecs);
    
    // IUnknown functions
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();
    STDMETHOD(QueryInterface) (REFIID, void**);

    // IRemUnknown methods 
    STDMETHOD(RemQueryInterface)
        (
        REFIPID         ipid,
        ULONG           cRefs,
        USHORT          cIIDs,
        const IID*      iids,
        REMQIRESULT**   ppQIResults
        );

    STDMETHOD(RemAddRef)
        (
        USHORT          cInterfaceRefs,
        REMINTERFACEREF interfaceRefs [],
        HRESULT*        pResults
        );

    STDMETHOD(RemRelease) 
        (
        USHORT          cInterfaceRefs,
        REMINTERFACEREF interfaceRefs []
        );

    // get OXID value
    OXID oxid () const { return m_oxid; };

    // get IPID of IRemUnknown
    IPID ipidRemUnknown () const
	{ return m_ipidRemUnknown; }

    // get server's listening network address
    HRESULT addressBinding (BSTR*);

    // method to marshal an object for remoting
    HRESULT objectMarshal
	(
	REFCLSID	rclsid,		// class ID
	IStream*	pStm,		// stream to marshal into
	REFIID		riid,		// interface IID
	IUnknown*	pUnk,		// interface-ptr to be marshaled
	DWORD		dwDestContext,  // destination context
	void*		pvDestContext,  // reserved for future use
	DWORD		mshlflags,	// reason for marshaling
	ObjectTableEntry** ppOTE	// resulting table entry
	);

    // methods to search for remoted objects
    ObjectTableEntry* objectFindByOid (OID o);
    ObjectTableEntry* objectFindByStream (IStream* pstm);
    ObjectTableEntry* objectFindByToken (DWORD dwToken);
    ObjectTableEntry* objectFindByIUnknown (IUnknown*);

    // method to unregister a remoted object
    bool              objectUnregister (OID o);

    // method to unregister all remoted objects forcefully
    void objectUnregisterAll ();

  protected:
    ObjectExporter ();		// ensure ObjectExporter is an ABC.

  private:

    OXID                m_oxid;		// our OXID
    IPID                m_ipidRemUnknown;// our IRemUnknown IPID
    VxObjectTable	m_objectTable;
    RpcDispatcher	m_dispatcher;
    VxMutex		m_mutex;	// internal sync
    OID			m_oid;
    LONG                m_dwRefCount;
    
    // unsupported
    ObjectExporter (const ObjectExporter& other);
    ObjectExporter& operator= (const ObjectExporter& rhs);
    };

#endif



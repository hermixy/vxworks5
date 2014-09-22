/* SCM - Service Control Manager */

/* Copyright (c) 1999 - 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01q,06aug01,dbs  remove instance-creation from SCM
01p,03aug01,dbs  obj exp is a proper COM object
01o,13jul01,dbs  fix up includes
01n,18aug99,dbs  improve resilience by use of RC-ptr to ObjExp
01m,26jul99,dbs  move marshaling into exporter
01l,16jul99,dbs  convert map/set with long long key to use new macros
01k,15jul99,aim  serverAddress now in base class
01j,12jul99,aim  added timers
01i,09jul99,dbs  implement ping functionality in SCM now
01h,08jul99,dbs  add oxidResolve() method
01g,07jul99,aim  change from RpcBinding to RpcIfClient
01f,06jul99,dbs  simplify activation mechanism
01e,06jul99,dbs  make SCM create ObjectExporter at startup
01d,28jun99,dbs  remove defaultInstance method
01c,25jun99,dbs  use channel-ID to determine channel authn status
01b,08jun99,aim  rework
01a,27may99,aim  created
*/

#ifndef __INCSCM_h
#define __INCSCM_h

#include "RpcIfServer.h"
#include "RemoteSCM.h"
#include "comObjLib.h"
#include "dcomLib.h"
#include "OxidResolver.h"
#include "ObjectExporter.h"
#include "DceDispatchTable.h"
#include "orpc.h"


class NTLMSSP;

////////////////////////////////////////////////////////////////////////////
//
// Default (current) protocol-sequence and endpoints for the SCM
// itself. If we ever add more protocols, then we need to expand this
// list somehow...
//

#define VXDCOM_SCM_ENDPOINT 135
#define VXDCOM_SCM_PROTSEQ  NCACN_IP_TCP

//////////////////////////////////////////////////////////////////////////
//
// SCM -- implements the 'Service Control Manager' for VxDCOM.
//

class SCM : public RpcIfServer
    {
    class PingTimer : public EventHandler
	{
      public:
	virtual int handleTimeout (const TimeValue& tv);
	};

    friend class PingTimer;

    HRESULT instanceCreate
        (
        DWORD		mode,		// get-class-obj?
        REFCLSID	clsid,		// CLSID to create
        DWORD		nInterfaces,	// num i/f's to return
        MULTI_QI*	mqi		// resulting itf ptrs
        );
    
  public:

    SCM (Reactor* r = Reactor::instance (), NTLMSSP* ssp = 0);
    virtual ~SCM ();

    // IPrivateSCM methods
    HRESULT IndirectActivation
	(
        LPWSTR                  pwszServerName, // PROTSEQ + server name 
	REFGUID			clsid,		// CLSID to activate
	DWORD			mode,		// all-1's == get-class-obj
	DWORD			nItfs,		// num of interfaces
	IID*			pIIDs,		// array of IIDs
	MInterfacePointer**	ppItfData,	// returned interface(s)
	HRESULT*		pResults	// returned results per i/f
	);

    HRESULT AddOid (OID);

    HRESULT DelOid (OID);

    HRESULT GetNextOid (OID*);

    HRESULT GetOxidResolverBinding
        (
        USHORT                  cProtseqs,
        USHORT                  arProtseqs[],
        DUALSTRINGARRAY**       ppdsaBindings
        );

    // SCM Singleton access
    static SCM* theSCM ();

    // called by dcomLibInit
    static int startService ();
    static int stopService ();
    
    // return next free Object ID / OXID
    OXID nextOXID ();
    OID  nextOid ();

    static ObjectExporter* objectExporter ();
    static NTLMSSP* ssp ();

    HRESULT addressBinding (BSTR*);

    void oidAdd (const RpcStringBinding& resAddr, OID oid);
    void oidDel (const RpcStringBinding& resAddr, OID oid);
    
    // methods for registering / unregistering OXIDs and Object
    // Exporters as they are created and destroyed...
    void oxidRegister (OXID, ObjectExporter*);
    void oxidUnregister (OXID);

    // method to register DCE RPC interface with stub dispatch-table
    HRESULT dceInterfaceRegister (REFIID, const VXDCOM_STUB_DISPTBL*);

    // method to resolve an OXID (and a string-binding) into a
    // RemoteOxid object (which contains a string-binding)...
    HRESULT oxidResolve
	(
	OXID			oxid,
	const RpcStringBinding&	resAddr,
	SPRemoteOxid&		remOxid
	);
    
    // IRemoteActivation method (called by remote clients)...
    HRESULT RemoteActivation
	(
	int			channelId, 	// channel ID
	ORPCTHIS*		pOrpcThis,	// housekeeping
	ORPCTHAT*		pOrpcThat,	// returned housekeeping
	GUID*			pClsid,		// CLSID to activate
	OLECHAR*		pwszObjName,	// NULL
	MInterfacePointer*	pObjStorage,	// NULL
	DWORD			clientImpLevel,	// security
	DWORD			mode,		// all-1's == get-class-obj
	DWORD			nItfs,		// num of interfaces
	IID*			pIIDs,		// size_is (nItfs)
	USHORT			cReqProtseqs,	// num of protseqs
	USHORT			arReqProtseqs[],// array of protseqs
	OXID*			pOxid,		// returned OXID
	DUALSTRINGARRAY**	ppdsaOxidBindings,// returned bindings
	IPID*			pipidRemUnknown,// returned IPID
	DWORD*			pAuthnHint,	// returned security info
	COMVERSION*		pSvrVersion,	// returned server version
	HRESULT*		phr,		// returned activation result
	MInterfacePointer**	ppItfData,	// returned interface(s)
	HRESULT*		pResults	// returned results per i/f
	);
    
    // IOXIDResolver methods - the method ResolveOxid() is not
    // implemented directly as its functionality is contained in
    // ResolveOxid2()...
    
    HRESULT SimplePing
	(
	SETID			pSetid		// [in] set ID
	);

    HRESULT ComplexPing
	(
	SETID*			pSetid,		// [in,out] set ID
	USHORT			SeqNum,		// [in] sequence number
	USHORT			cAddToSet,	// [in]
	USHORT			cDelFromSet,	// [in]
	OID			AddToSet [],	// [in]
	OID			DelFromSet [],	// [in]
	USHORT*			pPingBackoffFactor // [out]
	);

    HRESULT ResolveOxid2
	(
	OXID			oxid,		// [in] OXID to resolve
	USHORT			cReqProtseqs,	// [in] num of protseqs
	USHORT			arReqProtseqs[],// [in] array of protseqs
	DUALSTRINGARRAY*	oxidBindings [],// [out] bindings
	IPID*			pipidRemUnknown,// [out] IPID
	DWORD*			pAuthnHint,	// [out] security info
	COMVERSION*		pComVersion	// [out] COM version
	);

  private:

    void oxidBindingUpdate
	(
	OXID			oxid,
	const RpcStringBinding&	sbRemoteScm,
	REFIPID			ipidRemUnk,
	const RpcStringBinding&	sbRemoteOxid
	);
    

    HRESULT mqiMarshal (REFCLSID, const MULTI_QI&, MInterfacePointer**);
    
    enum { MAX_EXPORTERS=32 };

    typedef CComPtr<ObjectExporter>	ObjectExporterPtr;
    
    typedef STL_SET_LL			OIDSET;
    typedef STL_MAP_LL(OIDSET)		OIDSETMAP;
    typedef STL_MAP_LL(ObjectExporterPtr) OXIDMAP;
    typedef STL_MAP(RpcStringBinding, SPRemoteSCM) REMOTESCMMAP;
    
    VxMutex		m_mutex;	// task-safety
    OXIDMAP		m_exporters;	// object exporters
    OIDSETMAP		m_oidSets;	// all OID sets
    SETID		m_nextSetid;	// for OID sets
    DceDispatchTable	m_dispatchTable;
    RpcDispatcher	m_dispatcher;
    NTLMSSP*		m_ssp;
    REMOTESCMMAP	m_remoteScmTable;
    
    OXID		m_nextOxid;	// next free OX id
    OID 		m_nextOid;	// next free object ID

    static SCM*		s_theSCM;	// the one-and-only instance
    
    void tick (size_t);
    void registerStdInterfaces ();
    int  init (INETSockAddr&);
    
    HRESULT newObjectExporter (ObjectExporter**);

    // unsupported
    SCM (const SCM& other);
    SCM& operator= (const SCM& rhs);
    };


#endif // __INCSCM_h

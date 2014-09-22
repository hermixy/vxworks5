/* StdProxy.h - COM/DCOM StdProxy class definition */

/*
modification history
--------------------
01x,25jul01,dbs  fix and improve description for IUnknown proxy methods
01w,13jul01,dbs  fix up includes
01v,29mar01,nel  SPR#35873. Add extra param to FaceletAdd to indicate that
                 interface has been bound through RemQueryInterface and should
                 issue a context switch rather than a bind.
01u,06mar01,nel  SPR#35589. Add code to make Oxid addresses unique to prevent
                 clash with other targets.
01t,28feb00,dbs  fix IRemUnknown facelet so it is never Release()'ed
01s,15feb00,dbs  implement IRemUnknown calls directly
01r,16jul99,dbs  convert map/set with long long key to use new macros
01q,09jul99,dbs  implement ping functionality in SCM now
01p,08jul99,dbs  mods for RemoteOxid changes
01o,06jul99,aim  change from RpcBinding to RpcIfClient
01n,30jun99,dbs  make Facelet-map contain smart-pointers
01m,10jun99,dbs  remove inclusion of comNew.h
01l,08jun99,dbs  remove use of mtmap
01k,07jun99,dbs  change GuidMap to mtmap
01j,27may99,dbs  implement Ping functionality
01i,17may99,dbs  remove IID from args of interaceInfoGet
01h,13may99,dbs  change BufferInit method to interfaceInfoGet
01g,11may99,dbs  simplify proxy remoting architecture
01f,29apr99,dbs  fix -Wall warnings
01e,28apr99,dbs  make all classes allocate from same pool
01d,26apr99,dbs  add mem-pool to class
01c,22apr99,dbs  tidy up potential leaks
01b,21apr99,dbs  add length arg to orpcDSAFormat()
01a,20apr99,dbs  created during Grand Renaming

*/

#ifndef __INCStdProxy_h
#define __INCStdProxy_h

#include "private/comMisc.h"
#include "private/comStl.h"
#include "RemoteOxid.h"
#include "comObjLib.h"
#include "orpc.h"

typedef CComPtr<IOrpcClientChannel> IOrpcClientChannelPtr;

///////////////////////////////////////////////////////////////////////////
//
// VxStdProxy - the main 'proxy manager' object that is created to
// 'impersonate' a server object, when that server actually lives on a
// different machine.
//
// It maintains a list of 'facelets' (one per remoted interface) and
// defers all methods to those interfaces, except its own IUnknown and
// IMarshal methods. The member 'm_facelets' has one entry per
// facelet, plus one entry for the IUnknown of the VxStdProxy
// itself. The class provides methods to find the appropriate instance
// given any valid interface-pointer.
//

class VxStdProxy : public IMarshal
    {
  public:

    VxStdProxy ();

    virtual ~VxStdProxy ();

    // IUnknown methods...
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();
    STDMETHOD(QueryInterface) (REFIID riid, void** ppv);
    
    // IMarshal methods...
    HRESULT STDMETHODCALLTYPE GetUnmarshalClass
	(
        REFIID,
        void*, 
        DWORD, 
        void*, 
        DWORD, 
        CLSID*
        );
        
    HRESULT STDMETHODCALLTYPE GetMarshalSizeMax
	(
        REFIID, 
        void*, 
        DWORD, 
        void*, 
        DWORD, 
        DWORD*
        );

    HRESULT STDMETHODCALLTYPE MarshalInterface
	(
        IStream*, 
        REFIID, 
        void*, 
        DWORD, 
        void*, 
        DWORD
        );
        
    HRESULT STDMETHODCALLTYPE UnmarshalInterface
	( 
        IStream*	pStm,
        REFIID		riid,
        void**		ppv
	);
        
    HRESULT STDMETHODCALLTYPE ReleaseMarshalData (IStream *pStm);

    HRESULT STDMETHODCALLTYPE DisconnectObject (DWORD dwReserved);

    // Accessors for apartment and object identifiers...
    OID  oid () const { return m_oid; }
    OXID oxid () const
        {
        if (m_pOxid)
            return m_pOxid->oxid ();
        return 0;
        }

  private:

    // Private class for holding a facelet (a single interface proxy)
    // via its IOrpcProxy interface. We need to record some info about
    // the interface proxy, like its IPID and the number of remote
    // refs it has...

    struct facelet_t
        {
        CComPtr<IOrpcProxy>     pProxy;
        DWORD                   remoteRefCount;
        IPID                    ipid;

        facelet_t ()
            {}

        facelet_t (IOrpcProxy* px, DWORD n, REFIPID id)
            : pProxy (px), remoteRefCount (n), ipid (id)
            {}
        
        facelet_t (const facelet_t& x)
            { *this = x; }

        facelet_t& operator= (const facelet_t& x)
            {
            remoteRefCount = x.remoteRefCount;
            pProxy = x.pProxy;
            ipid = x.ipid;
            return *this;
            }
        };

    // Find a facelet representing the given interface...
    facelet_t* faceletFind (REFIID iid);

    // Add a facelet for the given IID/IPID to this std-proxy, with
    // <nRefs> remote references, and return the resulting interface
    // pointer (of type <iid>) at <ppv>...
    HRESULT faceletAdd
        (
        REFIID              riid,
        REFIPID             ipidNew,
        DWORD               nRefs,
        void**              ppv
        );
    
    // Private data-structures for recording remote Object Exporters,
    // which are effectively 'apartments' in M$-speak and are
    // identified by OXID, and mapping to a table of OID-to-proxy
    // lookup...

    typedef STL_MAP_LL(VxStdProxy*)		RemoteApartment_t;
    typedef STL_MAP_LL(RemoteApartment_t)       OXID2RemApartment_t;

    // Map of all facelets, indexed by interface-pointer...
    typedef STL_MAP(void*, facelet_t)           FACELETMAP;
    
    // Static data - map of OXID => remote-apartment...
    static OXID2RemApartment_t  s_allProxies;
    static VxMutex	        s_allProxiesMutex;
        
    // Instance data 
    LONG                m_dwRefCount;	// local ref count
    VxMutex		m_mutex;	// task-safety
    FACELETMAP          m_facelets;	// map of facelets
    SPRemoteOxid	m_pOxid;	// OXID proxy object
    IOrpcClientChannelPtr m_pChannel;	// RPC client handle
    OID                 m_oid;          // OID in server
    RpcStringBinding	m_resAddr;	// oxid-resolver address
    IRemUnknown*	m_pRemUnknown;	// proxy to obj exporter
    facelet_t*		m_pRemUnkFacelet; // special facelet
    };


#endif



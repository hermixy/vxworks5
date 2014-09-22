/* InterfaceProxy.cpp - COM/DCOM interface-proxy class implementation */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01q,17dec01,nel  Add include symbol for diab build.
01p,08aug01,dbs  fix compiler warning
01o,24jul01,dbs  add public inner-proxy structure
01n,13jul01,dbs  fix up includes
01m,19aug99,aim  added TraceCall header
01l,16jul99,aim  added vxdcom header
01k,10jun99,dbs  remove op new and delete
01j,03jun99,dbs  no return value from mutex lock
01i,03jun99,dbs  remove refs to comSyncLib
01h,25may99,dbs  initialise remote-ref count to zero in ctor
01g,13may99,dbs  remove usage of IRpcChannelBuffer
01f,11may99,dbs  simplify proxy remoting architecture
01e,29apr99,dbs  fix -Wall warnings
01d,28apr99,dbs  use COM_MEM_ALLOC for all classes
01c,27apr99,dbs  add mem-pool to class
01b,26apr99,aim  added TRACE_CALL
01a,20apr99,dbs  created during Grand Renaming

*/

#include "private/comMisc.h"
#include "private/comMisc.h"
#include "StdProxy.h"
#include "InterfaceProxy.h"
#include "TraceCall.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_InterfaceProxy (void)
    {
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//

const int MAGIC = 0xFACEFACE;

//////////////////////////////////////////////////////////////////////////
//
// VxInterfaceProxy method implementations...
//
//////////////////////////////////////////////////////////////////////////

VxInterfaceProxy::VxInterfaceProxy
    (
    REFIID	        iid,
    REFIPID	        ipid,
    IUnknown *          pUnkOuter,
    const void *        pvVtbl
    )
  : m_iidManaged (iid),
     m_ipid (ipid),
     m_dwRefCount (0),
     m_pUnkOuter (pUnkOuter),
     m_pChannel (0)
    {
    m_interface.lpVtbl = pvVtbl;
    m_interface.magic = MAGIC;
    m_interface.backptr = this;
    }
    
//////////////////////////////////////////////////////////////////////////
//

VxInterfaceProxy::~VxInterfaceProxy ()
    {
    m_interface.magic = 0;
    m_interface.backptr = 0;
    if (m_pChannel)
        m_pChannel->Release ();
    }

//////////////////////////////////////////////////////////////////////////
//

ULONG VxInterfaceProxy::AddRef ()
    {
    return comSafeInc (&m_dwRefCount);
    }

//////////////////////////////////////////////////////////////////////////
//

ULONG VxInterfaceProxy::Release ()
    {
    DWORD n = comSafeDec (&m_dwRefCount);
    if (n == 0)
	delete this;
    return n;
    }

//////////////////////////////////////////////////////////////////////////
//
// VxInterfaceProxy::QueryInterface - private QI implementation
//
// This is the QI implementation for the VxInterfaceProxy's private
// IUnknown (actually IOrpcProxy) which is seen only by the containing
// StdProxy. It can be QI'ed for either IOrpcProxy, in which case it
// returns its own primary interface, or for the managed interface, in
// which case it returns the delegating proxy-interface.
//

HRESULT VxInterfaceProxy::QueryInterface (REFIID riid, void** ppv)
    {
    if (riid == IID_IOrpcProxy)
        {
        *ppv = this;
        AddRef ();
        return S_OK;
        }
    else if (riid == m_iidManaged)
        {
        *ppv = &m_interface;
        m_pUnkOuter->AddRef ();
        return S_OK;
        }
    return E_NOINTERFACE;
    }

//////////////////////////////////////////////////////////////////////////
//

HRESULT VxInterfaceProxy::interfaceInfoGet
    (
    IPID *                      pIpid,
    IOrpcClientChannel**        ppChan
    )
    {
    *pIpid = m_ipid;
    if (! m_pChannel)
        cout << "Channel gone away!" << endl;
    
    return m_pChannel->QueryInterface (IID_IOrpcClientChannel,
                                       (void**) ppChan);
    }

//////////////////////////////////////////////////////////////////////////
//

HRESULT VxInterfaceProxy::Connect (IOrpcClientChannel* pChan)
    {
    // We should not be connected...
    if (m_pChannel && (m_pChannel != pChan))
        return E_UNEXPECTED;        

    // Store a ref to the channel...
    m_pChannel = pChan;
    m_pChannel->AddRef ();
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//

HRESULT VxInterfaceProxy::Disconnect ()
    {
    if (m_pChannel)
        {
        m_pChannel->Release ();
        m_pChannel = 0;
        return S_OK;
        }

    return E_UNEXPECTED;
    }


//////////////////////////////////////////////////////////////////////////
//

VxInterfaceProxy* VxInterfaceProxy::safe_cast (const void* pvInterface)
    {
    void * pv = const_cast<void*> (pvInterface);
    interface_t* pitf = reinterpret_cast<interface_t*> (pv);
    if (pitf->magic != MAGIC)
        return 0;
    return pitf->backptr;
    }


//////////////////////////////////////////////////////////////////////////
//
// The following functions provide the vtable entries for the IUnknown 
// slots in proxy interfaces. They provide the IUnknown remoting
// functionality for all exposed, derived interfaces of a proxy
// object. They effectively form the IUnknown-methods of the public
// interface of any instance of VxInterfaceProxy, and all they do is
// delegate to the controlling unknown...
//

HRESULT IUnknown_QueryInterface_vxproxy
    (
    IUnknown*       punkThis,
    REFIID          iid,
    void**          ppv
    )
    {
    VxInterfaceProxy* pProxy = VxInterfaceProxy::safe_cast (punkThis);
    if (! pProxy)
        return E_NOINTERFACE;

    return pProxy->pUnkOuter()->QueryInterface (iid, ppv);
    }

ULONG IUnknown_AddRef_vxproxy
    (
    IUnknown*        punkThis
    )
    {
    VxInterfaceProxy* pProxy = VxInterfaceProxy::safe_cast (punkThis);
    if (! pProxy)
        return 0;

    return pProxy->pUnkOuter()->AddRef ();
    }

ULONG IUnknown_Release_vxproxy
    (
    IUnknown*       punkThis
    )
    {
    VxInterfaceProxy* pProxy = VxInterfaceProxy::safe_cast (punkThis);
    if (! pProxy)
        return 0;

    return pProxy->pUnkOuter()->Release ();
    }


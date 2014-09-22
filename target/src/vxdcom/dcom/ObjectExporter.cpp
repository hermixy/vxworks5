/* ObjectExporter.cpp - COM/DCOM ObjectExporter class implementation */

/*
modification history
--------------------
02x,17dec01,nel  Add include symbol for diab.
02w,03aug01,dbs  re-instate ref-counting methods
02v,30jul01,dbs  import T2 changes for RemAddRef and object ID
02u,18jul01,dbs  clean up stray printfs
02t,13jul01,dbs  fix use of NEW macro to operator new
02s,02mar01,nel  SPR#62130 - implement CoDisconnectObject.
02r,13oct00,nel  SPR#34947. Correct marshalling  error introduced by T2 fix
                 merge.
02q,18sep00,nel  SPR#33730. Merge T2 OPC fixes into T3 branch.
02p,23feb00,dbs  add full ref-counting impl
02o,08feb00,dbs  add diagnostics on RemUnknown IPID
02n,19aug99,aim  change assert to VXDCOM_ASSERT
02m,17aug99,aim  removed copy ctor and operator= from ObjectTable
02l,09aug99,aim  now defaults to using ThreadPools
02k,30jul99,aim  added thread pooling
02j,26jul99,dbs  move marshaling into exporter
02i,15jul99,aim  replaced RpcINETAddr
02h,12jul99,dbs  remove unwanted prints
02g,09jul99,dbs  copy in replacement class from NObjectExporter files
01i,06jul99,dbs  simplify activation mechanism
01h,29jun99,dbs  remove ostream << OID refs
01g,29jun99,dbs  make StdStub a member of ObjectTableEntry
01f,28jun99,dbs  make AddRef() and Release() dummy functions
01e,23jun99,aim  removed purify_new_leaks()
01d,18jun99,aim  fixed serverAddr checks
01c,17jun99,aim  changed assert to assert
01b,08jun99,aim  rework
01a,27may99,aim  snapshot
*/

#include "TraceCall.h"
#include "ObjectExporter.h"
#include "RpcStringBinding.h"
#include "orpcLib.h"
#include "ObjectTable.h"
#include "StdMarshaler.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_ObjectExporter (void)
    {
    return 0;
    }

ObjectExporter::ObjectExporter (Reactor* r, OXID oxid)
  : RpcIfServer (r, &m_dispatcher, RpcIfServer::ThreadPooled),
    m_oxid (oxid),
    m_objectTable (),
    m_dispatcher (&m_objectTable),
    m_mutex (),
    m_oid (0),
    m_dwRefCount (1)
    {
    TRACE_CALL;
    }

HRESULT ObjectExporter::addressBinding (BSTR* pbsResAddr)
    {
    TRACE_CALL;

    if (rpcAddressFormat (pbsResAddr) < 0)
        return E_FAIL;
    else        
        return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// ObjectExporter -- dtor...
//

ObjectExporter::~ObjectExporter ()
    {
    TRACE_CALL;

    // Unregister our own marshaled interface...
    objectUnregister (m_oid);
    }

////////////////////////////////////////////////////////////////////////////
//
// AddRef -- standard IUnknown method
//

DWORD ObjectExporter::AddRef ()
    {
    return comSafeInc (&m_dwRefCount);
    }

////////////////////////////////////////////////////////////////////////////
//
// Release -- standard IUnknown method
//

DWORD ObjectExporter::Release ()
    {
    DWORD n = comSafeDec (&m_dwRefCount);
    if (n == 0)
        delete this;
    return n;
    }

////////////////////////////////////////////////////////////////////////////
//

HRESULT ObjectExporter::QueryInterface (REFIID iid, void** ppv)
    {
    TRACE_CALL;
    if ((iid == IID_IUnknown) || (iid == IID_IRemUnknown))
        {
        *ppv = static_cast<IUnknown*> (this);
        AddRef ();
        return S_OK;
        }
    return E_NOINTERFACE;
    }

////////////////////////////////////////////////////////////////////////////
//
// RemQueryInterface -- respond to a remote QI request...
//

HRESULT ObjectExporter::RemQueryInterface
    (
    REFIPID             ipid,
    ULONG               cRefs,
    USHORT              cIIDs,
    const IID*          iids,
    REMQIRESULT**       ppQIResults
    )
    {
    TRACE_CALL;
    REMQIRESULT*        pQIResults = 0;
    HRESULT             hr = S_OK;
    IPID                ipidNew;
    OID                 oid;
    ObjectTableEntry*   pOTE;
    VxStdStub*          pStub;

    // Find the stub-object corresponding to the IPID...
    oid = orpcOidFromIpid (ipid);
    pOTE = objectFindByOid (oid);
    if (pOTE)
        pStub = &pOTE->stdStub;
    else
        return RPC_E_INVALID_IPID;

    // Allocate space for results...
    pQIResults = (REMQIRESULT*)
                 CoTaskMemAlloc (sizeof (REMQIRESULT) * cIIDs);
    
    // For each requested interface, add it to the stub...
    for (int n=0; n < cIIDs; ++n)
        {
        pQIResults [n].hResult = pStub->interfaceAdd (iids [n],
                                                      cRefs,
                                                      &ipidNew);
        if (SUCCEEDED (pQIResults [n].hResult))
            {
            pQIResults [n].std.flags = 0;
            pQIResults [n].std.cPublicRefs = cRefs;
            pQIResults [n].std.oxid = m_oxid;
            pQIResults [n].std.oid = oid;
            pQIResults [n].std.ipid = ipidNew;
            }
        else
            {
            S_ERR (LOG_DCOM, "RemQueryInterface("
                   << vxcomGUID2String(iids[n])
                   << ") : HRESULT=0x" << pQIResults[n].hResult);
            hr = S_FALSE;
            }
        }

    // Done...
    *ppQIResults = pQIResults;
    return hr;
    }

////////////////////////////////////////////////////////////////////////////
//
// RemAddRef - first check that all referenced IPIDs are valid, if any
// aren't then return E_INVALIDARG and don't modify any refcounts. If
// they are all valid, then update the refcounts of each of the
// referenced IPIDs...
//

HRESULT ObjectExporter::RemAddRef
    (
    USHORT          cInterfaceRefs,
    REMINTERFACEREF interfaceRefs [],
    HRESULT*        pResults
    )
    {
    HRESULT hr = S_OK;
    int n;
    
    // Allocate array to cache OTEs...
    ObjectTableEntry** arrayOTEs = new ObjectTableEntry* [cInterfaceRefs];
    if (! arrayOTEs)
        return E_UNEXPECTED;

    // Scan interface refs, getting OID and caching OTE pointer...
    for (n=0; n < cInterfaceRefs; ++n)
        {
        OID oid = orpcOidFromIpid (interfaceRefs [n].ipid);
        ObjectTableEntry* pOTE = objectFindByOid (oid);
        if (pOTE)
            arrayOTEs [n] = pOTE;
        else
            {
            // Invalid IPID - we don't do any releasing, just exit
            // with E_INVALIDARG...
            hr = E_INVALIDARG;
            break;
            }
        }

    if (SUCCEEDED (hr))
        {
        // Now iterate over OTE pointers, adding refs...
        for (n=0; n < cInterfaceRefs; ++n)
            {
            arrayOTEs[n]->stdStub.AddRef (interfaceRefs[n].cPublicRefs,
                                          interfaceRefs [n].ipid);
            }
        }

    // Tidy up...
    delete [] arrayOTEs;

    return hr;
    }

////////////////////////////////////////////////////////////////////////////
//
// RemRelease - make sure we have all valid IPIDs, if not return
// E_INVALIDARG. If all okay, then remove the refs on each IPID in
// turn. If this results in zero total refs on that object, then it
// can be unregistered.
//

HRESULT ObjectExporter::RemRelease 
    (
    USHORT          cInterfaceRefs,
    REMINTERFACEREF interfaceRefs []
    )
    {
    HRESULT hr = S_OK;
    int n;
    
    // Allocate array to cache OTEs...
    ObjectTableEntry** arrayOTEs = new ObjectTableEntry* [cInterfaceRefs];
    if (! arrayOTEs)
        return E_UNEXPECTED;

    // Scan interface refs, getting OID and caching OTE pointer...
    for (n=0; n < cInterfaceRefs; ++n)
        {
        OID oid = orpcOidFromIpid (interfaceRefs [n].ipid);
        ObjectTableEntry* pOTE = objectFindByOid (oid);
        if (pOTE)
            arrayOTEs [n] = pOTE;
        else
            {
            // Invalid IPID - we don't do any releasing, just exit
            // with E_INVALIDARG...
            hr = E_INVALIDARG;
            break;
            }
        }

    if (SUCCEEDED (hr))
        {
        // Now iterate over OTE pointers, releasing refs. If the
        // result is non-zero, then there are outstanding remote refs
        // on that object, so we leave it alone. If the result is zero
        // remote refs, then we must unregister it...
        for (n=0; n < cInterfaceRefs; ++n)
            {
            ObjectTableEntry* pOTE = arrayOTEs [n];
            DWORD nr = pOTE->stdStub.Release (interfaceRefs[n].cPublicRefs,
                                              interfaceRefs [n].ipid);
            if (nr > 0)
                // Outstanding refs on this object, so we don't touch
                // it in the next scan...
                arrayOTEs [n] = 0;
            }

        // Now iterate over any non-NULL entries in arrayOTEs[] and
        // unregister them...
        for (n=0; n < cInterfaceRefs; ++n)
            {
            if (arrayOTEs [n])
                objectUnregister (arrayOTEs[n]->oid);
            }
        }

    // Tidy up...
    delete [] arrayOTEs;
    
    return hr;
    }

////////////////////////////////////////////////////////////////////////////
//
// ObjectExporter::objectMarshal -- marshal an object's interface for
// remoting. The class-ID and interface ID are recorded along with the
// other info in the object-table...
//

HRESULT ObjectExporter::objectMarshal
    (
    REFCLSID            rclsid,                // class ID
    IStream*            pStm,                // stream to marshal into
    REFIID              riid,                // interface IID
    IUnknown*           pUnk,                // interface-ptr to be marshaled
    DWORD               dwDestContext,  // destination context
    void*               pvDestContext,  // reserved for future use
    DWORD               mshlflags,        // reason for marshaling
    ObjectTableEntry**  ppOTE                // resulting table entry
    )
    {
    // See if this object wants to marshal itself (currently this is
    // only valid if the object is a StdProxy, since we don't support
    // custom marshaling)...
    IMarshal* pMshl;
    HRESULT hr = pUnk->QueryInterface (IID_IMarshal,
                                       (void**) &pMshl);
    if (SUCCEEDED (hr))
        {
        // Let the object marshal its own representation (for a
        // StdProxy, this will be a direct OBJREF to the remote
        // object, not to the proxy)...
        hr = pMshl->MarshalInterface (pStm,
                                      riid,
                                      pUnk,
                                      dwDestContext,
                                      pvDestContext,
                                      mshlflags);
        pMshl->Release ();
        if (ppOTE)
            *ppOTE = 0;
        return hr;
        }
    
    // First, see if we already have an entry for this object. To do
    // this, we must have the actual IUnknown of the object, so we
    // must QI for it first...
    IUnknown* punkActual = 0;
    hr = pUnk->QueryInterface (IID_IUnknown, (void**) &punkActual);
    if (FAILED (hr))
	return hr;
    punkActual->Release ();
    
    // Now we have the actual IUnknown for the object to be marshaled,
    // see if it already has an entry in the object table...
    ObjectTableEntry* pOTE
	= m_objectTable.objectFindByIUnknown (punkActual);

    if (! pOTE)
	{
	// No existing entry, so make one...
	pOTE = m_objectTable.objectRegister (pStm,
					     rclsid,
					     (mshlflags & MSHLFLAGS_NOPING) != 0);

	if (! pOTE)
	    return E_OUTOFMEMORY;

	S_DEBUG (LOG_OBJ_EXPORTER,
		 "Exporting Object OID=" << pOTE->oid
		 << " IID=" << vxcomGUID2String (riid));
	}
    
    // Create a new StdMarshaler object, with the OID of the object
    // being marshaled...
    pMshl = new VxStdMarshaler (pOTE->oid);

    if (! pMshl)
	return E_OUTOFMEMORY;
    
    pMshl->AddRef ();

    // Call IMarshal::MarshalInterface to marshal the object's
    // interface ptr into the stream...
    hr = pMshl->MarshalInterface (pStm,
				  riid,
				  pUnk,
				  dwDestContext,
				  pvDestContext,
				  mshlflags);

    // tidy up and return...
    if (ppOTE)
	*ppOTE = pOTE;
    pMshl->Release ();

    return hr;
    }


////////////////////////////////////////////////////////////////////////////
//

ObjectTableEntry* ObjectExporter::objectFindByOid
    (
    OID                        oid
    )
    {
    TRACE_CALL;

    return m_objectTable.objectFindByOid (oid);
    }

////////////////////////////////////////////////////////////////////////////
//

ObjectTableEntry* ObjectExporter::objectFindByStream
    (
    IStream*                pStm
    )
    {
    TRACE_CALL;
    
    return m_objectTable.objectFindByStream (pStm);
    }

////////////////////////////////////////////////////////////////////////////
//

ObjectTableEntry* ObjectExporter::objectFindByToken
    (
    DWORD                dwToken
    )
    {
    TRACE_CALL;
    
    return m_objectTable.objectFindByToken (dwToken);
    }

////////////////////////////////////////////////////////////////////////////
//

ObjectTableEntry* ObjectExporter::objectFindByIUnknown
    (
    IUnknown*           pUnk
    )
    {
    TRACE_CALL;
    
    return m_objectTable.objectFindByIUnknown (pUnk);
    }

////////////////////////////////////////////////////////////////////////////
//

bool ObjectExporter::objectUnregister (OID oid)
    {
    TRACE_CALL;

    return m_objectTable.objectUnregister (oid);
    }

////////////////////////////////////////////////////////////////////////////
//

void ObjectExporter::objectUnregisterAll ()
    {
    TRACE_CALL;

    // Safeguard against our own destruction...
    AddRef ();

    // Iterate over the table, releasing each object-table entry
    VxObjectTable::iterator i = m_objectTable.begin ();
    while (i != m_objectTable.end ())
        {
        OID oid = (*i).first;
        ++i;
        m_objectTable.objectUnregister (oid);
        }

    // Remove the safeguard...
    Release ();
    }

////////////////////////////////////////////////////////////////////////////
//
// ObjectExporter::init -- method to initialise an instance of the
// Object Exporter class.
//

HRESULT ObjectExporter::init ()
    {
    TRACE_CALL;

    // Marshal the Object Exporter's IRemUnknown interface into the
    // global object table - this allows its methods to be dispatched
    // by the normal ORPC mechanism. The marshaled interface pointer
    // will remain in the table forever...
    IStream* pStrm=0;
    HRESULT hr = VxRWMemStream::CreateInstance (0,
                                                IID_IStream,
                                                (void**) &pStrm);
    if (FAILED (hr))
        return hr;

    IRemUnknown* punk = static_cast<IRemUnknown*> (this);

    // Marshal the object exporter's IRemUnknown interface without
    // PING support, so it never expires...
    hr = CoMarshalInterface (pStrm,
                             IID_IRemUnknown,
                             punk,
                             MSHCTX_LOCAL,
                             0,
                             MSHLFLAGS_NOPING);
    pStrm->Release ();
    if (SUCCEEDED (hr))
        {
        // Create IRemUnknown IPID using  OID of just-marshaled
        // object...
        ObjectTableEntry* pOTE = objectFindByStream (pStrm);
        if (pOTE)
            {
            // Synthesise IPID of IRemUnknown...
            m_ipidRemUnknown = orpcIpidCreate (punk, pOTE->oid);

            S_DEBUG (LOG_RPC, "IPID of IRemUnknown is " << m_ipidRemUnknown);
            
            // Save our own OID...
            m_oid = pOTE->oid;
            }
        else
            hr = E_UNEXPECTED;
        }

    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//
// ObjectExporter::tick -- timeout all exported objects by 'nSecs'
// seconds. If any object expires, remove it from the table...
//

HRESULT ObjectExporter::tick (size_t nSecs)
    {
    STL_VECTOR(OID) oidsToDelete;
    
    VxCritSec cs (m_mutex);
    VxObjectTable::iterator i = m_objectTable.begin ();
    while (i != m_objectTable.end ())
        {
        ObjectTableEntry* ote = (*i).second;

        if (! ote->noPing)
            {
            ote->pingTimeout -= nSecs;
            if (ote->pingTimeout <= 0)
                {
                // Timeout has expired - mark the object for
                // destruction...
                oidsToDelete.push_back (ote->oid);
		S_DEBUG (LOG_OBJ_EXPORTER, "Timeout on object OID=" << ote->oid);
                }
            }
        ++i;
        }
    for (size_t n=0; n < oidsToDelete.size (); ++n)
        {
        S_DEBUG (LOG_OBJ_EXPORTER, "Deleting object OID=" << oidsToDelete [n] << endl);
        m_objectTable.objectUnregister (oidsToDelete [n]);
        }

    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// ObjectExporter::oidPing -- reset the timeout on a particular
// object, as it has been pinged...
//

HRESULT ObjectExporter::oidPing (OID oid)
    {
    ObjectTableEntry* pOTE = m_objectTable.objectFindByOid (oid);

    if (pOTE)
        {
        pOTE->pingTimeout = VXDCOM_PING_TIMEOUT;
        return S_OK;
        }

    return OR_INVALID_OID; 
    }


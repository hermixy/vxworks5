/* StdProxy.cpp - COM/DCOM StdProxy class implementation */

/*
modification history
--------------------
02l,03jan02,nel  Remove use of alloca.
02k,17dec01,nel  Add include symbold for diab build.
02j,10oct01,dbs  add AddKnownInterface() method to IOrpcClientChannel
02i,26jul01,dbs  use IOrpcClientChannel and IOrpcProxy interfaces
02h,13jul01,dbs  fix up includes
02g,13mar01,nel  SPR#35873. Modify code to search for first resovable address
                 in dual string passed as part of marshalling an interface.
02f,06mar01,nel  SPR#35589. Add code to make Oxid addresses unique to prevent
                 clash with other targets.
02e,05oct00,nel  SPR#34947. Correct marshling error introduced by T2 fix
                 merge.
02d,20sep00,nel  Add changes made in T2 since branch.
02c,28feb00,dbs  fix IRemUnknown facelet so it is never Release()'ed
02b,15feb00,dbs  implement IRemUnknown calls directly
02a,05aug99,dbs  change to byte instead of char
01z,09jul99,dbs  implement ping functionality in SCM now
01y,08jul99,dbs  use SCM's oxidResolve() method always
01x,06jul99,aim  change from RpcBinding to RpcIfClient
01w,30jun99,dbs  remove const-ness warnings in dtor
01v,30jun99,dbs  make Facelet-map contain smart-pointers
01u,30jun99,dbs  fix m_facelets search in RemoteQI()
01t,10jun99,dbs  remove op new and delete
01s,08jun99,dbs  remove use of mtmap
01r,07jun99,dbs  change GuidMap to mtmap
01q,03jun99,dbs  no return value from mutex lock
01p,03jun99,dbs  remove refs to comSyncLib
01o,27may99,dbs  implement Ping functionality
01n,20may99,dbs  free memory using CoTaskMemFree(), and reformat
01m,17may99,dbs  remove IID from args of interaceInfoGet
01l,13may99,dbs  change BufferInit method to interfaceInfoGet
01k,11may99,dbs  simplify proxy remoting architecture
01j,11may99,dbs  rename VXCOM to VXDCOM
01i,10may99,dbs  simplify binding-handle usage
01h,05may99,dbs  update existing RemoteOxid with resolver address
01g,28apr99,dbs  use COM_MEM_ALLOC for all classes
01f,27apr99,dbs  use new allocation calls
01e,26apr99,aim  added TRACE_CALL
01d,26apr99,dbs  add mem-pool to class
01c,22apr99,dbs  tidy up potential leaks
01b,21apr99,dbs  add length arg to orpcDSAFormat()
01a,20apr99,dbs  created during Grand Renaming

*/

/*
  DESCRIPTION:

  StdProxy -- 

*/

#include "StdProxy.h"
#include "orpcLib.h"
#include "dcomProxy.h"
#include "OxidResolver.h"
#include "PSFactory.h"
#include "NdrStreams.h"
#include "RpcStringBinding.h"
#include "RpcIfClient.h"
#include "SCM.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_StdProxy (void)
    {
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//
// Statics / globals...
//

VxStdProxy::OXID2RemApartment_t VxStdProxy::s_allProxies;
VxMutex                         VxStdProxy::s_allProxiesMutex;

//////////////////////////////////////////////////////////////////////////
//
// VxStdProxy ctor -- initialise members...
//

VxStdProxy::VxStdProxy ()
  : m_dwRefCount (0),
    m_pOxid (0),
    m_oid (0),
    m_pRemUnknown (0),
    m_pRemUnkFacelet (0)
    {
    }

//////////////////////////////////////////////////////////////////////////
// VxStdProxy dtor -- remove this proxy from records, and remove its
// OID from the ping tables in the SCM...
//

VxStdProxy::~VxStdProxy ()
    {
    // Remove this proxy from list of all known proxies
    s_allProxiesMutex.lock ();
    OXID2RemApartment_t::iterator i = s_allProxies.find (oxid ());
    if (i != s_allProxies.end ())
        (*i).second.erase (oid ());
    s_allProxiesMutex.unlock ();

    // Remove our OID from the ping list..
    SCM::theSCM()->oidDel (m_resAddr, m_oid);

    // Disconnect all facelets belonging to this proxy-object...
    FACELETMAP::iterator j;

    VxCritSec cs (m_mutex);
    
    for (j = m_facelets.begin (); j != m_facelets.end (); ++j)
	{
        // @@@ FIXME have a fake interface-proxy for IUnknown
        if ((*j).second.pProxy)
            (*j).second.pProxy->Disconnect ();
	}

    // Free the RemUnknown facelet
    if (m_pRemUnkFacelet)
        {
        m_pRemUnkFacelet->pProxy->Disconnect ();
        delete m_pRemUnkFacelet;
        }
    }


//////////////////////////////////////////////////////////////////////////
//
// UnmarshalInterface - initialises the proxy object, newly created in
// the client task, by reading the stream and extracting the OBJREF
// from it. The OBJREF is always marshaled in little-endian byte
// order, so it may have to be corrected.
//
// When unmarshaling an interface pointer, if the IID of the interface
// being unmarshaled is IID_IUnknown, then we can create a VxStdProxy
// and we don't need to QI since IUnknown is explicitly implemented
// by VxStdProxy. If we are unmarshaling another interface (not
// IUnknown), then we need to know if we already have this object (i.e.
// an instance of VxStdProxy with the same OXID and OID) proxied locally.
// If so, we should simply QI that object for the desired interface. If
// not, then we need to create a new VxStdProxy (which gives us an
// IUnknown proxy) and QI that for the desired interface. This is
// not currently implemented, however -- there don't seem to be any
// valid cases where we could get separately-marshaled interfaces
// for the same object.
//

HRESULT VxStdProxy::UnmarshalInterface
    (
    IStream*        pStm,
    REFIID          riid,
    void**          ppv
    )
    {
    TRACE_CALL;

    // Preset results...
    HRESULT hr = S_OK;
    *ppv = 0;
    
    // Unmarshal data from packet - first find out how big the
    // packet is...
    ULARGE_INTEGER len;
    pStm->Seek (0, STREAM_SEEK_END, &len);
    if (len > OBJREF_MAX)
        return RPC_E_INVALID_OBJREF;

    // Read the packet into a local buffer...
    byte mshlbuf [OBJREF_MAX];

    pStm->Seek (0, STREAM_SEEK_SET, 0);
    pStm->Read (mshlbuf, len, 0);

    // Prepare an NDR stream to assist unmarshaling
    NdrUnmarshalStream	us (NdrPhase::NOPHASE,
			    VXDCOM_DREP_LITTLE_ENDIAN,
			    mshlbuf,
			    sizeof (mshlbuf));

    // Unmarshal...
    OBJREF* pObjRef = (OBJREF*) malloc (len);
    hr = ndrUnmarshalOBJREF (&us, pObjRef);
    if (FAILED (hr))
        return hr;

    // Record OID (Object ID), OXID (object-exporter ID - this can be
    // used to look up the IPID of the IRemUnknown later) and IPID of
    // the new interface pointer...
    OID oidNew = pObjRef->u_objref.u_standard.std.oid;
    IPID ipidNew = pObjRef->u_objref.u_standard.std.ipid;
    DWORD nRefs = pObjRef->u_objref.u_standard.std.cPublicRefs;
    OXID oxidNew = pObjRef->u_objref.u_standard.std.oxid;
    DUALSTRINGARRAY* pdsa = &pObjRef->u_objref.u_standard.saResAddr;

    // Check the DUALSTRINGARRAY entries, to find one which can be
    // 'resolved' into an IP address on this machine. Win2K sometimes
    // provides hostnames as well as the IP numbers that NT usually
    // sends, so we need to scan the array to find an IP number...
    
    RpcStringBinding remoteAddr = RpcStringBinding (pdsa, VXDCOM_SCM_ENDPOINT);

    // If we couldn't resolve any addresses, we bail out now...
    if (! remoteAddr.resolve ())
        {
        free (pObjRef);
        return MAKE_HRESULT (SEVERITY_ERROR,
                             FACILITY_RPC,
                             RPC_S_INVALID_NET_ADDR);
        }

    if (remoteAddr.isLocalhost ())
	{
	// Its on this machine, so we must find the object table entry
	// corresponding to the given OXID/OID/IPID combination...
	ObjectExporter* pExp = SCM::theSCM()->objectExporter ();
	if (pExp->oxid () != oxidNew)
	    return OR_INVALID_OXID;

	ObjectTableEntry* pOTE = pExp->objectFindByOid (oidNew);
	if (! pOTE)
	    return OR_INVALID_OID;

	// Now we have the right object table entry, we must find the
	// right interface pointer corresponding to the IPID and
	// IID...
	IUnknown* punk = pOTE->stdStub.getIUnknown ();
	if (! punk)
	    return E_UNEXPECTED;

	// Now just QI for the requested interface...
	hr = punk->QueryInterface (riid, ppv);

	// Now we have an extra local ref, since the 'exported'
	// reference was never actually exported???
	}
    else
	{
	// If the OID and OXID match that of an existing proxy, then this
	// interface must belong to that object. In this case, we defer
	// the interface to that object, adding the interface-proxy to
	// that object, and calling Release() for this one so it can be
	// destroyed when the caller is done with it...
	VxStdProxy*	rcvr=0;

        // Lock 'allProxies' mutex while we search...
        s_allProxiesMutex.lock ();

        // First, find (or create, if it doesn't exist already) a
        // 'remote apartment' with the given OXID identifier...
        RemoteApartment_t& apt = s_allProxies [oxidNew];

        // Now look for an object in this apartment with the same
        // OID -- if it exists, its the same object as far as we are
        // concerned...
        RemoteApartment_t::iterator i = apt.find (oidNew);
        if (i == apt.end ())
            {
	    // No extant proxy has this OID/OXID combination, so this
	    // object must become a true proxy...
	    rcvr = this;
	    m_oid = oidNew;
	
	    // Insert into list of all known proxies...
            apt [oidNew] = this;

            // Its now safe to unlock...
            s_allProxiesMutex.unlock ();

	    // Ask the SCM to resolve the OXID and resolver-address
	    // (from the OBJREF) into a string-binding we can use to
	    // talk to the actual remote object...

	    m_resAddr = remoteAddr;

	    hr = SCM::theSCM()->oxidResolve (oxidNew,
					     remoteAddr,
					     m_pOxid);
	    if (SUCCEEDED (hr))
		{
		m_pChannel = new RpcIfClient (m_pOxid->stringBinding ());
		if (m_pChannel == 0)
		    hr = E_OUTOFMEMORY;

                // Now create a special facelet to handle the
                // IRemUnknown methods within this proxy. After
                // creation the interface proxy has one reference, and
                // when we QI it for IRemUnknown that adds a ref to
                // this StdProxy (since the interface-proxy delegates
                // its public interface's IUnknown methods to this
                // StdProxy object). However, we want to keep
                // IRemUnknown hidden, so we don't need this extra
                // ref. Hence, the Release() if the facelet creation
                // succeeds...

                IOrpcProxy* pProxy = 0;
                VxPSFactory* pPSFactory = VxPSFactory::theInstance ();

                hr = pPSFactory->CreateProxy (this, 
                                              IID_IRemUnknown,
                                              m_pOxid->ipidRemUnknown (),
                                              &pProxy);
                if (SUCCEEDED (hr))
                    {
                    // Connect the proxy...
                    pProxy->Connect (m_pChannel);
                    
                    // Now make a special facelet, and get the actual
                    // IRemUnknown pointer out, too...
                    m_pRemUnkFacelet = new facelet_t (pProxy,
                                                      0,
                                                      m_pOxid->ipidRemUnknown ()); 

                    // Now get the IRemUnknown interface we want...
                    hr = pProxy->QueryInterface (IID_IRemUnknown,
                                                 (void**) &m_pRemUnknown);
                    if (SUCCEEDED (hr))
                        Release ();

                    // Now the facelet has its own reference to the
                    // interface-proxy, so we can drop the original
                    // one we got from CreateProxy()...
                    pProxy->Release ();
                    }

		// Add our OID to the SCM's list of OIDs belonging to
		// the remote machine where our real object resides...
		SCM::theSCM()->oidAdd (m_resAddr, m_oid);
		}
            }
        else
            {
	    // Found an object with the same OID/OXID - we must add
	    // the new interface to it, and this object is just
	    // serving as an unmarshaler, so can be destroyed after
	    // this function completes its job...
	    hr = S_OK;
	    rcvr = (*i).second;
            s_allProxiesMutex.unlock ();
	    }


	// If we have a std-proxy suitable to handle this interface,
	// then add a facelet to it for the requested interface ID -
	// this will take care of registering the IPID against the
	// interface-ptr we export to the client... 
	if (SUCCEEDED (hr))
	    hr = rcvr->faceletAdd (riid, ipidNew, nRefs, ppv);
        }

    
    // Tidy up...
    free (pObjRef);

    return hr;
    }
        
//////////////////////////////////////////////////////////////////////////
//

HRESULT VxStdProxy::ReleaseMarshalData
    (
    IStream*	pStm
    )
    {
    TRACE_CALL;
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// VxStdProxy::DisconnectObject - disconnect all out-of-process clients
//
// Call Disconnect() for all interface proxies (facelets).
//

HRESULT VxStdProxy::DisconnectObject
    (
    DWORD	dwReserved
    )
    {
    if (! m_pChannel)
        return CO_E_OBJNOTCONNECTED;

    FACELETMAP::iterator i;

    VxCritSec cs (m_mutex);
    
    for (i = m_facelets.begin (); i != m_facelets.end (); ++i)
        {
        (*i).second.pProxy->Disconnect ();
        }

    return S_OK;
    }


//////////////////////////////////////////////////////////////////////////
//
// VxStdProxy::AddRef - implements AddRef() method of IUnknown
// implementation...
//

ULONG VxStdProxy::AddRef ()
    {
    return comSafeInc (&m_dwRefCount);
    }

//////////////////////////////////////////////////////////////////////////
//
// VxStdProxy::Release - implements remote reference-counting for std
// marshaling. When the last reference count is released then we
// must send a msg to the server task telling it so it can clear up.
//

ULONG VxStdProxy::Release ()
    {
    DWORD n = comSafeDec (&m_dwRefCount);

    // Is ref-count exhausted?
    if (n == 0)
	{
	// Object is about to be destroyed, we need to release
	// all refs we have on the remote object itself. We must
	// release all refs on a per-IPID basis...
	REMINTERFACEREF* pRef=0;
	int nRefs=0;

	{
	VxCritSec cs (m_mutex);

	nRefs = m_facelets.size ();
	pRef = new REMINTERFACEREF [nRefs];

	FACELETMAP::const_iterator i = m_facelets.begin ();
	for (int n=0; n < nRefs; ++n)
	    {
	    facelet_t* pFacelet = (facelet_t*) &((*i).second);
	    pRef [n].ipid = pFacelet->ipid;
	    pRef [n].cPublicRefs = pFacelet->remoteRefCount;
	    pRef [n].cPrivateRefs = 0;
	    ++i;
	    }
	}

	// Make remote call, if there are any remote refs to release
	if (nRefs)
	    {
	    HRESULT hr = m_pRemUnknown->RemRelease (nRefs, pRef);
	    if (FAILED (hr))
		S_ERR (LOG_DCOM, "RemRelease failed " << hr);
	    }

	// Tidy up and destroy this object...
	delete this;
	delete []pRef;
	}
    return n;
    }

//////////////////////////////////////////////////////////////////////////
//
// faceletFind - find a facelet for the given IID
//
// Searches the facelet-map for a facelet which implements <iid>, and
// returns a pointer to that facelet (or NULL if not found). The
// reference count of the std-proxy is the same after this operation
// as before it, although it may change during it.
//

VxStdProxy::facelet_t * VxStdProxy::faceletFind (REFIID iid)
    {
    facelet_t *pFacelet = 0;
    FACELETMAP::iterator i;

    VxCritSec cs (m_mutex);

    for (i = m_facelets.begin (); i != m_facelets.end (); ++i)
        {
        void* pv;
        
        // Is it the right interface? We borrow the 'pProxy' of each
        // facelet in turn (no need to addref/release since its scope
        // is shorter than that of the facelet) and ask it...
        IOrpcProxy* pProxy = (*i).second.pProxy;
        if (pProxy)
            {
            HRESULT hr = pProxy->QueryInterface (iid, &pv);
            if (SUCCEEDED (hr))
                {
                // Drop the ref-count after a successful QI, and return
                // this facelet instance...
                Release ();
                pFacelet = &((*i).second);
                break;
                }
            }
        }

    return pFacelet;
    }

//////////////////////////////////////////////////////////////////////////
//
// QueryInterface - implements QI functionality for the standard proxy
// object, which means that it may respond to the QI for any of the
// interfaces implemented by its facelets, or to IUnknown and IMarshal
// which it implements directly. It may make a remote call to
// IRemUnknown::RemQueryInterface() if the requested interface is not
// present in the proxy currently...
//

HRESULT VxStdProxy::QueryInterface
    (
    REFIID              iid,
    void**              ppv
    )
    {
    // Validate args...
    if (! ppv)
        return E_INVALIDARG;
    
    // If its one of our own interfaces, we can return it directly...
    if ((iid == IID_IUnknown) || (iid == IID_IMarshal))
        {
        *ppv = this;
        AddRef ();
        return S_OK;
        }

    // Is it an interface implemented by an existing facelet? If so,
    // then thats the interface we want -- if not, we must go remote

    facelet_t *pf = faceletFind (iid);
    if (pf)
        return pf->pProxy->QueryInterface (iid, ppv);

    
    // We didn't find the interface we were looking for, so we must
    // make a remote call to IRemUnknown::RemQueryInterface using the
    // most convenient IPID we have to hand, which happens to be the
    // first in the list...

    IPID ipid =  (*(m_facelets.begin ())).second.ipid;
    REMQIRESULT* pQIResult;
    HRESULT hr = m_pRemUnknown->RemQueryInterface (ipid,
                                                   5,
                                                   1,
                                                   &iid,
                                                   &pQIResult);
    if (FAILED (hr))
        return hr;

    // If the OID of the new interface isn't ours, then something went
    // horribly wrong...
    if (pQIResult->std.oid == m_oid)
        {
        hr = faceletAdd (iid,
                         pQIResult->std.ipid,
                         pQIResult->std.cPublicRefs,
                         ppv);
        }
    else
        hr = RPC_E_INVALID_OBJECT;

    // Tidy up the returned results...
    CoTaskMemFree (pQIResult);

    // Tell the channel that this interface is definitely known to be
    // available at the other end...
    m_pChannel->AddKnownInterface (iid);

    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//
// faceletAdd -- adds a (or operates on an existing) facelet for the
// given interface (IID) to the VxStdProxy object. The
// remote-ref-count for that IPID is bumped up by the given amount.
//

HRESULT VxStdProxy::faceletAdd
    (
    REFIID              iid,
    REFIPID             ipid,
    DWORD               nRefs,
    void**              ppv
    )
    {
    HRESULT	hr = S_OK;;
    void*	pvResult=0;
    
    // Look for existing facelet on this proxy with the right IID.
    // We need to do a linear search of m_facelets, and if we find the
    // right one, we need to get its 'iid' interface, too. This adds
    // an extra reference to the proxy, as well...

    facelet_t* pFacelet = faceletFind (iid);

    // Did we find one? If not, we need to create one...
    if (pFacelet)
        {
        // Get the requested interface, adding a local ref. Note that
        // this will *never* be called for IUnknown, since QI's for
        // IUnknown are handled directly by the StdProxy itself...
        COM_ASSERT(iid != IID_IUnknown);
        hr = pFacelet->pProxy->QueryInterface (iid, &pvResult);

        // Add the remote refs...
	pFacelet->remoteRefCount += nRefs;
        }
    else
	{
	// Check for explicit IUnknown special case - we must make an
	// entry for the IUnknown interface in the 'm_facelets' table
	// now it's IPID is known, but we don't give out the pointer
	// to the facelet, instead we return the StdProxy's IUnknown,
	// and the facelet is used only to count remote refs...

        IOrpcProxy* pProxy=0;
            
	if (iid == IID_IUnknown)
	    {
            // For IUnknown, we don't create an interface-proxy, and
            // we return the primary IUnknown of this StdProxy...
            
	    AddRef ();
	    pvResult = static_cast<IUnknown*> (this);
	    }
	else    
	    {
	    // For other IIDs, we need to create an interface-proxy
	    // from the factory. This results in one ref in pProxy,
            // which we must drop later...
            
	    VxPSFactory* pPSFactory = VxPSFactory::theInstance ();

            hr = pPSFactory->CreateProxy (this, iid, ipid, &pProxy);
            if (FAILED (hr))
                return hr;

            // QI the proxy for the desired interface...
            hr = pProxy->QueryInterface (iid, &pvResult);
            if (FAILED (hr))
                {
                pProxy->Release ();
                return hr;
                }
            
            // Connect the proxy to the channel...
            pProxy->Connect (m_pChannel);
            }

        // Create a facelet to refer to the interface-proxy...
	VxCritSec cs (m_mutex);
	m_facelets [pvResult] = facelet_t (pProxy, nRefs, ipid);

        // Now drop the ref we got from CreateProxy()...
        if (pProxy)
            pProxy->Release ();
	}

    // Return resulting interface ptr...
    if (ppv)
	*ppv = pvResult;

    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//

HRESULT VxStdProxy::GetUnmarshalClass
    (
    REFIID, 
    void*, 
    DWORD, 
    void*, 
    DWORD, 
    CLSID*
    )
    {
    TRACE_CALL;
    return E_NOTIMPL; 
    }
        
//////////////////////////////////////////////////////////////////////////
//

HRESULT VxStdProxy::GetMarshalSizeMax
    (
    REFIID          iid,
    void*           pv,
    DWORD           dwDestContext,
    void*           pvDestContext,
    DWORD           mshlflags,
    DWORD*          pSize
    )
    {
    TRACE_CALL;
    *pSize = OBJREF_MAX;
    return S_OK;
    }
        

//////////////////////////////////////////////////////////////////////////
//
// MarshalInterface -- this method hands out one of our existing
// remote interface-refs to another client, so we decrement the
// ref-count for that IPID. If the remaining reference count has
// dropped below 2 (i.e. to one) then we must go ask for some more
// refs from the real object, via RemAddRef(). It is usual to ask for
// 5 at a time...
//

HRESULT VxStdProxy::MarshalInterface
    (
    IStream*            pStrm, 
    REFIID              iid,
    void*               pvInterface, 
    DWORD               dwDestCtx,
    void*               pvDestCtx,
    DWORD               mshlflags
    )
    {
    HRESULT             hr = S_OK;

    // Look for a facelet supporting the requested interface...
    facelet_t* pFacelet = faceletFind (iid);
    if (! pFacelet)
        return E_NOINTERFACE;

    // Get one reference from the facelet - if its ref-count drops to
    // zero then we must request more from the original object...
    if (pFacelet->remoteRefCount < 2)
        {
        REMINTERFACEREF ref;
        HRESULT hrAddRef;
        
        // Invoke RemAddRef() to get more refs...
        ref.ipid = pFacelet->ipid;
        ref.cPublicRefs = 5;
        ref.cPrivateRefs = 0;
        hr = m_pRemUnknown->RemAddRef (1, &ref, &hrAddRef);
        if (FAILED (hr))
            return hr;
        if (FAILED (hrAddRef))
            return hrAddRef;

        pFacelet->remoteRefCount += ref.cPublicRefs;
        }
    
    // Now drop the one we want to give out...
    pFacelet->remoteRefCount -= 1;
    
    // We need to get the OXID-resolver address associated with our
    // remote object exporter, to fill in the OBJREF correctly.
    // Fortunately, we cached it when we unmarshaled our original
    // OBJREF...
    BSTR bsResAddr=0;
    
    if (m_resAddr.format (&bsResAddr, false) != 0)
        return E_FAIL;
    
    // Fill in the OBJREF for this object...
    OLECHAR wszSecInfo [] = { 0x0A, 0xFFFF, 0 };
    const DWORD dsaLen = sizeof (DUALSTRINGARRAY) +
                         (2 * SysStringLen (bsResAddr)) +
                         (2 * vxcom_wcslen (wszSecInfo)) +
                         16;
    OBJREF* pObjRef = (OBJREF*) malloc (sizeof (OBJREF) + dsaLen);
    if (! pObjRef)
        return E_OUTOFMEMORY;
    
    pObjRef->signature = OBJREF_SIGNATURE;
    pObjRef->flags = OBJREF_STANDARD;
    pObjRef->iid = iid;
    pObjRef->u_objref.u_standard.std.flags = 0;
    pObjRef->u_objref.u_standard.std.cPublicRefs = 1;
    pObjRef->u_objref.u_standard.std.oxid = m_pOxid->oxid ();
    pObjRef->u_objref.u_standard.std.oid = m_oid;
    pObjRef->u_objref.u_standard.std.ipid = pFacelet->ipid;

    // Fill in the DSA with the address info from the resolver...
    hr = orpcDSAFormat (&pObjRef->u_objref.u_standard.saResAddr,
                        dsaLen,
                        bsResAddr,
                        wszSecInfo);
    if (SUCCEEDED (hr))
        {
        // Now marshal the OBJREF into the marshaling packet, so it can be
        // accessed by the proxy...
        NdrMarshalStream ms (NdrPhase::NOPHASE,
                             VXDCOM_DREP_LITTLE_ENDIAN);
    
        hr = ndrMarshalOBJREF (&ms, pObjRef);
        if (SUCCEEDED (hr))
            hr = pStrm->Write (ms.begin (), ms.size (), 0);
        }

    // Tidy up
    free (pObjRef);

    // Tidy up...
    return hr;
    }
        






/* StdMarshaler.cpp - COM/DCOM StdMarshaler class implementation */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01y,17dec01,nel  Add include symbol for diab.
01x,13jul01,dbs  fix up includes
01w,16jul99,aim  replace resolverAddressGet => rpcAddressFormat
01v,09jul99,dbs  change to new ObjectExporter naming
01u,29jun99,dbs  make StdStub a member of ObjectTableEntry
01t,17jun99,aim  uses new SCM
01s,10jun99,dbs  remove op new and delete
01r,03jun99,dbs  no return value from mutex lock
01q,03jun99,dbs  remove refs to comSyncLib
01p,03jun99,dbs  remove reliance on TLS from CoMarshalInterface
01o,24may99,dbs  ask SCM for local object exporter
01n,20may99,dbs  move NDR phase into streams
01m,18may99,dbs  remove old marshaling scheme
01l,11may99,dbs  rename VXCOM to VXDCOM
01k,07may99,dbs  fix up call to SCM method
01j,30apr99,dbs  change name of ComSCM to SCM
01i,29apr99,dbs  fix -Wall warnings
01h,28apr99,dbs  use COM_MEM_ALLOC for all classes
01g,27apr99,dbs  use new allocation calls
01f,27apr99,dbs  add mem-pool to class
01e,26apr99,aim  added TRACE_CALL
01d,23apr99,dbs  use NEW() to create objects
01c,22apr99,dbs  tidy up potential leaks
01b,21apr99,dbs  add length arg to orpcDSAFormat()
01a,20apr99,dbs  created during Grand Renaming

*/

/*
  DESCRIPTION:

  StdMarshaler -- 

*/

#include "StdMarshaler.h"
#include "RemoteActivation.h"
#include "ObjectExporter.h"
#include "StdStub.h"
#include "SCM.h"
#include "dcomProxy.h"
#include "orpcLib.h"
#include "private/comMisc.h"
#include "NdrStreams.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_StdMarshaler (void)
    {
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//
// GetUnmarshalClass - for the std proxy, the unmarshaling class is
// always CLSID_StdMarshal, for the supported interface types...
//

HRESULT VxStdMarshaler::GetUnmarshalClass
    (
    REFIID	riid,
    void*	pv,
    DWORD 	dwDestContext,
    void*	pvDestContext,
    DWORD	mshlflags,
    CLSID*	pClsid
    )
    {
    TRACE_CALL;
    if ((riid == IID_IUnknown) ||
	(riid == IID_IClassFactory))
	{
	*pClsid = CLSID_StdMarshal;
	return S_OK;
	}
    return E_NOTIMPL;
    }

//////////////////////////////////////////////////////////////////////////
//
// GetMarshalSizeMax - returns max size of std marshaling packet...
//

HRESULT VxStdMarshaler::GetMarshalSizeMax
    (
    REFIID	riid,
    void*	pv,
    DWORD	dwDestContext,
    void*	pvDestContext,
    DWORD	mshlflags,
    DWORD*	pSize
    )
    {
    TRACE_CALL;
    *pSize = OBJREF_MAX;
    return S_OK;
    }
        
//////////////////////////////////////////////////////////////////////////
//
// MarshalInterface - does the real business of marshaling into the
// given stream enough information to construct a proxy in the client
// process. This means creating an RPC channel object, and writing its
// identifier into the stream (the VxWorks single address space
// architecture ensures that the interface ptr is still valid in the
// client task).
//
// Then, an entry is made into the GOT (Global Object Table), which
// will be used when unmarshaling. This entry pre-dates the entry
// that will be made by CoMarshalInterface anyway, and so if that
// function finds there is already an entry it will be re-used...
//
// The std marshaling packet looks like:-
//   1. OBJREF of marshaled interface
//   2. mshlflags
//
// Note that the RPC-channel for the server is not created until 
// a client tries to connect to the object-exporter exporting that
// object.
//

HRESULT VxStdMarshaler::MarshalInterface
    (
    IStream*	pStm,
    REFIID	riid,
    void*	pvInterface,
    DWORD	dwDestContext,
    void*	pvDestContext,
    DWORD	mshlflags
    )
    {
    TRACE_CALL;

    OBJREF*	pObjRef = 0;
    IPID        ipidNew = GUID_NULL;
    BSTR        bsResAddr;
    OLECHAR	bsSecInfo [] = { 0x0A, 0xFFFF, 0 };
    const int   cRefs = 5;

    // Get 'SCM addesss binding.
    HRESULT hr = SCM::theSCM ()->addressBinding (&bsResAddr);

    if (FAILED (hr))
	return hr;
    
    // Find OXID of our exporter
    ObjectExporter* px = SCM::objectExporter ();
    OXID oxid = px->oxid ();

    // Update the Exporters object-table...
    ObjectTableEntry* pOTE = px->objectFindByOid (m_oid);
    if (pOTE)
	{
	pOTE->stdStub.adopt (reinterpret_cast<IUnknown*> (pvInterface),
			     m_oid);

	hr = pOTE->stdStub.interfaceAdd (riid, cRefs, &ipidNew);
	if (FAILED (hr))
	    return hr;
	}
    
    // Fill in the OBJREF for this object...
    const DWORD dsaLen = sizeof (DUALSTRINGARRAY) +
			 (2 * SysStringLen (bsResAddr)) +
			 (2 * vxcom_wcslen (bsSecInfo)) +
			 16;
			       
    pObjRef = (OBJREF*) malloc (sizeof (OBJREF) + dsaLen);
    if (! pObjRef)
	return E_OUTOFMEMORY;

    pObjRef->signature = OBJREF_SIGNATURE;
    pObjRef->flags = OBJREF_STANDARD;
    pObjRef->iid = riid;
    pObjRef->u_objref.u_standard.std.flags = 0;
    pObjRef->u_objref.u_standard.std.cPublicRefs = cRefs;
    pObjRef->u_objref.u_standard.std.oxid = oxid;
    pObjRef->u_objref.u_standard.std.oid = m_oid;
    pObjRef->u_objref.u_standard.std.ipid = ipidNew;

    // Fill in the DSA with the address info from the resolver...
    hr = orpcDSAFormat (&pObjRef->u_objref.u_standard.saResAddr,
			dsaLen,
			bsResAddr,
			bsSecInfo);

    // Now marshal the OBJREF into the marshaling packet, so it can be
    // accessed by the proxy...
    NdrMarshalStream ms (NdrPhase::PROXY_MSHL,
			 VXDCOM_DREP_LITTLE_ENDIAN);
    
    hr = ndrMarshalOBJREF (&ms, pObjRef);
    if (FAILED (hr))
	return hr;
    hr = pStm->Write (ms.begin (), ms.size (), 0);
    if (FAILED (hr))
	return hr;
    
    // Tidy up
    free (pObjRef);
    SysFreeString (bsResAddr);

    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//
// VxStdMarshaler::AddRef - add a reference
//

ULONG VxStdMarshaler::AddRef ()
    {
    TRACE_CALL;
    m_mutex.lock ();
    ++m_dwRefCount;
    m_mutex.unlock ();
    return m_dwRefCount;
    }

//////////////////////////////////////////////////////////////////////////
//
// VxStdMarshaler::Release - remove a reference, destroy if no refs
// remain extant...
//

ULONG VxStdMarshaler::Release ()
    {
    TRACE_CALL;

    m_mutex.lock ();
    DWORD n = --m_dwRefCount;
    m_mutex.unlock ();
    if (n == 0)
	delete this;
    return n;
    }

//////////////////////////////////////////////////////////////////////////
//
// QueryInterface - implements QI functionality for the standard
// marshaler object, which only supports IUnknown and IMarshal...
//

HRESULT VxStdMarshaler::QueryInterface
    (
    REFIID	riid,
    void**	ppv
    )
    {
    TRACE_CALL;
    // Is it one of our own interfaces?
    if ((riid == IID_IUnknown) || (riid == IID_IMarshal))
	{
	*ppv = this;
        AddRef ();
	return S_OK;
	}
    return E_NOINTERFACE;
    }



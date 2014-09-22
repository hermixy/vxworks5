/* StdStub.cpp - COM/DCOM StdStub class implementation */

/*
modification history
--------------------
02f,17dec01,nel  Add include symbol for StdStub.
02e,31jul01,dbs  import full T2 changes for stublet refcounts
02d,13jul01,dbs  fix up includes
02c,20sep00,nel  Add changes made in T2 since branch.
02b,28feb00,dbs  add fake stub disp table for IUnknown
02a,02aug99,dbs  return correct err code when no stublet
01z,27jul99,drm  Returning CLSID from interfaceInfoGet().
01y,09jul99,dbs  change to new ObjectExporter naming
01x,08jul99,dbs  remove print statements
01w,29jun99,dbs  no longer need to unregister StdStub in dtor
01v,28jun99,dbs  make Relase() return refs so deletion can be done by OX
01u,17jun99,aim  uses new SCM
01t,10jun99,dbs  remove op new and delete
01s,08jun99,dbs  remove use of mtmap
01r,04jun99,dbs  change GuidMap to mtmap
01q,03jun99,dbs  no return value from mutex lock
01p,02jun99,dbs  inherit from RpcDispatchTable
01o,26may99,dbs  fix remote ref-count for IUnknown stublet
01n,25may99,dbs  fix stublet refcount for IUnknown stublet
01m,24may99,dbs  ask SCM for local object exporter
01l,18may99,dbs  change to new marshaling architecture
01k,11may99,dbs  fix pure-virtual call problem
01j,11may99,dbs  simplify stub remoting architecture
01i,07may99,dbs  fix method calls to exporter
01h,29apr99,dbs  fix -Wall warnings
01g,28apr99,dbs  use COM_MEM_ALLOC for all classes
01f,26apr99,aim  added TRACE_CALL
01e,26apr99,dbs  remove class-specific debug code
01d,26apr99,dbs  add mem-pool to StubInfo class
01c,23apr99,dbs  add mem-pool to class
01b,22apr99,dbs  tidy up potential leaks
01a,20apr99,dbs  created during Grand Renaming

*/

/*
  DESCRIPTION:

  StdStub -- 

*/

#include "StdStub.h"
#include "Stublet.h"
#include "SCM.h"
#include "orpcLib.h"
#include "PSFactory.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_StdStub (void)
    {
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//
// VxStdStub::VxStdStub -- default ctor...
//

VxStdStub::VxStdStub () : m_pUnkServer (0), m_oid (0)
    {
    }


//////////////////////////////////////////////////////////////////////////
//
// VxStdStub destructor --
//

VxStdStub::~VxStdStub ()
    {
    TRACE_CALL;

    // Release all the stublets associated with this instance of
    // VxStdStub...

    VxCritSec cs (m_mutex);
    STUBLETMAP::iterator i = m_stublets.begin ();
    while (i != m_stublets.end ())
	{
	VxStublet* pStublet = (*i).second;
	if (pStublet)
	    delete pStublet;
	++i;
	}
    
    // Release the server object we are representing
    if (m_pUnkServer)
        m_pUnkServer->Release ();
    }


//////////////////////////////////////////////////////////////////////////
//

ULONG VxStdStub::AddRef (ULONG n, REFIPID ipid)
    {
    TRACE_CALL;

    VxCritSec cs (m_mutex);
    STUBLETMAP::const_iterator i = m_stublets.find (ipid);
    if (i != m_stublets.end ())
	{
	VxStublet* pStublet = (*i).second;
	pStublet->addRefs (n);
	}
    
    return n;
    }

//////////////////////////////////////////////////////////////////////////
//

ULONG VxStdStub::Release (ULONG n, REFIPID ipid)
    {
    TRACE_CALL;
    ULONG nRefs=1;

    // Only take action if we have some refs to release...
    if (n)
	{
	VxCritSec cs (m_mutex);
    
	STUBLETMAP::const_iterator i = m_stublets.find (ipid);

	if (i != m_stublets.end ())
	    {
	    VxStublet* pStublet = (*i).second;
	    pStublet->relRefs (n);
	    }

	// Now add up remote refs, and destroy stub if zero...
	nRefs = 0;
	for (i = m_stublets.begin (); i != m_stublets.end (); ++i)
	    {
	    VxStublet* pStublet = (*i).second;
	    nRefs += pStublet->refs ();
	    }
	}
    
    return nRefs;
    }

//////////////////////////////////////////////////////////////////////////
//
// interfaceAdd -- adds an interface-stub (a 'stublet') for the
// given IID on the stubbed object, and adds 'nRefs' remote references
// for it. Returns the IPID of the interface if requested.
//
// As objects typically re-use one of their interfaces as their
// IUnknown we must always check the interface-ptr to see if an IPID
// for it already exists (IPID values are repeatable, given an OID
// value and an interface pointer) - if not we make a new entry.
//
// Also, there is no interface-stub (stublet) for the IUnknown interface
// as the StdStub provides this functionality, so IUnknown is treated
// as a special case here.
//
// Note the this StdStub object keeps one reference to the main
// IUnknown of the server object in m_pUnkServer, and each stublet
// also keeps one reference to the particular interface it is
// representing, so even an object with only one interface will have 2
// references to it when being remoted - one by the StdStub and one by
// the stublet for its primary interface.
//

HRESULT VxStdStub::interfaceAdd
    (
    REFIID      iid,
    DWORD       nRefs,
    IPID*       pIpidNew
    )
    {
    TRACE_CALL;

    HRESULT hr = S_OK;
    VxStublet* pStublet=0;
    VxStublet* pStubletTmp=0;
    
    // First, we must generate a unique IPID for this interface-ptr...
    IUnknown* pOther;
    hr = m_pUnkServer->QueryInterface (iid, (void**) &pOther);
    if (FAILED (hr))
	return hr;
    
    // Using the other interface-ptr, we can create a
    // unique IPID value...
    IPID ipid = orpcIpidCreate (pOther, m_oid);

    // See if we already have a Stublet for this interface - we need
    // to look in the per-IPID first, as that is the unique case...
    
    {
    VxCritSec cs (m_mutex);

    STUBLETMAP::const_iterator i = m_stublets.find (ipid);
    if (i != m_stublets.end ())
	{
	// there is already a stublet for this interface
	pStublet = (*i).second;

	// If it is a stublet for IUnknown, we need to convert it to
	// the new interface ID so it can dispatch methods beyond the
	// basic 3 methods of IUnknown. The simplest way of achieving
	// this is to erase it and make a new stublet...
	if (pStublet->iid () == IID_IUnknown)
	    {
	    // Save the remote refs we already had on the IUnknown ptr
	    // of the server-object...
	    pStubletTmp = pStublet;
	    nRefs += pStubletTmp->refs ();

	    // erase the table-entries
	    m_stublets.erase (ipid);
	    m_interfaces.erase (IID_IUnknown);

	    // make sure we re-create the stublet...
	    pStublet = 0;
	    }
	}

    // Now create a new stublet if required...
    if (pStublet == 0)
	{
	// We don't have a stublet, so we must create one. We don't
	// release the one reference we got through QI since this will
	// be held by the stublet itself, and released when the object
	// is destroyed...
	
	VxPSFactory* pPSFactory = VxPSFactory::theInstance ();
	pStublet = pPSFactory->CreateStublet (pOther,
					      iid,
					      ipid);
	if (pStublet)
	    {
	    VxCritSec cs2 (m_mutex);
	    m_stublets [ipid] = pStublet;
	    }
	else
	    hr = E_NOINTERFACE;
	}
    }

    // Common operations - add references to the stublet, and make
    // sure it has an entry in the per-interface table...
    if (pStublet)
	{
	VxCritSec cs3 (m_mutex);
	
	// make IID-related entry
	m_interfaces [iid] = pStublet;

	// add new remote-refs
	pStublet->addRefs (nRefs);
    
	// return the IPID value
	if (pIpidNew)
	    *pIpidNew = pStublet->ipid ();
	}

    // Get rid of the old stublet (if we had one)...
    if (pStubletTmp)
	delete pStubletTmp;
    
    // Release the other interface pointer, as the stublet will
    // maintain its own reference to the interface...
    pOther->Release ();
    
    return hr;
    }


//////////////////////////////////////////////////////////////////////////
//
// VxStdStub::interfaceInfoGet -- get the interface pointer and
// stub-function pointer for the given IPID value from its appropriate
// stublet...
//

HRESULT VxStdStub::interfaceInfoGet
    (
    REFIID              /*riid*/,
    REFIPID		ipid,
    ULONG		opnum,
    IUnknown**		ppunk,
    PFN_ORPC_STUB*	ppfn,
    CLSID &		classid
    )
    {
    TRACE_CALL;

    VxStublet*		pStublet = 0;
    HRESULT		hr = S_OK;

    // Make sure its one of our IPIDs --  look for per-stublet info,
    // based on IPID...
    {    
    VxCritSec cs (m_mutex);
    
    STUBLETMAP::const_iterator i = m_stublets.find (ipid);
    if (i == m_stublets.end ())
	hr = RPC_E_INVALID_IPID;
    else
	pStublet = (*i).second;
    }
    
    // Now find the interface ptr
    if (pStublet && SUCCEEDED (hr))
	hr = pStublet->stubInfoGet (opnum, ppunk, ppfn);

    classid = GUID_NULL;

    return hr;
    }


//////////////////////////////////////////////////////////////////////////
//
// VxStdStub::supportsInterface -- determine whether this stub object
// supports the given interface (by IID)...
//

bool VxStdStub::supportsInterface (REFIID riid)
    {
    TRACE_CALL;

    VxCritSec cs (m_mutex);
    STUBLETMAP::const_iterator i = m_interfaces.find (riid);
    return (i != m_interfaces.end ());
    }

//////////////////////////////////////////////////////////////////////////
//
// VxStdStub::adopt -- adopt a server-object to be 'exported'. If we
// haven't already got its IUnknown then we find it now, and thus hold
// one reference to it via m_pUnkServer. This reference will be
// released when this StdStub is destroyed...
//

void VxStdStub::adopt (IUnknown* punk, OID oid)
    {
    if (! m_pUnkServer)
        {
        punk->QueryInterface (IID_IUnknown,
			  (void**) &m_pUnkServer);
        m_oid = oid;
        }
    }


//////////////////////////////////////////////////////////////////////////
//
// Vtbl and stub dispatch-table for IUnknown -- these are never used,
// they are just here to fit into the same architecture as all the
// other _ps files. PSfactory requires them in order to be able to
// return a Facelet for IUnknown...
//

const PFN_ORPC_STUB IUnknown_vxstub_functbl [] = 
    { 
    0,0,0,
    };

const VXDCOM_STUB_DISPTBL IUnknown_vxstub_disptbl = 
    {
    3, 
    IUnknown_vxstub_functbl
    };

const int IUnknown_vxproxy_vtbl=0;

VXDCOM_PS_AUTOREGISTER(IUnknown);

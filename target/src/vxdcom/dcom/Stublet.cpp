/* Stublet.cpp - COM/DCOM Stublet class implementation */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01o,17dec01,nel  Add include symbol for diab build.
01n,13jul01,dbs  fix up includes
01m,05mar01,nel  SPR#35701. exported objects have too many refcounts.
01l,10jun99,dbs  remove op new and delete
01k,04jun99,dbs  add const to vtbl ctor arg
01j,28may99,dbs  make stub disp-tbl a structure
01i,27may99,dbs  check method number is in range
01h,25may99,dbs  Stublet no longer keeps refs to server
01g,18may99,dbs  change to new marshaling architecture
01f,14may99,dbs  change stub-function typename
01e,11may99,dbs  simplify stub remoting architecture
01d,28apr99,dbs  use COM_MEM_ALLOC for all classes
01c,27apr99,dbs  add mem-pool to class
01b,26apr99,aim  added TRACE_CALL
01a,20apr99,dbs  created during Grand Renaming

*/

/*
  DESCRIPTION:

  VxStublet -- records the interface-related into for one interface
  pointer for one (server) object. Its lifetime is always shorter than
  that of the object it is masquerading as, and always shorter than
  its owning StdStub object, so it doesn't keep any references to the
  server object at all.

*/

#include "Stublet.h"
#include "private/comMisc.h"
#include "TraceCall.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_Stublet (void)
    {
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//
// VxStublet method implementations. The stublet maintains a reference
// to the interface it is representing, as long as it lives. This
// ensures that even objects with 'tear-off' or transient interfaces
// can be remoted, as once they have handed out a reference to any
// interface, the stublet for that interface will maintain one
// reference to it until the object is completely destroyed.
//
//////////////////////////////////////////////////////////////////////////

VxStublet::VxStublet
    (
    IUnknown*			punkServer,
    REFIID			iid,
    REFIPID			ipid,
    const VXDCOM_STUB_DISPTBL*	dispTbl
    )
      : m_dwRefCount (0),
	m_punkServer (punkServer),
	m_iidServer (iid),
	m_ipid (ipid),
	m_pDispTbl (dispTbl)
    {
    TRACE_CALL;
    if (m_punkServer)
        m_punkServer->AddRef ();
    }

//////////////////////////////////////////////////////////////////////////
//

VxStublet::~VxStublet ()
    {
    TRACE_CALL;
    if (m_punkServer)
        m_punkServer->Release ();
    }


//////////////////////////////////////////////////////////////////////////
//

ULONG VxStublet::addRefs (ULONG nRefs)
    {
    TRACE_CALL;

    m_mutex.lock ();
    m_dwRefCount += nRefs;
    m_mutex.unlock ();
    return m_dwRefCount;
    }


//////////////////////////////////////////////////////////////////////////
//

ULONG VxStublet::relRefs (ULONG nRefs)
    {
    TRACE_CALL;

    m_mutex.lock ();
    if (nRefs > m_dwRefCount)
	m_dwRefCount = 0;
    else
	m_dwRefCount -= nRefs;
    m_mutex.unlock ();
    return m_dwRefCount;
    }

//////////////////////////////////////////////////////////////////////////
//
// VxStublet::stubInfoGet -- gives out the object's interface pointer,
// so in this case it *does* AddREf() it before handing it out. It is
// the caller's responsibility to Release() the interface later...
//

HRESULT VxStublet::stubInfoGet
    (
    ULONG		opnum,
    IUnknown**		ppunk,
    PFN_ORPC_STUB*	ppfn
    )
    {
    if (opnum >= m_pDispTbl->nFuncs)
	return MAKE_HRESULT (SEVERITY_ERROR,
			     FACILITY_RPC,
			     RPC_S_PROCNUM_OUT_OF_RANGE);
    
    m_punkServer->AddRef ();
    *ppunk = m_punkServer;
    *ppfn = m_pDispTbl->funcs [opnum];
    return S_OK;
    }



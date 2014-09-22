/* PSFactory.cpp - COM/DCOM PSFactory class implementation */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01r,17dec01,nel  Add include symbol for diab.
01q,25jul01,dbs  revise facelet/proxy classes
01p,13jul01,dbs  fix up includes
01o,30jul99,aim  fixed compiler warning
01n,30jul99,dbs  CreateStublet() must return NULL if no p/s found
01m,30jun99,dbs  create facelets with zero ref-count
01l,08jun99,dbs  remove use of mtmap
01k,04jun99,dbs  change GuidMap to mtmap
01j,03jun99,dbs  no return value from mutex lock
01i,28may99,dbs  make stub disp-tbl a structure
01h,14may99,dbs  use new stub-func type
01g,11may99,dbs  simplify proxy remoting architecture
01f,11may99,dbs  simplify stub remoting architecture
01e,29apr99,dbs  fix -Wall warnings
01d,27apr99,dbs  make PSFactory a true singleton
01c,27apr99,dbs  add mem-pool to classes
01b,26apr99,aim  added TRACE_CALL
01a,20apr99,dbs  created during Grand Renaming

*/

/*
  DESCRIPTION:

  PSFactory -- 

*/

#include "private/comMisc.h"
#include "PSFactory.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_PSFactory (void)
    {
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//

VxStublet* VxPSFactory::CreateStublet
    (
    IUnknown*		punkServer,
    REFIID		iid,
    REFIPID		ipid
    )
    {
    VxCritSec critSec (m_mutex);
    PSMAP::const_iterator i = m_psMap.find (iid);
    if (i == m_psMap.end ())
	return 0;

    return new VxStublet (punkServer,
			  iid,
			  ipid,
			  (*i).second.pStubDispTbl);
    }
    
//////////////////////////////////////////////////////////////////////////
//
// Register a proxy/stub pair with the factory...
//

HRESULT VxPSFactory::Register
    (
    REFIID			iid,
    const void*			pvProxyVtbl,
    const VXDCOM_STUB_DISPTBL*	pStubDispTbl
    )
    {
    VxCritSec cs (m_mutex);
    m_psMap [iid] = psentry (pvProxyVtbl, pStubDispTbl);
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// CreateProxy -- search the factory's table of registered p/s IIDs
// until it finds one that matches. If so, return the interface proxy
// (facelet) aggregated into the supplied VxStdProxy.
//

HRESULT VxPSFactory::CreateProxy
    (
    IUnknown *      pUnkOuter,
    REFIID          iid,
    REFIPID         ipid,
    IOrpcProxy**    ppProxy
    )
    {
    const void* pvVtbl = 0;
    
    // Search for registered interface...
    {
    VxCritSec critSec (m_mutex);
    PSMAP::const_iterator i = m_psMap.find (iid);
    if (i != m_psMap.end ())
	pvVtbl = (*i).second.pvProxyVtbl;
    }
	
    // Make sure we were given somewhere to put it...
    if (ppProxy)
        {
        VxInterfaceProxy* p = new VxInterfaceProxy (iid,
                                                    ipid,
                                                    pUnkOuter,
                                                    pvVtbl);
        *ppProxy = p;
        p->AddRef ();
        }
        
    return S_OK;
    }


//////////////////////////////////////////////////////////////////////////
//

VxPSFactory* VxPSFactory::theInstance ()
    {
    static VxPSFactory	s_theFactory;

    return &s_theFactory;
    }



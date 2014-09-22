/* PSFactory.h - COM/DCOM PSFactory class definition */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01n,25jul01,dbs  revise facelet/proxy classes
01m,13jul01,dbs  fix up includes
01l,08jun99,dbs  remove use of mtmap
01k,04jun99,dbs  change GuidMap to mtmap
01j,28may99,dbs  make stub disp-tbl a structure
01i,14may99,dbs  use new stub-func type
01h,11may99,dbs  simplify proxy remoting architecture
01g,11may99,dbs  change name of ChannelBuffer.h to Remoting.h
01f,11may99,dbs  simplify stub remoting architecture
01e,29apr99,dbs  fix -Wall warnings
01d,27apr99,dbs  make PSFactory a true singleton
01c,27apr99,dbs  add mem-pool to classes
01b,21apr99,dbs  add include for IRpcChannelBuffer
01a,20apr99,dbs  created during Grand Renaming

*/


#ifndef __INCPSFactory_h
#define __INCPSFactory_h

#include "dcomLib.h"
#include "private/comMisc.h"
#include "private/comStl.h"
#include "Stublet.h"
#include "InterfaceProxy.h"

///////////////////////////////////////////////////////////////////////////
//
// VxPSFactory - proxy/stub factory class - there is only ever one of
// these objects, and it remains around forever...
//

struct psentry
    {
    const void*			pvProxyVtbl;
    const VXDCOM_STUB_DISPTBL*	pStubDispTbl;

    psentry (const void* pp, const VXDCOM_STUB_DISPTBL* ps)
      : pvProxyVtbl (pp), pStubDispTbl (ps)
	{}

    psentry () : pvProxyVtbl (0), pStubDispTbl (0)
	{}
    };

class VxPSFactory
    {

    typedef STL_MAP(IID, psentry) PSMAP;

    PSMAP	m_psMap;
    VxMutex	m_mutex;
    
    VxPSFactory () {}

  public:

    static VxPSFactory* theInstance ();
    
    virtual ~VxPSFactory () {}

    // Register a proxy/stub pair with the factory...
    HRESULT Register
	(
	REFIID 				iid,
	const void*			pvProxyVtbl,
	const VXDCOM_STUB_DISPTBL*	pStubDispTbl
	);

    // Create a stublet...
    VxStublet* CreateStublet
	(
	IUnknown*	punkServer,
	REFIID		iid,
	REFIPID		ipid
	);

    // Create a facelet...
    HRESULT CreateProxy
	(
	IUnknown *      pUnkOuter,
	REFIID          iid,
	REFIPID         ipid,
        IOrpcProxy**    ppProxy
	);
    
    };


#endif



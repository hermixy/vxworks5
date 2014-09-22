/* Stublet.h - COM/DCOM Stublet class definition */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01p,13jul01,dbs  fix up includes
01o,06jul99,dbs  add copy-ctor and assignment-operator
01n,10jun99,dbs  remove inclusion of comNew.h
01m,04jun99,dbs  add const to vtbl ctor arg
01l,03jun99,dbs  remove refs to comSyncLib
01k,28may99,dbs  make stub disp-tbl a structure
01j,18may99,dbs  change to new marshaling architecture
01i,14may99,dbs  change stub-function typename
01h,11may99,dbs  fix pure-virtual call problem
01g,11may99,dbs  remove inclusion of ChannelBuffer.h
01f,11may99,dbs  simplify stub remoting architecture
01e,29apr99,dbs  fix -Wall warnings
01d,28apr99,dbs  make all classes allocate from same pool
01c,27apr99,dbs  add mem-pool to class
01b,21apr99,dbs  add include for IRpcChannelBuffer
01a,20apr99,dbs  created during Grand Renaming

*/

#ifndef __INCStublet_h
#define __INCStublet_h

#include "dcomProxy.h"
#include "private/comMisc.h"

///////////////////////////////////////////////////////////////////////////
//
// VxStublet - an object that implements the interface-stub
// functionality required by the std marshaling stub manager
// (VxStdStub) and dispatches RPC invocations into the actual object
// interface it is representing.
//

class VxStublet
    {
    DWORD		m_dwRefCount;	// ref count
    VxMutex		m_mutex;	// task safety
    IUnknown*		m_punkServer;	// interface we are stub of
    IID                 m_iidServer;    // IID of that interface
    IPID		m_ipid;		// IPID of that interface

    const VXDCOM_STUB_DISPTBL* m_pDispTbl;	// stub dispatch table

    VxStublet& operator= (const VxStublet&);
    VxStublet::VxStublet (const VxStublet&);

  public:

    VxStublet
	(
	IUnknown*			punkServer,
	REFIID				iid,
	REFIPID				ipid,
	const VXDCOM_STUB_DISPTBL*	dispTbl
	);
    
    ~VxStublet ();

    // accessors
    REFIID	iid () const	{ return m_iidServer; }
    REFIPID	ipid () const	{ return m_ipid; }
    ULONG	refs () const	{ return m_dwRefCount; }
    
    // add/remove remote references
    ULONG addRefs (ULONG nRefs);
    ULONG relRefs (ULONG nRefs);

    // get info about stublet for calling methods
    HRESULT stubInfoGet (ULONG, IUnknown**, PFN_ORPC_STUB*);
    };


#endif



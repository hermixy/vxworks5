/* RpcDispatchTable.h -- VxDCOM RPC dispatch table */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01c,20sep00,nel  Add changes made since T2 branch.
01b,27jul99,drm  Returning CLSID from interfaceInfoGet().
01a,27may99,aim  created
*/

#ifndef __INCRpcDispatchTable_h
#define __INCRpcDispatchTable_h

#include "dcomProxy.h"

//////////////////////////////////////////////////////////////////////////
//
// RpcDispatchTable -- defines an abstract base class that all types
// of dispatch table must implement. It defines 2 methods, which are
// pure virtual and must be over-ridden.
//
// supportsInterface() returns a 'bool' indicating whether the
// dispatch table supports the indicated interface (designated by the
// given IID) or not.
//
// interfaceInfoGet() takes an IPID and a method-number, and returns
// the information required to dispatch that method of that interface,
// namely the stub-function pointer and the IUnknown-ptr of the
// interface. If the interface is a DCE interface then it may
// legitimately return NULL as the IUnknown-ptr, and may ignore the
// IPID value, instead relying on the most-recent IID received via
// supportsInterface() to determine the interface identity.
//

class RpcDispatchTable
    {
  public:
    RpcDispatchTable () {}

    virtual ~RpcDispatchTable () {}

    virtual bool    supportsInterface (REFIID) =0;

    virtual HRESULT interfaceInfoGet
	(
	REFIID      riid,
	REFIPID		ipid,
	ULONG		methodNum,
	IUnknown**	ppUnk,
	PFN_ORPC_STUB*	ppfnStubFunc,
        CLSID &         classid
	) =0;
    
  private:
    // unsupported
    RpcDispatchTable (const RpcDispatchTable& other);
    RpcDispatchTable& operator= (const RpcDispatchTable& rhs);
    };

#endif // __INCRpcDispatchTable_h

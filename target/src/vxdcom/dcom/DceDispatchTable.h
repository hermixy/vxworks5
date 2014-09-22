/* DceDispatchTable.h - VxDCOM DceDispatchTable class definition */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01f,13jul01,dbs  fix includes
01e,20sep00,nel  Add REFIID parameter to interfaceInfoGet method.
01d,27jul99,drm  Returning CLSID from interfaceInfoGet().
01c,03jun99,dbs  no return value from mutex lock
01b,03jun99,dbs  remove refs to comSyncLib
01a,02jun99,dbs  created

*/


#ifndef __INCDceDispatchTable_h
#define __INCDceDispatchTable_h

#include "RpcDispatchTable.h"		// base class
#include "private/comStl.h"             // STL containers
#include "private/comMisc.h"            // mutex class


////////////////////////////////////////////////////////////////////////////
//
// DceDispatchTable -- this class implements a subclass of
// RpcDispatchTable which records all exported DCE interfaces...
//

class DceDispatchTable : public RpcDispatchTable
    {
    typedef STL_MAP(IID, VXDCOM_STUB_DISPTBL*) DISPMAP;

  public:

    DceDispatchTable () {}
    virtual ~DceDispatchTable () {}

    // methods inherited from RpcDispatchTable base-class, to look-up
    // interface IDs, and interface pointers
    bool    supportsInterface (REFIID riid);
    HRESULT interfaceInfoGet
	(
	REFIID      riid,
	REFIPID		/*ripid*/,
	ULONG		methodNum,
	IUnknown**	ppunk,
	PFN_ORPC_STUB*	ppfn,
        CLSID &		classid
	);

    // method to register DCE RPC interface with stub dispatch-table
    HRESULT dceInterfaceRegister
	(
	REFIID				riid,
	const VXDCOM_STUB_DISPTBL*	ptbl
	);
    
    void lock () { m_mutex.lock (); }
    void unlock () { m_mutex.unlock (); }
    
  private:

    VxMutex		m_mutex;	// for task safety
    DISPMAP		m_dispmap;	// dispatch table map
    IID			m_iid;		// most recent IID
    };

#endif /*  __INCDceDispatchTable_h */



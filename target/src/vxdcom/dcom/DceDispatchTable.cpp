/* DceDispatchTable.cpp - VxDCOM DceDispatchTable class */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01f,17dec01,nel  Add include symbol for diab.
01e,20sep00,nel  Add REFIID parameter to interfaceInfoGet method.
01d,18sep00,nel  SPR#33730. Merge T2 OPC fixes into T3 branch.
01c,10aug00,nel  Win2K fix.
01b,27jul99,drm  Returning CLSID from interfaceInfoGet().
01a,02jun99,dbs  created

*/


#include "DceDispatchTable.h"
#include "OxidResolver.h"
#include "Syslog.h"

/* Include symbol for diab build */

extern "C" int include_vxdcom_DceDispatchTable (void)
    {
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//

bool DceDispatchTable::supportsInterface (REFIID riid)
    {
    VxCritSec cs (m_mutex);
    return (m_dispmap.find (riid) != m_dispmap.end ());
    }
	
//////////////////////////////////////////////////////////////////////////
//

HRESULT DceDispatchTable::interfaceInfoGet
    (
    REFIID      riid,
    REFIPID		/*ripid*/,
    ULONG		methodNum,
    IUnknown**		ppunk,
    PFN_ORPC_STUB*	ppfn,
    CLSID &		classid
    )
    {
    classid = GUID_NULL;

    VxCritSec cs (m_mutex);

    VXDCOM_STUB_DISPTBL* ptbl = m_dispmap [riid];
    if (methodNum >= ptbl->nFuncs)
        {
        if (IsEqualGUID (riid, IID_IOXIDResolver))
            {
            // strange failure value required by win2k when querying 
            // OXIDResolver for method 5 (which it doesn't support
            return MAKE_HRESULT (0, 0x1c01, 2);	
            }
        return MAKE_HRESULT (SEVERITY_ERROR,
                             FACILITY_RPC,
                             RPC_S_PROCNUM_OUT_OF_RANGE);
        }
    *ppfn = ptbl->funcs [methodNum];
    *ppunk = 0;

    return S_OK;
    }
	
//////////////////////////////////////////////////////////////////////////
//
// DceDispatchTable::dceInterfaceRegister -- method to register DCE
// RPC interface with stub dispatch-table...
//

HRESULT DceDispatchTable::dceInterfaceRegister
    (
    REFIID			riid,
    const VXDCOM_STUB_DISPTBL*	ptbl
    )
    {
    m_dispmap [riid] = const_cast<VXDCOM_STUB_DISPTBL*> (ptbl);
    return S_OK;
    }



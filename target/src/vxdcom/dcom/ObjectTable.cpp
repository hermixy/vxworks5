/* ObjectTable.cpp -- VxDCOM ObjectTable implementation */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01t,17dec01,nel  Add include symbol for diab build.
01s,30jul01,dbs  fix stupid bug in findByIUnknown as in T2 original
01r,13jul01,dbs  fix up includes
01q,05mar01,nel  SPR#62130 - implement CoDisconnectObject.
01p,20sep00,nel  Add changes made in T2 since branch.
01o,18sep00,nel  SPR#33730. Merge T2 OPC fixes into T3 branch.
01n,19aug99,aim  TASK_LOCK now uses mutex
01m,19aug99,dbs  fix objectFindByOid() bug
01l,17aug99,aim  removed copy ctor and operator= from ObjectTable
01k,29jul99,dbs  fix erroneous return of RPC_E_INVALID_IPID
01j,27jul99,drm  Returning CLSID from interfaceInfoGet()
01i,26jul99,dbs  dont save p/s clsid any longer
01h,08jul99,dbs  remove print statements
01g,29jun99,dbs  remove const-ness warnings
01f,29jun99,dbs  remove ifdef DEBUG around ostream operators
01e,29jun99,dbs  make StdStub a member of ObjectTableEntry
01d,17jun99,aim  uses new SCM
01c,10jun99,dbs  remove op new and delete
01b,02jun99,dbs  use new OS-specific macros
01a,02jun99,dbs  created

*/

#include <stdlib.h>
#include "ObjectTable.h"
#include "dcomLib.h"
#include "private/comMisc.h"
#include "SCM.h"
#include "StdStub.h"
#include "orpcLib.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_ObjectTable (void)
    {
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//
// ObjectTableEntry default ctor -- 
//

ObjectTableEntry::ObjectTableEntry ()
  : pstmMarshaledItfPtr (0),
    oid (0),
    punkCF (0),
    dwRegToken (0),
    pingTimeout (0),
    noPing (false)
    {
    TRACE_CALL;
    }

//////////////////////////////////////////////////////////////////////////
//
// ObjectTableEntry ctor -- always AddRef() on stream while its in
// this table entry...
//

ObjectTableEntry::ObjectTableEntry
    (
    OID        o,
    REFCLSID    cls,
    IStream*    pstm,
    bool    npng
    )
      : pstmMarshaledItfPtr (pstm),
    oid (o),
    clsid (cls),
    punkCF (0),
    dwRegToken (0),
    pingTimeout (VXDCOM_PING_TIMEOUT),
    noPing (npng)
    {
    TRACE_CALL;
    if (pstmMarshaledItfPtr)
    pstmMarshaledItfPtr->AddRef ();
    }

//////////////////////////////////////////////////////////////////////////
//
// ObjectTableEntry dtor -- releases stream...
//

ObjectTableEntry::~ObjectTableEntry ()
    {
    TRACE_CALL;

    if (pstmMarshaledItfPtr)
    pstmMarshaledItfPtr->Release ();
    }

////////////////////////////////////////////////////////////////////////////
//

ObjectTableEntry* VxObjectTable::objectRegister
    (
    IStream*        pstmItfPtr,
    REFCLSID        cls,
    bool        noPing
    )
    {
    TRACE_CALL;

    VxCritSec cs (m_mutex);

    OID oidNew = SCM::theSCM()->nextOid ();

    // Add new entry to export table, for this OID...

    ObjectTableEntry* pOTE = new ObjectTableEntry (oidNew,
                           cls,
                           pstmItfPtr,
                           noPing);

    m_objectTable [oidNew] = pOTE;

    return pOTE;
    }

////////////////////////////////////////////////////////////////////////////
//

ObjectTableEntry* VxObjectTable::objectFindByOid
    (
    OID            oid
    )
    {
    TRACE_CALL;

    // Look for entry...
    VxCritSec cs (m_mutex);

    OBJECTMAP::const_iterator i = m_objectTable.find (oid);
    if (i == m_objectTable.end ())
    return 0;
    
    return (*i).second;
    }

////////////////////////////////////////////////////////////////////////////
//

ObjectTableEntry* VxObjectTable::objectFindByStream
    (
    IStream*        pStm
    )
    {
    TRACE_CALL;


    VxCritSec cs (m_mutex);

    // Must do linear search, as the table key is OID...
    OBJECTMAP::const_iterator i = m_objectTable.begin ();
    while (i != m_objectTable.end ())
    {
    ObjectTableEntry*    pOTE = (*i).second;
    
    if (pOTE->pstmMarshaledItfPtr == pStm)
        return pOTE;
    ++i;
    }
    
    return 0;
    }

////////////////////////////////////////////////////////////////////////////
//

ObjectTableEntry* VxObjectTable::objectFindByToken
    (
    DWORD        dwToken
    )
    {
    TRACE_CALL;


    VxCritSec cs (m_mutex);

    // Must do linear search, as table key is OID...
    OBJECTMAP::const_iterator i = m_objectTable.begin ();
    while (i != m_objectTable.end ())
    {
    ObjectTableEntry*    pOTE  = (*i).second;

    if (pOTE->dwRegToken == dwToken)
        return pOTE;
    ++i;
    }
    
    // Not found, if we get here...
    return 0;
    }

////////////////////////////////////////////////////////////////////////////
//

ObjectTableEntry* VxObjectTable::objectFindByIUnknown
    (
    IUnknown*	punk
    )
    {
    TRACE_CALL;

    ObjectTableEntry*	pOTE = 0;

    VxCritSec cs (m_mutex);

    // Must do linear search, as table key is OID...
    OBJECTMAP::const_iterator i = m_objectTable.begin ();
    while (i != m_objectTable.end ())
	{
	pOTE = (*i).second;

	if (pOTE->stdStub.getIUnknown () == punk)
            return pOTE;
	++i;
	}
    
    return 0;
    }

////////////////////////////////////////////////////////////////////////////
//

bool VxObjectTable::objectUnregister (OID oid)
    {
    TRACE_CALL;

    size_t nErased = 0;

    VxCritSec cs (m_mutex);

    ObjectTableEntry* pOTE = objectFindByOid (oid);

    DELZERO (pOTE);

    nErased = m_objectTable.erase (oid);

    return (nErased != 0);
    }

//////////////////////////////////////////////////////////////////////////
//
// VxObjectTable::supportsInterface -- determine if the table contains
// an entry corresponding to the given interface identifier...
//

bool VxObjectTable::supportsInterface (REFIID riid)
    {
    bool bSupports = false;
    
    // Must do linear search, as table key is OID...
    VxCritSec cs (m_mutex);
    
    OBJECTMAP::iterator i = m_objectTable.begin ();
    while (i != m_objectTable.end ())
    {
    if ((*i).second->stdStub.supportsInterface (riid))
        {
        bSupports = true;
        break;
        }
    ++i;
    }

    return bSupports;
    }

//////////////////////////////////////////////////////////////////////////
//
// VxObjectTable::interfaceInfoGet -- returns the info required to
// dispatch a method call on an Object Interface...
//

HRESULT VxObjectTable::interfaceInfoGet
    (
    REFIID      riid,
    REFIPID        ripid,
    ULONG        methodNum,
    IUnknown**        ppUnk,
    PFN_ORPC_STUB*    ppfn,
    CLSID &        classid
    )
    {
    HRESULT hr = S_OK;
    
    OID oid = orpcOidFromIpid (ripid);

    ObjectTableEntry* pOTE = objectFindByOid (oid);
    if (pOTE)
        {
    // Both the VxStdStub and VxObjectTable inherit from
    // RpcDispatchTable which declares the interfaceInfoGet() method.
    // Only the VxObjectTable knows the CLSID for the class being
    // invoked by RemoteActivation, so we ignore the CLSID from
    // VxStdStub::interfaceInfoGet() and replace it with the value
    // here.
    hr = pOTE->stdStub.interfaceInfoGet (riid, 
                         ripid,
                         methodNum,
                         ppUnk,
                         ppfn,
                         classid);
        if (SUCCEEDED (hr))
            classid = pOTE->clsid;
    else
        classid = GUID_NULL;
        }
    else
    hr = RPC_E_INVALID_IPID;

    return hr;
    }


//////////////////////////////////////////////////////////////////////////

void ObjectTableEntry::printOn (ostream& os) const
    {
    os << "ObjectTableEntry:oid=" << (int) oid << endl;
    }

void VxObjectTable::printOn (ostream& os) const
    {
    os << "ObjectTable:" << endl;
    for (const_iterator i = begin (); i != end (); ++i)
    os << (*i).second;
    }







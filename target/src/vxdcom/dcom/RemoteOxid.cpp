/* RemoteOxid.cpp - COM/DCOM RemoteOxid class implementation */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
02f,17dec01,nel  Add include symbol for diab.
02e,26jul01,dbs  remove obsolete code
02d,13jul01,dbs  fix up includes
02c,15feb00,dbs  move proxy/stub code out of this class
02b,07feb00,dbs  update struct-descs to latest NDR impl
02a,15sep99,dbs  adapt to NDR changes made for OPC support
01z,12aug99,dbs  comply with new NDR struct support
01y,02aug99,dbs  fix indirection when marshaling array of IIDs in
                 RemQueryInterface()
01x,30jul99,dbs  tighten up type-safety of NDR types
01w,09jul99,dbs  implement ping functionality in SCM now
01v,08jul99,dbs  move address and other info into new class
01u,06jul99,aim  change from RpcBinding to RpcIfClient
01t,05jul99,dbs  use correct binding for ping, remove wierdness when
                 combining vxcom_wcscpy and T2OLE in method
		 resolverAddressSet() 
01s,30jun99,dbs  remove unnecessary casts
01r,10jun99,dbs  remove op new and delete
01q,08jun99,dbs  remove use of mtmap
01p,07jun99,dbs  change GuidMap to mtmap
01o,03jun99,dbs  no return value from mutex lock
01n,03jun99,dbs  remove refs to comSyncLib
01m,28may99,dbs  implement Ping functionality
01l,20may99,dbs  fix handling of returned array-types
01k,18may99,dbs  change to new marshaling architecture
01j,10may99,dbs  simplify rpc-binding usage
01i,05may99,dbs  add update() method
01h,29apr99,dbs  fix -Wall warnings
01g,28apr99,dbs  use COM_MEM_ALLOC for all classes
01f,27apr99,dbs  use new allocation calls
01e,27apr99,dbs  add mem-pool to classes
01d,26apr99,aim  added TRACE_CALL
01c,22apr99,dbs  tidy up potential leaks
01b,22apr99,dbs  fix many calls to ndr_marshal_xxx()
01a,20apr99,dbs  created during Grand Renaming

*/

/*
  DESCRIPTION:

  RemoteOxid -- 

*/

#include "RemoteOxid.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_RemoteOxid (void)
    {
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//

VxRemoteOxid::VxRemoteOxid 
    (
    OXID			oxid,
    REFIPID			ipid,
    const RpcStringBinding&	oxidAddr
    )
      : m_dwRefCount (0),
	m_oxid (oxid),
	m_ipidRemUnk (ipid),
	m_stringBinding (oxidAddr)
    {
    }

//////////////////////////////////////////////////////////////////////////
//
// VxRemoteOxid dtor -- checks to see if there are any OIDs to be
// un-pinged. If so, it sends a final ComplexPing() to remove them
// all...
//

VxRemoteOxid::~VxRemoteOxid ()
    {
    }

//////////////////////////////////////////////////////////////////////////

ULONG VxRemoteOxid::AddRef ()
    {
    return comSafeInc (&m_dwRefCount);
    }

//////////////////////////////////////////////////////////////////////////
//

ULONG VxRemoteOxid::Release ()
    {
    DWORD n = comSafeDec (&m_dwRefCount);
    if (n == 0)
	delete this;
    return n;
    }


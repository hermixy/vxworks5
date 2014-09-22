/* RemoteSCM.cpp -- VxDCOM Remote SCM class */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,17dec01,nel  Add include symbol for diab.
01k,10dec01,dbs  diab build
01j,26jul01,dbs  use IOrpcClientChannel interface
01i,13jul01,dbs  fix use of NEW macro to operator new
01h,19aug99,aim  added TraceCall header
01g,22jul99,dbs  add re-use of remote-SCM connection
01f,16jul99,aim  fix UMR in ctor
01e,12jul99,dbs  re-instate ping code
01d,12jul99,dbs  simply update existing RemoteOxid object rather than
                 creating a new one every time
01c,09jul99,dbs  implement ping functionality in SCM now
01b,09jul99,dbs  update RemoteOxid if port changes
01a,09jul99,dbs  created
*/

#include "RemoteSCM.h"
#include "RpcIfClient.h"
#include "TraceCall.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_RemoteSCM (void)
    {
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//

RemoteSCM::RemoteSCM ()
  : m_mutex (),
    m_dwRefCount (0),
    m_remoteOxidTable (),
    m_strb (),
    m_pChannel (0),
    m_setid (0),
    m_pingSeqNum (1),
    m_oidsToAdd (),
    m_oidsToDel (),
    m_tickCount (0)
    {
    TRACE_CALL;
    }

//////////////////////////////////////////////////////////////////////////
//

RemoteSCM::RemoteSCM (const RpcStringBinding& sb)
  : m_mutex (),
    m_dwRefCount (0),
    m_remoteOxidTable (),
    m_strb (sb),
    m_pChannel (0),
    m_setid (0),
    m_pingSeqNum (1),
    m_oidsToAdd (),
    m_oidsToDel (),
    m_tickCount (0)
    {
    TRACE_CALL;
    }

//////////////////////////////////////////////////////////////////////////
//

RemoteSCM::~RemoteSCM ()
    {
    TRACE_CALL;
    
    // Check if we have any OIDs to delete from the remote SCM's list
    // of OIDs in our set...
    if (m_oidsToDel.size () && m_pChannel)
	{
	unsigned short backoffFactor;

#ifdef __DCC__
        OID* oidsToDel = new OID [m_oidsToDel.size ()];
        OID* oidsToAdd = new OID [m_oidsToAdd.size ()];
        copy (m_oidsToAdd.begin (), m_oidsToAdd.end (), oidsToAdd);
        copy (m_oidsToDel.begin (), m_oidsToDel.end (), oidsToDel);
#else
        OID* oidsToDel = m_oidsToDel.begin ();
        OID* oidsToAdd = m_oidsToAdd.begin ();
#endif
        if (1)
            {
            VxCritSec cs (m_mutex);
            IOXIDResolver_ComplexPing_vxproxy (m_pChannel,
                                               &m_setid,
                                               m_pingSeqNum,
                                               0,
                                               m_oidsToDel.size (),
                                               oidsToAdd,
                                               oidsToDel,
                                               &backoffFactor);
            }
#ifdef __DCC__
        delete oidsToAdd;
        delete oidsToDel;
#endif
	}
    }

//////////////////////////////////////////////////////////////////////////
//

void RemoteSCM::oxidBindingUpdate
    (
    OXID			oxid,
    REFIPID			ipidRemUnk,
    const RpcStringBinding&	sbRemoteOxid
    )
    {
    TRACE_CALL;

    // Always add a new entry, since we don't do this very often (only
    // after activations and oxid-resolutions), and if the server has
    // re-started it may have changed port-number...
    SPRemoteOxid& ro = m_remoteOxidTable [oxid];

    // Create one if there isn't already one, otherwise just update
    // the recorded address and IPID info...
    if (! ro)
	ro = new VxRemoteOxid (oxid, ipidRemUnk, sbRemoteOxid);
    else
	ro->update (ipidRemUnk, sbRemoteOxid);
    }

//////////////////////////////////////////////////////////////////////////
//
// oxidBindingLookup -- this function looks up an existing RemoteOxid
// entry in the table, and return true if one was found, false if
// not...
//

bool RemoteSCM::oxidBindingLookup (OXID oxid, SPRemoteOxid& ro) const
    {
    TRACE_CALL;

    // Is this OXID in the map?
    OXIDMAP::const_iterator i = m_remoteOxidTable.find (oxid);
    if (i == m_remoteOxidTable.end ())
	return false;

    ro = (*i).second;
    return true;
    }

//////////////////////////////////////////////////////////////////////////

ULONG RemoteSCM::AddRef ()
    {
    return comSafeInc (&m_dwRefCount);
    }

//////////////////////////////////////////////////////////////////////////
//

ULONG RemoteSCM::Release ()
    {
    DWORD n = comSafeDec (&m_dwRefCount);
    if (n == 0)
	delete this;
    return n;
    }

//////////////////////////////////////////////////////////////////////////
//

IOrpcClientChannel* RemoteSCM::connectionGet ()
    {
    // Create connection to remote SCM address
    if (! m_pChannel)
	m_pChannel = new RpcIfClient (m_strb);

    return m_pChannel;
    }

//////////////////////////////////////////////////////////////////////////
//
// RemoteSCM::pingTick -- another 'nSecs' has passed, so we need to
// determine whether its time to send a ping or not...
//

void RemoteSCM::pingTick (size_t nSecs)
    {
    m_tickCount -= nSecs;
    if (m_tickCount <= 0)
	{
	// Restart ping timer...
	m_tickCount = 120;

	// Need a binding to the remote OXID-resolver?
        IOrpcClientChannelPtr pChan = connectionGet ();
        if (! pChan)
	    return;
	
	// Decide whether to send a complex ping (if we have OIDs to
	// add or remove from the set, or we have never pinged yet) or
	// a simple ping (if there are no changes to the set)...
	if (m_oidsToAdd.size () ||
	    m_oidsToDel.size () ||
	    (m_setid == 0))
	    {
	    // Need a complex ping...
	    unsigned short backoffFactor;

	    HRESULT hr = S_OK;

#ifdef __DCC__
            OID* oidsToDel = new OID [m_oidsToDel.size ()];
            OID* oidsToAdd = new OID [m_oidsToAdd.size ()];
            copy (m_oidsToAdd.begin (), m_oidsToAdd.end (), oidsToAdd);
            copy (m_oidsToDel.begin (), m_oidsToDel.end (), oidsToDel);
#else
            OID* oidsToDel = m_oidsToDel.begin ();
            OID* oidsToAdd = m_oidsToAdd.begin ();
#endif
            if (1)
                {
                VxCritSec cs (m_mutex);
                hr=IOXIDResolver_ComplexPing_vxproxy (pChan,
                                                      &m_setid,
                                                      m_pingSeqNum,
                                                      m_oidsToAdd.size (),
                                                      m_oidsToDel.size (),
                                                      oidsToAdd,
                                                      oidsToDel,
                                                      &backoffFactor);
                }
#ifdef __DCC__
            delete oidsToAdd;
            delete oidsToDel;
#endif
	    if (SUCCEEDED (hr))
		{
		m_oidsToAdd.erase (m_oidsToAdd.begin (),
				   m_oidsToAdd.end ());
		m_oidsToDel.erase (m_oidsToDel.begin (),
				   m_oidsToDel.end ());
		++m_pingSeqNum;
		}
	    }
	else
	    {
	    // Just a simple ping...
	    IOXIDResolver_SimplePing_vxproxy (pChan,
					      &m_setid);
	    }
	}
    }



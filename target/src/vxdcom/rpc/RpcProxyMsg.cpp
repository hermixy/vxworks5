/* RpcProxyMsg.cpp - VxDCOM RpcProxyMsg class implementation */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
02c,17dec01,nel  Add include symbol for diab build.
02b,01oct01,nel  SPR#69557. Add extra padding bytes to make VT_BOOL type work.
02a,25sep01,nel  Correct prototype error under test harness build.
01z,02aug01,dbs  use simplified Thread API
01y,25jul01,dbs  simplify interface proxy system
01x,20jul01,dbs  fix include path of vxdcomGlobals.h
01w,13jul01,dbs  fix up includes
01v,29feb00,dbs  fix ORPC_EXTENT so its still inscope when marshaled
01u,28feb00,dbs  use correct ThreadOS method to get/set priority
01t,15feb00,dbs  fix addition of extent to ORPC msg
01s,19jan00,nel  Fix include of taskLib.h in Solaris build
01r,17nov99,nel  Include taskLib.h
01q,19aug99,aim  change assert to VXDCOM_ASSERT
01p,13aug99,aim  moved globals to vxdcomGlobals.h
01o,05aug99,dbs  change to byte instead of char
01n,30jul99,dbs  add client fake prio in Solaris
01m,29jul99,drm  Adding code to conditionally send client priority.
01l,09jul99,drm  Adding code to add a VXDCOM_EXTENT to an ORPCTHIS.
01k,09jul99,dbs  remove references to obsolete files
01j,06jul99,aim  change from RpcBinding to RpcIfClient
01i,25jun99,dbs  add auth-trailer to stub-msg
01h,17jun99,aim  changed assert to assert
01g,17jun99,dbs  change to COM_MEM_ALLOC
01f,20may99,dbs  move NDR phase into streams
01e,19may99,dbs  fix up priority encoding
01d,19may99,dbs  fix ORPCTHIS insertion to marshal correctly
01c,18may99,dbs  add proxy/stub marshaling phase to NDR-streams
01b,17may99,dbs  fix DCE usage of class
01a,12may99,dbs  created

*/

/*
  DESCRIPTION:


*/


#include "RpcProxyMsg.h"
#include "orpcLib.h"
#include "StdProxy.h"
#include "vxdcomExtent.h"
#include "private/vxdcomGlobals.h"
#include "Syslog.h"
#include "TraceCall.h"
#include "RpcPduFactory.h"
#include "InterfaceProxy.h"
#include "taskLib.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_RpcProxyMsg (void)
    {
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//
// RPC_STUB_MSG methods...
//

RPC_STUB_MSG::RPC_STUB_MSG
    (
    NdrUnmarshalStream*	pus,
    NdrMarshalStream*	pms,
    int			channelId
    )
  : m_pUnmshlStrm (pus),
    m_pMshlStrm (pms),
    m_channelId (channelId)
    {
    TRACE_CALL;
    }

NdrMarshalStream* RPC_STUB_MSG::marshalStreamGet ()
    {
    TRACE_CALL;

    COM_ASSERT(m_pMshlStrm);
    return m_pMshlStrm;
    }

NdrUnmarshalStream* RPC_STUB_MSG::unmarshalStreamGet ()
    {
    TRACE_CALL;

    COM_ASSERT(m_pUnmshlStrm);
    return m_pUnmshlStrm;
    }

int RPC_STUB_MSG::channelIdGet ()
    {
    TRACE_CALL;

    return m_channelId;
    }

//////////////////////////////////////////////////////////////////////////
//
// RPC_PROXY_MSG methods -- this wrapper-class is exposed to the user
// in dcomProxy.h, but its internal implementation is provided by the
// RpcProxyMsg class...
//

RPC_PROXY_MSG::RPC_PROXY_MSG
    (
    REFIID		riid,
    RpcMode::Mode_t	mode,
    ULONG		opnum,
    void*		pv
    )
    {
    TRACE_CALL;
    m_pImpl = new RpcProxyMsg (riid, mode, opnum, pv);
    }

RPC_PROXY_MSG::~RPC_PROXY_MSG ()
    {
    TRACE_CALL;
    if (m_pImpl)
	delete m_pImpl;
    }

HRESULT RPC_PROXY_MSG::SendReceive ()
    {
    TRACE_CALL;
    COM_ASSERT(m_pImpl);
    return m_pImpl->SendReceive ();
    }

NdrMarshalStream* RPC_PROXY_MSG::marshalStreamGet ()
    {
    TRACE_CALL;
    COM_ASSERT(m_pImpl);
    return m_pImpl->marshalStreamGet ();
    }

NdrUnmarshalStream* RPC_PROXY_MSG::unmarshalStreamGet ()
    {
    TRACE_CALL;

    COM_ASSERT(m_pImpl);
    return m_pImpl->unmarshalStreamGet ();
    }

//////////////////////////////////////////////////////////////////////////
//
// RpcProxyMsg::RpcProxyMsg -- ctor initialises rpc-mode and
// marshaling streams. The argument 'punk' is NULL for DCE-mode
// interface proxies, and must be non-NULL for OBJECT mode...
//

RpcProxyMsg::RpcProxyMsg
    (
    REFIID		riid,
    RpcMode::Mode_t	mode,
    ULONG		opnum,
    void*		pv
    )
      : m_mode (mode),
	m_mshlStrm (NdrPhase::PROXY_MSHL, ORPC_WIRE_DREP),
	m_unmshlStrm (),
	m_punkItfPtr (0),
	m_iid (riid),
	m_opnum (opnum),
	m_pReply (0),
	m_pChannel (0)
    {
    // If this transaction is in object-mode, we need to add the
    // ORPCTHIS info to the marshaling buffer before doing the
    // argument marshaling...
    if (mode == RpcMode::OBJECT)
	{
	bool addPrio = false;
	int  clientPriority=0;
	
	// Before we generate the ORPCTHIS, we need to see if we can
	// get the current thread's priority, so it can be propagated
	// to the server end...
        if (g_clientPriorityPropagation &&
            (::taskPriorityGet (::taskIdSelf (), &clientPriority)))
            {
	    addPrio = true;
            }

	// Now we declare all the ORPCTHIS-related variables, ready
	// for marshaling into the stream ahead of the method args...
	
	GUID			causality = GUID_NULL;
	VXDCOM_EXTENT		ext (clientPriority);
	ORPC_EXTENT*		exts [2] = { &ext, 0 };
	ORPC_EXTENT_ARRAY	extArray = { 2, 0, exts };

	ORPCTHIS		orpcThis =
	    {
		{
		    RpcPduFactory::rpcMajorVersion (),
		    RpcPduFactory::rpcMinorVersion ()
		},
		ORPCF_NULL,
		0,
		causality,
		addPrio ? &extArray : 0
	    };

	ndrMarshalORPCTHIS (&m_mshlStrm, &orpcThis);

	m_punkItfPtr = reinterpret_cast<IUnknown*> (pv);
	}
    else
	{
        // Non-object (DCE) mode, so the <pv> argument is actually the
        // ORPC client channel...
	m_pChannel = reinterpret_cast<IOrpcClientChannel*> (pv);
	}
    }

//////////////////////////////////////////////////////////////////////////
//
// RpcProxyMsg::~RpcProxyMsg -- dtor
//

RpcProxyMsg::~RpcProxyMsg ()
    {
    TRACE_CALL;
    if (m_pReply)
        delete [] m_pReply;
    }

//////////////////////////////////////////////////////////////////////////
//
// RpcProxyMsg::SendReceive -- performs the actual (O)RPC transaction
// over the wire. On entry, the stub-data has already been marshaled
// into the marshaling stream, and we have been initialised with the
// interface ID and IPID that we need...
//

HRESULT RpcProxyMsg::SendReceive ()
    {
    TRACE_CALL;

    HRESULT hr = S_OK;
    
    // Get interface-ptr ID and RPC-binding if we are in object mode,
    // and we don't yet have them. If we are in DCE mode we already
    // have the info we need to perform the transaction...
    if ((m_mode == RpcMode::OBJECT) && ! m_pChannel)
	{
        // Remember (see InterfaceProxy.h) that the XXX_vxproxy
        // functions generated by WIDL form the methods of the proxied
        // interface -- in the 'standard marshaling' case this is
        // always an instance of VxInterfaceProxy. Thus, we need to
        // find our way back to that instance, in order to get hold of
        // the necessary per-proxy information we need to dispatch the
        // remote method. This info is basically the channel-pointer,
        // plus the IPID which is required by the protocol...

        VxInterfaceProxy* pItf = VxInterfaceProxy::safe_cast (m_punkItfPtr);
        if (! pItf)
            return E_UNEXPECTED;

        // Now get the IPID and the client-channel...
        hr = pItf->interfaceInfoGet (&m_ipid, &m_pChannel);
        if (FAILED (hr))
            return hr;
	}

    // Add any extra pading required.
    if (m_mshlStrm.getEndPadding () != 0)
        {
        BYTE	b = 0xFF;
        DWORD	count;

        for (count = 0; count < m_mshlStrm.getEndPadding (); count++)
            {
    	    m_mshlStrm.insert (sizeof (BYTE), &b, false);
            }
        }

    // Now build a MSHL_BUFFER for the in-args...
    MSHL_BUFFER inbuf = { m_mshlStrm.begin (),
                          m_mshlStrm.size (),
                          m_mshlStrm.drep () };
                          
    // And one for the returned args and result...
    MSHL_BUFFER outbuf = { 0, 0, 0 };

    // Invoke the method, saving the HRESULT for later...
    hr = m_pChannel->InvokeMethod (m_iid,
                                   m_mode == RpcMode::DCE ? 0 : &m_ipid,
                                   m_opnum,
                                   &inbuf,
                                   &outbuf);

    // @@@ FIXME multiple copies going on here -- NdrMarshalStream
    // should inherit from MSHL_BUFFER perhaps?
    
    // Copy resulting stub-data to some local memory, which will stay
    // around after this function returns...
    COM_ASSERT(m_pReply == 0);
    m_pReply = new byte [outbuf.len];
    if (! m_pReply)
	return E_OUTOFMEMORY;
    memcpy (m_pReply, outbuf.buf, outbuf.len);

    // create a new unmarshal stream from the returned stub-data
    m_unmshlStrm = NdrUnmarshalStream (NdrPhase::PROXY_UNMSHL,
				       outbuf.drep,
				       m_pReply,
				       outbuf.len);

    // Release the MSHL_BUFFER...
    m_pChannel->FreeBuffer (&outbuf);

    // Finished with the channel for now...
    m_pChannel = 0;
    
    // NOW check the HRESULT of the invocation, after having tidied
    // up the marshaling buffers, etc...
    if (FAILED (hr))
	return hr;

    // extract the ORPCTHAT if in OBJECT mode
    if (m_mode == RpcMode::OBJECT)
	hr = m_unmshlStrm.extract (sizeof (ORPCTHAT), 0, false);

    // return and let the proxy handle the remaining unmarshaling
    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//
// RpcProxyMsg::marshalStreamGet -- returns the address of the
// marshaling stream...
//

NdrMarshalStream* RpcProxyMsg::marshalStreamGet ()
    {
    return &m_mshlStrm;
    }

//////////////////////////////////////////////////////////////////////////
//
// RpcProxyMsg::unmarshalStreamGet -- returns the address of the
// unmarshaling stream...
//

NdrUnmarshalStream* RpcProxyMsg::unmarshalStreamGet ()
    {
    return &m_unmshlStrm;
    }




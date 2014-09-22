/* RpcIfClient -- Encapsulates Remote Method Invocations */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01t,17dec01,nel  Add include symbol for diab.
01s,10oct01,dbs  add AddKnownInterface() method to IOrpcClientChannel
01r,02oct01,nel  Move debug hooks into SockStream class.
01q,26sep01,nel  Modify hooks to pass IP and Port.
01p,21aug01,nel  Add hooks to optionally display decoded packet traffic.
01o,24jul01,dbs  take fix from SPR#68234
01n,13jul01,dbs  add pres ctx ID to request pdu
01m,29mar01,nel  SPR#35873. Add extra code to hold the client side context IDs
                 for bound interfaces and modify the bindToIID method to send
                 alter context for already bound interfaces rather than always
                 sending bind.
01l,20jun00,dbs  fix client BIND/ALTER_CONTEXT handling
01k,24may00,dbs  add fault diagnostics
01j,04may00,nel  Add log output for client side requests.
01i,20aug99,aim  return SERVER_NOT_AVAILABLE when connect fails
01h,30jul99,aim  added S_INFO messages
01g,22jul99,dbs  enforce serialisation on method-calls
01f,19jul99,dbs  add client-side authn support
01e,13jul99,aim  syslog api changes
01d,06jul99,aim  new ctors
01c,02jul99,aim  added RpcInvocation functionality
01b,08jun99,aim  rework
01a,05jun99,aim  created
*/

#include "RpcIfClient.h"
#include "Reactor.h"
#include "Syslog.h"
#include "RpcPduFactory.h"
#include "comErr.h"
#include "SCM.h"
#include "private/comSysLib.h"
#include "private/DebugHooks.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_RpcIfClient (void)
    {
    return 0;
    }

RpcIfClient::CALL_ID
RpcIfClient::s_callId = 0;

RpcIfClient::~RpcIfClient ()
    {
    TRACE_CALL;

    // Remove records of this channel from NTLM authn service
    NTLMSSP* ssp = SCM::ssp ();

    if (ssp)
	ssp->channelRemove (channelId ());
    }

RpcIfClient::RpcIfClient (const INETSockAddr& peerAddr)
  : m_peerAddr (peerAddr),
    m_boundIIDs (),
    m_stream (),
    m_connector (),
    m_connected (false),
    m_dwRefCount (0)
    {
    }

RpcIfClient::RpcIfClient (const RpcStringBinding& binding)
  : m_peerAddr (),
    m_boundIIDs (),
    m_stream (),
    m_connector (),
    m_connected (false),
    m_dwRefCount (0)
    {
    m_peerAddr = INETSockAddr (binding.ipAddress (),
			       binding.portNumber ());
    }

int RpcIfClient::connect (const INETSockAddr& peerAddr, HRESULT& hresult)
    {
    TRACE_CALL;

    hresult = S_OK;

    if (m_connected)
	return 0;

    if (m_connector.connect (stream (), peerAddr) < 0)
	{
	S_ERR (LOG_RPC | LOG_ERRNO, "cannot connect to " << peerAddr);
	hresult = MAKE_HRESULT (SEVERITY_ERROR,
				FACILITY_WIN32,
				RPC_S_SERVER_UNAVAILABLE);
	}
    else
	{
	INETSockAddr pa;
	
	if (stream().peerAddrGet (pa) < 0)
	    {
	    hresult = MAKE_HRESULT (SEVERITY_ERROR,
				    FACILITY_WIN32,
				    RPC_S_SERVER_UNAVAILABLE);
	    m_connected = false;
	    stream().close ();
	    }
	else
	    {
	    S_DEBUG (LOG_RPC, "connected to " << pa);
	    hresult = S_OK;
	    m_connected = true;

	    // Add this channel to SSP's records so we can perform
	    // authentication in future if required...
	    NTLMSSP* pssp = SCM::ssp ();

	    if (pssp)
		pssp->channelAdd (channelId ());
	    }
	}

    return SUCCEEDED (hresult) ? 0 : -1;
    }

int RpcIfClient::connect ()
    {
    TRACE_CALL;
    HRESULT hr;
    connect (m_peerAddr, hr);
    return hr;
    }

SockStream& RpcIfClient::stream ()
    {
    return m_stream;
    }

bool RpcIfClient::connected () const
    {
    return m_connected;
    }

int RpcIfClient::sendPdu (RpcPdu& pdu)
    {
    size_t len;
    char *buf = 0;
    int result = -1;

    if (pdu.makeReplyBuffer (buf, len) < 0)
        stream().close ();
    else 
	{
	stream ().processDebugOutput (pRpcClientOutput, (BYTE *)buf, (DWORD)len);
	if (stream().send (buf, len) == len)
	    {
	    result = 0;
	    S_DEBUG (LOG_RPC, "sendPdu: " << pdu);
	    }
	}

    if (buf != NULL)
	{
	delete [] buf;
	}

    return result;
    }

RpcIfClient::CALL_ID RpcIfClient::nextCallId ()
    {
    return ++s_callId;
    }

int RpcIfClient::recvPdu (RpcPdu& pdu)
    {
    TRACE_CALL;
    const int buflen = 1024;
    DWORD totalLen = 0;
    char buf[buflen];

    while (1)
	{
	ssize_t n = stream().recv (buf, buflen);

	// no data or the peer closed the connection
	if (n <= 0)
	    return -1;

	totalLen += (DWORD)n;

	pdu.append (buf, n);

	if (pdu.complete ())
	    {
	    stream ().processDebugOutput (pRpcClientInput, (BYTE *)buf, (DWORD)buflen);
	    S_DEBUG (LOG_RPC, "recvPdu: " << pdu);
	    return 0;
	    }
	}

    return -1;
    }

int RpcIfClient::channelId () const
    {
    return reinterpret_cast<int> (this);
    }

//////////////////////////////////////////////////////////////////////////
//
// RpcIfClient::interfaceIsBound - is the given IID already bound?
//
// This method tests if the interface ID <iid> is already bound,
// i.e. we have at some point in the past, sent a BIND packet and
// successfully allocated a presentation-context ID for it.
//
// If we have, then it returns 'true' and sets the value of
// <presCtxId> appropriately. If not, then it returns 'false' and a
// new presentation context ID will need to be generated.
//

bool RpcIfClient::interfaceIsBound (const IID& iid, USHORT& presCtxId)
    {
    // Search through the vector and if the interface ID is in the
    // table, then set the presentation-context ID value and return
    // true. If we reach the end of the table, and the IID is not
    // present, return false (and do not set 'presCtxId' at all).

    InterfaceVec::iterator i;

    for (i = m_boundIIDs.begin (); i != m_boundIIDs.end (); i++)
    	{
    	if ((*i) == iid)
 	    {
	    presCtxId = i - m_boundIIDs.begin ();
	    return true;
	    }

	}

    return false;
    }


//////////////////////////////////////////////////////////////////////////
//
// RpcIfClient::bindToIID - bind this channel to the given IID
//


int RpcIfClient::bindToIID
    (
    const IID&	iid,
    const GUID*	objid,
    HRESULT&	hresult,
    USHORT&     presCtxId
    )
    {
    // Check if we are already bound to this IID. If so, we can use
    // that pres-ctx ID directly, and don't need to send either BIND
    // or ALTER_CONTEXT first...
    bool alreadyBound = interfaceIsBound (iid, presCtxId);

    // If we have already bound to this IID, then we can possibly
    // short-cut if its also the current IID (i.e. the last one we
    // sent a BIND or ALTER_CONTEXT for)...
    if (alreadyBound)
        {
        // If its the current IID, take the shortcut -- the
        // presentation context ID will be set to the right value
        // already...
        if (iid == m_currentIID)
            {
            hresult = S_OK;
            return 0;
            }

        // If its not the current IID, we need to fall through and do
        // an ALTER_CONTEXT anyway...
        }
    else
        {
        // We need to generate a new presentation context ID...
    	presCtxId = m_boundIIDs.size ();
        }
    
    // Locate default SSP for this machine
    NTLMSSP* pssp = SCM::ssp ();

    // Create BIND (or ALTER_CONTEXT) packet...
    ULONG assocGroupId = 0;
    RpcPdu bindPdu;
    RpcPduFactory::formatBindPdu (bindPdu, 
                                  iid, 
                                  nextCallId (), 
                                  assocGroupId, 
                                  alreadyBound,
                                  presCtxId);

    // Add any required auth-trailer...
    hresult = pssp->clientAuthnRequestAdd (channelId (), bindPdu);
    if (FAILED (hresult))
	return -1;

    // Send the PDU to the server...
    if (sendPdu (bindPdu) < 0)
	{
	hresult = RPC_E_DISCONNECTED;
	return -1;
	}

    // Receive reply (should be BIND-ACK or BIND-NAK)...
    RpcPdu reply;

    if (recvPdu (reply) < 0)
	{
	hresult = RPC_E_DISCONNECTED;
	return -1;
	}

    // What kind of response did we get?
    if (reply.isBindAck ())
	{
	// Process BIND-ACK packet, we may require to send an AUTH3 to
	// complete the authentication process. First record the bound
	// IID value...
        AddKnownInterface (iid);
	m_currentIID = iid;
	
	// Now process the rest of the authentication steps...
	RpcPdu auth3Pdu;
	bool sendAuth3 = false;
	RpcPduFactory::formatAuth3Pdu (auth3Pdu, reply.callId ());

	// Generate the AUTH3 trailer, the NTLM object will decide
	// whether we need to send it or not...
	hresult = pssp->clientAuthnResponse (channelId (),
					     reply,
					     &sendAuth3,
					     auth3Pdu);

	if (SUCCEEDED (hresult) && sendAuth3)
	    {
	    if (sendPdu (auth3Pdu) < 0)
		hresult = RPC_E_DISCONNECTED;
	    }
	}
    else if (reply.isAlterContextResp ())
	{
        // We got an ALTER_CONTEXT_RESPONSE so just set the current
        // IID to this IID...
	m_currentIID = iid;
	}
    else if (reply.isBindNak ())
	{
        // We got a BIND_NAK, that means the server end doesn't
        // support the IID on this channel...
	hresult = MAKE_HRESULT (SEVERITY_ERROR,
				FACILITY_RPC,
				RPC_S_INTERFACE_NOT_FOUND);
	}
    else
        {
        // Any other kind of packet is not what we can deal with...
	hresult = RPC_E_UNEXPECTED;
        }
    
    return SUCCEEDED (hresult) ? 0 : -1;
    }


//////////////////////////////////////////////////////////////////////////
//
// RpcIfClient::sendMethod - send a REQUEST and get back a RESPONSE
//
//

int RpcIfClient::sendMethod
    (
    RpcPdu&	pdu,
    RpcPdu&	resultPdu,
    HRESULT&	hresult
    )
    {
    // Send the REQUEST packet...
    if (sendPdu (pdu) < 0)
	{
	hresult = RPC_E_DISCONNECTED;
	return -1;
	}

    // Await the response...
    if (recvPdu (resultPdu) < 0)
	{
	hresult = RPC_E_DISCONNECTED;
	return -1;
	}

    // Is it the right kind of packet?
    if (resultPdu.isResponse ())
	hresult = S_OK;
    else if (resultPdu.isFault ())
	{
	S_ERR (LOG_RPC, "TxREQUEST:" << pdu);
	S_ERR (LOG_RPC, "RxFAULT:" << resultPdu);
	hresult = resultPdu.fault().status;
	}
    else
	hresult = RPC_E_UNEXPECTED;

    return SUCCEEDED (hresult) ? 0 : -1;
    }


//////////////////////////////////////////////////////////////////////////
//
// RpcIfClient::InvokeMethod - invoke a method on a remote machine
//

HRESULT RpcIfClient::InvokeMethod
    (
    const IID&	        iid,
    const GUID*	        objid,
    USHORT              opnum,
    const MSHL_BUFFER*  pMshlBufIn,
    MSHL_BUFFER*        pMshlBufOut
    )
    {
    USHORT presCtxId;
    HRESULT hr;
    
    // Wrap each method-invocation up in a critical-section, in case
    // this object is being shared between tasks...
    VxCritSec cs (m_mutex);

    // Make sure we are connected to the remote server...
    if (connect (m_peerAddr, hr) < 0)
        return hr;
                             
    // Bind the channel to the right IID...
    if (bindToIID (iid, objid, hr, presCtxId) < 0)
	return hr;

    // Make a REQUEST PDU ready to send...
    RpcPdu pdu;
    
    RpcPduFactory::formatRequestPdu (pdu,
                                     nextCallId (),
                                     opnum,
                                     objid,
                                     presCtxId);
    
    pdu.stubDataAppend (pMshlBufIn->buf, pMshlBufIn->len);


    // Add any authentication data necessary...
    NTLMSSP* pssp = SCM::ssp ();

    if (pssp)
	pssp->clientAuthn (channelId (), pdu);

    // Call the actual method, getting the resulting RESPONSE PDU back
    // containing the result stub-data...
    RpcPdu resultPdu;
    
    if (sendMethod (pdu, resultPdu, hr) < 0)
        return hr;

    // Copy the returned stub-data into the MSHL_BUFFER...
    AllocBuffer (pMshlBufOut, resultPdu.stubDataLen ());
    memcpy (pMshlBufOut->buf, resultPdu.stubData (), resultPdu.stubDataLen ());
    pMshlBufOut->drep = resultPdu.drep ();
    
    return S_OK;
    }


//////////////////////////////////////////////////////////////////////////
//
// AllocBuffer - allocate a buffer for this channel
//

HRESULT RpcIfClient::AllocBuffer
    (
    MSHL_BUFFER*        pBuf,
    DWORD               nBytes
    )
    {
    void* pv = comSysAlloc (nBytes);
    if (! pv)
        return E_OUTOFMEMORY;

    pBuf->buf = (byte*) pv;
    pBuf->len = nBytes;
    return S_OK;
    }


//////////////////////////////////////////////////////////////////////////
//
// FreeBuffer - free a previously-allocated channel buffer
//

HRESULT RpcIfClient::FreeBuffer
    (
    MSHL_BUFFER*        pBuf
    )
    {
    comSysFree (pBuf->buf);
    pBuf->buf = 0;
    pBuf->len = 0;
    return S_OK;
    }


//////////////////////////////////////////////////////////////////////////
//

ULONG RpcIfClient::AddRef ()
    {
    return comSafeInc (&m_dwRefCount);
    }

//////////////////////////////////////////////////////////////////////////
//

ULONG RpcIfClient::Release ()
    {
    DWORD n = comSafeDec (&m_dwRefCount);
    if (n == 0)
	delete this;
    return n;
    }

//////////////////////////////////////////////////////////////////////////
//

HRESULT RpcIfClient::QueryInterface (REFIID riid, void** ppv)
    {
    if ((riid == IID_IOrpcClientChannel) ||
        (riid == IID_IUnknown))
        {
        *ppv = this;
        AddRef ();
        return S_OK;
        }

    return E_NOINTERFACE;
    }

//////////////////////////////////////////////////////////////////////////
//

HRESULT RpcIfClient::AddKnownInterface (REFIID iid)
    {
    // Add it to the list of 'bound' IIDs...
    m_boundIIDs.push_back (iid);
    return S_OK;
    }


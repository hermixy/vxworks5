/* RpcPdu.cpp - COM/DCOM RpcPdu class implementation */

/*
modification history
--------------------
01z,17dec01,nel  Add include symbol for diab build.
01y,10dec01,dbs  diab build
01x,08oct01,nel  SPR#70120. reformat ALTER_CONTEXT_RESP separately from
                 BIND_ACK.
01w,13jul01,dbs  fix up includes
01v,15aug00,nel  Win2K Fix.
01u,27jun00,dbs  NDR-format presCtxId
01t,22jun00,dbs  add accessors for alter_context_resp
01s,19aug99,aim  change assert to VXDCOM_ASSERT
01r,20jul99,aim  grow vector when building incoming pdu
01q,09jul99,dbs  copied from NRpcPdu into RpcPdu files
01p,07jul99,aim  removed debug from makeReplyBuffer
01o,06jul99,aim  added isBindNak
01n,02jul99,aim  added isBindAck
01m,25jun99,dbs  add authn properties
01l,24jun99,dbs  move authn into new class
01k,18jun99,aim  set data rep on outgoing packets
01j,17jun99,aim  changed assert to assert
01i,01jun99,dbs  add NDR formatting to FAULT packet header
01h,19may99,dbs  send packet in fragments so no copying required
01g,11may99,dbs  rename VXCOM to VXDCOM
01f,07may99,dbs  add method to return object ID
01e,29apr99,dbs  fix -Wall warnings
01d,28apr99,dbs  add mem-pool
01c,26apr99,aim  added TRACE_CALL
01b,21apr99,dbs  change RPCMSG to RpcPdu
01a,20apr99,dbs  created during Grand Renaming

*/

/*
  DESCRIPTION:

  RpcPdu -- RPC Protocol Data Unit class.

*/

#include <stdio.h>
#include <ctype.h>
#include "RpcPdu.h"
#include "orpcLib.h"
#include "dcomProxy.h"
#include "Syslog.h"
#include "TraceCall.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_RpcPdu (void)
    {
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//

RpcPdu::RpcPdu ()
  : m_headerOffset (0),
    m_stubData (),
    m_requiredOctets (sizeof (rpc_cn_common_hdr_t)),
    m_pduState (HAVE_NONE)
    {
    TRACE_CALL;
    m_headerOffset = reinterpret_cast<char*> (&m_header);
    (void) ::memset (&m_header, 0, sizeof (m_header));
    }

RpcPdu::~RpcPdu ()
    {
    TRACE_CALL;
    }

//////////////////////////////////////////////////////////////////////////
//

void RpcPdu::guidReformat
    (
    GUID&                guid,
    ULONG                drep
    )
    {
    TRACE_CALL;
    ndr_make_right (guid.Data1, drep);
    ndr_make_right (guid.Data2, drep);
    ndr_make_right (guid.Data3, drep);
    }

//////////////////////////////////////////////////////////////////////////
//

void RpcPdu::commonHdrReformat
    (
    ULONG                drep
    )
    {
    TRACE_CALL;
    rpc_cn_packet_t* pHdr = &m_header;
    
    ndr_make_right (pHdr->request.hdr.fragLen, drep);
    ndr_make_right (pHdr->request.hdr.authLen, drep);
    ndr_make_right (pHdr->request.hdr.callId, drep);
    }

//////////////////////////////////////////////////////////////////////////
//

void RpcPdu::extHdrReformat
    (
    ULONG                drep
    )
    {
    TRACE_CALL;
    rpc_cn_packet_t* pHdr = &m_header;
    
    switch (pHdr->request.hdr.packetType)
        {
        case RPC_CN_PKT_REQUEST:
            ndr_make_right (pHdr->request.allocHint, drep);
            ndr_make_right (pHdr->request.presCtxId, drep);
            ndr_make_right (pHdr->request.methodNum, drep);
            if (pHdr->request.hdr.flags & RPC_CN_FLAGS_OBJECT_UUID)
                guidReformat (pHdr->request.objectId, drep);
            break;

        case RPC_CN_PKT_RESPONSE:
            ndr_make_right (pHdr->request.allocHint, drep);
            ndr_make_right (pHdr->request.presCtxId, drep);
            break;

        case RPC_CN_PKT_BIND:
        case RPC_CN_PKT_ALTER_CONTEXT:
            ndr_make_right (pHdr->bind.maxTxFrag, drep);
            ndr_make_right (pHdr->bind.maxRxFrag, drep);
            ndr_make_right (pHdr->bind.assocGroupId, drep);
            guidReformat (pHdr->bind.presCtxList.presCtxElem
                          [0].abstractSyntax.id,
                          drep);
            ndr_make_right (pHdr->bind.presCtxList.presCtxElem
                 [0].abstractSyntax.version, drep);
            ndr_make_right (pHdr->bind.presCtxList.presCtxElem[0].presCtxId, 
                            drep);
            guidReformat (pHdr->bind.presCtxList.presCtxElem
                          [0].transferSyntax [0].id,
                          drep);
            ndr_make_right (pHdr->bind.presCtxList.presCtxElem
                 [0].transferSyntax [0].version, drep);
            break;

        case RPC_CN_PKT_BIND_ACK:
	    ndr_make_right (pHdr->bind_ack.maxTxFrag, drep);
	    ndr_make_right (pHdr->bind_ack.maxRxFrag, drep);
	    ndr_make_right (pHdr->bind_ack.assocGroupId, drep);
	    ndr_make_right (pHdr->bind_ack.secAddr.len, drep);
            ndr_make_right (pHdr->bind_ack.resultList.presResult [0].result, drep);
            ndr_make_right (pHdr->bind_ack.resultList.presResult [0].reason, drep);
	    ndr_make_right (pHdr->bind_ack.resultList.presResult
			    [0].transferSyntax.version, drep);
	    guidReformat (pHdr->bind_ack.resultList.presResult
			  [0].transferSyntax.id, drep);
            break;




        case RPC_CN_PKT_ALTER_CONTEXT_RESP:
	    ndr_make_right (pHdr->alter_context_resp.maxTxFrag, drep);
	    ndr_make_right (pHdr->alter_context_resp.maxRxFrag, drep);
	    ndr_make_right (pHdr->alter_context_resp.assocGroupId, drep);
	    ndr_make_right (pHdr->alter_context_resp.secAddr, drep);
            ndr_make_right (pHdr->alter_context_resp.resultList.presResult [0].result, drep);
            ndr_make_right (pHdr->alter_context_resp.resultList.presResult [0].reason, drep);
	    ndr_make_right (pHdr->alter_context_resp.resultList.presResult
			    [0].transferSyntax.version, drep);
	    guidReformat (pHdr->alter_context_resp.resultList.presResult
			  [0].transferSyntax.id, drep);
            break;


        case RPC_CN_PKT_FAULT:
            ndr_make_right (pHdr->fault.status, drep);
            break;
        }
    }

//////////////////////////////////////////////////////////////////////////
//

const rpc_cn_auth_tlr_t*
RpcPdu::authTrailer () const
    {
    TRACE_CALL;

    ULONG alen = authLen ();

    if (alen == 0)
        return 0;

#ifdef __DCC__
    const char* stubDataOffset = &(*(m_stubData.begin ()));
#else
    const char* stubDataOffset = reinterpret_cast<const char*>
                                 (m_stubData.begin ());
#endif
    
    if (stubDataOffset == 0)
        return 0;

    return reinterpret_cast<rpc_cn_auth_tlr_t*>
        (const_cast<char *>
        (stubDataOffset + (payloadLen () -
                           (alen + SIZEOF_AUTH_TLR_PREAMBLE))));
    }

// this always includes the auth-trailer if present
size_t
RpcPdu::payloadLen () const
    {
    TRACE_CALL;
    return m_stubData.size ();
    }

// this *only* the stub data length
size_t RpcPdu::stubDataLen () const
    {
    return (payloadLen () - authLen ());
    }


ULONG
RpcPdu::hdrLen () const
    {
    TRACE_CALL;
    ULONG n = 0;

    switch (m_header.request.hdr.packetType)
        {
        case RPC_CN_PKT_REQUEST:
            n = sizeof (rpc_cn_request_hdr_t);
            if (! (m_header.request.hdr.flags & RPC_CN_FLAGS_OBJECT_UUID))
                n -= sizeof (GUID);
            break;

        case RPC_CN_PKT_RESPONSE:
            n = sizeof (rpc_cn_response_hdr_t);
            break;

        case RPC_CN_PKT_BIND:
        case RPC_CN_PKT_ALTER_CONTEXT:
            n = sizeof (rpc_cn_bind_hdr_t);
            break;

        case RPC_CN_PKT_BIND_ACK:
            n = sizeof (rpc_cn_bind_ack_hdr_t);
            break;

        case RPC_CN_PKT_ALTER_CONTEXT_RESP:
            n = sizeof (rpc_cn_alter_context_resp_hdr_t);
            break;

        case RPC_CN_PKT_BIND_NAK:
            n = sizeof (rpc_cn_bind_nak_hdr_t);
            break;

        case RPC_CN_PKT_AUTH3:
            n = sizeof (rpc_cn_auth3_hdr_t);
            break;
            
        case RPC_CN_PKT_SHUTDOWN:
        case RPC_CN_PKT_REMOTE_ALERT:
        case RPC_CN_PKT_ORPHANED:
            n = sizeof (rpc_cn_common_hdr_t);
            break;

        case RPC_CN_PKT_FAULT:
            n = sizeof (rpc_cn_fault_hdr_t);
            break;

        default:
            // log warning
            n = 4096;
            break;
        }

    return n;
    }
    
void
RpcPdu::opnumSet (USHORT op)
    {
    TRACE_CALL;
    m_header.request.methodNum = op;
    }

void
RpcPdu::fragLenSet ()
    {
    TRACE_CALL;
    int len = hdrLen () + payloadLen ();
    m_header.request.hdr.fragLen = len;
    if (packetType () == RPC_CN_PKT_REQUEST)
        m_header.request.allocHint = len;
    else if (packetType () == RPC_CN_PKT_RESPONSE)
        m_header.response.allocHint = len;
    }

void
RpcPdu::authLenSet (size_t n)
    {
    TRACE_CALL;
    m_header.request.hdr.authLen = n;
    }

void
RpcPdu::drepSet ()
    {
    TRACE_CALL;
    * (ULONG*) m_header.request.hdr.drep = ORPC_WIRE_DREP;
    }

void
RpcPdu::drepSet (ULONG dr)
    {
    TRACE_CALL;
    * (ULONG*) m_header.request.hdr.drep = dr;
    }

size_t
RpcPdu::stubDataAppend (const void* pv, size_t n)
    {
    TRACE_CALL;
    const char* p = reinterpret_cast<const char*> (pv);
    m_stubData.insert (m_stubData.end (), p, p +n);
    return n;
    }

const GUID& RpcPdu::objectId () const
    {
    TRACE_CALL;

    if (m_header.request.hdr.flags & RPC_CN_FLAGS_OBJECT_UUID)
        return m_header.request.objectId;

    return GUID_NULL;
    }

///////////////////////////////////////////////////////////////////////////////
//

bool
RpcPdu::complete () const
    {
    return m_pduState & HAVE_STUB;
    }

size_t
RpcPdu::appendCommonHdr (const char* buf, size_t len)
    {
    COM_ASSERT (m_pduState & HAVE_NONE);

    size_t n = min(len, m_requiredOctets); // octets to copy

    ::memcpy (m_headerOffset, buf, n);

    m_headerOffset += n;
    m_requiredOctets -= n;

    if (m_requiredOctets == 0)
        {
        // Un-NDR the packet header...
        commonHdrReformat (drep ());

        // mark the next state's requirements
        m_pduState = HAVE_HDR;
        m_requiredOctets = hdrLen () - sizeof (rpc_cn_common_hdr_t);

        if (len - n > 0)
            n += appendExtendedHdr (buf + n, len - n);
        }

    return n;
    }

size_t
RpcPdu::appendExtendedHdr (const char* buf, size_t len)
    {
    COM_ASSERT (m_pduState & HAVE_HDR);

    size_t n = min(len, m_requiredOctets); // octets to copy

    ::memcpy (m_headerOffset, buf, n);

    m_headerOffset += n;
    m_requiredOctets -= n;

    if (m_requiredOctets == 0)
        {
        // Un-NDR the remainder of the header...
        extHdrReformat (drep ());

        // mark the next state's requirements
        m_pduState = HAVE_XHDR;
        m_requiredOctets = fragLen () - hdrLen ();

        if (m_requiredOctets > 0)
            m_stubData.reserve (m_requiredOctets);

        if (m_requiredOctets > 0 && (len - n) > 0)
            n += appendStubData (buf + n, len - n);
        else if (m_requiredOctets == 0)
            m_pduState = HAVE_STUB;
        }

    return n;
    }

size_t
RpcPdu::appendStubData (const char* buf, size_t len)
    {
    size_t n = min(len, m_requiredOctets); // octets to copy

    m_stubData.insert (m_stubData.end (), buf, buf + n);

    if ((m_requiredOctets -= n) == 0)
        m_pduState = HAVE_STUB;

    return n;
    }

size_t
RpcPdu::append (const char* buf, size_t len)
    {
    if (len < 0)
        return 0;

    switch (m_pduState)
        {
    case HAVE_NONE:
        return appendCommonHdr (buf, len);
    case HAVE_HDR:
        return appendExtendedHdr (buf, len);
    case HAVE_XHDR:
        return appendStubData (buf, len);
    default:
        return 0;
        }

    return 0;
    }

const rpc_cn_common_hdr_t*
RpcPdu::commonHdr () const
    {
    return &m_header.request.hdr;
    }

size_t
RpcPdu::commonHdrLen () const
    {
    return sizeof (m_header.request.hdr);
    }
    
const rpc_cn_bind_ack_hdr_t&
RpcPdu::bind_ack () const
    {
    return m_header.bind_ack;
    }

const rpc_cn_bind_nak_hdr_t&
RpcPdu::bind_nak () const
    {
    return m_header.bind_nak;
    }

const rpc_cn_bind_hdr_t&
RpcPdu::bind () const
    {
    return m_header.bind;
    }

const rpc_cn_request_hdr_t&
RpcPdu::request () const
    {
    return m_header.request;
    }
    
const rpc_cn_response_hdr_t&
RpcPdu::response () const
    {
    return m_header.response;
    }

const rpc_cn_auth3_hdr_t&
RpcPdu::auth3 () const
    {
    return m_header.auth3;
    }

const rpc_cn_fault_hdr_t&
RpcPdu::fault () const
    {
    return m_header.fault;
    }

const rpc_cn_alter_context_resp_hdr_t&
RpcPdu::alter_context_resp () const
    {
    return m_header.alter_context_resp;
    }

rpc_cn_common_hdr_t*
RpcPdu::commonHdr () 
    {
    return &m_header.request.hdr;
    }
    
rpc_cn_bind_ack_hdr_t&
RpcPdu::bind_ack () 
    {
    return m_header.bind_ack;
    }
    
rpc_cn_bind_nak_hdr_t&
RpcPdu::bind_nak ()
    {
    return m_header.bind_nak;
    }

rpc_cn_bind_hdr_t&
RpcPdu::bind ()
    {
    return m_header.bind;
    }

rpc_cn_request_hdr_t&
RpcPdu::request ()
    {
    return m_header.request;
    }
    
rpc_cn_response_hdr_t&
RpcPdu::response ()
    {
    return m_header.response;
    }

rpc_cn_auth3_hdr_t&
RpcPdu::auth3 ()
    {
    return m_header.auth3;
    }

rpc_cn_fault_hdr_t&
RpcPdu::fault ()
    {
    return m_header.fault;
    }

rpc_cn_alter_context_resp_hdr_t&
RpcPdu::alter_context_resp ()
    {
    return m_header.alter_context_resp;
    }

ULONG
RpcPdu::drep () const
    {
    return * (reinterpret_cast<const ULONG*> (m_header.request.hdr.drep));
    }

ULONG 
RpcPdu::opnum () const
    {
    return m_header.request.methodNum;
    }

ULONG
RpcPdu::callId () const
    {
    return m_header.request.hdr.callId;
    }

int
RpcPdu::packetType () const
    {
    return m_header.request.hdr.packetType;
    }

USHORT
RpcPdu::fragLen () const
    {
    return m_header.request.hdr.fragLen;
    }

ULONG
RpcPdu::authLen () const
    {
    return m_header.request.hdr.authLen;
    }

const void*
RpcPdu::stubData () const
    {
#ifdef __DCC__
    return &(*(m_stubData.begin ()));
#else
    return m_stubData.begin ();
#endif
    }

void*
RpcPdu::stubData ()
    {
#ifdef __DCC__
    return &(*(m_stubData.begin ()));
#else
    return m_stubData.begin ();
#endif
    }

bool
RpcPdu::isRequest () const
    {
    return (packetType () == RPC_CN_PKT_REQUEST);
    }

bool
RpcPdu::isBind () const
    {
    return ((packetType () == RPC_CN_PKT_BIND) ||
            (packetType () == RPC_CN_PKT_ALTER_CONTEXT));
    }

bool
RpcPdu::isBindAck () const
    {
    return (packetType () == RPC_CN_PKT_BIND_ACK);
    }

bool
RpcPdu::isBindNak () const
    {
    return (packetType () == RPC_CN_PKT_BIND_NAK);
    }

bool
RpcPdu::isAuth3 () const
    {
    return (packetType () == RPC_CN_PKT_AUTH3);
    }

bool
RpcPdu::isResponse () const
    {
    return (packetType () == RPC_CN_PKT_RESPONSE);
    }

bool
RpcPdu::isFault () const
    {
    return (packetType () == RPC_CN_PKT_FAULT);
    }

bool
RpcPdu::isAlterContextResp () const
    {
    return (packetType () == RPC_CN_PKT_ALTER_CONTEXT_RESP);
    }

int
RpcPdu::makeReplyBuffer (char*& buf, size_t& buflen)
    {
    int hdrLength = hdrLen ();
    int payloadLength = payloadLen ();

    buflen = hdrLength + payloadLength;

    buf = new char[buflen];

    if (buf)
        {
        // Set the fragment-length field, and the DREP...
        fragLenSet ();
        drepSet ();

        // Format the packet into the correct NDR mode...
        commonHdrReformat (drep ());
        extHdrReformat (drep ());

        char* header = reinterpret_cast<char*> (&m_header);

        ::memcpy (buf, header, hdrLength);

        if (payloadLength > 0)
            ::memcpy (buf + hdrLength, stubData (), payloadLength);
        }

    return buf ? 0 : -1;
    }

GUID
RpcPdu::iid () const
    {
    // assumes its a bind packet
    return bind ().presCtxList.presCtxElem [0].abstractSyntax.id;
    }

ostream&
operator<< (ostream& os, const RpcPdu& pdu)
    {
    char hex_string [64];
    char chr_string [32];
    char details [8192];        // XXX
    char* s = "UNKNOWN!";
    int nBytes = pdu.payloadLen ();

    // Print packet type
    details [0] = 0;

    switch (pdu.packetType ())
        {
        case RPC_CN_PKT_REQUEST:
            s="REQUEST";
            sprintf (details, "allocHint=%lu pres-ctx=%u opnum=%u objUuid=%s",
                     pdu.request ().allocHint,
                     pdu.request ().presCtxId,
                     pdu.request ().methodNum,
                     (pdu.request ().hdr.flags & RPC_CN_FLAGS_OBJECT_UUID) ? 
                     vxcomGUID2String (pdu.request ().objectId) : "not-present");
            break;

        case RPC_CN_PKT_RESPONSE:
            s="RESPONSE";
            sprintf (details, "allocHint=%lu pres-ctx=%u alertCount=%u",
                     pdu.response ().allocHint,
                     pdu.response ().presCtxId,
                     pdu.response ().alertCount);
            break;

        case RPC_CN_PKT_FAULT: s="FAULT";
            sprintf (details, "faultCode=0x%lx", pdu.fault().status);
            break;

        case RPC_CN_PKT_ALTER_CONTEXT:
        case RPC_CN_PKT_BIND:
            if (pdu.packetType () == RPC_CN_PKT_BIND)
                s="BIND";
            else
                s="ALTER CONTEXT";
            sprintf (details,
                     "max-txFragSize=%u max-rxFragSize=%u assoc-grp-ID=0x%8.8lX pres-ctx=%d, IID=%s",
                     pdu.bind ().maxTxFrag,
                     pdu.bind ().maxRxFrag,
                     pdu.bind ().assocGroupId,
                     pdu.bind().presCtxList.presCtxElem[0].presCtxId,
                     vxcomGUID2String (pdu.bind ().presCtxList.presCtxElem [0].abstractSyntax.id));
            break;

        case RPC_CN_PKT_ALTER_CONTEXT_RESP:
        case RPC_CN_PKT_BIND_ACK:
            if (pdu.packetType () == RPC_CN_PKT_BIND_ACK)
                s="BIND ACK";
            else
                s="ALTER CONTEXT RESP";
            sprintf (details,
                     "max-txFragSize=%u max-rxFragSize=%u assoc-grp-ID=0x%8.8lX result=%u",
                     pdu.bind_ack ().maxTxFrag,
                     pdu.bind_ack ().maxRxFrag,
                     pdu.bind_ack ().assocGroupId,
                     pdu.bind_ack ().resultList.presResult [0].result);
            break;

        case RPC_CN_PKT_BIND_NAK:
            s="BIND NAK";
            break;

        case RPC_CN_PKT_AUTH3: s="AUTH3"; break;
        case RPC_CN_PKT_SHUTDOWN: s="SHUTDOWN"; break;
        case RPC_CN_PKT_REMOTE_ALERT: s="ALERT"; break;
        case RPC_CN_PKT_ORPHANED: s="ORPHANED"; break;
        }
    
    os << s << " " << details;

    // Print common-header info...
    sprintf (details,
             "ver=%d.%d flags=0x%2.2X fragmentLength=0x%X authLen=%u callID=%lu intDataRep=%s",
             pdu.request ().hdr.rpcVersion,
             pdu.request ().hdr.rpcMinorVersion,
             pdu.request ().hdr.flags,
             pdu.request ().hdr.fragLen,
             pdu.request ().hdr.authLen,
             pdu.request ().hdr.callId,
             pdu.request ().hdr.drep [0] ? "littleEndian" :
             "bigEndian");

    os << " " << details << endl;

    if (nBytes > 0)
        {
        os << " payloadLen: " << nBytes << endl;

        // Binary/hex dump...
        char* pbase = (char*) pdu.stubData ();
        char* p = pbase;
        while (nBytes > 0)
            {
            char*   phex = hex_string;
            char*   pchr = chr_string;
            
            for (int j=0; j < 16; ++j)
                {
                if (nBytes > 0)
                    {
                    int val = p [j] & 0xFF;
                    sprintf (phex, "%02X ", val);
                    sprintf (pchr, "%c", isprint (val) ? val : '.');
                    }
                else
                    sprintf (phex, "   ");

                phex += 3;
                ++pchr;
                --nBytes;
                }

              void* a = reinterpret_cast<void*> (p - pbase);
              os << a << ": " << hex_string << " " << chr_string << endl;

            *phex = 0;
            *pchr = 0;

            p += 16;
            }
        }

    return os;
    }

///////////////////////////////////////////////////////////////////////////////
//

void
RpcPdu::stubDataAlign (size_t n)
    {
    while ((m_stubData.size () % n) != 0)
        m_stubData.push_back (0);
    }

//////////////////////////////////////////////////////////////////////////
//
// authTrailerAppend -- appends the trailer in 'tlr' to the current
// packet, its auth-data is of length 'authlen'...
//

HRESULT RpcPdu::authTrailerAppend
    (
    const rpc_cn_auth_tlr_t&        tlr,
    size_t                        authlen
    )
    {
    stubDataAlign (4);
    stubDataAppend (&tlr, authlen + SIZEOF_AUTH_TLR_PREAMBLE);
    authLenSet (authlen);
    return S_OK;
    }


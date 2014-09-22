/* RpcPdu.h - COM/DCOM RpcPdu class definition */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01g,13jul01,dbs  fix up includes
01f,22jun00,dbs  add accessors for alter_context_resp
01e,06jul99,aim  added isBindNak
01d,02jul99,aim  added isBindAck
01c,25jun99,dbs  add authn properties
01b,24jun99,dbs  move authn into new class
01a,03jun99,aim  created from RpcPdu.h
*/

#ifndef __INCRpcPdu_h
#define __INCRpcPdu_h

#include <iostream>
#include "rpcDceProto.h"
#include "private/comStl.h"

//////////////////////////////////////////////////////////////////////////
//
// RpcPdu - this structure is used for conveying entire RPC packets
// around, removing as many copy-operations as possible. Entire packets
// can be allocated in one fell swoop, and passed from the raw receive
// procedure, to the point of being processed, and back to the transmit
// procedure, without having to do memcpy().
//
// The member 'm_bufptr' always points to the next free location in the
// buffer, so we can always insert extra bytes there.
//

class RpcPdu
    {
  public:

    // max packet size we want to see is 4K
    enum { MAXLEN=4096 };
    
    RpcPdu ();
    ~RpcPdu ();
    
    bool complete () const;
    size_t append (const char*, size_t len);

    // return address of various packet-header fields...

    size_t commonHdrLen () const;

    const rpc_cn_common_hdr_t* commonHdr () const;
    const rpc_cn_bind_ack_hdr_t& bind_ack () const;
    const rpc_cn_bind_nak_hdr_t& bind_nak () const;
    const rpc_cn_bind_hdr_t& bind () const;
    const rpc_cn_request_hdr_t& request () const;
    const rpc_cn_response_hdr_t& response () const;
    const rpc_cn_auth3_hdr_t& auth3 () const;
    const rpc_cn_fault_hdr_t& fault () const;
    const rpc_cn_alter_context_resp_hdr_t& alter_context_resp () const;

    operator rpc_cn_common_hdr_t& ();
    operator rpc_cn_bind_ack_hdr_t& ();
    operator rpc_cn_bind_nak_hdr_t& ();
    operator rpc_cn_bind_hdr_t& ();
    operator rpc_cn_request_hdr_t& ();
    operator rpc_cn_response_hdr_t& ();
    operator rpc_cn_auth3_hdr_t& ();
    operator rpc_cn_fault_hdr_t& ();

    operator const rpc_cn_common_hdr_t& ();
    operator const rpc_cn_bind_ack_hdr_t& ();
    operator const rpc_cn_bind_nak_hdr_t& ();
    operator const rpc_cn_bind_hdr_t& ();
    operator const rpc_cn_request_hdr_t& ();
    operator const rpc_cn_response_hdr_t& ();
    operator const rpc_cn_auth3_hdr_t& ();
    operator const rpc_cn_fault_hdr_t& ();

    rpc_cn_common_hdr_t* commonHdr ();
    rpc_cn_bind_ack_hdr_t& bind_ack ();
    rpc_cn_bind_nak_hdr_t& bind_nak ();
    rpc_cn_bind_hdr_t& bind ();
    rpc_cn_request_hdr_t& request ();
    rpc_cn_response_hdr_t& response ();
    rpc_cn_auth3_hdr_t& auth3 ();
    rpc_cn_fault_hdr_t& fault ();
    rpc_cn_alter_context_resp_hdr_t& alter_context_resp ();

    // methods to return stub-data start/length...
    const void* stubData () const;
    void*  stubData ();
    size_t stubDataLen () const;
    size_t stubDataAppend (const void* pv, size_t n);
    void   stubDataAlign (size_t n);

    // size of stub-data + auth-trailer
    size_t payloadLen () const;

    // methods to access authn-trailer...
    const rpc_cn_auth_tlr_t* authTrailer () const;
    ULONG authLen () const;
    HRESULT authTrailerAppend (const rpc_cn_auth_tlr_t&, size_t);
    
    // methods to return useful packet-header fields
    ULONG drep () const;
    ULONG opnum () const;
    ULONG callId () const;
    int packetType () const;
    USHORT fragLen () const;
    ULONG hdrLen () const;
    const GUID& objectId () const;
    
    // methods to modify header fields
    void opnumSet (USHORT op);
    void fragLenSet ();
    void drepSet ();
    void drepSet (ULONG);
    void authLenSet (size_t);

    GUID iid () const;
    bool isRequest () const;
    bool isBind () const;
    bool isBindAck () const;
    bool isBindNak () const;
    bool isAuth3 () const;
    bool isResponse () const;
    bool isFault () const;
    bool isAlterContextResp () const;
    
    int makeReplyBuffer (char*& buf, size_t& buflen);

    friend ostream& operator<< (ostream& os, const RpcPdu&);

  private:

    typedef STL_VECTOR(char) Stub;
    typedef STL_VECTOR(char)::iterator StubIter;

    rpc_cn_packet_t	m_header;
    char*		m_headerOffset;
    Stub                m_stubData;
    size_t		m_requiredOctets;
    unsigned long	m_pduState;

    enum { HAVE_NONE = 0x1,	// pduState
	   HAVE_HDR  = 0x2,
	   HAVE_XHDR = 0x4,
	   HAVE_STUB = 0x8 };

    // NDR-reformat header
    void commonHdrReformat (ULONG drep);
    void extHdrReformat (ULONG drep);

    size_t appendCommonHdr (const char* buf, size_t len);
    size_t appendExtendedHdr (const char* buf, size_t len);
    size_t appendStubData (const char* buf, size_t len);

    static void guidReformat (GUID&, ULONG drep);

    // unsupported
    RpcPdu (const RpcPdu&);
    RpcPdu& operator= (const RpcPdu&);
    };

#endif // __INCRpcPdu_h


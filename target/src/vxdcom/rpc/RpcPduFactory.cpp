/* RpcPduFactory - create DCOM protocol requests/responses */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01p,17dec01,nel  Add include symbol for diab build.
01o,08oct01,nel  SPR#70120. reformat ALTER_CONTEXT_RESP separately from
                 BIND_ACK.
01n,13jul01,dbs  add pres ctx ID to request pdu
01m,29mar01,nel  SPR#35873. Add context id parameter to formatBindPdu to allow
                 client side ctxId to be specified.
01l,16aug00,nel  Win2K patch.
01k,27jul00,dbs  fix pres-ctx-id returned in FAULT and RESPONSE packets
01j,26jun00,dbs  explicitly set presCtxId value in BIND msg
                 add 'did not execute' flag to FAULT
01i,22jun00,dbs  fix handling of alter-context packets
01h,19aug99,aim  added TraceCall header
01g,19jul99,dbs  add support for AUTH3 packets
01f,02jul99,aim  renamed makeBindFormatPdu and makeRequestFormatPdu
01e,24jun99,dbs  move authn into new class
01d,23jun99,dbs  fix authn on response packets
01c,08jun99,aim  rework
01b,08jun99,aim  now uses NRpcPdu
01a,03jun99,aim  created
*/

#include "RpcPduFactory.h"
#include "dcomProxy.h"
#include "ntlmssp.h"
#include "TraceCall.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_RpcPduFactory (void)
    {
    return 0;
    }

extern IID IID_ISystemActivator;

RpcPduFactory::RpcPduFactory ()
    {
    TRACE_CALL;
    }

RpcPduFactory::~RpcPduFactory ()
    {
    TRACE_CALL;
    }

BYTE
RpcPduFactory::rpcMajorVersion ()
    {
    return 5;
    }

BYTE
RpcPduFactory::rpcMinorVersion ()
    {
    return 0;
    }

void
RpcPduFactory::formatFaultPdu
    (
    const RpcPdu&	request,
    RpcPdu&		faultPdu,
    HRESULT		faultCode,
    bool		didNotExecute
    )
    {
    TRACE_CALL;

    RpcPdu& pdu = faultPdu; // shorter, easier!

    pdu.fault ().hdr.rpcVersion      = rpcMajorVersion ();
    pdu.fault ().hdr.rpcMinorVersion = rpcMinorVersion ();
    pdu.fault ().hdr.packetType      = RPC_CN_PKT_FAULT;
    pdu.fault ().hdr.flags           = RPC_CN_FLAGS_FIRST_FRAG |
				       RPC_CN_FLAGS_LAST_FRAG;

    if (didNotExecute)
	pdu.fault().hdr.flags |= RPC_CN_FLAGS_DID_NOT_EXECUTE;
    
    pdu.fault ().hdr.drep [0]        = VXDCOM_NDR_DATAREP0;
    pdu.fault ().hdr.drep [1]        = VXDCOM_NDR_DATAREP1;
    pdu.fault ().hdr.drep [2]        = VXDCOM_NDR_DATAREP2;
    pdu.fault ().hdr.drep [3]        = VXDCOM_NDR_DATAREP3;
    pdu.fault ().hdr.callId          = request.callId ();

    pdu.fault ().allocHint           = 0; // XXX fix up later
    pdu.fault ().presCtxId           = request.request().presCtxId;
    pdu.fault ().alertCount          = 0;
    pdu.fault ().reserved            = 0;
    pdu.fault ().status              = faultCode;
    pdu.fault ().reserved2           = 0;
    }

void
RpcPduFactory::formatBindAckPdu
    (
    const RpcPdu&	request,
    RpcPdu&		bindAckPdu,
    ULONG               assocGroupId
    )
    {
    TRACE_CALL;

    // Handle ALTER_CONTEXT response separately now...
    if (request.packetType () == RPC_CN_PKT_ALTER_CONTEXT)
	{
	formatAlterContextRespPdu (request, bindAckPdu);
	return;
	}

    // If the input is a BIND PDU then we need to send a BIND_ACK...

    GUID		xferSyntax;
    DWORD		version;
    RpcPdu&		pdu = bindAckPdu; // shorter, easier!

    xferSyntax = request.bind
		 ().presCtxList.presCtxElem[0].transferSyntax [0].id;

    version = request.bind
	      ().presCtxList.presCtxElem[0].transferSyntax [0].version;

    pdu.bind_ack ().hdr.rpcVersion = rpcMajorVersion ();
    pdu.bind_ack ().hdr.rpcMinorVersion = rpcMinorVersion ();

    pdu.bind_ack ().hdr.packetType = RPC_CN_PKT_BIND_ACK;

    pdu.bind_ack ().hdr.flags =      RPC_CN_FLAGS_FIRST_FRAG |
                                     RPC_CN_FLAGS_LAST_FRAG;
    
    pdu.bind_ack ().hdr.drep [0] = VXDCOM_NDR_DATAREP0;
    pdu.bind_ack ().hdr.drep [1] = VXDCOM_NDR_DATAREP1;
    pdu.bind_ack ().hdr.drep [2] = VXDCOM_NDR_DATAREP2;
    pdu.bind_ack ().hdr.drep [3] = VXDCOM_NDR_DATAREP3;

    pdu.bind_ack ().hdr.callId = request.callId ();

    pdu.bind_ack ().maxTxFrag = RpcPdu::MAXLEN;
    pdu.bind_ack ().maxRxFrag = RpcPdu::MAXLEN;

    // XXX aim
    pdu.bind_ack ().assocGroupId = (ULONG) assocGroupId;

    pdu.bind_ack ().secAddr.len = 4; // ???
    pdu.bind_ack ().secAddr.addr [0] = 0; // ???

    pdu.bind_ack ().resultList.numResults = 1;
    pdu.bind_ack ().resultList.presResult [0].result = 0; // accept
    pdu.bind_ack ().resultList.presResult [0].reason = 0; // not specified


    if (IsEqualGUID(request.bind().presCtxList.presCtxElem[0].abstractSyntax.id, 
                    IID_ISystemActivator))
        {
        // DCE spec lists these codes as network congestion. NT4 returns them
        // when queried for the ISystemActivator interface. This was arrived at
        // by examining the wire protocol.
        pdu.bind_ack ().resultList.presResult [0].result = 0x02;
        pdu.bind_ack ().resultList.presResult [0].reason = 0x01;
        pdu.bind_ack ().resultList.presResult [0].transferSyntax.id = GUID_NULL;
        pdu.bind_ack ().resultList.presResult [0].transferSyntax.version = 0;
        }
    else
        {
        pdu.bind_ack ().resultList.presResult [0].result = 0; // accept
        pdu.bind_ack ().resultList.presResult [0].reason = 0; // not specified
        pdu.bind_ack ().resultList.presResult [0].transferSyntax.id 
                                                                = xferSyntax;
        pdu.bind_ack ().resultList.presResult [0].transferSyntax.version 
                                                                = version;
        }
    }

void
RpcPduFactory::formatAlterContextRespPdu
    (
    const RpcPdu&	request,
    RpcPdu&		alterContextRespPdu
    )
    {
    TRACE_CALL;

    RpcPdu&		pdu = alterContextRespPdu; // shorter, easier!

    pdu.alter_context_resp().hdr.rpcVersion = rpcMajorVersion ();
    pdu.alter_context_resp().hdr.rpcMinorVersion = rpcMinorVersion ();

    pdu.alter_context_resp().hdr.packetType = RPC_CN_PKT_ALTER_CONTEXT_RESP;
    pdu.alter_context_resp().hdr.flags = 0;

    pdu.alter_context_resp().hdr.drep [0] = VXDCOM_NDR_DATAREP0;
    pdu.alter_context_resp().hdr.drep [1] = VXDCOM_NDR_DATAREP1;
    pdu.alter_context_resp().hdr.drep [2] = VXDCOM_NDR_DATAREP2;
    pdu.alter_context_resp().hdr.drep [3] = VXDCOM_NDR_DATAREP3;

    pdu.alter_context_resp().hdr.callId = request.callId ();

    pdu.alter_context_resp().maxTxFrag = RpcPdu::MAXLEN;
    pdu.alter_context_resp().maxRxFrag = RpcPdu::MAXLEN;

    pdu.alter_context_resp().assocGroupId = 0;

    pdu.alter_context_resp().secAddr = 0;
    pdu.alter_context_resp().pad = 0;

    pdu.alter_context_resp().resultList.numResults = 1;

    if (IsEqualGUID(request.bind().presCtxList.presCtxElem[0].abstractSyntax.id, 
                    IID_ISystemActivator))
        {
        // DCE spec lists these codes as network congestion. NT4 returns them
        // when queried for the ISystemActivator interface. This was arrived at
        // by examining the wire protocol.
        pdu.alter_context_resp().resultList.presResult [0].result = 0x02;
        pdu.alter_context_resp().resultList.presResult [0].reason = 0x01;
        pdu.alter_context_resp().resultList.presResult [0].transferSyntax.id = GUID_NULL;
        pdu.alter_context_resp().resultList.presResult [0].transferSyntax.version = 0;
        }
    else
        {
        pdu.alter_context_resp().resultList.presResult [0].result = 0; // accept
        pdu.alter_context_resp().resultList.presResult [0].reason = 0; // not specified
        pdu.alter_context_resp().resultList.presResult [0].transferSyntax.id
            = request.bind().presCtxList.presCtxElem[0].transferSyntax [0].id;

        pdu.alter_context_resp().resultList.presResult [0].transferSyntax.version
            = request.bind().presCtxList.presCtxElem[0].transferSyntax [0].version;
        }
    }

void
RpcPduFactory::formatBindNakPdu
    (
    const RpcPdu&	request,
    RpcPdu&		bindNakPdu
    )
    {
    TRACE_CALL;

    RpcPdu&		pdu = bindNakPdu; // shorter, easier!

    pdu.bind_nak ().hdr.rpcVersion = rpcMajorVersion ();
    pdu.bind_nak ().hdr.rpcMinorVersion = rpcMinorVersion ();
    pdu.bind_nak ().hdr.packetType = RPC_CN_PKT_BIND_NAK;
    pdu.bind_nak ().hdr.flags = RPC_CN_FLAGS_FIRST_FRAG |
				RPC_CN_FLAGS_LAST_FRAG;
    pdu.bind_nak ().hdr.drep [0] = VXDCOM_NDR_DATAREP0;
    pdu.bind_nak ().hdr.drep [1] = VXDCOM_NDR_DATAREP1;
    pdu.bind_nak ().hdr.drep [2] = VXDCOM_NDR_DATAREP2;
    pdu.bind_nak ().hdr.drep [3] = VXDCOM_NDR_DATAREP3;
    pdu.bind_nak ().hdr.callId = request.callId ();

    pdu.bind_nak ().reason = 0;
    pdu.bind_nak ().numProtocols = 1;
    pdu.bind_nak ().verMajor = rpcMajorVersion ();
    pdu.bind_nak ().verMinor = rpcMinorVersion ();
    }

void
RpcPduFactory::formatResponsePdu
    (
    const RpcPdu&	request,
    RpcPdu&		responsePdu
    )
    {
    TRACE_CALL;

    // Handle auth info - any outgoing trailer must be appended to the
    // stub-data, and must be 4-byte aligned...

    RpcPdu& pdu = responsePdu; // shorter, easier!

    // Now fill in response fields...
    pdu.response ().hdr.rpcVersion = rpcMajorVersion ();
    pdu.response ().hdr.rpcMinorVersion = rpcMinorVersion ();
    pdu.response ().hdr.packetType = RPC_CN_PKT_RESPONSE;
    pdu.response ().hdr.flags = RPC_CN_FLAGS_FIRST_FRAG |
				  RPC_CN_FLAGS_LAST_FRAG;
    pdu.response ().hdr.callId = request.callId ();

    pdu.response ().allocHint  = 0; // XXX fix up later
    pdu.response ().presCtxId  = request.request().presCtxId;
    pdu.response ().alertCount = 0;
    pdu.response ().reserved   = 0;
    }

void
RpcPduFactory::formatBindPdu
    (
    RpcPdu&		result,
    REFIID		riid,
    ULONG		callId,
    ULONG		assocGroupId,
    bool		alterCtxt,
    USHORT		ctxId
    )
    {
    // Build BIND packet

    result.bind().hdr.rpcVersion = rpcMajorVersion ();
    result.bind().hdr.rpcMinorVersion = rpcMinorVersion ();

    if (alterCtxt)
        result.bind().hdr.packetType = RPC_CN_PKT_ALTER_CONTEXT;
    else
        result.bind().hdr.packetType = RPC_CN_PKT_BIND;

    result.bind().hdr.flags = 0;

    result.bind().hdr.drep [0] = VXDCOM_NDR_DATAREP0;
    result.bind().hdr.drep [1] = VXDCOM_NDR_DATAREP1;
    result.bind().hdr.drep [2] = VXDCOM_NDR_DATAREP2;
    result.bind().hdr.drep [3] = VXDCOM_NDR_DATAREP3;
    result.bind().hdr.callId = callId;
    result.bind().maxTxFrag = RpcPdu::MAXLEN;
    result.bind().maxRxFrag = RpcPdu::MAXLEN;
    result.bind().assocGroupId = assocGroupId;
    result.bind().presCtxList.numCtxElems = 1;
    result.bind().presCtxList.presCtxElem [0].abstractSyntax.id = riid;
    result.bind().presCtxList.presCtxElem [0].abstractSyntax.version = 0;
    result.bind().presCtxList.presCtxElem [0].presCtxId = ctxId;
    result.bind().presCtxList.presCtxElem [0].nTransferSyntaxes = 1;
    result.bind().presCtxList.presCtxElem [0].transferSyntax [0].id = DCOM_XFER_SYNTAX;
    result.bind().presCtxList.presCtxElem [0].transferSyntax [0].version = 2;
    }

//////////////////////////////////////////////////////////////////////////
//

void
RpcPduFactory::formatRequestPdu
    (
    RpcPdu&		result,
    ULONG		callId,
    USHORT		opnum,
    const GUID*		pObjectUuid,
    USHORT              presCtxId
    )
    {
    result.request().hdr.rpcVersion = rpcMajorVersion ();
    result.request().hdr.rpcMinorVersion = rpcMinorVersion ();
    result.request().hdr.packetType = RPC_CN_PKT_REQUEST;
    result.request().hdr.flags = RPC_CN_FLAGS_FIRST_FRAG |
				 RPC_CN_FLAGS_LAST_FRAG;
    if (pObjectUuid)
        {
        result.request().hdr.flags |= RPC_CN_FLAGS_OBJECT_UUID;
        result.request().objectId = *pObjectUuid;
        }

    result.request().hdr.drep [0] = VXDCOM_NDR_DATAREP0;
    result.request().hdr.drep [1] = VXDCOM_NDR_DATAREP1;
    result.request().hdr.drep [2] = VXDCOM_NDR_DATAREP2;
    result.request().hdr.drep [3] = VXDCOM_NDR_DATAREP3;
    result.request().hdr.fragLen = 0; // fix later
    result.request().hdr.authLen = 0;
    result.request().hdr.callId = callId;
    result.request().allocHint = 0; // fixed up later
    result.request().presCtxId = presCtxId;
    result.request().methodNum = opnum;
    }

//////////////////////////////////////////////////////////////////////////
//

void
RpcPduFactory::formatAuth3Pdu
    (
    RpcPdu&		pdu,
    ULONG		callId
    )
    {
    pdu.auth3().hdr.rpcVersion = rpcMajorVersion ();
    pdu.auth3().hdr.rpcMinorVersion = rpcMinorVersion ();
    pdu.auth3().hdr.packetType = RPC_CN_PKT_AUTH3;
    pdu.auth3().hdr.flags = RPC_CN_FLAGS_FIRST_FRAG |
			     RPC_CN_FLAGS_LAST_FRAG;
    pdu.auth3().hdr.drep [0] = VXDCOM_NDR_DATAREP0;
    pdu.auth3().hdr.drep [1] = VXDCOM_NDR_DATAREP1;
    pdu.auth3().hdr.drep [2] = VXDCOM_NDR_DATAREP2;
    pdu.auth3().hdr.drep [3] = VXDCOM_NDR_DATAREP3;
    pdu.auth3().hdr.fragLen = 0; // fix later
    pdu.auth3().hdr.authLen = 0;
    pdu.auth3().hdr.callId = callId;
    pdu.auth3().maxTxFrag = RpcPdu::MAXLEN;
    pdu.auth3().maxRxFrag = RpcPdu::MAXLEN;
    }


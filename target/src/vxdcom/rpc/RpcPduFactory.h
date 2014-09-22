/* RpcPduFactory - create DCOM protocol requests/responses */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01l,13jul01,dbs  fix up includes
01k,29mar01,nel  SPR#35873. Add context id parameter to formatBindPdu to allow
                 client side ctxId to be specified.
01j,26jun00,dbs  add did-not-execute to FAULT
01i,22jun00,dbs  fix handling of alter-context packets
01h,19jul99,dbs  add support for AUTH3 packets
01g,09jul99,dbs  use final filenames
01f,02jul99,aim  renamed makeBindFormatPdu and makeRequestFormatPdu
01e,24jun99,dbs  move authn into new class
01d,23jun99,dbs  fix authn on response packets
01c,08jun99,aim  rework
01b,08jun99,aim  now uses NRpcPdu
01a,03Jun99,aim  created
*/

#ifndef __INCRpcPduFactory_h
#define __INCRpcPduFactory_h

#include "RpcPdu.h"
#include "orpcLib.h"

class RpcPduFactory
    {
  public:
    virtual ~RpcPduFactory ();
    RpcPduFactory ();

    static void formatFaultPdu
	(
	const RpcPdu&	request,
	RpcPdu&		result,
	HRESULT		faultCode,
	bool		didNotExecute
	);

    static void formatAuth3Pdu
	(
	RpcPdu&		auth3Pdu,
	ULONG		callId
	);

    static void formatBindAckPdu
	(
	const RpcPdu&	request,
	RpcPdu&		result,
	ULONG           assocGroupId
	);

    static void formatAlterContextRespPdu
	(
	const RpcPdu&	request,
	RpcPdu&		result
	);

    static void formatBindNakPdu
	(
	const RpcPdu&	request,
	RpcPdu&		result
	);

    static void formatResponsePdu
	(
	const RpcPdu&	request,
	RpcPdu&		result
	);

    static void formatBindPdu
	(
	RpcPdu&		result,
	REFIID		riid,
	ULONG		callId,
	ULONG		assocGroupId,
	bool		alterCtxt,
	USHORT		ctxId
	);

    static void formatRequestPdu
	(
	RpcPdu&		result,
	ULONG		callId,
	USHORT		opnum,
	const GUID*	pObjectUuid,
	USHORT		ctxId
	);

    static BYTE rpcMajorVersion ();
    static BYTE rpcMinorVersion ();

  private:

    // unsupported
    RpcPduFactory (const RpcPduFactory& other);
    RpcPduFactory& operator= (const RpcPduFactory& rhs);
    };

#endif // __INCRpcPduFactory_h

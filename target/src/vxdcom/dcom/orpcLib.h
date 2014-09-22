/* orpcLib.h - ORPC PUBLIC interfaces (VxDCOM) */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01z,13jul01,dbs  fix up includes
01y,15feb00,dbs  add OBJREF etc
01x,09jul99,dbs  use new filenames
01w,24jun99,dbs  move authn into new class
01v,17jun99,dbs  remove RPC_MEM_ALLOC macro
01u,26may99,dbs  fix auth func ptr type
01t,24may99,dbs  remove VXDCOM_IPPORT constant
01s,19may99,dbs  change orpcListenTaskCreate to orpcEndpointCreate
01r,18may99,dbs  remove old marshaling scheme
01q,16may99,dbs  use new NDR type-descriptors for marshaling
01p,13may99,drm  added prototypes for ORPC_EXTENT_ARRAY marshalling functions
01o,11may99,dbs  rename VXCOM to VXDCOM
01n,10may99,dbs  simplify binding-handle usage
01m,07may99,dbs  move major functionality into this lib
01l,22apr99,dbs  add task priorities
01k,21apr99,dbs  fix includes for dcomProxy.h
01j,21apr99,dbs  change RPCMSG to RPCPDU
01i,21apr99,dbs  add length arg to orpcDSAFormat()
01h,15feb99,dbs  move ORPC dispatch into orpcLib
01g,10feb99,dbs  improve handling of endpoints
01f,08feb99,dbs  formalise naming convention and ORPC/RPC interface
01e,04feb99,dbs  change wchar_t to OLECHAR
01d,22jan99,dbs  move some stuff into new rpcLib
01c,20jan99,dbs  fix file names, move to new VOBs
01b,23dec98,dbs  improve documentation
01a,22dec98,dbs  created from orpcLibP.h

*/


/*

  DESCRIPTION:

  This header collects some miscellaneous ORPC-related stuff together
  and prototypes some functions which are required for handling
  plain-DCE RPCs as well as ORPCs.
  
  */


#ifndef __INCorpcLib_h
#define __INCorpcLib_h

#ifdef _MSC_VER
#pragma warning (disable:4786)
#endif

#include "private/comMisc.h"
#include "dcomLib.h"			// basic types
#include "dcomProxy.h"			// proxy support
#include "RemoteActivation.h"		// DUALSTRINGARRAY
#include "rpcDceProto.h"		// protocol

////////////////////////////////////////////////////////////////////////////
//
// Task Names -- this allows them to be changed easily...
//

#define SCM_TASK_NAME		"tScmTask"
#define EP_TASK_NAME		"tRpcTask"
#define WORK_TASK_NAME		"tRpcWorker"

////////////////////////////////////////////////////////////////////////////
//
// OBJREF -- ORPC wire protocol marshaled interface pointer
// definition. (see also orpc.idl)
//

const unsigned int OBJREF_MAX = 1024;

typedef struct tagOBJREF
    {
    unsigned long	signature;
    unsigned long	flags;
    GUID		iid;
    union
	{
	// case OBJREF_STANDARD
	struct
	    {
	    STDOBJREF		std;
	    DUALSTRINGARRAY	saResAddr;
	    } u_standard;

	// case OBJREF_HANDLER
	struct
	    {
	    STDOBJREF		std;
	    CLSID		clsid; // of handler code
	    DUALSTRINGARRAY	saResAddr;
	    } u_handler;

	// case OBJREF_CUSTOM
	struct
	    {
	    CLSID		clsid; // of unmarshaler
	    unsigned long	cbExtension;
	    unsigned long	size;
	    char*		pData;
	    } u_custom;
	} u_objref;
    } OBJREF;



////////////////////////////////////////////////////////////////////////////
//
// _ACTIVATION_MAX -- the limit on the number of interfaces that can
// be dealt with in a single call to RemoteActivation() and/or
// CoCreateInstanceEx(). This limit is imposed by ASSERTs in the code,
// and should be plenty for most cases we have come across. It could
// be replaced by dynamic allocation, but there would be a speed and
// fragmentation penalty...
//

const int _ACTIVATION_MAX = 16;

///////////////////////////////////////////////////////////////////////////
//
// Private runtime support functions...
//

extern IPID    orpcIpidCreate (void* itf, OID oid);
extern OID     orpcOidFromIpid (REFIPID ipid);
extern HRESULT orpcDSAFormat
    (
    DUALSTRINGARRAY*	pdsa,
    DWORD		dsaLen,
    LPCOLESTR		addrInfo,
    LPCOLESTR		secInfo
    );


/////////////////////////////////////////////////////////////////////////
//
// Marshaling functions for ORPC types (OBJREF, ORPCTHIS) which are
// not publically exposed through DCOM APIs.
//

extern HRESULT ndrMarshalORPCTHIS (NdrMarshalStream*, ORPCTHIS*);
extern HRESULT ndrUnmarshalORPCTHIS (NdrUnmarshalStream*, ORPCTHIS*);

extern HRESULT ndrMarshalOBJREF (NdrMarshalStream*, OBJREF*);
extern HRESULT ndrUnmarshalOBJREF (NdrUnmarshalStream*, OBJREF*);


/////////////////////////////////////////////////////////////////////////
//
// The GUID used for NDR transfer syntax, which is the only transfer
// syntax DCOM recognises...
//

extern const GUID DCOM_XFER_SYNTAX;


/////////////////////////////////////////////////////////////////////////
//
// Define constants for common protocol-sequences...
//

#define NCACN_IP_TCP (0x07)
#define NCALRPC (0x99)


/////////////////////////////////////////////////////////////////////////
//
// Wire-format NDR always little-endian -- NT cannot cope with
// big-endian packets, even though DCE-RPC says it should...
//

#define ORPC_WIRE_DREP                  VXDCOM_DREP_LITTLE_ENDIAN

#endif



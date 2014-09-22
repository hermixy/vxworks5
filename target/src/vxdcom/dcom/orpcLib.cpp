/* orpcLib.cpp -- DCOM / ORPC interface functions */

/* Copyright (c) 1998 Wind River Systems, Inc. */

/*

modification history
--------------------
02z,17dec01,nel  Add include symbol for diab.
02y,13jul01,dbs  fix up includes
02x,15feb00,dbs  fix ORPCTHIS (un)marshaling
02w,08feb00,dbs  update struct-descs to latest NDR impl
02v,14sep99,dbs  use newest NDR marshaling routines
02u,19aug99,aim  change assert to VXDCOM_ASSERT
02t,12aug99,dbs  comply with new NDR struct support
02s,30jul99,dbs  tighten up type-safety of NDR types
02r,30jul99,dbs  fix cstruct initialiser to bind to address
02q,16jul99,dbs  fix possible alignment problem in ipid/oid conversions
02p,12jul99,dbs  always use method of ORPC_EXTENT_ARRAY to allocate storage
                 when unmarshaling, manually unmarshal extents
02o,09jul99,dbs  removing obsolete files (Scm.h) and obsolete code
02n,07jul99,dbs  fix comments in line with previous change
02m,07jul99,dbs  fix (un)marshaling of ORPCTHIS extents
02l,24jun99,dbs  move authn into new class
02k,17jun99,aim  changed assert to assert
02j,17jun99,dbs  change to COM_MEM_ALLOC
02i,07jun99,dbs  need to include stdlib
02h,02jun99,dbs  use new OS-specific macros
02g,01jun99,dbs  fix error condition handling
02f,28may99,dbs  simplify allocator usage
02e,28may99,dbs  remove obsolete include
02d,27may99,dbs  check method number is in range
02c,27may99,dbs  change to vxdcomTarget.h
02b,25may99,dbs  make sure PDUs are deleted after use
02a,19may99,dbs  change orpcListenTaskCreate to orpcEndpointCreate
01z,19may99,dbs  add ORPC_EXTENT marshalers
01y,18may99,dbs  remove old marshaling scheme
01x,10may99,dbs  simplify rpc-binding usage
01w,07may99,dbs  move major functionality into this lib
01v,30apr99,dbs  change name of ComSCM to SCM
01u,29apr99,dbs  fix -Wall warnings
01t,28apr99,dbs  init RPCOLEMESSAGE with RPC-channel in rpcCallback
01s,26apr99,aim  added TRACE_CALL
01r,22apr99,dbs  tidy up potential leaks
01q,21apr99,dbs  change RPCMSG to RPCPDU
01p,21apr99,dbs  add length arg to orpcDSAFormat()
01o,13apr99,dbs  fix VxSocket usage
01n,09apr99,drm  adding diagnostic output
01m,12mar99,dbs  add IOXIDResolver support
01l,01mar99,dbs  help tidy up RPC startup
01k,15feb99,dbs  move ORPC dispatch into orpcLib
01j,10feb99,dbs  improve handling of endpoints
01i,08feb99,dbs  formalise naming convention and ORPC/RPC interface
01h,04feb99,dbs  change wchar_t to OLECHAR
01g,22jan99,dbs  move some stuff into new rpcLib
01f,22dec98,dbs  improve ORPC interface
01e,18dec98,dbs  fix after changes for VXCOM
01d,16dec98,dbs  add flag to control packet-printing, remove
                 obsolete NDR-related code.
01c,10dec98,dbs  format packet headers for NDR
01b,07dec98,dbs  handle return-codes correctly in many cases.
01a,11nov98,dbs  created

*/

/*
  DESCRIPTION:

  This library implements some parts of the ORPC functionality which
  do not reside in any of the implementation classes (yet - in future
  it will hopefully become obsolete) and deals mainly with the
  provision of endpoints.

  */

#include <stdio.h>
#include "orpcLib.h"
#include "SCM.h"
#include "private/comMisc.h"
#include "RpcPdu.h"
#include "NdrStreams.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_orpcLib (void)
    {
    return 0;
    }

NdrTypeDesc StructDesc__GUID(NDRTYPES&);

//////////////////////////////////////////////////////////////////////////
//
// Globals...
//

const GUID DCOM_XFER_SYNTAX = 
    {0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}};


////////////////////////////////////////////////////////////////////////////
//
// orpcIpidCreate -- create an IPID (Interface Pointer ID) given the
// interface pointer value, and the OID of the object it points to.
//

IPID orpcIpidCreate (void* itf, OID oid)
    {
    GUID ipid;

    ipid.Data1 = (DWORD) itf;
    ipid.Data2 = 0;
    ipid.Data3 = 0;
    memcpy (ipid.Data4, &oid, sizeof(OID));
    
    return ipid;
    }

////////////////////////////////////////////////////////////////////////////
//
// orpcOidFromIpid -- given an IPID (created by orpcIpidCreate())
// return the OID of the object represented by that IPID.
//

OID orpcOidFromIpid (REFIPID ipid)
    {
    OID oid;
    ::memcpy (&oid, ipid.Data4, sizeof (oid));
    return oid;
    }


//////////////////////////////////////////////////////////////////////////
//
// orpcDSAFormat -- format a DUALSTRINGARRAY given the address and
// security info in string format. The address info should have the
// PROTSEQ in the first character, followed by the NULL-terninated
// address. An extra NULL is appended, followed by the security info,
// followed by two NULLs. As there is no security support, we ignore
// the arg...
//
// Assumes the memory pointer-to by 'pdsa' is large enough to handle
// the information...
//

HRESULT orpcDSAFormat
    (
    DUALSTRINGARRAY*	pdsa,
    DWORD		dsaLen,
    LPCOLESTR		addrInfo,
    LPCOLESTR		secInfo
    )
    {
    TRACE_CALL;
    OLECHAR*		p = pdsa->aStringArray;
    size_t		addrLen=0;
    size_t		secLen=0;

    // Length check...
    if (addrInfo)
	addrLen = vxcom_wcslen (addrInfo);
    if (secInfo)
	secLen = vxcom_wcslen (secInfo);
    if ((addrLen + secLen + 4) > dsaLen)
	return E_FAIL;
    
    if (addrInfo)
	{
	// Write address-info wide-chars into DSA...
	vxcom_wcscpy (p, addrInfo);
	p += addrLen;
	}
    
    // Write 2 terminating NULLs...
    *p++ = 0;
    *p++ = 0;

    // Set security-offset...
    pdsa->wSecurityOffset = p - pdsa->aStringArray;

    if (secInfo)
	{
	// Write security-info wide-chars into DSA...
	vxcom_wcscpy (p, secInfo);
	p += secLen;
	}
    
    // Write 2 terminating NULLs...
    *p++ = 0;
    *p++ = 0;

    // Set size...
    pdsa->wNumEntries = p - pdsa->aStringArray;

    return S_OK;
    }


//////////////////////////////////////////////////////////////////////////
//
// ndrMarshalOBJREF -- marshal an OBJREF into a marshaling stream...
//

HRESULT ndrMarshalOBJREF (NdrMarshalStream* pms, OBJREF* pObjRef)
    {
    NDRTYPES ndrtypes;
    
    // OBJREF header first...
    widlMarshal (&pObjRef->signature, pms, ndrtypes.ulong_t ());
    widlMarshal (&pObjRef->flags, pms, ndrtypes.ulong_t ());
    widlMarshal (&pObjRef->iid, pms, StructDesc__GUID(ndrtypes));

    // Now union part, depending on flags-field...
    if (pObjRef->flags != OBJREF_STANDARD)
	return E_NOTIMPL;

    // Fields of STDOBJREF...
    STDOBJREF* pStd = &pObjRef->u_objref.u_standard.std;
    
    widlMarshal (&pStd->flags, pms, ndrtypes.ulong_t ());
    widlMarshal (&pStd->cPublicRefs, pms, ndrtypes.ulong_t ());
    widlMarshal (&pStd->oxid, pms, ndrtypes.longlong_t ());
    widlMarshal (&pStd->oid, pms, ndrtypes.longlong_t ());
    widlMarshal (&pStd->ipid, pms, StructDesc__GUID(ndrtypes));

    // DUALSTRINGARRAY - in a std OBJREF this is marshaled as
    // contiguous USHORTs, with no preceding length field...
    DUALSTRINGARRAY* pdsa = &pObjRef->u_objref.u_standard.saResAddr;

    widlMarshal (&pdsa->wNumEntries, pms, ndrtypes.ushort_t ());
    widlMarshal (&pdsa->wSecurityOffset, pms, ndrtypes.ushort_t ());
    return widlMarshal (pdsa->aStringArray,
			pms,
			ndrtypes.array_t (ndrtypes.ushort_t (),
					  NDR_SIZEOF(short),
					  pdsa->wNumEntries));
    }

//////////////////////////////////////////////////////////////////////////
//

HRESULT ndrUnmarshalOBJREF (NdrUnmarshalStream* pus, OBJREF* pObjRef)
    {
    NDRTYPES ndrtypes;
    
    // OBJREF header first...
    widlUnmarshal (&pObjRef->signature, pus, ndrtypes.ulong_t ());
    widlUnmarshal (&pObjRef->flags, pus, ndrtypes.ulong_t ());
    widlUnmarshal (&pObjRef->iid, pus, StructDesc__GUID(ndrtypes));

    // Now union part, depending on flags-field...
    if (pObjRef->flags != OBJREF_STANDARD)
	return E_NOTIMPL;

    // Fields of STDOBJREF...
    STDOBJREF* pStd = &pObjRef->u_objref.u_standard.std;
    
    widlUnmarshal (&pStd->flags, pus, ndrtypes.ulong_t ());
    widlUnmarshal (&pStd->cPublicRefs, pus, ndrtypes.ulong_t ());
    widlUnmarshal (&pStd->oxid, pus, ndrtypes.longlong_t ());
    widlUnmarshal (&pStd->oid, pus, ndrtypes.longlong_t ());
    widlUnmarshal (&pStd->ipid, pus, StructDesc__GUID(ndrtypes));

    // DUALSTRINGARRAY - in a std OBJREF this is marshaled as
    // contiguous USHORTs, with no preceding length field...
    DUALSTRINGARRAY* pdsa = &pObjRef->u_objref.u_standard.saResAddr;

    widlUnmarshal (&pdsa->wNumEntries, pus, ndrtypes.ushort_t ());
    widlUnmarshal (&pdsa->wSecurityOffset, pus, ndrtypes.ushort_t ());
    return widlUnmarshal (pdsa->aStringArray,
			  pus,
			  ndrtypes.array_t (ndrtypes.ushort_t (),
					    NDR_SIZEOF(short),
					    pdsa->wNumEntries));
    }

//////////////////////////////////////////////////////////////////////////
//
// Declare type-desc function for externally defined structures.
//

NdrTypeDesc StructDesc_tagORPCTHIS (NDRTYPES& ndrtypes);
    
//////////////////////////////////////////////////////////////////////////
//
// ndrMarshalORPCTHIS -- marshal an ORPCTHIS
//
// This routine marshals an ORPCTHIS, which may also contain an
// ORPC_EXTENT_ARRAY.  In the NDR specification, a structure with a
// conformant array has the maximum count marshalled first, followed
// by the members of the structure, and then finally the elements of
// the array.  In the NDR specification, arrays of pointers are
// marshalled with the representations deferred.  That is, an array of
// pointers is marshalled as an array of arbitrary referent IDs
// followed by the deferred representations.
//
// Thus, with the DCOM spec as reference, the ORPC_EXTENT_ARRAY is
// marshalled as follows:-
//
//  Type          Contents
//  ----------    -------------------------------------
//  ulong        'size' structure member		// ORPC_EXTENT_ARRAY
//  ulong        'reserved' structure member (must be 0)// ORPC_EXTENT_ARRAY
//  ulong        'extent' structure member		// ORPC_EXTENT_ARRAY
//  ulong        array-size (rounded up to multiple of 2)
//  long         referent ID 1				// ORPC_EXTENT_ARRAY
//                   ...				// ORPC_EXTENT_ARRAY
//                   ...				// ORPC_EXTENT_ARRAY
//  long         referent ID n ((size+1) & ~1)		// ORPC_EXTENT_ARRAY
//
// To marshal the ORPC_EXTENT values, according to the NDR
// specification, the marshalled format is as follows:
// 
//  Type          Contents
//  ----------    -------------------------------------
//  long         size_is of 'data' array 		// ORPC_EXTENT
//  GUID         id					// ORPC_EXTENT
//  ulong        'size' structure member		// ORPC_EXTENT
//  byte	 'data' byte 1				// ORPC_EXTENT
//                  ...					// ORPC_EXTENT
//                  ...					// ORPC_EXTENT
//  byte         'data' byte n 				// ORPC_EXTENT
//
// 
//
// RETURNS: S_OK if successful and an HRESULT indicating error otherwise
// 
// HRESULT:
// .iP S_OK
// The operation was successful.
//
// SEE ALSO: ndrUnmarshalORPCTHIS()
// 
// nomanual
//


HRESULT ndrMarshalORPCTHIS
    (
    NdrMarshalStream*	pStrm,
    ORPCTHIS*		pOrpcThis
    )
    {
    TRACE_CALL;

    NDRTYPES ndrtypes;
    
    return widlMarshal (pOrpcThis, pStrm, StructDesc_tagORPCTHIS(ndrtypes));
    }

//////////////////////////////////////////////////////////////////////////
//
// ndrUnmarshalORPCTHIS -- unmarshal an ORPCTHIS structure from an
// NDR-stream. This function does not use the NdrTypes 'cstruct' class
// to do its unmarshaling, instead it does it 'manually' because the
// NDR-engine is designed for use with plain structures, whereas the
// ORCPTHIS, ORPC_EXTENT and ORPC_EXTENT_ARRAY are actually C++
// classes, and so cannot be manipulated with raw-memory operations
// (due to destructors being called, etc).
//
// Also, the ORPCTHIS dtor will try to free all memory it thinks it
// owns, but the NDR-engine will economise by simply pointing into the
// unmarshaling buffer in the stub-unmarshal phase, which effectively
// would leave a non-heap pointer inside the ORPCTHIS structures, and
// casue problems when the dtor fired.
//

HRESULT ndrUnmarshalORPCTHIS
    (
    NdrUnmarshalStream*	pStrm,
    ORPCTHIS*		pOrpcThis
    )
    {
    TRACE_CALL;

    NDRTYPES ndrtypes;

    return widlUnmarshal (pOrpcThis, pStrm, StructDesc_tagORPCTHIS(ndrtypes));
    }



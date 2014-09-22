/* dcomLib.h - VxWorks DCOM public API */

/* Copyright (c) 1998 Wind River Systems, Inc. */

/*

modification history
--------------------
01r,11oct01,nel  Add SCM Stack Size param to dcomLibInit.
01q,16jul01,dbs  fix up for plain C compilation, add definition of dcomLibInit
01p,12mar01,nel  SPR#62130. Add CoDisconectObject API def.
01o,20sep99,dbs  use vxidl.idl types
01n,09sep99,drm  Removing GUID_VXDCOM_EXTENT and VXDCOMEXTENT definitions from
                 here as they've been moved to the new file dcomExtent.h
01m,21jul99,drm  Adding GUID and structure for the VxDCOM extent.
01l,24jun99,dbs  add authn APIs
01k,21apr99,dbs  add fwd decls of RPC interfaces
01j,21apr99,dbs  move IRpcChannelBuffer into its own privte header
01i,29jan99,dbs  simplify proxy/stub code
01h,20jan99,dbs  fix file names - vxcom becomes com
01g,22dec98,dbs  tidy up ORPC interface
01f,18dec98,dbs  move some stuff to vxcomLib.h
01e,14dec98,dbs  enhance RPCOLEMESSAGE struct, speed up GUID ops
01d,11dec98,dbs  remove need for COM_NO_WINDOWS_H flag
01c,11dec98,dbs  simplify registry
01b,26nov98,dbs  add IRemUnknown functionality
01a,17nov98,dbs  created

*/

/*

DESCRIPTION:

This file defines a working subset of the COM API (as defined by
Microsoft) for support of DCOM in VxWorks.

*/

#ifndef __INCdcomLib_h
#define __INCdcomLib_h

#include "comLib.h"

EXTERN_C const CLSID CLSID_StdMarshal;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DCOM library initialisation/termination functions.
 */

STATUS dcomLibInit
    (
    int,			/* BSTR policy */
    int,			/* DCOM Authentication level */
    unsigned int,		/* thread priority */
    unsigned int,		/* Static threads */
    unsigned int,		/* Dynamic threads */
    unsigned int,		/* Stack Size of server thread */
    unsigned int,		/* Stack Size of SCM thread */
    int,			/* Client Priority propogation */
    int				/* Object Exporter Port Number */
    );

void dcomLibTerm (void);
    
/*
 * Public API Functions - these mimic the Win32 CoXxxx API calls,
 * specifically those related to marshaling and interface remoting.
 */

HRESULT CoGetClassObject
    (
    REFCLSID		rclsid,		/* CLSID of class object */
    DWORD		dwClsContext,	/* one of CLSCTX values */
    COSERVERINFO*	pServerInfo,    /* must be NULL */
    REFIID		riid,		/* IID of desired interface */
    void**		ppv		/* output interface pointer */
    );

HRESULT CoCreateInstanceEx
    (
    REFCLSID            rclsid,         /* CLSID of the object */
    IUnknown*           punkOuter,      /* ptr to aggregating object */
    DWORD               dwClsCtx,       /* one of CLSCTX values */
    COSERVERINFO*       pServerInfo,    /* machine to create object on */
    ULONG               cmq,            /* number of MULTI_QI structures */
    MULTI_QI*           pResults        /* array of MULTI_QI structures */
    );

HRESULT CoRegisterClassObject
    (
    REFCLSID            rclsid,         /* CLSID to be registered */
    IUnknown*           pUnk,           /* pointer to the class object */
    DWORD               dwClsContext,   /* context from CLSCTX_XXX enum */
    DWORD               flags,          /* how to connect to the class object */
    DWORD*              lpdwRegister    /* returned token */
    );

HRESULT CoRevokeClassObject
    (
    DWORD               dwRegister      /* token for class object */
    );

HRESULT CoGetStandardMarshal
    (
    REFIID		riid,		/* interface IID */
    IUnknown*		pUnk,		/* interface-ptr to be marshaled */
    DWORD		dwDestContext,  /* destination context */
    void*		pvDestContext,  /* reserved for future use */
    DWORD		mshlflags,      /* reason for marshaling */
    IMarshal**		ppMshl		/* output ptr */
    );

HRESULT CoMarshalInterface
    (
    IStream*		pStm,		/* stream to marshal into */
    REFIID		riid,		/* interface IID */
    IUnknown*		pUnk,		/* interface-ptr to be marshaled */
    DWORD		dwDestContext,  /* destination context */
    void*		pvDestContext,  /* reserved for future use */
    DWORD		mshlflags       /* reason for marshaling */
    );

HRESULT CoUnmarshalInterface
    (
    IStream*		pStm,		/* stream containing interface */
    REFIID		riid,		/* IID of the interface */
    void**		ppv		/* output variable to receive ptr */
    );

HRESULT CoGetMarshalSizeMax
    (
    ULONG*              pulSize,        /* ptr to the upper-bound value */
    REFIID              riid,           /* IID of interface */
    IUnknown*           pUnk,           /* interface-ptr to be marshaled */
    DWORD               dwDestContext,  /* destination process */
    LPVOID              pvDestContext,  /* reserved for future use */
    DWORD               mshlflags       /* reason for marshaling */
    );
 
HRESULT CoReleaseMarshalData
    (
    IStream*            pStrm           /* stream to release */
    );

HRESULT CoDisconnectObject
    (
    IUnknown*           pUnk,           /* object to be disconnected */
    DWORD               dwReserved      /* reserved for future use - MBZ */
    );

HRESULT CoGetPSClsid
    (
    REFIID              riid,           /* IID to use */
    LPCLSID             pClsid          /* resulting P/S CLSID */
    );

#define RPC_C_AUTHN_LEVEL_DEFAULT 0
#define RPC_C_AUTHN_LEVEL_NONE 1
#define RPC_C_AUTHN_LEVEL_CONNECT 2
#define RPC_C_AUTHN_LEVEL_CALL 3
#define RPC_C_AUTHN_LEVEL_PKT 4
#define RPC_C_AUTHN_LEVEL_PKT_INTEGRITY 5
#define RPC_C_AUTHN_LEVEL_PKT_PRIVACY 6

#define RPC_C_AUTHN_NONE 0
#define RPC_C_AUTHN_DCE_PRIVATE 1
#define RPC_C_AUTHN_DCE_PUBLIC 2
#define RPC_C_AUTHN_DEC_PUBLIC 4
#define RPC_C_AUTHN_WINNT       10
#define RPC_C_AUTHN_DEFAULT 0xFFFFFFFF

#define RPC_C_IMP_LEVEL_DEFAULT      0
#define RPC_C_IMP_LEVEL_ANONYMOUS    1
#define RPC_C_IMP_LEVEL_IDENTIFY     2
#define RPC_C_IMP_LEVEL_IMPERSONATE  3
#define RPC_C_IMP_LEVEL_DELEGATE     4

#define RPC_C_AUTHZ_NONE 0
#define RPC_C_AUTHZ_NAME 1
#define RPC_C_AUTHZ_DCE 2

#define EOAC_NONE (0)

/**************************************************************************
*
* vxdcomUserAdd -- add a user+password to VxDCOM
*
* This function adds a username, plus their password, to the VxDCOM
* security service. This is used when an authentication level other 
* than the default is required, the only valid level being
* RPC_C_AUTHN_LEVEL_CONNECT.
*
* RETURNS: n/a
*/

void vxdcomUserAdd
    (
    const char* userName,
    const char* userPassword
    );

/**************************************************************************
*
* CoInitializeSecurity -- initialize the security service
*
* This function initializes the VxDCOM security service (only
* NTLMSSP is supported by VxDCOM). The parameter 'dwAuthnLevel' sets
* both the default incoming authentication level and the default
* outgoing authentication level, and must be one of the
* RPC_C_AUTHN_LEVEL_xxx constants. Incoming calls with less than this
* level will be failed, outgoing calls will be made (by default) with
* this level. The parameter 'dwImpLevel' sets the default
* impersonation level for outgoing calls, and must be one of the
* RPC_C_IMP_LEVEL_xxx constants.
*
* Calling CoCreateInstanceEx() with a non-NULL 'pServerInfo' parameter
* will utilise the fields in the COSERVERINFO structure to determine
* where the server is to be created. If the structure has a non-NULL
* 'pAuthInfo' member, then the security settings in that structure
* will override the default settings for the duration of that
* activation request, and will be applied to the created proxy (if the
* activation is successful). Valid settings for the COAUTHINFO
* structure are described in its definition earlier in this file.
*
* RETURNS: an HRESULT value
*/

HRESULT CoInitializeSecurity
    (
    void*		psd,		/* security descriptor - MBZ */
    long		cAuths,		/* must be -1 */
    void*		asAuths,	/* array of services - MBZ */
    void*		pReserved1,	/* reserved - MBZ */
    DWORD		dwAuthnLevel,	/* default authentication level  */
    DWORD		dwImpLevel,	/* default impersonation level */
    void*		pAuthList,	/* per-service info - MBZ */
    DWORD		dwcapabilities,	/* capabilities - must be EOAC_NONE */
    void*		pReserved3	/* reserved - MBZ */
    );

#ifdef __cplusplus
}
#endif

#endif /* __INCdcomLib_h */



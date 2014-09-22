/* dcomLib.cpp - DCOM library (VxDCOM) */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
03z,03jan02,nel  Remove OLE2T.
03y,17dec01,nel  Add include sybmol for diab build.
03x,02nov01,nel  Correct docs errors.
03w,10oct01,nel  SPR#70838. Ensure that all threads are started with
                 VX_FP_TASK to get round any FP/longlong issues with certain
                 BSPs and also usage of FP in DCOM servers.
03v,06aug01,dbs  remove instance-creation from SCM
03u,02aug01,dbs  use simplified Thread API
03t,31jul01,dbs  change vxdcomPSRegister to autoreg ctor
03s,19jul01,dbs  fix include path of vxdcomGlobals.h
03r,16jul01,dbs  add library termination function
03q,13jul01,dbs  fix up includes
03p,02may01,nel  SPR#66227. Correct format of comments for refgen.
03o,02mar01,nel  SPR#62130. Add CoDisconectObject API.
03n,28feb00,dgp  update docs formatiing
03m,11feb00,dbs  IIDs moved to idl directory, no longer need to force
                 linkage of system stubs
03l,13oct99,dbs  add reference material/man pages
03k,26aug99,aim  protect pMshl->UnmarshalInterface Release
03j,24aug99,dbs  fix extraction of results from MQI in CoGetClassObject
03i,19aug99,aim  removed TASK_SPAWN
03h,17aug99,dbs  add skeleton CoInitSecurity()
03g,17aug99,aim  added g_vxdcomExportAddress globals
03f,13aug99,aim  reworked dcomLibInit
03e,13aug99,aim  added g_vxdcomMinThreads & g_vxdcomMaxThreads
03d,02aug99,dbs  replace p/s init funcs with extern symbols
03c,29jul99,drm  Adding bool to dcomLibInit for client priority.
03b,28jul99,drm  Moving g_defaultServerPriority to comLib.cpp.
03a,27jul99,drm  Adding global for default server priority.
02z,26jul99,dbs  move marshaling into exporter
02y,23jul99,drm  Adding stack size param to dcomLibInit().
02x,21jul99,drm  Changing dcomLibInit() to accept additional arguments.
02w,12jul99,aim  change SCM task entry point
02v,09jul99,dbs  change to new ObjectExporter naming
02u,06jul99,dbs  make CoCIEx() call SCM method
02t,30jun99,dbs  add a ref to StdProxy when created in CoUnmarshalInterface()
02s,28jun99,dbs  tighten up default authn setting
02r,28jun99,dbs  add vxdcomUserAdd() function
02q,28jun99,dbs  add default authn level as arg to dcomLibInit()
02p,25jun99,aim  remove SCM::startService on non target build
02o,24jun99,aim  fix solaris build warnings
02n,24jun99,dbs  add authn APIs
02m,10jun99,aim  uses new SCM
02l,08jun99,dbs  simplify creation of StdProxy
02k,04jun99,dbs  fix registry-access functions
02j,03jun99,dbs  remove reliance on TLS from CoMarshalInterface
02i,02jun99,aim  changes for solaris build
02h,02jun99,dbs  use new OS-specific macros
02g,01jun99,dbs  add NOPING flag
02f,28may99,dbs  make stub disp-tbl a structure
02e,27may99,dbs  change to vxdcomTarget.h
02d,25may99,dbs  correctly free allocated DSA
02c,24may99,dbs  ask SCM for local object exporter
02b,21may99,dbs  remove unnecessary registry entry
02a,20may99,dbs  fix handling of returned array-types
01z,11may99,dbs  remove unnecessary registry-entries for p/s
01y,11may99,dbs  rename VXCOM to VXDCOM
01x,10may99,dbs  don't delete binding after call to IRemoteActivation
01w,10may99,dbs  simplify binding-handle usage
01v,03may99,drm  preliminary priority scheme support
01u,29apr99,dbs  fix -Wall warnings
01t,28apr99,dbs  remove extraneous AddRef() in CoMarshalInterface
01s,28apr99,dbs  don't release RPC binding-handle after remote activation
01r,27apr99,dbs  add mem-alloc functions
01q,27apr99,dbs  use right new for std-mashaler class
01p,26apr99,aim  added TRACE_CALL
01o,23apr99,dbs  use streams properly
01n,23apr99,dbs  remove warnings for unused locals
01m,22apr99,dbs  tidy up potential leaks
01l,15apr99,dbs  increase _ACTIVATION_MAX limit
01k,13apr99,dbs  remove unused includes
01j,09apr99,drm  adding diagnostic output
01i,11mar99,dbs  add IOXIDResolver support
01h,01mar99,dbs  help tidy up RPC startup
01g,11feb99,dbs  check for correct COM initialization
01f,10feb99,dbs  improve handling of endpoints
01e,08feb99,dbs  formalize naming convention and ORPC/RPC interface
01d,03feb99,dbs  simplify p/s architecture
01c,11jan99,dbs  reduce STL usage
01b,22dec98,dbs  improve ORPC interface
01a,18dec98,dbs  move plain COM functions into vxcomLib.cpp
01c,11dec98,dbs  simplify registry
01b,26nov98,dbs  add IRemUnknown functionality
01a,17nov98,dbs  created
*/

#include "private/comMisc.h"            // for misc support funcs
#include "dcomProxy.h"			// for ndr_xxx() funcs
#include "StdProxy.h"			// for StdProxy class 
#include "MemoryStream.h"		// for VxRWMemStream class
#include "ObjectExporter.h"		// for ObjectExporter class
#include "SCM.h"			// for SCM class
#include "orpcLib.h"			// for ORPC constants
#include "RemoteOxid.h"			// for RemoteOxid class
#include "StdMarshaler.h"		// for StdMarshaler class
#include "PSFactory.h"			// for PSFactory class
#include "private/vxdcomGlobals.h"      // global variables
#include "taskLib.h"                    // VxWorks tasks
#include "RemoteRegistry.h"             // DCOM registry


/*

DESCRIPTION

This library provides a subset of the Win32 COM/DCOM API calls, in
order to support the implementation of COM and DCOM in the VxWorks
environment. 
    
*/

/* Include symbol for diab */
extern "C" int include_vxdcom_dcomLib (void)
    {
    return 0;
    }
    

/* Well-known GUIDs that are required by DCOM... */


const IID CLSID_StdMarshal =
    {0x00000017,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

const IID IID_IRpcChannelBuffer =
    {0xD5F56B60,0x593B,0x101A,{0xB5,0x69,0x08,0x00,0x2B,0x2D,0xBF,0x7A}};

const IID IID_IRpcProxyBuffer =
    {0xD5F56A34,0x593B,0x101A,{0xB5,0x69,0x08,0x00,0x2B,0x2D,0xBF,0x7A}};


extern "C"int scmTask ()
    {
    return SCM::startService ();
    }


/**************************************************************************
*
* dcomLibInit - dcomLib initialization function
*
* This function is called from the initialization stage of the
* VxWorks boot procedure, and establishes the VxDCOM environment
* according to the given arguments. These arguments are normally set
* via the Tornado Project Facility, and are detailed in the VxDCOM
* Component Supplement.
*
* RETURNS: the task-id of the SCM task.
*/

extern "C" int dcomLibInit
    (
    int			bstrPolicy,	/* BSTR policy */
    int			authnLevel,	/* DCOM Authentication level */
    unsigned int	priority,	/* thread priority */
    unsigned int	numStatic,	/* Static threads */
    unsigned int	numDynamic,	/* Dynamic threads */
    unsigned int	stackSize,	/* Stack Size of server thread */
    unsigned int	scmStackSize,	/* Stack Size of SCM thread */
    int			clientPrioPropagation,	/* Client Priority propagation */
    int			objectExporterPortNumber	/* Object Exporter Port Number */
    )
    {
    vxdcomBSTRPolicy = bstrPolicy;

    // Initialize the Remote Registry...
    dcomRemoteRegistryInit ();
    
    // Set the default authn level for NTLMSSP...
    if ((authnLevel < RPC_C_AUTHN_LEVEL_NONE) ||
	(authnLevel > RPC_C_AUTHN_LEVEL_CONNECT))
	authnLevel = RPC_C_AUTHN_LEVEL_NONE;	

    g_defaultAuthnLevel = authnLevel;

    // Set priority scheme global variables 
    g_defaultServerPriority = (int) priority;
    g_clientPriorityPropagation = clientPrioPropagation;
    
    g_vxdcomThreadPoolPriority = priority;
    g_vxdcomMinThreads = numStatic;
    g_vxdcomMaxThreads = numStatic + numDynamic;
    g_vxdcomDefaultStackSize = stackSize;
	
    g_scmTaskStackSize = scmStackSize;

    g_vxdcomObjectExporterPortNumber = objectExporterPortNumber;

    return ::taskSpawn ("tScmTask",
                        g_scmTaskPriority,
                        VX_FP_TASK,
                        g_scmTaskStackSize,
                        reinterpret_cast<FUNCPTR> (scmTask),
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }


/**************************************************************************
*
* dcomLibTerm - dcomLib termination function
*
* This function can be called to shut down the SCM and so terminate
* the DCOM functionality.
*/

void dcomLibTerm ()
    {
    SCM::stopService ();
    }


/**************************************************************************
*
* CoGetClassObject - return an instance of the class-object for a coclass
*
* This function differs from the Win32 equivalent in that it is only
* available when DCOM support is enabled, and not when only COM is
* enabled. It accepts only the CLSCTX_INPROC_SERVER or
* CLSCTX_REMOTE_SERVER values for 'dwClsContext' as the {local server}
* model is not implemented in VxDCOM.
*
* RETURNS: an HRESULT value
*/

HRESULT CoGetClassObject
    (
    REFCLSID		rclsid,		/* CLSID of class object */
    DWORD		dwClsContext,	/* context */
    COSERVERINFO*	pServerInfo,    /* machine info */
    REFIID		riid,		/* IID of desired interface */
    void**		ppv		/* var to receive i/f ptr */
    )
    {
    char * hint = 0;
    if (pServerInfo->pwszName)
	{
	hint = new char [comWideStrLen (pServerInfo->pwszName) + 1];
	comWideToAscii (hint,
			pServerInfo->pwszName, 
			comWideStrLen (pServerInfo->pwszName) + 1);
	}

    HRESULT hr =  comClassObjectGet (rclsid,
				     dwClsContext,
                                     hint,
                                     riid,
                                     ppv);
    if (hint)
	delete []hint;
    return hr;
    }


/**************************************************************************
*
* CoRegisterClassObject - Register a class-object with the global class factory.
*
* Register a class-object with the global class-factory table. This is 
* called from the server object when it is registered.
*
* RETURNS: S_OK on success, or CO_E_OBJISREG if the class is already in 
* the class object table.
*/

HRESULT CoRegisterClassObject
    (
    REFCLSID		rclsid,		/* CLSID to be registered */
    IUnknown*		pUnkCF,		/* pointer to the class object */
    DWORD		dwClsContext,	/* context (inproc/local/remote) */
    DWORD		flags,		/* how to connect to the class object */
    DWORD*		lpdwRegister	/* returned token */
    )
    {
    TRACE_CALL;

    // Validate args
    if (! pUnkCF)
	return E_INVALIDARG;

    //
    // 1. Marshal IUnknown of Class-factory, creating a new GOT entry.
    //
    IStream* pStream;
    HRESULT hr = VxRWMemStream::CreateInstance (0, IID_IStream, (void**) &pStream);
    if (FAILED (hr))
	return hr;
    
    hr = CoMarshalInterface (pStream,
			     IID_IUnknown,
			     pUnkCF,
			     MSHCTX_LOCAL,
			     0,
			     MSHLFLAGS_NORMAL);

    if (FAILED (hr))
	{
	pStream->Release ();
	return hr;
	}

    //
    // 2. Save CF in object-table. This is where it will be accessed
    // later by the client. The OID is the one we just got from
    // marshaling the interface-ptr...
    //
    ObjectExporter*	px = SCM::objectExporter ();
    ObjectTableEntry*	pOTE = px->objectFindByStream (pStream);
    DWORD		dwRegister = (DWORD) pOTE;

    if (pOTE)
	{
	pOTE->punkCF = pUnkCF;
	pOTE->clsid = rclsid;
	pOTE->dwRegToken = dwRegister;
	}
    *lpdwRegister = dwRegister;
    
    px->Release ();
    return S_OK;    
    }

/**************************************************************************
*
* CoRevokeClassObject - Unregister the class object.
*
* Un-register the class-factory from the class-factory table.
*
* RETURNS: S_OK on success, or S_FALSE on error.
*/

HRESULT CoRevokeClassObject
    (
    DWORD		dwRegister	/* token for class object */
    )
    {
    TRACE_CALL;
    
    ObjectExporter*	px = SCM::objectExporter ();
    ObjectTableEntry*	pOTE = px->objectFindByToken (dwRegister);
    HRESULT		hr = S_OK;
    
    if (pOTE)
	px->objectUnregister (pOTE->oid);
    else
	hr = S_FALSE;
    
    px->Release ();
    return hr;
    }

/**************************************************************************
*
* CoCreateInstanceEx - create a single instance of an object
*
* This function differs from the Win32 equivalent in that it is only
* available when DCOM support is enabled, and not when only COM is
* enabled.
*
* RETURNS: an HRESULT value
*/

HRESULT CoCreateInstanceEx
    (
    REFCLSID            rclsid,         /* CLSID of the object           */
    IUnknown*           pUnkOuter,      /* ptr to aggregating object     */
    DWORD               dwClsCtx,       /* one of CLSCTX values          */
    COSERVERINFO*       pServerInfo,    /* machine to create object on   */
    ULONG               nInterfaces,	/* number of MULTI_QI structures */ 
    MULTI_QI*           pResults        /* array of MULTI_QI structures  */
    )
    {
    char * hint = 0;
    if (pServerInfo->pwszName)
	{
	hint = new char [comWideStrLen (pServerInfo->pwszName) + 1];
	comWideToAscii (hint,
		        pServerInfo->pwszName,
			comWideStrLen (pServerInfo->pwszName) + 1);
	}

    HRESULT hr =  comInstanceCreate (rclsid,
                                     pUnkOuter,
                                     dwClsCtx,
                                     hint,
                                     nInterfaces,
                                     pResults);
    if (hint)
	delete []hint;

    return hr;
    }


/**************************************************************************
*
* CoGetStandardMarshal - Returns an instance of the standard marshaler.
*
* Returns an instance of the system standard marshaler. This object 
* encapsulates the mechanism for marshaling any interface pointer into a 
* stream, and is used (in this context) simply to marshal the given 
* interface, after that it will be destroyed.
*
* RETURNS: S_OK.
*/

HRESULT CoGetStandardMarshal
    (
    REFIID		riid,		/* interface IID */
    IUnknown*		pUnk,		/* interface-ptr to be marshaled */
    DWORD		dwDestContext,  /* destination context */
    void*		pvDestContext,  /* reserved for future use */
    DWORD		mshlflags,      /* reason for marshaling */
    IMarshal**		ppMshl		/* output ptr */
    )
    {
    TRACE_CALL;
    
    *ppMshl = new VxStdMarshaler ();

    (*ppMshl)->AddRef ();
    return S_OK;
    }

/**************************************************************************
*
* CoMarshalInterface - marshal an interface pointer into a stream
*
* This function differs from its Win32 counterpart in that only
* standard marshaling is supported. The interface to be marshaled is
* 'not' queried for 'IMarshal' and no attempt will be made to use
* custom marshaling even if the object supports it.
*
* RETURNS: an HRESULT value
*/

HRESULT CoMarshalInterface
    (
    IStream*		pStm,		/* stream to marshal into */
    REFIID		riid,		/* interface IID */
    IUnknown*		pUnk,		/* interface-ptr to be marshaled */
    DWORD		dwDestContext,  /* destination context */
    void*		pvDestContext,  /* reserved for future use */
    DWORD		mshlflags       /* reason for marshaling */
    )
    {
    TRACE_CALL;

    // We are now going to make an entry in the object exporter's
    // object table for the new object's marshaling packet. If at some
    // point this function fails, we need to remove this entry...

    ObjectExporter* pExp = SCM::objectExporter ();
    return pExp->objectMarshal (CLSID_NULL,
				pStm,
				riid,
				pUnk,
				dwDestContext,
				pvDestContext,
				mshlflags,
				0);
    }

/**************************************************************************
*
* CoUnmarshalInterface - Un-marshal an interface pointer from a stream
*
* This function differs from its Win32 counterpart in that only
* standard marshaling is supported. If the OBJREF being Un-marshaled
* indicates <custom marshaling> or <handler marshaling> then this call
* will fail.
*
* RETURNS: an HRESULT value
*/

HRESULT CoUnmarshalInterface
    (
    IStream*		pStm,		/* pointer to stream */
    REFIID		riid,		/* IID of the interface */
    void**		ppv		/* output variable to receive ptr */
    )
    {
    TRACE_CALL;
    
    // Record current location in stream before use...
    ULARGE_INTEGER currPos;
    HRESULT hr = pStm->Seek (0, STREAM_SEEK_CUR, &currPos);
    if (FAILED (hr))
	return hr;

    // Read signature and flags from stream, to see if its a STDOBJREF.
    // The STDOBJREF is known to be encoded in little-endian format as
    // the rules of DCOM say so!
    ULONG signature;
    ULONG flags;

    hr = pStm->Read (&signature, sizeof (signature), 0);
    ndr_make_right (signature, VXDCOM_DREP_LITTLE_ENDIAN);
    if (FAILED (hr))
	return hr;

    hr = pStm->Read (&flags, sizeof (flags), 0);
    ndr_make_right (flags, VXDCOM_DREP_LITTLE_ENDIAN);
    if (FAILED (hr))
	return hr;

    // Validate OBJREF format...
    if (signature != OBJREF_SIGNATURE)
        return RPC_E_INVALID_OBJREF;
    if (flags != OBJREF_STANDARD)
        return E_NOTIMPL;

    // Now rewind the stream, ready for proper Un-marshaling...
    pStm->Seek (currPos, STREAM_SEEK_SET, 0);

    // Create std-proxy-object to perform Un-marshaling...
    IMarshal* pMshl = new VxStdProxy ();
    if (! pMshl)
	return E_OUTOFMEMORY;
    pMshl->AddRef ();

    // Call UnmarshalInterface() to create connection
    hr = pMshl->UnmarshalInterface (pStm, riid, ppv);

    if (FAILED (hr))
	return hr;

    pMshl->Release ();

    return hr;
    }

/**************************************************************************
*
* CoGetPSClsid - returns the proxy/stub CLSID for a given interface ID
*
* This function is meaningless in VxDCOM as CLSIDs are not used to
* identify proxy/stub code as they are on Win32. Thus, this function
* always return S_OK and outputs CLSID_NULL.
*
* RETURNS: S_OK
*/

HRESULT CoGetPSClsid
    (
    REFIID              riid,           /* interface ID */
    LPCLSID             pClsid          /* resulting P/S CLSID */
    )
    {
    TRACE_CALL;

    *pClsid = CLSID_NULL;
    return S_OK;
    }


/**************************************************************************
*
* CoGetMarshalSizeMax - Returns the upper bound on the number of bytes needed to marshal the interface.
*
* Discover max size of marshaling packet required for given interface. As 
* we don't support custom or handler marshaling, its simply the size of the 
* std-mshl packet.
*
* RETURNS: S_OK on success or CO_E_NOTINITIALIZED.
*/

HRESULT CoGetMarshalSizeMax
    (
    ULONG*              pulSize,        /* ptr to the upper-bound value */
    REFIID              riid,           /* IID of interface */
    IUnknown*           pUnk,           /* interface-ptr to be marshaled */
    DWORD               dwDestContext,  /* destination process */
    LPVOID              pvDestContext,  /* reserved for future use */
    DWORD               mshlflags       /* reason for marshaling */
    )
    {
    TRACE_CALL;

    // Get std-marshaler, use its value...
    IMarshal* pMshl;
    HRESULT hr = CoGetStandardMarshal (riid,
				       pUnk,
				       dwDestContext,
				       pvDestContext,
				       mshlflags,
				       &pMshl);
    if (SUCCEEDED (hr))
	{
	// Now find out marshal-size requirements of interface...
	hr = pMshl->GetMarshalSizeMax (riid,
				       pUnk,
				       dwDestContext,
				       pvDestContext, 
				       mshlflags,
				       pulSize);
	}

    return hr;
    }

/*
* vxdcom_ps_autoreg -- used by proxy/stub modules to register their
* p/s classes with the system. Every server module that wants to be
* remoted must register a p/s entry via this function. It provides 2
* VTABLEs, one for proxies and one for stubs, and they are then
* managed by the system's own p/s factory object.
*/

vxdcom_ps_autoreg::vxdcom_ps_autoreg
    (
    REFIID			iid,		// IID of interface
    const void*			pvProxyVtbl,	// proxy's VTBL ptr
    const VXDCOM_STUB_DISPTBL*	pStubDispTbl	// stub dispatch table
    )
    {
    // Register vtbl pointers with system P/S class factory...
    VxPSFactory* pPSFactory = VxPSFactory::theInstance ();
    pPSFactory->Register (iid, pvProxyVtbl, pStubDispTbl);
    }

/**************************************************************************
*
* vxdcomUserAdd - adds a user to the NTLMSSP user table
*
* This function adds a user/password combination to the VxDCOM NTLM
* security tables. The password must be given in clear text, but only
* the <password hash> is stored. This means that VxDCOM can use the
* authentication setting RPC_CN_AUTHN_LEVEL_CONNECT to allow or
* disallow incoming requests from specific users. If the
* authentication level is set to this value, then only requests from
* users with valid entries in the VxDCOM security tables will be
* honored. Other users will be rejected with an 'access denied' error
* when trying to activate servers on a VxDCOM target.
*
* RETURNS: void
*/

void vxdcomUserAdd
    (
    const char*		userName,	/* user name */
    const char*		passwd		/* user's password in plain text */
    )
    {
    NTLMSSP::userAdd (userName, passwd);
    }


/**************************************************************************
*
* CoInitializeSecurity - establish security for the whole application 
*
* This function differs from its Win32 counterpart in that it only
* supports the de-activation of security settings. This is because
* VxDCOM does not support the full NTLM security subsystem, and so
* this API is provided for source-compatibility only. However, to
* prevent applications which rely on specific Win32 security behavior
* from misbehaving under VxDCOM, this API will fail any attempts to
* establish non-NULL security settings.
*
* RETURNS: S_OK if requested to disable security settings,
* E_INVALIDARG otherwise.
*/

HRESULT CoInitializeSecurity
    (
    void*		psd,		/* security descriptor - MBZ */
    long		cAuths,		/* must be -1 */
    void*		asAuths,	/* array of services - MBZ */
    void*		pReserved1,	/* reserved - MBZ */
    DWORD		dwAuthnLevel,	/* default authentication level */
    DWORD		dwImpLevel,	/* default impersonation level */
    void*		pAuthList,	/* per-service info - MBZ */
    DWORD		dwCapabilities,	/* capabilities - must be EOAC_NONE */
    void*		pReserved3	/* reserved - MBZ */
    )
    {
    if (psd || asAuths || pReserved1 || pAuthList || pReserved3 ||
	(cAuths != -1) ||
	(dwCapabilities != EOAC_NONE))
	return E_INVALIDARG;

    g_defaultAuthnLevel = dwAuthnLevel;
    return S_OK;
    }


/**************************************************************************
*
* CoDisconnectObject - disconnects all remote connections to an object
*
* This function forcibly disconnects all remote connections to the
* given object. It should only ever be called in the same address
* space (i.e. the same VxWorks instance) as the object being
* disconnected. It is not for use by a remote client, for example.
*
* When this function is called, all references on the object instance
* identified by 'pUnk' that are held by the VxDCOM object exporter,
* are released. The result is that the exporter's tables no longer
* hold references to that object, and so any subsequent attempts to
* call methods on any of its interfaces will result in
* RPC_E_INVALID_IPID errors. This approach means that there is no
* chance of object references (to disconnected objects) remaining in
* the server's export tables, and hence less likelyhood of leaks
* occurring at the VxDCOM server.
*
* RETURNS: S_OK if all connections successfully severed.
*/

HRESULT CoDisconnectObject
    (
    IUnknown*           pUnk,           /* object to be disconnected */
    DWORD               dwReserved      /* reserved for future use */
    )
    {
    // Does the object support IMarshal? If so, let it handle the
    // disconnection itself...
    IMarshal* pMshl=0;
    HRESULT hr = pUnk->QueryInterface (IID_IMarshal,
                                       (void**) &pMshl);
    if (FAILED (hr))
        {
        // It doesn't have IMarshal, so use the standard marshaler. We
        // must find the OID of the object in the exporter's tables...
        ObjectExporter* pExp = SCM::objectExporter ();
        ObjectTableEntry* pOTE = pExp->objectFindByIUnknown (pUnk);
        if (pOTE)
            {
            pMshl = new VxStdMarshaler (pOTE->oid);
            pMshl->AddRef ();
            hr = S_OK;
            }
        else
            hr = RPC_E_INVALID_OBJECT;
        }

    // Perform the disconnect, if everything went well...
    if (SUCCEEDED (hr) && pMshl)
        {
        // Use marshaler to disconnect...
        hr = pMshl->DisconnectObject (dwReserved);

        // Release the marshaler...
        pMshl->Release ();
        }

    // Return the result of the DisconnectObject() method...
    return hr;
    }

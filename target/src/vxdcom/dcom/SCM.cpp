/* SCM - Service Control Manager */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
02u,03jan02,nel  Remove usage of alloca.
02t,17dec01,nel  Add include symbol for diab build.
02s,06aug01,dbs  remove instance-creation from SCM
02r,03aug01,dbs  remove usage of Thread class
02q,01aug01,dbs  fix channel-auth for unix only
02p,24jul01,dbs  fix ping-set add/del order, tidy up and comment
02o,18jul01,dbs  clean up stray printfs
02n,13jul01,dbs  fix up includes
02m,14mar01,nel  SPR#35873. Modify code to search for first good address in
                 dual string array passed as part of marshaling an interface.
02l,02mar01,nel  SPR#35589. Add code to make Oxid addresses unique to prevent
                 clash with other targets.
02k,20sep00,nel  ensure SCM closes down properly.
02j,22aug00,nel  Add assert to catch case where SCM hasn't been init and
                 request of ObjectExporter is received.
02i,10aug00,nel  Win2K patch.
02h,07jun00,nel  Remove static c'tor dependency.
02g,15feb00,dbs  add global SCM interface functions
02f,20aug99,aim  make nextOID MT safe
02e,19aug99,aim  fix MLK
02d,19aug99,aim  change assert to VXDCOM_ASSERT
02c,18aug99,dbs  improve resilience by use of RC-ptr to ObjExp
02b,17aug99,aim  added Object Exporter project resources
02a,17aug99,dbs  protect remote-SCM list against threads
01z,05aug99,dbs  add sanity checks in CreateInstance()
01y,02aug99,dbs  remove stray printf
01x,26jul99,dbs  fix case where all QIs fail in successful activation
01w,26jul99,dbs  move marshaling into exporter
01v,22jul99,dbs  re-use remote SCM connections
01u,20jul99,dbs  make sure all out-args of RemoteActivation are zeroed when
                 call fails
01t,16jul99,aim  serverAddress functionality moved to base class
01s,15jul99,dbs  check for local-machine activation
01r,15jul99,dbs  ensure proper HRESULTs are returned
01q,13jul99,aim  syslog api changes
01p,12jul99,aim  added timers
01o,09jul99,dbs  implement ping functionality in SCM now
01n,08jul99,dbs  clean up remote-SCM connection after activation
01m,07jul99,dbs  init next-setid in SCM ctor
01l,07jul99,dbs  only marshal resulting ptrs if activation succeeded
01k,07jul99,aim  change from RpcBinding to RpcIfClient
01j,06jul99,dbs  check return-value of remote-activation
01i,06jul99,dbs  simplify activation mechanism
01h,06jul99,dbs  make SCM create ObjectExporter at startup
01g,30jun99,aim  fixed mod history
01f,30jun99,aim  rework for error reporting
01e,28jun99,dbs  remove defaultInstance method
01d,25jun99,dbs  use channel-ID to determine channel authn status
01c,18jun99,aim  resolverAddress return ObjectExporters address
01b,08jun99,aim  rework
01a,27may99,aim  created
*/

#include "SCM.h"
#include "Reactor.h"
#include "INETSockAddr.h"
#include "ObjectExporter.h"
#include "Syslog.h"
#include "orpcLib.h"
#include "private/comMisc.h"
#include "StdProxy.h"
#include "RpcIfClient.h"
#include "TimeValue.h"
#include "taskLib.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_SCM (void)
    {
    return 0;
    }

EXTERN_C const VXDCOM_STUB_DISPTBL IRemoteActivation_vxstub_disptbl;
EXTERN_C const VXDCOM_STUB_DISPTBL IOXIDResolver_vxstub_disptbl;
EXTERN_C const VXDCOM_STUB_DISPTBL ISystemActivator_vxstub_disptbl;

// SCM statics
SCM* SCM::s_theSCM = 0;

//////////////////////////////////////////////////////////////////////////
//
// SCM destructor
//

SCM::~SCM ()
    {
    COM_ASSERT (s_theSCM == this);

    ObjectExporterPtr pObjectExporter = objectExporter ();
    pObjectExporter->AddRef ();

    if (pObjectExporter)
	{
	// Force the exporter to release all exported objects
	pObjectExporter->objectUnregisterAll ();

	// Remove it from our map...
	oxidUnregister (pObjectExporter->oxid ());
	}

    pObjectExporter->Release ();    
    s_theSCM = 0;
    cout << "SCM : closed down" << endl;
    }

//////////////////////////////////////////////////////////////////////////
//

SCM::SCM (Reactor* r, NTLMSSP* ssp)
  : RpcIfServer (r, &m_dispatcher),
    m_mutex (),
    m_exporters (),
    m_oidSets (),
    m_nextSetid (0),
    m_dispatchTable (),
    m_dispatcher (&m_dispatchTable),
    m_ssp (ssp)
    {
    COM_ASSERT(s_theSCM == 0);
    s_theSCM = this;

    
    // Initialise OXID (Object Exporter ID). We factor this node's IP
    // address into the OXID to make sure it has some chance of being
    // unique, even among similar nodes. This code should really be
    // part of the 'network addressing' classes, but that requires
    // some deeper rework in future...

    char hostname [64];
    long adr = -1;

    if (gethostname (hostname, sizeof (hostname)) == 0)
        {
#ifdef VXDCOM_PLATFORM_VXWORKS
	adr = ::hostGetByName (hostname);
#else
	hostent* hp = ::gethostbyname (hostname);
	if (hp != 0)
            adr = (long) hp->h_addr;
#endif
        }

    // If we got some valid IP address, 
    if (adr != -1)
        // Use the host IP address to weight the initial OXID value...
        m_nextOxid = ((LONGLONG) adr) << 32;
    else
        // Make some feeble attempt to randomize it...
        m_nextOxid = ::time (0);

    // Initialise the next available OID (Object ID), using the
    // IP-factored OXID value, plus the thread ID...
    m_nextOid = m_nextOxid + ::taskIdSelf ();
    }

////////////////////////////////////////////////////////////////////////////
//
// returns the string indicating the address of the IOXIDResolver
// interface.

HRESULT SCM::addressBinding (BSTR* pbsResAddr)
    {
    if (rpcAddressFormat (pbsResAddr, 0 /* no portNumber */) < 0)
	return E_FAIL;
    else	
	return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// theSCM - returns the one and only instance
//

SCM* SCM::theSCM ()
    {
    return s_theSCM;
    }

//////////////////////////////////////////////////////////////////////////
//
// registerStdInterfaces - add DCE interfaces to tables
//

void SCM::registerStdInterfaces ()
    {
    m_dispatchTable.dceInterfaceRegister
	(IID_IRemoteActivation,
	 &IRemoteActivation_vxstub_disptbl);

    m_dispatchTable.dceInterfaceRegister
	(IID_IOXIDResolver,
	 &IOXIDResolver_vxstub_disptbl);

    m_dispatchTable.dceInterfaceRegister
	(IID_ISystemActivator,
	 &ISystemActivator_vxstub_disptbl);
    }

//////////////////////////////////////////////////////////////////////////
//

NTLMSSP* SCM::ssp ()
    {
    if (s_theSCM)
	return s_theSCM->m_ssp;
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//
// startService - initiate a SCM service in the current task
//

int SCM::startService ()
    {
    INETSockAddr addr(VXDCOM_SCM_ENDPOINT);

    Reactor r;
    NTLMSSP ssp;

    SCM scm (&r, &ssp);
    SCM::PingTimer pingTimer;

    int result = scm.init (addr);

    if (result == 0)
	{
	scm.reactorGet()->timerAdd (&pingTimer, TimeValue (5));
	S_INFO (LOG_SCM, "SCM bound to " << addr << " fd=" << scm.handleGet ());

	result = scm.reactorGet()->run ();
	}
    
    return result;
    }

//////////////////////////////////////////////////////////////////////////
//
// init - initialize the SCM
//

int SCM::init (INETSockAddr& addr)
    {
    registerStdInterfaces ();

    int result = open (addr, reactorGet(), 1 /* reuse addr */);

    if (result == 0)
	{
	if ((result = hostAddrGet (addr)) == 0)
	    {
	    // Now create the one-and-only object exporter
	    ObjectExporter* pExp=0;

	    HRESULT hr = newObjectExporter (&pExp);
	    if (SUCCEEDED (hr))
		{
		oxidRegister (pExp->oxid (), pExp);

		hr = pExp->init ();
		if (FAILED (hr))
		    {
		    result = hr;
		    }
		}
	    }
	}
    else
        {
	S_ERR (LOG_SCM|LOG_ERRNO, "cannot bind to " << addr);
	}

    return result;
    }

//////////////////////////////////////////////////////////////////////////
//
// stopService - terminate the SCM instance by closing down reactor
//

int SCM::stopService ()
    {
    if (s_theSCM)
	{
	s_theSCM->reactorGet()->eventLoopEnd ();
	s_theSCM->close ();
	}

    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//
// GetNextOid - return the next OID (Object ID) value
//

HRESULT SCM::GetNextOid (OID* pOid)
    {
    VxCritSec cs (m_mutex);
    *pOid = ++m_nextOid;
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// nextOXID - return the next OXID (Object eXporter ID) value
//

OXID SCM::nextOXID ()
    {
    VxCritSec cs (m_mutex);
    return ++m_nextOxid;
    }

//////////////////////////////////////////////////////////////////////////
//
// nextOID - return the next OID (Object ID) value
//

OID SCM::nextOid ()
    {
    OID o;
    this->GetNextOid (&o);
    return o;
    }

////////////////////////////////////////////////////////////////////////////
//
// SCM::objectExporter -- returns the object exporter for the
// current context, i.e. the task within which the call is made. In
// future this may be PD-dependent, etc, but for now, it simply
// returns the one and only exporter. Note that it *doesn't* add a
// reference for it...
//

ObjectExporter* SCM::objectExporter ()
    {
    SCM* scm = theSCM ();

    ObjectExporter* pExp = 0;

    // enter critical section
    VxCritSec critSec (scm->m_mutex);

    if (scm->m_exporters.size () == 0)
	pExp = 0;
    else
	pExp = (* (scm->m_exporters.begin ())).second;

    COM_ASSERT(pExp != NULL);

    return pExp;
    }

//////////////////////////////////////////////////////////////////////////
//
// newObjectExporter -- utility to create an instance of the 
// ObjectExporter. This is a COM object, so is dealt with as such...
//

HRESULT SCM::newObjectExporter (ObjectExporter** ppObjExp)
    {
    HRESULT hr = S_OK;
    
    // Create an instance of Object Exporter, with one ref...
    ObjectExporterPtr pOX = new ObjectExporter (reactorGet(),
                                               nextOXID ());

    // Did we get one?
    if (! pOX)
        return E_OUTOFMEMORY;
    
    // Open it on the required local endpoint...
    INETSockAddr addr (g_vxdcomObjectExporterPortNumber);

    if (pOX->open (addr, reactorGet(), 1) < 0)
        {
        // Cannot bind to loca endpoint...
        pOX->Release ();
        pOX = 0;
        hr = MAKE_HRESULT (SEVERITY_ERROR,
                           FACILITY_RPC,
                           RPC_S_CANT_CREATE_ENDPOINT);
        }
    else
        {
        // Make sure we got an endpoint?
        if (pOX->hostAddrGet (addr) < 0)
            {
            pOX->Release ();
            pOX = 0;
            hr = E_UNEXPECTED;
            }
        else
            {
            // Success!
            S_INFO (LOG_SCM, "ObjectExporter bound to "
                    << addr
                    << " fd="
                    << pOX->handleGet ());
            }
        }

    if (ppObjExp)
        *ppObjExp = pOX;
    
    return hr;        
    }


//////////////////////////////////////////////////////////////////////////
//
// SCM::RemoteActivation -- called by external DCE clients to activate
// an instance of an object-class on this machine.
//

HRESULT SCM::RemoteActivation
    (
    int			channelId,	// channel
    ORPCTHIS*		pOrpcThis,	// housekeeping
    ORPCTHAT*		pOrpcThat,	// returned housekeeping
    GUID*		pClsid,		// CLSID to activate
    OLECHAR*		pwszObjName,	// NULL
    MInterfacePointer*	pObjStorage,	// NULL
    DWORD		clientImpLevel,	// security
    DWORD		mode,		// all-1's == get-class-obj
    DWORD		nInterfaces,	// num of interfaces
    IID*		pIIDs,		// size_is (nItfs)
    USHORT		cReqProtseqs,	// num of protseqs
    USHORT		arReqProtseqs[],// array of protseqs
    OXID*		pOxid,		// returned OXID
    DUALSTRINGARRAY**	ppdsaOxidBindings,// returned bindings
    IPID*		pIpidRemUnknown,// returned IPID
    DWORD*		pAuthnHint,	// returned security info
    COMVERSION*		pSvrVersion,	// returned server version
    HRESULT*		phr,		// returned activation result
    MInterfacePointer**	ppItfData,	// returned interface(s)
    HRESULT*		pResults	// returned results per i/f
    )
    {
    // Preset default out-args...
    pOrpcThat->flags = 1;
    pOrpcThat->extensions = 0;
    pSvrVersion->MinorVersion = 2;
    pSvrVersion->MajorVersion = 5;
    *pAuthnHint = ssp()->authnLevel ();
    
    // First check with the authentication service that this
    // channel-ID is allowed to activate objects...

#ifndef VXDCOM_PLATFORM_SOLARIS
    HRESULT hrChannel = E_ACCESSDENIED;
    NTLMSSP* pssp = ssp ();
    if (pssp)
	hrChannel = pssp->channelStatusGet (channelId);
#else    
    HRESULT hrChannel = S_OK;
#endif

    if (FAILED (hrChannel))
	{
	// Set out-args to NULL...
	*pOxid = 0;
	*ppdsaOxidBindings = 0;
	*pIpidRemUnknown = GUID_NULL;
	for (DWORD n=0; n < nInterfaces; ++n)
	    {
	    pResults [n] = E_FAIL;
	    ppItfData [n] = 0;
	    }
	*phr = hrChannel;
	return S_OK;
	}
    
    // allocate space for MULTI_QI array
    MULTI_QI* mqi = new MULTI_QI [nInterfaces];

    if (0 == mqi) return E_OUTOFMEMORY;
    
    for (DWORD j=0; j < nInterfaces; ++j)
	{
	// fill MQI array with input arguments...
	mqi [j].pIID = &pIIDs [j];
	mqi [j].pItf = 0;
	mqi [j].hr = S_OK;

	// initialise output-array elements to NULL
	ppItfData [j] = 0;
	}

    // Now activate the requested object/class -- first we need to
    // know the current object-exporter so we can discover its
    // IRemUnknown IPID and OXID value...

    ObjectExporter* pExp = objectExporter ();
    *pOxid = pExp->oxid ();
    *pIpidRemUnknown = pExp->ipidRemUnknown ();

    HRESULT hrActivation = instanceCreate (mode,
					   *pClsid,
					   nInterfaces,
					   mqi);

    // Check that the entire activation was successful, i.e. all
    // marshaling worked...

    DUALSTRINGARRAY* pdsa = 0;
    
    if (SUCCEEDED (hrActivation))
	{
	// Now marshal each of the resulting interface pointers, and
	// output the HRESULTs. We also need to release the local
	// reference from each interface-ptr before it is exported. At
	// least one of the MQIs must have succeeded or else we return
	// E_NOINTERFACE as the hrActivation...

	bool allFailed = true;
	bool someFailed = false;
	
	for (DWORD k=0; k < nInterfaces; ++k)
	    {
	    // Marshal each individual interface ptr, or else fail it
	    // with the appropriate error-code...
	    
	    HRESULT hrm = mqiMarshal (*pClsid, mqi [k], &ppItfData [k]);
	    if (FAILED (hrm))
		pResults [k] = hrm;
	    else
		pResults [k] = mqi [k].hr;

	    // Fail the overall result if any individual interface
	    // failed to be marshaled, otherwise release the local ref
	    // as the interface has now been remoted successfully...
	
	    if (SUCCEEDED (pResults [k]))
		{
		allFailed = false;
		mqi [k].pItf->Release ();
		}
	    else
		{
		someFailed = true;
		hrActivation = mqi [k].hr;
		}
	    }

	// Decide on result -- if all failed that must take priority...
	if (someFailed)
	    hrActivation = CO_S_NOTALLINTERFACES;
	if (allFailed)
	    hrActivation = E_NOINTERFACE;

	if (SUCCEEDED (hrActivation))
	    {	    
	    // Find the address and endpoint of the exporter that is
	    // exporting the object that has just been marshaled
	    BSTR bsSvrAddr;

	    hrActivation = pExp->addressBinding (&bsSvrAddr);
	    if (SUCCEEDED (hrActivation))
		{
		const OLECHAR wszSecInfo [] = { 0x0A, 0xFFFF, 0 };

		// Allocate memory to hold the DSA, allowing some
		// leeway at the end (16 bytes)...
		const DWORD dsaLen = sizeof (DUALSTRINGARRAY) + 
				     (2 * SysStringLen (bsSvrAddr)) +
				     (2 * vxcom_wcslen (wszSecInfo)) +
				     16;
	    
		pdsa = (DUALSTRINGARRAY*) CoTaskMemAlloc (dsaLen);
		if (pdsa)
		    {
		    // Format the DSA...
		    hrActivation = orpcDSAFormat (pdsa,
						  dsaLen,
						  bsSvrAddr,
						  wszSecInfo);
		    }
		else
		    // Memory ran out?
		    hrActivation = E_OUTOFMEMORY;
	    
		SysFreeString (bsSvrAddr);
		}
	    }
	}
    
    // Activation HRESULT is an out-arg...
    *phr = hrActivation;
    *ppdsaOxidBindings = pdsa;

    // free up the mqi structure allocated on the heap
    delete []mqi;

    // This function always returns S_OK...
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// SCM::mqiMarshal -- marshals a single MULTI_QI entry into an
// MInterfacePointer structure...
//

HRESULT SCM::mqiMarshal
    (
    REFCLSID		clsid,		// class ID of containing object
    const MULTI_QI&	mqi,		// MULTI_QI structure
    MInterfacePointer** ppMIP		// output is marshaled ptr
    )
    {
    // Check the MQI contains a valid result...
    if (FAILED (mqi.hr))
	{
	*ppMIP = 0;
	return mqi.hr;
	}
    
    // Create a stream...
    IStream* pStrm=0;
    HRESULT hr = VxRWMemStream::CreateInstance (0,
						IID_IStream,
						(void**) &pStrm);
    if (SUCCEEDED (hr))
	{
	// Marshal interface-ptr into stream - we use the object
	// exporter to do this...
	ObjectExporter* pExp = objectExporter ();
	COM_ASSERT (pExp);
	hr = pExp->objectMarshal (clsid,
				  pStrm,
				  *(mqi.pIID),
				  mqi.pItf,
				  MSHCTX_DIFFERENTMACHINE,
				  0,
				  MSHLFLAGS_NORMAL,
				  0);

	// Now convert the marshaled packet into a MInterfacePointer...
	if (SUCCEEDED (hr))
	    {
	    ISimpleStream* pss=0;
	    hr = pStrm->QueryInterface (IID_ISimpleStream,
					(void**) &pss);
	    if (SUCCEEDED (hr))
		{
		size_t strmSize = pss->size ();
		pss->locationSet (0);
		MInterfacePointer* mip
		    = (MInterfacePointer*) CoTaskMemAlloc (sizeof
							   (MInterfacePointer) +
							   strmSize);
		if (mip)
		    {
		    mip->ulCntData = strmSize;
		    pss->extract (mip->abData, strmSize);
		    pStrm->Release ();
		    }
		else
		    hr = E_OUTOFMEMORY;

		*ppMIP = mip;
		}
	    }
	pStrm->Release ();
	}
    return hr;
    }

////////////////////////////////////////////////////////////////////////////
//
// IndirectActivation - activate remote server on behalf of local client
//
//

HRESULT SCM::IndirectActivation
    (
    LPWSTR              pwszServerName, // PROTSEQ + server name 
    REFGUID		clsid,          // CLSID to activate
    DWORD		mode,           // all-1's == get-class-obj
    DWORD		nItfs,          // num of interfaces
    IID*		iids,           // array of IIDs
    MInterfacePointer**	ppItfData,	// returned interface(s)
    HRESULT*		pResults	// returned results per i/f
    )
    {
    // Create a string-binding representing the remote SCM...
    RpcStringBinding sbRemoteScm (pwszServerName+1,
				  pwszServerName[0],
				  VXDCOM_SCM_ENDPOINT);

    // Prepare for RemoteActivation call...
    ORPCTHIS		orpcThis;
    ORPCTHAT		orpcThat;
    OXID		oxid;
    IPID		ipidRemUnknown;
    HRESULT		hrActivation;
    DUALSTRINGARRAY*	pdsaOxidBinding = 0;
    USHORT		arProtseqs [] = { pwszServerName[0] };
    COMVERSION		serverVersion;
    DWORD               authnHint;
	
    // Initialise in-args...
    orpcThis.version.MajorVersion = 5;
    orpcThis.version.MinorVersion = 1;
    orpcThis.flags = 0;
    orpcThis.reserved1 = 0;
    orpcThis.causality = CLSID_NULL;
    orpcThis.extensions = 0;


    // Get an RPC binding handle to the SCM on the machine where the
    // object (or rather its class-object) lives. We don't need to
    // free the binding handle later as it belongs to the RemoteSCM
    // object itself. First we look to see if we already have a
    // connection to that remote SCM...
 
    SPRemoteSCM& pscm = m_remoteScmTable [sbRemoteScm];
    if (! pscm)
        pscm = new RemoteSCM (sbRemoteScm);
 
    IOrpcClientChannelPtr pClient = pscm->connectionGet ();

    // Request activation - note that this function is defined to
    // always return S_OK even if some of the internal activation
    // results are failures...

    HRESULT hr = IRemoteActivation_RemoteActivation_vxproxy
        (pClient,			// Client handle
         &orpcThis,			// ORPCTHIS
         &orpcThat,			// [out] ORPCTHAT
         (CLSID*)&clsid,		// CLSID
         0,				// pwszObjName
         0,				// pObjStorage
         0,				// imp level
         mode,                          // class-mode?
         nItfs,                         // num itfs
         iids,                          // IIDs
         1,                             // num protseqs
         arProtseqs,                    // array of protseqs
         &oxid,                         // [out] oxid
         &pdsaOxidBinding,		// [out] string binding
         &ipidRemUnknown,		// [out] ipid of rem-unknown
         &authnHint,                    // [out] auth hint
         &serverVersion,		// [out] server version
         &hrActivation,                 // [out] activation result
         ppItfData,			// [out] resulting interfaces
         pResults);			// [out] results for each QI

    // Check results...
    if (FAILED (hr))
        return hr;
    if (FAILED (hrActivation))
        return hrActivation;
    if (! pdsaOxidBinding)
        return E_FAIL;
	
    // Now we have discovered the OXID-resolution for the OXID that is
    // exporting the newly-created object, we need to tell the SCM
    // about it...

    oxidBindingUpdate (oxid,
                       sbRemoteScm,
                       ipidRemUnknown, 
                       RpcStringBinding (pdsaOxidBinding));

    return S_OK;
    }


////////////////////////////////////////////////////////////////////////////
//
// instanceCreate -- creates an instance of a class, marshals its
// interface(s), and returns the whole lot in one go! This is used by
// the SCM to create instances in response to external requests via
// the IRemoteActivation interface.
//

HRESULT SCM::instanceCreate
    (
    DWORD		mode,		// get-class-obj?
    REFCLSID		clsid,		// CLSID to create
    DWORD		nInterfaces,	// num i/f's to return
    MULTI_QI*		mqi		// resulting itf ptrs
    )
    {
    // First create a class-factory object...

    cout << "In SCM::instanceCreate" << endl;
    
    IUnknown* punk=0;
    IClassFactory* pCF;

    HRESULT hr = comClassObjectGet (clsid,
                                    CLSCTX_INPROC_SERVER,
                                    "",
                                    IID_IClassFactory,
                                    (void**) &pCF);
    if (FAILED (hr))
        {
        cout << "no class object" << endl;
        return hr;
        }

    // If we are being called in CLASS-MODE then just return the
    // class-factory pointer, otherwise create an instance and QI
    // it for all requested interfaces...


    if (mode == MODE_GET_CLASS_OBJECT)
        {
        punk = pCF;
        }
    else
        {
        // Create an instance of the class...
        hr = pCF->CreateInstance (0,
                                  IID_IUnknown,
                                  (void**) &punk);
        pCF->Release();
        }
        
    // Now QI for each of the requested interfaces...
    if (SUCCEEDED (hr))
        {
        for (ULONG n=0; n < nInterfaces; ++n)
            {
            mqi[n].hr = punk->QueryInterface (*(mqi[n].pIID),
                                              (void**) &mqi[n].pItf);
            if (FAILED (mqi[n].hr))
                hr = CO_S_NOTALLINTERFACES;
            }

        punk->Release ();
        }
    else
        cout << "class-factory failed" << endl;
    
    return hr;
    }


//////////////////////////////////////////////////////////////////////////
//
// oxidBindingUpdate -- update our table of OXID-resolver and
// RemoteOxid entries...
//

void SCM::oxidBindingUpdate
    (
    OXID			oxid,
    const RpcStringBinding&	sbRemoteScm,
    REFIPID			ipidRemUnk,
    const RpcStringBinding&	sbRemoteOxid
    )
    {
    // See if we already know about a remote SCM at this address...
    SPRemoteSCM& rscm = m_remoteScmTable [sbRemoteScm];

    // If not, make a new one...
    if (! rscm)
	rscm = new RemoteSCM (sbRemoteScm);

    // Now update its knowledge of this OXID...
    rscm->oxidBindingUpdate (oxid, ipidRemUnk, sbRemoteOxid);
    }

//////////////////////////////////////////////////////////////////////////
//
// oxidResolve -- given an OXID and a string-binding to the remote
// machine on which the OXID is known to exist, return the address of
// the actual remote Object Exporter. The 'resAddr' may contain the
// port-number 135, or may omit it...
//

HRESULT SCM::oxidResolve
    (
    OXID			oxid,
    const RpcStringBinding&	resAddr,
    SPRemoteOxid&		remOxid
    )
    {
    // Create string-binding to remote SCM/OxidResolver, with explicit
    // port-number...
    RpcStringBinding sbor (resAddr.ipAddress (),
			   resAddr.protocolSequence (),
			   VXDCOM_SCM_ENDPOINT);

    // If we don't already have a RemoteSCM object representing that
    // network address, then create one...
    SPRemoteSCM rscm = 0;
    {
    VxCritSec cs1 (m_mutex);
    REMOTESCMMAP::const_iterator s = m_remoteScmTable.find (sbor);
    if (s == m_remoteScmTable.end ())
	{
	rscm = new RemoteSCM (sbor);
	m_remoteScmTable [sbor] = rscm;
	}
    else
	rscm = (*s).second;
    }

    // Now see if it knows about the specific OXID...
    if (! rscm->oxidBindingLookup (oxid, remOxid))
	{
	IPID			ipidRemUnk;
	DWORD			authnHint;
	DUALSTRINGARRAY*	pdsa=0;
	OLECHAR			arProtseqs [] = { NCACN_IP_TCP };
	HRESULT			hr = S_OK;
	
	// Need to remotely query the OXID-resolver for the answer...
        IOrpcClientChannelPtr pChannel = new RpcIfClient (resAddr);
	    
	// Find the (remote) server, given the OXID and the string-
	// binding for the resolver - this will be cached and
	// re-used in future if the same OXID recurs...
	hr = IOXIDResolver_ResolveOxid_vxproxy (pChannel,
						&oxid,
						1,
						arProtseqs,
						&pdsa,
						&ipidRemUnk,
						&authnHint);

	// Only S_OK is valid as a 'good' result, anything else may be
	// a warning that the method failed somehow...
	if (hr == S_OK)
	    {
            // Now we have all the info, we need to record it...
            oxidBindingUpdate (oxid,
                               sbor,
                               ipidRemUnk, 
                               RpcStringBinding (pdsa));
            }
                
	// Tidy up
	if (pdsa)
            CoTaskMemFree (pdsa);
	}
    
    // Return the new binding...
    if (! rscm->oxidBindingLookup (oxid, remOxid))
	return OR_INVALID_OXID;
    
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// SCM::SimplePing -- ping all the objects whose OIDs are in the set
// indicated by 'setid'...
//

HRESULT SCM::SimplePing
    (
    SETID		setid		// [in] set ID
    )
    {
    TRACE_CALL;
    
    S_INFO (LOG_SCM, "SimplePing()");

    // enter critical section
    VxCritSec critSec (m_mutex);
	
    // Find the appropriate set...
    OIDSETMAP::iterator i = m_oidSets.find (setid);
    if (i == m_oidSets.end ())
	return OR_INVALID_SET;

    HRESULT hr = S_OK;

    // Iterate over all OIDs in this set...
    OIDSET& oidset = (*i).second;
    for (OIDSET::iterator o = oidset.begin(); o != oidset.end (); ++o)
	{
	OID oid = (*o);
        bool pinged = false;

	// Ping the exporter associated with this OID - we don't know
	// which one it is so try them all until we hit one
	for (OXIDMAP::iterator j = m_exporters.begin ();
	     j != m_exporters.end ();
	     ++j)
	    {
	    ObjectExporterPtr pExp = (*j).second;
	    COM_ASSERT (pExp != 0);
	    if (pExp && pExp->oidPing (oid) == S_OK)
	        {
                pinged = true;
                break;
                }
	    }

            if (! pinged)
                hr = OR_INVALID_OID;
	}
    
    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//
// SCM::ComplexPing -- add/remove some OIDs from the ping-set
// indicated by the received SETID, which may also be zero menaing
// 'create a new set'...
//

HRESULT SCM::ComplexPing
    (
    SETID*		pSetid,		// [in,out] set ID
    USHORT		SeqNum,		// [in] sequence number
    USHORT		cAddToSet,	// [in]
    USHORT		cDelFromSet,	// [in]
    OID			AddToSet [],	// [in]
    OID			DelFromSet [],	// [in]
    USHORT*		pPingBackoffFactor // [out]
    )
    {
    TRACE_CALL;

    S_INFO (LOG_SCM, "ComplexPing()");
    
    SETID setid = *pSetid;

    // enter critical section
    VxCritSec critSec (m_mutex);
    
    // Are we being asked to create a new set?
    if (setid == 0)
	{
	// Yes we are -- make a new entry in the sets table...
	setid = ++m_nextSetid;
	m_oidSets [setid] = OIDSET ();
	}
    
    // Make sure we have the entry now...
    OIDSETMAP::iterator i = m_oidSets.find (setid);
    if (i == m_oidSets.end ())
	return OR_INVALID_SET;

	HRESULT hr = S_OK;

    // Delete some OIDs from the set...
    for (unsigned int d=0; d < cDelFromSet; ++d)
	(*i).second.erase (DelFromSet [d]);

    // Add some OIDs to the set...
    for (unsigned int a=0; a < cAddToSet; ++a)
	{
	OID oid = AddToSet [a];
	
	// Add the OID to the set...
	(*i).second.insert (oid);

	bool pinged = false;

	// Ping the exporter associated with this OID - we don't know
	// which one it is so try them all until we hit one
	for (OXIDMAP::iterator j = m_exporters.begin ();
	     j != m_exporters.end ();
	     ++j)
	    {
	    ObjectExporterPtr pExp = (*j).second;
	    COM_ASSERT (pExp);
            if (pExp && SUCCEEDED (pExp->oidPing (oid)))
                {
                pinged = true;
                break;
                }
            }
            if (! pinged)
                hr = OR_INVALID_OID;
	}
    
    // Update the SETID value...
    *pSetid = setid;
    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//
// SCM::ResolveOxid2 -- given an OXID, return the string-binding
// indicating how to connect to the Object Exporter represented by
// that OXID value...
//

HRESULT SCM::ResolveOxid2
    (
    OXID		oxid,		// [in] OXID to resolve
    USHORT		cReqProtseqs,	// [in] num of protseqs
    USHORT		arReqProtseqs[],// [in] array of protseqs
    DUALSTRINGARRAY**	ppdsaOxidBindings,// [out] bindings
    IPID*		pipidRemUnknown,// [out] IPID
    DWORD*		pAuthHint,	// [out] security info
    COMVERSION*		pComVersion	// [out] COM version
    )
    {
    TRACE_CALL;

    ObjectExporterPtr	pExp;
    DUALSTRINGARRAY*	pdsa=0;
    OLECHAR		wszSecInfo [] = { 0x0A, 0xFFFF, 0 };
    HRESULT		hr = S_OK;

    // Enumerate all object-exporters and see if any have the OXID
    // value given...

    if (cReqProtseqs)
	{
	VxCritSec critSec (m_mutex);

	OXIDMAP::iterator i = m_exporters.begin ();
	while (i != m_exporters.end ())
	    {
	    pExp = (*i).second;
	    if (pExp && pExp->oxid () == oxid)
		{
		pExp->AddRef ();
		break;
		}
	    else
		pExp = 0;
	    }
	}
    
    if (pExp)
	{
	// Get info we need from exporter...
	*pipidRemUnknown = pExp->ipidRemUnknown ();

	BSTR bsSvrAddr;

	HRESULT hrAddr = pExp->addressBinding (&bsSvrAddr);

	pExp->Release ();

	// Create DSA...
	if (SUCCEEDED (hrAddr))
	    {
	    // Allocate memory to hold the DSA...
	    const DWORD dsaLen = sizeof (DUALSTRINGARRAY) + 
				 (2 * SysStringLen (bsSvrAddr)) +
				 (2 * vxcom_wcslen (wszSecInfo)) +
				 16;
	    pdsa = (DUALSTRINGARRAY*) CoTaskMemAlloc (dsaLen);
	    if (pdsa)
		// Format the DSA...
		hr = orpcDSAFormat (pdsa, dsaLen, bsSvrAddr, wszSecInfo);
	    else
		// Memory ran out?
		hr = E_OUTOFMEMORY;
	    SysFreeString (bsSvrAddr);
	    }
	else
	    hr = E_OUTOFMEMORY;
	}
    else
	{
	*pipidRemUnknown = GUID_NULL;
	hr = OR_INVALID_OXID;
	}

    *ppdsaOxidBindings = pdsa;
    pComVersion->MajorVersion = 5;
    pComVersion->MinorVersion = 2;
    *pAuthHint = 1;

    return hr;
    }
    
//////////////////////////////////////////////////////////////////////////
//
// SCM::oxidRegister -- register an ObjectExporter (by its OXID
// value) with the SCM...
//

void SCM::oxidRegister (OXID oxid, ObjectExporter* pexp)
    {
    VxCritSec cs (m_mutex);
    m_exporters [oxid] = pexp;
    }

//////////////////////////////////////////////////////////////////////////
//
// SCM::oxidUnregister -- unregister an ObjectExporter (by its OXID
// value) from the SCM...
//

void SCM::oxidUnregister (OXID oxid)
    {
    VxCritSec cs (m_mutex);
    m_exporters.erase (oxid);
    }

//////////////////////////////////////////////////////////////////////////
//
// SCM::tick -- it is now 'nSecs' since this function was last
// called, so we need to prod all the exporters to timeout their
// objects. Rather than locking the 'm_exporters' table for a long
// time, we cache all the OXID values first, then unlock the table and
// individually tick() each exporter. Finally, we tick() all the
// RemoteSCM objects...
//

void SCM::tick (size_t nSecs)
    {
    OXID oxids [MAX_EXPORTERS];
    size_t nOxids=0;

    // Cache all OXID values
    if (nSecs)
	{
	VxCritSec cs1 (m_mutex);

	for (OXIDMAP::iterator i = m_exporters.begin ();
	     i != m_exporters.end ();
	     ++i)
	    {
	    oxids [nOxids++] = (*i).second->oxid ();
	    }
	}

    // Now ask each exporter to perform its timeouts...
    for (size_t n=0; n < nOxids; ++n)
	{
	ObjectExporterPtr pExp = m_exporters [ oxids[n] ];
	if (pExp)
	    pExp->tick (nSecs);
	}

    // Timeout each of the RemoteSCM objects...
    {
    VxCritSec cs2 (m_mutex);

    for (REMOTESCMMAP::iterator s = m_remoteScmTable.begin ();
	 s != m_remoteScmTable.end ();
	 ++s)
	{
	(*s).second->pingTick (nSecs);
	}
    }
    
    }

//////////////////////////////////////////////////////////////////////////
//

void SCM::oidAdd (const RpcStringBinding& resAddr, OID oid)
    {
    REMOTESCMMAP::iterator i = m_remoteScmTable.find (resAddr);
    if (i != m_remoteScmTable.end ())
        (*i).second->oidAdd (oid);
    }

//////////////////////////////////////////////////////////////////////////
//

void SCM::oidDel (const RpcStringBinding& resAddr, OID oid)
    {
    REMOTESCMMAP::iterator i = m_remoteScmTable.find (resAddr);
    if (i != m_remoteScmTable.end ())
        (*i).second->oidDel (oid);
    }

int
SCM::PingTimer::handleTimeout
    (
    const TimeValue&	timerValue
    )
    {
    S_DEBUG (LOG_SCM, "SCM::PingTimer::handleTimeout: " << timerValue);

    SCM* scm = SCM::theSCM ();

    if (scm)
	scm->tick (timerValue.sec ());

    return scm ? 0 : -1;
    }

//////////////////////////////////////////////////////////////////////////
//
// Global IRemoteActivation, IOXIDResolver and IPrivateSCM
// functions. They simply defer to the equivalent methods in the SCM.
//

HRESULT RemoteActivation
    (
    void*		channelId,
    ORPCTHIS*		pORPCthis,
    ORPCTHAT*		pORPCthat,
    GUID*		pClsid,
    WCHAR*		pwszObjectName,
    MInterfacePointer*	pObjectStorage,
    DWORD		clientImpLevel,
    DWORD		mode,
    DWORD		nItfs,
    IID*		pIIDs,
    unsigned short	cReqProtseqs,
    USHORT*		reqProtseqs,
    OXID*		pOxid,
    DUALSTRINGARRAY**	ppdsaOxidBindings,
    IPID*		pipidRemUnknown,
    DWORD*		pAuthnHint,
    COMVERSION*		pServerVersion,
    HRESULT*		phr,
    MInterfacePointer**	ppInterfaceData,
    HRESULT*		pResults
    )
    {
    SCM* pscm = SCM::theSCM ();
    return pscm->RemoteActivation ((int) channelId,
				   pORPCthis,
				   pORPCthat,
				   pClsid,
				   pwszObjectName,
				   pObjectStorage,
				   clientImpLevel,
				   mode,
				   nItfs,
				   pIIDs,
				   cReqProtseqs,
				   reqProtseqs,
				   pOxid,
				   ppdsaOxidBindings,
				   pipidRemUnknown,
				   pAuthnHint,
				   pServerVersion,
				   phr,
				   ppInterfaceData,
				   pResults);
    }




HRESULT ResolveOxid
    (
    void*		pvRpcChannel,
    OXID*		pOxid,
    unsigned short	cRequestedProtseqs,
    unsigned short*	arRequestedProtseqs,
    DUALSTRINGARRAY**	ppdsaOxidBindings,
    IPID*		pipidRemUnknown,
    DWORD*		pAuthnHint
    )
    {
    SCM* pscm = SCM::theSCM ();
    COMVERSION dummy;
    return pscm->ResolveOxid2 (*pOxid,
			       cRequestedProtseqs,
			       arRequestedProtseqs,
			       ppdsaOxidBindings,
			       pipidRemUnknown,
			       pAuthnHint,
			       &dummy);
    }


HRESULT SimplePing (void* pvRpcChannel, SETID* pSetId)
    {
    SCM* pscm = SCM::theSCM ();
    return pscm->SimplePing (*pSetId);
    }

HRESULT ComplexPing
    (
    void*		pvRpcChannel,
    SETID*		pSetId,
    unsigned short	SequenceNum,
    unsigned short	cAddToSet,
    unsigned short	cDelFromSet,
    OID*		AddToSet,
    OID*		DelFromSet,
    unsigned short*	pPingBackoffFactor
    )
    {
    SCM* pscm = SCM::theSCM ();
    return pscm->ComplexPing (pSetId,
			      SequenceNum,
			      cAddToSet,
			      cDelFromSet,
			      AddToSet,
			      DelFromSet,
			      pPingBackoffFactor);
    }

HRESULT ServerAlive (void* pvRpcChannel)
    {
    return S_OK;
    }

HRESULT ADummyMethod (void * pvRpcChannel)
    {
    return S_OK;
    }

HRESULT ResolveOxid2
    (
    void*		pvRpcChannel,
    OXID*		pOxid,
    unsigned short	cRequestedProtseqs,
    unsigned short*	arRequestedProtseqs,
    DUALSTRINGARRAY**	ppdsaOxidBindings,
    IPID*		pipidRemUnknown,
    DWORD*		pAuthnHint,
    COMVERSION*		pComVersion
    )
    {
    SCM* pscm = SCM::theSCM ();
    return pscm->ResolveOxid2 (*pOxid,
			       cRequestedProtseqs,
			       arRequestedProtseqs,
			       ppdsaOxidBindings,
			       pipidRemUnknown,
			       pAuthnHint,
			       pComVersion);
    }

#if 0
HRESULT IndirectActivation
    (
    LPWSTR              pwszServerName, // PROTSEQ + server name 
    REFGUID		clsid,		// CLSID to activate
    DWORD		mode,		// all-1's == get-class-obj
    DWORD		nItfs,		// num of interfaces
    IID*		pIIDs,		// array of IIDs
    HRESULT*		phr,		// returned activation result
    MInterfacePointer**	ppItfData,	// returned interface(s)
    HRESULT*		pResults	// returned results per i/f
    )
    {
    SCM* pscm = SCM::theSCM ();
    return pscm->IndirectActivation (pwszServername,
                                     clsid,
                                     mode,
                                     nItfs,
                                     pIIDs,
                                     phr,
                                     ppItfData,
                                     pResults);
    }

HRESULT AddOid (OID oid)
    {
    SCM* pscm = SCM::theSCM ();
    return pscm->AddOid (oid);
    }

HRESULT DelOid (OID oid)
    {
    SCM* pscm = SCM::theSCM ();
    return pscm->DelOid (oid);
    }

HRESULT GetNextOid (OID* pOid)
    {
    SCM* pscm = SCM::theSCM ();
    return pscm->GetNextOid (pOid);
    }

HRESULT GetOxidResolverBinding
    (
    USHORT                  cProtseqs,
    USHORT                  arProtseqs[],
    DUALSTRINGARRAY**       ppdsaBindings
    )
    {
    SCM* pscm = SCM::theSCM ();
    return pscm->GetOxidResolverBinding (cProtseqs,
                                         arProtseqs,
                                         ppdsaBindings);
    }
#endif


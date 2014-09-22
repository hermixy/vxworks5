/* RemoteRegistry.cpp - DCOM remote registry */

/* Copyright (c) 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,03jan02,nel  Remove T2OLE.
01c,17dec01,nel  Add include symbol for diab.
01b,16aug01,nel  Correct error code for invalid hint.
01a,07aug01,dbs  written
*/

/*
DESCRIPTION

This class is used to create remote instances of COM classes. It takes
the 'hint' parameter to IRegistry::CreateInstance() as being the
remote server address, and so attempts to create an instance there.

Since it is impossible to know whether the remote server exists, or if
it does indeed have that class registered, this registry always
responds with S_OK when asked IsClassRegistered(), even though it
doesn't actually know.

Then, when it asked to create an instance, or to get the class-object,
it attempts to establish communication with the remote server via
the IRemoteActivation DCE interface, and to get the requested object
that way.
*/


#include "RemoteRegistry.h"
#include "RpcStringBinding.h"
#include "orpc.h"
#include "orpcLib.h"
#include "SCM.h"
#include "RemoteActivation.h"
#include "RpcIfClient.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_RemoteRegistry (void)
    {
    return 0;
    }


//////////////////////////////////////////////////////////////////////////
//
// dcomRemoteRegistryInit - initialize the remote-registry
//

int dcomRemoteRegistryInit ()
    {
    IRegistry* pReg;

    HRESULT hr = RemoteRegistryClass::CreateInstance (0,
                                                      IID_IRegistry,
                                                      (void**) &pReg);
    if (FAILED (hr))
        return hr;

    hr = comRegistryAdd ("Remote Registry",
                         CLSCTX_REMOTE_SERVER,
                         pReg);

    pReg->Release ();

    return hr;
    }


//////////////////////////////////////////////////////////////////////////
//

RemoteRegistry::RemoteRegistry ()
    {
    }


//////////////////////////////////////////////////////////////////////////
//

RemoteRegistry::~RemoteRegistry ()
    {
    }



//////////////////////////////////////////////////////////////////////////
//
// RegisterClass - add a class to the registry
//
// Since we don't actually keep track of remote classes, we don't need
// to do anything.
//

HRESULT RemoteRegistry::RegisterClass
    (
    REFCLSID                clsid,
    void *                  pfnGetClassObject
    )
    {
    return S_OK;
    }


//////////////////////////////////////////////////////////////////////////
//
// IsClassRegistered - determine if the class is registered
//
// This method always returns S_OK, as it doesn't know if the class is
// registered or not until it tries to contact the remote server.
//

HRESULT RemoteRegistry::IsClassRegistered
    (
    REFCLSID                clsid
    )
    {
    return S_OK;
    }


//////////////////////////////////////////////////////////////////////////
//
// CreateInstance - creates a remote instance of the class
//
// 
//

HRESULT RemoteRegistry::CreateInstance
    (
    REFCLSID                clsid,
    IUnknown *              pUnkOuter,
    DWORD                   dwClsContext,
    const char *            hint,
    ULONG                   cMQIs,
    MULTI_QI *              pMQIs
    )
    {
    return instanceCreate (false,
                           clsid,
                           pUnkOuter,
                           dwClsContext,
                           hint,
                           cMQIs,
                           pMQIs);
    }


//////////////////////////////////////////////////////////////////////////
//
// GetClassObject - instantiate a remote class object
//

HRESULT RemoteRegistry::GetClassObject
    (
    REFCLSID                clsid,
    REFIID                  iid,
    DWORD                   dwClsContext,
    const char *            hint,
    IUnknown **             ppClsObj
    )
    {
    MULTI_QI mqi[] = { { &iid, 0, S_OK } };

    HRESULT hr = instanceCreate (true,
                                 clsid,
                                 0,
                                 dwClsContext,
                                 hint,
                                 1,
                                 mqi);
    if (FAILED (hr))
        return hr;

    if (ppClsObj)
        *ppClsObj = mqi[0].pItf;
    return mqi[0].hr;
    }


//////////////////////////////////////////////////////////////////////////
//
// GetClassID - for iterating over all registered class IDs
//
// As we don't keep track of them, we don't have naything to return.
//

HRESULT RemoteRegistry::GetClassID
    (
    DWORD                   dwIndex,
    LPCLSID                 pclsid
    )
    {
    return E_FAIL;
    }
    

//////////////////////////////////////////////////////////////////////////
//
// instanceCreate - create a remote instance of a server class
//
// This method is used to create remote instances of classes.
//

HRESULT RemoteRegistry::instanceCreate
    (
    bool                    classMode,
    REFCLSID                clsid,
    IUnknown *              pUnkOuter,
    DWORD                   dwClsContext,
    const char *            hint,
    ULONG                   cMQIs,
    MULTI_QI *              pMQIs
    )
    {
    // Validate args...
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (!(dwClsContext & CLSCTX_REMOTE_SERVER))
        return E_INVALIDARG;

    if ((! hint) || (strlen (hint) == 0))
        return REGDB_E_CLASSNOTREG;

    // Create a string-binding representing the remote SCM...
    RpcStringBinding sbRemoteScm (hint,
				  NCACN_IP_TCP,
				  VXDCOM_SCM_ENDPOINT);

    // Get the formatted address of the remote SCM, with the protseq
    // but no port-number...
    LPWSTR remAddr = new OLECHAR [strlen (sbRemoteScm.formatted (false)) + 1];
    
    comAsciiToWide (remAddr,
	            sbRemoteScm.formatted (false),
		    strlen (sbRemoteScm.formatted (false)) + 1);
    
    // Copy IIDs into array...

    IID iids [_ACTIVATION_MAX];

    {
    DWORD n;

    for (n=0; n < cMQIs; ++n)
        iids [n] = *(pMQIs[n].pIID);

    }

    // Results will be received here...
    MInterfacePointer* pItfData [_ACTIVATION_MAX];
    HRESULT hResults [_ACTIVATION_MAX];

    // Find our local SCM...
    SCM* pscm = SCM::theSCM ();
    
    // Ask SCM for indirect activation of remote server...
    HRESULT hr = pscm->IndirectActivation (remAddr,
                                           clsid,
                                           classMode ? MODE_GET_CLASS_OBJECT : 0,
                                           cMQIs,
                                           iids,
                                           pItfData,
                                           hResults);
    if (FAILED (hr))
	{
	delete []remAddr;
        return hr;
	}
    
    // Unmarshal the interface pointers - for this we need an IStream
    // implementation, supplied by VxRWMemStream...

    ULONG j;

    for (j=0; j < cMQIs; ++j)
        {
        if (pItfData [j])
            {
            VxRWMemStream   memStrm;
		
            // make sure we don't try to delete it!
            memStrm.AddRef ();
		
            // Initialise stream from the MInterfacePointer...
            memStrm.insert (pItfData [j]->abData,
                            pItfData [j]->ulCntData);
            memStrm.locationSet (0);

            // Unmarshal the stream
            pMQIs[j].hr = CoUnmarshalInterface (static_cast<IStream*> (&memStrm),
                                                *(pMQIs[j].pIID),
                                                (void**) &pMQIs[j].pItf);
            if (FAILED (pMQIs[j].hr))
                {
                hr = pMQIs[j].hr;
                break;
                }
            }
        else
            pMQIs[j].pItf = 0;
        }

    delete []remAddr;
		
    return hr;
    }


/* RpcDispatcher */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01r,17dec01,nel  Add include symbol for diab.
01q,13jul01,dbs  fix up includes, exclude priority propagation for now
01p,13jul00,nel  Win2K fix.
01o,26jun00,dbs  implement presentation context IDs
01n,19aug99,aim  added TraceCall header
01m,13aug99,drm  Documentation updates
01l,10aug99,aim  fix compiler warnings on solaris
01k,06aug99,drm  Correcting bug related to error checking.
01j,03aug99,drm  Adding code to set server priority.
01i,28jul99,drm  Adding clsid argument to interfaceInfoGet() call.
01h,09jul99,dbs  convert to final naming scheme
01g,02jul99,aim  fix for name changes in RpcPduFactory
01f,29jun99,dbs  release IUnknown-ptr after use
01e,25jun99,dbs  add channel ID to stub msg
01d,24jun99,dbs  move authn into new class
01c,23jun99,dbs  fix authn on response packets
01b,08jun99,aim  rework
01a,27may99,aim  created
*/

#include "RpcDispatcher.h"
#include "RpcPdu.h"
#include "RpcPduFactory.h"
#include "NdrStreams.h"		// marshaling streams
#include "Syslog.h"
#include "orpcLib.h"
#include "orpc.h"
#include "vxdcomExtent.h"
#include "TraceCall.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_RpcDispatcher (void)
    {
    return 0;
    }

RpcDispatcher::RpcDispatcher (RpcDispatchTable* dispatchTable)
  : m_dispatchTable (dispatchTable)
    {
    TRACE_CALL;
    }

RpcDispatcher::~RpcDispatcher ()
    {
    TRACE_CALL;
    }

bool
RpcDispatcher::supportsInterface (REFIID riid)
    {
    return m_dispatchTable->supportsInterface (riid);
    }

extern IID IID_ISystemActivator;

int
RpcDispatcher::dispatch (const RpcPdu& request, RpcPdu& reply, int channelId, REFIID iid)
    {
    PFN_ORPC_STUB	pfn;
    IUnknown* 		punkItf=0;
    CLSID 		classid = GUID_NULL;	// CLSID of object
    ORPCTHIS		orpcThis; // holds unmarshalled ORPCTHIS 

    HRESULT hr = m_dispatchTable->interfaceInfoGet (iid,
						    request.objectId (),
						    request.opnum (),
						    &punkItf,
						    &pfn,
						    classid);

    // Here, we test explicitly for S_OK (rather than using FAILED())
    // since there is only one success (S_OK) and *any* other result
    // is a kind of failure...
    if (hr == S_OK)
	{
	// Create a marshaling-stream ready to hold the reply
	NdrMarshalStream ms (NdrPhase::STUB_MSHL, request.drep ());

	// Create an unmarshaling-stream holding the received
	// stub-data
	NdrUnmarshalStream us (NdrPhase::STUB_UNMSHL,
			       request.drep (),
			       (byte*) request.stubData (),
			       request.stubDataLen ());
	
	if (request.objectId() != GUID_NULL)
	    {
	    // Remove the ORPCTHIS from the stream...
	    hr = ndrUnmarshalORPCTHIS (&us, &orpcThis);

	    // Insert an ORPCTHAT at the start of the stub-data
	    ORPCTHAT orpcThat = { 0, 0 };
	    ms.insert (sizeof (ORPCTHAT), &orpcThat, false);
	    }

	// Fill in msg structure
	RPC_STUB_MSG msg (&us, &ms, channelId);

#if 0
#ifdef VXDCOM_PLATFORM_VXWORKS
	int		oldPriority;		// used to restore priority 
	int 		currentId=0;		// task ID of current task
	BOOL		priorityChanged=FALSE;	// whether or not priority was set
	ORPC_EXTENT* 	pExtent;		// ptr to VXDCOM_EXTENT
        
        currentId = taskIdSelf ();

	// Lookup the priority scheme for the object and set the 
	// priority to the appropriate value.
        if (classid != GUID_NULL)
            {
            int priority;
            vxdcomClassObjRegInfo regInfo;      // result of lookup

            hr = vxdcomSymLookup (classid, &regInfo);
            if (SUCCEEDED (hr))
                {
                switch (regInfo.priorityScheme)
                    {
                    case PS_DEFAULT:
                        priority = g_defaultServerPriority;
                        break;

                    case PS_SVR_ASSIGNED:
                        priority = regInfo.priority;
                        break;

                    case PS_CLNT_PROPAGATED:
			// If client propagated priority is not present,
			// use the default server priority.
                        if (orpcThis.orpcExtentFind (GUID_VXDCOM_EXTENT,
                                                     &pExtent) == S_OK)
                            {
			    VXDCOM_EXTENT* pVExtent=NULL;	// ptr to derived type
			    pVExtent = static_cast<VXDCOM_EXTENT*> (pExtent);
                            if (pVExtent != NULL)
			        pVExtent->getPriority(priority);
                            else
                                priority = g_defaultServerPriority;
                            }
                        else
                            {
                            priority = regInfo.priority;
                            }
                        break;

                    default:
			// Unknown priority scheme.  Rather than produce an error
			// we will simply use the default server priority.
                        priority = g_defaultServerPriority;
                        break;
                    }


	        // Check for valid priority
                if ( (priority < 0)  || (priority > 255) )
                    {
                    // Invalid priority.  
		    // Let's use the system default priority.
                    priority = g_defaultServerPriority;
                    }

                if (taskPriorityGet (currentId, &oldPriority) == OK)
	            {
                    if (taskPrioritySet (currentId, priority) == OK)
                        {
                        priorityChanged = TRUE;
                        }
                    else
                        {
                        // Unable to set priority
                        // There's really nothing we can do in this case - we'll
                        // just run the function at the current priority.
                        }
                    }
                else
                    {
                    // Unable to get priority.
                    // We might be able to set the priority here, but we wouldn't be 
                    // able to restore, so we'll just run the function at the current
                    // priority.
                    }
                }
            }
        else
            {
            // Since we can't find the class object, we don't know what priority
            // scheme to use. We'll try to run the function at the default server 
            // priority.
            if (taskPriorityGet (currentId, &oldPriority) == OK)
	        {
                if (taskPrioritySet (currentId, g_defaultServerPriority) == OK)
                    {
                    priorityChanged = TRUE;
                    }
                else
                    {
                    // Unable to set priority
                    // There's really nothing we can do in this case - we'll
                    // just run the function at the current priority.
                    }
                }
            else
                {
                // Unable to get priority.
                // We might be able to set the priority here, but we wouldn't be 
                // able to restore, so we'll just run the function at the current
                // priority.
                }
            }
           
#endif
#endif


	// Call stub function...
	hr = pfn (punkItf, msg);

#if 0
#ifdef VXDCOM_PLATFORM_VXWORKS
	// Restore the priority to its previous value.
        if (priorityChanged)
            {
            if (taskPrioritySet (currentId, oldPriority) != OK)
                {
                // We were unable to restore the priority to the default value.  
                // There's not really anything we can do, but leave the thread at the
                // new priority.
                }
            }
#endif
#endif

	// Release itf ptr...
	if (punkItf)
	    punkItf->Release ();

	if (SUCCEEDED (hr))
	    {
	    RpcPduFactory::formatResponsePdu (request, reply);

	    if (ms.size() > 0)
		reply.stubDataAppend (ms.begin (), ms.size ());
	    }
	else
	    {
	    RpcPduFactory::formatFaultPdu (request, reply, hr, false);
	    }
	}
    else
	{
	RpcPduFactory::formatFaultPdu (request, reply, hr, true);
	}

    return hr;
    }

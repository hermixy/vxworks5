/* TaskAllocator.cpp - COM/DCOM TaskAllocator class implementation */

/*
modification history
--------------------
01r,17dec01,nel  Add include symbol for diab.
01q,23oct01,nel  SPR#71142. Add call to comSysRealloc to Realloc method.
01p,13jul01,dbs  remove NEW macro usage
01o,27jun01,dbs  fix include paths and names
01n,19aug99,aim  TASK_LOCK now uses mutex
01m,19aug99,aim  added TraceCall header
01l,05aug99,aim  added mutex
01k,16jul99,aim  undef Free if defined
01j,17jun99,dbs  use NEW not new
01i,17jun99,aim  added standard OPERATOR_NEW_AND_DELETE
01h,02jun99,dbs  use new OS-specific macros
01g,28may99,dbs  simplify allocator strategy
01f,27may99,dbs  change to vxdcomTarget.h
01e,05may99,dbs  fix TASK_LOCK macros under Win32, trim old code out
01d,29apr99,dbs  fix warnings under -Wall
01c,29apr99,dbs  use main COM allocator
01b,26apr99,aim  added TRACE_CALL
01a,20apr99,dbs  created during Grand Renaming

*/

/*
  DESCRIPTION:

  TaskAllocator -- 

*/

#include "TaskAllocator.h"
#include "private/comMisc.h"
#include "private/comSysLib.h"
#include "TraceCall.h"

/* Include symbol for diab */

extern "C" int include_vxcom_TaskAllocator (void)
    {
    return 0;
    }

////////////////////////////////////////////////////////////////////////////
//
// Global Variables -- the pointer 'pSysAllocator' points to the
// IMalloc interface of the current system allocator. In future, there
// may be one per application (one per PD, for example), but right now
// there can only be one ;-)
//
////////////////////////////////////////////////////////////////////////////

IMalloc* pSysAllocator = 0;
static VxMutex coGetMallocLock;

////////////////////////////////////////////////////////////////////////////
//

VxTaskAllocator::VxTaskAllocator ()
  : m_dwRefCount (0),
    m_mutex ()
    {
    TRACE_CALL;
    }

////////////////////////////////////////////////////////////////////////////
//

VxTaskAllocator::~VxTaskAllocator ()
    {
    TRACE_CALL;
    }

////////////////////////////////////////////////////////////////////////////
//

ULONG VxTaskAllocator::AddRef ()
    {
    TRACE_CALL;
    VxCritSec cs (m_mutex);
    return ++m_dwRefCount;
    }

////////////////////////////////////////////////////////////////////////////
//

ULONG VxTaskAllocator::Release ()
    {
    TRACE_CALL;
    VxCritSec cs (m_mutex);
    if (--m_dwRefCount)
	return m_dwRefCount;
    delete this;
    return 0;
    }

////////////////////////////////////////////////////////////////////////////
//

HRESULT VxTaskAllocator::QueryInterface
    (
    REFIID	riid,
    void**	ppv
    )
    {
    TRACE_CALL;
    // Is it one of our own interfaces?
    if ((riid == IID_IUnknown) || (riid == IID_IMalloc))
	{
	*ppv = this;
        AddRef ();
	return S_OK;
	}
    return E_NOINTERFACE;
    }

////////////////////////////////////////////////////////////////////////////
//

void* VxTaskAllocator::Alloc (ULONG cb)
    {
    TRACE_CALL;
    return comSysAlloc (cb);
    }

////////////////////////////////////////////////////////////////////////////
//

void* VxTaskAllocator::Realloc (void *pv, ULONG cb)
    {
    TRACE_CALL;

    return comSysRealloc (pv, cb);
    }

////////////////////////////////////////////////////////////////////////////
//

// resolve clashes in networking headers
#ifdef Free
#undef Free
#endif

void VxTaskAllocator::Free (void *pv)
    {
    TRACE_CALL;
    comSysFree (pv);
    }

////////////////////////////////////////////////////////////////////////////
//

ULONG VxTaskAllocator::GetSize (void *pv)
    {
    TRACE_CALL;
    return (ULONG) -1;
    }

////////////////////////////////////////////////////////////////////////////
//

int VxTaskAllocator::DidAlloc (void *pv)
    {
    TRACE_CALL;
    return -1;
    }

////////////////////////////////////////////////////////////////////////////
//

void VxTaskAllocator::HeapMinimize ()
    {
    TRACE_CALL;
    }

////////////////////////////////////////////////////////////////////////////
//
// comCoGetMalloc - gets a pointer to the task-allocator for the current
// task, which is always the system allocator. If there is already one
// then it simply adds a reference for it, if not then it creates one
// afresh...
//

HRESULT comCoGetMalloc 
    (
    DWORD               dwMemContext,   // MUST BE 1
    IMalloc**           ppMalloc        // output ptr
    )
    {
    TRACE_CALL;
    HRESULT		hr = S_OK;
    
    VxCritSec cs (coGetMallocLock);
    
    // Make sure context is valid
    if (dwMemContext != 1)
	return E_INVALIDARG;

    // Check for existing allocator...
    if (! pSysAllocator)
	{
	// Must create a new one, always keep one extra ref (from QI)
	// so it never gets destroyed...
	VxTaskAllocator* pa = new VxTaskAllocator ();
	if (pa)
	    hr = pa->QueryInterface (IID_IMalloc,
				     (void**) &pSysAllocator);
	else
	    hr = E_OUTOFMEMORY;
	}

    // Hand out a reference to the existing allocator...
    if (SUCCEEDED (hr))
	{
	pSysAllocator->AddRef ();
	*ppMalloc = pSysAllocator;
	}

    return hr;
    }



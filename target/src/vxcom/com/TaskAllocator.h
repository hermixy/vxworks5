/* TaskAllocator.h */

#ifndef __INCTaskAllocator_h
#define __INCTaskAllocator_h

#include "comLib.h"
#include "private/comMisc.h"

////////////////////////////////////////////////////////////////////////////
//
// VxTaskAllocator - this class *CANNOT* be implemented using the
// CComObject classes because it is the object that allows those
// objects to allocate memory, and this would be self-referential.
//
// This allocator just uses malloc() and friends, so it utilises the
// system memory partition.
//

// resolve clashes in networking headers
#ifdef Free
#undef Free
#endif

class VxTaskAllocator : public IMalloc
    {
  public:
    VxTaskAllocator ();
    virtual ~VxTaskAllocator ();

    // IUnknown methods...
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();
    STDMETHOD(QueryInterface) (REFIID riid, void** ppv);
    
    // IMalloc methods
    void* STDMETHODCALLTYPE Alloc (ULONG cb);
    void* STDMETHODCALLTYPE Realloc (void *pv, ULONG cb);
    void  STDMETHODCALLTYPE Free (void *pv);
    ULONG STDMETHODCALLTYPE GetSize (void *pv);
    int   STDMETHODCALLTYPE DidAlloc (void *pv);
    void  STDMETHODCALLTYPE HeapMinimize ();

 private:
    DWORD	m_dwRefCount;
    VxMutex	m_mutex;
    };

extern HRESULT comCoGetMalloc (DWORD dwMemContext, IMalloc** ppMalloc);

#endif


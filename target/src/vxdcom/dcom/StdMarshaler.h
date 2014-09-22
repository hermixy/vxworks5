/* StdMarshaler.h - COM/DCOM StdMarshaler class definition */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01h,13jul01,dbs  fix up includes
01g,10jun99,dbs  remove inclusion of comNew.h
01f,03jun99,dbs  remove refs to comSyncLib
01e,03jun99,dbs  remove reliance on TLS from CoMarshalInterface
01d,29apr99,dbs  fix -Wall warnings
01c,28apr99,dbs  make all classes allocate from same pool
01b,27apr99,dbs  add mem-pool to class
01a,20apr99,dbs  created during Grand Renaming

*/


#ifndef __INCStdMarshaler_h
#define __INCStdMarshaler_h

#include "dcomLib.h"
#include "private/comMisc.h"

///////////////////////////////////////////////////////////////////////////
//
// VxStdMarshaler - the system-standard marshaler, implements the
// functionality of IMarshal required to marshal interface pointers,
// but not to unmarshal them (that is done by VxStdProxy).
//

class VxStdMarshaler : public IMarshal
    {
    DWORD		m_dwRefCount;	// local ref-count
    VxMutex		m_mutex;	// task safety
    OID			m_oid;		// OID of object being
					// marshaled
    
  public:

    VxStdMarshaler (OID oid=0) : m_dwRefCount (0), m_oid (oid) {}

    virtual ~VxStdMarshaler () {}

    // IUnknown methods...
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();
    STDMETHOD(QueryInterface) (REFIID riid, void** ppv);
    
    // IMarshal methods...
    HRESULT STDMETHODCALLTYPE GetUnmarshalClass( 
        /* [in] */ REFIID riid,
        /* [unique][in] */ void  *pv,
        /* [in] */ DWORD dwDestContext,
        /* [unique][in] */ void  *pvDestContext,
        /* [in] */ DWORD mshlflags,
        /* [out] */ CLSID  *pCid);
        
    HRESULT STDMETHODCALLTYPE GetMarshalSizeMax( 
        /* [in] */ REFIID riid,
        /* [unique][in] */ void  *pv,
        /* [in] */ DWORD dwDestContext,
        /* [unique][in] */ void  *pvDestContext,
        /* [in] */ DWORD mshlflags,
        /* [out] */ DWORD  *pSize);
        
    HRESULT STDMETHODCALLTYPE MarshalInterface( 
        /* [unique][in] */ IStream  *pStm,
        /* [in] */ REFIID riid,
        /* [unique][in] */ void  *pv,
        /* [in] */ DWORD dwDestContext,
        /* [unique][in] */ void  *pvDestContext,
        /* [in] */ DWORD mshlflags);
        
    HRESULT STDMETHODCALLTYPE UnmarshalInterface (IStream*, REFIID, void**)
	{ return E_NOTIMPL; }
        
    HRESULT STDMETHODCALLTYPE ReleaseMarshalData (IStream*)
	{ return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE DisconnectObject (DWORD)
	{ return E_NOTIMPL; }

    };


#endif



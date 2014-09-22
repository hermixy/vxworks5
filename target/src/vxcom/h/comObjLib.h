/* comObjLib.h - template-based COM-object construction library (VxCOM) */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
02n,04oct01,dbs  add macros to export coclasses in shlibs
02m,24sep01,nel  SPR#70528. Fix memory leak in CComBSTR.
02l,31jul01,dbs  allow override of InternalAddRef/Release methods
02k,28jun01,dbs  use safe inc/dec funcs, remove VxMutex dependency
02j,21jun01,dbs  make use of VxMutex class
02i,09apr01,nel  SPR#65606. Correct minor formating errors.
02h,28feb01,nel  SPR#35792. Add more ATL defs to bring into line with T2.
02g,16aug00,nel  Add CComBSTR(char *) to CComStr to make MathDemo example work
                 under vxworks.
02f,30may00,nel  Add more variant support
02e,28feb00,nel  Remove VxComTrack dependency on hard coded idl stuff
02d,01feb00,nel  Added VxComTrack code and Fix for singleton operation
02c,15oct99,dbs  fix comments to refer to proper classnames
02b,18aug99,dbs  remove extraneous ptr-conversion operator in CComPtr
02a,13aug99,drm  Bux fix.
01z,13aug99,dbs  fix copy-ctor in CComPtr
01y,29jul99,aim  changed mutex types to void*
01x,28jul99,drm  Changing g_defaultServerPriority to g_defaultServerPriority.
01w,28jul99,dbs  remove PS_CLNT_ASSIGNED enum value
01v,26jul99,dbs  fix typo in COM_INTERFACE_ENTRY macros
01u,30jun99,dbs  add const operator-> to CComPtr
01t,28jun99,dbs  remove obsolete ptrAssign function
01s,04jun99,dbs  fix registry calls
01r,03jun99,dbs  remove comSyncLib and VxMutex class
01q,26may99,dbs  add further ATL compatibility
01p,20may99,dbs  move class-object functionality into CComCoClass
                 class so explicit template instantiation works
01o,04may99,dbs  check return value of alloc in CreateInstance
01n,03may99,drm  adding priority scheme support
01m,27apr99,dbs  add alloc helper funcs
01l,23apr99,dbs  make mutex publically available thru CComObject methods
01k,23apr99,dbs  improve QI implementation, remove virtualness,
                 enforce locking in CComObject class
01j,22apr99,dbs  remove potential leaks, remove extraneous debugging
                 code (will be replaced with official DEBUG-lib in
		 future), simplify locking strategy (we don't need to
		 provide a VxNoLock class at all).
01i,14apr99,dbs  put incr and decr methods into VxMutex class
01h,31mar99,dbs  add CComPtr and CComBSTR classes
01g,03feb99,dbs  use STDMETHOD macros
01f,20jan99,dbs  fix file names - vxcom becomes com
01e,11jan99,dbs  change classnames to be ATL compatible
01d,21dec98,dbs  changes for VXCOM
01c,15dec98,dbs  add singleton support
01b,11dec98,dbs  simplify registry
01a,17nov98,dbs  created

*/

/*

  DESCRIPTION:

  This file contains template classes for creating COM objects using
  multiple inheritance to implement multiple interfaces. Classes
  created with these templates feature class-factories, thread-safe
  reference counting, etc...

  They are source-level compatible with ATL, and use the same
  COM_MAP() style of interface mapping, although the implementation is
  somewhat different.
  */


#ifndef __INCcomObjLib_h__
#define __INCcomObjLib_h__

#include "comCoreLib.h"
#include "comLib.h"
#include <stdio.h>

#define DECLARE_CLASSFACTORY_SINGLETON() enum {singleton=1};

#define DECLARE_REGISTRY_RESOURCE(x)
#define DECLARE_REGISTRY(class, pid, vpid, nid, flags)

#ifndef VXDCOM_COMTRACK_LEVEL
#define VXDCOM_COMTRACK_LEVEL 0
#endif

#if (VXDCOM_COMTRACK_LEVEL == 0)
#define COMTRACK_ADD_CLS(c)
#define COMTRACK_ADD_IID(c) 
#define COMTRACK_UPDATE()
#else
#include "comShow.h"
#define COMTRACK_ADD_CLS(c) if (VxComTrack::theInstance()->addClassInstance ((void *)this, (char *)vxcomGUID2String (GetObjectCLSID ()), #c, m_dwRefCount, GetObjectCLSID ()) != NULL) _qi_impl (IID_IUnknown, NULL)
#define COMTRACK_ADD_IID(c) VxComTrack::theInstance()->addInterface ((void *)this, (char *)vxcomGUID2String (IID_##c), "IID_"#c, IID_##c)
#define COMTRACK_UPDATE() VxComTrack::theInstance()->updateClassInstance (this, m_dwRefCount)
#endif

//////////////////////////////////////////////////////////////////////////
//
// CComObjectRoot - the base class of all the VxDCOM classes. It
// provides basic IUnknown support, plus support for COM aggregation
// and reference-counting...
//

class CComObjectRoot
    {

  public:
    enum { singleton=0 };

    CComObjectRoot (IUnknown *punk=0)
      : m_dwRefCount (0),
	m_pUnkOuter (punk)
	{
	}

    virtual ~CComObjectRoot ()
	{
	}

    // IUnknown implementation support - these methods provide the
    // basis upon which the IUnknown methods are built...

    ULONG InternalAddRef ()
	{
        ULONG n = comSafeInc (&m_dwRefCount);
	return n;
	}

    ULONG InternalRelease ()
	{
        ULONG n = comSafeDec (&m_dwRefCount);

	COMTRACK_UPDATE ();

	if (n)
	    return n;

	// Must delete this object, so we temporarily set its
	// ref-count to 1 so we don't get circular destruction
	// chains...
	m_dwRefCount = 1;

	delete this;
	return 0;
	}
    
  protected:
    long		m_dwRefCount;	// reference-count
    IUnknown*		m_pUnkOuter;	// aggregating outer

    };

//////////////////////////////////////////////////////////////////////////
//
// CComClassFactory - implements IClassFactory to allow objects to
// be created at runtime...
//

template <class T> class CComClassFactory
    : public CComObjectRoot,
      public IClassFactory
    
    {

    enum { singleton_factory=T::singleton };
    
  public:

    CComClassFactory () {}
    
    // IUnknown implementation for class-factory

    ULONG STDMETHODCALLTYPE AddRef ()
        { return InternalAddRef (); }

    ULONG STDMETHODCALLTYPE Release ()
        { 
        ULONG result = InternalRelease ();
        return result;
        }

    HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void ** ppv)
	{
        if (! ppv)
            return E_POINTER;

        if ((riid == IID_IClassFactory) || (riid == IID_IUnknown))
            {
            *ppv = static_cast<IClassFactory*> (this);
            InternalAddRef ();
            return S_OK;
            }

	*ppv = 0;
        return E_NOINTERFACE;
        }


    // IClassFactory implementation
    HRESULT STDMETHODCALLTYPE CreateInstance
	(
	IUnknown*	pUnkOuter,
	REFIID		riid,
	void**		ppv
	);
        
    HRESULT STDMETHODCALLTYPE LockServer (BOOL bLock)
        { return S_OK; }

    };

//////////////////////////////////////////////////////////////////////////
//
// CComCoClass - represents the 'coclass' or component-object
// itself. The 'coclass' represents the whole object, not any
// particular one of its exposed interfaces, and is only required when
// an object/class must be 'externally' createable via
// CoCreateinstance(). Hence, the functions required by
// CComClassFactory and the registration mechanism are wrapped in this
// template-class. It should be mixed in with the user's class when
// this functionality is required.
//

template <class T, const CLSID* pclsid> class CComCoClass
    {
    typedef CComClassFactory<T> ClassFactory_t;

  public:

    static const CLSID& GetObjectCLSID () { return *pclsid; }
    
    // classObjectGet - gets the class' class-object (factory)...
    static HRESULT classObjectGet
	(
	REFCLSID	clsid,
	REFIID		iid,
	void**		ppv
	)
	
        {

	// Validate CLSID
	if (*pclsid != clsid)
	    return CLASS_E_CLASSNOTAVAILABLE;

	// Create new factory...	    
        ClassFactory_t* pCF = new ClassFactory_t;

	// Bump up CF ref count to 1
	pCF->AddRef ();

	// QI for requested interface - if successful it will leave
	// ref-count at 2, if not, then it will be 1 still...
	HRESULT hr = pCF->QueryInterface (iid, ppv);

	// release one reference - if QI failed this will destroy the
	// factory, but if successful will leave it around...
	pCF->Release ();
	
	return hr;
        }

    // classRegister - this function is used by the auto-registration
    // macro AUTOREGISTER_COCLASS to add the CLSID to the Registry
    static HRESULT classRegister 
        (
        VXDCOMPRIORITYSCHEME scheme, // priority scheme to use
        int priority		     // priority - not used by all schemes 
        )
	{
        int priorityToRegister;	// prio to register is determined by sheme

        switch (scheme)
            {
            case PS_DEFAULT:
                // Set priority to system default; ignore priority argument
                priorityToRegister = g_defaultServerPriority;
                break;
            
            case PS_SVR_ASSIGNED:
                // Set priority to given priority
                priorityToRegister = priority;
                break;

            case PS_CLNT_PROPAGATED:
                // Set priority to given priority for cases where the priority
                // is not provided by the client (as in Win32).  If the
                // provided priority is < 0, then use the default system
                // priority.
                if (priority < 0)
                    priorityToRegister = g_defaultServerPriority;
                else
                    {
                    if ((priority >=0) && (priority <=255))
                        priorityToRegister = priority;
                    else
                        priorityToRegister = g_defaultServerPriority;
                    }
                break;

            default:
                // Unknown or unsupported priority scheme.  Return an error.
                return E_UNEXPECTED;
                break;
            }

        // Register the structure against the CLSID
	return comClassRegister (*pclsid,
                                 CLSCTX_INPROC_SERVER,
                                 &classObjectGet,
                                 scheme,
                                 priorityToRegister);
        }

    };


//////////////////////////////////////////////////////////////////////////
//
// wotlQIHelper -- templated on the same implementation class as 
// CComObject, this function provides a type-safe way of invoking
// the class' _qi_impl() method.
//

template <class T>
HRESULT wotlQIHelper (T* pThis, REFIID riid, void** ppv)
    {
    return pThis->_qi_impl (riid, ppv);
    }

//////////////////////////////////////////////////////////////////////////
//
// CComObject - the main class for creating objects. The class 'T'
// must derive from CComObjectRoot (in order to gain an IUnknown
// implementation and other support functionality).
//

template <class T> class CComObject : public T
    {
  public:

    // CreateInstance() creates a single instance, no aggregation, no
    // specific COM interface...
    static HRESULT CreateInstance (CComObject<T>** pp);

    // CreateInstance - does the business of creating an instance of
    // the class, and finding the requested interface on it...
    static HRESULT CreateInstance
	(
	IUnknown*	punkOuter,
	REFIID		riid,
	void** 		ppv
	)
        {
	// Validate args
        if (!ppv)
            return E_POINTER;

	// Create an instance (refcount will be zero)
        CComObject* pObj = new CComObject;
	if (! pObj)
	    return E_OUTOFMEMORY;

	// preset return value in case we fail to QI
	*ppv = 0;

	// save aggregate IUnknown ptr
	pObj->m_pUnkOuter = punkOuter;

	// QI for desired interface - if it fails, we must get rid of
	// the recently-allocated instance before returning. If it
	// succeeds, returned interface-ptr will have one ref...
        HRESULT hr = pObj->_qi_impl (riid, ppv);
	if (FAILED (hr))
	    delete pObj;

	return hr;
        }
    
    // over-ride the base-class 'internal_qi' method so we can
    // implement QI via the COM_MAP macros...
    HRESULT internal_qi (REFIID riid, void** ppv)
	{
	return _qi_impl (riid, ppv);
	}

    // IUnknown implementation - these methods are used by each of the
    // multiply-inherited base interfaces as their own IUnknown
    // methods.
    
    ULONG STDMETHODCALLTYPE AddRef ()
	{
	if (m_pUnkOuter)
	    return m_pUnkOuter->AddRef ();
	return InternalAddRef ();
	}
    
    ULONG STDMETHODCALLTYPE Release ()
	{
	if (m_pUnkOuter)
	    return m_pUnkOuter->Release ();
        return InternalRelease ();
	}
    
    HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void ** ppv)
        {
	if (m_pUnkOuter)
	    return m_pUnkOuter->QueryInterface (riid, ppv);
	return wotlQIHelper<T> (this, riid, ppv);
	}

  private:
    static DWORD		m_dwRegToken;
    
    };

template <class T>
HRESULT CComObject<T>::CreateInstance (CComObject<T>** pp)
    {
    if (0 != pp)
        {
        *pp = new CComObject<T> ();
        if (0 != *pp)
            return S_OK;

        return E_OUTOFMEMORY;
        }

    return E_POINTER;
    }

template <class T> DWORD CComObject<T>::m_dwRegToken;

template <class T>
HRESULT CComClassFactory<T>::CreateInstance
    (
    IUnknown*		pUnkOuter,
    REFIID		riid,
    void**		ppv
    )
    {
    HRESULT		hr;
    static IUnknown*	m_punkTheInstance = 0;

    if (singleton_factory)
	{
	if (pUnkOuter)
	    return CLASS_E_NOAGGREGATION;
	
	if (! m_punkTheInstance)
	    {
	    hr = CComObject<T>::CreateInstance
		 (0, IID_IUnknown, (void**) &m_punkTheInstance);
	    if (FAILED (hr))
		return hr;
		// This is a singleton so we will never want to delete it, 
		// therefore we increment the ref count.
        AddRef ();
	    }

	return m_punkTheInstance->QueryInterface (riid, ppv);
	}
    
    return CComObject<T>::CreateInstance (pUnkOuter, riid, ppv);
    }


//////////////////////////////////////////////////////////////////////////
//
// CComPtr --  a smart-ptr class (like that of ATL) for holding COM
// interface pointers, and performing automatic ref-counting. We don't 
// provide the CComQIPtr class.
//

template <class Itf> class CComPtr
    {
    Itf* m_ptr;
    
  public:

    CComPtr () : m_ptr (0) {}
    
    CComPtr (Itf* p) : m_ptr (p)
	{
	if (m_ptr)
	    m_ptr->AddRef ();
	}
   
    CComPtr (const CComPtr<Itf>& sp) : m_ptr (sp.m_ptr)
	{
	if (m_ptr)
	    m_ptr->AddRef ();
	}
    
    ~CComPtr ()
	{
	if (m_ptr)
	    m_ptr->Release ();
	m_ptr = 0;
	}
    
    void Release ()
	{
	if (m_ptr)
	    m_ptr->Release ();
	m_ptr = 0;
	}
    
    operator Itf* () const
	{
	return m_ptr;
	}
    
    Itf** operator& ()
	{
	return &m_ptr;
	}
    
    Itf* operator-> ()
	{
	return m_ptr;
	}
    
    const Itf* operator-> () const
	{
	return m_ptr;
	}
    
    Itf* operator= (Itf* p)
	{
	if (p)
	    p->AddRef ();
	if (m_ptr)
	    m_ptr->Release ();
	m_ptr = p;
	return m_ptr;
	}
    
    Itf* operator= (const CComPtr<Itf>& sp)
	{
	if (sp.m_ptr)
	    sp.m_ptr->AddRef ();
	if (m_ptr)
	    m_ptr->Release ();
	m_ptr = sp.m_ptr;
	return m_ptr;
	}
    
    bool operator! () const
	{
	return (m_ptr == 0);
	}

    void Attach (Itf *p2)
        {
        if (0 != m_ptr)
            {
            m_ptr->Release ();
            }
        m_ptr = p2;
        }

    Itf *Detach()
        {
        Itf *pt = m_ptr;
        m_ptr = 0;
        return pt;
        }
    
    HRESULT CopyTo (Itf **ppT)
        {
        if (0 == ppT)
            {
            return E_POINTER;
            }
        *ppT = m_ptr;
        if (0 != p)
            {
            p->AddRef ();
            }
        return S_OK;
        }
    };

///////////////////////////////////////////////////////////////////////
//
// CComBSTR --  a class that wraps up the BSTR data type...
//

class CComBSTR
    {
    BSTR	m_str;
    
  public:
    CComBSTR () : m_str (0) {}

    explicit CComBSTR (int nSize, LPCOLESTR sz = 0)
	{
	m_str = ::SysAllocStringLen (sz, nSize);
	}

	CComBSTR (const char * pstr)
	{
	OLECHAR * wsz = new OLECHAR [(strlen (pstr) + 1) * 2];

	comAsciiToWide (wsz, pstr, strlen (pstr) + 1);

	m_str = ::SysAllocString (wsz);
	delete [] wsz;
	}

    explicit CComBSTR (LPCOLESTR psz)
	{
	m_str = ::SysAllocString (psz);
	}
    
    explicit CComBSTR (const CComBSTR& src)
	{
	m_str = src.Copy ();
	}
    
    CComBSTR& operator= (const CComBSTR& cbs);

    CComBSTR& operator= (LPCOLESTR pSrc);
    
    ~CComBSTR()
	{
	Empty ();
	}
    
    unsigned int Length () const
	{
	return ::SysStringLen (m_str);
	}
    
    operator BSTR () const
	{
	return m_str;
	}
    
    BSTR* operator& ()
	{
	return &m_str;
	}

    BSTR Copy() const
	{
	return ::SysAllocStringLen (m_str, ::SysStringLen (m_str));
	}
    
    void Attach (BSTR src)
	{
	m_str = src;
	}
    
    BSTR Detach ()
	{
	BSTR s = m_str;
	m_str = 0;
	return s;
	}
    
    void Empty ()
	{
	if (m_str)
	    ::SysFreeString (m_str);
	m_str = 0;
	}
    
    bool operator! () const
	{
	return (m_str == NULL);
	}

    void Append (const CComBSTR& cbs)
	{
	Append (cbs.m_str, ::SysStringLen (cbs.m_str));
	}
    
    void Append  (LPCOLESTR lpsz)
	{
	Append (lpsz, ::comWideStrLen (lpsz));
	}
    
    void AppendBSTR (BSTR bs)
	{
	Append (bs, ::SysStringLen (bs));
	}
    
    void Append (LPCOLESTR lpsz, int nLen);

    CComBSTR& operator+= (const CComBSTR& cbs)
	{
	AppendBSTR (cbs.m_str);
	return *this;
	}
    };

//////////////////////////////////////////////////////////////////////////
//
// CComVariant support
//


class CComVariant : public tagVARIANT
{
// Constructors
public:
    CComVariant()
	{
	::VariantInit(this);
	}
    ~CComVariant()
	{
	Clear();
	}

    CComVariant(const VARIANT& varSrc)
	{
	::VariantInit(this);
	InternalCopy(&varSrc);
	}

    CComVariant(const CComVariant& varSrc)
	{
        ::VariantInit(this);
        InternalCopy(&varSrc);
	}

    CComVariant(BSTR bstrSrc)
	{
        ::VariantInit(this);
        *this = bstrSrc;
	}

    CComVariant(LPCOLESTR lpszSrc)
	{
        ::VariantInit(this);
        *this = lpszSrc;
	}

    CComVariant(bool bSrc)
	{
        ::VariantInit(this);
        vt = VT_BOOL;
        boolVal = bSrc ? VARIANT_TRUE : VARIANT_FALSE;
	}

    CComVariant(int nSrc)
	{
        ::VariantInit(this);
        vt = VT_I4;
        lVal = nSrc;
	}

    CComVariant(BYTE nSrc)
	{
        ::VariantInit(this);
        vt = VT_UI1;
        bVal = nSrc;
	}

    CComVariant(short nSrc)
	{
        ::VariantInit(this);
        vt = VT_I2;
        iVal = nSrc;
	}

    CComVariant(long nSrc, VARTYPE vtSrc = VT_I4)
	{
        ::VariantInit(this);
        vt = vtSrc;
        lVal = nSrc;
	}

    CComVariant(float fltSrc)
	{
        ::VariantInit(this);
        vt = VT_R4;
        fltVal = fltSrc;
	}

    CComVariant(double dblSrc)
	{
        ::VariantInit(this);
        vt = VT_R8;
        dblVal = dblSrc;
	}

    CComVariant(CY cySrc)
	{
        ::VariantInit(this);
        vt = VT_CY;
        cyVal = cySrc;
	}

    CComVariant(IUnknown* pSrc)
	{
        ::VariantInit(this);
        vt = VT_UNKNOWN;
        punkVal = pSrc;
        if (punkVal != NULL)
            {
            punkVal->AddRef();
            }
	}

    CComVariant& operator=(const CComVariant& varSrc)
	{
        InternalCopy(&varSrc);
        return *this;
	}

    CComVariant& operator=(const VARIANT& varSrc)
	{
        InternalCopy(&varSrc);
        return *this;
	}

    CComVariant& operator=(BSTR bstrSrc);
    CComVariant& operator=(LPCOLESTR lpszSrc);

    CComVariant& operator=(bool bSrc);
    CComVariant& operator=(int nSrc);
    CComVariant& operator=(BYTE nSrc);
    CComVariant& operator=(short nSrc);
    CComVariant& operator=(long nSrc);
    CComVariant& operator=(float fltSrc);
    CComVariant& operator=(double dblSrc);
    CComVariant& operator=(CY cySrc);

    CComVariant& operator=(IUnknown* pSrc);

    bool operator==(const VARIANT& varSrc);
    bool operator!=(const VARIANT& varSrc) {return !operator==(varSrc);}

    HRESULT Clear() 
        { 
        return ::VariantClear(this); 
        }

    HRESULT Copy(const VARIANT* pSrc) 
        { 
        return ::VariantCopy(this, const_cast<VARIANT*>(pSrc)); 
        }

    HRESULT Attach(VARIANT* pSrc);
    HRESULT Detach(VARIANT* pDest);
    HRESULT ChangeType(VARTYPE vtNew, const VARIANT* pSrc = NULL);

    HRESULT InternalClear();
    void InternalCopy(const VARIANT* pSrc);
};


//////////////////////////////////////////////////////////////////////////
//
// Macros for making QueryInterface work.
//
// The COM_MAP macros define a function called _qi_impl() which does
// runtime casts to obtain the requested interface pointer.
//
// Other than that, the layout of the COM_MAP in the user's class is
// identical to an ATL map, but only allows for COM_INTERFACE_ENTRY
// and COM_INTERFACE_ENTRY_IID entries, not any of the other more
// fanciful types.
//
// The default IUnknown is always the first COM_INTERFACE_ENTRY in the
// table, so (by definition) this must be derived from IUnknown or the
// static_cast() will fail...
//

#define BEGIN_COM_MAP(cls) HRESULT _qi_impl (REFIID riid, void** ppv) { \
       COMTRACK_ADD_CLS(cls);

#define COM_INTERFACE_ENTRY(itf)                            \
    if (ppv == NULL)                                        \
	{COMTRACK_ADD_IID (itf);} else {		    \
    if ((IID_##itf == riid) || (IID_IUnknown == riid)) {    \
        *ppv = static_cast<itf*> (this);                    \
        InternalAddRef ();                                  \
        COMTRACK_UPDATE ();                                 \
        return S_OK; } }

#define COM_INTERFACE_ENTRY_IID(iid,itf)                    \
    if (ppv == NULL)                                        \
        {COMTRACK_ADD_IID (itf);} else {                    \
    if (riid == iid) {                                      \
        *ppv = static_cast<itf*> (this);                    \
        InternalAddRef ();                                  \
        COMTRACK_UPDATE ();                                 \
        return S_OK; }}
	
#define END_COM_MAP()                                       \
    if (ppv != 0) *ppv = 0;                                 \
    return E_NOINTERFACE; }

//////////////////////////////////////////////////////////////////////////
//
//        AUTOREGISTER_COCLASS(cls,name)
//
// This macro must be included in the object-implementation C++ source
// file after the object's class definition. It associates the C++
// class 'cls' with the CLSID and registers the module
// name in the VxRegistry, against the object's CLSID.
//
//////////////////////////////////////////////////////////////////////////

#define AUTOREGISTER_COCLASS(cls,priorityScheme, priority)	\
    struct cls ## _autoreg {                             	\
        cls ## _autoreg () {                             	\
            CComObject<cls>::classRegister (priorityScheme,	\
                                            priority); } };	\
    static cls ## _autoreg  __autoreg_ ## cls;


//////////////////////////////////////////////////////////////////////////
//
// BEGIN_COCLASS_TABLE, EXPORT_COCLASS, END_COCLASS_TABLE
//
// Macros for exporting coclasses from shared-libraries. They should
// be used like so:-
//
// BEGIN_COCLASS_TABLE
//   EXPORT_COCLASS(CoMyClass)
// END_COCLASS_TABLE
//
// where CoMyClass is a coclass (i.e. inherits CComCoClass) and thus
// has a static method 'classObjectGet' conforming to the typedef
// PFN_GETCLASSOBJECT as defined in comCoreLib.h
//
//////////////////////////////////////////////////////////////////////////

#define BEGIN_COCLASS_TABLE                                     \
extern "C" HRESULT DllGetClassObject                            \
    (REFCLSID clsid, REFIID iid, void** ppv) {

#define EXPORT_COCLASS(coclass)                                 \
    if (clsid == coclass::GetObjectCLSID ())                    \
        { return coclass::classObjectGet (clsid, iid, ppv); }

#define END_COCLASS_TABLE                                       \
    return CLASS_E_CLASSNOTAVAILABLE; }

#endif /* __INCcomObjLib_h__ */





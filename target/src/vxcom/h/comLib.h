/* comLib.h - VxWorks COM public API */

/* Copyright (c) 1998 Wind River Systems, Inc. */

/*

modification history
--------------------
02u,02jan02,nel  Fix alloca for diab build.
02t,10dec01,dbs  diab build
02s,08aug01,nel  Removed V_DECIMAL macros, not supported.
02r,16jul01,dbs  correct definition of comLibInit func
02q,13jul01,dbs  change vxcom back to vxidl
02p,28jun01,dbs  move VxMutex to private header
02o,27jun01,nel  Add extra SafeArray API.
02n,21jun01,dbs  move some definitions to comCoreLib
02m,20jun01,nel  Remove WindTypes and replace with vxcom.h.
02l,08feb01,nel  SPR#63885. SAFEARRAYs added. 
02k,30may00,nel  Add more variant support
02j,02feb00,dbs  add some Variant-related APIs
02i,22sep99,dbs  fix VARIANT type (moved to vxidl.idl)
02h,20sep99,dbs  move main typedefs into vxidl.idl/.h
02g,16sep99,dbs  add uchar typedef
02f,01sep99,dbs  add more IDL types
02e,18aug99,aim  fix GUID structure for SIMNT
02d,05aug99,dbs  add byte typedef
02c,30jul99,aim  changed mutex types to void*
02b,30jul99,dbs  fix build issues on SIMNT
02a,29jul99,dbs  add SIMNT support
01z,28jul99,drm  Changing g_defaultServerPriority to g_defaultServerPriority.
01y,27jul99,drm  Removing PS_CLNT_ASSIGNED enum entry.
01x,16jul99,dbs  reverse T2OLE expression, remove need for USES_CONVERSION
01w,10jun99,dbs  move vxdcom-private funcs out of here
01v,04jun99,dbs  remove public registry APIs
01u,03jun99,dbs  make mutex lock return void
01t,03jun99,dbs  fix long long type for W32
01s,02jun99,aim  #undef Free if it's defined
01r,02jun99,dbs  use new OS-specific macros
01q,20may99,dbs  add SysAllocStringByteLen() API
01p,07may99,dbs  add PROTSEQ typedef
01o,03may99,drm  adding priority scheme support
01n,28apr99,dbs  remove COM_NO_WINDOWS_H
01m,20apr99,dbs  add TLS defs for Win32, remove CoSetMalloc() API
01l,14apr99,dbs  fix alloca() definition for Win32
01k,14apr99,dbs  add definition for alloca() in gcc
01j,31mar99,dbs  added SysAllocStringLen API
01i,19mar99,drm  added CoCreateGuid() declaration
01h,01mar99,dbs  add GUID_NULL definition
01g,19feb99,dbs  add more wide-char support
01f,11feb99,dbs  add CoSetMalloc API
01e,04feb99,dbs  fix wide-char type for portability
01d,03feb99,dbs  use STDMETHOD macros
01c,20jan99,dbs  fix file names - vxcom becomes com
01b,08jan99,dbs  add TLS functions
01a,18dec98,dbs  created (from dcomLib.h)

*/

/*

 DESCRIPTION:

 This file defines a working subset of the COM API (as defined by
 Microsoft) for support of plain COM in VxWorks.

 A slight difference from the MS implementation is that
 CoCreateInstance() only works for CLSCTX_INPROC servers, and
 CoCreateInstanceEx() must be used for CLSCTX_REMOTE servers.

 Also, CoGetClassObject() is only available when DCOM is included, and
 not under plain VXCOM.
 
 */

#ifndef __INCcomLib_h
#define __INCcomLib_h

#include <string.h>
#include "vxidl.h"
#include "comCoreLib.h"
#include "semLib.h"

#ifdef __cplusplus
extern "C" {
#endif
    
extern int g_defaultServerPriority;

/*
 * The COM initialization type -- only COINIT_MULTITHREADED is
 * accepted by VxCOM / VxDCOM, the others are provided for Win32
 * compatibility.
*/
typedef enum
    {
    COINIT_APARTMENTTHREADED = 0x2,  // apartment model - not supported
    COINIT_MULTITHREADED     = 0x0,  // call objects on any thread.
    COINIT_DISABLE_OLE1DDE   = 0x4,  // [don't use DDE for Ole1 support]
    COINIT_SPEED_OVER_MEMORY = 0x8,  // [trade memory for speed?]
    } COINIT;


//////////////////////////////////////////////////////////////////////////
//
// Public API Functions - these mimic the Win32 CoXxxx API calls.
//
//////////////////////////////////////////////////////////////////////////

STATUS comLibInit (void);

HRESULT CoCreateInstance
    (
    REFCLSID		rclsid,		// CLSID of the object
    IUnknown*		pUnkOuter,	// pointer to aggregating object
    DWORD		dwClsContext,	// one of CLSCTX values
    REFIID		riid,		// IID of desired interface
    void**		ppv		// output interface ptr
    );

HRESULT CoInitialize (void*);

HRESULT CoInitializeEx (void*, DWORD);

void    CoUninitialize (void);

DWORD CoGetCurrentProcess (void);

void* CoTaskMemAlloc (ULONG cb); 

void* CoTaskMemRealloc (LPVOID pv, ULONG cb); 

void  CoTaskMemFree (LPVOID pv); 

HRESULT CoGetMalloc 
    (
    DWORD               dwMemContext,   // private or shared
    IMalloc**           ppMalloc        // output ptr
    );

HRESULT CoCreateGuid (GUID *pguid);

BSTR SysAllocString (const OLECHAR*);

BSTR SysAllocStringLen (const OLECHAR*, unsigned long);

BSTR SysAllocStringByteLen (const char*, unsigned long);

HRESULT SysFreeString (BSTR);

DWORD SysStringLen (BSTR);

DWORD SysStringByteLen (BSTR);

int StringFromGUID2
    (
    REFGUID		rguid,		// IID to be converted
    LPOLESTR		lpsz,		// resulting string
    int			cbMax		// max size of returned string
    );

HRESULT StringFromCLSID
    (
    REFCLSID		rclsid,		// CLSID to be converted
    LPOLESTR*		ppsz		// output var to receive string
    );

HRESULT StringFromIID
    (
    REFIID		riid,		// IID to be converted
    LPOLESTR*		ppsz		// output var to receive string
    );

HRESULT CLSIDFromString
    (
    LPCOLESTR		lpsz,		// string representation of CLSID
    LPCLSID		pclsid		// pointer to CLSID
    );

HRESULT IIDFromString
    (
    LPCOLESTR		lpsz,		// string representation of IID
    LPIID		piid		// pointer to IID
    );

BOOL IsEqualGUID 
    (
    REFGUID             guid1,
    REFGUID             guid2
    );

BOOL IsEqualCLSID 
    (
    REFCLSID            clsid1,
    REFCLSID            clsid2
    );

BOOL IsEqualIID 
    (
    REFIID              iid1,
    REFIID              iid2
    );

HRESULT WriteClassStm
    (
    IStream *           pStm,           // IStream to store in
    REFCLSID            rclsid          // CLSID to be stored in stream
    );

HRESULT ReadClassStm
    (
    IStream *           pStm,           // stream holding the CLSID
    CLSID *             pclsid          // output CLSID
    );

#ifndef V_VT

/* Variant access macros */

#define V_VT(X)         ((X)->vt)
#define V_ISARRAY(X)    (V_VT(X)&VT_ARRAY)

#define V_UI1(X)         ((X)->bVal)
#define V_I2(X)          ((X)->iVal)
#define V_I4(X)          ((X)->lVal)
#define V_R4(X)          ((X)->fltVal)
#define V_R8(X)          ((X)->dblVal)
#define V_CY(X)          ((X)->cyVal)
#define V_DATE(X)        ((X)->date)
#define V_BSTR(X)        ((X)->bstrVal)
#define V_ERROR(X)       ((X)->scode)
#define V_BOOL(X)        ((X)->boolVal)
#define V_UNKNOWN(X)     ((X)->punkVal)
#define V_ARRAY(X)       ((X)->parray)
#define V_VARIANT(X)	 ((X)->pvarVal)

#endif /* V_VT */

void VariantInit (VARIANT* v);

HRESULT VariantClear (VARIANT* v);

HRESULT VariantCopy (VARIANT* d, VARIANT* s);

HRESULT VariantChangeType (VARIANT * d, 
                                      VARIANT * s, 
                                      USHORT wFlags, 
                                      VARTYPE vt);

SAFEARRAY * SafeArrayCreate
    ( 
    VARTYPE             vt,
    UINT                cDims,
    SAFEARRAYBOUND *    rgsabound
    );

HRESULT SafeArrayDestroy
    (
    SAFEARRAY *         psa
    );


HRESULT SafeArrayLock (SAFEARRAY * psa);
HRESULT SafeArrayUnlock (SAFEARRAY * psa);


HRESULT SafeArrayPutElement
    (
    SAFEARRAY *     psa,
    long *          rgIndicies,
    void *          pv
    );

HRESULT SafeArrayGetElement
    (
    SAFEARRAY *     psa,
    long *          rgIndicies,
    void *          pv
    );

HRESULT SafeArrayAccessData
    ( 
    SAFEARRAY *  psa,       
    void **  ppvData  
    );

HRESULT SafeArrayUnaccessData
    ( 
    SAFEARRAY *  psa  
    );

HRESULT SafeArrayCopy
    (
    SAFEARRAY *		psa, 
    SAFEARRAY **	ppsaOut 
    );

HRESULT SafeArrayGetLBound
    (
    SAFEARRAY *		psa, 
    unsigned int	nDim, 
    long *		plLbound 
    );

HRESULT SafeArrayGetUBound
    (
    SAFEARRAY *		psa, 
    unsigned int	nDim, 
    long *		plUbound
    );
  
UINT SafeArrayGetDim
    ( 
    SAFEARRAY *		psa
    );
  
UINT SafeArrayGetElemsize
    (
    SAFEARRAY *		psa
    );

HRESULT SafeArrayGetVartype
    ( 
    SAFEARRAY *		psa, 
    VARTYPE *		pvt  
    );

const char* vxcomGUID2String (REFGUID guid);

HRESULT comStreamCreate
    (
    const void*		pMem,		// raw memory block
    unsigned long	len,		// length
    IStream**		ppStream	// output stream-ptr
    );

int comWideToAscii
    (
    char*		result,		// resulting ascii string
    const OLECHAR*	wstr,		// input wide-string
    int			maxLen		// max length to convert
    );


int comAsciiToWide
    (
    OLECHAR*		result,		// resulting wide string
    const char*		str,		// input string
    int			maxLen		// max length to convert
    );

size_t comWideStrLen
    (
    const OLECHAR*	wsz		// wide string
    );

OLECHAR* comWideStrCopy
    (
    OLECHAR*		wszDst,		// destination
    const OLECHAR*	wszSrc		// source
    );

HRESULT vxdcomClassRegister
    (
    REFCLSID		        clsid,		// key
    PFN_GETCLASSOBJECT          pFnGCO,         // ptr to GetClassObject() fn
    VXDCOMPRIORITYSCHEME        priorityScheme,	// priority scheme 
    int                         priority	// priority assoc. with scheme
    );

//////////////////////////////////////////////////////////////////////////
//
// Inline ASCII/WIDE conversion macros a la ATL. Unlike ATL, a
// function need not declare USES_CONVERSION at the top, but can
// freely use the macros to do inline conversion of wide-to-ascii
// (e.g. OLE2T(pwszSomeWideString)) or ascii-to-wide (e.g. T2OLE("some
// ASCII text")) on any architecture. Currently, OLECHAR != wchar_t on
// all VxWorks architectures, so use of Wide Literal Strings
// (e.g. L"some wide text") is not recommended.
//

#ifdef __GNUC__
# ifndef alloca
#  define alloca __builtin_alloca
# endif
#elif defined(__DCC__)
/* its a builtin for DCC */
#else
#include <malloc.h>
#endif

#ifndef USES_CONVERSION
#define USES_CONVERSION
#endif

OLECHAR* comT2OLEHelper (void*,const char*);
char*    comOLE2THelper (void*,const OLECHAR*);

#define T2OLE(psz) \
    ((psz) ? (comT2OLEHelper (alloca (sizeof (OLECHAR) * (strlen (psz) + 1)), \
			      psz)) : 0)

#define OLE2T(pwsz) \
    ((pwsz) ? (comOLE2THelper(alloca (comWideStrLen (pwsz) + 1), pwsz)) : 0)

#ifdef __cplusplus
}
#endif
    
#endif




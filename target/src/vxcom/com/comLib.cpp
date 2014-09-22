/* comLib.cpp - COM library (VxDCOM) */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
03f,03jan02,nel  Remove refs to T2OLE and OLE2T.
03e,17dec01,nel  Add include symbol for diab build.
03d,02nov01,nel  Fix docs build.
03c,19oct01,nel  SPR#71077. Fix error code returned by IIDFromString.
03b,19oct01,nel  SPR#71068. Fix StringFromCLSID parameter checking.
03a,17sep01,nel  SPR#70351. Check input parameters on StringFromGUID2.
02z,17sep01,nel  SPR#70350. Correct length count given by StringFromGUID2 to
                 include null terminator.
02y,13sep01,nel  SPR#70299. Add checking to VariantCopy on source parameter
                 for invalid VARIANT.
02x,12sep01,nel  SPR#70277. Fix parameter checking on CLSIDFromAscii.
02w,14aug01,nel  SPR#69615. CoCreateGUID check for NULL parameter.
02v,14aug01,nel  SPR#69609. SysAllocString should return NULL when passed a
                 NULL BSTR as input.
02u,08aug01,nel  SPR#69514. Correct SysAllocStringLen so that it doesn't add
                 two extra nulls.
02t,07aug01,dbs  use multi-qi in comCoreLib funcs
02s,06aug01,dbs  simplify vxcomGUID2String
02r,02aug01,nel  SPR#69373. Add extra checking to VariantInit and
                 VariantClear.
02q,30jul01,nel  SPR#69346. Add check to VariantClear for VT_IUNKNOWN.
02p,26jul01,nel  SPR#68698. Add numeric-to-string, string-to-numeric
                 capability to VariantChangeType.
02o,26jul01,nel  SPR#69289. Fix memory leak in SafeArrayDestroy.
02n,23jul01,nel  Correct return codes for CoCreateInstance docs.
02m,16jul01,dbs  correct definition of comLibInit func
02l,13jul01,dbs  fix up includes
02k,03jul01,nel  Remove IID_IMalloc because it's now defined in
                 comCoreTypes.idl
02j,27jun01,nel  Add in extra SAFEARRAY API.
02i,27jun01,dbs  fix include paths and names
02h,20jun01,nel  Remove dependencies on rpcDceTypes.h.
02g,02may01,nel  SPR#66227. Correct format of comments for refgen.
02f,08feb01,nel  SPR#63885. SAFEARRAYs added. 
02e,30may00,nel  Add more variant support
02d,06apr00,nel  use kernel level routine to access TCB structure for task
02c,01mar00,nel  Correct vxdcomGUID2String so that it can correctly convert
                 signed int to unsigned ints
02b,11feb00,dbs  IIDs moved to idl directory
02a,02feb00,dbs  add some Variant-related APIs
01z,07jan00,nel  Remove dependency on taskTcb to conform to new protection
                 scheme
01y,16nov99,nel  Corrected new name of pComLocal in WIND_TCB
01x,13oct99,dbs  add manpage/reference material
01w,19aug99,aim  change assert to VXDCOM_ASSERT
01v,13aug99,aim  moved globals to vxdcomGlobals.h
01u,28jul99,drm  Adding g_defaultServerPriority global.
01t,21jul99,dbs  fix string-length in OLE2T helper
01s,16jul99,aim  changed Free to Dealloc
01r,15jul99,dbs  add assertion-checking to T2OLE support macros
01q,17jun99,dbs  change argtypes of comMemAlloc
01p,04jun99,dbs  remove public registry APIs
01o,03jun99,dbs  fix CoCreateGuid() for non-VxWorks builds
01n,03jun99,dbs  remove reliance on TLS
01m,28may99,dbs  simplify allocator strategy
01l,28may99,aim  fixed scoping on array access in SysAllocString
01k,24may99,dbs  add BSTR marshaling policy variable
01j,20may99,dbs  add SysAllocStringByteLen() API
01i,10may99,drm  removed code in CoCreateGuid() which sets guid to GUID_NULL
01h,07may99,drm  documentation updates
02g,03may99,drm  adding preliminary priority scheme support
02f,29apr99,dbs  fix warnings under -Wall
02e,29apr99,dbs  must include MemPool.h
02d,28apr99,dbs  add more mem-alloc funcs
02c,27apr99,dbs  allocate all BSTRs from same pool
02b,27apr99,dbs  add alloc helper funcs
02a,26apr99,aim  added TRACE_CALL
01z,23apr99,dbs  use streams properly
01y,15apr99,dbs  improve hard-coded arrays
01x,14apr99,dbs  export registry-instance function to comShow
01w,14apr99,dbs  add return-value to GuidMap::erase()
01v,14apr99,dbs  use GuidMap's internal mutex
01u,14apr99,dbs  add check for T2OLE macro
01t,13apr99,dbs  add VxMutex into GuidMap class
01s,13apr99,dbs  replace std-lib with comUtilLib containers
01r,13apr99,dbs  fix Win32 compatibility in CoCreateGuid, fix indent
                 style, add semi-colons to VXDCOM_DEBUG usage
01q,08apr99,drm  adding diagnostic output using VXDCOM_DEBUG ;
01p,01apr99,dbs  add CComBSTR methods
01o,24mar99,drm  removing show routine
01n,19mar99,drm  added CoCreateGuid()
01m,09mar99,dbs  remove use of std::string
01l,03mar99,dbs  fix GuidMap to use correct find() method
01k,01mar99,dbs  add GUID_NULL definition
01j,19feb99,dbs  add more wide-char support
01i,11feb99,dbs  ensure all memory is allocated through task-allocator
01h,10feb99,dbs  tidy up per-task data items
01g,04feb99,dbs  change wchar_t to OLECHAR
01f,03feb99,dbs  add wide/ascii converters
01e,20jan99,dbs  fix lib-init function name
01d,20jan99,dbs  rename and move to main VOBs
01c,11jan99,dbs  reduce STL usage
01b,08jan99,dbs  Alloc registry from heap so it always works at
                 startup.
01a,18dec98,dbs  created

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comLib.h"
#include "private/comMisc.h"
#include "private/comSysLib.h"
#include "MemoryStream.h"
#include "TaskAllocator.h"
#include "TraceCall.h"
#include "comCoreLib.h"


/*

DESCRIPTION

This library provides a subset of the Win32 COM API calls, in order to
support the implementation of COM in the VxWorks environment.

*/

/* Include symbol for diab */

extern "C" int include_vxcom_comLib (void)
    {
    return 0;
    }

/* Well-known GUIDs that are defined by COM... */

const GUID GUID_NULL =
    {0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};

/**************************************************************************
*
* comLibInit - VxWorks COM library init function
*
* This function initialises the VxWorks enhanced COM library.
*
* RETURNS:
* \is
* \i OK
* On success
* \i ERROR
* If some failure occurs
* \ie
*/

int comLibInit ()
    {
    return 0;
    }

/**************************************************************************
*
* CoCreateInstance - create an in-process instance of an object class
*
* This function is used to create an instance of a local object (an
* <in-proc server>) under VxDCOM. The only valid value of
* 'dwClsContext' is CLSCTX_INPROC_SERVER.
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i E_NOINTERFACE
* If the class does not support the requested interface
* \i REG_E_CLASSNOTREG
* If 'dwClsContext' is invalid or the class isn't registered in the registry.
* \ie
*/

HRESULT CoCreateInstance
    (
    REFCLSID		rclsid,		/* CLSID of the object */
    IUnknown*		pUnkOuter,	/* pointer to aggregating object */
    DWORD		dwClsContext,	/* context */
    REFIID		riid,		/* IID of desired interface */
    void**		ppv		/* variable to receive interface ptr */
    )
    {
    if (ppv == NULL)
        return E_POINTER;

    MULTI_QI mqi[] = { { &riid, 0, S_OK } };

    // Create instance...
    HRESULT hr = comInstanceCreate (rclsid,
                                    pUnkOuter,
                                    dwClsContext,
                                    0,
                                    1,
                                    mqi);
    if (FAILED (hr))
        return hr;

    // Get resulting interface pointer out...
    if (ppv)
        *ppv = mqi[0].pItf;

    return mqi[0].hr;
    }


/**************************************************************************
*
* CoInitialize - initialize COM runtime support to APARTMENTTHREADED
*
* This function is provided for compatibility with the COM standard. Since
* VxDCOM does not support the apartment-threading model this function always 
* returns E_INVALIDARG.
*
* RETURNS: E_INVALIDARG under all conditions
*/

HRESULT CoInitialize 
    (
    void* 		pv		/* Reserved. Must be NULL */
    )
    {
    return CoInitializeEx (pv, COINIT_APARTMENTTHREADED);
    }

/**************************************************************************
*
* CoInitializeEx - initialize COM runtime support
*
* This function must be called with the 'dwCoInit' argument set to
* COINIT_MULTITHREADED, as there is no apartment-threading model in
* the VxWorks COM implementation.
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i E_INVALIDARG
* If the caller asks for apartment threading.
* \ie
*/

HRESULT CoInitializeEx 
    (
    void* 		pv, 		/* Reserved, must be NULL */
    DWORD 		dwCoInit	/* Must be CoINIT_MULTITHREADED */
    )
    {
    // Must be multi-threaded initialization...
    if (dwCoInit & COINIT_APARTMENTTHREADED)
	return E_INVALIDARG;

    return S_OK;
    }

/**************************************************************************
*
* CoUninitialize - un-initialize the COM runtime for this task.
*
* Provided for compatibility with COM standard only.
*
* RETURNS: None
*/

void CoUninitialize ()
    {
    }

/**************************************************************************
*
* CoGetCurrentProcess - Returns a value that is unique to the current thread.
*
* Returns a value that is unique to the current thread.
*
* RETURNS: A unique DWORD identifying the current thread.
*/

DWORD CoGetCurrentProcess ()
    {
    static DWORD s_nextTaskId = 0;
    
    if (comSysLocalGet () == 0)
        {
        comSysLocalSet (++s_nextTaskId);
        }
    return (DWORD) comSysLocalGet ();
    }

/**************************************************************************
*
* CLSIDFromAscii - Convert an ASCII string-format CLSID to a real CLSID structure.
*
* This function converts an ASCII string-format CLSID to a real CLSID 
* structure. The ASCII string must be of the following format
* {ABCDEFGH-IJKL-MNOP-QRST-UVWXYZ123456}.
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i CO_E_CLASSSTRING
* If the string is of the wrong format
* \i E_POINTER
* If a NULL pointer is given for a parameter.
* \ie
*/

HRESULT CLSIDFromAscii
    (
    const char*		s,		/* The string to convert. */
    LPCLSID             pClsid		/* The resultant CLSID structure. */
    )
    {
    TRACE_CALL;
    CLSID               clsid;
    char                cs [GUID_STRING_LEN];
    int                 i,j;

    if (s == NULL)
	{
	return E_POINTER;
	}

    if (pClsid == NULL)
	{
	return E_POINTER;
	}

    if (strlen (s) != 38)
	{
	return CO_E_CLASSSTRING;
	}

    if ((s [0] == '{') && (s [37] == '}'))
	strncpy (cs, s + 1, 36);
    else
	return CO_E_CLASSSTRING;

    // read string-format of CLSID into structure
    char * pCheck = NULL;
    cs [8] = 0;
    clsid.Data1 = strtoul (cs, &pCheck, 16);
    if (*pCheck)
	{
	return CO_E_CLASSSTRING;
	}

    cs [13] = 0;
    clsid.Data2 = strtoul (cs + 9, &pCheck, 16);
    if (*pCheck)
	{
	return CO_E_CLASSSTRING;
	}

    cs [18] = 0;
    clsid.Data3 = strtoul (cs + 14, &pCheck, 16);
    if (*pCheck)
	{
	return CO_E_CLASSSTRING;
	}

    for (i = 34, j = 7; i >= 24; i -= 2)
        {
        cs [i + 2] = 0;
        clsid.Data4 [j--] = strtoul (cs + i, &pCheck, 16);
	if (*pCheck)
	    {
	    return CO_E_CLASSSTRING;
	    }
        }

    for (i = 21; i >= 19; i -= 2)
        {
        cs [i + 2] = 0;
        clsid.Data4 [j--] = strtoul (cs + i, &pCheck, 16);
	if (*pCheck)
	    {
	    return CO_E_CLASSSTRING;
	    }
        }

    *pClsid = clsid;
    return S_OK;
    }

/**************************************************************************
*
* CoGetMalloc - Gets a pointer to the task-allocator for this task.
*
* Gets a pointer to the task-allocator for the current task, which is always
* the system allocator. If there is already one then it simply adds a
* reference to it, if not then it creates a new one.
*
* RETURNS: 
* \is
* \iE_INVALIDARG
* dwMemContext != 1.
* \iE_OUTOFMEMORY
* This isn't enough free memory to create the task-allocator.
* \iS_OK
* Success
* \ie
*/


HRESULT CoGetMalloc 
    (
    DWORD               dwMemContext,   /* Must be 1 */
    IMalloc**           ppMalloc        /* Interface pointer to task-allocator */
    )
    {
    return comCoGetMalloc (dwMemContext, ppMalloc);
    }

/**************************************************************************
*
* CoTaskMemAlloc - Allocates a block of memory for use by VxDCOM functions
*
* Allocates a block of memory for use by VxDCOM functions. The initial 
* contents of the memory block are undefined. CoGetMalloc need not be
* called prior to calling this function.
*
* RETURNS: pointer to block of memory or NULL on error.
*/

void* CoTaskMemAlloc 
    (
    ULONG 		cb		/* Size in bytes. */
    )
    {
    IMalloc*    pMalloc;
    void*       pv;

    if (FAILED (CoGetMalloc (1, &pMalloc)))
        return 0;

    pv = pMalloc->Alloc (cb);
    pMalloc->Release ();
    return pv;
    }

/**************************************************************************
*
* CoTaskMemRealloc - Changes the size of a memory block.
* 
* Changes the size of a memory block previously allocated using 
* CoTaskMemAlloc.
* 
* RETURNS: Pointer to reallocated memory block or NULL on error.
*/

void* CoTaskMemRealloc 
    (
    LPVOID		pv, 		/* Pointer to memory block. */
    ULONG 		cb		/* New size of memory block in bytes. */
    )
    {
    IMalloc*    pMalloc;
    void*       ptr;

    if (FAILED (CoGetMalloc (1, &pMalloc)))
        return 0;

    ptr = pMalloc->Realloc (pv, cb);
    pMalloc->Release ();
    return ptr;
    }

/* resolve clashes in networking headers */

#ifdef Free
#undef Free
#endif

/**************************************************************************
*
* CoTaskMemFree - Frees a block of task memory.
* 
* This function frees a block of memory previously allocated using 
* CoTaskMemAlloc.
*
* RETURNS: None.
*/

void CoTaskMemFree 
    (
    LPVOID 		pv		/* Pointer to memory block */
    )
    {
    IMalloc*    pMalloc;

    if (pv == 0)
        return;

    if (FAILED (CoGetMalloc (1, &pMalloc)))
        return;

    pMalloc->Free (pv);
    pMalloc->Release ();
    }


/**************************************************************************
*
* CoCreateGuid - create a GUID
*
* This routine creates a new GUID and copies it into the GUID pointed to
* by the <pGuid> argument if successful.  If the routine encounters an
* error creating the GUID, the GUID pointed to by <pGuid> will be left
* unchanged.
*
* RETURNS:
* \is
* \i S_OK
* If successful
* \i RPC_S_UUID_NO_ADDRESS
* If a hardware ethernet address cannot be obtained
* \i E_POINTER
* If pGuid is NULL
* \i RPC_S_INTERNAL_ERROR
* For all other errors.
* \ie
*/

HRESULT CoCreateGuid 
    (
    GUID*		pGuid		/* ptr to GUID where result will be returned */
    )
    {
    if (pGuid == NULL)
        {
        return E_POINTER;
        }
    comSysGuidCreate (pGuid);
    return S_OK;
    }

/**************************************************************************
*
* WriteClassStm - Writes a CLSID to a stream.
*
* This function writes a CLSID to a stream.
*
* RETURNS:
* \is
* \i S_OK
* On Success
* \i E_FAIL
* On failure
* \ie
*/

HRESULT WriteClassStm
    (
    IStream *           pStm,           /* IStream to store in. */
    REFCLSID            rclsid          /* CLSID to be stored in stream */
    )
    {
    TRACE_CALL;
    ULONG               nb;

    return pStm->Write (&rclsid, sizeof (CLSID), &nb);
    }

/**************************************************************************
*
* ReadClassStm - Reads a CLSID from a stream.
*
* This function reads a CLSID from a stream.
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i E_FAIL
* On failure
* \ie
*/

HRESULT ReadClassStm
    (
    IStream *           pStm,           /* stream holding the CLSID */
    CLSID *             pclsid          /* output CLSID */
    )
    {
    TRACE_CALL;
    ULONG               nb;

    return pStm->Read (pclsid, sizeof (CLSID), &nb);
    }

/**************************************************************************
*
* CLSIDFromString - Convert a wide string-format CLSID to a CLSID structure.
*
* This function converts a wide string-format CLSID to a CLSID structure.
* The string must be of the following format 
* {ABCDEFGH-IJKL-MNOP-QRST-UVWXYZ123456}.
*
* This function differs from the standard OLE function in that it doesn't
* check that the CLSID is in the registry.
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i CO_E_CLASSSTRING
* If the string is in the wrong format
* \i E_POINTER
* If a NULL pointer is given for a parameter.
* \ie
*/

HRESULT CLSIDFromString 
    (
    LPCOLESTR		wszClsid,	/* Wide string to convert */
    LPCLSID             pClsid		/* Pointer to CLSID which will contain result */
    )
    {
    TRACE_CALL;
    char                s [GUID_STRING_LEN];

    if (wszClsid == NULL)
	{
	return E_POINTER;
	}

    // convert to ascii from wide-string
    vxcom_wcstombs (s, wszClsid, sizeof (s));

    // use utility function to get conversion
    return CLSIDFromAscii (s, pClsid);
    }

/**************************************************************************
*
* vxcomGUID2String - Helper function that converts a GUID to string format.
*
* Helper function that converts a GUID to string format.
*
* RETURNS: The converted GUID as an ASCII string.
*
*.NOMANUAL
*/

const char * vxcomGUID2String
    (
    REFGUID		guid		/* GUID to convert */
    )
    {
    static char s [GUID_STRING_LEN];

    comCoreGUID2String (&guid, s);
    
    return s;
    }

/**************************************************************************
*
* StringFromGUID2 - basic GUID -> string conversion utility.
*
* This function converts a GUID structure into a wide string representation.
* lpsz must point to a valid wide string buffer allocated by CoTaskMemAlloc.
*
* RETURNS: The length of the resultant string or 0 is the buffer is to small 
* to hold the resultant string.
*/

int StringFromGUID2
    (
    REFGUID		rguid,		/* IID to be converted */
    LPOLESTR		lpsz,		/* resulting string */
    int			cbMax		/* max size of returned string */
    )
    {
    TRACE_CALL;
    char		sGuid [GUID_STRING_LEN];

    if (lpsz == NULL)
	return 0;

    strcpy (sGuid, (vxcomGUID2String (rguid)));

    // Check that result will fit.
    if (cbMax < (strlen (sGuid) + 1))
	return 0;

    // Trim to match available output space
    if ((size_t)cbMax < sizeof (sGuid))
	sGuid [cbMax] = 0;

    // Convert to wide-char...
    vxcom_mbstowcs (lpsz, sGuid, strlen (sGuid) + 1);

    // Return num chars in string...
    return strlen (sGuid) + 1;
    }

/**************************************************************************
*
* StringFromCLSID - converts a CLSID to a wide-string format.
* 
* This function converts a CLSID to a wide-string format. This routine 
* allocates the correct amount of memory and returns it via ppsz. This
* memory must be released using CoTaskMemFree.
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i E_OUTOFMEMORY
* If not enough memory could be allocated to hold the string.
* \ie
*/

HRESULT StringFromCLSID
    (
    REFCLSID		rclsid,		/* CLSID to be converted */
    LPOLESTR*		ppsz		/* output var to receive string */
    )
    {
    TRACE_CALL;

    /* Check for NULL parameter */
    if (ppsz == NULL)
	return E_POINTER;

    *ppsz = (LPOLESTR) CoTaskMemAlloc (GUID_STRING_LEN * sizeof (OLECHAR));
    if (*ppsz == NULL)
        {
        return E_OUTOFMEMORY;
        }
    StringFromGUID2 (rclsid, *ppsz, GUID_STRING_LEN);
    return S_OK;
    }

/**************************************************************************
*
* StringFromIID - converts a IID to a wide-string format.
* 
* This function converts a IID to a wide-string format. This routine 
* allocates the correct amount of memory and returns it via ppsz. This
* memory must be released using CoTaskMemFree.
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i E_OUTOFMEMORY
* If not enough memory could be allocated to hold the string.
* \ie
*/

HRESULT StringFromIID
    (
    REFIID		riid,		/* IID to be converted */
    LPOLESTR*		ppsz		/* output var to receive string */
    )
    {
    TRACE_CALL;
    *ppsz = (LPOLESTR) CoTaskMemAlloc (GUID_STRING_LEN * sizeof (OLECHAR));
    if (*ppsz == NULL)
        {
        return E_OUTOFMEMORY;
        }
    StringFromGUID2 (riid, *ppsz, GUID_STRING_LEN);
    return S_OK;
    }

/**************************************************************************
*
* IIDFromString - Convert a wide string-format IID to a IID structure.
*
* This function converts a wide string-format IID to a IID structure.
* The string must be of the following format 
* {ABCDEFGH-IJKL-MNOP-QRST-UVWXYZ123456}.
*
* RETURNS:
* \is
* \i S_OK
* On success.
* \i E_INVALIDARG
* If the string is in the wrong format or the return pointer is invalid.
* \ie
*/

HRESULT IIDFromString
    (
    LPCOLESTR		lpsz,		/* string representation of IID */
    LPIID		piid		/* pointer to IID */
    )
    {
    TRACE_CALL;
    HRESULT	hr;

    hr = CLSIDFromString (lpsz, piid);
    if (hr == CO_E_CLASSSTRING)
	return E_INVALIDARG;
    return hr;
    }

/**************************************************************************
*
* IsEqualGUID - Tests if two GUIDs are equivalent.
*
* This function tests if two GUIDs are equivalent.
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i CO_E_CLASSSTRING
* If the string is in the wrong format.
* \ie
*/

BOOL IsEqualGUID 
    (
    REFGUID             guid1,		/* GUID to compare to guid2 */
    REFGUID             guid2		/* GUID to compare to guid1 */
    )
    {
    TRACE_CALL;
    return (guid1 == guid2) ? TRUE : FALSE;
    }

/**************************************************************************
*
* IsEqualCLSID - Tests if two CLSIDs are equivalent.
*
* This function tests if two CLSIDs are equivalent.
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i CO_E_CLASSSTRING
* If the string is in the wrong format.
* \ie
*/

BOOL IsEqualCLSID 
    (
    REFCLSID            clsid1,		/* CLSID to compare to clsid2 */
    REFCLSID            clsid2		/* CLSID to compare to clsid1 */
    )
    {
    TRACE_CALL;
    return IsEqualGUID (clsid1, clsid2);
    }

/**************************************************************************
*
* IsEqualIID - Tests if two IIDs are equivalent.
*
* This function tests if two IIDs are equivalent.
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i CO_E_CLASSSTRING
* If the string is in the wrong format.
* \ie
*/

BOOL IsEqualIID 
    (
    REFIID              iid1,		/* IID to compare to iid2 */
    REFIID              iid2		/* IID to compare to iid1 */
    )
    {
    TRACE_CALL;
    return IsEqualGUID (iid1, iid2);
    }

/*
*
* This section implements the BSTR data type, named after the 'Basic
* String' type from MSVisual Basic. It is a counted wide-string, but
* with the byte-count stored before the character array, and the
* address of a BSTR is considered to be the address of the character
* array. Hence, the count-word is 'in front of' the pointer that
* points to it:-
*
* DWORD wchar wchar wchar wchar wchar
*       ^
*       BSTR points here...
*
*/

/**************************************************************************
*
* SysAllocString - Allocates a new string and copies the existing string into it.
*
* This function allocates a new string and copies the existing string to
* it.
*
* RETURNS: A pointer to the new string or NULL if memory can't be 
* allocated or psz was NULL.
*/

BSTR SysAllocString 
    (
    const OLECHAR* 	psz		/* String to copy into new string */
    )
    {
    if (psz == NULL)
        return NULL;

    // Count the characters, not incl terminating NULL...
    const OLECHAR* pChar = psz;
    size_t len=0;
    while (*pChar++)
	++len;

    // Get a string
    BSTR bstr = SysAllocStringByteLen (0, (len+1) * sizeof (OLECHAR));
    
    // Copy the string
    size_t n;

    for (n=0; n < len; ++n)
	bstr [n] = psz [n];
    bstr [n] = 0;

    return bstr;
    }

/**************************************************************************
*
* SysAllocStringLen - Create a string of the given length and initialize it from the passed string.
*
* Public API for creating a BSTR of the given length. After allocation, 
* 'nLen' characters are copied from 'psz' and a NULL character is appended.
* If 'psz' includes NULL characters they will be copied also.
*
* RETURNS: A pointer to the new string or NULL if memory can't be allocated.
*/

BSTR SysAllocStringLen 
    (
    const OLECHAR* 	psz, 		/* String to initialize new string from */
    unsigned long 	nLen		/* Length of new string */
    )
    {
    // Get a string
    BSTR bstr = SysAllocStringByteLen (0, nLen * sizeof (OLECHAR));
    
    // Copy the string, if supplied...
    if (psz)
	{
	for (size_t n=0; n < nLen; ++n)
	    bstr [n] = psz [n];
	}
    bstr [nLen] = 0;

    return bstr;
    }

/**************************************************************************
*
* SysAllocStringByteLen - Create a string of the given length and initialize it from the passed string.
*
* Public API for creating a BSTR containing 8-bit data, of the given length. 
* The input arg 'psz' may be NULL in which case the resulting BSTR has 
* uninitialized data. The argument 'bytes' indicates the number of bytes 
* to copy from 'psz'. Two NULL characters, or a single NULL OLECHAR, are 
* placed afterwards, allocating a total of (bytes + 2) bytes.
*
* RETURNS: A pointer to the new string or NULL if memory can't be allocated.
*/

BSTR SysAllocStringByteLen 
    (
    const char* 	psz, 		/* String to initialize new string from */
    unsigned long 	nBytes		/* number of bytes in new string */
    )
    {
    // Allocate the string
    void* pMem = comSysAlloc (sizeof (DWORD) + nBytes + sizeof (OLECHAR));
    if (! pMem)
	return 0;

    * ((DWORD*) pMem) = nBytes;
    BSTR bstr = (BSTR) (((DWORD*) pMem) + 1);
    if (psz)
	{
	memcpy (bstr, psz, nBytes);
	bstr [nBytes / sizeof (OLECHAR)] = 0;
	}
    return bstr;
    }

/**************************************************************************
*
* SysFreeString - Releases the memory allocated to a BSTR.
*
* Public API for freeing BSTRs. The input 'bstr' may be NULL.
*
* RETURNS: S_OK.
*/

HRESULT SysFreeString 
    (
    BSTR 		bstr		/* String to release */
    )
    {
    if (bstr)
	comSysFree (((DWORD*)bstr) - 1);
    return S_OK;
    }

/**************************************************************************
*
* SysStringByteLen - Calculates the number of bytes in a BSTR.
*
* Public API for finding the number of bytes in a BSTR (not the number of 
* wide-characters).
*
* RETURNS: Number of bytes in a BSTR, or 0 if the BSTR is NULL.
*/

DWORD SysStringByteLen 
    (
    BSTR 		bstr		/* String to calculate size of */
    )
    {
    if (bstr == NULL)
        return 0;

    return * (((DWORD*)bstr) - 1);
    }

/**************************************************************************
*
* SysStringLen - Calculate length of BSTR
*
* Public API for finding length of a BSTR. Note that this may differ from 
* the value returned by comWideStrLen() or wcslen() if the originating 
* string contained embedded NULLs.
*
* RETURNS: Number of OLECHAR in the string.
*/

DWORD SysStringLen 
    (
    BSTR 		bstr		/* String to calculate length of */
    )
    {
    return SysStringByteLen (bstr) / 2;
    }

/**************************************************************************
*
* comStreamCreate - creates a memory-based IStream from raw memory
*
* This function creates an in-memory stream, as an <IStream>
* implementation, based on the contents of the initial
* memory-block. The stream allocates a new block to hold the contents,
* and any changes occur inside the newly-allocated block, not inside
* the original block. Its contents can be accessed by rewinding the
* stream, finding its length (using SEEK_END) and then Read()ing the
* entire stream.
*
* On entry, the arguments 'pMem' and 'len' should describe a block of
* memory which is to be copied into the new stream object. The
* argument 'ppStream' should point to an 'IStream*' variable which
* will be set to point to the resulting stream object upon successful
* completion of the function.
*
* RETURNS:
* \is
* \i S_OK
* On Success
* \i E_OUTOFMEMORY if it is unable to allocate sufficient memory.
* \ie
*/

HRESULT comStreamCreate
    (
    const void*		pMem,		/* address of initial memory */
    unsigned long	len,		/* length of initial memory */
    IStream**		ppStream	/* address of output variable */
    )
    {
    TRACE_CALL;
    // Create a memory-stream object (will have 1 ref if successful)
    ISimpleStream* pStrm;
    HRESULT hr = VxRWMemStream::CreateInstance (0,
					        IID_ISimpleStream,
						(void**) &pStrm);
    if (FAILED (hr))
	return hr;

    // Write data into stream, then rewind it...
    pStrm->insert (pMem, len);
    pStrm->locationSet (0);

    // Hand out a single reference to its IStream interface
    hr = pStrm->QueryInterface (IID_IStream, (void**) ppStream);
    
    // If QI was successful, this will leave the caller with the
    // only ref, if not it will destroy the object...
    pStrm->Release ();
    return hr;
    }

/**************************************************************************
*
* comWideToAscii - convert a wide-string to a narrow (ascii) string
*
* This function simply converts a string consisting of 2-byte
* characters to its equivalent ASCII string. No character-set
* conversions are performed, the character values are simply narrowed
* to 8 bits. On entry, the arguments 'result' and 'maxLen' describe
* the output buffer where the resulting 8-bit string will be placed,
* and 'wstr' points to a NULL-terminated wide-character string to be
* converted.
*
* RETURNS: the number of characters converted.
*
*/

int comWideToAscii
    (
    char*		result,		/* resulting ascii string */
    const OLECHAR*	wstr,		/* input wide-string */
    int			maxLen		/* max length to convert */
    )
    {
    TRACE_CALL;
    return vxcom_wcstombs (result, wstr, maxLen);
    }


/**************************************************************************
*
* comAsciiToWide - convert a narrow (ascii) string to a wide-string
*
* This function simply converts a string consisting of single-byte
* ASCII characters to its equivalent wide-string. No character-set
* conversions are performed, the character values are simply widened
* to 16 bits. On entry, the arguments 'result' and 'maxLen' describe
* the output buffer where the resulting double-byte string will be
* placed, and 'str' points to a NULL-terminated string to be converted.
*
* RETURNS: the number of characters converted.
*
*/

int comAsciiToWide
    (
    OLECHAR*		result,		/* resulting wide string */
    const char*		str,		/* input string */
    int			maxLen		/* max length to convert */
    )
    {
    TRACE_CALL;
    return vxcom_mbstowcs (result, str, maxLen);
    }


/**************************************************************************
*
* comWideStrLen - find the length of a wide-string
*
* This function returns the length of the NULL-terminated
* wide-character string at 'wsz'.
*
* RETURNS: the number of characters in the string.
*
*/

size_t comWideStrLen
    (
    const OLECHAR*	wsz		/* wide string */
    )
    {
    TRACE_CALL;
    return vxcom_wcslen (wsz);
    }

/**************************************************************************
*
* comWideStrCopy - make a copy of a wide-string
*
* This function copies the NULL-terminated wide-character string at
* 'wszSrc' to the location pointed to by 'wszDst' which must contain
* enough space to hold the resulting string.
*
* RETURNS: the address of the copy of the wide-string
*
*/

OLECHAR* comWideStrCopy
    (
    OLECHAR*		wszDst,		/* destination */
    const OLECHAR*	wszSrc		/* source */
    )
    {
    TRACE_CALL;
    return vxcom_wcscpy (wszDst, wszSrc);
    }

/**************************************************************************
*
* comT2OLEHelper - Helper function for T2OLE macro.
*
* This function is used internaly by the T2OLE macro to convert the
* ASCII string into a wide string.
*
* RETURNS: The address of the wide string.
*
*.NOMANUAL
*/

OLECHAR * comT2OLEHelper
    (
    void *		pvWSZ,		/* pointer to wide string */
    const char *	psz		/* pointer to ASCII string */
    )
    {
    OLECHAR*	pwsz = (OLECHAR*) pvWSZ;

    COM_ASSERT(pvWSZ);
    comAsciiToWide (pwsz, psz, strlen (psz) + 1);
    return pwsz;
    }

/**************************************************************************
*
* comOLE2THelper - Helper function for OLE2T macro.
*
* This function is used internaly by the OLE2T macro to convert the
* wide string into an ASCII string.
*
* RETURNS: The address of the ASCII string.
*
*.NOMANUAL
*/

char * comOLE2THelper
    (
    void *		pvSZ,		/* pointer to ASCII string */
    const OLECHAR *	pwsz		/* pointer to wide string. */
    )
    {
    char*	psz = (char*) pvSZ;

    COM_ASSERT(pvSZ);
    comWideToAscii (psz, pwsz, vxcom_wcslen (pwsz) + 1);
    return psz;
    }

/* CComBSTR methods -- most are inline, but some are defined here... */

CComBSTR& CComBSTR::operator= (const CComBSTR& src)
    {
    TRACE_CALL;
    if (m_str != src.m_str)
	{
	if (m_str)
	    ::SysFreeString (m_str);
	m_str = src.Copy ();
	}
    return *this;
    }

CComBSTR& CComBSTR::operator= (LPCOLESTR pwsz)
    {
    TRACE_CALL;
    ::SysFreeString (m_str);
    m_str = ::SysAllocString (pwsz);
    return *this;
    }

void CComBSTR::Append (LPCOLESTR pwsz, int nLen)
    {
    TRACE_CALL;
    int myLen = Length ();
    BSTR bs = ::SysAllocStringLen (0, myLen + nLen);
    memcpy (bs, m_str, myLen * sizeof (OLECHAR));
    memcpy (bs + myLen, pwsz, nLen * sizeof (OLECHAR));
    bs [myLen + nLen] = 0;
    ::SysFreeString (m_str);
    m_str = bs;
    }

/* VARIANT-related APIs. */

/**************************************************************************
*
* VariantInit - Initialize a VARIANT.
*
* Initialize a VARIANT, regardless of its current contents, to be VT_EMPTY.
*
* RETURNS: void
*
*/

void VariantInit (VARIANT* v)
    {
    if (v == NULL) return;
    memset (v, 0, sizeof (VARIANT));
    v->vt = VT_EMPTY;
    }


/**************************************************************************
*
* VariantClear - clear the contents of a VARIANT.
*
* Clear the contents of a VARIANT back to VT_EMPTY, but taking account of 
* the current contents value. So, if the VARIANT is currently representing 
* a IUnknown pointer, that pointer will be released in this function. 
* Also, BSTRs and SAFEARRAY are freed.
*
* RETURNS:
* \is
* \i S_OK
* On Success.
* \i E_INVALIDARG
* If v is null or otherwise invalid.
* \i DISP_E_BARVARTYPE
* The VARIANT is not one of the following allowed types: VT_EMPTY, VT_NULL, 
* VT_I2, VT_I4, VT_R4, VT_R8, VT_CY, VT_DATE, VT_BSTR, VT_ERROR, VT_BOOL, 
* VT_UNKNOWN, VT_UI1.
* \ie
*/

HRESULT VariantClear (VARIANT* v)
    {
    HRESULT hr = S_OK;

    // check for NULL variant
    if (v == NULL)
        return E_INVALIDARG;

    // check for types of VARIANTs we can handle
    // and mask out ARRAY bit.
    switch (V_VT(v) & 0xDFFF)
        { 
        case VT_EMPTY:
        case VT_NULL:
        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_R8:
        case VT_CY:
        case VT_DATE:
        case VT_BSTR:
        case VT_ERROR:
        case VT_BOOL:
        case VT_UNKNOWN:
        case VT_UI1:
            break;

        default:
            return DISP_E_BADVARTYPE;
        }

    if (V_ISARRAY (v))
        {
        hr = SafeArrayDestroy (V_ARRAY(v));
        if (SUCCEEDED (hr))
            {
            VariantInit (v);
            }
        }
    else
        {
        switch (V_VT(v))
            {
            case VT_BSTR:
                hr = SysFreeString (V_BSTR(v));
                break;

            case VT_UNKNOWN:
		if (V_UNKNOWN(v))
		    V_UNKNOWN(v)->Release ();
                break;
            }
        VariantInit (v);
        }


    return hr;
    }


/**************************************************************************
*
* VariantCopy - Copy a VARIANT. 
*
* Frees the destination variant and makes a copy of the source variant.
* 
* If s is a BSTR a copy of the BSTR is made.
*
* Copying SAFEARRAYs isn't supported so E_INVALIDARG is returned if this 
* is attempted.
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i E_INVALIDARG
* If source and destination are the same.
* \i E_POINTER
* One of the VARIANTs is NULL or invalid.
* \i E_OUTOFMEMORY
* If there isn't enough memory to allocate the new VARIANT.
* \ie
*/

HRESULT VariantCopy 
    (
    VARIANT *           d,  /* Pointer to the VARIANT to receive the copy */
    VARIANT *           s   /* Pointer to the VARIANT to be copied */
    )
    {
    HRESULT hr = S_OK;

    if (s == NULL)
	{
	return E_POINTER;
	}

    if (d == NULL)
	{
	return E_POINTER;
	}

    if (V_ISARRAY (s))
        {
        hr = VariantClear (d);
        if (FAILED (hr)) return hr;
        if (FAILED (hr)) 
            {
            return hr;
            }

    	/* SPR#67806. set VT in dest. */
    	V_VT(d) = V_VT(s);

    	return SafeArrayCopy (V_ARRAY(s), &(V_ARRAY(d)));

        }

    if (d != s)
        {
    	hr = VariantClear (d);

    	/* SPR#67806. set VT in dest. */
    	V_VT(d) = V_VT(s);

    	if (SUCCEEDED (hr))
    	    {
            memcpy (d, s, sizeof(VARIANT));
            switch (V_VT (s))
                {
                case VT_BSTR:
                    V_BSTR(d) =  SysAllocString (V_BSTR(s));       
                    /* Check to see if the Alloc failed */
                    if ((V_BSTR (s) != NULL) && (V_BSTR (d) == NULL))
                        {
                        hr = E_OUTOFMEMORY;
                        }
                    break;

                case VT_UNKNOWN:
                    if (V_UNKNOWN(s))
                        {
                        hr = V_UNKNOWN(s)->QueryInterface(IID_IUnknown, 
                                                        (void **)&V_UNKNOWN(d));
                        }
                    break;
                    
                }
            }
        }
     else
        {
        hr = E_INVALIDARG;
        }

    return hr;
    }

/**************************************************************************
*
* GetVariantValue - Get certain VARIANT values as doubles.
*
* This function converts certain VARIANT values into doubles.
*
* RETURNS: The value of the VARIANT as a double or zero.
*.NOMANUAL
*/

static double GetVariantValue
    (
    VARIANT *   v
    )
    {
    switch (V_VT(v))
        {
        case VT_BOOL:   return (double) V_BOOL(v);
        case VT_UI1:    return (double) V_UI1(v);
        case VT_I2:     return (double) V_I2(v);
        case VT_I4:     return (double) V_I4(v);
        case VT_R4:     return (double) V_R4(v);
	case VT_R8:     return (double) V_R8(v);
	case VT_BSTR:	
	    {
	    double	result;
	    char *	str = new char [comWideStrLen (V_BSTR (v)) + 1];

	    if (str == 0) return 0;

	    comWideToAscii (str, V_BSTR(v), comWideStrLen (V_BSTR (v)) + 1);
	    result = atof (str);
	    delete [] str;
	    return result;
	    }
        }
    
    return 0;
    }

/**************************************************************************
*
* VariantChangeType - Converts a variant from one type to another.
*
* This function converts one VARIANT of a specified type to another
* VARIANT type. s and d must point to valid VARIANTs. 
*
* Only conversion between the following types is supported: VT_BOOL, 
* VT_UI1, VT_I2, VT_I4, VT_R4, VT_R8, VT_BSTR.
* 
* The wFlags parameter is ignored.
*
* RETURNS:
* \is
* \i S_OK
* On Success
* \i E_INVALIDARG
* If source or destination are not valid VARIANTS.
* \i DISP_E_BADVARTYPE
* The Variant isn't on of the following types: VT_BOOL, VT_UI1, VT_I2,
* VT_I4, VT_R4, VT_R8, VT_BSTR.
* \ie
*/

HRESULT VariantChangeType 
    (
    VARIANT *   d,      /* VARIANT to receive changed VARIANT */
    VARIANT *   s,      /* VARIANT that holds existing data */
    USHORT      wFlags, /* Ignored */
    VARTYPE     vt      /* VARIANT type to change to */
    )
    {
    HRESULT hr = S_OK;
    VARIANT tmp;

    /* check for NULL VARIANTs */
    if ((d == NULL) || (s == NULL))
        return E_INVALIDARG;

    /* Can only convert from some VT types */
    switch (V_VT(s))
	{
        case VT_BOOL:
        case VT_UI1:
        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_R8:
        case VT_BSTR:
            break;

        default:
            return DISP_E_BADVARTYPE;
	}

    /* SPR#67805. To allow inline coersion use a tmp VARIANT to store source */
    /* VARIANT so if it is inline it doesn't get errased. */
    VariantInit (&tmp);
    VariantCopy (&tmp, s);

    hr = VariantClear (d);

    if (SUCCEEDED (hr))
        {
        switch (vt)
            {
            case VT_BOOL:
                V_VT(d) = VT_BOOL;
                if (GetVariantValue (&tmp) == 0)
                    {
                    V_BOOL (d) = VARIANT_FALSE;
                    }
                else
                    {
                    V_BOOL (d) = VARIANT_TRUE;
                    }
                break;

            case VT_UI1:
                V_VT(d) = VT_UI1;
                V_UI1(d) = (BYTE)GetVariantValue (&tmp);
                break;

            case VT_I2:
                V_VT (d) = VT_I2;
                V_I2 (d) = (SHORT)GetVariantValue (&tmp);
                break;

            case VT_I4:
                V_VT (d) = VT_I4;
                V_I4 (d) = (LONG)GetVariantValue (&tmp);
                break;

            case VT_R4:
                V_VT (d) = VT_R4;
                V_R4 (d) = GetVariantValue(&tmp);
                break;

            case VT_R8:
                V_VT (d) = VT_R8;
                V_R8 (d) = GetVariantValue (&tmp);
                break;

	    case VT_BSTR:
	    	V_VT (d) = VT_BSTR;
	    	switch (V_VT (&tmp))
	    	    {
	    	    case VT_BSTR:
	    		// straight copy needed here.
	    		V_BSTR (d) = ::SysAllocString (V_BSTR (&tmp));
	    		break;

	    	    case VT_BOOL:
	    		// convert to True/False string
	    		if (GetVariantValue (&tmp) == 0)
			    {
			    const OLECHAR falseString [] = { 'F',
							     'a',
							     'l',
							     's',
							     'e',
							      0 };
	    		    V_BSTR (d) = ::SysAllocString (falseString);
			    }
	    		else
			    {
			    const OLECHAR trueString [] = { 'T',
							    'r',
							    'u',
							    'e',
							     0 };
	    		    V_BSTR (d) = ::SysAllocString (trueString);
			    }
	    		break;

	    	    default:
	    		{
	    		// Allocate tmp buffer this way so as not to stress
	    		// the stack too much.
	    		char * tmpBuffer = new char [128];

			if (0 == tmpBuffer)
			    return E_OUTOFMEMORY;

	    		sprintf (tmpBuffer, "%f", GetVariantValue (&tmp));

			OLECHAR * wStr = new OLECHAR [strlen (tmpBuffer) + 1];

			if (0 == wStr)
			    {
			    delete []tmpBuffer;
			    return E_OUTOFMEMORY;
			    }

			comAsciiToWide (wStr, tmpBuffer, strlen (tmpBuffer) + 1);

	    		V_BSTR (d) = ::SysAllocString (wStr);

			delete []wStr;
	    		delete []tmpBuffer;
	    		}

	    	    }
	    	break;

            default:
                hr = E_INVALIDARG;
                break;
            }
        }

    VariantClear (&tmp);

    return hr;
}

/* SAFEARRAY-related APIs. */

/**************************************************************************
*
* elementSize - Get's the size of the element in bytes.
* 
* Returns the size of a VARIANT data type in bytes.
*
* RETURNS: The size of the VARIANT data type or zero for unknown types.
*.NOMANUAL
*
*/

static ULONG elementSize (VARTYPE sf)
    {
    switch (sf)
        {
        case VT_ERROR:  return sizeof (SCODE);
        case VT_UI1:    return sizeof (BYTE);
        case VT_I2:     return sizeof (SHORT);
        case VT_I4:     return sizeof (LONG);
        case VT_BSTR:   return sizeof (BSTR);
        case VT_R4:     return sizeof (float);
        case VT_R8:     return sizeof (double);
        case VT_CY:     return sizeof (CY);
        case VT_DATE:   return sizeof (DATE);
        case VT_BOOL:   return sizeof (VARIANT_BOOL);
        case VT_UNKNOWN:return sizeof (IUnknown *);
        case VT_VARIANT:return sizeof (VARIANT);
        default:
            return 0;
        }
    }

/**************************************************************************
*
* SafeArrayCreate - Creates a new SAFEARRAY.
* 
* Creates a new array descriptor, allocates and initializes the data for 
* the array, and returns a pointer to the new array descriptor. 
*
* The array will always be created with the features FADF_HAVEVARTYPE | 
* FADF_FIXEDSIZE | FADF_AUTO and the appropriate BSTR, VARIANT or IUNKNOWN
* bit set.
*
* RETURNS: The array descriptor, or NULL if the array could not be created. 
*
*/

SAFEARRAY * SafeArrayCreate
    ( 
    VARTYPE             vt,         /* The base type of the array. Neither  */
                                    /* the VT_ARRAY or the VT_BYREF flag    */
                                    /* can be set.                          */
                                    /* VT_EMPTY and VT_NULL are not valid   */
                                    /* base types for the array. All other  */
                                    /* types are legal.                     */
    UINT                cDims,      /* Number of dimensions in the array.   */
    SAFEARRAYBOUND *    rgsabound   /* Vector of bounds to allocate         */
    )
    {
    MEM_SAFEARRAY *     pArrayDesc = NULL;

    if (rgsabound == NULL)
        return NULL;

    if (cDims < 1)
        return NULL;

    /* Check for supported types */
    switch (vt)
        {
        case VT_ERROR:
        case VT_UI1:
        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_R8:
        case VT_CY: 
        case VT_DATE:
        case VT_BOOL:
        case VT_UNKNOWN:
        case VT_VARIANT:
        case VT_BSTR:
            break;

        default:
            return NULL;
        }

    /* This will be sizeof (SAFEARRAYBOUND) bigger than it needs to be */
    /* because WIDL defines a dummy SAFEARRAYBOUND record so that the  */
    /* defined name can be used.                                       */
    pArrayDesc = reinterpret_cast<MEM_SAFEARRAY *>
                (COM_MEM_ALLOC (sizeof (MEM_SAFEARRAY) 
                                + sizeof (SAFEARRAYBOUND) * cDims));

    if (pArrayDesc == NULL)
        {
        /* No memory */
        return NULL;
        }

    /* Can get away with just zeroing the first part of the structure. */
    memset (pArrayDesc, 
            '\0', 
            sizeof (MEM_SAFEARRAY));

    /* Allocate mutex */
    pArrayDesc->pMutex = new VxMutex ();

    if (pArrayDesc->pMutex == NULL)
        {
        COM_MEM_FREE (pArrayDesc);
        return NULL;
        }

    /* Allocate data first since if it can't be done there isn't */
    /* any point in proceeding further                           */
    pArrayDesc->arrayData.cbElements = elementSize (vt);

    USHORT  index;

    pArrayDesc->dataCount = 1;

    for (index = 0; index < cDims; index++)
        {
        pArrayDesc->dataCount *= rgsabound [index].cElements;
        }

    pArrayDesc->arrayData.pvData = COM_MEM_ALLOC (pArrayDesc->dataCount * 
                                        pArrayDesc->arrayData.cbElements);

    if (pArrayDesc->arrayData.pvData == NULL)
        {
        /* We have a memory failure so delete the memory for the descriptor */
        /* and return an error.                                             */
        COM_MEM_FREE (pArrayDesc);
        return NULL;
        }

    // zero all allocated memory
    memset (pArrayDesc->arrayData.pvData, 
            '\0', 
            pArrayDesc->dataCount * pArrayDesc->arrayData.cbElements);

    /* Copy the rgsbound data into the correct place in the structure   */
    memcpy (pArrayDesc->arrayData.rgsabound, 
            rgsabound, 
            sizeof (SAFEARRAYBOUND) * cDims);

    /* Fill in various fields with appropriate values */
    pArrayDesc->vt = vt;
    pArrayDesc->arrayData.cDims = cDims;

    switch (vt)
        {
        case VT_VARIANT:
            {
            pArrayDesc->arrayData.fFeatures = FADF_VARIANT;
            /* VariantInit all members */
            VARIANT * pVar = (VARIANT *)(pArrayDesc->arrayData.pvData);
            for (index = 0; index < pArrayDesc->dataCount; index++)
                {
                VariantInit (pVar + index);
                }
            }
            break;

        case VT_BSTR:
            pArrayDesc->arrayData.fFeatures = FADF_BSTR;
            break;

        case VT_UNKNOWN:
            pArrayDesc->arrayData.fFeatures = FADF_UNKNOWN;
            break;

        default:
            pArrayDesc->arrayData.fFeatures = 0;
        }

    pArrayDesc->arrayData.fFeatures |= FADF_HAVEVARTYPE 
                                       | FADF_FIXEDSIZE 
                                       | FADF_AUTO;

    pArrayDesc->arrayData.cLocks = 0;

    pArrayDesc->signature = SAFEARRAY_SIG;

    return &(pArrayDesc->arrayData);
    }


/*
* ArrayElement - list class to hold reference to a SA so that the mutex
*                can be unlocked in case of failure during SA destroy.
*/

class ArrayElement
    {
    public:
    ArrayElement () { m_psa = NULL; }
    void setPsa (MEM_SAFEARRAY * psa) { m_psa = psa; }
    void unlock () { m_psa->pMutex->unlock (); }

    private:
    MEM_SAFEARRAY *     m_psa;
    };


/* List to hold above elements */

typedef VxGenericListElement<ArrayElement> ListElem;

typedef VxGenericList<ListElem,int,VxMutex> 
            ARRAY_LIST;


/**************************************************************************
*
* CheckArray - Check a SAFEARRAY for outstanding locks.
*
* This recurses through a SAFEARRAY of VARIANTs of SAFEARRAYs
* to check if there are any locks outstanding.
*
* RETURNS: S_OK if no locks exist and error code otherwise.
*.NOMANUAL
*/

static HRESULT CheckArray
    (
    MEM_SAFEARRAY *	pArrayDesc,	/* Array descriptor */
    ARRAY_LIST &	as		/* list of array elements */
    					/* that have been checked */
    )
    {
    ULONG    index;

    /* Check for a valid signature etc... */
    if (pArrayDesc->signature != SAFEARRAY_SIG)
        {
        return E_INVALIDARG;
        }

    /* Lock the mutex so that nothing can do anything bad underneath us */
    pArrayDesc->pMutex->lock();

    /* Save the descriptor so it can be unlocked later */
    as.pushHead (new ListElem);
    as.getHead ()->setPsa (pArrayDesc);

    if (pArrayDesc->arrayData.cLocks != 0)
        {
        return DISP_E_ARRAYISLOCKED;
        }

    /* Since we setup the array to have a VT when we create it we       */
    /* can use it in the deallocation.                                  */
    if (pArrayDesc->vt == VT_VARIANT)
        {
        VARIANT *   var = reinterpret_cast<VARIANT *>
                            (pArrayDesc->arrayData.pvData);

        for (index = 0; index < pArrayDesc->dataCount; index++)
            {
            /* Check if this variant contains an array */
            if (V_VT (&(var [index])) & VT_ARRAY)
                {
                MEM_SAFEARRAY * ptr = SA2MSA(V_ARRAY(&(var [index])));
                HRESULT hr = CheckArray (ptr, as);

                if (FAILED (hr))
                    {
                    return hr;
                    }
                }
            }
        }
    return S_OK;
    }

/**************************************************************************
* 
* DestroyArray - Deletes all SAFEARRAYs in a tree.
*
* Recurses through a SAFEARRAY (and any VARIANTS containing SAFEARRAYs)
* and deletes them.
*
* RETURNS: N/A
*.NOMANUAL
*/

void DestroyArray
    (
    MEM_SAFEARRAY *	pArrayDesc	/* Array descriptor */
    )
    {
    ULONG    index;

    switch (pArrayDesc->vt)
        {
        case VT_VARIANT:
            {
            VARIANT *   var = reinterpret_cast<VARIANT *>
                                (pArrayDesc->arrayData.pvData);

            for (index = 0; index < pArrayDesc->dataCount; index++)
                {
                /* Check if this variant contains an array */
                if (V_VT (&(var [index])) & VT_ARRAY)
                    {
                    DestroyArray (SA2MSA(V_ARRAY(&(var [index]))));
                    }
                else
                    {
                    VariantClear (&(var [index]));
                    }
                }
            }
            break;

        case VT_BSTR:
            {
            BSTR *  pBstr = reinterpret_cast<BSTR *>
                                (pArrayDesc->arrayData.pvData);

            for (index = 0; index < pArrayDesc->dataCount; index++)
                {
                if (pBstr [index] != NULL)
                    {
                    ::SysFreeString (pBstr [index]);
                    }
                }
             }
            break;

        case VT_UNKNOWN:
            {
            IUnknown **  ppUnk = reinterpret_cast<IUnknown **>
                                (pArrayDesc->arrayData.pvData);

            for (index = 0; index < pArrayDesc->dataCount; index++)
                {
                if (ppUnk [index] != NULL)
                    {
                    ppUnk [index]->Release ();
                    }
                }
             }
             break;
        }
    pArrayDesc->signature = 0;
    pArrayDesc->pMutex->unlock ();
    delete pArrayDesc->pMutex;
    COM_MEM_FREE (pArrayDesc->arrayData.pvData);
    COM_MEM_FREE (pArrayDesc);
    }

/**************************************************************************
*
* SafeArrayDestroy - Destroys an existing array descriptor.
* Destroys an existing array descriptor and all of the 
* data in the array. If complex types are stored in the
* array the appropriate destruction will be performed.
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i DISP_E_ARRAYISLOCKED
* If the SAFEARRAY is locked
* \i E_INVALIDARG 
* If one of the arguments is invalid.
* \ie
*/
HRESULT SafeArrayDestroy
    ( 
    SAFEARRAY *         psa     /* Pointer to an array descriptor created */
                                /* by SafeArrayCreate */
    )
    {
    MEM_SAFEARRAY *     pArrayDesc;
    ARRAY_LIST          as;     /* This is only required in the case where */
                                /* we have to unlock the mutexs if the     */
                                /* fails. Mutexs are unlocked as part of   */
                                /* the recursive delete otherwise.         */
    HRESULT             hr;

    /* Check descriptor is valid */
    if (psa == NULL)
        {
        return E_INVALIDARG;
        }

    pArrayDesc = SA2MSA(psa);

    hr = CheckArray (pArrayDesc, as);

    if (FAILED (hr))
        {
        /* release all locks */
        ArrayElement * ptr;

        while (!as.isEmpty ())
            {
            ptr = as.popHead ();
            ptr->unlock ();
            delete ptr;
            }
        return hr;
        }

    DestroyArray (pArrayDesc);

    as.destroyList ();

    return S_OK;

    }

/**************************************************************************
*
* SafeArrayLock - Increment the lock count of a SAFEARRAY.
*
* This function increments the lock count of a SAFEARRAY. 
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i E_INVALIDARG
* If the SAFEARRAY is invalid.
* \ie
*/

HRESULT SafeArrayLock 
    (
    SAFEARRAY *     psa     /* Pointer to an array descriptor created by */
                            /* SafeArrayCreate.                          */
    )
    {
    MEM_SAFEARRAY * pArrayDesc;

    if (psa == NULL)
        {
        return E_INVALIDARG;
        }

    pArrayDesc = SA2MSA(psa);

    /* Quick sanity check, if this passes but the mutex is invalid */
    /* we'll get a VxDCOM Assert.                                  */
    if (pArrayDesc->signature != SAFEARRAY_SIG)
        {
        return E_INVALIDARG;
        }

    pArrayDesc->pMutex->lock ();

    /* Check sig again just incase we're being woken up after SA has been */
    /* deleted. */
    if (pArrayDesc->signature != SAFEARRAY_SIG)
        {
        return E_INVALIDARG;
        }

    /* This is all OK so increment the lock */
    pArrayDesc->arrayData.cLocks++;

    pArrayDesc->pMutex->unlock ();

    return S_OK;
    }

/**************************************************************************
*
* SafeArrayUnlock - Decrement the lock count of a SAFEARRAY.
*
* This function decrements the lock count of a SAFEARRAY. 
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i E_INVALIDARG 
* If the SAFEARARAY is invalid.
* \i  E_UNEXPECTED
* If and attempt is made to decrement the lock count when it is zero.
* \ie
*/

HRESULT SafeArrayUnlock 
    (
    SAFEARRAY *     psa     /* Pointer to an array descriptor created by */
                            /* SafeArrayCreate.                          */
    )
    {
    MEM_SAFEARRAY * pArrayDesc;
    HRESULT         hr = S_OK;

    if (psa == NULL)
        {
        return E_INVALIDARG;
        }

    pArrayDesc = SA2MSA(psa);

    /* Quick sanity check, if this passes but the mutex is invalid */
    /* we'll get a VxDCOM Assert.                                  */
    if (pArrayDesc->signature != SAFEARRAY_SIG)
        {
        return E_INVALIDARG;
        }

    pArrayDesc->pMutex->lock ();

    /* Check sig again just incase we're being woken up after SA has been */
    /* deleted. */
    if (pArrayDesc->signature != SAFEARRAY_SIG)
        {
        pArrayDesc->pMutex->unlock ();
        return E_INVALIDARG;
        }

    /* This is all OK so increment the lock */
    if (pArrayDesc->arrayData.cLocks)
        {
        pArrayDesc->arrayData.cLocks--;
        }
    else
        {
        hr = E_UNEXPECTED;
        }

    pArrayDesc->pMutex->unlock ();

    return hr;
    }

/**************************************************************************
* 
* ArrayPos - Calculates the address of an array element.
* This calculates the position of an element in the array based on the
* following formula:
*
* p = i [0] + (d [0] * i [1]) + (d [0] * d [1] * i [2]) + ...
*
* RETURNS: S_OK on success, DISP_E_BADINDEX on failure.
*.NOMANUAL
*/


HRESULT ArrayPos
    (
    SAFEARRAY *		psa,		/* Array descriptor */
    long *		rgIndices,	/* Index into array */
    void **		pv		/* pointer to array dlement */
    )
    {
    LONG    inner;
    LONG    outer;
    LONG    sum = rgIndices [0] - psa->rgsabound [0].lLbound;

    *pv = NULL;

    /* Check that the index is in range since this is a Safearray */
    if ((rgIndices [0] < psa->rgsabound [0].lLbound) ||
        ((rgIndices [0] - psa->rgsabound [0].lLbound) 
            > (LONG)psa->rgsabound [0].cElements))
        {
        return DISP_E_BADINDEX;
        }

    for (outer = 1; outer < psa->cDims; outer++)
        {
        LONG    mul = 1;

        /* Check that the index is in range since this is a Safearray */
        if ((rgIndices [outer] < psa->rgsabound [outer].lLbound) ||
            ((rgIndices [outer] - psa->rgsabound [outer].lLbound) 
                > (LONG)psa->rgsabound [outer].cElements))
            {
            return DISP_E_BADINDEX;
            }

        /* Do the d [0] + d [1] + ... bit */
        for (inner = 0; inner < outer; inner++)
            {
            mul *= psa->rgsabound [inner].cElements;
            }

        /* Multiply by the current index */
        mul *= (rgIndices [outer] - psa->rgsabound [outer].lLbound);

        /* Add to the cumulative offset */
        sum += mul;
        }

    /* Multiply the offset by the element size to get to the right position */
    /* without having to typecast anything.                                 */
    sum *= psa->cbElements;

    /* Set the data position. */
    *pv = reinterpret_cast<void *>(reinterpret_cast<BYTE *>(psa->pvData) + sum);

    return S_OK;
    }

/**************************************************************************
*
* SafeArrayPutElement - Stores a copy of the data at a given location.
*
* This function assigns a single element to the array. This function 
* calls SafeArrayLock and SafeArrayUnlock before and after assigning 
* the element. If the data element is a string, object, or variant, 
* the function copies it correctly. If the existing element is a string, 
* object, or variant, it is cleared correctly.
*
* RETURNS:
* \is
* \i S_OK
* Success. 
* \i DISP_E_BADINDEX
* The specified index was invalid. 
* \i E_INVALIDARG
* One of the arguments is invalid. 
* \i E_OUTOFMEMORY
* Memory could not be allocated for the element. 
* \ie
*/

HRESULT SafeArrayPutElement
    (
    SAFEARRAY *     psa,        /* Pointer to an array descriptor created by */
                                /* SafeArrayCreate.                          */
    long *          rgIndicies, /* Pointer to a vector of indices for each   */
                                /* dimension of the array.                   */
    void *          pv          /* Pointer to the data to assign.            */
    )
    {
    void *          ptr;
    HRESULT         hr = S_OK;

    hr = SafeArrayLock (psa);

    if (FAILED (hr))
        {
        return hr;
        }

    /* get a pointer to the appropriate area of memory */
    hr = ArrayPos (psa, rgIndicies, &ptr);

    if (SUCCEEDED (hr))
        {
        switch (SA2MSA(psa)->vt)
            {
            case VT_ERROR:
            case VT_UI1:
            case VT_I2:
            case VT_I4:
            case VT_R4:
            case VT_R8:
            case VT_CY: 
            case VT_DATE:
            case VT_BOOL:
                memcpy (ptr, pv, psa->cbElements);
                break;

            case VT_UNKNOWN:
                {
                IUnknown ** iPtr = reinterpret_cast<IUnknown **>(ptr);
                if (*iPtr != NULL)
                    {
                    (*iPtr)->Release ();
                    *iPtr = NULL;
                    }
                if (reinterpret_cast<IUnknown *>(pv) != NULL)
                    {
                    hr = (reinterpret_cast<IUnknown *>(pv))->
                        QueryInterface(IID_IUnknown, 
                                       (void **)iPtr);
                    }
                }
                break;

            case VT_BSTR:
                {
                BSTR *      bPtr = reinterpret_cast<BSTR *>(ptr);

                if (*bPtr != NULL)
                    {
                    hr = ::SysFreeString (*bPtr);
                    *bPtr = NULL;
                    }
                if (SUCCEEDED (hr) && (pv != NULL))
                    {
                    *bPtr = ::SysAllocString ((BSTR)pv);
                    }
                }
                break;

            case VT_VARIANT:
                hr = VariantClear (reinterpret_cast<VARIANT *>(ptr));
                if (SUCCEEDED (hr))
                    {
                    hr = VariantCopy (reinterpret_cast<VARIANT *>(ptr), 
                                      reinterpret_cast<VARIANT *>(pv));
                    }
                break;
            }
        }

    HRESULT hr2 = SafeArrayUnlock (psa);

    if (FAILED (hr2))
        {
        return hr2;
        }

    return hr;
    }

/**************************************************************************
*
* SafeArrayGetElement - Retrieves a single element of the array. 
*
* Retrieves a single element of the array. This function calls 
* SafeArrayLock and SafeArrayUnlock automatically, before and 
* after retrieving the element. The caller must provide a storage 
* area of the correct size to receive the data. If the data element 
* is a string, object, or variant, the function copies the element 
* in the correct way. 
*
* RETURNS:
* \is
* \i S_OK
* Success. 
* \i DISP_E_BADINDEX
* The specified index was invalid. 
* \i E_INVALIDARG
* One of the arguments is invalid. 
* \i E_OUTOFMEMORY
* Memory could not be allocated for the element. 
* \ie
*/

HRESULT SafeArrayGetElement
    (
    SAFEARRAY *     psa,        /* Pointer to an array descriptor created by */
                                /* SafeArrayCreate.                          */
    long *          rgIndicies, /* Pointer to a vector of indices for each   */
                                /* dimension of the array.                   */
    void *          pv          /* Pointer to the returned data.             */
    )
    {
    HRESULT     hr = S_OK;
    void *      ptr;

    if (pv == NULL)
        {
        return E_INVALIDARG;
        }

    hr = SafeArrayLock (psa);

    if (FAILED (hr))
        {
        return hr;
        }


    /* get a pointer to the appropriate area of memory */
    hr = ArrayPos (psa, rgIndicies, &ptr);

    if (SUCCEEDED (hr))
        {        
        switch (SA2MSA(psa)->vt)
            {
            case VT_ERROR:
            case VT_UI1:
            case VT_I2:
            case VT_I4:
            case VT_R4:
            case VT_R8:
            case VT_CY: 
            case VT_DATE:
            case VT_BOOL:
                memcpy (pv, ptr, psa->cbElements);
                break;

            case VT_UNKNOWN:
                if (*reinterpret_cast<IUnknown **>(ptr))
                    {
                    hr = (*reinterpret_cast<IUnknown **>(ptr))->
                            QueryInterface(IID_IUnknown, 
                                           reinterpret_cast<void **>(pv));
                    }
                else
                    {
                    *reinterpret_cast<IUnknown **>(pv) = NULL;
                    }
                break;

            case VT_BSTR:
                    if (*reinterpret_cast<BSTR *>(ptr) != NULL)
                        {
                        *reinterpret_cast<BSTR *>(pv) = 
                            ::SysAllocString (*reinterpret_cast<BSTR *>(ptr));
                        }
                    else
                        {
                        *reinterpret_cast<BSTR *>(pv) = NULL;
                        }
                break;

            case VT_VARIANT:
                VariantInit (reinterpret_cast<VARIANT *>(pv));
                if (SUCCEEDED (hr))
                    {
                    hr = VariantCopy (reinterpret_cast<VARIANT *>(pv),
                                      reinterpret_cast<VARIANT *>(ptr));
                    }
                break;
            }
        

        }

    HRESULT hr2 = SafeArrayUnlock (psa);

    if (FAILED (hr2))
        {
        return hr2;
        }
    
    return hr;
    
    }

/**************************************************************************
*
* SafeArrayAccessData - Locks a SAFEARRAY.
* This function increments the lock count of an array, and retrieves a 
* pointer to the array data.
*
* The data array returned is arranged using the following formula:
*
* p = (i [0] + (d [0] * i [1]) + (d [0] * d [1] * i [2]) ...) * size of element
*
* Where:
*    i [n]  - holds the indices 
*    d [n]  - holds the dimensions 
*    p      - a pointer to the storage location 
*
* RETURNS:
* \is
* \i S_OK
* Success. 
* \i E_INVALIDARG
* One of the arguments is invalid. 
* \ie
*/

HRESULT SafeArrayAccessData
    ( 
    SAFEARRAY *     psa,        /* Pointer to an array descriptor */
    void **         ppvData     /* Pointer to a pointer to the array data */
    )
    {
    HRESULT hr = SafeArrayLock (psa);

    if (FAILED(hr))
        {
        return hr;
        }

    *ppvData = psa->pvData;
    return S_OK;
    }

/**************************************************************************
*
* SafeArrayUnaccessData - This function decrements the lock count of an array.
*
* This function decrements the lock count of an array which was locked 
* using SafeArrayAccessData.
*
* RETURNS:
* \is
* \i S_OK
* Success. 
* \i E_INVALIDARG
* One of the arguments is invalid. 
* \ie
*/

HRESULT SafeArrayUnaccessData
    ( 
    SAFEARRAY *         psa    /* Pointer to an array descriptor */
    )
    {
    return SafeArrayUnlock (psa);
    }

/**************************************************************************
*
* SafeArrayCopy() - Creates a copy of an existing safe array.
*
* This function creates a copy of an existing SAFEARRAY created with 
* SafeArrayCreate. If the SAFEARRAY contains interface pointers the reference
* counts are incremeneted. If the SAFEARRAY contains BSTRS copies of the BSTRS
* are made.
*
* RETURNS:
* \is
* \i S_OK
* Success. 
* \i E_INVALIDARG
* One of the arguments is invalid. 
* \i E_OUTOFMEMORY
* There isn't enought memory to complete the copy.
* \ie
*/

HRESULT SafeArrayCopy
    (
    SAFEARRAY *		psa, 		/* Pointer to SAFEARRAY */
    SAFEARRAY **	ppsaOut 	/* Pointer to destination */
    )
    {
    void *	psv;
    void *	pdv;
    HRESULT	hr = S_OK;

    hr = SafeArrayLock (psa);
    if (FAILED (hr))
        {
        return hr;
        }

    *ppsaOut = SafeArrayCreate (SA2MSA(psa)->vt, 
                                psa->cDims, 
                                psa->rgsabound);
    if (*ppsaOut == NULL) return hr;

    hr = SafeArrayAccessData (psa, &psv);
    if (FAILED (hr))
        {
        SafeArrayDestroy (*ppsaOut);
        return hr;
        }

    hr = SafeArrayAccessData (*ppsaOut, &pdv);
    if (FAILED (hr))
        {
        SafeArrayUnaccessData (psa);
        SafeArrayDestroy (*ppsaOut);
        return hr;
        }

    switch (SA2MSA(psa)->vt)
        {
        case VT_BSTR:
            {
            BSTR *  source = reinterpret_cast<BSTR *>(psv);
            BSTR *  dest = reinterpret_cast<BSTR *>(pdv);
            DWORD   index;

            for (index = 0; index < SA2MSA(psa)->dataCount; index++)
                {
                // only copy if a string is present, otherwise leave as NULL.
                if (source[index])
                    {
                    dest[index] = ::SysAllocString(source[index]);
                    }
                }
            }
            break;

        case VT_UNKNOWN:
            {
            IUnknown **     source = reinterpret_cast<IUnknown **>(psv);
            IUnknown **     dest = reinterpret_cast<IUnknown **>(pdv);
            DWORD           index;

            for (index = 0; index < SA2MSA(psa)->dataCount; index++)
                {
                // only copy if a string is present, otherwise leave as NULL.
                if (source[index])
                    {
                    source[index]->QueryInterface(IID_IUnknown, 
                                                  (void **)&dest[index]);
                    }
                }
            }
            break;

        case VT_VARIANT:
            {
            VARIANT *   source = reinterpret_cast<VARIANT *>(psv);
            VARIANT *   dest = reinterpret_cast<VARIANT *>(pdv);
            DWORD       index;

            for (index = 0; index < SA2MSA(psa)->dataCount; index++)
                {
                // only copy if a string is present, otherwise leave as NULL.
                VariantCopy(&dest [index], &source [index]);
                }
            }
            break;

        default:
            memcpy(pdv, 
                   psv, 
                   SA2MSA(psa)->dataCount * psa->cbElements);
            break;
        }

    SafeArrayUnaccessData (psa);
    SafeArrayUnlock (psa);
    hr = SafeArrayUnaccessData (*ppsaOut);
    return hr;
    }

/**************************************************************************
*
* SafeArrayGetLBound() - Gets the lower bound for any dimension of a SAFEARRAY.
*
* This functions gets the lower bound for any dimension in a SAFEARRAY.
*
* RETURNS:
* \is
* \i S_OK
* Success.
* \i DISP_E_BADINDEX
* The specified index is out of bounds.
* \i E_INVALIDARG
* One of the arguments is invalid.
* \ie
*/

HRESULT SafeArrayGetLBound
    (
    SAFEARRAY *		psa, 		/* Pointer to SAFEARRAY */
    unsigned int	nDim, 		/* Dimension of SAFEARRAY starting at 1 */
    long *		plLbound 	/* Pointer to result */
    )
    {
    if (plLbound == NULL)
    	{
	return E_INVALIDARG;
    	}

    if (nDim < 1)
        {
        return DISP_E_BADINDEX;
        }

    HRESULT	hr = SafeArrayLock (psa);

    if (FAILED (hr))
        {
        return hr;
        }

    if (nDim > psa->cDims)
        {
        SafeArrayUnlock (psa);
        return DISP_E_BADINDEX;
        }

    *plLbound = psa->rgsabound [nDim - 1].lLbound;

    return SafeArrayUnlock (psa);
    }

/**************************************************************************
*
* SafeArrayGetUBound() - Gets the upper bound for any dimension of a SAFEARRAY.
*
* This functions gets the upper bound for any dimension in a SAFEARRAY.
*
* RETURNS:
* \is
* \i S_OK
* Success.
* \i DISP_E_BADINDEX
* The specified index is out of bounds.
* \i E_INVALIDARG
* One of the arguments is invalid.
* \ie
*/

HRESULT SafeArrayGetUBound
    (
    SAFEARRAY *		psa, 		/* Pointer to a SAFEARRAY */
    unsigned int	nDim, 		/* Dimension of SAFEARRAY starting at 1 */
    long *		plUbound	/* Pointer to returned result */
    )
    {
    if (plUbound == NULL)
    	{
	return E_INVALIDARG;
    	}

    if (nDim < 1)
        {
        return DISP_E_BADINDEX;
        }

    HRESULT	hr = SafeArrayLock (psa);

    if (FAILED (hr))
        {
        return hr;
        }

    if (nDim > psa->cDims)
        {
        SafeArrayUnlock (psa);
        return DISP_E_BADINDEX;
        }

    *plUbound = psa->rgsabound [nDim - 1].lLbound + 
                (long)(psa->rgsabound [nDim - 1].cElements) - 1; 

    return SafeArrayUnlock (psa);
    }
  
/**************************************************************************
*
* SafeArrayGetDim() - Gets the number of dimensions in the array.
*
* This functions gets the number of dimensions in the array.
*
* RETURNS: 0 if an invalid SAFEARRAY is given, otherwise the number 
* dimmensions in the array
*/

UINT SafeArrayGetDim
    ( 
    SAFEARRAY *		psa		/* Pointer to a SAFEARRAY */
    )
    {
    UINT 		result;	

    if (FAILED (SafeArrayLock (psa)))
        {
        return 0;
        }

    result = psa->cDims;

    SafeArrayUnlock (psa);

    return result;
    }
  
/**************************************************************************
*
* SafeArrayGetElemsize() - Gets the size of an element in the SAFEARRAY.
*
* This functions gets the size of an element in the SAFEARRAY
*
* RETURNS: 0 if an invalid SAFEARRAY is given, otherwise the size of the
* element in the SAFEARRAY.
*/

UINT SafeArrayGetElemsize
    (
    SAFEARRAY *		psa		/* Pointer to a SAFEARRAY */
    )
    {
    UINT 		result;	

    if (FAILED (SafeArrayLock (psa)))
        {
        return 0;
        }

    result = psa->cbElements;

    SafeArrayUnlock (psa);

    return result;
    }

/**************************************************************************
*
* SafeArrayGetVartype() - Gets the VARTYPE stored in the given SAFEARRAY.
*
* This functions gets the VARTYPE stored in the given SAFEARRAY.
*
* RETURNS:
* \is
* \i S_OK
* On success
* \i E_INVALIDARG
* One of the arguments is invalid.
* \ie
*/

HRESULT SafeArrayGetVartype
    ( 
    SAFEARRAY *		psa, 		/* Pointer to a SAFEARRAY */
    VARTYPE *		pvt  		/* Pointer to the type returned */
    )
    {
    if (pvt == NULL)
    	{
	return E_INVALIDARG;
    	}

    if (FAILED (SafeArrayLock (psa)))
        {
        return E_INVALIDARG;
        }

    *pvt = SA2MSA (psa)->vt;

    SafeArrayUnlock (psa);

    return S_OK;
    }

/* CComVariant */

CComVariant& CComVariant::operator=(BSTR bstrSrc)
    {
    InternalClear();
    vt = VT_BSTR;
    bstrVal = ::SysAllocString(bstrSrc);
    if (bstrVal == NULL && bstrSrc != NULL)
        {
        vt = VT_ERROR;
        scode = E_OUTOFMEMORY;
	}
    return *this;
    }

CComVariant& CComVariant::operator=(LPCOLESTR lpszSrc)
    {
    InternalClear();
    vt = VT_BSTR;
    bstrVal = ::SysAllocString(lpszSrc);

    if (bstrVal == NULL && lpszSrc != NULL)
	{
        vt = VT_ERROR;
        scode = E_OUTOFMEMORY;
	}
    return *this;
    }

CComVariant& CComVariant::operator=(bool bSrc)
    {
    if (vt != VT_BOOL)
	{
        InternalClear();
        vt = VT_BOOL;
	}
    boolVal = bSrc ? VARIANT_TRUE : VARIANT_FALSE;
    return *this;
    }

CComVariant& CComVariant::operator=(int nSrc)
    {
    if (vt != VT_I4)
	{
        InternalClear();
        vt = VT_I4;
	}
    lVal = nSrc;

    return *this;
    }

CComVariant& CComVariant::operator=(BYTE nSrc)
    {
    if (vt != VT_UI1)
	{
        InternalClear();
        vt = VT_UI1;
	}
    bVal = nSrc;
    return *this;
    }

CComVariant& CComVariant::operator=(short nSrc)
    {
    if (vt != VT_I2)
	{
        InternalClear();
        vt = VT_I2;
	}
    iVal = nSrc;
    return *this;
    }

CComVariant& CComVariant::operator=(long nSrc)
    {
    if (vt != VT_I4)
	{
        InternalClear();
        vt = VT_I4;
	}
    lVal = nSrc;
    return *this;
    }

CComVariant& CComVariant::operator=(float fltSrc)
    {
    if (vt != VT_R4)
	{
        InternalClear();
        vt = VT_R4;
	}
    fltVal = fltSrc;
    return *this;
    }

CComVariant& CComVariant::operator=(double dblSrc)
    {
    if (vt != VT_R8)
	{
        InternalClear();
        vt = VT_R8;
	}
    dblVal = dblSrc;
    return *this;
    }

CComVariant& CComVariant::operator=(CY cySrc)
    {
    if (vt != VT_CY)
	{
        InternalClear();
        vt = VT_CY;
	}
    cyVal = cySrc;
    return *this;
    }

CComVariant& CComVariant::operator=(IUnknown* pSrc)
    {
    InternalClear();
    vt = VT_UNKNOWN;
    punkVal = pSrc;

    // Need to AddRef as VariantClear will Release
    if (punkVal != NULL)
        punkVal->AddRef();
    return *this;
    }

bool CComVariant::operator==(const VARIANT& varSrc)
    {
    if (this == &varSrc)
        return true;

    // Variants not equal if types don't match
    if (vt != varSrc.vt)
        return false;

    // Check type specific values
    switch (vt)
        {
        case VT_EMPTY:
        case VT_NULL:
            return true;

        case VT_BOOL:
            return boolVal == varSrc.boolVal;

        case VT_UI1:
            return bVal == varSrc.bVal;

        case VT_I2:
            return iVal == varSrc.iVal;

        case VT_I4:
            return lVal == varSrc.lVal;

        case VT_R4:
            return fltVal == varSrc.fltVal;

        case VT_R8:
            return dblVal == varSrc.dblVal;

        case VT_BSTR:
            return (::SysStringByteLen(bstrVal) == 
                    ::SysStringByteLen(varSrc.bstrVal)) &&
                   (::memcmp(bstrVal, 
                             varSrc.bstrVal, 
                             ::SysStringByteLen(bstrVal)) == 0);

        case VT_ERROR:
            return scode == varSrc.scode;

        case VT_UNKNOWN:
            return punkVal == varSrc.punkVal;

        default:
            COM_ASSERT (false);
            // fall through
	}

    return false;
    }

HRESULT CComVariant::Attach(VARIANT* pSrc)
    {
    // Clear out the variant
    HRESULT hr = Clear();
    if (!FAILED(hr))
	{
        // Copy the contents and give control to CComVariant
        memcpy(this, pSrc, sizeof(VARIANT));
        VariantInit(pSrc);
        hr = S_OK;
	}
    return hr;
    }

HRESULT CComVariant::Detach(VARIANT* pDest)
    {
    // Clear out the variant
    HRESULT hr = ::VariantClear(pDest);
    if (!FAILED(hr))
	{
        // Copy the contents and remove control from CComVariant
        memcpy(pDest, this, sizeof(VARIANT));
        vt = VT_EMPTY;
        hr = S_OK;
	}
    return hr;
    }

HRESULT CComVariant::ChangeType(VARTYPE vtNew, const VARIANT* pSrc)
    {
    VARIANT* pVar = const_cast<VARIANT*>(pSrc);
    // Convert in place if pSrc is NULL
    if (pVar == NULL)
        pVar = this;
    // Do nothing if doing in place convert and vts not different
    return ::VariantChangeType(this, pVar, 0, vtNew);
    }

HRESULT CComVariant::InternalClear()
    {
    HRESULT hr = Clear();
    COM_ASSERT (SUCCEEDED(hr));
    if (FAILED(hr))
	{
        vt = VT_ERROR;
        scode = hr;
	}
    return hr;
    }

void CComVariant::InternalCopy(const VARIANT* pSrc)
    {
    HRESULT hr = Copy(pSrc);
    if (FAILED(hr))
	{
        vt = VT_ERROR;
        scode = hr;
	}
    }

 

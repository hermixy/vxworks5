/* comBase.h - VxDCOM Base Definitions */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*

modification history
--------------------
01l,21dec01,nel  Use offset for DCC instead of adjustment in COM_ADJUST_THIS.
01k,10dec01,dbs  diab build
01j,13jul01,dbs  allow multiple vtables to be defined in same file
01i,02jul01,dbs  fix typo in vtbl formats
01h,29jun01,dbs  fix vtbl format for T3 compilers
01g,27jun01,dbs  fix warnings from COM_VTABLE macro
01f,25jun01,dbs  rename vtbl macros to COM_VTBL_XXX
01e,22jun01,dbs  add WIDL_ADJUST_THIS macro
01d,20jun01,dbs  fix C build
01c,15jun01,dbs  fix vtbl layouts for known compilers
01b,19jul00,dbs  add basic C support
01a,21sep99,dbs  created from comLib.h
*/

#ifndef __INCcomBase_h
#define __INCcomBase_h

/*
 * Always include vxWorks.h for basic types - IDL will need to define
 * those Win32-like types that aren't present in vxWorks.h but the
 * base OS definitions should always be present.
 */

#include <vxWorks.h>

/* Some common macros... */

#define __RPC_FAR
#define __RPC_USER
#define __RPC_STUB

#ifndef interface
#define interface struct
#endif

#ifndef STDCALL
#define STDCALL
#endif

#ifdef __cplusplus
#define EXTERN_C                extern "C"
#define STDMETHOD(method)       virtual HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type,method) virtual type STDMETHODCALLTYPE method
#else
#define EXTERN_C                extern
#define STDMETHOD(method)       HRESULT STDMETHODCALLTYPE (*method)
#define STDMETHOD_(type,method) type STDMETHODCALLTYPE (*method)
#endif

#define DECLSPEC_UUID(x)
#define STDMETHODCALLTYPE       STDCALL
#define STDMETHODIMP		HRESULT STDMETHODCALLTYPE
#define STDMETHODIMP_(type)	type STDMETHODCALLTYPE

#define __RPC_FAR
#define __RPC_USER
#define __RPC_STUB

#define PRPC_MESSAGE void*

/*
 * WIDL_CPP_REF is defined as a true C++ reference when compiling for 
 * C++, and as a pointer when compiling for plain C. This is used by
 * WIDL when emitting typedefs, etc, generated from IDL.
 */

#ifdef __cplusplus
#define WIDL_CPP_REF &
#else
#define WIDL_CPP_REF *
#endif

/**************************************************************************
*
*               -- VTable Layout --
*
* Whether we're using C++ or plain C, we must generate a vtbl that is
* compatible with the C++ compiler for this architecture. Currently we
* know about the two different versions of 'gcc' where pre-egcs
* versions (as shipped with Tornado 2.0) use the old-style entries
* with this-offsets held in the table itself, but later versions
* (egcs/gcc2.95 and beyond) can use one DWORD per entry, and the
* compiler supplies an inline 'thunk' code-fragment to adjust the
* this-ptr. However, for VxWorks AE 1.x (T3.x) compilers, this feature
* is not enabled, so the vtbl format is gcc2-offsets (i.e. the old
* style).
*
* To define a vtable structure, the usage is:-
*
* typedef struct
*    {
*    COM_VTBL_BEGIN
*    COM_VTBL_ENTRY (HRESULT, Method1, (int x));
*    COM_VTBL_ENTRY (HRESULT, Method2, (long x));
*    COM_VTBL_END
*    } IFooVtbl;
*
* To build a const vtable the usage is:-
*
* COM_VTABLE(IFoo) IFoo_proxy_vtbl = {
*    COM_VTBL_HEADER
*    COM_VTBL_METHOD (&IFoo_Method1_proxy),
*    COM_VTBL_METHOD (&IFoo_Method2_proxy)
* };
*
* Note carefully the use of commas and semicolons at the end of
* the lines.
*
***************************************************************************
*
* Firstly, enumerate what the possible vtable formats are.
*
**************************************************************************/

#define COM_VTBL_FORMAT_GCC2_OFFSET  1
#define COM_VTBL_FORMAT_GCC2_THUNKS  2
#define COM_VTBL_FORMAT_GCC3         3

/**************************************************************************
*
* Now, we figure out which version of GCC and which platform its
* running on, and then select the appropriate COM_VTBL_FORMAT_XXX
* macro to enforce one (and *only* one) of the vtable formats, for
* VxWorks (AE) targets. Host-builds (Linux, Solaris) must define the
* macro in their repsective makefiles.
*
***************************************************************************/

#if !defined(unix)
#ifdef __GNUC__
# if (__GNUC__ == 2)
#  define COM_VTBL_FORMAT COM_VTBL_FORMAT_GCC2_OFFSET
# elif (__GNUC__ == 3)
#  define COM_VTBL_FORMAT COM_VTBL_FORMAT_GCC3
# endif
#elif defined (__DCC__)
#  define COM_VTBL_FORMAT COM_VTBL_FORMAT_GCC2_OFFSET
#endif
#endif

/**************************************************************************
*
* Now we take the decision we have just made, and generate the
* correct vtable layout, calling-macros, etc.
*
***************************************************************************/

#if (COM_VTBL_FORMAT == COM_VTBL_FORMAT_GCC2_OFFSET)

/**************************************************************************
*
* This is the vtable format for gcc WITHOUT THUNKS. This format has an
* adjustment to the 'this' pointer stored at each call-site (vtable
* entry) and the caller has to adjust 'this' before dispatching the
* call.
*
***************************************************************************/

#define COM_VTBL_BEGIN short __adj; short __pad; void* pRTTI;

#define COM_VTBL_ENTRY(rt, fn, args) \
    short __adj##fn; short __pad##fn; rt (*fn) args

#define COM_VTBL_END

#define COM_VTBL_HEADER 0,0,0,
#define COM_VTBL_METHOD(fn) 0,0,fn

#define COM_ADJUST_THIS(pThis) (void*)(((char*)pThis)+pThis->lpVtbl->__adjQueryInterface)

#define COM_VTBL_SYMBOL com_vtbl_format_gcc2_offset

#elif (COM_VTBL_FORMAT == COM_VTBL_FORMAT_GCC2_THUNKS)

/**************************************************************************
*
* This is the vtable format for gcc2.x WITH THUNKS. This format simply
* stores the pointer at each call-site and the compiler supplies
* inline fragments of code to adjust the 'this' pointer (these are the
* thunks) so each vtable entry is simply a function pointer.
*
***************************************************************************/

#define COM_VTBL_BEGIN short __adj; short __pad; void* pRTTI;

#define COM_VTBL_ENTRY(rt, fn, args) rt (*fn) args

#define COM_VTBL_END

#define COM_VTBL_HEADER 0,0,0,
#define COM_VTBL_METHOD(fn) fn

#define COM_ADJUST_THIS(pThis) pThis

#define COM_VTBL_SYMBOL com_vtbl_format_gcc2_thunks

#elif (COM_VTBL_FORMAT == COM_VTBL_FORMAT_GCC3)

/**************************************************************************
*
* This is the vtable format for gcc3. This format is the same as the
* 'with thunks' format, but there is no 'header' i.e. the 'this'
* pointer offset is not stored in the vtable at all.
*
***************************************************************************/

#define COM_VTBL_BEGIN

#define COM_VTBL_ENTRY(rt, fn, args) rt (*fn) args
 
#define COM_VTBL_END

#define COM_VTBL_HEADER
#define COM_VTBL_METHOD(fn) fn

#define COM_ADJUST_THIS(pThis) pThis

#define COM_VTBL_SYMBOL com_vtbl_format_gcc3

#else
#error Unknown GCC vtable format - unable to determine host/compiler.
#endif

#define DECLARE_COM_VTBL_SYMBOL_HERE   int COM_VTBL_SYMBOL = 0;

#define COM_VTABLE(itf)                         \
    extern int COM_VTBL_SYMBOL;                 \
    static void widlref_##itf(int x)            \
        { widlref_##itf (x+COM_VTBL_SYMBOL); }  \
    const itf##Vtbl

#endif /* __INCcomBase_h */




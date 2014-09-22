/* NdrTypes.cpp - VxDCOM NDR marshaling types */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
02p,10jan02,nel  Conditional define out old sytle double handling for ARM for
                 Veloce.
02o,09jan02,nel  Fix duplicate double definition in ARM specific code.
02n,17dec01,nel  Add include symbol for diab.
02m,12oct01,nel  Add extra padding to other VARIANT types.
02l,01oct01,nel  SPR#69557. Add extra padding bytes to make VT_BOOL type work.
02k,02aug01,dbs  add [v1_enum] support
02j,24jul01,dbs  take fix for SPR#65311, unnecessary align() in struct
                 marshal2
02i,19jul01,dbs  fix include path of vxdcomGlobals.h
02h,13jul01,dbs  fix up includes
02g,30mar01,nel  SPR#35873. Modify extra pading bytes on the end of a
                 SAFEARRAY.
02f,08feb01,nel  SPR#63885. SAFEARRAYs added.   
02e,19sep00,nel  Make widlMarshal match dcomProxy def.
02d,24aug00,dbs  fix many OPC-related SPRs, copied over from T2 VxDCOM
02c,26jul00,dbs  add enum_t descriptor
02b,18jul00,dbs  add CV-array NDR type
02a,28feb00,dbs  fix (properly) pointer-unmarshaling bug
01z,28feb00,dbs  fix nasty bug when unmarshaling arrays of pointers
01y,18feb00,dbs  change signature of widlMarshal() to take const void*
01x,07feb00,dbs  simplify NdrType classes, enhance marshaling of arrays to
                 support all kinds
01w,14oct99,dbs  apply fix for ARM double format
01v,23sep99,dbs  add final parts of VARIANT marshaling
01u,16sep99,dbs  marshaling enhancements, part 2
01t,14sep99,dbs  add VARIANT, pointer and string types - first stage
01s,25aug99,dbs  correct marshaling of interface ptrs
01r,19aug99,aim  changed assert to VXDCOM_ASSERT
01q,16aug99,dbs  add variant and string support
01p,13aug99,aim  include vxdcomGlobals
01o,12aug99,dbs  improve struct support
01n,05aug99,dbs  fix warnings
01m,30jul99,dbs  tighten up type-safety of NDR types
01l,29jul99,dbs  make cstruct pointer bindings same for mshl and unmshl
01k,17jun99,aim  changed assert to assert
01j,17jun99,dbs  change to COM_MEM_ALLOC
01i,25may99,dbs  dynamically allocate IStream for marshaling
                 interfaces, remove NDRTYPES::alloc/free
01h,24may99,dbs  add BSTR marshaling policy variable
01g,24may99,dbs  fix interface-ptr marshaling
01f,20may99,dbs  fix BSTR marshaling functions for improved BSTR format
01e,20may99,dbs  improve documentation
01d,18may99,dbs  optimise unmarshaling by copying only pointers when possible
01c,17may99,dbs  remove dummy-msg object, add documentation
01b,14may99,dbs  fix alignment of array counts
01a,12may99,dbs  created

*/


/*
  DESCRIPTION: NdrTypes.cpp

  NDR Type Descriptors
  --------------------
  
  This module implements the 'type descriptor' system for NDR
  marshaling, in conjunction with the NdrMarshalStream and
  NdrUnmarshalStream classes.

  The NDR stream classes are unidirectional streams that can either be
  marshaled into, or can be unmarshaled from. The type-descriptor
  system described here implements some classes which know about
  marshaling one specific IDL or C++ type into one of those
  streams. In fact, each type-descriptor know about marshaling either
  one complex type (like GUID) or one or more simple types, all of
  which are the same size, for example, 'short' and 'unsigned short'
  are handled by the same type descriptor class.

  All type descriptors are subclasses of the abstract base class
  'NdrType' and the WIDL (un)marshaling functions widlMarshal() and
  widlUnmarshal() require a pointer to the appropriate kind of stream,
  and a pointer to an NdrType which describes the item to be
  (un)marshaled. All user-level marshaling (i.e. that which appears in
  the generated proxy/stub code) uses these 2 functions.

  Each proxy or stub function must include a local variable of type
  NDRTYPES. This object functions as a 'factory' for the type
  descriptors, and implements a fast allocation strategy using
  placement new. Whenever a variable needs to be marshaled, then one
  of the methods of the NDRTYPES factory class is called, to generate
  a type descriptor. The descriptors are 'nestable' in that recursive
  data types can be described, such as structures or arrays. The set
  of type descriptor classes covers arrays (both fixed-size and
  conformant) and structures (again, fixed or conformant) even though
  WIDL doesn't currently cope with these complex types.

  Each type descriptor can be 'bound' to a particular instance
  variable, by passing the variable's address into the factory
  method. Only the top-level descriptor needs to be bound to the
  variable, and it will propogate down to nested descriptors. The
  value that needs to be bound to the descriptor is usually the
  address of the variable to be (un)marshaled. Arrays and structures
  require the descriptor to be bound to the start-address, e.g. for an
  array of T, the descriptor would be:-

  T* aT;
  array_t (aT, ...);

  and for a structure (conformant or normal) of type T, it would be
  like this:-

  struct T t;
  struct_t (&t, ...);
  
  Generally, the factory methods (of the NDRTYPES class) require the
  address of a variable to bind to. This precludes marshaling
  immediate values, but this is not generally a problem. However,
  there are exceptions, which must be coded carefully (and will be
  handled by WIDL in future).


  IDL Syntax and its meaning
  --------------------------

  DCE IDL allows many ways of achieving the same thing, and is
  particularly confusing when it comes to args which are pointers or
  arrays. There are three kinds of pointer-attributes, namely [ref],
  [unique] and [ptr] (which is know as 'full' also). The [ref] pointer
  type is effectively a pass-by-reference pointer, which can never be
  NULL (like a C++ reference type) and so does not require any
  marshaling. However, both [unique] and [ptr] pointers can be NULL
  and so must be marshaled (the marshaled value is known as a
  'referent' and is usually the value of the pointer itself, but
  effectively only conveys the NULL-ness or otherwise of the
  pointer). Top-level pointers (i.e. those in method arguments, at the
  outermost level) default to [ref] unless a specific attribute is
  given in the IDL. Lower level pointers (i.e. those inside structures
  or arrays, or even the 2nd level of indirection in method args)
  default to [unique] and so require marshaling. The pointer_t
  descriptor represents these kinds of pointers, and so it appears
  frequently.

  Pointers, as well as having one the above three attributes, can also
  be 'sized' -- i.e. can represent arrays as they can in C. For
  example, if the IDL says:-

  typedef struct
    {
    long a;
    char b;
    } FOO;
    
  HRESULT FooBar ([in] long nFoos, [in, size_is(nFoos)] FOO* foos);

  then the variable 'foos' is a 'conformant array' whose length is
  given (at runtime) in the arg 'nFoos'. DCE IDL also allows for
  returning conformant arrays, but the syntax for arrays of simple
  types, and arrays of complex types, is different.

  To return an array of simple types, the IDL would be something like
  this:-

  HRESULT Quz ([in] long nBytes, [out, size_is(nBytes)] char* bytes);
  
  To return an array of fixed-size structures, like FOO defined above,
  then the syntax is different, like so:-
  
  HRESULT Func ([in] long num, [out, size_is(,num)] FOO** ppFoos);

  In this case, the first pointer (rightmost *) defaults to [ref], and
  the 2nd or inner pointer defaults to [unique], so this declaration
  should be read as:-

  (ref-ptr to (unique-array of dimension 'num' of (FOO)))
  
  In this case, the proxy allocates memory to hold the array of FOOs
  and stores the address of this block in the pointer pointed-to by
  the argument 'ppFoos'. The strange syntax size_is(,num) indicates
  that the size_is value binds to the second star, and so this means a
  'pointer to an array of FOOs'. Calling the function in C would look
  like this:-

  FOO* foos;

  Func (3, &foos);

  COM_MEM_FREE (foos);

  where Func() is actually calling the proxy, which makes the ORPC and
  returns the array of 3 FOO structures directly into the caller's
  address space.

  The final cases cover returning conformant structures, that is a
  structure whose final member is a conformant array. To return a
  single conformant structure, the IDL will be something like:-

  typedef struct
    {
    long                    nBytes;
    [size_is(nBytes)] char  data[];
    } BAR;

  HRESULT BarBaz ([out] BAR** ppBar);

  where ppBar is a ref-ptr to a unique-ptr to BAR. There is a subtle
  difference between this case and the case where the out-arg is an
  array of conformant-structures, like the BAR structure defined
  above. If the signature were:-

  HRESULT Func ([in] long num, [out, size_is(num)] BAR** ppBars);

  then this would mean 'ref-array of unique-pointer to BAR'
  and the size_is attribute applies to the top-level * declarator,
  i.e. ppBar is itself an array.

  
  NDR Phases
  ----------

  There are 4 phases of marshaling in ORPC : firstly, marshaling all
  the [in] args at the proxy; secondly, unmarshaling all the [in] args
  at the stub; thirdly, marshaling all the [out] args and the return
  value at the stub; and finally unmarshaling all the [out] args and
  the return value at the proxy.

  In each of these phases there are different requirements on the type
  descriptors in order to optimise the unmarshaling process,
  minimising dynamic memory allocations. Also, at the stub side, the
  stub function must provide the actual arguments to the real
  stub-function when it is invoked - this requires making a temporary
  local for each argument (whether [in] or [out]) and unmarshaling
  into (and then marshaling from) those variables.

  It can be seen from the descriptions of the IDL syntax above that
  the possibilities are numerous, and the right type of allocation
  strategy must be performed for each variant of the syntax. This is
  why the job should ideally be performed by an IDL compiler, and
  in future WIDL will do just that.

  This description will document the behaviour of the type-descriptor
  classes in each of the above cases, and recommend how they should be
  handled by the generated proxy/stub code.

  
  The 'conformant array' Type Descriptor
  --------------------------------------

  The factory method NDRTYPES::carray_t() returns a type descriptor
  designed to cope with conformant arrays. Marshaling conformant
  arrays is always the same, in both the PROXY_MSHL and STUB_MSHL
  phases. However, unmarshaling is a different matter.
  
  At the STUB_UNMSHL phase, unmarshaling can be optimised by letting
  the temporary variable in the stub-function point into the
  unmarshaling stream's buffer. To achieve this, the type-descriptor
  must be bound to the address of the pointer-variable, so it
  manipulate the contents.

  Unmarshaling at the PROXY_UNMSHL stage is the most complex. If the
  array-element type is a simple type, then the descriptor should be
  bound to the start-address of the array, and should just unmarshal
  directly into the caller-provided space. If the array-element is a
  fixed-size structure and the size_is(,num) IDL syntax is being used,
  then the type-descriptor must allocate dynamic memory to hold the
  unmarshaled data, which the caller must eventually free (by calling
  the CoTaskMemFree() function). This functionality is encoded in
  NdrConfArray::unmarshal(), which makes the distinction between
  simple types and others.

  However, if the [out] array is an array of pointers to conformant
  structures (see the Func(long,BAR**) example above) then the
  complexity increases. The array is marshaled differently (the array
  of pointer-referents is marshaled first, then each of the conformant
  structures follows, in order, and NULL referents may be present) and
  hence unmarshaling is different.

  Currently, the type descriptors do not support this case -- it must
  be hand-coded (by human hands, or by WIDL concocting the right
  sequence of elements) -- and so only cope with the 2 cases outlined
  earlier.

  Variable-Binding Rules
  ----------------------

  Most of the NDR type-descriptors bind to the address of a variable,
  whether marshaling or unmarshaling. The only exception is the
  'array' type (class NdrArray) which binds to the start of the array,
  i.e. the address of the first element. Note that conformant arrays
  don't -- they follow the usual rules and bind to the address of a
  variable holding the pointer to the first element of the array.

  Pointer Types
  -------------
  
  MIDL/WIDL supports 3 kinds of pointer-attributes, namely [ref],
  [unique] and [ptr] (known as 'full') pointers. The difference
  between unique and full pointers is that unique pointers must
  literally be unique, i.e. there must be no aliasing between any 2
  pointers within the context of a marshaling operation.

  The [ref] pointer type indicates 'pass by reference' semantics, thus
  a ref-pointer can never be NULL, and so [ref] pointers are not
  marshaled. However, [unique] pointers can be NULL, and so do have to
  be marshaled. Thus the type-descriptors for the pointer type (and
  associated array and string types) are only used when the IDL
  indicates a [unique] (or full) pointer. The pointer_t() descriptor
  is generated by WIDL when a simple [unique] pointer is required,
  i.e. an unsized, non-string pointer. If a dimension is supplied, or
  the [string] attribute is used, then WIDL will generate a call to
  either the carray_t() or wstring_t() descriptor, respectively.

  In all cases where a [unique] pointer is utilised, the binding for
  the descriptor is the address of the pointer variable. Only if a
  [ref] string or fixed-size array is used as a method-arg is there a
  potential problem.

  Also, WIDL only supports conformant arrays, and not varying, or
  conformant varying arrays. In IDL terms, this means the size_is
  attribute is recognised, but max_is, length_is, etc, are not.

  
*/

#include <stdlib.h>
#include "private/comMisc.h"
#include "NdrTypes.h"
#include "MemoryStream.h"
#include "orpcLib.h"
#include "private/vxdcomGlobals.h"
#include "TraceCall.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_NdrTypes (void)
    {
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//
// ARM-specific code for (un)marshaling doubles...
//

#if defined (CPU_FAMILY) && (CPU_FAMILY == ARM) && \
    defined (VXDCOM_PLATFORM_VXWORKS) && (VXDCOM_PLATFORM_VXWORKS != 5)

#ifdef __GNUC__
template class NdrSimple<double>;
#endif

union armDouble
    {
    double        doubleValue;
    struct
        {
        long        msw;
        long        lsw;
        }        txValue;
    };

static HRESULT marshalDouble (NdrMarshalStream * pStrm, double * value)
    {
    pStrm->align (sizeof (double));

    armDouble d;
    d.doubleValue = *value;

    pStrm->insert (sizeof (long), &d.txValue.lsw, true);
    return pStrm->insert (sizeof (long), &d.txValue.msw, true);
    }

HRESULT NdrSimple<double>::marshal1 (NdrMarshalStream* pStrm)
    {
    return marshalDouble(pStrm, m_pValue);
    }

static HRESULT unmarshalDouble (NdrUnmarshalStream * pStrm, double * value)
    {
    pStrm->align (sizeof (double));

    armDouble d;

    pStrm->extract (sizeof (long), &d.txValue.lsw, true);
    HRESULT hr = pStrm->extract (sizeof (long), &d.txValue.msw, true);

    if (SUCCEEDED (hr))
        *value = d.doubleValue;

    return hr;
    }
    
HRESULT NdrSimple<double>::unmarshal1 (NdrUnmarshalStream* pStrm)
    {
    return unmarshalDouble(pStrm, m_pValue);
    }

#else
static HRESULT marshalDouble (NdrMarshalStream * pStrm, double * value)
    {
    pStrm->align (sizeof (double));
    return pStrm->insert (sizeof (double), value, true);
    }

static HRESULT unmarshalDouble (NdrUnmarshalStream * pStrm, double * value)
    {
    pStrm->align (sizeof (double));
    return pStrm->extract (sizeof (double), value, true);
    }
#endif

//////////////////////////////////////////////////////////////////////////
//
// (un)marshaling methods for enum types.
//

size_t NdrEnum::alignment () const
    {
    return (m_bV1Enum ? sizeof (DWORD) : sizeof (short));
    }

HRESULT NdrEnum::marshal1 (NdrMarshalStream* pStrm)
    {
    HRESULT hr = S_OK;
    
    if (m_bV1Enum)
        {
        // New-style 32-bit enum...
        DWORD val = *m_pValue;
        pStrm->align (alignment ());
        hr = pStrm->insert (sizeof (val), &val, true);
        }
    else
        {
        // Old-style 16-bit enum...
        short val = *m_pValue;
        pStrm->align (alignment ());
        hr = pStrm->insert (sizeof (val), &val, true);
        }

    return hr;
    }
    
HRESULT NdrEnum::unmarshal1 (NdrUnmarshalStream* pStrm)
    {
    HRESULT hr = pStrm->align (alignment ());
    if (FAILED (hr))
        return hr;

    if (m_bV1Enum)
        {
        // New-style 32-bit enum...
        DWORD val;
        hr = pStrm->extract (sizeof (val), &val, true);
        if (FAILED (hr))
            return hr;
        *m_pValue = (DUMMY) val;
        }
    else
        {
        // Old-style 16-bit enum...
        short val;
        hr = pStrm->extract (sizeof (val), &val, true);
        if (FAILED (hr))
            return hr;
        *m_pValue = (DUMMY) val;
        }

    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//

void* NdrType::operator new (size_t n, NdrTypeFactory* pf)
    {
    return pf->allocate (n);
    }

//////////////////////////////////////////////////////////////////////////
//

void NdrType::operator delete (void*)
    {}

//////////////////////////////////////////////////////////////////////////
//

NdrTypeFactory::NdrTypeFactory (int hint)
    {
    TRACE_CALL;

    size_t nBytes = hint * TYPESIZE;
    m_begin = new char [nBytes];
    m_curr = m_begin;
    m_end = m_begin + nBytes;
    }


//////////////////////////////////////////////////////////////////////////
//

NdrTypeFactory::~NdrTypeFactory ()
    {
    TRACE_CALL;

    if (m_begin)
        delete [] m_begin;
    }

//////////////////////////////////////////////////////////////////////////
//

void* NdrTypeFactory::allocate (size_t nb)
    {
    COM_ASSERT((m_end - m_curr) > (int) nb);
    if ((m_end - m_curr) < (int) nb)
        return 0;
    
    void* pv = m_curr;
    m_curr += nb;
    return pv;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrType base-class default implementations of the marshaling
// functions. Derived classes should put their main marshaling code in
// marshal1() (and correspondingly, their main unmarshaling code in
// unmarshal1()), the default implementations of marshal() and
// unmarshal() simply call 1 then 2. This potential for splitting the
// marshaling into 2 stages allows the marshaling of [unique] pointers
// inside structures and arrays to work correctly.
//

HRESULT NdrType::marshal2 (NdrMarshalStream*)
    {
    return S_OK;
    }

HRESULT NdrType::unmarshal2 (NdrUnmarshalStream*)
    {
    return S_OK;
    }

HRESULT NdrType::marshal (NdrMarshalStream* pStrm)
    {
    HRESULT hr = marshal1 (pStrm);
    if (FAILED (hr))
        return hr;
    return marshal2 (pStrm);
    }

HRESULT NdrType::unmarshal (NdrUnmarshalStream* pStrm)
    {
    HRESULT hr = unmarshal1 (pStrm);
    if (FAILED (hr))
        return hr;
    return unmarshal2 (pStrm);
    }

void NdrType::arraySizeSet (int n)
    {
    m_ndrtypes.arraySize = n;
    }

int NdrType::arraySizeGet ()
    {
    return m_ndrtypes.arraySize;
    }

//////////////////////////////////////////////////////////////////////////
//

size_t NdrStruct::size (NdrUnmarshalStream* pStrm)
    {
    TRACE_CALL;
    
    size_t        nSize = 0;

    for (size_t n=0; n < m_nMembers; ++n)
        nSize += m_pMemberInfo [n].pType->size (pStrm);

    return nSize;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrStruct::alignment -- the alignment of a structure is the largest
// alignment() value of any of its members...
//

size_t NdrStruct::alignment () const
    {
    TRACE_CALL;
    
    size_t        nAlign = 0;

    for (size_t i=0; i < m_nMembers; ++i)
        {
        size_t n = m_pMemberInfo [i].pType->alignment ();
        if (n > nAlign)
            nAlign = n;
        }

    return nAlign;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrStruct::init -- record structure info, including optionally the
// index of the structure-member which holds the [size_is] value for
// dynamically-sized structure members. This index will be -1 if there
// is no dynamically-sized member in the structure.
//

void NdrStruct::init (size_t n, const NdrMemberInfo members [], int nsize_is)
    {
    TRACE_CALL;
    
    m_nMembers = n;
    m_pMemberInfo = new NdrMemberInfo [n];
    for (size_t i=0; i < n; ++i)
        m_pMemberInfo[i] = members[i];
    m_nSizeIs = nsize_is;
    m_pInstance = 0;
    }

//////////////////////////////////////////////////////////////////////////
//

NdrStruct::~NdrStruct ()
    {
    if (m_pMemberInfo)
        delete [] m_pMemberInfo;
    }


//////////////////////////////////////////////////////////////////////////
//
// NdrStruct::bind -- bind to a new instance of the type we
// represent. We defer the binding of individual structure members
// until the last minute...
//

void NdrStruct::bind (void* pv)
    {
    TRACE_CALL;
    
    m_pInstance = pv;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrConfStruct::confElemResize -- calculate the actual value of the
// 'size_is' attribute, given its index in the m_nSizeIs member, and
// resize the appropriate structure-member to this value. This
// method should *NOT* be called if this index is negative, i.e. there
// is no size_is attribute...
//

size_t NdrConfStruct::confElemResize ()
    {
    COM_ASSERT (m_nSizeIs >= 0);
    
    char* addr = (char*) m_pInstance;
    
    // find the value of the 'conformance' variable
    NdrMemberInfo* pConf = &m_pMemberInfo [m_nSizeIs];
    pConf->pType->bind (addr + pConf->nOffset);
    unsigned long nArrayCount = pConf->pType->value ();

    // resize the varying-length element so it knows to marshal 
    // the right number of items (if applicable)...
    m_pMemberInfo [m_nMembers - 1].pType->resize (nArrayCount);
    
    return nArrayCount;
    }


//////////////////////////////////////////////////////////////////////////
//
// NdrStruct::marshal1 -- call marshal1() for all the members...
//

HRESULT NdrStruct::marshal1 (NdrMarshalStream* pStrm)
    {
    TRACE_CALL;
    
    // find the start of the current instance
    char* addr = (char*) m_pInstance;
    
    // align
    pStrm->align (alignment ());

    // call marshal1() for each element 
    for (size_t n=0; n < m_nMembers; ++n)
        {
        m_pMemberInfo [n].pType->bind (addr + m_pMemberInfo [n].nOffset);
        HRESULT hr = m_pMemberInfo [n].pType->marshal1 (pStrm);
        if (FAILED (hr))
            return hr;
        }
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrStruct::marshal2 -- call marshal2() for all the members. Any
// members which have a valid 'size_is' indicator must be resized to
// the value nominated in their member-descriptor. This information is
// provided for all struct and cstruct members which have a [size_is]
// attribute present...
//

HRESULT NdrStruct::marshal2 (NdrMarshalStream* pStrm)
    {
    TRACE_CALL;
    
    char* addr = (char*) m_pInstance;
    
    // call marshal2() for each element 
    for (size_t n=0; n < m_nMembers; ++n)
        {
        NdrMemberInfo& member = m_pMemberInfo [n];

        // Does it have a [size_is] attribute...
        if (member.nSizeIs >= 0)
            {
            NdrMemberInfo& sizeMem = m_pMemberInfo [member.nSizeIs];
            sizeMem.pType->bind (addr + sizeMem.nOffset);
            size_t size_is = sizeMem.pType->value ();
            member.pType->resize (size_is);
            }

        // Now marshal the actual member...
        member.pType->bind (addr + member.nOffset);
        HRESULT hr = member.pType->marshal2 (pStrm);
        if (FAILED (hr))
            return hr;
        }
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrStruct::unmarshal1 -- unmarshal all the structure members one by
// one from the stream. The repeated operation mirrors the marshaling
// side... 
//

HRESULT NdrStruct::unmarshal1 (NdrUnmarshalStream* pStrm)
    {
    TRACE_CALL;
    
    S_DEBUG (LOG_DCOM, "U1:struct @ " << m_pInstance);

    // find the start of the current instance
    char* addr = (char*) m_pInstance;
    
    // align
    pStrm->align (alignment ());

    // call unmarshal1() for each element
    for (size_t n=0; n < m_nMembers; ++n)
        {
        m_pMemberInfo [n].pType->bind (addr + m_pMemberInfo [n].nOffset);
        HRESULT hr = m_pMemberInfo [n].pType->unmarshal1 (pStrm);
        if (FAILED (hr))
            return hr;
        }
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrStruct::unmarshal2 -- call unmarshal2() for all the members...
//

HRESULT NdrStruct::unmarshal2 (NdrUnmarshalStream* pStrm)
    {
    TRACE_CALL;
    
    char* addr = (char*) m_pInstance;
    
    S_DEBUG (LOG_DCOM, "U2:struct @ " << m_pInstance);

    // call unmarshal2() for each element
    for (size_t n=0; n < m_nMembers; ++n)
        {
        m_pMemberInfo [n].pType->bind (addr + m_pMemberInfo [n].nOffset);
        HRESULT hr = m_pMemberInfo [n].pType->unmarshal2 (pStrm);
        if (FAILED (hr))
            return hr;
        }
    return S_OK;
    }


//////////////////////////////////////////////////////////////////////////
//
// NdrConfStruct::marshal1 -- marshal all the structure members one by
// one into the stream, preceded by the array length. On entry, this
// descriptor is bound to the start of the structure...
//

HRESULT NdrConfStruct::marshal1 (NdrMarshalStream* pStrm)
    {
    TRACE_CALL;

    // Always calculate dynamic size for conf-structs...
    unsigned long nArrayCount = confElemResize ();
    
    // marshal array-count first, as 4-byte value (unsigned long)
    pStrm->align (4);
    HRESULT hr = pStrm->insert (4, &nArrayCount, true);
    if (FAILED (hr))
        return hr;

    S_DEBUG (LOG_DCOM, "M1:cstruct:size_is=" << nArrayCount);
    
    // marshal structure member-wise
    return NdrStruct::marshal1 (pStrm);
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrConfStruct::unmarshal1 -- unmarshal all the structure members one
// by one from the stream, preceded by the array-length of the last
// member. The m_pInstance pointer is pointing to the start of space
// allocated to hold the structure itself...
//

HRESULT NdrConfStruct::unmarshal1 (NdrUnmarshalStream* pStrm)
    {
    TRACE_CALL;
    
    S_DEBUG (LOG_DCOM, "U1:cstruct");
    
    // unmarshal structure member-wise
    return NdrStruct::unmarshal1 (pStrm);
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrConfStruct::size -- during unmarshaling, extract the required
// size of this structure (accounting for the variable-length array)
// from the unmarshaling stream, and modify the descriptor for that
// array to hold the discovered size.
//

size_t NdrConfStruct::size (NdrUnmarshalStream* pStrm)
    {
    TRACE_CALL;
    
    // unmarshal array-count first, as 4-byte value (unsigned long)
    unsigned long nArrayCount;
    pStrm->align (4);
    HRESULT hr = pStrm->extract (4, &nArrayCount, true);
    if (FAILED (hr))
        return hr;

    S_DEBUG (LOG_DCOM, "SZ:cstruct:size_is=" << nArrayCount);
    
    // resize the varying-length element so it knows to unmarshal 
    // the right number of items (if applicable)...
    m_pMemberInfo [m_nMembers - 1].pType->resize (nArrayCount);

    // Now add up the size of all elements...
    size_t nSize = 0;
    for (size_t n=0; n < m_nMembers; ++n)
        nSize += m_pMemberInfo [n].pType->size (pStrm);

    return nSize;
    }
 
//////////////////////////////////////////////////////////////////////////
//

void NdrArray::init
    (
    const NdrTypeDesc&        elementType,
    size_t                elemSize,
    size_t                max,
    size_t                offset,
    size_t                len
    )
    {
    TRACE_CALL;
    
    m_pElementType = elementType;
    m_nElementSize = elemSize;
    m_arraySize = len;
    m_offset = offset;
    m_max = max;
    m_ptr = 0;
    }

//////////////////////////////////////////////////////////////////////////
//

size_t NdrArray::size (NdrUnmarshalStream*)
    {
    return m_arraySize * m_nElementSize;
    }

//////////////////////////////////////////////////////////////////////////
//

size_t NdrArray::alignment () const
    {
    return m_pElementType->alignment ();
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrArray::bind -- bind to an actual array of whatever type we are
// dealing with...
//

void NdrArray::bind (void* pv)
    {
    TRACE_CALL;
    
    // Bind to instance address...
    m_ptr = pv;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrArray::marshal1 -- marshal the array into the stream...
//

HRESULT NdrArray::marshal1 (NdrMarshalStream* pStrm)
    {
    TRACE_CALL;
    
    // locate start of array in user memory
    char* pmem = (char*) m_ptr;

    // align
    pStrm->align (alignment ());
    
    // call marshal1() for individual elements
    for (size_t n=0; n < m_arraySize; ++n)
        {
        m_pElementType->bind (pmem);
        HRESULT hr = m_pElementType->marshal1 (pStrm);
        if (FAILED (hr))
            return hr;
        pmem += m_nElementSize;
        }

    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrArray::marshal2 -- call marshal2() for each element
//

HRESULT NdrArray::marshal2 (NdrMarshalStream* pStrm)
    {
    TRACE_CALL;
    
    // locate start of array in user memory
    char* pmem = (char*) m_ptr;

    // align
    pStrm->align (alignment ());
    
    // call marshal2() for individual elements
    for (size_t n=0; n < m_arraySize; ++n)
        {
        m_pElementType->bind (pmem);
        HRESULT hr = m_pElementType->marshal2 (pStrm);
        if (FAILED (hr))
            return hr;
        pmem += m_nElementSize;
        }

    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrArray::unmarshal1 -- unmarshal the fixed-size array from the
// stream, assuming that the memory to hold the array already
// exists...
//

HRESULT NdrArray::unmarshal1 (NdrUnmarshalStream* pStrm)
    {
    TRACE_CALL;
    
    // locate the start of the array
    char* pmem = (char*) m_ptr;

    S_DEBUG (LOG_DCOM, "U1:array");
    
    // align
    pStrm->align (alignment ());
    
    // call unmarshal1() for elements one at a time
    for (size_t n=0; n < m_arraySize; ++n)
        {
        m_pElementType->bind (pmem);
        HRESULT hr = m_pElementType->unmarshal1 (pStrm);
        if (FAILED (hr))
            return hr;
        pmem += m_nElementSize;
        }
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrArray::unmarshal2 -- call unmarshal2() for each element...
//

HRESULT NdrArray::unmarshal2 (NdrUnmarshalStream* pStrm)
    {
    TRACE_CALL;
    
    // locate the start of the array
    char* pmem = (char*) m_ptr;

    S_DEBUG (LOG_DCOM, "U2:array");
    
    // align
    pStrm->align (alignment ());
    
    // call unmarshal2() for elements one at a time
    for (size_t n=0; n < m_arraySize; ++n)
        {
        m_pElementType->bind (pmem);
        HRESULT hr = m_pElementType->unmarshal2 (pStrm);
        if (FAILED (hr))
            return hr;
        pmem += m_nElementSize;
        }
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//

size_t NdrConfArray::size (NdrUnmarshalStream* pStrm)
    {
    // unmarshal array-count first, as 4-byte value (unsigned long)
    unsigned long nElements;
    pStrm->align (4);
    HRESULT hr = pStrm->extract (4, &nElements, true);
    if (FAILED (hr))
        return hr;
    
    S_DEBUG (LOG_DCOM, "SZ:carray:size_is=" << nElements);
    
    // In NDR rules for arrays, 'conformance' value as found here does
    // not have to be emitted to a variable - the argument that indicates
    // the length will be marshaled separately. So, all we do is set
    // the size of the array to be unmarshaled, and treat it as 'fixed
    // length' from then on...

    m_arraySize = nElements;

    // Return total size of array...
    return m_arraySize * m_nElementSize;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrConfArray::marshal1 -- marshal the array into the stream. On
// entry, the descriptor is bound to the start of the array...
//

HRESULT NdrConfArray::marshal1 (NdrMarshalStream* pStrm)
    {
    TRACE_CALL;
    
    // marshal array-count first, as 4-byte value (unsigned long)
    unsigned long nElements = m_arraySize;
    pStrm->align (4);
    HRESULT hr = pStrm->insert (4, &nElements, true);
    if (FAILED (hr))
        return hr;

    // Defer to base-class...
    return NdrArray::marshal1 (pStrm);
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrConfArray::unmarshal1 -- unmarshal the array from the stream...
//

HRESULT NdrConfArray::unmarshal1 (NdrUnmarshalStream* pStrm)
    {
    TRACE_CALL;
    
    S_DEBUG (LOG_DCOM, "U1:carray");
    
    // treat as a normal array now...
    return NdrArray::unmarshal1 (pStrm);
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrConfVarArray::size -- returns size of memory needed to hold
// unmarshaled array. It unmarshals the 'max' count, then the offset
// (which should always be zero), then defers to the base class
// (NdrConfArray) to unmarshal the actual transmitted size. This is
// the value we return, since this is the amount of memory we would
// need to allocate...
//

size_t NdrConfVarArray::size (NdrUnmarshalStream* pStrm)
    {
    // unmarshal array-max and offset first, as 4-byte values
    // (unsigned long)...
    
    pStrm->align (4);
    pStrm->extract (4, &m_max, true);
    HRESULT hr = pStrm->extract (4, &m_offset, true);
    if (FAILED (hr))
        return hr;

    // Return transmitted size of array...
    return NdrConfArray::size (pStrm);
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrConfVarArray::marshal1 -- marshal the array into the stream. On
// entry, the descriptor is bound to the start of the array...
//

HRESULT NdrConfVarArray::marshal1 (NdrMarshalStream* pStrm)
    {
    TRACE_CALL;
    
    // marshal array-max, and zero-offset first, as 4-byte values
    // (unsigned long)...
    pStrm->align (4);
    pStrm->insert (4, &m_max, true);
    HRESULT hr = pStrm->insert (4, &m_offset, true);
    if (FAILED (hr))
        return hr;

    // Defer to base-class to marshal the actual runtime size of the
    // array, plus the array contents...
    return NdrConfArray::marshal1 (pStrm);
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrInterface::bind -- bind to an instance of an interface
// pointer. The address in 'pv' is always the address of the
// pointer-variable itself.

void NdrInterface::bind (void* pv)
    {
    TRACE_CALL;
    
    m_pPointer = (IUnknown**) pv;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrInterface::marshal1 -- marshal the interface-pointer, it goes
// into a MInterfacePointer conformant structure before being sent
// over the wire...
//

HRESULT NdrInterface::marshal1 (NdrMarshalStream* pStrm)
    {
    TRACE_CALL;

    // Get hold of the interface ptr...
    IUnknown* punk = * (IUnknown**) m_pPointer;

    // We need to marshal a 'pointer referent' first...
    pStrm->align (4);
    HRESULT hr = pStrm->insert (sizeof (long), &punk, true);

    return hr;
    }

HRESULT NdrInterface::marshal2 (NdrMarshalStream* pStrm)
    {
    HRESULT hr = S_OK;
    IUnknown* punk = * (IUnknown**) m_pPointer;

    // If non-NULL pointer, we must marshal pointer/OBJREF...
    if (punk)
        {
        // Use a 'memory stream' to marshal the interface into
        VxRWMemStream* pMemStrm = new VxRWMemStream;
        IStream* pIStream=0;

        // QI for IStream, this gets us one reference...
        hr = pMemStrm->QueryInterface (IID_IStream,
                                       (void**) &pIStream);
        if (FAILED (hr))
            return hr;
    
        // marshal the interface-ptr
        hr = CoMarshalInterface (pIStream, m_iid, punk, 0, 0, 0);
        if (SUCCEEDED (hr))
            {
            // okay we got it - now align the stream and put the
            // marshaling packet into there, in the shape of a
            // MInterfacePointer structure, i.e. conformance-value
            // (length), then ulCntData, then abData[] containing the
            // actual packet...
            ULONG ulCntData = pMemStrm->size ();
        
            pStrm->insert (sizeof (ulCntData), &ulCntData, true);
            pStrm->insert (sizeof (ulCntData), &ulCntData, true);
            hr = pStrm->insert (ulCntData, pMemStrm->begin (), false);
            }

        pMemStrm->Release ();
        }
    
    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrInterface::unmarshal -- see marshal() method for details of
// formatting of a marshaled interface pointer...
// A cosutom marsh routine is required so that unmarshal2 can be used by
// the SAFEARRAY routines.
//

HRESULT NdrInterface::unmarshal (NdrUnmarshalStream* pStrm)
    {
    HRESULT hr = unmarshal1 (pStrm);

    // Check to see if the ptr is NULL and stop if it is,
    if (hr == S_FALSE)
        {
        return S_OK;
        }
    
    if (FAILED (hr))
        return hr;

    return unmarshal2 (pStrm);
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrInterface::unmarshal1 -- see marshal() method for details of
// formatting of a marshaled interface pointer...
//

HRESULT NdrInterface::unmarshal1 (NdrUnmarshalStream* pStrm)
    {
    TRACE_CALL;
    
    // align the buffer before we begin
    HRESULT hr = pStrm->align (4);

    // unmarshal the conformance-value (length), then the conf-array
    // member 'ulCntData' (which will be the same) and then the array
    // of bytes...
    
    // need to extract 'ptr referent' first...

    ULONG pp;
    hr = pStrm->extract (sizeof (pp), &pp, true);

    // If ptr is NULL, thats okay, but we have nothing else to do so
    // return a dummy code to unmarshal to indicate this.
    if (pp == 0)
        return S_FALSE;

    return hr;
    }

    
HRESULT NdrInterface::unmarshal2 (NdrUnmarshalStream* pStrm)
    {
    TRACE_CALL;

    HRESULT     hr;

    // Use a 'memory stream' to hold the marshaling packet, and make
    // sure we don't try to delete it from the stack!
    VxRWMemStream memStrm;
    memStrm.AddRef ();

    // Now extract the actual MInterfacePointer...
    ULONG length;
    hr = pStrm->extract (sizeof (length), &length, true);
    if (SUCCEEDED (hr))
        {
        ULONG ulCntData;
        hr = pStrm->extract (sizeof (ulCntData), &ulCntData, true);
        if (SUCCEEDED (hr))
            {
            COM_ASSERT(length == ulCntData);

            // get the bytes from the unmarshaling stream into the
            // memory-stream, ready for CoUnmarshalInterface()...
            
            memStrm.insert (pStrm->curr (), ulCntData);
            hr = pStrm->extract (memStrm.locationGet (), 0, false);
            if (SUCCEEDED (hr))
                {
                // unmarshal the interface
                memStrm.locationSet (0);
                hr = CoUnmarshalInterface (static_cast<IStream*> (&memStrm),
                                           m_iid,
                                           (void**) m_pPointer);
                }
            }
        }
    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrBSTR::marshal1/2 -- according to the MS headers (and IDL) the wire
// format of a BSTR is FLAGGED_WORD_BLOB structure -- a flags-word,
// the array-length, then the array of wide-chars. However, looking at
// real marshaled packets on the network shows another structure where
// the characters 'User' precede the string! Which is right???
// Obviously, the info snooped off the network.
//
// The format is 'User' followed by the string as a CV-array of wide
// chars... 
//

HRESULT NdrBSTR::marshal1 (NdrMarshalStream* pStrm)
    {
    // align buffer correctly first
    HRESULT hr = pStrm->align (sizeof (long));
    if (FAILED (hr))
        return hr;
    
    // now insert the chars 'User' one at a time
    char userText [] = "User";
    
    return pStrm->insert (4, userText, false);
    }

HRESULT NdrBSTR::marshal2 (NdrMarshalStream* pStrm)
    {
    long a, b, c;
    
    BSTR bstr = *m_pBstr;
    if (bstr)
        {
        b = ::SysStringByteLen (bstr);
        a = c = (b/2);
        }
    else
        {
        a = c = 0;
        b = -1;
        }
    
    // align first
    HRESULT hr = pStrm->align (sizeof (long));
    if (FAILED (hr))
        return hr;
    
    // now insert the length words
    pStrm->insert (sizeof (long), &a, true);
    pStrm->insert (sizeof (long), &b, true);
    hr = pStrm->insert (sizeof (long), &c, true);
    if (FAILED (hr))
        return hr;

    // Now insert the text -- we assume it is wide-character data,
    // unless the user-configurable policy parameter has been set,
    // which means we should treat BSTRs as byte arrays...

    if (vxdcomBSTRPolicy == 0)
        {
        // treat BSTR as array of OLECHARs, marshaling each one
        // individually so byte-ordering is preserved...
        for (long n=0; n < a; ++n)
            hr = pStrm->insert (sizeof (short), &bstr[n], true);
        }
    else
        hr = pStrm->insert (b, bstr, false);

    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrBSTR::unmarshal1 -- simply reverses the above process...
//

HRESULT NdrBSTR::unmarshal1 (NdrUnmarshalStream* pStrm)
    {
    char user [8];

    /// align first
    HRESULT hr = pStrm->align (sizeof (long));
    if (FAILED (hr))
        return hr;
    
    for (int n=0; n < 4; ++n)
        pStrm->extract (sizeof (char), &user [n], false);
    user [4] = 0;
    if (strcmp (user, "User") != 0)
        return E_UNEXPECTED;

    return S_OK;
    }

HRESULT NdrBSTR::unmarshal2 (NdrUnmarshalStream* pStrm)
    {
    /// align first
    HRESULT hr = pStrm->align (sizeof (long));
    if (FAILED (hr))
        return hr;
    
    // now unmarshal length fields
    long a, b, c;
    pStrm->extract (sizeof (long), &a, true);
    pStrm->extract (sizeof (long), &b, true);
    hr = pStrm->extract (sizeof (long), &c, true);

    // 'a' should == 'c' and 'b' is twice 'a', unless the BSTR is
    // NULL, in which case 'a' and 'c' are zero and 'b' is -1 (or
    // 0xFFFFFFFF if you prefer)...
    BSTR bstr = 0;
    if (a)
        {
        COM_ASSERT(a == c);
        COM_ASSERT(b == 2*a);

        // Get a string - remember, we are working in terms of OLECHARs
        // unless the system was built with the 'marshal as chars' policy

        bstr = SysAllocStringLen (0, a);

        // Read the string - if we are treating BSTRs as array of
        // OLECHAR then we need to marshal each wide-char individually
        // to retain byte-ordering...
        if (vxdcomBSTRPolicy == 0)
            {
            for (long n=0; n < a; ++n)
                hr = pStrm->extract (sizeof (short), &bstr [n], true);
            }
        else
            hr = pStrm->extract (b, bstr, false);
        
        // Terminate the string...
        bstr [a] = 0;
        }

    *m_pBstr = bstr;

    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//
// Strings are marshaled as conformant varying arrays, with both the
// max and the length being strlen() + 1 effectively...
//
 
HRESULT NdrWString::marshal1 (NdrMarshalStream* pStrm)
    {
    pStrm->align (4);

    WCHAR* str = m_pString;

    m_len = vxcom_wcslen (str) + 1;
    m_max = m_len;
    m_offset = 0;

    pStrm->insert (sizeof (m_max), &m_max, true);
    pStrm->insert (sizeof (m_offset), &m_offset, true);
    HRESULT hr = pStrm->insert (sizeof (m_len), &m_len, true);
    if (FAILED (hr))
        return hr;
    for (ULONG i=0; i < m_len; ++i)
        hr = pStrm->insert (sizeof(WCHAR), &str [i], true);
    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//

HRESULT NdrWString::unmarshal1 (NdrUnmarshalStream* pStrm)
    {
    HRESULT hr = S_OK;

    S_DEBUG (LOG_DCOM, "U1:wstring");
    
    WCHAR* wsz = m_pString + m_offset;
    for (ULONG i=0; i < m_len; ++i)
        hr = pStrm->extract (sizeof(WCHAR), &wsz [i], true);

    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//

size_t NdrWString::size (NdrUnmarshalStream* pStrm)
    {
    pStrm->align (4);

    pStrm->extract (sizeof (m_max), &m_max, true);
    pStrm->extract (sizeof (m_offset), &m_offset, true);
    HRESULT hr = pStrm->extract (sizeof (m_len), &m_len, true);
    if (FAILED (hr))
        return 0;

    size_t nBytes = m_max * sizeof(WCHAR);
    S_DEBUG (LOG_DCOM, "SZ:wstring:len=" << nBytes);
    
    return nBytes;
    }
 
//////////////////////////////////////////////////////////////////////////
//

HRESULT NdrCString::marshal1 (NdrMarshalStream* pStrm)
    {
    pStrm->align (4);

    m_len = strlen (m_pString) + 1;
    m_max = m_len;
    m_offset = 0;

    pStrm->insert (sizeof (m_max), &m_max, true);
    pStrm->insert (sizeof (m_offset), &m_offset, true);
    pStrm->insert (sizeof (m_len), &m_len, true);
    return pStrm->insert (m_len, m_pString + m_offset, false);
    }

//////////////////////////////////////////////////////////////////////////
//

HRESULT NdrCString::unmarshal1 (NdrUnmarshalStream* pStrm)
    {
    return pStrm->extract (m_len, m_pString + m_offset, false);
    }

//////////////////////////////////////////////////////////////////////////
//

size_t NdrCString::size (NdrUnmarshalStream* pStrm)
    {
    pStrm->align (4);

    pStrm->extract (sizeof (m_max), &m_max, true);
    pStrm->extract (sizeof (m_offset), &m_offset, true);
    HRESULT hr = pStrm->extract (sizeof (m_len), &m_len, true);
    if (FAILED (hr))
        return 0;
    return m_max;
    }
 
//////////////////////////////////////////////////////////////////////////
//
// NdrPointer::resize -- no sense in resizing the pointer, but as it
// may be pointing to an array or something, we simply pass it on...
//

void NdrPointer::resize (size_t n)
    {
    m_pointeeType->resize (n);
    }


//////////////////////////////////////////////////////////////////////////
//
// NdrPointer::marshal1 -- first-stage of marshaling a pointer. If its
// a [unique] pointer, just deal with the pointer-referent, deferring
// the pointee until marshal2() time...
//

HRESULT NdrPointer::marshal1 (NdrMarshalStream* pStrm)
    {
    // Find actual pointer value from binding...
    void* referent = *m_pPtr;
    
    if (! m_refptr)
        {
        // Its a [unique] ptr...
        pStrm->align (4);
        HRESULT hr = pStrm->insert (sizeof (long), &referent, true);
        if (FAILED (hr))
            return hr;
        }
    return S_OK;
    }

HRESULT NdrPointer::marshal2 (NdrMarshalStream* pStrm)
    {
    // Find actual pointer value from binding...
    void* referent = *m_pPtr;

    // Nothing to do if its NULL...
    if (referent == 0)
        return S_OK;
    
    // Either a [unique] ptr with non-NULL referent, or a [ref]
    // pointer, so go ahead and marshal the pointee...
    m_pointeeType->bind (referent);
    return m_pointeeType->marshal (pStrm);
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrPointer::unmarshal1/2 -- when unmarshaling pointers, we need to
// keep the pointer-value (actually, the 'referent' which is not a
// true pointer, just an indicator of NULL-ness or otherwise)
// somewhere safe. Formerly, it was kept in m_referent, but since this
// may be re-used inside, for example, an array of pointers, by many
// pointer instances, we now keep the referent in '*m_pPtr' between
// calls to unmarshal1() and unmarshal2()...
//

HRESULT NdrPointer::unmarshal1 (NdrUnmarshalStream* pStrm)
    {
    // Now decide how to dereference this pointer. If its a [ref]
    // pointer then, by definition, it must point to something (its a
    // pass-by-reference)...

    if (m_refptr)
        {
        // Its a [ref] pointer, so if we are unmarshaling in the stub, 
        // then we need to mark it with some arbitrary non-NULL
        // value. If its in the proxy-unmshl phase, it will have a
        // real (non-NULL) value, so we can leave it as-is...
        if (pStrm->phaseGet () == NdrPhase::STUB_UNMSHL)
            *m_pPtr = (void*) 0xDEADBEEF;

        S_DEBUG (LOG_DCOM, "U1:[ref]ptr:referent=" << (*m_pPtr));
        }
    else
        {
        // Its a [unique] pointer, so we need to allocate memory if
        // unmarshaling return-values in the proxy, 

        pStrm->align (4);

        void* referent;
        HRESULT hr = pStrm->extract (sizeof (long), &referent, true);
        if (FAILED (hr))
            return hr;

        // Save referent-value (although its not a true pointer in this
        // context, yet) in the pointer variable...
        *m_pPtr = referent;

        S_DEBUG (LOG_DCOM, "U1:[unique]ptr:referent=" << referent);
        }

    return S_OK;
    }

HRESULT NdrPointer::unmarshal2 (NdrUnmarshalStream* pStrm)
    {
    // On entry, the referent-value is either NULL, or some
    // meaningless, but non-NULL, value. If its NULL, there is no
    // pointee to unmarshal...
    void* referent = *m_pPtr;
    if (referent == 0)
        return S_OK;
    
    // Non-NULL referent, so we can unmarshal the pointee. First, we
    // ask the pointee for its size, this forces it to read the
    // unmarshaling stream if its a conformant type...
    
    size_t objSize = m_pointeeType->size (pStrm);

    // If its a [unique] pointer, we may need to allocate some memory
    // for the pointer to point to. However, if we're in the
    // stub-unmarshaling phase we can borrow the stream memory. Either
    // way, we need to get the referent pointing to some real memory
    
    if (pStrm->phaseGet () == NdrPhase::STUB_UNMSHL)
        {
            //referent = pStrm->curr ();
        referent = pStrm->stubAlloc (objSize);
        if (m_refptr)
            {S_DEBUG (LOG_DCOM, "U2:[ref]ptr:value=" << referent
                      << " size=" << objSize);}
        else
            {S_DEBUG (LOG_DCOM, "U2:[unique]ptr:value=" << referent
                      << " size=" << objSize);}
        }
    else
        {
        // If its a [ref] ptr, then it must (since we're in the proxy
        // unmarshal phase) be pointing at something real, so we
        // simply dereference it to get to the pointee. If its a
        // [unique] pointer, in the proxy-unmarshal phase, we need to
        // allocate memory to hold the pointee... 
        if (m_refptr)
            {
            S_DEBUG (LOG_DCOM, "U2:[ref]ptr:value=" << referent
                     << " size=" << objSize);
            }
        else
            {

            referent = CoTaskMemAlloc (objSize);
            if (! referent)
                return E_OUTOFMEMORY;
            S_DEBUG (LOG_DCOM, "U2:[unique]ptr:value=" << referent
                     << " size=" << objSize);
            }
        }
    
    // Set the value of the pointer-variable...
    *m_pPtr = referent;

    // Now unmarshal the pointee, at last...
    m_pointeeType->bind (referent);
    return m_pointeeType->unmarshal (pStrm);
    }

NDRTYPES::NDRTYPES (int hint)
    {
    m_pFactory = new NdrTypeFactory (hint);
    }

NDRTYPES::~NDRTYPES ()
    {
    if (m_pFactory)
        delete m_pFactory;
    }

NdrTypeDesc NDRTYPES::byte_t ()
    {
    COM_ASSERT(m_pFactory);
    NdrSimple<byte>* td = new (m_pFactory) NdrSimple<byte> (*this);
    return td;
    }

NdrTypeDesc NDRTYPES::short_t ()
    {
    COM_ASSERT(m_pFactory);
    NdrSimple<short> *td = new (m_pFactory) NdrSimple<short> (*this);
    return td;
    }


NdrTypeDesc NDRTYPES::long_t ()
    {
    COM_ASSERT(m_pFactory);
    NdrSimple<long> *td = new (m_pFactory) NdrSimple<long> (*this);
    return td;
    }


NdrTypeDesc NDRTYPES::enum_t ()
    {
    COM_ASSERT(m_pFactory);
    NdrEnum *td = new (m_pFactory) NdrEnum (*this);
    td->init (false);
    return td;
    }


NdrTypeDesc NDRTYPES::v1enum_t ()
    {
    COM_ASSERT(m_pFactory);
    NdrEnum *td = new (m_pFactory) NdrEnum (*this);
    td->init (true);
    return td;
    }


NdrTypeDesc NDRTYPES::hyper_t ()
    {
    COM_ASSERT(m_pFactory);
    NdrSimple<hyper> *td = new (m_pFactory) NdrSimple<hyper> (*this);
    return td;
    }


NdrTypeDesc NDRTYPES::float_t ()
    {
    COM_ASSERT(m_pFactory);
    NdrSimple<float> *td = new (m_pFactory) NdrSimple<float> (*this);
    return td;
    }


NdrTypeDesc NDRTYPES::double_t ()
    {
    COM_ASSERT(m_pFactory);
    NdrSimple<double> *td = new (m_pFactory) NdrSimple<double> (*this);
    return td;
    }

NdrTypeDesc NDRTYPES::bstr_t ()
    {
    COM_ASSERT(m_pFactory);
    NdrBSTR *td = new (m_pFactory) NdrBSTR (*this);
    return td;
    }

NdrTypeDesc NDRTYPES::struct_t (int nelems, const NdrMemberInfo m[], int nSizeIs)
    {
    COM_ASSERT(m_pFactory);
    NdrStruct *td = new (m_pFactory) NdrStruct (*this);
    td->init (nelems, m, nSizeIs);
    return td;
    }


NdrTypeDesc NDRTYPES::cstruct_t (int nelems, const NdrMemberInfo m[], int nSizeIs)
    {
    COM_ASSERT(m_pFactory);
    NdrConfStruct *td = new (m_pFactory) NdrConfStruct (*this);
    td->init (nelems, m, nSizeIs);
    return td;
    }

NdrTypeDesc NDRTYPES::array_t (const NdrTypeDesc& eltype, size_t elsz, size_t nelems)
    {
    COM_ASSERT(m_pFactory);
    NdrArray *td = new (m_pFactory) NdrArray (*this);
    td->init (eltype, elsz, nelems, 0, nelems);
    return td;
    }

NdrTypeDesc NDRTYPES::carray_t (const NdrTypeDesc& eltype, size_t elsz, size_t nelems)
    {
    COM_ASSERT(m_pFactory);
    NdrConfArray *td = new (m_pFactory) NdrConfArray (*this);
    td->init (eltype, elsz, nelems, 0, nelems);
    return td;
    }

NdrTypeDesc NDRTYPES::cvarray_t (const NdrTypeDesc& eltype, size_t elsz, size_t nelems, size_t nmax)
    {
    COM_ASSERT(m_pFactory);
    NdrConfVarArray *td = new (m_pFactory) NdrConfVarArray (*this);
    td->NdrArray::init (eltype, elsz, nmax, 0, nelems);
    return td;
    }

NdrTypeDesc NDRTYPES::interfaceptr_t (const IID& riid)
    {
    COM_ASSERT(m_pFactory);
    NdrInterface *td = new (m_pFactory) NdrInterface (*this);
    td->init (riid);
    return td;
    }

NdrTypeDesc NDRTYPES::variant_t ()
    {
    COM_ASSERT(m_pFactory);
    NdrVariant *td = new (m_pFactory) NdrVariant (*this);
    return td;
    }

NdrTypeDesc NDRTYPES::wstring_t ()
    {
    COM_ASSERT(m_pFactory);
    NdrWString *td = new (m_pFactory) NdrWString (*this);
    return td;
    }

NdrTypeDesc NDRTYPES::cstring_t ()
    {
    COM_ASSERT(m_pFactory);
    NdrCString *td = new (m_pFactory) NdrCString (*this);
    return td;
    }

NdrTypeDesc NDRTYPES::pointer_t (const NdrTypeDesc& pt)
    {
    COM_ASSERT(m_pFactory);
    NdrPointer *td = new (m_pFactory) NdrPointer (*this, false);
    td->init (pt);
    return td;
    }

NdrTypeDesc NDRTYPES::refptr_t (const NdrTypeDesc& pt)
    {
    COM_ASSERT(m_pFactory);
    NdrPointer *td = new (m_pFactory) NdrPointer (*this, true);
    td->init (pt);
    return td;
    }

//////////////////////////////////////////////////////////////////////////
//
// widlMarshal -- simplified WIDL runtime marshaling routine, takes
// the stream, the address of the top-level variable to be marshaled,
// and the type-descriptor, and binds the descriptor to the variable
// before invoking its marshal() method...
//

HRESULT widlMarshal (void const* pv, NdrMarshalStream* pms, const NdrTypeDesc& t)
    {
    NdrTypeDesc& td = const_cast<NdrTypeDesc&> (t);
    td->bind (const_cast<void*> (pv));
    HRESULT hr = td->marshal1 (pms);
    if (FAILED (hr))
        return hr;
    return td->marshal2 (pms);
    }

HRESULT widlUnmarshal (void* pv, NdrUnmarshalStream* pus, const NdrTypeDesc& t)
    {
    NdrTypeDesc& td = const_cast<NdrTypeDesc&> (t);
    td->bind (pv);
    HRESULT hr = td->unmarshal1 (pus);
    if (FAILED (hr))
        return hr;
    return td->unmarshal2 (pus);
    }

//////////////////////////////////////////////////////////////////////////
//
// VARIANT marshaling support. The VARIANT structure, in true MS
// fashion, is wire-marshaled differently from its in-memory
// representation. To see the wire-representation, look at OAIDL.IDL,
// which contains the full details. Basically, the start of the
// wireVARIANT structure is as follows:-
//
//    DWORD  clSize;          /* wire buffer length in units of hyper */
//    DWORD  rpcReserved;     /* for future use */
//    USHORT vt;
//    USHORT wReserved1;
//    USHORT wReserved2;
//    USHORT wReserved3;
//
// ...followed by the usual union, with 'vt' as the discriminator,
// and a slightly restricted set of types within the union. In the
// first release, we will only cope with simple VARIANTs, and none of
// the pointer or BYREF vartypes. This means effectively the intrinsic
// types plus VT_UNKNOWN and VT_BSTR.
//

HRESULT NdrVariant::marshal1 (NdrMarshalStream* pStrm)
    {
    char user [] = "User";
    
    // Start with the header ("User" + padding)...
    pStrm->align (4);
    return pStrm->insert (4, user, false);
    }

HRESULT NdrVariant::marshal2 (NdrMarshalStream* pStrm)
    {
    // First, make sure its a VT we can cope with...
    switch (m_pVariant->vt & VT_TYPEMASK)
        {
        case VT_EMPTY:
        case VT_NULL:
        case VT_UI1:
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
            break;

        default:
            S_ERR (LOG_DCOM, "Unsupported VARTYPE:" << hex << m_pVariant->vt << endl);
            return DISP_E_BADVARTYPE;
        }

    // The total length of required buffer space must be calculated,
    // in units of hyper. The size of the header is constant, and its
    // only the union part that is variable. Without counting [unique]
    // pointers, the max extra size is sizeof(double) (i.e. 8 bytes)
    // except for the interface-ptr and BSTR cases. To simplify this
    // process, we create a 2nd marshaling stream and marshal the
    // entire structure into there first, (including the leading
    // DWORD, so we can get alignment right). We then use its size to
    // calculate the rounded-up packet size, and copy the whole lot
    // into the real stream...

    NdrMarshalStream s2 (pStrm->phaseGet (), pStrm-> drep ());
    
    DWORD rpcReserved=0;
    USHORT wReserved=0;

    // Stuff a dummy clSize value in, to allow for correct alignment
    // within the VARIANT packet...
    ULONG clSizeDummy=0;
    s2.insert (sizeof(ULONG), &clSizeDummy, true);
    
    // Now the post-clSize contents...
    s2.insert (sizeof(rpcReserved), &rpcReserved, true);
    s2.insert (sizeof(USHORT), &m_pVariant->vt, true);
    s2.insert (sizeof(USHORT), &wReserved, true);
    s2.insert (sizeof(USHORT), &wReserved, true);
    HRESULT hr = s2.insert (sizeof(USHORT), &wReserved, true);
    if (FAILED (hr))
        return hr;

    // Now the union part -- first the discriminator, then the
    // body. We don't marshal the discriminator (as there's no body)
    // if its VT_EMPTY or VT_NULL, nice one MS!
    
    // Marshal discriminator
    ULONG discrim;

    if (V_ISARRAY(m_pVariant))
        {
        discrim = VT_ARRAY;
        }
    else
        {
        discrim = m_pVariant->vt & VT_TYPEMASK;
        }

    s2.insert (sizeof (ULONG), &discrim, true);

    NDRTYPES ndrtypes;

    // Marshal union-body?
    if (V_ISARRAY(m_pVariant))
        {
        NdrSafearray ndrSafearray (ndrtypes);

        ndrSafearray.bind (m_pVariant);
        hr = ndrSafearray.marshal (&s2);
        }
    else
        {
        // This is a  simple type.
        switch (m_pVariant->vt & VT_TYPEMASK)
            {
            case VT_EMPTY:
            case VT_NULL:
                break;
            
            case VT_UI1:
                hr = s2.insert (sizeof (byte), &V_UI1(m_pVariant), false);
		s2.addEndPadding (8);
                break;
            
            case VT_I2:
                hr = s2.insert (sizeof (short), &V_I2(m_pVariant), true);
		s2.addEndPadding (8);
                break;
            
            case VT_I4:
                hr = s2.insert (sizeof (long), &V_I4(m_pVariant), true);
                break;
            
            case VT_R4:
                hr = s2.insert (sizeof (float), &V_R4(m_pVariant), true);
                break;
            
            case VT_R8:
                hr = marshalDouble (&s2, &V_R8(m_pVariant));
                break;
            
            case VT_CY:
                s2.align (sizeof (CY));
                hr = s2.insert (sizeof (CY), &V_CY(m_pVariant) , true);
                break;
            
            case VT_DATE:
                s2.align (sizeof (DATE));
                hr = s2.insert (sizeof (DATE), &V_DATE(m_pVariant), true);
                break;
        
            case VT_ERROR:
                hr = s2.insert (sizeof (SCODE), &V_ERROR(m_pVariant), true);
                break;
        
            case VT_BOOL:
                hr = s2.insert (sizeof (VARIANT_BOOL), &V_BOOL(m_pVariant), true);
		s2.addEndPadding (8);
                break;
        
            case VT_BSTR:
                {
                // We marshal the BSTR pointer referent, then the value itself...
                s2.insert (sizeof(ULONG), &V_BSTR(m_pVariant), true);
                if (V_BSTR(m_pVariant))
                    {
                    NdrBSTR ndrBSTR (ndrtypes);

                    ndrBSTR.bind (&V_BSTR(m_pVariant));
                    hr = ndrBSTR.marshal2 (&s2);
                    }
		s2.addEndPadding (8);
                }
                break;
        
            case VT_UNKNOWN:
                {
                // Need to add the length of the marshaled interface pointer
                // packet...

                NdrInterface ndrItf (ndrtypes);

                ndrItf.init (IID_IUnknown);
                ndrItf.bind (&V_UNKNOWN(m_pVariant));

                hr = ndrItf.marshal (&s2);
                }
                break;

            default:
                hr = DISP_E_BADVARTYPE;
            }
        }

    // Check marshaling results...
    if (FAILED (hr))
        return hr;
    
    // Calculate the size including the leading 'clSize' word, which
    // will be a multiple of sizeof(hyper) since we aligned the stream
    // already...
    ULONG clSize = (s2.size () + sizeof(hyper) - 1) / sizeof(hyper);

    // Now insert the actual clSize value into the main stream...
    pStrm->align (sizeof (hyper));
    pStrm->insert (sizeof (ULONG), &clSize, true);

     // Transfer any padding that have been added.
    pStrm->addEndPadding (s2.getEndPadding ());

    // Now copy s2 (except for its first 4 bytes) to the main stream...
    return pStrm->insert (s2.size () - sizeof(ULONG),
                          s2.begin () + sizeof(ULONG),
                          false);
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrVariant::size -- determine the actual wire-size, and return the
// required memory size (which is sizeof(VARIANT) since we don't
// support indirected types yet). This advances the stream position
// over all the run-in data, leaving only the actual data in the
// stream. Thus, when it gets unmarshaled, it can be over-written in
// place, like all other data types...
//

size_t NdrVariant::size (NdrUnmarshalStream* pStrm)
    {
    S_DEBUG (LOG_DCOM, "SZ:variant");
    return sizeof (VARIANT);
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrVariant::unmarshal1 -- unmarshal the "User" string, and check it
// is correct...
//

HRESULT NdrVariant::unmarshal1 (NdrUnmarshalStream* pStrm)
    {
    HRESULT hr = pStrm->align (4);
    if (FAILED (hr))
        return hr;

    char user[5];

    // "User" string
    hr = pStrm->extract (4, user, false);
    user[4]=0;

    S_DEBUG (LOG_DCOM, "U1:variant");

    if (strcmp (user, "User") != 0)
        return E_UNEXPECTED;
    
    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrVariant::unmarshal2 -- complete the unmarshaling of the union.
//

HRESULT NdrVariant::unmarshal2 (NdrUnmarshalStream* pStrm)
    {
    // Align properly
    HRESULT hr = pStrm->align (sizeof (hyper));
    if (FAILED (hr))
        return hr;

    // clSize
    ULONG clSize;
    pStrm->extract (sizeof (clSize), &clSize, true);

    DWORD tmp;

    // rpcReserved
    hr = pStrm->extract (sizeof(DWORD), &tmp, true);

    // VT and reserved fields
    pStrm->extract (sizeof(USHORT), &m_pVariant->vt, true);
    pStrm->extract (sizeof(USHORT), &m_pVariant->wReserved1, true);
    pStrm->extract (sizeof(USHORT), &m_pVariant->wReserved2, true);
    hr = pStrm->extract (sizeof(USHORT), &m_pVariant->wReserved3, true);
    if (FAILED (hr))
        return hr;

    // Union discriminator
    ULONG discrim;
    pStrm->extract (sizeof(ULONG), &discrim, true);
    
    S_DEBUG (LOG_DCOM, "U2:variant @ " << m_pVariant << " vt=" << m_pVariant->vt);

    NDRTYPES    ndrtypes;

    if (discrim & VT_ARRAY)
        {
        NdrSafearray ndrSafearray (ndrtypes);

        ndrSafearray.bind (m_pVariant);
        hr = ndrSafearray.unmarshal (pStrm);
        }
    else
        {
        COM_ASSERT (discrim == m_pVariant->vt);
    
        // Now the union part...
        switch (m_pVariant->vt & VT_TYPEMASK)
            {
            case VT_EMPTY:
            case VT_NULL:
                break;
        
            case VT_UI1:
                hr = pStrm->extract (sizeof (byte), &V_UI1(m_pVariant), false);
                break;
        
            case VT_I2:
                hr = pStrm->extract (sizeof (short), &V_I2(m_pVariant), true);
                break;
        
            case VT_I4:
                hr = pStrm->extract (sizeof (long), &V_I4(m_pVariant), true);
                break;
            
            case VT_R4:
                hr = pStrm->extract (sizeof (float), &V_R4(m_pVariant), true);
                break;
            
            case VT_R8:
                hr = unmarshalDouble (pStrm, &V_R8(m_pVariant));
                break;
            
            case VT_CY:
                pStrm->align (sizeof (CY));
                hr = pStrm->extract (sizeof (CY), &V_CY(m_pVariant), true);
                break;
            
            case VT_DATE:
                pStrm->align (sizeof (DATE));
                hr = pStrm->extract (sizeof (DATE), &V_DATE(m_pVariant), true);
                break;
            
            case VT_ERROR:
                hr = pStrm->extract (sizeof (SCODE), &V_ERROR(m_pVariant), true);
                break;
            
            case VT_BOOL:
                hr = pStrm->extract (sizeof (VARIANT_BOOL), &V_BOOL(m_pVariant), true);
                break;
            
            case VT_BSTR:
                {
                // Reverse the marshaling process - first, get the
                // pointer-referent, if its non-NULL, then unmarshal2() the
                // remainder of the BSTR, as its "User" string won't be
                // present due to it being inside a VARIANT...

                ULONG ref;
                hr = pStrm->extract (sizeof(ULONG), &ref, true);
                if (SUCCEEDED (hr) && ref)
                    {
                    NdrBSTR ndrBSTR (ndrtypes);

                    ndrBSTR.bind (&V_BSTR(m_pVariant));
                    hr = ndrBSTR.unmarshal2 (pStrm);
                    }
                }
                break;
        
            case VT_UNKNOWN:
                {
                // Need to add the length of the marshaled interface pointer
                // packet...

                NdrInterface ndrItf (ndrtypes);

                ndrItf.init (IID_IUnknown);
                ndrItf.bind (&V_UNKNOWN(m_pVariant));

                hr = ndrItf.unmarshal (pStrm);
                }
                break;

            default:
                S_ERR (LOG_DCOM, "Unsupported VARTYPE: 0x" << hex << m_pVariant->vt << endl);
                hr = DISP_E_BADVARTYPE;
            }
        }

    return hr;
    }

//////////////////////////////////////////////////////////////////////////////
//
// NdrSafearray::NdrSafearray - Constructor for NdrSafearray class.
//

NdrSafearray::NdrSafearray (NDRTYPES& n) : NdrType (n), 
                                           m_pVariant (0), 
                                           m_phase (PHASE1), 
                                           m_pTag (0)
    {
    }

//////////////////////////////////////////////////////////////////////////////
//
// NdrSafearray::NdrSafearray - Destructor for NdrSafearray class.
//

NdrSafearray::~NdrSafearray ()
    {
    }

//////////////////////////////////////////////////////////////////////////////
//
// NdrSafearray::bind - bind containing variant to NdrSafearray.
//

void NdrSafearray :: bind (void * pv) 
    { 
    m_pVariant = reinterpret_cast<VARIANT *>(pv);
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrSafearray::marshal1 -- marshal the SAFEARRAY header
//

HRESULT NdrSafearray::marshal1 (NdrMarshalStream* pStrm)
    {
    // marshall parray
    return pStrm->insert (sizeof (SAFEARRAY *), &V_ARRAY(m_pVariant), true);
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrSafearray::marshal2 -- marshal the SAFEARRAY body
//

HRESULT NdrSafearray::marshal2 (NdrMarshalStream* pStrm)
    {
    USHORT  tmpS;
    ULONG   tmpL;
    HRESULT hr = S_OK;

    // structure tag
    pStrm->insert (sizeof (SAFEARRAY *), &V_ARRAY(m_pVariant), true);

    // cDims
    tmpL = V_ARRAY(m_pVariant)->cDims;
    pStrm->insert (sizeof (ULONG), &tmpL, true);

    tmpS = V_ARRAY(m_pVariant)->cDims;
    pStrm->insert (sizeof (USHORT), &tmpS, true);

    // Features
    pStrm->insert (sizeof (USHORT), &V_ARRAY(m_pVariant)->fFeatures, true);

    // cbElements
    pStrm->insert (sizeof (ULONG), &V_ARRAY(m_pVariant)->cbElements, true);

    // cLocks
    tmpS = V_ARRAY(m_pVariant)->cLocks;
    pStrm->insert (sizeof (USHORT), &tmpS, true);

    // VT
    tmpS = V_VT(m_pVariant) & VT_TYPEMASK;
    pStrm->insert (sizeof (USHORT), &tmpS, true);

    // storage type, e.g. VT_R8 maps to VT_I8
    switch (V_VT(m_pVariant) & VT_TYPEMASK)
        {
        case VT_UI1:        tmpL = VT_I1; break;
        case VT_I2:         tmpL = VT_I2; break;
        case VT_I4:         tmpL = VT_I4; break;
        case VT_R4:         tmpL = VT_I4; break;
        case VT_R8:         tmpL = VT_I8; break;
        case VT_CY:         tmpL = VT_I8; break;
        case VT_DATE:       tmpL = VT_I8; break;
        case VT_BSTR:       tmpL = VT_BSTR; break;
        case VT_ERROR:      tmpL = VT_I4; break;
        case VT_BOOL:       tmpL = VT_I2; break;
        case VT_UNKNOWN:    tmpL = VT_UNKNOWN; break;
        default:
            COM_ASSERT (0);
            break;
        }

    pStrm->insert (sizeof (ULONG), &tmpL, true);

    // total count of elements
    pStrm->insert (sizeof (ULONG), &SA2MSA(V_ARRAY(m_pVariant))->dataCount, true);

    // rgsabound tag
    tmpL = reinterpret_cast<ULONG>
            (reinterpret_cast<BYTE *>(V_ARRAY(m_pVariant)) 
                    + sizeof (SAFEARRAY) - 1);
    pStrm->insert (sizeof (ULONG), &tmpL, true);

    ULONG index;

    // rgsabound

    for (index = 0; index < V_ARRAY(m_pVariant)->cDims; index++)
        {
        pStrm->insert (sizeof (ULONG), 
                   &V_ARRAY(m_pVariant)->rgsabound [index].cElements, true);
        hr = pStrm->insert (sizeof (LONG), 
                   &V_ARRAY(m_pVariant)->rgsabound [index].lLbound, true);
        }

    if (FAILED (hr))
        {
        return hr;
        }

    // total count of elements (again!)
    pStrm->insert (sizeof (ULONG), &SA2MSA(V_ARRAY(m_pVariant))->dataCount, true);

    // fill in data
    long * ix = new long [V_ARRAY(m_pVariant)->cDims];
    if (ix == NULL)
        {
        return E_OUTOFMEMORY;
        }

    switch (V_VT(m_pVariant) & VT_TYPEMASK)
        {
        case VT_BSTR:
        case VT_UNKNOWN:
            m_phase = PHASE1;
            hr = actionArray (V_ARRAY(m_pVariant)->cDims - 1, 
                              pStrm, 
                              NULL, 
                              ix);
            if (SUCCEEDED (hr))
                {
                m_phase = PHASE2;
                hr = actionArray (V_ARRAY(m_pVariant)->cDims - 1, 
                                  pStrm, 
                                  NULL, 
                                  ix);
                }
            break;

        default:
            hr = actionArray (V_ARRAY(m_pVariant)->cDims - 1, 
                              pStrm, 
                              NULL, 
                              ix);
            break;
        }
    delete [] ix;

    // Extra padding and alignment 
    pStrm->addEndPadding (8);
    pStrm->align(8);

    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrSafearray::unmarshal1 -- unmarshal the SAFEARRAY header
//


HRESULT NdrSafearray::unmarshal1 (NdrUnmarshalStream* pStrm)
    {
    void *              ptr;

    // Safearray marshalled as a pointer to a structure so get rid of
    // ptr.
    return pStrm->extract (sizeof (void *), &ptr, true);
    }

//////////////////////////////////////////////////////////////////////////
//
// NdrSafearray::unmarshal2 -- complete the unmarshaling of the SAFEARRAY.
//

HRESULT NdrSafearray::unmarshal2 (NdrUnmarshalStream* pStrm)
    {
    HRESULT             hr = S_OK;
    void *              ptr;
    USHORT              dims;
    USHORT              features;
    ULONG               cbElements;
    USHORT              locks;
    ULONG               cElements;
    SAFEARRAYBOUND *    sab;
    USHORT              index;

    ULONG               tmpL;
    USHORT              tmpS;

    hr = pStrm->extract (sizeof (void *), &ptr, true);

    // cDims
    hr = pStrm->extract (sizeof (USHORT), &dims, true);
    hr = pStrm->extract (sizeof (ULONG), &tmpL, true);

    // cFeatures
    hr = pStrm->extract (sizeof (USHORT), &features, true);

    // cbElements
    hr = pStrm->extract (sizeof (ULONG), &cbElements, true);

    // cLocks
    hr = pStrm->extract (sizeof (USHORT), &locks, true);

    // cbElements again as short and long
    hr = pStrm->extract (sizeof (USHORT), &tmpS, true);
    hr = pStrm->extract (sizeof (ULONG), &tmpL, true);

    // total count of elements
    hr = pStrm->extract (sizeof (ULONG), &cElements, true);

    // now follows the rgsa structure
    hr = pStrm->extract (sizeof (void *), &ptr, true);
    if (FAILED (hr)) return hr;

    sab = new SAFEARRAYBOUND [dims];

    if (sab == NULL)
        {
        return E_OUTOFMEMORY;
        }

    for (index = 0; index < dims; index++)
        {
        hr = pStrm->extract (sizeof (ULONG), &(sab [index].cElements), true);
        if (FAILED (hr))
            {
            delete sab;
            return hr;
            }
        hr = pStrm->extract (sizeof (LONG), &(sab [index].lLbound), true);
        if (FAILED (hr))
            {
            delete sab;
            return hr;
            }
        }

    // we now have enough info to contruct the safearray.
    V_ARRAY(m_pVariant) = SafeArrayCreate (V_VT(m_pVariant) & VT_TYPEMASK,
                                           dims,
                                           sab);

    // a copy of the SAFEARRAYBOUNDS has been made in SAFEARRAY
    delete [] sab;

    if (V_ARRAY(m_pVariant) == NULL)
        {
        // couldn't create safearray
        return E_OUTOFMEMORY;
        }

    hr = pStrm->extract (sizeof (ULONG), &cbElements, true);

    long *      ix = new long [V_ARRAY(m_pVariant)->cDims];

    if (ix == NULL)
        {
        SafeArrayDestroy (V_ARRAY(m_pVariant));
        return E_OUTOFMEMORY;
        }

    switch (V_VT(m_pVariant) & VT_TYPEMASK)
        {
        case VT_BSTR:
        case VT_UNKNOWN:
            {
            // We use a second SAFEARRAY to store pointer tags on
            // the first pass so they can be retrieved on the second 
            // pass to check for NULL pointers.
            m_pTag = SafeArrayCreate (VT_I4, 
                                      V_ARRAY(m_pVariant)->cDims, 
                                      V_ARRAY(m_pVariant)->rgsabound);

            if (m_pTag == NULL)
                {
                delete [] ix;
                SafeArrayDestroy (V_ARRAY(m_pVariant));
                return E_OUTOFMEMORY;
                }

            m_phase = PHASE1;

            hr = actionArray (V_ARRAY(m_pVariant)->cDims - 1, NULL, pStrm, ix);

            if (SUCCEEDED (hr))
                {
                m_phase = PHASE2;
                hr = actionArray (V_ARRAY(m_pVariant)->cDims - 1, 
                                  NULL, 
                                  pStrm, 
                                  ix);
                }
            // get rid of tag SAFEARRAY.
            SafeArrayDestroy (m_pTag);
            }
            break;

        default:
            hr = actionArray (V_ARRAY(m_pVariant)->cDims - 1, NULL, pStrm, ix);
            break;
        }
    delete [] ix;
    
    return hr;
    }

/////////////////////////////////////////////////////////////////////////////
//
// NdrSafearray::unmarshalBody - unmarshal a single instance of 

HRESULT NdrSafearray::unmarshalBody (NdrUnmarshalStream * pStrm, long * ix)
    {
    HRESULT     hr = S_OK;

    switch (V_VT(m_pVariant) & VT_TYPEMASK)
        {
        case VT_R8:
        case VT_CY:
        case VT_DATE:
            {
            double  value;
            hr = unmarshalDouble (pStrm, &value);

            if (SUCCEEDED (hr))
                {
                hr = SafeArrayPutElement (V_ARRAY(m_pVariant), ix, &value);
                }
            }
            break;

        case VT_ERROR:
        case VT_BOOL:
        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_UI1:
            {
            BYTE *  data = new BYTE [V_ARRAY(m_pVariant)->cbElements];

            if (data == NULL)
                {
                return E_OUTOFMEMORY;
                }

            hr = pStrm->extract (V_ARRAY(m_pVariant)->cbElements, data, true);
            if (SUCCEEDED(hr))
                {
                hr = SafeArrayPutElement (V_ARRAY(m_pVariant), ix, data);
                }

            delete [] data;
            }
            break;
        
        case VT_UNKNOWN:
        case VT_BSTR:
            switch (m_phase)
                {
                case PHASE1:
                    {
                    // unmarshall the tag
                    ULONG ref;
                    hr = pStrm->extract (sizeof(ULONG), &ref, true);
                    if (SUCCEEDED(hr))
                        {
                        // Store ref so it can be used in phase 2.
                        hr = SafeArrayPutElement (m_pTag, ix, &ref);
                        }
                    }
                    break;

                case PHASE2:
                    {
                    ULONG       ref;

                    hr = SafeArrayGetElement (m_pTag, ix, &ref);

                    // We only need to unmarshall data for pointers 
                    // that are non-NULL.
                    if (SUCCEEDED (hr) && ref)
                        {
                        NDRTYPES    ndrtypes;

                        switch (V_VT(m_pVariant) & VT_TYPEMASK)
                            {
                            case VT_BSTR:
                                {
                                BSTR        bstr;
                                NdrBSTR     ndrBSTR (ndrtypes);

                                ndrBSTR.bind (&bstr);
                                hr = ndrBSTR.unmarshal2 (pStrm);
                                if (SUCCEEDED (hr))
                                    {
                                    hr = SafeArrayPutElement (V_ARRAY(m_pVariant),
                                                              ix, 
                                                              bstr);
                                    }
                                }
                                break;

                            case VT_UNKNOWN:
                                {
                                NdrInterface ndrItf (ndrtypes);
                                IUnknown *   pItf;

                                ndrItf.init (IID_IUnknown);
                                ndrItf.bind (&pItf);

                                hr = ndrItf.unmarshal2 (pStrm);
                                if (SUCCEEDED (hr))
                                    {
                                    hr = SafeArrayPutElement (V_ARRAY(m_pVariant),
                                                              ix, 
                                                              pItf);
                                    }
                                pItf->Release ();
                                }
                                break;
                            }
                        }
                    }
                    break;
                }
            break;

        default:
            return E_INVALIDARG;
        }

    return hr;
    }

HRESULT NdrSafearray::marshalBody (NdrMarshalStream * pStrm, long * ix)
    {
    HRESULT     hr = S_OK;

    switch (V_VT(m_pVariant) & VT_TYPEMASK)
        {
        case VT_R8:
        case VT_CY:
        case VT_DATE:
            {
            double                  value;
            hr = SafeArrayGetElement (V_ARRAY (m_pVariant), ix, &value);

            if (SUCCEEDED (hr))
                {
                hr = marshalDouble (pStrm, &value);
                }
            }
            break;

        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_ERROR:
        case VT_BOOL:
        case VT_UI1:
            {
            BYTE *  data = new BYTE [V_ARRAY(m_pVariant)->cbElements];

            if (data == NULL)
                {
                return E_OUTOFMEMORY;
                }

            hr = SafeArrayGetElement (V_ARRAY (m_pVariant), ix, data);

            if (FAILED (hr))
                {
                delete data;
                return hr;
                }

            hr = pStrm->insert (V_ARRAY(m_pVariant)->cbElements, data, true);

            delete [] data;
            }
            break;
        
        case VT_UNKNOWN:
        case VT_BSTR:
            {
            switch (m_phase)
                {
                case PHASE1:
                    {
                    ULONG       ref;
                    VARTYPE     oldVt = SA2MSA(V_ARRAY(m_pVariant))->vt;

                    // get the tag as a pointer by changing the type of the 
                    // SAFEARRAY to I4 (same size as a pointer) and thus
                    // prevent a copy of the BSTR being returned.
                    SA2MSA(V_ARRAY(m_pVariant))->vt = VT_I4;
                    hr = SafeArrayGetElement(V_ARRAY(m_pVariant), ix, &ref);
                    SA2MSA(V_ARRAY(m_pVariant))->vt = oldVt;

                    if (FAILED(hr))
                        {
                        return hr;
                        }

                    // marshall the tag
                    hr = pStrm->insert (sizeof(ULONG), &ref, true);
                    }
                    break;

                case PHASE2:
                    {
                    ULONG       ref;
                    VARTYPE     oldVt = SA2MSA(V_ARRAY(m_pVariant))->vt;

                    // get the tag as a pointer by changing the type of the 
                    // SAFEARRAY to I4 (same size as a pointer) and thus
                    // prevent a copy of the BSTR being returned.
                    SA2MSA(V_ARRAY(m_pVariant))->vt = VT_I4;
                    hr = SafeArrayGetElement(V_ARRAY(m_pVariant), ix, &ref);
                    SA2MSA(V_ARRAY(m_pVariant))->vt = oldVt;

                    if (FAILED(hr))
                        {
                        return hr;
                        }

                    // only marshal data for non-NULL types.
                    if (ref)
                        {
                        NDRTYPES ndrtypes;

                        switch (V_VT(m_pVariant) & VT_TYPEMASK)
                            {
                            case VT_BSTR:
                                {
                                BSTR    bstr;

                                hr = SafeArrayGetElement (V_ARRAY(m_pVariant),
                                                          ix,
                                                          &bstr);

                                if (FAILED(hr))
                                    {
                                    return hr;
                                    }

                                NdrBSTR ndrBSTR (ndrtypes);

                                ndrBSTR.bind (&bstr);
                                hr = ndrBSTR.marshal2 (pStrm);
                                }
                                break;

                            case VT_UNKNOWN:
                                {
                                IUnknown * pItf;

                                hr = SafeArrayGetElement (V_ARRAY(m_pVariant),
                                                          ix,
                                                          &pItf);

                                if (FAILED(hr))
                                    {
                                    return hr;
                                    }

                                NdrInterface ndrInterface (ndrtypes);

                                ndrInterface.bind (&pItf);
                                hr = ndrInterface.marshal2 (pStrm);
                                }
                                break;

                            default:
                                COM_ASSERT(0);
                                break;
                            }
                        }
                    }
                    break;

                default:
                    COM_ASSERT(0);
                }
            }
            break;

        default:
            return E_INVALIDARG;
        }

    return hr;
    }

HRESULT NdrSafearray::actionArray 
    (
    int                     dim, 
    NdrMarshalStream *      pMaStrm,
    NdrUnmarshalStream *    pUnStrm,
    long *                  ix
    )
    {
    HRESULT hr = S_OK;
    long index;

    for (index = 0; 
            index < (long)V_ARRAY(m_pVariant)->rgsabound [dim].cElements; 
                index++)
        {
        ix [dim] = index + V_ARRAY(m_pVariant)->rgsabound [dim].lLbound;
        if (dim == 0)
            {
            if (pMaStrm)
                {
                // we have been given a marshaling pointer so me must be 
                // marshling the data.
                hr = marshalBody (pMaStrm, ix);
                }
            else
                {
                hr = unmarshalBody (pUnStrm, ix);
                }
            if (FAILED(hr))
                {
                return hr;
                }
            }
        else
            {
            hr = actionArray (dim - 1, pMaStrm, pUnStrm, ix);
            if (FAILED (hr))
                {
                return hr;
                }
            }
        }
    return hr;
    }




